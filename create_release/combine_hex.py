#!/usr/bin/env python3
"""Combines the bootloader binary and an application binary into a single Intel HEX file
that can be flashed in one shot (ST-LINK, STM32CubeProgrammer, J-Flash, etc.).

This replaces the old `srec_cat` invocation -- Intel HEX is a plain text record format, so
no external tooling is required, only the standard library.

Output layout (defaults, matching bootloader_config.h / stm32c5a3xg_flash.ld):

    0x08000000  bootloader image      (BOOT_ROM, capped at 128 KB)
    0x08020000  application image     (APP_ROM, APP_FLASH_BASE_ADDR)

The gap between the end of the bootloader and APP_FLASH_BASE_ADDR is intentionally left out
of the file so those pages stay erased rather than being programmed with filler.

Application binaries produced for the SPI-flash update path carry a 16-byte
ede_update_file_header_t (see packer.py) in front of the vector table. That header must NOT
be programmed into internal flash -- the bootloader expects a bare vector table at
APP_FLASH_BASE_ADDR. This script detects such a header and strips it automatically; use
--header auto|strip|keep to override the decision.

This is create_release's own copy, deliberately independent of the bootloader submodule's tools
folder so a release never depends on anything inside that submodule except its source. It shares
only the standard library and the crc16_ccitt.py alongside it.

Usage:
    python combine_hex.py <app_bin> [-o combined.hex]
    python combine_hex.py <app_bin> --bootloader path/to/ede-stm32-bootloader.bin
"""

import argparse
import struct
import sys
from pathlib import Path

from crc16_ccitt import crc16_ccitt, CRC16_CCITT_INITIAL_VALUE

REPO_ROOT = Path(__file__).resolve().parent.parent

# Where the bootloader project drops its binary, relative to the repository root, so the common case
# needs no --bootloader. gen_build.bash passes both paths explicitly and does not rely on this.
DEFAULT_BOOTLOADER_GLOB = "stepper_144pin_cmake/stm32c5-bootloader/build/*/ede-stm32-bootloader.bin"

DEFAULT_BOOT_BASE_ADDR = 0x08000000
DEFAULT_APP_BASE_ADDR = 0x08020000
DEFAULT_BYTES_PER_RECORD = 16

# ede_update_file_header_t, see packer.py -- kept in sync with bootloader/update_image.h.
UPDATE_HEADER_FORMAT = "<HHIBBBBI"
UPDATE_HEADER_SIZE = struct.calcsize(UPDATE_HEADER_FORMAT)

RECORD_TYPE_DATA = 0x00
RECORD_TYPE_EOF = 0x01
RECORD_TYPE_EXTENDED_LINEAR_ADDRESS = 0x04
RECORD_TYPE_START_LINEAR_ADDRESS = 0x05


def make_record(byte_count_payload: bytes, address_offset: int, record_type: int) -> str:
    """Formats one Intel HEX record, including the two's-complement checksum byte."""
    fields = bytes([len(byte_count_payload), (address_offset >> 8) & 0xFF, address_offset & 0xFF,
                    record_type]) + byte_count_payload
    checksum = (-sum(fields)) & 0xFF
    return ":" + (fields + bytes([checksum])).hex().upper()


def emit_data_records(data: bytes, base_address: int, bytes_per_record: int) -> list:
    """Renders `data` as Intel HEX data records starting at `base_address`, inserting an
    extended-linear-address record whenever the upper 16 bits of the address change."""
    records = []
    current_upper_address = None

    for offset in range(0, len(data), bytes_per_record):
        chunk = data[offset:offset + bytes_per_record]
        address = base_address + offset
        upper_address = (address >> 16) & 0xFFFF

        # A record must not straddle a 64 KB boundary, since only the low 16 address bits are
        # carried in the record itself.
        if ((address & 0xFFFF) + len(chunk)) > 0x10000:
            split = 0x10000 - (address & 0xFFFF)
            chunk = chunk[:split]

        if upper_address != current_upper_address:
            records.append(make_record(struct.pack(">H", upper_address), 0,
                                       RECORD_TYPE_EXTENDED_LINEAR_ADDRESS))
            current_upper_address = upper_address

        records.append(make_record(chunk, address & 0xFFFF, RECORD_TYPE_DATA))

    return records


def looks_like_update_header(image: bytes) -> bool:
    """Returns True if `image` starts with a plausible ede_update_file_header_t: the declared
    payload size accounts for the rest of the file and the CRC over that payload matches."""
    result = False

    if len(image) > UPDATE_HEADER_SIZE:
        (_version, checksum, binary_size, _major, _minor, _build, _reserved1,
         _reserved2) = struct.unpack(UPDATE_HEADER_FORMAT, image[:UPDATE_HEADER_SIZE])
        payload = image[UPDATE_HEADER_SIZE:]
        if binary_size == len(payload):
            result = (crc16_ccitt(payload, CRC16_CCITT_INITIAL_VALUE) == checksum)

    return result


def describe_update_header(image: bytes) -> str:
    (version, checksum, binary_size, major, minor, build, _reserved1,
     _reserved2) = struct.unpack(UPDATE_HEADER_FORMAT, image[:UPDATE_HEADER_SIZE])
    return (f"ede_update_file_header_t: v{major}.{minor}.{build} "
            f"(version={version}, crc=0x{checksum:04X}, binary_size={binary_size})")


def find_default_bootloader_bin() -> Path:
    """Locates the single bootloader .bin in the bootloader project's build tree, so the common case
    needs no --bootloader. Returns a non-existent placeholder path if the search is ambiguous or
    empty, letting the caller report a normal 'not found' error."""
    candidates = sorted(REPO_ROOT.glob(DEFAULT_BOOTLOADER_GLOB))

    if len(candidates) == 1:
        result = candidates[0]
    else:
        result = REPO_ROOT / DEFAULT_BOOTLOADER_GLOB.replace("/*/", "/")

    return result


