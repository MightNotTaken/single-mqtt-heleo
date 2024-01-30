#include "core/core.h"
#include "core/quectel.h" 
#include "device.h"
void Core::setupCore0() {
  Core::core0.onSetup([]() {
    Serial.begin(115200);
    Quectel::onReboot([]() {
      Serial.println("rebooted");
      Core::core0.setTimeout([]() {
        Serial_println("connecting to MQTT Broker");
        Quectel::MQTT::configure(
          Configuration::MQTT::baseURL,
          Configuration::MQTT::port,
          Configuration::MQTT::username,
          Configuration::MQTT::password,
          Configuration::MQTT::apn
        );
        Quectel::MQTT::onError([]() {
          Quectel::MQTT::connect();
        });
        Quectel::MQTT::onConnect([]() {
          Serial_println("MQTT connected succcessfully");
          
          Device::updateConfiguration();
          Device::listen();
          Core::core0.setTimeout([]() {
            Device::reRegister();
          }, SECONDS(2));
        });
        Quectel::MQTT::connect();
      }, SECONDS(2)); 
    });
    Quectel::begin(&core0, Configuration::Quectel::powerKey);
    Core::core0.setInterval([]() {
      if (Serial.available()) {
        String command = Serial.readString();
        String expectedResponse = command.substring(command.indexOf(' '), command.length());
        command = command.substring(0, command.indexOf(' '));

        expectedResponse.trim();
        command.trim();
        Quectel::sendCommand(command, expectedResponse, [](std::vector<String> responses) {
          // for (auto response: responses) {
          //   Serial_println(response);
          // }
        });
      }
    }, 100); 
  });
}

void Core::setupCore1() {
  // Core::core1.onSetup([]() {
  // });
}

void setup() {
  // make no changes to setup function
  Serial.begin(115200);
  Core::setupCore0();
  Core::setupCore1();
  Core::launch();
}

void loop() {
  // do nothing here
}
