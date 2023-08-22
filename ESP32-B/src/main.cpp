#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Adafruit_INA219.h>

constexpr char WIFI_SSID[] = "WIR-Guest";

uint8_t esp32_M_MAC[] = {0xF4, 0xCF, 0xA2, 0xA8, 0xCC, 0x2C};

const long interval = 30000;
long previousMillis = 0;

typedef struct struct_message_ina219 {
  float busVoltage;
  float current;
  float power;
  float shuntVoltage;
} struct_message_ina219;

struct_message_ina219 myData_ina219;
esp_now_peer_info_t peerInfo;
Adafruit_INA219 ina219;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

void initESPnow() {
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }

    esp_now_register_send_cb(OnDataSent);

    memcpy(peerInfo.peer_addr, esp32_M_MAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
      return;
    }
}

void initINA219() {
    if (!ina219.begin()) {
      Serial.println("Failed to find INA219 chip");
      while (1);
    }
}

void readDataSensor() {
    float busVoltage = ina219.getBusVoltage_V();
    float current = ina219.getCurrent_mA();
    float power = ina219.getPower_mW();
    float shuntVoltage = ina219.getShuntVoltage_mV();

    // Set values to send
    myData_ina219.busVoltage = busVoltage;
    myData_ina219.current = current;
    myData_ina219.power = power;
    myData_ina219.shuntVoltage = shuntVoltage;

    Serial.println("INA219 data: ");
    Serial.print("Bus Voltage: ");
    Serial.print(busVoltage);
    Serial.print(", Current: ");
    Serial.print(current);
    Serial.print(", Power: ");
    Serial.print(power);
    Serial.print(", ShuntVoltage: ");
    Serial.println(shuntVoltage);
}

void setup() {
    Serial.begin(9600);

    WiFi.mode(WIFI_STA);
    int32_t channel = getWiFiChannel(WIFI_SSID);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    initESPnow();

    initINA219();
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        readDataSensor();

        esp_err_t result = esp_now_send(esp32_M_MAC, (uint8_t *)&myData_ina219, sizeof(myData_ina219));

        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }
    }
}