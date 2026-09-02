#include "screenhelper.h"
#include "screenorder.h"
#include <QDebug>

ScreenHelper::ScreenHelper(QQuickWindow *window, QObject *parent)
    : QObject(parent), m_window(window)
{
    if (m_window) {
        connect(m_window, &QWindow::screenChanged, this, &ScreenHelper::syncFromWindow);
    }
}

void ScreenHelper::setTargetScreen(const QString &name)
{
    if (!m_window) {
        return;
    }
    // First answer wins: the timeout fallback fires with an empty name and must
    // not stomp a name KWin already delivered.
    if (m_window->isVisible() && !m_targetScreenName.isEmpty()) {
        return;
    }

    QScreen *screen = nullptr;
    for (QScreen *s : QGuiApplication::screens()) {
        if (s->name() == name) {
            screen = s;
            break;
        }
    }
    if (!screen) {
        if (!name.isEmpty()) {
            qWarning() << "[ScreenHelper] no screen named" << name << "- using current";
        }
        screen = m_window->screen() ? m_window->screen() : QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    // ponytail: setScreen + showFullScreen is the only placement a Wayland
    // client gets — plain setGeometry cannot move a window between outputs.
    // Must happen before the first show or the output hint is ignored.
    m_window->setScreen(screen);
    m_window->setGeometry(screen->geometry());
    m_requestedName = name.isEmpty() ? QString() : screen->name();
    if (m_targetScreenName != screen->name()) {
        m_targetScreenName = screen->name();
        emit targetScreenChanged();
    }
    qDebug() << "[ScreenHelper] target screen" << m_targetScreenName;
    m_window->showFullScreen();
}

void ScreenHelper::cycleScreen(int delta)
{
    if (!m_window) {
        return;
    }

    QList<ScreenEntry> entries;
    QScreen *target = nullptr;
    for (QScreen *s : QGuiApplication::screens()) {
        entries.append(ScreenEntry{s->name(), s->geometry()});
    }
    const QString next = nextScreenName(orderScreenNames(entries), m_targetScreenName, delta);
    if (next.isEmpty() || next == m_targetScreenName) {
        qDebug() << "[ScreenHelper] no other screen to hop to";
        return;
    }
    for (QScreen *s : QGuiApplication::screens()) {
        if (s->name() == next) {
            target = s;
            break;
        }
    }
    if (!target) {
        return;
    }

    // Names first, setScreen() second: setScreen() emits screenChanged
    // synchronously, and syncFromWindow() compares against m_requestedName. With
    // the old name still in place it reads our own move as placement loss and
    // enforceScreen() drags the window straight back.
    m_requestedName = next;
    m_targetScreenName = next;
    qDebug() << "[ScreenHelper] hop to" << next << "want" << target->geometry()
             << "window was" << m_window->geometry()
             << "fullscreen" << (m_window->windowStates() & Qt::WindowFullScreen);

    m_window->setScreen(target);
    m_window->setGeometry(target->geometry());
    m_window->showFullScreen();
    qDebug() << "[ScreenHelper] after place, window" << m_window->geometry()
             << "on" << (m_window->screen() ? m_window->screen()->name() : QStringLiteral("none"));
    // main.cpp answers this with enforceScreen(), which is the KWin script that
    // performs the move Wayland won't let the client do itself; Main.qml answers
    // it by re-syncing the carousel to the new screen's wallpaper.
    emit targetScreenChanged();
}

void ScreenHelper::markPlaced()
{
    if (!m_placed) {
        m_placed = true;
        emit placedChanged();
    }
}

// The compositor may hand us a different screen than we asked for. When KWin
// named the screen, that name stays authoritative: adopting the compositor's
// choice would follow a position-memory script onto the wrong output and apply
// the wallpaper there.
void ScreenHelper::syncFromWindow()
{
    if (!m_window || !m_window->screen()) {
        return;
    }
    const QString name = m_window->screen()->name();
    if (!m_requestedName.isEmpty()) {
        if (name != m_requestedName) {
            qDebug() << "[ScreenHelper] compositor moved us to" << name << "- keeping" << m_requestedName;
            emit placementLost(m_requestedName);
        }
        return;
    }
    if (m_targetScreenName != name) {
        m_targetScreenName = name;
        emit targetScreenChanged();
    }
}
