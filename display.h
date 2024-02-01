#ifndef DISPLAY_H__
#define DISPLAY_H__
#include <LiquidCrystal_I2C.h>
#include "core/core.h"
#include <vector>
#include <functional>
#include "data-peripheral.h"
namespace Display {
  LiquidCrystal_I2C lcd(0x20, 16, 2);
  Core::Core_T* core;
  std::vector<std::function<void()>> screens;
  std::function<void()> initCallback;

  enum {
    LINE_0=0,
    LINE_1
  };

  template <typename T>
  String format(T input, int width = 16, char fill = ' ') {
    String inp(input);
    if (inp.length() > width) {
      inp = inp.substring(0, width - 2) + "..";
      return inp.c_str();
    }
    if (inp.length() == width) {
      return inp.c_str();
    }  
    int margin = (width - inp.length()) / 2;
    for (int i=0; i<margin; i++) {
      inp += fill;
    }
    while (inp.length() < width) {
      inp = String(fill) + inp;
    }
    return inp;
  }


  void set(int line, String data) {
    Display::lcd.setCursor(0, line);
    Display::lcd.print(format(data));
  }

  void showSplash(uint32_t timeout = SECONDS(3)) {
    Display::set(Display::LINE_0, "Heleo");
    Display::set(Display::LINE_1, "Electronics");
    Display::core->setTimeout([]() {
      invoke(Display::initCallback);
    }, timeout);
  }

  void showVI(DataPeripheral::Phase P) {
    Display::set(Display::LINE_0, format(P.name + "v", 8) + format(P.name + "i", 8));
    Display::set(Display::LINE_1, format(String(P.v) + " V", 8) + format(String(P.i) + " A", 8));    
  }

  void showR() {
    showVI(DataPeripheral::R);
  }

  void showY() {
    showVI(DataPeripheral::Y);
  }

  void showB() {
    showVI(DataPeripheral::B);
  }

  void showEnergy() {
    Display::set(Display::LINE_0, format("Units Consumed"));
    Display::set(Display::LINE_1, format(DataPeripheral::totalEnergy()));    
  }

  void onInit(std::function<void()> callback) {
    Display::initCallback = callback;
  }
  
  void begin(Core::Core_T* core) {
    Display::core = core;
    Display::lcd.init();
    Display::lcd.backlight();
    Display::showSplash();

    Display::screens.push_back(showR);
    Display::screens.push_back(showY);
    Display::screens.push_back(showB);
    Display::screens.push_back(showEnergy);
  
    Display::onInit([core]() {
      core->setInterval([]() {
        if (Display::screens.size()) {
          auto front = Display::screens.front();
          invoke(front);
          Display::screens.push_back(front);
          Display::screens.erase(Display::screens.begin());
        }
      }, SECONDS(3));
    });
  }

};
#endif