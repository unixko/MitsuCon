// Home Assistant Mitsubishi Electric Heat Pump Controller https://github.com/unixko/MitsuCon
// using native MQTT Climate (HVAC) component with MQTT discovery for automatic configuration
// Set PubSubClient.h MQTT_MAX_PACKET_SIZE to 2048

// enable extra MQTT topic for debug/timer info
bool _debugMode  = false;
bool _timersAttr = false;

// comment out to disable OTA
#define OTA
const char* ota_password  = "OTA_PASSWORD";

// wifi settings
const char* ssid          = "WIFI_SSID";
const char* password      = "WIFI_PASSWORD";

// mqtt server settings
const char* mqtt_server   = "MQTT_SERVER_NAME_OR_IP";
const int mqtt_port       = 1883;
const char* mqtt_username = "MQTT_USER";
const char* mqtt_password = "MQTT_PASSWORD";

// mqtt client settings
// Change "heatpump" to be same on all lines
const char* name                                = "Heat Pump"; // Device Name displayed in Home Assistant
const char* client_id                           = "heatpump"; // WiFi hostname, OTA hostname, MQTT hostname
const char* heatpump_topic                      = "heatpump"; // MQTT topic, must be unique between heat pump unit
const char* heatpump_availability_topic         = "heatpump/tele/avty";
const char* heatpump_state_topic                = "heatpump/tele/stat";
const char* heatpump_current_topic              = "heatpump/tele/curr";
const char* heatpump_attribute_topic            = "heatpump/tele/attr";
const char* heatpump_mode_command_topic         = "heatpump/cmnd/mode";
const char* heatpump_temperature_command_topic  = "heatpump/cmnd/temp";
const char* heatpump_fan_mode_command_topic     = "heatpump/cmnd/fan";
const char* heatpump_swing_mode_command_topic   = "heatpump/cmnd/vane";
const char* heatpump_debug_topic                = "heatpump/debug";

// Customization
const char* min_temp                    = "16"; // Minimum temperature, check value from heatpump remote control
const char* max_temp                    = "31"; // Maximum temperature, check value from heatpump remote control
const char* temp_step                   = "1"; // Temperature setting step, check value from heatpump remote control
const char* mqtt_discov_prefix          = "homeassistant"; // Home Assistant MQTT Discovery Prefix

// pinouts
const int redLedPin  = 0; // Onboard LED = digital pin 0 (red LED on adafruit ESP8266 huzzah)
const int blueLedPin = 2; // Onboard LED = digital pin 0 (blue LED on adafruit ESP8266 huzzah)

// sketch settings
const unsigned int SEND_ROOM_TEMP_INTERVAL_MS = 60000;
