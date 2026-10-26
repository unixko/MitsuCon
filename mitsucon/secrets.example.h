// Optional: copy to secrets.h to pre-fill settings at build time. secrets.h is not committed.
// Anything left out can be entered later on the controller's web interface (/config).
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define MQTT_SERVER   "MQTT_SERVER_NAME_OR_IP"
#define MQTT_USER     HP_NAME // per-unit login, e.g. "heatpump0"
#define MQTT_PASSWORD "MQTT_PASSWORD"
#define OTA_PASSWORD  "OTA_PASSWORD"

// Optional: override thermostat limits advertised to Home Assistant (defaults 16 / 31 / 1)
// #define MIN_TEMP  16
// #define MAX_TEMP  31
// #define TEMP_STEP 1
