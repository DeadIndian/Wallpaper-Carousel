#pragma once
#include <QObject>
#include <QString>
#include <QScreen>
#include <QGuiApplication>
#include <QDBusInterface>
#include <QMap>
#include <QStringList>

class WallpaperController : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.wallpapercarousel")
    // Read once at startup from config.toml; 0 disables the reveal animation.
    Q_PROPERTY(int animationDuration READ animationDuration CONSTANT)
public:
    explicit WallpaperController(QObject *parent = nullptr);

    int animationDuration() const { return m_animationDuration; }

    // screenName is a connector name ("DP-1"), see ScreenHelper.
    Q_INVOKABLE void applyWallpaperForScreen(const QString &screenName, const QString &imagePath);
    Q_INVOKABLE QString wallpaperDir() const;
    Q_INVOKABLE QString currentWallpaper(const QString &screenName) const;
    // Asks KWin which output is active; answer arrives on activeScreenReported.
    void queryActiveScreen();
    // Re-places the picker after it is mapped: window-position-memory scripts
    // (e.g. rememberwindowpositions) move it to wherever it opened last time,
    // which beats the setScreen() output hint.
    void enforceScreen(const QString &screenName);

public slots:
    Q_SCRIPTABLE void setWindowsMinimized(bool minimized);
    Q_SCRIPTABLE void rememberWindow(const QString &internalId);
    Q_SCRIPTABLE void reportActiveScreen(const QString &screenName, int cursorX, int cursorY);

signals:
    void error(const QString &msg);
    void activeScreenReported(const QString &screenName);
private:
    QString m_wallpaperDir;
    QString m_theme;
    int m_animationDuration = 700;
    QStringList m_minimizedIds;
    QMap<QString, QPair<QString, QString>> m_pluginConfig;
    QMap<QString, QString> m_extensionCategoryMap = {};
    bool setPlasmaWallpaperJavaScript(const QString &screenName, const QString &imagePath, const QString &pluginType, const QString &writeKey);

};
