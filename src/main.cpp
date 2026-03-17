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
 * Panel note:
 *   Physical panel is 64x32 with 1/32 scan (HUB75E, 5-bit row address).
 *   The library is configured as 64x64 — the standard workaround for
 *   1/32-scan 64x32 panels — which forces the E address bit to be driven.
 *   Only the top 32 rows of the virtual canvas map to real pixels.
 *   All drawing is clipped to y=0..31 to avoid artefacts on the lower half.
 *
 *   Driver: SHIFTREG (plain shift-register, no init sequence).
 *   ICN2038S / FM6126A init sequences are not needed and can corrupt scan.
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

// ─── GPIO ──────────────────────────────────────────────────────────────────
#define PIN_R1   4
#define PIN_G1   5
#define PIN_B1  12
#define PIN_R2  13
#define PIN_G2  14
#define PIN_B2  15
#define PIN_A   38
#define PIN_B   39
#define PIN_C   40
#define PIN_D   41
#define PIN_E   42   // GPIO42 — for 1/32 scan panels
#define PIN_CLK  2
#define PIN_LAT  1
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
// Virtual canvas is 64x64 to force E-pin / 1/32-scan addressing.
// Physical pixels exist only in rows 0–31.
#define PANEL_WIDTH    64
#define PANEL_HEIGHT   64   // virtual — forces E-pin toggling
#define REAL_HEIGHT    32   // actual physical rows on the panel
#define PANEL_CHAIN     1

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
        // Fill only physical rows
        matrix->fillRect(0, 0, PANEL_WIDTH, REAL_HEIGHT, C(c.r, c.g, c.b));
        // Clear virtual lower half to avoid ghost rows
        matrix->fillRect(0, REAL_HEIGHT, PANEL_WIDTH, REAL_HEIGHT, C(0, 0, 0));
        // Contrasting label in upper area
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
        matrix->fillRect(i * bw, 0, bw, REAL_HEIGHT, bars[i]);
    }
    delay(BARS_DURATION);
}

// ─── Test 3: Walking pixel ─────────────────────────────────────────────────
void testWalkingPixel() {
    Serial.println("[3] Walking pixel");
    matrix->clearScreen();
    uint16_t white = C(255, 255, 255);
    for (int y = 0; y < REAL_HEIGHT; y++) {
        for (int x = 0; x < PANEL_WIDTH; x++) {
            if (x > 0)
                matrix->drawPixel(x - 1, y,               C(0, 0, 0));
            else if (y > 0)
                matrix->drawPixel(PANEL_WIDTH - 1, y - 1, C(0, 0, 0));
            matrix->drawPixel(x, y, white);
            delay(WALK_DELAY);
        }
    }
    matrix->drawPixel(PANEL_WIDTH - 1, REAL_HEIGHT - 1, C(0, 0, 0));
}

// ─── Test 4: Gradient ─────────────────────────────────────────────────────
void testGradient() {
    Serial.println("[4] Gradient");
    matrix->clearScreen();
    for (int x = 0; x < PANEL_WIDTH; x++) {
        uint8_t v = map(x, 0, PANEL_WIDTH - 1, 0, 255);
        for (int y = 0; y < REAL_HEIGHT; y++) {
            if (y < REAL_HEIGHT / 2)
                matrix->drawPixel(x, y, C(v, 0, 0));   // red top half
            else
                matrix->drawPixel(x, y, C(0, 0, v));   // blue bottom half
        }
    }
    delay(GRADIENT_DURATION);
}

// ─── Test 5: Checkerboard ─────────────────────────────────────────────────
void testCheckerboard() {
    Serial.println("[5] Checkerboard");
    for (int blink = 0; blink < CHECKER_BLINKS; blink++) {
        for (int y = 0; y < REAL_HEIGHT; y++) {
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
        
        const char *msg = "  HUB75E OK  P4-64x32  ESP32-S3  ";
        
        int16_t x1, y1;
        uint16_t w, h;    
        matrix->getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);        
        int startX = PANEL_WIDTH;
        int endX   = -w;
        int textY  = (REAL_HEIGHT - h) / 2;
        matrix->setTextColor(C(0, 255, 80));
        matrix->setTextSize(1);
        
        for (int pass = 0; pass < SCROLL_PASSES; pass++) {
                for (int xpos = startX; xpos > endX; xpos--) {
                    
                    matrix->clearScreen();   
                    matrix->setCursor(xpos, textY); 
                    matrix->print(msg);                     
                    matrix->flipDMABuffer(); // important if using DMA
                    
                    delay(40); // Higher the value = slower the scroll. Lower the value = faster the scroll
        }
    }   
}

// ─── Test 7: Border / corners ─────────────────────────────────────────────
void testBorder() {
    Serial.println("[7] Border");
    matrix->clearScreen();

    // Outer and inner border confined to physical rows
    matrix->drawRect(0, 0, PANEL_WIDTH, REAL_HEIGHT, C(255, 255, 255));
    matrix->drawRect(2, 2, PANEL_WIDTH - 4, REAL_HEIGHT - 4, C(0, 200, 0));

    // Red corner dots
    matrix->drawPixel(0,               0,               C(255, 0, 0));
    matrix->drawPixel(PANEL_WIDTH - 1, 0,               C(255, 0, 0));
    matrix->drawPixel(0,               REAL_HEIGHT - 1, C(255, 0, 0));
    matrix->drawPixel(PANEL_WIDTH - 1, REAL_HEIGHT - 1, C(255, 0, 0));

    // Yellow centre cross
    int cx = PANEL_WIDTH / 2;
    int cy = REAL_HEIGHT / 2;
    matrix->drawLine(cx - 4, cy, cx + 4, cy, C(255, 255, 0));
    matrix->drawLine(cx, cy - 4, cx, cy + 4, C(255, 255, 0));

    delay(BORDER_DURATION);
}

// ─── Test 8: Brightness ramp ───────────────────────────────────────────────
void testBrightnessRamp() {
    Serial.println("[8] Brightness ramp");
    matrix->fillRect(0, 0, PANEL_WIDTH, REAL_HEIGHT, C(255, 255, 255));
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
    delay(500);
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
    mxconfig.gpio.e   = PIN_E;
    mxconfig.gpio.clk = PIN_CLK;
    mxconfig.gpio.lat = PIN_LAT;
    mxconfig.gpio.oe  = PIN_OE;

    // Plain shift-register driver — no FM6126A/ICN2038S init sequence.
    // 64x64 virtual height forces the E address bit to be driven,
    // which is required for this panel's 1/32 scan rate.
    mxconfig.driver      = HUB75_I2S_CFG::SHIFTREG;
    mxconfig.clkphase    = false;
    mxconfig.double_buff = false;

    matrix = new MatrixPanel_I2S_DMA(mxconfig);

    if (!matrix->begin()) {
        Serial.println("ERROR: matrix->begin() failed!");
        Serial.println("  Check: wiring, GPIO conflicts, power supply.");
        while (true) { delay(500); Serial.print('.'); }
    }

    matrix->setBrightness8(128);
    matrix->clearScreen();

    Serial.println("Matrix init OK");
    Serial.printf("Virtual canvas %dx%d  Physical rows 0-%d  Chain=%d\n",
                  PANEL_WIDTH, PANEL_HEIGHT, REAL_HEIGHT - 1, PANEL_CHAIN);
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