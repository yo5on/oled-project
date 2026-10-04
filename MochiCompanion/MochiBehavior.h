// =====================================================
// MOCHI BEHAVIOR — a small character that feels something and shows it
//
//   EVENT ─► FEELINGS ─► MOOD ─► INTENT ─► EXPRESSION ─► sequence ─► MochiPlayer ─► MochiMorph
//
// EVENT: button 1 (affection / pet), button 2 (poke / tease), repeated and rapid
//   presses, and time passing (quiet, boredom, tiredness).
// FEELINGS (MochiEmotion.h): happiness, affection, playfulness, annoyance,
//   curiosity, energy, excitement and a grudge. Events move them; they drift back
//   over seconds to minutes.
// MOOD: derived from the feelings, with inertia: at least MOOD_MIN_DWELL_MS in a
//   mood, and a new mood must clearly win. Every change is logged with its reason.
// INTENT: react to a press (context ladders: affection grows, annoyance escalates),
//   a short episode of related expressions, a single expression, or a calm moment.
// EXPRESSION (MochiExpressions.h): an animation or segment whose tags fit the
//   intent; randomness only chooses among fitting ones (70% the mood, 20% a
//   neighbouring mood, 10% calm), avoiding repeats, with the starvation boost.
//   Special moments (FURIOUS, SCREAM, CRYING, EVIL...) need their condition.
// Animations chain: short pause on the last frame, or a calm moment on the mood's
// resting face (with blinks), then a smooth morph into the next expression.
// When tired and left alone Mochi gets sleepy, then falls asleep (the OLED stays on).
// After 5-15 minutes it wakes up on its own, slowly and groggily; a button wakes it at
// once. Every decision is logged with a reason code.
// =====================================================

#pragma once

#include <stdint.h>
#include <stdio.h>
#include "MochiPlayer.h"
#include "MochiEmotion.h"
#include "MochiExpressions.h"

#define MOCHI_COUNT(a) (sizeof(a) / sizeof((a)[0]))

// ---- Time ----
#ifdef MOCHI_FAST_TIMING
static const uint32_t TIME_SCALE = 5;    // HARDWARE TEST ONLY: feelings and moods change 5x faster
#else
static const uint32_t TIME_SCALE = 1;
#endif
static const uint32_t TICK_MS             = 1000 / TIME_SCALE;          // one "second" of feelings
static const uint32_t MOOD_MIN_DWELL_MS   = 20000 / TIME_SCALE;         // shortest time in a mood
static const int16_t  MOOD_CHANGE_MARGIN  = 100;                         // a new mood must win by this
static const int16_t  SLEEP_ENERGY        = 150;                         // tired enough to fall asleep ...
static const uint32_t SLEEP_MIN_IDLE_MS   = 180000UL / TIME_SCALE;       // ... and left alone this long
static const uint32_t SLEEP_AFTER_MS      = 20UL * 60 * 1000 / TIME_SCALE;  // asleep at the latest
static const uint32_t RARE_COOLDOWN_MS    = 120000UL / TIME_SCALE;       // a special moment stays special
static const uint32_t RARE_SPACING_MS     = 45000UL / TIME_SCALE;        // ... and two never come back to back
static const uint32_t GREETING_IDLE_MS    = 180000UL / TIME_SCALE;       // first press after this: a greeting

// ---- Buttons ----
static const uint32_t REACTION_COOLDOWN_MS = 600;       // presses inside this only count
static const uint32_t PRESS_WINDOW_MS     = 3000;       // window for counting rapid presses
static const uint32_t STREAK_GAP_MS       = 8000;       // presses closer than this build a streak
static const uint8_t  RAPID_OVERSTIM      = 4;          // presses within PRESS_WINDOW_MS: overwhelmed
static const uint8_t  RAPID_MELTDOWN      = 6;          // ... "STOP!"
static const uint32_t STOP_COOLDOWN_MS    = 3000;       // after "STOP!", presses do not get a reaction
static const uint32_t AFTERGLOW_MS        = 8000;       // after a reaction, idle stays close to it

// ---- Blinks / rhythm ----
static const uint32_t BLINK_MIN_MS        = 2000;      // time between blinks (a blink waits for
static const uint32_t BLINK_MAX_MS        = 6000;      // the next pause or resting moment)
static const uint16_t REST_REBLINK_MS     = 2000;      // a long resting moment blinks again after this
static const uint16_t REST_STILL_MAX_MS   = 2200;      // resting faces that cannot blink rest at most this long
static const uint8_t  DOUBLE_BLINK_PCT    = 20;
static const uint16_t BLINK_CLOSED_MS     = 110;        // closed-eye frame of a frame blink
static const uint16_t BLINK_OPEN_MS       = 140;        // open again between a double blink
static const uint16_t EYELID_TWICE_MS     = 380;        // eyelid double blink: the second one starts this later
static const uint16_t SWAY_STEP_MS        = 90;         // a resting face without eyelids: frame by frame ...
static const uint16_t SWAY_HOLD_MS        = 220;        // ... out to its sway frame, a moment there, and back

// ---- Transitions: universal expression morph (MochiMorph.h) ----
static const uint16_t MORPH_MIN_MS        = 120;  // autonomous switches: 120..220 ms
static const uint16_t MORPH_MAX_MS        = 220;
static const uint16_t MORPH_REACT_MS      = 105;  // button reactions: quick morph (2 in-between frames), starts at once
static const uint16_t SETTLE_EMOTION_MS   = 180;  // last frame stays a moment after an expression
static const uint16_t SETTLE_SLEEP_MS     = 250;  // squeezed-shut eyes before the eyes close
static const uint16_t EYES_OPEN_BOOT_MS   = 220;  // closed eyes at boot, then the first expression
static const uint8_t  CONTRAST_AWAKE      = 0xCF; // SSD1306 brightness, set once at boot (also asleep)

// ---- Sleeping ----
// Falling asleep: SLEEPY_3 closes its eyes (frames 0 -> 1 -> 2). Asleep, Mochi drifts
// between three closed-eye faces of existing animations (light, deep, deepest sleep) and
// breathes: a slow partial morph toward a nearby closed-eye frame and back, never all the
// way, with varied depth and timing; now and then a tiny twitch or a deeper sigh. The eyes
// never open (every frame used, and every morph between them, has closed eyes).
static const AnimId   SLEEP_ANIM          = ANIM_SLEEPY_3;
static const uint8_t  SLEEP_FRAME         = 2;    // falling asleep ends here: curved, closed lids
static const uint8_t  DOZE_FRAME          = 1;    // lids a little thicker (still closed)
enum SleepDepth : uint8_t { SLEEP_LIGHT, SLEEP_DEEP, SLEEP_DEEPEST };
struct SleepFace {
  AnimId anim; uint8_t frame;                 // the face
  AnimId toward; uint8_t towardFrame;         // breathing drifts part of the way toward this
  uint8_t depthMin, depthMax;                 // how far (of 256)
  uint16_t inMin, inMax, outMin, outMax;      // breathing in / out (ms)
  uint16_t restMin, restMax;                  // pause between breaths (ms)
  uint16_t staySMin, staySMax;                // how long this sleep depth lasts (s)
  const char* name;
};
static const SleepFace SLEEP_FACES[3] = {
  { ANIM_SLEEPY_3, 2, ANIM_SLEEPY_3, 1, 45, 85,  1300, 2000, 1600, 2600, 1200, 3500, 25, 70, "light" },
  { ANIM_RELAXED, 32, ANIM_SLEEPY_3, 2, 40, 70,  1800, 2600, 2200, 3200, 2000, 5000, 40, 120, "deep" },
  { ANIM_ANGRY,   27, ANIM_RELAXED, 32, 40, 70,  2200, 3000, 2600, 3600, 3000, 6000, 10, 25, "deepest" },
};
static const uint8_t  BREATH_STEPS        = 8;    // in-between frames of a breath (>= 160 ms apart)
static const uint8_t  SLEEP_TWITCH_PCT    = 7;    // per breath: a tiny twitch ...
static const uint8_t  SLEEP_SIGH_PCT      = 8;    // ... or a deeper sigh
static const uint8_t  SLEEP_DEEPEST_PCT   = 35;   // leaving deep sleep: sometimes deeper still
static const uint16_t SLEEP_FACE_MIN_MS   = 1800, SLEEP_FACE_MAX_MS = 2600;  // drifting between depths
// Waking up on its own: after SLEEP_MIN..MAX asleep (random, mostly mid-range), between
// two breaths: the lids come back to light sleep, lift slowly (SLEEPY_3 frames 2 -> 1 -> 0),
// a slow blink, a yawn, then a heavy-lidded groggy face; groggy for a while, then normal.
static const uint32_t SLEEP_MIN_MS        = 5UL * 60 * 1000 / TIME_SCALE;
static const uint32_t SLEEP_MAX_MS        = 15UL * 60 * 1000 / TIME_SCALE;
static const uint16_t GROGGY_MIN_MS       = 25000, GROGGY_MAX_MS = 45000;   // only calm, sleepy faces
static const AnimId   GROGGY_ANIM         = ANIM_ANGRY;                      // heavy lids
static const uint8_t  GROGGY_FRAME        = 20;

