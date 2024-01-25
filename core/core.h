#ifndef CORE_H__
#define CORE_H__
#include <vector>
#include <functional>
#include "interval.h"
#include "timeout.h"
#include "utility.h"
#include "mac.h"
#include "gpio.h"

typedef uint8_t core_t;
typedef uint8_t core_mode_t;
namespace Core {
  TaskHandle_t Task0;
  TaskHandle_t Task1;

  enum {
    NO_CORE_SELECTED = 0b00,
    CORE_0           = 0b01,
    CORE_1           = 0b10,
    DUAL_CORE        = 0b11
  };

  core_mode_t operationMode = Core::NO_CORE_SELECTED;

  class Core_T {
    std::vector <Interval*> intervals;
    std::vector <Timeout*>  timeouts;
    std::function<void()> setupCallback;
    GPIO_T GPIOHandler;
    core_t core;

    public:

      Core_T(core_t core): core(core), setupCallback(nullptr) {}

      IntervalReference& setInterval(std::function<void()> callback, IntervalDuration duration) {
        Interval* interval = new Interval(callback, duration);
        this->intervals.push_back(interval);
        return interval->getID();
      }

      IntervalReference& setImmediate(std::function<void()> callback, IntervalDuration duration) {
        callback();
        return setInterval(callback, duration);
      }

      TimeoutReference& setTimeout(std::function<void()> callback, IntervalDuration duration) {
        Timeout* timeout = new Timeout(callback, duration);
        this->timeouts.push_back(timeout);
        return timeout->getID();
      }

      void clearInterval(IntervalReference& ref) {
        for (auto it = intervals.begin(); it != intervals.end(); ++it) {
          if ((*it)->getID() == ref) {
            ref = NULL_REFERENCE;
            delete *it;
            intervals.erase(it);
            break;
          }
        }
      }

      void execute(std::function<void()> executable) {
        this->setTimeout(executable, 0);
      }

      void clearImmediate(IntervalReference& ref) {
        clearInterval(ref);
      }

      void clearTimeout(TimeoutReference& ref) {
        for (auto it = timeouts.begin(); it != timeouts.end(); ++it) {
          if ((*it)->getID() == ref) {
            ref = NULL_REFERENCE;
            delete *it;
            timeouts.erase(it);
            break;
          }
        }
      }

      void onSetup(std::function<void()> callback) {
        if (!callback) {
          return;
        }
        this->begin();
        this->setupCallback = callback;
      }

      void setup() {
        if (this->setupCallback) {
          this->setupCallback();
        }
      }

      String coreName() {
        switch(this->core)  {
          case NO_CORE_SELECTED: return var(NO_CORE_SELECTED);
          case CORE_0: return var(CORE_0);
          case CORE_1: return var(CORE_1);
          case DUAL_CORE: return var(DUAL_CORE);
        }
      }

      void begin(uint32_t intervalSize = MINUTES(1)) {
        Core::operationMode |= this->core;
        setInterval([this]() {
          Serial_print(this->coreName() + " uptime: ");
          Serial_println(formatMillis(millis()));
        }, intervalSize);
      }
      
      void registerInputGPIO(InputGPIO* GPIOReference) {
        this->GPIOHandler.registerInput(GPIOReference);
      }

      void loop() {
        
        for (auto& interval: this->intervals) {
          interval->execute();
        }
        
        for (auto& timeout: this->timeouts) {
          if (timeout->execute()) {
            this->clearTimeout(timeout->getID());
          }
        }
        
        this->GPIOHandler.loop();
      }
  } core0(CORE_0), core1(CORE_1);

  void launch() {
    serialSemaphore = xSemaphoreCreateMutex();
    MAC::load();
    delay(SECONDS(.5));
    showX(Core::operationMode);
    if (Core::operationMode == Core::NO_CORE_SELECTED) {
      Serial_println("Please initialize some core to run the program");
      return;
    }
    switch (Core::operationMode) {
      case Core::DUAL_CORE: {
        xTaskCreatePinnedToCore([](void* params) {
          core0.setup();
          while (true) {
            core0.loop();
          }
        }, "Task0", 10000, NULL, 1, &Task0, 0);          
        xTaskCreatePinnedToCore([](void* params) {
          core1.setup();
          while (true) {
            core1.loop();
          }
        }, "Task1", 10000, NULL, 1, &Task1, 1);
      } break;
      case Core::CORE_0: 
      case Core::CORE_1: {
        core0.setup();
        core1.setup();
        while (true) {
          core0.loop();
          core1.loop();
        }
      } break;      
    }
  }

  void setupCore0();
  void setupCore1();
};

#endif