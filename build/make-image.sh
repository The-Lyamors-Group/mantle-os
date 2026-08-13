#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WORK="$ROOT/build/work"
OUT="$ROOT/build/out"
PROFILE=${MANTLE_PROFILE:-personal}
ISO_BACKEND=${MANTLE_ISO_BACKEND:-auto}

if command -v x86_64-elf-gcc >/dev/null 2>&1 && command -v x86_64-elf-ld >/dev/null 2>&1; then
    MANTLE_CC='x86_64-elf-gcc'
    MANTLE_LD='x86_64-elf-ld'
elif command -v clang >/dev/null 2>&1 && command -v ld.lld >/dev/null 2>&1 &&
     clang --target=x86_64-elf -print-target-triple 2>/dev/null | grep -Eq '^x86_64-.*-elf$'; then
    MANTLE_CC='clang --target=x86_64-elf'
    MANTLE_LD='ld.lld'
elif command -v gcc >/dev/null 2>&1 && command -v ld >/dev/null 2>&1 &&
     [ "$(gcc -dumpmachine 2>/dev/null)" != x86_64-pc-cygwin ]; then
    MANTLE_CC='gcc'
    MANTLE_LD='ld'
else
    echo "[toolchain] ERROR: aucun compilateur ELF x86_64 compatible MantleOS" >&2
    echo "[toolchain] MSYS2: installe mingw-w64-x86_64-clang et mingw-w64-x86_64-lld, puis utilise /mingw64/bin." >&2
    exit 1
fi
export MANTLE_CC MANTLE_LD
echo "[toolchain] CC=$MANTLE_CC LD=$MANTLE_LD"

case "$PROFILE" in
    personal|education|government) ;;
    *) echo "Profil MantleOS inconnu: $PROFILE" >&2; exit 2 ;;
esac

mkdir -p "$OUT"
rm -f "$OUT/mantleos-amd64.iso" "$OUT/mantleos-amd64.iso.sha256" "$OUT/mantleos-build-info.txt"
rm -rf "$WORK"
mkdir -p "$WORK/kernel" "$WORK/userspace" "$WORK/iso/boot/grub"

echo "[userspace] compilation des programmes MantleOS x86_64"
# BUILD reste relatif au répertoire choisi par -C. Il ne devient donc jamais
# une liste de cibles Make contenant les espaces du chemin Windows du dépôt.
make -C "$ROOT/userspace" BUILD="../build/work/userspace" CC="$MANTLE_CC" LD="$MANTLE_LD" clean all

if command -v python3 >/dev/null 2>&1; then
    PYTHON=python3
elif command -v python >/dev/null 2>&1; then
    PYTHON=python
else
    echo "[rootfs] ERROR: python3 ou python est requis" >&2
    exit 1
fi

echo "[rootfs] génération du module filesystem MantleOS"
"$PYTHON" "$ROOT/userspace/build-rootfs.py" "$WORK/iso/boot/mantle-rootfs.mfs" \
    /sbin/init="$WORK/userspace/init.elf" \
    /bin/hello="$WORK/userspace/hello.elf" \
    /bin/mantle="$WORK/userspace/mantle.elf" \
    /bin/mantle-shell="$WORK/userspace/mantle-shell.elf"
for program in init hello mantle mantle-shell; do
    test -s "$WORK/userspace/$program.elf"
done
if command -v readelf >/dev/null 2>&1; then
    for program in init hello mantle mantle-shell; do
        readelf -h "$WORK/userspace/$program.elf" | grep -Eq 'Class:[[:space:]]+ELF64'
        readelf -h "$WORK/userspace/$program.elf" | grep -Eq 'Machine:[[:space:]]+Advanced Micro Devices X86-64'
    done
fi

echo "[kernel] compilation du noyau MantleOS x86_64"
make -C "$ROOT/kernel" BUILD="../build/work/kernel" CC="$MANTLE_CC" LD="$MANTLE_LD" clean all

KERNEL="$WORK/kernel/mantle-kernel.elf"
if [ ! -f "$KERNEL" ]; then
    echo "[kernel] ERROR: noyau absent après compilation" >&2
    exit 1
fi

