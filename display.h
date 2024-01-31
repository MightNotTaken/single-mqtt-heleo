#ifndef DISPLAY_H__
#define DISPLAY_H__
#include <LiquidCrystal_I2C.h>
#include "core/core.h"
namespace Display {
  LiquidCrystal_I2C lcd(0x20, 16, 2);
  Core::Core_T* core;
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

  void showSplash(std::function<void()> onInit = nullptr, uint32_t timeout = SECONDS(3)) {
    Display::set(Display::LINE_0, "Heleo");
    Display::set(Display::LINE_1, "Electronics");
    Display::core->setTimeout([onInit]() {
      invoke(onInit);
    }, timeout);
  }
  
  void begin(Core::Core_T* core) {
    Display::core = core;
    Display::lcd.init();
    Display::lcd.backlight();
    Display::showSplash();   
  }

};
#endif