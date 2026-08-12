#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/build/out"
[ -s "$OUT/mantleos-amd64.iso" ] || { echo 'ISO absente' >&2; exit 1; }
[ -s "$OUT/mantle-initramfs.cpio.gz" ] || { echo 'initramfs absent' >&2; exit 1; }
[ -s "$OUT/mantleos-root.ext4" ] || { echo 'rootfs ext4 absent' >&2; exit 1; }
[ -s "$OUT/mantleos-disk.img" ] || { echo 'disque persistant absent' >&2; exit 1; }
[ -s "$OUT/mantleos-build-info.txt" ] || { echo 'métadonnées de build absentes' >&2; exit 1; }
[ -s "$OUT/build-info.txt" ] || { echo 'build-info.txt absent' >&2; exit 1; }
gzip -t "$OUT/mantle-initramfs.cpio.gz"
entries=$(gzip -dc "$OUT/mantle-initramfs.cpio.gz" | cpio -t 2>/dev/null)
printf '%s\n' "$entries" | grep -qx './init'
printf '%s\n' "$entries" | grep -qx './sbin/mantle-supervise'
printf '%s\n' "$entries" | grep -qx './sbin/mantle-splash'
sha256sum -c "$OUT/mantleos-amd64.iso.sha256"
grep -q '^version=' "$OUT/mantleos-build-info.txt"
grep -q '^profile=' "$OUT/mantleos-build-info.txt"
grep -q '^commit=' "$OUT/mantleos-build-info.txt"
echo 'MantleOS image checks: OK'
