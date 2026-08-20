# ESP32 Audio Classifier (v1.3.0 Edge Production Firmware)

TensorFlow Lite Micro audio classifier optimized for the **ESP32-S3**.

The sketch runs a compact **26.24 KB INT8 TensorFlow Lite Micro model** locally on the ESP32-S3 and drives LED indicators based on the predicted audio class.

---

## Features

- **On-Device Edge AI**: Runs fully on-device using TensorFlow Lite Micro.
- **4 Consolidated Classes**: `baby_crying`, `background_noise`, `door_knocking`, `emergency_alert`.
- **40 KB SRAM Optimization**: Fits entirely within internal ESP32-S3 SRAM .
- **I2S Microphone Support**: Live acoustic inference via INMP441 / SPM0405HD4H I2S MEMS microphone.
- **Binary Serial Protocol**: Framed communication for host bridges and live GUI integration.
- **ASCII Command Mode**: Simple Serial Monitor commands for manual hardware testing.
- **LED Output Indicators**: Hardware visual notifications per class.

---

## Audio Classes & LED Pin Wiring

Connect one LED (with a 220 Ω to 330 Ω resistor) to each GPIO listed below:

| Index | Class Label | GPIO Pin | LED Color | Visual / Hardware Response |
|------:|:------------|:--------:|:---------:|:--------------------------|
| **0** | **Baby Crying** 👶 | **GPIO 6** | 🟡 Yellow | Trigger alert notification |
| **1** | **Background Noise** 🍃 | **GPIO 7** | 🟢 Green | Normal state (Boot default) |
| **2** | **Door Knocking** 🚪 | **GPIO 15** | 🟣 Purple | Trigger chime notification |
| **3** | **Emergency Alert** 🚨 | **GPIO 4** | 🔴 Red | Trigger emergency siren strobe |

*Outputs are active HIGH.*

---

## I2S MEMS Microphone Pin Mapping

Default pin mapping in `microphone_input.cpp` (for INMP441):

| Signal | ESP32-S3 Pin |
|:-------|:------------:|
| **BCK / SCK** (Bit Clock) | **GPIO 41** |
| **WS / LRCLK** (Word Select) | **GPIO 42** |
| **SD / DOUT** (Data Out) | **GPIO 40** |
| **VCC** | **3.3V** |
| **GND** | **GND** |

---

## Serial Protocol & Commands

**Baud Rate:** `115200`

### Interactive ASCII Commands (Serial Monitor)

| Command | Action |
|:--------|:-------|
| `1`–`4` | Trigger a synthetic test pattern for the selected class |
| `M` or `m` | Toggle live I2S microphone inference on/off |
| `S` or `s` | Process an unframed spectrogram payload matching model input size |

### Binary Frame Request

| Bytes | Field | Description |
|:------|:------|:------------|
| `0` | Header 1 | `0xAA` |
| `1` | Header 2 | `0x55` |
| `2–3` | Payload Length | Little-endian uint16 (expected input tensor size) |
| `4…N` | Spectrogram Payload | Raw INT8 spectrogram tensor |

### Binary Frame Response (27 Bytes Total)

| Bytes | Field | Type | Description |
|:------|:------|:----:|:------------|
| `0–1` | Header | uint8 | `0x55 0xAA` |
| `2` | Winner Index | uint8 | Class index `0..3` |
| `3–6` | Max Confidence | float32 | Probability % `0.0 - 100.0` |
| `7–10` | Latency | float32 | Inference time in ms |
| `11–26` | Probabilities | 4×float32 | Array of 4 class confidence values |

---

## Quick Start

### VS Code + PlatformIO (Recommended)
1. Open this directory in VS Code with the **PlatformIO** extension.
2. Connect your ESP32-S3 via USB.
3. Click **Build** or **Upload**. PlatformIO will automatically fetch `TensorFlowLite_ESP32` dependencies.

### Arduino IDE
1. Select **ESP32S3 Dev Module**.
2. Install `TensorFlowLite_ESP32` via Library Manager.
3. Open `ESP32_Audio_Classifier.ino` and click **Upload**.

---

## Project File Structure

```text
ESP32_Audio_Classifier/
├── ESP32_Audio_Classifier.ino  # Main Arduino entry point & serial handler
├── audio_classifier.h          # TFLite Micro engine header (4-class, 40KB arena)
├── audio_classifier.cpp        # Inference predictor & quantized INT8 de-quantizer
├── led_indicator.h             # LED indicator manager header
├── led_indicator.cpp           # GPIO LED pin driver (Yellow/Green/Purple/Red)
├── microphone_input.h          # I2S MEMS mic header
├── microphone_input.cpp        # I2S DMA audio capture driver (16kHz mono)
├── model_data.h                # v1.3.0 INT8 quantized TFLite model array (26.24 KB)
├── model_data.cpp              # Header link reference
├── platformio.ini              # PlatformIO environment config
└── README.md                   # Technical documentation
```