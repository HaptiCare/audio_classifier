#ifndef AUDIO_CLASSIFIER_H_
#define AUDIO_CLASSIFIER_H_

#include <Arduino.h>
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

struct InferenceResult {
  int best_class;
  float max_confidence;
  float latency_ms;
  float probabilities[5];
};

class AudioClassifier {
public:
  AudioClassifier();
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

  static constexpr int kTensorArenaSize = 40 * 1024;
  alignas(16) uint8_t tensor_arena[kTensorArenaSize];
};

#endif // AUDIO_CLASSIFIER_H_
