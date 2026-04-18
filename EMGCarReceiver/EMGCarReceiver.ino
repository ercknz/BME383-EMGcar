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

// --- Communcation Global variables ---
volatile ControlPacket latestPacket = {0, 2048}; // Default: stopped, straight
volatile bool newDataAvailable = false;
unsigned long lastReceiveTime  = 0;
const int SIGNAL_TIMEOUT_MS = 5000;
uint16_t sensorValue1;
uint16_t sensorValue2;


// --- ESP-NOW receive callback ---
void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len == sizeof(ControlPacket)) {
    memcpy((void *)&latestPacket, data, sizeof(ControlPacket));
    newDataAvailable = true;
    lastReceiveTime  = millis();
    digitalWrite(LED_BUILTIN,HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN,LOW);
    delay(100);
  }
}
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// =============================================================================
// STUDENT'S GLOBAL VARIABLES:
// Tank drive Pins
int leftMotorPin1 = A0;
int leftMotorPin2 = A1;

int rightMotorPin1 = A2;
int rightMotorPin2 = A3;

int leftMotorSpeed = 0;
int rightMotorSpeed = 0;
int steering = 0;

int EMGcap = 2000;
int lastSpeed = 0;
float alpha = 0.75f;
int speedValue = 0;
// =============================================================================

void setup() {
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // COMMUNICATION SETUP() (Do not modify unless you have to!)
  Serial.begin(115200);
  Serial.println("=== RECEIVER starting ===");
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);

  // --- Starts Wifi and print this board's MAC address --
  WiFi.mode(WIFI_MODE_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }
  Serial.print(">>> RECEIVER MAC ADDRESS (copy into transmitter code): ");
  Serial.println(WiFi.macAddress()); // Copy this mac address to your transmitter code

  // --- Initialise ESP-NOW ---
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed. Restart the board.");
    while (true);
  }
  esp_now_register_recv_cb(onDataReceived);

  Serial.println("Waiting for transmitter...");
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


  // =============================================================================
  // STUDENT'S SETUP() CODE STARTS HERE:
  // Set up Left and Right Motor Pins
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);

  // Set up direction for motor
  digitalWrite(leftMotorPin1, LOW);
  digitalWrite(leftMotorPin2, LOW);
  digitalWrite(rightMotorPin1, LOW);
  digitalWrite(rightMotorPin2, LOW);
  // =============================================================================
}

void loop() {
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // COMMUNICATION SIGNAL LOST CHECK (Do not modify unless you have to!)
  if (millis() - lastReceiveTime > SIGNAL_TIMEOUT_MS && lastReceiveTime != 0) {
    Serial.println("⚠ Signal lost — motors stopped.");
    lastReceiveTime = millis(); // Prevent spamming the serial port
    return;
  }

  if (!newDataAvailable) return;
  newDataAvailable = false;
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  // =============================================================================
  // STUDENT'S MAIN LOOP() CODE STARTS HERE:
  sensorValue1 = latestPacket.SensorVal1;
  sensorValue2 = latestPacket.SensorVal2;

  // Exponential Filter
  speedValue = round(sensorValue1 * alpha + lastSpeed * (1.0 - alpha));
  lastSpeed = speedValue;
  
  // Control motor
  steering = map(sensorValue2,0,4095,-2047,2047);
  leftMotorSpeed = speedValue - steering - EMGcap;
  rightMotorSpeed = speedValue + steering - EMGcap;
  leftMotorSpeed = map(leftMotorSpeed, 2047 - EMGcap, 6142 - EMGcap, 0, 255);
  leftMotorSpeed = constrain(leftMotorSpeed, 0, 255);
  rightMotorSpeed = map(rightMotorSpeed, 2047 - EMGcap, 6142 - EMGcap, 0, 255);
  rightMotorSpeed = constrain(rightMotorSpeed, 0, 255);

  // Write to Motors
  analogWrite(leftMotorPin1, leftMotorSpeed);
  digitalWrite(leftMotorPin2, LOW);
  analogWrite(rightMotorPin1, rightMotorSpeed);
  digitalWrite(rightMotorPin2, LOW);

  // Print data
  Serial.print("Left: "); 
  Serial.print(leftMotorSpeed);
  Serial.print(", Right: "); 
  Serial.println(rightMotorSpeed);
  // =============================================================================
}
