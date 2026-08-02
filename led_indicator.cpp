#include "led_indicator.h"

LEDIndicator::LEDIndicator()
  : current_latched_class(3), last_event_time(0) {}

void LEDIndicator::begin() {
  for (int i = 0; i < 5; i++) {
    pinMode(kLedPins[i], OUTPUT);
    digitalWrite(kLedPins[i], LOW);
  }
  digitalWrite(kLedPins[3], HIGH);
}

void LEDIndicator::allOff() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(kLedPins[i], LOW);
  }
}

void LEDIndicator::setWinner(int classIndex, float confidence) {
  if (classIndex < 0 || classIndex >= 5) {
    classIndex = 3;
  }

  current_latched_class = classIndex;
  last_event_time = millis();

  for (int i = 0; i < 5; i++) {
    digitalWrite(kLedPins[i], (i == current_latched_class) ? HIGH : LOW);
  }
}

void LEDIndicator::update() {
  if (current_latched_class != 3 && (millis() - last_event_time > kHoldTimeMs)) {
    current_latched_class = 3;
    for (int i = 0; i < 5; i++) {
      digitalWrite(kLedPins[i], (i == 3) ? HIGH : LOW);
    }
  }
}

void LEDIndicator::startupAnimation() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(kLedPins[i], HIGH);
    delay(150);
    digitalWrite(kLedPins[i], LOW);
  }
  digitalWrite(kLedPins[3], HIGH);
}
