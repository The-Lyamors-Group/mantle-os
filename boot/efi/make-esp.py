#!/usr/bin/env python3
"""Create the FAT16 EFI System Partition used by the native ISO path."""

import struct
import sys
from pathlib import Path


SECTOR = 512
TOTAL_SECTORS = 32768
ROOT_ENTRIES = 512
FAT_SECTORS = 128
ROOT_SECTORS = (ROOT_ENTRIES * 32 + SECTOR - 1) // SECTOR
DATA_SECTOR = 1 + 2 * FAT_SECTORS + ROOT_SECTORS


def fat16_set(fat: bytearray, cluster: int, value: int) -> None:
    struct.pack_into("<H", fat, cluster * 2, value)


def directory_entry(name: bytes, attr: int, cluster: int, size: int = 0) -> bytes:
    entry = bytearray(32)
    entry[0:11] = name.ljust(11, b" ")[:11]
    entry[11] = attr
    struct.pack_into("<H", entry, 26, cluster)
    struct.pack_into("<I", entry, 28, size)
    return bytes(entry)


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} OUTPUT IMAGE", file=sys.stderr)
        return 2
    output = Path(sys.argv[1])
    payload = Path(sys.argv[2]).read_bytes()
    cluster_count = (len(payload) + SECTOR - 1) // SECTOR
    if cluster_count == 0 or cluster_count > 65524:
        raise SystemExit("EFI loader does not fit in FAT16 ESP")

    image = bytearray(SECTOR * TOTAL_SECTORS)
    image[0:3] = b"\xEB\x3C\x90"
    image[3:11] = b"MANTLEOS"
    struct.pack_into("<H", image, 11, SECTOR)
    image[13] = 1
    struct.pack_into("<H", image, 14, 1)
    image[16] = 2
    struct.pack_into("<H", image, 17, ROOT_ENTRIES)
    struct.pack_into("<H", image, 19, 0)
    image[21] = 0xF8
    struct.pack_into("<H", image, 22, FAT_SECTORS)
    struct.pack_into("<H", image, 24, 18)
    struct.pack_into("<H", image, 26, 2)
    image[38] = 0x29
    struct.pack_into("<I", image, 39, 0x4D544F53)
    image[43:54] = b"MANTLE EFI "
    image[54:62] = b"FAT16   "
    struct.pack_into("<I", image, 32, TOTAL_SECTORS)
    image[510:512] = b"\x55\xAA"

    fat = bytearray(FAT_SECTORS * SECTOR)
    struct.pack_into("<H", fat, 0, 0xFFF8)
    struct.pack_into("<H", fat, 2, 0xFFFF)
    fat16_set(fat, 2, 0xFFFF)
    fat16_set(fat, 3, 0xFFFF)
    for cluster in range(cluster_count):
        fat16_set(fat, 4 + cluster, 0xFFFF if cluster + 1 == cluster_count else 5 + cluster)
    image[SECTOR:SECTOR + len(fat)] = fat
    image[SECTOR * (1 + FAT_SECTORS):SECTOR * (1 + 2 * FAT_SECTORS)] = fat

    root = SECTOR * (1 + 2 * FAT_SECTORS)
    image[root:root + 32] = directory_entry(b"EFI", 0x10, 2)
    efi = SECTOR * DATA_SECTOR
    # FAT directory streams must contain the self and parent entries.  Some
    # firmware accepts their omission, but OVMF's FAT driver is stricter when
    # the directory is used as an El Torito EFI System Partition.
    image[efi:efi + 32] = directory_entry(b".          ", 0x10, 2)
    image[efi + 32:efi + 64] = directory_entry(b"..         ", 0x10, 0)
    image[efi + 64:efi + 96] = directory_entry(b"BOOT", 0x10, 3)
    boot = efi + SECTOR
    image[boot:boot + 32] = directory_entry(b".          ", 0x10, 3)
    image[boot + 32:boot + 64] = directory_entry(b"..         ", 0x10, 2)
    image[boot + 64:boot + 96] = directory_entry(b"BOOTX64 EFI", 0x20, 4, len(payload))
    payload_offset = efi + 2 * SECTOR
    image[payload_offset:payload_offset + len(payload)] = payload
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(image)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
