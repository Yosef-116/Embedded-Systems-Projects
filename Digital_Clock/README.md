# ⏰ Digital Clock — RTC + OLED Display

A real-time digital clock that reads time from a **DS3231 RTC module** and renders it live on a **128×64 SSD1306 OLED display**. The DS3231 keeps accurate time independently of the microcontroller, retaining time even when power is removed (with a backup coin cell).

---

## What It Does

- Reads current hours, minutes, and seconds from the DS3231 over I²C
- Formats and displays the time as `HH:MM:SS` on the OLED
- Optionally displays the date (day, month, year)
- Auto-sets the RTC to compile time on first upload if power was lost

---

## Hardware

| Component | Connection |
|---|---|
| SSD1306 OLED (128×64) | SDA → A4, SCL → A5 |
| DS3231 RTC Module | SDA → A4, SCL → A5 (shared I²C bus) |
| CR2032 coin cell | Backup power for RTC |

> Both the OLED and RTC share the same I²C bus — they have different addresses (`0x3C` for OLED, `0x68` for DS3231).

---

## Libraries Required

- `RTClib` by Adafruit
- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `Wire` (built-in)

---

## Concepts Covered

- Multi-device I²C bus management
- RTC time-keeping and `DateTime` object handling
- Formatted string rendering on OLED
- Persistent timekeeping independent of MCU power state