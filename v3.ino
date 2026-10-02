// =====================================================
// OLED 128x64 — 5 VIDEO SLOTS + 5 IMAGE SLOTS (EMPTY) + ANIMATED EYES
// LEFT BUTTON  = PREVIOUS
// RIGHT BUTTON = NEXT
// BOTH BUTTONS = ENTER / EXIT EYE MODE
// =====================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDR 0x3C

// Change these two pins to the button pins you selected
#define BTN_PREV  25
#define BTN_NEXT  26

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// Animated robot-face emotions (drawn live, no bitmaps) - see EyeVariants.h
#include "EyeVariants.h"


// =====================================================
// MEDIA SLOTS — INTENTIONALLY EMPTY
// =====================================================
// No video or image data is included yet. Add media later:
//
//   Video: paste 128x64 frames as
//            const uint8_t PROGMEM videoN_frame0[1024] = { ... };
//          list them in
//            const uint8_t* videoNFrames[] = { videoN_frame0, ... };
//            const uint16_t videoNFrameCount = sizeof(videoNFrames) / sizeof(videoNFrames[0]);
//          then set the slot in contents[] to
//            { VIDEO, videoNFrames, videoNFrameCount, nullptr }
//          and its frame time (ms) in videoFrameMs[].
//
//   Image: paste a 128x64 bitmap as
//            const uint8_t PROGMEM imageN[1024] = { ... };
//          then set the slot in contents[] to
//            { IMAGE, nullptr, 0, imageN }
//
// Empty slots show "Slot N: not loaded".

// 👇 VIDEO 1 DATA HERE (empty)

// 👇 VIDEO 2 DATA HERE (empty)

// 👇 VIDEO 3 DATA HERE (empty)

// 👇 VIDEO 4 DATA HERE (empty)

// 👇 VIDEO 5 DATA HERE (empty)

// 👇 IMAGE 1 DATA HERE (empty)

// 👇 IMAGE 2 DATA HERE (empty)

// 👇 IMAGE 3 DATA HERE (empty)

// 👇 IMAGE 4 DATA HERE (empty)

// 👇 IMAGE 5 DATA HERE (empty)

enum ContentType {
  VIDEO,
  IMAGE,
  EYES   // frameCount = EyeVariantId (EyeVariants.h)
};

struct Content {
  ContentType type;
  const uint8_t **frames;
  uint16_t frameCount;
  const uint8_t *image;
};


// =====================================================
// CURRENT CONTENT
// =====================================================

int currentContent = 0;

// Slots 0-4 = videos 1-5, slots 5-9 = images 1-5, slots 10+ = eye variants.
// Media slots are intentionally EMPTY for now and show "not loaded".
Content contents[] = {
  { VIDEO, nullptr, 0, nullptr },                      // 0: Video 1 - EMPTY (add later)
  { VIDEO, nullptr, 0, nullptr },                      // 1: Video 2 - EMPTY (add later)
  { VIDEO, nullptr, 0, nullptr },                      // 2: Video 3 - EMPTY (add later)
  { VIDEO, nullptr, 0, nullptr },                      // 3: Video 4 - EMPTY (add later)
  { VIDEO, nullptr, 0, nullptr },                      // 4: Video 5 - EMPTY (add later)
  { IMAGE, nullptr, 0, nullptr },                      // 5: Image 1 - EMPTY (add later)
  { IMAGE, nullptr, 0, nullptr },                      // 6: Image 2 - EMPTY (add later)
  { IMAGE, nullptr, 0, nullptr },                      // 7: Image 3 - EMPTY (add later)
  { IMAGE, nullptr, 0, nullptr },                      // 8: Image 4 - EMPTY (add later)
  { IMAGE, nullptr, 0, nullptr },                      // 9: Image 5 - EMPTY (add later)
  { EYES,  nullptr, EYE_HAPPY, nullptr },              // 10: Eye 1 - Happy
  { EYES,  nullptr, EYE_SAD, nullptr },                // 11: Eye 2 - Sad
  { EYES,  nullptr, EYE_ANGRY, nullptr },              // 12: Eye 3 - Angry
  { EYES,  nullptr, EYE_SLEEPY, nullptr },             // 13: Eye 4 - Sleepy
  { EYES,  nullptr, EYE_SURPRISED, nullptr },          // 14: Eye 5 - Surprised
  { EYES,  nullptr, EYE_WORRIED, nullptr },            // 15: Eye 6 - Worried
  { EYES,  nullptr, EYE_CONFUSED, nullptr },           // 16: Eye 7 - Confused
  { EYES,  nullptr, EYE_EXCITED, nullptr },            // 17: Eye 8 - Excited
  { EYES,  nullptr, EYE_BORED, nullptr },              // 18: Eye 9 - Bored
  { EYES,  nullptr, EYE_SCARED, nullptr },             // 19: Eye 10 - Scared
  { EYES,  nullptr, EYE_FURIOUS, nullptr },            // 20: Eye 11 - Furious
};

