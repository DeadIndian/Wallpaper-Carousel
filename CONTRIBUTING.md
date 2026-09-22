# Contributing to Wallpaper Carousel

Thanks for taking the time to contribute. This is a KDE Plasma 6 project: a
compiled Qt6 picker app plus a pure-QML wallpaper plugin.

## Before you start

- Check [existing issues](https://github.com/DeadIndian/Wallpaper-Carousel/issues)
  so you are not duplicating work.
- For anything larger than a bug fix, open an issue first so we can agree on the
  approach before you write code.
- By contributing you agree your changes are licensed under the same terms as the
  file you are touching — MIT for `src/`, `qml/`, packaging and docs;
  GPL-2.0-or-later for `plugin/org.wallpapercarousel.slideshow/`. See
  [README → License](README.md#license). Keep that split intact.

## Build and test

Prerequisites: Qt6 6.5+ (Quick, Core, Gui, DBus, Multimedia, Widgets), the
Qt5Compat QML module, and CMake 3.16+. tomlplusplus is fetched at configure time if
it is not already installed.

```bash
# Fedora build dependencies
sudo dnf install -y cmake gcc-c++ git \
    qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtmultimedia-devel \
    qt6-qt5compat-devel tomlplusplus-devel

cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

Run the tests before opening a PR:

```bash
ctest --test-dir build --output-on-failure
```

The suite is two assert-based self-checks (`wc-tests`, `wc-tests-screenorder`)
covering appletsrc parsing and Tab screen-hop ordering. If you change that logic,
extend the matching test in `tests/`.

Much of the app is DBus/plasmashell side effects that are not unit-testable here.
Verify those in a live Plasma session and say so in the PR description — describe
what you tested and on what setup.

## QML changes

QML files are checked with `qmllint`. Unresolved Plasma imports produce non-fatal
remarks; genuine syntax errors do not. The clean-room packaging check parses every
QML file:

```bash
bash package.sh   # builds the plugin archive; fails on a QML parse error
```

## Pull requests

1. Fork the repo and branch off `main` (`git checkout -b fix/short-description`).
2. Keep the change focused — one concern per PR.
3. Match the surrounding code style; the codebase favours small, commented functions
   that explain *why*, not *what*.
4. Run the build and tests. Do not commit `build/` or `dist/`.
5. Open the PR against `main` with a description of the change and how you verified
   it.

## Reporting bugs

Open an issue with your Plasma and Qt versions (`plasmashell --version`), your
monitor setup, and the steps to reproduce. A log helps: run the picker with
`WALLPAPER_CAROUSEL_LOG=/tmp/wc.log` and attach `/tmp/wc.log`.
</content>
