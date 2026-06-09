/////////////////////////////////////////////////////////
//        ESP-NOW TX + BNO08X (YAW + ROLL)            //
/////////////////////////////////////////////////////////

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <Wire.h>
#include <Adafruit_BNO08x.h>

/////////////////////////////////////////////////////////
// SETTINGS
/////////////////////////////////////////////////////////

#define ESPNOW_CHANNEL 1

/////////////////////////////////////////////////////////
// RX MAC ADDRESS          !! MUST CHANGE WITH PROPER !!
/////////////////////////////////////////////////////////

uint8_t receiverAddress[] = {
  0x20, 0xE7, 0xC8, 0x67, 0x6F, 0xF8
};

/////////////////////////////////////////////////////////
// DATA STRUCTURE
/////////////////////////////////////////////////////////

typedef struct {

  float yaw;
  float roll;

} DataPacket;

DataPacket data;

/////////////////////////////////////////////////////////
// IMU
/////////////////////////////////////////////////////////

Adafruit_BNO08x bno08x;

sh2_SensorValue_t sensorValue;

/////////////////////////////////////////////////////////
// OFFSETS
/////////////////////////////////////////////////////////

float yawOffset = 0;
float rollOffset = 0;

bool calibrated = false;

/////////////////////////////////////////////////////////
// SEND CALLBACK
/////////////////////////////////////////////////////////

void onDataSent(
  const wifi_tx_info_t *info,
  esp_now_send_status_t status
) {

  if (status == ESP_NOW_SEND_SUCCESS) {

    Serial.println("Delivery Success");
  }
  else {

    Serial.println("Delivery Failed");
  }
}

/////////////////////////////////////////////////////////
// SETUP
/////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println("=================================");
  Serial.println("      BNO08X YAW + ROLL TX");
  Serial.println("=================================");

  /////////////////////////////////////////////////////////
  // I2C
  /////////////////////////////////////////////////////////

  Wire.begin(21, 19);

  Wire.setClock(50000);

  /////////////////////////////////////////////////////////
  // BNO08X
  /////////////////////////////////////////////////////////

  Serial.println("Initializing BNO08X...");

  if (!bno08x.begin_I2C(0x4B)) {

    Serial.println("Trying 0x4A...");

    if (!bno08x.begin_I2C(0x4A)) {

      Serial.println("BNO08X NOT FOUND");

      while (1);
    }
  }¿

  Serial.println("BNO08X READY");

  /////////////////////////////////////////////////////////
  // GAME ROTATION VECTOR
  /////////////////////////////////////////////////////////

  bno08x.enableReport(
    SH2_GAME_ROTATION_VECTOR
  );

  /////////////////////////////////////////////////////////
  // WIFI
  /////////////////////////////////////////////////////////

  WiFi.disconnect(true);

  delay(1000);

  WiFi.mode(WIFI_STA);

  delay(1000);

  esp_wifi_set_promiscuous(true);

  esp_wifi_set_channel(
    ESPNOW_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );

  esp_wifi_set_promiscuous(false);

  /////////////////////////////////////////////////////////
  // PRINT MAC
  /////////////////////////////////////////////////////////

  Serial.print("TX MAC: ");

  Serial.println(WiFi.macAddress());

  /////////////////////////////////////////////////////////
  // ESP-NOW
  /////////////////////////////////////////////////////////

  if (esp_now_init() != ESP_OK) {

    Serial.println("ESP-NOW INIT FAILED");

    return;
  }

  esp_now_register_send_cb(onDataSent);

  /////////////////////////////////////////////////////////
  // PEER
  /////////////////////////////////////////////////////////

  esp_now_peer_info_t peerInfo = {};

  memcpy(
    peerInfo.peer_addr,
    receiverAddress,
    6
  );

  peerInfo.channel = ESPNOW_CHANNEL;

  peerInfo.encrypt = false;

  peerInfo.ifidx = WIFI_IF_STA;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {

    Serial.println("FAILED TO ADD PEER");

    return;
  }

  Serial.println("TX READY");

  Serial.println("=================================");
}

/////////////////////////////////////////////////////////
// LOOP
/////////////////////////////////////////////////////////

void loop() {

  if (bno08x.getSensorEvent(&sensorValue)) {

    if (sensorValue.sensorId ==
        SH2_GAME_ROTATION_VECTOR) {

      /////////////////////////////////////////////////////
      // QUATERNIONS
      /////////////////////////////////////////////////////

      float qx =
        sensorValue.un.rotationVector.i;

      float qy =
        sensorValue.un.rotationVector.j;

      float qz =
        sensorValue.un.rotationVector.k;

      float qw =
        sensorValue.un.rotationVector.real;

      /////////////////////////////////////////////////////
      // YAW
      /////////////////////////////////////////////////////

      float yaw = atan2(
        2.0 * (qw*qz + qx*qy),
        1.0 - 2.0 *
        (qy*qy + qz*qz)
      ) * 180.0 / PI;

      /////////////////////////////////////////////////////
      // ROLL
      /////////////////////////////////////////////////////

      float roll = atan2(
        2.0 * (qw*qx + qy*qz),
        1.0 - 2.0 *
        (qx*qx + qy*qy)
      ) * 180.0 / PI;

      /////////////////////////////////////////////////////
      // INITIAL CALIBRATION
      /////////////////////////////////////////////////////

      if (!calibrated) {

        yawOffset = yaw;

        rollOffset = roll;

        calibrated = true;

        Serial.println("CENTER POSITION SAVED");
      }

      /////////////////////////////////////////////////////
      // RELATIVE MOVEMENT
      /////////////////////////////////////////////////////

      float relativeYaw =
        yaw - yawOffset;

      float relativeRoll =
        roll - rollOffset;

      /////////////////////////////////////////////////////
      // FIX WRAPAROUND
      /////////////////////////////////////////////////////

      if (relativeYaw > 180)
        relativeYaw -= 360;

      if (relativeYaw < -180)
        relativeYaw += 360;

      if (relativeRoll > 180)
        relativeRoll -= 360;

      if (relativeRoll < -180)
        relativeRoll += 360;

      /////////////////////////////////////////////////////
      // DEADZONE
      /////////////////////////////////////////////////////

      if (abs(relativeYaw) < 1.0)
        relativeYaw = 0;

      if (abs(relativeRoll) < 1.0)
        relativeRoll = 0;

      /////////////////////////////////////////////////////
      // LIMITS
      /////////////////////////////////////////////////////

      relativeYaw = constrain(
        relativeYaw,
        -90,
        90
      );

      relativeRoll = constrain(
        relativeRoll,
        -45,
        45
      );

      /////////////////////////////////////////////////////
      // ASSIGN DATA
      /////////////////////////////////////////////////////

      data.yaw = relativeYaw;

      data.roll = relativeRoll;

      /////////////////////////////////////////////////////
      // SEND
      /////////////////////////////////////////////////////

      esp_now_send(
        receiverAddress,
        (uint8_t *)&data,
        sizeof(data)
      );

      /////////////////////////////////////////////////////
      // DEBUG
      /////////////////////////////////////////////////////

      Serial.print("Yaw: ");

      Serial.print(data.yaw);

      Serial.print(" | Roll: ");

      Serial.println(data.roll);
    }
  }

  delay(20);
}
