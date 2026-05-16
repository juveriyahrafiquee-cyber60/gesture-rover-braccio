# 🤖 Hand Gesture Controlled Rover with Braccio Robotic Arm

A wireless, gesture-controlled robotic system built as a team project. Tilt your hand to drive the rover in any direction and control a mounted Braccio robotic arm remotely via an IoT dashboard, all without touching the robot.

---

## 🎯 What It Does

- **Drive the rover** using hand tilt gestures (forward, backward, left, right)
- **Control the Braccio arm** remotely via the VandeIoT dashboard to pick and place objects
- **Fully wireless** — no physical connection between the glove and the robot

---

## 🧠 How It Works

The system has two independent control channels:

### 1. Gesture → Rover (RF Channel)
```
Hand Glove (MPU6050 + NodeMCU)
        ↓  tilt detected
  NRF24L01+ (transmitter)
        ↓  wireless RF @ 250Kbps, Channel 108
  NRF24L01+ (receiver on Pico)
        ↓  decoded packet
  Raspberry Pi Pico
        ↓  PWM signals
  2x L298N Motor Drivers → 4x BO Motors
```

### 2. IoT Dashboard → Braccio Arm (WiFi Channel)
```
VandeIoT Dashboard (button press)
        ↓  HTTPS API
  Arduino Uno R4 WiFi (polls every 5s)
        ↓  servo commands
  Braccio Shield → 6-DOF Robotic Arm
```

---

## 🛠️ Hardware

| Component | Role |
|---|---|
| ESP8266 NodeMCU | Reads glove MPU6050, transmits gestures via NRF24L01+ |
| Raspberry Pi Pico | Receives RF packets, drives motors via L298N |
| Arduino Uno R4 WiFi | Connects to VandeIoT, controls Braccio arm |
| MPU-6050 | Gyroscope/accelerometer on the hand glove |
| NRF24L01+ (×2) | Wireless RF communication between glove and rover |
| 2× L298N Motor Driver | Controls 4 BO motors (tank-style differential steering) |
| Braccio Robotic Arm | 6-DOF arm for pick and place operations |

---

## 📁 File Structure

```
├── glove_nodemcu_nrf24L01.ino   # NodeMCU: reads MPU6050, transmits gestures
├── car_pico_receiver.py          # Raspberry Pi Pico: receives RF, drives motors
├── nrf24l01.py                   # NRF24L01 MicroPython driver (for Pico)
└── braccio_arduino_uno_r4.ino   # Arduino Uno R4: polls VandeIoT, controls Braccio
```

---

## ⚙️ Gesture Mapping

The MPU-6050 measures pitch and roll angles from the glove:

| Hand Gesture | Rover Action |
|---|---|
| Tilt forward | Move forward |
| Tilt backward | Move backward |
| Tilt left | Turn left (differential steering) |
| Tilt right | Turn right (differential steering) |
| Flat/neutral | Stop |

> A **dead zone** of ±4° filters out accidental micro-movements.  
> Speed scales proportionally with tilt angle (up to 25° = full speed).  
> A **500ms safety timeout** stops the rover automatically if the RF signal is lost.

---

## 🦾 Braccio Arm Control

The Braccio arm is controlled independently via the **VandeIoT IoT platform**:

- The Arduino Uno R4 WiFi polls the VandeIoT API every 5 seconds
- Button presses on the dashboard trigger pre-programmed arm movements
- **Action 1**: Pick sequence (arm extends, grips object)
- **Action 2**: Place sequence (arm repositions, releases object)
- Only executes when the command **changes** — avoids repeating the same action

---

## 📡 RF Communication Protocol

Each packet is **5 bytes**:

```
[ dir (1 byte) | speed (2 bytes int16) | steer (2 bytes int16) ]
```

- `dir`: ASCII character — `'F'` (forward), `'B'` (backward), `'S'` (stop)
- `speed`: 0–255 PWM value
- `steer`: -255 (full left) to +255 (full right)

**RF Settings:** Channel 108, 250Kbps, Max Power, Address `"GCAR1"`

---

## 🔧 Dependencies

### NodeMCU (Arduino IDE)
- `RF24` library
- `Wire` library (built-in)

### Raspberry Pi Pico (MicroPython)
- `nrf24l01.py` — included in this repo  
  *(Source: [micropython-lib](https://github.com/micropython/micropython-lib))*

### Arduino Uno R4 WiFi (Arduino IDE)
- `WiFiS3` library
- `ArduinoHttpClient` library
- `Braccio` library
- `Servo` library (built-in)

---

## 🚀 Setup

1. **Glove side**: Flash `glove_nodemcu_nrf24L01.ino` to NodeMCU. The MPU-6050 calibrates automatically on startup — keep the glove flat and still for the first 2 seconds.

2. **Rover side**: Copy `car_pico_receiver.py` and `nrf24l01.py` to the Raspberry Pi Pico via Thonny. The onboard LED blinks 3 times when ready.

3. **Braccio side**: Flash `braccio_arduino_uno_r4.ino` to Arduino Uno R4 WiFi. Update the WiFi credentials and VandeIoT API key in the file before uploading.

4. Power up the rover first, then the glove. The rover will stop automatically if the glove goes out of range.

---

## 👥 Team Project

Built as a collaborative team project integrating embedded systems, wireless communication, IoT, and robotics.

---

## 📄 License

MIT License
