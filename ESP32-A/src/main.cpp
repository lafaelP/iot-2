#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#define uS_TO_S_FACTOR 1000000  
#define TIME_TO_SLEEP  30       

constexpr char WIFI_SSID[] = "WIR-Guest";

uint8_t esp32_M_MAC[] = {0xF4, 0xCF, 0xA2, 0xA8, 0xCC, 0x2C};

typedef struct struct_message_bme680 {
  float temperature;
  float humidity;
  float pressure;
} struct_message_bme680;

struct_message_bme680 myData_bme680;
esp_now_peer_info_t peerInfo;
Adafruit_BME680 bme;

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

void initBME680(){
    if (!bme.begin()) {
      Serial.println("Failed to initialize BME680 sensor");
      while (1);
    }
}

void readDataSensor() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0;

    myData_bme680.temperature = temperature;
    myData_bme680.humidity = humidity;
    myData_bme680.pressure = pressure;

    Serial.println("BME680 data: ");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(", Humidity: ");
    Serial.print(humidity);
    Serial.print(", Pressure: ");
    Serial.println(pressure);
}


void setup() {
    Serial.begin(9600);

    WiFi.mode(WIFI_STA);
    int32_t channel = getWiFiChannel(WIFI_SSID);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    
    initESPnow();

    initBME680();
}

void loop() {
    readDataSensor();

    esp_err_t result = esp_now_send(esp32_M_MAC, (uint8_t *)&myData_bme680, sizeof(myData_bme680));

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    delay(1000);

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
    Serial.println("Going to sleep now");
    delay(1000);
    Serial.flush();
    esp_deep_sleep_start();
}
