# Autonomous Solar Tracker with DHT11 Climate Monitor 

An embedded IoT project built using an **Arduino Uno/Nano** architecture that tracks the sun's maximum light intensity dynamically while simultaneously monitoring local ambient temperature and humidity. 

The system features a custom non-blocking execution cycle, preventing hardware jitter and displaying clean diagnostic data on an I2C-driven 16x2 LCD layout.

---

## Hardware Requirements & Pinout

| Component | Pin (Arduino) | Description |
| :--- | :--- | :--- |
| **Servo Motor** | `D9` | Controls solar panel angular tracking |
| **DHT11 Sensor** | `A2` | Reads real-time environmental Temp & Humidity |
| **LDR Left** | `A1` | Light Dependent Resistor (Left Differential Input) |
| **LDR Right** | `A0` | Light Dependent Resistor (Right Differential Input) |
| **16x2 LCD (I2C)** | `A4 (SDA)`, `A5 (SCL)` | Diagnostic control UI terminal |

---

## Key Architectural Features

* **Non-Blocking Multithreading (`millis()` logic):** Replaced traditional disruptive `delay()` routines with absolute timestamp checkpoints. This keeps the application responsive, letting the servo adjust angle dynamically while the DHT11 reads strictly every 2000ms.
* **Dead-Band Jitter Prevention:** Implemented an dynamic threshold configuration `LDR_THRESHOLD = 15` to ensure the servo does not micro-stutter when the light intensities are near-equivalent.
* **Custom LCD Bitmaps:** Injected compiled byte-arrays directly into the HD44780 controller logic to render custom visual icons (Thermometer , Water Drop , and Degree Symbol °).
* **Fault-Tolerant Exception Handling:** Integrated `isnan()` state tracking. If a sensor wire disconnects, the LCD instantly captures the failure and alters the visual UI state safely instead of printing corrupt runtime registers.

---

## Step-by-Step Installation

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/UmarMujahid07/solar-tracker-dht11-display.git](https://github.com/UmarMujahid07/solar-tracker-dht11-display.git)
   cd solar-tracker-dht11-display