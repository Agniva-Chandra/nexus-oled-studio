
# ⚡ NEXUS OLED
### ESP32 Animation & Display Studio

<p align="center">
  <b>Turn a tiny OLED display into a futuristic animation playground.</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge&logo=espressif" />
  <img src="https://img.shields.io/badge/Framework-Arduino-00979D?style=for-the-badge&logo=arduino" />
  <img src="https://img.shields.io/badge/Display-SH1106-black?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Interface-Web%20Dashboard-8A2BE2?style=for-the-badge" />
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" />
</p>

---

## 🚀 Overview

**NEXUS OLED** is an ESP32-powered wireless OLED animation and control system.

It transforms a small 128×64 SH1106 OLED display into a programmable animation platform that can be controlled directly from a smartphone, laptop, or desktop browser.

The ESP32 creates its own Wi-Fi Access Point and hosts a futuristic web dashboard. From this dashboard, users can switch between animations, adjust display brightness, control animation speed, monitor system performance, and upload custom animations using JSON.

No external cloud service or internet connection is required for the core experience.

## ✨ Features

### 🎨 Built-in Animation Engine

Explore multiple real-time OLED animation modes:

- 🔷 **3D Cube** – Rotating wireframe 3D cube.
- ⚪ **Bouncing Ball** – Animated ball with motion trails.
- 🤖 **Robotic Face** – Animated eyes, blinking, and expressions.
- 📡 **Radar Scanner** – Rotating radar with target indicators.
- 🌊 **Wave Animation** – Dynamic mathematical wave patterns.
- 🧩 **Custom Animation** – Upload your own JSON animations.

### 🌐 Wireless Web Dashboard

Control your OLED display through a locally hosted web interface.

Dashboard features:

- Futuristic dark-themed interface.
- Wireless animation selection.
- Live system telemetry.
- Real-time FPS monitoring.
- Free heap memory monitoring.
- Animation speed adjustment.
- OLED brightness control.
- Pause and resume controls.
- JSON animation editor.
- Live browser animation preview.
- Upload and execute custom animations.

### ⚙️ JSON Animation Studio

Create your own animations without rewriting the ESP32 firmware.

Supported drawing elements:

| Element | Description |
|---|---|
| `text` | Display text |
| `line` | Draw lines |
| `circle` | Outline circle |
| `fillCircle` | Filled circle |
| `rect` | Rectangle |
| `fillRect` | Filled rectangle |
| `round` | Rounded rectangle |
| `triangle` | Triangle |
| `pixel` | Single pixel |
| `ray` | Rotating line |
| `bar` | Animated progress bar |

### 🎬 Animation Effects

The custom animation engine supports:

- Property interpolation.
- Position and size transitions.
- Rotation animation.
- Multiple easing functions.
- Typewriter text effect.
- Blinking elements.
- Static layers.
- Looping animations.
- Multi-frame animation sequences.

Supported easing functions:

- `linear`
- `in`
- `out`
- `inout`

## 🖥️ Dashboard Preview

The web dashboard is hosted directly by the ESP32.

**Connection details:**

| Setting | Value |
|---|---|
| Wi-Fi SSID | `ESP32-OLED` |
| Wi-Fi Password | `12345678` |
| Dashboard URL | `http://192.168.4.1` |
| Server Port | `80` |

Connect your smartphone or computer to the ESP32 Wi-Fi network and open the dashboard in a browser.

## 🔌 Hardware Requirements

| Component | Specification |
|---|---|
| Microcontroller | ESP32 Development Board |
| OLED Display | SH1106 |
| Resolution | 128 × 64 pixels |
| Interface | I2C |
| OLED Address | `0x3C` |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Wiring Diagram

| ESP32 | OLED |
|---|---|
| GPIO 21 | SDA |
| GPIO 22 | SCL |
| 3.3V | VCC |
| GND | GND |

**Note:** Check your OLED module's voltage requirements before connecting it.

## 🛠️ Software Requirements

Install the following Arduino libraries:

1. Adafruit GFX Library
2. Adafruit SH110X
3. ArduinoJson (Version 7)

Also required:

- Arduino IDE
- ESP32 Board Package
- ESP32 Development Board

## ⚡ Installation & Setup

### Step 1 — Clone the Repository

```bash
git clone https://github.com/YOUR-USERNAME/nexus-oled-studio.git
```

### Step 2 — Open the Project

Open the `.ino` firmware file in Arduino IDE.

### Step 3 — Install Libraries

Install the required libraries using:

`Arduino IDE → Library Manager`

### Step 4 — Connect the Hardware

Connect the SH1106 OLED display to the ESP32 using the wiring table above.

### Step 5 — Configure the Board

Select your ESP32 board and the correct COM port.

### Step 6 — Upload Firmware

