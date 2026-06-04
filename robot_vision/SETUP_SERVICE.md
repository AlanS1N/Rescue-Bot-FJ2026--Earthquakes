# Setup Automático - Robot Vision Service

## ¿Qué hace?
- **Corre automáticamente al iniciar la Jetson** sin necesidad de SSH
- **Detecta automáticamente el puerto serial** (USB0 o ACM0)
- **Guarda logs** en `robot_vision/logs/` con timestamp
- **Reinicia automáticamente** si falla
- **Cámara 0 fija** (sin necesidad de cambiar parámetros)

## Instalación en la Jetson

### 1. Copiar archivos
```bash
cd ~/robot_vision
# Los archivos ya deben estar aquí:
# - start_vision.sh
# - robot-vision.service
```

### 2. Dar permisos al script
```bash
chmod +x ~/robot_vision/start_vision.sh
```

### 3. Copiar service a systemd
```bash
sudo cp ~/robot_vision/robot-vision.service /etc/systemd/system/
```

### 4. Recargar systemd
```bash
sudo systemctl daemon-reload
```

### 5. Habilitar el service (para que corra al boot)
```bash
sudo systemctl enable robot-vision.service
```

### 6. Iniciar el service (opcional, si no quieres esperar al reboot)
```bash
sudo systemctl start robot-vision.service
```

---

## Comandos útiles

### Ver estado del service
```bash
sudo systemctl status robot-vision.service
```

### Ver logs en vivo
```bash
sudo journalctl -u robot-vision.service -f
```

### Ver logs de una hora específica
```bash
sudo journalctl -u robot-vision.service --since "2026-06-04 10:00:00"
```

### Ver logs guardados en archivo
```bash
ls ~/robot_vision/logs/
cat ~/robot_vision/logs/vision_20260604_101234.log
```

### Parar el service
```bash
sudo systemctl stop robot-vision.service
```

### Reiniciar el service
```bash
sudo systemctl restart robot-vision.service
```

### Deshabilitar el service (no corra al boot)
```bash
sudo systemctl disable robot-vision.service
```

---

## Troubleshooting

### El service no inicia
```bash
sudo systemctl status robot-vision.service
# Ver el error específico
```

### Permisos del puerto serial
Si ves "Permission denied" en los logs:
```bash
sudo usermod -a -G dialout jetson
# Luego logout y login de nuevo
```

### Ver si el puerto está siendo detectado
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

### Ver logs del service en tiempo real
```bash
sudo journalctl -u robot-vision.service -f --no-pager
```

### El venv no se crea bien
```bash
# Borrar el venv y dejar que el script lo recree
rm -rf ~/robot_vision/.venv
sudo systemctl restart robot-vision.service
```

---

## Automatización adicional (opcional)

Si quieres **parar el service sin usar SSH**:

1. Crea un archivo vacío que signifique "parar"
2. Modifica el script para checarlo

O usa:
```bash
# Parar remotamente
ssh jetson@<IP> "sudo systemctl stop robot-vision.service"
```

---

## Flujo después de boot

1. Jetson arranca
2. systemd inicia el service automáticamente
3. Script detecta puerto serial
4. Corre `infer_camera.py` con los parámetros correctos
5. Logs se guardan en `logs/vision_TIMESTAMP.log`
6. Si falla, reintentar después de 10 segundos
