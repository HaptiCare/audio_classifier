#include "microphone_input.h"

#include "driver/i2s.h"

namespace {

static constexpr i2s_port_t kI2SPort = I2S_NUM_0;
static constexpr uint32_t kSampleRate = 16000;
static constexpr int kBckPin = 41;
static constexpr int kWsPin = 42;
static constexpr int kDataInPin = 40;
static constexpr int kChunkSamples = 128;

inline int8_t clampToInt8(int32_t v) {
  if (v > 127) return 127;
  if (v < -128) return -128;
  return static_cast<int8_t>(v);
}

}  // namespace

MicrophoneInput::MicrophoneInput() : ready_(false) {}

bool MicrophoneInput::begin() {
  i2s_config_t i2s_config = {};
  i2s_config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  i2s_config.sample_rate = kSampleRate;
  i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2s_config.dma_buf_count = 8;
  i2s_config.dma_buf_len = 256;
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = false;
  i2s_config.fixed_mclk = 0;

  i2s_pin_config_t pin_config = {};
  pin_config.bck_io_num = kBckPin;
  pin_config.ws_io_num = kWsPin;
  pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  pin_config.data_in_num = kDataInPin;

  esp_err_t err = i2s_driver_install(kI2SPort, &i2s_config, 0, nullptr);
  if (err != ESP_OK) {
    Serial.printf("[MIC] i2s_driver_install failed: %d\n", static_cast<int>(err));
    ready_ = false;
    return false;
  }

  err = i2s_set_pin(kI2SPort, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("[MIC] i2s_set_pin failed: %d\n", static_cast<int>(err));
    i2s_driver_uninstall(kI2SPort);
    ready_ = false;
    return false;
  }

  i2s_zero_dma_buffer(kI2SPort);
  ready_ = true;

  Serial.printf("[MIC] I2S ready. BCK=%d WS=%d SD=%d @ %lu Hz\n",
                kBckPin, kWsPin, kDataInPin,
                static_cast<unsigned long>(kSampleRate));
  return true;
}

bool MicrophoneInput::captureToInt8(int8_t* dst, int bytes_to_fill, uint32_t timeout_ms) {
  if (!ready_ || dst == nullptr || bytes_to_fill <= 0) {
    return false;
  }

  int32_t raw[kChunkSamples];
  int filled = 0;

  while (filled < bytes_to_fill) {
    int samples_needed = bytes_to_fill - filled;
    int samples_to_read = samples_needed < kChunkSamples ? samples_needed : kChunkSamples;

    size_t bytes_read = 0;
    esp_err_t err = i2s_read(
        kI2SPort,
        raw,
        static_cast<size_t>(samples_to_read) * sizeof(int32_t),
        &bytes_read,
        pdMS_TO_TICKS(timeout_ms));

    if (err != ESP_OK || bytes_read == 0) {
      Serial.printf("[MIC] i2s_read failed: err=%d bytes=%u\n",
                    static_cast<int>(err), static_cast<unsigned>(bytes_read));
      return false;
    }

    int got = static_cast<int>(bytes_read / sizeof(int32_t));
    if (got <= 0) {
      return false;
    }

    int32_t max_abs = 1;
    int32_t pcm[kChunkSamples];
    for (int i = 0; i < got; i++) {
      int32_t s = raw[i] >> 14;
      pcm[i] = s;
      int32_t a = (s >= 0) ? s : -s;
      if (a > max_abs) max_abs = a;
    }

    float gain = 100.0f / static_cast<float>(max_abs);
    for (int i = 0; i < got; i++) {
      int32_t scaled = static_cast<int32_t>(pcm[i] * gain);
      dst[filled++] = clampToInt8(scaled);
      if (filled >= bytes_to_fill) break;
    }
  }

  return true;
}

bool MicrophoneInput::isReady() const {
  return ready_;
}
