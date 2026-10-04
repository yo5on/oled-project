// =====================================================
// MOCHI EXPRESSIONS — what each of the 35 animations means
//
// An expression is an animation, or the part of a long clip that carries one
// expression (many long clips hold several expressions and long calm stretches).
// Frame ranges were measured on the frames themselves; no artwork is changed.
// ANGRY_2 frames 30-74 are not used: the source GIF has a camera-glow halo there that
// becomes ragged, flickering edges in 1 bit (looks blurry on the OLED, smears morphs).
// Tags say which feelings an expression can show, level how strong it is
// (1 mild, 2 clear, 3 strong); rare ones only appear when their condition holds.
// weight: calm faces that fit many moods are chosen a little less often each.
// =====================================================

#pragma once

#include <stdint.h>
#include "MochiAnimations.h"

enum ExprTag : uint16_t {
  T_IDLE      = 1 << 0,    // calm, neutral
  T_HAPPY     = 1 << 1,
  T_AFFECTION = 1 << 2,
  T_PLAYFUL   = 1 << 3,
  T_MISCHIEF  = 1 << 4,
  T_CURIOUS   = 1 << 5,
  T_SURPRISE  = 1 << 6,
  T_EXCITED   = 1 << 7,
  T_ANNOYED   = 1 << 8,
  T_ANGRY     = 1 << 9,
  T_SAD       = 1 << 10,
  T_SLEEPY    = 1 << 11,
  T_OVERSTIM  = 1 << 12,   // too much at once: flustered, wincing, dizzy
};

// Conditions for the special moments (checked by MochiBehavior::rareOk)
enum RareKind : uint8_t {
  R_NONE,
  R_FURIOUS,     // annoyance has built up very high
  R_SCREAM,      // extreme reaction: very excited and upset
  R_CRYING,      // strong sadness
  R_MISCHIEF,    // playful and happy: a mischievous grin
  R_SPITE,       // very mischievous, or seething
  R_DIZZY,       // overstimulated by many presses
  R_GLITCH,      // very curious after a quiet while
};

// Animations shown with edge cleanup (MochiPlayer::setCleanup): their stored frames have
// lone pixels and one-pixel holes along their strokes (noise pixels per frame, shown
// range: WORRIED 10.2 -> 1.2, FURIOUS 14.0 -> 2.6, ANNOYED 12.1 -> 1.7, EVIL 14.0 -> 2.5,
// ANGRY_2 6.8 -> 0.5, LAUGH (hearts) 10.2 -> 0.2; motion and thin marks unchanged).
// Not cleaned: CRYING, MOCHI_26, SCREAM (it would eat small real details), EVIL_GRIN
// (its stripes are the artwork), SQUINT and the rest (already clean).
static const AnimId CLEANED_ANIMS[] = { ANIM_MOCHI_29, ANIM_FURIOUS, ANIM_ANNOYED, ANIM_EVIL, ANIM_ANGRY_2, ANIM_LAUGH };
// Animations whose source video was interlaced: 1-pixel dark rows run through their solid
// shapes (EVIL_GRIN: up to 330 pixels per frame, visible as broken horizontal lines on the
// OLED). They are closed before the edge cleanup (MochiPlayer::setScanlineFill).
static const AnimId SCANLINE_ANIMS[] = { ANIM_EVIL_GRIN };

enum ExprId : uint8_t {
  E_CALM, E_BLINK, E_GLANCE, E_LOOKAROUND, E_BORED, E_DROOP, E_YAWN,
  E_SMILE, E_HAPPY3, E_BEAM, E_GRIN, E_SMUG,
  E_UWU, E_KISS, E_HEARTS, E_SHY,
  E_CHEEKY, E_XD, E_EVILGRIN, E_EVIL,
  E_PEEK, E_SWEAT, E_WINCE, E_BUG,
  E_SURPRISED, E_SHOCK, E_CHATTER, E_SCREAM, E_DIZZY,
  E_DEADPAN, E_WORRIED, E_SQUINT, E_ANNOYED, E_SQUEEZE, E_ANGRY3, E_FURIOUS,
  E_GLOOM, E_LOOKAWAY, E_TEARY, E_SOB, E_CRYING,
  E_COUNT,
  E_NONE = 0xFF
};

