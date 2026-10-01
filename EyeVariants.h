// =====================================================
// EYE VARIANTS — eye expressions for SSD1306 128x64
//
// Ported from the ESP32 Eyes project (esp32-eyes-main/):
//   - Preset_* values: copied verbatim from EyePresets.h
//   - eyeDraw():       EyeDrawer::Draw() with u8g2 calls
//                      replaced by Adafruit_GFX calls
//   - eye placement, mirroring, LookAt() and blink math:
//                      Face.cpp, Eye.cpp, LookAssistant.cpp,
//                      EyeBlink.cpp/.h
// Original: Copyright (c) 2023 Alastair Aitchison, Playful
// Technology, 2020 Luis Llamas (www.luisllamas.es), AGPL-3.0.
//
// No bitmaps: each variant is ~2 config pointers + flags,
// drawn once with fillRect/fillTriangle/drawFastHLine.
// Uses the global `display` from v3.ino (include after it).
// =====================================================

#ifndef EYE_VARIANTS_H
#define EYE_VARIANTS_H

#include <Arduino.h>

struct EyeConfig
{
	int16_t OffsetX;
	int16_t OffsetY;

 	int16_t Height;
	int16_t Width;

	float Slope_Top;
	float Slope_Bottom;

	int16_t Radius_Top;
	int16_t Radius_Bottom;

	int16_t Inverse_Radius_Top;
	int16_t Inverse_Radius_Bottom;

	int16_t Inverse_Offset_Top;
	int16_t Inverse_Offset_Bottom;
};

