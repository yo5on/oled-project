// =====================================================
// MOCHI ANIMATIONS — the 35 existing Mochi animations
//
// Same data and frame ranges as the ESP32_Mochi player:
//  - 20 clips of 250frames.gif: raw 1024-byte frames inside the one
//    intro_trimmed_frames[] array (stored once, black-on-white -> inverted)
//  - 15 emote GIFs: PackBits-compressed 1024-byte frames
// Every frame is 128x64, 1 bit per pixel, 16 bytes per row, MSB = left pixel.
// =====================================================

#pragma once

#include <stdint.h>

#include "animations/intro_trimmed.h"
#include "animations/happy.h"
#include "animations/happy_2.h"
#include "animations/angry.h"
#include "animations/angry_2.h"
#include "animations/confused_2.h"
#include "animations/content.h"
#include "animations/determined.h"
#include "animations/embarrassed.h"
#include "animations/excited_2.h"
#include "animations/frustrated.h"
#include "animations/laugh.h"
#include "animations/love.h"
#include "animations/proud.h"
#include "animations/relaxed.h"
#include "animations/sleepy_3.h"

enum AnimId : uint8_t {
  ANIM_SMILE, ANIM_ANNOYED, ANIM_FURIOUS, ANIM_EVIL, ANIM_MOCHI_10, ANIM_MOCHI_11,
  ANIM_BUG, ANIM_SCREAM, ANIM_EVIL_GRIN, ANIM_ANGRY_3, ANIM_SQUINT, ANIM_BLINK,
  ANIM_MOCHI_26, ANIM_HAPPY_3, ANIM_MOCHI_29, ANIM_SURPRISED,
  ANIM_UWU, ANIM_CRYING, ANIM_HAPPY, ANIM_HAPPY_2, ANIM_ANGRY, ANIM_ANGRY_2,
  ANIM_DIZZY, ANIM_KISS, ANIM_CONFUSED_2, ANIM_DETERMINED, ANIM_CONTENT,
  ANIM_EMBARRASSED, ANIM_EXCITED_2, ANIM_FRUSTRATED, ANIM_LAUGH, ANIM_LOVE,
  ANIM_PROUD, ANIM_RELAXED, ANIM_SLEEPY_3,
  ANIM_COUNT
};

struct MochiAnim {
  const char* name;
  const uint8_t* data;
  uint16_t startFrame;     // RAW only: first frame inside data (PACKED: always 0)
  uint16_t frameCount;
  uint8_t fps;
  bool packed;             // false = RAW 1024-byte frames, true = PackBits
  bool inverted;           // data stored black-on-white: show with the display inverted
};

#define INTRO intro_trimmed_frames[0]

// Indexed by AnimId (same order)
static const MochiAnim MOCHI_ANIMS[ANIM_COUNT] = {
  { "SMILE",       INTRO,   0,  4, 15, false, true },   //  #3  GIF frames  17-20
  { "ANNOYED",     INTRO,   4,  8, 15, false, true },   //  #4  GIF frames  21-28
  { "FURIOUS",     INTRO,  12,  7, 15, false, true },   //  #5  GIF frames  29-35
  { "EVIL",        INTRO,  26,  6, 15, false, true },   //  #8  GIF frames  51-56
  { "MOCHI_10",    INTRO,  32,  8, 15, false, true },   // #10  GIF frames  65-72
  { "MOCHI_11",    INTRO,  40,  6, 15, false, true },   // #11  GIF frames  73-78
  { "BUG",         INTRO,  46, 14, 15, false, true },   // #14  GIF frames  94-107
  { "SCREAM",      INTRO,  67,  7, 15, false, true },   // #17  GIF frames 123-129
  { "EVIL_GRIN",   INTRO,  74,  8, 15, false, true },   // #18  GIF frames 130-137
  { "ANGRY_3",     INTRO,  82,  7, 15, false, true },   // #20  GIF frames 145-151
  { "SQUINT",      INTRO,  96,  7, 15, false, true },   // #24  GIF frames 174-180
  { "BLINK",       INTRO, 103,  7, 15, false, true },   // #25  GIF frames 181-187
  { "MOCHI_26",    INTRO, 110,  8, 15, false, true },   // #26  GIF frames 188-195
  { "HAPPY_3",     INTRO, 125,  7, 15, false, true },   // #28  GIF frames 203-209
  { "MOCHI_29",    INTRO, 132,  8, 15, false, true },   // #29  GIF frames 210-217
  { "SURPRISED",   INTRO, 140,  7, 15, false, true },   // #30  GIF frames 218-224
  { "UWU",         INTRO,  60,  7, 15, false, true },   // #16  GIF frames 116-122
  { "CRYING",      INTRO,  19,  7, 15, false, true },   //  #7  GIF frames  44-50
  { "HAPPY",       anim_happy,       0, ANIM_HAPPY_FRAMES,       ANIM_HAPPY_FPS,       true, false },
  { "HAPPY_2",     anim_happy_2,     0, ANIM_HAPPY_2_FRAMES,     ANIM_HAPPY_2_FPS,     true, false },
  { "ANGRY",       anim_angry,       0, ANIM_ANGRY_FRAMES,       ANIM_ANGRY_FPS,       true, false },
  { "ANGRY_2",     anim_angry_2,     0, ANIM_ANGRY_2_FRAMES,     ANIM_ANGRY_2_FPS,     true, false },
  { "DIZZY",       INTRO,  89,  7, 15, false, true },   // #21  GIF frames 152-158
  { "KISS",        INTRO, 118,  7, 15, false, true },   // #27  GIF frames 196-202
  { "CONFUSED_2",  anim_confused_2,  0, ANIM_CONFUSED_2_FRAMES,  ANIM_CONFUSED_2_FPS,  true, false },
  { "DETERMINED",  anim_determined,  0, ANIM_DETERMINED_FRAMES,  ANIM_DETERMINED_FPS,  true, false },
  { "CONTENT",     anim_content,     0, ANIM_CONTENT_FRAMES,     ANIM_CONTENT_FPS,     true, false },
  { "EMBARRASSED", anim_embarrassed, 0, ANIM_EMBARRASSED_FRAMES, ANIM_EMBARRASSED_FPS, true, false },
  { "EXCITED_2",   anim_excited_2,   0, ANIM_EXCITED_2_FRAMES,   ANIM_EXCITED_2_FPS,   true, false },
  { "FRUSTRATED",  anim_frustrated,  0, ANIM_FRUSTRATED_FRAMES,  ANIM_FRUSTRATED_FPS,  true, false },
  { "LAUGH",       anim_laugh,       0, ANIM_LAUGH_FRAMES,       ANIM_LAUGH_FPS,       true, false },
  { "LOVE",        anim_love,        0, ANIM_LOVE_FRAMES,        ANIM_LOVE_FPS,        true, false },
  { "PROUD",       anim_proud,       0, ANIM_PROUD_FRAMES,       ANIM_PROUD_FPS,       true, false },
  { "RELAXED",     anim_relaxed,     0, ANIM_RELAXED_FRAMES,     ANIM_RELAXED_FPS,     true, false },
  { "SLEEPY_3",    anim_sleepy_3,    0, ANIM_SLEEPY_3_FRAMES,    ANIM_SLEEPY_3_FPS,    true, false },
};

// Sleeping face: a still frame of an existing animation (no new artwork).
// The awake resting faces depend on the mood (POSES in MochiBehavior.h).
static const AnimId POSE_SLEEP_ANIM = ANIM_RELAXED;   // calm closed eyes "- -"
static const uint16_t POSE_SLEEP_FRAME = 32;
