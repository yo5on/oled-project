<div align="center">
<samp><b>SPARKY · OLED COMPANION</b></samp>

<samp>esp32 · ssd1306 · arduino · embedded animation</samp>

<br/>

<img src="docs/sparky/sparky-happy.gif" width="400" alt="Sparky smiling and changing expressions"/>

</div>

---

<div align="center"><samp>A tiny animated companion that lives on a 128×64 OLED. Sparky has moods, reacts to two buttons, drifts between expressions with smooth morphs, falls asleep when left alone, and wakes up on its own.</samp></div>

---

<h2 align="center"><samp><b>Overview</b></samp></h2>

<samp>Sparky is an ESP32 sketch that turns a small SSD1306 OLED into a character with a life of its own. You don't pick animations from a menu. Sparky chooses its own faces based on how it feels, and the two buttons are ways to interact with it: a friendly tap, a poke, or pestering it until it gets cross.</samp>

<samp>Both buttons together open **Gallery Mode**, which plays photos and short videos on the same screen and then hands the display back to Sparky exactly where it left off.</samp>

<samp>The repository also keeps the earlier **OLED Eye Animation** sketch, a procedurally drawn robot face, as a separate working project.</samp>

<h2 align="center"><samp><b>Features</b></samp></h2>

- <samp>**Expressive faces.** 40 expressions built from 34 Sparky animations: happy, sad, angry, excited, funny and dramatic.</samp>
- <samp>**Smooth morphs.** Every change of face is morphed shape by shape. Nothing jumps straight to the next picture.</samp>
- <samp>**Adaptive transition timing.** Small changes are quick, while large ones take a few more in-between frames.</samp>
- <samp>**Personality and mood.** Feelings build up over time and set a mood, and the mood decides which faces Sparky picks.</samp>
- <samp>**Button reactions.** The two buttons do different things, and Sparky remembers how you have been treating it.</samp>
- <samp>**Sleep and breathing.** When tired and left alone, Sparky dozes off and breathes slowly in its sleep.</samp>
- <samp>**Natural and manual wake.** It wakes on its own after a while, or right away when you press a button.</samp>
- <samp>**Gallery Mode.** Browse photos and play looping videos, then go straight back to Sparky.</samp>

<h2 align="center"><samp><b>Hardware and Wiring</b></samp></h2>

<p align="center"><a href="docs/hardware/sparky-wiring.svg"><img src="docs/hardware/sparky-wiring.png" width="900" alt="Sparky OLED Companion wiring diagram: ESP32 Dev Module, SSD1306 OLED on GPIO 22 (SDA) and GPIO 21 (SCL), Button 1 on GPIO 25 and Button 2 on GPIO 26 to GND"/></a></p>

<div align="center"><sub>Click for the full-resolution SVG. Solid wires come straight from the pin definitions in <code>SparkyCompanion.ino</code>; dashed wires come from the project's hardware notes.</sub></div>

<h3 align="center"><samp><b>Parts</b></samp></h3>

| <samp>Part</samp> | Notes |
|---|---|
| <samp>ESP32 Dev Module</samp> | board selected for the build; Arduino core for ESP32 2.0.17 |
| <samp>SSD1306 OLED, 128×64, I2C</samp> | address `0x3C`, no reset pin used |
| <samp>2 × momentary push buttons</samp> | each between its GPIO and GND |

<samp>No external resistors or other parts are needed for the firmware to work.</samp>

<h3 align="center"><samp><b>Connections</b></samp></h3>

| <samp>Component</samp> | Component pin/signal | <samp>ESP32 GPIO or power</samp> | Notes |
|---|---|---|---|
| <samp>SSD1306 OLED</samp> | VCC | <samp>3V3</samp> | from the project's hardware notes, not checkable in code |
| <samp>SSD1306 OLED</samp> | GND | <samp>GND</samp> | common ground with the ESP32 |
| <samp>SSD1306 OLED</samp> | SDA | <samp>GPIO 22</samp> | `OLED_SDA 22`, `Wire.begin(OLED_SDA, OLED_SCL)` |
| <samp>SSD1306 OLED</samp> | SCL | <samp>GPIO 21</samp> | `OLED_SCL 21` |
| <samp>Button 1</samp> | leg 1 | <samp>GPIO 25</samp> | `BTN_1 25`, `INPUT_PULLUP` |
| <samp>Button 1</samp> | leg 2 | <samp>GND</samp> | pressed = `LOW` |
| <samp>Button 2</samp> | leg 1 | <samp>GPIO 26</samp> | `BTN_2 26`, `INPUT_PULLUP` |
| <samp>Button 2</samp> | leg 2 | <samp>GND</samp> | pressed = `LOW` |

