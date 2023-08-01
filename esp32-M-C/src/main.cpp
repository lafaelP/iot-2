#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <PubSubClient.h>

uint8_t esp32_A_MAC[] = {0xC8, 0xF0, 0x9E, 0xF2, 0x2F, 0x20};
uint8_t esp32_B_MAC[] = {0x94, 0xB9, 0x7E, 0xDA, 0x73, 0x74};
uint8_t esp32_C_MAC[] = {0xB0, 0xB2, 0x1C, 0xA8, 0x92, 0x54};

const char* ssid = "WIR-Guest";
const char* password = "Guest@WIRgroup";

const char* MQTT_BROKER = "172.25.20.88"; 
const int MQTT_PORT = 1883; 
const char* MQTT_USER = "lafaelpiMQTT";      
const char* MQTT_PASSWORD = "lafaelpiMQTT";  

#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "WsDF2EMLG0BX27qAxysiWA3t4VTPka2XGp2krlne5vbD5HPjpC4PSlGqDiZf3hF6WZ5zQIOys1bdC7H93q0OLw=="
#define INFLUXDB_ORG "a11b07c582b5ad36"
#define INFLUXDB_BUCKET "iot-2"
#define TZ_INFO "WET0WEST,M3.5.0/1,M10.5.0"
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensorBMEReadings("BME measurements");
Point sensorINAReadings("INA measurements");

typedef struct struct_message_bme680 {
  float temperature;
  float humidity;
  float pressure;
} struct_message_bme680;

// Create a struct_message called myData
struct_message_bme680 myData_bme680;

float last_temperature, last_humidity, last_pressure, last_temp_C;
esp_now_peer_info_t peerInfo;

// Structure for INA219 data
typedef struct struct_message_ina219 {
  float busVoltage;
  float current;
  float power;
  float shuntVoltage;
} struct_message_ina219;

// Create a struct_message_ina219 called myData_ina219
struct_message_ina219 myData_ina219;

float last_busVoltage, last_current, last_power, last_shuntVoltage;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char* MQTT_TOPIC = "esp32_data";

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

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    if (memcmp(mac, esp32_A_MAC, 6) == 0) {
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

  
  } else if (memcmp(mac, esp32_B_MAC, 6) == 0) {
    // Data received from ESP32-B (INA219)
    memcpy(&myData_ina219, incomingData, sizeof(myData_ina219));

    //Print the received data from ESP32-B (INA219)
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

void setup()
{
    Serial.begin(9600);
    WiFi.mode(WIFI_AP_STA);
    int32_t channel = getWiFiChannel(ssid);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Setting as a Wi-Fi Station..");
    }
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel());

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    if (client.validateConnection())
    {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client.getServerUrl());
    }
    else
    {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client.getLastErrorMessage());
    }

if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
    esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, esp32_C_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("esp now connected");
  esp_now_register_recv_cb(OnDataRecv); // Register the callback for receiving data

}

void loop()
{
    if (last_temperature != myData_bme680.temperature || last_humidity != myData_bme680.humidity || last_pressure != myData_bme680.pressure)
    

    {     
        sensorBMEReadings.addField("Temperature", myData_bme680.temperature);
        sensorBMEReadings.addField("Humidity", myData_bme680.humidity);
        sensorBMEReadings.addField("Pressure", myData_bme680.pressure);

        Serial.print("Writing: ");
        Serial.println(client.pointToLineProtocol(sensorBMEReadings));
        Serial.println();

        client.writePoint(sensorBMEReadings);
        sensorBMEReadings.clearFields();

    

        String jsonMessage = "{\"temperature\": " + String(myData_bme680.temperature) +
                         ", \"humidity\": " + String(myData_bme680.humidity) +
                         ", \"pressure\": " + String(myData_bme680.pressure) + "}";

        char charMessage[jsonMessage.length() + 1];
        jsonMessage.toCharArray(charMessage, sizeof(charMessage));

        mqttClient.publish(MQTT_TOPIC, charMessage);
    
    } else if (last_busVoltage != myData_ina219.busVoltage || last_current != myData_ina219.current || last_power != myData_ina219.power || last_shuntVoltage != myData_ina219.shuntVoltage)
      {
        sensorINAReadings.addField("busVoltage", myData_ina219.busVoltage);
        sensorINAReadings.addField("Current", myData_ina219.current);
        sensorINAReadings.addField("Power", myData_ina219.power);
        sensorINAReadings.addField("ShuntVoltage", myData_ina219.shuntVoltage);

        Serial.print("Writing: ");
        Serial.println(client.pointToLineProtocol(sensorINAReadings));
        Serial.println();

        client.writePoint(sensorINAReadings);
        sensorINAReadings.clearFields();

        String jsonMessage = "{\"busVoltage\": " + String(myData_ina219.busVoltage) +
                         ", \"current\": " + String(myData_ina219.current) +
                         ", \"power\": " + String(myData_ina219.power) +
                         ", \"shuntVoltage\": " + String(myData_ina219.shuntVoltage) + "}";

        char charMessage[jsonMessage.length() + 1];
        jsonMessage.toCharArray(charMessage, sizeof(charMessage));
        mqttClient.publish(MQTT_TOPIC, charMessage);
      }
    last_temperature = myData_bme680.temperature;
    last_humidity = myData_bme680.humidity;
    last_pressure = myData_bme680.pressure;
    last_busVoltage = myData_ina219.busVoltage;
    last_current = myData_ina219.current;
    last_power = myData_ina219.power;
    last_shuntVoltage = myData_ina219.shuntVoltage;

    if (last_temp_C != last_temperature)
    {// Send message via ESP-NOW
  esp_err_t result = esp_now_send(esp32_C_MAC, (uint8_t *)&myData_bme680, sizeof(myData_bme680));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(1000);}

  last_temp_C = last_temperature;



    
  //   if (!mqttClient.connected()) {
  //   Serial.println("Reconnecting to MQTT...");
  //   if (mqttClient.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) {
  //     Serial.println("Connected to MQTT broker.");
  //   } else {
  //     Serial.print("Failed to connect to MQTT broker. State: ");
  //     Serial.println(mqttClient.state());
  //     delay(2000);
  //     return;
  //   }
  // }

  // mqttClient.loop();
}