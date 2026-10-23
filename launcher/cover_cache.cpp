#include "cover_cache.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QNetworkReply>
#include <QSaveFile>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTimer>
#include <memory>

namespace {
constexpr qsizetype maximumBytes = 8 * 1024 * 1024;

bool validImage(const QByteArray& bytes) {
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    const QSize size = reader.size();
    if ((reader.format() != "jpeg" && reader.format() != "png") ||
        size.width() <= 0 || size.height() <= 0 ||
        size.width() > 4096 || size.height() > 4096) return false;
    return !reader.read().isNull();
}
}

QMap<QString, QUrl> CoverCache::defaultSources() {
    return {{"tekken6", QUrl("https://images.launchbox-app.com/c572bb46-d132-48b8-a5db-7481675a9826.jpg")},
            {"ttt2", QUrl("https://images.launchbox-app.com/99a77a89-16ca-42e2-9714-53960176b045.jpg")}};
}

CoverCache::CoverCache(QString directory, QMap<QString, QUrl> sources, QObject* parent)
    : QObject(parent),
      directory_(directory.isEmpty()
          ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/covers"
          : std::move(directory)),
      sources_(std::move(sources)) {}

void CoverCache::load() {
    if (loaded_) return;
    loaded_ = true;
    if (!QDir().mkpath(directory_)) return;
    for (auto it = sources_.cbegin(); it != sources_.cend(); ++it) {
        const QString id = it.key();
        // Only these two fixed cache filenames are accepted.
        if (id != "tekken6" && id != "ttt2") continue;
        const QString path = directory_ + '/' + id + ".jpg";
        QFile cached(path);
        if (cached.open(QIODevice::ReadOnly) && cached.size() <= maximumBytes &&
            validImage(cached.read(maximumBytes + 1))) {
            covers_.insert(id, QUrl::fromLocalFile(path));
            emit changed();
            emit settled(id);
            continue;
        }
        cached.close();
        QNetworkRequest request(it.value());
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setMaximumRedirectsAllowed(3);
        auto* reply = network_.get(request);
        reply->setReadBufferSize(64 * 1024);
        // Bound the entire request, not just the time between received packets.
        QTimer::singleShot(15000, reply, [reply] {
            if (!reply->isFinished()) reply->abort();
        });
        auto bytes = std::make_shared<QByteArray>();
        auto consume = [reply, bytes] {
            bytes->append(reply->read(maximumBytes - bytes->size() + 1));
            if (bytes->size() > maximumBytes) reply->abort();
        };
        connect(reply, &QIODevice::readyRead, this, consume);
        connect(reply, &QNetworkReply::finished, this, [this, reply, bytes, id, path, consume] {
            const auto settled = qScopeGuard([this, id] { emit this->settled(id); });
            if (reply->error() == QNetworkReply::NoError) consume();
            const bool ok = reply->error() == QNetworkReply::NoError &&
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
                bytes->size() <= maximumBytes && validImage(*bytes);
            reply->deleteLater();
            if (!ok) return; // Text cards remain usable when artwork is unavailable.
            QSaveFile file(path);
            if (!file.open(QIODevice::WriteOnly) || file.write(*bytes) != bytes->size() ||
                !file.commit()) return;
            covers_.insert(id, QUrl::fromLocalFile(path));
            emit changed();
        });
    }
}
