#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WORK="$ROOT/build/work"
OUT="$ROOT/build/out"
mkdir -p "$WORK/src" "$OUT" "$WORK/rootfs" "$WORK/iso/boot/grub"

LINUX_VERSION=${LINUX_VERSION:-6.12.60}
BUSYBOX_VERSION=${BUSYBOX_VERSION:-1.36.1}
MUSL_VERSION=${MUSL_VERSION:-1.2.5}
fetch() { url=$1; file=$2; [ -f "$file" ] || curl -fL --retry 3 --output "$file" "$url"; }

fetch "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-$LINUX_VERSION.tar.xz" "$WORK/src/linux-$LINUX_VERSION.tar.xz"
fetch "https://busybox.net/downloads/busybox-$BUSYBOX_VERSION.tar.bz2" "$WORK/src/busybox-$BUSYBOX_VERSION.tar.bz2"
fetch "https://musl.libc.org/releases/musl-$MUSL_VERSION.tar.gz" "$WORK/src/musl-$MUSL_VERSION.tar.gz"
[ -d "$WORK/linux-$LINUX_VERSION" ] || tar -xf "$WORK/src/linux-$LINUX_VERSION.tar.xz" -C "$WORK"
[ -d "$WORK/busybox-$BUSYBOX_VERSION" ] || tar -xf "$WORK/src/busybox-$BUSYBOX_VERSION.tar.bz2" -C "$WORK"
[ -d "$WORK/musl-$MUSL_VERSION" ] || tar -xf "$WORK/src/musl-$MUSL_VERSION.tar.gz" -C "$WORK"

K="$WORK/linux-$LINUX_VERSION"
B="$WORK/busybox-$BUSYBOX_VERSION"
M="$WORK/musl-$MUSL_VERSION"
make -C "$K" O="$WORK/kernel" ARCH=x86_64 mrproper
cp "$ROOT/kernel/config.x86_64" "$WORK/kernel/.config"
make -C "$K" O="$WORK/kernel" ARCH=x86_64 olddefconfig
make -C "$K" O="$WORK/kernel" ARCH=x86_64 -j"$(nproc)" bzImage

rm -rf "$WORK/sysroot"
mkdir -p "$WORK/sysroot"
(cd "$M" && ./configure --prefix="$WORK/sysroot" --disable-shared --enable-static)
make -C "$M" -j"$(nproc)"
make -C "$M" install

make -C "$B" distclean
make -C "$B" defconfig
sed -i 's/^# CONFIG_STATIC is not set/CONFIG_STATIC=y/' "$B/.config"
sed -i 's/^# CONFIG_FEATURE_SHADOWPASSWDS is not set/CONFIG_FEATURE_SHADOWPASSWDS=y/' "$B/.config"
sed -i 's/^# CONFIG_PASSWD is not set/CONFIG_PASSWD=y/' "$B/.config"
sed -i 's/^# CONFIG_ADDUSER is not set/CONFIG_ADDUSER=y/' "$B/.config"
sed -i 's/^# CONFIG_ADDGROUP is not set/CONFIG_ADDGROUP=y/' "$B/.config"
sed -i 's/^# CONFIG_HTTPD is not set/CONFIG_HTTPD=y/' "$B/.config"
sed -i 's/^# CONFIG_SHA256SUM is not set/CONFIG_SHA256SUM=y/' "$B/.config"
sed -i 's/^# CONFIG_SU is not set/CONFIG_SU=y/' "$B/.config"
sed -i 's/^# CONFIG_LOGIN is not set/CONFIG_LOGIN=y/' "$B/.config"
sed -i 's/^CONFIG_PREFIX=.*/CONFIG_PREFIX="\/"/' "$B/.config"
make -C "$B" olddefconfig
make -C "$B" CC="$WORK/sysroot/bin/musl-gcc" -j"$(nproc)"
rm -rf "$WORK/rootfs" "$WORK/initramfs"
mkdir -p "$WORK/rootfs/bin" "$WORK/rootfs/sbin" "$WORK/rootfs/usr/bin" "$WORK/rootfs/usr/sbin" "$WORK/rootfs/usr/lib" "$WORK/rootfs/usr/include" "$WORK/rootfs/lib" "$WORK/rootfs/dev" "$WORK/rootfs/proc" "$WORK/rootfs/sys" "$WORK/rootfs/run" "$WORK/rootfs/tmp" "$WORK/rootfs/etc" "$WORK/rootfs/root"
make -C "$B" CONFIG_PREFIX="$WORK/rootfs" install

