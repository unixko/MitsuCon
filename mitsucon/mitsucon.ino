#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HeatPump.h>

#include "mitsucon.h"
#include "webui.h"

#ifdef OTA
  #include <ESP8266mDNS.h>
  #include <ArduinoOTA.h>
#endif

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
HeatPump hp;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;
unsigned long lastTempSend;
unsigned long lastMqttAttempt;
unsigned long lastWifiConnected;
unsigned long restartAt;
bool apActive;
bool staPaused;

const char* controller_sw_version       = "20261018";

// wifi/mqtt settings saved from the web interface, firmware defaults otherwise
const uint32_t CONFIG_MAGIC = 0x4d430001;
struct Config {
  uint32_t magic;
  char ssid[33];
  char pass[65];
  char mqtt_server[65];
  uint16_t mqtt_port;
  char mqtt_user[33];
  char mqtt_pass[65];
} cfg;
bool cfgStored;

const char* FAN_VALUES[]  = {"AUTO", "QUIET", "1", "2", "3", "4"};
const char* VANE_VALUES[] = {"AUTO", "1", "2", "3", "4", "5", "SWING"};

void loadConfig() {
  EEPROM.begin(sizeof(Config));
  EEPROM.get(0, cfg);
  cfgStored = cfg.magic == CONFIG_MAGIC;
  if (cfgStored) {
    cfg.ssid[sizeof(cfg.ssid) - 1] = '\0';
    cfg.pass[sizeof(cfg.pass) - 1] = '\0';
    cfg.mqtt_server[sizeof(cfg.mqtt_server) - 1] = '\0';
    cfg.mqtt_user[sizeof(cfg.mqtt_user) - 1] = '\0';
    cfg.mqtt_pass[sizeof(cfg.mqtt_pass) - 1] = '\0';
    return;
  }
  memset(&cfg, 0, sizeof(cfg));
  strlcpy(cfg.ssid, cfg_default_ssid, sizeof(cfg.ssid));
  strlcpy(cfg.pass, cfg_default_password, sizeof(cfg.pass));
  strlcpy(cfg.mqtt_server, cfg_default_mqtt_server, sizeof(cfg.mqtt_server));
  cfg.mqtt_port = cfg_default_mqtt_port;
  strlcpy(cfg.mqtt_user, cfg_default_mqtt_username, sizeof(cfg.mqtt_user));
  strlcpy(cfg.mqtt_pass, cfg_default_mqtt_password, sizeof(cfg.mqtt_pass));
}

void saveConfig() {
  cfg.magic = CONFIG_MAGIC;
  EEPROM.put(0, cfg);
  EEPROM.commit();
}

void clearConfig() {
  cfg.magic = 0;
  EEPROM.put(0, cfg);
  EEPROM.commit();
}

void mqttConnect() {
  if (lastMqttAttempt != 0 && millis() - lastMqttAttempt < MQTT_RETRY_INTERVAL_MS) {
    return;
  }
  lastMqttAttempt = millis();
  const char* user = cfg.mqtt_user[0] ? cfg.mqtt_user : NULL;
  const char* pass = cfg.mqtt_pass[0] ? cfg.mqtt_pass : NULL;
  if (mqtt_client.connect(client_id, user, pass, heatpump_availability_topic, 1, 1, "offline")) {
    mqtt_client.subscribe(heatpump_mode_command_topic);
    mqtt_client.subscribe(heatpump_temperature_command_topic);
    mqtt_client.subscribe(heatpump_fan_mode_command_topic);
    mqtt_client.subscribe(heatpump_swing_mode_command_topic);
    mqtt_client.subscribe(ha_status_topic);
    mqtt_client.publish(heatpump_availability_topic, "online", true);
    mqttAutoDiscovery();
  }
}

