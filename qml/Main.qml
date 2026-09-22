import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import Qt5Compat.GraphicalEffects

Window {
    id: root
    // Shown by ScreenHelper once KWin has told us which output is active.
    visible: false
    flags: Qt.FramelessWindowHint | Qt.Tool
    color: "transparent"
    width: Screen.width
    height: Screen.height

    onVisibleChanged: {
        controller.setWindowsMinimized(visible)
        if (visible) {
            syncToCurrentWallpaper()
            stateProbe.kick()
        }
    }

    // One-shot: the picker is on its way out, no second Enter/Escape.
    property bool revealing: false

    // Reveal first, apply after: plasma switching the wallpaper mid-animation
    // would change the desktop *behind* the overlay and kill the effect.
    function startReveal(url) {
        var item = carousel.currentItem;
        if (revealing || !item) {
            applyAndClose(url);
            return;
        }
        revealing = true;
        // null = scene coordinates. Not `root`: mapToItem() takes an Item and
        // throws on a Window, which silently killed the whole reveal.
        var seed = item.mapToItem(null, item.width / 2, item.height / 2);
        container.opacity = 0;
        // Never let a broken animation strand the picker on screen with the
        // wallpaper unset.
        var started = false;
        try {
            started = reveal.start(url, seed.x, seed.y);
        } catch (e) {
            console.log("[reveal] failed:", e);
        }
        if (!started) {
            console.log("[reveal] no animation, applying directly");
            applyAndClose(url);
        }
    }

    function applyAndClose(url) {
        controller.applyWallpaperForScreen(screenHelper.targetScreenName, url);
        // Grace period so plasmashell has painted the real wallpaper before the
        // overlay goes away — otherwise the old one flashes back for a frame.
        closeTimer.start();
    }

    Timer {
        id: closeTimer
        interval: 150
        onTriggered: root.close()
    }

    // Deferred tail of syncToCurrentWallpaper: the animated hop to the target
    // slide, fired once the ListView has done its initial layout.
    Timer {
        id: spinTimer
        interval: 50
        property int target: -1
        function spinTo(t) { target = t; restart(); }
        onTriggered: {
            carousel.highlightMoveDuration = 6 * 100; // per-item, like arrow keys
            carousel.currentIndex = target;
        }
    }

    function syncToCurrentWallpaper() {
        if (typeof screenHelper === "undefined" || !screenHelper) return;
        var current = controller.currentWallpaper(screenHelper.targetScreenName);
        var n = infiniteModel.imageCount();
        if (!current || n === 0) return;
        var i = infiniteModel.indexOfFile(current);
        console.log("[sync] screen=" + screenHelper.targetScreenName + " current=" + current + " index=" + i + " count=" + n);
        if (i < 0) return;
        var target = 100000 - (100000 % n) + i;
        // Already in place or spinning there: sync runs twice (screen known,
        // then placed) — don't restart the spin.
        if (Math.abs(carousel.currentIndex - target) <= 6) return;
        // Same virtual-index trick as the 100000 default, but on the current slide.
        // Teleport to a few slides before the target, then let the list spin the
        // rest the same way the arrow keys move: animating the whole distance
        // (tens of thousands of px) lands off-target, a short run doesn't.
        // Index changes in the same run never animate — the view is still doing
        // its initial layout — so the spin is deferred past that by spinTimer.
        carousel.highlightMoveDuration = 0;
        carousel.currentIndex = target - 6;
        carousel.positionViewAtIndex(carousel.currentIndex, ListView.SnapPosition);
        spinTimer.spinTo(target);
    }
    // Target screen is known only after KWin answers, and the compositor can
    // still put us elsewhere — re-seek whenever it settles.
    Connections {
        target: typeof screenHelper !== "undefined" ? screenHelper : null
        function onTargetScreenChanged() {
            if (root.visible) root.syncToCurrentWallpaper()
        }
        // Re-seek once the window is actually on its final output and laid out:
        // the first sync can run before the ListView has a width.
        function onPlacedChanged() {
            if (root.visible && screenHelper.placed) root.syncToCurrentWallpaper()
        }
    }

    // TEMPORARY diagnostics for the "carousel invisible after Tab hop" bug.
    // Remove once the cause is confirmed.
    function logState(tag) {
        var names = []
        for (var i = 0; i < Qt.application.screens.length; ++i)
            names.push(Qt.application.screens[i].name + "@"
                + Qt.application.screens[i].virtualX + ","
                + Qt.application.screens[i].virtualY)
        console.log("[state " + tag + "]"
            + " target=" + screenHelper.targetScreenName
            + " placed=" + screenHelper.placed
            + " qtScreens=[" + names.join(" ") + "]"
            + " Screen=" + Screen.name + " " + Screen.width + "x" + Screen.height
            + " dpr=" + Screen.devicePixelRatio
            + " vis_state=" + visibility
            + " root=" + width + "x" + height + " vis=" + visible + " active=" + active
            + " container=" + container.width + "x" + container.height
            + " @" + Math.round(container.x) + "," + Math.round(container.y)
            + " op=" + container.opacity
            + " list=" + carousel.width + "x" + carousel.height
            + " contentX=" + Math.round(carousel.contentX)
            + " idx=" + carousel.currentIndex + " count=" + carousel.count)
    }

    onWidthChanged: logState("width")
    onHeightChanged: logState("height")

    Timer {
        id: stateProbe
        interval: 400
        repeat: true
        property int shots: 0
        onTriggered: {
            root.logState("probe" + (++shots))
            if (shots >= 4) stop()
        }
        function kick() { shots = 0; restart() }
    }

    Rectangle {
        id: container
        width: Math.min(parent.width * 0.8, 5 * 220 + 4 * 20) // 5 images + 4 spaces
        height: 180
        x: (parent.width - width)/2
        y: parent.height - height - 40
        radius: 14
        color: Qt.rgba(1,1,1,0.00)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        // Hidden until the output correction has landed: window opacity is a
        // no-op on Wayland ("plugin does not support setting window opacity"),
        // so the content is what gets faded instead.
        opacity: (typeof screenHelper !== "undefined" && screenHelper.placed) ? 0.98 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }

        ListView {
            id: carousel
            anchors.fill: parent
            model: infiniteModel
            orientation: ListView.Horizontal
            boundsBehavior: Flickable.StopAtBounds
            snapMode: ListView.SnapToItem
            highlightRangeMode: ListView.StrictlyEnforceRange
            spacing: 20
            focus: true
            clip: true
            currentIndex: 100000

            preferredHighlightBegin: width / 2 - 110
            preferredHighlightEnd: width / 2 - 110

            delegate: Item {
                width: 220
                height: carousel.height

                Image {
                    anchors.fill: parent
                    source: (cachedUrl ? cachedUrl : url)
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    id: img
                    property bool rounded: true

                    layer.enabled: rounded
                    layer.effect: OpacityMask {
                        maskSource: Item {
                            width: img.width
                            height: img.height
                            Rectangle {
                                anchors.centerIn: parent
                                width: 220
                                height: carousel.height
                                radius: Math.min(width/10, height/10)
                            }
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        carousel.currentIndex = index
                    }
                }
            }

            Keys.onLeftPressed: { currentIndex-- }
            Keys.onRightPressed: { currentIndex++ }
            // Restore snappy per-item duration after a long accelerated jump
            onMovementEnded: highlightMoveDuration = 100
            Keys.onReturnPressed: {
                var realUrl = infiniteModel.urlAt(currentIndex)
                if (realUrl) {
                    root.startReveal(realUrl);
                } else {
                    root.close()
                }
            }
            Keys.onEscapePressed: { if (!root.revealing) root.close() }
            // Manual override for the active-screen guess: KWin reports the wrong
            // output after a monitor hotplug, and without this the picker is
            // stuck there. event.accepted or Qt Quick's focus navigation eats it.
            Keys.onTabPressed: (event) => {
                if (!root.revealing) {
                    root.logState("pre-tab")
                    screenHelper.cycleScreen(1)
                    root.logState("post-tab")
                    stateProbe.kick()
                }
                event.accepted = true
            }
            Keys.onBacktabPressed: (event) => {
                if (!root.revealing) {
                    screenHelper.cycleScreen(-1)
                    stateProbe.kick()
                }
                event.accepted = true
            }
        }
    }

    RevealOverlay {
        id: reveal
        anchors.fill: parent
        duration: controller.animationDuration
        onRevealFinished: root.applyAndClose(reveal.source)
    }
}