const int TOTAL_CONTENT = sizeof(contents) / sizeof(contents[0]);

// Frame time per slot in ms (only used by VIDEO slots; set when a video is added)
const unsigned long videoFrameMs[TOTAL_CONTENT] = {
  0, 0, 0, 0, 0,     // 0-4: videos 1-5 (empty)
  0, 0, 0, 0, 0,     // 5-9: images 1-5
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // 10+: eye variants
};


// =====================================================
// MODE
// =====================================================

// NORMAL_MODE: PREV/NEXT step through contents[] (unchanged)
// EYE_MODE:    autonomous - picks mood-weighted random eyeVariants[];
//              single PREV/NEXT presses count toward a random 1..5
//              threshold; Happy and Sad then type a message on a white screen
// Press PREV + NEXT together to toggle between the two.
enum Mode {
  NORMAL_MODE,
  EYE_MODE
};

Mode currentMode = NORMAL_MODE;
uint8_t currentEye = EYE_HAPPY;   // index into eyeVariants[] (EyeVariants.h)


// =====================================================
// BUTTON STATE
// =====================================================

struct DebouncedButton {
  uint8_t pin;
  bool down;                  // debounced state (true = pressed)
  bool rawDown;               // last raw reading
  unsigned long rawChangedAt; // when the raw reading last changed
  bool pending;               // pressed, single action not fired yet
  unsigned long pressedAt;    // when the debounced press started
};

DebouncedButton btnPrev = { BTN_PREV, false, false, 0, false, 0 };
DebouncedButton btnNext = { BTN_NEXT, false, false, 0, false, 0 };

unsigned long lastButtonTime = 0;
const unsigned long debounceTime = 200;  // min gap between single actions (as before)
const unsigned long stableTime = 30;     // contact-bounce filter
const unsigned long comboWindow = 80;    // time to press the 2nd button for PREV+NEXT

bool comboLatched = false;        // PREV+NEXT handled; wait until both are released
bool contentNeedsRedraw = false;  // makes playContent() redraw after leaving EYE_MODE


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // OLED I2C
  Wire.begin(22, 21);   // SDA, SCL

  // Buttons
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);

  // OLED
  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        SCREEN_ADDR
      )) {

    Serial.println("SSD1306 allocation failed");

    while (true);
  }

  display.clearDisplay();
  display.display();

  Serial.println("OLED READY");
}


// =====================================================
// NEXT CONTENT
// =====================================================

void nextContent() {

  // Normal mode skips EYES slots (eyes are reached through EYE_MODE only)
  for (int i = 0; i < TOTAL_CONTENT; i++) {
    currentContent++;

    if (currentContent >= TOTAL_CONTENT) {
      currentContent = 0;
    }

    if (contents[currentContent].type != EYES) break;
  }

  Serial.print("Next: ");
  Serial.println(currentContent);
}


// =====================================================
// PREVIOUS CONTENT
// =====================================================

void previousContent() {

  // Normal mode skips EYES slots (eyes are reached through EYE_MODE only)
  for (int i = 0; i < TOTAL_CONTENT; i++) {
    currentContent--;

    if (currentContent < 0) {
      currentContent = TOTAL_CONTENT - 1;
    }

    if (contents[currentContent].type != EYES) break;
  }

  Serial.print("Previous: ");
  Serial.println(currentContent);
}


// =====================================================
// PLAY CONTENT
// =====================================================