// ---- Flow between actions ----
// After an action the next one usually follows after a short pause (the last frame
// stays); sometimes Mochi rests on its mood's face first. Both vary per mood a little.
struct MoodFlow {
  uint8_t restPct;              // chance of a resting moment instead of chaining on
  uint16_t pauseMin, pauseMax;  // short pause before the next animation (ms)
  uint16_t restMin, restMax;    // resting moment on the mood's face (ms)
};
static const MoodFlow MOOD_FLOW[MOOD_COUNT] = {
  { 30, 350, 800, 1500, 3500 },   // NEUTRAL: calm
  { 20, 300, 650, 1000, 2500 },   // HAPPY
  { 25, 400, 900, 1200, 3000 },   // CURIOUS: looks a little longer
  { 15, 300, 550, 1000, 2000 },   // PLAYFUL: quick
  { 12, 250, 500, 800, 1800 },    // EXCITED: quickest
  { 20, 300, 550, 1000, 2200 },   // ANNOYED
  { 35, 500, 900, 1800, 3500 },   // SAD: slow
  { 40, 600, 1000, 2000, 4000 },  // SLEEPY: slowest
  { 25, 350, 700, 1500, 3000 },   // AFFECTIONATE: soft
};
static const uint16_t PAUSE_BLINK_DELAY_MS = 60;       // a blink in a pause starts this late ...
static const uint16_t PAUSE_BLINK_MS      = 220;       // ... and only if the pause lasts this long still

// What each mood expresses: its own tags, and the neighbouring feelings it drifts into
static const uint16_t MOOD_TAGS[MOOD_COUNT] = {
  T_IDLE, T_HAPPY, T_CURIOUS, T_PLAYFUL | T_MISCHIEF, T_EXCITED | T_SURPRISE,
  T_ANNOYED | T_ANGRY, T_SAD, T_SLEEPY, T_AFFECTION };
static const uint16_t MOOD_NEIGHBORS[MOOD_COUNT] = {
  T_CURIOUS | T_HAPPY,                      // NEUTRAL
  T_PLAYFUL | T_AFFECTION | T_CURIOUS,      // HAPPY
  T_IDLE | T_SURPRISE | T_PLAYFUL,          // CURIOUS
  T_HAPPY | T_EXCITED | T_AFFECTION,        // PLAYFUL
  T_HAPPY | T_PLAYFUL,                      // EXCITED
  T_IDLE | T_SAD,                           // ANNOYED
  T_SLEEPY | T_IDLE,                        // SAD
  T_IDLE | T_SAD,                           // SLEEPY
  T_HAPPY | T_PLAYFUL };                    // AFFECTIONATE
// Feelings a mood does not show while idle (an annoyed Mochi does not smile idly, a
// happy one does not look teary); reactions and lingering feelings are not limited
static const uint16_t MOOD_FORBID[MOOD_COUNT] = {
  T_ANGRY,                                  // NEUTRAL
  T_SAD | T_ANGRY | T_ANNOYED,              // HAPPY
  T_ANGRY,                                  // CURIOUS
  T_SAD | T_ANNOYED,                        // PLAYFUL
  T_SAD,                                    // EXCITED
  T_HAPPY | T_AFFECTION | T_PLAYFUL,        // ANNOYED
  T_HAPPY | T_PLAYFUL | T_MISCHIEF,         // SAD
  T_EXCITED | T_ANGRY,                      // SLEEPY
  T_SAD | T_ANGRY | T_ANNOYED };            // AFFECTIONATE
static const char* const MOOD_IDLE_REASON[MOOD_COUNT] = {
  "CALM_IDLE", "HAPPY_IDLE", "CURIOUS_IDLE", "PLAYFUL_IDLE", "EXCITED_IDLE",
  "ANNOYED_IDLE", "SAD_IDLE", "SLEEPY_IDLE", "AFFECTION_IDLE" };
static const char* const MOOD_EPISODE_REASON[MOOD_COUNT] = {
  "CALM_EPISODE", "HAPPY_EPISODE", "CURIOUS_EPISODE", "PLAYFUL_EPISODE", "EXCITED_EPISODE",
  "ANNOYED_EPISODE", "SAD_EPISODE", "SLEEPY_EPISODE", "AFFECTION_EPISODE" };

// Starvation boost: once an expression is eligible (fits the moment), its weight grows
// the longer its animation has not played. No animation is forced.
struct StarveStep { uint32_t afterMs; uint16_t pct; };
static const StarveStep STARVE_BOOST[] = {
  { 30UL * 60 * 1000, 220 }, { 20UL * 60 * 1000, 160 }, { 10UL * 60 * 1000, 125 } };   // else 100%

// ---- Resting faces: still frames of existing animations, two per mood ----
// blink: BLINK_EYELID: the eyelids of the face itself close and open (MochiPlayer::blink,
//        a morph of the image on screen: no cut);
//        BLINK_FRAME: the clip's own closed-eye frame (blinkAnim, blinkFrame) briefly;
//        BLINK_SWAY: no eyelids (outlined eyes, brows): the clip's own frames step out to a
//        nearby frame (blinkFrame, at most 6 away) and back, so the face stays alive with
//        its real motion (a morph between such close frames would smear fine details);
//        BLINK_NONE: nothing (a shorter rest instead).
// (A closed-eye frame from a different clip is never cut in: its eyes sit elsewhere.)
enum BlinkKind : uint8_t { BLINK_EYELID, BLINK_FRAME, BLINK_SWAY, BLINK_NONE };

struct Pose {
  AnimId anim;
  uint8_t frame;
  BlinkKind blink;
  AnimId blinkAnim;
  uint8_t blinkFrame;
};

// Closed eyes of the emote GIFs' art style (the slow blink when waking up)
#define CLOSED_GIF  ANIM_RELAXED, 32
#define EYELID      BLINK_EYELID, ANIM_BLINK, 0

static const Pose POSES[MOOD_COUNT][2] = {
  { { ANIM_BLINK, 0, EYELID },                      { ANIM_HAPPY, 0, EYELID } },                          // NEUTRAL
  { { ANIM_SMILE, 0, EYELID },                      { ANIM_HAPPY_2, 60, EYELID } },                       // HAPPY
  { { ANIM_SURPRISED, 0, EYELID },                  { ANIM_EMBARRASSED, 60, EYELID } },                   // CURIOUS
  { { ANIM_HAPPY_3, 0, EYELID },                    { ANIM_HAPPY_2, 20, BLINK_FRAME, ANIM_HAPPY_2, 18 } }, // PLAYFUL
  { { ANIM_SURPRISED, 3, EYELID },                  { ANIM_ANGRY_2, 25, BLINK_SWAY, ANIM_ANGRY_2, 29 } }, // EXCITED
  { { ANIM_ANNOYED, 3, BLINK_SWAY, ANIM_ANNOYED, 6 }, { ANIM_FRUSTRATED, 45, BLINK_FRAME, ANIM_FRUSTRATED, 52 } }, // ANNOYED
  { { ANIM_MOCHI_29, 4, BLINK_SWAY, ANIM_MOCHI_29, 7 }, { ANIM_EXCITED_2, 40, EYELID } },                 // SAD (worried, gloomy)
  { { ANIM_FRUSTRATED, 45, BLINK_FRAME, ANIM_FRUSTRATED, 52 }, { ANIM_ANGRY, 20, EYELID } },            // SLEEPY
  { { ANIM_LOVE, 20, BLINK_SWAY, ANIM_LOVE, 23 },   { ANIM_UWU, 0, BLINK_FRAME, ANIM_UWU, 1 } },          // AFFECTIONATE
};

// ---- Button ladders: each press of a streak goes one step further ----
struct Rung { ExprId e[5]; uint8_t n; };
static const Rung AFFECTION_LADDER[] = {        // button 1, repeated: warmer and warmer
  { { E_SMILE, E_HAPPY3 }, 2 },
  { { E_HAPPY3, E_BEAM }, 2 },
  { { E_GRIN, E_UWU }, 2 },
  { { E_KISS, E_SMUG }, 2 },
  { { E_HEARTS, E_KISS }, 2 },
  { { E_UWU, E_HEARTS, E_SMUG, E_GRIN }, 4 } };
static const Rung ANNOYANCE_LADDER[] = {        // button 2 when not playing: by annoyance
  { { E_SURPRISED, E_PEEK }, 2 },               //   < 250
  { { E_WORRIED, E_DEADPAN }, 2 },              //   < 400
  { { E_SQUINT, E_DEADPAN }, 2 },               //   < 550
  { { E_ANNOYED, E_SQUEEZE }, 2 },              //   < 700
  { { E_ANGRY3, E_ANNOYED }, 2 },               //   < 850
  { { E_FURIOUS, E_ANGRY3, E_SQUEEZE, E_EVIL }, 4 } };  //   >= 850 (FURIOUS / EVIL only when their condition holds)
static const Rung TEASE_PLAY = { { E_XD, E_CHEEKY, E_UWU, E_EVILGRIN, E_EVIL }, 5 };
static const Rung CURIOUS_POKE = { { E_SURPRISED, E_WINCE, E_BUG }, 3 };
static const Rung GRUDGE = { { E_DEADPAN, E_SQUINT, E_WORRIED }, 3 };
static const Rung RECOVERY = { { E_SHY, E_TEARY, E_SURPRISED }, 3 };
static const Rung SLEEPY_PET = { { E_BLINK, E_SQUINT, E_SMILE }, 3 };
static const Rung GRUMPY_POKE = { { E_SQUINT, E_DEADPAN, E_ANNOYED }, 3 };
static const Rung GREETING = { { E_SURPRISED, E_SMILE }, 2 };
static const Rung OVERSTIM_PET = { { E_SHY, E_WINCE, E_DIZZY }, 3 };
static const Rung OVERSTIM_POKE = { { E_SQUEEZE, E_WINCE, E_SHOCK, E_DIZZY }, 4 };


