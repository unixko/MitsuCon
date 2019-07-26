
# Classic Controller

This code is proven to working well on Home Assistant <=0.96. It has 2 parts:

* arduino sketch for ESP8266/ESP32 board
* Home Assistant custom component file to place in Home Assistant server

Chris Davis has a very detailed instruction blog:

https://chrdavis.github.io/hacking-a-mitsubishi-heat-pump-Part-1/

https://chrdavis.github.io/hacking-a-mitsubishi-heat-pump-Part-2/

## Arduino
This sketch supports ESP8266 and ESP32 board. Modify your credentials like WiFi password and MQTT server in mitsubishi_heatpump_mqtt_esp8266_esp32.h first and then compile and flash mitsubishi_heatpump_mqtt_esp8266_esp32.ino to baord.

## Home Assistant
You need MQTT Server like Mosquito broker add-ons.

### Installation
Copy custom_components folder to home assistant config directory.
Resulting folder structure should be <home assistant config directory>/custom_components/mitsubishi_mqtt/climate.py

Add the following to your configuration.yaml:
```c++
climate:
   - platform: mitsubishi_mqtt
     name: "Mistubishi Heatpump"
     command_topic: "heatpump/set"
     temperature_state_topic: "heatpump/status"
     state_topic: "heatpump"

```
or for device not support HEAT mode, modes is optional
```c++
climate:
   - platform: mitsubishi_mqtt
     name: "Mistubishi Heatpump"
     modes:
      - "AUTO"
      - "COOL"
      - "DRY"
      - "FAN"
      - "OFF"
     command_topic: "heatpump/set"
     temperature_state_topic: "heatpump/status"
     state_topic: "heatpump"

```
