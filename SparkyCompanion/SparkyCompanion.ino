// =====================================================
// SPARKY COMPANION — a tiny autonomous character on a 128x64 OLED
// ESP32 + SSD1306 (I2C) + 2 buttons
//
// Sparky picks its own animations from its mood (SparkyBehavior.h).
// Buttons are interactions, not a menu:
//   BUTTON 1 : friendly (wakes Sparky, makes it happy / playful)
//   BUTTON 2 : poke (wakes Sparky, surprises it; makes it curious / playful)
//   many presses quickly : annoyed, sometimes dizzy
//   BOTH BUTTONS TOGETHER : Gallery Mode (the V3 photos / videos); together again: back
//     in Gallery Mode: BUTTON 1 = previous item, BUTTON 2 = next item (as in V3)
// Serial (115200) logs moods, choices and button reactions.
//
// Gallery media: gallery_media_private.h, made locally by tools/make_gallery.py from the
// V3 sketch (never committed). Its ~600 KB need Tools > Partition Scheme > "Huge APP".
// Without that file the gallery shows one placeholder picture.
// =====================================================

// 1 = HARDWARE TEST MODE: fast timings (moods 10-15 s, emotions 3-5 s,
//     sleep after 2 min), verbose Serial log, Serial '1'/'2' act as buttons, '3' as both.
// 0 = normal companion (production)
#define SPARKY_HW_TEST 0

#if SPARKY_HW_TEST
#define SPARKY_FAST_TIMING
#endif

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bootloader_random.h"

#include "SparkyAnimations.h"
#include "SparkyPlayer.h"
#include "SparkyBehavior.h"
#include "SparkyButtons.h"
#include "SparkyGallery.h"

#if defined(SPARKY_GALLERY_PRIVATE) && defined(ARDUINO_PARTITION_default)
#error "The private gallery media do not fit the default partition: Tools > Partition Scheme > Huge APP (3MB No OTA/1MB SPIFFS)"
#endif

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

// SparkyScreen for the SSD1306. The panel's own invert mode stays OFF: an inverted
// frame is flipped in software while it is drawn, so every update is one complete,
// final image (switching the panel's invert flashed the old image for one transfer).
class OledScreen : public SparkyScreen {
public:
  void showFrame(const uint8_t* frame, bool inverted) override {
    if (inverted) display.drawBitmap(0, 0, frame, SCREEN_W, SCREEN_H, SSD1306_BLACK, SSD1306_WHITE);
    else display.drawBitmap(0, 0, frame, SCREEN_W, SCREEN_H, SSD1306_WHITE, SSD1306_BLACK);
    display.display();
  }
  void setContrast(uint8_t level) override {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(level);
  }
};

OledScreen screen;
SparkyPlayer player(screen);
SparkyGallery gallery(screen);                // separate from Sparky; owns the screen while active
SparkyButtons buttons;                        // single presses and both-together (SparkyButtons.h)

uint32_t sparkyRandom(uint32_t n) {
  return n ? (uint32_t)random((long)n) : 0;   // ESP32 hardware RNG (esp_random)
}

void sparkyLog(const char* event, const char* detail) {
  Serial.print(millis() / 1000.0f, 1);
  Serial.print("s  ");
  Serial.print(event);
  Serial.print(": ");
  Serial.println(detail);
}

SparkyBehavior sparky(player, sparkyRandom, sparkyLog);

// Both buttons together: into Gallery Mode, or back to Sparky (where it left off)
void toggleGallery(uint32_t now) {
  if (!gallery.active()) {
    gallery.enter(now);
    sparkyLog("GALLERY", gallery.item().name);
  } else {
    gallery.exit();
    sparkyLog("GALLERY", "exit");
    sparky.resume(now);
  }
}

// A button event: in Gallery Mode it navigates, otherwise it goes to Sparky
void onButtonEvent(ButtonEvent ev, uint32_t now) {
  if (ev == ButtonEvent::None) return;
  if (ev == ButtonEvent::Both) {
    toggleGallery(now);
  } else if (gallery.active()) {             // button 1: previous item, button 2: next item
    bool moved = ev == ButtonEvent::Button1 ? gallery.previous(now) : gallery.next(now);
    if (moved) sparkyLog(ev == ButtonEvent::Button1 ? "GALLERY <" : "GALLERY >", gallery.item().name);
  } else {
    sparky.onButton(ev == ButtonEvent::Button1 ? 0 : 1, now);
  }
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
  display.invertDisplay(false);              // never changed afterwards
  Serial.println();
  Serial.println(SPARKY_HW_TEST ? "BOOT (SPARKY COMPANION - HARDWARE TEST MODE)" : "BOOT (SPARKY COMPANION)");

  sparky.setVerbose(SPARKY_HW_TEST);
  // True hardware entropy while the first mood/animation are chosen (no Wi-Fi/BT running)
  bootloader_random_enable();
  sparky.begin(millis());
  bootloader_random_disable();
}

void loop() {
  uint32_t now = millis();
  onButtonEvent(buttons.update(digitalRead(BTN_1) == LOW, digitalRead(BTN_2) == LOW, now), now);
#if SPARKY_HW_TEST
  while (Serial.available()) {                 // '1' / '2' over Serial = button 1 / 2, '3' = both
    char c = Serial.read();
    if (c == '1' || c == '2' || c == '3') onButtonEvent(c == '1' ? ButtonEvent::Button1 : c == '2' ? ButtonEvent::Button2 : ButtonEvent::Both, millis());
  }
#endif
  if (gallery.active()) gallery.update(now);   // Sparky is paused meanwhile (nothing advances)
  else sparky.update(now);
}
