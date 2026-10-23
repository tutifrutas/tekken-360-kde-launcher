#pragma once
#include <QObject>
#include <QSettings>
#include <QVariantMap>

// Owns per-game startup settings, never claims these are live SDL state.
class LaunchSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY changed)
    Q_PROPERTY(QString gameId READ gameId NOTIFY changed)
    Q_PROPERTY(QString gameTitle READ gameTitle NOTIFY changed)
    Q_PROPERTY(QString executableName READ executableName NOTIFY changed)
    Q_PROPERTY(bool supportsSceneResolution READ supportsSceneResolution NOTIFY changed)
public:
    explicit LaunchSettings(const QString& path = {}, QObject* parent = nullptr);
    QVariantMap values() const { return values_; }
    QString gameId() const { return gameId_; }
    QString gameTitle() const;
    QString executableName() const { return gameId_; }
    bool supportsSceneResolution() const { return gameId_ == "tekken6"; }
    Q_INVOKABLE bool selectGame(const QString& id);
    Q_INVOKABLE bool setValue(const QString& key, const QVariant& value);
    void setGameRunning(bool running) { gameRunning_ = running; }
    QString root() const { return values_.value("root").toString(); }
    QString rootForGame(const QString& id) const;
    QStringList arguments() const;
signals:
    void changed();
    void error(const QString& message);
private:
    void loadValues();
    QVariant normalize(const QString& key, const QVariant& value) const;
    QSettings settings_;
    QString gameId_ = "tekken6";
    QVariantMap values_;
    bool gameRunning_ = false;
};
