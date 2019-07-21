# mehpha
Mitsubishi Electric Heat Pump Controller for Home Assistant

Build ESP-8266 device to interface between Mitsubishi Electric heat pump and Home Assistant. This controller utilizes Home Assistant native MQTT HVAC component so mostly guaranteed compatible with future Home Assistant updates. This code is based on https://github.com/SwiCago/HeatPump library so you need it to build this sketch.

![Wemos D1 Mini Adapter](https://user-images.githubusercontent.com/44964969/51798270-c3392980-2242-11e9-8986-cffc5fe4d287.jpg)

## Integration
This code uses Home Assistant MQTT discovery feature so if you already configured MQTT integration in Home Assistant your heat pump will be added automatiacally once you power it on. 

Due to large string in MQTT discovery function, PubSubClient.h MQTT_MAX_PACKET_SIZE must set to 1280.

## Known Issues
**Please note that I'm not actual programmer so please kindly help me pull.**
* ESP-8266 hangs after running about 2 weeks, restarts device will work again. Need to find overflow in codes.
* Sometimes heat pump unit turns off instead when send temperature changing command.
* When change setting on HA UI, change will effect immediately but UI will delay for next data pull to refresh new status.

Tested on Wemos D1 mini board with Home Assistant 0.96 and Mosquitto broker add-on 5.0.

## Credits
Thanks to all contributors especially:
* lekobob https://github.com/lekobob/mitsu_mqtt
* SwiCago https://github.com/SwiCago/HeatPump
* Hadley  https://nicegear.co.nz/blog/hacking-a-mitsubishi-heat-pump-air-conditioner/
