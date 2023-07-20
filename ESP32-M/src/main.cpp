#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <InfluxDbClient.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

// Define the Wi-Fi and MQTT settings
const char* WIFI_SSID = "WIR-Guest";
const char* WIFI_PASSWORD = "Guest@WIRgroup";
const char* MQTT_BROKER = "172.25.20.88"; // Replace with the IP address of your Raspberry Pi
const int MQTT_PORT = 1883; // Default MQTT port is 1883
const char* MQTT_USER = "lafaelpiMQTT";      // if you don't have MQTT Username, no need input
const char* MQTT_PASSWORD = "lafaelpiMQTT";  // if you don't have MQTT Password, no need input

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

// Wi-Fi client instance
WiFiClient wifiClient;

// MQTT client instance
PubSubClient mqttClient(wifiClient);

// MQTT topic to publish data
const char* MQTT_TOPIC = "esp32_data";

// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "WsDF2EMLG0BX27qAxysiWA3t4VTPka2XGp2krlne5vbD5HPjpC4PSlGqDiZf3hF6WZ5zQIOys1bdC7H93q0OLw=="
#define INFLUXDB_ORG "a11b07c582b5ad36"
#define INFLUXDB_BUCKET "bme_ina"
#define TZ_INFO "WET0WEST,M3.5.0/1,M10.5.0"
InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);


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

    // Write data to InfluxDB
    Point sensorReadings("measurements");
    sensorReadings.addTag("device", "ESP32");
    sensorReadings.addTag("sensor", "BME680");
    sensorReadings.addField("temperature", myData_bme680.temperature);
    sensorReadings.addField("humidity", myData_bme680.humidity);
    sensorReadings.addField("pressure", myData_bme680.pressure);
    influxClient.writePoint(sensorReadings);

   String jsonMessage = "{\"temperature\": " + String(myData_bme680.temperature) +
                         ", \"humidity\": " + String(myData_bme680.humidity) +
                         ", \"pressure\": " + String(myData_bme680.pressure) + "}";

    // Convert JSON message to char array
    char charMessage[jsonMessage.length() + 1];
    jsonMessage.toCharArray(charMessage, sizeof(charMessage));

    // Publish data to MQTT
    mqttClient.publish(MQTT_TOPIC, charMessage);
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

    // Write data to InfluxDB
    Point sensorReadings("measurements");
    sensorReadings.addTag("device", "ESP32");
    sensorReadings.addTag("sensor", "INA219");
    sensorReadings.addField("busVoltage", myData_ina219.busVoltage);
    sensorReadings.addField("current", myData_ina219.current);
    sensorReadings.addField("power", myData_ina219.power);
    sensorReadings.addField("shuntVoltage", myData_ina219.shuntVoltage);
    influxClient.writePoint(sensorReadings);

    // Publish data to MQTT
    String jsonMessage = "{\"busVoltage\": " + String(myData_ina219.busVoltage) +
                         ", \"current\": " + String(myData_ina219.current) +
                         ", \"power\": " + String(myData_ina219.power) +
                         ", \"shuntVoltage\": " + String(myData_ina219.shuntVoltage) + "}";

    // Convert JSON message to char array
    char charMessage[jsonMessage.length() + 1];
    jsonMessage.toCharArray(charMessage, sizeof(charMessage));

    // Publish data to MQTT
    mqttClient.publish(MQTT_TOPIC, charMessage);
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to Wi-Fi. IP address: ");
  Serial.println(WiFi.localIP());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set up MQTT client
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  // Optional: If your MQTT broker requires authentication, uncomment the next two lines and replace with your username and password
  // mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);

  // Check server connection
  if (influxClient.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(influxClient.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(influxClient.getLastErrorMessage());
  }

  // Once ESPNow is successfully initialized, we will register the callback for data reception
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // Maintain the MQTT connection
  if (!mqttClient.connected()) {
    Serial.println("Reconnecting to MQTT...");
    if (mqttClient.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Connected to MQTT broker.");
    } else {
      Serial.print("Failed to connect to MQTT broker. State: ");
      Serial.println(mqttClient.state());
      delay(2000);
      return;
    }
  }

  mqttClient.loop();
  // Add any other code you want to run in the loop...
}