#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ISO="$ROOT/build/out/mantleos-amd64.iso"
[ -f "$ISO" ] || { echo "Construire l’image avant le test" >&2; exit 1; }
OVMF=${OVMF:-}
for candidate in "$OVMF" /usr/share/OVMF/OVMF_CODE.fd /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/ovmf/OVMF_CODE.fd /usr/share/edk2/ovmf/OVMF_CODE.fd /usr/share/qemu/OVMF_CODE.fd; do
    if [ -n "$candidate" ] && [ -f "$candidate" ]; then OVMF=$candidate; break; fi
done
[ -n "$OVMF" ] && [ -f "$OVMF" ] || { echo "Firmware UEFI OVMF absent" >&2; exit 1; }
qemu-system-x86_64 -m 256 -serial stdio -display gtk -bios "$OVMF" -cdrom "$ISO" -no-reboot
