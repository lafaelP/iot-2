#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define LED 2

constexpr char WIFI_SSID[] = "WIR-Guest";

uint8_t esp32_M_MAC[] = {0xF4, 0xCF, 0xA2, 0xA8, 0xCC, 0x2C}; 

float temp;

typedef struct struct_message_bme680 {
  float temperature;
} struct_message_bme680;

struct_message_bme680 incomingReading;
esp_now_peer_info_t peerInfo;

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

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&incomingReading, incomingData, sizeof(incomingReading));
    temp = incomingReading.temperature;
    Serial.print("Received temperature: ");
    Serial.println(temp);
    if(temp > 24.00){
      digitalWrite(LED, HIGH);
      Serial.println("LED is on");
    } else {
      digitalWrite(LED, LOW);
      Serial.println("LED is off");
    }
}

void initESPnow() {
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }

    memcpy(peerInfo.peer_addr, esp32_M_MAC, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
            
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    }

    esp_now_register_recv_cb(OnDataRecv);
}

void setup() {
    Serial.begin(9600);
    pinMode(LED, OUTPUT);

    WiFi.mode(WIFI_STA);
    int32_t channel = getWiFiChannel(WIFI_SSID);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    initESPnow();
}

void loop() {
}