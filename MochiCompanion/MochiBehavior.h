// =====================================================
// MOCHI BEHAVIOR — mood + autonomous animation choice
//
//   mood ─► behavior (idle timers, events, buttons) ─► AnimId ─► MochiPlayer
//
// At boot Mochi picks a random mood and immediately plays one of that mood's
// animations. Animations chain into each other: after one ends, its last frame
// stays for a short, varied pause and the next pick (a small action, or an emotion
// on a randomized timer, chosen by weighted random from the mood, avoiding recent
// picks) morphs in. Now and then Mochi instead rests 1-3 s on a calm face of its
// mood (a still frame of an existing animation) and blinks with a matching
// closed-eye frame; in the short pauses it blinks by squeezing the eyes shut.
// Moods last 1.5-4 minutes. After 10 minutes without a button press it gets sleepy
// and falls asleep (once the current action is over); asleep, the sleeping face
// breathes slowly. A button wakes it at once. The OLED stays on at the same
// brightness all the time.
// Every switch between animations/resting faces is a universal expression morph
// (MochiMorph.h): eyes and mouth change shape progressively, special elements leave
// first and arrive last. Button reactions start a quick morph immediately (also in the
// middle of another morph).
// =====================================================

#pragma once

#include <stdint.h>
#include <stdio.h>
#include "MochiPlayer.h"

enum Mood : uint8_t {
  NEUTRAL,
  HAPPY,
  CURIOUS,
  PLAYFUL,
  EXCITED,
  ANNOYED,
  SAD,
  SLEEPY,
  MOOD_COUNT
};

static const char* const MOOD_NAMES[MOOD_COUNT] = {
  "NEUTRAL", "HAPPY", "CURIOUS", "PLAYFUL", "EXCITED", "ANNOYED", "SAD", "SLEEPY"
};

struct WeightedAnim {
  AnimId id;
  uint8_t weight;
};

#define MOCHI_COUNT(a) (sizeof(a) / sizeof((a)[0]))

// ---- Timing (ms) ----
#ifdef MOCHI_FAST_TIMING
// HARDWARE TEST ONLY: everything happens quickly so a short test shows all behaviors
static const uint32_t EMOTION_MIN_MS = 3000, EMOTION_MAX_MS = 5000;
static const uint32_t MOOD_MIN_MS = 10000,  MOOD_MAX_MS = 15000;
static const uint32_t SLEEP_AFTER_MS = 120000;        // no button press -> fall asleep
#else
static const uint32_t EMOTION_MIN_MS = 12000, EMOTION_MAX_MS = 30000;
static const uint32_t MOOD_MIN_MS = 90000,  MOOD_MAX_MS = 240000;
static const uint32_t SLEEP_AFTER_MS = 10UL * 60 * 1000;      // no button press -> fall asleep
#endif
static const uint32_t BLINK_MIN_MS        = 2000;      // time between blinks (a blink waits for
static const uint32_t BLINK_MAX_MS        = 6000;      // the next pause or resting moment)
static const uint8_t  EMOTION_CHAIN_PCT   = 25;        // chance the next action is an emotion anyway
static const uint8_t  REST_BLINK_PCT      = 70;        // chance of a blink during a resting moment
static const uint32_t REACTION_COOLDOWN_MS = 600;       // presses inside this only count
static const uint32_t PRESS_WINDOW_MS     = 3000;       // window for counting rapid presses
static const uint8_t  PRESSES_ANNOYED     = 3;          // rapid presses -> annoyed
static const uint8_t  PRESSES_DIZZY       = 5;          // more rapid presses -> sometimes dizzy
static const uint8_t  DOUBLE_BLINK_PCT    = 20;
static const uint8_t  SPECIAL_PCT         = 5;          // chance an emotion is a rare event
static const uint16_t BLINK_CLOSED_MS     = 110;        // closed-eye frame of a frame blink
static const uint16_t BLINK_OPEN_MS       = 140;        // open again between a double blink

// ---- Transitions: universal expression morph (MochiMorph.h) ----
// Every switch between two images morphs the face parts (eyes, mouth, special
// elements) from what is on screen to the next frame. Durations vary a little.
static const uint16_t MORPH_MIN_MS        = 120;  // autonomous switches: 120..220 ms
static const uint16_t MORPH_MAX_MS        = 220;
static const uint16_t MORPH_REACT_MS      = 105;  // button reactions: quick morph (2 in-between frames), starts at once
static const uint16_t SETTLE_EMOTION_MS   = 180;  // last frame stays a moment after an emotion
static const uint16_t SETTLE_SMALL_MS     = 120;  // ... after a small action
static const uint16_t SETTLE_SLEEP_MS     = 250;  // squeezed-shut eyes before the eyes close
static const uint16_t EYES_OPEN_BOOT_MS   = 220;  // closed eyes at boot, then the first animation
static const uint8_t  CONTRAST_AWAKE      = 0xCF; // SSD1306 brightness, set once at boot (also asleep)