void playContent() {

  static int shownContent = -1;
  static uint16_t frameIdx = 0;
  static unsigned long lastFrameMs = 0;

  const Content &c = contents[currentContent];
  bool slotChanged = (shownContent != currentContent);
  bool changed = slotChanged || contentNeedsRedraw;  // redraw after EYE_MODE
  contentNeedsRedraw = false;

  if (slotChanged) {
    shownContent = currentContent;
    frameIdx = 0;
  }
  if (changed) {
    lastFrameMs = 0;
  }

  // Eye slot: static fallback only (normal navigation skips EYES slots;
  // the animated eyes are drawn in EYE_MODE)
  if (c.type == EYES) {
    if (changed) {
      drawEyeVariant(c.frameCount);
      Serial.print("Eye: ");
      Serial.println(c.frameCount < EYE_VARIANT_COUNT ? eyeVariants[c.frameCount].name : "?");
    }
    return;
  }

  // Empty slot
  if ((c.type == VIDEO && (c.frames == nullptr || c.frameCount == 0)) ||
      (c.type == IMAGE && c.image == nullptr)) {
    if (changed) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 24);
      display.print("Slot ");
      display.print(currentContent + 1);
      display.print(": not loaded");
      display.display();
    }
    return;
  }

  // Static image: draw once
  if (c.type == IMAGE) {
    if (changed) {
      display.clearDisplay();
      display.drawBitmap(0, 0, c.image, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
      display.display();
    }
    return;
  }

  // Video: advance one frame every videoFrameMs[slot]
  if (changed || millis() - lastFrameMs >= videoFrameMs[currentContent]) {
    lastFrameMs = millis();
    display.clearDisplay();
    display.drawBitmap(0, 0, c.frames[frameIdx], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    frameIdx = (frameIdx + 1) % c.frameCount;
  }
}


// =====================================================
// EYE MODE
// =====================================================

// Mood table for autonomous EYE_MODE: how often each expression is
// picked (relative weight) and how long it stays (random min..max ms).
// Same order as eyeVariants[] / EyeVariantId.
struct EyeMood {
  uint8_t weight;
  uint16_t minMs;
  uint16_t maxMs;
};

const EyeMood eyeMoods[] = {
  { 14, 4000,  8000 },  //  1. Happy        - common
  {  3, 5000,  8000 },  //  2. Sad          - less frequent
  {  4, 3500,  6000 },  //  3. Angry        - occasional
  {  5, 6000, 10000 },  //  4. Sleepy       - occasional (slow cycle)
  {  2, 2500,  4500 },  //  5. Surprised    - rare
  {  3, 4000,  7000 },  //  6. Worried      - less frequent
  {  5, 4000,  7000 },  //  7. Confused     - occasional (curious)
  {  8, 3000,  6000 },  //  8. Excited      - common
  {  5, 5000,  9000 },  //  9. Bored        - occasional
  {  2, 3000,  5000 },  // 10. Scared       - rare
  {  2, 3000,  5000 },  // 11. Furious      - rare
};
static_assert(sizeof(eyeMoods) / sizeof(eyeMoods[0]) == EYE_VARIANT_COUNT, "eyeMoods[] needs one entry per eye variant");

unsigned long eyeShownAtMs = 0;   // when currentEye was selected
unsigned long eyeHoldMs = 0;      // how long currentEye stays

// Weighted random pick that never returns `exclude` (no immediate repeat)
uint8_t pickRandomEye(uint8_t exclude) {
  uint16_t total = 0;
  for (uint8_t i = 0; i < EYE_VARIANT_COUNT; i++) {
    if (i != exclude) total += eyeMoods[i].weight;
  }

  long r = random(total);
  for (uint8_t i = 0; i < EYE_VARIANT_COUNT; i++) {
    if (i == exclude) continue;
    r -= eyeMoods[i].weight;
    if (r < 0) return i;
  }
  return (exclude == EYE_HAPPY) ? EYE_SAD : EYE_HAPPY;  // not reached
}

// Select currentEye: restart its animation and draw the first frame now
void showEye() {
  eyeAnimRestart();
  eyeAnimUpdate(currentEye, true);
  Serial.print("Eye Mode: ");
  Serial.println(eyeVariants[currentEye].name);
}

// Pick the next expression (different from `exclude`) and how long it stays
void autoSelectEye(uint8_t exclude) {
  currentEye = pickRandomEye(exclude);
  const EyeMood &m = eyeMoods[currentEye];
  eyeHoldMs = random(m.minMs, m.maxMs + 1);
  eyeShownAtMs = millis();
  showEye();
}

// =====================================================
// EYE MODE REACTIONS (Happy / Sad)
// =====================================================
// Single PREV/NEXT presses in Eye Mode count up to a random threshold (1..5).
// When it is reached, Happy and Sad "type" a message on a white screen, hold it,
// then return to the same emotion. Other emotions only start a new count.

enum ReactState { REACT_NONE, REACT_TYPING, REACT_SHOWING };
ReactState reactState = REACT_NONE;
uint8_t reactClicks = 0;
uint8_t reactThreshold = 1;
const char *reactText = nullptr;
uint8_t reactSX = 1, reactSY = 1;           // text size (horizontal, vertical) used for the message
uint8_t reactLineStart[4], reactLineLen[4], reactLines = 0;
uint8_t reactTotal = 0, reactShown = 0;     // characters to type / typed so far
unsigned long reactStepMs = 0;
const unsigned long REACT_CHAR_MS = 70;     // typing speed
const unsigned long REACT_HOLD_MS = 1600;   // full message stays this long
const int16_t REACT_MARGIN = 4;             // safe margin around the text

void newReactThreshold() {
  reactClicks = 0;
  reactThreshold = random(1, 6);            // 1..5, ESP32 hardware RNG
}

// Word-wrap `text` for text size `size` inside the margins; false if it doesn't fit
// (bold text is drawn twice, 1 px apart, so each line is 1 px wider)
bool layoutReaction(const char *text, uint8_t sx, uint8_t sy) {
  uint8_t len = strlen(text);
  uint8_t maxChars = (SCREEN_WIDTH - 2 * REACT_MARGIN - 1) / (6 * sx);
  reactLines = 0;
  reactTotal = 0;
  uint8_t i = 0;
  while (i < len) {
    while (i < len && text[i] == ' ') i++;
    if (i >= len) break;
    if (reactLines == 4) return false;
    uint8_t start = i, lineLen = 0, j = i;
    while (j < len) {
      uint8_t k = j;
      while (k < len && text[k] != ' ') k++;
      if (k - start > maxChars) break;
      lineLen = k - start;
      j = k;
      while (j < len && text[j] == ' ') j++;
    }
    if (lineLen == 0) return false;         // a word wider than the screen
    reactLineStart[reactLines] = start;
    reactLineLen[reactLines] = lineLen;
    reactLines++;
    reactTotal += lineLen;
    i = start + lineLen;
  }
  return reactLines > 0 && reactLines * 8 * sy <= SCREEN_HEIGHT - 2 * REACT_MARGIN;
}

// Inverted screen (all white) with the first `shown` characters of the message in black,
// each line centred, bold
void drawReaction(uint8_t shown) {
  display.fillScreen(SSD1306_WHITE);
  display.setTextSize(reactSX, reactSY);
  display.setTextColor(SSD1306_BLACK);
  display.setTextWrap(false);
  int16_t y = (SCREEN_HEIGHT - reactLines * 8 * reactSY) / 2;
  char buf[22];
  for (uint8_t l = 0; l < reactLines && shown > 0; l++) {
    uint8_t n = reactLineLen[l];
    memcpy(buf, reactText + reactLineStart[l], n);
    buf[n] = 0;
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);   // full line width: no shift while typing
    if (shown < n) {
      buf[shown] = 0;
      n = shown;
    }
    int16_t x = (SCREEN_WIDTH - (int16_t)w - 1) / 2;
    display.setCursor(x, y);
    display.print(buf);
    display.setCursor(x + 1, y);                         // second pass, 1 px right: bold
    display.print(buf);
    shown -= n;
    y += 8 * reactSY;
  }
  display.display();
}

