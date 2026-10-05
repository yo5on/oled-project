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
// A full morph gets enough in-between frames that no step changes more than about
// MORPH_STEP_PX pixels (big face changes move gradually, small ones stay quick): up to
// MORPH_MAX_STEPS, each at least MORPH_PACED_STEP_MS apart (an in-between's real cost on the
// ESP32: render + OLED write), so none of them is skipped
static const uint16_t MORPH_STEP_PX = 350;
static const uint8_t  MORPH_MAX_STEPS = 7;
static const uint16_t MORPH_PACED_STEP_MS = 40;
// Eye-bridge transition: when the eyes change shape a lot (area x2 or height x1.7) and both
// faces have eyelids, the old eyes close, the closed face changes (lids move, mouth and the
// rest morph), then the new eyes open: eyes and mouth move at their own time, like a face,
// instead of one shape melting into another
static const uint16_t BRIDGE_CLOSE[2] = { 150, 70 };     // eyelid openness while closing ...
static const uint16_t BRIDGE_OPEN[2]  = { 70, 150 };     // ... and while the new eyes open
static const uint16_t BRIDGE_LIDS     = 70;              // nearly shut (solid lids, not 2 px lines) while the face changes
static const uint8_t  BRIDGE_MAX_MID  = 4;               // in-betweens of the closed faces

// Eyelid blink: half shut, shut, half open, open (the image from before the blink)
struct BlinkStep { uint16_t open, ms; };
static const BlinkStep BLINK_STEPS[4] = { { 100, 35 }, { 0, 90 }, { 100, 35 }, { 256, 0 } };

class MochiPlayer {
public:
  explicit MochiPlayer(MochiScreen& screen) : screen_(screen) {}