class MochiBehavior {
public:
  enum State : uint8_t { IDLE, ACTING, FALLING_ASLEEP, ASLEEP, WAKING, WAKING_NATURALLY };

  typedef uint32_t (*RandFn)(uint32_t n);                          // 0 .. n-1
  typedef void (*LogFn)(const char* event, const char* detail);

  MochiBehavior(MochiPlayer& player, RandFn rand, LogFn log = nullptr)
    : player_(player), rand_(rand), log_(log) {}

  // verbose: also log animation starts/ends, pauses, rests, blinks and feelings
  void setVerbose(bool v) { verbose_ = v; }

  // Boot: a personality and first feelings; Mochi opens its eyes into its first expression
  void begin(uint32_t now) {
    now_ = now;
    for (uint8_t i = 0; i < ANIM_COUNT; i++) lastPlayed_[i] = now;   // starvation counts from boot
    for (uint8_t i = 0; i <= R_GLITCH; i++) lastRare_[i] = now - RARE_COOLDOWN_MS;
    lastAnyRare_ = now - RARE_SPACING_MS;
    lastInteraction_ = now;
    lastReactionMs_ = now - REACTION_COOLDOWN_MS;   // the very first press always reacts
    lastTick_ = now;
    player_.screen().setContrast(CONTRAST_AWAKE);
    for (uint8_t i = 0; i < sizeof(CLEANED_ANIMS) / sizeof(CLEANED_ANIMS[0]); i++) player_.setCleanup(CLEANED_ANIMS[i], true);
    for (uint8_t i = 0; i < sizeof(SCANLINE_ANIMS) / sizeof(SCANLINE_ANIMS[0]); i++) player_.setScanlineFill(SCANLINE_ANIMS[i], true);
    for (const ClosedFrameFix& c : CLOSED_FRAME_FIXES) player_.setClosedFrame(c.anim, c.frame, c.from, c.open);

    Personality& p = emo_.p;
    p.happy = (int16_t)(100 + rand_(250));
    p.affection = (int16_t)(300 + rand_(250));
    p.playful = (int16_t)(50 + rand_(200));
    p.curious = (int16_t)(200 + rand_(250));
    emo_.happy = (int16_t)(p.happy + rand_(200));
    emo_.affection = p.affection;
    emo_.playful = (int16_t)(p.playful + rand_(250));
    emo_.curious = (int16_t)(p.curious + rand_(200));
    emo_.energy = (int16_t)(700 + rand_(250));
    emo_.excite = 300;
    snprintf(text_, sizeof(text_), "rest levels: happy %d affection %d playful %d curious %d",
             p.happy / 10, p.affection / 10, p.playful / 10, p.curious / 10);
    log("PERSONALITY", text_);

    mood_ = bestMood();
    moodSince_ = now;
    pose_ = (uint8_t)rand_(2);
    snprintf(text_, sizeof(text_), "%s (BOOT) | %s", MOOD_NAMES[mood_], feelings());
    log("INITIAL MOOD", text_);

    ExprId e = pickExpr(MOOD_TAGS[mood_], 2, false);
    beginSequence();
    const Expression& x = EXPRESSIONS[e];
    const ClosedEyes c = closedFor(x.anim);
    queueStill(c.anim, c.frame, EYES_OPEN_BOOT_MS);           // eyes closed ...
    queueMorph(x.anim, x.from, morphMs());                     // ... morph open ...
    queueExpr(e, loopsFor(e));                                 // ... into the expression
    decision("BOOT", e);
    run("BOOT", SETTLE_EMOTION_MS, now);
  }

  // Call every loop()
  void update(uint32_t now) {
    now_ = now;
    player_.update(now);
    tickFeelings(now);

    if (state_ == ASLEEP) {
      breathe(now);
      return;
    }

    if (state_ != IDLE) {                    // a sequence is playing
      if (stepBusy(now)) return;
      if (stepIsAnim_) logv("ANIMATION END", MOCHI_ANIMS[player_.current()].name);
      stepIsAnim_ = false;
      if (seqPos_ < seqLen_) {
        startStep(now);
        return;
      }
      if (state_ == FALLING_ASLEEP) {
        sleep(now);
      } else if (state_ == WAKING_NATURALLY) {    // awake: a groggy moment on the sleepy face
        state_ = ACTING;
        restMs_ = (uint16_t)natural(3000, 6000);
        returnToRest(now);
      } else if (!returning_) {
        afterAction(now);                    // pause, calm moment, or sleep
      } else {
        rest(now);
      }
      return;
    }

    // IDLE: a short pause on the last frame, or a calm moment on the mood's face
    if (sleepDue(now)) {
      fallAsleep(now);
      return;
    }
    if ((int32_t)(now - nextAction_) >= 0) {
      nextAction(now);
    } else if (blinkPending_ && (int32_t)(now - nextBlink_) >= 0) {
      blinkPending_ = false;
      bool twice = !blinkAgain_ && rand_(100) < DOUBLE_BLINK_PCT;
      blinkAgain_ = false;
      blink(twice, now);
    } else if (resting_ && !blinkPending_ && !player_.busy() && restFace().blink != BLINK_NONE && now - lastBlink_ >= REST_REBLINK_MS &&
               (int32_t)(nextAction_ - now) >= 600) {
      blink(false, now);                     // a long resting moment: blink again (never a frozen face)
    } else if (!resting_ && !player_.busy() && (int32_t)(now - nextBlink_) >= 0 &&
               now - pauseStart_ >= PAUSE_BLINK_DELAY_MS && (int32_t)(nextAction_ - now) >= PAUSE_BLINK_MS) {
      nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);     // a blink in the pause, on any face
      if (player_.blink(now)) {
        lastBlink_ = now;
        log("BLINK", "eyelids");
      }
    }
  }

  // Back on screen after something else had it (Gallery Mode; update() was not called
  // meanwhile): Mochi's last image is shown again and everything carries on from where it
  // was (mood, feelings, sequence, sleep). The time away was spent with the user, so it
  // does not count as being alone (no falling asleep at once, no "long time no see").
  void resume(uint32_t now) {
    now_ = now;
    lastInteraction_ = now;
    player_.redraw();
    log("RESUME", "back from another screen");
  }

  // A button press (0 = button 1: affection / pet, 1 = button 2: poke / tease)
  void onButton(uint8_t button, uint32_t now) {
    now_ = now;
    uint32_t idleBefore = now - lastInteraction_;
    lastInteraction_ = now;
    log(button == 0 ? "BUTTON 1" : "BUTTON 2", "pressed");

    // streaks of the same button; the other one's streak ends
    if (button == 0) {
      b1Streak_ = (b1Streak_ && now - lastB1_ <= STREAK_GAP_MS) ? b1Streak_ + 1 : 1;
      lastB1_ = now;
      b2Streak_ = 0;
    } else {
      b2Streak_ = (b2Streak_ && now - lastB2_ <= STREAK_GAP_MS) ? b2Streak_ + 1 : 1;
      lastB2_ = now;
      b1Streak_ = 0;
    }
    pressTimes_[pressHead_] = now;           // remember recent presses (either button)
    pressHead_ = (pressHead_ + 1) % PRESS_HISTORY;

    groggyUntil_ = now;                      // a press wakes Mochi up properly
    if (state_ == ASLEEP || state_ == FALLING_ASLEEP || state_ == WAKING_NATURALLY) {
      wake(button, now);                     // the press wins over waking up slowly
      return;
    }
    if ((int32_t)(stopUntil_ - now) > 0) {   // still upset after "STOP!"
      MochiEmotion::add(emo_.annoy, 30, 0, 1000);
      log("STOP_COOLDOWN", "still upset: press noted, no reaction");
      return;
    }

    int16_t annoyBefore = emo_.annoy, happyBefore = emo_.happy;
    uint8_t rapid = recentCount(now);
    bool play = button == 1 && playfulContext(annoyBefore);
    pressFeelings(button, play, annoyBefore, rapid);

    if (now - lastReactionMs_ < REACTION_COOLDOWN_MS && rapid < RAPID_MELTDOWN) {
      logv("BUTTON", "cooldown, counted only");
      return;
    }
    lastReactionMs_ = now;

    if (rapid >= RAPID_MELTDOWN) {
      meltdown(now);
      return;
    }
    ExprId e;
    const char* why;
    if (rapid >= RAPID_OVERSTIM) {           // too much at once
      e = pickRung(button == 0 ? OVERSTIM_PET : OVERSTIM_POKE);
      why = "OVERSTIMULATED";
    } else if (button == 0) {
      if (annoyBefore >= 400) { e = pickRung(GRUDGE); why = "GRUDGING_FORGIVENESS"; }
      else if (mood_ == SAD || happyBefore < -150) { e = pickRung(RECOVERY); why = "HAPPINESS_RECOVERY"; }
      else if (mood_ == SLEEPY || emo_.energy < 300) { e = pickRung(SLEEPY_PET); why = "SLEEPY_PET"; }
      else if (idleBefore >= GREETING_IDLE_MS) { e = pickRung(GREETING); why = "GREETING"; b1Streak_ = 1; }
      else {
        uint8_t step = b1Streak_ + (mood_ == AFFECTIONATE ? 1 : 0);
        uint8_t n = MOCHI_COUNT(AFFECTION_LADDER);
        e = pickRung(AFFECTION_LADDER[(step > n ? n : step) - 1]);
        why = "BUTTON1_AFFECTION";
      }
    } else {
      if (mood_ == SLEEPY || emo_.energy < 300) { e = pickRung(GRUMPY_POKE); why = "GRUMPY_POKE"; }
      else if (play) { e = pickRung(TEASE_PLAY); why = "BUTTON2_TEASE"; }
      else if (mood_ == CURIOUS && annoyBefore < 300) { e = pickRung(CURIOUS_POKE); why = "CURIOUS_POKE"; }
      else {
        int16_t a = emo_.annoy;
        uint8_t step = a < 250 ? 0 : a < 400 ? 1 : a < 550 ? 2 : a < 700 ? 3 : a < 850 ? 4 : 5;
        e = pickRung(ANNOYANCE_LADDER[step]);
        why = "ANNOYANCE_ESCALATION";
      }
    }
    if (EXPRESSIONS[e].rare != R_NONE) why = rareReason(EXPRESSIONS[e].rare, why);
    react(e, why, now);
  }

  Mood mood() const { return mood_; }
  State state() const { return state_; }
  const Pose& pose() const { return POSES[mood_][pose_]; }
  // The resting face on screen: chosen when Mochi went to rest (a mood change during the
  // rest must not make the blink act on a different face)
  const Pose& restFace() const { return restPose_ ? *restPose_ : pose(); }
  uint32_t morphs() const { return morphs_; }      // expression morphs started
  const MochiEmotion& feelingsState() const { return emo_; }
  ExprId lastExpression() const { return exprHist_[(exprHead_ + EXPR_HISTORY - 1) % EXPR_HISTORY]; }

