// =====================================================
// MOCHI BEHAVIOR — mood + autonomous animation choice
//
//   mood ─► behavior (idle timers, events, buttons) ─► AnimId ─► MochiPlayer
//
// At boot Mochi picks a random mood and immediately plays one of that mood's
// animations. Between actions it rests on a calm face that belongs to the
// current mood (a still frame of an existing animation) and blinks with a
// matching closed-eye frame. On randomized timers it does a small action or
// an emotion chosen by weighted random from its mood, avoiding recent picks.
// Moods last 1.5-4 minutes. After 10 minutes without a button press it falls
// asleep; a button wakes it. Plain C++: time comes in as `now` (ms),
// randomness through rand(n).
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
static const uint32_t BLINK_MIN_MS = 1500,  BLINK_MAX_MS = 4000;
static const uint32_t SMALL_MIN_MS = 2000,  SMALL_MAX_MS = 4000;
static const uint32_t EMOTION_MIN_MS = 3000, EMOTION_MAX_MS = 5000;
static const uint32_t MOOD_MIN_MS = 10000,  MOOD_MAX_MS = 15000;
static const uint32_t SLEEP_AFTER_MS = 120000;        // no button press -> fall asleep
static const uint32_t SCREEN_OFF_AFTER_MS = 30000;    // asleep this long -> OLED off
#else
static const uint32_t BLINK_MIN_MS = 2000,  BLINK_MAX_MS = 7000;
static const uint32_t SMALL_MIN_MS = 4000,  SMALL_MAX_MS = 10000;
static const uint32_t EMOTION_MIN_MS = 12000, EMOTION_MAX_MS = 30000;
static const uint32_t MOOD_MIN_MS = 90000,  MOOD_MAX_MS = 240000;
static const uint32_t SLEEP_AFTER_MS = 10UL * 60 * 1000;      // no button press -> fall asleep
static const uint32_t SCREEN_OFF_AFTER_MS = 20UL * 60 * 1000; // asleep this long -> OLED off
#endif
static const uint32_t REST_AFTER_ACTION_MS = 1200;      // minimum calm time after an action
static const uint32_t REACTION_COOLDOWN_MS = 600;       // presses inside this only count
static const uint32_t PRESS_WINDOW_MS     = 3000;       // window for counting rapid presses
static const uint8_t  PRESSES_ANNOYED     = 3;          // rapid presses -> annoyed
static const uint8_t  PRESSES_DIZZY       = 5;          // more rapid presses -> sometimes dizzy
static const uint8_t  DOUBLE_BLINK_PCT    = 20;
static const uint8_t  SPECIAL_PCT         = 5;          // chance an emotion is a rare event
static const uint16_t BLINK_CLOSED_MS     = 110;        // closed-eye frame of a frame blink
static const uint16_t BLINK_OPEN_MS       = 140;        // open again between a double blink

// Speed of a mood's small actions and emotions (percent of the base interval)
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
  { { ANIM_ANNOYED, 3, BLINK_FRAME, CLOSED_250 },    { ANIM_FRUSTRATED, 45, BLINK_FRAME, CLOSED_GIF } },  // ANNOYED
  { { ANIM_CRYING, 3, BLINK_FRAME, CLOSED_250 },     { ANIM_FRUSTRATED, 74, BLINK_FRAME, CLOSED_GIF } },  // SAD
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
  { ANIM_CONFUSED_2, 15 }, { ANIM_BUG, 10 }, { ANIM_MOCHI_26, 10 } };
static const WeightedAnim EMO_PLAYFUL[] = {
  { ANIM_EMBARRASSED, 18 }, { ANIM_HAPPY_2, 15 }, { ANIM_CONTENT, 15 }, { ANIM_DETERMINED, 12 },
  { ANIM_EXCITED_2, 10 }, { ANIM_LAUGH, 10 }, { ANIM_UWU, 10 }, { ANIM_KISS, 8 } };
static const WeightedAnim EMO_EXCITED[] = {
  { ANIM_ANGRY_2, 20 }, { ANIM_CONFUSED_2, 15 }, { ANIM_DETERMINED, 15 }, { ANIM_LAUGH, 15 },
  { ANIM_SURPRISED, 10 }, { ANIM_EXCITED_2, 10 } };
static const WeightedAnim EMO_ANNOYED[] = {
  { ANIM_ANNOYED, 30 }, { ANIM_SQUINT, 20 }, { ANIM_FRUSTRATED, 20 }, { ANIM_ANGRY_3, 15 },
  { ANIM_FURIOUS, 6 } };
