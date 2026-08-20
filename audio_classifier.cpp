#include "audio_classifier.h"
#include "model_data.h"
#include "esp_heap_caps.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const char* kClassNames[4] = {
  "BABY CRYING",
  "BACKGROUND NOISE",
  "DOOR KNOCKING",
  "EMERGENCY ALERT"
};

AudioClassifier::AudioClassifier()
  : error_reporter(nullptr), model(nullptr), interpreter(nullptr),
    input(nullptr), output(nullptr), tensor_arena(nullptr) {}

AudioClassifier::~AudioClassifier() {
  if (tensor_arena != nullptr) {
    heap_caps_free(tensor_arena);
    tensor_arena = nullptr;
  }
}

bool AudioClassifier::begin() {
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  if (tensor_arena == nullptr) {
    if (psramFound()) {
      tensor_arena = static_cast<uint8_t*>(
          heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
      if (tensor_arena != nullptr) {
        Serial.printf("Tensor arena allocated in PSRAM: %u bytes\n",
                      (unsigned)kTensorArenaSize);
      }
    }

    if (tensor_arena == nullptr) {
      tensor_arena = static_cast<uint8_t*>(
          heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
      if (tensor_arena != nullptr) {
        Serial.printf("Tensor arena allocated in internal RAM: %u bytes\n",
                      (unsigned)kTensorArenaSize);
      }
    }

    if (tensor_arena == nullptr) {
      TF_LITE_REPORT_ERROR(error_reporter,
                           "Failed to allocate %u-byte tensor arena. Free heap=%u, free PSRAM=%u",
                           (unsigned)kTensorArenaSize,
                           (unsigned)ESP.getFreeHeap(),
                           (unsigned)ESP.getFreePsram());
      return false;
    }
  }

  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter, "Model schema version mismatch!");
    return false;
  }

  static tflite::AllOpsResolver resolver;
  interpreter = new tflite::MicroInterpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed!");
    delete interpreter;
    interpreter = nullptr;
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
  if (index >= 0 && index < 4) return kClassNames[index];
  return "UNKNOWN";
}

InferenceResult AudioClassifier::predict() {
  InferenceResult res;
  res.best_class = 1; // Default to background noise
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

  for (int i = 0; i < 4; i++) {
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
