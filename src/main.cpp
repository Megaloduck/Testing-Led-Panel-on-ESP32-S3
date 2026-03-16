/*
 * HUB75E Panel Test — P4-2121-64x32-32S-JHT3.0
 * Target: ESP32-S3-N16R8 (16MB flash, 8MB octal PSRAM)
 * Library: ESP32-HUB75-MatrixPanel-I2S-DMA v3.x
 *
 * GPIO selection rules for ESP32-S3-N16R8:
 *   AVOID  0, 3, 45, 46        — strapping pins
 *   AVOID  6–11                — internal QSPI flash
 *   AVOID  35, 36, 37          — octal PSRAM/flash (N16R8 specific)
 *   SAFE   1–2, 4–5, 12–21,
 *          26–34, 38–48        — general use
 *
 * HUB75E pin mapping used here (all safe for S3-N16R8):
 *   R1=4   G1=5   B1=12
 *   R2=13  G2=14  B2=15
 *   A=38   B=39   C=40   D=41  E=42
 *   CLK=2  LAT=1  OE=16
 *
 * Tests cycle forever:
 *   1. Solid colours
 *   2. Colour bars
 *   3. Walking pixel
 *   4. Gradient (red/blue)
 *   5. Checkerboard
 *   6. Scrolling text
 *   7. Border / corner test
 *   8. Brightness ramp
 */

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PIN_R1   4
#define PIN_G1   5
#define PIN_B1   12
#define PIN_R2   13
#define PIN_G2   14
#define PIN_B2  15
#define PIN_A   38
#define PIN_B   39
#define PIN_C   40
#define PIN_D   41
#define PIN_E   48
#define PIN_CLK 2
#define PIN_LAT 1
#define PIN_OE  16

// ─── Timing ────────────────────────────────────────────────────────────────
#define SOLID_DURATION      1500   // ms per solid colour
#define BARS_DURATION       2000
#define WALK_DELAY             4   // ms per pixel
#define GRADIENT_DURATION   1500
#define CHECKER_BLINKS         4
#define SCROLL_PASSES          2
#define BORDER_DURATION     2000
#define BRIGHTNESS_STEP_MS    10

// ─── Panel geometry ────────────────────────────────────────────────────────
#define PANEL_WIDTH   64
#define PANEL_HEIGHT  32
#define PANEL_CHAIN    1
 
MatrixPanel_I2S_DMA *matrix = nullptr;
 
// ─── Colour helper ─────────────────────────────────────────────────────────
inline uint16_t C(uint8_t r, uint8_t g, uint8_t b) {
    return matrix->color565(r, g, b);
}
 
// ─── Test 1: Solid colours ─────────────────────────────────────────────────
void testSolidColours() {
    Serial.println("[1] Solid colours");
    struct { uint8_t r, g, b; const char *name; } cols[] = {
        {255,   0,   0, "RED"},
        {  0, 255,   0, "GREEN"},
        {  0,   0, 255, "BLUE"},
        {255, 255, 255, "WHITE"},
        {255, 255,   0, "YELLOW"},
        {  0, 255, 255, "CYAN"},
        {255,   0, 255, "MAGENTA"},
        {  0,   0,   0, "BLACK"},
    };
    for (auto &c : cols) {
        Serial.printf("    %s\n", c.name);
        matrix->fillScreenRGB888(c.r, c.g, c.b);
        // contrasting label
        matrix->setTextColor(C(255 - c.r, 255 - c.g, 255 - c.b));
        matrix->setTextSize(1);
        matrix->setCursor(1, 12);
        matrix->print(c.name);
        delay(SOLID_DURATION);
    }
}
 
// ─── Test 2: Colour bars ───────────────────────────────────────────────────
void testColourBars() {
    Serial.println("[2] Colour bars");
    matrix->clearScreen();
    int bw = PANEL_WIDTH / 8;
    uint16_t bars[8] = {
        C(255,   0,   0), C(255, 165,   0), C(255, 255,   0),
        C(  0, 255,   0), C(  0, 255, 255), C(  0,   0, 255),
        C(128,   0, 128), C(255, 255, 255),
    };
    for (int i = 0; i < 8; i++) {
        matrix->fillRect(i * bw, 0, bw, PANEL_HEIGHT, bars[i]);
    }
    delay(BARS_DURATION);
}
 
