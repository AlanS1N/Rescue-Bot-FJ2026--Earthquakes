// ============================================================
// ROCKER BOGIE ROVER
// ESP32 + 2x IBT2 + FlySky iBUS
// ============================================================

// ---------------- RIGHT IBT2 ----------------
const int RPWM_R = 25;
const int LPWM_R = 26;
const int R_EN_R = 27;
const int L_EN_R = 14;

// ---------------- LEFT IBT2 -----------------
const int RPWM_L = 33;
const int LPWM_L = 32;
const int R_EN_L = 13;
const int L_EN_L = 12;

// ---------------- IBUS ----------------------
const int IBUS_MIN = 1000;
const int IBUS_MID = 1500;
const int IBUS_MAX = 2000;
const int IBUS_DEADZONE = 35;

uint16_t chSteering = IBUS_MID;  // CH1 -> derecha/izquierda
uint16_t chThrottle = IBUS_MID;  // CH2 -> adelante/atras
uint16_t chSWA = 1000;           // CH7 -> switch screenshot/camara

bool swaActivoAnterior = false;
unsigned long ultimaLectura = 0;

// ---------------- DRIVE TUNING ---------------
const int PWM_MAX = 230;       // Menos velocidad punta = menos patinaje.
const int PWM_START = 75;      // Minimo util para vencer friccion.
const int TURN_PWM_MAX = 210;  // Giro sobre eje mas controlado.
const int TURN_PWM_START = 90;
const int RAMP_STEP = 8;       // Aceleracion suave para mejorar agarre.

int velIzqActual = 0;
int velDerActual = 0;

// ============================================================
// READ IBUS
// ============================================================

void leerIBus() {

  while (Serial2.available()) {

    static uint8_t buffer[32];
    static uint8_t indice = 0;

    uint8_t b = Serial2.read();

    if (indice == 0 && b != 0x20) continue;

    if (indice == 1 && b != 0x40) {
      indice = 0;
      continue;
    }

    buffer[indice++] = b;

    if (indice == 32) {

      uint16_t tempCH1 = buffer[2] | (buffer[3] << 8);
      uint16_t tempCH2 = buffer[4] | (buffer[5] << 8);
      uint16_t tempCH7 = buffer[14] | (buffer[15] << 8);

      if (canalValido(tempCH1)) {
        chSteering = (chSteering * 0.5) + (tempCH1 * 0.5);
      }

      if (canalValido(tempCH2)) {
        chThrottle = (chThrottle * 0.5) + (tempCH2 * 0.5);
      }

      if (tempCH7 >= 800 && tempCH7 <= 2200) {
        chSWA = tempCH7;
      }

      ultimaLectura = millis();
      indice = 0;
    }
  }
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  // iBUS RX -> GPIO16
  Serial2.begin(115200, SERIAL_8N1, 16, -1);

  pinMode(R_EN_R, OUTPUT);
  pinMode(L_EN_R, OUTPUT);
  pinMode(R_EN_L, OUTPUT);
  pinMode(L_EN_L, OUTPUT);

  digitalWrite(R_EN_R, HIGH);
  digitalWrite(L_EN_R, HIGH);
  digitalWrite(R_EN_L, HIGH);
  digitalWrite(L_EN_L, HIGH);

  ledcAttach(RPWM_R, 20000, 8);
  ledcAttach(LPWM_R, 20000, 8);
  ledcAttach(RPWM_L, 20000, 8);
  ledcAttach(LPWM_L, 20000, 8);

  frenar();

  Serial.println("ROVER READY");
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  leerIBus();
  revisarSWA();

  if (millis() - ultimaLectura > 300) {
    frenar();
    return;
  }

  int throttle = mapCanal(chThrottle, PWM_MAX, PWM_START);
  int steering = mapCanal(chSteering, PWM_MAX, PWM_START);

  int velIzq = 0;
  int velDer = 0;

  if (throttle == 0 && steering != 0) {
    int giro = escalarPWM(steering, TURN_PWM_MAX, TURN_PWM_START);
    velIzq = giro;
    velDer = -giro;
  } else {
    velIzq = throttle + steering;
    velDer = throttle - steering;
  }

  velIzq = constrain(velIzq, -255, 255);
  velDer = constrain(velDer, -255, 255);

  velIzqActual = aplicarRampa(velIzqActual, velIzq);
  velDerActual = aplicarRampa(velDerActual, velDer);

  moverMotores(velIzqActual, velDerActual);

  Serial.print("THR: ");
  Serial.print(throttle);
  Serial.print(" STR: ");
  Serial.print(steering);
  Serial.print(" L: ");
  Serial.print(velIzqActual);
  Serial.print(" R: ");
  Serial.println(velDerActual);

  delay(5);
}

// ============================================================
// CONTROL SWITCH EVENTS
// ============================================================

void revisarSWA() {

  bool swaActivo = chSWA > 1500;

  if (swaActivo && !swaActivoAnterior) {
    Serial.println("PHOTO_TRIGGER");
  }

  swaActivoAnterior = swaActivo;
}

// ============================================================
// CHANNEL / DRIVE HELPERS
// ============================================================

bool canalValido(uint16_t canal) {
  return canal >= 900 && canal <= 2100;
}

int mapCanal(uint16_t canal, int pwmMax, int pwmStart) {

  int offset = canal - IBUS_MID;

  if (abs(offset) < IBUS_DEADZONE) return 0;

  int salida;

  if (offset > 0) {
    salida = map(offset, IBUS_DEADZONE, IBUS_MAX - IBUS_MID, pwmStart, pwmMax);
  } else {
    salida = -map(abs(offset), IBUS_DEADZONE, IBUS_MID - IBUS_MIN, pwmStart, pwmMax);
  }

  return constrain(salida, -pwmMax, pwmMax);
}

int escalarPWM(int valor, int pwmMax, int pwmStart) {

  if (valor == 0) return 0;

  int salida = map(abs(valor), PWM_START, PWM_MAX, pwmStart, pwmMax);
  salida = constrain(salida, pwmStart, pwmMax);

  return valor > 0 ? salida : -salida;
}

int aplicarRampa(int actual, int objetivo) {

  if (actual < objetivo) {
    return min(actual + RAMP_STEP, objetivo);
  }

  if (actual > objetivo) {
    return max(actual - RAMP_STEP, objetivo);
  }

  return actual;
}

// ============================================================
// MOTOR CONTROL
// ============================================================

void moverMotores(int izq, int der) {

  if (izq > 0) {
    ledcWrite(RPWM_L, izq);
    ledcWrite(LPWM_L, 0);
  } else {
    ledcWrite(RPWM_L, 0);
    ledcWrite(LPWM_L, abs(izq));
  }

  // El lado derecho va invertido por montaje/cableado.
  der = -der;

  if (der > 0) {
    ledcWrite(RPWM_R, der);
    ledcWrite(LPWM_R, 0);
  } else {
    ledcWrite(RPWM_R, 0);
    ledcWrite(LPWM_R, abs(der));
  }
}

// ============================================================
// BRAKE
// ============================================================

void frenar() {

  velIzqActual = 0;
  velDerActual = 0;

  ledcWrite(RPWM_L, 0);
  ledcWrite(LPWM_L, 0);
  ledcWrite(RPWM_R, 0);
  ledcWrite(LPWM_R, 0);
}