private:
  static const uint8_t SEQ_MAX = 12;
  static const uint8_t HISTORY = 4;
  static const uint8_t EXPR_HISTORY = 4;
  static const uint8_t PRESS_HISTORY = 8;
  static const int16_t WAIT = -2;     // step: keep the image on screen, just wait
  static const int16_t MORPH = -3;    // step: morph to (id, loops = frame) over ms

  struct Step {
    AnimId id;
    uint8_t loops;      // animation: play this many times
    int16_t frame;      // >= 0: still frame ...; WAIT: show nothing new; MORPH: morph to (id, loops); -1: animation
    uint16_t ms;        // ... for this long
    uint8_t from, to;   // animation: frames to play
    uint8_t expr;       // animation: the expression it shows (E_NONE: sleep/wake/blink clips)
  };

  struct ClosedEyes {
    AnimId anim;
    uint8_t frame;
  };

  // Closed eyes in the art style of an animation (full-screen 250frames clips / emote GIFs)
  static ClosedEyes closedFor(AnimId id) {
    ClosedEyes gif = { ANIM_RELAXED, 32 }, clip = { ANIM_BLINK, 3 };
    return MOCHI_ANIMS[id].packed ? gif : clip;
  }

  // ---- Feelings over time, and the mood they lead to ----
  void tickFeelings(uint32_t now) {
    if (now - lastTick_ > 10 * TICK_MS) lastTick_ = now - TICK_MS;   // after a long gap: no catch-up burst
    while (now - lastTick_ >= TICK_MS) {
      lastTick_ += TICK_MS;
      uint32_t idleS = (now - awakeRef()) / 1000 * TIME_SCALE;
      emo_.tick(idleS, state_ == ASLEEP);
      if (state_ == IDLE || state_ == ACTING) updateMood(now);
    }
  }

  Mood bestMood() const {
    Mood best = NEUTRAL;
    for (uint8_t m = 0; m < MOOD_COUNT; m++)
      if (emo_.score((Mood)m) > emo_.score(best)) best = (Mood)m;
    return best;
  }

  void updateMood(uint32_t now) {
    Mood best = bestMood();
    if (best == mood_ || now - moodSince_ < MOOD_MIN_DWELL_MS) return;
    if (emo_.score(best) < emo_.score(mood_) + MOOD_CHANGE_MARGIN) return;
    changeMood(best, moodReason(best, now), now);
  }

  void changeMood(Mood m, const char* why, uint32_t now) {
    if (m != mood_) {
      snprintf(text_, sizeof(text_), "%s -> %s because %s | %s", MOOD_NAMES[mood_], MOOD_NAMES[m], why, feelings());
      log("MOOD CHANGE", text_);
      prevMood_ = mood_;
      moodChanged_ = true;
      pose_ = (uint8_t)rand_(2);
    }
    mood_ = m;
    moodSince_ = now;
  }

  // Why the feelings now favour mood m (the decisive event, or the passing of time)
  const char* moodReason(Mood m, uint32_t now) const {
    bool b1 = lastB1Set() && now - lastB1_ < 30000;
    bool b2 = lastB2Set() && now - lastB2_ < 30000;
    uint32_t idle = now - awakeRef();
    switch (m) {
      case ANNOYED:      return "ANNOYANCE_ESCALATION";
      case HAPPY:        return b1 ? "BUTTON1_AFFECTION" : b2 ? "BUTTON2_TEASE" : "HAPPINESS_RECOVERY";
      case AFFECTIONATE: return b1 ? "BUTTON1_AFFECTION" : "AFFECTION_GLOW";
      case PLAYFUL:      return b2 ? "BUTTON2_TEASE" : b1 ? "BUTTON1_AFFECTION" : "PLAYFUL_DRIFT";
      case EXCITED:      return "EXCITEMENT_SPIKE";
      case CURIOUS:      return idle > 60000 / TIME_SCALE ? "CURIOUS_IDLE" : "SOMETHING_NEW";
      case SAD:          return (b2 || emo_.grudge > 300) ? "HURT_FEELINGS" : "NEGLECTED";
      case SLEEPY:       return "SLEEPINESS_THRESHOLD";
      default:           return idle > 240000 / TIME_SCALE ? "BOREDOM" : "EMOTIONS_SETTLED";
    }
  }

  uint8_t intensity() const {                // 0..100: how strongly the mood is felt
    int16_t s = emo_.score(mood_);
    if (mood_ == NEUTRAL) s = 300;
    return (uint8_t)(s < 0 ? 0 : s > 1000 ? 100 : s / 10);
  }

  uint8_t intensityLevel() const {
    uint8_t i = intensity();
    return i >= 65 ? 3 : i >= 35 ? 2 : 1;
  }

  // ---- Buttons change the feelings first; the reaction follows from them ----
  bool playfulContext(int16_t annoyBefore) const {
    return annoyBefore < 300 && b2Streak_ <= 3 &&
           (mood_ == HAPPY || mood_ == PLAYFUL || mood_ == AFFECTIONATE || mood_ == EXCITED ||
            emo_.playful >= 400 || emo_.happy >= 300);
  }

  void pressFeelings(uint8_t button, bool play, int16_t annoyBefore, uint8_t rapid) {
    MochiEmotion& f = emo_;
    if (button == 0) {                       // affection: warmer, forgives, a bit less each time in a row
      uint8_t s = b1Streak_ > 6 ? 6 : b1Streak_;
      int16_t gain = (int16_t)(120 - 15 * (s - 1));
      if (annoyBefore >= 400) gain /= 2;     // still a little cross
      MochiEmotion::add(f.happy, gain, -1000, 1000);
      MochiEmotion::add(f.affection, 40, 0, 1000);
      MochiEmotion::add(f.annoy, -220, 0, 1000);
      MochiEmotion::add(f.playful, 30, 0, 1000);
    } else if (play) {                       // a tease while in a good mood: a game
      MochiEmotion::add(f.playful, 220, 0, 1000);
      MochiEmotion::add(f.happy, 40, -1000, 1000);
      MochiEmotion::add(f.annoy, 40, 0, 1000);
    } else {                                 // a poke: annoying, a bit more in a row or with a grudge
      uint8_t s = b2Streak_ > 5 ? 5 : b2Streak_;
      MochiEmotion::add(f.annoy, (int16_t)(100 + 15 * (s - 1) + f.grudge / 10), 0, 1000);
      MochiEmotion::add(f.happy, -70, -1000, 1000);
      MochiEmotion::add(f.playful, -100, 0, 1000);
    }
    MochiEmotion::add(f.curious, -200, 0, 1000);
    MochiEmotion::add(f.excite, (int16_t)(button == 0 ? 120 : 150), 0, 1000);
    MochiEmotion::add(f.energy, 20, 0, 1000);
    if (rapid >= RAPID_OVERSTIM) {           // too much at once
      MochiEmotion::add(f.excite, 150, 0, 1000);
      MochiEmotion::add(f.annoy, 60, 0, 1000);
    }
  }

  // "STOP!": shock, a scream (the extreme reaction), then tears; presses are ignored a moment
  void meltdown(uint32_t now) {
    MochiEmotion& f = emo_;
    MochiEmotion::add(f.annoy, 250, 0, 1000);
    MochiEmotion::add(f.happy, -300, -1000, 1000);
    f.excite = 1000;
    f.playful = 0;
    snprintf(text_, sizeof(text_), "too many presses: STOP! | %s", feelings());
    log("EXTREME_REACTION", text_);
    beginSequence();
    restMs_ = 0;
    blinkPending_ = false;
    resting_ = false;
    ExprId first = justShown(EXPRESSIONS[E_SHOCK].anim) ? E_WINCE : E_SHOCK;
    queueMorph(EXPRESSIONS[first].anim, EXPRESSIONS[first].from, MORPH_REACT_MS);
    queueExpr(first, 1);
    if (now - lastRare_[R_SCREAM] >= RARE_COOLDOWN_MS) { queueAfterExpr(E_SCREAM, 2); lastRare_[R_SCREAM] = now; }
    queueAfterExpr(E_CRYING, 2);
    lastRare_[R_CRYING] = now;
    stopUntil_ = now + STOP_COOLDOWN_MS;
    clearPresses();
    b1Streak_ = b2Streak_ = 0;
    episodeLeft_ = 0;
    afterglowTags_ = T_SAD | T_ANNOYED;
    afterglowUntil_ = now + AFTERGLOW_MS;
    run("EXTREME_REACTION", SETTLE_EMOTION_MS, now);
  }

  // A reaction to a press: quick morph, starts now (also mid-morph)
  void react(ExprId e, const char* why, uint32_t now) {
    episodeLeft_ = 0;
    episodeEnded_ = false;
    afterglowTags_ = EXPRESSIONS[e].tags;
    afterglowUntil_ = now + AFTERGLOW_MS;
    act(e, why, now, MORPH_REACT_MS);
  }

  // ---- Sequences ----
  void beginSequence() {
    seqLen_ = seqPos_ = 0;
    returning_ = false;
  }

  void run(const char* why, uint16_t settleMs, uint32_t now) {
    state_ = ACTING;
    stepWhy_ = why;
    settleMs_ = settleMs;
    startStep(now);
  }

  // One expression, morphing into it from whatever is on screen now (also mid-morph)
  void act(ExprId e, const char* why, uint32_t now, uint16_t morphMsValue) {
    restMs_ = 0;                             // (a press can cut a resting moment short)
    blinkPending_ = false;
    resting_ = false;
    beginSequence();
    queueMorph(EXPRESSIONS[e].anim, EXPRESSIONS[e].from, morphMsValue);
    queueExpr(e, loopsFor(e));
    decision(why, e);
    if (EXPRESSIONS[e].rare != R_NONE) lastRare_[EXPRESSIONS[e].rare] = lastAnyRare_ = now;
    run(why, SETTLE_EMOTION_MS, now);
  }

  // Log a decision: why, which expression, and the feelings behind it
  void decision(const char* why, ExprId e) {
    const Expression& x = EXPRESSIONS[e];
    snprintf(text_, sizeof(text_), "%s (%s) | %s %u%% | %s", x.name, MOCHI_ANIMS[x.anim].name,
             MOOD_NAMES[mood_], (unsigned)intensity(), feelings());
    log(why, text_);
  }

  const char* feelings() {
    snprintf(feel_, sizeof(feel_), "h%d a%d p%d n%d c%d e%d x%d",
             emo_.happy / 10, emo_.affection / 10, emo_.playful / 10, emo_.annoy / 10,
             emo_.curious / 10, emo_.energy / 10, emo_.excite / 10);
    return feel_;
  }

  void queue(AnimId id, uint8_t loops) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { id, loops, -1, 0, 0, 0xFF, E_NONE };
  }

  void queueExpr(ExprId e, uint8_t loops) {
    const Expression& x = EXPRESSIONS[e];
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { x.anim, loops, -1, 0, x.from, x.to, (uint8_t)e };
  }

  void queueStill(AnimId id, uint8_t frame, uint16_t ms) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { id, 1, (int16_t)frame, ms, 0, 0, E_NONE };
  }

  void queueWait(uint16_t ms) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { ANIM_BLINK, 1, WAIT, ms, 0, 0, E_NONE };
  }

  // Morph from what is on screen (when the step starts) to frame `frame` of `id`
  void queueMorph(AnimId id, uint8_t frame, uint16_t ms) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { id, frame, MORPH, ms, 0, 0, E_NONE };
  }

  // Next animation of a sequence, morphing from the previous one's last frame
  void queueAfter(AnimId id, uint8_t loops) {
    queueMorph(id, 0, morphMs());
    queue(id, loops);
  }

  void queueAfterExpr(ExprId e, uint8_t loops) {
    queueMorph(EXPRESSIONS[e].anim, EXPRESSIONS[e].from, morphMs());
    queueExpr(e, loops);
  }

  uint16_t morphMs() { return (uint16_t)range(MORPH_MIN_MS, MORPH_MAX_MS); }

  void startStep(uint32_t now) {
    const Step& s = seq_[seqPos_++];
    stepIsAnim_ = false;
    if (s.frame == WAIT) {                   // keep the current image a moment
      stillUntil_ = now + s.ms;
      return;
    }
    if (s.frame == MORPH) {                  // expression morph (s.loops = target frame)
      uint8_t steps = MochiPlayer::morphSteps(s.ms >= 300 ? 8 : 4, s.ms);   // slow ones (sleep, wake): smoother
      morphs_++;
      if (verbose_) {
        snprintf(text_, sizeof(text_), "-> %s f%u in %u ms (%u frames)", MOCHI_ANIMS[s.id].name,
                 (unsigned)s.loops, (unsigned)s.ms, (unsigned)steps);
        logv("MORPH", text_);
      }
      player_.morphTo(s.id, s.loops, steps, s.ms, now);
      stillUntil_ = now;
      return;
    }
    if (s.frame >= 0) {                      // still frame (blink / bridge)
      player_.hold(s.id, (uint16_t)s.frame);
      stillUntil_ = now + s.ms;
      return;
    }
    if (s.id != ANIM_BLINK) remember(s.id);
    if (s.expr != E_NONE) rememberExpr((ExprId)s.expr);
    lastStarted_ = s.id;
    lastPlayed_[s.id] = now;                 // starvation counter starts again
    stepIsAnim_ = true;
    stillUntil_ = now;
    if (verbose_) {
      snprintf(text_, sizeof(text_), "%s f%u-%u (%s)", MOCHI_ANIMS[s.id].name, (unsigned)s.from,
               (unsigned)(s.to == 0xFF ? MOCHI_ANIMS[s.id].frameCount - 1 : s.to),
               s.expr != E_NONE ? EXPRESSIONS[s.expr].name : stepWhy_);
      logv("ANIMATION START", text_);
    }
    player_.play(s.id, s.loops, now, s.from, s.to);
  }

  bool stepBusy(uint32_t now) const {
    return player_.busy() || (int32_t)(now - stillUntil_) < 0;
  }

  // ---- Between actions ----
  // An action ended: sleep if tired and alone; else a short pause (chaining on) or a
  // calm moment on the mood's face; after an episode, always a calm moment.
  void afterAction(uint32_t now) {
    if (sleepDue(now)) {
      fallAsleep(now);
      return;
    }
    const MoodFlow& f = MOOD_FLOW[mood_];
    bool rest;
    uint16_t restMs = (uint16_t)natural(f.restMin, f.restMax);
    if (episodeLeft_ > 0) {
      rest = false;
    } else if (episodeEnded_) {
      episodeEnded_ = false;
      rest = true;
      restMs = (uint16_t)natural(2000, 4500);
      logv("EPISODE_SETTLE", "calm moment after an episode");
    } else {
      int16_t pct = f.restPct + (emo_.energy < 400 ? 15 : 0) - (emo_.excite > 600 ? 10 : 0) +
                    ((int32_t)(groggyUntil_ - now) > 0 ? 30 : 0);
      rest = (int16_t)rand_(100) < pct;
    }
    if (rest) {
      restMs_ = restMs;
      returnToRest(now);
      return;
    }
    state_ = IDLE;
    seqLen_ = seqPos_ = 0;
    returning_ = false;
    blinkPending_ = false;                   // (the resting face's own blink)
    resting_ = false;
    pauseStart_ = now;
    uint32_t ms = natural(f.pauseMin, f.pauseMax);
    nextAction_ = now + ms;
    if (verbose_) {
      snprintf(text_, sizeof(text_), "%u ms on %s f%u", (unsigned)ms, MOCHI_ANIMS[player_.current()].name,
               (unsigned)player_.frameIndex());
      logv("PAUSE", text_);
    }
  }

  // What does Mochi want to express now?
  //  - continue an episode (related expressions: build up, peak, settle)
  //  - after a mood change: a short episode that bridges the old mood to the new one
  //  - sometimes start an episode of the mood (more likely when it is felt strongly)
  //  - a strong feeling the mood has not caught up with yet (the mood waits out its
  //    minimum time) shows through: lingering annoyance or sadness
  //  - otherwise one expression: close to the last reaction (afterglow), or 70% the
  //    mood, 20% a neighbouring mood, 10% calm
  void nextAction(uint32_t now) {
    ExprId e;
    const char* why;
    uint16_t strong = 0;
    const char* strongWhy = "";
    if (mood_ != ANNOYED && emo_.annoy >= 450) { strong = T_ANNOYED | T_ANGRY; strongWhy = "LINGERING_ANNOYANCE"; }
    else if (mood_ != SAD && emo_.happy <= -350) { strong = T_SAD; strongWhy = "LINGERING_SADNESS"; }
    if ((int32_t)(groggyUntil_ - now) > 0) {   // just woke up on its own: calm and sleepy only
      episodeLeft_ = 0;
      moodChanged_ = false;
      e = pickExpr(T_SLEEPY | T_IDLE, 1, false, T_EXCITED | T_ANGRY | T_SURPRISE);
      act(e, "GROGGY_WAKE", now, morphMs());
      return;
    }
    if (strong && (emo_.annoy >= 600 || emo_.happy <= -600 || rand_(100) < 75)) {   // very strong: always
      episodeLeft_ = 0;
      e = pickExpr(strong, emo_.annoy >= 700 ? 3 : 2, true);
      why = strongWhy;
    } else if (episodeLeft_ > 0) {
      e = episodeStep(why);
    } else if (moodChanged_) {
      moodChanged_ = false;
      startEpisode(MOOD_TAGS[mood_], (uint8_t)(2 + rand_(2)), true);
      e = episodeStep(why);
    } else if (rand_(100) < 12u + intensity() / 10u) {
      startEpisode(MOOD_TAGS[mood_], (uint8_t)(2 + rand_(3)), false);
      e = episodeStep(why);
    } else {
      uint16_t tags;
      bool glow = false;
      uint32_t r = rand_(100);
      if ((int32_t)(afterglowUntil_ - now) > 0 && r < 60) { tags = afterglowTags_; why = "AFTERGLOW"; glow = true; }
      else if (r < 70) { tags = MOOD_TAGS[mood_]; why = MOOD_IDLE_REASON[mood_]; }
      else if (r < 90) { tags = MOOD_NEIGHBORS[mood_]; why = "NEIGHBOR_MOOD"; }
      else { tags = T_IDLE; why = "CALM_MOMENT"; }
      e = pickExpr(tags, intensityLevel(), true, glow ? 0 : MOOD_FORBID[mood_]);
    }
    if (EXPRESSIONS[e].rare != R_NONE) why = rareReason(EXPRESSIONS[e].rare, why);
    act(e, why, now, morphMs());
  }

  void startEpisode(uint16_t tags, uint8_t len, bool bridge) {
    episodeTags_ = tags;
    episodeLen_ = len;
    episodeLeft_ = len;
    episodeBridge_ = bridge;
  }

  // Episode step i of n: level 1 first (or a bridge from the previous mood), the
  // mood's strength in the middle, level 1 again to settle
  ExprId episodeStep(const char*& why) {
    uint8_t i = episodeLen_ - episodeLeft_;
    episodeLeft_--;
    if (episodeLeft_ == 0) episodeEnded_ = true;
    uint8_t peak = intensityLevel();
    uint8_t level = (i == 0 || (episodeLeft_ == 0 && episodeLen_ > 2)) ? 1 : peak;
    uint16_t tags = episodeTags_;
    why = MOOD_EPISODE_REASON[mood_];
    if (i == 0 && episodeBridge_) {          // mild: the new mood, or a feeling both moods share
      tags = (uint16_t)(MOOD_TAGS[mood_] | (MOOD_NEIGHBORS[mood_] & (MOOD_TAGS[prevMood_] | MOOD_NEIGHBORS[prevMood_])));
      why = "MOOD_BRIDGE";
      level = 1;
    }
    return pickExpr(tags, level < 1 ? 1 : level, i > 0, MOOD_FORBID[mood_]);
  }

  // ---- Choosing an expression ----
  // Among expressions with one of `tags`: not stronger than maxLevel (rare ones need
  // their condition), without a `forbid` feeling, not shown in the last few actions,
  // weighted toward maxLevel, by its own weight and by the starvation boost.
  // Relaxes the repeat rule if nothing is left.
  ExprId pickExpr(uint16_t tags, uint8_t maxLevel, bool allowRare, uint16_t forbid = 0) {
    uint32_t w[E_COUNT];
    for (uint8_t pass = 0; pass < 3; pass++) {
      uint32_t total = 0;
      for (uint8_t i = 0; i < E_COUNT; i++) {
        const Expression& x = EXPRESSIONS[i];
        w[i] = 0;
        if (!(x.tags & tags) || (x.tags & forbid)) continue;
        if (x.rare != R_NONE) {
          if (!allowRare || !rareOk(x.rare) || now_ - lastAnyRare_ < RARE_SPACING_MS) continue;
        } else if (x.level > maxLevel) {
          continue;
        }
        if (justShown(x.anim)) continue;                                      // never the same twice in a row
        if (pass == 0 && (recentExpr((ExprId)i, 3) || recent(x.anim, 3))) continue;
        if (pass == 1 && recentExpr((ExprId)i, 1)) continue;
        uint32_t base = x.rare != R_NONE ? 400 : x.level == maxLevel ? 300 : 100 + 50 * x.level;
        w[i] = base * x.weight / 10 * starvePct(x.anim) / 100;
        total += w[i];
      }
      if (!total) continue;
      uint32_t r = rand_(total);
      for (uint8_t i = 0; i < E_COUNT; i++) {
        if (r < w[i]) return (ExprId)i;
        r -= w[i];
      }
    }
    return justShown(ANIM_HAPPY) ? E_GLANCE : E_CALM;
  }

  // One step of a button ladder: any fitting option that was not just shown
  ExprId pickRung(const Rung& r) {
    ExprId opts[5];
    uint8_t n = 0;
    for (uint8_t pass = 0; pass < 2 && !n; pass++)
      for (uint8_t i = 0; i < r.n; i++) {
        ExprId e = r.e[i];
        const Expression& x = EXPRESSIONS[e];
        if (x.rare != R_NONE && !rareOk(x.rare)) continue;
        if (justShown(x.anim)) continue;
        if (pass == 0 && recentExpr(e, 2)) continue;
        opts[n++] = e;
      }
    if (!n) {                                // all just shown: something else of the same feeling
      uint16_t tags = 0;
      for (uint8_t i = 0; i < r.n; i++) tags |= EXPRESSIONS[r.e[i]].tags;
      return pickExpr(tags, 3, false);
    }
    // a special option, when allowed, is the strongest fit: prefer it (FURIOUS always: it
    // is the peak of the teasing ladder, and its condition and cooldown keep it rare)
    for (uint8_t i = 0; i < n; i++)
      if (EXPRESSIONS[opts[i]].rare != R_NONE && (EXPRESSIONS[opts[i]].rare == R_FURIOUS || rand_(100) < 60)) return opts[i];
    return opts[rand_(n)];
  }

  // Special moments only when their feeling is really there (and not too often)
  bool rareOk(RareKind k) const {
    if (now_ - lastRare_[k] < RARE_COOLDOWN_MS) return false;
    const MochiEmotion& f = emo_;
    switch (k) {
      case R_FURIOUS:  return f.annoy >= 850;
      case R_SCREAM:   return f.excite >= 800 && f.annoy >= 600;
      case R_CRYING:   return f.happy <= -500;
      case R_MISCHIEF: return f.playful >= 450 && f.happy >= 150;
      case R_SPITE:    return (f.playful >= 600 && f.happy >= 300) || f.annoy >= 900;
      case R_DIZZY:    return (f.excite >= 700 && recentCount(now_) >= RAPID_OVERSTIM - 1) || f.excite >= 850;
      case R_GLITCH:   return f.curious >= 550;
      default:         return false;
    }
  }

  static const char* rareReason(RareKind k, const char* fallback) {
    switch (k) {
      case R_FURIOUS:  return "ANNOYANCE_PEAK";
      case R_SCREAM:   return "EXTREME_REACTION";
      case R_CRYING:   return "STRONG_SADNESS";
      case R_MISCHIEF: return "RARE_MISCHIEF";
      case R_SPITE:    return "RARE_SPITE";
      case R_DIZZY:    return "OVERSTIMULATED";
      case R_GLITCH:   return "RARE_GLITCH";
      default:         return fallback;
    }
  }

  uint32_t starvePct(AnimId id) const {
    uint32_t since = now_ - lastPlayed_[id];
    for (const StarveStep& s : STARVE_BOOST)
      if (since >= s.afterMs) return s.pct;
    return 100;
  }

  // ---- Calm moments and blinks ----
  // Resting moment: the last frame settles, then a morph to the mood's face
  void returnToRest(uint32_t now) {
    AnimId cur = player_.current();
    uint16_t f = player_.frameIndex();
    const Pose& p = pose();
    restPose_ = &p;                          // the face rested on (kept even if the mood changes meanwhile)
    beginSequence();
    returning_ = true;
    if (settleMs_) queueWait(settleMs_);
    if (!(cur == p.anim && f == p.frame)) queueMorph(p.anim, p.frame, morphMs());
    if (seqLen_ == 0) {
      rest(now);
      return;
    }
    startStep(now);
  }

  // On the mood's resting face. Arriving for a resting moment (restMs_ set) starts its
  // timer and maybe schedules a blink inside it; coming back from that blink keeps both.
  void rest(uint32_t now) {
    state_ = IDLE;
    seqLen_ = seqPos_ = 0;
    returning_ = false;
    const Pose& p = restFace();
    if (player_.busy() || player_.current() != p.anim || player_.frameIndex() != p.frame) player_.hold(p.anim, p.frame);
    resting_ = true;
    if (!restMs_) return;
    if (verbose_) {
      snprintf(text_, sizeof(text_), "%s f%u for %u ms", MOCHI_ANIMS[p.anim].name, (unsigned)p.frame, (unsigned)restMs_);
      logv("REST", text_);
    }
    if (p.blink == BLINK_NONE && restMs_ > REST_STILL_MAX_MS) restMs_ = REST_STILL_MAX_MS;   // no blink: not long still
    nextAction_ = now + restMs_;
    blinkPending_ = restMs_ >= 1200 && p.blink != BLINK_NONE;
    if (blinkPending_) nextBlink_ = now + 600 + rand_(restMs_ >= 2200 ? 1000 : restMs_ - 1200);   // soon, not right at either end
    else if ((int32_t)(nextBlink_ - now) < 0) nextBlink_ = now + restMs_;  // wait for the next pause
    restMs_ = 0;
  }

  // Blink that matches the resting face (it is its own transition: no extra bridge)
  void blink(bool twice, uint32_t now) {
    const Pose& p = restFace();
    lastBlink_ = now;
    nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
    if (p.blink == BLINK_NONE) return;
    if (p.blink == BLINK_EYELID) {           // the face's own eyelids; Mochi stays resting
      if (!player_.blink(now)) return;
      log("BLINK", "eyelids");
      if (twice) {                           // the second one right after this one
        blinkAgain_ = true;
        blinkPending_ = true;
        nextBlink_ = now + EYELID_TWICE_MS;
      }
      return;
    }
    beginSequence();
    if (p.blink == BLINK_SWAY) {             // the clip's own frames: out to blinkFrame and back
      int8_t dir = p.blinkFrame > p.frame ? 1 : -1;
      for (int16_t f = p.frame + dir; f != p.blinkFrame; f += dir) queueStill(p.anim, (uint8_t)f, SWAY_STEP_MS);
      queueStill(p.anim, p.blinkFrame, SWAY_HOLD_MS);
      for (int16_t f = p.blinkFrame - dir; f != p.frame; f -= dir) queueStill(p.anim, (uint8_t)f, SWAY_STEP_MS);
      logv("SWAY", MOCHI_ANIMS[p.anim].name);
      run("SWAY", 0, now);
      returning_ = true;
      return;
    }
    queueStill(p.blinkAnim, p.blinkFrame, BLINK_CLOSED_MS);
    if (twice) {
      queueStill(p.anim, p.frame, BLINK_OPEN_MS);
      queueStill(p.blinkAnim, p.blinkFrame, BLINK_CLOSED_MS);
    }
    log(twice ? "BLINK x2" : "BLINK", "closed-eye frame");
    run(twice ? "BLINK x2" : "BLINK", 0, now);
    returning_ = true;                       // straight back to the face afterwards
  }

  // ---- Sleep / wake ----
  // Time alone counts from the last press, or from waking up on its own (so Mochi does not
  // fall straight back asleep after a natural wake)
  uint32_t awakeRef() const { return (int32_t)(wokeAt_ - lastInteraction_) > 0 ? wokeAt_ : lastInteraction_; }

  bool sleepDue(uint32_t now) const {
    uint32_t idle = now - awakeRef();
    return (emo_.energy <= SLEEP_ENERGY && idle >= SLEEP_MIN_IDLE_MS) || idle >= SLEEP_AFTER_MS;
  }

  // Called between actions only, so it never cuts an animation or morph short.
  // Gradual: relax, eyes squeeze, then SLEEPY_3's eyes close (frames 0 -> 1 -> 2)
  void fallAsleep(uint32_t now) {
    snprintf(text_, sizeof(text_), "falling asleep (energy %d, alone %lu s)", emo_.energy / 10,
             (unsigned long)((now - lastInteraction_) / 1000 * TIME_SCALE));
    log("SLEEPINESS_THRESHOLD", text_);
    beginSequence();
    episodeLeft_ = 0;
    AnimId first = recent(ANIM_RELAXED, 1) ? ANIM_SQUINT : ANIM_RELAXED;
    queueMorph(first, 0, morphMs());
    queue(first, first == ANIM_SQUINT ? 2 : 1);
    if (first == ANIM_RELAXED) queueAfter(ANIM_SQUINT, 2);
    queueWait(SETTLE_SLEEP_MS);
    queueMorph(SLEEP_ANIM, 0, MORPH_MAX_MS);                 // drowsy eyes ...
    queueStill(SLEEP_ANIM, 0, 600);
    queueMorph(SLEEP_ANIM, DOZE_FRAME, 400);                 // ... nearly shut ...
    queueStill(SLEEP_ANIM, DOZE_FRAME, 500);
    queueMorph(SLEEP_ANIM, SLEEP_FRAME, 500);                // ... asleep
    changeMood(SLEEPY, "SLEEPINESS_THRESHOLD", now);
    log("SLEEP", "falling asleep");
    run("SLEEP SEQ", 0, now);
    state_ = FALLING_ASLEEP;
  }

  void sleep(uint32_t now) {
    state_ = ASLEEP;
    seqLen_ = seqPos_ = 0;
    player_.hold(SLEEP_ANIM, SLEEP_FRAME);          // light sleep: where falling asleep ended
    sleepDepth_ = SLEEP_LIGHT;
    sleepPhase_ = SP_REST;
    breathLanding_ = false;
    sleepDepthUntil_ = now + range(SLEEP_FACES[SLEEP_LIGHT].staySMin, SLEEP_FACES[SLEEP_LIGHT].staySMax) * 1000UL / TIME_SCALE;
    nextBreath_ = now + range(1200, 2200);          // the first breath comes soon: settling in
    sleptAt_ = now;
    wakeAt_ = now + natural(SLEEP_MIN_MS, SLEEP_MAX_MS);
    snprintf(text_, sizeof(text_), "asleep (will wake up on its own in %lu s)", (unsigned long)((wakeAt_ - now) / 1000 * TIME_SCALE));
    log("SLEEP", text_);
  }

  // Asleep: breathe (part of the way toward a nearby closed-eye frame and back), now and
  // then twitch or sigh, and drift between light, deep and deepest sleep. Never mechanical:
  // depth and every duration vary within bounds.
  void breathe(uint32_t now) {
    if (player_.busy()) return;                  // a morph is on its way
    const SleepFace& f = SLEEP_FACES[sleepDepth_];
    if (breathLanding_) {                        // back on the face: show the frame itself
      player_.hold(f.anim, f.frame);
      breathLanding_ = false;
    }
    if (sleepPhase_ == SP_REST && (int32_t)(now - wakeAt_) >= 0) {   // slept enough: between two breaths
      naturalWake(now);
      return;
    }
    if ((int32_t)(now - nextBreath_) < 0) return;
    if (sleepPhase_ == SP_IN || sleepPhase_ == SP_TWITCH) {   // breathe out / settle after a twitch
      bool twitch = sleepPhase_ == SP_TWITCH;
      uint16_t ms = (uint16_t)(twitch ? range(220, 320) : natural(f.outMin, f.outMax));
      player_.morphTo(f.anim, f.frame, twitch ? 3 : BREATH_STEPS, ms, now);
      breathLanding_ = true;
      sleepPhase_ = SP_REST;
      nextBreath_ = now + ms + natural(f.restMin, f.restMax);
      logv("BREATH", "out");
      return;
    }
    if ((int32_t)(now - sleepDepthUntil_) >= 0) {             // drift to another sleep depth
      SleepDepth next = sleepDepth_ == SLEEP_LIGHT ? SLEEP_DEEP
                      : sleepDepth_ == SLEEP_DEEPEST ? SLEEP_DEEP
                      : (rand_(100) < SLEEP_DEEPEST_PCT ? SLEEP_DEEPEST : SLEEP_LIGHT);
      const SleepFace& g = SLEEP_FACES[next];
      uint16_t ms = (uint16_t)natural(SLEEP_FACE_MIN_MS, SLEEP_FACE_MAX_MS);
      player_.morphTo(g.anim, g.frame, BREATH_STEPS, ms, now);
      breathLanding_ = true;
      log(next > sleepDepth_ ? "SLEEP_DEEPER" : "SLEEP_LIGHTER", g.name);
      sleepDepth_ = next;
      sleepDepthUntil_ = now + ms + range(g.staySMin, g.staySMax) * 1000UL / TIME_SCALE;
      nextBreath_ = now + ms + natural(g.restMin, g.restMax);
      return;
    }
    uint32_t r = rand_(100);
    uint16_t depth, ms;
    uint8_t steps = BREATH_STEPS;
    if (r < SLEEP_TWITCH_PCT && sleepDepth_ != SLEEP_DEEPEST) {   // a tiny sleepy twitch
      depth = (uint16_t)range(60, 90);
      ms = (uint16_t)range(140, 200);
      steps = 2;
      sleepPhase_ = SP_TWITCH;
      nextBreath_ = now + ms + range(60, 150);
      log("SLEEP_TWITCH", f.name);
    } else {                                                      // a breath (sometimes a deeper sigh)
      bool sigh = r < SLEEP_TWITCH_PCT + SLEEP_SIGH_PCT;
      depth = sigh ? (uint16_t)(f.depthMax + range(20, 35)) : (uint16_t)natural(f.depthMin, f.depthMax);
      ms = (uint16_t)(natural(f.inMin, f.inMax) + (sigh ? 600 : 0));
      sleepPhase_ = SP_IN;
      nextBreath_ = now + ms + range(150, 450);                   // a short pause, full of breath
      if (sigh) log("SLEEP_SIGH", f.name); else logv("BREATH", "in");
    }
    player_.morphTo(f.toward, f.towardFrame, steps, ms, now, depth);
  }

  // Waking up on its own: slow and groggy, nothing like the startled button wake
  void naturalWake(uint32_t now) {
    snprintf(text_, sizeof(text_), "woke up on its own after %lu s asleep (energy %d)",
             (unsigned long)((now - sleptAt_) / 1000 * TIME_SCALE), emo_.energy / 10);
    breathLanding_ = false;
    sleepPhase_ = SP_REST;
    beginSequence();
    queueMorph(SLEEP_ANIM, SLEEP_FRAME, 1600);           // the lids come back from deep sleep ...
    queueStill(SLEEP_ANIM, SLEEP_FRAME, 800);
    queueMorph(SLEEP_ANIM, DOZE_FRAME, 1200);            // ... lift a little ...
    queueStill(SLEEP_ANIM, DOZE_FRAME, 700);
    queueMorph(SLEEP_ANIM, 0, 1200);                     // ... sleepy, half-open eyes
    queueStill(SLEEP_ANIM, 0, 900);
    queueMorph(CLOSED_GIF, 320);                         // a slow, sleepy blink
    queueStill(CLOSED_GIF, 180);
    queueMorph(EXPRESSIONS[E_YAWN].anim, EXPRESSIONS[E_YAWN].from, 400);   // a yawn
    queueExpr(E_YAWN, 1);
    queueMorph(GROGGY_ANIM, GROGGY_FRAME, 700);          // heavy-lidded, still groggy
    MochiEmotion::add(emo_.happy, 60, -1000, 1000);      // rested, a little content
    emo_.excite = 60;
    int16_t rested = (int16_t)natural(700, 1000);        // not always equally rested
    if (emo_.energy > rested) emo_.energy = rested;
    wokeAt_ = now;
    groggyUntil_ = now + natural(GROGGY_MIN_MS, GROGGY_MAX_MS);
    b1Streak_ = b2Streak_ = 0;
    episodeLeft_ = 0;
    log("NATURAL_WAKE", text_);
    changeMood(SLEEPY, "NATURAL_WAKE", now);
    moodChanged_ = false;
    lastBlink_ = now;
    nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
    run("NATURAL_WAKE", 0, now);
    state_ = WAKING_NATURALLY;
  }

  // Immediate: the sleeping eyes open into SURPRISED (morph starts now), then a blink;
  // the mood after waking follows from the feelings (a poke makes it grumpy)
  void wake(uint8_t button, uint32_t now) {
    breathLanding_ = false;
    sleepPhase_ = SP_REST;
    MochiEmotion::add(emo_.excite, 400, 0, 1000);
    MochiEmotion::add(emo_.energy, 100, 0, 1000);
    if (button == 1) MochiEmotion::add(emo_.annoy, 150, 0, 1000);
    else MochiEmotion::add(emo_.happy, 100, -1000, 1000);
    b1Streak_ = b2Streak_ = 0;
    episodeLeft_ = 0;
    moodChanged_ = false;
    beginSequence();
    queueMorph(ANIM_SURPRISED, 0, MORPH_MIN_MS);    // sleeping eyes open up
    queue(ANIM_SURPRISED, 1);
    queueAfter(ANIM_BLINK, 1);
    log("WAKE", button == 1 ? "woken up by a poke" : "woken up");
    changeMood(bestMood(), button == 1 ? "GRUMPY_WAKE" : "WOKEN_UP", now);
    moodChanged_ = false;
    lastBlink_ = now;                                // the wake sequence ends with a blink
    nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
    run("WAKE SEQ", 0, now);
    state_ = WAKING;
  }

  // ---- Memory ----
  bool recent(AnimId id, uint8_t depth) const {
    for (uint8_t i = 0; i < depth && i < historyLen_; i++)
      if (history_[(historyHead_ + HISTORY - 1 - i) % HISTORY] == id) return true;
    return false;
  }

  AnimId lastAnim() const { return lastStarted_; }
  bool justShown(AnimId id) const { return id == lastStarted_ || id == player_.current(); }

  void remember(AnimId id) {
    history_[historyHead_] = id;
    historyHead_ = (historyHead_ + 1) % HISTORY;
    if (historyLen_ < HISTORY) historyLen_++;
  }

  bool recentExpr(ExprId e, uint8_t depth) const {
    for (uint8_t i = 0; i < depth && i < exprLen_; i++)
      if (exprHist_[(exprHead_ + EXPR_HISTORY - 1 - i) % EXPR_HISTORY] == e) return true;
    return false;
  }

  void rememberExpr(ExprId e) {
    exprHist_[exprHead_] = e;
    exprHead_ = (exprHead_ + 1) % EXPR_HISTORY;
    if (exprLen_ < EXPR_HISTORY) exprLen_++;
  }

  bool lastB1Set() const { return lastB1_ != 0 || b1Streak_; }
  bool lastB2Set() const { return lastB2_ != 0 || b2Streak_; }

  // ---- Helpers ----
  uint8_t loopsFor(ExprId e) {
    const Expression& x = EXPRESSIONS[e];
    uint16_t to = x.to == LAST ? MOCHI_ANIMS[x.anim].frameCount - 1 : x.to;
    uint16_t len = to - x.from + 1;
    for (uint8_t i = 0; i < sizeof(PLAY_ONCE) / sizeof(PLAY_ONCE[0]); i++)
      if (PLAY_ONCE[i] == e) return 1;
    // short clips (< 1 s) read better when repeated a couple of times
    return len < 4 ? 1 : len < 12 ? (uint8_t)(2 + rand_(2)) : 1;
  }

  uint32_t range(uint32_t lo, uint32_t hi) { return lo + rand_(hi - lo + 1); }
  // lo..hi, more often near the middle than at the ends (sum of two random draws)
  uint32_t natural(uint32_t lo, uint32_t hi) { return (range(lo, hi) + range(lo, hi)) / 2; }

  uint8_t recentCount(uint32_t now) const {
    uint8_t c = 0;
    for (uint8_t i = 0; i < PRESS_HISTORY; i++)
      if (pressTimes_[i] && now - pressTimes_[i] <= PRESS_WINDOW_MS) c++;
    return c;
  }

  void clearPresses() {
    for (uint8_t i = 0; i < PRESS_HISTORY; i++) pressTimes_[i] = 0;
  }

  void log(const char* event, const char* detail) {
    if (log_) log_(event, detail);
  }

  void logv(const char* event, const char* detail) {
    if (verbose_ && log_) log_(event, detail);
  }

  MochiPlayer& player_;
  RandFn rand_;
  LogFn log_;
  bool verbose_ = false;
  char text_[128];
  char feel_[56];

  State state_ = IDLE;
  MochiEmotion emo_;
  Mood mood_ = NEUTRAL, prevMood_ = NEUTRAL;
  uint32_t moodSince_ = 0;
  bool moodChanged_ = false;             // the next action bridges into the new mood
  uint8_t pose_ = 0;
  uint32_t lastTick_ = 0;

  // episode: a few related expressions in a row
  uint16_t episodeTags_ = 0;
  uint8_t episodeLen_ = 0, episodeLeft_ = 0;
  bool episodeBridge_ = false, episodeEnded_ = false;
  uint16_t afterglowTags_ = 0;
  uint32_t afterglowUntil_ = 0;

  uint32_t nextBlink_ = 0, nextAction_ = 0, lastBlink_ = 0;
  bool blinkPending_ = false;            // a blink is planned inside the current resting moment
  bool blinkAgain_ = false;              // that planned blink is the second of a double blink
  const Pose* restPose_ = nullptr;       // the resting face being shown (see restFace())
  bool resting_ = false;                 // IDLE on the mood's face (else: a short pause on a last frame)
  uint32_t pauseStart_ = 0;
  uint16_t restMs_ = 0;                  // length of the resting moment being entered (0: none)
  uint32_t lastInteraction_ = 0, lastReactionMs_ = 0, stopUntil_ = 0;
  uint32_t lastB1_ = 0, lastB2_ = 0;
  uint8_t b1Streak_ = 0, b2Streak_ = 0;
  uint32_t stillUntil_ = 0;
  uint32_t nextBreath_ = 0, sleepDepthUntil_ = 0;
  uint32_t wakeAt_ = 0, sleptAt_ = 0;    // asleep: when Mochi wakes up on its own / fell asleep
  uint32_t wokeAt_ = 0;                  // when it last woke up on its own (time alone counts from here)
  uint32_t groggyUntil_ = 0;             // after waking up on its own: calm and sleepy until then
  enum SleepPhase : uint8_t { SP_REST, SP_IN, SP_TWITCH };
  SleepDepth sleepDepth_ = SLEEP_LIGHT;  // asleep: which closed-eye face
  SleepPhase sleepPhase_ = SP_REST;      // on the face / breathed in (partial) / mid-twitch
  bool breathLanding_ = false;           // a morph back to the face is running; show the face when done
  bool stepIsAnim_ = false;
  bool returning_ = false;               // the current sequence is the way back to the resting face
  uint16_t settleMs_ = 0;                // how long the last frame stays after the current action
  uint32_t morphs_ = 0;                  // expression morphs started

  Step seq_[SEQ_MAX];
  uint8_t seqLen_ = 0, seqPos_ = 0;
  const char* stepWhy_ = "";

  AnimId history_[HISTORY] = { ANIM_BLINK, ANIM_BLINK, ANIM_BLINK, ANIM_BLINK };
  uint8_t historyHead_ = 0, historyLen_ = 0;
  ExprId exprHist_[EXPR_HISTORY] = { E_CALM, E_CALM, E_CALM, E_CALM };
  uint8_t exprHead_ = 0, exprLen_ = 0;
  uint32_t lastPlayed_[ANIM_COUNT] = { 0 };   // when each animation last started (starvation boost)
  uint32_t lastRare_[R_GLITCH + 1] = { 0 };   // when each special moment last happened
  uint32_t lastAnyRare_ = 0;
  uint32_t now_ = 0;                          // time of the current update()/onButton()
  AnimId lastStarted_ = ANIM_BLINK;           // the animation that played last (no repeats)

  uint32_t pressTimes_[PRESS_HISTORY] = { 0 };
  uint8_t pressHead_ = 0;
};