// Short expressions that are not repeated: their last frame does not lead back to the
// first, so a repeat would be a hard cut (BLINK: eyes jump across the screen; SQUINT:
// squeezed shut -> wide open; PEEK, HAPPY3: a different pose) or adds nothing but a
// longer still image (GLANCE: almost no motion; BEAM, XD: near-still parts of long clips;
// WORRIED: its brows pop back at the restart)
static const ExprId PLAY_ONCE[] = { E_BLINK, E_BEAM, E_XD, E_SQUINT, E_PEEK, E_HAPPY3, E_GLANCE, E_WORRIED };

struct Expression {
  const char* name;
  AnimId anim;
  uint8_t from, to;      // frames of the animation (to = 0xFF: its last frame)
  uint16_t tags;
  uint8_t level;
  RareKind rare;
  uint8_t weight;        // how often it is chosen among fitting ones (10 = normal)
};

static const uint8_t LAST = 0xFF;

static const Expression EXPRESSIONS[E_COUNT] = {
  // calm / idle / sleepy
  { "CALM",       ANIM_HAPPY,       0, 30, T_IDLE,                          1, R_NONE, 10 },   // f31-63 almost still, f64: sudden shift
  { "BLINK",      ANIM_BLINK,       0, LAST, T_SLEEPY,                      1, R_NONE, 10 },
  { "GLANCE",     ANIM_MOCHI_10,    0, LAST, T_IDLE | T_CURIOUS,            1, R_NONE, 7 },
  { "LOOKAROUND", ANIM_RELAXED,    24, 74, T_IDLE | T_CURIOUS,              1, R_NONE, 5 },
  { "BORED",      ANIM_ANGRY,      14, 40, T_SLEEPY | T_IDLE,               1, R_NONE, 6 },   // f13: one-frame swollen in-between
  { "DROOP",      ANIM_SLEEPY_3,    0, 2,  T_SLEEPY,                        2, R_NONE, 10 },
  { "YAWN",       ANIM_RELAXED,    15, 22, T_SLEEPY,                        2, R_NONE, 10 },
  // happy
  { "SMILE",      ANIM_SMILE,       0, LAST, T_HAPPY | T_IDLE,              1, R_NONE, 10 },
  { "HAPPY3",     ANIM_HAPPY_3,     0, LAST, T_HAPPY,                       2, R_NONE, 10 },
  { "BEAM",       ANIM_CONTENT,    54, 63, T_HAPPY,                         2, R_NONE, 10 },   // f64: eyes already open again
  { "GRIN",       ANIM_LOVE,        0, 43, T_HAPPY | T_AFFECTION,           3, R_NONE, 10 },
  { "SMUG",       ANIM_PROUD,      36, 74, T_HAPPY | T_AFFECTION,           2, R_NONE, 10 },
  // affection
  { "UWU",        ANIM_UWU,         0, LAST, T_AFFECTION | T_PLAYFUL,       2, R_NONE, 10 },
  { "KISS",       ANIM_KISS,        1, LAST, T_AFFECTION,                   3, R_NONE, 10 },   // f0: smeared cut-in frame of the source montage
  { "HEARTS",     ANIM_LAUGH,      23, 34, T_AFFECTION,                     3, R_NONE, 10 },   // f21-22, f36-39: noisy dissolves; f35: hearts already shrinking; f40+: a kiss face
  { "SHY",        ANIM_EMBARRASSED, 0, 41, T_AFFECTION | T_PLAYFUL,       1, R_NONE, 10 },   // f42: specks of the dissolve that follows
  // playful / mischief
  { "CHEEKY",     ANIM_HAPPY_2,     1, 47, T_PLAYFUL,                       1, R_NONE, 10 },
  { "XD",         ANIM_FRUSTRATED,  0, 7,  T_PLAYFUL | T_EXCITED,         2, R_NONE, 10 },   // f8-10: squash and eyes closing (held mid-motion)
  { "EVIL_GRIN",  ANIM_EVIL_GRIN,   1, LAST, T_MISCHIEF | T_PLAYFUL,      3, R_MISCHIEF, 10 },   // f0: sparse fade-in frame
  { "EVIL",       ANIM_EVIL,        0, 3,  T_MISCHIEF | T_ANGRY,        3, R_SPITE, 10 },   // f4-5: dissolve into a different clip of the montage
  // curious
  { "PEEK",       ANIM_MOCHI_11,    0, LAST, T_CURIOUS,                     2, R_NONE, 10 },
  { "SWEAT",      ANIM_MOCHI_26,    0, 6,  T_CURIOUS | T_SAD,             1, R_NONE, 6 },    // f7: cut-out frame into the next clip (KISS)
  { "WINCE",      ANIM_CONFUSED_2,  0, 38, T_CURIOUS | T_OVERSTIM,        2, R_NONE, 10 },
  { "GLITCH",     ANIM_BUG,         0, LAST, T_CURIOUS,                     3, R_GLITCH, 10 },
  // surprise / excitement
  { "SURPRISED",  ANIM_SURPRISED,   0, LAST, T_SURPRISE | T_CURIOUS | T_EXCITED, 2, R_NONE, 10 },
  { "SHOCK",      ANIM_ANGRY_2,     3, 14, T_SURPRISE | T_OVERSTIM,       3, R_NONE, 10 },   // f2 / f15-16: fragments of the eyes changing shape
  { "CHATTER",    ANIM_CONTENT,    24, 52, T_EXCITED | T_HAPPY,           2, R_NONE, 10 },   // f53: striped dissolve frame
  { "SCREAM",     ANIM_SCREAM,      0, LAST, T_EXCITED | T_OVERSTIM | T_ANGRY, 3, R_SCREAM, 10 },
  { "DIZZY",      ANIM_DIZZY,       0, LAST, T_OVERSTIM | T_EXCITED,        3, R_DIZZY, 10 },
  // annoyed / angry
  { "DEADPAN",    ANIM_FRUSTRATED, 35, 51, T_ANNOYED,                     1, R_NONE, 10 },   // f34 / f52-53: eyes closing / opening
  { "WORRIED",    ANIM_MOCHI_29,    0, LAST, T_ANNOYED | T_SAD,           1, R_NONE, 10 },
  { "SQUINT",     ANIM_SQUINT,      0, LAST, T_ANNOYED | T_SLEEPY,        1, R_NONE, 10 },
  { "ANNOYED",    ANIM_ANNOYED,     0, 6,  T_ANNOYED,                       2, R_NONE, 10 },   // f7: dissolve frame of the source video
  { "SQUEEZE",    ANIM_DETERMINED,  0, 22, T_ANNOYED | T_OVERSTIM,        2, R_NONE, 10 },   // f23-26: eyes already sliding up (held mid-motion)
  { "ANGRY_3",    ANIM_ANGRY_3,     0, 5,  T_ANNOYED | T_ANGRY,           3, R_NONE, 10 },   // f6: dissolve frame of the source video
  { "FURIOUS",    ANIM_FURIOUS,     0, LAST, T_ANGRY,                       3, R_FURIOUS, 10 },
  // sad
  { "GLOOM",      ANIM_EXCITED_2,  17, 44, T_SAD,                         1, R_NONE, 10 },   // f45-74: 2 s with almost no change (looked stuck)
  { "LOOKAWAY",   ANIM_DETERMINED, 27, 42, T_SAD,                         1, R_NONE, 10 },   // f43-44: full white block (a flash)
  { "TEARY",      ANIM_ANGRY_2,    19, 29, T_SAD,                         2, R_NONE, 10 },   // later frames: camera glow halo in the source
  { "SOB",        ANIM_SLEEPY_3,    4, 51, T_SAD,                         3, R_NONE, 10 },   // f3: closing-eyes in-between; f52: cross-fade smear
  { "CRYING",     ANIM_CRYING,      0, LAST, T_SAD,                         3, R_CRYING, 10 },
};