// ---- Sleeping: SLEEPY_3 closes its eyes (frames 0 -> 1 -> 2); asleep, the face
// breathes: the lids drift up to frame 1 and settle back on frame 2 (morphs) ----
static const AnimId   SLEEP_ANIM          = ANIM_SLEEPY_3;
static const uint8_t  SLEEP_FRAME         = 2;    // asleep: curved, closed lids
static const uint8_t  DOZE_FRAME          = 1;    // lids a little higher (breathing in)
static const uint16_t BREATH_IN_MIN_MS    = 600,  BREATH_IN_MAX_MS  = 800;   // morph to DOZE_FRAME
static const uint16_t BREATH_OUT_MIN_MS   = 800,  BREATH_OUT_MAX_MS = 1000;  // morph back to SLEEP_FRAME
static const uint16_t BREATH_HOLD_MIN_MS  = 250,  BREATH_HOLD_MAX_MS = 500;  // on DOZE_FRAME
static const uint16_t BREATH_REST_MIN_MS  = 2600, BREATH_REST_MAX_MS = 4200; // on SLEEP_FRAME
static const uint8_t  BREATH_STEPS        = 5;

// ---- Flow between actions ----
// After an action the next one usually follows after a short pause (the last frame
// stays); sometimes Mochi rests on its mood's face first. Both vary per mood a little.
struct MoodFlow {
  uint8_t restPct;              // chance of a resting moment instead of chaining on
  uint16_t pauseMin, pauseMax;  // short pause before the next animation (ms)
  uint16_t restMin, restMax;    // resting moment on the mood's face (ms)
};
static const MoodFlow MOOD_FLOW[MOOD_COUNT] = {
  { 25, 300, 800, 1000, 3000 },   // NEUTRAL
  { 20, 300, 650, 1000, 2500 },   // HAPPY: shorter pauses, more chaining
  { 25, 300, 800, 1200, 3000 },   // CURIOUS (sometimes looks a little longer, see pauseMs())
  { 20, 300, 600, 1000, 2200 },   // PLAYFUL
  { 18, 300, 550, 1000, 2000 },   // EXCITED
  { 20, 300, 550, 1000, 2000 },   // ANNOYED: quick
  { 30, 450, 800, 1500, 3000 },   // SAD: longer resting moments
  { 33, 500, 800, 1800, 3000 },   // SLEEPY
};
static const uint16_t PAUSE_BLINK_DELAY_MS = 60;       // a blink in a pause starts this late ...
static const uint16_t PAUSE_BLINK_MS      = 220;       // ... and only if the pause lasts this long still
static const uint8_t  CURIOUS_LOOK_PCT    = 20;        // CURIOUS: chance of a slightly longer pause
static const uint16_t CURIOUS_LOOK_MS     = 250;

// Starvation boost: once a pick is eligible (mood/event table, not just played), its
// weight grows the longer that animation has not played. No animation is forced.
struct StarveStep { uint32_t afterMs; uint16_t pct; };
static const StarveStep STARVE_BOOST[] = {
  { 30UL * 60 * 1000, 220 }, { 20UL * 60 * 1000, 160 }, { 10UL * 60 * 1000, 125 } };   // else 100%

// Speed of a mood's emotions (percent of the base interval)
static const uint8_t MOOD_TEMPO_PCT[MOOD_COUNT] = { 100, 90, 90, 75, 60, 100, 130, 150 };

// How likely each mood is to come next (SLEEPY grows the longer nobody presses a button)
static const uint8_t MOOD_NEXT_WEIGHT[MOOD_COUNT] = { 30, 25, 15, 12, 6, 4, 3, 5 };

// Mood right after boot
static const uint8_t MOOD_BOOT_WEIGHT[MOOD_COUNT] = { 25, 30, 20, 20, 5, 0, 0, 0 };

// ---- Resting faces: still frames of existing animations, two per mood ----
// blink: BLINK_CLIP plays the BLINK animation (its face is the BLINK pose),
//        BLINK_FRAME shows the closed-eye frame (blinkAnim, blinkFrame) briefly.
enum BlinkKind : uint8_t { BLINK_CLIP, BLINK_FRAME };

struct Pose {
  AnimId anim;
  uint8_t frame;
  BlinkKind blink;
  AnimId blinkAnim;
  uint8_t blinkFrame;
};

// Closed eyes of the two art styles: full-screen 250frames clips / narrower emote GIFs
#define CLOSED_250  ANIM_BLINK, 3
#define CLOSED_GIF  ANIM_RELAXED, 32

static const Pose POSES[MOOD_COUNT][2] = {
  { { ANIM_BLINK, 0, BLINK_CLIP, ANIM_BLINK, 0 },    { ANIM_HAPPY, 0, BLINK_FRAME, CLOSED_GIF } },        // NEUTRAL
  { { ANIM_SMILE, 0, BLINK_FRAME, CLOSED_250 },      { ANIM_HAPPY_2, 60, BLINK_FRAME, CLOSED_GIF } },     // HAPPY
  { { ANIM_SURPRISED, 0, BLINK_FRAME, CLOSED_250 },  { ANIM_EMBARRASSED, 60, BLINK_FRAME, CLOSED_GIF } }, // CURIOUS
  { { ANIM_HAPPY_3, 0, BLINK_FRAME, CLOSED_250 },    { ANIM_HAPPY_2, 20, BLINK_FRAME, CLOSED_GIF } },     // PLAYFUL
  { { ANIM_SURPRISED, 3, BLINK_FRAME, CLOSED_250 },  { ANIM_ANGRY_2, 25, BLINK_FRAME, CLOSED_GIF } },     // EXCITED
  { { ANIM_ANNOYED, 3, BLINK_FRAME, CLOSED_250 },   { ANIM_FRUSTRATED, 45, BLINK_FRAME, CLOSED_GIF } },  // ANNOYED
  { { ANIM_CRYING, 3, BLINK_FRAME, CLOSED_250 },    { ANIM_FRUSTRATED, 74, BLINK_FRAME, CLOSED_GIF } },  // SAD
  { { ANIM_FRUSTRATED, 45, BLINK_FRAME, CLOSED_GIF }, { ANIM_ANGRY, 20, BLINK_FRAME, CLOSED_GIF } },      // SLEEPY
};


