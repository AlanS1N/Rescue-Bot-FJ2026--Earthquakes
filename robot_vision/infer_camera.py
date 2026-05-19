import argparse
import os
import time
from pathlib import Path

import cv2
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
    return parser.parse_args()


def open_camera(camera: str, width: int, height: int) -> cv2.VideoCapture:
    if camera.isdigit():
        cap = cv2.VideoCapture(int(camera))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
        return cap

    return cv2.VideoCapture(camera, cv2.CAP_GSTREAMER)


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

    last_print = 0.0

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("No se pudo leer frame")
                break

            result = model.predict(frame, verbose=False)[0]
            top_id = int(result.probs.top1)
            class_name = result.names[top_id]
            confidence = float(result.probs.top1conf.item())
            label = LABELS.get(class_name, class_name)
            is_valid = confidence >= args.min_conf

            now = time.monotonic()
            if now - last_print >= args.print_every:
                status = "ok" if is_valid else "low_conf"
                print(f"{status} class={class_name} label='{label}' conf={confidence:.3f}", flush=True)
                last_print = now

            if args.show:
                color = (0, 0, 255) if class_name == "damage" else (0, 180, 0)
                text = f"{label} {confidence * 100:.1f}%"
                cv2.rectangle(frame, (12, 12), (360, 62), (0, 0, 0), -1)
                cv2.putText(
                    frame,
                    text,
                    (22, 46),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.9,
                    color,
                    2,
                    cv2.LINE_AA,
                )
                cv2.imshow("Rescue Vision", frame)
                if cv2.waitKey(1) & 0xFF == ord("q"):
                    break
    finally:
        cap.release()
        if args.show:
            cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