<samp>Each GPIO has exactly one job: 21 and 22 for I2C, and 25 and 26 for the buttons. None of them is a boot-strapping pin. The same pinout is used by the earlier V3 viewer and the `OLED_Eye_Animation` sketch.</samp>

<h3 align="center"><samp><b>Power</b></samp></h3>

<samp>The OLED is powered from the ESP32's **3V3** pin, so its I2C lines run at the ESP32's 3.3 V logic level. All grounds (ESP32, OLED and both buttons) are common. Don't feed the OLED from 5 V unless you have confirmed that your module's I2C pull-ups go to its own regulated 3.3 V.</samp>

<h3 align="center"><samp><b>OLED interface</b></samp></h3>

<samp>The display is an SSD1306 on I2C at address `0x3C`, started with `display.begin(SSD1306_SWITCHCAPVCC, 0x3C)`. Its internal charge pump makes the panel voltage, so only VCC and GND are needed. **SDA and SCL are deliberately swapped** compared with the ESP32's usual defaults: SDA is on GPIO 22 and SCL on GPIO 21. Wire them exactly as shown. No reset line is connected (`-1` in the driver).</samp>

<samp>Every frame is a full 128×64, 1-bit image (1024 bytes) pushed in one transfer, about 29 ms per frame. Some source clips are stored black-on-white, and the player inverts them in software so every face appears as light on dark.</samp>

<h3 align="center"><samp><b>Buttons</b></samp></h3>

<samp>Each button connects its GPIO to GND. The firmware enables the ESP32's internal pull-ups (`INPUT_PULLUP`), so a button reads `HIGH` when released and `LOW` when pressed, and no external resistors are needed. Inputs are debounced (30 ms). Pressing both buttons within 150 ms of each other counts as a combo. A combo fires once and then waits until both buttons are released, so it never leaks into single presses.</samp>

<h3 align="center"><samp><b>ESP32 memory</b></samp></h3>

<samp>The animations live in flash as raw or PackBits-compressed frames, and RAM use stays around 105 KB. The public build fits the default partition. The build with your own gallery media needs **Huge APP**, and the sketch stops with a clear compile error if you forget.</samp>

<h3 align="center"><samp><b>Still to check on the real hardware</b></samp></h3>

<samp>These can't be confirmed from the code or the repository:</samp>

- <samp>**OLED supply voltage.** 3V3 comes from the project's hardware notes. Check your module's rating; most SSD1306 breakouts accept 3.3–5 V.</samp>
- <samp>**ESP32 power source.** The diagram assumes USB, which is also used for upload and the serial monitor.</samp>
- <samp>**OLED header pin order.** It varies between modules (often GND, VCC, SCL, SDA), so wire by the printed pin names, not by position.</samp>
- <samp>**I2C pull-ups.** These are expected on the OLED module, as on most breakouts, but not confirmed. The firmware doesn't enable any for I2C.</samp>
- <samp>**Exact dev board.** The board model and the physical position of each pin on its headers depend on your board. Find the pins by their GPIO label.</samp>

<h2 align="center"><samp><b>Sparky expression system</b></samp></h2>

<samp>Every expression is a named slice of an animation, such as `SMILE`, `GLOOM`, `FURIOUS` or `HEARTS`. Each one carries:</samp>

- <samp>which frames to play, and how many times;</samp>
- <samp>the moods it belongs to (happy, curious, playful, sad and so on);</samp>
- <samp>how strong it is, so mild moods pick gentle faces and strong moods pick big ones;</samp>
- <samp>how often it should come up compared with its neighbours.</samp>

<samp>Sparky doesn't repeat itself much: recently used faces are less likely, and no single animation dominates a long session. Some expressions play once and settle, like a blink, a glance or a beam. Others loop for a moment before moving on.</samp>

<h2 align="center"><samp><b>Natural expression transitions</b></samp></h2>

<samp>Switching faces is where an OLED character usually looks cheap, so most of the work went here.</samp>

