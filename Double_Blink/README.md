# 🔆 Double Blink — Two Independent LEDs

Two LEDs blink simultaneously or at different rates, introducing the concept of managing multiple outputs and timing without overlapping delays. This project is a stepping stone toward non-blocking firmware patterns.

---

## What It Does

- Controls two LEDs on separate digital pins
- Each LED blinks at its own interval
- Explores how `delay()` affects both channels and motivates the need for `millis()`-based timing

---

## Hardware

| Component | Connection |
|---|---|
| LED 1 | Digital pin (e.g. D8) |
| LED 2 | Digital pin (e.g. D9) |
| Resistor ×2 (220–330Ω) | In series with each LED to GND |

---

## Concepts Covered

- Multi-output GPIO management
- How blocking `delay()` limits simultaneous timing
- Foundation for `millis()`-based non-blocking blink patterns