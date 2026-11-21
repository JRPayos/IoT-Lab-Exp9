// Part B: TX (BME280 + BH1750)

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <math.h>

#define SS_PIN    5
#define RST_PIN   14
#define DIO0_PIN  2

// Radio config (match RX)
#define LORA_FREQ   433.2E6
#define LORA_SYNC   0x34
#define CURRENT_SF  7

// I2C sensors (SDA=21, SCL=22)
Adafruit_BME280 bme;
BH1750 lightMeter;

bool bmeReady = false;
bool bhReady  = false;

struct EnvRecord {
  float tempC;
  float humPct;
  float presHpa;
  float lux;
  uint8_t flags; // bit0=BME280, bit1=BH1750
};

const int PAYLOAD_LEN = 9;

// [0..1] tempC*100, [2..3] hum*100, [4..5] (pres-800)*10, [6..7] lux/10, [8] flags
void pack(const EnvRecord& r, uint8_t* b) {
  float tClamped = constrain(r.tempC, -327.68f, 327.67f);
  float hClamped = constrain(r.humPct, 0.0f, 655.35f);
  float pClamped = constrain(r.presHpa, 800.0f, 7353.5f);
  float lClamped = constrain(r.lux, 0.0f, 655350.0f);

  int16_t  t = (int16_t)lroundf(tClamped * 100.0f);
  uint16_t h = (uint16_t)lroundf(hClamped * 100.0f);
  uint16_t p = (uint16_t)lroundf((pClamped - 800.0f) * 10.0f);
  uint16_t l = (uint16_t)lroundf(lClamped / 10.0f);

  b[0] = (t >> 8) & 0xFF; b[1] = t & 0xFF;
  b[2] = (h >> 8) & 0xFF; b[3] = h & 0xFF;
  b[4] = (p >> 8) & 0xFF; b[5] = p & 0xFF;
  b[6] = (l >> 8) & 0xFF; b[7] = l & 0xFF;
  b[8] = r.flags;
}

void radioBegin() {
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("radio init failed");
    while (1);
  }
  LoRa.setSpreadingFactor(CURRENT_SF);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(LORA_SYNC);
  LoRa.setTxPower(14);
}

void sensorsBegin() {
  bmeReady = bme.begin(0x76, &Wire);
  if (!bmeReady) {
    bmeReady = bme.begin(0x77, &Wire);
  }
  Serial.println(bmeReady ? "BME280: OK" : "BME280: NOT FOUND (using fallback values)");

  bhReady = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  if (!bhReady) {
    bhReady = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire);
  }
  Serial.println(bhReady ? "BH1750: OK" : "BH1750: NOT FOUND (using fallback values)");
}

EnvRecord readSensors() {
  EnvRecord r;
  r.tempC   = 27.35f;
  r.humPct  = 65.40f;
  r.presHpa = 1013.2f;
  r.lux     = 450.0f;
  r.flags   = 0x00;

  if (bmeReady) {
    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0f;
    if (!isnan(t) && !isnan(h) && !isnan(p) && p >= 800.0f) {
      r.tempC   = t;
      r.humPct  = h;
      r.presHpa = p;
      r.flags  |= 0x01;
    }
  }

  if (bhReady) {
    float lx = lightMeter.readLightLevel();
    if (!isnan(lx) && lx >= 0.0f) {
      r.lux    = lx;
      r.flags |= 0x02;
    }
  }

  return r;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Part B: Ra-02 Packed Telemetry TX Starting...");

  Wire.begin(21, 22);
  sensorsBegin();
  radioBegin();

  Serial.printf("Radio Init OK (%.1f MHz, SF%d, BW 125kHz, Sync 0x%02X)\n",
                LORA_FREQ / 1E6, CURRENT_SF, LORA_SYNC);
}

void loop() {
  EnvRecord rec = readSensors();
  uint8_t buf[PAYLOAD_LEN];
  pack(rec, buf);

  LoRa.beginPacket();
  LoRa.write(buf, PAYLOAD_LEN);
  LoRa.endPacket();

  char jsonBuf[128];
  int jsonLen = snprintf(jsonBuf, sizeof(jsonBuf),
                         "{\"tempC\":%.2f,\"humPct\":%.2f,\"presHpa\":%.1f,\"lux\":%.0f,\"flags\":%u}",
                         rec.tempC, rec.humPct, rec.presHpa, rec.lux, rec.flags);

  Serial.printf("TX (%dB vs %dB JSON) -> T=%.2fC H=%.2f%% P=%.1fhPa L=%.0flux F=0x%02X | Hex: ",
                PAYLOAD_LEN, jsonLen, rec.tempC, rec.humPct, rec.presHpa, rec.lux, rec.flags);
  for (int i = 0; i < PAYLOAD_LEN; i++) {
    Serial.printf("%02X ", buf[i]);
  }
  Serial.println();

  delay(4000);
}
