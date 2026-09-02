# Super+W Picker Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Super+W opens Wallpaper Carousel; Enter on a slideshow screen jumps the slideshow to the chosen image instead of switching plugins.

**Architecture:** Branch inside the single DBus JS script sent to plasmashell: read `wallpaperPlugin` of the target desktop; if `org.kde.slideshow`, write chosen path to the slideshow `Image` key; else existing plugin/write-key logic. Bind Super+W to the installed .desktop via kglobalshortcutsrc; clear `plasma_next_wallpaper`.

**Tech Stack:** C++/Qt6/QDBus (existing), KDE kglobalshortcutrc config.

## Global Constraints

- No new dependencies, no new files in `src/`.
- Non-slideshow behavior unchanged (per-category plugin config from `~/.config/wallpaper-carousel/config.toml`).
- Manual verification only (DBus/plasmashell side effects not unit-testable here).

---

### Task 1: Slideshow-aware apply

**Files:**
- Modify: `src/wallpapercontroller.cpp` (`setPlasmaWallpaperJavaScript`, lines ~158-207)

**Interfaces:**
- Consumes: existing `applyWallpaperForScreen(int, QString)`.
- Produces: same signature; behavior change only.

- [ ] **Step 1: Replace JS body with plugin-branching script**

```cpp
QString js = QString(R"(
const allDesktops = desktops()
    .filter(d => d.screen !== -1)
    .sort((a, b) => {
        const ga = screenGeometry(a.screen);
        const gb = screenGeometry(b.screen);
        if (ga.top === gb.top) {
            return ga.left - gb.left;
        }
        return ga.top - gb.top;
    });

const targetDesktop = allDesktops[%1];
if (targetDesktop) {
    if (targetDesktop.wallpaperPlugin === "org.kde.slideshow") {
        targetDesktop.currentConfigGroup = ["Wallpaper", "org.kde.slideshow", "General"];
        targetDesktop.writeConfig("Image", "file://%4");
    } else {
        targetDesktop.wallpaperPlugin = "%2";
        targetDesktop.currentConfigGroup = ["Wallpaper", "%2", "General"];
        targetDesktop.writeConfig("Image", "file:///dev/null");
        targetDesktop.writeConfig("%3", "file://%4");
    }
}
)").arg(screenIndex).arg(pluginType).arg(writeKey).arg(path);
```

- [ ] **Step 2: Rebuild**

Run: `cmake --build build -j`
Expected: compiles clean.

- [ ] **Step 3: Manual test on slideshow screen**

Set screen wallpaper type to Slideshow (folder = `~/Pictures/Wallpapers`), run `./build/wallpaper-carousel`, arrow to an image, Enter. Expect: wallpaper becomes chosen image; slideshow still advances on its timer.

### Task 2: Super+W binding

**Files:**
- Modify: `~/.config/kglobalshortcutsrc` (via `kwriteconfig6`), `README.md`

- [ ] **Step 1: Install .desktop for current user**

```bash
cp packaging/wallpaper-carousel.desktop ~/.local/share/applications/
```

- [ ] **Step 2: Clear old binding, add Super+W**

```bash
kwriteconfig6 --file kglobalshortcutsrc --group plasmashell --key plasma_next_wallpaper "none,none,Next Wallpaper Image"
kwriteconfig6 --file kglobalshortcutsrc --group services --key wallpaper-carousel.desktop "_launch,Meta+W,Wallpaper Carousel"
```

Then `qdbus6 org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel.blockGlobalShortcuts false 2>/dev/null; kquitapp6 kglobalaccel 2>/dev/null; sleep 1` — kglobalaccel restarts and reloads. (Fallback: relogin.)

- [ ] **Step 3: Manual test**

Press Super+W. Expect: picker opens. Old Meta+W next-wallpaper no longer fires.

- [ ] **Step 4: Document in README**

Add "Super+W shortcut" section with the two kwriteconfig6 commands.
