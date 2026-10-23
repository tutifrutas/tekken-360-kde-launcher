#pragma once
#include "settings.h"
#include <QProcess>
#include <QTimer>

class GameSession final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit GameSession(LaunchSettings& settings, QObject* parent = nullptr);
    ~GameSession() override;
    bool running() const { return process_.state() != QProcess::NotRunning; }
    QString status() const { return status_; }
    QString error() const { return error_; }
    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
signals:
    void changed();
private:
    LaunchSettings& settings_;
    QProcess process_;
    QTimer killTimer_;
    QString status_ = "Stopped", error_;
    bool stopping_ = false;
};
