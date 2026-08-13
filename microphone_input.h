#ifndef MICROPHONE_INPUT_H_
#define MICROPHONE_INPUT_H_

#include <Arduino.h>

class MicrophoneInput {
public:
  MicrophoneInput();
  bool begin();
  bool captureToInt8(int8_t* dst, int bytes_to_fill, uint32_t timeout_ms = 1200);
  bool isReady() const;

private:
  bool ready_;
};

#endif // MICROPHONE_INPUT_H_
