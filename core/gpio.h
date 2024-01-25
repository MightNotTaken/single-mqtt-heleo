#ifndef GPIO_H__
#define GPIO_H__
#include <vector>
#include "./GPIO/input.h"
class GPIO_T {
  std::vector<InputGPIO*> inputs;
  public:
    void registerInput(InputGPIO* input) {
      this->inputs.push_back(input);
    }
    void loop() {
      for (auto& input: this->inputs) {
        input->listen();
      }
    }
};
#endif