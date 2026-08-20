#include <Arduino.h>
#include "audio_classifier.h"
#include "led_indicator.h"
#include "microphone_input.h"

static AudioClassifier classifier;
static LEDIndicator leds;
static MicrophoneInput mic;
static bool mic_live_mode = false;
static unsigned long last_live_inference_ms = 0;
static constexpr unsigned long kLiveInferenceIntervalMs = 200;
static constexpr float kLiveReportThreshold = 45.0f;

static constexpr uint8_t SYNC_BYTE_1 = 0xAA;
static constexpr uint8_t SYNC_BYTE_2 = 0x55;
static constexpr uint8_t RESP_BYTE_1 = 0x55;
static constexpr uint8_t RESP_BYTE_2 = 0xAA;

void printReport(const InferenceResult& res) {
  Serial.println("--------------------------------------------------");
  Serial.printf(" Inference Time: %.2f ms (ESP32-S3 LX7 @ 240MHz)\n", res.latency_ms);
  Serial.println("--------------------------------------------------");

  for (int i = 0; i < 4; i++) {
    float p = res.probabilities[i];
    int bar_len = (int)(p / 5.0f);
    if (bar_len > 20) bar_len = 20;

    char bar[21];
    for (int b = 0; b < 20; b++) bar[b] = (b < bar_len) ? '#' : '.';
    bar[20] = '\0';

    Serial.printf("   %-22s [%s] %5.1f%%\n", classifier.getClassName(i), bar, p);
  }

  Serial.printf("\n DETECTED RESULT: %s\n", classifier.getClassName(res.best_class));
  Serial.println("--------------------------------------------------\n");
}

bool readExact(uint8_t* buf, int count, unsigned long timeout_ms) {
  int received = 0;
  unsigned long start = millis();
  while (received < count) {
    if (Serial.available() > 0) {
      buf[received++] = (uint8_t)Serial.read();
    } else if (millis() - start > timeout_ms) {
      return false;
    }
    yield();
  }
  return true;
}

void sendBinaryResponse(const InferenceResult& res) {
  uint8_t resp[27]; // 27 bytes total for 4 output classes (2 sync + 1 winner + 4 latency + 4 max_conf + 16 probs)
  resp[0] = RESP_BYTE_1;
  resp[1] = RESP_BYTE_2;
  resp[2] = (uint8_t)res.best_class;
  memcpy(&resp[3], &res.max_confidence, sizeof(float));
  memcpy(&resp[7], &res.latency_ms, sizeof(float));
  for (int i = 0; i < 4; i++) {
    memcpy(&resp[11 + i * 4], &res.probabilities[i], sizeof(float));
  }
  Serial.write(resp, sizeof(resp));
  Serial.flush();
}

void handleBinaryFrame() {
  unsigned long sync_wait = millis();
  while (!Serial.available()) {
    if (millis() - sync_wait > 500) return;
    yield();
  }
  if ((uint8_t)Serial.read() != SYNC_BYTE_2) return;

  uint8_t len_buf[2];
  if (!readExact(len_buf, 2, 1000)) return;
  uint16_t payload_len = (uint16_t)len_buf[0] | ((uint16_t)len_buf[1] << 8);

  int expected = classifier.getInputByteSize();
  if ((int)payload_len != expected) {
    Serial.printf("[FRAME] Size mismatch: got %d, expected %d - skipping\n",
                  payload_len, expected);
    return;
  }

  if (!readExact((uint8_t*)classifier.getInputBuffer(), payload_len, 5000)) {
    Serial.println("[FRAME] Payload read timeout");
    return;
  }

  InferenceResult res = classifier.predict();
  leds.setWinner(res.best_class, res.max_confidence);

  sendBinaryResponse(res);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  leds.begin();
  leds.startupAnimation();

  Serial.println("\n==================================================");
  Serial.println("  MODULAR ESP32-S3 TFLITE MICRO ENGINE (4-CLASS)");
  Serial.println("==================================================\n");

  if (!classifier.begin()) {
    Serial.println(" Classifier initialization failed!");
    return;
  }

  Serial.printf("Model input tensor bytes: %d\n", classifier.getInputByteSize());

  if (mic.begin()) {
    mic_live_mode = true;
    Serial.println("Live microphone mode: ON");
  } else {
    Serial.println("Live microphone mode: OFF (check mic wiring/pins)");
  }

  Serial.println("TFLite Micro Engine & Hardware Modules Ready!");
  Serial.println("Send '1'..'4' for tests, 'M' to toggle live mic, or stream framed audio.\n");
}

void loop() {
  leds.update();

  if (Serial.available() > 0) {
    uint8_t first_byte = Serial.peek();

    if (first_byte == SYNC_BYTE_1) {
      Serial.read();
      handleBinaryFrame();
    } else {
      char cmd = (char)Serial.read();

      if (cmd >= '1' && cmd <= '4') {
        int target_class = cmd - '1';
        Serial.printf("Triggering Test Pattern for %s...\n",
                      classifier.getClassName(target_class));

        int8_t* in_buf = classifier.getInputBuffer();
        int bytes = classifier.getInputByteSize();

        for (int i = 0; i < bytes; i++) {
          int8_t val = (int8_t)(-100 + (rand() % 20));
          if ((i % 4) == target_class) {
            val = (int8_t)(20 + (rand() % 100));
          }
          in_buf[i] = val;
        }

        InferenceResult res = classifier.predict();
        printReport(res);
        leds.setWinner(res.best_class, res.max_confidence);
      }
      else if (cmd == 'S' || cmd == 's') {
        int bytes = classifier.getInputByteSize();
        Serial.readBytes((char*)classifier.getInputBuffer(), bytes);
        InferenceResult res = classifier.predict();
        printReport(res);
        leds.setWinner(res.best_class, res.max_confidence);
      } else if (cmd == 'M' || cmd == 'm') {
        if (!mic.isReady()) {
          Serial.println("Mic is not initialized. Check I2S pins in microphone_input.cpp");
        } else {
          mic_live_mode = !mic_live_mode;
          Serial.printf("Live microphone mode: %s\n", mic_live_mode ? "ON" : "OFF");
        }
      }
    }
  }

  if (mic_live_mode && millis() - last_live_inference_ms >= kLiveInferenceIntervalMs) {
    last_live_inference_ms = millis();
    int bytes = classifier.getInputByteSize();

    if (!mic.captureToInt8(classifier.getInputBuffer(), bytes)) {
      return;
    }

    InferenceResult res = classifier.predict();
    leds.setWinner(res.best_class, res.max_confidence);

    if (res.max_confidence >= kLiveReportThreshold) {
      Serial.printf("[LIVE] %s (%.1f%%)\n",
                    classifier.getClassName(res.best_class),
                    res.max_confidence);
    }
  }
}
