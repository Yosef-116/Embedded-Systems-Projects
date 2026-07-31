# 🌅 Smooth Dimmer — PWM LED Fade

An LED smoothly fades in and out using Pulse Width Modulation (PWM). This project introduces analog-like output control from a digital microcontroller pin, a fundamental technique for motor speed control, LED brightness, and audio signal generation.

---

## What It Does

- Uses `analogWrite()` to drive a PWM signal on a timer-capable pin
- Gradually increases brightness from 0 → 255, then fades back to 0
- Creates a smooth, continuous breathing/pulsing effect

---

## Hardware

| Component | Connection |
|---|---|
| LED | PWM-capable pin (D3, D5, D6, D9, D10, or D11 on Uno) |
| Resistor (220–330Ω) | In series with LED to GND |

---

## Concepts Covered

- PWM (Pulse Width Modulation) and duty cycle
- `analogWrite()` vs `digitalWrite()`
- Hardware timer usage via the Arduino framework
- Smooth animation loops with step-based increment/decrement