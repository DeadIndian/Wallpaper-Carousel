#include "imageloader.h"
#include <QDir>
#include <QFileInfoList>
#include <QDebug>
#include <QStandardPaths>

static const QStringList supported = {"png","jpg","jpeg","bmp","webp", "gif", "mp4", "webm"};

ImageLoader::ImageLoader(const QString &directory, QObject *parent)
    : QObject(parent)
{
    scanDirectory(directory);
}

QStringList ImageLoader::imageList() const { return m_images; }

void ImageLoader::scanDirectory(const QString &directory)
{
    QDir dir(directory);
    if (!dir.exists()) {
        qWarning() << "Wallpapers directory does not exist:" << directory;
        emit imagesChanged(QStringList());
        return;
    }
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable, QDir::Name);
    for (const QFileInfo &fi : list) {
        QString ext = fi.suffix().toLower();
        if (supported.contains(ext)) {
            m_images.append(fi.absoluteFilePath());
        } else {
            qDebug() << "Skipping unsupported file:" << fi.fileName();
        }
    }
    emit imagesChanged(m_images);
}
