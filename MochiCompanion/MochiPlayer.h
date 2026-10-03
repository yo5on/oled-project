// =====================================================
// MOCHI PLAYER — plays one MochiAnim at a time, non-blocking
//
// The behavior engine only calls play(), hold(), morphTo(), blink(), busy() and update().
// Frame data stays in flash: RAW frames are drawn straight from flash,
// PACKED frames are decoded one at a time into a single 1024-byte buffer.
// morphTo() morphs from exactly what is on screen to any frame (MochiMorph.h); its
// in-between frames are scheduled from the morph's start, never faster than the OLED
// can show them. blink() closes and opens the eyes of what is on screen.
// The screen is behind MochiScreen (OLED on the ESP32).
// =====================================================

#pragma once

#include <stdint.h>
#include <string.h>
#include "MochiAnimations.h"
#include "MochiMorph.h"

// What the player needs from a display
class MochiScreen {
public:
  // A complete 128x64 frame (1024 bytes); inverted: show it with every pixel flipped
  virtual void showFrame(const uint8_t* frame, bool inverted) = 0;
  virtual void setContrast(uint8_t level) = 0;                        // brightness 0..255
};

// One full OLED frame takes ~32 ms (render + I2C): morph steps are never planned closer
static const uint16_t MORPH_MIN_STEP_MS = 34;

// Eyelid blink: half shut, shut, half open, open (the image from before the blink)
struct BlinkStep { uint16_t open, ms; };
static const BlinkStep BLINK_STEPS[4] = { { 100, 35 }, { 0, 90 }, { 100, 35 }, { 256, 0 } };

class MochiPlayer {
public:
  explicit MochiPlayer(MochiScreen& screen) : screen_(screen) {}

  // Start an animation now; it plays `loops` times, then stays on its last frame.
  void play(AnimId id, uint8_t loops, uint32_t now) {
    anim_ = id;
    loopsLeft_ = loops ? loops : 1;
    playing_ = true;
    frameIdx_ = 0;
    showCurrent();
    lastFrameMs_ = now;
  }

  // Show one still frame of an animation (no playback)
  void hold(AnimId id, uint16_t frame) {
    const MochiAnim& a = MOCHI_ANIMS[id];
    anim_ = id;
    playing_ = false;
    frameIdx_ = frame < a.frameCount ? frame : a.frameCount - 1;
    showCurrent();
  }

  // In-between frames a morph of `ms` gets: up to `maxSteps`, with every step (and the
  // last one to the destination) at least MORPH_MIN_STEP_MS apart; at least one.
  static uint8_t morphSteps(uint8_t maxSteps, uint16_t ms) {
    uint16_t fit = ms / MORPH_MIN_STEP_MS;   // steps + 1 intervals fit
    uint8_t n = fit > 1 ? (uint8_t)(fit - 1) : 1;
    return n < maxSteps ? n : maxSteps;
  }

  // Morph from what is on screen now to frame `frame` of `id` over `ms`: up to `steps`
  // in-between frames (see morphSteps), step k due at now + k * ms / (steps + 1); the
  // morph ends at now + ms (the destination itself is shown by the next play()/hold()).
  void morphTo(AnimId id, uint16_t frame, uint8_t steps, uint16_t ms, uint32_t now) {
    const MochiAnim& a = MOCHI_ANIMS[id];
    if (frame >= a.frameCount) frame = a.frameCount - 1;
    const uint8_t* src = this->frame(a, frame);
    for (uint16_t i = 0; i < 1024; i++) target_[i] = a.inverted ? (uint8_t)~pgm_read_byte(src + i) : pgm_read_byte(src + i);
    morph_.begin(shown_, target_);
    blinking_ = false;
    morphing_ = true;
    playing_ = true;
    mStep_ = 0;
    mSteps_ = morphSteps(steps ? steps : 1, ms);
    mMs_ = ms;
    mStart_ = now;
  }

  // Blink on whatever is on screen: the eyes squeeze shut and open again (~200 ms).
  // False (nothing happens) while something plays or when the image has no two eyes.
  bool blink(uint32_t now) {
    if (playing_) return false;
    memcpy(target_, shown_, 1024);           // the open eyes to come back to
    bStep_ = 0;
    if (!blinkStep()) return false;
    playing_ = true;
    lastFrameMs_ = now;
    return true;
  }

