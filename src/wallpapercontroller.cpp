#include "wallpapercontroller.h"
#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QGuiApplication>
#include <QDBusReply>
#include <QDBusConnection>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <toml++/toml.hpp>
#include "appletsrcparse.h"

WallpaperController::WallpaperController(QObject *parent)
    : QObject(parent)
{
    // 🔹 Initialisation des correspondances extensions -> catégories
    m_extensionCategoryMap["png"] = "image";
    m_extensionCategoryMap["jpg"] = "image";
    m_extensionCategoryMap["jpeg"] = "image";
    m_extensionCategoryMap["bmp"] = "image";
    m_extensionCategoryMap["gif"] = "animation";
    m_extensionCategoryMap["webp"] = "animation";
    m_extensionCategoryMap["mp4"] = "video";
    m_extensionCategoryMap["webm"] = "video";

    QString configDir = QDir::homePath() + "/.config/wallpaper-carousel";
    QString configPath = configDir + "/config.toml";
    QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString defaultDir = pictures + "/Wallpapers";

    // 🔹 Création du dossier de configuration
    QDir().mkpath(configDir);

    QString wallpaperDir;

    if (!QFile::exists(configPath)) {
        // 🔹 Si le fichier n'existe pas, on le crée avec les valeurs par défaut
        QFile file(configPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "# Wallpaper Carousel configuration\n";
            out << "# wallpaper_dir: path to folder with wallpapers\n";
            out << "wallpaper_dir = \"" << defaultDir << "\"\n";
            out << "\n# animation_duration_ms: length of the wallpaper reveal animation.\n";
            out << "# 0 disables it and applies the wallpaper immediately.\n";
            out << "animation_duration_ms = 700\n";
            out << "\n# [plugins] section allows you to define custom wallpaper plugins\n";
            out << "# format: [plugins.category]\n";
            out << "# type = \"PluginType\"\n";
            out << "# write_key = \"WriteKey\"\n";
            out << "\n[plugins.image]\n";
            out << "type = \"org.kde.image\"\n";
            out << "write_key = \"Image\"\n";
            out << "\n[plugins.video]\n";
            out << "type = \"luisbocanegra.smart.video.wallpaper.reborn\"\n";
            out << "write_key = \"VideoUrls\"\n";
            out << "\n[plugins.animation]\n";
            out << "type = \"org.kde.image\"\n";
            out << "write_key = \"Image\"\n";
            file.close();
            qDebug() << "[WallpaperController] Created default config at" << configPath;
        }
        wallpaperDir = defaultDir;
    } else {
        try {
            toml::table tbl = toml::parse_file(configPath.toStdString());

            if (auto val = tbl["wallpaper_dir"].value<std::string>()) {
                QString candidate = QString::fromStdString(*val);
                candidate.replace("$HOME", QDir::homePath());
                wallpaperDir = candidate;
            }

            if (auto val = tbl["animation_duration_ms"].value<int64_t>()) {
                // Clamped: a negative duration makes NumberAnimation misbehave and
                // anything over ~5 s just holds the user's windows minimized.
                m_animationDuration = qBound<int>(0, static_cast<int>(*val), 5000);
            }

            // 🔹 Lecture de la configuration des plugins (par catégorie)
            if (auto pluginsTable = tbl.get_as<toml::table>("plugins")) {
                for (auto&& [key, val] : *pluginsTable) {
                    if (auto plugin = val.as_table()) {
                        if (auto typeVal = plugin->get_as<std::string>("type")) {
                            if (auto writeKeyVal = plugin->get_as<std::string>("write_key")) {
                                QString category = QString::fromStdString(std::string(key.str()));
                                QString pluginType = QString::fromStdString(std::string(*typeVal));
                                QString writeKey = QString::fromStdString(std::string(*writeKeyVal));
                                m_pluginConfig[category] = qMakePair(pluginType, writeKey);
                                qDebug() << "[WallpaperController] Loaded custom plugin config for category:" << category;
                            }
                        }
                    }
                }
            }
        }
        catch (const toml::parse_error &err) {
            qWarning() << "[WallpaperController] Failed to parse config:" << err.description().data();
            wallpaperDir = defaultDir;
        }
    }

    if (wallpaperDir.isEmpty()) {
        wallpaperDir = defaultDir;
    }

    m_wallpaperDir = wallpaperDir;

    qDebug() << "[WallpaperController] Initialized with wallpaperDir:" << m_wallpaperDir;

    QDBusConnection::sessionBus().registerService("org.wallpapercarousel");
    QDBusConnection::sessionBus().registerObject("/controller", this,
                                                 QDBusConnection::ExportScriptableSlots);
}