// ---- Presets (verbatim from esp32-eyes-main/EyePresets.h) ----
static const EyeConfig Preset_Normal = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 40,
	.Width = 40,
	.Slope_Top = 0,
	.Slope_Bottom = 0,
	.Radius_Top = 8,
	.Radius_Bottom = 8,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Glee = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 8,
	.Width = 40,
	.Slope_Top = 0,
	.Slope_Bottom = 0,
	.Radius_Top = 8,
	.Radius_Bottom = 0,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 5,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Sad = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 15,
	.Width = 40,
	.Slope_Top = -0.5,
	.Slope_Bottom = 0,
	.Radius_Top = 1,
	.Radius_Bottom = 10,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Worried = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 25,
	.Width = 40,
	.Slope_Top = -0.1,
	.Slope_Bottom = 0,
	.Radius_Top = 6,
	.Radius_Bottom = 10,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Worried_Alt = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 35,
	.Width = 40,
	.Slope_Top = -0.2,
	.Slope_Bottom = 0,
	.Radius_Top = 6,
	.Radius_Bottom = 10,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Focused = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 14,
	.Width = 40,
	.Slope_Top = 0.2,
	.Slope_Bottom = 0,
	.Radius_Top = 3,
	.Radius_Bottom = 1,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Annoyed = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 12,
	.Width = 40,
	.Slope_Top = 0,
	.Slope_Bottom = 0,
	.Radius_Top = 0,
	.Radius_Bottom = 10,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Annoyed_Alt = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 5,
	.Width = 40,
	.Slope_Top = 0,
	.Slope_Bottom = 0,
	.Radius_Top = 0,
	.Radius_Bottom = 4,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Frustrated = {
	.OffsetX = 3,
	.OffsetY = -5,
	.Height = 12,
	.Width = 40,
	.Slope_Top = 0,
	.Slope_Bottom = 0,
	.Radius_Top = 0,
	.Radius_Bottom = 10,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Sleepy = {
	.OffsetX = 0,
	.OffsetY = -2,
	.Height = 14,
	.Width = 40,
	.Slope_Top = -0.5,
	.Slope_Bottom = -0.5,
	.Radius_Top = 3,
	.Radius_Bottom = 3,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Sleepy_Alt = {
	.OffsetX = 0,
	.OffsetY = -2,
	.Height = 8,
	.Width = 40,
	.Slope_Top = -0.5,
	.Slope_Bottom = -0.5,
	.Radius_Top = 3,
	.Radius_Bottom = 3,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Suspicious = {
	.OffsetX = 0,
	.OffsetY = 0,
	.Height = 22,
	.Width = 40,
	.Slope_Top = 0,
	.Slope_Bottom = 0,
	.Radius_Top = 8,
	.Radius_Bottom = 3,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Suspicious_Alt = {
	.OffsetX = 0,
	.OffsetY = -3,
	.Height = 16,
	.Width = 40,
	.Slope_Top = 0.2,
	.Slope_Bottom = 0,
	.Radius_Top = 6,
	.Radius_Bottom = 3,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Angry = {
	.OffsetX = -3,
	.OffsetY = 0,
	.Height = 20,
	.Width = 40,
	.Slope_Top = 0.3,
	.Slope_Bottom = 0,
	.Radius_Top = 2,
	.Radius_Bottom = 12,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Scared = {
	.OffsetX = -3,
	.OffsetY = 0,
	.Height = 40,
	.Width = 40,
	.Slope_Top = -0.1,
	.Slope_Bottom = 0,
	.Radius_Top = 12,
	.Radius_Bottom = 8,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

static const EyeConfig Preset_Awe = {
	.OffsetX = 2,
	.OffsetY = 0,
	.Height = 35,
	.Width = 45,
	.Slope_Top = -0.1,
	.Slope_Bottom = 0.1,
	.Radius_Top = 12,
	.Radius_Bottom = 12,
	.Inverse_Radius_Top = 0,
	.Inverse_Radius_Bottom = 0,
	.Inverse_Offset_Top = 0,
	.Inverse_Offset_Bottom = 0
};

// ---- Variant IDs ----
enum EyeVariantId : uint8_t {
  EYE_NEUTRAL = 0,      //  1. Neutral
  EYE_BLINK_HIGH,       //  2. Blink (high)
  EYE_GLEE,             //  3. Glee
  EYE_SAD_UP,           //  4. Sad (looking up to user)
  EYE_WORRIED,          //  5. Worried
  EYE_FOCUSED,          //  6. Focused/Determined
  EYE_ANNOYED,          //  7. Annoyed
  EYE_FRUSTRATED,       //  8. Frustrated/Bored
  EYE_SLEEPY,           //  9. Sleepy Eyes
  EYE_SUSPICIOUS,       // 10. Suspicious
  EYE_ANGRY,            // 11. Angry
  EYE_SCARED,           // 12. Scared
  EYE_AWE,              // 13. Awe
  EYE_VARIANT_COUNT
};

// look: 0 = front, +1 = LookTop(), -1 = LookBottom()  (Face.cpp)
// blink: eyes fully closed (EyeBlink at t = 1)
// right/left: presets as assigned in FaceExpression::GoTo_*()
struct EyeVariant {
  const char *name;
  const EyeConfig *right;
  const EyeConfig *left;
  int8_t look;
  bool blink;
};

static const EyeVariant eyeVariants[] = {
  { "Neutral",                  &Preset_Normal,      &Preset_Normal,          0, false },
  { "Blink (high)",             &Preset_Normal,      &Preset_Normal,         +1, true  },
  { "Glee",                     &Preset_Glee,        &Preset_Glee,            0, false },
  { "Sad (looking up to user)", &Preset_Sad,         &Preset_Sad,            +1, false },
  { "Worried",                  &Preset_Worried,     &Preset_Worried_Alt,     0, false },
  { "Focused/Determined",       &Preset_Focused,     &Preset_Focused,         0, false },
  { "Annoyed",                  &Preset_Annoyed,     &Preset_Annoyed_Alt,     0, false },
  { "Frustrated/Bored",         &Preset_Frustrated,  &Preset_Frustrated,      0, false },
  { "Sleepy Eyes",              &Preset_Sleepy,      &Preset_Sleepy_Alt,      0, false },
  { "Suspicious",               &Preset_Suspicious,  &Preset_Suspicious_Alt,  0, false },
  { "Angry",                    &Preset_Angry,       &Preset_Angry,           0, false },
  { "Scared",                   &Preset_Scared,      &Preset_Scared,          0, false },
  { "Awe",                      &Preset_Awe,         &Preset_Awe,             0, false },
};
static_assert(sizeof(eyeVariants) / sizeof(eyeVariants[0]) == EYE_VARIANT_COUNT, "eyeVariants[] needs one entry per EyeVariantId");

// ---- Drawing primitives (u8g2 -> Adafruit_GFX) ----

enum EyeCornerType { EYE_T_R, EYE_T_L, EYE_B_L, EYE_B_R };

static void eyeHLine(int32_t x, int32_t y, int32_t w) {
  if (w > 0) display.drawFastHLine(x, y, w, SSD1306_WHITE);
}

// EyeDrawer::FillRectangle
static void eyeFillRectangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color) {
  int32_t l = min(x0, x1);
  int32_t r = max(x0, x1);
  int32_t t = min(y0, y1);
  int32_t b = max(y0, y1);
  int32_t w = r - l;
  int32_t h = b - t;
  if (w > 0 && h > 0) display.fillRect(l, t, w, h, color ? SSD1306_WHITE : SSD1306_BLACK);
}

// EyeDrawer::FillRectangularTriangle
static void eyeFillRectangularTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color) {
  display.fillTriangle(x0, y0, x1, y1, x1, y0, color ? SSD1306_WHITE : SSD1306_BLACK);
}

// EyeDrawer::FillEllipseCorner
static void eyeFillEllipseCorner(EyeCornerType corner, int16_t x0, int16_t y0, int32_t rx, int32_t ry) {
  if (rx < 2) return;
  if (ry < 2) return;
  int32_t x, y;
  int32_t rx2 = rx * rx;
  int32_t ry2 = ry * ry;
  int32_t fx2 = 4 * rx2;
  int32_t fy2 = 4 * ry2;
  int32_t s;

  if (corner == EYE_T_R) {
    for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
      eyeHLine(x0, y0 - y, x);
      if (s >= 0) { s += fx2 * (1 - y); y--; }
      s += ry2 * ((4 * x) + 6);
    }
    for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
      eyeHLine(x0, y0 - y, x);
      if (s >= 0) { s += fy2 * (1 - x); x--; }
      s += rx2 * ((4 * y) + 6);
    }
  }
  else if (corner == EYE_B_R) {
    for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
      eyeHLine(x0, y0 + y - 1, x);
      if (s >= 0) { s += fx2 * (1 - y); y--; }
      s += ry2 * ((4 * x) + 6);
    }
    for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
      eyeHLine(x0, y0 + y - 1, x);
      if (s >= 0) { s += fy2 * (1 - x); x--; }
      s += rx2 * ((4 * y) + 6);
    }
  }
  else if (corner == EYE_T_L) {
    for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
      eyeHLine(x0 - x, y0 - y, x);
      if (s >= 0) { s += fx2 * (1 - y); y--; }
      s += ry2 * ((4 * x) + 6);
    }
    for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
      eyeHLine(x0 - x, y0 - y, x);
      if (s >= 0) { s += fy2 * (1 - x); x--; }
      s += rx2 * ((4 * y) + 6);
    }
  }
  else if (corner == EYE_B_L) {
    for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
      eyeHLine(x0 - x, y0 + y - 1, x);
      if (s >= 0) { s += fx2 * (1 - y); y--; }
      s += ry2 * ((4 * x) + 6);
    }
    for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
      eyeHLine(x0 - x, y0 + y, x);
      if (s >= 0) { s += fy2 * (1 - x); x--; }
      s += rx2 * ((4 * y) + 6);
    }
  }
}

