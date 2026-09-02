import QtQuick 2.15
import Qt5Compat.GraphicalEffects

// Expanding circular ("bubble") reveal of the picked wallpaper, grown out of the
// thumbnail the user selected. Sits on top of the whole picker window; behind it
// the desktop still shows the *old* wallpaper, which is what makes the reveal
// read as a wallpaper change.
Item {
    id: overlay
    visible: false

    signal revealFinished()

    property real seedX: 0
    property real seedY: 0
    property real radius: 0
    // Set from config.toml (animation_duration_ms); 0 = no animation at all.
    property int duration: 700
    readonly property alias source: image.source

    // Farthest screen corner from the seed: the circle has to reach it to cover
    // the screen completely.
    readonly property real maxRadius: {
        var dx = Math.max(seedX, width - seedX);
        var dy = Math.max(seedY, height - seedY);
        return Math.sqrt(dx * dx + dy * dy);
    }

    // Returns false when the image isn't decoded yet: the caller then applies the
    // wallpaper straight away rather than sitting on a blank screen.
    function start(url, x, y) {
        if (duration <= 0) return false;
        seedX = x;
        seedY = y;
        image.source = url;
        if (image.status !== Image.Ready) return false;
        visible = true;
        grow.restart();
        return true;
    }

    Image {
        id: image
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        smooth: true
        // ponytail: same Image for video files — it renders the poster frame or
        // nothing, and the crop handles the scale. QtMultimedia here if that
        // ever looks wrong.
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Item {
                width: overlay.width
                height: overlay.height
                Rectangle {
                    x: overlay.seedX - overlay.radius
                    y: overlay.seedY - overlay.radius
                    width: overlay.radius * 2
                    height: overlay.radius * 2
                    radius: overlay.radius
                }
            }
        }
    }

    NumberAnimation {
        id: grow
        target: overlay
        property: "radius"
        from: 0
        to: overlay.maxRadius
        duration: overlay.duration
        easing.type: Easing.OutCubic
        onFinished: overlay.revealFinished()
    }
}
