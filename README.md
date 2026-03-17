# HUB75E Panel Test — ESP32-S3-N16R8

A PlatformIO / Arduino test firmware for the **P4-2121-64×32-32S-JHT3.0** HUB75E LED matrix panel driven by an **ESP32-S3-N16R8** (16 MB flash, 8 MB octal PSRAM).

---

## Hardware

| Item | Details |
|---|---|
| MCU board | ESP32-S3-DevKitC-1 (N16R8) |
| Panel | P4-2121-64×32, 1/32 scan, HUB75E connector |
| Scan type | 1/32 (5-bit row address — A B C D **E**) |
| Library | [ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA) v3.x |

---

## Wiring

| HUB75E Signal | GPIO | Notes |
|---|---|---|
| R1 | 4 | |
| G1 | 5 | |
| B1 | 12 | |
| R2 | 13 | |
| G2 | 14 | |
| B2 | 15 | |
| A | 38 | |
| B | 39 | |
| C | 40 | |
| D | 41 | |
| **E** | **42** | Required for 1/32 scan |
| CLK | 2 | |
| LAT | 1 | |
| OE | 16 | |

### ESP32-S3-N16R8 GPIO rules

| Range | Status |
|---|---|
| 0, 3, 45, 46 | ⚠️ Strapping pins — avoid |
| 6 – 11 | ⛔ Internal QSPI flash |
| 35, 36, 37 | ⛔ Octal PSRAM/flash (N16R8 specific) |
| 48 | ⚠️ Connected to on-board RGB LED on many DevKitC-1 boards |
| 1–2, 4–5, 12–21, 26–34, 38–47 | ✅ Safe for general use |

---

## Key Configuration Notes

### Why `PANEL_HEIGHT = 64` for a 32-row panel

This panel uses **1/32 scan**, meaning it needs a 5-bit row address (A–E). The library only drives the E pin when the virtual canvas height is ≥ 64. Configuring the library as `64×64` is the standard workaround — the physical panel only uses rows `0–31` of that virtual canvas.

All drawing in the test functions is explicitly clipped to `REAL_HEIGHT = 32`.

### Driver choice

`HUB75_I2S_CFG::SHIFTREG` is used (plain shift-register, no init sequence). The `ICN2038S` and `FM6126A` drivers send a chip-specific initialisation sequence on startup that this panel's actual shift-register IC ignores or misinterprets, which can corrupt the scan output.

---

## Project Structure

```
├── src/
│   └── main.cpp        — All test patterns and setup
├── include/            — (empty, reserved for headers)
├── lib/                — (empty, reserved for private libs)
├── test/               — (empty, reserved for unit tests)
└── platformio.ini      — Build configuration
```

---

## Dependencies

Declared in `platformio.ini` — PlatformIO fetches them automatically:

```
mrfaptastic/ESP32 HUB75 LED MATRIX PANEL DMA Display @ ^3.0.14
adafruit/Adafruit GFX Library @ ^1.12.5
```

---

## Build & Flash

```bash
# Build
pio run

# Flash
pio run --target upload

# Monitor serial output
pio device monitor
```

Serial output is at **115 200 baud**. On first boot the ESP32-S3 USB CDC needs ~500 ms to settle before the first log line appears.

---

## Test Sequence

The firmware loops through eight tests indefinitely:

| # | Test | What to look for |
|---|---|---|
| 1 | **Solid colours** | Full-panel fills: red, green, blue, white, yellow, cyan, magenta, black — with a contrasting label |
| 2 | **Colour bars** | 8 vertical bars across the panel |
| 3 | **Walking pixel** | Single white pixel traverses every pixel left-to-right, top-to-bottom |
| 4 | **Gradient** | Horizontal ramp — red on the top half, blue on the bottom half |
| 5 | **Checkerboard** | 4×4 pixel checker pattern, blinks 4 times |
| 6 | **Scrolling text** | Green text scrolls right-to-left, vertically centred |
| 7 | **Border / corners** | White outer rect, green inner rect, red corner pixels, yellow centre cross |
| 8 | **Brightness ramp** | White fill ramps from 0 → 255 → 0, then resets to 50% |

---

## Timing Constants

All durations are `#define`d at the top of `main.cpp` for easy adjustment:

| Constant | Default | Controls |
|---|---|---|
| `SOLID_DURATION` | 1500 ms | Time per solid colour |
| `BARS_DURATION` | 2000 ms | Colour bars hold time |
| `WALK_DELAY` | 4 ms | Delay between each walking pixel step |
| `GRADIENT_DURATION` | 1500 ms | Gradient hold time |
| `CHECKER_BLINKS` | 4 | Number of checkerboard blink cycles |
| `SCROLL_PASSES` | 2 | Number of full scroll passes |
| `BORDER_DURATION` | 2000 ms | Border test hold time |
| `BRIGHTNESS_STEP_MS` | 10 ms | Delay between each brightness step |

Scroll speed is set by the `delay()` value inside `testScrollText()` — higher value = slower scroll, lower value = faster scroll.

---

## Troubleshooting

**Only the top half of the panel shows content, mirrored on the bottom half**
The E pin is not being driven. Confirm `PANEL_HEIGHT = 64` in the config and that `mxconfig.gpio.e` is assigned to a valid GPIO.

**Panel is blank / `matrix->begin()` fails**
Check power supply (5 V, adequate current), verify all GPIO assignments, and confirm no strapping/reserved pins are used.

**Corrupted colours or scan glitches**
Try `mxconfig.driver = HUB75_I2S_CFG::SHIFTREG` if another driver is set. FM6126A/ICN2038S init sequences can corrupt standard shift-register panels.

**GPIO 48 behaving unexpectedly**
GPIO 48 drives the on-board RGB LED on many ESP32-S3-DevKitC-1 boards. Use GPIO 42 for the E pin instead.