// EyeDrawer::Draw — config is taken by value (the original adjusts radii in place)
static void eyeDraw(int16_t centerX, int16_t centerY, EyeConfig cfg) {
  EyeConfig *config = &cfg;

  int32_t delta_y_top = config->Height * config->Slope_Top / 2.0;
  int32_t delta_y_bottom = config->Height * config->Slope_Bottom / 2.0;
  int32_t totalHeight = config->Height + delta_y_top - delta_y_bottom;

  if (config->Radius_Bottom > 0 && config->Radius_Top > 0 && totalHeight - 1 < config->Radius_Bottom + config->Radius_Top) {
    int32_t corrected_radius_top = (float)config->Radius_Top * (totalHeight - 1) / (config->Radius_Bottom + config->Radius_Top);
    int32_t corrected_radius_bottom = (float)config->Radius_Bottom * (totalHeight - 1) / (config->Radius_Bottom + config->Radius_Top);
    config->Radius_Top = corrected_radius_top;
    config->Radius_Bottom = corrected_radius_bottom;
  }

  int32_t TLc_y = centerY + config->OffsetY - config->Height/2 + config->Radius_Top - delta_y_top;
  int32_t TLc_x = centerX + config->OffsetX - config->Width/2 + config->Radius_Top;
  int32_t TRc_y = centerY + config->OffsetY - config->Height/2 + config->Radius_Top + delta_y_top;
  int32_t TRc_x = centerX + config->OffsetX + config->Width/2 - config->Radius_Top;
  int32_t BLc_y = centerY + config->OffsetY + config->Height/2 - config->Radius_Bottom - delta_y_bottom;
  int32_t BLc_x = centerX + config->OffsetX - config->Width/2 + config->Radius_Bottom;
  int32_t BRc_y = centerY + config->OffsetY + config->Height/2 - config->Radius_Bottom + delta_y_bottom;
  int32_t BRc_x = centerX + config->OffsetX + config->Width/2 - config->Radius_Bottom;

  int32_t min_c_x = min(TLc_x, BLc_x);
  int32_t max_c_x = max(TRc_x, BRc_x);
  int32_t min_c_y = min(TLc_y, TRc_y);
  int32_t max_c_y = max(BLc_y, BRc_y);

  eyeFillRectangle(min_c_x, min_c_y, max_c_x, max_c_y, 1);

  eyeFillRectangle(TRc_x, TRc_y, BRc_x + config->Radius_Bottom, BRc_y, 1); // Right
  eyeFillRectangle(TLc_x - config->Radius_Top, TLc_y, BLc_x, BLc_y, 1);    // Left
  eyeFillRectangle(TLc_x, TLc_y - config->Radius_Top, TRc_x, TRc_y, 1);    // Top
  eyeFillRectangle(BLc_x, BLc_y, BRc_x, BRc_y + config->Radius_Bottom, 1); // Bottom

  if (config->Slope_Top > 0) {
    eyeFillRectangularTriangle(TLc_x, TLc_y - config->Radius_Top, TRc_x, TRc_y - config->Radius_Top, 0);
    eyeFillRectangularTriangle(TRc_x, TRc_y - config->Radius_Top, TLc_x, TLc_y - config->Radius_Top, 1);
  }
  else if (config->Slope_Top < 0) {
    eyeFillRectangularTriangle(TRc_x, TRc_y - config->Radius_Top, TLc_x, TLc_y - config->Radius_Top, 0);
    eyeFillRectangularTriangle(TLc_x, TLc_y - config->Radius_Top, TRc_x, TRc_y - config->Radius_Top, 1);
  }

  if (config->Slope_Bottom > 0) {
    eyeFillRectangularTriangle(BRc_x + config->Radius_Bottom, BRc_y + config->Radius_Bottom, BLc_x - config->Radius_Bottom, BLc_y + config->Radius_Bottom, 0);
    eyeFillRectangularTriangle(BLc_x - config->Radius_Bottom, BLc_y + config->Radius_Bottom, BRc_x + config->Radius_Bottom, BRc_y + config->Radius_Bottom, 1);
  }
  else if (config->Slope_Bottom < 0) {
    eyeFillRectangularTriangle(BLc_x - config->Radius_Bottom, BLc_y + config->Radius_Bottom, BRc_x + config->Radius_Bottom, BRc_y + config->Radius_Bottom, 0);
    eyeFillRectangularTriangle(BRc_x + config->Radius_Bottom, BRc_y + config->Radius_Bottom, BLc_x - config->Radius_Bottom, BLc_y + config->Radius_Bottom, 1);
  }

  if (config->Radius_Top > 0) {
    eyeFillEllipseCorner(EYE_T_L, TLc_x, TLc_y, config->Radius_Top, config->Radius_Top);
    eyeFillEllipseCorner(EYE_T_R, TRc_x, TRc_y, config->Radius_Top, config->Radius_Top);
  }
  if (config->Radius_Bottom > 0) {
    eyeFillEllipseCorner(EYE_B_L, BLc_x, BLc_y, config->Radius_Bottom, config->Radius_Bottom);
    eyeFillEllipseCorner(EYE_B_R, BRc_x, BRc_y, config->Radius_Bottom, config->Radius_Bottom);
  }
}

