#!/usr/bin/env bash
# Install the Carousel Slideshow wallpaper (org.wallpapercarousel.slideshow) for the
# current user. --system installs for everyone. Safe to re-run: it upgrades in place.
#
# This installs the *wallpaper plugin* only. The wallpaper-carousel app itself is
# compiled; build it with cmake (see README) or install it from COPR.
set -euo pipefail

ID="org.wallpapercarousel.slideshow"
STRUCTURE="Plasma/Wallpaper"
SUBDIR="plasma/wallpapers"
SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/plugin/org.wallpapercarousel.slideshow"

system=0
[[ "${1:-}" == "--system" ]] && system=1

if (( system )); then
    [[ $EUID -eq 0 ]] || { echo "--system needs root: sudo $0 --system" >&2; exit 1; }
    DEST="${PREFIX:-/usr}/share/$SUBDIR/$ID"
else
    DEST="${XDG_DATA_HOME:-$HOME/.local/share}/$SUBDIR/$ID"
fi

[[ -f "$SRC/metadata.json" ]] || { echo "error: $SRC/metadata.json not found" >&2; exit 1; }

# kpackagetool6 validates the metadata and updates Plasma's package registry, so use it
# whenever it exists. A plain copy is the fallback for minimal or system installs.
if command -v kpackagetool6 >/dev/null && (( ! system )); then
    if kpackagetool6 --type "$STRUCTURE" --list 2>/dev/null | grep -qF "$ID"; then
        kpackagetool6 --type "$STRUCTURE" --upgrade "$SRC"
    else
        kpackagetool6 --type "$STRUCTURE" --install "$SRC"
    fi
else
    mkdir -p "$DEST"
    rm -rf "${DEST:?}"/*
    cp -r "$SRC"/. "$DEST/"
fi

command -v kbuildsycoca6 >/dev/null 2>&1 && kbuildsycoca6 >/dev/null 2>&1 || true

echo "installed $ID -> $DEST"
echo
echo "Reload Plasma to pick it up:"
echo "  kquitapp6 plasmashell && (kstart plasmashell >/dev/null 2>&1 &)"
echo "Then select it:"
echo "  right click the desktop -> Configure Desktop and Wallpaper ->"
echo "  Wallpaper type -> Carousel Slideshow"
