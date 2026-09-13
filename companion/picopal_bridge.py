#!/usr/bin/env python3
"""Send versioned host-state events to PicoPal over USB serial."""

from __future__ import annotations

import argparse
import json
import sys
import time
import serial


COMMANDS = {
    "coding": "PICO/1 base coding",
    "done": "PICO/1 react codex_done",
    "error": "PICO/1 base error",
    "recovered": "PICO/1 base idle",
    "disconnected": "PICO/1 base disconnected",
}


def send(port: str, event: str) -> None:
    command = COMMANDS[event]
    with serial.Serial(port, 115200, timeout=1, write_timeout=2) as device:
        time.sleep(0.15)
        device.write((command + "\n").encode("ascii"))
        device.flush()


def notification_event(raw: str) -> str | None:
    try:
        payload = json.loads(raw)
    except json.JSONDecodeError:
        return None
    kind = payload.get("type", "")
    return "done" if kind in {"agent-turn-complete", "turn-complete"} else None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="USB serial port, for example /dev/cu.usbmodem1101")
    parser.add_argument("event", nargs="?", choices=COMMANDS)
    parser.add_argument("--notification", help="Codex notification JSON; normally supplied by Codex")
    args = parser.parse_args()

    event = args.event
    if args.notification:
        event = notification_event(args.notification)
        if event is None:
            return 0
    if event is None:
        parser.error("provide an event or --notification JSON")

    try:
        send(args.port, event)
    except (serial.SerialException, OSError) as error:
        print(f"PicoPal bridge: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