void mqttAutoDiscovery() {
  const String chip_id                 = String(ESP.getChipId());
  const String mqtt_discov_topic       = String(mqtt_discov_prefix) + "/climate/" + chip_id + "/config";

  JsonDocument rootDiscovery;

  rootDiscovery["name"]                = name;
  rootDiscovery["uniq_id"]             = chip_id;
  rootDiscovery["~"]                   = heatpump_topic;
  rootDiscovery["min_temp"]            = min_temp;
  rootDiscovery["max_temp"]            = max_temp;
  rootDiscovery["temp_step"]           = temp_step;
  rootDiscovery["temperature_unit"]           = "C";
  JsonArray modes                      = rootDiscovery["modes"].to<JsonArray>();
    modes.add("heat_cool");
    modes.add("cool");
    modes.add("dry");
    modes.add("heat");
    modes.add("fan_only");
    modes.add("off");
  JsonArray fan_modes                  = rootDiscovery["fan_modes"].to<JsonArray>();
    fan_modes.add("auto");
    fan_modes.add("quiet");
    fan_modes.add("1");
    fan_modes.add("2");
    fan_modes.add("3");
    fan_modes.add("4");
  JsonArray swing_modes                = rootDiscovery["swing_modes"].to<JsonArray>();
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
  JsonObject device                    = rootDiscovery["device"].to<JsonObject>();
    device["name"]                     = name;
    JsonArray ids = device["ids"].to<JsonArray>();
      ids.add(chip_id);
    device["mf"]                       = "MitsuCon";
    device["mdl"]                      = "Mitsubishi Electric Heat Pump";
    device["sw"]                       = controller_sw_version;
    device["cu"]                       = "http://" + WiFi.localIP().toString() + "/";

  String bufferDiscovery;
  serializeJson(rootDiscovery, bufferDiscovery);

  if (!mqtt_client.publish(mqtt_discov_topic.c_str(), bufferDiscovery.c_str(), true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish DISCOV topic");
  }
}

void hpSettingsChanged() {
  JsonDocument root;

  heatpumpSettings currentSettings = hp.getSettings();
  heatpumpStatus currentStatus = hp.getStatus();

  if (currentSettings.power == NULL) {
    return; // not synced with the heat pump yet
  }

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
  JsonDocument rootInfo;

  rootInfo["name"]              = name;
  rootInfo["roomTemperature"]   = currentStatus.roomTemperature;
  rootInfo["operating"]         = currentStatus.operating;

  char bufferInfo[512];
  serializeJson(rootInfo, bufferInfo);

  if (!mqtt_client.publish(heatpump_current_topic, bufferInfo, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish CURR topic");
  }

#ifdef _timersAttr
  JsonDocument rootTimers;

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

    JsonDocument root;

    root[packetDirection] = message;

    char buffer[512];
    serializeJson(root, buffer);

    if (!mqtt_client.publish(heatpump_debug_topic, buffer)) {
      mqtt_client.publish(heatpump_debug_topic, "failed to publish DEBUG topic");
    }
  }
}

const char* lookupValue(const char* value, const char** values, int count) {
  for (int i = 0; i < count; i++) {
    if (strcasecmp(value, values[i]) == 0) {
      return values[i];
    }
  }
  return NULL;
}

// Apply a command using Home Assistant vocabulary, shared by MQTT and the web interface.
// cmd is one of "mode", "temp", "fan", "vane". Returns false for unknown values.
bool hpCommand(const char* cmd, const char* value) {
  JsonDocument root;

  heatpumpSettings currentSettings = hp.getSettings();

  root["power"]       = currentSettings.power;
  root["mode"]        = currentSettings.mode;
  root["temperature"] = currentSettings.temperature;
  root["fan"]         = currentSettings.fan;
  root["vane"]        = currentSettings.vane;

  if (strcmp(cmd, "mode") == 0) {
    const char* mode = NULL;
    if      (strcmp(value, "off") == 0)       mode = "";
    else if (strcmp(value, "heat_cool") == 0) mode = "AUTO";
    else if (strcmp(value, "fan_only") == 0)  mode = "FAN";
    else if (strcmp(value, "heat") == 0)      mode = "HEAT";
    else if (strcmp(value, "cool") == 0)      mode = "COOL";
    else if (strcmp(value, "dry") == 0)       mode = "DRY";
    if (mode == NULL) {
      return false;
    }
    if (mode[0] == '\0') {
      hp.setPowerSetting("OFF");
      root["power"] = "OFF";
    } else {
      hp.setPowerSetting("ON");
      root["power"] = "ON";
      hp.setModeSetting(mode);
      root["mode"] = mode;
    }
  } else if (strcmp(cmd, "temp") == 0) {
    char* end;
    float temperature = strtof(value, &end);
    if (end == value) {
      return false;
    }
    temperature = constrain(temperature, min_temp, max_temp);
    hp.setTemperature(temperature);
    root["temperature"] = temperature;
  } else if (strcmp(cmd, "fan") == 0) {
    const char* fan = lookupValue(value, FAN_VALUES, 6);
    if (fan == NULL) {
      return false;
    }
    hp.setFanSpeed(fan);
    root["fan"] = fan;
  } else if (strcmp(cmd, "vane") == 0) {
    const char* vane = lookupValue(value, VANE_VALUES, 7);
    if (vane == NULL) {
      return false;
    }
    hp.setVaneSetting(vane);
    root["vane"] = vane;
  } else {
    return false;
  }

  hp.update();

  char buffer[512];
  serializeJson(root, buffer);

  if (!mqtt_client.publish(heatpump_state_topic, buffer, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish STATE topic");
  }
  return true;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  if (strcmp(topic, ha_status_topic) == 0) {
    if (strcmp(message, "online") == 0) {
      mqttAutoDiscovery();
      hpSettingsChanged();
    }
  } else if (strcmp(topic, heatpump_mode_command_topic) == 0) {
    hpCommand("mode", message);
  } else if (strcmp(topic, heatpump_temperature_command_topic) == 0) {
    hpCommand("temp", message);
  } else if (strcmp(topic, heatpump_fan_mode_command_topic) == 0) {
    hpCommand("fan", message);
  } else if (strcmp(topic, heatpump_swing_mode_command_topic) == 0) {
    hpCommand("vane", message);
  }
}

// ---- WiFi with fallback access point ----

void startAccessPoint() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid);
  dnsServer.start(53, "*", WiFi.softAPIP());
  apActive = true;
  staPaused = false;
}

