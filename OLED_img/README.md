# 🖼️ OLED Image — Bitmap Rendering on SSD1306

Renders a custom bitmap image on the 128×64 SSD1306 OLED display. This project covers converting an image into a C byte array and drawing it to the display buffer using `drawBitmap()`.

---

## What It Does

- Loads a monochrome bitmap stored as a `PROGMEM` byte array
- Draws the full image (or a sprite) to the OLED using `drawBitmap()`
- Can animate or scroll the image across the screen

---

## Hardware

| Component | Connection |
|---|---|
| SSD1306 OLED (128×64) | SDA → A4, SCL → A5 (Uno) |
| VCC | 3.3V or 5V |
| GND | GND |

---

## How to Convert an Image

1. Resize your image to **128×64 pixels** (or smaller for sprites), black & white
2. Use [image2cpp](https://javl.github.io/image2cpp/) to convert it to a C byte array
3. Paste the array into your sketch as `const uint8_t PROGMEM myBitmap[] = { ... };`
4. Call `display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE);`

---

## Libraries Required

- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `Wire` (built-in)

---

## Concepts Covered

- `PROGMEM` for storing data in flash instead of RAM
- Bitmap encoding (row-major, MSB-first byte order)
- `drawBitmap()` API
- Memory constraints on 8-bit AVR microcontrollers