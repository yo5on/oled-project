// =====================================================
// MOCHI COMPANION — a tiny autonomous character on a 128x64 OLED
// ESP32 + SSD1306 (I2C) + 2 buttons
//
// Mochi picks its own animations from its mood (MochiBehavior.h).
// Buttons are interactions, not a menu:
//   BUTTON 1 : friendly (wakes Mochi, makes it happy / playful)
//   BUTTON 2 : poke (wakes Mochi, surprises it; makes it curious / playful)
//   many presses quickly : annoyed, sometimes dizzy
// Serial (115200) logs moods, choices and button reactions.
// =====================================================

// 1 = HARDWARE TEST MODE: fast timings (moods 10-15 s, emotions 3-5 s,
//     sleep after 2 min), verbose Serial log, Serial '1'/'2' act as buttons.
// 0 = normal companion (production)
#define MOCHI_HW_TEST 0

#if MOCHI_HW_TEST
#define MOCHI_FAST_TIMING
#endif

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bootloader_random.h"

#include "MochiAnimations.h"
#include "MochiPlayer.h"
#include "MochiBehavior.h"

// ---- Buttons (to GND, INPUT_PULLUP) ----
#define BTN_1      25
#define BTN_2      26

// ---- OLED (I2C) ----
#define OLED_SDA   22
#define OLED_SCL   21
#define OLED_ADDR  0x3C
#define SCREEN_W   128
#define SCREEN_H   64

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// MochiScreen for the SSD1306
class OledScreen : public MochiScreen {
public:
  void showFrame(const uint8_t* frame, bool inverted) override {
    if (inverted != inverted_) {
      display.invertDisplay(inverted);
      inverted_ = inverted;
    }
    display.clearDisplay();
    display.drawBitmap(0, 0, frame, SCREEN_W, SCREEN_H, SSD1306_WHITE);
    display.display();
  }
  void setDim(bool dim) override { display.dim(dim); }
  void setPower(bool on) override { display.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF); }

private:
  bool inverted_ = false;
};

struct Button {
  uint8_t pin;
  bool stable;               // debounced level (true = pressed)
  bool lastRaw;
  unsigned long changedAt;
};

OledScreen screen;
MochiPlayer player(screen);

Button btn1 = { BTN_1, false, false, 0 };
Button btn2 = { BTN_2, false, false, 0 };
const unsigned long DEBOUNCE_MS = 30;

uint32_t mochiRandom(uint32_t n) {
  return n ? (uint32_t)random((long)n) : 0;   // ESP32 hardware RNG (esp_random)
}

void mochiLog(const char* event, const char* detail) {
  Serial.print(millis() / 1000.0f, 1);
  Serial.print("s  ");
  Serial.print(event);
  Serial.print(": ");
  Serial.println(detail);
}

MochiBehavior mochi(player, mochiRandom, mochiLog);

// True once per press (debounced; holding a button counts once)
bool pressed(Button &b) {
  bool raw = (digitalRead(b.pin) == LOW);
  unsigned long now = millis();
  if (raw != b.lastRaw) {
    b.lastRaw = raw;
    b.changedAt = now;
  }
  if (raw != b.stable && now - b.changedAt >= DEBOUNCE_MS) {
    b.stable = raw;
    return b.stable;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 not found");
    while (true) delay(1000);
  }
  Serial.println();
  Serial.println(MOCHI_HW_TEST ? "BOOT (MOCHI COMPANION - HARDWARE TEST MODE)" : "BOOT (MOCHI COMPANION)");

  mochi.setVerbose(MOCHI_HW_TEST);
  // True hardware entropy while the first mood/animation are chosen (no Wi-Fi/BT running)
  bootloader_random_enable();
  mochi.begin(millis());
  bootloader_random_disable();
}

void loop() {
  uint32_t now = millis();
  if (pressed(btn1)) mochi.onButton(0, now);
  if (pressed(btn2)) mochi.onButton(1, now);
#if MOCHI_HW_TEST
  while (Serial.available()) {                 // '1' / '2' over Serial = button 1 / 2
    char c = Serial.read();
    if (c == '1' || c == '2') mochi.onButton(c - '1', millis());
  }
#endif
  mochi.update(now);
}
