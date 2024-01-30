#ifndef DEVICE_H__
#define DEVICE_H__
#include "core/quectel.h"
#include "core/JSON.h"
#include "config.h"
#include "core/mac.h"
#include  <functional>

namespace Device {
  bool listening = false;
  std::function<void(String)> dataCallbac;
  void sendFirmwareRequest() {
    String command = JSON::JSON();
    JSON::add(command, "type", "firmware_request");
    JSON::add(command, "installed_firmware", FIRMWARE_VERSION);
    JSON::add(command, "mac", MAC::getMac());
    Quectel::MQTT::publish(MAC::getMac() + "-dev", command);
  }
  
  void updateConfiguration(std::function<void()> callbac) {
    Serial_println("updating configuration");
    String config = Configuration::toJSON();
    JSON::add(config, "mac", MAC::getMac());
    JSON::add(config, "type", "config");
    JSON::add(config, "installed_firmware", FIRMWARE_VERSION);
    JSON::prettify(config);
    Quectel::MQTT::publish(MAC::getMac() + "-dev", config, callbac);
  }

  void reRegister(std::function<void()> callbac) {
    Quectel::MQTT::publish("register", MAC::getMac(), callbac);
  }

  void onData(std::function<void(String)> callbac) {
    Device::dataCallbac = callbac;
  }

  void listen(std::function<void()> callbac) {
    Device::listening = false;
    Quectel::MQTT::on(MAC::getMac(), Device::dataCallbac, callbac);
  }
};
#endif