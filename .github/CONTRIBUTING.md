# Contributing to DarkLight Cover Calibrator

Welcome, and thank you for your interest in contributing to the **DarkLight Cover Calibrator** project hosted at:

👉 https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator

This project includes:
- **Arduino-based firmware** (in the `dlc_firmware/` folder)
- **ASCOM driver and application files** (in the `ascom/` folder)
- **INDI driver code** (in the `indi-dlc-src` folder)

We welcome reporting of issues, feature suggestions, and pull requests!

---

## 🧰 Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/10thTeeAstronomy/DarkLight_CoverCalibrator.git
   ```

## 🧠 Making Changes

- Always create a new branch from `master`:
   ```bash
   git checkout -b feature-or-fix-name
   ```

- Keep changes focused and well-documented in your commit messages.

- For firmware: ensure it compiles and behaves correctly on real hardware.

- For INDI: build and test using your preferred environment. We recommend KStars.

---

### 🔧 Arduino Firmware (dlc_firmware)

1. Change into the directory:
   ```bash
   cd DarkLight_CoverCalibrator/dlc_firmware/
   ```

2. Open the sketch in Arduino IDE or VS Code + PlatformIO.

3. Select the correct board (e.g., Arduino Nano or ESP32, depending on your hardware).

4. Required libraries:
   > 📌 A `README.md` is included in the `dlc_firmware/` folder with setup instructions and library details.

---

### 🧪 INDI Driver (indi-dlc-src)<a name="INDI"></a>

1. Download and set up INDI, following the standard INDI build process.

2. Complete the following to build and install the INDI DLC driver:
   ```bash
   cd indi-dlc-src
   mkdir build && cd build
   cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../
   make
   sudo make install
   ```

---


## 📦 Continuous Integration

This project uses **GitHub Actions** to automatically build and verify:

- ✅ Arduino firmware (compile check using standard configuration)
- ✅ INDI driver (CMake build and installation process)

When you open a pull request, these checks will run automatically.

To ensure your PR passes:

- Make sure your firmware compiles with the standard `#define` configuration.
- Confirm any new libraries are included in `dlc_firmware/DLC_Library/`.
- Test INDI driver changes using the provided CMake steps.

⚠️ Pull requests that fail CI **cannot be merged**.  
Click the “Details” link next to any failed check for troubleshooting info.

---

## 🔁 Merge Policy

To keep the commit history clean, **project maintainers** use a linear history strategy. When merging PRs, we:

- ✅ Prefer **Squash and Merge** for cleanup  
- ✅ May use **Rebase and Merge** for organized commits  
- ❌ Do **not** use "Create a merge commit"

_You don’t need to worry about this unless you're part of the maintainer team._

---

## ✅ Submitting a Pull Request

- Push your feature branch to your fork.
- Open a pull request against `10thTeeAstronomy:master`.
- Clearly describe what the PR does and why.
- Reference any issues using `Closes #123` or `Fixes #456`.

### ✔️ Pull Request Requirements:
- Keep PRs small and focused.
- Follow Arduino/INDI coding conventions.
- Comment complex logic.
- Test firmware on target hardware.
- Ensure INDI driver builds and functions properly.

⚠️ **NOTE:** To ensure your changes will pass automated CI checks and compile properly, follow the steps below:

### 🔧 Arduino Firmware Requirements

1. In the `.ino` file, use this standard configuration to make sure the following are uncommented:
   ```cpp
   #define COVER_INSTALLED
   #define LIGHT_INSTALLED
   #define HEATER_INSTALLED
   #define ENABLE_SERIAL_CONTROL
   #define ENABLE_MANUAL_CONTROL
   #define ENABLE_SAVING_TO_MEMORY
   #define USE_LINEAR
   #define HEATER_ONE_INSTALLED
   #define HEATER_TWO_INSTALLED
   #define ENABLE_BME280
   ```

2. If a new library is added or edited, make sure to place in:
   ```
   dlc_firmware/DLC_Library/
   ```

⚠️ Changes that break this configuration **will fail CI and cannot be merged.**

### 🛰 INDI Driver Requirements

To ensure your pull request is accepted and passes CI, your changes must compile successfully using the standard CMake process:

Review [INDI Driver (indi-dlc-src)](#INDI) under Making Changes for setup instructions.

1. Test with **KStars** or another INDI-compatible client:
   - Confirm the driver appears in the device list
   - Ensure it connects without errors
   - Verify basic functionality (e.g., open/close, turn on/off calibrator)

⚠️ Changes that break the INDI build or basic functionality **will fail CI and cannot be merged.**

---

## 🧯 Troubleshooting Pull Requests

### 🔄 Can’t merge due to conflicts?

This means your branch is out of date with the latest `master`. Fix this by running:

```bash
git fetch origin
git rebase origin/master
# Resolve any conflicts, then:
git add .
git rebase --continue
```

Or click **“Update branch”** in your GitHub PR if available.

---

### 🚨 CI failed?

Check the **Details** link next to the failed check. Common reasons include:
- Missing `#define` in firmware config
- INDI driver failed to compile
- Library not placed in `DLC_Library/`

---

### 📥 No feedback after submitting?

If it's been more than 5 days without feedback, feel free to tag `@10thTeeAstronomy` in a comment or email us at:

📧 [10thTeeAstronomy@gmail.com](mailto:10thTeeAstronomy@gmail.com)

---

## 🛡️ Reporting Security Issues

If you discover a security issue, please do **not** open a GitHub issue.

Instead, email us at:

📧 [10thTeeAstronomy@gmail.com](mailto:10thTeeAstronomy@gmail.com)

---

## 🙌 Thank You!

Whether it's reporting a bug, improving documentation, or writing code — your contribution helps make this project better for the entire astronomy community.

Happy contributing!  
— **The 10thTeeAstronomy Team**
