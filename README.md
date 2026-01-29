# 🐼 PandaTouch Bluetooth StreamDeck

Transform your **BigTreeTech PandaTouch** into a powerful **native Bluetooth StreamDeck (HID)**. Control your apps, macros, and multimedia with no additional software or cables required on your PC.

![PandaTouch](docs/images/pandatouch.png)

## ✨ Key Features

- ⌨️ **Native Bluetooth HID**: Identifies as a standard Bluetooth keyboard. Zero latency.
- 🍏🪟 **Dual Support (Windows & macOS)**: Independent profiles that automatically adapt to your operating system.
- 📂 **Web Management**: Configure buttons, icons, colors, and WiFi from an intuitive dashboard in your browser.
- 🔄 **Web OTA Updates**: Update firmware wirelessly directly from the control panel.
- 📑 **Backup & Restore**: Easily save and recover your configurations via JSON files.
- 🇪🇸 **Localized Keyboard**: Full support for special characters in both Spanish and US layouts.

---

## �️ Installation Guide

### Option A: Easy Flashing (No-Code)
The fastest way for non-developers. You only need a Chrome-based browser:
1. Download the latest `firmware.bin` from the releases page.
2. Go to [ESP Web Tools](https://web.esphome.io/) or [Adafruit Web Serial ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/).
3. Connect your PandaTouch via USB-C and click **Connect**.
4. Select the `.bin` file and flash it.

### Option B: Advanced (PlatformIO)
Recommended if you want to modify the code:
1. **Preparation**: Connect your PandaTouch via **USB-C**.
2. **Drivers**: Ensure you have the serial port drivers (CH340) installed.
3. **Flashing**:
   - Open this project in **VS Code** with the **PlatformIO** extension.
   - Click the "Ant" icon (PlatformIO) and select `Upload`.
   - Alternatively, use the terminal: `pio run -t upload`.
4. **First Boot**: The device will restart. Look for a Bluetooth device named **"PandaTouch Deck"** on your PC/Mac and pair it.

> [!IMPORTANT]
> **Why use a cable for the first time?**
> While the original PandaTouch firmware has an update option, it is highly recommended to use a USB-C cable for the first installation. This project uses a custom partition table to allocate more space for icons and configurations (LittleFS) which the stock OTA might not handle correctly, potentially leading to stability issues.

---

## 🔄 How to Update

Once you have this firmware installed, you don't need cables for future versions:

1. Access the **Web Dashboard** by entering the device's IP address in your browser.
2. In the right sidebar, find the **"Firmware OTA"** section.
3. Select the generated or downloaded `.bin` file.
4. Click **"Update"**. You will see a message on the device screen indicating progress. Do not turn it off until it reboots.

---

## 🛠️ How to Revert to Original Firmware

If you want to return to the original BigTreeTech system to control your 3D printer, you can do so at any time:

1. Download the official BigTreeTech recovery tool: [Recovery Tool V1.0.6](https://github.com/bigtreetech/PandaTouch/blob/master/Recovery_toolV1.0.6.zip).
2. Follow the instructions included in the `.zip` to flash the original firmware (.img/.bin) provided by BTT.
3. This will erase the StreamDeck mode and restore all factory functions.

---

## 🖥️ Web Configuration

To configure your buttons, simply enter the IP address displayed on the device's home screen:

- **OS Toggle**: Select Windows or Mac at the top right.
- **Icons**: Choose from the built-in LVGL symbol library or upload your own images in the **Library** section.
- **Commands**: Enter the app path or the link you want to execute (supports up to 255 characters).
- **Backups**: Use "Download Backup" to save your current layout.

---

## ⚠️ Troubleshooting

- **Screen flickering**: This is the low battery warning. Connect the device to its dock or a USB charger.
- **Not appearing in Bluetooth**: Make sure it's not connected to another device. "Forget" the device on your PC and search again.
- **File upload error**: Verify that your WiFi network is 2.4GHz (ESP32 does not support 5GHz).
