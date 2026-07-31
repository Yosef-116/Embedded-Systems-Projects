# 🎛️ LED Control — Interactive LED Toggling

An LED whose state is controlled by user input — either via a physical push-button or serial monitor commands. This project introduces reading inputs and responding to them in real time.

---

## What It Does

- Reads a digital input (button) or serial command
- Toggles an LED on or off based on the input state
- Demonstrates debouncing logic for reliable button reads

---

## Hardware

| Component | Connection |
|---|---|
| LED | Digital output pin |
| Push button | Digital input pin with pull-up resistor |
| Resistor (220–330Ω) | In series with LED |

---

## Concepts Covered

- `digitalRead()` for input sensing
- Pull-up resistors and `INPUT_PULLUP` mode
- Button debouncing
- Conditional logic in embedded firmware