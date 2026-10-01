// =====================================================
// EYE VARIANTS — curved OLED robot face emotions for SSD1306 128x64
//
// Style follows the OLED Animation Maker reference character: a
// compact face in the middle of the screen with plenty of black
// space, small curved "capsule" eyes, curved eyelids, small curved
// lower pieces and a small central mouth.
//
// One shape does almost everything: rfEye() draws a superellipse
// (between an oval and a rounded rectangle: curved, slightly
// flattened top/bottom, short straight sides, never a perfect
// circle) column by column, so it can be
//   - stretched / squashed      (rx, ry)
//   - tilted                    (shear)
//   - cut by curved upper/lower lids (top*/bot* depth + curve)
// A bottom lid that bulges upward turns the eye into a ∩ eyelid,
// a top lid that bulges downward makes a ∪ smile or lower piece,
// deep lids make thin slits. Emotions deform this same geometry;
// the animations move/close/stretch it.
//
// Interface used by v3.ino:
//   EyeVariantId / EYE_VARIANT_COUNT / eyeVariants[].name
//   eyeAnimRestart(), eyeAnimUpdate(id, force), drawEyeVariant(id)
// Uses the global `display` from v3.ino (include after it).
//
// Integer-only math, so the docs/ previews are rendered from the
// same values on a PC.
// =====================================================

#ifndef EYE_VARIANTS_H
#define EYE_VARIANTS_H

#include <Arduino.h>

// ---- Emotion IDs ----
enum EyeVariantId : uint8_t {
  EYE_HAPPY = 0,        //  1. Happy
  EYE_SAD,          //  2. Sad
  EYE_ANGRY,        //  3. Angry
  EYE_SLEEPY,       //  4. Sleepy
  EYE_SURPRISED,    //  5. Surprised
  EYE_WORRIED,      //  6. Worried
  EYE_CONFUSED,     //  7. Confused
  EYE_EXCITED,      //  8. Excited
  EYE_BORED,        //  9. Bored
  EYE_SCARED,       // 10. Scared
  EYE_FURIOUS,      // 11. Furious
  EYE_VARIANT_COUNT
};

struct EyeVariant {
  const char *name;
  const char *motion;   // what the animation does (also listed in README)
};

static const EyeVariant eyeVariants[] = {
  { "Happy",      "Eyes curve into ∩ eyelids over small curved lower pieces, with a bean smile; gentle bounce and mouth pulse" },
  { "Sad",        "Eyes tilt and droop outward over curved lower pieces; slow sinking, a small tear, slow blinks; frown" },
  { "Angry",      "Eyes tilt hard with the inner edges cut down toward the centre; they creep inward and tremble; small frown" },
  { "Sleepy",     "Eyes become thin curved eyelids that slowly close, stay shut while a 'z' floats up and a tiny mouth snores, then reopen" },
  { "Surprised",  "Eyes stretch taller with an overshoot every 2.4 s; small open mouth grows with them" },
  { "Worried",    "Slightly taller, uneven eyes (left and right differ) that quiver and glance around quickly; wobbly mouth" },
  { "Confused",   "One taller eye and one small tilted eye whose sizes and angle keep changing; tilted mouth; floating '?'" },
  { "Excited",    "Wider eyes bouncing every 0.42 s and pulsing wider; curved grin opening and closing; sparkles" },
  { "Bored",      "Very flat half-lidded eyes drifting slowly sideways; long slow blinks; small flat mouth" },
  { "Scared",     "Small eyes set wider apart that tremble, pulse, dart around and blink fast; tiny nervous mouth" },
  { "Furious",    "Strongly angled eyes with the inner edges cut down hard toward the centre; hard shaking, anger mark, jagged mouth" },
};
static_assert(sizeof(eyeVariants) / sizeof(eyeVariants[0]) == EYE_VARIANT_COUNT, "eyeVariants[] needs one entry per EyeVariantId");

