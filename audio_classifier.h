#ifndef AUDIO_CLASSIFIER_H_
#define AUDIO_CLASSIFIER_H_

#include <Arduino.h>

namespace tflite {
  class ErrorReporter;
  class Model;
  class MicroInterpreter;
}
struct TfLiteTensor;

struct InferenceResult {
  int best_class;
  float max_confidence;
  float latency_ms;
  float probabilities[4];
};

class AudioClassifier {
public:
  AudioClassifier();
  ~AudioClassifier();
  bool begin();
  int8_t* getInputBuffer();
  int getInputByteSize();
  InferenceResult predict();
  const char* getClassName(int index);

private:
  tflite::ErrorReporter* error_reporter;
  const tflite::Model* model;
  tflite::MicroInterpreter* interpreter;
  TfLiteTensor* input;
  TfLiteTensor* output;

  static constexpr size_t kTensorArenaSize = 40 * 1024; // 40 KB internal SRAM
  uint8_t* tensor_arena;
};

#endif // AUDIO_CLASSIFIER_H_
