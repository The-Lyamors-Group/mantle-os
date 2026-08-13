#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/build/out"
ISO="$OUT/mantleos-amd64.iso"
[ -f "$ISO" ] || { echo "ISO absente: $ISO" >&2; exit 1; }
[ -f "$OUT/mantleos-amd64.iso.sha256" ] || exit 1
[ -f "$OUT/mantleos-build-info.txt" ] || exit 1
sha256sum -c "$OUT/mantleos-amd64.iso.sha256"
grep -Eq '^boot=UEFI\+(grub|windows-native)$' "$OUT/mantleos-build-info.txt"
grep -q '^kernel=kernel/arch/x86_64$' "$OUT/mantleos-build-info.txt"
if command -v xorriso >/dev/null 2>&1; then
    listing=$(xorriso -indev "$ISO" -find / -type f -exec lsdl 2>/dev/null)
    printf '%s\n' "$listing" | grep -q 'mantle-kernel.elf'
    printf '%s\n' "$listing" | grep -q 'mantle-rootfs.mfs'
    if grep -q '^boot=UEFI+grub$' "$OUT/mantleos-build-info.txt"; then
        printf '%s\n' "$listing" | grep -q 'grub.cfg'
    else
        printf '%s\n' "$listing" | grep -q 'EFI/BOOT/BOOTX64.EFI'
    fi
fi
KERNEL="$ROOT/build/work/kernel/mantle-kernel.elf"
if command -v grub-file >/dev/null 2>&1 && [ -f "$KERNEL" ]; then
    grub-file --is-x86-multiboot2 "$KERNEL"
fi
echo "[verify] image MantleOS indépendante validée"
