#ifndef INTERVAL_H__
#define INTERVAL_H__

typedef uint32_t IntervalReference;
typedef uint32_t IntervalDuration;

#define NULL_REFERENCE       0

class Interval {
public:
  IntervalDuration duration;
  uint32_t executionTime;
  uint32_t start;
  static uint32_t count;
  IntervalReference id;
  std::function<void()> callback;

  Interval(std::function<void()> callback, IntervalDuration duration)
    : callback(callback), duration(duration) {
    this->id = ++Interval::count;
    this->start = millis();
  }

  ~Interval() {
    // Serial.printf("%d interval removed\n", this->getID());
  }

  uint32_t getExecutionTime() {
    return this->executionTime;
  }

  IntervalDuration getDuration() {
    return this->duration;
  }

  IntervalReference& getID() {
    return this->id;
  }

  virtual bool execute() {
    if (!this->callback) {
      return false;
    }
    if (millis() - this->start > this->duration) {
      this->start = millis();
      this->executionTime = micros();
      callback();
      this->executionTime = micros() - this->executionTime;
    }
    return true;
  }
};
uint32_t Interval::count = 0;
#endif