void stopAccessPoint() {
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive = false;
}

void wifiLoop() {
  if (WiFi.status() == WL_CONNECTED) {
    lastWifiConnected = millis();
    if (apActive && WiFi.softAPgetStationNum() == 0) {
      stopAccessPoint();
    }
  } else if (!apActive && (cfg.ssid[0] == '\0' || millis() - lastWifiConnected > WIFI_AP_FALLBACK_MS)) {
    startAccessPoint();
  }

  if (apActive) {
    dnsServer.processNextRequest();
    // station reconnect attempts hop channels and drop AP clients, so only retry while nobody is connected
    if (WiFi.softAPgetStationNum() > 0) {
      if (!staPaused && WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect();
        staPaused = true;
      }
    } else if (staPaused && cfg.ssid[0] != '\0') {
      WiFi.begin(cfg.ssid, cfg.pass);
      staPaused = false;
    }
  }

  // blue LED: off when connected, blinking otherwise (slow while the access point is up)
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(blueLedPin, HIGH);
  } else {
    digitalWrite(blueLedPin, (millis() / (apActive ? 1000 : 250)) % 2 ? LOW : HIGH);
  }
}

// ---- Web interface ----

void sendPage(PGM_P page) {
  server.chunkedResponseModeStart(200, "text/html");
  server.sendContent(F("<!doctype html><html lang=en><head><meta charset=utf-8>"
                       "<meta name=viewport content=\"width=device-width,initial-scale=1\"><title>MitsuCon</title>"));
  server.sendContent_P(PAGE_STYLE);
  server.sendContent(F("</head><body>"));
  server.sendContent_P(page);
  server.sendContent(F("</body></html>"));
  server.chunkedResponseFinalize();
}

