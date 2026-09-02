#pragma once

#include <QObject>
#include <QQuickWindow>
#include <QScreen>
#include <QGuiApplication>

// Holds the screen the picker acts on, identified by connector name ("DP-1").
// Names are the only screen identity that survives the trip through KWin,
// plasmashell and appletsrc — the index spaces of those three disagree.
class ScreenHelper : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString targetScreenName READ targetScreenName NOTIFY targetScreenChanged)
    // False until placement has been re-asserted; the window stays transparent
    // until then so the KWin-side output correction isn't visible as a jump.
    Q_PROPERTY(bool placed READ placed NOTIFY placedChanged)

public:
    explicit ScreenHelper(QQuickWindow *window, QObject *parent = nullptr);

    QString targetScreenName() const { return m_targetScreenName; }
    bool placed() const { return m_placed; }

    // Tab / Shift+Tab. KWin's guess at the active output is wrong often enough
    // after a monitor hotplug that the user needs a way to move the picker by
    // hand. delta is +1 for the next screen, -1 for the previous one.
    Q_INVOKABLE void cycleScreen(int delta = 1);

public slots:
    // Empty name = KWin didn't answer; fall back to the window's own screen.
    // Moves the window onto that screen and shows it.
    void setTargetScreen(const QString &name);
    void markPlaced();

signals:
    void targetScreenChanged();
    void placedChanged();
    // The compositor put us on the wrong output; placement has to be re-asserted.
    void placementLost(const QString &wantedScreenName);

private slots:
    void syncFromWindow();

private:
    QQuickWindow *m_window;
    QString m_targetScreenName;
    // The name KWin gave us: authoritative, never overwritten by a compositor-
    // side move (position-memory scripts move us to last launch's output).
    QString m_requestedName;
    bool m_placed = false;
};
