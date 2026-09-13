# PicoPal

PicoPal is an ESP32-S3 desktop companion built around a 128×64 SSD1306 OLED. It combines a focus timer, an animated pet named Pico, a persistent drawing gallery, motion and touch reactions, and USB notifications from a host computer.

## Features

- **Timer:** a count-up focus timer displayed as `MMM:SS`. The dedicated button starts or pauses it, and pressing the joystick resets it while the Timer page is active.
- **Pico:** animated eyes with idle, coding, sleeping, disconnected, and error base states, plus temporary reactions to touch, motion, and task completion.
- **Drawing gallery:** draw from an iPhone or another browser, save up to 100 monochrome images, delete or select them from the web interface, and browse them on the OLED with the joystick.
- **Local setup:** on first boot, PicoPal creates a temporary `PicoPal-Setup` Wi-Fi network. Credentials are stored in NVS and are never compiled into the firmware.
- **Host bridge:** a small Python utility sends versioned Pico state events over USB serial.

## Hardware

- ESP32-S3 DevKitC-1 N16R8
- 128×64 SSD1306 I²C OLED at address `0x3C`
- MPU6050 accelerometer at address `0x68`
- Dual-axis analog joystick with a push switch
- One normally-open momentary button
- A conductive touch area connected to GPIO4

See [Hardware and wiring](docs/hardware.md) before powering the circuit.

## Build and flash

PicoPal is an ESP-IDF project targeting ESP32-S3. It has been verified with ESP-IDF 5.4.

```bash
git clone <repository-url>
cd PicoPal
. "$HOME/esp/esp-idf/export.sh"
idf.py build
idf.py flash monitor
```

The component manager downloads the pinned LittleFS dependency during the first build. The custom partition table expects a 16 MB flash device.

## First-time Wi-Fi setup

1. Power PicoPal and connect the phone to the open `PicoPal-Setup` network.
2. Open `http://192.168.4.1` in Safari.
3. Enter the home Wi-Fi name and password.
4. PicoPal restarts and joins that network. Open the IP address printed in the serial log.

The web interface contains both Focus and Drawing controls. Use **Forget Wi-Fi** to erase saved credentials and return to setup mode.

## Controls

| Input | Action |
| --- | --- |
| Joystick left/right | Move between Timer, Pico, and Draw without wrapping |
| Joystick up/down | Select the previous/next image on the Draw page without wrapping |
| Joystick press | Reset the timer, only on the Timer page |
| GPIO15 button | Start or pause the timer, only on the Timer page |
| Short touch | Play Pico's happy reaction |
| Long touch | Put Pico to sleep; another touch wakes Pico |
| Motion | Play surprised or dizzy reactions |

Pico is always the initial page after boot.

## Host bridge

Create an isolated Python environment and install the serial dependency:

```bash
python3 -m venv companion/.venv
companion/.venv/bin/pip install -r companion/requirements.txt
ls /dev/cu.usbmodem* /dev/cu.usbserial* 2>/dev/null
```

Send a state or reaction using the detected port:

```bash
companion/.venv/bin/python companion/picopal_bridge.py \
  --port /dev/cu.usbmodemXXXX coding
companion/.venv/bin/python companion/picopal_bridge.py \
  --port /dev/cu.usbmodemXXXX done
```

Supported bridge events are `coding`, `done`, `error`, `recovered`, and `disconnected`. Do not run the bridge while another program, such as `idf.py monitor`, owns the same serial port.

## Tests

The model, command parser, and CRC implementation can be tested without ESP-IDF hardware:

```bash
cmake -S tests -B build-host
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

## Documentation

- [Hardware and wiring](docs/hardware.md)
- [HTTP and serial API](docs/api.md)

## Project structure

```text
PicoPal/
├── companion/         # USB host bridge
├── docs/              # Public hardware and protocol documentation
├── main/              # ESP-IDF firmware component
├── tests/             # Host-side unit tests
├── web/               # Embedded mobile web interface
├── partitions.csv     # 16 MB OTA and LittleFS layout
└── sdkconfig.defaults # Reproducible project defaults
```