// ---- Preset -> final eye config (static, no random look/blink) ----

// Eye::ApplyPreset() mirroring + LookAssistant::LookAt(0, look) via
// EyeTransformation::Apply() + EyeBlink::Apply(t = 1) when blinking.
static EyeConfig eyeFinalConfig(const EyeConfig &p, bool mirrored, int8_t look, bool blink) {
  EyeConfig c;
  c.OffsetX = mirrored ? -p.OffsetX : p.OffsetX;
  c.OffsetY = -p.OffsetY;
  c.Height = p.Height;
  c.Width = p.Width;
  c.Slope_Top = mirrored ? p.Slope_Top : -p.Slope_Top;
  c.Slope_Bottom = mirrored ? p.Slope_Bottom : -p.Slope_Bottom;
  c.Radius_Top = p.Radius_Top;
  c.Radius_Bottom = p.Radius_Bottom;
  c.Inverse_Radius_Top = p.Inverse_Radius_Top;
  c.Inverse_Radius_Bottom = p.Inverse_Radius_Bottom;
  c.Inverse_Offset_Top = 0;
  c.Inverse_Offset_Bottom = 0;

  // LookAt(0, y): MoveY = 20*y, ScaleY = 1 - |y|*0.4
  float moveY = 20 * look;
  float scaleY = 1.0 - (look > 0 ? look : -look) * 0.4;
  c.OffsetY = c.OffsetY - moveY;
  c.Height = c.Height * scaleY;

  if (blink) {
    // EyeBlink at t = 1 (BlinkHeight = 2). Width is kept at the eye's own
    // width instead of BlinkWidth = 60 so the two closed eyes stay separate.
    c.Height = 2;
    c.Slope_Top = 0;
    c.Slope_Bottom = 0;
    c.Radius_Top = 0;
    c.Radius_Bottom = 0;
    c.Inverse_Radius_Top = 0;
    c.Inverse_Radius_Bottom = 0;
  }
  return c;
}

