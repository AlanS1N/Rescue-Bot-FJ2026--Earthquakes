# Robot Vision

README de chill para correr la vision del robot en la Jetson y entender como funciona lo del screenshot.

## Que hace

Esta carpeta trae el clasificador de dano/no dano para la camara del robot.

El modelo es un YOLO classifier de Ultralytics. Ojo: no detecta cajas ni marca exactamente donde esta el dano; ve el frame completo y decide una clase:

```text
damage
no_damage
```

El script principal es:

```text
infer_camera.py
```

Ese script abre la camara, corre el modelo en vivo y, si esta conectado al ESP32 por serial, tambien escucha triggers para guardar screenshot.

## Lo del screenshot

Si, esta version ya tiene el trigger.

La idea es:

```text
ESP32 manda por serial: PHOTO_TRIGGER
Python lo lee en infer_camera.py
Python guarda la foto actual con la prediccion dibujada
Python guarda tambien un JSON con metadata
```

El texto tiene que llegar exactamente asi:

```text
PHOTO_TRIGGER
```

Cuando llega, Python guarda archivos en:

```text
robot_vision/captures/
```

Ejemplo de salida:

```text
captures/trigger-20260521-154233.jpg
captures/trigger-20260521-154233.json
```

El `.jpg` es la imagen con el overlay de la prediccion. El `.json` trae cosas como clase, confianza, status y ruta de la imagen.

Ejemplo del mensaje que imprime Python cuando guarda:

```text
photo_saved metadata=captures/trigger-20260521-154233.json class=damage conf=0.932
```

## Archivos importantes

```text
robot_vision/
|-- infer_camera.py        -> corre camara + modelo + trigger PHOTO_TRIGGER
|-- smoke_test.py          -> prueba rapida con una imagen
|-- requirements.txt       -> dependencias de Python
|-- README.md              -> este archivo
|-- models/
|   `-- bestfinal.pt       -> modelo recomendado
`-- captures/              -> aqui se guardan screenshots cuando hay trigger
```

Para correr en el robot, lo importante es:

```text
infer_camera.py
smoke_test.py
requirements.txt
models/bestfinal.pt
```

No hace falta mover datasets, runs, imagenes de entrenamiento ni cosas de preparacion.

## Instalar en la Jetson

Desde el repo del robot:

```bash
cd robot_vision
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Si la Jetson ya tiene PyTorch/Ultralytics instalado en otro ambiente, puedes usar ese ambiente y saltarte el `.venv`.

Dependencias de esta carpeta:

```text
ultralytics
opencv-python
pyserial
```

## Probar que el modelo carga

Usa cualquier imagen:

```bash
python3 smoke_test.py /ruta/a/imagen.jpg
```

Salida esperada:

```text
class=damage conf=0.932
```

o:

```text
class=no_damage conf=0.884
```

Si esto funciona, el modelo esta cargando bien.

## Probar camara sin ESP32

Primero prueba que la camara abre:

```bash
python3 infer_camera.py --camera 0 --show
```

Para cerrar la ventana, presiona `q`.

Si no abre, prueba otra camara:

```bash
python3 infer_camera.py --camera 1 --show
```

Modo sin ventana, mas parecido a integracion real:

```bash
python3 infer_camera.py --camera 0
```

El script imprime algo cada medio segundo:

```text
ok class=damage label='Damage' conf=0.932
ok class=no_damage label='No damage' conf=0.884
```

Puedes cambiar cada cuanto imprime:

```bash
python3 infer_camera.py --camera 0 --print-every 0.2
```

Y puedes pedir confianza minima:

```bash
python3 infer_camera.py --camera 0 --min-conf 0.70
```

Si la confianza queda abajo de eso, imprime:

```text
low_conf class=damage label='Damage' conf=0.621
```

## Probar camara CSI con GStreamer

Si la Jetson usa camara CSI, puedes pasar un pipeline GStreamer como `--camera`.

Ejemplo base:

```bash
python3 infer_camera.py --camera "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=640, height=480, framerate=30/1 ! nvvidconv ! video/x-raw, format=BGRx ! videoconvert ! video/x-raw, format=BGR ! appsink" --show
```

Si usas USB webcam, normalmente basta con `--camera 0`.

## Probar el trigger del screenshot en la Jetson

Conecta el ESP32 por USB y revisa que puerto le dio Linux:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

Puede ser algo como:

```text
/dev/ttyUSB0
```

o:

```text
/dev/ttyACM0
```

Corre vision con serial:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --baud 115200 --show
```

