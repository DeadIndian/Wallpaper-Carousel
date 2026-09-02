#pragma once

#include <QList>
#include <QRect>
#include <QString>
#include <QStringList>

#include <algorithm>

// Ordering for the Tab screen hop. Kept free of QScreen/QWindow so it can be
// asserted in tests the way appletsrcparse.h is; ScreenHelper feeds it the
// name+geometry pairs it reads off QGuiApplication::screens().
struct ScreenEntry {
    QString name;
    QRect geometry;
};

// Left to right, then top to bottom: Tab should walk the outputs in the order
// they sit on the desk, not in the order KWin happens to enumerate them.
inline QStringList orderScreenNames(QList<ScreenEntry> entries)
{
    std::sort(entries.begin(), entries.end(), [](const ScreenEntry &a, const ScreenEntry &b) {
        if (a.geometry.x() != b.geometry.x()) {
            return a.geometry.x() < b.geometry.x();
        }
        if (a.geometry.y() != b.geometry.y()) {
            return a.geometry.y() < b.geometry.y();
        }
        // Mirrored outputs share a geometry; the name keeps the order stable.
        return a.name < b.name;
    });

    QStringList names;
    names.reserve(entries.size());
    for (const ScreenEntry &entry : entries) {
        names << entry.name;
    }
    return names;
}

// Empty return means "nowhere to go" — don't move the window.
inline QString nextScreenName(const QStringList &ordered, const QString &current, int delta)
{
    const int n = ordered.size();
    if (n == 0) {
        return QString();
    }
    const int i = ordered.indexOf(current);
    // Current screen isn't in the list: it was unplugged. Land on the first live
    // one instead of refusing to move, so Tab is a way out of a dead target.
    if (i < 0) {
        return ordered.first();
    }
    if (n < 2) {
        return QString();
    }
    // Wrap in both directions; C++ % keeps the sign of the dividend.
    return ordered.at(((i + delta) % n + n) % n);
}