// ---- Face layout (pixels): compact, centred, lots of black space ----
static const int16_t RF_LX = 41;   // left eye centre x
static const int16_t RF_RX = 87;   // right eye centre x
static const int16_t RF_EY = 25;   // eye centre y
static const int16_t RF_LY = 35;   // lower eye pieces, centre y
static const int16_t RF_MX = 64;   // mouth centre x
static const int16_t RF_MY = 40;   // mouth centre y

// Half-height (per mille) of the eye shape at horizontal position i/32 from the
// centre: superellipse |x|^2.6 + |y|^2.6 = 1 (curved, slightly flat, not a circle)
static const int16_t RF_PROF[33] = {
  1000, 1000, 1000, 999, 998, 997, 995, 993, 989, 986, 981, 976, 969, 962, 953, 944,
  933, 921, 907, 892, 874, 855, 833, 809, 781, 750, 714, 673, 624, 564, 488, 377, 0
};

// ---- Integer math helpers ----

// sin() for a quarter wave in 65 steps, scaled to 0..1000
static const int16_t RF_SIN_Q[65] = {
  0, 25, 49, 74, 98, 122, 147, 171, 195, 219, 243, 267, 290, 314, 337, 360,
  383, 405, 428, 450, 471, 493, 514, 535, 556, 576, 596, 615, 634, 653, 672, 690,
  707, 724, 741, 757, 773, 788, 803, 818, 831, 845, 858, 870, 882, 893, 904, 914,
  924, 933, 942, 950, 957, 964, 970, 976, 981, 985, 989, 992, 995, 997, 999, 1000, 1000
};

// One full sine cycle per 256 steps of p, result -1000..1000
static int16_t rfSin(uint32_t p) {
  p &= 255;
  uint8_t i = p & 63;
  switch (p >> 6) {
    case 0:  return RF_SIN_Q[i];
    case 1:  return RF_SIN_Q[64 - i];
    case 2:  return -RF_SIN_Q[i];
    default: return -RF_SIN_Q[64 - i];
  }
}

// Sine wave with the given period in ms, -1000..1000
static int16_t rfWave(uint32_t t, uint32_t period) {
  return rfSin((t % period) * 256 / period);
}

static int16_t rfAbs(int16_t v) {
  return v < 0 ? -v : v;
}

// Division rounded to nearest (b > 0)
static int32_t rfRdiv(int32_t a, int32_t b) {
  return a >= 0 ? (a + b / 2) / b : (a - b / 2) / b;
}

static uint32_t rfHash(uint32_t n) {
  n ^= n >> 16;
  n *= 0x7feb352dUL;
  n ^= n >> 15;
  n *= 0x846ca68bUL;
  n ^= n >> 16;
  return n;
}

// Pseudo-random offset -range..range that changes every stepMs
static int16_t rfJitter(uint32_t t, uint16_t stepMs, uint8_t range, uint8_t seed) {
  return (int16_t)(rfHash(t / stepMs * 7 + seed) % (2 * range + 1)) - range;
}

static uint16_t rfIsqrt(uint32_t v) {
  uint32_t res = 0, bit = 1UL << 30;
  while (bit > v) bit >>= 2;
  while (bit) {
    if (v >= res + bit) {
      v -= res + bit;
      res = (res >> 1) + bit;
    } else {
      res >>= 1;
    }
    bit >>= 2;
  }
  return res;
}

// One blink: 0 -> 1000 (closed) -> 0 over dur ms, starting at `start`
static int16_t rfBlinkShape(uint32_t local, uint32_t start, uint16_t dur) {
  if (local < start || local >= start + dur) return 0;
  uint32_t e = local - start, half = dur / 2;
  if (e < half) return e * 1000 / half;
  return (dur - e) * 1000 / (dur - half);
}

