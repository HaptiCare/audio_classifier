# ESP32 Audio Classifier

TensorFlow Lite Micro audio classifier for the ESP32-S3.

The sketch runs an INT8 TensorFlow Lite model locally on the ESP32-S3 and drives one of five LEDs based on the predicted audio class.

---

## Features

- Runs fully on-device using TensorFlow Lite Micro
- Supports five audio classes
- Optional live I2S microphone inference mode
- Binary serial protocol for host applications
- Simple ASCII commands for testing
- LED output for visual predictions

---

## Quick Start

1. Install the ESP32 board package in Arduino IDE.
2. Install the TensorFlowLite_ESP32 library.
3. Open the `ESP32_Audio_Classifier` folder.
4. Select your ESP32-S3 board and serial port.
5. Build and upload.

### Recommended board settings

| Setting | Value |
|---------|-------|
| Board | Your ESP32-S3 module |
| USB CDC on Boot | Enabled (if required) |
| PSRAM | Enabled (if available) |
| Upload Speed | Fastest stable value |

---

## Audio Classes

| Index | Class |
|------:|-------|
| 0 | Alarm |
| 1 | Ambulance |
| 2 | Baby crying |
| 3 | Background noise |
| 4 | Door knocking |

---

## Project Structure

```
ESP32_Audio_Classifier/
├── ESP32_Audio_Classifier.ino
├── audio_classifier.cpp
├── audio_classifier.h
├── led_indicator.cpp
├── led_indicator.h
├── microphone_input.cpp
├── microphone_input.h
├── model_data.cpp
└── model_data.h
```

---

## Wiring

This project is written for an ESP32-S3. Connect one LED (with a 220 Ω to 330 Ω current-limiting resistor) to each GPIO listed below. For every LED, connect:

- LED anode (+) → the selected GPIO
- LED cathode (−) → GND
- Resistor in series with the LED

A simple layout is shown below:

```text
ESP32-S3
+----------------------------------------------+
|                                              |
|  GPIO 2  ──> Alarm LED ──┐                   |
|  GPIO 15 ──> Ambulance LED ──┤               |
|  GPIO 16 ──> Baby Crying LED ──┤             |
|  GPIO 17 ──> Background LED ──┤              |
|  GPIO 21 ──> Door Knocking LED ──┤           |
|  GND ──────────────────────────┴─────────────┘ |
|                                              |
+----------------------------------------------+
```

| Class | GPIO |
|------|------|
| Alarm | GPIO 2 |
| Ambulance | GPIO 15 |
| Baby crying | GPIO 16 |
| Background noise | GPIO 17 |
| Door knocking | GPIO 21 |

Outputs are **active HIGH**.

### I2S Microphone (for live sound detection)

Default pin mapping in `microphone_input.cpp`:

- `BCK/SCK` -> GPIO 41
- `WS/LRCLK` -> GPIO 42
- `SD/DOUT` -> GPIO 40
- `VCC` -> 3.3V
- `GND` -> GND

If your board uses different pins, edit these constants in `microphone_input.cpp`.

---

## Serial Communication

**Baud rate:** `115200`

### Legacy Commands

| Command | Action |
|---------|--------|
| `1`–`5` | Run a synthetic test for the selected class |
| `M` or `m` | Toggle live microphone inference on/off |
| `S` or `s` | Send an unframed spectrogram payload matching the model input size |

### Binary Request

| Bytes | Description |
|-------|-------------|
| `0` | `0xAA` |
| `1` | `0x55` |
| `2-3` | Payload length (little-endian) |
| `4...N` | Raw INT8 spectrogram |

The payload length must exactly match the model input tensor size.

### Binary Response

| Bytes | Description |
|-------|-------------|
| `0` | `0x55` |
| `1` | `0xAA` |
| `2` | Predicted class index |
| `3-6` | Maximum confidence (`float`) |
| `7-10` | Inference latency in milliseconds (`float`) |
| `11-30` | Five class probabilities (`float`) |

---

## Notes

- `model_data.h` contains the TensorFlow Lite model.
- The background-noise LED is enabled at startup.
- If inference fails, ensure the payload length matches the model input tensor size.

---

## Troubleshooting

**TensorFlow headers not found**

Reinstall the `TensorFlowLite_ESP32` library.

**Upload or serial connection fails**

Check your board selection, USB mode, and boot procedure.

**Unexpected LED output**

Verify the GPIO connections match the wiring table.

**`Failed to resize buffer` / `AllocateTensors() failed!`**

The model needs a larger TensorFlow Lite Micro tensor arena than the default small allocation.

- Ensure **PSRAM is enabled** in board settings.
- Use an ESP32-S3 variant with PSRAM when possible.
- If needed, increase `kTensorArenaSize` in `audio_classifier.h`.