#!/usr/bin/env python3
"""Build the small, native Mantle root filesystem module."""

from pathlib import Path
import struct
import sys

MAGIC = 0x31534652  # bytes: RFS1


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: build-rootfs.py OUTPUT DEST=SOURCE ...", file=sys.stderr)
        return 2
    output = Path(sys.argv[1])
    files = []
    for item in sys.argv[2:]:
        destination, separator, source = item.partition("=")
        if not separator or not destination.startswith("/"):
            raise SystemExit(f"invalid rootfs entry: {item}")
        data = Path(source).resolve().read_bytes()
        files.append((destination.encode("ascii"), 0o100755, data))

    table_size = sum(12 + len(path) for path, _, _ in files)
    offset = 8 + table_size
    entries = []
    for path, mode, data in files:
        entries.append(struct.pack("<HHII", len(path), mode, offset, len(data)) + path)
        offset += len(data)

    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + ".part")
    with temporary.open("wb") as stream:
        stream.write(struct.pack("<II", MAGIC, len(files)))
        for entry in entries:
            stream.write(entry)
        for _, _, data in files:
            stream.write(data)
    temporary.replace(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
