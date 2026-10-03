// =====================================================
// MOCHI MORPH — universal expression morph between any two 128x64 frames
//
// Works on what is on screen (1 = lit pixel, 16 bytes per row, MSB = left), so it
// handles every pair of the 35 animations, resting faces and even a half-finished
// morph (button pressed mid-transition). Nothing is precomputed ahead of time:
// begin() analyses the two images once, render() only cuts the blended fields.
//
// 1. Each image is split into face parts: left eye, right eye, mouth, and small
//    special elements (anger mark, sparkles, "?", dots). Large extra shapes
//    (slashes, tears) belong to the nearest eye/mouth.
// 2. Matching parts morph their OUTLINE: both shapes are turned into signed
//    distance fields (chamfer 3-4, integer), centred on their centroids, blended,
//    and cut at the level that gives an area between the two areas (so thin strokes
//    never vanish). The part glides from the old to the new position meanwhile.
// 3. Parts that exist on one side only shrink away (early) or grow in (late);
//    special elements leave first and arrive last, after the face has changed.
// No pixel fades: every intermediate frame is a set of solid shapes.
// =====================================================

#pragma once

#include <stdint.h>
#include <string.h>

class MochiMorph {
public:
  static const int16_t W = 128, H = 64, N = W * H;
  enum Part : uint8_t { P_NONE = 0, P_LEFT, P_RIGHT, P_MOUTH, P_SPECIAL, P_COUNT };

  // Prepare a morph from image a to image b (displayed pixels, 1024 bytes each)
  void begin(const uint8_t* a, const uint8_t* b) {
    faceA_ = segment(a, partA_, infoA_);
    faceB_ = segment(b, partB_, infoB_);
    prepare();
  }

  // Blink on any face: the eyes of img squeezed toward their centre line, column by
  // column (open = 256: as drawn .. 0: shut, a 2 px line); everything else unchanged.
  // False when img has no two open, solid eyes (slits, brows, tears, full-screen art:
  // no blink there). (Uses the morph buffers: not during a morph.)
  bool blinkFrame(const uint8_t* img, uint16_t open, uint8_t* out) {
    if (!segment(img, partA_, infoA_)) return false;
    if (infoA_[P_MOUTH].area > infoA_[P_LEFT].area + infoA_[P_RIGHT].area) return false;   // a big grin, not eyes
    if (infoA_[P_RIGHT].x0 - infoA_[P_LEFT].x1 < 10) return false;                        // eyes run together
    for (uint8_t p = P_LEFT; p <= P_RIGHT; p++) {
      const PartInfo& q = infoA_[p];
      int32_t w = q.x1 - q.x0 + 1, h = q.y1 - q.y0 + 1;
      if (h < 8 || w < 5 || q.area > 1600 || (int32_t)q.area * 100 < 55 * w * h) return false;
      if (q.x0 == 0 || q.y0 == 0 || q.x1 == W - 1 || q.y1 == H - 1) return false;
    }
    memcpy(out, img, 1024);
    for (uint8_t p = P_LEFT; p <= P_RIGHT; p++) {
      const PartInfo& q = infoA_[p];
      for (int16_t x = q.x0; x <= q.x1; x++) {
        int16_t top = -1, bot = -1;
        for (int16_t y = q.y0; y <= q.y1; y++) {
          if (partA_[y * W + x] != p) continue;
          out[(y * W + x) >> 3] &= (uint8_t)~(0x80 >> (x & 7));
          if (top < 0) top = y;
          bot = y;
        }
        if (top < 0) continue;
        int16_t t2 = (int16_t)(q.cy + (int32_t)(top - q.cy) * open / 256);
        int16_t b2 = (int16_t)(q.cy + (int32_t)(bot - q.cy) * open / 256);
        if (b2 <= t2) { t2 = t2 < q.cy ? t2 : q.cy; b2 = t2 + 1; }
        for (int16_t y = t2; y <= b2; y++)
          if (y >= 0 && y < H) out[(y * W + x) >> 3] |= 0x80 >> (x & 7);
      }
    }
    return true;
  }

