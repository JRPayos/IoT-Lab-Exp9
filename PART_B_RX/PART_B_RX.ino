#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 26

#define LCD_SDA 21
#define LCD_SCL 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LORA_FREQ   433.2E6
#define LORA_SYNC   0x34
#define CURRENT_SF  7
#define PAYLOAD_LEN 9

struct EnvRecord {
  float tempC;
  float humPct;
  float presHpa;
  float lux;
  uint8_t flags;
};

unsigned long lastPacket = 0;
bool packetReceived = false;

void unpack(const uint8_t* b, EnvRecord& r) {
  r.tempC   = (int16_t)((b[0] << 8) | b[1]) / 100.0f;
  r.humPct  = (uint16_t)((b[2] << 8) | b[3]) / 100.0f;
  r.presHpa = (uint16_t)((b[4] << 8) | b[5]) / 10.0f + 800.0f;
  r.lux     = (uint16_t)((b[6] << 8) | b[7]) * 10.0f;
  r.flags   = b[8];
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(LCD_SDA, LCD_SCL);
  Wire.setClock(100000);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("LoRa Receiver");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  SPI.begin(18, 19, 23, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa initialization failed!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LoRa INIT ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Check Wiring");

    while (true) {
      delay(1000);
    }
  }

  LoRa.setSpreadingFactor(CURRENT_SF);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(LORA_SYNC);

  Serial.println("LoRa Receiver Ready!");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LoRa RX Ready");
  lcd.setCursor(0, 1);
  lcd.print("Waiting Packet");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize > 0) {
    uint8_t buf[32];
    int n = 0;
    while (LoRa.available() && n < (int)sizeof(buf)) {
      buf[n++] = LoRa.read();
    }
    while (LoRa.available()) {
      LoRa.read();
    }

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    Serial.println("--------------------");
    if (n == PAYLOAD_LEN) {
      EnvRecord r;
      unpack(buf, r);

      Serial.printf("T=%.2fC H=%.2f%% P=%.1fhPa Lux=%.0f Flags=0x%02X\n",
                    r.tempC, r.humPct, r.presHpa, r.lux, r.flags);
      Serial.printf("RSSI: %d dBm | SNR: %.1f dB\n", rssi, snr);

      char line0[17], line1[17];
      snprintf(line0, sizeof(line0), "%4.1fC %2.0f%% %4.0fhP", r.tempC, r.humPct, r.presHpa);
      snprintf(line1, sizeof(line1), "L:%-4.0fF:%X %4ddB", r.lux, r.flags & 0x0F, rssi);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(line0);
      lcd.setCursor(0, 1);
      lcd.print(line1);
    } else {
      String message = "";
      for (int i = 0; i < n; i++) {
        message += (char)buf[i];
      }

      Serial.print("Received: ");
      Serial.println(message);
      Serial.printf("RSSI: %d dBm | SNR: %.1f dB\n", rssi, snr);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("RX:");
      lcd.print(message.substring(0, 13));
      lcd.setCursor(0, 1);
      lcd.print("RSSI:");
      lcd.print(rssi);
      lcd.print(" dBm");
    }

    lastPacket = millis();
    packetReceived = true;
  }

  if (packetReceived && millis() - lastPacket >= 10000) {
    packetReceived = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LoRa RX Ready");
    lcd.setCursor(0, 1);
    lcd.print("Waiting Packet");
  }
}
