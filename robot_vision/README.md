# Robot Vision

Vision system for the rescue robot running on the Jetson. It uses a YOLO classifier to detect whether the camera frame looks like:

```text
damage
no_damage
```

It can run continuously, and it can also save a screenshot when the ESP32 sends a serial trigger:

```text
PHOTO_TRIGGER
```

That trigger currently comes from the FlySky switch handled by the ESP32 movement code.

## How It Works

The flow is:

```text
FlySky switch
-> receiver iBUS
-> ESP32 movement code
-> USB Serial message: PHOTO_TRIGGER
-> Jetson Python script
-> YOLO prediction on current frame
-> saves image + JSON data
```

The ESP32 does not run YOLO. It only sends a small serial message. The Jetson runs the camera and the model.

When the Jetson receives `PHOTO_TRIGGER`, it saves:

```text
captures/trigger-YYYYMMDD-HHMMSS.jpg
captures/trigger-YYYYMMDD-HHMMSS.json
```

The JPG is annotated with the prediction text, for example:

```text
Damage 93.2%
```

The JSON stores the data:

```json
{
  "timestamp": "20260521-193000",
  "class": "damage",
  "label": "Damage",
  "confidence": 0.932,
  "confidence_percent": 93.2,
  "status": "ok",
  "image_path": "captures/trigger-20260521-193000.jpg"
}
```

## Files Needed On The Jetson

Clone or copy this folder:

```text
robot_vision/
|-- infer_camera.py
|-- smoke_test.py
|-- requirements.txt
|-- README.md
`-- models/
    `-- bestfinal.pt
```

The recommended model is:

```text
models/bestfinal.pt
```

## Install On Jetson

From the repository:

```bash
cd robot_vision
python3 -m venv .venv --system-site-packages
source .venv/bin/activate
pip install -r requirements.txt
```

If OpenCV gives trouble on Jetson, install it with apt:

```bash
sudo apt update
sudo apt install python3-opencv
```

Then recreate the venv with system packages:

```bash
deactivate
rm -rf .venv
python3 -m venv .venv --system-site-packages
source .venv/bin/activate
pip install ultralytics pyserial
```

## Important Jetson Time Fix

If `pip install` fails with SSL errors like:

```text
SystemTimeWarning: System time is way off
certificate verify failed: certificate is not yet valid
```

fix the Jetson date/time first:

```bash
date
sudo timedatectl set-ntp true
sudo systemctl restart systemd-timesyncd
date
```

If needed, set it manually:

```bash
sudo date -s "2026-05-21 19:30:00"
```

Then run `pip install` again.

## Test The Model

With any image:

```bash
python3 smoke_test.py /path/to/image.jpg
```

Expected output:

```text
class=damage conf=0.932
```

or:

```text
class=no_damage conf=0.884
```

## Run Camera Only

Without display window:

```bash
python3 infer_camera.py --camera 0
```

With display window:

```bash
python3 infer_camera.py --camera 0 --show
```

If the camera does not open:

```bash
python3 infer_camera.py --camera 1 --show
```

For a CSI camera, pass a GStreamer pipeline:

```bash
python3 infer_camera.py --camera "YOUR_GSTREAMER_PIPELINE" --show
```

## Run With ESP32 Photo Trigger

Connect the ESP32 to the Jetson using USB:

```text
Jetson USB -> ESP32 USB
```

That cable handles both:

```text
power + serial communication
```

Find the ESP32 serial port:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

It will usually be one of these:

```text
/dev/ttyUSB0
/dev/ttyACM0
```

Run vision with trigger support:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --show
```

or:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyACM0 --show
```

When the ESP32 sends:

```text
PHOTO_TRIGGER
```

the script prints something like:

```text
photo_saved metadata=captures/trigger-20260521-193000.json class=damage conf=0.932
```

and saves the JPG + JSON in:

```text
captures/
```

## Serial Permission Fix

If the serial port exists but Python cannot open it, add the user to `dialout`:

```bash
sudo usermod -a -G dialout $USER
```

Then log out and back in, or reboot the Jetson.

## Useful Commands

Run with confidence threshold:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --min-conf 0.70 --show
```

Save captures somewhere else:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --capture-dir ~/Desktop/captures --show
```

Print predictions faster:

```bash
python3 infer_camera.py --camera 0 --print-every 0.2
```

## What The Script Prints

Normal prediction log:

```text
ok class=damage label='Damage' conf=0.932
ok class=no_damage label='No damage' conf=0.884
```

Low confidence:

```text
low_conf class=damage label='Damage' conf=0.421
```

Photo trigger:

```text
photo_saved metadata=captures/trigger-20260521-193000.json class=damage conf=0.932
```

## Quick Full Run

Most common robot command:

```bash
cd robot_vision
source .venv/bin/activate
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --show
```

Use the FlySky switch once to save one capture. To save another one, switch it off and on again.
