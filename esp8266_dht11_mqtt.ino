#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>

#include "DHT.h"

#define DHTPIN 5 // what digital pin we're connected to NodeMCU (D1)

#define DHTTYPE DHT11 // DHT 11

DHT dht(DHTPIN, DHTTYPE);

#define wifi_ssid "NETGEAR52"
#define wifi_password "niftylake235"

#define mqtt_server "broker.hivemq.com"
//#define mqtt_user "user"
//#define mqtt_password "password"

#define humidity_topic "sensor/humidity"
#define temperature_celsius_topic "sensor/temperature_celsius"
#define temperature_fahrenheit_topic "sensor/temperature_fahrenheit"
#define telemetry "sander/camera"

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
    Serial.begin(115200);
    dht.begin();
    setup_wifi();
    client.setServer(mqtt_server, 1883);
}

String macToStr(const uint8_t *mac)
{
    String result;
    for (int i = 0; i < 6; ++i)
    {
        result += String(mac[i], 16);
        if (i < 5)
            result += ':';
    }
    return result;
}

void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);

    WiFi.begin(wifi_ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");

        // Generate client name based on MAC address and last 8 bits of microsecond counter
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
        if (client.connect((char *)clientName.c_str()))
        {
            //if (client.connect((char*) clientName.c_str()), mqtt_user, mqtt_password)) {
            Serial.println("connected");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void loop()
{

    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    // Wait a few seconds between measurements.
    delay(2000);

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f))
    {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);

    Serial.println("Humidity: %s % , Temperature: %s *C , Heat index: %s *C", h, t, hic);

    //String msg = ("Humidity: %s % , Temperature: %s *C , Heat index: %s *C", h, t, hic) ;
    //Serial.println(msg);

    client.publish(telemetry, String(t).c_str(), true);
    //client.publish(telemetry, String(msg).c_str(), true);

/*
    Serial.print("Temperature in Celsius:");
    Serial.println(String(t).c_str());
    client.publish(temperature_celsius_topic, String(t).c_str(), true);

    Serial.print("Temperature in Fahrenheit:");
    Serial.println(String(f).c_str());
    client.publish(temperature_fahrenheit_topic, String(f).c_str(), true);

    Serial.print("Humidity:");
    Serial.println(String(h).c_str());
    client.publish(humidity_topic, String(h).c_str(), true);
    */
}
