# M5Stack NanoC6 — Arduino IDE Setup & OpenThread LED Control

## Hardware

| Feature | Detail |
|---|---|
| SoC | ESP32-C6FH4 (RISC-V, 160 MHz) |
| Flash | 4 MB |
| Wireless | Wi-Fi 6 (802.11ax), BLE 5.3, IEEE 802.15.4 (Thread / Zigbee) |
| Blue LED | GPIO7 |
| RGB LED (WS2812) | GPIO20 (data), GPIO19 (power enable) |
| Button | GPIO9 (active low) |
| IR Transmitter | GPIO3 |
| Grove I2C | SDA → GPIO2, SCL → GPIO1 |

---

## 1. Arduino IDE Setup

### 1.1 Install Arduino IDE

Download from https://www.arduino.cc/en/software (version 2.x recommended).

### 1.2 Add M5Stack Board Manager URL

1. Open **File → Preferences**
2. Paste the following into *Additional boards manager URLs*:
   ```
   https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
   ```
3. Click **OK**

### 1.3 Install the M5Stack Board Package

1. Open **Tools → Board → Boards Manager**
2. Search for `M5Stack`
3. Install **M5Stack** version **3.2.5 or newer**

### 1.4 Select the Board

**Tools → Board → M5Stack → M5NanoC6**

### 1.5 Required Tools Menu Settings

| Setting | Value |
|---|---|
| Board | M5NanoC6 |
| Port | *(your USB port)* |
| USB CDC On Boot | Enabled |
| Flash Size | 4MB |
| Partition Scheme | Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS) |

> **Note:** For Matter sketches the partition scheme must be changed to  
> `Huge APP (3MB No OTA / 1MB SPIFFS)` and *Erase All Flash Before Sketch Upload* must be enabled.

### 1.6 Entering Download Mode

If the device is not detected or upload fails:

1. **Hold** the button (GPIO9)
2. **Connect** the USB cable while holding the button
3. Release the button — the device is now in download mode

---

## 2. Libraries Required

Install via **Tools → Manage Libraries** (`Ctrl+Shift+I` / `Cmd+Shift+I`):

| Library | Purpose |
|---|---|
| M5NanoC6 | Board support helpers (`NanoC6.begin()`) |
| Adafruit NeoPixel | RGB LED control |

For each library:
1. Search the library name
2. Find the entry by **Adafruit** / **M5Stack** (avoid third-party clones)
3. Click **Install**
4. When prompted *"Would you like to install all missing dependencies?"* — click **Install All**

OpenThread (`OThreadCLI.h`) is bundled with the M5Stack board package — no separate install needed.

---

## 3. OpenThread LED Control Sketch

The sketch `NanoC6_Thread_LED.ino` starts the device as an OpenThread node and listens on **UDP port 5000** for simple text commands over the Thread network:

| Command | Action |
|---|---|
| `LED_ON` | Turn blue LED on |
| `LED_OFF` | Turn blue LED off |
| `LED_TOGGLE` | Toggle blue LED |
| `RGB_ON` | Turn RGB LED on (white) |
| `RGB_OFF` | Turn RGB LED off |

### Sending a command from another Thread node (or CLI)

```bash
# From the OpenThread CLI on another device:
udp open
udp send <IPv6-address-of-NanoC6> 5000 "LED_ON"

# Or broadcast to all nodes on the Thread mesh:
udp send ff03::1 5000 "LED_TOGGLE"
```

The device IPv6 address is printed to Serial on boot.

### Default Thread Network Credentials

The sketch uses these defaults (change them in the `#define` block at the top of the `.ino`):

| Parameter | Default |
|---|---|
| Network Key | `00112233445566778899aabbccddeeff` |
| Channel | `15` |
| PAN ID | `0x1234` |

---

## 4. Project Files

```
NanoC6_Thread/
├── README.md                  # This file
├── m5stack-nanoc6.yaml        # ESPHome configuration (Wi-Fi / BLE Proxy)
└── NanoC6_Thread_LED/
    └── NanoC6_Thread_LED.ino  # Arduino OpenThread LED sketch
```

---

## 5. Useful Links

- [M5Stack NanoC6 Docs](https://docs.m5stack.com/en/core/M5NanoC6)
- [M5Stack NanoC6 Arduino Thread Examples](https://docs.m5stack.com/en/arduino/m5nanoc6/thread)
- [arduino-esp32 OpenThread docs](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/openthread.html)
- [OpenThread CLI reference](https://openthread.io/reference/cli)
