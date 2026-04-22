# ESP32 WiFi-Based Fog Detection Car

This repository contains the implementation of an **ESP32-based fog detection and WiFi-controlled robotic car**, developed as part of the **GE-107 course project** at IIT Ropar.

The system detects fog conditions using a **laser–LDR based visibility setup**, dynamically controls motor speed, and allows remote operation through a **WiFi-hosted web interface**.

---

## 📌 Project Overview

Fog significantly reduces visibility and poses safety risks in autonomous and remotely operated vehicles.  
This project demonstrates a **fog-aware robotic car** that:

- Detects fog density using light attenuation
- Adjusts motor speed based on visibility conditions
- Alerts users via buzzer and LCD
- Allows real-time remote control over WiFi

---

## ⚙️ System Features

- ESP32-based control and processing
- Laser + LDR based fog detection
- Ultrasonic sensor for obstacle distance estimation
- Dynamic speed regulation under fog conditions
- Buzzer alerts for reduced visibility
- LCD display for live system status
- WiFi-based control using a mobile-friendly web interface
- Safe shutdown handling when laser/system is turned off

---

## 🧱 Hardware Components

- **ESP32**
- **Laser Module**
- **LDR (Light Dependent Resistor)**
- **Ultrasonic Sensor (HC-SR04)**
- **Motor Driver**
- **DC Motors**
- **Buzzer**
- **16×2 I2C LCD**

---

## 🧠 Fog Detection Logic

Fog detection is based on **laser light attenuation**:

- High LDR value → Clear visibility
- Moderate drop → Light fog (speed limited)
- Severe drop → Dense fog (vehicle stopped)
- Very low reading → System/laser off (silent shutdown)

Thresholds are calibrated to ensure reliable detection and prevent false alerts.

---

## 🕹 WiFi Control Interface

The ESP32 hosts a **local WiFi access point** and web server:

- Mobile-friendly D-pad interface
- Supports Forward, Backward, Left, Right, and Stop commands
- Low-latency control using HTTP requests

No external internet connection is required.

---

## 🧪 Sensor Reliability Enhancements

- Ultrasonic distance readings are filtered to remove invalid spikes
- LDR readings are averaged to reduce noise
- Safe fallback logic prevents erratic motor behavior

---

## 🧰 Tech Stack

- Embedded C/C++
- ESP32 WiFi & WebServer
- PWM Motor Control
- Arduino Framework
- HTML, CSS, JavaScript (Web UI)

---

## 📚 Learning Outcomes

- Practical experience with ESP32 and embedded systems
- Sensor fusion and real-time decision making
- PWM-based motor control
- Designing fault-tolerant embedded logic
- Building WiFi-enabled hardware interfaces

---

## 👤 Author's: 

Moinak Goswami 2024CSB1225
Sarthak Nitin Keshaowar 2024CSB1215
Harshit Sahu 2024CSB1203
Palak Gulati 2024CSB1137
Garima Kamra 2024CSB1115
