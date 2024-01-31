#ifndef DEVICE_H__
#define DEVICE_H__
#include "core/quectel.h"
#include "core/JSON.h"
#include "core/core.h"
#include "config.h"
#include "core/mac.h"
#include "core/database.h"
#include  <functional>
#include  <map>

namespace Device {
  bool listening = false;
  std::function<void(uint8_t, uint8_t)> relayUpdateCallback;
  std::function<void(uint8_t, uint8_t)> fanUpdateCallback;
  String stateString = JSON::JSON();
  Core::Core_T* core;

  std::vector<byte> relays;

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
  void updateStateString(String key, T value) {
    JSON::add(Device::stateString, key, String(value));
  }

  void publishStateString() {
    Database::writeFile("/state.json", Device::stateString);
    String configuration = Device::stateString;
    JSON::add(configuration, "type", "manual_update");
    JSON::add(configuration, "mac", MAC::getMac());
    JSON::prettify(Device::stateString);
    Quectel::MQTT::publish(MAC::getMac() + "-dev", configuration, []() {
      Serial_println("\nState published successfully");
    });
  }

  void flushStateString() {
    Device::stateString = JSON::JSON();
  }

  void onData(String data) {
    String type = JSON::read(data, "type");
    if (type == "update") {
      Device::flushStateString();
      for (int i=0; i<Configuration::Peripherals::relays; i++) {
        int state = JSON::read(data, String("r") + i).toInt();
        invoke(relayUpdateCallback, i, state);
        
      }
      for (int i=0; i<Configuration::Peripherals::dimmers; i++) {
        int state = JSON::read(data, String("d") + i).toInt();
        invoke(fanUpdateCallback, i, state);
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
    for (auto gpio: Configuration::Peripherals::GPIO::relays) {
      showX(gpio);
      Device::relays.push_back(gpio);
      pinMode(gpio, OUTPUT);
    }
    for (int i=0; i<Configuration::Peripherals::Touch::total; i++) {
      InputGPIO* touch = new InputGPIO(Configuration::Peripherals::Touch::gpios[i]);
      touch->onStateHigh([i]() {
        Serial_println("touch high");
        return;
        invoke(relayUpdateCallback, i, HIGH);
        Device::publishStateString();
      });
      touch->onStateLow([i]() {
        Serial_println("touch low");
        return;
        invoke(relayUpdateCallback, i, LOW);
        Device::publishStateString();
      });
      Device::core->registerInputGPIO(touch);
    }
    if (Database::readFile("/state.json")) {
      Device::stateString = Database::payload();
      for (int i=0; i<Configuration::Peripherals::relays; i++) {
        int state = JSON::read(Device::stateString, String("r") + i).toInt();
        invoke(relayUpdateCallback, i, state);
      }
    }
  }
};
#endif