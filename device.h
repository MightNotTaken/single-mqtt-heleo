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
  std::function<void(bool)> updateCallback;
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

  void onUpdate(std::function<void(bool)> callback) {
    Device::updateCallback = callback;
  }

  void onData(String data) {
    String type = JSON::read(data, "type");
    if (type == "update") {
      Database::writeFile("/data.txt", JSON::read(data, "r0"));
      int state = JSON::read(data, "r0").toInt();
      invoke(updateCallback, state);
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
};
#endif