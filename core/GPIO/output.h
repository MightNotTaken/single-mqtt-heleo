#ifndef INDICATOR_H__
#define INDICATOR_H__
#include <Shifty.h>
#include <Adafruit_NeoPixel.h>
#include "./../../board.h"

#define GPIO_TYPE_WS_2811      1
#define GPIO_TYPE_HC_595       2
#define GPIO_TYPE_GPIO         3

#define NUMPIXELS 2
Adafruit_NeoPixel WS2811(NUMPIXELS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);

Shifty expander;
class OutputGPIO {
  int GPIO;
  byte type;
  uint32_t lastColor;
  uint8_t pwmChanel;
  uint8_t maxIntensity;
  static uint8_t PWMChanel;
  static bool initialized;
  static int count; 
  int id; 
  public:
    OutputGPIO() {
      this->maxIntensity = 100;
      this->id = OutputGPIO::count++;
    }
    OutputGPIO(int GPIO, int maxIntensity = 100) {
      this->maxIntensity = maxIntensity;
      this->attach(GPIO);
    }
    int getID() {
      return this->id;
    }
    void attach(int GPIO) {
      this->GPIO = GPIO % 100;
      if (this->isExpanderBit(GPIO)) {
        this->type = GPIO_TYPE_HC_595;
      } else if (this->is2811Bit(GPIO)) {
        this->type = GPIO_TYPE_WS_2811;
      } else {
        this->type = GPIO_TYPE_GPIO;
        this->pwmChanel = OutputGPIO::PWMChanel;
        ledcAttachPin(this->GPIO, this->pwmChanel);
        OutputGPIO::PWMChanel ++;
        OutputGPIO::PWMChanel %= 16;
      }
      this->turnOff();
    }

    static void begin() {
      expander.setBitCount(16);
      expander.setPins(HC_595_SH_DATA, HC_595_SH_CP, HC_595_ST_CP);

      WS2811.begin();
      WS2811.clear();
      WS2811.show();

      ledcSetup(0, 2000, 8);
      OutputGPIO::initialized = true;
    }

    void setIntensity(uint8_t intensity = 50) {
      switch (this->type) {
        case GPIO_TYPE_WS_2811: WS2811.setBrightness(intensity);
          break;
        case GPIO_TYPE_HC_595: expander.writeBit(this->GPIO, intensity >= 50 ? HIGH : LOW);
          break;
        default: ledcWrite(this->pwmChanel, map(intensity, 0, 100, 0, 255));          
      }
      
    }

    void setColor(uint32_t color, byte brightness = 30) {
      WS2811.setBrightness(brightness);
      WS2811.setPixelColor(this->GPIO, color);
      WS2811.show();
      this->lastColor = color;
    }

    bool isExpanderBit(int bit) {
      return bit >= 100;
    }
    
    bool is2811Bit(int bit) {
      return bit >= 200;
    }

    void turnOn() {
      this->setIntensity(this->maxIntensity);
    }

    void turnOff() {
      this->setIntensity(0);
    }

    void setState(bool state) {
      state ? this->turnOn() : this->turnOff();
    }


};
uint8_t OutputGPIO::PWMChanel = 0;
bool OutputGPIO::initialized = false;
int OutputGPIO::count = 0;
#endif