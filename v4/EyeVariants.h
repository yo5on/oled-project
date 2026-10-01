// =====================================================
// EYE VARIANTS — compact pixel-art robot face for SSD1306 128x64
//
// Each emotion gets its own eye silhouette: elongated OLED-pixel strokes,
// eyelid arcs, asymmetric cylinders, flattened slits, or sharp wedges.
// Shapes are kept in the central face area with generous black space.
// Everything is drawn live from small integer-friendly primitives:
//   rfCapsule()  thick rounded bar between two points
//   rfArc()      thick arc made of capsules (∩, ∪, drooping, tilted)
//   rfBox()      rounded block, rfOval() filled oval, rfHeart()
// Emotion cases use different primitives and proportions; animation
// timing, mood selection, and mode control remain in their existing code.
//
// Interface used by v4.ino:
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
  EYE_NEUTRAL = 0,  //  1. Neutral
  EYE_HAPPY,        //  2. Happy
  EYE_SAD,          //  3. Sad
  EYE_ANGRY,        //  4. Angry
  EYE_SLEEPY,       //  5. Sleepy
  EYE_SURPRISED,    //  6. Surprised
  EYE_WORRIED,      //  7. Worried
  EYE_CONFUSED,     //  8. Confused
  EYE_EXCITED,      //  9. Excited
  EYE_SUSPICIOUS,   // 10. Suspicious
  EYE_LOVE,         // 11. Love/Cute
  EYE_BORED,        // 12. Bored
  EYE_SCARED,       // 13. Scared
  EYE_FURIOUS,      // 14. Furious
  EYE_VARIANT_COUNT
};

struct EyeVariant {
  const char *name;
  const char *motion;   // what the animation does (also listed in README)
};

static const EyeVariant eyeVariants[] = {
  { "Neutral",    "Two long, thin rounded LED bars breathe, glance subtly, and blink into slits above a small curved mouth" },
  { "Happy",      "Upper arch eyelids, short lower eye pieces, and a chunky U-shaped mouth bounce gently" },
  { "Sad",        "Drooping diagonal eyes, small lower curves, a falling pixel tear, and a frown" },
  { "Angry",      "Thick centre-pointing wedges tighten and tremble above a short angry mouth" },
  { "Sleepy",     "Separate horizontal eyelids sink over thin lower arcs, close, pause, then lift" },
  { "Surprised",  "Tall rounded-cylinder eyes pop wider with overshoot; a tiny open mouth pulses" },
  { "Worried",    "Unequal tall rounded eyes and tilted brows bob independently over a nervous mouth" },
  { "Confused",   "One larger block eye faces a tilted narrow slit; a tilted mouth and bobbing '?' appear" },
  { "Excited",    "Wide compact eye bars bounce and widen with an opening mouth and small sparkles" },
  { "Suspicious", "One long narrow horizontal eye and one short open pill shift asymmetrically beside a smirk" },
  { "Love/Cute",  "Short rounded eyes squint into arcs above lower pieces while a tiny heart beats twice" },
  { "Bored",      "Extremely flat horizontal eyelids drift slowly and blink over a small flat mouth" },
  { "Scared",     "Small rounded vertical capsules tremble and dart with ample empty space and a tiny nervous mouth" },
  { "Furious",    "Heavy sharp centre-facing wedges shake hard above a tight jagged mouth" },
};
static_assert(sizeof(eyeVariants) / sizeof(eyeVariants[0]) == EYE_VARIANT_COUNT, "eyeVariants[] needs one entry per EyeVariantId");

// ---- Face layout (pixels) ----
static const int16_t RF_LX = 39;   // left eye centre x
static const int16_t RF_RX = 89;   // right eye centre x
static const int16_t RF_UY = 27;   // main eye pieces, centre y
static const int16_t RF_LY = 36;   // lower lids / eye pieces, centre y
static const int16_t RF_MX = 64;   // central element (mouth) x
static const int16_t RF_MY = 44;   // central element (mouth) y

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

// Size after closing by c (0..1000) down to `minSize`
static int16_t rfShrinkTo(int16_t size, int16_t c, int16_t minSize) {
  return size - (int32_t)(size - minSize) * c / 1000;
}

// ---- Look paths (whole eye shapes slide by these x offsets) ----
static const int8_t RF_LOOK_NEUTRAL[] = { 0, -4, 0, 4 };
static const int8_t RF_LOOK_WORRIED[] = { 0, -4, 0, 4, -2 };
static const int8_t RF_LOOK_SHIFTY[]  = { -6, 6 };
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