  // Call every loop(); draws the next frame when it is due
  void update(uint32_t now) {
    if (!playing_) return;
    if (blinking_) {
      if (now - lastFrameMs_ < BLINK_STEPS[bStep_ - 1].ms) return;
      lastFrameMs_ = now;
      if (!blinkStep() || !blinking_) playing_ = false;   // the open eyes are back
      return;
    }
    if (morphing_) {                         // the latest step that is due (late: skip, keep the end time)
      uint32_t k = (uint32_t)(now - mStart_) * (mSteps_ + 1) / mMs_;
      if (k <= mStep_) return;
      if (k <= mSteps_) {
        mStep_ = (uint8_t)k;
        morph_.render((uint16_t)(256UL * mStep_ / (mSteps_ + 1)), morphBuf_);
        screen_.showFrame(morphBuf_, false);
        memcpy(shown_, morphBuf_, 1024);
      } else {
        morphing_ = false;
        playing_ = false;
      }
      return;
    }
    const MochiAnim& a = MOCHI_ANIMS[anim_];
    if (now - lastFrameMs_ < 1000UL / a.fps) return;
    lastFrameMs_ = now;
    if (frameIdx_ + 1 < a.frameCount) {
      frameIdx_++;
    } else if (--loopsLeft_ > 0) {
      frameIdx_ = 0;
    } else {
      playing_ = false;                      // done: last frame stays on screen
      return;
    }
    showCurrent();
  }

  bool busy() const { return playing_; }
  AnimId current() const { return anim_; }
  uint16_t frameIndex() const { return frameIdx_; }

  MochiScreen& screen() { return screen_; }

private:
  void showCurrent() {
    const MochiAnim& a = MOCHI_ANIMS[anim_];
    const uint8_t* f = frame(a, frameIdx_);
    screen_.showFrame(f, a.inverted);
    for (uint16_t i = 0; i < 1024; i++) shown_[i] = a.inverted ? (uint8_t)~pgm_read_byte(f + i) : pgm_read_byte(f + i);
    morphing_ = false;
    blinking_ = false;
  }

  // Show the next blink step; false when the blink is over (or impossible)
  bool blinkStep() {
    if (bStep_ >= 4) { blinking_ = false; return false; }
    const BlinkStep& s = BLINK_STEPS[bStep_++];
    if (s.open >= 256) {
      memcpy(morphBuf_, target_, 1024);
    } else if (!morph_.blinkFrame(target_, s.open, morphBuf_)) {
      blinking_ = false;
      return false;
    }
    screen_.showFrame(morphBuf_, false);
    memcpy(shown_, morphBuf_, 1024);
    blinking_ = s.open < 256;
    return true;
  }

  // Pointer to frame `idx` of animation `a`
  const uint8_t* frame(const MochiAnim& a, uint16_t idx) {
    if (!a.packed) return a.data + (uint32_t)(a.startFrame + idx) * 1024;

    // PACKED: frames can only be decoded in order, so rewind when needed
    if (decodedAnim_ != &a || decodedIdx_ < 0 || idx < (uint16_t)decodedIdx_) {
      decodedAnim_ = &a;
      packedPos_ = a.data;
      decodedIdx_ = -1;
    }
    while (decodedIdx_ < (int16_t)idx) {
      packedPos_ = unpack(packedPos_, frameBuf_);
      decodedIdx_++;
    }
    return frameBuf_;
  }

  // Decode one PackBits frame (exactly 1024 bytes) into buf
  static const uint8_t* unpack(const uint8_t* src, uint8_t* buf) {
    uint16_t out = 0;
    while (out < 1024) {
      uint8_t c = pgm_read_byte(src++);
      if (c < 128) {                         // c + 1 literal bytes
        uint16_t n = c + 1;
        memcpy_P(buf + out, src, n);
        src += n;
        out += n;
      } else if (c > 128) {                  // next byte repeated 257 - c times
        uint16_t n = 257 - c;
        memset(buf + out, pgm_read_byte(src++), n);
        out += n;
      }
    }
    return src;
  }

  MochiScreen& screen_;
  AnimId anim_ = ANIM_BLINK;
  bool playing_ = false;
  uint8_t loopsLeft_ = 0;
  uint16_t frameIdx_ = 0;
  uint32_t lastFrameMs_ = 0;

  uint8_t frameBuf_[1024];                   // the one reused buffer for PACKED playback
  uint8_t shown_[1024];                      // exactly what is on screen (lit pixels)
  uint8_t target_[1024], morphBuf_[1024];    // morph destination / current in-between frame
  MochiMorph morph_;
  bool morphing_ = false, blinking_ = false;
  uint8_t bStep_ = 0;
  uint8_t mStep_ = 0, mSteps_ = 0;
  uint16_t mMs_ = 1;
  uint32_t mStart_ = 0;
  const MochiAnim* decodedAnim_ = nullptr;
  const uint8_t* packedPos_ = nullptr;
  int16_t decodedIdx_ = -1;
};
