#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ISO="$ROOT/build/out/mantleos-amd64.iso"
LOG="$ROOT/build/out/qemu-serial.log"
[ -f "$ISO" ] || { echo "ISO absente: $ISO" >&2; exit 1; }
OVMF=${OVMF:-}
for candidate in "$OVMF" /usr/share/OVMF/OVMF_CODE.fd /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/ovmf/OVMF_CODE.fd /usr/share/edk2/ovmf/OVMF_CODE.fd /usr/share/qemu/OVMF_CODE.fd; do
    if [ -n "$candidate" ] && [ -f "$candidate" ]; then OVMF=$candidate; break; fi
done
[ -n "$OVMF" ] && [ -f "$OVMF" ] || { echo "OVMF absent" >&2; exit 1; }
rm -f "$LOG"
if [ -e /dev/kvm ]; then
    qemu-system-x86_64 -enable-kvm -m 256 -display none -monitor none -serial "file:$LOG" -bios "$OVMF" -cdrom "$ISO" -no-reboot &
else
    qemu-system-x86_64 -accel tcg,thread=single -m 256 -display none -monitor none -serial "file:$LOG" -bios "$OVMF" -cdrom "$ISO" -no-reboot &
fi
QEMU_PID=$!
trap 'kill "$QEMU_PID" 2>/dev/null || true' EXIT HUP INT TERM
status=1
for _ in $(seq 1 30); do
    if grep -q '^MANTLE_KERNEL_OK$' "$LOG" 2>/dev/null; then status=0; break; fi
    sleep 1
done
if [ "$status" -ne 0 ]; then
    echo "[qemu] kernel MantleOS non détecté" >&2
    cat "$LOG" >&2 2>/dev/null || true
    exit 1
fi
echo "[qemu] MANTLE_KERNEL_OK"
