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
    Quectel::sendCommand("AT", "Call Ready", [](SerialResponse_T response) {
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

  void httpGET(String url) {
    Quectel::sendCommand("AT", "OK", [url](SerialResponse_T resp) {
      Quectel::sendCommand("AT+CPIN?", "OK", [url](SerialResponse_T resp) {
        Quectel::sendCommand("AT+CREG?", "OK", [url](SerialResponse_T resp) {
          Quectel::sendCommand("AT+CGREG?", "OK", [url](SerialResponse_T resp) {
            Quectel::sendCommand("AT+QIFGCNT=0", "OK", [url](SerialResponse_T resp) {
              Quectel::sendCommand(String("AT+QICSGP=1,\"") + "airtelgprs.com" + '"' , "OK", [url](SerialResponse_T resp) {
                Quectel::sendCommand("AT+QIREGAPP", "OK", [url](SerialResponse_T resp) {
                  Quectel::sendCommand("AT+QIACT", "OK", [url](SerialResponse_T resp) {
                    Quectel::sendCommand(String("AT+QHTTPURL=") + String(url.length()) + ',' + 30, "CONNECT", [url](SerialResponse_T resp) {
                      Quectel::sendCommand(url, "OK", [url](SerialResponse_T resp) {
                        Quectel::sendCommand("AT+QHTTPGET=60", "OK", [](SerialResponse_T resp) {
                          Quectel::sendCommand("AT+QHTTPREAD=30", "CONNECT", [](SerialResponse_T resp) {
                            Quectel::critical = true;
                            Serial.println("dopne");
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
      });
    });
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
    delay(250);
    Quectel::flush();
    Quectel::readUntil = readUntil;
    Serial2.print(command);
    Serial2.write('\r');
    Serial2.write('\n');
    showX(command);
  }


  void sendCommand(String command, String readUntil, Quectel::SerialCallback_T callback) {
    if (!readUntil.length()) {
      readUntil = "OK";
    }
    Quectel::serialCallback = callback;
    Quectel::sendCommand(command, readUntil);
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
          Quectel::operationalCore->setTimeout([callback]() {
            invoke(callback);
          }, SECONDS(.5));
        });
      });
    }

    void connect() {
      Quectel::errorCallback = []() {
        invoke(MQTT::errorCallback);
      };
      Quectel::sendCommand("AT", "OK", [](SerialResponse_T resp) {
        Quectel::sendCommand("AT+CPIN?", "OK", [](SerialResponse_T resp) {
          Quectel::sendCommand("AT+CREG?", "OK", [](SerialResponse_T resp) {
            Quectel::sendCommand("AT+CGREG?", "OK", [](SerialResponse_T resp) {
              Quectel::sendCommand("AT+QIMODE=0", "OK", [](SerialResponse_T resp) {
                Quectel::sendCommand(String("AT+QICSGP=1,\"") + MQTT::configuration.apn + '"' , "OK", [](SerialResponse_T resp) {
                  Quectel::sendCommand("AT+QIREGAPP", "OK", [](SerialResponse_T resp) {
                    Quectel::sendCommand("AT+QIACT", "OK", [](SerialResponse_T resp) {
                      Quectel::sendCommand(String("AT+QMTOPEN=0,\"") + MQTT::configuration.serverURL + "\"," + MQTT::configuration.port, "+QMTOPEN: 0,0", [](SerialResponse_T resp) {
                        Quectel::sendCommand(String("AT+QMTCONN=0,\"") + MAC::getMac() + "\",\""  + MQTT::configuration.username + "\",\"" + MQTT::configuration.password + '"', "+QMTCONN: 0,0,0", [](SerialResponse_T resp) {
                          Quectel::errorCallback = nullptr;
                          invoke(MQTT::connectionCallback);
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
        // Serial.print(ch);
        if (current.indexOf(Quectel::readUntil) > -1) {
          current = "";
          invoke(Quectel::serialCallback, responseList);
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
          
          if (current.indexOf("ERROR") > -1) {
            current = "";
            invoke(Quectel::errorCallback);
            Quectel::flush();
            Quectel::errorCallback = nullptr;
          }
          if (current.indexOf("+QMTRECV:") > -1) {
            Serial_println("data received");
            MQTT::handleData(current);
          }
          if (current.indexOf("+QMTSTAT: 0,1") > -1) {
            invoke(MQTT::errorCallback);
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