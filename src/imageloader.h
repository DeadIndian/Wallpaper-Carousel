#pragma once
#include <QObject>
#include <QStringList>

class ImageLoader : public QObject
{
    Q_OBJECT
public:
    explicit ImageLoader(const QString &directory, QObject *parent = nullptr);
    Q_INVOKABLE QStringList imageList() const;
signals:
    void imagesChanged(const QStringList &list);
private:
    QStringList m_images;
    void scanDirectory(const QString &directory);
};
