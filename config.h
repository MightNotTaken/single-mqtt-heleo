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

  namespace Peripherals {
    int relays =  1;
    int dimmers = 0;

    namespace GPIO {
      int relays[] = {23};
      int dimmers[][2] = {};
    };
  };



  String toJSON() {
    String json = JSON::JSON();
    JSON::add(json, "relays", Peripherals::relays, true);
    JSON::add(json, "dimmer", Peripherals::dimmers, true);
    JSON::add(json, "installed_firmware", FIRMWARE_VERSION);
    return json;
  }
};
#endif