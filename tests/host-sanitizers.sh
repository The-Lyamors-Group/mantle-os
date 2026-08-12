#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT=$(mktemp -d)
cleanup(){ rm -rf "$OUT"; }
trap cleanup EXIT INT TERM
CFLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer'
cc $CFLAGS -o "$OUT/mantle-shell" "$ROOT/shell/mantle-shell.c"
cc $CFLAGS -o "$OUT/mantle-script" "$ROOT/scripting/mantle-script.c"
cc $CFLAGS -o "$OUT/mantle-command" "$ROOT/scripting/mantle-command.c"
cc $CFLAGS -o "$OUT/mantle-shell-state" "$ROOT/ui/shell/mantle-shell-state.c" "$ROOT/ui/shell/test-state.c"
"$OUT/mantle-shell" --version
"$OUT/mantle-shell" -c 'printf sanitizer-ok | grep sanitizer-ok'
"$OUT/mantle-script" "$ROOT/tests/fixtures/boot.mt" >/dev/null
"$OUT/mantle-command" "$ROOT/tests/fixtures/boot.mtc"
"$OUT/mantle-shell-state"
echo 'MantleOS host sanitizer checks: OK'
