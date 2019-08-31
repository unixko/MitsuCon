# MitsuCon
Mitsubishi Heat Pump (Air Conditioner) Controller for Home Assistant

**Native code works with Home Assistant 0.98.1**

![Wemos D1 Mini Adapter](https://user-images.githubusercontent.com/44964969/51798270-c3392980-2242-11e9-8986-cffc5fe4d287.jpg)

Arduino project to build Mitsubishi heat pump controller for Home Assistant. It has 2 branches, classic and native.

## Classic
This version is proven to working well on Home Assistant <=0.96. It has 2 parts:

* arduino sketch for ESP8266/ESP32 board
* Home Assistant custom component file to place in Home Assistant server

Chris Davis has a very detailed instruction blog:

https://chrdavis.github.io/hacking-a-mitsubishi-heat-pump-Part-1/
https://chrdavis.github.io/hacking-a-mitsubishi-heat-pump-Part-2/

## Native: 
New development code using native MQTT HVAC component built-in Home Assistant, not required custom component.

This project is based on https://github.com/SwiCago/HeatPump library.

## Credits
Thanks to all contributors especially:
* lekobob https://github.com/lekobob/mitsu_mqtt
* SwiCago https://github.com/SwiCago/HeatPump
* Hadley  https://nicegear.co.nz/blog/hacking-a-mitsubishi-heat-pump-air-conditioner/

*Mitsubishi means Mitsubishi Electric, not Mitsubishi Heavy Industries.
