// Part A: RX

#include <SPI.h>
#include <LoRa.h>

#define SS_PIN    5
#define RST_PIN   14
#define DIO0_PIN  2

#define LORA_FREQ 433E6

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

void decodeAndShow(const uint8_t* buf, int n) {
  Serial.print("Payload: ");
  for (int i = 0; i < n; i++) {
    Serial.print((char)buf[i]);
  }
  Serial.println();
}

void receiveLoop() {
  int size = LoRa.parsePacket();
  if (!size) return;

  uint8_t buf[64];
  int n = 0;
  while (LoRa.available() && n < (int)sizeof(buf)) {
    buf[n++] = LoRa.read();
  }
  while (LoRa.available()) {
    LoRa.read();
  }

  Serial.printf("rx %d bytes rssi=%d snr=%.1f\n",
                n, LoRa.packetRssi(), LoRa.packetSnr());
  decodeAndShow(buf, n);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Part A: Ra-02 (SX1278) LoRa RX Waiting for packets...");
  radioBegin();
  Serial.println("Radio Init OK (433 MHz, SF7, BW 125kHz, CR 4/5, Sync 0x34)");
}

void loop() {
  receiveLoop();
}
