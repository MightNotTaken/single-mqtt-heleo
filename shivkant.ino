#include "core/core.h"
#include "core/quectel.h" 

void Core::setupCore0() {
  Core::core0.onSetup([]() {
    Serial.begin(115200);
    Quectel::onReboot([]() {
      Serial.println("rebooted");
      Quectel::configureMQTT("heleo.app", 1883, "rajesh", "Rajesh.007");
      Quectel::connectToMQTT();
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
          for (auto response: responses) {
            Serial_println(response);
          }
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
