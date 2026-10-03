// =====================================================
// MOCHI PLAYER — plays one MochiAnim at a time, non-blocking
//
// The behavior engine only calls play(), hold(), busy() and update().
// Frame data stays in flash: RAW frames are drawn straight from flash,
// PACKED frames are decoded one at a time into a single 1024-byte buffer.
// The screen itself is behind MochiScreen (OLED on the ESP32).
// =====================================================

#pragma once

#include <stdint.h>
#include <string.h>
#include "MochiAnimations.h"

// What the player needs from a display
class MochiScreen {
public:
  virtual void showFrame(const uint8_t* frame, bool inverted) = 0;   // 128x64, 1024 bytes
  virtual void setDim(bool dim) = 0;
  virtual void setPower(bool on) = 0;
};

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

  // Call every loop(); draws the next frame when it is due
  void update(uint32_t now) {
    if (!playing_) return;
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
    screen_.showFrame(frame(a, frameIdx_), a.inverted);
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
      packedPos_ = unpackFrame(packedPos_);
      decodedIdx_++;
    }
    return frameBuf_;
  }

  // Decode one PackBits frame (exactly 1024 bytes) into frameBuf_
  const uint8_t* unpackFrame(const uint8_t* src) {
    uint16_t out = 0;
    while (out < sizeof(frameBuf_)) {
      uint8_t c = pgm_read_byte(src++);
      if (c < 128) {                         // c + 1 literal bytes
        uint16_t n = c + 1;
        memcpy_P(frameBuf_ + out, src, n);
        src += n;
        out += n;
      } else if (c > 128) {                  // next byte repeated 257 - c times
        uint16_t n = 257 - c;
        memset(frameBuf_ + out, pgm_read_byte(src++), n);
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

  uint8_t frameBuf_[1024];                   // the one reused buffer for PACKED frames
  const MochiAnim* decodedAnim_ = nullptr;
  const uint8_t* packedPos_ = nullptr;
  int16_t decodedIdx_ = -1;
};