const char* haMode(heatpumpSettings& s) {
  if (s.power == NULL)                return "unknown";
  if (strcmp(s.power, "OFF") == 0)    return "off";
  if (strcmp(s.mode, "AUTO") == 0)    return "heat_cool";
  if (strcmp(s.mode, "FAN") == 0)     return "fan_only";
  if (strcmp(s.mode, "HEAT") == 0)    return "heat";
  if (strcmp(s.mode, "COOL") == 0)    return "cool";
  if (strcmp(s.mode, "DRY") == 0)     return "dry";
  return "unknown";
}

const char* haAction(heatpumpSettings& s, heatpumpStatus& st) {
  if (s.power == NULL)                     return "unknown";
  if (strcmp(s.power, "OFF") == 0)         return "off";
  if (strcmp(s.mode, "FAN") == 0)          return "fan";
  if (!st.operating)                       return "idle";
  if (strcmp(s.mode, "HEAT") == 0)         return "heating";
  if (strcmp(s.mode, "COOL") == 0)         return "cooling";
  if (strcmp(s.mode, "DRY") == 0)          return "drying";
  return "idle";
}

void handleStatus() {
  JsonDocument root;
  heatpumpSettings s = hp.getSettings();
  heatpumpStatus st = hp.getStatus();
  String fan = s.fan ? s.fan : "auto";
  String vane = s.vane ? s.vane : "auto";
  fan.toLowerCase();
  vane.toLowerCase();

  root["name"]   = name;
  root["fw"]     = controller_sw_version;
  root["uptime"] = millis() / 1000;
  root["heap"]   = ESP.getFreeHeap();
  root["reset"]  = ESP.getResetReason();

  JsonObject limits = root["limits"].to<JsonObject>();
    limits["min"]  = min_temp;
    limits["max"]  = max_temp;
    limits["step"] = temp_step;

  JsonObject jhp = root["hp"].to<JsonObject>();
    jhp["connected"]       = hp.isConnected();
    jhp["mode"]            = haMode(s);
    jhp["action"]          = haAction(s, st);
    jhp["temperature"]     = s.temperature;
    jhp["fan"]             = fan;
    jhp["vane"]            = vane;
    jhp["roomTemperature"] = st.roomTemperature;
    jhp["operating"]       = st.operating;

  JsonObject wifi = root["wifi"].to<JsonObject>();
    wifi["connected"]  = WiFi.status() == WL_CONNECTED;
    wifi["ssid"]       = cfg.ssid;
    wifi["rssi"]       = WiFi.RSSI();
    wifi["ip"]         = WiFi.localIP().toString();
    wifi["ap"]         = apActive;
    wifi["ap_ssid"]    = ap_ssid;
    wifi["ap_clients"] = apActive ? WiFi.softAPgetStationNum() : 0;

  JsonObject mqtt = root["mqtt"].to<JsonObject>();
    mqtt["connected"] = mqtt_client.connected();
    mqtt["server"]    = cfg.mqtt_server;

  String out;
  serializeJson(root, out);
  server.send(200, "application/json", out);
}

void handleSet() {
  const char* cmds[] = {"mode", "temp", "fan", "vane"};
  for (const char* cmd : cmds) {
    if (server.hasArg(cmd) && !hpCommand(cmd, server.arg(cmd).c_str())) {
      server.send(400, "text/plain", String("invalid ") + cmd);
      return;
    }
  }
  server.send(200, "text/plain", "ok");
}

void handleGetConfig() {
  JsonDocument root;
  root["stored"]      = cfgStored;
  root["ssid"]        = cfg.ssid;
  root["mqtt_server"] = cfg.mqtt_server;
  root["mqtt_port"]   = cfg.mqtt_port;
  root["mqtt_user"]   = cfg.mqtt_user;
  String out;
  serializeJson(root, out);
  server.send(200, "application/json", out);
}

bool copyArg(const char* arg, char* dest, size_t size, bool keepIfEmpty) {
  const String value = server.arg(arg);
  if (value.length() >= size) {
    return false;
  }
  if (value.length() > 0 || !keepIfEmpty) {
    strlcpy(dest, value.c_str(), size);
  }
  return true;
}