static const WeightedAnim EMO_SAD[] = {
  { ANIM_CRYING, 30 }, { ANIM_SLEEPY_3, 30 }, { ANIM_FRUSTRATED, 15 }, { ANIM_SQUINT, 10 },
  { ANIM_RELAXED, 10 } };
static const WeightedAnim EMO_SLEEPY[] = {
  { ANIM_RELAXED, 30 }, { ANIM_SQUINT, 25 }, { ANIM_FRUSTRATED, 15 }, { ANIM_HAPPY, 10 },
  { ANIM_ANGRY, 10 } };

// Small actions: short and subtle, but several per mood so no clip dominates
static const WeightedAnim SMALL_NEUTRAL[] = {
  { ANIM_MOCHI_10, 20 }, { ANIM_SMILE, 20 }, { ANIM_HAPPY_3, 15 }, { ANIM_HAPPY, 15 },
  { ANIM_SURPRISED, 10 }, { ANIM_SQUINT, 10 }, { ANIM_RELAXED, 10 } };
static const WeightedAnim SMALL_HAPPY[] = {
  { ANIM_SMILE, 20 }, { ANIM_HAPPY_3, 20 }, { ANIM_HAPPY, 15 }, { ANIM_MOCHI_10, 10 },
  { ANIM_UWU, 10 }, { ANIM_PROUD, 10 }, { ANIM_KISS, 8 }, { ANIM_LOVE, 7 } };
static const WeightedAnim SMALL_CURIOUS[] = {
  { ANIM_MOCHI_10, 25 }, { ANIM_SURPRISED, 25 }, { ANIM_SMILE, 10 }, { ANIM_BUG, 10 },
  { ANIM_MOCHI_26, 10 }, { ANIM_CONFUSED_2, 10 }, { ANIM_RELAXED, 10 } };
static const WeightedAnim SMALL_PLAYFUL[] = {
  { ANIM_HAPPY_3, 20 }, { ANIM_UWU, 15 }, { ANIM_SMILE, 15 }, { ANIM_MOCHI_10, 15 },
  { ANIM_EMBARRASSED, 15 }, { ANIM_KISS, 10 }, { ANIM_HAPPY_2, 10 } };
static const WeightedAnim SMALL_EXCITED[] = {
  { ANIM_SURPRISED, 25 }, { ANIM_HAPPY_3, 20 }, { ANIM_SMILE, 15 }, { ANIM_UWU, 15 },
  { ANIM_ANGRY_2, 15 }, { ANIM_LAUGH, 10 } };
static const WeightedAnim SMALL_ANNOYED[] = {
  { ANIM_SQUINT, 30 }, { ANIM_ANNOYED, 30 }, { ANIM_ANGRY_3, 20 }, { ANIM_MOCHI_10, 10 },
  { ANIM_FRUSTRATED, 10 } };
static const WeightedAnim SMALL_SAD[] = {
  { ANIM_CRYING, 30 }, { ANIM_MOCHI_10, 25 }, { ANIM_SQUINT, 25 }, { ANIM_SLEEPY_3, 20 } };
static const WeightedAnim SMALL_SLEEPY[] = {
  { ANIM_SQUINT, 35 }, { ANIM_RELAXED, 35 }, { ANIM_MOCHI_10, 30 } };

