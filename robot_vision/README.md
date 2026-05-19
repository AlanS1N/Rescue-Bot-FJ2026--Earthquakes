# Rescue Vision for Jetson

Quick guide to deploy the classifier to the robot's repository.

## What to move to the robot's repository

Move this entire folder:

```text
robot_vision/
|-- infer_camera.py
|-- smoke_test.py
|-- requirements.txt
|-- README.md
`-- models/
    `-- bestfinal.pt
```

This is what's needed to run on the Jetson Orin. There's no need to move `dataset.zip`, `.venv`, `runs/`, `Random`, or the image folders. These are used for training or data preparation, but not for inference on the robot.

## What it does

The model is a YOLO classifier from Ultralytics. It takes a full image/frame and outputs one of these classes:

```text
damage
no_damage
```

Important: this does not draw boxes or locate exactly where the damage is. It only indicates whether the entire frame appears to have damage or not.

## Models available in this repository

```text
models/best.pt        -> small/old model, 2.9 MB
models/best82.9.pt    -> final/backup model, 10.2 MB
models/bestfinal.pt   -> recommended model, 10.2 MB
yolo11n-cls.pt        -> base model for training, not the final trained model
```

For the robot, use:

```text
robot_vision/models/bestfinal.pt
```

## Versions of camera_test.py

```text
camera_test.py   -> only tests if the camera captures 1 frame and saves test.jpg
camera_test2.py  -> shows raw video from the camera
camera_test3.py  -> loads YOLO and shows predictions, but uses best.pt in the root
```

For the robot, use better `robot_vision/infer_camera.py`. It's the ordered version of that idea: accepts arguments, uses `models/bestfinal.pt`, and can run with or without a window.

## Install on Jetson

On the Jetson, from the robot's repository:

```bash
cd robot_vision
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

If you already have PyTorch/Ultralytics installed in the robot's environment, you don't need to create another `.venv`.

## Test that the model loads

With any image:

```bash
python3 smoke_test.py /ruta/a/imagen.jpg
```

Expected output:

```text
class=damage conf=0.932
```

or:

```text
class=no_damage conf=0.884
```

## Run with camera

Without window, recommended for integration with the robot:

```bash
python3 infer_camera.py --camera 0
```

With window for debugging:

```bash
python3 infer_camera.py --camera 0 --show
```

If the camera doesn't open, try:

```bash
python3 infer_camera.py --camera 1 --show
```

You can also pass a GStreamer pipeline if you're using a CSI camera:

```bash
python3 infer_camera.py --camera "TU_PIPELINE_GSTREAMER_AQUI" --show
```

## Output for integration

The script prints a line every half second:

```text
ok class=damage label='Dano' conf=0.932
ok class=no_damage label='Sin dano' conf=0.884
```

You can adjust the interval:

```bash
python3 infer_camera.py --print-every 0.2
```

And you can request minimum confidence:

```bash
python3 infer_camera.py --min-conf 0.70
```

If the confidence is below that value, it prints `low_conf`.

## How it was trained

The flow was:

```text
imagenes damage/no_damage
-> prepare_cls_dataset.py
-> dataset/train y dataset/val
-> yolo classify train
-> best.pt / bestfinal.pt
```

Base command used/recommended:

```bash
yolo classify train data=dataset model=yolo11n-cls.pt imgsz=224 epochs=30 batch=32 name=damage_retrain
```

After training, the important weight remains in:

```text
runs/classify/damage_retrain/weights/best.pt
```

This file is copied as:

```text
models/bestfinal.pt
```

## Simple rule

For running on the robot:

```text
modelo + infer_camera.py + requirements.txt
```

For training:

```text
imagenes + prepare_cls_dataset.py + yolo11n-cls.pt + training commands
```