// ---- Animation categories (chosen from the actual animation content) ----
// Emotions: the main animation of a mood
static const WeightedAnim EMO_NEUTRAL[] = {
  { ANIM_HAPPY, 25 }, { ANIM_SMILE, 20 }, { ANIM_ANGRY, 15 }, { ANIM_RELAXED, 15 },
  { ANIM_MOCHI_10, 10 }, { ANIM_HAPPY_3, 10 }, { ANIM_SURPRISED, 5 } };
static const WeightedAnim EMO_HAPPY[] = {
  { ANIM_SMILE, 20 }, { ANIM_HAPPY, 20 }, { ANIM_HAPPY_3, 18 }, { ANIM_HAPPY_2, 15 },
  { ANIM_PROUD, 12 }, { ANIM_LAUGH, 12 }, { ANIM_CONTENT, 10 }, { ANIM_LOVE, 8 },
  { ANIM_UWU, 6 }, { ANIM_KISS, 5 } };
static const WeightedAnim EMO_CURIOUS[] = {
  { ANIM_MOCHI_10, 20 }, { ANIM_SURPRISED, 20 }, { ANIM_RELAXED, 15 },
  { ANIM_CONFUSED_2, 15 }, { ANIM_BUG, 10 }, { ANIM_MOCHI_26, 10 },
  { ANIM_MOCHI_11, 8 }, { ANIM_MOCHI_29, 8 } };
static const WeightedAnim EMO_PLAYFUL[] = {
  { ANIM_EMBARRASSED, 18 }, { ANIM_HAPPY_2, 15 }, { ANIM_CONTENT, 15 }, { ANIM_DETERMINED, 12 },
  { ANIM_EXCITED_2, 10 }, { ANIM_LAUGH, 10 }, { ANIM_UWU, 10 }, { ANIM_KISS, 8 },
  { ANIM_EVIL_GRIN, 7 }, { ANIM_MOCHI_29, 6 }, { ANIM_EVIL, 5 } };
static const WeightedAnim EMO_EXCITED[] = {
  { ANIM_ANGRY_2, 20 }, { ANIM_CONFUSED_2, 15 }, { ANIM_DETERMINED, 15 }, { ANIM_LAUGH, 15 },
  { ANIM_SURPRISED, 10 }, { ANIM_EXCITED_2, 10 }, { ANIM_SCREAM, 8 } };
static const WeightedAnim EMO_ANNOYED[] = {
  { ANIM_ANNOYED, 30 }, { ANIM_SQUINT, 20 }, { ANIM_FRUSTRATED, 20 }, { ANIM_ANGRY_3, 15 },
  { ANIM_FURIOUS, 12 }, { ANIM_EVIL, 8 }, { ANIM_SCREAM, 6 } };
static const WeightedAnim EMO_SAD[] = {
  { ANIM_CRYING, 30 }, { ANIM_SLEEPY_3, 30 }, { ANIM_FRUSTRATED, 15 }, { ANIM_SQUINT, 10 },
  { ANIM_RELAXED, 10 } };
static const WeightedAnim EMO_SLEEPY[] = {
  { ANIM_RELAXED, 30 }, { ANIM_SQUINT, 25 }, { ANIM_FRUSTRATED, 15 }, { ANIM_HAPPY, 10 },
  { ANIM_ANGRY, 10 }, { ANIM_SLEEPY_3, 10 }, { ANIM_CRYING, 6 } };

// Small actions: short and subtle, but several per mood so no clip dominates
static const WeightedAnim SMALL_NEUTRAL[] = {
  { ANIM_MOCHI_10, 10 }, { ANIM_SMILE, 20 }, { ANIM_HAPPY_3, 15 }, { ANIM_HAPPY, 15 },
  { ANIM_SURPRISED, 10 }, { ANIM_SQUINT, 10 }, { ANIM_RELAXED, 10 } };
static const WeightedAnim SMALL_HAPPY[] = {
  { ANIM_SMILE, 20 }, { ANIM_HAPPY_3, 20 }, { ANIM_HAPPY, 15 },
  { ANIM_UWU, 10 }, { ANIM_PROUD, 10 }, { ANIM_KISS, 8 }, { ANIM_LOVE, 7 } };
static const WeightedAnim SMALL_CURIOUS[] = {
  { ANIM_MOCHI_10, 15 }, { ANIM_SURPRISED, 25 }, { ANIM_SMILE, 10 }, { ANIM_BUG, 10 },
  { ANIM_MOCHI_26, 10 }, { ANIM_CONFUSED_2, 10 }, { ANIM_RELAXED, 10 } };