  // Did the last begin() find a face (two eyes) in the source / destination image?
  bool sourceHasFace() const { return faceA_; }
  bool destHasFace() const { return faceB_; }

  // Intermediate image at t = 0..256 (0 = a, 256 = b) into out (1024 bytes)
  void render(uint16_t t, uint8_t* out) {
    memset(out, 0, 1024);
    uint16_t te = smooth(t);
    if (!faceA_ || !faceB_) {                            // no face structure on one side:
      for (uint8_t p = P_LEFT; p < P_COUNT; p++) {       // old shapes shrink away, new ones grow in
        if (infoA_[p].area) grow(fieldA(p), infoA_[p], (uint32_t)infoA_[p].area * clampT(256 - t * 8 / 5) / 256, out);
        if (infoB_[p].area) grow(fieldB(p), infoB_[p], (uint32_t)infoB_[p].area * clampT((t - 102) * 5 / 3) / 256, out);
      }
      return;
    }
    for (uint8_t p = P_LEFT; p < P_COUNT; p++) {
      const PartInfo& A = infoA_[p];
      const PartInfo& B = infoB_[p];
      if (!A.area && !B.area) continue;
      if (p == P_SPECIAL) {                              // special elements: out early, in late
        if (A.area) grow(fieldA(p), A, (uint32_t)A.area * clampT(256 - t * 2) / 256, out);
        if (B.area) grow(fieldB(p), B, (uint32_t)B.area * clampT(t * 2 - 256) / 256, out);
      } else if (A.area && B.area) {
        blend(p, A, B, te, out);
      } else if (A.area) {                               // part only on the old face: shrink away
        grow(fieldA(p), A, (uint32_t)A.area * clampT(256 - t * 8 / 5) / 256, out);
      } else {                                           // part only on the new face: grow in
        grow(fieldB(p), B, (uint32_t)B.area * clampT((t - 102) * 5 / 3) / 256, out);
      }
    }
  }

private:
  struct PartInfo {
    uint16_t area;
    int16_t cx, cy;                                    // centroid (pixels)
    int16_t x0, y0, x1, y1;                            // bounding box
  };

  static uint16_t clampT(int32_t v) { return v < 0 ? 0 : v > 256 ? 256 : (uint16_t)v; }
  static uint16_t smooth(uint16_t t) {                 // smoothstep in 0..256
    uint32_t x = t;
    return (uint16_t)((x * x * (768 - 2 * x)) >> 16);
  }
  static bool lit(const uint8_t* img, int16_t i) { return img[i >> 3] & (0x80 >> (i & 7)); }

