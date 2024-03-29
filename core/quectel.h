#ifndef SERIAL_H__
#define SERIAL_H__
#include "core.h"
#include "mac.h"
#include <vector>
#include <map>
#define MAXIMUM_STRINGS       5
#define RXD                   16
#define TXD                   17
#define IGNORE_ERROR          true
namespace Quectel {
  typedef std::vector<String> SerialResponse_T;
  typedef std::function<void(SerialResponse_T)> SerialCallback_T;
  TimeoutReference rebootTimeout;
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

  bool critical = false;

  std::vector<String> responseList;
  String current;
  Core::Core_T* operationalCore;
  byte powerKey = 25;
  Quectel::SerialCallback_T serialCallback;
  std::function<void()> errorCallback;
  std::function<void()> rebootCallback;
  String readUntil = "OK";
  bool ignoreError = false;
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
    Quectel::sendCommand("AT", "SMS Ready", [](SerialResponse_T response) {
      Quectel::rebooted = true;
      Quectel::operationalCore->clearTimeout(Quectel::rebootTimeout);
      invoke(Quectel::rebootCallback);
    });
    Quectel::operationalCore->setTimeout([]() {
      if (Quectel::rebooted) {
        return;
      }
      Quectel::reboot();
    }, SECONDS(10));
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
  }


  void sendCommand(String command, String readUntil, Quectel::SerialCallback_T callback) {
    if (!readUntil.length()) {
      readUntil = "OK";
    }
    Quectel::serialCallback = callback;
    Quectel::sendCommand(command, readUntil);
  }

  void sendCommand(String command, String readUntil, Quectel::SerialCallback_T onSuccess, std::function<void()> onError) {
    Quectel::errorCallback = onError;
    sendCommand(command, readUntil, onSuccess);
  }


  void sendCommand(String command, String readUntil, bool ignoreError, Quectel::SerialCallback_T callback) {
    Quectel::ignoreError = ignoreError;
    sendCommand(command, readUntil, callback);
  }


  void registerNetwork(std::function<void()> callback, std::function<void()> onError = []() {
    Serial.println("Error in registering network");
  }) {
    Serial.println("Registering network");
    Quectel::errorCallback = onError;
    Quectel::sendCommand("AT+CREG=1", "+CREG: 1", [callback](SerialResponse_T resp) {
      Quectel::sendCommand("AT+CREG?", "+CREG: 1,1|+CREG: 1,5", [callback](SerialResponse_T resp) {
        Quectel::sendCommand("AT+CGREG=1", "+CGREG: 1", [callback](SerialResponse_T resp) {
          Quectel::sendCommand("AT+CGREG?", "+CGREG: 1,1|+CGREG: 1,5", [callback](SerialResponse_T resp) {
            Quectel::serialCallback = nullptr;
            invoke(callback);
          });
        });
      });
    });
  }

  void turnOnInternet(std::function<void()> callback, std::function<void()> onFailure = []() {
    Serial.println("Internet activation failed");
  }, int attempts = 3) {
    if (!attempts) {
      invoke(onFailure);
      return;
    }
    Serial.printf("Turning on internet, attempts left: %d\n", attempts);
    Quectel::sendCommand("AT+QIREGAPP=\"airtelgprs.com\",\"\",\"\"", "OK", [callback, onFailure](SerialResponse_T resp) {
      Quectel::sendCommand("AT+QIACT", "OK", [callback](SerialResponse_T resp) {
        Quectel::serialCallback = nullptr;
        invoke(callback);
      }, onFailure);
    }, [onFailure, callback, &attempts]() {
      Quectel::sendCommand("AT+QIDEACT", "OK", [callback, onFailure, &attempts](SerialResponse_T resp) {
        attempts --;
        invoke(Quectel::turnOnInternet, callback, onFailure, attempts);
      });
    });
  }

  
  namespace MQTT {
    std::map<String, std::function<void(String)>> events;
    Quectel::ServerConfiguration configuration;

    std::function<void()> connectionCallback;
    std::function<void()> errorCallback;

    void configure(String serverURL, int port, String username = "", String password = "", String apn = "airtelgprs.com") {
      MQTT::configuration.set(serverURL, port, username, password, apn);
    }

    void onConnect(std::function<void()> callback) {
      MQTT::connectionCallback = callback;
    }
    

    void onError(std::function<void()> callback) {
      MQTT::errorCallback = callback;
    }

    void removeAllListeners() {
      MQTT::events.clear();
    }
    
    void subscribeEvent(String event, std::function<void()> callback) {
      Quectel::sendCommand(String("AT+QMTSUB=0,1,\"") + event + "\",1", "+QMTSUB: 0,1,0,1", [event, callback](SerialResponse_T resp) {
        Serial_println(String("subscribed to \"") + event + '"' + "successfully");
        invoke(callback);
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
        MQTT::events.erase(it);
      } 
      MQTT::unsubscribeEvent(event, callback);
    }
    
    void on(String event, std::function<void(String)> callback, std::function<void()> onSuccess = nullptr) {
      MQTT::events[event] = callback;
      MQTT::subscribeEvent(event, onSuccess);
    }

    void publish(String event, String data, std::function<void()> callback = nullptr) {
      Quectel::sendCommand(String("AT+QMTPUB=0,1,1,0,\"") + event + "\"", ">", [callback, data](SerialResponse_T resp) { 
        Quectel::sendCommand(data+"", "+QMTPUB: 0,1,0", [callback](SerialResponse_T resp) {
          invoke(callback);
        });
      });
    }

    void connect(std::function<void()> callback = nullptr) {
      Quectel::sendCommand(String("AT+QMTOPEN=0,\"") + MQTT::configuration.serverURL + "\"," + MQTT::configuration.port, "+QMTOPEN: 0,0", [callback](SerialResponse_T resp) {
        Quectel::sendCommand(String("AT+QMTCONN=0,\"") + MAC::getMac() + "\",\""  + MQTT::configuration.username + "\",\"" + MQTT::configuration.password + '"', "+QMTCONN: 0,0,0", [callback](SerialResponse_T resp) {
          invoke(callback);
          invoke(MQTT::connectionCallback);
          Quectel::serialCallback = nullptr;
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

  bool includeAnyPart(String host, String parts) {
    String subPart = parts;
    int seperatorIndex = parts.indexOf('|');
    if (seperatorIndex > -1) {
      subPart = parts.substring(0, seperatorIndex);
      parts = parts.substring(seperatorIndex+1, parts.length());
    } else {
      return host.indexOf(subPart) > -1;
    }
    if (host.indexOf(subPart) > -1) {
      return true;
    }
    return includeAnyPart(host, parts);
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
        Serial.print(ch);
        if (includeAnyPart(current, Quectel::readUntil)) {
          current = "";
          Serial.println();
          invoke(Quectel::serialCallback, responseList);
        }
        
        if (current.indexOf("ERROR") > -1) {
          current = "";
          if (Quectel::ignoreError) {
            Serial.println("error ignored");
            Quectel::errorCallback = nullptr;
            Quectel::ignoreError = false;
            invoke(Quectel::serialCallback, responseList);
            return;
          }
          invoke(Quectel::errorCallback);
          Quectel::flush();
          Quectel::errorCallback = nullptr;
        }
        if (ch == '\n') {
          if (responseList.size() >= MAXIMUM_STRINGS) {
            responseList.erase(responseList.begin());
          }
          current.trim();
          if (!current.length()) {
            return;
          }
          responseList.push_back(current);
          
          if (current.indexOf("+QMTRECV:") > -1) {
            MQTT::handleData(current);
          }
          if (current.indexOf("+QMTSTAT: 0,1") > -1) {
            invoke(MQTT::errorCallback);
            Quectel::errorCallback = nullptr;
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