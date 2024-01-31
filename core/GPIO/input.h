#ifndef INPUT_H__
#define INPUT_H__
#include <functional>
#include "../utility.h"
class InputGPIO {
  uint8_t GPIO;
  std::function<void()> stateHighCallback;
  std::function<void()> stateLowCallback;
  std::function<void(bool)> stateChangeCallback;
  bool lastState;
  uint32_t debounceDuration;
  public:
    InputGPIO(uint8_t GPIO, uint8_t mode = INPUT_PULLUP, uint32_t debounce = 200): GPIO(GPIO), debounceDuration(debounce) {
      pinMode(GPIO, mode);
      this->lastState = digitalRead(GPIO);
    }

    void onStateHigh(std::function<void()> callback) {
      this->stateHighCallback = callback;
    }

    void onStateLow(std::function<void()> callback) {
      this->stateLowCallback = callback;
    }

    void onStateChange(std::function<void(bool)> callback) {
      this->stateChangeCallback = callback;
    }

    void listen() {
      static uint32_t debounce = 0;
      if (millis() - debounce < this->debounceDuration) {
        return;
      }
      debounce = 0;
      if (digitalRead(this->GPIO) != this->lastState) {
        debounce = millis();
        this->lastState = digitalRead(this->GPIO);
        invoke(this->stateChangeCallback, this->lastState);
        switch(digitalRead(this->GPIO)) {
          case HIGH: invoke(this->stateHighCallback);
            break;
          case LOW: invoke(this->stateLowCallback);
            break;
        }
      }
    }
};
#endif