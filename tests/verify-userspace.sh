#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD="$ROOT/build/work/userspace"
ROOTFS="$ROOT/build/work/iso/boot/mantle-rootfs.mfs"

for program in init hello mantle mantle-shell; do
    file="$BUILD/$program.elf"
    [ -s "$file" ] || { echo "userspace absent: $file" >&2; exit 1; }
    file "$file" | grep -Eq 'ELF 64-bit.*x86-64'
    readelf -h "$file" | grep -Eq 'Type:[[:space:]]+EXEC'
    readelf -h "$file" | grep -Eq 'Machine:[[:space:]]+Advanced Micro Devices X86-64'
done

[ -s "$ROOTFS" ] || { echo "module rootfs absent: $ROOTFS" >&2; exit 1; }
python3 - "$ROOTFS" <<'PY'
import struct
import sys

data = open(sys.argv[1], "rb").read()
magic, count = struct.unpack_from("<II", data)
assert magic == 0x31534652 and count == 4
cursor = 8
names = []
for _ in range(count):
    length, mode, offset, size = struct.unpack_from("<HHII", data, cursor)
    cursor += 12
    name = data[cursor:cursor + length].decode("ascii")
    cursor += length
    assert name.startswith("/") and not name.startswith("//")
    assert offset + size <= len(data)
    names.append(name)
assert names == ["/sbin/init", "/bin/hello", "/bin/mantle", "/bin/mantle-shell"]
PY
echo "[verify] MantleOS userspace ELF et rootfs valides"
