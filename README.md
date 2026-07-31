# 🔧 Embedded Systems & Firmware Engineering

A collection of embedded systems and firmware projects written in C/C++ for Arduino-compatible microcontrollers, developed during my **Embedded Systems and Firmware Engineering** internship at **INSA (Information Network Security Administration)**.

Projects range from fundamental GPIO exercises to more advanced peripherals like OLED displays, servo-controlled security systems, PWM dimming, and a simulated mini operating system — all built with register-level awareness and hardware-close C code.

---

## Projects

| Folder | Description |
|---|---|
| [`Led`](./Led/) | Basic LED on/off — the embedded "Hello, World" |
| [`Blinker`](./Blinker/) | LED blinking with `delay()` — timing fundamentals |
| [`Double_Blink`](./Double_Blink/) | Two LEDs blinking at independent rates |
| [`LED_Control`](./LED_Control/) | Button or serial-controlled LED toggling |
| [`Smooth_Dimmer`](./Smooth_Dimmer/) | PWM-based LED brightness fade in/out |
| [`OLED_Display`](./OLED_Display/) | Text and graphics on a 128×64 SSD1306 OLED |
| [`OLED_img`](./OLED_img/) | Bitmap image rendering on SSD1306 OLED |
| [`Digital_Clock`](./Digital_Clock/) | Real-time clock display using DS3231 + OLED |
| [`Security_Gate`](./Security_Gate/) | Keypad-access servo gate with LCD feedback |
| [`mini-OS Mobile Simulator`](./mini-OS%20Mobile%20Simulator/) | Simulated mobile OS UI with menu navigation on OLED |

---

## Tech Stack

- **Language:** C / C++ (Arduino framework)
- **IDE:** Arduino IDE
- **Microcontroller:** Arduino Uno / Nano (ATmega328P)
- **Key Peripherals:** SSD1306 OLED, DS3231 RTC, SG90 Servo, 4×4 Keypad, LCD1602, LEDs

---

## Libraries Used

| Library | Purpose |
|---|---|
| `Adafruit_SSD1306` | OLED display driver |
| `Adafruit_GFX` | Graphics rendering primitives |
| `Wire` | I²C communication |
| `RTClib` | DS3231 real-time clock |
| `Keypad` | Matrix keypad scanning |
| `Servo` | Servo motor control |
| `LiquidCrystal` | LCD1602 character display |

---

## Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/Yosef-116/Embedded-Systems-and-Firmware-Engineering-.git
   ```
2. Open any project's `.ino` file in the **Arduino IDE**.
3. Install the required libraries via `Sketch → Include Library → Manage Libraries`.
4. Select your board (`Arduino Uno` or `Nano`) and COM port under `Tools`.
5. Click **Upload**.

---

## Author

**Yosef Hassen** — [@Yosef-116](https://github.com/Yosef-116) · [LinkedIn](https://linkedin.com/in/yosefchaka)