#ifndef DATA_PERIPHERAL_H__
#define DATA_PERIPHERAL_H__
#include "core/core.h"
#include "core/database.h"
#include <SoftwareSerial.h>
#include <functional>
#define RX_DP       26
#define TX_DP       27
namespace DataPeripheral {
  struct Phase {
    float v;
    float i;
    float E;
    String name;
    uint32_t lastCalculatedOn;

    void update(float v, float i) {
      this->v = v;
      this->i = i;
      float deltaT = (millis() - lastCalculatedOn) / (1.0 * HOURS(1));
      this->E += v * i * deltaT;
    }

    String fileName() {
      return this->name + ".txt";
    }

    

    void show() {
      Serial.printf("%cv: %f\n", this->name, this->v);
      Serial.printf("%ci: %f\n", this->name, this->i);
      Serial.printf("%fkWh\n", this->E);
    }

    Phase(String name) {
      this->name = name;
      this->lastCalculatedOn = millis();
      if (!Database::hasFile(this->fileName())) {
        Database::writeFile(this->fileName(), String(0)); 
      }
      if (Database::readFile(this->fileName())) {
        this->E = Database::payload().toFloat();
      }
      Core::core0.setInterval([this]() {
        Database::writeFile(this->fileName(), String(this->E));
      }, SECONDS(10));
    }
  } R("R"), Y("Y"), B("B");

  float totalEnergy() {
    return R.E + Y.E + B.E;
  }
  struct Data {    
    float Rv;
    float Ri;
    float Yv;
    float Yi;
    float Bv;
    float Bi;
    String data;
    void add(char ch) {
      if (this->data.length() > 100) {
        this->data = "";
      }
      this->data += ch;
    }
    void show() {
      return;
      showX(Rv);
      showX(Ri);
      showX(Yv);
      showX(Yi);
      showX(Bv);
      showX(Bi);
    }
    void parse() {
      sscanf(this->data.c_str(), "%f %f %f %f %f %f", &Rv, &Ri, &Yv, &Yi, &Bv, &Bi);
      R.update(Rv, Ri);
      Y.update(Yv, Yi);
      B.update(Bv, Bi);
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
