"""Render OLED-sized emotion and animation previews for v4.

The drawing helpers and emotion cases mirror EyeVariants.h. Each frame is
rendered on a 128x64, 1-bit canvas, then enlarged only with nearest-neighbour
scaling for the labelled sheet. The GIF stays at the device's native size.
Requires Pillow: py -m pip install Pillow
"""

from __future__ import annotations

import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
W, H = 128, 64
LX, RX, UY, LY, MX, MY = 39, 89, 27, 36, 64, 44
CYAN = (0, 229, 255)
WHITE = 1
SIN_Q = [0,25,49,74,98,122,147,171,195,219,243,267,290,314,337,360,
         383,405,428,450,471,493,514,535,556,576,596,615,634,653,672,690,
         707,724,741,757,773,788,803,818,831,845,858,870,882,893,904,914,
         924,933,942,950,957,964,970,976,981,985,989,992,995,997,999,1000,1000]
NAMES = ["Neutral", "Happy", "Sad", "Angry", "Sleepy", "Surprised", "Worried",
         "Confused", "Excited", "Suspicious", "Love/Cute", "Bored", "Scared", "Furious"]


def cdiv(a: int, b: int) -> int:
    return a // b if a >= 0 else -((-a) // b)


def rdiv(a: int, b: int) -> int:
    return cdiv(a + (b // 2 if a >= 0 else -(b // 2)), b)


def sin256(p: int) -> int:
    p &= 255
    quadrant = p >> 6
    i = p & 63
    q = SIN_Q[i if quadrant in (0, 2) else 64 - i]
    return q if quadrant < 2 else -q


def wave(t: int, period: int) -> int:
    return sin256((t % period) * 256 // period)


def hash32(n: int) -> int:
    n &= 0xFFFFFFFF
    n ^= n >> 16
    n = (n * 0x7FEB352D) & 0xFFFFFFFF
    n ^= n >> 15
    n = (n * 0x846CA68B) & 0xFFFFFFFF
    n ^= n >> 16
    return n & 0xFFFFFFFF


def jitter(t: int, step: int, span: int, seed: int) -> int:
    return hash32(t // step * 7 + seed) % (2 * span + 1) - span


def blink_shape(local: int, start: int, dur: int) -> int:
    if local < start or local >= start + dur:
        return 0
    elapsed, half = local - start, dur // 2
    return elapsed * 1000 // half if elapsed < half else (dur - elapsed) * 1000 // (dur - half)


def blink(t: int, period: int, dur: int, seed: int) -> int:
    n = t // period
    h = hash32(n * 31 + seed)
    local = t % period
    c = blink_shape(local, h % (period // 2) + period // 4, dur)
    if ((h >> 8) % 4) == 0:
        c = max(c, blink_shape(local, h % (period // 2) + period // 4 + dur + 90, dur))
    return c


def path(t: int, points: list[int], hold: int, move: int) -> int:
    seg, within = t // hold, t % hold
    target, source = points[seg % len(points)], points[(seg + len(points) - 1) % len(points)]
    if seg == 0 or within >= move:
        return target
    eased = (1000 - sin256(within * 128 // move + 64)) // 2
    return source + cdiv((target - source) * eased, 1000)


def shrink(size: int, closure: int, min_size: int) -> int:
    return size - cdiv((size - min_size) * closure, 1000)


class OLED:
    def __init__(self) -> None:
        self.im = Image.new("1", (W, H), 0)
        self.d = ImageDraw.Draw(self.im)

    def hline(self, x: int, y: int, w: int) -> None:
        if w > 0:
            self.d.line((x, y, x + w - 1, y), fill=WHITE)

    def circle(self, x: int, y: int, r: int) -> None:
        if r >= 0:
            self.d.ellipse((x-r, y-r, x+r, y+r), fill=WHITE)

    def triangle(self, *pts: tuple[int, int]) -> None:
        self.d.polygon(pts, fill=WHITE)

    def box(self, cx: int, cy: int, w: int, h: int, r: int) -> None:
        if w < 1 or h < 1:
            return
        r = max(0, min(r, w // 2, h // 2))
        self.d.rounded_rectangle((cx-w//2, cy-h//2, cx+(w-1)//2, cy+(h-1)//2), radius=r, fill=WHITE)

    def capsule(self, x0: int, y0: int, x1: int, y1: int, r: int) -> None:
        self.circle(x0, y0, r)
        self.circle(x1, y1, r)
        self.d.line((x0, y0, x1, y1), fill=WHITE, width=2*r+1)

    def bar(self, cx: int, cy: int, half: int, r: int, side: int, din: int, dout: int) -> None:
        self.capsule(cx-side*half, cy+din, cx+side*half, cy+dout, r)

    def wedge(self, cx: int, cy: int, side: int, half: int, drop: int, radius: int) -> None:
        ox, ix = cx+side*half, cx-side*half
        oy, iy = cy-drop//2, cy+drop
        bx, by = ix+side*3, iy-1
        self.capsule(ox, oy, bx, by, radius)
        self.triangle((bx, by-radius), (bx, by+radius), (ix, iy))

    def ellipse_rows(self, cx: int, cy: int, rx: int, ry: int, ymin: int, ymax: int) -> None:
        if rx < 0 or ry <= 0:
            return
        rr, xx = ry*ry+ry, rx*rx+rx
        for dy in range(-ry, ry+1):
            y = cy + dy
            if ymin <= y <= ymax:
                dx = math.isqrt(xx * (rr - dy*dy) // rr)
                self.hline(cx-dx, y, 2*dx+1)

    def oval(self, cx: int, cy: int, rx: int, ry: int) -> None:
        self.ellipse_rows(cx, cy, rx, ry, cy-ry, cy+ry)

    def half_oval(self, cx: int, cy: int, rx: int, ry: int) -> None:
        self.ellipse_rows(cx, cy, rx, ry, cy, cy+ry)

    def arc(self, cx: int, cy: int, rx: int, ry: int, a0: int, a1: int, radius: int, tilt: int) -> None:
        previous = None
        for i in range(7):
            a = a0 + cdiv((a1-a0)*i, 6)
            ox = rdiv(rx*sin256(a+64), 1000)
            p = (cx+ox, cy-rdiv(ry*sin256(a), 1000)+rdiv(ox*tilt,16))
            if previous is not None:
                self.capsule(*previous, *p, radius)
            previous = p

    def eye_arc(self, cx: int, cy: int, rx: int, ry: int, radius: int, side: int, droop: int) -> None:
        self.arc(cx, cy, rx, ry, 10, 118, radius, side*droop)

    def heart(self, cx: int, cy: int, r: int) -> None:
        self.circle(cx-r, cy-r//2, r)
        self.circle(cx+r, cy-r//2, r)
        self.triangle((cx-2*r, cy-r//2+1), (cx+2*r, cy-r//2+1), (cx, cy+3*r//2))

    def zig(self, cx: int, cy: int, half: int, amp: int, flip: int, radius: int) -> None:
        step, x = half//2, cx-half
        s = amp if flip else -amp
        for _ in range(4):
            self.capsule(x, cy+s, x+step, cy-s, radius)
            x += step
            s = -s

    def bitmap(self, x: int, y: int, data: tuple[int, ...], w: int, h: int) -> None:
        rowbytes = (w+7)//8
        for yy in range(h):
            for xx in range(w):
                if data[yy*rowbytes + xx//8] & (0x80 >> (xx & 7)):
                    self.im.putpixel((x+xx, y+yy), WHITE)

    def rgb(self) -> Image.Image:
        rgb = Image.new("RGB", (W, H), (0, 0, 0))
        rgb.putdata([CYAN if p else (0, 0, 0) for p in self.im.get_flattened_data()])
        return rgb


QUESTION = (0x3C,0x7E,0x66,0x06,0x0C,0x18,0x18,0x00,0x18,0x18)
ZMARK = (0xFE,0xFE,0x0C,0x18,0x30,0xFE,0xFE)
SPARK = (0x10,0x10,0x38,0xFE,0x38,0x10,0x10)
ANGER = (0x63,0x00,0xE3,0x80,0xC1,0x80,0x00,0x00,0x00,0x00,0xC1,0x80,0xE3,0x80,0x63,0x00)


def render(i: int, t: int) -> Image.Image:
    d = OLED()
    if i == 0:  # Neutral
        look = path(t, [0,-4,0,4], 2300, 260)
        h = shrink(6 + cdiv(wave(t,2600),1500), blink(t,3100,180,1), 1)
        for side in (-1,1): d.box((LX if side < 0 else RX)+look, UY, 17, h, 3)
        d.arc(MX+cdiv(look,2), MY+1, 7, 3, 138, 246, 1, 0)
    elif i == 1:  # Happy
        b, dy = abs(wave(t,900)), 0
        dy = -(b*2//1000)
        ry = 4 - (1000-b)//1000
        for side in (-1,1):
            ex = LX if side < 0 else RX
            d.eye_arc(ex, UY+3+dy, 6, ry, 2, side, 0)
            d.bar(ex, LY+dy, 4, 2, side, 0, -1)
        d.arc(MX, MY, 8, 4, 138, 246, 2, 0)
    elif i == 2:  # Sad
        dy = cdiv(wave(t,4200)*2,1000)
        lid = shrink(2, blink(t,3600,300,3), 0)
        for side in (-1,1):
            ex = LX if side < 0 else RX
            d.bar(ex, UY+dy, 5, 2-lid, side, -1, 2)
            d.arc(ex, LY+1+dy, 6, 2, 138, 246, 1, 0)
        ph=t%3400
        if ph<1100:
            ty=LY+4+dy+ph*9//1100
            d.triangle((RX+5,ty-3),(RX+7,ty),(RX+4,ty+1))
        d.arc(MX,MY+1,6,3,10,118,1,0)
    elif i == 3:  # Angry
        inward=(wave(t,1400)+1000)*2//2000
        j=jitter(t,70,1,5)
        for side in (-1,1): d.wedge((LX if side<0 else RX)-side*inward, UY+j, side, 8, 4, 1)
        d.bar(MX,MY+1+j,4,1,1,0,0)
    elif i == 4:  # Sleepy
        ph=t%5200
        if ph<2600: k=400+300*ph//2600
        elif ph<3200: k=700+300*(ph-2600)//600
        elif ph<4300: k=1000
        else: k=1000-600*(ph-4300)//900
        lid=UY-3+10*k//1000
        for side in (-1,1):
            ex=LX if side<0 else RX
            d.arc(ex,LY-1,7,2,138,246,1,0)
            d.bar(ex,lid,7,1,side,0,0)
        if ph>=3000:
            z=ph-3000; d.bitmap(108+z//700,14-z*8//2200,ZMARK,7,7)
        if 3200<=ph<4300: d.box(MX,MY+1,4,2,1)
    elif i == 5:  # Surprised
        ph=t%2400
        if ph<110: pop=6*ph//110
        elif ph<260: pop=6-2*(ph-110)//150
        else: pop=4
        w=16+pop//2; h=shrink(6+pop//2,blink(t,5000,140,4),1)
        for side in (-1,1):
            ex=LX if side<0 else RX
            d.box(ex,UY+1,w,h,3)
        d.oval(MX,MY+1,2,3)
    elif i == 6:  # Worried
        look=path(t,[0,-4,0,4,-2],700,120); c=blink(t,2700,150,7)
        for side in (-1,1):
            ex=(LX if side<0 else RX)+cdiv(look,2)
            q=cdiv(wave(t,330 if side<0 else 410),700)
            w=15 if side<0 else 18
            h=shrink(7 if side<0 else 9,c,1)
            d.bar(ex,UY-8+q,4,1,side,-1 if side<0 else 1,1 if side<0 else -1)
            d.box(ex,UY+q,w,h,3)
        d.zig(MX,MY+1,4,1,(t//300)&1,1)
    elif i == 7:  # Confused
        s=wave(t,3600)
        d.box(LX,UY-cdiv(s,700),17,shrink(7+cdiv(s*2,1000),blink(t,3900,160,3),1),3)
        d.bar(LX,LY,3,1,-1,0,0)
        d.bar(RX,UY+cdiv(s,700),4,2,1,-2-cdiv(s*2,1000),2+cdiv(s*2,1000))
        d.bitmap(111,11+abs(wave(t,1000))*2//1000,QUESTION,8,10)
        m=cdiv(s*2,1000); d.capsule(MX-4,MY+1+m,MX+4,MY+1-m,1)
    elif i == 8:  # Excited
        b=abs(wave(t,840)); dy=-(b*2//1000); wv=(wave(t,700)+1000)//1000
        for side in (-1,1):
            ex=LX if side<0 else RX
            d.box(ex,UY+dy,17+wv,6,2); d.bar(ex,LY+dy,5,1,side,0,0)
        d.half_oval(MX,MY-1+dy,7,2+(wave(t,500)+1000)*2//2000)
        if (t//180)%2==0:
            d.bitmap(10,12,SPARK,7,7); d.bitmap(111,39,SPARK,7,7)
        else:
            d.bitmap(10,39,SPARK,7,7); d.bitmap(111,12,SPARK,7,7)
    elif i == 9:  # Suspicious
        look=path(t,[-6,6],1500,450)
        tilt=cdiv(wave(t,2900)*2,1000)
        d.bar(LX+look,UY+1,8,1,-1,1+tilt,-1-tilt)
        d.box(RX-cdiv(look,3),UY,15,shrink(6,blink(t,4600,140,9),1),3)
        d.capsule(MX+2+cdiv(look,3),MY+1,MX+8+cdiv(look,3),MY-1,1)
    elif i == 10:  # Love/Cute
        ph=t%1250; pulse=0
        if ph<120: pulse=3*ph//120
        elif ph<240: pulse=3-3*(ph-120)//120
        elif 300<=ph<400: pulse=2*(ph-300)//100
        elif 400<=ph<520: pulse=2-2*(ph-400)//120
        dy=cdiv(wave(t,2600)*2,1000); squint=t%3200>=2600
        for side in (-1,1):
            ex=LX if side<0 else RX
            if squint: d.eye_arc(ex,UY+3+dy,7,4,2,side,0)
            else: d.box(ex,UY,15,6,3)
            d.bar(ex,LY+dy,3,1,side,0,-1)
        d.heart(MX,MY+dy,2+(pulse+1)//2)
    elif i == 11:  # Bored
        look=path(t,[0,5,5,-4],2600,1400); c=blink(t,4800,500,11)
        lid=UY+1+3*c//1000
        for side in (-1,1):
            ex=(LX if side<0 else RX)+look
            d.box(ex,lid,17,3,1); d.box(ex,LY-1,10,1,0)
        d.bar(MX+cdiv(look,2),MY+1,4,1,1,0,0)
    elif i == 12:  # Scared
        look=path(t,[0,-6,4,-5,6,-2],650,60)
        jx,jy=jitter(t,50,1,13),jitter(t,50,1,17)
        h=shrink(6+(wave(t,400)+1000)//1500,blink(t,2200,90,13),1)
        for side in (-1,1):
            ex=(LX if side<0 else RX)+side*2+look+jx
            d.box(ex,UY+jy,10,h,2); d.bar(ex,LY+jy,2,1,side,0,0)
        d.zig(MX+jx,MY+1,3,1,(t//90)&1,1)
    elif i == 13:  # Furious
        sx,sy=jitter(t,40,1,19),jitter(t,40,1,23)
        fl=3 if wave(t,600)>0 else 2
        for side in (-1,1): d.wedge((LX if side<0 else RX)+sx,UY+sy,side,6,6,fl)
        if (t//250)%2==0: d.bitmap(108,10,ANGER,9,8)
        d.zig(MX+sx,MY+1+sy,6,1,(t//120)&1,1)
    return d.rgb()


def build_sheet() -> None:
    # UI-only gallery restyle based on the supplied card layout. The OLED
    # frames still come directly from render(); no emotion geometry changes.
    cols, scale, gap, pad, label_h = 4, 2, 18, 20, 42
    screen_w, screen_h = W*scale, H*scale
    card_w, card_h = screen_w, screen_h + label_h
    rows = math.ceil(len(NAMES)/cols)
    out = Image.new("RGB", (2*pad+cols*card_w+(cols-1)*gap,
                             2*pad+rows*card_h+(rows-1)*gap), (239,241,246))
    draw = ImageDraw.Draw(out)
    try: font = ImageFont.truetype("arialbd.ttf", 16)
    except OSError:
        try: font = ImageFont.truetype("DejaVuSans-Bold.ttf", 16)
        except OSError: font = ImageFont.load_default()
    for i, name in enumerate(NAMES):
        x, y = pad+(i%cols)*(card_w+gap), pad+(i//cols)*(card_h+gap)
        draw.rounded_rectangle((x,y,x+card_w-1,y+card_h-1),radius=15,
                               fill=(255,255,255),outline=(218,222,231),width=1)
        # Rounded top corners on a 2x nearest-neighbour OLED screen.
        draw.rounded_rectangle((x,y,x+card_w-1,y+screen_h-1),radius=14,fill=(0,0,0))
        draw.rectangle((x,y+screen_h//2,x+card_w-1,y+screen_h-1),fill=(0,0,0))
        screen = render(i,500).resize((screen_w,screen_h),Image.Resampling.NEAREST)
        screen_mask = Image.new("L", (screen_w,screen_h), 0)
        md = ImageDraw.Draw(screen_mask)
        md.rounded_rectangle((0,0,screen_w-1,screen_h-1),radius=14,fill=255)
        md.rectangle((0,screen_h//2,screen_w-1,screen_h-1),fill=255)
        out.paste(screen,(x,y),screen_mask)
        draw.text((x+14,y+screen_h+8),f"{i+1}. {name}",font=font,fill=(38,41,48))
    out.save(DOCS/"eye-emotions-sheet.png",optimize=True)


def build_gif() -> None:
    # One full 128x64 OLED at a time; sample at 10fps and retain each
    # emotion's different cycle length so slow expressions can be judged.
    durations = [3600,900,4200,1400,5200,2400,2100,1800,1260,3000,2500,4800,1300,1200]
    frames=[]
    for i,duration in enumerate(durations):
        for t in range(0,duration,100):
            frames.append(render(i,t).convert("P",palette=Image.Palette.ADAPTIVE,colors=4))
    frames[0].save(DOCS/"eye-mode-preview.gif",save_all=True,append_images=frames[1:],duration=100,loop=0,optimize=True,disposal=2)


def build_style_reference() -> None:
    # Five dark/light OLED module preview cards in the arrangement of the
    # supplied reference. The screen pixels come directly from render().
    out=Image.new("RGB",(1392,490),(20,20,22)); d=ImageDraw.Draw(out)
    card_w,card_h,scale=420,210,3
    positions=[(24,20),(486,20),(948,20),(255,258),(717,258)]
    picks=[(0,500),(1,100),(8,630),(9,2300),(10,430)]
    for index,((emotion,time),(x,y)) in enumerate(zip(picks,positions)):
        d.rounded_rectangle((x,y,x+card_w-1,y+card_h-1),radius=14,fill=(19,19,20),outline=(67,67,70),width=2)
        if index in (0,3):
            d.rounded_rectangle((x+6,y+6,x+card_w-6,y+card_h-6),radius=10,fill=(226,230,232))
            sx,sy=x+18,y+9
        elif index in (1,4):
            d.rounded_rectangle((x+8,y+7,x+card_w-8,y+card_h-7),radius=9,fill=(14,14,15),outline=(61,61,64),width=1)
            d.rectangle((x+1,y+21,x+card_w-1,y+card_h-12),fill=(226,230,232))
            sx,sy=x+18,y+9
        else:
            d.rounded_rectangle((x+8,y+8,x+card_w-8,y+card_h-8),radius=10,fill=(13,13,14),outline=(54,54,57),width=1)
            sx,sy=x+18,y+9
        screen=render(emotion,time).resize((384,192),Image.Resampling.NEAREST)
        out.paste((8,8,9),(sx,sy,sx+384,sy+192)); out.paste(screen,(sx,sy))
        texture=Image.new("RGBA",(384,192),(0,0,0,0)); td=ImageDraw.Draw(texture)
        for gx in range(3,384,3): td.line((gx,0,gx,191),fill=(180,230,236,8))
        for gy in range(3,192,3): td.line((0,gy,383,gy),fill=(180,230,236,7))
        out.paste(Image.alpha_composite(out.crop((sx,sy,sx+384,sy+192)).convert("RGBA"),texture).convert("RGB"),(sx,sy))
    out.save(DOCS/"eye-style-reference.png",optimize=True)


if __name__ == "__main__":
    DOCS.mkdir(parents=True,exist_ok=True)
    build_sheet()
    build_gif()
    build_style_reference()
    print("Rendered native 128x64 emotion sheet and animation previews.")
