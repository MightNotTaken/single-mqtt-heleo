#ifndef CONFIG_H__
#define CONFIG_H__
#define FIRMWARE_VERSION    String("V_30_01_2024")
#define BOARD_TYPE          String("FOUR_RELAY_NO_DIMMER")
#include "core/mac.h"
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

    namespace Touch {
      int total  = 1;
      int gpios[] = {13};
    };
  };

  String getBoardSpecQuery(String target_version) {
    return String("?boardType=") + BOARD_TYPE + "&version=" + target_version + "&mac=" + MAC::getMac();
  }

  String toJSON() {
    String json = JSON::JSON();
    JSON::add(json, "relays", Peripherals::relays, true);
    JSON::add(json, "dimmer", Peripherals::dimmers, true);
    JSON::add(json, "installed_firmware", FIRMWARE_VERSION);
    return json;
  }
};
#endif