// ---- Chunky primitives ----

static int16_t rfEyeX(int8_t side) {   // side: -1 = left eye, +1 = right eye
  return side < 0 ? RF_LX : RF_RX;
}

// Filled ellipse, only rows with yMin <= y <= yMax. Uses (r + 0.5)^2 ~ r*r + r so
// the top/bottom/sides are short flat runs instead of single-pixel points.
static void rfEllipseRows(int16_t cx, int16_t cy, int16_t rx, int16_t ry, int16_t yMin, int16_t yMax) {
  if (rx < 0 || ry <= 0) return;
  int32_t rr = (int32_t)ry * ry + ry;
  int32_t xx = (int32_t)rx * rx + rx;
  for (int16_t dy = -ry; dy <= ry; dy++) {
    int16_t y = cy + dy;
    if (y < yMin || y > yMax) continue;
    int16_t dx = rfIsqrt((uint32_t)(xx * (rr - (int32_t)dy * dy) / rr));
    display.drawFastHLine(cx - dx, y, 2 * dx + 1, SSD1306_WHITE);
  }
}

static void rfOval(int16_t cx, int16_t cy, int16_t rx, int16_t ry) {
  rfEllipseRows(cx, cy, rx, ry, cy - ry, cy + ry);
}

// Lower half of an oval: open / smiling mouth
static void rfHalfOval(int16_t cx, int16_t cy, int16_t rx, int16_t ry) {
  rfEllipseRows(cx, cy, rx, ry, cy, cy + ry);
}

// Rounded block centred on (cx, cy)
static void rfBox(int16_t cx, int16_t cy, int16_t w, int16_t h, int16_t r) {
  if (w < 1 || h < 1) return;
  display.fillRoundRect(cx - w / 2, cy - h / 2, w, h, r, SSD1306_WHITE);
}

// Thick bar with round ends from (x0, y0) to (x1, y1); thickness 2r+1
static void rfCapsule(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t r) {
  display.fillCircle(x0, y0, r, SSD1306_WHITE);
  display.fillCircle(x1, y1, r, SSD1306_WHITE);
  int32_t dx = x1 - x0, dy = y1 - y0;
  int16_t len = rfIsqrt(dx * dx + dy * dy);
  if (len == 0) return;
  int16_t px = rfRdiv(-dy * r, len), py = rfRdiv(dx * r, len);
  display.fillTriangle(x0 + px, y0 + py, x1 + px, y1 + py, x1 - px, y1 - py, SSD1306_WHITE);
  display.fillTriangle(x0 + px, y0 + py, x1 - px, y1 - py, x0 - px, y0 - py, SSD1306_WHITE);
}

// Eye bar: capsule centred on (cx, cy), `half` px each side; dIn/dOut lift the
// inner/outer end (side -1 = left eye, +1 = right eye, outer = away from the face centre)
static void rfBar(int16_t cx, int16_t cy, int16_t half, int16_t r, int8_t side, int16_t dIn, int16_t dOut) {
  rfCapsule(cx - side * half, cy + dIn, cx + side * half, cy + dOut, r);
}

// A short rounded blade with a pointed inner end. `innerDrop` makes the
// centre-facing tip lower than the outer corner for angry expressions.
static void rfWedge(int16_t cx, int16_t cy, int8_t side, int16_t half,
                    int16_t innerDrop, int16_t radius) {
  int16_t outerX = cx + side * half;
  int16_t innerX = cx - side * half;
  int16_t outerY = cy - innerDrop / 2;
  int16_t innerY = cy + innerDrop;
  int16_t baseX = innerX + side * 3;
  int16_t baseY = innerY - 1;
  rfCapsule(outerX, outerY, baseX, baseY, radius);
  display.fillTriangle(baseX, baseY - radius, baseX, baseY + radius,
                       innerX, innerY, SSD1306_WHITE);
}

// Thick arc along an ellipse from angle a0 to a1 (256 = full turn, 0 = right,
// 64 = top), built from 6 capsules of radius r. tilt adds tilt/16 px of y per px of x.
static void rfArc(int16_t cx, int16_t cy, int16_t rx, int16_t ry, int16_t a0, int16_t a1, int16_t r, int16_t tilt) {
  int16_t px = 0, py = 0;
  for (uint8_t i = 0; i <= 6; i++) {
    int16_t a = a0 + (int32_t)(a1 - a0) * i / 6;
    int16_t ox = rfRdiv((int32_t)rx * rfSin(a + 64), 1000);
    int16_t x = cx + ox;
    int16_t y = cy - rfRdiv((int32_t)ry * rfSin(a), 1000) + rfRdiv((int32_t)ox * tilt, 16);
    if (i > 0) rfCapsule(px, py, x, y, r);
    px = x;
    py = y;
  }
}

