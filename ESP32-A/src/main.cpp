#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30       /* Time ESP32 will go to sleep (in seconds) */

// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t receiverAddress[] = {0xF4, 0xCF, 0xA2, 0xA8, 0xCC, 0x2C};

// Structure example to send BME680 data
typedef struct struct_message_bme680 {
  float temperature;
  float humidity;
  float pressure;
} struct_message_bme680;

// Create a struct_message_bme680 called myData_bme680
struct_message_bme680 myData_bme680;

// Create peer interface
esp_now_peer_info_t peerInfo;

// BME680 sensor object
Adafruit_BME680 bme;

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
  // get the status of Trasnmitted packet
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

  // Initialize BME680 sensor
  if (!bme.begin()) {
    Serial.println("Failed to initialize BME680 sensor");
    while (1);
  }

}

void loop() {
  // Read BME680 sensor data
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0;

  // Set values to send
  myData_bme680.temperature = temperature;
  myData_bme680.humidity = humidity;
  myData_bme680.pressure = pressure;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData_bme680, sizeof(myData_bme680));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(1000);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");
  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
}
