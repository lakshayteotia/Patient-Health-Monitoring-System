# Patient Health Monitoring System

An IoT-based patient health monitoring prototype using an ESP32, health sensors, MQTT communication, EMQX Cloud, and a live web dashboard.

## Live Dashboard

[Open the Wearable Health Monitor](https://lakshayteotia.github.io/Patient-Health-Monitoring-System/)

## Project Overview

This project collects sensor readings using an ESP32 and publishes telemetry to an EMQX Cloud MQTT broker. A browser-based dashboard subscribes to the telemetry topic and displays incoming readings.

The system is intended for educational and prototype development purposes. It is not a certified medical device.

## Hardware Components

- ESP32-WROOM-32 development board
- MAX30102 pulse oximeter and heart-rate sensor
- DHT11 temperature and humidity sensor
- HW-827 pulse sensor module
- Jumper wires and breadboard
- USB power and programming cable

## Sensor Connections

| Component | ESP32 connection |
|---|---|
| DHT11 data | GPIO 4 |
| MAX30102 SDA | GPIO 21 |
| MAX30102 SCL | GPIO 22 |
| HW-827 analog output | GPIO 34 |
| Sensor power and ground | According to module specifications |

Use the appropriate voltage levels for each sensor module.

## Software and Technologies

- Arduino IDE
- ESP32 Arduino Core
- C++ firmware
- MQTT communication using PubSubClient
- EMQX Cloud
- MQTT over TLS for ESP32 communication
- MQTT over Secure WebSockets for the browser dashboard
- HTML, CSS, and JavaScript

## MQTT Configuration

The ESP32 publishes telemetry to:

`health/esp32/device001/telemetry`

Broker configuration:

| Setting | Value |
|---|---|
| Broker | `h4d22089.ala.asia-southeast1.emqxsl.com` |
| ESP32 MQTT port | `8883` |
| Dashboard WebSocket port | `8084` |
| WebSocket path | `/mqtt` |
| Protocol | MQTT over TLS / Secure WebSockets |

The firmware uses the MQTT publisher account. The dashboard uses a separate subscriber account.

Do not publish passwords, private keys, or other credentials in this repository.

## Telemetry Data

The dashboard is designed to process fields including:

- `deviceId`
- `temperature`
- `humidity`
- `heartRate`
- `spo2`
- `hw827HeartRate`
- `fingerDetected`

Unavailable or invalid SpO₂ readings should be represented as `null`.

## Firmware Setup

1. Install Arduino IDE.
2. Install ESP32 board support.
3. Select **ESP32 Dev Module**.
4. Install the required sensor and MQTT libraries used by the firmware.
5. Open `patient_health_monitoring_emqx.ino`.
6. Enter your Wi-Fi and MQTT credentials locally in the firmware.
7. Verify the EMQX broker settings and certificate configuration.
8. Compile and upload the sketch to the ESP32.

Keep your actual credentials private. Do not commit them to GitHub.

## Dashboard Setup

The dashboard is hosted using GitHub Pages.

To run it locally, open `index.html` in a browser, select the live MQTT source, and authenticate using the dashboard subscriber account.

The dashboard connects through EMQX Secure WebSockets and subscribes to the telemetry topic.

## Testing

MQTT messages can be tested using MQTTX before connecting the physical ESP32.

Example test payload:

```json
{
  "deviceId": "device001",
  "temperature": 25.0,
  "humidity": 65.0,
  "heartRate": 75,
  "spo2": 98,
  "hw827HeartRate": 76,
  "fingerDetected": true
}
```

This example contains simulated values and does not represent actual sensor measurements.

## Limitations and Safety

- This is an educational prototype, not a medical device.
- Sensor readings and calculated SpO₂ values have not been clinically validated.
- Simulated MQTT values must not be interpreted as real patient measurements.
- Battery level and Wi-Fi signal strength are not currently supplied by the firmware.
- The PPG waveform and OLED display shown in the dashboard are illustrative mock-ups, not live hardware data.

## Repository Structure

```text
Patient-Health-Monitoring-System/
├── README.md
├── index.html
└── patient_health_monitoring_emqx.ino
```

## Future Improvements

- Validate heart-rate and SpO₂ readings using physical hardware.
- Improve sensor signal processing and error handling.
- Add secure device provisioning.
- Add historical telemetry storage.
- Improve patient and device management.
- Test long-term MQTT connection reliability.

## License

No license has currently been specified for this repository.
