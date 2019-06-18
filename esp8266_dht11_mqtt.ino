#include "DHT.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
/*
libs_links:
https://github.com/knolleary/pubsubclient
https://github.com/bblanchon/ArduinoJson
https://github.com/adafruit/DHT-sensor-library
https://github.com/adafruit/Adafruit_Sensor
https://github.com/arduino-libraries/NTPClient
*/

#define DHTPIN 5      // what digital pin we're connected to NodeMCU (D1)
#define DHTTYPE DHT22 // DHT 22

#define wifi_ssid "WIFI_TEST"
#define wifi_password "bpsguest"

#define mqtt_server "broker.hivemq.com"
// string mqtt_user = "user"
// string mqtt_password  = "password"

#define humidity_topic "sensor/humidity"
#define temperature_celsius_topic "sensor/temperature_celsius"
#define temperature_fahrenheit_topic "sensor/temperature_fahrenheit"

#define device "camera" // name of the device

/*###topics###*/
//#define telemetry "sander/"+device
#define telemetry "sander/camera"
/*###topics###*/

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  timeClient.begin();
  timeClient.setTimeOffset(7200); // set timezone = GMT+2
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

/*
void jsonComposer(float h, float t, float hic)
{

  const intcapacity=JSON_OBJECT_SIZE(3);StaticJsonDocument<capacity>doc;







    //StaticJsonBuffer<300> JSONbuffer;

    //JsonObject &jsonMsg = JSONbuffer.createObject(); //arduinojson V5
    JsonObject jsonMsg = JSONbuffer.createObject(); //arduinojson V6
    jsonMsg["device"] = device;

    JsonObject& dht11_1 = jsonMsg.createNestedObject("dht11_1");
    dht11_1.set("humidity", h);
    dht11_1.set("temperature", t);
    dht11_1.set("heat index", hic);

    //JsonObject& dht11_1 = jsonMsg.createNestedObject("dht11_2");
    //dht11_2.set("humidity", h);
    //dht11_2.set("temperature", t);
    //dht11_2.set("heat index", hic);


    jsonMsg.printTo(Serial);

}
*/

String readDht() {
  
  String msg = "null";
  timeClient.update();
  String time = timeClient.getFormattedTime();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    msg = "Failed to read from DHT sensor!";

  } else {
    float hic = dht.computeHeatIndex(t, h, false);

    msg = ("Time: " + time + ", Humidity: " + String(h).c_str() + "%, Temperature: " + String(t).c_str() + "C, Heat index: " + String(hic).c_str());
    //msg = ("Time: " + time + ", Humidity: " + h_s + ", Temperature: " + t_s + ", Heat index: " + hic_s);
  }

  return msg;
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //implement to prevent delay() if((Psec - Csec)>5){send msg}
  String msg = readDht();
  Serial.println(msg);
  client.publish(telemetry, String(msg).c_str(), true);

  delay(5000);
}