QString WallpaperController::wallpaperDir() const {
    qDebug() << "[wallpaperDir] Returning:" << m_wallpaperDir;
    return m_wallpaperDir;
}

// Run a one-shot KWin script. print() output is hidden by default logging,
// so scripts only communicate via callDBus back to rememberWindow().
// ponytail: unique name per run — KWin won't re-run a script under an
// already-loaded pluginName; finished scripts pile up until KWin restarts.
static int s_scriptCounter = 0;
static void runKWinScript(const QString &pluginName, const QString &js)
{
    // PID: counter alone resets per launch, second process would reuse a
    // pluginName KWin already has loaded and silently refuse the script.
    QString uniqueName = QStringLiteral("%1-%2-%3")
                             .arg(pluginName)
                             .arg(QCoreApplication::applicationPid())
                             .arg(++s_scriptCounter);
    QString path = QDir::temp().absoluteFilePath(uniqueName + ".js");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[runKWinScript] cannot write" << path;
        return;
    }
    file.write(js.toUtf8());
    file.close();

    QDBusInterface scripting("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting");
    scripting.call("loadScript", path, uniqueName);
    scripting.call("start");
}

void WallpaperController::rememberWindow(const QString &internalId)
{
    // Anything but a KWin internalId is dropped: the ids are interpolated into a
    // JS array literal, so one stray string makes the whole restore script a
    // syntax error and *every* window stays minimized.
    static const QRegularExpression uuid(
        QStringLiteral("^\\{[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\\}$"));
    if (!uuid.match(internalId).hasMatch()) {
        qWarning() << "[rememberWindow] ignoring non-id" << internalId;
        return;
    }
    qDebug() << "[rememberWindow]" << internalId;
    m_minimizedIds.append(internalId);
}

