# Wallpaper Reveal Animation — Design

Date: 2026-08-31

## Goal

When the user picks a wallpaper, the new wallpaper is revealed with an expanding
circular ("bubble") animation growing out of the selected thumbnail. Minimized
windows are restored only after the animation finishes.

## Current behaviour

`qml/Main.qml`:

- `Keys.onReturnPressed` calls `controller.applyWallpaperForScreen(...)` then
  `root.close()`.
- `onVisibleChanged` calls `controller.setWindowsMinimized(visible)`, so closing
  the window restores every window the KWin script minimized.

The picker window is already fullscreen, frameless and transparent, so it can be
used as the animation canvas without any new window.

## Design

### Sequence

1. User selects a wallpaper (Enter, or thumbnail click). Capture:
   - the full-resolution `url` of the selected item,
   - the selected delegate's center point mapped into `root` coordinates (the
     bubble seed).
2. `container.opacity = 0` — the carousel bar fades out (~150 ms) so the screen
   is clean while the bubble grows.
3. A fullscreen overlay `Image` (`fillMode: Image.PreserveAspectCrop`,
   `source: url`) is shown, clipped by a circular mask centered on the seed
   point. The mask circle's radius animates `0 → maxRadius` over **700 ms**,
   `Easing.OutCubic`. The revealed area is fully opaque — a crisp Material-style
   circular reveal, no feathering or opacity ramp.
   `maxRadius` = max distance from the seed point to the four screen corners.
4. On animation completion: call
   `controller.applyWallpaperForScreen(screenHelper.targetScreenName, url)`,
   then wait a ~150 ms grace period (so plasmashell has painted the real
   wallpaper) and call `root.close()`.
5. `onVisibleChanged` fires → `controller.setWindowsMinimized(false)` → windows
   restore. This already happens after the close, so no C++ change is needed to
   satisfy the "windows wait for the animation" requirement.

Because the real wallpaper is applied only at the end of the reveal, the desktop
behind the overlay still shows the *old* wallpaper for the whole animation,
which produces the intended reveal illusion.

### Components

- **New** `qml/RevealOverlay.qml` (~40 lines): the masked image, the circle, the
  `NumberAnimation` on radius, a `start(url, seedX, seedY)` function and a
  `revealFinished` signal. Registered via `qml/qmldir` like `CarouselItem.qml`.
- **Modified** `qml/Main.qml`: a `startReveal(url, seedPoint)` function called
  from `Keys.onReturnPressed`. Clicking a thumbnail keeps its current behaviour
  (moves `currentIndex` only); applying still requires Enter, so Enter is the
  only entry point to the reveal.
- **No C++ changes.**

### Edge cases

- **Slow decode:** the reveal starts only when the overlay image reaches
  `Image.Ready`. If it is not ready, apply the wallpaper and close immediately —
  never hang waiting on a decode.
- **Video wallpapers** (`.mp4`, `.webm`): same path. The overlay `Image` renders
  whatever the source yields (thumbnail/poster frame or nothing);
  `PreserveAspectCrop` handles the scale. No QtMultimedia in the overlay.
- **Double input:** the reveal is one-shot. Once started, the carousel is
  disabled so a second Enter/Escape cannot re-enter or close mid-animation.
- **Apply failure:** `applyWallpaperForScreen` emits `error`; the close still
  happens, matching today's behaviour.

### Verification

The only non-trivial logic is `maxRadius` (max over four corner distances) and
the animation ordering. Verified manually by running the app: pick a wallpaper on
each monitor, confirm the bubble grows from the thumbnail, the new wallpaper is
in place when the bubble finishes, and windows restore only afterwards. No new
unit test — a QML animation timing check would need a test harness that does not
exist in this project.

## Out of scope

- Per-monitor simultaneous reveal (this is a single floating window app).
- Alternate reveal shapes/easings or a config knob for duration. Add when
  someone asks.
- Animating inside the bundled `org.wallpapercarousel.slideshow` plugin.
