#!/bin/sh
set -eu
ROOTFS=${1:?rootfs path required}
TESTS=/usr/share/mantleos/tests
mkdir -p "$ROOTFS$TESTS"
cp tests/fixtures/hello.mt tests/fixtures/update.mtc tests/fixtures/invalid.mt tests/fixtures/security.mt "$ROOTFS$TESTS/"
chroot "$ROOTFS" /usr/bin/mantle run "$TESTS/hello.mt" >/tmp/mantle-script-test.out
grep -q 'Installation de mantle-test' /tmp/mantle-script-test.out
chroot "$ROOTFS" /usr/bin/mantle exec "$TESTS/update.mtc"
if chroot "$ROOTFS" /usr/bin/mantle run "$TESTS/invalid.mt" >/dev/null 2>&1; then exit 1; fi
chroot "$ROOTFS" /usr/bin/mantle run "$TESTS/security.mt" >/tmp/mantle-security-test.out
grep -q 'safe;not-a-command' /tmp/mantle-security-test.out
chroot "$ROOTFS" /bin/mantle-shell -c 'printf "pipe-value\n" | grep pipe-value' >/tmp/mantle-shell-test.out
grep -q pipe-value /tmp/mantle-shell-test.out
if chroot "$ROOTFS" /bin/mantle-shell -c 'mantle-command-does-not-exist' >/dev/null 2>&1; then exit 1; fi
chroot "$ROOTFS" /bin/mantle-shell -c 'printf redirect-value > /tmp/mantle-shell-redirection'
grep -q redirect-value "$ROOTFS/tmp/mantle-shell-redirection"
echo 'Mantle language checks: OK'
