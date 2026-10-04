// =====================================================
// MOCHI BUTTONS — two buttons, single presses and "both together"
//
// Same scheme as the V3 media viewer: a press is debounced (contact bounce), then it
// waits a short combo window before it counts as a single press, so pressing both
// buttons together is one action (BOTH) and never a button 1 / button 2 press as well.
// After BOTH, nothing else fires until both buttons are released again.
// Plain C++ (the sketch reads the pins): feed it the raw levels and the time.
// =====================================================

#pragma once

#include <stdint.h>

// Scoped names: the sketch's pin macros (BTN_1 / BTN_2) can never replace them
enum class ButtonEvent : uint8_t { None, Button1, Button2, Both };

class MochiButtons {
public:
  static const uint32_t STABLE_MS = 30;      // contact-bounce filter
  static const uint32_t COMBO_MS  = 150;     // time to press the second button for BOTH (>= 110 ms apart
                                             // by hand, even while the loop is busy drawing frames)

  // raw1 / raw2: true while the button is held down. Returns at most one event per call.
  ButtonEvent update(bool raw1, bool raw2, uint32_t now) {
    debounce(b_[0], raw1, now);
    debounce(b_[1], raw2, now);
    if (latched_) {                          // BOTH handled: wait until both are released
      if (!b_[0].down && !b_[1].down) latched_ = false;
      b_[0].pending = b_[1].pending = false;
      return ButtonEvent::None;
    }
    if (b_[0].down && b_[1].down) {          // both held: one action, before any single press
      latched_ = true;
      b_[0].pending = b_[1].pending = false;
      return ButtonEvent::Both;
    }
    if (ready(b_[0], now)) return ButtonEvent::Button1;
    if (ready(b_[1], now)) return ButtonEvent::Button2;
    return ButtonEvent::None;
  }

private:
  struct Btn {
    bool down = false, raw = false, pending = false;
    uint32_t rawChangedAt = 0, pressedAt = 0;
  };

  static void debounce(Btn& b, bool raw, uint32_t now) {
    if (raw != b.raw) {
      b.raw = raw;
      b.rawChangedAt = now;
    }
    if (b.down != b.raw && now - b.rawChangedAt >= STABLE_MS) {
      b.down = b.raw;
      if (b.down) {
        b.pending = true;
        b.pressedAt = now;
      }
    }
  }

  // A single press fires once the combo window has passed, or on an earlier release
  static bool ready(Btn& b, uint32_t now) {
    if (!b.pending) return false;
    if (b.down && now - b.pressedAt < COMBO_MS) return false;
    b.pending = false;
    return true;
  }

  Btn b_[2];
  bool latched_ = false;
};
