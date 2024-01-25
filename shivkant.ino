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
        Quectel::MQTT::connect([]() {
          Serial_println("MQTT connected succcessfully");
          Quectel::MQTT::on("myevent", [](String data) {
            Serial_println(data + " received from myevent");
          });
          // Core::core0.setTimeout([]() {
          //   // Serial_println("unsubscribing to myevent");
          //   // Quectel::MQTT::unsubscribe("myevent", []() {
          //   //   Serial_println("Succcessfully unsubscribed from myevent");
          //   //   Serial_println("publishing data");
          //   //   Core::core0.setInterval([]() {
          //   //     Quectel::MQTT::publish("3C610530D9B4", "this is a sample data", []() {
          //   //       Serial_println("data published");
          //   //     });
          //   //   }, SECONDS(3));
          //   // });
          // }, SECONDS(12));

        }, []() {
          Serial_println("Error in connecting to MQTT");
        });
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
