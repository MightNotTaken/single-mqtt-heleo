#ifndef DEVICE_H__
#define DEVICE_H__
#include "core/quectel.h"
#include "core/JSON.h"
#include "config.h"
#include "core/mac.h"
#include "core/database.h"
#include  <functional>

namespace Device {
  bool listening = false;
  std::function<void(uint8_t, uint8_t)> relayUpdateCallback;
  std::function<void(uint8_t, uint8_t)> fanUpdateCallback;
  void sendFirmwareRequest() {
    String command = JSON::JSON();
    JSON::add(command, "type", "firmware_request");
    JSON::add(command, "installed_firmware", FIRMWARE_VERSION);
    JSON::add(command, "mac", MAC::getMac());
    Quectel::MQTT::publish(MAC::getMac() + "-dev", command);
  }
  
  void updateConfiguration(std::function<void()> callback) {
    Serial_println("updating configuration");
    String config = Configuration::toJSON();
    JSON::add(config, "mac", MAC::getMac());
    JSON::add(config, "type", "config");
    JSON::add(config, "installed_firmware", FIRMWARE_VERSION);
    JSON::prettify(config);
    Quectel::MQTT::publish(MAC::getMac() + "-dev", config, callback);
  }

  void reRegister(std::function<void()> callback) {
    Quectel::MQTT::publish("register", MAC::getMac(), callback);
  }

  void onRelayUpdate(std::function<void(uint8_t, uint8_t)> callback) {
    Device::relayUpdateCallback = callback;
  }
  void onFanUpdate(std::function<void(uint8_t, uint8_t)> callback) {
    Device::fanUpdateCallback = callback;
  }

  void onData(String data) {
    String type = JSON::read(data, "type");
    if (type == "update") {
      for (int i=0; i<Configuration::relays; i++) {
        int state = JSON::read(data, String("r") + i).toInt();
        invoke(relayUpdateCallback, i, state);
      }
      for (int i=0; i<Configuration::dimmers; i++) {
        int state = JSON::read(data, String("d") + i).toInt();
        invoke(fanUpdateCallback, i, state);
      }
    } else if (type == "restart") {
      ESP.restart();
    } else if (type == "update_firmware") {

    } else if (type == "force_update_firmware") {

    }
  }

  void listen(std::function<void()> callback) {
    Device::listening = false;
    Quectel::MQTT::on(MAC::getMac(), Device::onData, callback);
  }

  void begin() {

  }
};
#endif