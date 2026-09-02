#include "infinitelistmodel.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QCryptographicHash>
#include <limits>

#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QEventLoop>
#include <QTimer>

// Génère une vignette synchronement (1ère frame ou proche)
static QString generateVideoThumbnail(const QString &path, int w=480, int h=270) {
    QFileInfo fi(path);
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty()) {
        cacheDir = QDir::homePath() + "/.cache/wallpaper-carousel";
    }
    QDir().mkpath(cacheDir);
    QString thumbFile = cacheDir + QDir::separator() + fi.completeBaseName() + ".thumb.webp";

    if (QFileInfo::exists(thumbFile)) {
        return thumbFile;
    }

    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);

    QEventLoop loop;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, [&](const QVideoFrame &frame) {
        QImage img = frame.toImage();
        if (!img.isNull()) {
            QImage scaled = img.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            scaled.save(thumbFile, "WEBP");
            loop.quit();
        }
    });

    player.setSource(QUrl::fromLocalFile(path));
    player.play();

    // sécurité : ne pas bloquer éternellement (timeout 2s)
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();

    if (QFileInfo::exists(thumbFile)) {
        return thumbFile;
    }
    return QString(":/icons/video_placeholder.png"); // fallback si échec
}


static QString cachedPathFor(const QString &original, int w=1920 / 4, int h=1080 / 4) {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty()) {
        cacheDir = QDir::homePath() + "/.cache/wallpaper-carousel";
    }
    QDir().mkpath(cacheDir);
    QFileInfo fi(original);

    QString suffix = fi.suffix().toLower();

    static QStringList videoExt = {"mp4","mkv","avi","webm","mov"};
    if (videoExt.contains(suffix)) {
        return generateVideoThumbnail(original, w, h);
    }

    QByteArray hash = QCryptographicHash::hash(original.toUtf8() + QByteArray::number(fi.lastModified().toSecsSinceEpoch()), QCryptographicHash::Sha1);
    QString cacheFile = cacheDir + QDir::separator() + fi.completeBaseName() + "_" + QString(hash.toHex()) + ".webp";
    QFileInfo cfi(cacheFile);
    if (cfi.exists()) {
        return cacheFile;
    }
    // try to load and scale and save as webp
    QImage img(original);
    if (img.isNull()) {
        return original;
    }
    QImage s = img.scaled(w,h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    s.save(cacheFile, "WEBP");
    return cacheFile;
}

InfiniteListModel::InfiniteListModel(QObject *parent)
: QAbstractListModel(parent) {}

void InfiniteListModel::setImages(const QStringList &images) {
    beginResetModel();
    m_images = images;
    endResetModel();
}

QUrl InfiniteListModel::urlAt(qint64 row) const {
    if (m_images.isEmpty()) {
        return QUrl();
    }
    // row peut être grand (modèle "virtuel"), on normalize
    qint64 real = row % m_images.size();
    return QUrl::fromLocalFile(m_images.at(static_cast<int>(real)));
}

int InfiniteListModel::indexOfFile(const QString &path) const
{
    if (int i = m_images.indexOf(path); i >= 0) {
        return i;
    }
    // Fallback: match by file name (e.g. path spelling differences)
    const QString name = QFileInfo(path).fileName();
    for (int i = 0; i < m_images.size(); ++i) {
        if (QFileInfo(m_images.at(i)).fileName() == name) {
            return i;
        }
    }
    return -1;
}

int InfiniteListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid() || m_images.isEmpty()){
        return 0;
    }
    return std::numeric_limits<int>::max(); // effet rouleau
}

QVariant InfiniteListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || m_images.isEmpty()){
        return QVariant();
    }

    int realIndex = index.row() % m_images.size();

    if (role == UrlRole) {
        return QUrl::fromLocalFile(m_images[realIndex]);
    }
    if (role == CachedUrlRole) {
        QString orig = m_images[realIndex];
        QString cache = cachedPathFor(orig);
        return QUrl::fromLocalFile(cache);
    }

    return QVariant();
}

QHash<int, QByteArray> InfiniteListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[CachedUrlRole] = "cachedUrl";
    return roles;
}
