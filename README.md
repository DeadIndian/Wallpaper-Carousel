<div align="center">

# Wallpaper Carousel

### Pick a wallpaper per screen from a bottom-centered carousel of thumbnails

Small utility for KDE Plasma that shows a bottom-centered carousel of images and lets you set the wallpaper per-screen — keyboard-driven, animated, and able to steer a running slideshow.

[![License](https://img.shields.io/github/license/DeadIndian/Wallpaper-Carousel?style=flat-square)](LICENSE)
[![Release](https://img.shields.io/github/v/release/DeadIndian/Wallpaper-Carousel?style=flat-square)](https://github.com/DeadIndian/Wallpaper-Carousel/releases)
[![Stars](https://img.shields.io/github/stars/DeadIndian/Wallpaper-Carousel?style=flat-square)](https://github.com/DeadIndian/Wallpaper-Carousel/stargazers)
[![KDE Plasma 6](https://img.shields.io/badge/KDE%20Plasma-6-1d99f3?style=flat-square&logo=kde&logoColor=white)](https://kde.org/plasma-desktop/)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](CONTRIBUTING.md)

[Installation](#install) ·
[Usage](#using-the-carousel-slideshow-wallpaper) ·
[Report Bug](https://github.com/DeadIndian/Wallpaper-Carousel/issues) ·
[Request Feature](https://github.com/DeadIndian/Wallpaper-Carousel/issues)

<!-- USER: capture the picker open over a desktop and save it to assets/screenshots/hero.png -->
<img src="assets/screenshots/hero.png" alt="Wallpaper Carousel picker" width="80%" />

</div>

---

## Table of Contents

- [What ships here](#what-ships-here)
- [Features](#features)
- [Screenshots](#screenshots)
- [Install](#install)
- [Using the Carousel Slideshow wallpaper](#using-the-carousel-slideshow-wallpaper)
- [Config](#config)
- [Notes](#notes)
- [Packaging](#packaging)
- [Contributing](#contributing)
- [License](#license)

---

## What ships here

Two pieces ship from this repo:

| | What it is | Needs compiling |
|---|---|---|
| **`wallpaper-carousel`** | The picker app. Qt Quick carousel, applies wallpapers through plasmashell. | yes |
| **Carousel Slideshow** | Optional wallpaper plugin the picker can drive mid-rotation. Pure QML. | no |

The app works on its own with the stock `org.kde.image` wallpaper. The plugin is what
lets you keep a slideshow running *and* jump it to a chosen image.

## Features
- Carousel UI (Qt Quick) placed at the bottom center.
- Keyboard navigation (← / →) and `Enter` to apply wallpaper.
- `Tab` / `Shift+Tab` move the picker to the next / previous screen, for when the
  compositor guesses the wrong one (common right after a monitor hotplug).
- Scans `~/{XDG_PICTURES_DIR}/Wallpapers` by default (configurable).
- Uses plasmashell DBus `evaluateScript` to set wallpapers per screen.
- Expanding-circle reveal animation grown out of the picked thumbnail.

## Screenshots

<!-- USER: capture each view and drop the files at the paths below, then commit them. -->

| Picker carousel | Reveal animation |
| :---: | :---: |
| <img src="assets/screenshots/carousel.png" width="100%" /> | <img src="assets/screenshots/reveal.png" width="100%" /> |

## Install

### Fedora, from COPR

```bash
sudo dnf copr enable deadindian/wallpaper-carousel
sudo dnf install wallpaper-carousel
```

> The COPR project is not published yet. Until it is, build from source below.
> `packaging/wallpaper-carousel.spec` is ready for it and installs both halves.

### The wallpaper plugin on its own

No build, no root. Either from the KDE Store — right click the desktop, *Configure
Desktop and Wallpaper*, *Wallpaper type*, *Get New Plugins*, search **Carousel
Slideshow** — or from a release archive:

```bash
kpackagetool6 --type Plasma/Wallpaper --install org.wallpapercarousel.slideshow-0.1.0.tar.gz
```

From a git checkout, `./install.sh` does the same thing (`./uninstall.sh` reverses it).

### Build from source

Requires Qt6 6.5+ (Quick, Core, Gui, DBus, Multimedia, Widgets), the Qt5Compat QML
module (for `Qt5Compat.GraphicalEffects`), and CMake 3.16+. tomlplusplus is fetched
automatically at configure time if it isn't already installed.

```bash
# Fedora build dependencies
sudo dnf install -y cmake gcc-c++ git \
    qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtmultimedia-devel \
    qt6-qt5compat-devel tomlplusplus-devel

cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build          # goes to /usr/local by default
```

That single install step lays down all four pieces:

| Path (under the install prefix) | What |
|---|---|
| `bin/wallpaper-carousel` | the picker |
| `share/applications/wallpaper-carousel.desktop` | menu entry, and what the global shortcut resolves |
| `share/metainfo/io.github.deadindian.WallpaperCarousel.metainfo.xml` | so Discover can show it |
| `share/plasma/wallpapers/org.wallpapercarousel.slideshow/` | the wallpaper plugin |

Then put some wallpapers where it looks and run it:

```bash
mkdir -p ~/Pictures/Wallpapers
cp /path/to/your/images/* ~/Pictures/Wallpapers/
wallpaper-carousel
```

### Bind it to a key

The picker is meant to be summoned, not left running. It has no tray icon and it
minimizes your windows while it is up, so it is deliberately **not** autostarted.

```bash
kwriteconfig6 --file kglobalshortcutsrc --group services \
    --key wallpaper-carousel.desktop "_launch,Meta+W,Wallpaper Carousel"
kquitapp6 kglobalaccel && kstart kglobalaccel
```

## Using the Carousel Slideshow wallpaper

Right click the desktop → *Configure Desktop and Wallpaper* → *Wallpaper type* →
**Carousel Slideshow**. Configure it exactly like the stock Plasma slideshow: folders,
ordering, fill mode, blur or solid background, dynamic day/night images.

What it adds over the stock one: the picker writes the `Image` config key, so pressing
`Enter` jumps that screen's slideshow to the chosen picture and the rotation timer
carries on from there. It also records the slide currently on screen and which monitor
it is on, which is how the picker knows what you are looking at.

With any other wallpaper type the picker still works — it switches the desktop to
`org.kde.image` and sets a single image instead.

## Config

Edit `~/.config/wallpaper-carousel/config.toml` to change `wallpaper_dir` and KDE plugins.
It is created on first run.

`animation_duration_ms` sets the length of the reveal animation. Default `700`, clamped
to `0`–`5000`; `0` turns the animation off and applies the wallpaper immediately.

The `video` category defaults to the third-party
`luisbocanegra.smart.video.wallpaper.reborn` plugin. Install it from the KDE Store if you
want video wallpapers; static images work out of the box.

## Notes
- One floating window, not one carousel per monitor. `Tab` hops it to the next screen,
  then `Enter` sets that screen's wallpaper.
- `qDebug` output goes nowhere in a Plasma session. Set
  `WALLPAPER_CAROUSEL_LOG=/tmp/wc.log` to get a log file.
- If `/usr/local/bin` isn't on your `PATH`, run `/usr/local/bin/wallpaper-carousel`
  directly or install with `-DCMAKE_INSTALL_PREFIX=/usr`.

## Packaging

`./package.sh` builds `dist/org.wallpapercarousel.slideshow-<version>.tar.gz` plus its
sha256 — that archive is what goes on a GitHub release and to store.kde.org.
`plugin/org.wallpapercarousel.slideshow/metadata.json` holds the version and is the
source of truth; the release workflow refuses a tag that disagrees with it.
`packaging/store-description.txt` is the store listing text.

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for build and test
steps, and the [Code of Conduct](CODE_OF_CONDUCT.md) before opening a PR.

## License

Two licenses, by directory — they are not interchangeable, so keep the split when you
copy code around:

- **`src/`, `qml/`, packaging, docs — MIT.** See [`LICENSE`](LICENSE).
- **`plugin/org.wallpapercarousel.slideshow/` — GPL-2.0-or-later.** It is a fork of
  KDE's own `org.kde.slideshow` wallpaper from plasma-workspace, and keeps its
  copyright headers and license. See
  [`plugin/org.wallpapercarousel.slideshow/LICENSE`](plugin/org.wallpapercarousel.slideshow/LICENSE).
</content>
</invoke>
