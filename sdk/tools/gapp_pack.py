#!/usr/bin/env python3
"""Build apps/<app_id>/app.bin for the gigaSDK application engine."""

import argparse
import binascii
import pathlib
import struct

MAGIC = 0x50504147
FORMAT_VERSION = 1
ABI_VERSION = 1
TARGET_FLASH = 1
LOAD_ADDRESS = 0x08020000
HEADER_SIZE = 256
ICON_WIDTH = 48
ICON_HEIGHT = 48
ICON_SIZE = ICON_WIDTH * ICON_HEIGHT * 2
ICON_OFFSET = HEADER_SIZE
PAYLOAD_OFFSET = ICON_OFFSET + ICON_SIZE
MAX_PAYLOAD_SIZE = 384 * 1024


def fixed_utf8(value: str, size: int, field: str) -> bytes:
    encoded = value.encode("utf-8")
    if len(encoded) >= size:
        raise ValueError(f"{field} must be shorter than {size} UTF-8 bytes")
    return encoded + bytes(size - len(encoded))


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def load_icon(path: pathlib.Path | None, color: str) -> bytes:
    if path is not None:
        data = path.read_bytes()
        if len(data) != ICON_SIZE:
            raise ValueError(
                f"raw RGB565 icon must be exactly {ICON_SIZE} bytes"
            )
        return data
    components = color.split(",")
    if len(components) != 3:
        raise ValueError("--icon-color must have the form R,G,B")
    red, green, blue = (int(component, 0) for component in components)
    if not all(0 <= component <= 255 for component in (red, green, blue)):
        raise ValueError("icon color components must be in range 0..255")
    pixel = struct.pack("<H", rgb565(red, green, blue))
    return pixel * (ICON_WIDTH * ICON_HEIGHT)


def build_header(payload: bytes, name: str, version: str) -> bytes:
    header = bytearray(HEADER_SIZE)
    struct.pack_into(
        "<IHHHHIIIIIIHHHH",
        header,
        0,
        MAGIC,
        FORMAT_VERSION,
        HEADER_SIZE,
        ABI_VERSION,
        TARGET_FLASH,
        LOAD_ADDRESS,
        PAYLOAD_OFFSET,
        len(payload),
        binascii.crc32(payload) & 0xFFFFFFFF,
        ICON_OFFSET,
        ICON_SIZE,
        ICON_WIDTH,
        ICON_HEIGHT,
        1,  # RGB565 little-endian
        0,
    )
    header[44:92] = fixed_utf8(name, 48, "name")
    header[92:108] = fixed_utf8(version, 16, "version")
    struct.pack_into("<I", header, 108, 0)
    struct.pack_into("<I", header, 108, binascii.crc32(header) & 0xFFFFFFFF)
    return bytes(header)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--payload", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--version", default="1.0.0")
    parser.add_argument(
        "--icon", type=pathlib.Path,
        help="48x48 raw RGB565 little-endian file (4608 bytes)",
    )
    parser.add_argument(
        "--icon-color", default="32,96,192",
        help="solid fallback icon as R,G,B",
    )
    args = parser.parse_args()

    payload = args.payload.read_bytes()
    if not 8 <= len(payload) <= MAX_PAYLOAD_SIZE:
        raise ValueError(f"payload must be 8..{MAX_PAYLOAD_SIZE} bytes")
    initial_sp, reset_handler = struct.unpack_from("<II", payload)
    reset_address = reset_handler & ~1
    if not (0x20000000 <= initial_sp <= 0x20020000 and initial_sp % 8 == 0):
        raise ValueError("payload has an invalid initial stack pointer")
    if not (reset_handler & 1 and
            LOAD_ADDRESS <= reset_address < LOAD_ADDRESS + len(payload)):
        raise ValueError("payload has an invalid reset vector")

    icon = load_icon(args.icon, args.icon_color)
    image = build_header(payload, args.name, args.version) + icon + payload
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(image)
    print(
        f"{args.output}: {len(image)} bytes, payload={len(payload)}, "
        f"crc32={binascii.crc32(payload) & 0xFFFFFFFF:08x}"
    )


if __name__ == "__main__":
    main()
