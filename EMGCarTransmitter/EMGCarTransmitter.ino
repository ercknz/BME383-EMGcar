// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// COMMUNICATION LIBRARIES AND DEFINITIONS (Do not modify unless you have to!)
#include <WiFi.h>
#include <esp_now.h>

#define ESPNOW_WIFI_CHANNEL 6
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// =============================================================================
// STUDENT'S ADDITIONAL LIBRARIES AND DEFINITIONS HERE:

// =============================================================================

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// COMMUNICATION PACKET SET UP (Do not modify unless you have to!)
// --- Data packet structure — must match the TRANSMITTER exactly ---
struct ControlPacket {
  uint16_t SensorVal1;   // 0 → 4095
  uint16_t SensorVal2;   // 0 - 4095
};

// --- Communication Global Variables ---
ControlPacket packet;
uint16_t sensorValue1;
uint16_t sensorValue2;

// --- Data Send Callback send ---
void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "    Sent" : "    FAILED");
}
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// =============================================================================
// STUDENT'S GLOBAL VARIABLES:
uint8_t receiverMAC[] = {0x1C, 0xDB, 0xD4, 0x78, 0xA6, 0xB0}; // ← REPLACE THIS
int sensorVal1;
int sensorVal2;
int biopac1Pin = A0;
int biopac2Pin = A1;
// =============================================================================

void setup() {
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // COMMUNICATION SET UP (Do not modify unless you have to!)
  Serial.begin(115200);
  Serial.println("=== TRANSMITTER starting ===");

  // --- Starts Wifi in station mode and prints MAC address ---
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }
  Serial.print("Transmitter MAC: ");
  Serial.println(WiFi.macAddress());

  // --- Initialise ESP-NOW ---
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed. Restart the board.");
    while (true) delay(10000);
  }
  // --- Register send callback ---
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  // --- Register the receiver as a peer ---
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;    // 0 = use current channel
  peerInfo.encrypt = false; // No encryption

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add receiver as peer.");
    Serial.println("Make sure receiverMAC[] is set correctly.");
    while (true);
  }

  Serial.println("ESP-NOW ready. Sending data...");
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


  // =============================================================================
  // STUDENT'S SETUP() CODE STARTS HERE:

  // =============================================================================
}

void loop() {
  // =============================================================================
  // STUDENTS'S MAIN LOOP() CODE STARTS HERE:

  sensorVal1 = analogRead(biopac1Pin);
  sensorVal2 = analogRead(biopac2Pin);

  // Print data
  Serial.print("Sensor1:"); 
  Serial.print(sensorVal1);
  Serial.print(", Sensor2: "); 
  Serial.print(sensorVal2);

  //=============================================================================

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // PACKET COMMUNICATION (Do not modify unless you have to!)
  // --- Clamp to valid range ---
  packet.SensorVal1 = constrain(sensorVal1, 0, 4095);
  packet.SensorVal2 = constrain(sensorVal2, 0, 4095);

  // --- Send packet to receiver ---
  esp_now_send(receiverMAC, (uint8_t *)&packet, sizeof(packet));
  delay(250);
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
}