// Wayland gives Qt no usable global cursor position before our surface is
// mapped, so the compositor is the only source for "which screen do I open on".
// evaluateScript/loadScript return values don't marshal, hence the callDBus
// round trip.
//
// The cursor wins over workspace.activeScreen: activeScreen tracks the focused
// window unless the user turned on active-screen-follows-mouse, so it sticks to
// whichever screen holds focus and the picker would keep opening there.
// activeScreen is only the fallback when there's no pointer.
void WallpaperController::queryActiveScreen()
{
    runKWinScript("wallpapercarousel-activescreen", QStringLiteral(R"(
const p = workspace.cursorPos;
const out = workspace.activeScreen;
callDBus("org.wallpapercarousel", "/controller", "org.wallpapercarousel", "reportActiveScreen",
         out ? "" + out.name : "", Math.round(p.x), Math.round(p.y));
)"));
}

void WallpaperController::reportActiveScreen(const QString &screenName, int cursorX, int cursorY)
{
    // Resolved here rather than with workspace.screenAt(): that returned the
    // active output for a point that sits on the other screen.
    QString name = screenName;
    if (QScreen *s = QGuiApplication::screenAt(QPoint(cursorX, cursorY))) {
        name = s->name();
    }
    qDebug() << "[reportActiveScreen] cursor" << cursorX << cursorY << "active" << screenName << "->" << name;
    emit activeScreenReported(name);
}

void WallpaperController::enforceScreen(const QString &screenName)
{
    if (screenName.isEmpty()) {
        return;
    }
    // fullScreen has to drop first: sendClientToScreen() is a measured no-op on a
    // fullscreen window.
    runKWinScript("wallpapercarousel-place", QStringLiteral(R"(
const target = workspace.screens.find(s => s.name === "%1");
for (const w of workspace.windowList()) {
    if (w.resourceClass === "wallpaper-carousel" && target) {
        if (w.output.name !== target.name) {
            w.fullScreen = false;
            workspace.sendClientToScreen(w, target);
            w.fullScreen = true;
        }
        // Window-management scripts (e.g. RememberWindowPositions) restore a
        // stale saved geometry on top of the fullscreen state, shrinking the
        // picker to a corner of the screen. Re-assert the full output size.
        if (w.fullScreen && (w.frameGeometry.width !== target.geometry.width
                || w.frameGeometry.height !== target.geometry.height)) {
            w.frameGeometry = target.geometry;
        }
    }
}
)").arg(screenName));
}

void WallpaperController::setWindowsMinimized(bool minimized)
{
    if (minimized) {
        m_minimizedIds.clear();
        // KWin script: minimize every window except our own; report each
        // touched window back so restore only undoes exactly these.
        runKWinScript("wallpapercarousel-engage", QStringLiteral(R"(
for (const w of workspace.windowList()) {
    if (!w.minimized && w.normalWindow && w.resourceClass !== "wallpaper-carousel") {
        callDBus("org.wallpapercarousel", "/controller", "org.wallpapercarousel", "rememberWindow", "" + w.internalId);
        w.minimized = true;
    }
}
)"));
    } else {
        // Quoted per element; validated in rememberWindow(), so no escaping needed.
        QStringList quoted;
        for (const QString &id : m_minimizedIds) {
            quoted << QLatin1Char('"') + id + QLatin1Char('"');
        }
        const QString ids = QLatin1Char('[') + quoted.join(QLatin1Char(',')) + QLatin1Char(']');
        runKWinScript("wallpapercarousel-restore", QStringLiteral(R"(
const ids = %1;
for (const w of workspace.windowList()) {
    if (ids.indexOf("" + w.internalId) !== -1) {
        w.minimized = false;
    }
}
)").arg(ids));
        m_minimizedIds.clear();
    }
}

// The slide currently shown comes from plasma's appletsrc: the plugin rewrites
// "CurrentImage" on every rotation. (evaluateScript's return value doesn't
// marshal over DBus, and QSettings can't parse plasma's "[Containments][442]"
// section names, hence the hand-rolled scan in appletsrcparse.h.)
QString WallpaperController::currentWallpaper(const QString &screenName) const
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + QLatin1String("/plasma-org.kde.plasma.desktop-appletsrc");
    const QString image = slideForScreen(path, screenName);
    qDebug() << "[currentWallpaper]" << screenName << image;
    return image;
}

void WallpaperController::applyWallpaperForScreen(const QString &screenName, const QString &imagePath)
{
    qDebug() << "[applyWallpaperForScreen] Called with screen:" << screenName << "imagePath:" << imagePath;

    QString path = imagePath;
    if (path.startsWith("file://")) {
        path = path.mid(7);
        qDebug() << "[applyWallpaperForScreen] Stripped file:// ->" << path;
    }

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        QString errorMsg = QString("Image not found: %1").arg(path);
        qWarning() << "[applyWallpaperForScreen] " << errorMsg;
        emit error(errorMsg);
        return;
    }

    QString fileExtension = fileInfo.suffix().toLower();
    QString pluginType = "org.kde.image";
    QString writeKey = "Image";

    // 🔹 Logique de déduction : on cherche la catégorie à partir de l'extension
    QString category;
    if (m_extensionCategoryMap.contains(fileExtension)) {
        category = m_extensionCategoryMap[fileExtension];
    } else {
        qWarning() << "[applyWallpaperForScreen] No category found for extension:" << fileExtension << "Using default 'image' category.";
        category = "image";
    }

    // 🔹 On utilise la catégorie pour trouver les informations du plugin
    if (m_pluginConfig.contains(category)) {
        pluginType = m_pluginConfig[category].first;
        writeKey = m_pluginConfig[category].second;
        qDebug() << "[applyWallpaperForScreen] Found plugin config for category:" << category << "-> Type:" << pluginType << "WriteKey:" << writeKey;
    } else {
        qDebug() << "[applyWallpaperForScreen] Plugin config not found for category:" << category << "Using default 'org.kde.image'.";
    }

    if (!setPlasmaWallpaperJavaScript(screenName, path, pluginType, writeKey)) {
        QString errorMsg = QString("Failed to set wallpaper for screen %1").arg(screenName);
        qWarning() << "[applyWallpaperForScreen] " << errorMsg;
        emit error(errorMsg);
    } else {
        qDebug() << "[applyWallpaperForScreen] Wallpaper applied successfully.";
    }
}

// Reste du code pour setPlasmaWallpaperJavaScript...
bool WallpaperController::setPlasmaWallpaperJavaScript(const QString &screenName, const QString &imagePath, const QString &pluginType, const QString &writeKey)
{
    qDebug() << "[setPlasmaWallpaperJavaScript] Called with screen:" << screenName << "imagePath:" << imagePath << "pluginType:" << pluginType << "writeKey:" << writeKey;

    QString path = imagePath;
    if (path.startsWith("file://")) {
        path = path.mid(7);
        qDebug() << "[setPlasmaWallpaperJavaScript] Stripped file:// ->" << path;
    }

    // Desktops are picked by geometry, not by index: plasma's d.screen ids are
    // its own screen-pool numbering and don't line up with Qt's screen list.
    // A point inside the target screen is unambiguous in both.
    QScreen *screen = nullptr;
    for (QScreen *s : QGuiApplication::screens()) {
        if (s->name() == screenName) {
            screen = s;
            break;
        }
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
        qWarning() << "[setPlasmaWallpaperJavaScript] unknown screen" << screenName
                   << "- falling back to" << (screen ? screen->name() : QStringLiteral("none"));
    }
    if (!screen) {
        return false;
    }
    const QPoint center = screen->geometry().center();

    QString js = QString(R"(
const px = %1, py = %2;
const wantScreen = "%6";
const isCarousel = d => d.wallpaperPlugin === "org.kde.slideshow"
        || d.wallpaperPlugin === "org.wallpapercarousel.slideshow";
// Plasma 6 reports d.screen === -1 for assigned outputs, so geometry matching
// finds nothing and the wallpaper never changes. The carousel plugin stamps
// its connector into CurrentScreen — match on that first. Geometry stays as a
// fallback for the day d.screen works again; single-carousel as a last resort.
let targetDesktop = desktops().find(d => {
    if (!isCarousel(d)) {
        return false;
    }
    d.currentConfigGroup = ["Wallpaper", d.wallpaperPlugin, "General"];
    return d.readConfig("CurrentScreen") === wantScreen;
});
if (!targetDesktop) {
    targetDesktop = desktops().find(d => {
        if (d.screen === -1) {
            return false;
        }
        const g = screenGeometry(d.screen);
        return px >= g.left && px < g.left + g.width && py >= g.top && py < g.top + g.height;
    });
}
if (!targetDesktop) {
    const carousels = desktops().filter(isCarousel);
    if (carousels.length === 1) {
        targetDesktop = carousels[0];
    }
}

if (targetDesktop) {
    if (targetDesktop.wallpaperPlugin === "org.kde.slideshow"
            || targetDesktop.wallpaperPlugin === "org.wallpapercarousel.slideshow") {
        // Carousel Slideshow displays the "Image" config key directly; writing it
        // switches the current slide and the QML timer keeps running.
        targetDesktop.currentConfigGroup = ["Wallpaper", targetDesktop.wallpaperPlugin, "General"];
        targetDesktop.writeConfig("Image", "file://%5");
    } else {
        targetDesktop.wallpaperPlugin = "%3";
        targetDesktop.currentConfigGroup = ["Wallpaper", "%3", "General"];
        targetDesktop.writeConfig("Image", "file:///dev/null");
        targetDesktop.writeConfig("%4", "file://%5");
    }
}
)").arg(center.x()).arg(center.y()).arg(pluginType).arg(writeKey).arg(path).arg(screenName);

    qDebug() << "[setPlasmaWallpaperJavaScript] JavaScript being sent to DBus:\n" << js;

    QDBusInterface iface("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell");
    if (!iface.isValid()) {
        qWarning() << "[setPlasmaWallpaperJavaScript] plasmashell DBus interface not available";
        return false;
    }

    QDBusReply<QString> reply = iface.call("evaluateScript", js);
    if (!reply.isValid()) {
        qWarning() << "[setPlasmaWallpaperJavaScript] evaluateScript call failed:" << reply.error().message();
        return false;
    }

    qDebug() << "[setPlasmaWallpaperJavaScript] Wallpaper script executed successfully.";
    return true;
}
