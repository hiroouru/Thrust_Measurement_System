
## Project Overview

The project consists of two major components:

1. **Thrust Measurement System**:
   - Uses an HX711 load cell to measure thrust data from an engine or motor.
   - BLE communication is implemented to send the measurement data to a BLE client.
   - Data is saved to an SD card and can be visualized using a Python script.

2. **BLE-Based Ignition System**:
   - A BLE-enabled ignition system that triggers an ignition sequence via relays.
   - The client and server communicate over BLE to handle commands like ignition, start, and stop measurements.

## Hardware Requirements

- **ESP32** (two boards for client-server communication)
- **HX711 Load Cell Amplifier** for thrust measurement
- **SD Card Module** for saving thrust data
- **Relays** for ignition control
- **LEDs** for status indication
- **Bluetooth Low Energy (BLE)** for communication between the ESP32 boards

## Software Requirements

- **Arduino IDE**: For programming the ESP32 boards.
- **Python**: For data plotting (`plot_V2.py`).
- **Arduino Libraries**:
  - `BLEDevice` for BLE communication
  - `HX711_ADC` for load cell data
  - `SimpleKalmanFilter` for data smoothing
  - `SPI` and `SD` for SD card handling

## Code Breakdown

### `client.ino`
This code is used on the client-side ESP32, which scans for the BLE server (the thrust measurement system) and sends commands to trigger the ignition and start the thrust measurement.

### `server_v3.ino`
This is the code running on the server-side ESP32, which collects data from the load cell, performs data smoothing using a Kalman filter, and communicates with the client to send measurement data over BLE. It also controls the SD card for saving the data.

### `igniting_code.ino`, `relay_first.ino`, and `relay_second.ino`
These files contain the code for the BLE-based ignition system. The relays are triggered by BLE commands sent from the client to the server to start the ignition sequence.

### `plot_V2.py`
This Python script is used to plot the thrust data saved on the SD card. It reads the data file and plots it using matplotlib.

## Setup Instructions

### Arduino Code
1. **Install the necessary libraries** in the Arduino IDE:
   - `BLEDevice`
   - `HX711_ADC`
   - `SimpleKalmanFilter`
   - `SPI`
   - `SD`

2. **Upload the code**:
   - Upload `client.ino` to the **client-side ESP32**.
   - Upload `server_v3.ino` to the **server-side ESP32**.
   - Upload the respective ignition codes (`igniting_code.ino`, `relay_first.ino`, `relay_second.ino`) to the ignition controllers.

3. **Hardware connections**:
   - Connect the load cell to the HX711 amplifier, and then to the server-side ESP32.
   - Connect the relays and LEDs to the client and server-side ESP32s as specified in the code.

### Python Script
1. Install the required Python libraries:
   ```bash
   pip install matplotlib numpy