// Face(128, 64, eyeSize = 40), EyeInterDistance = 4; left eye is mirrored
static const int16_t EYE_SIZE = 40;
static const int16_t EYE_INTER_DISTANCE = 4;

static void drawEyeVariant(uint8_t id) {
  if (id >= EYE_VARIANT_COUNT) return;
  const EyeVariant &v = eyeVariants[id];

  const int16_t cx = SCREEN_WIDTH / 2;
  const int16_t cy = SCREEN_HEIGHT / 2;

  display.clearDisplay();
  eyeDraw(cx - EYE_SIZE / 2 - EYE_INTER_DISTANCE, cy, eyeFinalConfig(*v.left, true, v.look, v.blink));
  eyeDraw(cx + EYE_SIZE / 2 + EYE_INTER_DISTANCE, cy, eyeFinalConfig(*v.right, false, v.look, v.blink));
  display.display();
}

// =====================================================
// EYE ANIMATION (EYE_MODE) - millis()-based, no delay()
//
// Built from the library's own animation pieces, per eye:
//   preset (mirrored) -> LookAt(x, y)     (LookAssistant / EyeTransformation)
//                     -> Variation1, 2    (EyeVariation + TrapeziumPulseAnimation)
//                     -> blink            (EyeBlink + TrapeziumAnimation, t*t)
// eyeAnims[] is parallel to eyeVariants[]; the variants are unchanged.
// =====================================================

// TrapeziumPulseAnimation phases (ms): wait t0, rise over t1, hold t2,
// fall over t3, rest t4. Its 0..1 value is applied as (2t - 1).
struct EyeWave { uint16_t t0, t1, t2, t3, t4; };

// EyeVariation::Values (pixels) driven by a wave
struct EyeMotion { int8_t OffsetX, OffsetY, Height, Width; EyeWave wave; };

