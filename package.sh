#!/usr/bin/env bash
# Build the Carousel Slideshow wallpaper archive that goes on the release page and
# gets uploaded to store.kde.org. The version comes from metadata.json so the
# filename can never drift from what Plasma reports.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG="$ROOT/plugin/org.wallpapercarousel.slideshow"
ID="org.wallpapercarousel.slideshow"
EXT=".tar.gz"

VERSION="$(sed -n 's/.*"Version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' \
    "$PKG/metadata.json" | head -1)"
[[ -n "$VERSION" ]] || { echo "error: no Version in $PKG/metadata.json" >&2; exit 1; }

OUT="$ROOT/dist/${ID}-${VERSION}${EXT}"
mkdir -p "$ROOT/dist"
rm -f "$OUT"

# metadata.json must land at the archive root, so archive the *contents* of $PKG.
# Do not add --exclude='.*' here: GNU tar lets * match / , so with -C "$PKG" .
# every member is named ./something and the pattern eats the whole archive.
# --exclude-vcs already covers .git, .gitignore and friends.
tar --exclude-vcs --exclude='*~' --exclude='.DS_Store' --exclude='__pycache__' \
    -czf "$OUT" -C "$PKG" .

members="$(tar tzf "$OUT" | wc -l)"
(( members > 5 )) || { echo "error: archive has only $members members" >&2; exit 1; }

(cd "$ROOT/dist" && sha256sum "$(basename "$OUT")" > "$(basename "$OUT").sha256")
echo "built $OUT ($members members)"
echo "sha256: $(cut -d' ' -f1 "$OUT.sha256")"
