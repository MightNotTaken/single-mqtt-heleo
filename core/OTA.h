#ifndef OTA_H__
#define OTA_H__
#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <functional>
#include "utility.h"

namespace OTA {
  uint8_t status;
  void update_started();
  void update_finished();
  void update_progress(int, int);
  void update_error(int);
  std::function<void()> statusCallback = nullptr;
  std::function<void()> completionCallback = nullptr;
  int lastPercentage = 0;

  String firmwareURL;

  void begin(String URL) {
    OTA::firmwareURL = URL;
    showX(OTA::firmwareURL);
    
    httpUpdate.onStart(update_started);
    httpUpdate.onEnd(update_finished);
    httpUpdate.onProgress(update_progress);
    httpUpdate.onError(update_error);
  }

  void whileProgramming(std::function<void()> callback) {
    OTA::statusCallback = callback;
  }
  
  void onFinished(std::function<void()> callback) {
    OTA::completionCallback = callback;
  }
  
  void update_started() {
    Serial.println("CALLBACK:  HTTP update process started");
  }

  void update_finished() {
    Serial.println("CALLBACK:  HTTP update process finished");
    invoke(OTA::completionCallback);
  }

  void update_progress(int cur, int total) {
    if (100 * cur / total != lastPercentage) {
      lastPercentage = 100 * cur / total;
      Serial.printf("Writing at 0x%08x... (%d %%)\n", cur, lastPercentage);
      invoke(OTA::statusCallback);
    }
  }

  void update_error(int err) {
    Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
  }


  bool performUpdate() {
    WiFiClient client;
    t_httpUpdate_return ret = httpUpdate.update(client, OTA::firmwareURL);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        return false;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        return false;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        return true;
    }
  }
};

#endif