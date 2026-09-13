# Hardware and wiring

PicoPal targets the ESP32-S3 DevKitC-1 N16R8. All peripheral signals use 3.3 V logic. Disconnect USB power before changing the wiring.

## Pin assignment

| ESP32-S3 pin | Peripheral pin | Purpose |
| --- | --- | --- |
| 3V3 | OLED VCC, MPU6050 VCC, joystick VCC | Peripheral power |
| GND | All peripheral grounds | Common ground |
| GPIO8 | OLED SDA, MPU6050 SDA | Shared I²C data |
| GPIO9 | OLED SCL, MPU6050 SCL | Shared I²C clock |
| GPIO5 / ADC1 channel 4 | Joystick VRx | Left/right page navigation |
| GPIO6 / ADC1 channel 5 | Joystick VRy | Up/down drawing navigation |
| GPIO7 | Joystick SW | Timer reset |
| GPIO15 | Momentary button | Timer start/pause |
| GPIO4 / touch pad 4 | Conductive touch area | Pico touch input |

Connect both the joystick switch and the timer button between their GPIO pin and GND. The firmware enables internal pull-ups, so external pull-up resistors are not required for these two inputs.

## I²C bus

The OLED and MPU6050 share the same bus because their 7-bit addresses are different:

| Device | Address | Bus speed used by firmware |
| --- | --- | --- |
| SSD1306 OLED | `0x3C` | 100 kHz |
| MPU6050 | `0x68` | 400 kHz |

The firmware enables the ESP32-S3 internal I²C pull-ups. Many breakout boards also include pull-ups. For longer wiring or unreliable communication, add suitable external pull-ups to 3.3 V and keep the bus short.

## Joystick orientation

The expected behavior is left/right for page navigation and up/down for image navigation. Analog joystick modules can mount their potentiometers in different orientations. If physical directions are reversed, swap the corresponding low/high event mapping in `main/picopal_input.c`; do not connect an analog output to 5 V because the ESP32-S3 ADC is not 5 V tolerant.

## Flash layout

The project uses the board's 16 MB flash:

| Partition | Size | Purpose |
| --- | ---: | --- |
| NVS | 24 KiB | Wi-Fi credentials and system settings |
| OTA data | 8 KiB | Active OTA slot selection |
| OTA app 0 | 3 MiB | Application image |
| OTA app 1 | 3 MiB | Update/rollback application image |
| Storage | 9.875 MiB | LittleFS drawing data and future web assets |

Images are stored as 1-bit 128×64 framebuffers. Each image consumes 1,024 bytes plus filesystem and index overhead. The gallery enforces a limit of 100 images even when more flash is available.