// Clear to a white screen and start typing `text` (black) in the largest size that fits
void startReaction(const char *text) {
  reactText = text;
  // largest size that fits, tallest first: 3x3, 2x3, 2x2, 1x2, 1x1 (width x height)
  static const uint8_t SIZES[5][2] = { {3, 3}, {2, 3}, {2, 2}, {1, 2}, {1, 1} };
  reactSX = 1;
  reactSY = 1;
  for (uint8_t k = 0; k < 5; k++) {
    if (layoutReaction(text, SIZES[k][0], SIZES[k][1])) {
      reactSX = SIZES[k][0];
      reactSY = SIZES[k][1];
      break;
    }
  }
  layoutReaction(text, reactSX, reactSY);
  reactShown = 0;
  reactStepMs = millis();
  reactState = REACT_TYPING;
  display.fillScreen(SSD1306_WHITE);        // reaction screen: white, no eyes
  display.display();
  Serial.print("Eye Mode reaction: ");
  Serial.println(text);
}

void cancelReaction() {
  reactState = REACT_NONE;
  newReactThreshold();
}

// Type the next character / hold the message; then resume the same emotion
void updateReaction() {
  unsigned long now = millis();
  if (reactState == REACT_TYPING) {
    if (now - reactStepMs < REACT_CHAR_MS) return;
    reactStepMs = now;
    drawReaction(++reactShown);
    if (reactShown >= reactTotal) reactState = REACT_SHOWING;
  } else if (reactState == REACT_SHOWING) {
    if (now - reactStepMs < REACT_HOLD_MS) return;
    reactState = REACT_NONE;
    newReactThreshold();                    // next reaction needs a new random 1..5 clicks
    eyeShownAtMs = millis();                // same emotion continues, no new random pick
    eyeAnimUpdate(currentEye, true);        // text cleared, eyes drawn again
  }
}

