// =====================================================
// MOCHI GALLERY — the V3 photo / video viewer as a separate mode
//
// Media are the V3 gallery's own data, unchanged: 128x64 1-bit frames of 1024 bytes
// (16 bytes per row, MSB = left pixel, 1 = lit), stored in flash. A photo is one frame
// and stays until navigation; a video plays its frames at its own frame time and starts
// again at the end (as in V3) until another item is chosen. Nothing here touches Mochi's
// animations or player: the gallery draws through the same MochiScreen only while it is
// active, and the sketch hands the screen back to Mochi when it is left.
//
// The media list comes from gallery_media_private.h (generated locally from V3 by
// tools/make_gallery.py, never committed) or, when that file does not exist, from
// gallery_placeholder.h (one non-private "no media" picture).
// =====================================================

#pragma once

#include <stdint.h>
#include "MochiPlayer.h"                     // MochiScreen

struct GalleryItem {
  const uint8_t* const* frames;              // frameCount frames of 1024 bytes
  uint16_t frameCount;                       // 1: a photo
  uint16_t frameMs;                          // video: time per frame (0: a photo)
  const char* name;
};

#if __has_include("gallery_media_private.h")
#include "gallery_media_private.h"           // local only: the real photos / videos
#else
#include "gallery_placeholder.h"             // public build: no private media
#endif

class MochiGallery {
public:
  static const uint32_t NAV_GAP_MS = 200;    // V3: shortest time between two navigations

  explicit MochiGallery(MochiScreen& screen) : screen_(screen) {}

  static uint16_t count() { return GALLERY_ITEM_COUNT; }
  bool active() const { return active_; }
  uint16_t index() const { return index_; }
  uint16_t frame() const { return frame_; }
  const GalleryItem& item() const { return GALLERY_ITEMS[index_]; }

  // Enter: show the item looked at last time (the first one at first), from its start
  void enter(uint32_t now) {
    active_ = true;
    lastNav_ = now - NAV_GAP_MS;
    show(now);
  }

  void exit() { active_ = false; }

  // Button 1 / button 2 in the gallery (V3: PREV / NEXT, wrapping around)
  bool previous(uint32_t now) { return step(-1, now); }
  bool next(uint32_t now) { return step(1, now); }

  // Call every loop() while active: the next video frame when it is due
  void update(uint32_t now) {
    if (!active_) return;
    const GalleryItem& it = item();
    if (it.frameCount < 2 || it.frameMs == 0) return;          // a photo stays as it is
    if (now - lastFrameMs_ < it.frameMs) return;
    lastFrameMs_ = now;
    frame_ = (uint16_t)((frame_ + 1) % it.frameCount);          // the end: from the start again
    screen_.showFrame(it.frames[frame_], false);
  }

private:
  bool step(int8_t dir, uint32_t now) {
    if (!active_ || now - lastNav_ < NAV_GAP_MS) return false;
    lastNav_ = now;
    index_ = (uint16_t)((index_ + count() + dir) % count());
    show(now);
    return true;
  }

  void show(uint32_t now) {
    frame_ = 0;
    lastFrameMs_ = now;
    screen_.showFrame(item().frames[0], false);
  }

  MochiScreen& screen_;
  bool active_ = false;
  uint16_t index_ = 0, frame_ = 0;
  uint32_t lastFrameMs_ = 0, lastNav_ = 0;
};