static const WeightedAnim SMALL_PLAYFUL[] = {
  { ANIM_HAPPY_3, 20 }, { ANIM_UWU, 15 }, { ANIM_SMILE, 15 },
  { ANIM_EMBARRASSED, 15 }, { ANIM_KISS, 10 }, { ANIM_HAPPY_2, 10 } };
static const WeightedAnim SMALL_EXCITED[] = {
  { ANIM_SURPRISED, 25 }, { ANIM_HAPPY_3, 20 }, { ANIM_SMILE, 15 }, { ANIM_UWU, 15 },
  { ANIM_ANGRY_2, 15 }, { ANIM_LAUGH, 10 } };
static const WeightedAnim SMALL_ANNOYED[] = {
  { ANIM_SQUINT, 30 }, { ANIM_ANNOYED, 30 }, { ANIM_ANGRY_3, 20 },
  { ANIM_FRUSTRATED, 10 } };
static const WeightedAnim SMALL_SAD[] = {
  { ANIM_CRYING, 30 }, { ANIM_MOCHI_10, 15 }, { ANIM_SQUINT, 25 }, { ANIM_SLEEPY_3, 20 } };
static const WeightedAnim SMALL_SLEEPY[] = {
  { ANIM_SQUINT, 35 }, { ANIM_RELAXED, 35 }, { ANIM_MOCHI_10, 15 } };

// Rare personality events (any mood)
static const WeightedAnim SPECIAL[] = {
  { ANIM_EVIL, 20 }, { ANIM_EVIL_GRIN, 15 }, { ANIM_BUG, 20 }, { ANIM_MOCHI_11, 15 },
  { ANIM_MOCHI_29, 15 }, { ANIM_MOCHI_26, 10 }, { ANIM_UWU, 10 }, { ANIM_DIZZY, 8 },
  { ANIM_SURPRISED, 10 }, { ANIM_SCREAM, 4 }, { ANIM_FURIOUS, 8 }, { ANIM_ANGRY_3, 8 } };

// Button reactions
static const WeightedAnim REACT_FRIENDLY[] = {      // button 1
  { ANIM_SMILE, 20 }, { ANIM_HAPPY, 15 }, { ANIM_HAPPY_3, 15 }, { ANIM_HAPPY_2, 12 },
  { ANIM_UWU, 12 }, { ANIM_KISS, 10 }, { ANIM_LOVE, 10 }, { ANIM_PROUD, 6 } };
static const WeightedAnim REACT_POKE[] = {          // button 2
  { ANIM_SURPRISED, 25 }, { ANIM_CONFUSED_2, 15 }, { ANIM_SQUINT, 15 }, { ANIM_EMBARRASSED, 15 },
  { ANIM_HAPPY_2, 15 }, { ANIM_MOCHI_10, 10 }, { ANIM_DIZZY, 5 } };
static const WeightedAnim REACT_TOO_MUCH[] = {      // 3+ presses within 3 s (either button)
  { ANIM_ANNOYED, 35 }, { ANIM_ANGRY_3, 25 }, { ANIM_SQUINT, 25 }, { ANIM_FRUSTRATED, 15 } };

struct AnimTable {
  const WeightedAnim* items;
  uint8_t count;
};

static const AnimTable EMOTIONS[MOOD_COUNT] = {
  { EMO_NEUTRAL, MOCHI_COUNT(EMO_NEUTRAL) }, { EMO_HAPPY, MOCHI_COUNT(EMO_HAPPY) },
  { EMO_CURIOUS, MOCHI_COUNT(EMO_CURIOUS) }, { EMO_PLAYFUL, MOCHI_COUNT(EMO_PLAYFUL) },
  { EMO_EXCITED, MOCHI_COUNT(EMO_EXCITED) }, { EMO_ANNOYED, MOCHI_COUNT(EMO_ANNOYED) },
  { EMO_SAD, MOCHI_COUNT(EMO_SAD) },         { EMO_SLEEPY, MOCHI_COUNT(EMO_SLEEPY) } };
static const AnimTable SMALL_ACTIONS[MOOD_COUNT] = {
  { SMALL_NEUTRAL, MOCHI_COUNT(SMALL_NEUTRAL) }, { SMALL_HAPPY, MOCHI_COUNT(SMALL_HAPPY) },
  { SMALL_CURIOUS, MOCHI_COUNT(SMALL_CURIOUS) }, { SMALL_PLAYFUL, MOCHI_COUNT(SMALL_PLAYFUL) },
  { SMALL_EXCITED, MOCHI_COUNT(SMALL_EXCITED) }, { SMALL_ANNOYED, MOCHI_COUNT(SMALL_ANNOYED) },
  { SMALL_SAD, MOCHI_COUNT(SMALL_SAD) },         { SMALL_SLEEPY, MOCHI_COUNT(SMALL_SLEEPY) } };


class MochiBehavior {
public:
  enum State : uint8_t { IDLE, ACTING, FALLING_ASLEEP, ASLEEP, WAKING };

  typedef uint32_t (*RandFn)(uint32_t n);                          // 0 .. n-1
  typedef void (*LogFn)(const char* event, const char* detail);

  MochiBehavior(MochiPlayer& player, RandFn rand, LogFn log = nullptr)
    : player_(player), rand_(rand), log_(log) {}

  // verbose: also log every random pick, animation end, pose and blink
  void setVerbose(bool v) { verbose_ = v; }

