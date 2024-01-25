#ifndef SERIAL_H__
#define SERIAL_H__
#include "core.h"
#include <vector>
#define MAXIMUM_STRINGS       5
#define RXD                   16
#define TXD                   17
namespace Quectel {
  typedef std::vector<String> SerialResponse_T;
  typedef std::function<void(SerialResponse_T)> SerialCallback_T;
  struct ServerConfiguration {
    String baseURL;
    int port;
    String username;
    String password;
    ServerConfiguration() {}
    void set(
      String baseURL,
      int port,
      String username,
      String password
    ) {
      this->baseURL = baseURL;
      this->port = port;
      this->username = username;
      this->password = password;
    }

  } MQTTConfiguration;
  std::vector<String> responseList;
  String current;
  Core::Core_T* operationalCore;
  byte powerKey = 25;
  Quectel::SerialCallback_T serialCallback;
  std::function<void()> rebootCallback;
  String readUntil = "OK";
  bool rebooted = false;
  void sendCommand(String, String, Quectel::SerialCallback_T);

  void loop();
  void begin();
  void flush();
  void reboot();

  void flush() {
    Quectel::responseList.clear();
    while (Serial2.available()) {
      Serial2.read();
    }
  }

  void begin(Core::Core_T* operationalCore, byte powerKey) {
    Quectel::operationalCore = operationalCore;
    Serial2.begin(115200, SERIAL_8N1, RXD, TXD);
    Quectel::powerKey = powerKey; 
    pinMode(powerKey, OUTPUT);
    Quectel::loop();
    Serial_println("begining");
    Quectel::reboot();
  }



  void reboot() {
    Quectel::rebooted = false;
    digitalWrite(powerKey, HIGH);
    delay(1000);
    digitalWrite(powerKey, LOW);
    delay(500);
    Quectel::sendCommand("AT", "OK", [](SerialResponse_T response) {
      Quectel::rebooted = true;
      invoke(Quectel::rebootCallback);
    });
    Quectel::operationalCore->setTimeout([]() {
      if (Quectel::rebooted) {
        return;
      }
      Quectel::reboot();
    }, SECONDS(1));
  }

  void onReboot(std::function<void()> callback) {
    Quectel::rebootCallback = callback;
  }

  void sendCommand(String command, String readUntil="OK") {
    command.trim();
    if (command == "reset") {
      ESP.restart();
      return;
    }
    Quectel::flush();
    Quectel::readUntil = readUntil;
    Serial2.print(command);
    Serial2.write('\r');
    Serial2.write('\n');
    showX(command);
  }


  void sendCommand(String command, String readUntil, Quectel::SerialCallback_T callback) {
    Quectel::serialCallback = callback;
    Quectel::operationalCore->setTimeout([command, readUntil]() {
      Quectel::sendCommand(command, readUntil);

    }, 500);
  }

  void configureMQTT(String serverURL, int port, String username = "", String password = "") {
    Quectel::MQTTConfiguration.set(serverURL, port, username, password);
  }
  
  void connectToMQTT() {
    Quectel::sendCommand("AT", "OK", [](SerialResponse_T resp) {
      Serial.println("->AT OK");
      Quectel::sendCommand("AT+CPIN?", "OK", [](SerialResponse_T resp) {
        Serial.println("->AT+cpin OK");
        Quectel::sendCommand("AT+CREG?", "OK", [](SerialResponse_T resp) {
          Serial.println("->AT+creg OK");
          Quectel::sendCommand("AT+CGREG?", "OK", [](SerialResponse_T resp) {
            Serial.println("->AT+cgreg OK");
            Quectel::sendCommand("AT", "OK", [](SerialResponse_T resp) {
              Serial.println("->AT OK");
              Serial.println("All commands successfully executed");
            });
          });
        });
      });
    });
  }

  void loop() {
    static bool looping = false;
    if (looping) {
      return;
    }
    looping = true;
    Quectel::operationalCore->setInterval([]() {
      while (Serial2.available()) {
        char ch = Serial2.read();
        if (ch == '\n') {
          if (responseList.size() >= MAXIMUM_STRINGS) {
            responseList.erase(responseList.begin());
          }
          current.trim();
          if (!current.length()) {
            return;
          }
          responseList.push_back(current);
          if (current.indexOf(Quectel::readUntil) >= 0) {
            invoke(Quectel::serialCallback, responseList);
            Quectel::flush();
          }
          current = "";
        } else {
          if (ch != '\r') {
            current += ch;
          }
        }
      }
    }, 100);
  }
};
#endif