#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ISO="$ROOT/build/out/mantleos-amd64.iso"
[ -f "$ISO" ] || { echo "Construire l’image avant le test" >&2; exit 1; }
DISK="$ROOT/build/out/mantleos-disk.img"
[ -f "$DISK" ] || { echo "Disque root MantleOS absent" >&2; exit 1; }
OVMF=${OVMF:-/usr/share/OVMF/OVMF_CODE.fd}
[ -f "$OVMF" ] || { echo "Firmware UEFI OVMF absent: $OVMF" >&2; exit 1; }
qemu-system-x86_64 -enable-kvm -m 1024 -smp 2 -serial stdio -display gtk -bios "$OVMF" -cdrom "$ISO" -drive file="$DISK",format=raw,if=virtio -netdev user,id=net0 -device virtio-net-pci,netdev=net0