  // Boot: random mood; Mochi opens its eyes straight into one of its animations
  void begin(uint32_t now) {
    now_ = now;
    for (uint8_t i = 0; i < ANIM_COUNT; i++) lastPlayed_[i] = now;   // starvation counts from boot
    lastInteraction_ = now;
    lastReactionMs_ = now - REACTION_COOLDOWN_MS;   // the very first press always reacts
    player_.screen().setContrast(CONTRAST_AWAKE);
    Mood m = pickMood(MOOD_BOOT_WEIGHT);
    setMood(m, now, "INITIAL MOOD");
    nextEmotion_ = now + scaled(range(EMOTION_MIN_MS, EMOTION_MAX_MS));
    nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
    AnimId id = pick(EMOTIONS[mood_].items, EMOTIONS[mood_].count, 0, "initial");
    beginSequence();
    const ClosedEyes c = closedFor(id);
    queueStill(c.anim, c.frame, EYES_OPEN_BOOT_MS);           // eyes closed ...
    queueMorph(id, 0, morphMs());                              // ... morph open ...
    queue(id, loopsFor(id));                                   // ... into the animation
    run("INITIAL ANIMATION", SETTLE_EMOTION_MS, now);
  }

  // Call every loop()
  void update(uint32_t now) {
    now_ = now;
    player_.update(now);

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
      } else if (!returning_) {
        afterAction(now);                    // short pause, or now and then a resting moment
      } else {
        rest(now);
      }
      return;
    }

    // IDLE: a short pause on the last frame, or a resting moment on the mood's face
    if (now - lastInteraction_ >= SLEEP_AFTER_MS) {
      fallAsleep(now);
      return;
    }
    if ((int32_t)(now - nextAction_) >= 0) {
      if ((int32_t)(now - moodEnd_) >= 0) setMood(nextMood(now), now, "MOOD CHANGE");
      nextAction(now);
    } else if (blinkPending_ && (int32_t)(now - nextBlink_) >= 0) {
      blinkPending_ = false;
      blink(rand_(100) < DOUBLE_BLINK_PCT, now);
    } else if (!resting_ && !player_.busy() && (int32_t)(now - nextBlink_) >= 0 &&
               now - pauseStart_ >= PAUSE_BLINK_DELAY_MS && (int32_t)(nextAction_ - now) >= PAUSE_BLINK_MS) {
      nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);     // a blink in the pause, on any face
      if (player_.blink(now)) {
        lastBlink_ = now;
        log("BLINK", "eyelids");
      }
    }
  }

  // A button press (0 = button 1, 1 = button 2)
  void onButton(uint8_t button, uint32_t now) {
    now_ = now;
    lastInteraction_ = now;
    log(button == 0 ? "BUTTON 1" : "BUTTON 2", "pressed");

    if (state_ == ASLEEP || state_ == FALLING_ASLEEP) {
      wake(now);
      return;
    }

    pressTimes_[pressHead_] = now;           // remember recent presses (either button)
    pressHead_ = (pressHead_ + 1) % PRESS_HISTORY;

    if (now - lastReactionMs_ < REACTION_COOLDOWN_MS) {
      logv("BUTTON", "cooldown, counted only");
      return;
    }
    lastReactionMs_ = now;

    uint8_t presses = recentCount(now);
    AnimId id;
    if (presses >= PRESSES_DIZZY && rand_(100) < 50 && !recent(ANIM_DIZZY, 1)) {
      id = ANIM_DIZZY;                       // pressed again and again: dizzy
      setMood(ANNOYED, now, "MOOD");
      clearPresses();
    } else if (presses >= PRESSES_ANNOYED) { // too many presses: annoyed
      id = pick(REACT_TOO_MUCH, MOCHI_COUNT(REACT_TOO_MUCH), 1, "too many presses");
      if (presses >= PRESSES_DIZZY) clearPresses();
      if (mood_ != ANNOYED && rand_(100) < 70) setMood(ANNOYED, now, "MOOD");
    } else if (button == 0) {                // button 1: friendly -> HAPPY / PLAYFUL
      id = pick(REACT_FRIENDLY, MOCHI_COUNT(REACT_FRIENDLY), 1, "button 1");
      if (mood_ != HAPPY && mood_ != PLAYFUL) setMood(rand_(100) < 65 ? HAPPY : PLAYFUL, now, "MOOD");
      else moodEnd_ = now + range(MOOD_MIN_MS, MOOD_MAX_MS);   // stays happy longer
    } else {                                 // button 2: poke -> CURIOUS / PLAYFUL
      id = pick(REACT_POKE, MOCHI_COUNT(REACT_POKE), 1, "button 2");
      Mood to = (mood_ == HAPPY || mood_ == PLAYFUL || mood_ == EXCITED) ? PLAYFUL : CURIOUS;
      if (mood_ != ANNOYED && mood_ != to && rand_(100) < 70) setMood(to, now, "MOOD");
    }
    act(id, loopsFor(id), "REACTION", now, MORPH_REACT_MS, SETTLE_EMOTION_MS);  // quick morph, starts now
  }

  Mood mood() const { return mood_; }
  State state() const { return state_; }
  const Pose& pose() const { return POSES[mood_][pose_]; }
  uint32_t morphs() const { return morphs_; }      // expression morphs started

