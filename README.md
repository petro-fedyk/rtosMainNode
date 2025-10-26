# ESP32 Greenhouse Wi-Fi Server

This project implements a **Wi-Fi server** on the **ESP32** using **FreeRTOS**, serving sensor data and handling client messages. It reads environmental data like soil level, temperature, humidity, and light, and allows remote access via TCP and HTTP.

---

## Features

- Connects to Wi-Fi and automatically reconnects if connection is lost.
- TCP server on port **5000** for client communication.
- HTTP server on port **80** serving:
  - `/` → `index.html` from LittleFS
  - `/data` → JSON with current sensor readings.
- Uses **FreeRTOS tasks** for:
  - Wi-Fi management
  - TCP server
  - HTTP server and data handling
- Thread-safe data exchange via **FreeRTOS queues**.
- Supports JSON communication with clients.
- Debug output via **Serial monitor** at 115200 baud.

---

## Hardware Requirements

- **ESP32 development board**.
- Sensors providing:
  - Soil moisture
  - Hall sensor input
  - Temperature
  - Humidity
  - Light level
- Optional: Web client or TCP client to connect to ESP32.

---

## Software Requirements

- **Arduino IDE** with ESP32 core installed.
- Libraries:
  - `WiFi.h`
  - `ArduinoJson`
  - `LittleFS`
  - `WebServer`
  - `freertos/event_groups.h` (included with ESP32 Arduino core)

---

## Usage

1. Flash the firmware to your ESP32.
2. Connect to Wi-Fi with SSID and password defined in the code.
3. TCP clients can connect to port **5000** to send/receive messages.
4. Open a browser and visit `http://<ESP32_IP>/` to view `index.html`.
5. Request JSON data at `http://<ESP32_IP>/data`.

---

## JSON Data Example

```json
{
  "soilLevel": 50.3,
  "hallLevel": 1,
  "temp_c": 24.7,
  "humidity": 60.2,
  "light": 120.5
}
