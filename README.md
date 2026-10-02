# OLED Eye Animation

An ESP32 and SSD1306 OLED project with a mood-driven animated robot face, two-button controls, and an empty media framework ready for future content.

## Current Features

- Eye Mode with 11 animated emotions, selected using weighted randomness with no immediate repeats.
- A rounded, curved superellipse eye shape with narrowed eyes and a larger face.
- Non-blocking animation and button handling.
- PREV and NEXT navigate Normal Mode content slots. Press both buttons together to enter or exit Eye Mode.
- In Eye Mode, single PREV/NEXT presses count toward a random threshold of 1–5 clicks. The existing reaction state machine currently types messages for Happy and Sad; other emotions reset the click counter at the threshold without showing a message.
- Five empty video slots and five empty image slots. Media is intentionally absent so it can be added later.

## Eye Mode and Emotions

Press PREV and NEXT together to enter Eye Mode. An emotion is selected immediately, then animates while it is active. After a random duration for that emotion, another is selected; the same emotion is not selected twice in a row. Single button presses do not change the emotion.

The current emotions and animations are:

| Emotion | Current animation |
|---|---|
| Happy | Curved eyelids over small lower pieces, a bean smile, gentle bounce, and mouth pulse |
| Sad | Drooping eyes, slow sinking, a tear, slow blinks, and a frown |
| Angry | Tilted eyes with inward-cut lids, creeping inward and trembling |
| Sleepy | Curved lids slowly close as a floating z and tiny snore appear, then reopen |
| Surprised | Eyes stretch taller with an overshoot and a small open mouth |
| Worried | Taller uneven eyes quiver and glance around with a wobbly mouth |
| Confused | One taller eye and one smaller tilted eye change shape, with a tilted mouth and question mark |
| Excited | Wider eyes bounce and pulse with a curved grin and sparkles |
| Bored | Flat half-lidded eyes drift sideways with long blinks and a flat mouth |
| Scared | Small, wide-set eyes tremble, dart, pulse, and blink quickly |
| Furious | Strongly angled eyes shake with an anger mark and jagged mouth |

The face retains the current larger layout and narrow, rounded/cylindrical eye proportions. The same curved eye geometry is deformed for each expression. The face is drawn live; emotion animation does not rely on per-frame bitmaps.

## Buttons and Reactions

| Input | Normal Mode | Eye Mode |
|---|---|---|
| PREV | Previous content slot | Adds one click toward the random reaction threshold |
| NEXT | Next content slot | Adds one click toward the random reaction threshold |
| PREV + NEXT | Enter Eye Mode | Exit Eye Mode and resume the previous content slot |

The threshold is selected randomly from 1 through 5 using `random(1, 6)` when Eye Mode starts and after each threshold cycle. Presses during a reaction are ignored, while both-button exit remains available.

Happy and Sad retain their current firmware reaction messages. Reaction screens use bold black text on a white screen, type one character about every 70 ms, remain for about 1.6 seconds, and resume the same emotion.

## Empty Future Media Slots

The Normal Mode content list contains exactly five video placeholders and five image placeholders. No video frames or image bitmaps are included in this version; the media is intentionally left empty for future addition. Empty slots display a simple “not loaded” screen.

## Recommended Future Eye Reactions

**FUTURE / RECOMMENDED ONLY — documentation suggestions; none of these messages are implemented in firmware.**

| Emotion | Suggested messages |
|---|---|
| Angry | “Grr, I need a moment.” / “Please be gentle!” |
| Happy | “You made me smile!” / “I’m so glad you’re here!” |
| Sad | “Could I have a little comfort?” / “I miss you.” |
| Worried | “Is everything going to be okay?” / “I feel a little nervous.” |
| Bored | “Let’s do something fun!” / “Can we play?” |
| Sleepy | “Five more minutes…” / “Time for a little nap.” |

## Eye Design Reference

The visual reference and inspiration for the eye design is [OLED Animation Maker](https://www.oledanimationmaker.com/?s=DBdtgyEY6vSbQFSxpzuu). The project keeps its own implementation of the curved eye geometry and animations.

## Hardware

| Part | Connection | ESP32 |
|---|---|---|
| SSD1306 OLED 128×64 | VCC | 3V3 |
| SSD1306 OLED 128×64 | GND | GND |
| SSD1306 OLED 128×64 | SDA | GPIO 22 |
| SSD1306 OLED 128×64 | SCL | GPIO 21 |
| PREV button | One leg | GPIO 25 |
| PREV button | Other leg | GND |
| NEXT button | One leg | GPIO 26 |
| NEXT button | Other leg | GND |

OLED address is `0x3C`. Buttons use `INPUT_PULLUP`, so a pressed button reads LOW. I2C is initialized with `Wire.begin(22, 21)` (SDA, SCL).

## Build Requirements

- Arduino IDE and ESP32 Arduino core
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO

Open `v3.ino` with `EyeVariants.h` in the same sketch folder. The `esp32-eyes-main/` directory is reference material and is not compiled by this sketch.

## Project Files

- `v3.ino` — OLED setup, empty media slots, content navigation, buttons, Eye Mode, and reaction state machine
- `EyeVariants.h` — eye shape, 11 emotions, and animation timing
- `docs/eye-emotions-sheet.png` and `docs/eye-mode-preview.gif` — eye emotion previews
- `esp32-eyes-main/` — retained reference library

