// Smallest thing that fails if Tab hops to the wrong screen: run ./wc-tests-screenorder.
#include "../src/screenorder.h"

#include <cassert>
#include <cstdio>

int main()
{
    // Fed in deliberately jumbled: ordering must come from geometry, not input order.
    const QStringList three = orderScreenNames({
        {"HDMI-A-1", QRect(3840, 0, 1920, 1080)},
        {"eDP-1", QRect(0, 0, 1920, 1080)},
        {"DP-1", QRect(1920, 0, 1920, 1080)},
    });
    assert(three == QStringList({"eDP-1", "DP-1", "HDMI-A-1"}));

    // Same column, stacked: top one first.
    const QStringList stacked = orderScreenNames({
        {"DP-2", QRect(0, 1080, 1920, 1080)},
        {"DP-1", QRect(0, 0, 1920, 1080)},
    });
    assert(stacked == QStringList({"DP-1", "DP-2"}));

    // Mirrored outputs share a geometry; name breaks the tie so the order is stable.
    const QStringList mirrored = orderScreenNames({
        {"HDMI-A-1", QRect(0, 0, 1920, 1080)},
        {"eDP-1", QRect(0, 0, 1920, 1080)},
    });
    assert(mirrored == QStringList({"HDMI-A-1", "eDP-1"}));

    // Forward, including the wrap off the right edge.
    assert(nextScreenName(three, "eDP-1", 1) == "DP-1");
    assert(nextScreenName(three, "DP-1", 1) == "HDMI-A-1");
    assert(nextScreenName(three, "HDMI-A-1", 1) == "eDP-1");
    // Backward, including the wrap off the left edge.
    assert(nextScreenName(three, "DP-1", -1) == "eDP-1");
    assert(nextScreenName(three, "eDP-1", -1) == "HDMI-A-1");

    // Target screen was unplugged: land on the first live one rather than refuse
    // to move, so Tab is a way out of a dead target.
    assert(nextScreenName(three, "DVI-I-1", 1) == "eDP-1");
    assert(nextScreenName({"eDP-1"}, "DP-1", 1) == "eDP-1");

    // Nowhere to go: one screen and we're already on it, or no screens at all.
    assert(nextScreenName({"eDP-1"}, "eDP-1", 1).isEmpty());
    assert(nextScreenName({}, "eDP-1", 1).isEmpty());

    puts("ok");
    return 0;
}
