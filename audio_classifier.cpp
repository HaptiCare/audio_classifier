#include "audio_classifier.h"
#include "model_data.h"

static const char* kClassNames[5] = {
  "ALARM",
  "AMBULANCE",
  "BABY CRYING",
  "BACKGROUND NOISE",
  "DOOR KNOCKING"
};

AudioClassifier::AudioClassifier()
  : error_reporter(nullptr), model(nullptr), interpreter(nullptr),
    input(nullptr), output(nullptr) {}

bool AudioClassifier::begin() {
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter, "Model schema version mismatch!");
    return false;
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed!");
    return false;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);
  return true;
}

int8_t* AudioClassifier::getInputBuffer() {
  return input->data.int8;
}

int AudioClassifier::getInputByteSize() {
  return input->bytes;
}

const char* AudioClassifier::getClassName(int index) {
  if (index >= 0 && index < 5) return kClassNames[index];
  return "UNKNOWN";
}

InferenceResult AudioClassifier::predict() {
  InferenceResult res;
  res.best_class = 0;
  res.max_confidence = 0.0f;
  res.latency_ms = 0.0f;

  unsigned long start_t = micros();
  TfLiteStatus status = interpreter->Invoke();
  unsigned long elapsed_us = micros() - start_t;
  res.latency_ms = elapsed_us / 1000.0f;

  if (status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke() failed!");
    return res;
  }

  float scale = output->params.scale;
  int zero_point = output->params.zero_point;
  int8_t max_val = -128;

  for (int i = 0; i < 5; i++) {
    int8_t raw = output->data.int8[i];
    float prob = (raw - zero_point) * scale * 100.0f;
    if (prob < 0.0f) prob = 0.0f;

    res.probabilities[i] = prob;
    if (raw > max_val) {
      max_val = raw;
      res.best_class = i;
    }
  }

  res.max_confidence = res.probabilities[res.best_class];
  return res;
}