  // Start an animation now; it plays `loops` times, then stays on its last frame.
  // from/to: play only that part of it (an expression segment); to past the end = last frame.
  void play(AnimId id, uint8_t loops, uint32_t now, uint16_t from = 0, uint16_t to = 0xFFFF) {
    const MochiAnim& a = MOCHI_ANIMS[id];
    anim_ = id;
    loopsLeft_ = loops ? loops : 1;
    playing_ = true;
    segTo_ = to < a.frameCount ? to : a.frameCount - 1;
    segFrom_ = from <= segTo_ ? from : segTo_;
    frameIdx_ = segFrom_;
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
  // A full morph between very different faces gets more steps (and the time they need):
  // see MORPH_STEP_PX.
  // depth < 256: only part of the way (a breath): steps k = 1..steps show the morph at
  // depth * k / steps, the last one at now + ms, and that partial image stays on screen.
  void morphTo(AnimId id, uint16_t frame, uint8_t steps, uint16_t ms, uint32_t now, uint16_t depth = 256) {
    const MochiAnim& a = MOCHI_ANIMS[id];
    if (frame >= a.frameCount) frame = a.frameCount - 1;
    const uint8_t* src = this->frame(a, frame);
    for (uint16_t i = 0; i < 1024; i++) target_[i] = a.inverted ? (uint8_t)~pgm_read_byte(src + i) : pgm_read_byte(src + i);
    if (!closedFrame(id, frame, target_) && processed(id)) process(id, target_, morphBuf_);   // morph to the frame as it is shown
    bridge_ = depth >= 256 && eyeBridge();
    if (!bridge_) morph_.begin(shown_, target_);
    blinking_ = false;
    morphing_ = true;
    playing_ = true;
    mStep_ = 0;
    mSteps_ = morphSteps(steps ? steps : 1, ms);
    mMs_ = ms;
    if (bridge_) {                           // close, change, open: paced like a blink
      mSteps_ = (uint8_t)(2 + bMid_ + 2);
      uint16_t paced = (uint16_t)((mSteps_ + 1) * MORPH_PACED_STEP_MS);
      if (paced > mMs_) mMs_ = paced;
    } else if (depth >= 256) {               // paced by how much the face changes
      uint16_t change = 0;
      for (uint16_t i = 0; i < 1024; i++) change += popcount8((uint8_t)(shown_[i] ^ target_[i]));
      uint16_t need = change / MORPH_STEP_PX;  // in-between frames for steps of <= MORPH_STEP_PX
      if (need > MORPH_MAX_STEPS) need = MORPH_MAX_STEPS;
      if (need > mSteps_) {
        mSteps_ = (uint8_t)need;
        uint16_t paced = (uint16_t)((need + 1) * MORPH_PACED_STEP_MS);
        if (paced > mMs_) mMs_ = paced;
      }
    }
    mStart_ = now;
    mDepth_ = depth < 256 ? depth : 256;
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
      bool partial = mDepth_ < 256;
      uint32_t k = (uint32_t)(now - mStart_) * (partial ? mSteps_ : mSteps_ + 1) / mMs_;
      if (k <= mStep_) return;
      if (k <= mSteps_) {
        mStep_ = (uint8_t)k;
        if (bridge_) bridgeFrame(mStep_, morphBuf_);
        else morph_.render(partial ? (uint16_t)((uint32_t)mDepth_ * mStep_ / mSteps_) : evenStep(mStep_, mSteps_), morphBuf_);
        screen_.showFrame(morphBuf_, false);
        memcpy(shown_, morphBuf_, 1024);
        if (partial && mStep_ == mSteps_) { morphing_ = false; playing_ = false; }   // stays part of the way
      } else {
        morphing_ = false;
        playing_ = false;
      }
      return;
    }
    const MochiAnim& a = MOCHI_ANIMS[anim_];
    if (now - lastFrameMs_ < 1000UL / a.fps) return;
    lastFrameMs_ = now;
    if (frameIdx_ < segTo_) {
      frameIdx_++;
    } else if (--loopsLeft_ > 0) {
      frameIdx_ = segFrom_;
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

  // Show again exactly what the player last put on screen (after something else used
  // the screen, e.g. Gallery Mode); nothing about playback changes
  void redraw() { screen_.showFrame(shown_, false); }

  // Edge cleanup for one animation (off by default): when its frames are shown (playing,
  // still, or as a morph's start/end), lone pixels are dropped and one-pixel holes filled.
  // Only for animations whose stored 1-bit frames have noisy edges; the data is untouched.
  void setCleanup(AnimId id, bool on) {
    if (on) cleanMask_ |= 1ULL << id; else cleanMask_ &= ~(1ULL << id);
  }

  // Scanline repair for one animation (off by default; implies the edge cleanup): its
  // source video was interlaced, so solid shapes have 1-pixel dark rows through them.
  // A dark pixel with lit pixels directly above and below is lit before the cleanup.
  void setScanlineFill(AnimId id, bool on) {
    if (on) lineMask_ |= 1ULL << id; else lineMask_ &= ~(1ULL << id);
    if (on) setCleanup(id, true);
  }


  // Show frame `frame` of `id` as the eyes of frame `from` (same animation) closed to
  // `open` (MochiMorph::blinkFrame, 0 = shut .. 256 = as drawn): for a broken closed-eye
  // frame of the source video. The stored frame is not changed. Up to 4.
  void setClosedFrame(AnimId id, uint8_t frame, uint8_t from, uint16_t open) {
    if (nFixes_ < 4) fixes_[nFixes_++] = { id, frame, from, open };
  }

private:
  void showCurrent() {
    const MochiAnim& a = MOCHI_ANIMS[anim_];
    if (closedFrame(anim_, frameIdx_, shown_)) {   // a replaced closed-eye frame
      screen_.showFrame(shown_, false);
      morphing_ = false;
      blinking_ = false;
      return;
    }
    const uint8_t* f = frame(a, frameIdx_);
    if (processed(anim_)) {                  // displayed pixels, repaired, shown as they are
      for (uint16_t i = 0; i < 1024; i++) cleanBuf_[i] = a.inverted ? (uint8_t)~pgm_read_byte(f + i) : pgm_read_byte(f + i);
      process(anim_, cleanBuf_, shown_);
      memcpy(shown_, cleanBuf_, 1024);
      screen_.showFrame(shown_, false);
    } else {
      screen_.showFrame(f, a.inverted);
      for (uint16_t i = 0; i < 1024; i++) shown_[i] = a.inverted ? (uint8_t)~pgm_read_byte(f + i) : pgm_read_byte(f + i);
    }
    morphing_ = false;
    blinking_ = false;
  }

  bool cleaned(AnimId id) const { return (cleanMask_ >> id) & 1; }

  static uint8_t popcount8(uint8_t v) { uint8_t n = 0; while (v) { n += v & 1; v >>= 1; } return n; }

  // Eyes of img (lit pixels the eyelid blink removes): their area and their tallest column
  bool eyeSize(const uint8_t* img, uint8_t* closed, uint16_t& area, uint8_t& height) {
    if (!morph_.blinkFrame(img, 0, closed)) return false;
    area = 0; height = 0;
    for (uint8_t x = 0; x < 128; x++) {
      uint8_t h = 0;
      for (uint8_t y = 0; y < 64; y++) {
        uint16_t i = y * 16 + (x >> 3); uint8_t bit = 0x80 >> (x & 7);
        if ((img[i] & bit) && !(closed[i] & bit)) { h++; area++; }
      }
      if (h > height) height = h;
    }
    return area > 0;
  }

  // Use the eye bridge for shown_ -> target_? (sets bMid_, keeps the start image)
  bool eyeBridge() {
    uint16_t aA, aB; uint8_t hA, hB;
    if (!eyeSize(shown_, morphBuf_, aA, hA) || !eyeSize(target_, cleanBuf_, aB, hB)) return false;
    bool differ = aA > 2 * aB || aB > 2 * aA || hA * 10 > hB * 17 || hB * 10 > hA * 17;
    if (!differ) return false;
    uint16_t change = 0;                     // how much the closed faces differ
    for (uint16_t i = 0; i < 1024; i++) change += popcount8((uint8_t)(morphBuf_[i] ^ cleanBuf_[i]));
    bMid_ = (uint8_t)(change / MORPH_STEP_PX);
    if (bMid_ < 1) bMid_ = 1;
    if (bMid_ > BRIDGE_MAX_MID) bMid_ = BRIDGE_MAX_MID;
    memcpy(startBuf_, shown_, 1024);
    bReady_ = false;
    return true;
  }

  // Step k (1..mSteps_) of an eye-bridge transition into out
  void bridgeFrame(uint8_t k, uint8_t* out) {
    bool ok;
    if (k <= 2) {
      ok = morph_.blinkFrame(startBuf_, BRIDGE_CLOSE[k - 1], out);           // the old eyes close
    } else if (k <= 2 + bMid_) {
      if (!bReady_) {                        // the closed faces: morph between them
        morph_.blinkFrame(startBuf_, BRIDGE_LIDS, out);
        morph_.blinkFrame(target_, BRIDGE_LIDS, cleanBuf_);
        morph_.begin(out, cleanBuf_);
        bReady_ = true;
      }
      morph_.render(evenStep((uint8_t)(k - 2), bMid_), out);
      ok = true;
    } else {
      ok = morph_.blinkFrame(target_, BRIDGE_OPEN[k - 3 - bMid_], out);      // the new eyes open
    }
    if (!ok) memcpy(out, target_, 1024);
  }

  // MochiMorph eases every full morph (smoothstep): on its own, the first and the last
  // in-between frame hardly differ from the start / end image (the face seems to wait,
  // then jump). Step k of n is placed so the face is half-way between an even and an eased
  // progress: still soft at both ends, but every in-between frame moves the face.
  static uint16_t evenStep(uint8_t k, uint8_t n) {
    uint32_t lin = 256UL * k / (n + 1);
    uint32_t want = (lin + easedT((uint16_t)lin)) / 2;          // the progress to show
    uint16_t lo = 0, hi = 256;                                  // t with easedT(t) = want
    while (lo < hi) { uint16_t mid = (lo + hi) / 2; if (easedT(mid) < want) lo = mid + 1; else hi = mid; }
    return lo;
  }
  static uint16_t easedT(uint16_t t) { uint32_t x = t; return (uint16_t)((x * x * (768 - 2 * x)) >> 16); }   // = MochiMorph's smoothstep

  // Frame idx of id as replaced by setClosedFrame (displayed pixels into out); false: not
  // replaced (or the source frame has no eyes to close)
  bool closedFrame(AnimId id, uint16_t idx, uint8_t* out) {
    for (uint8_t k = 0; k < nFixes_; k++) {
      const ClosedFix& c = fixes_[k];
      if (c.anim != id || c.frame != idx) continue;
      const MochiAnim& a = MOCHI_ANIMS[id];
      const uint8_t* f = frame(a, c.from);
      for (uint16_t i = 0; i < 1024; i++) cleanBuf_[i] = a.inverted ? (uint8_t)~pgm_read_byte(f + i) : pgm_read_byte(f + i);
      if (processed(id)) process(id, cleanBuf_, out);
      return morph_.blinkFrame(cleanBuf_, c.open, out);
    }
    return false;
  }

  bool processed(AnimId id) const { return ((cleanMask_ | lineMask_) >> id) & 1; }

  // The display repairs of one animation, in place on displayed pixels (tmp: scratch)
  void process(AnimId id, uint8_t* io, uint8_t* tmp) const {
    if ((lineMask_ >> id) & 1) fillScanlines(io);
    if ((cleanMask_ >> id) & 1) { despeckle(io, tmp); memcpy(io, tmp, 1024); }
  }

  static bool px(const uint8_t* b, int16_t x, int16_t y) {
    return x >= 0 && y >= 0 && x < 128 && y < 64 && (b[y * 16 + (x >> 3)] & (0x80 >> (x & 7)));
  }

  // Close 1-pixel horizontal gaps: dark pixels with lit pixels directly above and below
  // (decided on the original rows, so a gap row never closes the next one)
  static void fillScanlines(uint8_t* b) {
    uint8_t prev[16], cur[16];
    memcpy(prev, b, 16);
    for (int16_t y = 1; y < 63; y++) {
      memcpy(cur, b + y * 16, 16);
      for (uint8_t i = 0; i < 16; i++) b[y * 16 + i] = (uint8_t)(cur[i] | (prev[i] & b[(y + 1) * 16 + i]));
      memcpy(prev, cur, 16);
    }
  }

  // Lone lit pixels (at most one lit neighbour of eight, so thin diagonal lines stay) go
  // dark; dark pixels with at least three lit side neighbours (one-pixel holes) light up
  static void despeckle(const uint8_t* src, uint8_t* dst) {
    for (int16_t y = 0; y < 64; y++) for (int16_t x = 0; x < 128; x++) {
      uint8_t n4 = px(src, x - 1, y) + px(src, x + 1, y) + px(src, x, y - 1) + px(src, x, y + 1);
      bool on = px(src, x, y);
      if (on) on = n4 + px(src, x - 1, y - 1) + px(src, x + 1, y - 1) + px(src, x - 1, y + 1) + px(src, x + 1, y + 1) > 1;
      else on = n4 >= 3;
      uint8_t bit = 0x80 >> (x & 7);
      if (on) dst[y * 16 + (x >> 3)] |= bit; else dst[y * 16 + (x >> 3)] &= (uint8_t)~bit;
    }
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
  uint16_t segFrom_ = 0, segTo_ = 0;        // part of the animation being played
  uint32_t lastFrameMs_ = 0;

  uint8_t frameBuf_[1024];                   // the one reused buffer for PACKED playback
  uint8_t shown_[1024];                      // exactly what is on screen (lit pixels)
  uint8_t target_[1024], morphBuf_[1024];    // morph destination / current in-between frame
  uint8_t cleanBuf_[1024];                   // a frame of a cleaned animation before cleanup
  uint8_t startBuf_[1024];                   // eye bridge: the face it started from
  bool bridge_ = false, bReady_ = false;     // eye bridge running / its closed-face morph prepared
  uint8_t bMid_ = 1;                         // eye bridge: in-betweens of the closed faces
  uint64_t cleanMask_ = 0;                   // animations shown with edge cleanup (bit = AnimId)
  uint64_t lineMask_ = 0;                    // ... and with scanline repair first
  struct ClosedFix { AnimId anim; uint8_t frame, from; uint16_t open; };
  ClosedFix fixes_[4];                       // replaced closed-eye frames (setClosedFrame)
  uint8_t nFixes_ = 0;
  MochiMorph morph_;
  bool morphing_ = false, blinking_ = false;
  uint8_t bStep_ = 0;
  uint8_t mStep_ = 0, mSteps_ = 0;
  uint16_t mMs_ = 1;
  uint16_t mDepth_ = 256;                    // 256: a full morph; less: a partial one (breathing)
  uint32_t mStart_ = 0;
  const MochiAnim* decodedAnim_ = nullptr;
  const uint8_t* packedPos_ = nullptr;
  int16_t decodedIdx_ = -1;
};
