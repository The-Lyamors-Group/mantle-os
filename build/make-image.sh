#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WORK="$ROOT/build/work"
OUT="$ROOT/build/out"
PROFILE=${MANTLE_PROFILE:-personal}

case "$PROFILE" in
    personal|education|government) ;;
    *) echo "Profil MantleOS inconnu: $PROFILE" >&2; exit 2 ;;
esac

mkdir -p "$OUT"
rm -f "$OUT/mantleos-amd64.iso" "$OUT/mantleos-amd64.iso.sha256" "$OUT/mantleos-build-info.txt"
rm -rf "$WORK"
mkdir -p "$WORK/kernel" "$WORK/iso/boot/grub"

echo "[kernel] compilation du noyau MantleOS x86_64"
make -C "$ROOT/kernel" clean all

KERNEL="$WORK/kernel/mantle-kernel.elf"
if [ ! -f "$KERNEL" ]; then
    echo "[kernel] ERROR: noyau absent après compilation" >&2
    exit 1
fi

if command -v grub-file >/dev/null 2>&1; then
    grub-file --is-x86-multiboot2 "$KERNEL"
else
    echo "[kernel] ERROR: grub-file est requis pour valider Multiboot2" >&2
    exit 1
fi

cp "$KERNEL" "$WORK/iso/boot/mantle-kernel.elf"
cp "$ROOT/boot/grub/grub.cfg" "$WORK/iso/boot/grub/grub.cfg"

echo "[iso] génération de l’image UEFI"
rm -f "$OUT/mantleos-amd64.iso" "$OUT/mantleos-amd64.iso.sha256"
grub-mkrescue -o "$OUT/mantleos-amd64.iso" "$WORK/iso"

sha256sum "$OUT/mantleos-amd64.iso" > "$OUT/mantleos-amd64.iso.sha256"

commit=unknown
if command -v git >/dev/null 2>&1; then
    commit=$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || printf '%s' unknown)
fi
cat > "$OUT/mantleos-build-info.txt" <<EOF
version=MantleOS 0.1-kernel
profile=$PROFILE
architecture=x86_64
boot=UEFI+GRUB+Multiboot2
kernel=kernel/arch/x86_64
commit=$commit
build_date=$(date -u +%Y-%m-%dT%H:%M:%SZ)
iso_sha256=$(cut -d' ' -f1 "$OUT/mantleos-amd64.iso.sha256")
userspace=not-yet-implemented
EOF

echo "[iso] OK: $OUT/mantleos-amd64.iso"
