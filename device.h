#ifndef DEVICE_H__
#define DEVICE_H__
#include "core/quectel.h"
#include "core/JSON.h"
#include "config.h"
#include "core/mac.h"
#include  <functional>

namespace Device {
  void sendFirmwareRequest() {
    String command = JSON::JSON();
    JSON::add(command, "type", "firmware_request");
    JSON::add(command, "installed_firmware", FIRMWARE_VERSION);
    JSON::add(command, "mac", MAC::getMac());
    Quectel::MQTT::publish(MAC::getMac(), command);
  }
  
  void updateConfiguration(std::function<void()> callbac) {
    Serial_println("updating configuration");
    String config = Configuration::toJSON();
    JSON::add(config, "mac", MAC::getMac());
    JSON::add(config, "type", "config");
    JSON::add(config, "installed_firmware", FIRMWARE_VERSION);
    JSON::prettify(config);
    Quectel::MQTT::publish(MAC::getMac(), config, callbac);
  }

  void reRegister(std::function<void()> callbac) {
    Quectel::MQTT::publish("register", MAC::getMac(), callbac});
  }

  void listen(std::function<void()> callbac) {
    Quectel::MQTT::on(MAC::getMac(), callbac);
  }
};
#endif