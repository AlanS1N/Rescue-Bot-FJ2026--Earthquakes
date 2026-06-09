// ============================================================
// ROCKER BOGIE ROVER
// + ESP-NOW HEAD TRACKING RX
// + PHOTO TRIGGER
// ============================================================

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <ESP32Servo.h>
#include <ESP32PWM.h>

#define ESPNOW_CHANNEL 1

// ============================================================
// ESP-NOW DATA
// ============================================================

typedef struct {
  float yaw;
  float roll;
} DataPacket;

DataPacket latestData;
volatile bool newData = false;

// ============================================================
// RIGHT IBT2
// ============================================================

Servo servoYaw;
Servo servoRoll;

float currentYaw = 90;
float currentRoll = 90;

const int YAW_MIN = 5;
const int YAW_MAX = 175;

const int ROLL_MIN = 30;
const int ROLL_MAX = 150;

// ============================================================
// LEFT IBT2
// ============================================================

const int RPWM_L = 32;
const int LPWM_L = 33;
const int R_EN_L = 26;
const int L_EN_L = 25;

// ============================================================
// RIGHT IBT2
// ============================================================

const int RPWM_R = 14;
const int LPWM_R = 27;
const int R_EN_R = 13;
const int L_EN_R = 12;


// ============================================================
// IBUS
// ============================================================

const int IBUS_MIN = 1000;
const int IBUS_MID = 1500;
const int IBUS_MAX = 2000;
const int IBUS_DEADZONE = 35;

uint16_t chSteering = IBUS_MID;
uint16_t chThrottle = IBUS_MID;
uint16_t chSWA = 1000;

bool swaActivoAnterior = false;
unsigned long ultimaLectura = 0;

// ============================================================
// DRIVE TUNING
// ============================================================

const int PWM_MAX = 230;
const int PWM_START = 75;

const int TURN_PWM_MAX = 210;
const int TURN_PWM_START = 90;

const int RAMP_STEP = 8;

int velIzqActual = 0;
int velDerActual = 0;

// DEBUG
unsigned long ultimoDebug = 0;
const unsigned long DEBUG_INTERVAL = 100;

// ============================================================
// ESP-NOW CALLBACK
// ============================================================

void onReceive(
  const esp_now_recv_info *info,
  const uint8_t *data,
  int len
) {

  if (len != sizeof(latestData)) {
    return;
  }

  memcpy(
    &latestData,
    data,
    sizeof(latestData)
  );

  newData = true;
}

