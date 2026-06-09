#!/bin/bash

# Script para iniciar vision con auto-detección de puerto serial
# Guarda logs en robot_vision/logs/

LOG_DIR="/home/traxxas/Desktop/rescue_robot/Final_old/robot_vision/logs"
LOG_FILE="$LOG_DIR/vision_$(date +%Y%m%d_%H%M%S).log"
VENV_PATH="/home/traxxas/Desktop/rescue_robot/Final_old/robot_vision/.venv"
SCRIPT_PATH="/home/traxxas/Desktop/rescue_robot/Final_old/robot_vision"

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
# Evitar que Python use paquetes de usuario instalados en /home/$USER/.local
export PYTHONNOUSERSITE=1
log_msg "Virtual environment activado (PYTHONNOUSERSITE=1)"

# Instalar dependencias si es necesario
if [ ! -f "$SCRIPT_PATH/requirements.txt" ]; then
    log_msg "ERROR: requirements.txt no encontrado"
    exit 1
fi

log_msg "Verificando dependencias..."
"$VENV_PATH/bin/python" -m pip install --upgrade pip >> "$LOG_FILE" 2>&1
"$VENV_PATH/bin/python" -m pip install -q -r requirements.txt >> "$LOG_FILE" 2>&1
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

# Si no hay puerto, continuar sin serial y permitir que infer_camera.py lo detecte en runtime
if [ -z "$SERIAL_PORT" ]; then
    log_msg "WARN: No se encontró puerto serial al iniciar. Se continuará sin ESP32. infer_camera.py seguirá intentando detectar el puerto en caliente."
fi

# Correr vision

log_msg "Iniciando infer_camera.py..."

# Detectar y logear DISPLAY automáticamente si no está seteado
if [ -z "$DISPLAY" ]; then
    # Intentar encontrar DISPLAY del usuario traxxas en sesiones activas
    FOUND_DISPLAY=$(ps -u traxxas -o env= 2>/dev/null | grep -o 'DISPLAY=[^[:space:]]*' | head -1 | cut -d= -f2)
    if [ -n "$FOUND_DISPLAY" ]; then
        export DISPLAY="$FOUND_DISPLAY"
        log_msg "DISPLAY no seteado en environment. Detectado automáticamente: $DISPLAY"
    else
        log_msg "WARN: No se pudo detectar DISPLAY. Asignando :0 por defecto."
        export DISPLAY=":0"
    fi
else
    log_msg "DISPLAY ya seteado: $DISPLAY"
fi

# Mostrar ventana gráfica si SHOW_WINDOW está seteado
SHOW_ARG=""
SHOW_WINDOW_VALUE="${SHOW_WINDOW:-false}"
log_msg "DEBUG: SHOW_WINDOW env var = '$SHOW_WINDOW_VALUE'"

if [ "$SHOW_WINDOW_VALUE" = "true" ] || [ "$SHOW_WINDOW_VALUE" = "1" ]; then
    SHOW_ARG="--show"
    log_msg "SHOW_WINDOW=true: se habilitará la ventana OpenCV en $DISPLAY"
else
    log_msg "SHOW_WINDOW está deshabilitado (valor: '$SHOW_WINDOW_VALUE')"
fi

# Use the venv python and call the project's infer_camera.py explícitamente
PYTHON_EXEC="$VENV_PATH/bin/python"

CMD_ARGS=("$SCRIPT_PATH/infer_camera.py" --camera 0 --baud 115200 $SHOW_ARG)
if [ -n "$SERIAL_PORT" ]; then
    CMD_ARGS+=(--serial-port "$SERIAL_PORT")
fi

log_msg "DISPLAY será: $DISPLAY"
log_msg "Comando: $PYTHON_EXEC ${CMD_ARGS[*]}"
"$PYTHON_EXEC" "${CMD_ARGS[@]}" >> "$LOG_FILE" 2>&1
EXIT_CODE=$?

# Si falla con exit code 2 (argumentos no reconocidos), reintentar sin los argumentos seriales
if [ "$EXIT_CODE" -eq 2 ]; then
    log_msg "infer_camera.py devolvió exit code 2; reintentando sin argumentos seriales"
    log_msg "Comando: $PYTHON_EXEC $SCRIPT_PATH/infer_camera.py --camera 0 $SHOW_ARG"
    "$PYTHON_EXEC" "$SCRIPT_PATH/infer_camera.py" --camera 0 $SHOW_ARG >> "$LOG_FILE" 2>&1
    EXIT_CODE=$?
fi

EXIT_CODE=$?
log_msg "========== Script finalizado (exit code: $EXIT_CODE) =========="
