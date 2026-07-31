# 💡 Led — Basic LED Control

The embedded systems equivalent of "Hello, World." A single LED is connected to a digital output pin and turned on by writing `HIGH` to it. No blinking, no delay — just confirming the toolchain, wiring, and pin configuration all work correctly.

---

## What It Does

- Sets a digital pin as `OUTPUT`
- Writes `HIGH` to turn the LED on
- Demonstrates the minimal structure of an Arduino sketch (`setup()` + `loop()`)

---

## Hardware

| Component | Connection |
|---|---|
| LED | Digital pin (e.g. D13 or D8) |
| Resistor (220–330Ω) | In series with LED to GND |

---

## Concepts Covered

- `pinMode()` and `digitalWrite()`
- GPIO output configuration
- Current-limiting resistor sizing for LEDs