// LookAt(x / 10.0, y / 10.0)
struct EyeLookPoint { int8_t x, y; };

struct EyeAnim {
  EyeMotion right1, right2, left1, left2;          // like Eye::Variation1/2
  uint16_t blinkEveryMs;                           // 0 = no blink
  uint16_t blinkCloseMs, blinkHoldMs, blinkOpenMs; // TrapeziumAnimation(t0, t1, t2)
  const EyeLookPoint *look;                        // nullptr = fixed look (EyeVariant.look)
  uint8_t lookCount;
  uint16_t lookDwellMs;                            // time per look point
  uint16_t lookMoveMs;                             // ramp to the next point (library: 200)
};

#define EW_NONE    { 0, 0, 0, 0, 0 }
#define EW_TRI(T)  { 0, (T) / 2, 0, (T) / 2, 0 }      // SetTriangle(T, 0)
#define EW_V2      { 0, 200, 200, 200, 200 }          // Eye.cpp Variation2 default
#define EM_NONE    { 0, 0, 0, 0, EW_NONE }
#define EYE_LOOK(a)  a, (uint8_t)(sizeof(a) / sizeof(a[0]))
#define EYE_NO_LOOK  nullptr, 0

static const EyeLookPoint LOOK_SIDE[]      = { {0, 0}, {-6, 0}, {0, 0}, {6, 0} };
static const EyeLookPoint LOOK_AROUND[]    = { {0, 0}, {-5, 3}, {5, 3}, {5, -3}, {-5, -3} };
static const EyeLookPoint LOOK_SHIFTY[]    = { {-7, 0}, {7, 0} };
static const EyeLookPoint LOOK_AWAY[]      = { {0, 0}, {6, 1} };
static const EyeLookPoint LOOK_AWE[]       = { {0, 3} };

static const EyeAnim eyeAnims[] = {
  // 1. Neutral: library Normal variations (breathing) + natural blink (BlinkAssistant 3500 ms, EyeBlink 40/100/40)
  { {0, 0, 3, 0, EW_TRI(1000)}, {0, 0, 0, 1, EW_V2}, {0, 0, 2, 0, EW_TRI(1000)}, {0, 0, 0, 2, EW_V2},
    3500, 40, 100, 40, EYE_NO_LOOK, 0, 0 },
  // 2. Blink (high): looking up, open -> half -> closed -> half -> open, repeating
  { EM_NONE, EM_NONE, EM_NONE, EM_NONE, 1300, 200, 100, 200, EYE_NO_LOOK, 0, 0 },
  // 3. Glee: library Glee variation (OffsetY 5, SetTriangle(300))
  { {0, 5, 0, 0, EW_TRI(300)}, EM_NONE, {0, 5, 0, 0, EW_TRI(300)}, EM_NONE,
    0, 0, 0, 0, EYE_NO_LOOK, 0, 0 },
  // 4. Sad (looking up to user): pleading quiver + blink
  { {0, 0, 2, 0, EW_TRI(400)}, EM_NONE, {0, 0, 2, 0, EW_TRI(400)}, EM_NONE,
    4500, 60, 120, 80, EYE_NO_LOOK, 0, 0 },
  // 5. Worried: nervous side glances + quiver + blink
  { {0, 0, 2, 0, EW_TRI(500)}, EM_NONE, {0, 0, 2, 0, EW_TRI(500)}, EM_NONE,
    3000, 40, 100, 40, EYE_LOOK(LOOK_SIDE), 900, 200 },
  // 6. Focused/Determined: slow narrowing pulse + rare blink
  { {0, 0, 2, 0, EW_TRI(1500)}, EM_NONE, {0, 0, 2, 0, EW_TRI(1500)}, EM_NONE,
    6000, 40, 100, 40, EYE_NO_LOOK, 0, 0 },
  // 7. Annoyed: glance away and back + slow blink
  { EM_NONE, EM_NONE, EM_NONE, EM_NONE, 3500, 80, 150, 80, EYE_LOOK(LOOK_AWAY), 1500, 250 },
  // 8. Frustrated/Bored: wandering look around + slow blink
  { EM_NONE, EM_NONE, EM_NONE, EM_NONE, 3000, 100, 150, 150, EYE_LOOK(LOOK_AROUND), 1400, 500 },
  // 9. Sleepy Eyes: drooping lids + slow heavy blink
  { {0, 1, 2, 0, EW_TRI(3000)}, EM_NONE, {0, 1, 2, 0, EW_TRI(3000)}, EM_NONE,
    2500, 300, 400, 300, EYE_NO_LOOK, 0, 0 },
  // 10. Suspicious: shifty left/right look + blink
  { EM_NONE, EM_NONE, EM_NONE, EM_NONE, 5000, 40, 100, 40, EYE_LOOK(LOOK_SHIFTY), 1200, 300 },
  // 11. Angry: library Angry variation (OffsetY 2, SetTriangle(300)) + blink
  { {0, 2, 0, 0, EW_TRI(300)}, EM_NONE, {0, 2, 0, 0, EW_TRI(300)}, EM_NONE,
    4000, 40, 100, 40, EYE_NO_LOOK, 0, 0 },
  // 12. Scared: tremble + darting side looks + quick blink
  { {1, 0, 0, 0, EW_TRI(100)}, EM_NONE, {1, 0, 0, 0, EW_TRI(100)}, EM_NONE,
    3000, 30, 60, 30, EYE_LOOK(LOOK_SIDE), 700, 120 },
  // 13. Awe: slow wonder pulse, looking slightly up + blink
  { {0, 0, 3, 2, EW_TRI(1500)}, EM_NONE, {0, 0, 3, 2, EW_TRI(1500)}, EM_NONE,
    4500, 40, 100, 40, EYE_LOOK(LOOK_AWE), 1000, 0 },
};
static_assert(sizeof(eyeAnims) / sizeof(eyeAnims[0]) == EYE_VARIANT_COUNT, "eyeAnims[] needs one entry per eye variant");