CC="$WORK/sysroot/bin/musl-gcc"
STRICT_CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -Werror -fstack-protector-strong -D_FORTIFY_SOURCE=2"
COMPAT_PROFILE=${MANTLE_COMPAT:-full}
sh "$ROOT/compat/build-compat.sh" "$WORK/rootfs" "$WORK/sysroot" "$WORK/src" "$CC" "$COMPAT_PROFILE"
if [ "${MANTLE_SDK:-0}" = 1 ]; then sh "$ROOT/compat/build-sdk.sh" "$WORK/rootfs" "$WORK/sysroot" "$WORK/src" "$CC"; fi
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/sbin/mantle-init" "$ROOT/system/mantle-root-init.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/sbin/mantle-supervise" "$ROOT/services/mantle-supervise.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/bin/mantlectl" "$ROOT/services/mantlectl.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/usr/bin/mantle" "$ROOT/services/mantle.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/bin/mantle-shell" "$ROOT/shell/mantle-shell.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/usr/bin/mantle-script" "$ROOT/scripting/mantle-script.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/usr/bin/mantle-command" "$ROOT/scripting/mantle-command.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/sbin/mantle-network" "$ROOT/system/mantle-network.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/sbin/mantle-logd" "$ROOT/system/mantle-logd.c"
$CC $STRICT_CFLAGS -static -Os -Wl,-z,relro,-z,now -o "$WORK/rootfs/sbin/mantle-splash" "$ROOT/boot/splash/mantle-splash.c"
chmod 0755 "$WORK/rootfs/sbin/mantle-init" "$WORK/rootfs/sbin/mantle-supervise" "$WORK/rootfs/bin/mantlectl"
chmod 0755 "$WORK/rootfs/usr/bin/mantle" "$WORK/rootfs/usr/bin/mantle-script" "$WORK/rootfs/usr/bin/mantle-command"
chmod 0755 "$WORK/rootfs/bin/mantle-shell"
ln -sf /usr/bin/mantle "$WORK/rootfs/bin/mantle"
ln -sf /bin/mantle-shell "$WORK/rootfs/usr/bin/mantle-shell"
chmod 0755 "$WORK/rootfs/sbin/mantle-network" "$WORK/rootfs/sbin/mantle-logd"
chmod 0755 "$WORK/rootfs/sbin/mantle-supervise"
chmod 0755 "$WORK/rootfs/sbin/mantle-splash"
cat > "$WORK/rootfs/etc/profile" <<'EOF'
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PS1='\[\033[1;34m\]mantle\[\033[0m\]# '
clear 2>/dev/null || true
echo 'MantleOS 0.1 — environnement de récupération'
echo 'Aucune donnée ne quitte cet appareil sans action explicite.'
EOF
printf 'MantleOS 0.1\n' > "$WORK/rootfs/etc/os-release"
ln -sf /proc/mounts "$WORK/rootfs/etc/mtab"
touch "$WORK/rootfs/etc/resolv.conf"
mkdir -p "$WORK/rootfs/etc/mantleos"
printf '%s\n' "${MANTLE_PROFILE:-personal}" > "$WORK/rootfs/etc/mantleos/profile"
cat > "$WORK/rootfs/etc/hostname" <<'EOF'
mantle
EOF
cat > "$WORK/rootfs/etc/passwd" <<'EOF'
root:x:0:0:root:/root:/bin/sh
mantle:x:1000:1000:Mantle User:/home/mantle:/bin/sh
EOF
cat > "$WORK/rootfs/etc/group" <<'EOF'
root:x:0:
mantle:x:1000:
wheel:x:10:mantle
audio:x:29:mantle
video:x:44:mantle
netdev:x:1001:mantle
EOF
cat > "$WORK/rootfs/etc/shadow" <<'EOF'
root:!:1:0:99999:7:::
mantle:!:1:0:99999:7:::
EOF
chmod 0640 "$WORK/rootfs/etc/shadow"
mkdir -p "$WORK/rootfs/home/mantle" "$WORK/rootfs/var/log" "$WORK/rootfs/var/lib/mantleos" "$WORK/rootfs/etc/mantleos/services"
mkdir -p "$WORK/rootfs/etc/mantleos/trust"
mkdir -p "$WORK/rootfs/etc/mantleos/trust/keys"
touch "$WORK/rootfs/etc/mantleos/trust/revoked"
chmod 0700 "$WORK/rootfs/etc/mantleos/trust" "$WORK/rootfs/etc/mantleos/trust/keys"
mkdir -p "$WORK/rootfs/etc/sudoers.d"
cat > "$WORK/rootfs/etc/sudoers" <<'EOF'
Defaults env_reset
root ALL=(ALL) ALL
%wheel ALL=(ALL) ALL
EOF
chmod 0440 "$WORK/rootfs/etc/sudoers"
chown -R 1000:1000 "$WORK/rootfs/home/mantle"
cat > "$WORK/rootfs/etc/mantleos/dhcp.script" <<'EOF'
#!/bin/sh
set -eu
case "$1" in
 bound|renew) [ -n "${ip:-}" ] && [ -n "${subnet:-}" ] && /bin/ip addr replace "${ip}/${subnet}" dev "${interface}"; if [ -n "${router:-}" ]; then /bin/ip route replace default via "${router}" dev "${interface}"; else exit 1; fi; printf '%s\n' "nameserver ${dns:-1.1.1.1}" > /etc/resolv.conf; [ -n "${domain:-}" ] && printf 'search %s\n' "$domain" >> /etc/resolv.conf; hostname "${hostname:-mantle}"; mkdir -p /run/mantle; touch /run/mantle/network.ready;;
