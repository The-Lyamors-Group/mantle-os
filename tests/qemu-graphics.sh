#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sh "$ROOT/tests/qemu-boot.sh"
if ! grep -q '^MANTLE_GRAPHICS_OK$' "$ROOT/build/out/qemu-serial.log"; then
    echo "[graphics] framebuffer MantleOS non initialisé" >&2
    exit 1
fi
echo "[graphics] MANTLE_GRAPHICS_OK"
