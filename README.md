<div align="center">

ClimaControl

IoT Automated Cooling Matrix

Feel The Future Of Temperature Control, Thinking Ahead Of Temperature.

<!-- Badges -->

<p align="center">
<img src="https://www.google.com/search?q=https://img.shields.io/badge/Hardware-ESP32-blue%3Fstyle%3Dfor-the-badge%26logo%3Despressif%26logoColor%3Dwhite" alt="Hardware: ESP32" />
<img src="https://www.google.com/search?q=https://img.shields.io/badge/Frontend-HTML5%2520|%20TailwindCSS-orange?style=for-the-badge&logo=html5&logoColor=white" alt="Frontend: HTML5 & Tailwind" />
<img src="https://www.google.com/search?q=https://img.shields.io/badge/Database-Firebase%2520RTDB-FFCA28%3Fstyle%3Dfor-the-badge%26logo%3Dfirebase%26logoColor%3Dwhite" alt="Database: Firebase RTDB" />
<img src="https://www.google.com/search?q=https://img.shields.io/badge/AI-Google%2520Gemini-8E75B2%3Fstyle%3Dfor-the-badge%26logo%3Dgoogle%26logoColor%3Dwhite" alt="AI: Google Gemini" />
</p>

</div>

🌌 Overview

ClimaControl is an advanced, AI-powered IoT temperature regulation system. Built on an ESP32 microcontroller, it gathers environmental telemetry via a DHT11 sensor and controls a high-speed cooling turbine using an L298N (HW-095) motor driver.

The physical hardware connects seamlessly to a stunning, futuristic Neo-Glass web dashboard via Firebase Realtime Database. The system goes beyond standard automation by integrating a Gemini AI Neural Link that actively analyzes thermal conditions to calculate the exact optimal fan PWM speed.

✨ Features

🧠 Gemini Neural Link (AI Mode): Leverages Google's Gemini 2.5 Flash API to analyze live telemetry and dynamically generate optimal cooling speeds.

🎛️ Neural Directives: Take manual command with intuitive overrides (OFF, AUTO, LOW, MID, HIGH, AI). Each state features unique neon-glow feedback.

🎨 Neo-Glassmorphism UI: A highly immersive aesthetic blending 75% Glassmorphism (frosted blurs) and 25% Neumorphism (soft extruded shadows).

🚨 Critical Overheat Alarms: A built-in Web Audio API synthesizer generates a pulsating warning siren and visual modal if the core exceeds 32°C.

🌗 Adaptive Theme Engine: Seamlessly toggle between deep Dark Mode and clean Light Mode.

📈 Live Telemetry: Real-time data streaming visualized through custom-styled, neon Chart.js graphs.

🛡️ Security Overlay: Integrated anti-inspect and anti-copy roadblocks to protect the dashboard's source code.

🧰 Hardware Architecture

To build the physical Thermal Core, you will need:

Microcontroller: ESP32 Development Board

Sensor: DHT11 Temperature & Humidity Sensor

Motor Driver: HW-095 (L298N Dual Motor Driver)

Cooling Unit: 130-Size DC Motor (3V-6V) + Fan Blade

Display: 16x2 LCD with HW-61 I2C Backpack

Indicators: Red/Green LEDs & Active Buzzer

Power: 6V External Battery Pack

🔌 Connection Diagram

Component

Pin / Wire

ESP32 Pin

Note

DHT11

DATA

GPIO 4

Needs 10k pull-up resistor

HW-095 (L298N)

ENA (PWM)

GPIO 16

Controls speed

HW-095 (L298N)

IN1 (Power)

GPIO 17

Controls On/Off

HW-095 (L298N)

IN2 (GND)

GPIO 5

Ground logic for forward spin

LCD (I2C)

SDA / SCL

GPIO 21 / GPIO 22

Must be powered by 5V (VIN)

LEDs & Buzzer

Green / Red / Buzzer

18 / 19 / 23

Active HIGH triggers

⚠️ CRITICAL: The ESP32 GND, the 6V Battery (-), and the HW-095 GND MUST all be connected together to share a common ground.

🚀 Getting Started

1. Firebase Configuration

Create a Firebase project and deploy a Realtime Database.

Enable Anonymous Authentication in the Auth tab.

Apply the following Database Rules for production security:

{
  "rules": {
    ".read": "auth != null",
    ".write": "auth != null"
  }
}


Copy your Firebase Config object.

2. ESP32 Deployment

Open ESP32_Thermal_Monitor.ino in the Arduino IDE.

Install dependencies: DHT sensor library, LiquidCrystal I2C, and Firebase ESP Client.

Update the WIFI_SSID, WIFI_PASSWORD, FIREBASE_HOST, and API_KEY macros.

Upload to your ESP32. Listen for the 1-second diagnostic boot beep!

3. WebApp Deployment

Open index.html.

Replace the firebaseConfig object with your own credentials.

Insert your Google AI Studio API key into GEMINI_API_KEY.

Host the file via GitHub Pages, Firebase Hosting, or simply open it locally in your browser.

📸 Interface Preview

<div align="center">

[ Your Dashboard Screenshot Goes Here ]

Engineered for optimal thermals. Designed for the future.

</div>