// TrapeziumPulseAnimation::Calculate()
static float eyeWaveValue(const EyeWave &w, unsigned long elapsed) {
  unsigned long interval = (unsigned long)w.t0 + w.t1 + w.t2 + w.t3 + w.t4;
  if (interval == 0) return 0.5f;  // no wave: variation contributes 0
  unsigned long e = elapsed % interval;
  if (e < w.t0) return 0.0f;
  if (e < (unsigned long)w.t0 + w.t1) return (float)(e - w.t0) / w.t1;
  if (e < (unsigned long)w.t0 + w.t1 + w.t2) return 1.0f;
  if (e < (unsigned long)w.t0 + w.t1 + w.t2 + w.t3) return 1.0f - (float)(e - w.t0 - w.t1 - w.t2) / w.t3;
  return 0.0f;
}

// EyeVariation::Apply(2t - 1)
static void eyeApplyMotion(EyeConfig &c, const EyeMotion &m, unsigned long elapsed) {
  float t = 2.0f * eyeWaveValue(m.wave, elapsed) - 1.0f;
  c.OffsetX = c.OffsetX + m.OffsetX * t;
  c.OffsetY = c.OffsetY + m.OffsetY * t;
  c.Height = c.Height + m.Height * t;
  c.Width = c.Width + m.Width * t;
}

// Blink at the end of every blinkEveryMs period: TrapeziumAnimation, then t*t (EyeBlink::Update)
static float eyeBlinkAmount(const EyeAnim &a, unsigned long elapsed) {
  if (a.blinkEveryMs == 0) return 0.0f;
  unsigned long dur = (unsigned long)a.blinkCloseMs + a.blinkHoldMs + a.blinkOpenMs;
  unsigned long phase = elapsed % a.blinkEveryMs;
  if (phase + dur < a.blinkEveryMs) return 0.0f;
  unsigned long e = phase - (a.blinkEveryMs - dur);
  float t;
  if (e < a.blinkCloseMs) t = (float)e / a.blinkCloseMs;
  else if (e < (unsigned long)a.blinkCloseMs + a.blinkHoldMs) t = 1.0f;
  else t = 1.0f - (float)(e - a.blinkCloseMs - a.blinkHoldMs) / a.blinkOpenMs;
  if (t < 0.0f) t = 0.0f;
  return t * t;
}