  // ---------------- segmentation into face parts ----------------
  // Returns true when a face (two eyes side by side) was found
  bool segment(const uint8_t* img, uint8_t* part, PartInfo* info) {
    // connected components (8-neighbour) with an explicit stack (tmp_ is free here)
    int16_t* stack_ = tmp_;
    memset(label_, 0, sizeof(label_));
    uint8_t n = 0;
    for (int16_t i = 0; i < N; i++) {
      if (!lit(img, i) || label_[i]) continue;
      if (n == MAX_COMP) { label_[i] = 0xFF; continue; }     // too many tiny pieces: special
      n++;
      Comp& c = comp_[n];
      c.area = 0; c.sx = 0; c.sy = 0; c.x0 = W; c.y0 = H; c.x1 = -1; c.y1 = -1;
      int16_t sp = 0;
      stack_[sp++] = i;
      label_[i] = n;
      while (sp) {
        int16_t k = stack_[--sp];
        int16_t x = k % W, y = k / W;
        c.area++; c.sx += x; c.sy += y;
        if (x < c.x0) c.x0 = x;
        if (x > c.x1) c.x1 = x;
        if (y < c.y0) c.y0 = y;
        if (y > c.y1) c.y1 = y;
        for (int8_t dy = -1; dy <= 1; dy++) for (int8_t dx = -1; dx <= 1; dx++) {
          int16_t xx = x + dx, yy = y + dy;
          if (xx < 0 || yy < 0 || xx >= W || yy >= H) continue;
          int16_t kk = yy * W + xx;
          if (!label_[kk] && lit(img, kk)) { label_[kk] = n; stack_[sp++] = kk; }
        }
      }
    }
    // role of each component
    uint8_t L = 0, R = 0;
    pickEyes(n, L, R);
    int16_t eyeY = 0, midX = W / 2, mouthMinY = H;
    uint8_t M = 0;
    if (L) {
      eyeY = (cy(L) + cy(R)) / 2;
      midX = (cx(L) + cx(R)) / 2;
      uint32_t best = 0;
      for (uint8_t i = 1; i <= n; i++) {                  // mouth: largest piece below, between the eyes
        if (i == L || i == R) continue;
        if (cy(i) > eyeY - 2 && cx(i) > cx(L) - 5 && cx(i) < cx(R) + 5 && comp_[i].area > best) { best = comp_[i].area; M = i; }
      }
      if (M) mouthMinY = (eyeY + cy(M)) / 2;
    } else {                                               // no two eyes: split all ink at its centre
      uint32_t sx = 0, a = 0;
      for (uint8_t i = 1; i <= n; i++) { sx += comp_[i].sx; a += comp_[i].area; }
      if (a) midX = (int16_t)(sx / a);
    }
    for (uint8_t i = 1; i <= n; i++) {
      Comp& c = comp_[i];
      if (i == L) c.role = P_LEFT;
      else if (i == R) c.role = P_RIGHT;
      else if (i == M) c.role = P_MOUTH;
      else if (L && c.area < SPECIAL_MAX_AREA) c.role = P_SPECIAL;        // marks, sparkles, dots
      else if (M && cy(i) >= mouthMinY && cx(i) > cx(L) && cx(i) < cx(R)) c.role = P_MOUTH;
      else c.role = cx(i) < midX ? P_LEFT : P_RIGHT;                       // big extras join an eye
    }
    // part map + part statistics
    for (uint8_t p = 0; p < P_COUNT; p++) {
      PartInfo& q = info[p];
      q.area = 0; q.x0 = W; q.y0 = H; q.x1 = -1; q.y1 = -1;
      sumX_[p] = 0; sumY_[p] = 0;
    }
    for (int16_t i = 0; i < N; i++) {
      uint8_t l = label_[i];
      uint8_t p = l == 0 ? P_NONE : l == 0xFF ? P_SPECIAL : comp_[l].role;
      part[i] = p;
      if (p == P_NONE) continue;
      PartInfo& q = info[p];
      int16_t x = i % W, y = i / W;
      q.area++; sumX_[p] += x; sumY_[p] += y;
      if (x < q.x0) q.x0 = x;
      if (x > q.x1) q.x1 = x;
      if (y < q.y0) q.y0 = y;
      if (y > q.y1) q.y1 = y;
    }
    for (uint8_t p = 0; p < P_COUNT; p++)
      if (info[p].area) { info[p].cx = (int16_t)(sumX_[p] / info[p].area); info[p].cy = (int16_t)(sumY_[p] / info[p].area); }
    return L != 0;
  }

