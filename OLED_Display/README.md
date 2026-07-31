# 🖥️ OLED Display — Text & Graphics on SSD1306

Drives a 128×64 monochrome OLED display over I²C using the Adafruit SSD1306 library. Renders text at different sizes, draws geometric shapes, and demonstrates how to build a simple UI on a small embedded display.

---

## What It Does

- Initializes the SSD1306 OLED over the I²C bus (address `0x3C`)
- Prints text strings at various font sizes
- Draws lines, rectangles, circles, and other GFX primitives
- Demonstrates display clearing and refresh cycles

---

## Hardware

| Component | Connection |
|---|---|
| SSD1306 OLED (128×64) | SDA → A4, SCL → A5 (Uno) |
| VCC | 3.3V or 5V |
| GND | GND |

---

## Libraries Required

Install via Arduino Library Manager:
- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `Wire` (built-in)

---

## Concepts Covered

- I²C bus communication protocol
- Display buffer and `display.display()` flush model
- Font scaling and pixel-coordinate graphics
- Adafruit GFX API