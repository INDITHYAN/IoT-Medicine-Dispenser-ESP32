@ -0,0 +1,53 @@
# 💊 IoT-Based Smart Medicine Dispenser with Alert System

## 🚀 Overview
An intelligent medicine dispensing system using ESP32 that reminds users to take medication, detects intake using sensors, and alerts caregivers via IoT communication (Telegram + Twilio).

## 🔥 Features
- Automatic medicine dispensing using servo
- Countdown timer for dosage
- Ultrasonic-based hand detection
- Buzzer alerts
- Telegram notifications
- Emergency phone call (Twilio API)
- LCD display interface

## 🧠 Working
1. System runs a countdown timer
2. When time is up:
   - Dispenses medicine using servo
   - Alerts user via buzzer and LCD
3. Detects if medicine is taken using ultrasonic sensor
4. If NOT taken:
   - Sends Telegram alert
   - Makes emergency phone call

## 🛠️ Components Used
- ESP32
- Servo Motor
- Ultrasonic Sensor (HC-SR04)
- LCD (I2C)
- Buzzer
- WiFi module
- Twilio API
- Telegram Bot API

## 🔐 Security Note
API keys and tokens have been removed for safety. Replace them with your own credentials.

## 👨‍💻 My Contribution
- Implemented Telegram alert system
- Integrated Twilio calling API
- Developed hand detection logic
- Contributed to system logic and debugging

## 📂 Code Structure
- `main.ino` → Main program

## 📸 Output
(Add images/videos here)

## 🔮 Future Improvements
- Mobile app integration
- Voice assistant support
- AI-based reminder system