// Rare personality events (any mood)
static const WeightedAnim SPECIAL[] = {
  { ANIM_EVIL, 20 }, { ANIM_EVIL_GRIN, 15 }, { ANIM_BUG, 20 }, { ANIM_MOCHI_11, 15 },
  { ANIM_MOCHI_29, 15 }, { ANIM_MOCHI_26, 10 }, { ANIM_UWU, 10 }, { ANIM_DIZZY, 8 },
  { ANIM_SURPRISED, 10 }, { ANIM_SCREAM, 4 } };

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

  // Boot: random mood, then straight into one of its animations
  void begin(uint32_t now) {
    lastInteraction_ = now;
    Mood m = pickMood(MOOD_BOOT_WEIGHT);
    setMood(m, now, "INITIAL MOOD");
    nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
    nextSmall_ = now + scaled(range(SMALL_MIN_MS, SMALL_MAX_MS));
    nextEmotion_ = now + scaled(range(EMOTION_MIN_MS, EMOTION_MAX_MS));
    AnimId id = pick(EMOTIONS[mood_].items, EMOTIONS[mood_].count, 0, "initial");
    act(id, loopsFor(id), "INITIAL ANIMATION", now);
  }

  // Call every loop()
  void update(uint32_t now) {
    player_.update(now);

    if (state_ == ASLEEP) {
      if (!screenOff_ && now - asleepSince_ >= SCREEN_OFF_AFTER_MS) {
        player_.screen().setPower(false);
        screenOff_ = true;
        log("SCREEN", "off");
      }
      return;
    }

    if (state_ != IDLE) {                    // a sequence is playing
      if (stepBusy(now)) return;
      if (stepIsAnim_) logv("ANIMATION END", MOCHI_ANIMS[player_.current()].name);
      if (seqPos_ < seqLen_) {
        startStep(now);
        return;
      }
      if (state_ == FALLING_ASLEEP) {
        sleep(now);
      } else {
        rest(now);
      }
      return;
    }

    // IDLE: resting on the mood's face
    if (now - lastInteraction_ >= SLEEP_AFTER_MS) {
      fallAsleep(now);
      return;
    }
    if ((int32_t)(now - moodEnd_) >= 0) setMood(nextMood(now), now, "MOOD CHANGE");

    if ((int32_t)(now - nextEmotion_) >= 0) {
      bool special = rand_(100) < SPECIAL_PCT;
      AnimId id = special ? pick(SPECIAL, MOCHI_COUNT(SPECIAL), 4, "rare")
                          : pick(EMOTIONS[mood_].items, EMOTIONS[mood_].count, 4, "emotion");
      nextEmotion_ = now + scaled(range(EMOTION_MIN_MS, EMOTION_MAX_MS));
      nextSmall_ = now + scaled(range(SMALL_MIN_MS, SMALL_MAX_MS));
      if (rand_(100) < 30) pose_ = (uint8_t)rand_(2);   // sometimes rest on the other face after
      act(id, loopsFor(id), special ? "RARE" : "EMOTION", now);
    } else if ((int32_t)(now - nextSmall_) >= 0) {
      AnimId id = pick(SMALL_ACTIONS[mood_].items, SMALL_ACTIONS[mood_].count, 3, "small");
      nextSmall_ = now + scaled(range(SMALL_MIN_MS, SMALL_MAX_MS));
      act(id, loopsFor(id), "SMALL", now);
    } else if ((int32_t)(now - nextBlink_) >= 0) {
      nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
      blink(rand_(100) < DOUBLE_BLINK_PCT, now);
    }
  }

  // A button press (0 = button 1, 1 = button 2)
  void onButton(uint8_t button, uint32_t now) {
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
    act(id, loopsFor(id), "REACTION", now);  // interrupts whatever is playing
  }

  Mood mood() const { return mood_; }
  State state() const { return state_; }
  bool screenOff() const { return screenOff_; }
  const Pose& pose() const { return POSES[mood_][pose_]; }