// ============================================================
// IBUS READER
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
        chThrottle =
          (chThrottle * 0.5) +
          (tempCH1 * 0.5);
      }

      if (canalValido(tempCH2)) {
        chSteering =
          (chSteering * 0.5) +
          (tempCH2 * 0.5);
      }

      if (tempCH7 >= 800 &&
          tempCH7 <= 2200) {
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

  // ----------------------------------------------------------
  // iBUS
  // ----------------------------------------------------------

  Serial2.begin(
    115200,
    SERIAL_8N1,
    16,
    -1
  );

  // ----------------------------------------------------------
  // WIFI / ESP-NOW
  // ----------------------------------------------------------

  WiFi.disconnect(true);

  WiFi.mode(WIFI_STA);

  delay(500);

  esp_wifi_set_channel(
    ESPNOW_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );

  if (esp_now_init() != ESP_OK) {

    Serial.println(
      "ESP-NOW INIT FAILED"
    );

    while (true);
  }

  esp_now_register_recv_cb(
    onReceive
  );

  // ----------------------------------------------------------
  // SERVOS
  // ----------------------------------------------------------

  ESP32PWM::allocateTimer(0);

  servoYaw.setPeriodHertz(50);
  servoRoll.setPeriodHertz(50);

  servoYaw.attach(21, 500, 2500);
  servoRoll.attach(19, 500, 2500);

  servoYaw.write(90);
  servoRoll.write(90);

  // ----------------------------------------------------------
  // MOTOR DRIVERS
  // ----------------------------------------------------------

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
  // ----------------------------------------------------------
  // PRINT MAC ADDRESS
  // ----------------------------------------------------------

  Serial.print("RX MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("ROVER READY");
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  leerIBus();

  revisarSWA();

  actualizarHeadTracking();

  if (millis() - ultimaLectura > 300) {

    frenar();

    return;
  }

  int throttle =
    mapCanal(
      chThrottle,
      PWM_MAX,
      PWM_START
    );

  int steering =
    mapCanal(
      chSteering,
      PWM_MAX,
      PWM_START
    );

  int velIzq = 0;
  int velDer = 0;

  if (throttle == 0 &&
      steering != 0) {

    int giro =
      escalarPWM(
        steering,
        TURN_PWM_MAX,
        TURN_PWM_START
      );

    velIzq = -giro;
    velDer = giro;

  } else {

    velIzq =
      throttle - steering;

    velDer =
      throttle + steering;
  }

  velIzq =
    constrain(
      velIzq,
      -255,
      255
    );

  velDer =
    constrain(
      velDer,
      -255,
      255
    );

  velIzqActual =
    aplicarRampa(
      velIzqActual,
      velIzq
    );

  velDerActual =
    aplicarRampa(
      velDerActual,
      velDer
    );

  moverMotores(
    velIzqActual,
    velDerActual
  );

  if (millis() - ultimoDebug >= DEBUG_INTERVAL) {

    ultimoDebug = millis();

    Serial.print("CH1:");
    Serial.print(chSteering);

    Serial.print(" CH2:");
    Serial.print(chThrottle);

    Serial.print(" CH7:");
    Serial.print(chSWA);

    Serial.print(" THR:");
    Serial.print(throttle);

    Serial.print(" STR:");
    Serial.print(steering);

    Serial.print(" L:");
    Serial.print(velIzqActual);

    Serial.print(" R:");
    Serial.print(velDerActual);

    Serial.print(" | Yaw:");
    Serial.print(latestData.yaw, 1);

    Serial.print(" Roll:");
    Serial.print(latestData.roll, 1);

    Serial.print(" ServoYaw:");
    Serial.print(currentYaw, 1);

    Serial.print(" ServoRoll:");
    Serial.println(currentRoll, 1);
  }

  delay(5);
}

// ============================================================
// HEAD TRACKING
// ============================================================

void actualizarHeadTracking() {

  if (!newData) return;

  newData = false;

  float yaw = latestData.yaw;
  float roll = latestData.roll;

  float yawServoPos =
    map(
      yaw,
      -90,
      90,
      YAW_MIN,
      YAW_MAX
    );

  float rollServoPos =
    map(
      roll,
      -45,
      45,
      ROLL_MIN,
      ROLL_MAX
    );

  yawServoPos =
    constrain(
      yawServoPos,
      YAW_MIN,
      YAW_MAX
    );

  rollServoPos =
    constrain(
      rollServoPos,
      ROLL_MIN,
      ROLL_MAX
    );

  currentYaw +=
    (yawServoPos - currentYaw)
    * 0.12;

  currentRoll +=
    (rollServoPos - currentRoll)
    * 0.12;

  servoYaw.write(
    (int)currentYaw
  );

  servoRoll.write(
    (int)currentRoll
  );
}

// ============================================================
// PHOTO TRIGGER
// ============================================================

void revisarSWA() {

  bool swaActivo =
    chSWA > 1500;

  if (
    swaActivo &&
    !swaActivoAnterior
  ) {

    Serial.println(
      "PHOTO_TRIGGER"
    );
  }

  swaActivoAnterior =
    swaActivo;
}

// ============================================================
// HELPERS
// ============================================================

bool canalValido(uint16_t canal) {
  return canal >= 900 &&
         canal <= 2100;
}

int mapCanal(
  uint16_t canal,
  int pwmMax,
  int pwmStart
) {

  int offset =
    canal - IBUS_MID;

  if (abs(offset)
      < IBUS_DEADZONE)
    return 0;

  int salida;

  if (offset > 0) {

    salida =
      map(
        offset,
        IBUS_DEADZONE,
        IBUS_MAX - IBUS_MID,
        pwmStart,
        pwmMax
      );

  } else {

    salida =
      -map(
        abs(offset),
        IBUS_DEADZONE,
        IBUS_MID - IBUS_MIN,
        pwmStart,
        pwmMax
      );
  }

  return constrain(
    salida,
    -pwmMax,
    pwmMax
  );
}

int escalarPWM(
  int valor,
  int pwmMax,
  int pwmStart
) {

  if (valor == 0)
    return 0;

  int salida =
    map(
      abs(valor),
      PWM_START,
      PWM_MAX,
      pwmStart,
      pwmMax
    );

  salida =
    constrain(
      salida,
      pwmStart,
      pwmMax
    );

  return valor > 0 ?
         salida :
         -salida;
}

int aplicarRampa(
  int actual,
  int objetivo
) {

  if (actual < objetivo)
    return min(
      actual + RAMP_STEP,
      objetivo
    );

  if (actual > objetivo)
    return max(
      actual - RAMP_STEP,
      objetivo
    );

  return actual;
}

// ============================================================
// MOTORS
// ============================================================

void moverMotores(
  int izq,
  int der
) {

  if (izq > 0) {

    ledcWrite(RPWM_L, izq);
    ledcWrite(LPWM_L, 0);

  } else {

    ledcWrite(RPWM_L, 0);
    ledcWrite(LPWM_L, abs(izq));
  }

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
