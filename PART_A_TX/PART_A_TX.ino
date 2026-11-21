// Part A: TX

#include <SPI.h>
#include <LoRa.h>

#define SS_PIN    5
#define RST_PIN   14
#define DIO0_PIN  26

#define LORA_FREQ 433.2E6

uint32_t counter = 0;

void radioBegin() {
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("radio init failed");
    while (1);
  }
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x34);
  LoRa.setTxPower(14);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Part A: Ra-02 (SX1278) LoRa TX Starting...");
  radioBegin();
  Serial.println("Radio Init OK (433 MHz, SF7, BW 125kHz, CR 4/5, Sync 0x34, 14 dBm)");
}

void loop() {
  char msg[24];
  snprintf(msg, sizeof(msg), "G6 PING #%lu", (unsigned long)counter++);

  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  Serial.printf("TX sent: %s\n", msg);
  delay(3000); 
}
