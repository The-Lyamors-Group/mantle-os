#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BOOT="$ROOT/kernel/arch/x86_64/boot.S"
MAIN="$ROOT/kernel/arch/x86_64/kernel.c"
FRAMEBUFFER="$ROOT/kernel/graphics/framebuffer.c"
ELF="$ROOT/kernel/process/elf.c"
SYSCALL="$ROOT/kernel/syscall.c"
MAKEFILE="$ROOT/kernel/Makefile"
grep -q '0xe85250d6' "$BOOT"
grep -q 'long_mode_entry' "$BOOT"
grep -q 'MANTLE_KERNEL_OK' "$MAIN"
grep -q 'MANTLE_GRAPHICS_OK' "$MAIN"
grep -q 'MULTIBOOT_TAG_TYPE_FRAMEBUFFER' "$FRAMEBUFFER"
grep -q 'mantle_enter_user' "$ROOT/kernel/arch/x86_64/boot.S"
grep -q 'MANTLE_USERSPACE_OK' "$SYSCALL"
grep -q 'ELFCLASS64' "$ELF"
grep -q -- '-ffreestanding' "$MAKEFILE"
grep -q -- '-nostdlib' "$MAKEFILE"
if grep -Eq '#include[[:space:]]+[<"](linux|stdio|stdlib|unistd)' "$MAIN"; then
    echo "En-tête de distribution ou libc détecté dans le noyau" >&2
    exit 1
fi
echo "[verify] sources du noyau MantleOS valides"
