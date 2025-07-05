# INDI Driver: DarkLight Cover Calibrator

This is the INDI driver source code for the **DarkLight Cover Calibrator (DLC)**. It enables control of the DLC hardware via the INDI protocol, providing compatibility with astronomy software on Linux systems.

---

## 🔖 Firmware Version


📄 See full [INDI Version History](https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator/wiki/INDI-Version-History)

---

## 📦 Features

- Cover control (Open/Close/Halt)  
- Calibrator control (On/Off/Brightness)  
- Dew heater  control  
- Support for ASCOM-style commands over INDI  
- Fully compatible with INDI clients like KStars and Ekos  

---

## 🛠️ Building and Installing

To compile and install the driver from source:

### 1. Clone the repository
```bash
git clone https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator.git
cd DarkLight_CoverCalibrator/indi-dlc-src
```

### 2. Create a `build` directory
```bash
mkdir build
cd build
```

### 3. Run `cmake`
```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
```

### 4. Compile the driver
```bash
make
```

### 5. Install the driver
```bash
sudo make install
```

---

## 📚 Resources

- Main Project: [DarkLight Cover Calibrator GitHub](https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator)  
- Firmware: [dlc_firmware](https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator/tree/main/dlc_firmware)  
- Documentation: See the [GitHub Wiki](https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator/wiki)  

---

## 🤝 Contributing

We welcome contributions! Please see the `CONTRIBUTING.md` file in the root project for intallation and guidelines on submitting pull requests.

---

## 📜 License

© 2020–2025 10th Tee Astronomy. All rights reserved.

This project is licensed under the  
**Creative Commons Attribution-NonCommercial 4.0 International License**.

- You may share and adapt the materials for personal or academic use  
- Commercial use is prohibited without written permission  
- Modified versions must credit the original work  

See `LICENSE.md` for full terms.  

**Happy imaging!**  
— *The 10thTeeAstronomy Team*
