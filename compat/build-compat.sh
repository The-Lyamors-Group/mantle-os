#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ROOTFS=$1
SYSROOT=$2
SRC=$3
CC=$4
PROFILE=${5:-core}
PREFIX="$ROOTFS/usr"
export PATH="$SYSROOT/bin:$PATH"
mkdir -p "$SRC" "$PREFIX" "$ROOTFS/etc/ssh" "$ROOTFS/etc/ssl"
fetch(){ url=$1;file=$2;[ -f "$file" ]||curl -fL --retry 3 --output "$file" "$url"; }
unpack(){ archive=$1;dir=$2;[ -d "$dir" ]||tar -xf "$archive" -C "$SRC"; }
build_autoconf(){ dir=$1;shift;(cd "$dir" && CC="$CC" CFLAGS="-Os -static" LDFLAGS="-static -L$SYSROOT/lib" ./configure --host=x86_64-linux-musl --prefix=/usr --disable-shared --enable-static "$@" && make -j"$(nproc)" && make DESTDIR="$ROOTFS" install); }

fetch https://git.kernel.org/pub/scm/utils/dash/dash.git/snapshot/dash-0.5.12.tar.gz "$SRC/dash.tar.gz"
unpack "$SRC/dash.tar.gz" "$SRC/dash-0.5.12"
(cd "$SRC/dash-0.5.12" && CC="$CC" CFLAGS="-Os" LDFLAGS="-static" ./configure --host=x86_64-linux-musl --prefix=/usr && make -j"$(nproc)" && make DESTDIR="$ROOTFS" install)
cp "$ROOTFS/usr/bin/dash" "$ROOTFS/bin/dash"
ln -sf dash "$ROOTFS/bin/sh"

fetch https://zlib.net/fossils/zlib-1.3.1.tar.gz "$SRC/zlib.tar.gz"
unpack "$SRC/zlib.tar.gz" "$SRC/zlib-1.3.1"
(cd "$SRC/zlib-1.3.1" && CC="$CC" CFLAGS="-Os" ./configure --static --prefix="$SYSROOT" && make -j"$(nproc)" && make install)

fetch https://www.openssl.org/source/openssl-3.4.1.tar.gz "$SRC/openssl.tar.gz"
unpack "$SRC/openssl.tar.gz" "$SRC/openssl-3.4.1"
(cd "$SRC/openssl-3.4.1" && CC="$CC" ./Configure linux-x86_64 no-shared no-tests --prefix="$SYSROOT" --openssldir=/etc/ssl && make -j"$(nproc)" && make install_sw)
cp -a "$SYSROOT/lib/." "$ROOTFS/usr/lib/"
cp -a "$SYSROOT/bin/openssl" "$ROOTFS/usr/bin/"

fetch https://curl.se/download/curl-8.10.1.tar.xz "$SRC/curl.tar.xz"
unpack "$SRC/curl.tar.xz" "$SRC/curl-8.10.1"
build_autoconf "$SRC/curl-8.10.1" --with-openssl="$SYSROOT" --with-zlib="$SYSROOT" --without-libpsl --disable-docs --disable-manual --disable-ldap --disable-rtsp --disable-telnet --disable-tftp --disable-dict --disable-gopher --disable-imap --disable-pop3 --disable-smb --disable-smtp --disable-mqtt
fetch https://curl.se/ca/cacert.pem "$ROOTFS/etc/ssl/cert.pem"

fetch https://www.kernel.org/pub/software/scm/git/git-2.47.1.tar.xz "$SRC/git.tar.xz"
unpack "$SRC/git.tar.xz" "$SRC/git-2.47.1"
(cd "$SRC/git-2.47.1" && make CC="$CC" CFLAGS="-Os -I$SYSROOT/include" LDFLAGS="-static -L$SYSROOT/lib" NO_GETTEXT=Yes NO_TCLTK=Yes NO_PERL=Yes NO_PYTHON=Yes NO_INSTALL_HARDLINKS=Yes CURL_CONFIG="$ROOTFS/usr/bin/curl-config" -j"$(nproc)" && make CC="$CC" prefix=/usr DESTDIR="$ROOTFS" NO_GETTEXT=Yes NO_TCLTK=Yes NO_PERL=Yes NO_PYTHON=Yes NO_INSTALL_HARDLINKS=Yes install)

fetch https://download.samba.org/pub/rsync/src/rsync-3.3.0.tar.gz "$SRC/rsync.tar.gz"
unpack "$SRC/rsync.tar.gz" "$SRC/rsync-3.3.0"
build_autoconf "$SRC/rsync-3.3.0" --disable-openssl --disable-iconv

fetch https://www.sudo.ws/dist/sudo-1.9.16p2.tar.gz "$SRC/sudo.tar.gz"
unpack "$SRC/sudo.tar.gz" "$SRC/sudo-1.9.16p2"
build_autoconf "$SRC/sudo-1.9.16p2" --without-pam --without-ldap --with-iologdir=/var/log/sudo-io

fetch https://cdn.openbsd.org/pub/OpenBSD/OpenSSH/portable/openssh-9.9p1.tar.gz "$SRC/openssh.tar.gz"
unpack "$SRC/openssh.tar.gz" "$SRC/openssh-9.9p1"
build_autoconf "$SRC/openssh-9.9p1" --with-ssl-dir="$SYSROOT" --without-pam --without-selinux --without-kerberos5 --without-libedit --sysconfdir=/etc/ssh

fetch https://ftp.gnu.org/gnu/make/make-4.4.1.tar.xz "$SRC/make.tar.xz"
unpack "$SRC/make.tar.xz" "$SRC/make-4.4.1"
build_autoconf "$SRC/make-4.4.1"

fetch https://pkg-config.freedesktop.org/releases/pkg-config-0.29.2.tar.gz "$SRC/pkg-config.tar.gz"
unpack "$SRC/pkg-config.tar.gz" "$SRC/pkg-config-0.29.2"
build_autoconf "$SRC/pkg-config-0.29.2" --with-internal-glib --disable-host-tool

if [ "$PROFILE" = full ]; then
  fetch https://www.python.org/ftp/python/3.13.1/Python-3.13.1.tgz "$SRC/python.tgz"
  unpack "$SRC/python.tgz" "$SRC/Python-3.13.1"
  build_autoconf "$SRC/Python-3.13.1" --without-ensurepip --disable-test-modules --disable-shared
  if [ -x "$ROOTFS/usr/bin/python3" ]; then ln -sf python3 "$ROOTFS/usr/bin/python"; fi
fi

ln -sf /bin/busybox "$ROOTFS/usr/bin/wget"
mkdir -p "$ROOTFS/usr/bin" "$ROOTFS/usr/lib/pkgconfig" "$ROOTFS/etc/mantleos"
printf '%s\n' "$PROFILE" > "$ROOTFS/etc/mantleos/compat-profile"
