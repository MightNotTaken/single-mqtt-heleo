#include "core/core.h"
#include "core/quectel.h" 

void Core::setupCore0() {
  Core::core0.onSetup([]() {
    Serial.begin(115200);
    Quectel::onReboot([]() {
      Serial.println("rebooted");
      Core::core0.setTimeout([]() {
        Serial_println("connecting to MQTT Broker");
        Quectel::MQTT::configure("heleo.app", 1883, "rajesh", "Rajesh.007", "airtelgprs.com");
        Quectel::MQTT::onError([]() {
          Quectel::begin(&core0, 25);
        });
        Quectel::MQTT::onConnect([]() {
          Serial_println("MQTT connected succcessfully");
          Quectel::MQTT::on(MAC::getMac(), [](String data) {
            Serial_println(data + " received from myevent");
          });
          Quectel::MQTT::publish("register", MAC::getMac(), []() {
            Serial_println("Device register called");
          });
        });
        Quectel::MQTT::connect();
      }, SECONDS(10));
    });
    Quectel::begin(&core0, 25);
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
