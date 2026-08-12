#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/build/out"
ISO="$OUT/mantleos-amd64.iso"
OVMF_VARS="$OUT/OVMF_VARS-interactive.fd"
[ -f "$ISO" ] || { echo "Construire l’image avant le test" >&2; exit 1; }

OVMF_CODE=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
OVMF_VARS_TEMPLATE=${OVMF_VARS_TEMPLATE:-/usr/share/OVMF/OVMF_VARS_4M.fd}
[ -r "$OVMF_CODE" ] || { echo "Firmware UEFI OVMF_CODE absent: $OVMF_CODE" >&2; exit 1; }
[ -r "$OVMF_VARS_TEMPLATE" ] || { echo "Template OVMF_VARS absent: $OVMF_VARS_TEMPLATE" >&2; exit 1; }
cp "$OVMF_VARS_TEMPLATE" "$OVMF_VARS"
[ -w "$OVMF_VARS" ] || { echo "VARS OVMF non writable: $OVMF_VARS" >&2; exit 1; }

qemu-system-x86_64 \
    -machine q35 \
    -accel tcg,thread=single \
    -m 256 \
    -serial stdio \
    -display gtk \
    -drive "if=pflash,format=raw,unit=0,readonly=on,file=$OVMF_CODE" \
    -drive "if=pflash,format=raw,unit=1,file=$OVMF_VARS" \
    -cdrom "$ISO" \
    -no-reboot
