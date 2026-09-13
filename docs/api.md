# HTTP and serial API

PicoPal serves its web interface and HTTP API on port 80. The API is intended for devices on the same trusted local network and does not provide authentication or TLS.

## HTTP endpoints

### Device

#### `GET /health`

Returns the current network mode and IP address.

```json
{"status":"ok","mode":"station","ip":"192.168.1.42"}
```

#### `POST /configure`

Available in setup mode. Accepts `application/x-www-form-urlencoded` fields named `ssid` and `password`, saves them to NVS, and restarts the device. Credentials must not be logged or committed to the repository.

#### `POST /api/wifi/forget`

Erases saved Wi-Fi credentials and restarts into setup mode.

```json
{"restarting":true}
```

### Timer

#### `GET /api/timer`

Returns live timer state. Responses use `Cache-Control: no-store`.

```json
{"running":true,"elapsed_seconds":125,"display":"002:05"}
```

#### `POST /api/timer/toggle`

Queues a remote start/pause action and returns HTTP `202`:

```json
{"accepted":true}
```

#### `POST /api/timer/reset`

Queues a remote reset action and returns HTTP `202`. Unlike the physical controls, remote timer commands are accepted regardless of the current OLED page.

### Drawing gallery

#### `GET /api/images`

Returns ordered image IDs and the selected ID:

```json
{"count":2,"selected":7,"items":[{"id":4},{"id":7}]}
```

#### `GET /api/images/current`

Returns the selected image as `application/octet-stream`, or HTTP `404` when the gallery is empty. The body is exactly 1,024 bytes in SSD1306 page layout:

```text
byte index = (y / 8) * 128 + x
bit index  = y % 8
```

#### `POST /api/images/current`

Adds and selects an image. Requirements:

- `Content-Type: application/octet-stream`
- body length: exactly 1,024 bytes
- `X-Image-CRC32`: one to eight hexadecimal characters using standard CRC-32/ISO-HDLC

Example response:

```json
{"saved":true,"id":8}
```

The server validates the complete body and CRC before promoting temporary files. It returns HTTP `507` when the 100-image limit or storage reserve prevents another image.

#### `POST /api/images/select?id=8`

Selects an existing image and updates the OLED.

```json
{"selected":true}
```

#### `DELETE /api/images/8`

Deletes an image and selects an adjacent image when the deleted item was active.

```json
{"deleted":true}
```

## Serial protocol

The host protocol is newline-delimited ASCII at 115,200 baud. Versioned commands use this form:

```text
PICO/1 base coding
PICO/1 base idle
PICO/1 base error
PICO/1 base disconnected
PICO/1 react codex_done
```

`base` commands set persistent Pico state. `react` commands start temporary, priority-controlled reactions and then return to the current base state. Unknown versions, states, trailing tokens, and overlong lines are rejected.

For interactive diagnostics, the firmware also accepts `status`, `help`, and the compact forms printed by `help`.
