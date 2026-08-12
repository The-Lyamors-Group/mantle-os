#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"
profile=${MANTLE_PROFILE:-personal}
case "$profile" in personal|education|government) ;; *) echo "Profil invalide: $profile" >&2; exit 2;; esac
export MANTLE_PROFILE="$profile"
mkdir -p "$ROOT/build/out"
LOG="$ROOT/build/out/build.log"
if sh "$ROOT/build/make-image.sh" >"$LOG" 2>&1; then
    cat "$LOG"
else
    status=$?
    cat "$LOG" >&2
    exit "$status"
fi
