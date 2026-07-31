# 🔦 Blinker — LED Blink

A single LED blinks on and off at a fixed interval using `delay()`. This is the classic first firmware exercise for learning timing, program flow, and the Arduino `setup()`/`loop()` structure.

---

## What It Does

- Turns an LED on, waits, turns it off, waits — indefinitely
- Demonstrates blocking delay-based timing

---

## Hardware

| Component | Connection |
|---|---|
| LED | Digital pin (e.g. D13) |
| Resistor (220–330Ω) | In series with LED to GND |

---

## Concepts Covered

- `delay()` for blocking time control
- Digital output write cycle
- The Arduino execution model (`setup()` runs once, `loop()` runs forever)