private:
  static const uint8_t SEQ_MAX = 12;
  static const uint8_t HISTORY = 4;
  static const uint8_t PRESS_HISTORY = 8;
  static const int16_t WAIT = -2;     // step: keep the image on screen, just wait
  static const int16_t MORPH = -3;    // step: morph to (id, loops = frame) over ms

  struct Step {
    AnimId id;
    uint8_t loops;      // animation: play this many times
    int16_t frame;      // >= 0: still frame ...; WAIT: show nothing new; MORPH: morph to (id, loops)
    uint16_t ms;        // ... for this long
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

  // One animation, morphing into it from whatever is on screen now (also mid-morph)
  void act(AnimId id, uint8_t loops, const char* why, uint32_t now, uint16_t morphMsValue, uint16_t settleMs) {
    restMs_ = 0;                             // (a press can cut a resting moment short)
    blinkPending_ = false;
    resting_ = false;
    beginSequence();
    queueMorph(id, 0, morphMsValue);
    queue(id, loops);
    run(why, settleMs, now);
  }

  void queue(AnimId id, uint8_t loops) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { id, loops, -1, 0 };
  }

  void queueStill(AnimId id, uint8_t frame, uint16_t ms) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { id, 1, (int16_t)frame, ms };
  }

  void queueWait(uint16_t ms) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { ANIM_BLINK, 1, WAIT, ms };
  }

  // Morph from what is on screen (when the step starts) to frame `frame` of `id`
  void queueMorph(AnimId id, uint8_t frame, uint16_t ms) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { id, frame, MORPH, ms };
  }

  // Next animation of a sequence, morphing from the previous one's last frame
  void queueAfter(AnimId id, uint8_t loops) {
    queueMorph(id, 0, morphMs());
    queue(id, loops);
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
      uint8_t steps = MochiPlayer::morphSteps(4, s.ms);
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
    lastPlayed_[s.id] = now;                 // starvation counter starts again
    stepIsAnim_ = true;
    stillUntil_ = now;
    log(stepWhy_, MOCHI_ANIMS[s.id].name);
    logv("ANIMATION START", MOCHI_ANIMS[s.id].name);
    player_.play(s.id, s.loops, now);
  }

  bool stepBusy(uint32_t now) const {
    return player_.busy() || (int32_t)(now - stillUntil_) < 0;
  }

  // An action ended: usually its last frame stays for a short pause and the next
  // animation morphs in; sometimes Mochi first rests on its mood's face for 1-3 s
  void afterAction(uint32_t now) {
    const MoodFlow& f = MOOD_FLOW[mood_];
    if (rand_(100) < f.restPct) {
      restMs_ = (uint16_t)natural(f.restMin, f.restMax);
      returnToRest(now);
      return;
    }
    state_ = IDLE;
    seqLen_ = seqPos_ = 0;
    returning_ = false;
    blinkPending_ = false;                   // (the resting face's own blink)
    resting_ = false;
    pauseStart_ = now;
    uint32_t ms = pauseMs();
    nextAction_ = now + ms;
    if (verbose_) {
      snprintf(text_, sizeof(text_), "%u ms on %s f%u", (unsigned)ms, MOCHI_ANIMS[player_.current()].name,
               (unsigned)player_.frameIndex());
      logv("PAUSE", text_);
    }
  }

  uint32_t pauseMs() {
    const MoodFlow& f = MOOD_FLOW[mood_];
    uint32_t ms = natural(f.pauseMin, f.pauseMax);
    if (mood_ == CURIOUS && rand_(100) < CURIOUS_LOOK_PCT) ms += CURIOUS_LOOK_MS;
    return ms;
  }

  // Next action: an emotion when its timer is due or by chance (sometimes a rare event), else a small action
  void nextAction(uint32_t now) {
    if ((int32_t)(now - nextEmotion_) >= 0 || rand_(100) < EMOTION_CHAIN_PCT) {
      bool special = rand_(100) < SPECIAL_PCT;
      AnimId id = special ? pick(SPECIAL, MOCHI_COUNT(SPECIAL), 4, "rare")
                          : pick(EMOTIONS[mood_].items, EMOTIONS[mood_].count, 4, "emotion");
      nextEmotion_ = now + scaled(range(EMOTION_MIN_MS, EMOTION_MAX_MS));
      if (rand_(100) < 30) pose_ = (uint8_t)rand_(2);   // sometimes rest on the other face later
      act(id, loopsFor(id), special ? "RARE" : "EMOTION", now, morphMs(), SETTLE_EMOTION_MS);
    } else {
      AnimId id = pick(SMALL_ACTIONS[mood_].items, SMALL_ACTIONS[mood_].count, 3, "small");
      act(id, loopsFor(id), "SMALL", now, morphMs(), SETTLE_SMALL_MS);
    }
  }

  // Resting moment: the last frame settles, then a morph to the mood's face
  void returnToRest(uint32_t now) {
    AnimId cur = player_.current();
    uint16_t f = player_.frameIndex();
    const Pose& p = pose();
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
    const Pose& p = pose();
    if (player_.busy() || player_.current() != p.anim || player_.frameIndex() != p.frame) player_.hold(p.anim, p.frame);
    resting_ = true;
    if (!restMs_) return;
    if (verbose_) {
      snprintf(text_, sizeof(text_), "%s f%u for %u ms", MOCHI_ANIMS[p.anim].name, (unsigned)p.frame, (unsigned)restMs_);
      logv("REST", text_);
    }
    nextAction_ = now + restMs_;
    blinkPending_ = restMs_ >= 1200 && now - lastBlink_ >= BLINK_MIN_MS && rand_(100) < REST_BLINK_PCT;
    if (blinkPending_) nextBlink_ = now + 300 + rand_(restMs_ - 900);    // not right at either end
    else if ((int32_t)(nextBlink_ - now) < 0) nextBlink_ = now + restMs_;  // wait for the next pause
    restMs_ = 0;
  }

  // Blink that matches the resting face (it is its own transition: no extra bridge)
  void blink(bool twice, uint32_t now) {
    const Pose& p = pose();
    beginSequence();
    if (p.blink == BLINK_CLIP) {
      queue(ANIM_BLINK, twice ? 2 : 1);
    } else {
      queueStill(p.blinkAnim, p.blinkFrame, BLINK_CLOSED_MS);
      if (twice) {
        queueStill(p.anim, p.frame, BLINK_OPEN_MS);
        queueStill(p.blinkAnim, p.blinkFrame, BLINK_CLOSED_MS);
      }
    }
    if (p.blink != BLINK_CLIP) log(twice ? "BLINK x2" : "BLINK", "closed-eye frame");
    lastBlink_ = now;
    nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
    run(twice ? "BLINK x2" : "BLINK", 0, now);
    returning_ = true;                       // straight back to the face afterwards
  }

  // ---- Sleep / wake ----
  // Called between actions only (IDLE), so it never cuts an animation or morph short.
  // Gradual: relax, eyes squeeze, then SLEEPY_3's eyes close (frames 0 -> 1 -> 2)
  void fallAsleep(uint32_t now) {
    beginSequence();
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
    setMood(SLEEPY, now, "MOOD");
    log("SLEEP", "falling asleep");
    run("SLEEP SEQ", 0, now);
    state_ = FALLING_ASLEEP;
  }

  void sleep(uint32_t now) {
    state_ = ASLEEP;
    seqLen_ = seqPos_ = 0;
    player_.hold(SLEEP_ANIM, SLEEP_FRAME);
    breathFrame_ = SLEEP_FRAME;
    breathLanding_ = false;
    nextBreath_ = now + range(BREATH_REST_MIN_MS, BREATH_REST_MAX_MS);
    log("SLEEP", "asleep");
  }

  // Asleep: the lids drift up a little and settle again, slowly and never quite the same
  void breathe(uint32_t now) {
    if (player_.busy()) return;                  // a breath (morph) is on its way
    if (breathLanding_) {                        // morph done: show the frame itself
      player_.hold(SLEEP_ANIM, breathFrame_);
      breathLanding_ = false;
    }
    if ((int32_t)(now - nextBreath_) < 0) return;
    bool in = breathFrame_ == SLEEP_FRAME;
    breathFrame_ = in ? DOZE_FRAME : SLEEP_FRAME;
    uint16_t ms = (uint16_t)(in ? range(BREATH_IN_MIN_MS, BREATH_IN_MAX_MS) : range(BREATH_OUT_MIN_MS, BREATH_OUT_MAX_MS));
    player_.morphTo(SLEEP_ANIM, breathFrame_, BREATH_STEPS, ms, now);
    breathLanding_ = true;
    nextBreath_ = now + ms + (in ? range(BREATH_HOLD_MIN_MS, BREATH_HOLD_MAX_MS) : range(BREATH_REST_MIN_MS, BREATH_REST_MAX_MS));
    logv("BREATH", in ? "in" : "out");
  }

  // Immediate: the sleeping eyes open into SURPRISED (morph starts now), then a blink
  void wake(uint32_t now) {
    breathLanding_ = false;
    beginSequence();
    queueMorph(ANIM_SURPRISED, 0, MORPH_MIN_MS);    // sleeping eyes open up
    queue(ANIM_SURPRISED, 1);
    queueAfter(ANIM_BLINK, 1);
    log("WAKE", "woken up");
    setMood(rand_(100) < 60 ? HAPPY : NEUTRAL, now, "MOOD");
    nextEmotion_ = now + scaled(range(EMOTION_MIN_MS, EMOTION_MAX_MS));
    lastBlink_ = now;                                // the wake sequence ends with a blink
    nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
    run("WAKE SEQ", 0, now);
    state_ = WAKING;
  }

  // ---- Mood ----
  void setMood(Mood m, uint32_t now, const char* why) {
    if (m != mood_ || !moodSet_) {
      if (moodSet_) snprintf(text_, sizeof(text_), "%s -> %s", MOOD_NAMES[mood_], MOOD_NAMES[m]);
      else snprintf(text_, sizeof(text_), "%s", MOOD_NAMES[m]);
      log(why, text_);
      pose_ = (uint8_t)rand_(2);             // a new mood gets one of its two faces
    }
    mood_ = m;
    moodSet_ = true;
    moodEnd_ = now + range(MOOD_MIN_MS, MOOD_MAX_MS);
  }

  Mood pickMood(const uint8_t* weights) {
    uint16_t total = 0;
    for (uint8_t m = 0; m < MOOD_COUNT; m++) total += weights[m];
    uint32_t r = rand_(total);
    for (uint8_t m = 0; m < MOOD_COUNT; m++) {
      if (r < weights[m]) return (Mood)m;
      r -= weights[m];
    }
    return NEUTRAL;
  }

  Mood nextMood(uint32_t now) {
    uint32_t idleMin = (now - lastInteraction_) / 60000;
    uint8_t w[MOOD_COUNT];
    for (uint8_t m = 0; m < MOOD_COUNT; m++) {
      uint32_t v = MOOD_NEXT_WEIGHT[m];
      if (m == SLEEPY) v += idleMin * 10;      // no attention -> sleepier
      if (m == mood_) v = 0;                   // a new mood, not the same again
      w[m] = (uint8_t)(v > 255 ? 255 : v);
    }
    return pickMood(w);
  }

  // Weight of an eligible pick after the starvation boost (percent units)
  uint32_t boosted(const WeightedAnim& w) const {
    uint32_t since = now_ - lastPlayed_[w.id];
    for (const StarveStep& s : STARVE_BOOST)
      if (since >= s.afterMs) return (uint32_t)w.weight * s.pct;
    return (uint32_t)w.weight * 100;
  }

  // ---- Weighted pick (starvation-boosted) that avoids the last `avoid` animations ----
  AnimId pick(const WeightedAnim* items, uint8_t n, uint8_t avoid, const char* what) {
    for (; ; avoid = avoid > 1 ? 1 : 0) {    // relax if every option was played recently
      uint32_t total = 0;
      for (uint8_t i = 0; i < n; i++)
        if (!recent(items[i].id, avoid)) total += boosted(items[i]);
      if (total > 0 || avoid == 0) {
        uint32_t r = rand_(total), r0 = r;
        for (uint8_t i = 0; i < n; i++) {
          if (recent(items[i].id, avoid)) continue;
          uint32_t w = boosted(items[i]);
          if (r < w) {
            if (verbose_) {
              snprintf(text_, sizeof(text_), "%s: random %u of %u -> %s (%s)", what, (unsigned)r0,
                       (unsigned)total, MOCHI_ANIMS[items[i].id].name, MOOD_NAMES[mood_]);
              logv("PICK", text_);
            }
            return items[i].id;
          }
          r -= w;
        }
      }
    }
  }

  bool recent(AnimId id, uint8_t depth) const {
    for (uint8_t i = 0; i < depth && i < historyLen_; i++)
      if (history_[(historyHead_ + HISTORY - 1 - i) % HISTORY] == id) return true;
    return false;
  }

  void remember(AnimId id) {
    history_[historyHead_] = id;
    historyHead_ = (historyHead_ + 1) % HISTORY;
    if (historyLen_ < HISTORY) historyLen_++;
  }

  // ---- Helpers ----
  uint8_t loopsFor(AnimId id) {
    // Short 250frames clips (< 1 s) read better when repeated a couple of times
    return MOCHI_ANIMS[id].frameCount < 12 ? (uint8_t)(2 + rand_(2)) : 1;
  }

  uint32_t range(uint32_t lo, uint32_t hi) { return lo + rand_(hi - lo + 1); }
  // lo..hi, more often near the middle than at the ends (sum of two random draws)
  uint32_t natural(uint32_t lo, uint32_t hi) { return (range(lo, hi) + range(lo, hi)) / 2; }
  uint32_t scaled(uint32_t ms) const { return ms * MOOD_TEMPO_PCT[mood_] / 100; }

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
  char text_[72];

  State state_ = IDLE;
  Mood mood_ = NEUTRAL;
  bool moodSet_ = false;
  uint8_t pose_ = 0;
  uint32_t moodEnd_ = 0;

  uint32_t nextBlink_ = 0, nextEmotion_ = 0, nextAction_ = 0, lastBlink_ = 0;
  bool blinkPending_ = false;            // a blink is planned inside the current resting moment
  bool resting_ = false;                 // IDLE on the mood's face (else: a short pause on a last frame)
  uint32_t pauseStart_ = 0;
  uint16_t restMs_ = 0;                  // length of the resting moment being entered (0: none)
  uint32_t lastInteraction_ = 0, lastReactionMs_ = 0;
  uint32_t stillUntil_ = 0;
  uint32_t nextBreath_ = 0;
  uint8_t breathFrame_ = SLEEP_FRAME;    // asleep: the frame the face is on / breathing toward
  bool breathLanding_ = false;           // a breath morph is running; show its frame when done
  bool stepIsAnim_ = false;
  bool returning_ = false;               // the current sequence is the way back to the resting face
  uint16_t settleMs_ = 0;                // how long the last frame stays after the current action
  uint32_t morphs_ = 0;                  // expression morphs started

  Step seq_[SEQ_MAX];
  uint8_t seqLen_ = 0, seqPos_ = 0;
  const char* stepWhy_ = "";

  AnimId history_[HISTORY] = { ANIM_BLINK, ANIM_BLINK, ANIM_BLINK, ANIM_BLINK };
  uint32_t lastPlayed_[ANIM_COUNT] = { 0 };   // when each animation last started (starvation boost)
  uint32_t now_ = 0;                          // time of the current update()/onButton()
  uint8_t historyHead_ = 0, historyLen_ = 0;

  uint32_t pressTimes_[PRESS_HISTORY] = { 0 };
  uint8_t pressHead_ = 0;
};
