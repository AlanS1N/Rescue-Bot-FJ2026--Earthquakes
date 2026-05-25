# Prueba rapida: trigger + screenshot en Jetson

Objetivo: comprobar que el ESP32 manda el trigger y que la Jetson guarda aunque sea una screenshot.

## 1. Cargar el codigo al ESP32

En Arduino IDE:

1. Abre el sketch del ESP32.
2. Selecciona la placa ESP32 correcta.
3. Selecciona el puerto USB del ESP32.
4. Sube el codigo.

Lo importante es que, cuando quieras tomar foto, el ESP32 mande esto por Serial:

```cpp
Serial.println("PHOTO_TRIGGER");
```

El baud debe coincidir con Python. Recomendado:

```cpp
Serial.begin(115200);
```

## 2. Conectar ESP32 a la Jetson

Conecta el ESP32 a la Jetson por USB.

En la Jetson corre:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

Normalmente sera uno de estos:

```text
/dev/ttyUSB0
/dev/ttyACM0
```

Ese es el puerto que vas a usar en `--serial-port`.

## 3. Entrar a robot_vision

```bash
cd robot_vision
source .venv/bin/activate
```

Si todavia no tienes el ambiente:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## 4. Probar que la camara abre

Primero sin ESP32:

```bash
python3 infer_camera.py --camera 0 --show
```

Si no abre:

```bash
python3 infer_camera.py --camera 1 --show
```

Para salir, presiona `q`.

## 5. Correr vision escuchando al ESP32

Si tu puerto fue `/dev/ttyUSB0`:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --baud 115200 --show
```

Si tu puerto fue `/dev/ttyACM0`:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyACM0 --baud 115200 --show
```

Deja esa terminal corriendo.

## 6. Disparar el trigger

Ahora haz que el ESP32 mande:

```text
PHOTO_TRIGGER
```

Puede ser por boton, por tu logica del robot, o temporalmente con algo simple en Arduino:

```cpp
void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("PHOTO_TRIGGER");
  delay(3000);
}
```

Con ese sketch de prueba, debe tomar una screenshot cada 3 segundos.

## 7. Confirmar que si guardo

Cuando Python recibe el trigger, en la terminal debe salir algo asi:

```text
photo_saved metadata=captures/trigger-20260521-154233.json class=damage conf=0.932
```

Revisa la carpeta:

```bash
ls captures
```

Debes ver archivos tipo:

```text
trigger-20260521-154233.jpg
trigger-20260521-154233.json
```

Abre el `.jpg` y listo: esa es la screenshot tomada por trigger.

## Comando final mas probable

Este es el que casi seguro vas a usar:

```bash
cd robot_vision
source .venv/bin/activate
python3 infer_camera.py --camera 0 --serial-port /dev/ttyUSB0 --baud 115200 --show
```

Si el ESP32 aparece como ACM:

```bash
python3 infer_camera.py --camera 0 --serial-port /dev/ttyACM0 --baud 115200 --show
```

## Si no jala

Si no aparece `photo_saved`:

1. Revisa que el ESP32 mande exactamente `PHOTO_TRIGGER`.
2. Revisa que tenga salto de linea: usa `Serial.println`, no `Serial.print`.
3. Revisa que el baud sea `115200` en ambos lados.
4. Revisa que estes usando el puerto correcto.
5. Revisa permisos del puerto:

```bash
ls -l /dev/ttyUSB0
sudo usermod -a -G dialout $USER
```

Despues de cambiar grupo, cierra sesion y entra otra vez.
