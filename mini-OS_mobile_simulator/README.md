# 📱 mini-OS Mobile Simulator

A simulated mobile operating system UI running entirely on a **128×64 SSD1306 OLED** and navigated with physical buttons. The firmware implements a multi-screen menu system with app icons, selection highlighting, and screen transitions — mimicking the feel of a basic mobile phone interface.

---

## What It Does

- Renders a home screen with a grid of app icons on the OLED
- Navigates between items using **Up / Down / Select** buttons
- Opens "apps" (sub-menus or info screens) on selection
- Supports back-navigation to return to the home screen
- Simulates a basic task/screen manager in firmware — no OS, no RTOS, pure C++

---

## Hardware

| Component | Connection |
|---|---|
| SSD1306 OLED (128×64) | SDA → A4, SCL → A5 |
| Navigation button (Up) | Digital input pin |
| Navigation button (Down) | Digital input pin |
| Select / OK button | Digital input pin |
| Back button | Digital input pin |

All buttons use internal pull-up resistors (`INPUT_PULLUP`).

---

## UI Structure

```
Home Screen
├── App 1 (e.g. Clock)
├── App 2 (e.g. Settings)
├── App 3 (e.g. Info)
└── App 4 (e.g. Games)
```

Each app is a separate screen rendered from its own draw function, with state managed by a screen index variable.

---

## Libraries Required

- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `Wire` (built-in)

---

## Concepts Covered

- Finite State Machine (FSM) for UI screen management
- Custom icon rendering with `drawBitmap()`
- Button debouncing and input event handling
- Menu navigation and selection highlighting
- Structuring complex firmware into modular screen handlers