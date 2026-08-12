#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/build/out"
ISO="$OUT/mantleos-amd64.iso"
SERIAL_LOG="$OUT/qemu-serial.log"
QEMU_LOG="$OUT/qemu.log"
OVMF_VARS="$OUT/OVMF_VARS.fd"

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

OVMF_CODE=${OVMF_CODE:-}
OVMF_VARS_TEMPLATE=${OVMF_VARS_TEMPLATE:-}
if [ -n "$OVMF_CODE" ]; then
    if [ ! -r "$OVMF_CODE" ]; then
        echo "[qemu] ERROR: OVMF_CODE illisible: $OVMF_CODE" >&2
        exit 1
    fi
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
        if [ -r "$candidate" ]; then
            OVMF_CODE=$candidate
            break
        fi
    done
fi

if [ -n "$OVMF_VARS_TEMPLATE" ]; then
    if [ ! -r "$OVMF_VARS_TEMPLATE" ]; then
        echo "[qemu] ERROR: OVMF_VARS_TEMPLATE illisible: $OVMF_VARS_TEMPLATE" >&2
        exit 1
    fi
else
    for candidate in \
        /usr/share/OVMF/OVMF_VARS_4M.fd \
        /usr/share/OVMF/OVMF_VARS.fd \
        /usr/share/ovmf/OVMF_VARS_4M.fd \
        /usr/share/ovmf/OVMF_VARS.fd \
        /usr/share/edk2/ovmf/OVMF_VARS_4M.fd \
        /usr/share/edk2/ovmf/OVMF_VARS.fd \
        /usr/share/qemu/OVMF_VARS_4M.fd \
        /usr/share/qemu/OVMF_VARS.fd; do
        if [ -r "$candidate" ]; then
            OVMF_VARS_TEMPLATE=$candidate
            break
        fi
    done
fi

if [ -z "$OVMF_CODE" ] || [ ! -r "$OVMF_CODE" ]; then
    echo "[qemu] ERROR: aucun OVMF_CODE lisible trouvé" >&2
    exit 1
fi
if [ -z "$OVMF_VARS_TEMPLATE" ] || [ ! -r "$OVMF_VARS_TEMPLATE" ]; then
    echo "[qemu] ERROR: aucun OVMF_VARS_TEMPLATE lisible trouvé" >&2
    exit 1
fi

echo "[qemu] OVMF_CODE: $OVMF_CODE"
echo "[qemu] OVMF_VARS_TEMPLATE: $OVMF_VARS_TEMPLATE"
echo "[qemu] OVMF_VARS: $OVMF_VARS"
if ! cp "$OVMF_VARS_TEMPLATE" "$OVMF_VARS" 2>>"$QEMU_LOG"; then
    echo "[qemu] ERROR: impossible de copier le template OVMF_VARS" >&2
    exit 1
fi
if [ ! -w "$OVMF_VARS" ]; then
    echo "[qemu] ERROR: OVMF_VARS non writable: $OVMF_VARS" >&2
    exit 1
fi
ls -lh "$OVMF_CODE" "$OVMF_VARS_TEMPLATE" "$OVMF_VARS" | tee -a "$QEMU_LOG"
printf '[qemu] firmware: UEFI pflash CODE readonly + VARS writable\n' | tee -a "$QEMU_LOG"

if [ -e /dev/kvm ] && [ -r /dev/kvm ]; then
    set -- qemu-system-x86_64 -enable-kvm
    echo "[qemu] accélération: KVM"
else
    set -- qemu-system-x86_64 -accel tcg,thread=single
    echo "[qemu] accélération: TCG"
fi

set -- "$@" -machine q35 -m 256 -display none -monitor none \
    -serial "file:$SERIAL_LOG" \
    -drive "if=pflash,format=raw,unit=0,readonly=on,file=$OVMF_CODE" \
    -drive "if=pflash,format=raw,unit=1,file=$OVMF_VARS" \
    -cdrom "$ISO" -no-reboot
printf '[qemu] commande:' | tee -a "$QEMU_LOG"
printf ' %s' "$@" | tee -a "$QEMU_LOG"
printf '\n' | tee -a "$QEMU_LOG"

echo "[qemu] phase: lancement QEMU + firmware UEFI, timeout: 35s"
timeout --foreground 35s "$@" >>"$QEMU_LOG" 2>&1 &
QEMU_PID=$!

marker_seen=1
for _ in $(seq 1 35); do
    if grep -q '^MANTLE_KERNEL_OK$' "$SERIAL_LOG" 2>/dev/null; then
        marker_seen=0
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
echo "[qemu] code de sortie QEMU/timeout: $QEMU_STATUS" | tee -a "$QEMU_LOG"

if [ "$marker_seen" -eq 0 ] && { [ "$QEMU_STATUS" -eq 0 ] || [ "$QEMU_STATUS" -eq 124 ]; }; then
    echo "[qemu] phase: kernel MantleOS atteint"
    echo "[qemu] MANTLE_KERNEL_OK"
    exit 0
fi

echo "[qemu] ERROR: boot MantleOS non validé" >&2
if [ "$QEMU_STATUS" -ne 0 ] && [ "$QEMU_STATUS" -ne 124 ]; then
    echo "[qemu] phase en échec: lancement QEMU ou initialisation firmware UEFI" >&2
elif [ ! -s "$SERIAL_LOG" ]; then
    echo "[qemu] phase en échec: firmware UEFI ou bootloader, aucune sortie série" >&2
elif grep -q '^MANTLE_KERNEL_OK$' "$SERIAL_LOG" 2>/dev/null; then
    echo "[qemu] phase en échec: arrêt QEMU après le marqueur kernel" >&2
else
    echo "[qemu] phase en échec: bootloader ou kernel avant MANTLE_KERNEL_OK" >&2
fi
echo "[qemu] init: non applicable — image noyau-only actuelle" >&2
echo "[qemu] rootfs: non applicable — aucun rootfs dans l’image actuelle" >&2
echo "[qemu] --- qemu.log ---" >&2
cat "$QEMU_LOG" >&2 || true
echo "[qemu] --- qemu-serial.log ---" >&2
cat "$SERIAL_LOG" >&2 || true
exit 1
