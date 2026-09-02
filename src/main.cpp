#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QLoggingCategory>
#include "wallpapercontroller.h"
#include "imageloader.h"
#include "infinitelistmodel.h"
#include "screenhelper.h"

// qDebug goes nowhere on a Plasma-launched session (no stderr, nothing in
// journald), so WALLPAPER_CAROUSEL_LOG=/tmp/wc.log is the only way to see it.
static void fileLogger(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    static QFile log(qEnvironmentVariable("WALLPAPER_CAROUSEL_LOG"));
    if (!log.isOpen() && !log.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream(&log) << msg << '\n';
    log.flush();
}

int main(int argc, char *argv[]) {
    if (!qEnvironmentVariableIsEmpty("WALLPAPER_CAROUSEL_LOG")) {
        qInstallMessageHandler(fileLogger);
        // Plasma's session ships default rules that mute qDebug for uncategorised
        // logging; without this the log file only ever gets warnings.
        QLoggingCategory::setFilterRules(QStringLiteral("*.debug=true\nqt.*.debug=false"));
    }
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Ostrale");
    app.setApplicationName("Wallpaper Carousel");

    WallpaperController controller;
    ImageLoader loader(controller.wallpaperDir());
    InfiniteListModel infiniteModel;
    infiniteModel.setImages(loader.imageList());

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("controller", &controller);
    engine.rootContext()->setContextProperty("infiniteModel", &infiniteModel);

    engine.loadFromModule("CarouselUI", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    auto screenHelper = new ScreenHelper(window);
    engine.rootContext()->setContextProperty("screenHelper", screenHelper);

    // The window starts hidden: on Wayland its output can only be chosen before
    // the first show, and KWin answers asynchronously.
    QObject::connect(&controller, &WallpaperController::activeScreenReported,
                     screenHelper, &ScreenHelper::setTargetScreen);
    // Placement is re-asserted once the window is mapped, because KWin position-
    // memory scripts pull it to the output it opened on last time. The window is
    // transparent until the correction has landed, so the move isn't visible.
    QObject::connect(screenHelper, &ScreenHelper::targetScreenChanged, &controller,
                     [&controller, screenHelper] {
                         const QString name = screenHelper->targetScreenName();
                         QTimer::singleShot(60, &controller, [&controller, name] {
                             controller.enforceScreen(name);
                         });
                         QTimer::singleShot(180, screenHelper, [screenHelper] {
                             screenHelper->markPlaced();
                         });
                     });
    // Position-memory scripts can move us again after the map, so every stray
    // move is answered instead of being corrected only once.
    QObject::connect(screenHelper, &ScreenHelper::placementLost, &controller,
                     &WallpaperController::enforceScreen);
    controller.queryActiveScreen();
    // No KWin, no kwin_scripting, script rejected: show anyway rather than hang.
    QTimer::singleShot(200, screenHelper, [screenHelper] { screenHelper->setTargetScreen(QString()); });

    QObject::connect(&loader, &ImageLoader::imagesChanged, &infiniteModel, &InfiniteListModel::setImages);

    return app.exec();
}

