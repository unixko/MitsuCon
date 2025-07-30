
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HeatPump.h>

#include "mitsucon.h"

#ifdef OTA
  #include <ESP8266mDNS.h>
  #include <ArduinoOTA.h>
#endif

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
HeatPump hp;
unsigned long lastTempSend;

const char* controller_sw_version       = "20200429";

void mqttConnect() {
  while (!mqtt_client.connected()) {
    if (mqtt_client.connect(client_id, mqtt_username, mqtt_password, heatpump_availability_topic, 1, 1, "offline")) {
      mqtt_client.subscribe(heatpump_mode_command_topic);
      mqtt_client.subscribe(heatpump_temperature_command_topic);
      mqtt_client.subscribe(heatpump_fan_mode_command_topic);
      mqtt_client.subscribe(heatpump_swing_mode_command_topic);
      mqtt_client.publish(heatpump_availability_topic, "online", true);
    } else {
      delay(5000);
    }
  }
}

void mqttAutoDiscovery() {
  const String chip_id                 = String(ESP.getChipId());
  const String mqtt_discov_topic       = String(mqtt_discov_prefix) + "/climate/" + chip_id + "/config";
  
  const size_t bufferSizeDiscovery = JSON_OBJECT_SIZE(60);
  DynamicJsonDocument rootDiscovery(bufferSizeDiscovery);
  
  rootDiscovery["name"]                = name;
  rootDiscovery["uniq_id"]             = chip_id;
  rootDiscovery["~"]                   = heatpump_topic;
  rootDiscovery["min_temp"]            = min_temp;
  rootDiscovery["max_temp"]            = max_temp;
  rootDiscovery["temp_step"]           = temp_step;
  rootDiscovery["temperature_unit"]           = "C";
  JsonArray modes                      = rootDiscovery.createNestedArray("modes");
    modes.add("heat_cool");
    modes.add("cool");
    modes.add("dry");
    modes.add("heat");
    modes.add("fan_only");
    modes.add("off");
  JsonArray fan_modes                  = rootDiscovery.createNestedArray("fan_modes");
    fan_modes.add("auto");
    fan_modes.add("quiet");
    fan_modes.add("1");
    fan_modes.add("2");
    fan_modes.add("3");
    fan_modes.add("4");
  JsonArray swing_modes                = rootDiscovery.createNestedArray("swing_modes");
    swing_modes.add("auto");
    swing_modes.add("1");
    swing_modes.add("2");
    swing_modes.add("3");
    swing_modes.add("4");
    swing_modes.add("5");
    swing_modes.add("swing");
  rootDiscovery["avty_t"]              = "~/tele/avty";
  rootDiscovery["curr_temp_t"]         = "~/tele/curr";
  rootDiscovery["curr_temp_tpl"]       = "{{ value_json.roomTemperature }}";
  rootDiscovery["mode_cmd_t"]          = "~/cmnd/mode";
  rootDiscovery["mode_stat_t"]         = "~/tele/stat";
  rootDiscovery["mode_stat_tpl"]       = "{{ 'off' if value_json.power == 'OFF' else value_json.mode | lower | replace('auto', 'heat_cool') | replace('fan', 'fan_only') }}";
  rootDiscovery["temp_cmd_t"]          = "~/cmnd/temp";
  rootDiscovery["temp_stat_t"]         = "~/tele/stat";
  rootDiscovery["temp_stat_tpl"]       = "{{ value_json.temperature }}";
  rootDiscovery["fan_mode_cmd_t"]      = "~/cmnd/fan";
  rootDiscovery["fan_mode_stat_t"]     = "~/tele/stat";
  rootDiscovery["fan_mode_stat_tpl"]   = "{{ value_json.fan | lower }}";
  rootDiscovery["swing_mode_cmd_t"]    = "~/cmnd/vane";
  rootDiscovery["swing_mode_stat_t"]   = "~/tele/stat";
  rootDiscovery["swing_mode_stat_tpl"] = "{{ value_json.vane | lower }}";
  rootDiscovery["act_t"]               = "~/tele/stat";
  rootDiscovery["act_tpl"]             = "{%if value_json.power=='OFF'%}off{%elif value_json.mode=='FAN'%}fan{%elif value_json.operating and value_json.mode=='HEAT'%}heating{%elif value_json.operating and value_json.mode=='COOL'%}cooling{%elif value_json.operating and value_json.mode=='DRY'%}drying{%else%}idle{%endif%}";
#ifdef _timersAttr
  rootDiscovery["json_attr_t"]         = "~/tele/attr";
#endif
  JsonObject device                    = rootDiscovery.createNestedObject("device");
    device["name"]                     = name;
    JsonArray ids = device.createNestedArray("ids");
      ids.add(chip_id);
    device["mf"]                       = "MitsuCon";
    device["mdl"]                      = "Mitsubishi Electric Heat Pump";
    device["sw"]                       = controller_sw_version;

  char bufferDiscovery[2048];
  serializeJson(rootDiscovery, bufferDiscovery);

  if (!mqtt_client.publish(mqtt_discov_topic.c_str(), bufferDiscovery, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish DISCOV topic");
  }
}

void hpSettingsChanged() {
  const size_t bufferSize = JSON_OBJECT_SIZE(7);
  DynamicJsonDocument root(bufferSize);

  heatpumpSettings currentSettings = hp.getSettings();
  heatpumpStatus currentStatus = hp.getStatus();

  root["power"]       = currentSettings.power;
  root["mode"]        = currentSettings.mode;
  root["temperature"] = currentSettings.temperature;
  root["fan"]         = currentSettings.fan;
  root["vane"]        = currentSettings.vane;
  root["wideVane"]    = currentSettings.wideVane;
  root["operating"]   = currentStatus.operating;

  char buffer[512];
  serializeJson(root, buffer);

  if (!mqtt_client.publish(heatpump_state_topic, buffer, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish STATE topic");
  }
}

void hpStatusChanged(heatpumpStatus currentStatus) {
  const size_t bufferSizeInfo = JSON_OBJECT_SIZE(4);
  DynamicJsonDocument rootInfo(bufferSizeInfo);

  rootInfo["name"]              = name;
  rootInfo["roomTemperature"]   = currentStatus.roomTemperature;
  rootInfo["operating"]         = currentStatus.operating;

  char bufferInfo[512];
  serializeJson(rootInfo, bufferInfo);

  if (!mqtt_client.publish(heatpump_current_topic, bufferInfo, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish CURR topic");
  }

#ifdef _timersAttr
  const size_t bufferSizeTimers = JSON_OBJECT_SIZE(5);
  DynamicJsonDocument rootTimers(bufferSizeTimers);
                    
														   
  rootTimers["timer_set"]        = currentStatus.timers.mode;
  rootTimers["timer_on_mins"]    = currentStatus.timers.onMinutesSet;
  rootTimers["timer_on_remain"]  = currentStatus.timers.onMinutesRemaining;
  rootTimers["timer_off_mins"]   = currentStatus.timers.offMinutesSet;
  rootTimers["timer_off_remain"] = currentStatus.timers.offMinutesRemaining;

  char bufferTimers[512];
  serializeJson(rootTimers, bufferTimers);

  if (!mqtt_client.publish(heatpump_attribute_topic, bufferTimers, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish ATTR topic");
  }
#endif

  mqtt_client.publish(heatpump_availability_topic, "online", true);
}

void hpPacketDebug(byte* packet, unsigned int length, char* packetDirection) {
  if (_debugMode) {
    String message;
    for (int idx = 0; idx < length; idx++) {
      if (packet[idx] < 16) {
        message += "0";
      }
      message += String(packet[idx], HEX) + " ";
    }

    const size_t bufferSize = JSON_OBJECT_SIZE(8);
    DynamicJsonDocument root(bufferSize);

    root[packetDirection] = message;

    char buffer[512];
    serializeJson(root, buffer);

    if (!mqtt_client.publish(heatpump_debug_topic, buffer)) {
      mqtt_client.publish(heatpump_debug_topic, "failed to publish DEBUG topic");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  const size_t bufferSize = JSON_OBJECT_SIZE(6);
  DynamicJsonDocument root(bufferSize);

  heatpumpSettings currentSettings = hp.getSettings();

  root["power"]       = currentSettings.power;
  root["mode"]        = currentSettings.mode;
  root["temperature"] = currentSettings.temperature;
  root["fan"]         = currentSettings.fan;
  root["vane"]        = currentSettings.vane;

  if (strcmp(topic, heatpump_mode_command_topic) == 0) {
    if (strcmp(message, "off") == 0) {
      const char* power = "OFF";
      hp.setPowerSetting(power);
      root["power"] = power;
    } else if (strcmp(message, "heat_cool") == 0) {
      const char* power = "ON";
      hp.setPowerSetting(power);      
      root["power"] = power;
      const char* mode = "AUTO";
      hp.setModeSetting(mode);
      root["mode"] = mode;
    } else if (strcmp(message, "fan_only") == 0) {
      const char* power = "ON";
      hp.setPowerSetting(power);      
      root["power"] = power;
      const char* mode = "FAN";
      hp.setModeSetting(mode);
      root["mode"] = mode;
    } else {
      const char* power = "ON";
      hp.setPowerSetting(power);
      root["power"] = power;
      const char* mode = strupr(message);
      hp.setModeSetting(mode);
      root["mode"] = mode;
    }
  } else if (strcmp(topic, heatpump_temperature_command_topic) == 0) {
    float temperature = atof(message);
    hp.setTemperature(temperature);
    root["temperature"] = temperature;
  } else if (strcmp(topic, heatpump_fan_mode_command_topic) == 0) {
    const char* fan = strupr(message);      
    hp.setFanSpeed(fan);
    root["fan"] = fan;
  } else if (strcmp(topic, heatpump_swing_mode_command_topic) == 0) {
    const char* vane = strupr(message);
    hp.setVaneSetting(vane);
    root["vane"] = vane;
  }

  hp.update();

  char buffer[512];
  serializeJson(root, buffer);

  if (!mqtt_client.publish(heatpump_state_topic, buffer, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish STATE topic");
  }
}

void setup() {
  pinMode(redLedPin, OUTPUT);
  digitalWrite(redLedPin, HIGH);
  pinMode(blueLedPin, OUTPUT);
  digitalWrite(blueLedPin, HIGH);

  WiFi.hostname(client_id);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(blueLedPin, LOW);
    delay(250);
    digitalWrite(blueLedPin, HIGH);
    delay(250);
  }

  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  mqttConnect();

  hp.setSettingsChangedCallback(hpSettingsChanged);
  hp.setStatusChangedCallback(hpStatusChanged);
  hp.setPacketCallback(hpPacketDebug);

  #ifdef OTA
  ArduinoOTA.setHostname(client_id);
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.begin();
  #endif

  hp.connect(&Serial);

  mqttAutoDiscovery();
  
  lastTempSend = millis();
}

void loop() {
  if (!mqtt_client.connected()) {
    mqttConnect();
  }

  hp.sync();

  if (millis() > (lastTempSend + SEND_ROOM_TEMP_INTERVAL_MS)) {
    hpStatusChanged(hp.getStatus());
    lastTempSend = millis();
  }

  mqtt_client.loop();

#ifdef OTA
  ArduinoOTA.handle();
#endif
}
