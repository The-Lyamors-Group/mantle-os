#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/build/out"
ISO="$OUT/mantleos-amd64.iso"
[ -f "$ISO" ] || { echo "ISO absente: $ISO" >&2; exit 1; }
[ -f "$OUT/mantleos-amd64.iso.sha256" ] || exit 1
[ -f "$OUT/mantleos-build-info.txt" ] || exit 1
sha256sum -c "$OUT/mantleos-amd64.iso.sha256"
grep -q '^boot=UEFI+GRUB+Multiboot2$' "$OUT/mantleos-build-info.txt"
grep -q '^kernel=kernel/arch/x86_64$' "$OUT/mantleos-build-info.txt"
if command -v xorriso >/dev/null 2>&1; then
    listing=$(xorriso -indev "$ISO" -find /boot -type f -exec lsdl 2>/dev/null)
    printf '%s\n' "$listing" | grep -q 'mantle-kernel.elf'
    printf '%s\n' "$listing" | grep -q 'mantle-rootfs.mfs'
    printf '%s\n' "$listing" | grep -q 'grub.cfg'
fi
KERNEL="$ROOT/build/work/kernel/mantle-kernel.elf"
if command -v grub-file >/dev/null 2>&1 && [ -f "$KERNEL" ]; then
    grub-file --is-x86-multiboot2 "$KERNEL"
fi
echo "[verify] image MantleOS indépendante validée"
