#!/bin/bash

# Script para iniciar vision con auto-detección de puerto serial
# Guarda logs en robot_vision/logs/

LOG_DIR="/home/jetson/robot_vision/logs"
LOG_FILE="$LOG_DIR/vision_$(date +%Y%m%d_%H%M%S).log"
VENV_PATH="/home/jetson/robot_vision/.venv"
SCRIPT_PATH="/home/jetson/robot_vision"

# Crear directorio de logs si no existe
mkdir -p "$LOG_DIR"

# Log wrapper function
log_msg() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

log_msg "========== Iniciando Robot Vision =========="

# Cambiar a directorio
cd "$SCRIPT_PATH" || { log_msg "ERROR: No se puede acceder a $SCRIPT_PATH"; exit 1; }
log_msg "Directorio: $(pwd)"

# Activar virtual environment
if [ ! -d "$VENV_PATH" ]; then
    log_msg "Virtual environment no encontrado. Creando..."
    python3 -m venv "$VENV_PATH" >> "$LOG_FILE" 2>&1
    log_msg "Virtual environment creado"
fi

source "$VENV_PATH/bin/activate" >> "$LOG_FILE" 2>&1
log_msg "Virtual environment activado"

# Instalar dependencias si es necesario
if [ ! -f "$SCRIPT_PATH/requirements.txt" ]; then
    log_msg "ERROR: requirements.txt no encontrado"
    exit 1
fi

log_msg "Verificando dependencias..."
pip install -q -r requirements.txt >> "$LOG_FILE" 2>&1
log_msg "Dependencias verificadas"

# Detectar puerto serial automáticamente
SERIAL_PORT=""

# Buscar en ttyUSB primero
for port in /dev/ttyUSB*; do
    if [ -e "$port" ]; then
        SERIAL_PORT="$port"
        log_msg "Puerto detectado: $SERIAL_PORT (USB)"
        break
    fi
done

# Si no hay USB, buscar en ttyACM
if [ -z "$SERIAL_PORT" ]; then
    for port in /dev/ttyACM*; do
        if [ -e "$port" ]; then
            SERIAL_PORT="$port"
            log_msg "Puerto detectado: $SERIAL_PORT (ACM)"
            break
        fi
    done
fi

# Si no hay puerto, salir
if [ -z "$SERIAL_PORT" ]; then
    log_msg "ERROR: No se encontró puerto serial. Conecta el ESP32 por USB"
    exit 1
fi

# Correr vision
log_msg "Iniciando infer_camera.py..."
log_msg "Comando: python3 infer_camera.py --camera 0 --serial-port $SERIAL_PORT --baud 115200"
log_msg "=========================================="

# No mostrar ventana gráfica en el servicio de systemd (headless)
SHOW_ARG=""
if [ "${SHOW_WINDOW:-false}" = "true" ]; then
    SHOW_ARG="--show"
    log_msg "SHOW_WINDOW=true: se habilitará la ventana OpenCV"
fi

python3 infer_camera.py --camera 0 --serial-port "$SERIAL_PORT" --baud 115200 $SHOW_ARG >> "$LOG_FILE" 2>&1

EXIT_CODE=$?
log_msg "========== Script finalizado (exit code: $EXIT_CODE) =========="
