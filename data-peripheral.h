#ifndef DATA_PERIPHERAL_H__
#define DATA_PERIPHERAL_H__
#include  "core/core.h"
#include <SoftwareSerial.h>
#define TX_DP       26
#define RX_DP       277
namespace DataPeripheral {
  Core::Core_T* core;
  SoftwareSerial serial;

  void loop() {
    static bool looping = false;
    if (looping) {
      return;
    }
    looping = true;
    DataPeripheral::core->setInterval([]() {
      while (DataPeripheral::serial.available()) {
        Serial_print((char)DataPeripheral::serial.read());
      }
    }, 100);
  }
  void begin(Core::Core_T* core) {
    DataPeripheral::core  = core;
    DataPeripheral::serial.begin(115200, SWSERIAL_8N1, RX_DP, TX_DP);
    DataPeripheral::loop();
  }
};
#endif