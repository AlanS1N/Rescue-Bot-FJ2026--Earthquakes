import argparse
from pathlib import Path

from ultralytics import YOLO


BASE_DIR = Path(__file__).resolve().parent
DEFAULT_MODEL = BASE_DIR / "models" / "bestfinal.pt"


def main() -> None:
    parser = argparse.ArgumentParser(description="Prueba rapida de carga/prediccion del modelo.")
    parser.add_argument("image", help="Imagen de prueba")
    parser.add_argument("--model", default=str(DEFAULT_MODEL), help="Ruta al modelo .pt")
    args = parser.parse_args()

    model_path = Path(args.model)
    image_path = Path(args.image)

    if not model_path.exists():
        raise FileNotFoundError(f"No existe el modelo: {model_path}")
    if not image_path.exists():
        raise FileNotFoundError(f"No existe la imagen: {image_path}")

    model = YOLO(str(model_path))
    result = model.predict(str(image_path), verbose=False)[0]
    top_id = int(result.probs.top1)
    class_name = result.names[top_id]
    confidence = float(result.probs.top1conf.item())

    print(f"class={class_name} conf={confidence:.3f}")


if __name__ == "__main__":
    main()
