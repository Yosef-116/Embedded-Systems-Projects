# 🔐 Security Gate — Keypad Access Control with Servo

A PIN-protected access control system. The user enters a password on a **4×4 matrix keypad**; if the code matches, a **servo motor** opens a gate and an **LCD** shows an access-granted message. Wrong codes are rejected with an error message.

---

## What It Does

- Scans a 4×4 matrix keypad for key presses
- Compares the entered PIN against a stored password
- On correct PIN: rotates a servo to "open" position and shows `ACCESS GRANTED` on LCD
- On wrong PIN: displays `ACCESS DENIED` and resets input
- Supports `*` to clear the current entry and `#` to confirm

---

## Hardware

| Component | Connection |
|---|---|
| 4×4 Matrix Keypad | Digital pins D0–D7 (row/col) |
| SG90 Servo Motor | PWM pin (D9) |
| LCD1602 (with I²C backpack) | SDA → A4, SCL → A5 |

---

## Libraries Required

- `Keypad` by Mark Stanley & Alexander Brevig
- `Servo` (built-in)
- `LiquidCrystal_I2C` (if using I²C LCD backpack)

---

## Default PIN

The default access PIN is defined as a constant in the sketch — change it before deploying:
```cpp
const String correctPassword = "1234";
```

---

## Concepts Covered

- Matrix keypad scanning and key mapping
- String comparison and input buffering
- Servo angle control via PWM
- LCD feedback for user interaction
- Simple finite state machine (idle → entering PIN → granted/denied → idle)