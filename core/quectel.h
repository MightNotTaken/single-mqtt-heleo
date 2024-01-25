#ifndef SERIAL_H__
#define SERIAL_H__
#include "core.h"
#include "mac.h"
#include <vector>
#include <map>
#define MAXIMUM_STRINGS       5
#define RXD                   16
#define TXD                   17
namespace Quectel {
  typedef std::vector<String> SerialResponse_T;
  typedef std::function<void(SerialResponse_T)> SerialCallback_T;

  struct ServerConfiguration {
    String serverURL;
    int port;
    String username;
    String password;
    String apn;
    ServerConfiguration() {}
    void set(
      String serverURL,
      int port,
      String username,
      String password,
      String apn
    ) {
      this->serverURL = serverURL;
      this->port = port;
      this->username = username;
      this->password = password;
      this->apn = apn;
    }

  };

  std::vector<String> responseList;
  String current;
  Core::Core_T* operationalCore;
  byte powerKey = 25;
  Quectel::SerialCallback_T serialCallback;
  std::function<void()> errorCallback;
  std::function<void()> rebootCallback;
  String readUntil = "OK";
  bool rebooted = false;
  void sendCommand(String, String, Quectel::SerialCallback_T);

  bool isCallReady = false;
  bool isSMSReady = false;
  std::function<void()> moduleReadyCallback;
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

  void onModuleReady(std::function<void()> callback) {
    Quectel::moduleReadyCallback = callback;
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


  
  namespace MQTT {
    std::map<String, std::function<void(String)>> events;
    Quectel::ServerConfiguration configuration;

    void configure(String serverURL, int port, String username = "", String password = "", String apn = "airtelgprs.com") {
      Quectel::MQTT::configuration.set(serverURL, port, username, password, apn);
    }
    
    void connect(std::function<void()> callback, std::function<void()> errorCallback) {
      Quectel::errorCallback = errorCallback;
      Quectel::sendCommand("AT", "OK", [callback](SerialResponse_T resp) {
        Quectel::sendCommand("AT+CPIN?", "OK", [callback](SerialResponse_T resp) {
          Quectel::sendCommand("AT+CREG?", "OK", [callback](SerialResponse_T resp) {
            Quectel::sendCommand("AT+CGREG?", "OK", [callback](SerialResponse_T resp) {
              Quectel::sendCommand("AT+QIMODE=0", "OK", [callback](SerialResponse_T resp) {
                Quectel::sendCommand(String("AT+QICSGP=1,\"") + Quectel::MQTT::configuration.apn + '"' , "OK", [callback](SerialResponse_T resp) {
                  Quectel::sendCommand("AT+QIREGAPP", "OK", [callback](SerialResponse_T resp) {
                    Quectel::sendCommand("AT+QIACT", "OK", [callback](SerialResponse_T resp) {
                      Quectel::sendCommand(String("AT+QMTOPEN=0,\"") + Quectel::MQTT::configuration.serverURL + "\"," + Quectel::MQTT::configuration.port, "+QMTOPEN: 0,0", [callback](SerialResponse_T resp) {
                        Quectel::sendCommand(String("AT+QMTCONN=0,\"") + MAC::getMac() + "\",\""  + Quectel::MQTT::configuration.username + "\",\"" + Quectel::MQTT::configuration.password + '"', "+QMTCONN: 0,0,0", [callback](SerialResponse_T resp) {
                          callback();
                        });
                      });
                    });
                  });
                });
              });
            });
          });
        });
      });
    }

    void subscribeEvent(String event) {
      Quectel::sendCommand(String("AT+QMTSUB=0,1,\"") + event + "\",1", "+QMTSUB: 0,1,0,1", [event](SerialResponse_T resp) {
        Serial_println(String("subscribed to \"") + event + '"' + "successfully");
      });
    }

    void unsubscribeEvent(String event, std::function<void()> callback) {
      Quectel::sendCommand(String("AT+QMTUNS=0,1,\"") + event + "\"", "OK", [callback](SerialResponse_T resp) {
        callback();
      });
    }
    
    void unsubscribe(String event, std::function<void()> callback) {
      auto it = events.find(event);
      if (it != events.end()) {
        MQTT::unsubscribeEvent(event, callback);
        MQTT::events.erase(it);
      }
    }
    
    void on(String event, std::function<void(String)> callback) {
      MQTT::events[event] = callback;
      MQTT::subscribeEvent(event);
    }

    void publish(String event, String data, std::function<void()> callback) {
      Quectel::sendCommand(String("AT+QMTPUB=0,1,1,0,\"") + event + "\"", ">", [callback, data](SerialResponse_T resp) { 
        Quectel::sendCommand(data+"", "+QMTPUB: 0,1,0", [callback](SerialResponse_T resp) {
          callback();
        });
      });
    }

    void handleData(String data) {
      data = data.substring(data.indexOf(':') + 6);
      String event = data.substring(0, data.indexOf(','));
      data = data.substring(data.indexOf(',') + 1, data.length());
      invoke(MQTT::events[event], data);
    }
  };

  
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
          Serial.println(current);
          if (responseList.size() >= MAXIMUM_STRINGS) {
            responseList.erase(responseList.begin());
          }
          current.trim();
          if (!current.length()) {
            return;
          }
          responseList.push_back(current);
          if (current.indexOf(Quectel::readUntil) > -1) {
            current = "";
            invoke(Quectel::serialCallback, responseList);
            Quectel::flush();
          }
          if (current.indexOf("ERROR") > -1) {
            current = "";
            invoke(Quectel::errorCallback);
            Quectel::flush();
            Quectel::errorCallback = nullptr;
          }
          if (current.indexOf("+QMTRECV:") > -1) {
            Quectel::MQTT::handleData(current);
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