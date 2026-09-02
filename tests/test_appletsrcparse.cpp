// Smallest thing that fails if appletsrc parsing breaks: run ./wc-tests.
#include "../src/appletsrcparse.h"

#include <QTemporaryFile>
#include <cassert>
#include <cstdio>

static QString parse(const QByteArray &conf, const QString &screen)
{
    QTemporaryFile f;
    assert(f.open());
    f.write(conf);
    f.flush();
    return slideForScreen(f.fileName(), screen);
}

int main()
{
    // Two carousels on two screens: each must get its own slide, not the other's.
    const QByteArray twoScreens =
        "[Containments][10]\n"
        "lastScreen=0\n"
        "wallpaperplugin=org.wallpapercarousel.slideshow\n"
        "[Containments][10][Wallpaper][org.wallpapercarousel.slideshow][General]\n"
        "CurrentImage=file:///pics/left.png\n"
        "CurrentScreen=DP-1\n"
        "[Containments][20]\n"
        "lastScreen=0\n"                 // plasma pool ids collide/lie; must be ignored
        "wallpaperplugin=org.wallpapercarousel.slideshow\n"
        "[Containments][20][Wallpaper][org.wallpapercarousel.slideshow][General]\n"
        "CurrentImage=file:///pics/right.png\n"
        "CurrentScreen=HDMI-A-1\n";
    assert(parse(twoScreens, "DP-1") == "/pics/left.png");
    assert(parse(twoScreens, "HDMI-A-1") == "/pics/right.png");
    // Unknown screen: reporting some other monitor's slide would be worse.
    assert(parse(twoScreens, "eDP-1").isEmpty());
    assert(parse(twoScreens, QString()).isEmpty());

    // One carousel, no CurrentScreen yet (plugin hasn't rotated since upgrade).
    const QByteArray oneOld =
        "[Containments][10]\n"
        "wallpaperplugin=org.wallpapercarousel.slideshow\n"
        "[Containments][10][Wallpaper][org.wallpapercarousel.slideshow][General]\n"
        "Image=file:///pics/only.png\n";
    assert(parse(oneOld, "DP-1") == "/pics/only.png");

    // Other wallpaper plugins must not be mistaken for ours.
    const QByteArray foreign =
        "[Containments][10]\n"
        "wallpaperplugin=org.kde.image\n"
        "[Containments][10][Wallpaper][org.kde.image][General]\n"
        "Image=file:///pics/other.png\n";
    assert(parse(foreign, "DP-1").isEmpty());

    // CurrentImage (live) wins over Image (stale), whatever the order.
    const QByteArray both =
        "[Containments][10]\n"
        "wallpaperplugin=org.wallpapercarousel.slideshow\n"
        "[Containments][10][Wallpaper][org.wallpapercarousel.slideshow][General]\n"
        "Image=file:///pics/stale.png\n"
        "CurrentImage=file:///pics/live.png\n"
        "CurrentScreen=DP-1\n";
    assert(parse(both, "DP-1") == "/pics/live.png");

    puts("ok");
    return 0;
}
