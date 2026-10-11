"""Generate docs/hardware/mochi-wiring.svg, the Mochi wiring diagram.

    python docs/hardware/mochi_wiring.py

Every pin here comes from MochiCompanion/MochiCompanion.ino:
    BTN_1 25, BTN_2 26 (INPUT_PULLUP, pressed = LOW, "to GND")
    OLED_SDA 22, OLED_SCL 21 (Wire.begin(OLED_SDA, OLED_SCL)), OLED_ADDR 0x3C, no reset pin (-1)
The OLED supply (VCC -> 3V3, GND -> GND) is not visible in code; it comes from the project's
hardware docs and is drawn dashed. Change the firmware first, then this file, never the reverse.

PNG export (same folder), e.g. with Microsoft Edge:
    msedge --headless --disable-gpu --force-device-scale-factor=2 --window-size=1000,910
           --screenshot=mochi-wiring.png mochi-wiring.svg
"""
import os

W, H = 1000, 910
FONT = "Segoe UI, Helvetica, Arial, sans-serif"
C = {  # wire colours (also the legend)
    "3V3": "#d62828", "GND": "#222222", "SDA": "#1f6feb", "SCL": "#d98a00",
    "BTN1": "#2a9d43", "BTN2": "#8a3ffc",
}
INK, MUTED, BOX, BOARD = "#1d232b", "#5b6573", "#f4f6f9", "#e8eef6"

# rows (y) of the ESP32 pins; the OLED pins sit on the same rows, so wires run straight
Y = {"3V3": 190, "GND_A": 235, "SDA": 280, "SCL": 325, "BTN1": 470, "BTN2": 550, "GND_B": 630}
ESP_X0, ESP_X1, PIN_X = 60, 330, 350          # board edges, end of the pin stubs
OLED_X0, OLED_X1, OLED_Y0, OLED_Y1 = 640, 940, 150, 370

