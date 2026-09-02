# Super+W Wallpaper Picker — Design

Date: 2026-08-29

## Goal
Open the Wallpaper Carousel picker with Super+W. Navigate with ←/→, apply with Enter. Selecting an image on a screen running the slideshow plugin jumps the slideshow to that image (slideshow keeps running).

## Decisions
- Enter behavior: jump slideshow to chosen image (write into `Image` config key of `org.kde.slideshow`), do not switch plugin. Non-slideshow screens keep existing per-category plugin logic.
- Shortcut: Super+W opens picker. Existing `plasma_next_wallpaper` Meta+W binding is removed; no replacement "next image" shortcut.

## Behavior
1. On apply, the DBus JS script reads `desktops()[screenIndex].wallpaperPlugin`.
   - If `org.kde.slideshow`: set `currentConfigGroup = ["Wallpaper","org.kde.slideshow","General"]`, `writeConfig("Image", "file://<path>")`. Plugin untouched, slideshow continues from chosen image.
   - Else: existing logic (plugin type + write key from config per extension category).
2. Super+W launches `wallpaper-carousel` via a KDE service shortcut bound in `~/.config/kglobalshortcutsrc` (group `services`, key `wallpaper-carousel.desktop`). Requires the .desktop installed in `~/.local/share/applications/`.
3. `plasma_next_wallpaper` in kglobalshortcutsrc group `plasmashell` cleared (set to `none`).

## Changes
- `src/wallpapercontroller.cpp` / `.h`: extend `setPlasmaWallpaperJavaScript` (or new sibling) to branch on current plugin at runtime. No config changes needed.
- Installer/README: document the two `kwriteconfig6` commands. Apply them directly on this machine.

## Non-goals
- No new config options, no new dependencies, no per-launch static/slideshow toggle.

## Testing
- Manual: run app on slideshow screen, Enter, verify wallpaper changes and slideshow continues advancing later. Run on static screen, verify unchanged behavior. Press Super+W, verify picker opens.
