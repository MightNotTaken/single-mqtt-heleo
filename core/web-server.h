#ifndef WEB_SERVER_H__
#define WEB_SERVER_H__

#include <HTTPClient.h>
#include <WebServer.h>
#include <functional>
#include "core.h"
#include "utility.h"

namespace HTTPServer {
  WebServer server;
  IntervalReference listener;
  Core::Core_T* operationalCore;
  
  void cors() {
    HTTPServer::server.sendHeader("Access-Control-Allow-Origin", "*");
    HTTPServer::server.sendHeader("Access-Control-Max-Age", "10000");
    HTTPServer::server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
    HTTPServer::server.sendHeader("Access-Control-Allow-Headers", "*");
  }

  void begin(Core::Core_T* operationalCore, uint32_t port = 80) {
    Serial_println("Starting webserver");
    HTTPServer::operationalCore = operationalCore;
    HTTPServer::server.begin(port);
    HTTPServer::server.on("/test", []() {
      HTTPServer::cors();
      HTTPServer::server.send(200, "application/json", "{\"type\":\"success\",\"message\":\"connection okay\"}");
    });
  }

  void on(String route, std::function<void(WebServer&)> callback) {
    HTTPServer::server.on(route, [callback]() {
      HTTPServer::cors();
      callback(HTTPServer::server);
    });
  }

  void stopListening() {
    if (HTTPServer::listener) {
      HTTPServer::operationalCore->clearInterval(HTTPServer::listener);
      HTTPServer::listener = 0;
    }
  }

  void listen() {
    HTTPServer::stopListening();
    HTTPServer::listener = HTTPServer::operationalCore->setInterval([]() {
      HTTPServer::server.handleClient();
    }, 50);
  }
};
#endif