// Natural blinking: one blink per period at a random moment, sometimes a double blink.
// Returns closure 0..1000. Needs period > 4 * (2 * dur + 90) for the double blink to fit.
static int16_t rfBlink(uint32_t t, uint16_t period, uint16_t dur, uint8_t seed) {
  uint32_t n = t / period;
  uint32_t h = rfHash(n * 31 + seed);
  uint32_t start = h % (period / 2) + period / 4;
  uint32_t local = t % period;
  int16_t c = rfBlinkShape(local, start, dur);
  if ((h >> 8) % 4 == 0) {
    int16_t c2 = rfBlinkShape(local, start + dur + 90, dur);
    if (c2 > c) c = c2;
  }
  return c;
}

// Steps through pts[], holding each for `hold` ms and easing to it over `move` ms
static int16_t rfPath(uint32_t t, const int8_t *pts, uint8_t n, uint16_t hold, uint16_t move) {
  uint32_t seg = t / hold, in = t % hold;
  int16_t to = pts[seg % n], from = pts[(seg + n - 1) % n];
  if (seg == 0 || in >= move) return to;
  int32_t e = (1000 - rfSin(in * 128 / move + 64)) / 2;   // ease in-out 0..1000
  return from + (int32_t)(to - from) * e / 1000;
}

// ---- Look paths (whole eye shapes slide by these x offsets) ----
static const int8_t RF_LOOK_WORRIED[] = { 0, -4, 0, 4, -2 };
static const int8_t RF_LOOK_BORED[]   = { 0, 5, 5, -4 };
static const int8_t RF_LOOK_SCARED[]  = { 0, -6, 4, -5, 6, -2 };

// ---- Chunky icons (1 bit per pixel, MSB first, 2 px strokes) ----
static const uint8_t RF_BMP_QUESTION[] PROGMEM = {   // 8x10 '?'
  0x3C, 0x7E, 0x66, 0x06, 0x0C, 0x18, 0x18, 0x00, 0x18, 0x18
};
static const uint8_t RF_BMP_Z[] PROGMEM = {          // 7x7 'z'
  0xFE, 0xFE, 0x0C, 0x18, 0x30, 0xFE, 0xFE
};
static const uint8_t RF_BMP_SPARK[] PROGMEM = {      // 7x7 sparkle
  0x10, 0x10, 0x38, 0xFE, 0x38, 0x10, 0x10
};
static const uint8_t RF_BMP_ANGER[] PROGMEM = {      // 9x8 anger mark
  0x63, 0x00, 0xE3, 0x80, 0xC1, 0x80, 0x00, 0x00,
  0x00, 0x00, 0xC1, 0x80, 0xE3, 0x80, 0x63, 0x00
};

// ---- Curved OLED geometry ----

// Whole face is scaled by RF_SCALE percent about (64, RF_SCY): coordinates and sizes in
// the emotion code are written for the base size and scaled here, in the primitives.
static const int16_t RF_SCALE = 140;       // whole face 40% bigger than the base design
static const int16_t RF_EYE_NARROW = 72;   // eye shapes 28% narrower (width only), so eyes stay narrow
static const int16_t RF_SCY = 30;
static int16_t rfS(int16_t v) { return rfRdiv((int32_t)v * RF_SCALE, 100); }
static int16_t rfSX(int16_t x) { return 64 + rfS(x - 64); }
static int16_t rfSY(int16_t y) { return RF_SCY + rfS(y - RF_SCY); }
// Icon at a scaled position, kept at least 2 px inside the screen
static void rfBitmap(int16_t x, int16_t y, const uint8_t *bmp, int16_t w, int16_t h) {
  x = rfSX(x);
  y = rfSY(y);
  if (x < 2) x = 2;
  if (x > SCREEN_WIDTH - 2 - w) x = SCREEN_WIDTH - 2 - w;
  if (y < 2) y = 2;
  if (y > SCREEN_HEIGHT - 2 - h) y = SCREEN_HEIGHT - 2 - h;
  display.drawBitmap(x, y, bmp, w, h, SSD1306_WHITE);
}

static int16_t rfEyeX(int8_t side) {   // side: -1 = left eye, +1 = right eye
  return side < 0 ? RF_LX : RF_RX;
}

