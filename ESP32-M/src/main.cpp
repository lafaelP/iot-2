#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
 # include <HTTPClient.h>

WiFiMulti wifiMulti;
// WiFi AP SSID
#define WIFI_SSID "WIR-Guest"
// WiFi password
#define WIFI_PASSWORD "Guest@WIRgroup"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "WsDF2EMLG0BX27qAxysiWA3t4VTPka2XGp2krlne5vbD5HPjpC4PSlGqDiZf3hF6WZ5zQIOys1bdC7H93q0OLw=="
#define INFLUXDB_ORG "a11b07c582b5ad36"
#define INFLUXDB_BUCKET "iot-2"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "WET0WEST,M3.5.0/1,M10.5.0"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point for both BME680 and INA219 sensors
Point sensorReadings("measurements");

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

    // Add readings as fields to point
    sensorReadings.clearFields(); // Clear previous fields
    sensorReadings.addTag("device", "ESP32");
    sensorReadings.addTag("sensor", "BME680");
    sensorReadings.addField("temperature", myData_bme680.temperature);
    sensorReadings.addField("humidity", myData_bme680.humidity);
    sensorReadings.addField("pressure", myData_bme680.pressure);
    
    // Write point into buffer
    client.writePoint(sensorReadings);

  } else if (memcmp(mac_addr, esp32_B_MAC, 6) == 0) {
    // Data received from ESP32-B (INA219)
    memcpy(&myData_ina219, incomingData, sizeof(myData_ina219));

    // Add readings as fields to point
    sensorReadings.clearFields(); // Clear previous fields
    sensorReadings.addTag("device", "ESP32");
    sensorReadings.addTag("sensor", "INA219");
    sensorReadings.addField("busVoltage", myData_ina219.busVoltage);
    sensorReadings.addField("current", myData_ina219.current);
    sensorReadings.addField("power", myData_ina219.power);
    sensorReadings.addField("shuntVoltage", myData_ina219.shuntVoltage);

    // Write point into buffer
    client.writePoint(sensorReadings);
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
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packet info
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  // Nothing to do here
}