void enterEyeMode() {
  currentMode = EYE_MODE;
  Serial.println("EYE MODE");
  cancelReaction();                   // fresh click count / random threshold
  autoSelectEye(EYE_VARIANT_COUNT);   // nothing to exclude yet
}

// Called every loop() in EYE_MODE: switch expression when its time is up,
// otherwise draw the next animation frame (every EYE_FRAME_MS)
void updateEyeAnimation() {
  if (reactState != REACT_NONE) {    // reaction text screen: no eyes, no mood change
    updateReaction();
    return;
  }
  if (millis() - eyeShownAtMs >= eyeHoldMs) {
    autoSelectEye(currentEye);
    return;
  }
  eyeAnimUpdate(currentEye, false);
}

void exitEyeMode() {
  currentMode = NORMAL_MODE;
  cancelReaction();            // drop any reaction in progress
  contentNeedsRedraw = true;   // redraw the content that was showing
  Serial.println("NORMAL MODE");
}


// =====================================================
// BUTTON HANDLING
// =====================================================

void updateButton(DebouncedButton &b) {
  unsigned long now = millis();
  bool raw = (digitalRead(b.pin) == LOW);

  if (raw != b.rawDown) {
    b.rawDown = raw;
    b.rawChangedAt = now;
  }

  if (b.down != b.rawDown && now - b.rawChangedAt >= stableTime) {
    b.down = b.rawDown;
    if (b.down) {
      b.pending = true;
      b.pressedAt = now;
    }
  }
}

// A single press fires once the combo window has passed, or on an
// earlier release. Until then it may still become PREV+NEXT.
bool singlePressReady(DebouncedButton &b) {
  if (!b.pending) return false;
  if (b.down && millis() - b.pressedAt < comboWindow) return false;
  b.pending = false;
  return true;
}

void handleNormalModeButtons(bool prev, bool next) {
  if (prev) previousContent();
  else if (next) nextContent();
}

// EYE_MODE: a single PREV or NEXT press counts toward the random 1..5 threshold;
// it never changes the emotion (PREV+NEXT together still exits Eye Mode)
void handleEyeModePress() {
  if (reactState != REACT_NONE) return;     // reaction screen showing: ignore presses
  reactClicks++;
  if (reactClicks < reactThreshold) return;
  if (currentEye == EYE_HAPPY) startReaction("I lovee youhhh! Mwahhh mwahh mwahhh");
  else if (currentEye == EYE_SAD) startReaction("I miss youhh!");
  else newReactThreshold();                 // other emotions: no text, start a new count
}

void handleButtons() {
  updateButton(btnPrev);
  updateButton(btnNext);

  // PREV+NEXT already handled: ignore everything until both are released
  if (comboLatched) {
    if (!btnPrev.down && !btnNext.down) comboLatched = false;
    btnPrev.pending = false;
    btnNext.pending = false;
    return;
  }

  // PREV+NEXT together: has priority over single presses, fires once
  if (btnPrev.down && btnNext.down) {
    comboLatched = true;
    btnPrev.pending = false;
    btnNext.pending = false;
    lastButtonTime = millis();
    if (currentMode == NORMAL_MODE) enterEyeMode();
    else exitEyeMode();
    return;
  }

  bool prev = singlePressReady(btnPrev);
  bool next = singlePressReady(btnNext);
  if (!prev && !next) return;

  if (millis() - lastButtonTime <= debounceTime) return;
  lastButtonTime = millis();

  if (currentMode == NORMAL_MODE) handleNormalModeButtons(prev, next);
  else handleEyeModePress();             // EYE_MODE: reactions only
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  handleButtons();

  if (currentMode == NORMAL_MODE) {
    playContent();
  } else {
    updateEyeAnimation();
  }
}