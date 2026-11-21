// Part C: TX (SF7 vs SF12)

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
#define LORA_FREQ       433.2E6
#define LORA_SYNC       0x34
#define CURRENT_SF      7       // 7 or 12
#define TX_INTERVAL_MS  3000

Adafruit_BME280 bme;
BH1750 lightMeter;

bool bmeReady = false;
bool bhReady  = false;
uint32_t txCount = 0;

struct EnvRecord {
  float tempC;
  float humPct;
  float presHpa;
  float lux;
  uint8_t flags;
};

const int PAYLOAD_LEN = 9;

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
  bmeReady = bme.begin(0x76, &Wire) || bme.begin(0x77, &Wire);
  bhReady  = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire) ||
             lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire);
}

EnvRecord readSensors() {
  EnvRecord r = {27.35f, 65.40f, 1013.2f, 450.0f, 0x00};

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
  Wire.begin(21, 22);
  sensorsBegin();
  radioBegin();
  Serial.printf("Part C TX Ready (%.1f MHz, SF%d, BW 125kHz, Sync 0x%02X)\n",
                LORA_FREQ / 1E6, CURRENT_SF, LORA_SYNC);
}

void loop() {
  EnvRecord rec = readSensors();
  uint8_t buf[PAYLOAD_LEN];
  pack(rec, buf);

  txCount++;
  unsigned long tStart = millis();
  LoRa.beginPacket();
  LoRa.write(buf, PAYLOAD_LEN);
  LoRa.endPacket();
  unsigned long txMs = millis() - tStart;

  Serial.printf("TX #%lu (SF%d, %lums) -> T=%.2fC H=%.2f%% P=%.1fhPa L=%.0flux F=0x%02X\n",
                (unsigned long)txCount, CURRENT_SF, txMs,
                rec.tempC, rec.humPct, rec.presHpa, rec.lux, rec.flags);

  delay(TX_INTERVAL_MS);
}
