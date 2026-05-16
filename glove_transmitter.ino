//glove_nodemcu_nrf24L01

#include <SPI.h>
#include <RF24.h>
#include <Wire.h>

// ─── NRF24L01 ──────────────────────────────────────────────────
RF24 radio(2, 15);
const byte ADDRESS[6] = "GCAR1";

// ─── MPU-6050 ──────────────────────────────────────────────────
const int MPU_ADDR = 0x68;
float pitch = 0.0, roll = 0.0;

// ─── TUNING ─────────────────────────────────────────────────────
const float DEAD_ZONE = 4.0;

const float MAX_SPEED_ANGLE = 25.0;
const float MAX_TURN_ANGLE  = 15.0;

// ─── PACKET ────────────────────────────────────────────────────
uint8_t packet[5];

// ─── HELPERS ────────────────────────────────────────────────────
float applyDeadZone(float angle) {
  if (abs(angle) < DEAD_ZONE) return 0.0;
  return (angle > 0 ? 1.0 : -1.0) * (abs(angle) - DEAD_ZONE);
}

// Speed mapping
int pitchToPWM(float angle) {
  float eff = abs(applyDeadZone(angle));
  float maxEff = MAX_SPEED_ANGLE - DEAD_ZONE;
  return (int)constrain((eff / maxEff) * 255.0, 0, 255);
}

// write int16
void writeInt16(uint8_t* buf, int offset, int16_t val) {
  buf[offset]     = (uint8_t)(val & 0xFF);
  buf[offset + 1] = (uint8_t)((val >> 8) & 0xFF);
}

// ─── MPU INIT ───────────────────────────────────────────────────
void initMPU() {
  Wire.begin(4, 5);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  delay(100);
}

// ─── READ MPU ───────────────────────────────────────────────────
void readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU_ADDR, 6, true);

  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();

  float axg = ax / 16384.0;
  float ayg = ay / 16384.0;
  float azg = az / 16384.0;

  pitch = atan2(axg, sqrt(ayg * ayg + azg * azg)) * 180.0 / PI;
  roll  = atan2(ayg, sqrt(axg * axg + azg * azg)) * 180.0 / PI;
}

// ─── SETUP ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  initMPU();

  if (!radio.begin()) {
    while (true);
  }

  radio.openWritingPipe(ADDRESS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);
  radio.setPayloadSize(5);
  radio.stopListening();
}

// ─── LOOP ──────────────────────────────────────────────────────
void loop() {
  readMPU();

  float pitchEff = applyDeadZone(pitch);
  float rollEff  = applyDeadZone(roll);

  // ─── 🚗 FULL FLIPPED DIRECTION ─────────────────────────────
  char dir = 'S';
  int16_t speed = 0;

  if (pitchEff > 0) {
    // 🔁 FLIPPED: forward becomes backward
    dir   = 'F';
    speed = pitchToPWM(pitch);
  }
  else if (pitchEff < 0) {
    dir   = 'B';
    speed = pitchToPWM(pitch);
  }

  // ─── 🔄 FULL FLIPPED STEERING ─────────────────────────────
  int16_t steer = 0;

  if (abs(rollEff) > 0) {

    // 🔁 FLIPPED LEFT/RIGHT
    float flippedRoll = -rollEff;

    float maxEff = MAX_TURN_ANGLE - DEAD_ZONE;

    float norm = flippedRoll / maxEff;

    float curved = norm * norm * (norm > 0 ? 1 : -1);

    steer = (int16_t)constrain(curved * 255.0, -255, 255);
  }

  // ─── PACKET ───────────────────────────────────────────────
  packet[0] = (uint8_t)dir;
  writeInt16(packet, 1, speed);
  writeInt16(packet, 3, steer);

  radio.write(packet, 5);

  // Debug
  Serial.printf("dir=%c spd=%d steer=%d | pitch=%.1f roll=%.1f\n",
    dir, speed, steer, pitch, roll);

  delay(20);
}