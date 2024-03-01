#ifndef UTILITY_H__
#define UTILITY_H__
#include <TimeLib.h>

typedef uint32_t TimeReference;

#define var(x)        #x
#define showX(x)       Serial.println(String(var(x)) + " : " + String(x))

#define SECONDS(x)    (x * 1000L)
#define MINUTES(x)    (x * SECONDS(60))
#define HOURS(x)      (x * MINUTES(60))
#define DAYS(x)       (X * HOURS(24))

#define invoke(func, ...)  do {\
  if (func) {\
    func(__VA_ARGS__);\
  }\
} while (0)

SemaphoreHandle_t serialSemaphore;

#define Serial_print(x)          do {\
  if (xSemaphoreTake(serialSemaphore, portMAX_DELAY)) {\
    Serial.print(x);\
    xSemaphoreGive(serialSemaphore);\
  }\
} while (0)
#define Serial_println(x)        Serial_print(String(x) + '\n')

String formatDays(int days, uint8_t hours) {
  return String(days) + "d " + hours + "h"; 
}
String formatHours(uint16_t hours, uint8_t minutes, uint8_t seconds) {
  if (hours < 24) {
    return String(hours) + ":" + minutes + ":" + seconds;
  }
  return formatDays(hours / 24.0, hours % 24);
}

String formatMinutes(uint16_t minutes, uint8_t seconds) {
  if (minutes < 60) {
    return String(minutes) + "m " + seconds + "s";
  }
  return formatHours(minutes / 60.0, minutes % 60, seconds);
}

String formatSeconds(uint32_t seconds, uint16_t ms) {
  if (seconds < 60) {
    return String(seconds) + "s " + ms + "ms";
  }
  return formatMinutes(seconds / 60.0, seconds % 60);
}

String formatMillis(uint32_t ms) {
  if (ms < 1000) {
    return String(ms) + "ms";
  }
  return formatSeconds(ms / 1000.0, ms % 1000);
}

String formatMicros(uint32_t us) {
  if (us < 1E3) {
    return String(us) + "us";
  }
  return formatMillis(us / 1000.0);
}

String getTimeStampFromMS(uint32_t input) {
  input /= 1000;
  uint8_t s = input % 60;
  input /= 60;
  uint8_t m = input % 60;
  input /= 60;
  char timeStamp[10];
  sprintf(timeStamp, "%02d:%02d:%02d", input % 24, m, s);
  return timeStamp;
}

uint32_t getCurrentTimeMS() {
  tmElements_t timeInfo;
  breakTime(now(), timeInfo);  
  return HOURS(timeInfo.Hour) + MINUTES(timeInfo.Minute) + SECONDS(timeInfo.Second);
}

uint32_t getMSFromTimeStamp(String timestamp) {
  uint8_t h;
  uint8_t m;
  uint8_t s;
  sscanf(timestamp.c_str(), "%d:%d:%d", &h, &m, &s);
  return HOURS(h) + MINUTES(m) + SECONDS(s);
}

uint32_t getTimeStampDifference(String to, String from = "") {
  uint32_t fromMS;
  if (!from.length()) {
    fromMS = getCurrentTimeMS();
  } else {
    fromMS = getMSFromTimeStamp(from);
  }
  uint32_t toMS = getMSFromTimeStamp(to);
  if (fromMS > toMS) {
    toMS += HOURS(24);
  }
  return toMS - fromMS;
}


String getTimeStamp() {
  char stamp[10];
  tmElements_t timeInfo;
  breakTime(now(), timeInfo);
  sprintf(stamp, "%02d:%02d:%02d", timeInfo.Hour, timeInfo.Minute, timeInfo.Second);
  return stamp;
}

String getDateStamp() {
  char stamp[12];
  tmElements_t timeInfo;
  breakTime(now(), timeInfo);
  sprintf(stamp, "%04d:%02d:%02d", timeInfo.Year + 1970, timeInfo.Month, timeInfo.Day);
  return stamp;
}

String getTimeDateStamp() {
  return getTimeStamp() + " " + getDateStamp();
}

String getDateTimeStamp() {
  return getDateStamp() + " " + getTimeStamp();
}
#endif