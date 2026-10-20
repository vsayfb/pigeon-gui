#!/bin/sh
set -eu
self=$(readlink -f "$0")
bundle=$(dirname "$(dirname "$self")")
case "${1:-}" in
    --uninstall)
        [ "$#" = 1 ] || { printf 'Usage: pigeon-gui --uninstall\n' >&2; exit 1; }
        exec sh "$bundle/share/uninstall.sh" "$bundle" gui ;;
    --help|-h)
        "$bundle/bin/pigeon-gui.real" "$@"
        printf '\n  --uninstall  Remove GUI, keeping the collector and traffic history\n'
        exit 0 ;;
esac
# Version bundles live alongside the collector's shared database.
if [ "$#" = 0 ]; then
    root=$(dirname "$(dirname "$bundle")")
    set -- --database "${PIGEON_DATABASE:-$root/pigeon.db}"
fi
exec "$bundle/bin/pigeon-gui.real" "$@"
