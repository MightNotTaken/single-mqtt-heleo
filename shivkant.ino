#include "core/core.h"
#include "core/WIFI.h"
#include "core/quectel.h"
#include "device.h"
#include "data-peripheral.h"
#include "display.h"
#include "core/OTA.h"

void Core::setupCore0() {
  Core::core0.onSetup([]() {
    Serial.begin(115200);
    DataPeripheral::begin(&Core::core0);
    Device::begin(&Core::core0);
    Display::begin(&Core::core0);
    DataPeripheral::onData([](DataPeripheral::Data data) {
      data.show();
    });

    Core::core0.setInterval([]() {
      if (Serial.available()) {
        String command = Serial.readString();
        command.trim();
        command += "\r\n";
        Quectel::sendCommand(command);
      }
    }, 100);
    
    WIFI::configure(&Core::core0);
    WIFI::onConnect([]() {
      Serial_println("Wifi connected");
    });
    WIFI::onDisconnect([]() {
      Serial_println("Wifi disconnected");
    });
    WIFI::Hotspot::configure("HN");
    WIFI::Hotspot::turnOn();
    WIFI::connect();
    
    Quectel::onReboot([]() {
      Serial.println("rebooted");
      // Core::core0.setTimeout([]() {
      //   Serial_println("connecting to MQTT Broker");
      //   Quectel::MQTT::configure(
      //     Configuration::MQTT::baseURL,
      //     Configuration::MQTT::port,
      //     Configuration::MQTT::username,
      //     Configuration::MQTT::password,
      //     Configuration::MQTT::apn
      //   );
      //   Quectel::MQTT::onError([]() {
      //     Quectel::MQTT::connect();
      //   });
      //   Quectel::MQTT::onConnect([]() {
      //     Serial_println("MQTT connected succcessfully");
      //     Device::onRelayUpdate([](uint8_t index, uint8_t state) {
      //       if (index < Device::relays.size()) {
      //         digitalWrite(Device::relays[index], state);
      //         Device::updateStateString(String("r") + index, state);
      //       }
      //     });
      //     Device::onFanUpdate([](uint8_t fan, uint8_t state) {
      //       showX(fan);
      //       showX(state);
      //       Device::updateStateString(String("d") + fan, state);
      //     });
      //     Device::onFirmwareUpdate([](String url, String version) {
      //       OTA::begin(url + Configuration::getBoardSpecQuery(version));
      //       if (OTA::performUpdate()) {
      //         ESP.restart();
      //       } else {
      //         WiFi.disconnect();
      //       }            
      //     });
      //     Device::listen([]() {
      //       Serial_println("Device listening");
      //       Device::reRegister([]() {
      //         Serial_println("device registered");
      //         Device::updateConfiguration([]() {
      //           Serial_println("configuration updated succcessfully");
      //           Device::sendFirmwareRequest();
      //         });
      //       });
      //     });          
      //   });
      //   Quectel::MQTT::connect([]() {
      //     Quectel::MQTT::connect();
      //   });
      // }, SECONDS(5));

      Quectel::registerNetwork([]() {
        Serial.println("Successfully registered");
        Quectel::turnOnInternet([]() {
          Serial.println("Internet turned on");
        });
      });
    }); 
    Quectel::begin(&core0, Configuration::Quectel::powerKey);
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