// ─── Test 3: Walking pixel ─────────────────────────────────────────────────
void testWalkingPixel() {
    Serial.println("[3] Walking pixel");
    matrix->clearScreen();
    uint16_t white = C(255, 255, 255);
    for (int y = 0; y < PANEL_HEIGHT; y++) {
        for (int x = 0; x < PANEL_WIDTH; x++) {
            // erase previous
            if (x > 0)       matrix->drawPixel(x - 1, y,     C(0, 0, 0));
            else if (y > 0)  matrix->drawPixel(PANEL_WIDTH - 1, y - 1, C(0, 0, 0));
            matrix->drawPixel(x, y, white);
            delay(WALK_DELAY);
        }
    }
    matrix->drawPixel(PANEL_WIDTH - 1, PANEL_HEIGHT - 1, C(0, 0, 0));
}
 
// ─── Test 4: Gradient ─────────────────────────────────────────────────────
void testGradient() {
    Serial.println("[4] Gradient");
    matrix->clearScreen();
    for (int x = 0; x < PANEL_WIDTH; x++) {
        uint8_t v = map(x, 0, PANEL_WIDTH - 1, 0, 255);
        for (int y = 0; y < PANEL_HEIGHT; y++) {
            if (y < PANEL_HEIGHT / 2)
                matrix->drawPixel(x, y, C(v, 0, 0));       // red top
            else
                matrix->drawPixel(x, y, C(0, 0, v));       // blue bottom
        }
    }
    delay(GRADIENT_DURATION);
}
 
// ─── Test 5: Checkerboard ─────────────────────────────────────────────────
void testCheckerboard() {
    Serial.println("[5] Checkerboard");
    for (int blink = 0; blink < CHECKER_BLINKS; blink++) {
        for (int y = 0; y < PANEL_HEIGHT; y++) {
            for (int x = 0; x < PANEL_WIDTH; x++) {
                bool on = ((x / 4 + y / 4 + blink) % 2) == 0;
                matrix->drawPixel(x, y, on ? C(255, 255, 255) : C(0, 0, 0));
            }
        }
        delay(500);
    }
}
 
// ─── Test 6: Scrolling text ────────────────────────────────────────────────
void testScrollText() {
    Serial.println("[6] Scroll text");
    const char *msg     = "  HUB75E OK  P4-64x32  ESP32-S3  ";
    int msgPixelLen     = strlen(msg) * 6;   // font-1 = 6px/char
    int startX          = PANEL_WIDTH;
    int endX            = -msgPixelLen;
 
    for (int pass = 0; pass < SCROLL_PASSES; pass++) {
        for (int xpos = startX; xpos > endX; xpos--) {
            matrix->clearScreen();
            matrix->setTextColor(C(0, 255, 80));
            matrix->setTextSize(1);
            matrix->setCursor(xpos, 12);
            matrix->print(msg);
            delay(18);
        }
    }
}
 
// ─── Test 7: Border / corners ─────────────────────────────────────────────
void testBorder() {
    Serial.println("[7] Border");
    matrix->clearScreen();
 
    matrix->drawRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT, C(255, 255, 255));
    matrix->drawRect(2, 2, PANEL_WIDTH - 4, PANEL_HEIGHT - 4, C(0, 200, 0));
 
    // red corner dots
    matrix->drawPixel(0,               0,                C(255, 0, 0));
    matrix->drawPixel(PANEL_WIDTH - 1, 0,                C(255, 0, 0));
    matrix->drawPixel(0,               PANEL_HEIGHT - 1, C(255, 0, 0));
    matrix->drawPixel(PANEL_WIDTH - 1, PANEL_HEIGHT - 1, C(255, 0, 0));
 
    // yellow centre cross
    int cx = PANEL_WIDTH  / 2;
    int cy = PANEL_HEIGHT / 2;
    matrix->drawLine(cx - 3, cy, cx + 3, cy, C(255, 255, 0));
    matrix->drawLine(cx, cy - 3, cx, cy + 3, C(255, 255, 0));
 
    delay(BORDER_DURATION);
}
 