// The one eye shape. Superellipse of half-size rx x ry centred on (cx, cy), drawn
// column by column. side (-1/+1) mirrors it so "outer" means away from the face centre.
// shear: outer end lower by shear/16 px per px (negative = outer end higher).
// Upper lid: cuts topIn/topOut px down at the inner/outer edge, plus topCurve px extra in
// the middle (positive = cut bulges down, e.g. a ∪ smile). Lower lid: same from below
// (positive botCurve bulges up, e.g. a ∩ eyelid).
static void rfEye(int16_t cx, int16_t cy, int16_t rx, int16_t ry, int8_t side, int16_t shear,
                  int16_t topIn, int16_t topOut, int16_t topCurve,
                  int16_t botIn, int16_t botOut, int16_t botCurve) {
  if (rx < 1 || ry < 1) return;
  cx = rfSX(cx); cy = rfSY(cy); ry = rfS(ry);
  rx = rfRdiv((int32_t)rfS(rx) * RF_EYE_NARROW, 100);
  if (rx < 1) rx = 1;
  topIn = rfS(topIn); topOut = rfS(topOut); topCurve = rfS(topCurve);
  botIn = rfS(botIn); botOut = rfS(botOut); botCurve = rfS(botCurve);
  for (int16_t x = -rx; x <= rx; x++) {
    int16_t ax = x < 0 ? -x : x;
    int16_t hy = rfRdiv((int32_t)ry * RF_PROF[(int32_t)ax * 64 / (2 * rx + 1)], 1000);
    int16_t xo = x * side;                                     // + = outer side
    int16_t yc = cy + rfRdiv((int32_t)xo * shear, 16);
    int32_t num = xo + rx;                                     // 0 (inner) .. 2rx (outer)
    int32_t uu = 1000 - (int32_t)x * x * 1000 / ((int32_t)rx * rx);   // 1 - u^2, per mille
    int16_t top = yc - ry + rfRdiv(topIn * (2 * rx - num) + topOut * num, 2 * rx) + rfRdiv(topCurve * uu, 1000);
    int16_t bot = yc + ry - rfRdiv(botIn * (2 * rx - num) + botOut * num, 2 * rx) - rfRdiv(botCurve * uu, 1000);
    if (top < yc - hy) top = yc - hy;
    if (bot > yc + hy) bot = yc + hy;
    if (bot >= top) display.drawFastVLine(cx + x, top, bot - top + 1, SSD1306_WHITE);
  }
}

// Plain (uncut) eye shape
static void rfPlain(int16_t cx, int16_t cy, int16_t rx, int16_t ry, int8_t side, int16_t shear) {
  rfEye(cx, cy, rx, ry, side, shear, 0, 0, 0, 0, 0, 0);
}

// Blink: both lids close toward the middle (c = 0..1000), leaving a thin curved line
static void rfLids(int16_t cx, int16_t cy, int16_t rx, int16_t ry, int8_t side, int16_t shear, int16_t c) {
  int16_t k = (int32_t)(ry - 1) * c / 1000;
  rfEye(cx, cy, rx, ry, side, shear, k, k, 0, k, k, 0);
}

// Thick stroke with round ends (thickness 2r+1), screen coordinates
static void rfStrokeRaw(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t r) {
  display.fillCircle(x0, y0, r, SSD1306_WHITE);
  display.fillCircle(x1, y1, r, SSD1306_WHITE);
  int32_t dx = x1 - x0, dy = y1 - y0;
  int16_t len = rfIsqrt(dx * dx + dy * dy);
  if (len == 0) return;
  int16_t px = rfRdiv(-dy * r, len), py = rfRdiv(dx * r, len);
  display.fillTriangle(x0 + px, y0 + py, x1 + px, y1 + py, x1 - px, y1 - py, SSD1306_WHITE);
  display.fillTriangle(x0 + px, y0 + py, x1 - px, y1 - py, x0 - px, y0 - py, SSD1306_WHITE);
}

