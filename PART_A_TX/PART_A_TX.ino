// === PART A: NODE 1 (TRANSMITTER) — Ra-02 (SX1278) BENCH LINK TEST ===
// Hardware adjustment: Ai-Thinker Ra-02 (SX1278) operating at 433 MHz (433E6)
// CRITICAL RF SAFETY: Attach a 433 MHz antenna to the Ra-02 BEFORE powering on!

#include <SPI.h>
#include <LoRa.h>

// ESP32-DevKitC (VSPI: SCK=18, MISO=19, MOSI=23) <-> Ra-02 (SX1278)
#define SS_PIN    5     // Ra-02 NSS
#define RST_PIN   14    // Ra-02 RESET
#define DIO0_PIN  26     // Ra-02 DIO0 (use GPIO 26 if GPIO 2 affects upload)

#define LORA_FREQ 433.2E6 // 433.0 MHz for Ra-02 (SX1278 410-525 MHz band)

uint32_t counter = 0;

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