// Upper eye arc (∩); tilt > 0 droops the outer end
static void rfEyeArc(int16_t cx, int16_t cy, int16_t rx, int16_t ry, int16_t r, int8_t side, int16_t droop) {
  rfArc(cx, cy, rx, ry, 10, 118, r, side * droop);
}

static void rfHeart(int16_t cx, int16_t cy, int16_t r) {
  display.fillCircle(cx - r, cy - r / 2, r, SSD1306_WHITE);
  display.fillCircle(cx + r, cy - r / 2, r, SSD1306_WHITE);
  display.fillTriangle(cx - 2 * r, cy - r / 2 + 1, cx + 2 * r, cy - r / 2 + 1, cx, cy + 3 * r / 2, SSD1306_WHITE);
}

// Chunky zig-zag of capsules: 4 segments, flip swaps the phase
static void rfZig(int16_t cx, int16_t cy, int16_t half, int16_t amp, uint8_t flip, int16_t r) {
  int16_t step = half / 2;
  int16_t x = cx - half;
  int16_t s = flip ? amp : -amp;
  for (uint8_t i = 0; i < 4; i++) {
    rfCapsule(x, cy + s, x + step, cy - s, r);
    x += step;
    s = -s;
  }
}

// ---- Emotions ----

static void rfDrawEmotion(uint8_t id, uint32_t t) {
  switch (id) {

    case EYE_NEUTRAL: {
      int16_t lx = rfPath(t, RF_LOOK_NEUTRAL, 4, 2300, 260);
        int16_t h = rfShrinkTo(6 + rfWave(t, 2600) / 1500, rfBlink(t, 3100, 180, 1), 1);
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s) + lx;
        rfBox(ex, RF_UY, 17, h, 3);                  // narrower rounded OLED bar; height is unchanged
      }
      rfArc(RF_MX + lx / 2, RF_MY + 1, 7, 3, 138, 246, 1, 0);
      break;
    }

    case EYE_HAPPY: {
      int16_t b = rfAbs(rfWave(t, 900));            // bounce every 450 ms
      int16_t dy = -(b * 2 / 1000);
      int16_t ry = 4 - (1000 - b) / 1000;             // compact arch opens on the rise
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s);
        rfEyeArc(ex, RF_UY + 3 + dy, 6, ry, 2, s, 0);
        rfBar(ex, RF_LY + dy, 4, 2, s, 0, -1);        // short lower eye piece
      }
      rfArc(RF_MX, RF_MY, 8, 4, 138, 246, 2, 0);      // chunky smile
      break;
    }

    case EYE_SAD: {
      int16_t dy = rfWave(t, 4200) * 2 / 1000;        // slow droop and lift
      int16_t lid = rfShrinkTo(2, rfBlink(t, 3600, 300, 3), 0);
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s);
        rfBar(ex, RF_UY + dy, 5, 2 - lid, s, -1, 2); // outer corners droop
        rfArc(ex, RF_LY + 1 + dy, 6, 2, 138, 246, 1, 0);
      }
      int32_t ph = t % 3400;
      if (ph < 1100) {                               // chunky tear
        int16_t ty = RF_LY + 4 + dy + ph * 9 / 1100;
        display.fillTriangle(RF_RX + 5, ty - 3, RF_RX + 7, ty,
                             RF_RX + 4, ty + 1, SSD1306_WHITE);
      }
      rfArc(RF_MX, RF_MY + 1, 6, 3, 10, 118, 1, 0);  // small frown
      break;
    }

    case EYE_ANGRY: {
      int16_t in = (rfWave(t, 1400) + 1000) * 2 / 2000;   // creep inward 0..2 px
      int16_t j = rfJitter(t, 70, 1, 5);
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s) - s * in;
        rfWedge(ex, RF_UY + j, s, 8, 4, 1);           // narrower wedge, inner tip points down
      }
      rfBar(RF_MX, RF_MY + 1 + j, 4, 1, 1, 0, 0);    // small angry mouth
      break;
    }

    case EYE_SLEEPY: {
      int32_t ph = t % 5200;
      int32_t k;                                     // closure 0..1000
      if (ph < 2600) k = 400 + 300 * ph / 2600;              // drooping
      else if (ph < 3200) k = 700 + 300 * (ph - 2600) / 600;  // closing
      else if (ph < 4300) k = 1000;                           // asleep
      else k = 1000 - 600 * (ph - 4300) / 900;                // half-open again
      int16_t lid = RF_UY - 3 + 10 * k / 1000;   // separate horizontal lid sinks to closure
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s);
        rfArc(ex, RF_LY - 1, 7, 2, 138, 246, 1, 0);  // thin lower crescent
        rfBar(ex, lid, 7, 1, s, 0, 0);               // shorter flat padded eyelid
      }
      if (ph >= 3000) {
        int32_t z = ph - 3000;
        display.drawBitmap(108 + z / 700, 14 - z * 8 / 2200, RF_BMP_Z, 7, 7, SSD1306_WHITE);
      }
      if (ph >= 3200 && ph < 4300) rfBox(RF_MX, RF_MY + 1, 4, 2, 1); // tiny snore
      break;
    }

    case EYE_SURPRISED: {
      int32_t ph = t % 2400;
      int16_t pop;
      if (ph < 110) pop = 6 * ph / 110;                // rapid open
      else if (ph < 260) pop = 6 - 2 * (ph - 110) / 150; // soft overshoot settle
      else pop = 4;
      int16_t w = 16 + pop / 2;
      int16_t h = rfShrinkTo(6 + pop / 2, rfBlink(t, 5000, 140, 4), 1);
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s);
        rfBox(ex, RF_UY + 1, w, h, 3);                 // rapid horizontal OLED expansion
      }
      rfOval(RF_MX, RF_MY + 1, 2, 3);                  // small open mouth
      break;
    }

    case EYE_WORRIED: {
      int16_t lx = rfPath(t, RF_LOOK_WORRIED, 5, 700, 120);
      int16_t c = rfBlink(t, 2700, 150, 7);
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s) + lx / 2;
        int16_t q = s < 0 ? rfWave(t, 330) / 700 : rfWave(t, 410) / 700;
        int16_t w = s < 0 ? 15 : 18;
        int16_t h = rfShrinkTo(s < 0 ? 7 : 9, c, 1);
        rfBar(ex, RF_UY - 8 + q, 4, 1, s, s < 0 ? -1 : 1, s < 0 ? 1 : -1); // uneven brows
        rfBox(ex, RF_UY + q, w, h, 3);               // unequal long, thin LED forms
      }
      rfZig(RF_MX, RF_MY + 1, 4, 1, (t / 300) & 1, 1);
      break;
    }

    case EYE_CONFUSED: {
      int16_t s = rfWave(t, 3600);
      int16_t hl = rfShrinkTo(14 + s * 3 / 1000, rfBlink(t, 3900, 160, 9), 3);
      rfBox(RF_LX, RF_UY - s / 700, 17, rfShrinkTo(7 + s * 2 / 1000, rfBlink(t,3900,160,3), 1), 3); // wider eye stays asymmetric
      rfBar(RF_LX, RF_LY, 3, 1, -1, 0, 0);
      rfBar(RF_RX, RF_UY + s / 700, 4, 2, 1, -2 - s * 2 / 1000, 2 + s * 2 / 1000); // tilted slit
      display.drawBitmap(111, 11 + rfAbs(rfWave(t, 1000)) * 2 / 1000, RF_BMP_QUESTION, 8, 10, SSD1306_WHITE);
      int16_t m = s * 2 / 1000;
      rfCapsule(RF_MX - 4, RF_MY + 1 + m, RF_MX + 4, RF_MY + 1 - m, 1);
      break;
    }

    case EYE_EXCITED: {
      int16_t b = rfAbs(rfWave(t, 840));             // bounce every 420 ms
      int16_t dy = -(b * 2 / 1000);
      int16_t wv = (rfWave(t, 700) + 1000) / 1000;
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s);
        rfBox(ex, RF_UY + dy, 17 + wv, 6, 2);         // thinner-width bouncing LED bars
        rfBar(ex, RF_LY + dy, 5, 1, s, 0, 0);
      }
      rfHalfOval(RF_MX, RF_MY - 1 + dy, 7, 2 + (rfWave(t, 500) + 1000) * 2 / 2000); // quick grin pulse
      if ((t / 180) % 2 == 0) {
        display.drawBitmap(10, 12, RF_BMP_SPARK, 7, 7, SSD1306_WHITE);
        display.drawBitmap(111, 39, RF_BMP_SPARK, 7, 7, SSD1306_WHITE);
      } else {
        display.drawBitmap(10, 39, RF_BMP_SPARK, 7, 7, SSD1306_WHITE);
        display.drawBitmap(111, 12, RF_BMP_SPARK, 7, 7, SSD1306_WHITE);
      }
      break;
    }

    case EYE_SUSPICIOUS: {
      int16_t lx = rfPath(t, RF_LOOK_SHIFTY, 2, 1500, 450);
      int16_t tl = rfWave(t, 2900) * 2 / 1000;
      rfBar(RF_LX + lx, RF_UY + 1, 8, 1, -1, 1 + tl, -1 - tl);   // narrow suspicious slit
      rfBox(RF_RX - lx / 3, RF_UY, 15, rfShrinkTo(6, rfBlink(t, 4600, 140, 9), 1), 3);
      rfCapsule(RF_MX + 2 + lx / 3, RF_MY + 1, RF_MX + 8 + lx / 3, RF_MY - 1, 1); // smirk
      break;
    }

    case EYE_LOVE: {
      int32_t ph = t % 1250;                         // lub-dub heartbeat
      int16_t p = 0;
      if (ph < 120) p = 3 * ph / 120;
      else if (ph < 240) p = 3 - 3 * (ph - 120) / 120;
      else if (ph >= 300 && ph < 400) p = 2 * (ph - 300) / 100;
      else if (ph >= 400 && ph < 520) p = 2 - 2 * (ph - 400) / 120;
      int16_t dy = rfWave(t, 2600) * 2 / 1000;
      bool squint = (t % 3200) >= 2600;              // happy squint for 0.6 s
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s);
        if (squint) rfEyeArc(ex, RF_UY + 3 + dy, 7, 4, 2, s, 0);
        else rfBox(ex, RF_UY, 15, 6, 3);            // narrower rounded pixel eyes
        rfBar(ex, RF_LY + dy, 3, 1, s, 0, -1);      // tiny lower pieces
      }
      rfHeart(RF_MX, RF_MY + dy, 2 + (p + 1) / 2);
      break;
    }

    case EYE_BORED: {
      int16_t lx = rfPath(t, RF_LOOK_BORED, 4, 2600, 1400);
      int16_t c = rfBlink(t, 4800, 500, 11);
      int16_t lid = RF_UY + 1 + 3 * c / 1000;        // nearly flat lid, barely opens
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s) + lx;
        rfBox(ex, lid, 17, 3, 1);                    // extremely flat horizontal lid
        rfBox(ex, RF_LY - 1, 10, 1, 0);             // fine lower glint
      }
      rfBar(RF_MX + lx / 2, RF_MY + 1, 4, 1, 1, 0, 0); // flat mouth
      break;
    }

    case EYE_SCARED: {
      int16_t lx = rfPath(t, RF_LOOK_SCARED, 6, 650, 60);
      int16_t jx = rfJitter(t, 50, 1, 13), jy = rfJitter(t, 50, 1, 17);
        int16_t h = rfShrinkTo(6 + (rfWave(t, 400) + 1000) / 1500, rfBlink(t, 2200, 90, 13), 1);
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s) + s * 2 + lx + jx;   // compact and wide-set
        rfBox(ex, RF_UY + jy, 10, h, 2);            // small nervous horizontal LED capsule
        rfBar(ex, RF_LY + jy, 2, 1, s, 0, 0);
      }
      rfZig(RF_MX + jx, RF_MY + 1, 3, 1, (t / 90) & 1, 1);
      break;
    }

    case EYE_FURIOUS: {
      int16_t sx = rfJitter(t, 40, 1, 19), sy = rfJitter(t, 40, 1, 23);
      int16_t fl = rfWave(t, 600) > 0 ? 3 : 2;       // subtle flare in blade weight
      for (int8_t s = -1; s <= 1; s += 2) {
        int16_t ex = rfEyeX(s) + sx;
        rfWedge(ex, RF_UY + sy, s, 6, 6, fl);         // heavy sharp centre-facing blade
      }
      if ((t / 250) % 2 == 0) display.drawBitmap(108, 10, RF_BMP_ANGER, 9, 8, SSD1306_WHITE);
      rfZig(RF_MX + sx, RF_MY + 1 + sy, 6, 1, (t / 120) & 1, 1);
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
