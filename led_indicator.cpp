#include "led_indicator.h"

LEDIndicator::LEDIndicator()
  : current_latched_class(1), last_event_time(0) {} // Default to index 1 (BACKGROUND NOISE)

void LEDIndicator::begin() {
  for (int i = 0; i < 4; i++) {
    pinMode(kLedPins[i], OUTPUT);
    digitalWrite(kLedPins[i], LOW);
  }
  digitalWrite(kLedPins[1], HIGH); // Light up Background Noise LED (Green) at boot
}

void LEDIndicator::allOff() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(kLedPins[i], LOW);
  }
}

void LEDIndicator::setWinner(int classIndex, float confidence) {
  if (classIndex < 0 || classIndex >= 4) {
    classIndex = 1; // Default to background noise
  }

  current_latched_class = classIndex;
  last_event_time = millis();

  allOff();
  digitalWrite(kLedPins[classIndex], HIGH);
}

void LEDIndicator::update() {
  // Auto-revert to Background Noise LED (index 1) after hold time expires
  if (current_latched_class != 1 && (millis() - last_event_time > kHoldTimeMs)) {
    current_latched_class = 1;
    allOff();
    digitalWrite(kLedPins[1], HIGH);
  }
}

void LEDIndicator::startupAnimation() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(kLedPins[i], HIGH);
    delay(150);
    digitalWrite(kLedPins[i], LOW);
  }
  digitalWrite(kLedPins[1], HIGH);
}