Si tu puerto es otro:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyACM0 --baud 115200 --show
```

Cuando el ESP32 mande `PHOTO_TRIGGER`, Python debe imprimir:

```text
photo_saved metadata=... class=... conf=...
```

Luego revisa:

```bash
ls captures
```

Deberias ver un `.jpg` y un `.json` por cada trigger.

## Probar trigger sin ESP32 real

Si quieres probar solo la parte de Python/serial sin depender del codigo del ESP32, puedes usar dos terminales con un puerto serial virtual. En Jetson normalmente sirve `socat`.

Instala si hace falta:

```bash
sudo apt install socat
```

Terminal 1:

```bash
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

Eso imprime dos puertos, por ejemplo:

```text
/dev/pts/3
/dev/pts/4
```

Terminal 2, corre vision usando el primer puerto:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/pts/3 --show
```

Terminal 3, manda el trigger por el segundo puerto:

```bash
printf "PHOTO_TRIGGER\n" > /dev/pts/4
```

Si todo esta bien, se guarda la captura en `captures/`.

## Comando recomendado para integracion real

Para el robot, sin ventana:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --baud 115200 --capture-dir captures --min-conf 0.70
```

Con ventana para debug:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --baud 115200 --capture-dir captures --min-conf 0.70 --show
```

## Argumentos utiles

```text
--model         Ruta al modelo .pt. Default: models/bestfinal.pt
--camera        Indice de camara o pipeline GStreamer. Default: 0
--width         Ancho de captura. Default: 640
--height        Alto de captura. Default: 480
--show          Muestra ventana de OpenCV
--min-conf      Confianza minima para status ok
--print-every   Cada cuantos segundos imprime prediccion
--serial-port   Puerto del ESP32, por ejemplo /dev/ttyUSB0
--baud          Baud rate del ESP32. Default: 115200
--capture-dir   Carpeta donde se guardan screenshots. Default: captures
```

## Problemas comunes

Si la camara no abre:

```bash
python3 infer_camera.py --camera 1 --show
```

Si el serial no abre, revisa permisos:

```bash
ls -l /dev/ttyUSB0
```

Puede que necesites agregar tu usuario al grupo `dialout`:

```bash
sudo usermod -a -G dialout $USER
```

Luego cierra sesion y vuelve a entrar.

Si no se guarda screenshot:

```text
1. Confirma que estas pasando --serial-port.
2. Confirma que el ESP32 esta mandando exactamente PHOTO_TRIGGER con salto de linea.
3. Confirma que el baud coincide, normalmente 115200.
4. Mira la consola de Python: debe aparecer photo_saved.
5. Revisa la carpeta captures/.
```

Si el modelo no carga:

```text
1. Revisa que exista models/bestfinal.pt.
2. Corre smoke_test.py con una imagen.
3. Confirma que ultralytics este instalado en el ambiente activo.
```

## Mini resumen

Para probar rapido en Jetson:

```bash
cd robot_vision
source .venv/bin/activate
python3 smoke_test.py /ruta/a/imagen.jpg
python3 infer_camera.py --camera 0 --show
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --baud 115200 --show
```

Y cuando el ESP32 mande:

```text
PHOTO_TRIGGER
```

Python toma el frame actual, corre/usa la prediccion actual, guarda screenshot y metadata en:

```text
captures/
```
