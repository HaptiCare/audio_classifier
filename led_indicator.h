#ifndef LED_INDICATOR_H_
#define LED_INDICATOR_H_

#include <Arduino.h>

class LEDIndicator {
public:
  LEDIndicator();
  void begin();
  void setWinner(int classIndex, float confidence = 100.0f);
  void update();
  void allOff();
  void startupAnimation();

private:
  static constexpr int kLedPins[5] = {2, 15, 16, 17, 21};
  int current_latched_class;
  unsigned long last_event_time;
  static constexpr unsigned long kHoldTimeMs = 4000;
};

#endif // LED_INDICATOR_H_
