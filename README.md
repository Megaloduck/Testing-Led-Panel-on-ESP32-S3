📟 HUB75E Panel Test — ESP32-S3 (P4 64×32)

A comprehensive hardware validation and diagnostic suite for HUB75E RGB LED matrix panels using the ESP32-S3 and DMA-based driving.

Designed specifically for:

P4-2121-64x32-32S-JHT3.0

1/32 scan HUB75E panels

ESP32-S3-N16R8 (16MB Flash + 8MB PSRAM)

🎯 Features

✅ Full HUB75E signal validation (R1/G1/B1, R2/G2/B2, A–E, CLK, LAT, OE)

✅ 8 visual diagnostic tests

✅ DMA-driven rendering (high refresh, low flicker)

✅ Safe GPIO mapping for ESP32-S3

✅ Compatible with SHIFTREG-based panels (no init sequence required)

🧠 Architecture Overview

This project uses:

DMA parallel output via I2S

Virtual canvas workaround (64×64) for 1/32 scan panels

Clipped rendering to physical rows (0–31)

Why 64×64?

HUB75E panels require 5-bit row addressing (A–E).

The library internally enables this only when height ≥ 64, so:

Virtual: 64×64
Physical: 64×32

Only the top half (0–31) maps to real LEDs.

🔌 Hardware Setup
🧩 Components

ESP32-S3 (N16R8 recommended)

HUB75E P4 64×32 LED Matrix

5V Power Supply (≥ 3A recommended)

⚠️ GPIO Constraints (ESP32-S3)

Avoid these pins:

Type	Pins
Strapping	0, 3, 45, 46
Flash (QSPI)	6–11
PSRAM (Octal)	35, 36, 37
✅ GPIO Mapping Used
Signal	GPIO
R1	4
G1	5
B1	12
R2	13
G2	14
B2	15
A	38
B	39
C	40
D	41
E	42
CLK	2
LAT	1
OE	16
⚙️ Library

ESP32-HUB75-MatrixPanel-I2S-DMA v3.x

Driver mode:

mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

⚠️ Do NOT enable FM6126A / ICN2038S init sequences
This panel uses plain shift-register logic.

🧪 Test Suite

The firmware cycles through 8 diagnostic modes:

1. 🎨 Solid Colours

Verifies full-panel fill

Checks RGB channel integrity

2. 🌈 Colour Bars

Confirms horizontal mapping

Detects column misalignment

3. 🚶 Walking Pixel

Pixel-by-pixel scan test

Validates addressing + timing

4. 📊 Gradient

Tests color depth & PWM behavior

Top: Red gradient

Bottom: Blue gradient

5. ♟️ Checkerboard

High-frequency pixel toggling

Detects ghosting / flicker

6. 🔤 Scrolling Text

Text rendering + DMA buffering

Confirms font pipeline

7. 📦 Border Test

Edge alignment validation

Corner pixel accuracy

8. 🔆 Brightness Ramp

Full 0–255 brightness sweep

Validates OE + PWM control

🚀 Getting Started
PlatformIO (Recommended)
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

lib_deps =
  ESP32-HUB75-MatrixPanel-I2S-DMA
Upload
pio run -t upload
Serial Monitor
pio device monitor

Expected output:

=== HUB75E Test ESP32-S3-N16R8 ===
Matrix init OK
Virtual canvas 64x64  Physical rows 0-31
⚠️ Power Considerations

Use 5V ≥ 3A supply

Do NOT power from USB alone

Connect GND (ESP32 ↔ Panel)

🐛 Troubleshooting
❌ No display

Check HUB75 cable orientation

Verify OE / LAT / CLK wiring

Confirm power supply

❌ Flickering / unstable image

Ensure stable 5V supply

Avoid bad GPIO pins

Reduce brightness

❌ Half panel glitching

Confirm E pin (GPIO42) connected

Ensure 64×64 virtual config

❌ Random noise / corruption

Wrong driver mode → use SHIFTREG

Remove FM6126A init if enabled

📌 Notes

This firmware is intended for hardware validation, not production UI

All rendering is clipped to REAL_HEIGHT (32) to avoid ghost rows

DMA is used for high refresh and minimal CPU usage