- <samp>**Morphs.** The current face and the next one are split into parts, such as eyes, mouth and extras. Each part is blended into its counterpart over a few in-between frames.</samp>
- <samp>**Adaptive pacing.** The number of in-between frames depends on how much actually changes. A twitch of the eyelids takes one or two steps, and a whole new face takes up to seven. The easing sits between linear and smoothstep, so motion neither crawls nor snaps.</samp>
- <samp>**Eye bridge.** When the eyes change shape completely, for example round eyes turning into heart eyes, Sparky briefly closes its eyes, the closed faces morph, and the new eyes open. It reads like a natural blink.</samp>
- <samp>**Waypoints.** A few dramatic faces, like sobbing and gloom, pass through an in-between pose on the way in or out, so they ease in instead of popping.</samp>
- <samp>**Button reactions** start from whatever is on screen at that moment, even mid-morph.</samp>

<h2 align="center"><samp><b>Personality and behaviour</b></samp></h2>

<samp>Sparky's behaviour runs as a small pipeline:</samp>

```text
feelings  →  mood  →  intent  →  expression
```

- <samp>**Feelings** (energy, happiness, annoyance, curiosity and so on) drift slowly over time and change with every interaction.</samp>
- <samp>**Mood** is one of nine states: neutral, happy, curious, playful, excited, annoyed, sad, sleepy and affectionate. A mood lasts a while and only moves to neighbouring moods. A happy Sparky doesn't suddenly turn furious.</samp>
- <samp>**Intent** decides what to do next: chain another expression, pause briefly, or rest on the mood's face.</samp>
- <samp>**Resting faces** aren't frozen. They blink, sway slightly or play their own idle frames.</samp>

<h2 align="center"><samp><b>Sleep, breathing and natural wake</b></samp></h2>

- <samp>**Falling asleep.** When Sparky is low on energy and has been left alone for a few minutes, its eyes get heavy and close in stages. It always falls asleep eventually if nobody plays with it.</samp>
- <samp>**Breathing.** While asleep, the closed eyes slowly swell and settle. It drifts between light, deep and deepest sleep, and every so often gives a tiny twitch or a deeper sigh. The eyes never open during sleep.</samp>
- <samp>**Natural wake.** After 5–15 minutes Sparky wakes by itself between two breaths: the lids lift slowly, it gives a slow blink and a yawn, and it stays groggy for a while before getting back to normal.</samp>
- <samp>**Manual wake.** Press a button and Sparky wakes immediately, a little startled.</samp>

<h2 align="center"><samp><b>Button interactions</b></samp></h2>

| <samp>Input</samp> | Sparky | <samp>Gallery Mode</samp> |
|---|---|---|
| <samp>**Button 1** (GPIO 25)</samp> | Friendly tap: wakes Sparky and makes it happier and more playful | <samp>Previous item</samp> |
| <samp>**Button 2** (GPIO 26)</samp> | Poke: surprises Sparky and makes it curious | <samp>Next item</samp> |
| <samp>**Many presses quickly**</samp> | Sparky gets annoyed, sometimes dizzy, and eventually furious | <samp>–</samp> |
| <samp>**Both buttons**</samp> | Enter Gallery Mode | <samp>Back to Sparky</samp> |

<samp>Sparky's reaction depends on its mood. A sleepy Sparky blinks at you, and a happy one beams. Keep pestering it and it climbs a ladder from deadpan to annoyed to furious.</samp>

<h2 align="center"><samp><b>Gallery Mode</b></samp></h2>

<samp>Gallery Mode is a small photo and video viewer for the same screen, reusing the media from the earlier V3 viewer sketch.</samp>

- <samp>Photos stay on screen until you move on.</samp>
- <samp>Videos play at their own frame rate and loop.</samp>
- <samp>Button 1 and button 2 move backwards and forwards with wrap-around.</samp>
- <samp>Leaving the gallery returns Sparky exactly as it was: same face, same mood. The button press that closed the gallery doesn't count as poking Sparky.</samp>

<h3 align="center"><samp><b>Using your own media</b></samp></h3>

<samp>The media is never stored in this repository. It's generated locally into a git-ignored header:</samp>

```bash
python SparkyCompanion/tools/make_gallery.py path/to/v3.ino
```

