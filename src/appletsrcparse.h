#pragma once

#include <QFile>
#include <QMap>
#include <QString>

// Finds the slide a carousel containment is currently showing, for one screen.
//
// Containments are told apart by the "CurrentScreen" key, which the wallpaper
// plugin writes with its own connector name. plasma's own "lastScreen=" is
// deliberately ignored: it holds a screen-pool id from a numbering that matches
// neither KWin's nor Qt's, so comparing it to anything else picks the wrong
// monitor on a multi-screen setup.
//
// Header-only so tests/test_appletsrcparse.cpp can drive it on a fixture file.
inline QString slideForScreen(const QString &configPath, const QString &screenName)
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    struct Slide { QString image; QString screen; };
    QMap<QString, Slide> found; // containment id -> slide
    QString section;
    QString currentId;
    const QString pluginSection = QStringLiteral("[Wallpaper][org.wallpapercarousel.slideshow][General]");
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.startsWith(QLatin1Char('['))) {
            section = line;
            // e.g. [Containments][442] or [Containments][442][Wallpaper]...
            const qsizetype open = section.indexOf(QLatin1String("[Containments]["));
            currentId.clear();
            if (open >= 0) {
                const qsizetype start = open + QStringLiteral("[Containments][").size();
                const qsizetype end = section.indexOf(QLatin1Char(']'), start);
                if (end > start) {
                    currentId = section.mid(start, end - start);
                }
            }
            continue;
        }
        if (currentId.isEmpty()) {
            continue;
        }
        if (section == QStringLiteral("[Containments][") + currentId + QLatin1Char(']')) {
            if (line.startsWith(QLatin1String("wallpaperplugin="))
                    && line.mid(strlen("wallpaperplugin=")) != QLatin1String("org.wallpapercarousel.slideshow")) {
                currentId.clear(); // not a carousel containment, skip the rest of its keys
            }
        } else if (section.endsWith(pluginSection)
                   && section.startsWith(QLatin1String("[Containments][") + currentId + QLatin1String("]"))) {
            if (line.startsWith(QLatin1String("CurrentImage="))) {
                found[currentId].image = line.mid(strlen("CurrentImage=")); // live, written by plugin
            } else if (line.startsWith(QLatin1String("Image=")) && found[currentId].image.isEmpty()) {
                found[currentId].image = line.mid(strlen("Image=")); // stale fallback
            } else if (line.startsWith(QLatin1String("CurrentScreen="))) {
                found[currentId].screen = line.mid(strlen("CurrentScreen="));
            }
        }
    }

    QString image;
    for (auto it = found.constBegin(); it != found.constEnd(); ++it) {
        if (it.value().image.isEmpty()) {
            continue;
        }
        if (!screenName.isEmpty() && it.value().screen == screenName) {
            image = it.value().image;
            break;
        }
        // Single carousel, or one predating CurrentScreen: unambiguous. With
        // several, a guess would sync to another monitor's slide, so report
        // nothing instead.
        if (found.size() == 1) {
            image = it.value().image;
        }
    }
    if (image.startsWith(QLatin1String("file://"))) {
        image = image.mid(7);
    }
    return image;
}
