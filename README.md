# IoT Climate Monitoring Dashboard

A simple IoT project that uses an ESP32 to read temperature and humidity from an SHT41 sensor, display the values on an OLED screen, send the data to a FastAPI backend, store readings in SQLite, and show the latest readings on a web dashboard.

## What It Does

- Reads temperature and humidity using an SHT41 sensor
- Displays live readings on an I2C OLED screen
- Sends readings from the ESP32 to a Python FastAPI backend
- Stores readings in a SQLite database
- Shows current temperature, humidity, last updated time, and a 24-hour graph on a website

## Parts Used

- ESP32 development board
- SHT41 temperature and humidity sensor
- I2C OLED display
- Breadboard
- Jumper wires
- USB cable
- Laptop for running the backend and frontend

## Wiring

Both the OLED and SHT41 use I2C, so they share the same SDA and SCL pins.

| ESP32  | SHT41 | OLED |
| ------ | ----- | ---- |
| 3.3V   | VIN   | VCC  |
| GND    | GND   | GND  |
| GPIO21 | SDA   | SDA  |
| GPIO22 | SCL   | SCL  |

Detected I2C addresses:

```text
OLED: 0x3C
SHT41: 0x44
```

# Commands

Firmware:

- ipconfig getifaddr en0
- idf.py build flash monitor

Backend:

- python3 -m venv .venv
- source .venv/bin/activate
- pip install -r requirements.txt
- uvicorn main:app --reload --host 0.0.0.0 --port 8000

Frontend:

- python3 -m http.server 5500