<samp>This writes `SparkyCompanion/gallery_media_private.h`. It is listed in `.gitignore` and should stay local. The tool also accepts OLED Animation Maker exports and raw bitmaps (`file.txt@WxH`), and supports `--order` and `--skip`. Without that file the sketch builds with a single placeholder screen.</samp>

<samp>> Build with **Tools → Partition Scheme → Huge APP (3MB No OTA/1MB SPIFFS)** when the private media header is present.</samp>

<h2 align="center"><samp><b>Animation system</b></samp></h2>

<samp>Two frame formats share one player:</samp>

| <samp>Format</samp> | Source | <samp>Storage</samp> |
|---|---|---|
| <samp>RAW</samp> | 19 clips cut from one long 250-frame source recording | <samp>1024 bytes per frame, stored once and inverted on the fly</samp> |
| <samp>PACKED</samp> | 15 separate emote GIFs | <samp>PackBits-compressed frames</samp> |

<samp>The player (`SparkyPlayer.h`) plays, holds and morphs frames, and the morph engine (`SparkyMorph.h`) works on the parts of a face rather than on raw pixels. All timing is non-blocking with `millis()`, so the buttons stay responsive during every animation and morph.</samp>

<h2 align="center"><samp><b>Animation cleanup and quality improvements</b></samp></h2>

<samp>The original frames were extracted from video and had small defects that look bad on a crisp OLED. These are fixed at playback time, and the stored animation data is unchanged:</samp>

- <samp>**Despeckle.** Stray single pixels are removed from a handful of noisy animations, such as Furious, Annoyed and Laugh.</samp>
- <samp>**Scanline fill.** One-pixel gaps in the Evil Grin eyes are closed.</samp>
- <samp>**Closed-eye frame fix.** A broken blink frame is replaced with clean, slightly open lids.</samp>
- <samp>**Range tuning.** Each expression starts and ends on a clean frame, so morphs begin from a sensible face.</samp>
- <samp>**Removed clip.** One animation (clip 26 of the source recording) was dropped completely because it couldn't be made to look right.</samp>

<h2 align="center"><samp><b>Animation Preview</b></samp></h2>

<div align="center"><samp>Recorded from the firmware's own frames and timing (expression → morph → expression), rendered in the blue-white OLED look.</samp></div>

<br/>

<table>
<tr>
<td align="center"><img src="docs/sparky/sparky-happy.gif" width="380" alt="Happy and calm faces"/><br/><samp><b>Happy / calm</b></samp><br/><sub>calm · smile · beam · grin · smug</sub></td>
<td align="center"><img src="docs/sparky/sparky-sad.gif" width="380" alt="Sad faces"/><br/><samp><b>Sad</b></samp><br/><sub>worried · teary · gloom · sob · crying</sub></td>
</tr>
<tr>
<td align="center"><img src="docs/sparky/sparky-angry.gif" width="380" alt="Angry faces"/><br/><samp><b>Angry</b></samp><br/><sub>deadpan · annoyed · squeeze · furious</sub></td>
<td align="center"><img src="docs/sparky/sparky-excited.gif" width="380" alt="Excited faces"/><br/><samp><b>Excited</b></samp><br/><sub>surprised · chatter · XD · hearts · kiss</sub></td>
</tr>
<tr>
<td align="center"><img src="docs/sparky/sparky-funny.gif" width="380" alt="Funny faces"/><br/><samp><b>Funny</b></samp><br/><sub>uwu · cheeky · shy · peek · evil grin</sub></td>
<td align="center"><img src="docs/sparky/sparky-dramatic.gif" width="380" alt="Dramatic faces"/><br/><samp><b>Special / dramatic</b></samp><br/><sub>shock · scream · dizzy · glitch · evil</sub></td>
</tr>
<tr>
<td align="center" colspan="2"><img src="docs/sparky/sparky-sleep.gif" width="380" alt="Falling asleep, breathing and waking up"/><br/><samp><b>Sleep / wake</b></samp><br/><sub>drowsy → asleep and breathing → woken by a button → groggy</sub></td>
</tr>
</table>

<h2 align="center"><samp><b>Build and upload</b></samp></h2>

