#pragma once
#include <QAbstractListModel>
#include <QStringList>
#include <QUrl>

class InfiniteListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        UrlRole = Qt::UserRole + 1,
        CachedUrlRole
    };

    explicit InfiniteListModel(QObject *parent = nullptr);

    void setImages(const QStringList &images);
    Q_INVOKABLE QUrl urlAt(qint64 row) const;
    Q_INVOKABLE int imageCount() const { return m_images.size(); }
    Q_INVOKABLE int indexOfFile(const QString &path) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QStringList m_images;
};


