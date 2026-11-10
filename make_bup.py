#!/usr/bin/env python3
"""Make a BUP1 package for XMODEM-CRC (128-byte blocks) transmission."""
import argparse
import pathlib
import struct
import zlib

APP_ADDR = 0x08004000
APP_CAPACITY = 0x6000
STAGE_CAPACITY = 0x5C00
MAGIC = 0x31505542
SRAM_START = 0x20000000
SRAM_END = 0x20005000


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=pathlib.Path, help="APP raw .bin built at 0x08004000")
    parser.add_argument("output", type=pathlib.Path, help="output .bup file")
    parser.add_argument("--version", type=int, default=1)
    a = parser.parse_args()
    app = a.input.read_bytes()
    if not (8 <= len(app) <= min(APP_CAPACITY, STAGE_CAPACITY)):
        parser.error(f"APP size {len(app)} bytes is invalid; maximum 23552 bytes")
    msp, reset = struct.unpack_from("<II", app)
    if not (SRAM_START < msp <= SRAM_END and (msp & 7) == 0):
        parser.error(f"invalid APP MSP: 0x{msp:08X}")
    if (reset & 1) == 0 or not (APP_ADDR <= (reset & ~1) < APP_ADDR + len(app)):
        parser.error(f"invalid APP reset vector: 0x{reset:08X}")
    if not (0 <= a.version <= 0xFFFFFFFF):
        parser.error("version must fit uint32_t")
    crc = zlib.crc32(app) & 0xFFFFFFFF
    hdr = struct.pack("<IIIII", MAGIC, APP_ADDR, len(app), crc, a.version)
    hdr += struct.pack("<I", zlib.crc32(hdr) & 0xFFFFFFFF)
    a.output.write_bytes(hdr + app)
    print(f"Created {a.output} ({len(app)} payload bytes, CRC32=0x{crc:08X})")
    print("Send .bup as XMODEM-CRC, 128-byte blocks (not XMODEM-1K).")


if __name__ == "__main__":
    main()