  // the two largest pieces side by side are the eyes
  void pickEyes(uint8_t n, uint8_t& L, uint8_t& R) {
    uint8_t top[5] = { 0 };
    for (uint8_t i = 1; i <= n; i++) {
      for (uint8_t k = 0; k < 5; k++) {
        if (!top[k] || comp_[i].area > comp_[top[k]].area) {
          for (uint8_t m = 4; m > k; m--) top[m] = top[m - 1];
          top[k] = i;
          break;
        }
      }
    }
    for (uint8_t a = 0; a < 4; a++) for (uint8_t b = a + 1; b < 5; b++) {
      uint8_t i = top[a], j = top[b];
      if (!i || !j) continue;
      uint32_t ai = comp_[i].area, aj = comp_[j].area;
      int16_t dy = cy(i) - cy(j), dx = cx(i) - cx(j);
      if (dy < 0) dy = -dy;
      if (dx < 0) dx = -dx;
      if (dy <= 12 && dx >= 18 && 4 * (ai < aj ? ai : aj) >= (ai > aj ? ai : aj)) {
        L = cx(i) < cx(j) ? i : j;
        R = L == i ? j : i;
        return;
      }
    }
  }

  int16_t cx(uint8_t i) const { return (int16_t)(comp_[i].sx / comp_[i].area); }
  int16_t cy(uint8_t i) const { return (int16_t)(comp_[i].sy / comp_[i].area); }

  // ---------------- signed distance field (chamfer 3-4) ----------------
  // negative inside the part, positive outside; units of 1/3 pixel.
  // Computed once per morph, only inside the window the render will sample. The
  // window holds the whole part plus a margin, and chamfer paths stay inside the box
  // of their two end points, so the values equal a full-screen field.
  struct Field {
    int16_t* d;                                        // w*h values
    int16_t x0, y0, w, h;
  };

  void sdf(const uint8_t* part, uint8_t p, Field& f) {
    const int16_t BIG = 2000;
    int16_t W2 = f.w, H2 = f.h;
    int32_t n = (int32_t)W2 * H2;
    for (int pass = 0; pass < 2; pass++) {           // pass 0: distance to the part, 1: to outside
      int16_t* d = pass == 0 ? f.d : tmp_;
      for (int16_t y = 0; y < H2; y++) for (int16_t x = 0; x < W2; x++)
        d[y * W2 + x] = ((part[(f.y0 + y) * W + f.x0 + x] == p) == (pass == 0)) ? 0 : BIG;
      for (int16_t y = 0; y < H2; y++) for (int16_t x = 0; x < W2; x++) {
        int16_t i = y * W2 + x, v = d[i];
        if (x > 0 && d[i - 1] + 3 < v) v = d[i - 1] + 3;
        if (y > 0) {
          if (d[i - W2] + 3 < v) v = d[i - W2] + 3;
          if (x > 0 && d[i - W2 - 1] + 4 < v) v = d[i - W2 - 1] + 4;
          if (x < W2 - 1 && d[i - W2 + 1] + 4 < v) v = d[i - W2 + 1] + 4;
        }
        d[i] = v;
      }
      for (int16_t y = H2 - 1; y >= 0; y--) for (int16_t x = W2 - 1; x >= 0; x--) {
        int16_t i = y * W2 + x, v = d[i];
        if (x < W2 - 1 && d[i + 1] + 3 < v) v = d[i + 1] + 3;
        if (y < H2 - 1) {
          if (d[i + W2] + 3 < v) v = d[i + W2] + 3;
          if (x < W2 - 1 && d[i + W2 + 1] + 4 < v) v = d[i + W2 + 1] + 4;
          if (x > 0 && d[i + W2 - 1] + 4 < v) v = d[i + W2 - 1] + 4;
        }
        d[i] = v;
      }
    }
    for (int32_t i = 0; i < n; i++) f.d[i] = f.d[i] - tmp_[i];   // outside distance - inside distance
  }

  static int16_t sample(const Field& f, int16_t x, int16_t y) {
    if (x < 0 || y < 0 || x >= W || y >= H) return 1000;
    x -= f.x0; y -= f.y0;
    if (x < 0 || y < 0 || x >= f.w || y >= f.h) return 1000;   // never sampled by construction
    return f.d[y * f.w + x];
  }

