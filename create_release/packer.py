#!/usr/bin/env python3
"""Packs an arbitrary binary blob into the legacy ede_update_file_header_t + payload format
the bootloader expects staged in SPI flash.

Header layout (16 bytes, explicit little-endian, matching bootloader/update_image.h field
order and sizes exactly):

    uint16_t version;
    uint16_t crc16_ccit_checksum;   # CRC over the payload only, not the header
    uint32_t binary_size;           # payload size, excluding this header
    uint8_t  version_major;
    uint8_t  version_minor;
    uint8_t  version_build;
    uint8_t  reserved1;
    uint32_t reserved2;

Usage:
    python packer.py dummy_blob.bin test_image.bin --major 1 --minor 0 --build 3
"""

import argparse
import struct
import sys
from pathlib import Path

from crc16_ccitt import crc16_ccitt, CRC16_CCITT_INITIAL_VALUE

HEADER_FORMAT = "<HHIBBBBI"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
assert HEADER_SIZE == 16, f"header format must pack to 16 bytes, got {HEADER_SIZE}"


def build_header(payload: bytes, version: int, version_major: int, version_minor: int, version_build: int) -> bytes:
    checksum = crc16_ccitt(payload, CRC16_CCITT_INITIAL_VALUE)
    return struct.pack(
        HEADER_FORMAT,
        version,
        checksum,
        len(payload),
        version_major,
        version_minor,
        version_build,
        0,  # reserved1
        0,  # reserved2
    )


def format_header(header: bytes) -> str:
    """Renders a packed header as a human-readable field-by-field dump, in
    ede_update_file_header_t field order."""
    (version, checksum, binary_size, version_major, version_minor, version_build, reserved1,
     reserved2) = struct.unpack(HEADER_FORMAT, header)
    return (
        "ede_update_file_header_t:\n"
        f"  version              = {version} (0x{version:04X})\n"
        f"  crc16_ccit_checksum  = 0x{checksum:04X}\n"
        f"  binary_size          = {binary_size} bytes\n"
        f"  version_major        = {version_major}\n"
        f"  version_minor        = {version_minor}\n"
        f"  version_build        = {version_build}\n"
        f"  reserved1            = {reserved1}\n"
        f"  reserved2            = {reserved2}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input_file", type=Path, help="arbitrary payload file (a dummy blob is fine)")
    parser.add_argument("output_file", type=Path, help="path to write header + payload to")
    parser.add_argument("--version", type=int, default=0, help="raw 16-bit version field (default 0)")
    parser.add_argument("--major", type=int, default=0, help="version_major byte (default 0)")
    parser.add_argument("--minor", type=int, default=0, help="version_minor byte (default 0)")
    parser.add_argument("--build", type=int, default=0, help="version_build byte (default 0)")
    args = parser.parse_args()

    payload = args.input_file.read_bytes()
    header = build_header(payload, args.version, args.major, args.minor, args.build)

    args.output_file.write_bytes(header + payload)

    print(f"wrote {args.output_file} ({len(header) + len(payload)} bytes total: "
          f"{len(header)}-byte header + {len(payload)}-byte payload)")
    print(format_header(header))

    return 0


if __name__ == "__main__":
    sys.exit(main())
