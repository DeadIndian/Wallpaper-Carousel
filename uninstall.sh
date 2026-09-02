#!/usr/bin/env bash
# Remove the Carousel Slideshow wallpaper (org.wallpapercarousel.slideshow).
# Configuration is left alone unless --purge is given.
set -euo pipefail

ID="org.wallpapercarousel.slideshow"
STRUCTURE="Plasma/Wallpaper"
SUBDIR="plasma/wallpapers"
DEST="${XDG_DATA_HOME:-$HOME/.local/share}/$SUBDIR/$ID"

purge=0
[[ "${1:-}" == "--purge" ]] && purge=1

if command -v kpackagetool6 >/dev/null; then
    kpackagetool6 --type "$STRUCTURE" --remove "$ID" >/dev/null 2>&1 || true
fi
[[ -d "$DEST" ]] && rm -rf "${DEST:?}"

command -v kbuildsycoca6 >/dev/null 2>&1 && kbuildsycoca6 >/dev/null 2>&1 || true
echo "removed $ID"

if (( purge )); then
    # Wallpaper settings live in the containment, not in a per-package rc file, so
    # there is nothing of ours to delete outside plasma's own appletsrc.
    echo "nothing else to purge: wallpaper settings live in"
    echo "  ${XDG_CONFIG_HOME:-$HOME/.config}/plasma-org.kde.plasma.desktop-appletsrc"
fi
echo
echo "Any desktop still set to Carousel Slideshow falls back to a blank wallpaper"
echo "until you pick another type. Reload with:"
echo "  kquitapp6 plasmashell && (kstart plasmashell >/dev/null 2>&1 &)"