// Look point at this moment: hold each point lookDwellMs, ramping to it over lookMoveMs
static void eyeLookNow(const EyeAnim &a, int8_t fixedY, unsigned long elapsed, float &x, float &y) {
  if (a.look == nullptr || a.lookCount == 0 || a.lookDwellMs == 0) {
    x = 0.0f;
    y = fixedY;
    return;
  }
  unsigned long seg = elapsed / a.lookDwellMs;
  unsigned long inSeg = elapsed % a.lookDwellMs;
  const EyeLookPoint &to = a.look[seg % a.lookCount];
  const EyeLookPoint &from = a.look[(seg + a.lookCount - 1) % a.lookCount];
  float r = (seg == 0 || inSeg >= a.lookMoveMs) ? 1.0f : (float)inSeg / a.lookMoveMs;
  x = (from.x + (to.x - from.x) * r) / 10.0f;
  y = (from.y + (to.y - from.y) * r) / 10.0f;
}

static EyeConfig eyeAnimatedConfig(const EyeConfig &preset, bool mirrored, const EyeMotion &m1, const EyeMotion &m2,
                                   float lookX, float lookY, float blinkT, unsigned long elapsed) {
  EyeConfig c = eyeFinalConfig(preset, mirrored, 0, false);  // Eye::ApplyPreset() mirroring

  // LookAssistant::LookAt(x, y) -> EyeTransformation::Apply()
  int16_t moveX = -25 * lookX;
  int16_t moveY = 20 * lookY;
  float scaleY = (mirrored ? 1.0f + lookX * 0.2f : 1.0f - lookX * 0.2f) * (1.0f - fabsf(lookY) * 0.4f);
  c.OffsetX = c.OffsetX + moveX;
  c.OffsetY = c.OffsetY - moveY;
  c.Height = c.Height * scaleY;

  eyeApplyMotion(c, m1, elapsed);
  eyeApplyMotion(c, m2, elapsed);

  // EyeBlink::Apply(t); width kept so the closed eyes stay separate (as in the static blink)
  if (blinkT > 0.0f) {
    c.Height = (2 - c.Height) * blinkT + c.Height;
    c.Slope_Top = c.Slope_Top * (1.0f - blinkT);
    c.Slope_Bottom = c.Slope_Bottom * (1.0f - blinkT);
    c.Radius_Top = c.Radius_Top * (1.0f - blinkT);
    c.Radius_Bottom = c.Radius_Bottom * (1.0f - blinkT);
    c.Inverse_Radius_Top = c.Inverse_Radius_Top * (1.0f - blinkT);
    c.Inverse_Radius_Bottom = c.Inverse_Radius_Bottom * (1.0f - blinkT);
  }
  return c;
}

static void drawEyeFrame(uint8_t id, unsigned long elapsed) {
  if (id >= EYE_VARIANT_COUNT) return;
  const EyeVariant &v = eyeVariants[id];
  const EyeAnim &a = eyeAnims[id];

  float lookX, lookY;
  eyeLookNow(a, v.look, elapsed, lookX, lookY);
  float blinkT = eyeBlinkAmount(a, elapsed);

  const int16_t cx = SCREEN_WIDTH / 2;
  const int16_t cy = SCREEN_HEIGHT / 2;

  display.clearDisplay();
  eyeDraw(cx - EYE_SIZE / 2 - EYE_INTER_DISTANCE, cy,
          eyeAnimatedConfig(*v.left, true, a.left1, a.left2, lookX, lookY, blinkT, elapsed));
  eyeDraw(cx + EYE_SIZE / 2 + EYE_INTER_DISTANCE, cy,
          eyeAnimatedConfig(*v.right, false, a.right1, a.right2, lookX, lookY, blinkT, elapsed));
  display.display();
}

static const unsigned long EYE_FRAME_MS = 33;   // ~30 fps
static unsigned long eyeAnimStartMs = 0;
static unsigned long eyeLastFrameMs = 0;

// Restart the selected variant's animation from its first frame
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
