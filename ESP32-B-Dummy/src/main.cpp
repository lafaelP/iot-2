#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_INA219.h>

// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t receiverAddress[] = {0xF4, 0xCF, 0xA2, 0xA8, 0xCC, 0x2C};

// Structure example to send INA219 data
typedef struct struct_message_ina219 {
  float busVoltage;
  float current;
  float power;
  float shuntVoltage;
} struct_message_ina219;

// Create a struct_message_ina219 called myData_ina219
struct_message_ina219 myData_ina219;

// Create peer interface
esp_now_peer_info_t peerInfo;

// INA219 sensor object
Adafruit_INA219 ina219;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  // Init Serial Monitor
  Serial.begin(9600);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Initialize INA219 sensor
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1);
  }
}

void loop() {
  // Read INA219 sensor data
  float busVoltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA();
  float power = ina219.getPower_mW();
  float shuntVoltage = ina219.getShuntVoltage_mV();

  // Set values to send
  myData_ina219.busVoltage = busVoltage;
  myData_ina219.current = current;
  myData_ina219.power = power;
  myData_ina219.shuntVoltage = shuntVoltage;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData_ina219, sizeof(myData_ina219));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  delay(10000);
}
