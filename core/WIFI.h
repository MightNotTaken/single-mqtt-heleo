#ifndef WIFI_H__
#define WIFI_H__
#include "core.h"
#include "web-server.h"
#include <functional>
#include <WiFi.h>
#include "JSON.h"
#include "database.h"
#include "web-server.h"
#include  <utility>

#define WIFI_INDEX_FILE_PATH         "/wifi/index.txt"

namespace WIFI {

  enum {
    DISCONNECTED = 0,
    CONNECTED,
    CONNECTING
  };

  typedef uint8_t wifi_status_t;

  String ssid;
  String password;
  String defaultSSID = "_Tahir_";
  String defaultPASS = "bhaijaan";

  Core::Core_T* operationalCore;
  IntervalReference statusTracker;
  TimeReference timeoutDuration;
  wifi_status_t status;
  
  std::pair<String, String> loadCredentials();
  std::pair<String, String> getCredentials(byte);
  String configFile(int);
  void updateCredentials();
  void deleteCredentials(byte);
  void nextCredentials(int);
  void restorFactory();
  void turnOffHotspot();
  void turnOnHotspot();
  void registerRouteHandlers();
  void loop();

  void onConnect(std::function<void()>);
  void onDisconnect(std::function<void()>);
  void onConnectionStart(std::function<void(String, String)>);
  void onTimeout(std::function<void()>);
  void onFirstTimeConnection(std::function<void()>);
  void connect(Core::Core_T*, TimeReference);
  
  
  std::function<void()> connectCallback;
  std::function<void()> disconnectCallback;
  std::function<void(String, String)> connectionStartCallback;
  std::function<void()> connectionTimeoutCallback;
  std::function<void()> firstTimeConnectionCallback;
  namespace Hotspot {
    String APSSID = MAC::getMac();
    String APPASS = "12345678";

    void configure(String deviceName, String password = "12345678") {
      Hotspot::APSSID = deviceName + "-" + MAC::getMac();
      Hotspot::APPASS = password;
    }

    void turnOn() {
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(APSSID.c_str(), APPASS.c_str());
      HTTPServer::begin(WIFI::operationalCore);
      HTTPServer::listen();
    }

    void turnOff() {
      WiFi.softAPdisconnect(true);
    }

  };


  void flush() {
    WIFI::operationalCore->clearInterval(WIFI::statusTracker);
  }

  void configure(Core::Core_T* operationalCore, TimeReference timeoutDuration = SECONDS(15)) {
    WIFI::operationalCore = operationalCore;
    WIFI::timeoutDuration = timeoutDuration;
  }
  void connect() {
    auto [ssid, password] = WIFI::loadCredentials();
    WIFI::flush();
    WiFi.begin(ssid, password);
    invoke(WIFI::connectionStartCallback, ssid, password);
    WIFI::status = WIFI::CONNECTING;
    WIFI::loop();
    WIFI::statusTracker = WIFI::operationalCore->setTimeout([]() {
      if (WiFi.status() != WL_CONNECTED) {
        invoke(connectionTimeoutCallback);
      }
    }, WIFI::timeoutDuration);
  }

  void onConnect(std::function<void()> callback) {
    WIFI::connectCallback = callback;
  }

  void onConnectionStart(std::function<void(String, String)> callback) {
    WIFI::connectionStartCallback = callback;
  }

  void onDisconnect(std::function<void()> callback) {
    WIFI::disconnectCallback = callback;
  }

  void onTimeout(std::function<void()> callback) {
    WIFI::connectionTimeoutCallback = callback;
  }

  void onFirstTimeConnection(std::function<void()> callback) {
    WIFI::firstTimeConnectionCallback = callback;
  }
  
  void registerRouteHandlers() {
    HTTPServer::on("/set-credentials", [](WebServer& server) {
      WIFI::ssid = server.arg(0);
      WIFI::password = server.arg(1);
      Serial.println(WIFI::ssid);
      Serial.println(WIFI::password);
      WIFI::updateCredentials();
      server.send(200, "application/json", "{\"type\":\"success\",\"message\":\"connection okay\"}");
    });
    HTTPServer::on("/api/wifi/add", [](WebServer& server) {
      WIFI::ssid = server.arg("apName");
      WIFI::password = server.arg("apPass");
      Serial.println(WIFI::ssid);
      Serial.println(WIFI::password);
      WIFI::updateCredentials();
      server.send(200, "application/json", "{\"type\":\"success\",\"message\":\"connection okay\"}");
    });
    
    HTTPServer::on("/api/wifi/id", [](WebServer& server) {
      int id = server.arg(0).toInt();
      WIFI::deleteCredentials(id);      
      server.send(200, "application/json", "{\"type\":\"success\",\"message\":\"connection okay\"}");
    });
    
    HTTPServer::on("/api/wifi/config", [](WebServer& server) {
      int id = server.arg(0).toInt();
      auto [ssid0, password0] = WIFI::getCredentials(0);
      auto [ssid1, password1] = WIFI::getCredentials(1);
      auto [ssid2, password2] = WIFI::getCredentials(2);
      std::function<String(String, String)> toJSON = [](String a, String b) {
        return String("{\"apName\":\"") + a + "\",\"apPass\":" + b + "\"}";
      };
      server.send(200, "application/json", 
        String("[") + toJSON(ssid0, password0) + ',' + toJSON(ssid1, password1) + ',' + toJSON(ssid2, password2) + "]"
      );
    });
  }

