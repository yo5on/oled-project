// =====================================================
// MOCHI EMOTION — what Mochi feels, and how it changes over time
//
// A few feelings (0..1000, happiness -1000..1000) rise and fall with events (button
// presses, see MochiBehavior::onButton) and drift back toward Mochi's own resting
// levels as time passes. Nothing here is random: a small personality, chosen once at
// boot, only sets those resting levels.
// The mood is derived from the feelings (score(): the strongest one wins); the
// behavior adds inertia (minimum time in a mood, a clear margin to change).
// =====================================================

#pragma once

#include <stdint.h>

enum Mood : uint8_t {
  NEUTRAL,
  HAPPY,
  CURIOUS,
  PLAYFUL,
  EXCITED,
  ANNOYED,
  SAD,
  SLEEPY,
  AFFECTIONATE,
  MOOD_COUNT
};

static const char* const MOOD_NAMES[MOOD_COUNT] = {
  "NEUTRAL", "HAPPY", "CURIOUS", "PLAYFUL", "EXCITED", "ANNOYED", "SAD", "SLEEPY", "AFFECTIONATE"
};

// Resting levels that make one Mochi a little different from another (set at boot)
struct Personality {
  int16_t happy;      // cheerful .. gloomy
  int16_t affection;  // cuddly .. aloof
  int16_t playful;
  int16_t curious;
};

struct MochiEmotion {
  int16_t happy = 200;       // -1000 .. 1000
  int16_t affection = 400;   // bond with the person; slow
  int16_t playful = 100;
  int16_t annoy = 0;
  int16_t curious = 300;
  int16_t energy = 800;      // falls while awake (faster when nothing happens), sleep restores it
  int16_t excite = 100;      // intensity of the moment; fades within seconds
  int16_t grudge = 0;        // remembers recent anger for a few minutes
  Personality p = { 200, 400, 100, 300 };

  // One second of time. idleS: seconds since the last button press.
  void tick(uint32_t idleS, bool asleep) {
    if (asleep) {
      add(energy, 3, 0, 1000);
      toward(happy, p.happy, 8);
      toward(annoy, 0, 25);
      toward(playful, p.playful, 15);
      toward(curious, 200, 10);
      toward(excite, 50, 55);
      toward(grudge, 0, 3);
      return;
    }
    // left alone for long, Mochi feels a little neglected (more when attached)
    int16_t hRest = p.happy;
    if (idleS > 300) {
      int32_t drop = (int32_t)(idleS - 300) * (affection >= 500 ? 2 : 1);
      hRest -= (int16_t)(drop > 700 ? 700 : drop);
    }
    toward(happy, hRest, 4);                                  // half-life ~3 min: a good mood lasts
    toward(affection, idleS > 600 ? 200 : p.affection, 1);    // the bond changes slowly
    toward(playful, p.playful, 8);                            // ~1.5 min
    toward(annoy, 0, 17);                                     // ~40 s
    toward(grudge, annoy > grudge ? annoy : 0, annoy > grudge ? 1000 : 3);   // holds the peak, fades over minutes
    // quiet makes Mochi curious (1.5-7 min; less when it is very happy), then boredom takes over
    int16_t cRest = idleS < 90 ? p.curious : idleS < 420 ? (int16_t)(happy > 350 ? 450 : 650) : 250;
    toward(curious, cRest, 8);
    toward(excite, 100, 35);                                  // ~20 s
    // energy: slowly while awake, faster when nothing happens (drowsy)
    energyAcc_ += idleS > 240 ? 12 : 2;
    energy -= (int16_t)(energyAcc_ / 10);
    energyAcc_ %= 10;
    if (energy < 0) energy = 0;
  }

  // How strongly each mood fits the feelings right now (0 .. ~1000)
  int16_t score(Mood m) const {
    switch (m) {
      case NEUTRAL:      return 300;
      case HAPPY:        return happy - 50;
      case AFFECTIONATE: return affection >= 500 ? (int16_t)(affection - 200 + happy / 2) : 0;
      case PLAYFUL:      return playful + (happy > 200 ? 50 : -100);
      case EXCITED:      return happy > 100 ? excite - 150 : 0;
      case CURIOUS:      return curious - 150;
      case ANNOYED:      return annoy + 50;
      case SAD:          return 50 - happy;
      case SLEEPY:       return 650 - energy;
      default:           return 0;
    }
  }

  static void add(int16_t& v, int16_t d, int16_t lo, int16_t hi) {
    int32_t x = (int32_t)v + d;
    v = (int16_t)(x < lo ? lo : x > hi ? hi : x);
  }

  // Move v toward target by perMille of the distance (at least one step)
  static void toward(int16_t& v, int16_t target, int16_t perMille) {
    int32_t d = ((int32_t)target - v) * perMille / 1000;
    if (d == 0 && v != target) d = target > v ? 1 : -1;
    v = (int16_t)(v + d);
  }

  static int16_t min16(int16_t a, int16_t b) { return a < b ? a : b; }

private:
  uint16_t energyAcc_ = 0;
};
