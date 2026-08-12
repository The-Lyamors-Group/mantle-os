#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BOOT="$ROOT/kernel/arch/x86_64/boot.S"
MAIN="$ROOT/kernel/arch/x86_64/kernel.c"
MAKEFILE="$ROOT/kernel/Makefile"
grep -q '0xe85250d6' "$BOOT"
grep -q 'long_mode_entry' "$BOOT"
grep -q 'MANTLE_KERNEL_OK' "$MAIN"
grep -q -- '-ffreestanding' "$MAKEFILE"
grep -q -- '-nostdlib' "$MAKEFILE"
if grep -Eq '#include[[:space:]]+[<"](linux|stdio|stdlib|unistd)' "$MAIN"; then
    echo "En-tête de distribution ou libc détecté dans le noyau" >&2
    exit 1
fi
echo "[verify] sources du noyau MantleOS valides"
