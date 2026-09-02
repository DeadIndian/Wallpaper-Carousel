# Tab Screen Hop — Design

Date: 2026-09-01

## Goal

Give the user a manual override for which output the picker acts on: `Tab` moves
the picker window to the next screen, `Shift+Tab` to the previous one. The
carousel then reflects that screen's current wallpaper, and `Enter` applies to it.

## Problem

The target output is chosen once at startup by asking KWin
(`WallpaperController::queryActiveScreen`) and is never revisited. When that
answer is wrong the picker is stuck: there is no way to move it.

The answer is wrong after a monitor hotplug. Reported repro: unplug a monitor,
then launch the picker — it opens on an output the user is not looking at.

Two candidates inside `reportActiveScreen`, not yet distinguished by evidence:

- `workspace.cursorPos` is stale after a hotplug (KWin has not reshuffled pointer
  coordinates until the pointer moves), so `QGuiApplication::screenAt()` returns
  null.
- With `screenAt()` null the name falls back to `workspace.activeScreen`, which
  tracks the focused window's output, not where the user is looking.

Both fall back the same wrong way. Diagnosing which one requires a logged run;
the existing `[reportActiveScreen] cursor X Y active NAME -> NAME` line already
distinguishes them. That fix is deliberately **out of scope here** — the manual
override is worth having regardless of how good the automatic guess gets.

## Design

### `src/screenorder.h` (new, header-only)

Pure index math, no Qt Quick, so it is testable the way `appletsrcparse.h` is.

```cpp
struct ScreenEntry { QString name; QRect geometry; };
QStringList orderScreenNames(QList<ScreenEntry> entries);
QString nextScreenName(const QStringList &ordered, const QString &current, int delta);
```

- `orderScreenNames` sorts left-to-right by `geometry().x()`, then `y()`, then
  name as a stability tiebreak, so `Tab` walks the outputs in the order they sit
  on the desk rather than in KWin's arbitrary enumeration order.
- `nextScreenName` wraps around. `current` not in the list (its screen was
  unplugged) returns the first live screen — the recovery path. Fewer than two
  screens and a known `current` returns an empty string, meaning "nowhere to go".

### `ScreenHelper::cycleScreen(int delta)`

New `Q_INVOKABLE`. Builds the entry list from `QGuiApplication::screens()`, asks
`nextScreenName`, and if it gets a different screen:

1. Sets `m_requestedName` and `m_targetScreenName` to the new name **before**
   calling `setScreen()`. `setScreen()` emits `screenChanged` synchronously, and
   `syncFromWindow()` compares against `m_requestedName`; with the old name still
   in place it would read our own move as placement loss and emit
   `placementLost(old)`, dragging the window straight back.
2. `setScreen()` + `setGeometry()` + `showFullScreen()`, mirroring
   `setTargetScreen()`.
3. Emits `targetScreenChanged`.

No new placement machinery. `src/main.cpp` already answers
`targetScreenChanged` with `enforceScreen()` after 60 ms, which is the KWin
script that performs the actual Wayland move; `qml/Main.qml` already answers it
with `syncToCurrentWallpaper()`.

### `qml/Main.qml`

On the carousel `ListView`:

```qml
Keys.onTabPressed: (event) => { if (!root.revealing) screenHelper.cycleScreen(1); event.accepted = true }
Keys.onBacktabPressed: (event) => { if (!root.revealing) screenHelper.cycleScreen(-1); event.accepted = true }
```

`event.accepted` is required or Qt Quick's default focus navigation also consumes
the key. The `revealing` guard matches `Keys.onEscapePressed`: once the reveal
animation is running the picker is on its way out.

## Feedback to the user

None added. The window physically moves to the other screen, which is the
feedback. No screen-name label.

## Testing

`tests/test_screenorder.cpp`, an assert-based `main()` built as its own target
alongside `wc-tests`, covering: left-to-right ordering independent of input
order, forward and backward wraparound, unknown `current` landing on the first
screen, and the single-screen no-op. The physical move needs a human on a
multi-monitor session.

## Out of scope

- Fixing the automatic active-screen guess (needs a logged run first).
- Handling a screen disappearing while the picker is already open
  (`QGuiApplication::screenRemoved`). The reported repro is unplug-then-launch,
  so this is speculative; `nextScreenName`'s unknown-`current` recovery already
  makes `Tab` a way out if it happens.
- Making the Tab binding configurable.
