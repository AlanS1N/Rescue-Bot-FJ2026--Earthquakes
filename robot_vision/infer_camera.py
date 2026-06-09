import argparse
import json
import os
import time
from pathlib import Path
from typing import List, Optional

import cv2
import serial
from ultralytics import YOLO


BASE_DIR = Path(__file__).resolve().parent
DEFAULT_MODEL = BASE_DIR / "models" / "bestfinal.pt"

LABELS = {
    "damage": "Damage",
    "no_damage": "No damage",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Real-time damage/no_damage classification for Jetson/camera."
    )
    parser.add_argument("--model", default=str(DEFAULT_MODEL), help="Path to the .pt model")
    parser.add_argument("--camera", default="0", help="Camera index or GStreamer pipeline")
    parser.add_argument("--width", type=int, default=640, help="Capture width")
    parser.add_argument("--height", type=int, default=480, help="Capture height")
    parser.add_argument("--show", action="store_true", help="Display window with OpenCV")
    parser.add_argument(
        "--min-conf",
        type=float,
        default=0.0,
        help="Minimum confidence to consider a prediction valid",
    )
    parser.add_argument(
        "--print-every",
        type=float,
        default=0.5,
        help="Seconds between prediction logs",
    )
    parser.add_argument(
        "--serial-port",
        help="ESP32 serial port, for example /dev/ttyUSB0 or /dev/ttyACM0",
    )
    parser.add_argument("--baud", type=int, default=115200, help="ESP32 serial baud rate")
    parser.add_argument(
        "--capture-dir",
        default=str(BASE_DIR / "captures"),
        help="Directory where PHOTO_TRIGGER frames are saved",
    )
    return parser.parse_args()


def open_camera(camera: str, width: int, height: int) -> cv2.VideoCapture:
    if camera.isdigit():
        cap = cv2.VideoCapture(int(camera))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
        return cap

    return cv2.VideoCapture(camera, cv2.CAP_GSTREAMER)


def open_esp32_serial(port: Optional[str], baud: int) -> Optional[serial.Serial]:
    if not port:
        return None

    try:
        ser = serial.Serial(port, baudrate=baud, timeout=0)
        print(f"ESP32 serial abierto en {port} @ {baud}", flush=True)
        return ser
    except Exception as e:
        print(f"No pude abrir puerto serial {port}: {e}", flush=True)
        return None


def detect_serial_port() -> Optional[str]:
    # Busca ttyUSB* y ttyACM* y devuelve la primera que exista
    for port in sorted(Path('/dev').glob('ttyUSB*')):
        if port.exists():
            return str(port)
    for port in sorted(Path('/dev').glob('ttyACM*')):
        if port.exists():
            return str(port)
    return None


def read_serial_events(esp32: Optional[serial.Serial]) -> List[str]:
    if esp32 is None:
        return []

    events = []
    while esp32.in_waiting:
        line = esp32.readline().decode(errors="ignore").strip()
        if line:
            events.append(line)

    return events


def draw_prediction(frame, label: str, class_name: str, confidence: float):
    annotated = frame.copy()
    color = (0, 0, 255) if class_name == "damage" else (0, 180, 0)
    text = f"{label} {confidence * 100:.1f}%"
    cv2.rectangle(annotated, (12, 12), (360, 62), (0, 0, 0), -1)
    cv2.putText(
        annotated,
        text,
        (22, 46),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.9,
        color,
        2,
        cv2.LINE_AA,
    )
    return annotated


def save_trigger_data(
    frame,
    capture_dir: Path,
    class_name: str,
    label: str,
    confidence: float,
    is_valid: bool,
) -> Path:
    capture_dir.mkdir(parents=True, exist_ok=True)
    timestamp = time.strftime("%Y%m%d-%H%M%S")
    image_path = capture_dir / f"trigger-{timestamp}.jpg"
    metadata_path = capture_dir / f"trigger-{timestamp}.json"

    annotated = draw_prediction(frame, label, class_name, confidence)
    cv2.imwrite(str(image_path), annotated)

    metadata = {
        "timestamp": timestamp,
        "class": class_name,
        "label": label,
        "confidence": confidence,
        "confidence_percent": round(confidence * 100, 2),
        "status": "ok" if is_valid else "low_conf",
        "image_path": str(image_path),
    }

    metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    return metadata_path


def draw_live_overlay(frame, label: str, class_name: str, confidence: float) -> None:
    annotated = draw_prediction(frame, label, class_name, confidence)
    frame[:] = annotated


def main() -> None:
    args = parse_args()
    os.environ.setdefault("YOLO_CONFIG_DIR", str(BASE_DIR / "Ultralytics"))

    model_path = Path(args.model)
    if not model_path.exists():
        raise FileNotFoundError(f"No existe el modelo: {model_path}")

    model = YOLO(str(model_path))
    cap = open_camera(args.camera, args.width, args.height)
    if not cap.isOpened():
        raise RuntimeError("No pude abrir la camara. Prueba --camera 1 o revisa el pipeline.")

    # Intentar abrir serial si se pasó por args, si no, detectar automáticamente
    if not args.serial_port:
        detected = detect_serial_port()
        if detected:
            args.serial_port = detected
            print(f"Puerto serial detectado automáticamente: {args.serial_port}", flush=True)

    esp32 = open_esp32_serial(args.serial_port, args.baud)
    capture_dir = Path(args.capture_dir)
    last_print = 0.0

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("No se pudo leer frame")
                break

            # Si no tenemos serial abierto, intentar reconectar cada N iteraciones
            if esp32 is None:
                detected = detect_serial_port()
                if detected:
                    esp32 = open_esp32_serial(detected, args.baud)
                    if esp32 is not None:
                        print(f"ESP32 conectado en {detected}", flush=True)

            result = model.predict(frame, verbose=False)[0]
            top_id = int(result.probs.top1)
            class_name = result.names[top_id]
            confidence = float(result.probs.top1conf.item())
            label = LABELS.get(class_name, class_name)
            is_valid = confidence >= args.min_conf

            for event in read_serial_events(esp32):
                if event == "PHOTO_TRIGGER":
                    path = save_trigger_data(
                        frame,
                        capture_dir,
                        class_name,
                        label,
                        confidence,
                        is_valid,
                    )
                    print(
                        f"photo_saved metadata={path} class={class_name} conf={confidence:.3f}",
                        flush=True,
                    )

            now = time.monotonic()
            if now - last_print >= args.print_every:
                status = "ok" if is_valid else "low_conf"
                print(f"{status} class={class_name} label='{label}' conf={confidence:.3f}", flush=True)
                last_print = now

            if args.show:
                draw_live_overlay(frame, label, class_name, confidence)
                cv2.imshow("Rescue Vision", frame)
                if cv2.waitKey(1) & 0xFF == ord("q"):
                    break
    finally:
        if esp32 is not None:
            esp32.close()
        cap.release()
        if args.show:
            cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
