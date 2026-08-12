#!/bin/sh
set -eu
ROOTFS=${1:?rootfs path required}
TMP="$ROOTFS/tmp/mtpkg-tests"
CHROOT_TMP=/tmp/mtpkg-tests
KEY_DIR="$ROOTFS/etc/mantleos/trust/keys"
REVOCATION="$ROOTFS/etc/mantleos/trust/revoked"
cleanup(){ status=$?; rm -rf "$TMP" "$KEY_DIR/testkey.pub"; : > "$REVOCATION"; exit "$status"; }
trap cleanup EXIT INT TERM
mkdir -p "$TMP/work/META" "$TMP/work/payload/usr/share/mantleos" "$KEY_DIR"
openssl genrsa -traditional -out "$TMP/testkey.pem" 2048 >/dev/null 2>&1
openssl rsa -in "$TMP/testkey.pem" -pubout -out "$KEY_DIR/testkey.pub" >/dev/null 2>&1

make_package(){
    dir=$1; archive=$2; name=$3
    (cd "$dir" && sha256sum payload/usr/share/mantleos/mtpkg-test-file > META/MANIFEST)
    printf '%s\n' "$name" > "$dir/META/NAME"
    cat > "$dir/META/metadata.json" <<EOF
{"name":"$name","version":"1.0.0","architecture":"x86_64","dependencies":[],"description":"CI package","license":"MIT","maintainer":"MantleOS","build":"ci"}
EOF
    cat "$dir/META/metadata.json" "$dir/META/MANIFEST" > "$dir/META/SIGNED"
    printf 'testkey\n' > "$dir/META/KEY-ID"
    openssl dgst -sha256 -sign "$TMP/testkey.pem" -out "$dir/META/SIGNATURE" "$dir/META/SIGNED"
    tar -czf "$archive" -C "$dir" META payload
}

printf 'valid-package\n' > "$TMP/work/payload/usr/share/mantleos/mtpkg-test-file"
make_package "$TMP/work" "$TMP/valid.mtpkg" valid-test
chroot "$ROOTFS" /usr/bin/mantle verify "$CHROOT_TMP/valid.mtpkg"
chroot "$ROOTFS" /usr/bin/mantle install "$CHROOT_TMP/valid.mtpkg"
[ -f "$ROOTFS/usr/share/mantleos/mtpkg-test-file" ]
chroot "$ROOTFS" /usr/bin/mantle remove valid-test
[ ! -e "$ROOTFS/usr/share/mantleos/mtpkg-test-file" ]

cp -a "$TMP/work" "$TMP/bad-hash"
printf 'changed-after-signing\n' > "$TMP/bad-hash/payload/usr/share/mantleos/mtpkg-test-file"
tar -czf "$TMP/bad-hash.mtpkg" -C "$TMP/bad-hash" META payload
if chroot "$ROOTFS" /usr/bin/mantle verify "$CHROOT_TMP/bad-hash.mtpkg"; then exit 1; fi

cp -a "$TMP/work" "$TMP/bad-signature"
printf 'invalid-signature' > "$TMP/bad-signature/META/SIGNATURE"
tar -czf "$TMP/bad-signature.mtpkg" -C "$TMP/bad-signature" META payload
if chroot "$ROOTFS" /usr/bin/mantle verify "$CHROOT_TMP/bad-signature.mtpkg"; then exit 1; fi

printf 'testkey\n' > "$REVOCATION"
if chroot "$ROOTFS" /usr/bin/mantle verify "$CHROOT_TMP/valid.mtpkg"; then exit 1; fi
: > "$REVOCATION"

python3 - "$TMP/unsafe.mtpkg" "$TMP/symlink.mtpkg" <<'PY'
import io, sys, tarfile
def write(path, name, kind):
    with tarfile.open(path, 'w:gz') as archive:
        item = tarfile.TarInfo(name)
        if kind == 'symlink':
            item.type = tarfile.SYMTYPE
            item.linkname = '/etc/passwd'
            archive.addfile(item)
        else:
            data = b'x'
            item.size = len(data)
            archive.addfile(item, io.BytesIO(data))
write(sys.argv[1], '/etc/passwd', 'absolute')
write(sys.argv[2], 'payload/usr/share/mantleos/link', 'symlink')
PY
if chroot "$ROOTFS" /usr/bin/mantle verify "$CHROOT_TMP/unsafe.mtpkg"; then exit 1; fi
if chroot "$ROOTFS" /usr/bin/mantle verify "$CHROOT_TMP/symlink.mtpkg"; then exit 1; fi
printf 'not-an-archive' > "$TMP/corrupt.mtpkg"
if chroot "$ROOTFS" /usr/bin/mantle verify "$CHROOT_TMP/corrupt.mtpkg"; then exit 1; fi

mkdir -p "$TMP/rollback/META" "$TMP/rollback/payload/usr/share/mantleos" "$TMP/rollback/payload/sbin"
printf 'rollback-file\n' > "$TMP/rollback/payload/usr/share/mantleos/mtpkg-test-file"
printf 'blocked\n' > "$TMP/rollback/payload/sbin/mantle-init"
(cd "$TMP/rollback" && sha256sum payload/usr/share/mantleos/mtpkg-test-file payload/sbin/mantle-init > META/MANIFEST)
printf 'rollback-test\n' > "$TMP/rollback/META/NAME"
cat > "$TMP/rollback/META/metadata.json" <<'EOF'
{"name":"rollback-test","version":"1.0.0","architecture":"x86_64","dependencies":[],"description":"CI rollback package","license":"MIT","maintainer":"MantleOS","build":"ci"}
EOF
cat "$TMP/rollback/META/metadata.json" "$TMP/rollback/META/MANIFEST" > "$TMP/rollback/META/SIGNED"
printf 'testkey\n' > "$TMP/rollback/META/KEY-ID"
openssl dgst -sha256 -sign "$TMP/testkey.pem" -out "$TMP/rollback/META/SIGNATURE" "$TMP/rollback/META/SIGNED"
tar -czf "$TMP/rollback.mtpkg" -C "$TMP/rollback" META payload
if chroot "$ROOTFS" /usr/bin/mantle install "$CHROOT_TMP/rollback.mtpkg"; then exit 1; fi
[ ! -e "$ROOTFS/usr/share/mantleos/mtpkg-test-file" ]
echo 'Mantle package checks: OK'