<samp>1. Install **Arduino IDE** and add **esp32 by Espressif Systems** (2.0.17) in Boards Manager.</samp>
<samp>2. Install **Adafruit SSD1306** from Library Manager and accept its dependencies (Adafruit GFX, Adafruit BusIO).</samp>
<samp>3. Open `SparkyCompanion/SparkyCompanion.ino`.</samp>
<samp>4. Select **Tools → Board → ESP32 Dev Module** and your COM port.</samp>
<samp>5. Partition scheme: **Default** for the public build, or **Huge APP** if you generated your own gallery media.</samp>
<samp>6. Upload. If it hangs at `Connecting...`, hold **BOOT** while the upload starts.</samp>
<samp>7. Optional: open Serial Monitor at **115200** baud to watch Sparky's moods and decisions.</samp>

| <samp>Build</samp> | Flash | <samp>RAM</samp> |
|---|---|---|
| <samp>Public (placeholder gallery)</samp> | ~895 KB | <samp>~105 KB</samp> |
| <samp>With private gallery media (Huge APP)</samp> | ~1.49 MB | <samp>~105 KB</samp> |

<samp>`SPARKY_HW_TEST` at the top of the sketch switches to fast timings for bench testing: moods change in seconds, Sparky sleeps after 2 minutes, and serial keys act as buttons.</samp>

<h2 align="center"><samp><b>Project structure</b></samp></h2>

```text
oled-project/
├── SparkyCompanion/                 Sparky, the animated companion (main project)
│   ├── SparkyCompanion.ino          Display, buttons, mode switching
│   ├── SparkyBehavior.h             Personality, moods, sleep and wake, reactions
│   ├── SparkyEmotion.h              Feelings and moods
│   ├── SparkyExpressions.h          The 40 expressions and their tuning
│   ├── SparkyPlayer.h               Playback, morph pacing, eye bridge, cleanup
│   ├── SparkyMorph.h                Part-based morph engine
│   ├── SparkyAnimations.h           Animation table
│   ├── SparkyButtons.h              Debounce and two-button combo
│   ├── SparkyGallery.h              Gallery Mode
│   ├── gallery_placeholder.h       Public placeholder gallery
│   ├── animations/                 Frame data
│   └── tools/make_gallery.py       Builds your local, git-ignored gallery header
├── OLED_Eye_Animation/             Earlier procedural robot-eye sketch (still works)
│   ├── OLED_Eye_Animation.ino
│   └── EyeVariants.h
├── docs/
│   ├── sparky/                      Animation previews
│   ├── hardware/                   Wiring diagram (SVG, PNG and its generator script)
│   └── eyes/                       Robot-eye previews
└── archive/
    └── esp32-eyes-main/            Old reference code, not compiled
```

<h3 align="center"><samp><b>OLED Eye Animation</b></samp></h3>

<samp>The earlier sketch draws 11 robot-eye emotions live from one curved eye shape, with no bitmaps, and has its own media slots and button reactions. Open `OLED_Eye_Animation/OLED_Eye_Animation.ino` to use it.</samp>

<p align="center"><img src="docs/eyes/eye-mode-preview.gif" width="380" alt="Robot eye emotions"/></p>

<h2 align="center"><samp><b>Archive</b></samp></h2>

<samp>`archive/` keeps code that is no longer part of any build but is worth keeping for reference and history:</samp>

- <samp>**`archive/esp32-eyes-main/`** is the [esp32-eyes](https://github.com/playfultechnology/esp32-eyes) source that an early version of the robot-eye sketch was based on. It is kept with its original AGPL-3.0 license.</samp>

<h2 align="center"><samp><b>Credits</b></samp></h2>

- <samp>**Animation artwork.** Sparky's face animations come from the *Dasai Mochi* emotes, via the [huykhong](https://www.tiktok.com/@_huykhong) ESP32 Mochi clone and gif2cpp. All rights to the original character and artwork belong to their creators.</samp>
- <samp>**Robot-eye style** was inspired by a design made with [OLED Animation Maker](https://www.oledanimationmaker.com/). The `EyeVariants.h` implementation is original.</samp>
- <samp>**esp32-eyes** is by Alastair Aitchison (Playful Technology) and Luis Llamas, AGPL-3.0 (archived reference).</samp>
- <samp>**Libraries:** Adafruit SSD1306, Adafruit GFX and Adafruit BusIO.</samp>

<div align="center">

---

<samp><strong>Yoson</strong> · software, AI/ML, embedded systems and robotics</samp>

<samp><a href="https://github.com/yo5on">github.com/yo5on</a> · <a href="https://www.linkedin.com/in/yo5on">LinkedIn</a></samp>

</div>
