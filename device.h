#ifndef DEVICE_H__
#define DEVICE_H__
#include "core/quectel.h"
#include "core/JSON.h"
#include "core/core.h"
#include "config.h"
#include "core/mac.h"
#include "core/database.h"
#include  <functional>

namespace Device {
  bool listening = false;
  std::function<void(uint8_t, uint8_t)> relayUpdateCallback;
  std::function<void(uint8_t, uint8_t)> fanUpdateCallback;
  String stateString = JSON::JSON();
  Core::Core_T* core;
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

  template<typename T>
  void updateStateString(String key, T value, bool overwrite = false) {
    if (overwrite) {
      Device::stateString = JSON::JSON();
    }
    JSON::add(Device::stateString, key, String(value));
  }

  void publishStateString() {
    Database::writeFile("/state.json", Device::stateString);
    String configuration = Device::stateString;
    JSON::add(configuration, "type", "manual_update");
    JSON::add(configuration, "mac", MAC::getMac());
    JSON::prettify(Device::stateString);
    Quectel::MQTT::publish(MAC::getMac() + "-dev", configuration, []() {
      Serial_println("configuration updated successfully");
    });
  }

  void onData(String data) {
    String type = JSON::read(data, "type");
    if (type == "update") {
      Device::updateStateString("mac", MAC::getMac(), true);
      for (int i=0; i<Configuration::relays; i++) {
        int state = JSON::read(data, String("r") + i).toInt();
        invoke(relayUpdateCallback, i, state);
        Device::updateStateString(String("r") + i, state);
      }
      for (int i=0; i<Configuration::dimmers; i++) {
        int state = JSON::read(data, String("d") + i).toInt();
        invoke(fanUpdateCallback, i, state);
        Device::updateStateString(String("d") + i, state);
      }
      Device::updateStateString("device", JSON::read(data, "device"));
      Device::publishStateString();
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

  void begin(Core::Core_T* core) {
    Device::core = core;
  }
};
#endif