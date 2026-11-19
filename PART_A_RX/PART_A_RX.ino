// === PART A: NODE 2 (RECEIVER) — Ra-02 (SX1278) BENCH LINK TEST ===
// Hardware adjustment: Ai-Thinker Ra-02 (SX1278) operating at 433 MHz (433E6)
// CRITICAL RF SAFETY: Attach a 433 MHz antenna to the Ra-02 BEFORE powering on!

#include <SPI.h>
#include <LoRa.h>

// ESP32-DevKitC (VSPI: SCK=18, MISO=19, MOSI=23) <-> Ra-02 (SX1278)
#define SS_PIN    5     // Ra-02 NSS
#define RST_PIN   14    // Ra-02 RESET
#define DIO0_PIN  2     // Ra-02 DIO0 (use GPIO 26 if GPIO 2 affects upload)

#define LORA_FREQ 433E6 // 433.0 MHz for Ra-02 (SX1278 410-525 MHz band)

void radioBegin() {
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("radio init failed");
    while (1);
  }
  LoRa.setSpreadingFactor(7);       // SF7 — both nodes must match
  LoRa.setSignalBandwidth(125E3);   // 125 kHz — both nodes must match
  LoRa.setCodingRate4(5);           // CR 4/5 — both nodes must match
  LoRa.setSyncWord(0x34);           // Sync word 0x34 — both nodes must match
  LoRa.setTxPower(14);              // 14 dBm
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
    LoRa.read(); // Flush any remaining bytes if packet exceeded buffer
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
