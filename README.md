# MitsuCon
Mitsubishi Heat Pump (Air Conditioner) Controller for Home Assistant

*New version with hvac_action. Plase increase PubSubClient.h MQTT_MAX_PACKET_SIZE to 2048.

![Wemos D1 Mini Adapter](https://user-images.githubusercontent.com/44964969/51798270-c3392980-2242-11e9-8986-cffc5fe4d287.jpg)

Build ESP-8266 device to interface between Mitsubishi Electric heat pump and Home Assistant. This code utilizes Home Assistant native MQTT HVAC component with MQTT discovery feature so it should be compatible with future Home Assistant updates and will be added to Home Assistant automatically without any configuration.

## Hardware
Tested on Wemos D1 mini board with Home Assistant 0.96 and Mosquitto broker add-on 5.0.

## Build a code
Using Arduino IDE to modify configuration and complie file for your hardware.

Arduino IDE 1.8.10
ArduinoJson 6.12.0
PubSubClient 2.7.0

Edit file Arduino\libraries\PubSubClient\PubSubClient.h MQTT_MAX_PACKET_SIZE to 2048.

## Known Issues
* Sometimes heat pump unit turns off instead when send temperature changing command.
* When change setting on HA UI, change will effect immediately but UI will delay for next data pull to refresh new status.
* Cannot use imperial (fahrenheit) unit. Waiting for Home Assistant support soon.

## Classic Code
The Classic folder is a code proven to working well on Home Assistant < 0.100.0. It has 2 parts:

* arduino sketch for ESP8266/ESP32 board
* Home Assistant custom component file to place in Home Assistant server

Chris Davis has a very detailed instruction blog:

https://chrdavis.github.io/hacking-a-mitsubishi-heat-pump-Part-1/

https://chrdavis.github.io/hacking-a-mitsubishi-heat-pump-Part-2/

## Other Projects
Interesting projects developed with different design choices: 

WiFi manager and web interface
https://github.com/gysmo38/mitsubishi2MQTT

Enhanced WiFi manager, web interface and mroe functions
https://github.com/dzungpv/mitsubishi2MQTT

Added Platform IO, Added SSL, Added an RGB LED
https://github.com/SittingDuc/MitsuCon


## Credits
Thanks to all contributors especially:
* lekobob https://github.com/lekobob/mitsu_mqtt
* SwiCago https://github.com/SwiCago/HeatPump
* Hadley  https://nicegear.co.nz/blog/hacking-a-mitsubishi-heat-pump-air-conditioner/

*Mitsubishi means Mitsubishi Electric, not Mitsubishi Heavy Industries.