// ─── Test 8: Brightness ramp ───────────────────────────────────────────────
void testBrightnessRamp() {
    Serial.println("[8] Brightness ramp");
    matrix->fillScreenRGB888(255, 255, 255);
    for (int b = 0; b <= 255; b++) {
        matrix->setBrightness8(b);
        delay(BRIGHTNESS_STEP_MS);
    }
    for (int b = 255; b >= 0; b--) {
        matrix->setBrightness8(b);
        delay(BRIGHTNESS_STEP_MS);
    }
    matrix->setBrightness8(128);
}
 
// ─── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);   // let USB CDC settle on S3
    Serial.println("\n=== HUB75E Test  ESP32-S3-N16R8 ===");
 
    HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN);
 
    mxconfig.gpio.r1  = PIN_R1;
    mxconfig.gpio.g1  = PIN_G1;
    mxconfig.gpio.b1  = PIN_B1;
    mxconfig.gpio.r2  = PIN_R2;
    mxconfig.gpio.g2  = PIN_G2;
    mxconfig.gpio.b2  = PIN_B2;
    mxconfig.gpio.a   = PIN_A;
    mxconfig.gpio.b   = PIN_B;
    mxconfig.gpio.c   = PIN_C;
    mxconfig.gpio.d   = PIN_D;
    mxconfig.gpio.e   = PIN_E;   // 1/32 scan — critical
    mxconfig.gpio.clk = PIN_CLK;
    mxconfig.gpio.lat = PIN_LAT;
    mxconfig.gpio.oe  = PIN_OE;

    // ── Driver selection ────────────────────────────────────────────────
    // JHT3.0 / standard shift-register panels with E pin need this driver.
    // SHIFTREG_ABC_BIN_DE enables the 5th address bit (E) for 1/32 scan.
    // If the panel still shows only the top half, try FM6126A instead.
    mxconfig.driver = HUB75_I2S_CFG::ICN2038S;
 
    // 74HC04D on the panel inverts OE; library drives active-low,
    // inverter restores correct polarity — no extra config needed.
    mxconfig.clkphase = false;
 
    // S3-N16R8 has 8MB PSRAM — allocate DMA buffer from PSRAM if needed.
    // The library will use internal RAM first; with a single 64x32 panel
    // this fits comfortably in SRAM without touching PSRAM.
    mxconfig.double_buff = false;  // single buffer fine for test patterns
 
    matrix = new MatrixPanel_I2S_DMA(mxconfig);
 
    if (!matrix->begin()) {
        Serial.println("ERROR: matrix->begin() failed!");
        Serial.println("  Check: wiring, GPIO conflicts, power supply.");
        while (true) {
            delay(500);
            Serial.print('.');
        }
    }
 
    matrix->setBrightness8(128);   // 50% — safe for bench testing
    matrix->clearScreen();
 
    Serial.println("Matrix init OK");
    Serial.printf("Panel %dx%d, chain=%d\n", PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN);
    Serial.println("GPIO map:");
    Serial.printf("  R1=%d G1=%d B1=%d  R2=%d G2=%d B2=%d\n",
                  PIN_R1, PIN_G1, PIN_B1, PIN_R2, PIN_G2, PIN_B2);
    Serial.printf("  A=%d B=%d C=%d D=%d E=%d\n",
                  PIN_A, PIN_B, PIN_C, PIN_D, PIN_E);
    Serial.printf("  CLK=%d LAT=%d OE=%d\n", PIN_CLK, PIN_LAT, PIN_OE);
}
 
// ─── Loop ──────────────────────────────────────────────────────────────────
void loop() {
    testSolidColours();
    testColourBars();
    testWalkingPixel();
    testGradient();
    testCheckerboard();
    testScrollText();
    testBorder();
    testBrightnessRamp();
 
    Serial.println("--- cycle complete ---\n");
    delay(500);
}