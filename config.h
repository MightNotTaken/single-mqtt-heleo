#ifndef CONFIG_H__
#define CONFIG_H__
#define FIRMWARE_VERSION    "V_30_01_2024"
namespace Configuration {
  namespace MQTT {
    String baseURL = "heleo.app";
    int port = 1883;
    String username = "rajesh";
    String password = "Rajesh.007";
    String apn = "airtelgprs.com";
  };

  namespace Quectel {
    int powerKey = 25;
  };

  int relays =  6;
  int dimmers = 1;
  
  String toJSON() {
    String json = JSON::JSON();
    JSON::add(json, "relays", relays, true);
    JSON::add(json, "dimmer", dimmers, true);
    JSON::add(json, "installed_firmware", FIRMWARE_VERSION);
    return json;
  }
};
#endif