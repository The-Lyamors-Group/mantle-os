#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"
profile=${MANTLE_PROFILE:-personal}
export MANTLE_COMPAT=${MANTLE_COMPAT:-full}
export MANTLE_SDK=${MANTLE_SDK:-0}
case "$profile" in personal|education|government) ;; *) echo "Profil invalide: $profile" >&2; exit 2;; esac
export MANTLE_PROFILE="$profile"
sh "$ROOT/build/make-image.sh"
