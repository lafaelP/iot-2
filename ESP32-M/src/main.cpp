#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Replace with the MAC address of ESP32-A
uint8_t esp32_A_MAC[] = {0xC8, 0xF0, 0x9E, 0xF2, 0x2F, 0x20};

// Replace with the MAC address of ESP32-B
uint8_t esp32_B_MAC[] = {0x94, 0xB9, 0x7E, 0xDA, 0x73, 0x74};

// Structure for BME680 data
typedef struct struct_message_bme680 {
  float temperature;
  float humidity;
  float pressure;
} struct_message_bme680;

// Create a struct_message_bme680 called myData_bme680
struct_message_bme680 myData_bme680;

// Structure for INA219 data
typedef struct struct_message_ina219 {
  float busVoltage;
  float current;
  float power;
  float shuntVoltage;
} struct_message_ina219;

// Create a struct_message_ina219 called myData_ina219
struct_message_ina219 myData_ina219;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  // Check the MAC address of the sender
  if (memcmp(mac_addr, esp32_A_MAC, 6) == 0) {
    // Data received from ESP32-A (BME680)
    memcpy(&myData_bme680, incomingData, sizeof(myData_bme680));
  
    // Print the received data from ESP32-A (BME680)
    Serial.println("Data received from ESP32-A (BME680):");
    Serial.print("Temperature: ");
    Serial.print(myData_bme680.temperature);
    Serial.println(" °C");
    Serial.print("Humidity: ");
    Serial.print(myData_bme680.humidity);
    Serial.println(" %");
    Serial.print("Pressure: ");
    Serial.print(myData_bme680.pressure);
    Serial.println(" hPa");
  } else if (memcmp(mac_addr, esp32_B_MAC, 6) == 0) {
    // Data received from ESP32-B (INA219)
    memcpy(&myData_ina219, incomingData, sizeof(myData_ina219));
  
    // Print the received data from ESP32-B (INA219)
    Serial.println("Data received from ESP32-B (INA219):");
    Serial.print("Bus Voltage: ");
    Serial.print(myData_ina219.busVoltage);
    Serial.println(" V");
    Serial.print("Current: ");
    Serial.print(myData_ina219.current);
    Serial.println(" mA");
    Serial.print("Power: ");
    Serial.print(myData_ina219.power);
    Serial.println(" mW");
    Serial.print("Shunt Voltage: ");
    Serial.print(myData_ina219.shuntVoltage);
    Serial.println(" mV");
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packet info
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  // Nothing to do here
}