private:
  static const uint8_t SEQ_MAX = 4;
  static const uint8_t HISTORY = 4;
  static const uint8_t PRESS_HISTORY = 8;

  struct Step {
    AnimId id;
    uint8_t loops;      // animation: play this many times
    int16_t frame;      // >= 0: show this still frame instead ...
    uint16_t ms;        // ... for this long
  };

  // ---- Sequences ----
  void act(AnimId id, uint8_t loops, const char* why, uint32_t now) {
    seqLen_ = 0;
    seqPos_ = 0;
    queue(id, loops);
    state_ = ACTING;
    stepWhy_ = why;
    startStep(now);
  }

  void queue(AnimId id, uint8_t loops) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { id, loops, -1, 0 };
  }

  void queueStill(AnimId id, uint8_t frame, uint16_t ms) {
    if (seqLen_ < SEQ_MAX) seq_[seqLen_++] = { id, 1, (int16_t)frame, ms };
  }

  void startStep(uint32_t now) {
    const Step& s = seq_[seqPos_++];
    if (s.frame >= 0) {                      // still frame (frame blink)
      player_.hold(s.id, (uint16_t)s.frame);
      stillUntil_ = now + s.ms;
      stepIsAnim_ = false;
      return;
    }
    if (s.id != ANIM_BLINK) remember(s.id);
    stepIsAnim_ = true;
    stillUntil_ = now;
    log(stepWhy_, MOCHI_ANIMS[s.id].name);
    logv("ANIMATION START", MOCHI_ANIMS[s.id].name);
    player_.play(s.id, s.loops, now);
  }

  bool stepBusy(uint32_t now) const {
    return player_.busy() || (int32_t)(now - stillUntil_) < 0;
  }

  // Back to the mood's resting face; keep a short calm moment before the next action
  void rest(uint32_t now) {
    state_ = IDLE;
    seqLen_ = seqPos_ = 0;
    const Pose& p = pose();
    player_.hold(p.anim, p.frame);
    logv("POSE", MOCHI_ANIMS[p.anim].name);
    uint32_t calm = now + REST_AFTER_ACTION_MS;
    if ((int32_t)(nextBlink_ - calm) < 0) nextBlink_ = calm + rand_(1800);
    if ((int32_t)(nextSmall_ - calm) < 0) nextSmall_ = calm + 1000 + rand_(3000);
    if ((int32_t)(nextEmotion_ - calm) < 0) nextEmotion_ = calm + 1000 + rand_(3000);
  }

  // Blink that matches the resting face
  void blink(bool twice, uint32_t now) {
    const Pose& p = pose();
    seqLen_ = seqPos_ = 0;
    if (p.blink == BLINK_CLIP) {
      queue(ANIM_BLINK, twice ? 2 : 1);
    } else {
      queueStill(p.blinkAnim, p.blinkFrame, BLINK_CLOSED_MS);
      if (twice) {
        queueStill(p.anim, p.frame, BLINK_OPEN_MS);
        queueStill(p.blinkAnim, p.blinkFrame, BLINK_CLOSED_MS);
      }
    }
    state_ = ACTING;
    stepWhy_ = twice ? "BLINK x2" : "BLINK";
    if (p.blink != BLINK_CLIP) log(stepWhy_, "closed-eye frame");
    startStep(now);
  }

  // ---- Sleep / wake ----
  void fallAsleep(uint32_t now) {
    seqLen_ = seqPos_ = 0;
    if (!recent(ANIM_RELAXED, 1)) queue(ANIM_RELAXED, 1);   // relax, then eyes squeeze shut
    queue(ANIM_SQUINT, 2);
    state_ = FALLING_ASLEEP;
    setMood(SLEEPY, now, "MOOD");
    log("SLEEP", "falling asleep");
    stepWhy_ = "SLEEP SEQ";
    startStep(now);
  }

  void sleep(uint32_t now) {
    state_ = ASLEEP;
    asleepSince_ = now;
    seqLen_ = seqPos_ = 0;
    player_.hold(POSE_SLEEP_ANIM, POSE_SLEEP_FRAME);
    player_.screen().setDim(true);
    log("SLEEP", "asleep (dimmed)");
  }

  void wake(uint32_t now) {
    if (screenOff_) player_.screen().setPower(true);
    screenOff_ = false;
    player_.screen().setDim(false);
    seqLen_ = seqPos_ = 0;
    queue(ANIM_SURPRISED, 1);
    queue(ANIM_BLINK, 1);
    state_ = WAKING;
    log("WAKE", "woken up");
    setMood(rand_(100) < 60 ? HAPPY : NEUTRAL, now, "MOOD");
    nextBlink_ = now + range(BLINK_MIN_MS, BLINK_MAX_MS);
    nextSmall_ = now + scaled(range(SMALL_MIN_MS, SMALL_MAX_MS));
    nextEmotion_ = now + scaled(range(EMOTION_MIN_MS, EMOTION_MAX_MS));
    stepWhy_ = "WAKE SEQ";
    startStep(now);
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

  // ---- Weighted pick that avoids the last `avoid` animations ----
  AnimId pick(const WeightedAnim* items, uint8_t n, uint8_t avoid, const char* what) {
    for (; ; avoid = avoid > 1 ? 1 : 0) {    // relax if every option was played recently
      uint16_t total = 0;
      for (uint8_t i = 0; i < n; i++)
        if (!recent(items[i].id, avoid)) total += items[i].weight;
      if (total > 0 || avoid == 0) {
        uint32_t r = rand_(total), r0 = r;
        for (uint8_t i = 0; i < n; i++) {
          if (recent(items[i].id, avoid)) continue;
          if (r < items[i].weight) {
            if (verbose_) {
              snprintf(text_, sizeof(text_), "%s: random %u of %u -> %s (%s)", what, (unsigned)r0,
                       (unsigned)total, MOCHI_ANIMS[items[i].id].name, MOOD_NAMES[mood_]);
              logv("PICK", text_);
            }
            return items[i].id;
          }
          r -= items[i].weight;
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

  uint32_t nextBlink_ = 0, nextSmall_ = 0, nextEmotion_ = 0;
  uint32_t lastInteraction_ = 0, lastReactionMs_ = 0, asleepSince_ = 0;
  uint32_t stillUntil_ = 0;
  bool screenOff_ = false;
  bool stepIsAnim_ = false;

  Step seq_[SEQ_MAX];
  uint8_t seqLen_ = 0, seqPos_ = 0;
  const char* stepWhy_ = "";

  AnimId history_[HISTORY] = { ANIM_BLINK, ANIM_BLINK, ANIM_BLINK, ANIM_BLINK };
  uint8_t historyHead_ = 0, historyLen_ = 0;

  uint32_t pressTimes_[PRESS_HISTORY] = { 0 };
  uint8_t pressHead_ = 0;
};
