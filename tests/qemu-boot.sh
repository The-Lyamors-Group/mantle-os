#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/build/out"
ISO="$OUT/mantleos-amd64.iso"
SERIAL_LOG="$OUT/qemu-serial.log"
QEMU_LOG="$OUT/qemu.log"

mkdir -p "$OUT"
: > "$SERIAL_LOG"
: > "$QEMU_LOG"

echo "[qemu] ISO: $ISO"
if [ ! -f "$ISO" ]; then
    echo "[qemu] ERROR: ISO absente: $ISO" >&2
    exit 1
fi

echo "[qemu] diagnostic: qemu-system-x86_64 --version"
qemu-system-x86_64 --version | tee -a "$QEMU_LOG"
echo "[qemu] diagnostic: ISO"
ls -lh "$ISO" | tee -a "$QEMU_LOG"
file "$ISO" | tee -a "$QEMU_LOG"
echo "[qemu] diagnostic: OVMF files"
find /usr/share/OVMF /usr/share/ovmf -maxdepth 2 -type f 2>/dev/null | tee -a "$QEMU_LOG" || true

OVMF=${OVMF:-}
if [ -n "$OVMF" ]; then
    [ -f "$OVMF" ] || { echo "[qemu] ERROR: OVMF introuvable: $OVMF" >&2; exit 1; }
else
    for candidate in \
        /usr/share/OVMF/OVMF_CODE_4M.fd \
        /usr/share/OVMF/OVMF_CODE.fd \
        /usr/share/ovmf/OVMF_CODE_4M.fd \
        /usr/share/ovmf/OVMF_CODE.fd \
        /usr/share/edk2/ovmf/OVMF_CODE_4M.fd \
        /usr/share/edk2/ovmf/OVMF_CODE.fd \
        /usr/share/qemu/OVMF_CODE_4M.fd \
        /usr/share/qemu/OVMF_CODE.fd; do
        if [ -f "$candidate" ]; then
            OVMF=$candidate
            break
        fi
    done
fi
[ -n "$OVMF" ] && [ -f "$OVMF" ] || { echo "[qemu] ERROR: aucun firmware OVMF_CODE trouvé" >&2; exit 1; }
echo "[qemu] OVMF: $OVMF"

if [ -e /dev/kvm ] && [ -r /dev/kvm ]; then
    set -- qemu-system-x86_64 -enable-kvm
    echo "[qemu] accélération: KVM"
else
    set -- qemu-system-x86_64 -accel tcg,thread=single
    echo "[qemu] accélération: TCG"
fi

set -- "$@" -machine q35 -m 256 -display none -monitor none \
    -serial "file:$SERIAL_LOG" -bios "$OVMF" -cdrom "$ISO" -no-reboot
printf '[qemu] commande:' | tee -a "$QEMU_LOG"
printf ' %s' "$@" | tee -a "$QEMU_LOG"
printf '\n' | tee -a "$QEMU_LOG"

echo "[qemu] lancement, timeout: 35s"
timeout --foreground 35s "$@" >>"$QEMU_LOG" 2>&1 &
QEMU_PID=$!

status=1
for _ in $(seq 1 35); do
    if grep -q '^MANTLE_KERNEL_OK$' "$SERIAL_LOG" 2>/dev/null; then
        status=0
        break
    fi
    if ! kill -0 "$QEMU_PID" 2>/dev/null; then
        break
    fi
    sleep 1
done

set +e
wait "$QEMU_PID"
QEMU_STATUS=$?
set -e
echo "[qemu] code de sortie: $QEMU_STATUS" | tee -a "$QEMU_LOG"

if [ "$status" -eq 0 ] && { [ "$QEMU_STATUS" -eq 0 ] || [ "$QEMU_STATUS" -eq 124 ]; }; then
    echo "[qemu] MANTLE_KERNEL_OK"
    exit 0
fi

echo "[qemu] ERROR: le kernel MantleOS n’a pas démarré correctement" >&2
echo "[qemu] --- qemu.log ---" >&2
cat "$QEMU_LOG" >&2 || true
echo "[qemu] --- qemu-serial.log ---" >&2
cat "$SERIAL_LOG" >&2 || true
exit 1