// Thick stroke in face coordinates (scaled)
static void rfStroke(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t r) {
  rfStrokeRaw(rfSX(x0), rfSY(y0), rfSX(x1), rfSY(y1), rfS(r));
}

// Smooth thick arc along an ellipse from angle a0 to a1 (256 = full turn, 0 = right,
// 64 = top), built from 6 strokes of radius r. 10..118 = ∩, 138..246 = ∪.
static void rfArc(int16_t cx, int16_t cy, int16_t rx, int16_t ry, int16_t a0, int16_t a1, int16_t r) {
  int16_t px = 0, py = 0;
  for (uint8_t i = 0; i <= 6; i++) {
    int16_t a = a0 + (int32_t)(a1 - a0) * i / 6;
    int16_t x = cx + rfRdiv((int32_t)rx * rfSin(a + 64), 1000);
    int16_t y = cy - rfRdiv((int32_t)ry * rfSin(a), 1000);
    if (i > 0) rfStroke(px, py, x, y, r);
    px = x;
    py = y;
  }
}

// Small wobbly zig-zag of strokes: 4 segments, flip swaps the phase
static void rfZig(int16_t cx, int16_t cy, int16_t half, int16_t amp, uint8_t flip, int16_t r) {
  int16_t step = half / 2;
  int16_t x = cx - half;
  int16_t s = flip ? amp : -amp;
  for (uint8_t i = 0; i < 4; i++) {
    rfStroke(x, cy + s, x + step, cy - s, r);
    x += step;
    s = -s;
  }
}

// ---- Emotions ----