  // Window: box x0..x1, y0..y1 grown by m, clipped to the screen
  static void window(Field& f, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m) {
    x0 -= m; y0 -= m; x1 += m; y1 += m;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > W - 1) x1 = W - 1;
    if (y1 > H - 1) y1 = H - 1;
    f.x0 = x0; f.y0 = y0; f.w = x1 - x0 + 1; f.h = y1 - y0 + 1;
  }

  // Decide every part's window and compute its field once for the whole morph.
  // All fields share one pool; if they do not fit (very large shapes), each side gets
  // half of it and a side that does not fit its half is computed at each render instead.
  void prepare() {
    bool face = faceA_ && faceB_;
    for (uint8_t p = P_LEFT; p < P_COUNT; p++) {
      const PartInfo& A = infoA_[p];
      const PartInfo& B = infoB_[p];
      blendPart_[p] = face && p != P_SPECIAL && A.area && B.area;
      if (blendPart_[p]) {                           // blend samples A's box and B's box seen from A (and back)
        int16_t sx = A.cx - B.cx, sy = A.cy - B.cy;
        window(fieldA_[p], min16(A.x0, B.x0 + sx), min16(A.y0, B.y0 + sy), max16(A.x1, B.x1 + sx), max16(A.y1, B.y1 + sy), 3);
        window(fieldB_[p], min16(B.x0, A.x0 - sx), min16(B.y0, A.y0 - sy), max16(B.x1, A.x1 - sx), max16(B.y1, A.y1 - sy), 3);
      } else {                                       // grow/shrink samples the part's own box
        if (A.area) window(fieldA_[p], A.x0, A.y0, A.x1, A.y1, 1);
        if (B.area) window(fieldB_[p], B.x0, B.y0, B.x1, B.y1, 1);
      }
    }
    int32_t used = 0;
    cachedA_ = place(infoA_, fieldA_, pool_, used, 2 * N);
    cachedB_ = cachedA_ && place(infoB_, fieldB_, pool_, used, 2 * N);
    if (!cachedB_) {
      used = 0;
      cachedA_ = place(infoA_, fieldA_, pool_, used, N);
      used = N;
      cachedB_ = place(infoB_, fieldB_, pool_, used, 2 * N);
    }
    for (uint8_t p = P_LEFT; p < P_COUNT; p++) {
      if (cachedA_ && infoA_[p].area) sdf(partA_, p, fieldA_[p]);
      if (cachedB_ && infoB_[p].area) sdf(partB_, p, fieldB_[p]);
    }
  }

  // Give each part of one side its place in pool[used..end); false (and every field of
  // the side at pool + used, as scratch) when they do not fit
  static bool place(const PartInfo* info, Field* f, int16_t* pool, int32_t& used, int32_t end) {
    int32_t start = used;
    for (uint8_t p = P_LEFT; p < P_COUNT; p++) {
      if (!info[p].area) continue;
      int32_t n = (int32_t)f[p].w * f[p].h;
      if (used + n > end) {
        for (uint8_t q = P_LEFT; q < P_COUNT; q++) f[q].d = pool + start;
        used = start;
        return false;
      }
      f[p].d = pool + used;
      used += n;
    }
    return true;
  }

  // Field of part p of image a / b (computed now when that side is not cached)
  const Field& fieldA(uint8_t p) { if (!cachedA_) sdf(partA_, p, fieldA_[p]); return fieldA_[p]; }
  const Field& fieldB(uint8_t p) { if (!cachedB_) sdf(partB_, p, fieldB_[p]); return fieldB_[p]; }

  // Draw the pixels of `region` whose field value is among the `target` lowest
  void cut(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t target, uint8_t* out,
           int16_t (*field)(MochiMorph*, int16_t, int16_t), int16_t lo, int16_t hi) {
    if (target == 0) return;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > W - 1) x1 = W - 1;
    if (y1 > H - 1) y1 = H - 1;
    memset(hist_, 0, sizeof(hist_));
    for (int16_t y = y0; y <= y1; y++) for (int16_t x = x0; x <= x1; x++) {
      int16_t v = field(this, x, y);
      if (v >= lo && v <= hi) hist_[v - lo]++;
    }
    int16_t tau = lo - 1;
    uint32_t c = 0;
    for (int16_t v = lo; v <= hi; v++) { c += hist_[v - lo]; tau = v; if (c >= target) break; }
    for (int16_t y = y0; y <= y1; y++) for (int16_t x = x0; x <= x1; x++)
      if (field(this, x, y) <= tau) out[(y * W + x) >> 3] |= 0x80 >> (x & 7);
  }

  // matching parts: outline blend + glide
  void blend(uint8_t p, const PartInfo& A, const PartInfo& B, uint16_t t, uint8_t* out) {
    fa_ = &fieldA(p);
    fb_ = &fieldB(p);
    t_ = t;
    int16_t cxT = A.cx + (int32_t)(B.cx - A.cx) * t / 256, cyT = A.cy + (int32_t)(B.cy - A.cy) * t / 256;
    dax_ = A.cx - cxT; day_ = A.cy - cyT;          // output pixel -> A coordinates
    dbx_ = B.cx - cxT; dby_ = B.cy - cyT;
    uint32_t target = ((uint32_t)A.area * (256 - t) + (uint32_t)B.area * t) / 256;
    int16_t x0 = min16(A.x0 - dax_, B.x0 - dbx_) - 3, x1 = max16(A.x1 - dax_, B.x1 - dbx_) + 3;
    int16_t y0 = min16(A.y0 - day_, B.y0 - dby_) - 3, y1 = max16(A.y1 - day_, B.y1 - dby_) + 3;
    cut(x0, y0, x1, y1, target, out, &blendField, -HIST_HALF, HIST_HALF - 1);
  }

  static int16_t blendField(MochiMorph* m, int16_t x, int16_t y) {
    int32_t a = sample(*m->fa_, x + m->dax_, y + m->day_);
    int32_t b = sample(*m->fb_, x + m->dbx_, y + m->dby_);
    return (int16_t)((a * (256 - m->t_) + b * m->t_) >> 8);
  }

  // one-sided part: keep the `target` innermost pixels (shrinks/grows from the inside out)
  void grow(const Field& f, const PartInfo& Q, uint32_t target, uint8_t* out) {
    if (!target) return;
    fa_ = &f;
    cut(Q.x0, Q.y0, Q.x1, Q.y1, target, out, &oneField, -HIST_HALF, 0);
  }

  static int16_t oneField(MochiMorph* m, int16_t x, int16_t y) { return sample(*m->fa_, x, y); }

  static int16_t min16(int16_t a, int16_t b) { return a < b ? a : b; }
  static int16_t max16(int16_t a, int16_t b) { return a > b ? a : b; }

  static const uint8_t MAX_COMP = 60;
  static const uint16_t SPECIAL_MAX_AREA = 40;         // smaller extra pieces are "special elements"
  static const int16_t HIST_HALF = 512;

  struct Comp {
    uint32_t area, sx, sy;
    int16_t x0, y0, x1, y1;
    uint8_t role;
  };

  Comp comp_[MAX_COMP + 1];
  uint8_t label_[N];
  uint8_t partA_[N], partB_[N];
  PartInfo infoA_[P_COUNT], infoB_[P_COUNT];
  bool faceA_ = false, faceB_ = false;
  uint32_t sumX_[P_COUNT], sumY_[P_COUNT];
  int16_t pool_[2 * N], tmp_[N];                       // distance fields, scratch
  Field fieldA_[P_COUNT], fieldB_[P_COUNT];
  bool blendPart_[P_COUNT];
  bool cachedA_ = false, cachedB_ = false;
  const Field* fa_ = nullptr;
  const Field* fb_ = nullptr;
  uint16_t hist_[2 * HIST_HALF];
  uint16_t t_ = 0;
  int16_t dax_ = 0, day_ = 0, dbx_ = 0, dby_ = 0;
};