build_native_loader()
{
    mkdir -p "$WORK/efi"
    clang --target=x86_64-pc-windows-msvc -DMS_WIN64 -DHAVE_USE_MS_ABI \
        -ffreestanding -fshort-wchar -fno-stack-protector -mno-red-zone \
        -I/usr/include/efi -I/usr/include/efi/x86_64 \
        -c "$ROOT/boot/efi/loader.c" -o "$WORK/efi/loader.o"
    lld-link -subsystem:efi_application -entry:efi_main -nodefaultlib \
        -out:"$WORK/efi/BOOTX64.EFI" "$WORK/efi/loader.o"
}

if command -v grub-mkrescue >/dev/null 2>&1 && command -v grub-file >/dev/null 2>&1; then
    DETECTED_BACKEND=grub
elif command -v xorriso >/dev/null 2>&1; then
    if [ -f "$ROOT/boot/efi/loader.c" ] && command -v clang >/dev/null 2>&1 && command -v lld-link >/dev/null 2>&1; then
        DETECTED_BACKEND=windows-native
    else
        echo "[iso] ERROR: xorriso est disponible mais boot/efi/BOOTX64.EFI est absent" >&2
        echo "[iso] xorriso seul ne peut pas produire une ISO UEFI amorçable" >&2
        exit 1
    fi
else
    echo "[iso] ERROR: aucun backend disponible" >&2
    echo "[iso] GRUB nécessite grub-mkrescue + grub-file; windows-native nécessite boot/efi/BOOTX64.EFI + xorriso." >&2
    exit 1
fi

if [ "$ISO_BACKEND" != auto ] && [ "$ISO_BACKEND" != "$DETECTED_BACKEND" ]; then
    echo "[iso] ERROR: backend demandé '$ISO_BACKEND' indisponible (détecté: $DETECTED_BACKEND)" >&2
    exit 1
fi
ISO_BACKEND=$DETECTED_BACKEND
echo "[iso] backend: $ISO_BACKEND"

if [ "$ISO_BACKEND" = grub ]; then
    grub-file --is-x86-multiboot2 "$KERNEL"
fi

cp "$KERNEL" "$WORK/iso/boot/mantle-kernel.elf"
cp "$ROOT/boot/grub/grub.cfg" "$WORK/iso/boot/grub/grub.cfg"

echo "[iso] génération de l’image UEFI"
rm -f "$OUT/mantleos-amd64.iso" "$OUT/mantleos-amd64.iso.sha256"
if [ "$ISO_BACKEND" = grub ]; then
    grub-mkrescue -o "$OUT/mantleos-amd64.iso" "$WORK/iso"
else
    build_native_loader
    mkdir -p "$WORK/iso/EFI/BOOT"
    cp "$WORK/efi/BOOTX64.EFI" "$WORK/iso/EFI/BOOT/BOOTX64.EFI"
    "$PYTHON" "$ROOT/boot/efi/make-esp.py" "$WORK/efi/esp.img" "$WORK/efi/BOOTX64.EFI"
    cp "$WORK/efi/esp.img" "$WORK/iso/EFI/esp.img"
    xorriso -as mkisofs -R -J -V MANTLEOS \
        -e EFI/esp.img -no-emul-boot \
        -o "$OUT/mantleos-amd64.iso" "$WORK/iso"
fi

sha256sum "$OUT/mantleos-amd64.iso" > "$OUT/mantleos-amd64.iso.sha256"

commit=unknown
if command -v git >/dev/null 2>&1; then
    commit=$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || printf '%s' unknown)
fi
cat > "$OUT/mantleos-build-info.txt" <<EOF
version=MantleOS 0.1-kernel
profile=$PROFILE
architecture=x86_64
boot=UEFI+$ISO_BACKEND
kernel=kernel/arch/x86_64
commit=$commit
build_date=$(date -u +%Y-%m-%dT%H:%M:%SZ)
iso_sha256=$(cut -d' ' -f1 "$OUT/mantleos-amd64.iso.sha256")
userspace=mantle-rootfs-module-ring3
programs=/sbin/init,/bin/hello,/bin/mantle,/bin/mantle-shell
EOF

echo "[iso] OK: $OUT/mantleos-amd64.iso"