esac
EOF
chmod 0755 "$WORK/rootfs/etc/mantleos/dhcp.script"
cat > "$WORK/rootfs/etc/mantleos/network.conf" <<'EOF'
mode=dhcp
interface=eth0
EOF
cat > "$WORK/rootfs/etc/mantleos/services/session.service" <<'EOF'
name=session
description=Session console MantleOS
exec=/bin/sh -l
user=mantle
restart=on-failure
depends=log
EOF
cat > "$WORK/rootfs/etc/mantleos/services/network.service" <<'EOF'
name=network
description=Réseau MantleOS DHCP ou statique
exec=/sbin/mantle-network
restart=on-failure
depends=filesystem
EOF
cat > "$WORK/rootfs/etc/mantleos/services/log.service" <<'EOF'
name=log
description=Journal local MantleOS avec rotation
exec=/sbin/mantle-logd
restart=always
depends=filesystem
EOF
chmod 0700 "$WORK/rootfs/root"
mkdir -p "$WORK/rootfs/usr/share/mantleos"
mkdir -p "$WORK/rootfs/usr/share/mantleos/site"
cp -a "$ROOT/site/." "$WORK/rootfs/usr/share/mantleos/site/"
sh "$ROOT/tests/language-tests.sh" "$WORK/rootfs"
sh "$ROOT/tests/package-tests.sh" "$WORK/rootfs"
mkdir -p "$WORK/rootfs/usr/share/mime/packages" "$WORK/rootfs/usr/share/applications"
cp "$ROOT/apps/mime/mantle.xml" "$WORK/rootfs/usr/share/mime/packages/mantle.xml"
cp "$ROOT/apps/mime/mantle-script.desktop" "$WORK/rootfs/usr/share/applications/mantle-script.desktop"
convert "$ROOT/boot/splash/mantleos-dark.svg" PPM:"$WORK/rootfs/usr/share/mantleos/mantleos-dark.ppm"
convert "$ROOT/boot/splash/mantleos-light.svg" PPM:"$WORK/rootfs/usr/share/mantleos/mantleos-light.ppm"
chmod 1777 "$WORK/rootfs/tmp"
dd if=/dev/zero of="$OUT/mantleos-root.ext4" bs=1M count=512 status=none
mkfs.ext4 -F -L MANTLE_ROOT "$OUT/mantleos-root.ext4" >/dev/null
mkdir -p "$WORK/mnt-root"
mount -o loop "$OUT/mantleos-root.ext4" "$WORK/mnt-root"
cp -a "$WORK/rootfs/." "$WORK/mnt-root/"
sync
umount "$WORK/mnt-root"
mkdir -p "$WORK/initramfs/bin" "$WORK/initramfs/sbin" "$WORK/initramfs/dev" "$WORK/initramfs/proc" "$WORK/initramfs/sys" "$WORK/initramfs/run" "$WORK/initramfs/newroot" "$WORK/initramfs/etc"
cp -a "$WORK/rootfs/bin/." "$WORK/initramfs/bin/"
cp "$WORK/rootfs/sbin/mantle-supervise" "$WORK/initramfs/sbin/mantle-supervise"
cp "$WORK/rootfs/sbin/mantle-splash" "$WORK/initramfs/bin/mantle-splash"
mkdir -p "$WORK/initramfs/usr/share/mantleos"
cp -a "$WORK/rootfs/usr/share/mantleos/." "$WORK/initramfs/usr/share/mantleos/"
$CC -static -Os -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wl,-z,relro,-z,now -o "$WORK/initramfs/init" "$ROOT/init/mantle-init.c"
chmod 0755 "$WORK/initramfs/init" "$WORK/initramfs/bin/mantle-splash"
(cd "$WORK/initramfs" && find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > "$OUT/mantle-initramfs.cpio.gz")
cp "$WORK/kernel/arch/x86/boot/bzImage" "$OUT/mantle-kernel"
cp "$OUT/mantle-kernel" "$WORK/iso/boot/vmlinuz"
cp "$OUT/mantle-initramfs.cpio.gz" "$WORK/iso/boot/initramfs.cpio.gz"
cp "$OUT/mantleos-root.ext4" "$WORK/iso/boot/rootfs.ext4"
dd if=/dev/zero of="$OUT/mantleos-disk.img" bs=1M count=512 status=none
mkfs.ext4 -F -L MANTLE_ROOT "$OUT/mantleos-disk.img" >/dev/null
mkdir -p "$WORK/mnt-disk"
mount -o loop "$OUT/mantleos-disk.img" "$WORK/mnt-disk"
mount -o loop "$OUT/mantleos-root.ext4" "$WORK/mnt-root"
cp -a "$WORK/mnt-root/." "$WORK/mnt-disk/"
umount "$WORK/mnt-root"; umount "$WORK/mnt-disk"
cp "$ROOT/boot/grub/grub.cfg" "$WORK/iso/boot/grub/grub.cfg"
grub-mkrescue -o "$OUT/mantleos-amd64.iso" "$WORK/iso" >/dev/null
sha256sum "$OUT/mantleos-amd64.iso" > "$OUT/mantleos-amd64.iso.sha256"
commit=unknown
if command -v git >/dev/null 2>&1; then commit=$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || printf '%s' unknown); fi
cat > "$OUT/mantleos-build-info.txt" <<EOF
version=MantleOS 0.1
profile=${MANTLE_PROFILE:-personal}
architecture=amd64
commit=$commit
iso_sha256=$(cut -d' ' -f1 "$OUT/mantleos-amd64.iso.sha256")
EOF
echo "Image créée: $OUT/mantleos-amd64.iso"
