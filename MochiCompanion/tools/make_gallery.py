"""Build the PRIVATE gallery header for Gallery Mode from your local media sources.

    python tools/make_gallery.py [--skip NAME ...] [--order NAME,NAME,...] SOURCE [SOURCE ...]

SOURCE can be (none of them is ever modified):
  * the V3 media viewer sketch (.ino): its contents[] table gives the order of the media
    (VIDEO / IMAGE slots; EYES slots are skipped) and videoFrameMs[] the video speeds
  * an "OLED Display Animation Maker" export (.txt / .ino): frame0, frame1, ... arrays of
    1024 bytes; one frame = a photo, several = a video at the export's frame time
  * a raw bitmap of another size, written as FILE@WxH (e.g. spiderman.txt@256x256): only
    its 0x.. bytes, W x H, 1 bit per pixel, rows MSB first; scaled down to fit 128x64
    and centred (the only media that are converted; everything else is copied as is)
--skip NAME leaves an item out (names from the sources: V3 slot names such as video2,
or the file name of an export, e.g. beard).
--order NAME,... the order of the gallery (every remaining item, once); default: as given.
In the gallery the items are then numbered in that order: video1, video2, ... and image1,
image2, ... (photos).

Writes gallery_media_private.h next to MochiCompanion.ino: LOCAL ONLY. It is listed in
MochiCompanion/.gitignore and must never be committed; without it the sketch builds with
gallery_placeholder.h (no private media). Needs only the Python standard library.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), 'gallery_media_private.h')
DEFAULT_MS = 67


def strip_comments(s):
    return re.sub(r'//[^\n]*', '', re.sub(r'/\*.*?\*/', '', s, flags=re.S))


def frame_arrays(src):
    out = {}
    for m in re.finditer(r'const\s+uint8_t\s+PROGMEM\s+(\w+)\s*\[\s*1024\s*\]\s*=\s*\{(.*?)\};', src, re.S):
        data = [int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', m.group(2))]
        if len(data) != 1024:
            sys.exit(f'{m.group(1)}: {len(data)} bytes, expected 1024')
        out[m.group(1)] = data
    return out


def from_v3(path, src):
    arrays = frame_arrays(src)
    lists = {m.group(1): re.findall(r'\w+', m.group(2))
             for m in re.finditer(r'const\s+uint8_t\s*\*\s*(\w+)\s*\[\s*\]\s*=\s*\{(.*?)\};', src, re.S)}
    table = re.search(r'Content\s+contents\s*\[\s*\]\s*=\s*\{(.*?)\};', src, re.S)
    times = re.search(r'videoFrameMs\s*\[[^\]]*\]\s*=\s*\{(.*?)\};', src, re.S)
    slots = re.findall(r'\{\s*(VIDEO|IMAGE|EYES)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\}', table.group(1))
    frame_ms = [int(x) for x in re.findall(r'\b(\d+)\b', strip_comments(times.group(1)))]
    items = []
    for slot, (kind, frames, _count, image) in enumerate(slots):
        if kind == 'VIDEO' and frames in lists:
            items.append((frames.replace('Frames', ''), [arrays[a] for a in lists[frames]], frame_ms[slot]))
        elif kind == 'IMAGE' and image in arrays:
            items.append((image, [arrays[image]], 0))
    return items


def from_export(path, src):
    arrays = frame_arrays(src)
    names = sorted((n for n in arrays if re.fullmatch(r'frame\d+', n)), key=lambda n: int(n[5:]))
    if not names:
        sys.exit(f'{path}: no frameN[1024] arrays')
    m = re.search(r'millis\(\)\s*-\s*\w+\s*>=\s*(\d+)', src)
    ms = int(m.group(1)) if m else DEFAULT_MS
    name = re.sub(r'\W', '_', os.path.splitext(os.path.basename(path))[0])
    frames = [arrays[n] for n in names]
    return [(name, frames, ms if len(frames) > 1 else 0)]


def from_raw(path, src, w, h):
    data = [int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', strip_comments(src))]
    if len(data) * 8 != w * h:
        sys.exit(f'{path}: {len(data)} bytes, but {w}x{h} needs {w * h // 8}')
    bits = [[(data[y * (w // 8) + x // 8] >> (7 - x % 8)) & 1 for x in range(w)] for y in range(h)]
    s = max((w + 127) // 128, (h + 63) // 64)          # whole-number shrink so it fits 128x64
    nw, nh = w // s, h // s
    frame = [0] * 1024
    ox, oy = (128 - nw) // 2, (64 - nh) // 2
    for y in range(nh):
        for x in range(nw):
            lit = sum(bits[y * s + j][x * s + i] for j in range(s) for i in range(s))
            if lit * 100 >= 35 * s * s:                 # keeps thin strokes
                px, py = ox + x, oy + y
                frame[py * 16 + px // 8] |= 0x80 >> (px % 8)
    name = re.sub(r'\W', '_', os.path.splitext(os.path.basename(path))[0])
    return [(name, [frame], 0)]


def main():
    args = sys.argv[1:]
    skip, order = set(), None
    while args and args[0] in ('--skip', '--order'):
        if args[0] == '--skip': skip.add(args[1])
        else: order = [n.strip() for n in args[1].split(',') if n.strip()]
        args = args[2:]
    if not args:
        sys.exit(__doc__)
    items = []
    for a in args:
        size = re.fullmatch(r'(.*)@(\d+)x(\d+)', a)
        path = size.group(1) if size else a
        src = open(path, encoding='utf-8', errors='replace').read()
        if size:
            items += from_raw(path, src, int(size.group(2)), int(size.group(3)))
        elif re.search(r'Content\s+contents\s*\[', src):
            items += from_v3(path, src)
        else:
            items += from_export(path, src)
    missing = skip - {n for n, _, _ in items}
    if missing:
        sys.exit(f'--skip: no item named {", ".join(sorted(missing))}')
    items = [i for i in items if i[0] not in skip]
    names = [n for n, _, _ in items]
    if len(set(names)) != len(names):
        sys.exit('two items with the same name: ' + ', '.join(names))
    if order:
        if sorted(order) != sorted(names):
            sys.exit(f'--order must list every item once: {", ".join(names)}')
        items = [items[names.index(n)] for n in order]
    nv = ni = 0                              # numbered in gallery order
    renamed = []
    for name, frames, ms in items:
        if ms: nv += 1; new = f'video{nv}'
        else: ni += 1; new = f'image{ni}'
        renamed.append((new, frames, ms, name))
    items = [(n, f, m) for n, f, m, _ in renamed]

    with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('// GENERATED by tools/make_gallery.py from local media. PRIVATE MEDIA: LOCAL ONLY.\n')
        f.write('// Listed in .gitignore. Never commit, copy into a repository, or share this file.\n')
        f.write('#pragma once\n#define MOCHI_GALLERY_PRIVATE 1\n\n')
        for name, frames, _ in items:
            for k, fr in enumerate(frames):
                body = ',\n'.join('  ' + ', '.join(f'0x{b:02x}' for b in fr[i:i + 16]) for i in range(0, 1024, 16))
                f.write(f'static const uint8_t PROGMEM gal_{name}_{k}[1024] = {{\n{body}\n}};\n')
            f.write(f'static const uint8_t* const gal_{name}_frames[] = {{ '
                    + ', '.join(f'gal_{name}_{k}' for k in range(len(frames))) + ' };\n')
        f.write('\nstatic const GalleryItem GALLERY_ITEMS[] = {\n')
        for name, frames, ms in items:
            f.write(f'  {{ gal_{name}_frames, {len(frames)}, {ms}, "{name}" }},\n')
        f.write('};\nstatic const uint16_t GALLERY_ITEM_COUNT = sizeof(GALLERY_ITEMS) / sizeof(GALLERY_ITEMS[0]);\n')
    total = sum(len(fr) for _, fr, _ in items)
    print(f'wrote {OUT}: {len(items)} items, {total} frames = {total} KB of flash')
    for n, (name, frames, ms, src) in enumerate(renamed, 1):
        print(f'  {n:2}. {name} (from {src}): ' + (f'video, {len(frames)} frames, {ms} ms/frame' if ms else 'photo'))


if __name__ == '__main__':
    main()