static void rfDrawEmotion(uint8_t id, uint32_t t) {
  switch (id) {

    case EYE_HAPPY: {
      int16_t b = rfAbs(rfWave(t, 900));              // bounce every 450 ms
      int16_t dy = -(b * 2 / 1000);
      int16_t sq = (1000 - b) / 500;                  // squash when landing
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s);
        rfEye(ex, RF_EY + 1 + dy, 8, 8, s, 0, 0, 0, 0, 0, 0, 11 - sq);  // ∩ eyelid
        rfEye(ex, RF_LY + dy, 5, 2, s, -3, 0, 0, 0, 0, 0, 0);            // lower piece
      }
      rfEye(RF_MX, RF_MY - 1 + dy, 7 + (rfWave(t, 450) + 1000) / 1000, 4, 1, 3, 0, 0, 0, 0, 0, 0);   // bean mouth
      break;
    }

    case EYE_SAD: {
      int16_t dy = 1 + rfWave(t, 4200) * 2 / 1000;    // slow sink
      int16_t c = rfBlink(t, 3600, 300, 3);
      int16_t k = 5 * c / 1000;
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s);
        rfEye(ex, RF_EY + 1 + dy, 7, 6, s, 6, k, 4 + k, 1, k, k, 0);     // drooping outward
        rfPlain(ex, RF_LY + 2 + dy, 5, 2, s, 4);                         // tilted lower piece
      }
      int32_t ph = t % 3400;
      if (ph < 1100) {                                // small tear
        int16_t ty = RF_LY + 5 + dy + ph * 7 / 1100;    // short fall: stays on screen
        display.fillCircle(rfSX(RF_RX + 9), rfSY(ty), 1, SSD1306_WHITE);
        display.fillTriangle(rfSX(RF_RX + 8), rfSY(ty - 1), rfSX(RF_RX + 10), rfSY(ty - 1), rfSX(RF_RX + 9), rfSY(ty - 4), SSD1306_WHITE);
      }
      rfArc(RF_MX, RF_MY + 3, 5, 3, 10, 118, 1);                        // frown
      break;
    }

    case EYE_ANGRY: {
      int16_t in = (rfWave(t, 1400) + 1000) * 2 / 2000;   // creep inward 0..2 px
      int16_t j = rfJitter(t, 70, 1, 5);
      int16_t k = 5 * rfBlink(t, 4200, 120, 5) / 1000;
      for (int8_t s = -1; s <= 1; s += 2)
        rfEye(rfEyeX(s) - s * in, RF_EY + 2 + j, 8, 7, s, -6, 6 + k, k, 0, k, k, 0);
      rfArc(RF_MX, RF_MY + 3, 5, 2, 10, 118, 1);
      break;
    }

    case EYE_SLEEPY: {
      int32_t ph = t % 5200;
      int32_t k;                                      // closure 0..1000
      if (ph < 2600) k = 400 + 300 * ph / 2600;              // drooping
      else if (ph < 3200) k = 700 + 300 * (ph - 2600) / 600;  // closing
      else if (ph < 4300) k = 1000;                           // asleep
      else k = 1000 - 600 * (ph - 4300) / 900;                // reopening
      int16_t lid = 3 + 6 * k / 1000;
      for (int8_t s = -1; s <= 1; s += 2)
        rfEye(rfEyeX(s), RF_EY + 2 + 2 * k / 1000, 8, 5, s, 0, lid, lid, -2, 0, 0, 0);   // thin curved lid
      if (ph >= 3000) {
        int32_t z = ph - 3000;
        rfBitmap(100 + z / 500, 14 - z * 10 / 2200, RF_BMP_Z, 7, 7);
      }
      if (ph >= 3200 && ph < 4300) {                  // snore
        int16_t q = 1 + (t / 400) % 2;
        rfPlain(RF_MX, RF_MY, q + 1, q, 1, 0);
      }
      break;
    }

    case EYE_SURPRISED: {
      int32_t ph = t % 2400;
      int16_t p;
      if (ph < 110) p = 6 * ph / 110;                 // stretch open
      else if (ph < 260) p = 6 - (ph - 110) / 150;    // settle
      else p = 5;
      int16_t c = rfBlink(t, 5000, 140, 4);
      for (int8_t s = -1; s <= 1; s += 2) rfLids(rfEyeX(s), RF_EY + 1, 6 + p / 3, 6 + p, s, 0, c);
      rfPlain(RF_MX, RF_MY + 3, 2 + p / 4, 3 + p / 3, 1, 0);              // small open mouth
      break;
    }

    case EYE_WORRIED: {
      int16_t lx = rfPath(t, RF_LOOK_WORRIED, 5, 700, 120);
      int16_t k = 7 * rfBlink(t, 2700, 150, 7) / 1000;
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t q = s < 0 ? rfWave(t, 330) / 700 : rfWave(t, 410) / 700;   // independent quiver
        rfEye(rfEyeX(s) + lx, RF_EY + q, 6, s < 0 ? 8 : 7, s, s < 0 ? 3 : 2, k, 3 + k, 0, k, k, 0);
      }
      rfZig(RF_MX, RF_MY + 1, 6, 1, (t / 300) & 1, 1);
      break;
    }

    case EYE_CONFUSED: {
      int16_t s = rfWave(t, 3600);
      int16_t k = 7 * rfBlink(t, 3900, 160, 9) / 1000;
      rfEye(RF_LX, RF_EY - s * 2 / 1000, 7, 8 + s * 2 / 1000, -1, 0, k, k, 0, k, k, 0);   // taller eye
      rfPlain(RF_RX, RF_EY + 3, 6, 4 - s / 1000, 1, s * 5 / 1000);                       // small tilted eye
      rfBitmap(108, 4 + rfAbs(rfWave(t, 1000)) * 3 / 1000, RF_BMP_QUESTION, 8, 10);
      rfPlain(RF_MX, RF_MY + 1, 5, 2, 1, s * 5 / 1000);                                   // tilted mouth
      break;
    }

    case EYE_EXCITED: {
      int16_t b = rfAbs(rfWave(t, 840));              // bounce every 420 ms
      int16_t dy = -(b * 2 / 1000);
      int16_t wv = (rfWave(t, 700) + 1000) * 2 / 2000;
      for (int8_t s = -1; s <= 1; s += 2) rfPlain(rfEyeX(s), RF_EY + dy, 9 + wv, 7 + wv / 2, s, 0);
      int16_t gy = 4 + (rfWave(t, 500) + 1000) / 1000;
      rfEye(RF_MX, RF_MY - 2 + dy, 7, gy, 1, 0, gy, gy, 0, 0, 0, 0);   // D grin opening/closing
      if ((t / 180) % 2 == 0) {
        rfBitmap(18, 8, RF_BMP_SPARK, 7, 7);
        rfBitmap(104, 40, RF_BMP_SPARK, 7, 7);
      } else {
        rfBitmap(17, 40, RF_BMP_SPARK, 7, 7);
        rfBitmap(103, 8, RF_BMP_SPARK, 7, 7);
      }
      break;
    }

    case EYE_BORED: {
      int16_t lx = rfPath(t, RF_LOOK_BORED, 4, 2600, 1400);
      int16_t k = 3 * rfBlink(t, 4800, 500, 11) / 1000;
      for (int8_t s = -1; s <= 1; s += 2)
        rfEye(rfEyeX(s) + lx, RF_EY + 3, 9, 4, s, 0, 3 + k, 3 + k, -1, k, k, 0);   // flat, half-lidded
      rfPlain(RF_MX + lx / 2, RF_MY, 5, 1, 1, 0);
      break;
    }

    case EYE_SCARED: {
      int16_t lx = rfPath(t, RF_LOOK_SCARED, 6, 650, 60);
      int16_t jx = rfJitter(t, 50, 1, 13), jy = rfJitter(t, 50, 1, 17);
      int16_t p = (rfWave(t, 400) + 1000) / 1000;     // quick pulse
      int16_t c = rfBlink(t, 2200, 90, 13);
      for (int8_t s = -1; s <= 1; s += 2) rfLids(rfEyeX(s) + s * 4 + lx + jx, RF_EY + 1 + jy, 4 + p, 6 + p, s, 0, c);
      rfZig(RF_MX + jx, RF_MY, 4, 1, (t / 90) & 1, 1);
      break;
    }

    case EYE_FURIOUS: {
      int16_t sx = rfJitter(t, 40, 2, 19), sy = rfJitter(t, 40, 1, 23);
      int16_t fl = rfWave(t, 600) > 0 ? 1 : 0;        // flare
      for (int8_t s = -1; s <= 1; s += 2)
        rfEye(rfEyeX(s) + sx, RF_EY + 2 + sy, 9, 8 + fl, s, -9, 9, 0, 0, 0, 3, 0);
      if ((t / 250) % 2 == 0) rfBitmap(108, 4, RF_BMP_ANGER, 9, 8);
      rfZig(RF_MX + sx, RF_MY + 1 + sy, 6, 2, (t / 120) & 1, 1);
      break;
    }
  }
}

// ---- Frame drawing / timing ----

static void drawEyeFrame(uint8_t id, unsigned long elapsed) {
  if (id >= EYE_VARIANT_COUNT) return;
  display.clearDisplay();
  rfDrawEmotion(id, elapsed);
  display.display();
}

// Static picture of an emotion (a representative moment of its animation)
static void drawEyeVariant(uint8_t id) {
  drawEyeFrame(id, 500);
}

static const unsigned long EYE_FRAME_MS = 33;   // ~30 fps
static unsigned long eyeAnimStartMs = 0;
static unsigned long eyeLastFrameMs = 0;

// Restart the selected emotion's animation from its first frame
static void eyeAnimRestart() {
  eyeAnimStartMs = millis();
  eyeLastFrameMs = eyeAnimStartMs;
}

// Draw the next frame when it is due (or now, if force)
static void eyeAnimUpdate(uint8_t id, bool force) {
  unsigned long now = millis();
  if (!force && now - eyeLastFrameMs < EYE_FRAME_MS) return;
  eyeLastFrameMs = now;
  drawEyeFrame(id, now - eyeAnimStartMs);
}

#endif
