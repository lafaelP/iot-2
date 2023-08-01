#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiClient.h>

uint8_t esp32_M_MAC[] = {0xF4, 0xCF, 0xA2, 0xA8, 0xCC, 0x2C}; // ESP32-M MAC address

const char* ssid = "WIR-Guest";
const char* password = "Guest@WIRgroup";

typedef struct struct_message_bme680 {
  float temperature;
  float humidity;
  float pressure;
} struct_message_bme680;

// Create a struct_message called myData
struct_message_bme680 myData_bme680;

const int ledPin = 2; // GPIO pin connected to the LED

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (memcmp(mac, esp32_M_MAC, 6) == 0) {
    // Data received from ESP32-M (BME680)
    memcpy(&myData_bme680, incomingData, sizeof(myData_bme680));

    // Print the received temperature from ESP32-M (BME680)
    Serial.print("Received Temperature: ");
    Serial.println(myData_bme680.temperature);

    // Check if the received temperature is above 24 degrees Celsius
    if (myData_bme680.temperature > 24.0) {
      digitalWrite(ledPin, HIGH); // Turn on the LED
    } else {
      digitalWrite(ledPin, LOW); // Turn off the LED
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT); // Set the LED pin as OUTPUT

  WiFi.mode(WIFI_AP_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("esp now connected");
  esp_now_register_recv_cb(OnDataRecv); // Register the callback for receiving data
}

void loop() {
  // Do any other tasks here if needed...
}