void handleSaveConfig() {
  Config previous = cfg;
  long port = server.arg("mqtt_port").toInt();
  if (server.arg("ssid").length() == 0 || server.arg("mqtt_server").length() == 0 || port < 1 || port > 65535 ||
      !copyArg("ssid", cfg.ssid, sizeof(cfg.ssid), false) ||
      !copyArg("pass", cfg.pass, sizeof(cfg.pass), true) ||
      !copyArg("mqtt_server", cfg.mqtt_server, sizeof(cfg.mqtt_server), false) ||
      !copyArg("mqtt_user", cfg.mqtt_user, sizeof(cfg.mqtt_user), false) ||
      !copyArg("mqtt_pass", cfg.mqtt_pass, sizeof(cfg.mqtt_pass), true)) {
    cfg = previous;
    server.send(400, "text/plain", "invalid settings");
    return;
  }
  cfg.mqtt_port = port;
  saveConfig();
  server.send(200, "text/plain", "Saved, rebooting");
  restartAt = millis() + 1000;
}

void handleScan() {
  int count = WiFi.scanNetworks();
  JsonDocument root;
  JsonArray nets = root.to<JsonArray>();
  for (int i = 0; i < count && nets.size() < 25; i++) {
    if (WiFi.SSID(i).length() == 0) continue;
    JsonObject net = nets.add<JsonObject>();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
    net["open"] = WiFi.encryptionType(i) == ENC_TYPE_NONE;
  }
  WiFi.scanDelete();
  String out;
  serializeJson(root, out);
  server.send(200, "application/json", out);
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting");
  restartAt = millis() + 1000;
}

void handleFactory() {
  clearConfig();
  server.send(200, "text/plain", "Settings cleared, rebooting");
  restartAt = millis() + 1000;
}

void handleNotFound() {
  // captive portal: send anything addressed elsewhere to the settings page
  if (apActive && server.hostHeader() != WiFi.softAPIP().toString()) {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/config", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "Not found");
}

void webSetup() {
  server.on("/", HTTP_GET, []() { sendPage(PAGE_INDEX); });
  server.on("/config", HTTP_GET, []() { sendPage(PAGE_CONFIG); });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/set", HTTP_POST, handleSet);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSaveConfig);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/api/factory", HTTP_POST, handleFactory);
  server.onNotFound(handleNotFound);
  httpUpdater.setup(&server, "/update", "admin", ota_password);
  server.begin();
}

void setup() {
  pinMode(redLedPin, OUTPUT);
  digitalWrite(redLedPin, HIGH);
  pinMode(blueLedPin, OUTPUT);
  digitalWrite(blueLedPin, HIGH);

  loadConfig();

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(client_id);
  WiFi.setAutoReconnect(true);
  if (cfg.ssid[0] != '\0') {
    WiFi.begin(cfg.ssid, cfg.pass);
  }
  lastWifiConnected = millis();

  mqtt_client.setServer(cfg.mqtt_server, cfg.mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  mqtt_client.setBufferSize(2560);
  mqtt_client.setSocketTimeout(5);

  hp.setSettingsChangedCallback(hpSettingsChanged);
  hp.setStatusChangedCallback(hpStatusChanged);
  hp.setPacketCallback(hpPacketDebug);

  #ifdef OTA
  ArduinoOTA.setHostname(client_id);
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.begin();
  MDNS.addService("http", "tcp", 80);
  #endif

  webSetup();

  hp.connect(&Serial);

  lastTempSend = millis();
}

void loop() {
  wifiLoop();

  if (WiFi.status() == WL_CONNECTED) {
    if (cfg.mqtt_server[0] != '\0' && !mqtt_client.connected()) {
      mqttConnect();
    }
    mqtt_client.loop();
  }

  hp.sync();

  if (millis() - lastTempSend > SEND_ROOM_TEMP_INTERVAL_MS) {
    hpStatusChanged(hp.getStatus());
    lastTempSend = millis();
  }

  server.handleClient();

#ifdef OTA
  ArduinoOTA.handle();
#endif

  if (restartAt != 0 && (long)(millis() - restartAt) >= 0) {
    ESP.restart();
  }
}
