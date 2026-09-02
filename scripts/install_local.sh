#!/usr/bin/env bash
# Build and install the picker app system-wide (/usr/local by default).
# Not to be confused with ../install.sh, which installs only the Carousel Slideshow
# wallpaper plugin into your home directory and needs no root.
set -e
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build
echo "Installed. The launcher, metainfo and the Carousel Slideshow wallpaper went in too;"
echo "see the README for the Meta+W shortcut."
