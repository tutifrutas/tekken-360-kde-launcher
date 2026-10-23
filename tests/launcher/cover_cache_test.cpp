#include "cover_cache.h"
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTest>

class ImageServer final : public QTcpServer {
public:
    QByteArray body;
    int status = 200;
    int requests = 0;
    ImageServer() {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto* socket = nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                    socket->readAll();
                    if (socket->property("answered").toBool()) return;
                    socket->setProperty("answered", true);
                    ++requests;
                    socket->write("HTTP/1.1 " + QByteArray::number(status) + " Test\r\n"
                        "Content-Type: image/png\r\nContent-Length: " +
                        QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    }
    QUrl url() const { return QUrl("http://127.0.0.1:" + QString::number(serverPort()) + "/cover"); }
};

class CoverCacheTest final : public QObject {
    Q_OBJECT
    static QByteArray image() {
        QImage image(16, 24, QImage::Format_RGB32);
        image.fill(Qt::blue);
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        return bytes;
    }
private slots:
    void downloadsAndReusesCache() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ImageServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        server.body = image();
        const QMap<QString, QUrl> sources{{"tekken6", server.url()}};
        CoverCache first(directory.path(), sources);
        QVERIFY(first.tekken6().isEmpty());
        first.load();
        QTRY_VERIFY_WITH_TIMEOUT(first.tekken6().isLocalFile(), 5000);
        QCOMPARE(server.requests, 1);
        QFile cached(first.tekken6().toLocalFile());
        QVERIFY(cached.open(QIODevice::ReadOnly));
        QCOMPARE(cached.readAll(), server.body);
        first.load();
        server.close();
        CoverCache second(directory.path(), sources);
        second.load();
        QCOMPARE(second.tekken6(), first.tekken6()); // Immediate cache hit, even offline.
        QVERIFY(second.ttt2().isEmpty());
        QCOMPARE(server.requests, 1);
    }
    void replacesCorruptCache() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile corrupt(directory.path() + "/ttt2.jpg");
        QVERIFY(corrupt.open(QIODevice::WriteOnly));
        corrupt.write("not an image");
        corrupt.close();
        ImageServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        server.body = image();
        CoverCache cache(directory.path(), {{"ttt2", server.url()}});
        cache.load();
        QTRY_VERIFY_WITH_TIMEOUT(cache.ttt2().isLocalFile(), 5000);
        QCOMPARE(server.requests, 1);
    }
    void rejectsBadResponses_data() {
        QTest::addColumn<int>("status");
        QTest::addColumn<QByteArray>("body");
        QTest::newRow("http-error") << 404 << image();
        QTest::newRow("not-image") << 200 << QByteArray("<html>unavailable</html>");
        QTest::newRow("too-large") << 200 << QByteArray(8 * 1024 * 1024 + 1, 'x');
    }
    void rejectsBadResponses() {
        QFETCH(int, status);
        QFETCH(QByteArray, body);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ImageServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        server.body = body;
        server.status = status;
        CoverCache cache(directory.path(), {{"tekken6", server.url()}});
        QSignalSpy settled(&cache, &CoverCache::settled);
        cache.load();
        QTRY_COMPARE_WITH_TIMEOUT(settled.count(), 1, 5000);
        QCOMPARE(server.requests, 1);
        QVERIFY(cache.tekken6().isEmpty());
        QVERIFY(!QFile::exists(directory.path() + "/tekken6.jpg"));
    }
};
QTEST_GUILESS_MAIN(CoverCacheTest)
#include "cover_cache_test.moc"
