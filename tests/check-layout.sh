#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
for path in kernel/arch/x86_64/boot.S kernel/arch/x86_64/kernel.c kernel/linker.ld kernel/Makefile include/mantle/types.h build/make-image.sh boot/grub/grub.cfg tests/verify-image.sh tests/qemu-boot.sh tests/verify-kernel-source.sh sources.lock; do
    [ -f "$ROOT/$path" ] || { echo "Fichier absent: $path" >&2; exit 1; }
done
active=$(cat "$ROOT/build.sh" "$ROOT/build/make-image.sh" "$ROOT/boot/grub/grub.cfg" "$ROOT/.github/workflows/mantleos-build.yml")
if printf '%s\n' "$active" | grep -Eiq 'linux-[0-9]|busybox-[0-9]|musl-[0-9]|cdn\.kernel\.org|busybox\.net|musl\.libc\.org|initramfs|rootfs\.ext4|vmlinuz|(^|[[:space:]])linux[[:space:]]'; then
    echo 'Dépendance Linux active détectée' >&2
    exit 1
fi
grep -q 'multiboot2 /boot/mantle-kernel\.elf' "$ROOT/boot/grub/grub.cfg"
if grep -Eiq 'linux|busybox|musl|https?://' "$ROOT/sources.lock"; then
    echo 'sources.lock contient une dépendance externe interdite' >&2
    exit 1
fi
echo 'MantleOS kernel boundary checks: OK'
