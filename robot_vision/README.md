# Rescue Vision para la Jetson

Guia rapida para llevar el clasificador al repo del robot.

## Que mover al repo del robot

Mueve esta carpeta completa:

```text
robot_vision/
|-- infer_camera.py
|-- smoke_test.py
|-- requirements.txt
|-- README.md
`-- models/
    `-- bestfinal.pt
```

Eso es lo importante para correr en la Jetson Orin. No hace falta mover `dataset.zip`, `.venv`, `runs/`, `Random` ni las carpetas de imagenes. Esas sirven para entrenar o preparar datos, pero no para que el robot haga inferencia.

## Que hace

El modelo es un clasificador YOLO de Ultralytics. Recibe una imagen/frame completo y responde una de estas clases:

```text
damage
no_damage
```

Importante: esto no dibuja cajas ni localiza exactamente donde esta el dano. Solo dice si el frame completo parece tener dano o no.

## Modelos que hay en este repo

```text
models/best.pt        -> modelo chico/viejo, 2.9 MB
models/best82.9.pt    -> modelo final/backup, 10.2 MB
models/bestfinal.pt   -> modelo recomendado, 10.2 MB
yolo11n-cls.pt        -> modelo base para entrenamiento, no es el entrenado final
```

Para el robot usa:

```text
robot_vision/models/bestfinal.pt
```

## Versiones de camera_test.py

```text
camera_test.py   -> solo prueba que la camara capture 1 frame y guarda test.jpg
camera_test2.py  -> muestra video crudo de la camara
camera_test3.py  -> carga YOLO y muestra predicciones, pero usa best.pt en la raiz
```

Para el robot, usa mejor `robot_vision/infer_camera.py`. Es la version ordenada de esa idea: acepta argumentos, usa `models/bestfinal.pt` y puede correr con o sin ventana.

## Instalar en Jetson

En la Jetson, desde el repo del robot:

```bash
cd robot_vision
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Si ya tienes PyTorch/Ultralytics instalados en el entorno del robot, no necesitas crear otro `.venv`.

## Probar que el modelo carga

Con una imagen cualquiera:

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

## Correr con camara

Sin ventana, recomendado para integrarlo con el robot:

```bash
python3 infer_camera.py --camera 0
```

Con ventana para debug:

```bash
python3 infer_camera.py --camera 0 --show
```

Si la camara no abre, prueba:

```bash
python3 infer_camera.py --camera 1 --show
```

Tambien puedes pasar un pipeline GStreamer si estas usando una camara CSI:

```bash
python3 infer_camera.py --camera "TU_PIPELINE_GSTREAMER_AQUI" --show
```

## Salida para integrar

El script imprime una linea cada medio segundo:

```text
ok class=damage label='Dano' conf=0.932
ok class=no_damage label='Sin dano' conf=0.884
```

Puedes ajustar el intervalo:

```bash
python3 infer_camera.py --print-every 0.2
```

Y puedes pedir confianza minima:

```bash
python3 infer_camera.py --min-conf 0.70
```

Si la confianza queda abajo de ese valor, imprime `low_conf`.

## Como se entreno

El flujo fue:

```text
imagenes damage/no_damage
-> prepare_cls_dataset.py
-> dataset/train y dataset/val
-> yolo classify train
-> best.pt / bestfinal.pt
```

Comando base usado/recomendado:

```bash
yolo classify train data=dataset model=yolo11n-cls.pt imgsz=224 epochs=30 batch=32 name=damage_retrain
```

Despues del entrenamiento, el peso importante queda en:

```text
runs/classify/damage_retrain/weights/best.pt
```

Ese archivo se copia como:

```text
models/bestfinal.pt
```

## Regla simple

Para correr en el robot:

```text
modelo + infer_camera.py + requirements.txt
```

Para entrenar:

```text
imagenes + prepare_cls_dataset.py + yolo11n-cls.pt + comandos de entrenamiento
```
