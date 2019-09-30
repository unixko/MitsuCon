
# Classic Controller

TESTED with HA <=0.99.3 - added heat/cool mode and hvac_action.

This code is proven to working well on Home Assistant for years before HA developed native MQTT HVAC template. Since it creates a custom component directly in HA it works faster than native component. User interface is responsed almost instantly. However, a custom component aldo need to be maintained along each release of Home Assistant for frequently break changes.

Classic Controller has 2 parts:

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
if your device is not support HEAT mode, like air conditioner unit, define `modes` for supported modes.
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
