#ifndef DATA_PERIPHERAL_H__
#define DATA_PERIPHERAL_H__
#include  "core/core.h"
#include <SoftwareSerial.h>
#include <functional>
#define RX_DP       26
#define TX_DP       27
namespace DataPeripheral {

  struct Data {
    float v1;
    float v2;
    float v3;
    float c1;
    float c2;
    float c3;
    String data;
    void show() {
      showX(v1);
      showX(v2);
      showX(v3);
      showX(c1);
      showX(c2);
      showX(c3);
    }
    void add(char ch) {
      if (this->data.length() > 100) {
        this->data = "";
      }
      this->data += ch;
    }
    void parse() {
      sscanf(this->data.c_str(), "%f %f %f %f %f %f", &v1, &v2, &v3, &c1, &c2, &c3);
      this->data = "";
    }
  } parser;
  Core::Core_T* core;
  SoftwareSerial serial;
  std::function<void(DataPeripheral::Data)> dataCallback;

  void onData(std::function<void(DataPeripheral::Data)> callback) {
    DataPeripheral::dataCallback = callback;
  }

  void loop() {
    static bool looping = false;
    if (looping) {
      return;
    }
    looping = true;
    DataPeripheral::core->setInterval([]() {
      while (DataPeripheral::serial.available()) {
        char ch = (char)DataPeripheral::serial.read();
        if (ch == '\n') {
          DataPeripheral::parser.parse();
          invoke(DataPeripheral::dataCallback, DataPeripheral::parser);
        } else {
          DataPeripheral::parser.add(ch);
        }
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