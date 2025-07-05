# DarkLight Cover Calibrator Firmware

This folder contains the **firmware** for the [DarkLight Cover Calibrator](https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator), to control a standalone cover, light panel, or combination flip-flat mechanism, and optional dew heater.

---

## 🔖 Firmware Version



📄 See full [Firmware Version History](https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator/wiki/Firmware-Version-History)

---

## 🚀 Getting Started

You can use either the **Arduino IDE** or **PlatformIO** to build and upload the firmware.

---

### 🛠 Supported Boards

The firmware supports:
- **Arduino Nano / Uno / Mega** (AVR)

---

## 🧰 Arduino IDE Setup

1. Open `dlc_firmware.ino` in the Arduino IDE.

2. Select the correct board:
   - For Nano: `Arduino Nano` → ATmega328P

3. Install the required libraries below.

4. Connect your board via USB and click **Upload**.

---

## 📦 Required Libraries

| Library                  | Notes                                       |
|--------------------------|---------------------------------------------|
| `Adafruit_BME280_Library`  | Environmental sensor support (BME280)       |
| `Adafruit_BusIO`           | Required by BME280 library                  |
| `Adafruit_Unified_Sensor`  | Shared sensor support for Adafruit drivers |
| `DHT_sensor_library`       | DHT11/DHT22 sensor support                  |
| `DallasTemperature`        | 1-Wire temperature sensor                   |
| `OneWire`                  | Required by DallasTemperature               |
| `EEPROMWearLevel`          | EEPROM wear management (AVR only)           |
| `dlcServo`                 | Smooth servo movement with speed control    |

> ⚠️ Depending on the enabled features, not all libraries may be required.  Refer to the [Wiki](https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator/wiki) for more details.

---

## 📚 Configuration, Testing & Troubleshooting

Refer to the project’s [Wiki](https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator/wiki) for detailed instructions on:

- Setting up your hardware
- Enabling/disabling features
- Uploading firmware
- Using INDI, ASCOM, and Serial control
- Troubleshooting and tuning performance

---

## 🤝 Contributing

We welcome contributions! Please see the [CONTRIBUTING.md](../.github/CONTRIBUTING.md) file in the root project for guidelines on submitting pull requests.

---

## 📜 License

© 2020–2025 10th Tee Astronomy. All rights reserved.

This project is licensed under the  
**Creative Commons Attribution-NonCommercial 4.0 International License**.

- You may share and adapt the materials for personal or academic use  
- Commercial use is **prohibited** without written permission  
- Modified versions must credit the original work  
- See [`LICENSE.md`](../LICENSE.md) for full terms

---

Happy imaging!  
— **The 10thTeeAstronomy Team**
