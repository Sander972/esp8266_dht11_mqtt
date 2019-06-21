//#define MQTT_MAX_PACKET_SIZE 512
#include "DHT.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <Wire.h>

#include "Adafruit_BME680.h"
#include <Adafruit_Sensor.h>
#include <SPI.h>

/*
libs_links:
https://github.com/knolleary/pubsubclient
https://github.com/bblanchon/ArduinoJson
https://github.com/adafruit/DHT-sensor-library
https://github.com/adafruit/Adafruit_Sensor
https://github.com/arduino-libraries/NTPClient
https://github.com/adafruit/Adafruit_BME680
*/

#define DHTPIN_1 13   // D7
#define DHTPIN_2 15   // D8
#define DHTTYPE DHT22 // DHT 22
//voc sensor --> SCL-D1   SDA-D2

#define wifi_ssid "NETGEAR52"
#define wifi_password "niftylake235"

#define mqtt_server "broker.hivemq.com"
//#define mqtt_server "192.168.101.234"
// string mqtt_user = "user"
// string mqtt_password  = "password"

#define device "camera" // name of the device

/*###topics###*/
//#define telemetry "sander/"+device
#define telemetry "sander/camera"
/*###topics###*/

#define SEALEVELPRESSURE_HPA (1013.25)

DHT dht_1(DHTPIN_1, DHTTYPE);
DHT dht_2(DHTPIN_2, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Adafruit_BME680 bme;

void setup() {
  Serial.begin(115200);
  dht_1.begin();
  dht_2.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  timeClient.begin();
  timeClient.setTimeOffset(7200); // set timezone = GMT+2
  pinMode(BUILTIN_LED, OUTPUT);

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

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Generate client name based on MAC address and last 8 bits of microsecond
    // counter
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

    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    if (client.connect((char*)clientName.c_str())) {
      // if (client.connect((char*) clientName.c_str()), mqtt_user,
      // mqtt_password)) {
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
    dht22_1["status"].set("err sensor dht22_1");
  } else {
    dht22_1["temperature"].set(t1);
    dht22_1["humidity"].set(h1);
    dht22_1["heat index"].set(dht_1.computeHeatIndex(t1, h1, false));
  }

  JsonObject dht22_2 = msg.createNestedObject("dht22_2");
  if (isnan(t2)) {
    dht22_2["status"].set("err sensor dht22_2");
  } else {
    dht22_2["temperature"].set(t2);
    dht22_2["humidity"].set(h1);
    dht22_2["heat index"].set(dht_2.computeHeatIndex(t2, h2, false));
  }

  JsonObject bme680 = msg.createNestedObject("bme680");
  if (isnan(t3)) {
    bme680["status"].set("err sensor VOC");
  } else {
    bme680["temperature"].set(t3);
    bme680["humidity"].set(bme.humidity);
    bme680["pressure"].set(bme.pressure / 100.0);
    bme680["gas resistance"].set(bme.gas_resistance / 1000.0);
    bme680["circa altitude"].set(bme.readAltitude(SEALEVELPRESSURE_HPA));
  }

  String json;
  serializeJson(msg, json);
  return json;
}

void loop() {

  timeClient.update();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // implement to prevent delay() if((Psec - Csec)>5){send(msg)}

  String msg = jsonComposer();
  //String msg = jsonComposer().c_str();
  Serial.println(msg);

  int ok = client.publish(telemetry, String(msg).c_str(), true);
  //int ok = client.publish(telemetry, msg, true);

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