  void restorFactory() {
    Database::removeFile(WIFI_INDEX_FILE_PATH);
    Database::removeFile(WIFI::configFile(0));
    Database::removeFile(WIFI::configFile(1));
    Database::removeFile(WIFI::configFile(2));
  }

  String configFile(int index) {
    return String("/wifi/") + index + ".json";
  }

  void nextCredentials(int attempt = 3) {
    if (!attempt) {
      return;
    }
    if (Database::readFile(WIFI_INDEX_FILE_PATH)) {
      int index = (Database::payload().toInt() + 1) % 3;
      Database::writeFile(WIFI_INDEX_FILE_PATH, String(index));
    }
    auto [ssid, password] = WIFI::loadCredentials();
    if (!ssid.length()) {
      nextCredentials(attempt - 1);
    }
  }

  void deleteCredentials(byte index) {
    index %= 3;
    String json = JSON::JSON();
    JSON::add(json, "apName", "");
    JSON::add(json, "apPass", "");
    Database::writeFile(WIFI::configFile(index), json);
  }

  void updateCredentials() {
    if (!Database::hasFile(WIFI_INDEX_FILE_PATH)) {
      Database::writeFile(WIFI_INDEX_FILE_PATH, "2");
    }
    if (Database::readFile(WIFI_INDEX_FILE_PATH)) {
      int index = Database::payload().toInt();
      index ++;
      index %= 3;
      String json = JSON::JSON();
      JSON::add(json, "apName", WIFI::ssid);
      JSON::add(json, "apPass", WIFI::password);
      Database::writeFile(WIFI::configFile(index), json);
      Database::writeFile(WIFI_INDEX_FILE_PATH, String(index));
    }
  }

  std::pair<String, String> getCredentials(byte index) {
    if(Database::readFile(WIFI::configFile(index))) {
      String ssid = JSON::read(Database::payload(), "apName");
      String pass = JSON::read(Database::payload(), "apPass");
      return std::make_pair(ssid, pass);
    }
    return std::make_pair("", "");
  }

  std::pair<String, String> loadCredentials() {
    if (!Database::readFile(WIFI_INDEX_FILE_PATH)) {
      WIFI::ssid = WIFI::defaultSSID;
      WIFI::password = WIFI::defaultPASS;
      return std::make_pair(WIFI::ssid, WIFI::password);
    } else {
      int indx = Database::payload().toInt();
      std::vector<int> indices = {indx % 3, (indx + 1)%3, (indx + 2) % 3};
      for (auto index: indices) {
        auto [ssid, password] = WIFI::getCredentials(index);
        if (ssid.length()) {
          if (index != indx) {
            Database::writeFile(WIFI_INDEX_FILE_PATH, String(index));
          }
          return std::make_pair(ssid, password);
        }
      }
    }
    return std::make_pair(WIFI::ssid, WIFI::password);
  }

  void loop() {
    static bool looping = false;
    if (looping) {
      return;
    }
    WIFI::registerRouteHandlers();
    WIFI::operationalCore->setInterval([]() {
      if ((WIFI::status == WIFI::CONNECTING || WIFI::status == WIFI::DISCONNECTED) && WiFi.status() == WL_CONNECTED) {
        WIFI::status = WIFI::CONNECTED;
        invoke(WIFI::connectCallback);
        invoke(WIFI::firstTimeConnectionCallback);
        WIFI::firstTimeConnectionCallback = nullptr;  // remove listener forever
      } else if (WIFI::status == WIFI::CONNECTED && WiFi.status() != WL_CONNECTED) {
        WIFI::status = WIFI::DISCONNECTED;
        invoke(WIFI::disconnectCallback);
      }
    }, 10);
  }
};
#endif