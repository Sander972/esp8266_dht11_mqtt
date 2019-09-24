//#define MQTT_MAX_PACKET_SIZE 512
#include <ArduinoJson.h>                    //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>                      //https://github.com/arduino-libraries/NTPClient
#include <PubSubClient.h>                   //https://github.com/knolleary/pubsubclient
#include <WiFiUdp.h>
#include <Wire.h>

#include "Adafruit_BME680.h"                //https://github.com/adafruit/Adafruit_BME680
#include "DHT.h"                            //https://github.com/adafruit/DHT-sensor-library
#include <Adafruit_Sensor.h>                //https://github.com/adafruit/Adafruit_Sensor
#include <SPI.h>
#include "secret.h"

#define DHTPIN_1 13   // D7
#define DHTPIN_2 15   // D8
#define DHTTYPE DHT22 // DHT 22
// voc sensor --> SCL-D1   SDA-D2

#define SEALEVELPRESSURE_HPA (1013.25)

DHT dht_1(DHTPIN_1, DHTTYPE);
DHT dht_2(DHTPIN_2, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Adafruit_BME680 bme;
ESP8266WiFiMulti wifiMulti;

void setup() {
  Serial.begin(115200);
  setup_wifi();

  client.setServer(mqtt_server, 1883);
  timeClient.begin();
  timeClient.setTimeOffset(7200); // set timezone = GMT+2

  pinMode(BUILTIN_LED, OUTPUT);

  dht_1.begin();
  dht_2.begin();

  bme.begin(0x76);
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void scanWifi() {
  Serial.println("Scanning WiFis");

  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  scanWifi();
  wifiMulti.addAP(ssid_palazzetti, pws_palazzetti);
  wifiMulti.addAP(ssid_casa, pws_casa);
  wifiMulti.addAP(ssid_hotspot, pws_hotspot);

  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientName;
    clientName += "esp8266-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += macToStr(mac);
    clientName += "-";
    clientName += String(micros() & 0xff, 16);
    Serial.print("Connecting to ");
    Serial.print(mqtt_server);
    Serial.print(" as ");
    Serial.println(clientName);

    //if (client.connect((char*)clientName.c_str(), mqtt_user, mqtt_password)) {      //with pws
    if (client.connect((char*)clientName.c_str())) {                                //without pws
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String jsonComposer() {

  // timeClient.update();
  String time = timeClient.getFormattedTime();

  float t1 = dht_1.readTemperature();
  float h1 = dht_1.readHumidity();
  // float hic1 = dht_1.computeHeatIndex(t1, h1, false);

  float t2 = dht_2.readTemperature();
  float h2 = dht_2.readHumidity();
  // float hic2 = dht_2.computeHeatIndex(t2, h2, false);

  bme.performReading();
  float t3 = bme.temperature;
  // float h3 = bme.humidity;
  // float pre = (bme.pressure / 100.0);
  // float gasr = (bme.gas_resistance / 1000.0);
  // float alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
  // String(msg).c_str()

  const int capacity = JSON_OBJECT_SIZE(35); // how much entries are in the json
  StaticJsonDocument<capacity> msg;

  msg["device"].set(device);
  msg["time"].set(time);

  JsonObject dht22_1 = msg.createNestedObject("dht22_1");
  if (isnan(t1)) {
    dht22_1["temperature"].set(0);
    dht22_1["humidity"].set(0);
    dht22_1["heat_index"].set(0);
  } else {
    dht22_1["temperature"].set(t1);
    dht22_1["humidity"].set(h1);
    dht22_1["heat_index"].set(dht_1.computeHeatIndex(t1, h1, false));
  }

  JsonObject dht22_2 = msg.createNestedObject("dht22_2");
  if (isnan(t2)) {
    dht22_2["temperature"].set(0);
    dht22_2["humidity"].set(0);
    dht22_2["heat_index"].set(0);
  } else {
    dht22_2["temperature"].set(t2);
    dht22_2["humidity"].set(h1);
    dht22_2["heat_index"].set(dht_2.computeHeatIndex(t2, h2, false));
  }

  JsonObject bme680 = msg.createNestedObject("bme680");
  if (isnan(t3)) {
    bme680["temperature"].set(0);
    bme680["humidity"].set(0);
    bme680["pressure"].set(0);
    bme680["gas_resistance"].set(0);
    bme680["circa_altitude"].set(0);
  } else {
    bme680["temperature"].set(t3);
    bme680["humidity"].set(bme.humidity);
    bme680["pressure"].set(bme.pressure / 100.0);
    bme680["gas_resistance"].set(bme.gas_resistance / 1000.0);
    bme680["circa_altitude"].set(bme.readAltitude(SEALEVELPRESSURE_HPA));
  }

  String json;
  serializeJson(msg, json);
  return json;
}

void loop() {

  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    // delay(1000);
  }

  timeClient.update();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  String msg = jsonComposer();
  Serial.println(msg);

  int ok = client.publish(telemetry, msg.c_str(), true);

  if (ok) {
    // Serial.println(msg);
    Serial.println("Publish ok");
    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    Serial.println("err publishing");
    digitalWrite(BUILTIN_LED, LOW);
  }

  delay(5000);
}
