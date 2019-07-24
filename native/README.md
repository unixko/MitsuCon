*This is work in process code. Please use Classic code for stable release.

# Native Controller

Build ESP-8266 device to interface between Mitsubishi Electric heat pump and Home Assistant. This controller utilizes Home Assistant native MQTT HVAC component so mostly guaranteed compatible with future Home Assistant updates. 

## Integration
This code uses Home Assistant MQTT discovery feature so if you already configured MQTT integration in Home Assistant your heat pump will be added automatiacally once you power it on. 

Due to large string in MQTT discovery function, PubSubClient.h MQTT_MAX_PACKET_SIZE must set to 1280.

## Known Issues
**Please note that I'm not actual programmer so please kindly help me pull.**
* ESP-8266 hangs after running about 2 weeks, restarts device will work again. Need to find overflow in codes.
* Sometimes heat pump unit turns off instead when send temperature changing command.
* When change setting on HA UI, change will effect immediately but UI will delay for next data pull to refresh new status.

Tested on Wemos D1 mini board with Home Assistant 0.96 and Mosquitto broker add-on 5.0.