Compile and upload the firmware to your ESP32.

### Step 7 — Connect to the Dashboard

1. Enable Wi-Fi on your smartphone or computer.
2. Connect to `ESP32-OLED`.
3. Enter password `12345678`.
4. Open `http://192.168.4.1`.
5. Start controlling your OLED!

## 🎮 How to Use

Once connected to the dashboard:

1. Select an animation mode.
2. Adjust the frame interval.
3. Change OLED brightness.
4. Pause or resume animations.
5. Select a JSON preset.
6. Modify the animation program.
7. Upload and run your custom animation.

### Example JSON Animation

```json
{
  "frames": [
    {
      "duration": 1000,
      "items": [
        {
          "type": "circle",
          "x": 64,
          "y": 32,
          "r": 10,
          "toR": 25,
          "ease": "inout"
        }
      ]
    }
  ]
}
```

This example creates an animated circle that expands and repeats.

## 📊 System Architecture

```text
       SMARTPHONE / COMPUTER
                 |
                 | Wi-Fi
                 |
          ESP32 ACCESS POINT
                 |
          HTTP WEB SERVER
                 |
          WEB DASHBOARD
                 |
       ---------------------
       |         |         |
     MODE     DISPLAY    JSON
    CONTROL   SETTINGS  UPLOAD
       |         |         |
       ---------------------
                 |
          ANIMATION ENGINE
                 |
          I2C COMMUNICATION
                 |
            SH1106 OLED
             128 × 64
```

## 🔍 System Telemetry

The dashboard provides access to live system information:

- Current animation mode.
- Frames per second (FPS).
- Available ESP32 heap memory.
- Current animation speed.
- Display contrast.
- Custom animation availability.

The state endpoint is:

```http
GET /api/state
```

## 🔗 HTTP API

| Endpoint | Method | Function |
|---|---|---|
| `/` | GET | Open dashboard |
| `/mode?m=cube` | GET | Change animation |
| `/pause` | GET | Pause animation |
| `/resume` | GET | Resume animation |
| `/speed?v=20` | GET | Adjust frame interval |
| `/contrast?v=207` | GET | Adjust brightness |
| `/api/state` | GET | Read system telemetry |
| `/upload` | POST | Upload JSON animation |

**Custom animation upload:** JSON payload is limited to 8 KB in the current firmware.

## 📁 Project Structure

```text
nexus-oled-studio/
│
├── firmware/
│   └── nexus_oled_studio.ino
│
├── docs/
│   ├── wiring.md
│   └── animation-guide.md
│
├── examples/
│   ├── boot.json
│   ├── orbit.json
│   ├── radar.json
│   └── hello.json
│
├── screenshots/
│   └── dashboard.png
│
├── README.md
└── LICENSE
```

*This is a suggested repository organization. Add the folders and example files as you create them.*

## 🔐 Security Notes

- The ESP32 operates as a local Wi-Fi Access Point.
- The dashboard is accessible to devices connected to that network.
- Change the default Wi-Fi password before sharing or deploying the project.
- This project is designed for local demonstration and experimentation.

## 🚧 Future Improvements

Ideas for future development:

- [ ] More robotic face expressions.
- [ ] Animated 3D globe.
- [ ] Additional 3D animations.
- [ ] Custom animation library.
- [ ] Save and load JSON presets.
- [ ] OLED theme customization.
- [ ] Improved dashboard telemetry.
- [ ] Multiple OLED display support.
- [ ] Touch-based animation controls.
- [ ] Wireless firmware update support.

## 🤝 Contributing

Contributions, ideas, and improvements are welcome!

1. Fork this repository.
2. Create a new branch.
3. Make your changes.
4. Submit a pull request.

## 📜 License

This project is licensed under the MIT License.

## 👨‍💻 Author

**Agniva Chandra**

Developer | Electronics Enthusiast | Embedded Systems Explorer

GitHub: [@Agniva-Chandra](https://github.com/Agniva-Chandra)

---

<p align="center">
  <b>Built with ⚡ ESP32, creativity, and a passion for electronics.</b>
  <br />
  <i>Small Display. Unlimited Possibilities.</i>
</p>
```

---

## Recommended GitHub repository settings

| Field | Recommended value |
|---|---|
| Repository name | `nexus-oled-studio` |
| Website | Leave blank for now |
| Topics | `esp32`, `oled`, `sh1106`, `arduino`, `iot`, `web-dashboard`, `json-animation`, `embedded-systems` |
| License | MIT |
| Visibility | Public |

**My recommendation:** Use the tagline **"Small Display. Unlimited Possibilities."** It gives the project a strong identity.

One important thing: Your current firmware uses GPIO 21 and 22 for I2C, and the README reflects those exact pins. The README also separates implemented features from future ideas rather than claiming unfinished features are already available.
