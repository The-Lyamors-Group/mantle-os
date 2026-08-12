#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ISO="$ROOT/build/out/mantleos-amd64.iso"
DISK="$ROOT/build/out/mantleos-disk.img"
OVMF=${OVMF:-/usr/share/OVMF/OVMF_CODE.fd}
LOG="$ROOT/build/out/qemu-boot.log"
[ -s "$ISO" ] && [ -s "$DISK" ] && [ -f "$OVMF" ] || { echo 'Construire l’image et installer OVMF avant le test' >&2; exit 1; }
set +e
if [ -e /dev/kvm ]; then accel='-enable-kvm'; accel_arg=''; else accel='-accel'; accel_arg='tcg,thread=single'; fi
timeout 45s qemu-system-x86_64 $accel $accel_arg -m 1024 -smp 2 -display none -monitor none -serial file:"$LOG" -bios "$OVMF" -cdrom "$ISO" -drive file="$DISK",format=raw,if=virtio -netdev user,id=net0 -device virtio-net-pci,netdev=net0 -no-reboot
code=$?
set -e
grep -q '^MANTLE_KERNEL_OK$' "$LOG"
grep -q '^MANTLE_INIT_OK$' "$LOG"
grep -q '^MANTLE_ROOTFS_OK$' "$LOG"
grep -q '^MANTLE_SERVICES_OK$' "$LOG"
grep -q '^MANTLE_NETWORK_OK$' "$LOG"
echo 'MantleOS QEMU boot checks: OK'