out = []
def add(s): out.append(s)
def text(x, y, s, size=16, anchor="start", weight="normal", fill=INK, style=""):
    add(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" fill="{fill}" {style}>{s}</text>')
def wire(points, color, dashed=False):
    d = " ".join(f"{x},{y}" for x, y in points)
    dash = ' stroke-dasharray="10 7"' if dashed else ""
    add(f'<polyline points="{d}" fill="none" stroke="{color}" stroke-width="4" stroke-linecap="round" stroke-linejoin="round"{dash}/>')
def dot(x, y, color): add(f'<circle cx="{x}" cy="{y}" r="6" fill="{color}"/>')
def pill(x, y, s, color):
    w = 10 * len(s) + 22
    add(f'<rect x="{x - w / 2}" y="{y - 14}" width="{w}" height="28" rx="14" fill="#ffffff" stroke="{color}" stroke-width="2.5"/>')
    text(x, y + 6, s, 15, "middle", "bold", color)
def pin_stub(x0, x1, y): add(f'<line x1="{x0}" y1="{y}" x2="{x1}" y2="{y}" stroke="#9aa4b2" stroke-width="6" stroke-linecap="round"/>')

add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="{FONT}">')
add(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
text(W / 2, 52, "Mochi OLED Companion — Wiring Diagram", 28, "middle", "bold")
text(W / 2, 84, "ESP32 Dev Module · SSD1306 128×64 OLED over I2C (address 0x3C) · two push buttons", 16, "middle", fill=MUTED)

# ---- ESP32 ----
add(f'<rect x="{ESP_X0}" y="120" width="{ESP_X1 - ESP_X0}" height="565" rx="16" fill="{BOARD}" stroke="#7d8ea3" stroke-width="2.5"/>')
text(ESP_X0 + 20, 155, "ESP32 Dev Module", 20, weight="bold")
for key, label in [("3V3", "3V3"), ("GND_A", "GND"), ("SDA", "GPIO 22 · SDA"), ("SCL", "GPIO 21 · SCL"),
                   ("BTN1", "GPIO 25 · Button 1"), ("BTN2", "GPIO 26 · Button 2"), ("GND_B", "GND")]:
    pin_stub(ESP_X1, PIN_X, Y[key]); text(ESP_X1 - 14, Y[key] + 6, label, 17, "end", "bold")
# internal pull-ups on the button inputs
add(f'<rect x="{ESP_X0 + 18}" y="{Y["BTN1"] - 28}" width="{ESP_X1 - ESP_X0 - 30}" height="{Y["BTN2"] - Y["BTN1"] + 70}" rx="10" fill="none" stroke="#7d8ea3" stroke-width="1.5" stroke-dasharray="5 5"/>')
text(ESP_X0 + 32, Y["BTN2"] + 32, "internal pull-ups (INPUT_PULLUP)", 13, fill=MUTED)
text(ESP_X0 + 20, 370, "I2C: Wire.begin(22, 21)", 14, fill=MUTED)
text(ESP_X0 + 20, 390, "SDA/SCL swapped vs. ESP32 default", 13, fill=MUTED)
# USB (power + upload) - not verifiable from the firmware
add(f'<rect x="{(ESP_X0 + ESP_X1) / 2 - 34}" y="671" width="68" height="28" rx="5" fill="#c9d2dd" stroke="#7d8ea3" stroke-width="2"/>')
text((ESP_X0 + ESP_X1) / 2, 690, "USB", 14, "middle", "bold")
text((ESP_X0 + ESP_X1) / 2, 723, "power + upload (assumed, not verified)", 13, "middle", fill=MUTED)

# ---- OLED module ----
add(f'<rect x="{OLED_X0}" y="{OLED_Y0}" width="{OLED_X1 - OLED_X0}" height="{OLED_Y1 - OLED_Y0}" rx="14" fill="#123a73" stroke="#0b2a55" stroke-width="2.5"/>')
for key, label in [("3V3", "VCC"), ("GND_A", "GND"), ("SDA", "SDA"), ("SCL", "SCL")]:
    pin_stub(OLED_X0 - 20, OLED_X0, Y[key]); text(OLED_X0 + 14, Y[key] + 6, label, 17, weight="bold", fill="#ffffff")
add(f'<rect x="725" y="178" width="196" height="118" rx="6" fill="#05080d" stroke="#3d5f8f" stroke-width="2"/>')
add('<rect x="778" y="208" width="30" height="46" rx="11" fill="#cfe8ff"/><rect x="838" y="208" width="30" height="46" rx="11" fill="#cfe8ff"/>')
add('<path d="M813,262 q10,9 20,0" stroke="#cfe8ff" stroke-width="4" fill="none" stroke-linecap="round"/>')
text(823, 324, "SSD1306 OLED 128×64", 17, "middle", "bold", "#ffffff")
text(823, 348, "I2C · 0x3C · no reset pin", 14, "middle", fill="#c3d4ea")

# ---- wires: ESP32 -> OLED (straight, same rows) ----
wire([(PIN_X, Y["3V3"]), (OLED_X0 - 20, Y["3V3"])], C["3V3"], dashed=True)
wire([(PIN_X, Y["GND_A"]), (OLED_X0 - 20, Y["GND_A"])], C["GND"], dashed=True)
wire([(PIN_X, Y["SDA"]), (OLED_X0 - 20, Y["SDA"])], C["SDA"])
wire([(PIN_X, Y["SCL"]), (OLED_X0 - 20, Y["SCL"])], C["SCL"])
for key, s in [("3V3", "3V3 → VCC"), ("GND_A", "GND → GND"), ("SDA", "SDA"), ("SCL", "SCL")]:
    pill(495, Y[key], s, C["GND"] if key == "GND_A" else C[key])

# ---- buttons: GPIO -> button -> common GND bus (staggered so no wire crosses) ----
def button(x0, y, name, color):
    x1 = x0 + 90
    wire([(PIN_X, y), (x0, y)], color)
    dot(x0, y, color); dot(x1, y, C["GND"])
    add(f'<line x1="{x0}" y1="{y}" x2="{x1 - 8}" y2="{y - 24}" stroke="{INK}" stroke-width="4" stroke-linecap="round"/>')
    add(f'<line x1="{x0 + 45}" y1="{y - 14}" x2="{x0 + 45}" y2="{y - 40}" stroke="{INK}" stroke-width="3"/>')
    add(f'<rect x="{x0 + 25}" y="{y - 50}" width="40" height="10" rx="3" fill="{INK}"/>')
    text(x0 + 45, y - 62, name, 17, "middle", "bold")
    return x1
b1 = button(770, Y["BTN1"], "Button 1", C["BTN1"])
b2 = button(570, Y["BTN2"], "Button 2", C["BTN2"])
text(b1 - 45, Y["BTN1"] + 30, "momentary, pressed = LOW", 13, "middle", fill=MUTED)
pill(470, Y["BTN1"], "Button 1", C["BTN1"])
pill(460, Y["BTN2"], "Button 2", C["BTN2"])
wire([(b1, Y["BTN1"]), (b1 + 40, Y["BTN1"]), (b1 + 40, Y["GND_B"]), (PIN_X, Y["GND_B"])], C["GND"])
wire([(b2, Y["BTN2"]), (b2, Y["GND_B"])], C["GND"])
dot(b2, Y["GND_B"], C["GND"])
pill(470, Y["GND_B"], "common GND", C["GND"])

# ---- legend ----
LY = 760
add(f'<rect x="40" y="{LY}" width="{W - 80}" height="128" rx="12" fill="{BOX}" stroke="#d5dbe3"/>')
text(60, LY + 30, "Legend", 16, weight="bold")
items = [("3V3", "3V3 power"), ("GND", "GND"), ("SDA", "I2C SDA"), ("SCL", "I2C SCL"), ("BTN1", "Button 1"), ("BTN2", "Button 2")]
for i, (k, s) in enumerate(items):
    x = 150 + i * 138
    add(f'<line x1="{x}" y1="{LY + 25}" x2="{x + 36}" y2="{LY + 25}" stroke="{C[k]}" stroke-width="5" stroke-linecap="round"/>')
    text(x + 44, LY + 30, s, 14)
add(f'<line x1="60" y1="{LY + 62}" x2="96" y2="{LY + 62}" stroke="{INK}" stroke-width="4"/>')
text(104, LY + 67, "solid: pin taken from the firmware (MochiCompanion.ino)", 14)
add(f'<line x1="530" y1="{LY + 62}" x2="566" y2="{LY + 62}" stroke="{INK}" stroke-width="4" stroke-dasharray="10 7"/>')
text(574, LY + 67, "dashed: from project docs, not checkable in code", 14)
text(60, LY + 100, "Not verified: OLED supply voltage (3V3 per project docs) · ESP32 power source · OLED header pin order", 14, fill=MUTED)
text(60, LY + 118, "(varies by module: wire by pin name) · I2C pull-up resistors on the OLED module (common, not confirmed)", 14, fill=MUTED)

add("</svg>")
path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "mochi-wiring.svg")
with open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(out) + "\n")
print("wrote", path)
