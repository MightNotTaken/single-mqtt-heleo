#ifndef DEVICE_H__
#define DEVICE_H__
#include "core/quectel.h"
#include "core/JSON.h"
#include "config.h"
#include "core/mac.h"
using namespace Quectel;
namespace Device {
  void sendFirmwareRequest() {
    String command = JSON::JSON();
    JSON::add(command, "type", "firmware_request");
    JSON::add(command, "installed_firmware", FIRMWARE_VERSION);
    JSON::add(command, "mac", MAC::getMac());
    MQTT::publish(MAC::getMac(), command);
  }
  
  void updateConfiguration() {
    Serial_println("updating configuration");
    String config = Configuration::toJSON();
    JSON::add(config, "mac", MAC::getMac());
    JSON::add(config, "type", "config");
    JSON::add(config, "installed_firmware", FIRMWARE_VERSION);
    MQTT::publish(MAC::getMac(), config);
  }

  void reRegister() {
    MQTT::publish("register", MAC::getMac(), []() {
      Serial_println("Device register called");
    });
  }

  void listen() {
    MQTT::on(MAC::getMac(), [](String data) {
      Serial_println(data + " received from myevent");
    });
  }
};
#endif