def load_images(args: argparse.Namespace) -> tuple:
    """Reads both binaries, applying the update-header policy to the application image.
    Returns (bootloader_bytes, app_bytes)."""
    bootloader = args.bootloader.read_bytes()
    app = args.app_bin.read_bytes()

    has_header = looks_like_update_header(app)
    if has_header:
        print(f"{args.app_bin.name}: {describe_update_header(app)}")

    if args.header == "strip":
        strip = True
    elif args.header == "keep":
        strip = False
    else:
        strip = has_header

    if strip:
        if len(app) <= UPDATE_HEADER_SIZE:
            raise ValueError(f"{args.app_bin} is too small to contain a {UPDATE_HEADER_SIZE}-byte "
                             "update header")
        app = app[UPDATE_HEADER_SIZE:]
        print(f"stripped {UPDATE_HEADER_SIZE}-byte update header from application image")
    elif has_header:
        print("WARNING: update header detected but kept (--header keep); the bootloader expects a "
              "bare vector table at the application base address")

    return bootloader, app


def check_layout(bootloader: bytes, app: bytes, args: argparse.Namespace) -> None:
    """Validates that the two images fit their partitions and do not overlap."""
    boot_end = args.boot_addr + len(bootloader)
    app_end = args.app_addr + len(app)

    if boot_end > args.app_addr:
        raise ValueError(f"bootloader ({len(bootloader)} bytes) ends at 0x{boot_end:08X}, past the "
                         f"application base 0x{args.app_addr:08X}")

    if app_end > args.app_end_addr:
        raise ValueError(f"application ({len(app)} bytes) ends at 0x{app_end:08X}, past the flash "
                         f"end 0x{args.app_end_addr:08X}")

    if len(app) >= 8:
        reset_vector = struct.unpack("<I", app[4:8])[0]
        if (reset_vector < args.app_addr) or (reset_vector >= args.app_end_addr):
            print(f"WARNING: application reset vector 0x{reset_vector:08X} is outside "
                  f"0x{args.app_addr:08X}-0x{args.app_end_addr:08X}; the image may not be linked "
                  "for the application partition")


def build_hex(bootloader: bytes, app: bytes, args: argparse.Namespace) -> str:
    records = []
    records += emit_data_records(bootloader, args.boot_addr, args.bytes_per_record)
    records += emit_data_records(app, args.app_addr, args.bytes_per_record)

    # Entry point is the bootloader's reset vector, matching what objcopy emits for the
    # bootloader-only .hex.
    if len(bootloader) >= 8:
        boot_reset_vector = struct.unpack("<I", bootloader[4:8])[0]
        records.append(make_record(struct.pack(">I", boot_reset_vector), 0,
                                   RECORD_TYPE_START_LINEAR_ADDRESS))

    records.append(make_record(b"", 0, RECORD_TYPE_EOF))

    return "\n".join(records) + "\n"


def parse_int(text: str) -> int:
    return int(text, 0)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("app_bin", type=Path, help="application binary to place at --app-addr")
    parser.add_argument("-o", "--output", type=Path, default=Path("combined.hex"),
                        help="path to write the combined Intel HEX to (default combined.hex)")
    parser.add_argument("-b", "--bootloader", type=Path, default=find_default_bootloader_bin(),
                        help="bootloader binary to place at --boot-addr "
                             "(default: the single build/*/ede-stm32-bootloader.bin)")
    parser.add_argument("--boot-addr", type=parse_int, default=DEFAULT_BOOT_BASE_ADDR,
                        help=f"bootloader base address (default 0x{DEFAULT_BOOT_BASE_ADDR:08X})")
    parser.add_argument("--app-addr", type=parse_int, default=DEFAULT_APP_BASE_ADDR,
                        help=f"APP_FLASH_BASE_ADDR (default 0x{DEFAULT_APP_BASE_ADDR:08X})")
    parser.add_argument("--app-end-addr", type=parse_int, default=0x08100000,
                        help="APP_FLASH_END_ADDR, one past the last usable byte (default 0x08100000)")
    parser.add_argument("--header", choices=("auto", "strip", "keep"), default="auto",
                        help="what to do with an ede_update_file_header_t on the application "
                             "image (default auto: strip it when one is detected)")
    parser.add_argument("--bytes-per-record", type=int, default=DEFAULT_BYTES_PER_RECORD,
                        help=f"data bytes per HEX record (default {DEFAULT_BYTES_PER_RECORD})")
    args = parser.parse_args()

    if (args.bytes_per_record < 1) or (args.bytes_per_record > 255):
        print("error: --bytes-per-record must be between 1 and 255", file=sys.stderr)
        return 2

    for path in (args.bootloader, args.app_bin):
        if not path.is_file():
            print(f"error: {path} not found", file=sys.stderr)
            return 1

    try:
        bootloader, app = load_images(args)
        check_layout(bootloader, app, args)
        hex_text = build_hex(bootloader, app, args)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    args.output.write_text(hex_text, encoding="ascii", newline="\n")

    print(f"bootloader  0x{args.boot_addr:08X}-0x{args.boot_addr + len(bootloader) - 1:08X} "
          f"({len(bootloader)} bytes)  {args.bootloader}")
    print(f"application 0x{args.app_addr:08X}-0x{args.app_addr + len(app) - 1:08X} "
          f"({len(app)} bytes)  {args.app_bin}")
    print(f"wrote {args.output} ({len(hex_text.splitlines())} records)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
