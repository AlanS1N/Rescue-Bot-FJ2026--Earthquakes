# Robot Vision - Guía rápida para la Jetson

Esto es para que pruebes primero el programa y luego lo dejes arrancar solo al encender.

## Paso 1 - Ir a la carpeta correcta

```bash
cd ~/Desktop/rescue_robot/Final_old/robot_vision
```|

## Paso 2 - Activar el entorno virtual

```bash
source .venv/bin/activate
```

Si no existe `.venv`, créalo con:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Paso 3 - Arreglar la compatibilidad de NumPy

Tu error actual dice que NumPy 2.x no funciona con Torch/Ultralytics.

Haz esto dentro del venv:

```bash
python3 -m pip install --upgrade pip
python3 -m pip install "numpy<2"
python3 -m pip install -r requirements.txt
```

Si el error persiste y todavía ve paquetes desde `/home/<usuario>/.local`, fuerza que Python NO use paquetes de usuario:

```bash
export PYTHONNOUSERSITE=1
python3 -m pip install "numpy<2"
python3 -m pip install -r requirements.txt
```

### Si sigues con problemas, recrea el venv limpio

```bash
cd ~/Desktop/rescue_robot/Final_old/robot_vision
rm -rf .venv
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install --upgrade pip
export PYTHONNOUSERSITE=1
python3 -m pip install "numpy<2"
python3 -m pip install -r requirements.txt
```

Con eso se asegura que el entorno use versiones compatibles y no mezcle paquetes de usuario.

## Paso 4 - Probar el programa manualmente

Antes de tocar el service, prueba que esto funcione:

```bash
python3 infer_camera.py --camera 0
```

Si estás delante de la Jetson y quieres ver la cámara:

```bash
SHOW_WINDOW=true python3 infer_camera.py --camera 0
```

### Qué debe pasar

- El script debe arrancar y quedarse ejecutando
- Debe leer la cámara `/dev/video0` o el índice correcto
- Debe mostrar mensajes de `ok` o `low_conf` cada cierto tiempo

Si se cierra rápido, no sigas aún con el servicio.

## Paso 4.1 - Si no abre la cámara

Si ves este error:

```text
No pude abrir la camara. Prueba --camera 1 o revisa el pipeline.
```

haz esto:

```bash
ls /dev/video*
v4l2-ctl --list-devices
```

Si aparece `/dev/video1`, prueba:

```bash
SHOW_WINDOW=true python3 infer_camera.py --camera 1
```

Si tu cámara es una cámara NVIDIA/Jetson integrada, prueba la pipeline de GStreamer:

```bash
python3 infer_camera.py --camera "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=640, height=480, framerate=30/1 ! nvvidconv ! video/x-raw, format=BGRx ! videoconvert ! video/x-raw, format=BGR ! appsink"
```

Si `v4l2-ctl` no existe, instálalo con:

```bash
sudo apt install v4l-utils
```

Solo sigue al servicio cuando puedas abrir la cámara sin error.

## Paso 5 - Configurar el service para arrancar al boot

```bash
chmod +x start_vision.sh
sudo cp robot-vision.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable robot-vision.service
sudo systemctl restart robot-vision.service
```

## Paso 6 - Verificar el servicio

```bash
sudo systemctl status robot-vision.service
sudo journalctl -u robot-vision.service -f
```

Si ves que el service sigue reiniciando y saliendo con `status=1`, entonces el programa aún no corre bien.

## Paso 7 - Probar reinicio de la Jetson

Si ya funciona manualmente y el service se mantiene activo:

```bash
sudo reboot
```

Luego, al volver a conectar por SSH:

```bash
sudo systemctl status robot-vision.service
sudo journalctl -u robot-vision.service -n 50 --no-pager
```

## Nota importante

- El service no debe usar `--show` automáticamente.
- Si quieres ver la pantalla, hazlo manualmente con `SHOW_WINDOW=true`.
- Primero asegúrate de que `python3 infer_camera.py --camera 0` funcione.
- Solo después habilita `systemctl enable robot-vision.service`.

## Diagnóstico rápido

Si algo falla, revisa esto:

```bash
ls -l models/bestfinal.pt
python3 infer_camera.py --camera 0 2>&1 | tee /tmp/infer_debug.txt
```

Eso te dirá si falta el modelo o si hay error de librerías.
