#pragma once

#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

// Artwork is downloaded separately from the application, never bundled.
class CoverCache final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl tekken6 READ tekken6 NOTIFY changed)
    Q_PROPERTY(QUrl ttt2 READ ttt2 NOTIFY changed)
public:
    static QMap<QString, QUrl> defaultSources();
    explicit CoverCache(QString directory = {},
                        QMap<QString, QUrl> sources = defaultSources(), QObject* parent = nullptr);
    QUrl tekken6() const { return covers_.value("tekken6"); }
    QUrl ttt2() const { return covers_.value("ttt2"); }
    void load();
signals:
    void changed();
    void settled(const QString& gameId);
private:
    QString directory_;
    QMap<QString, QUrl> sources_;
    QMap<QString, QUrl> covers_;
    QNetworkAccessManager network_;
    bool loaded_ = false;
};
