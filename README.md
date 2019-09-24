# esp8266_dht11_mqtt

A client that upload to broker via mqtt messages readings from 2 dht11 sensors, in the future I will add a VOC sensor

## Installation

Remember to create a "secret.h" file containing all pws.

### Example:

```sh
#define device "device_name" 
#define telemetry "topic/topic"
#define mqtt_server "broker.com" 
const char*  ssid = "***********"; 
const char*  pws = "***********"; 
```

