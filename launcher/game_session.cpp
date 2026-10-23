#include "game_session.h"
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <cerrno>
#include <sys/resource.h>

GameSession::GameSession(LaunchSettings& settings, QObject* parent) : QObject(parent), settings_(settings) {
    // Desktop sessions often start with a 1024-FD soft limit. NTSYNC needs
    // descriptors for guest synchronization objects; use the permitted hard
    // limit in the game child only, without changing the launcher's limit.
    process_.setChildProcessModifier([this] {
        struct rlimit limit{};
        if (getrlimit(RLIMIT_NOFILE, &limit) != 0)
            process_.failChildProcessModifier("getrlimit(RLIMIT_NOFILE)", errno);
        if (limit.rlim_cur < limit.rlim_max) {
            limit.rlim_cur = limit.rlim_max;
            if (setrlimit(RLIMIT_NOFILE, &limit) != 0)
                process_.failChildProcessModifier("setrlimit(RLIMIT_NOFILE)", errno);
        }
    });
    connect(this, &GameSession::changed, &settings_, [this] { settings_.setGameRunning(running()); });
    killTimer_.setSingleShot(true);
    connect(&killTimer_, &QTimer::timeout, &process_, &QProcess::kill);
    connect(&process_, &QProcess::started, this, [this] { status_ = "Running"; emit changed(); });
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (!stopping_) error_ = process_.errorString();
        if (process_.state() == QProcess::NotRunning) { killTimer_.stop(); status_ = "Failed"; }
        emit changed();
    });
    connect(&process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        killTimer_.stop(); status_ = "Stopped";
        if (!stopping_ && error_.isEmpty()) {
            if (status == QProcess::CrashExit) error_ = "The game terminated unexpectedly";
            else if (code) error_ = QString("The game exited with code %1").arg(code);
        }
        stopping_ = false;
        emit changed();
    });
}
GameSession::~GameSession() {
    killTimer_.stop();
    if (running()) {
        process_.terminate();
        if (!process_.waitForFinished(2000)) { process_.kill(); process_.waitForFinished(2000); }
    }
}
bool GameSession::start() {
    if (running()) return false;
    error_.clear(); stopping_ = false;
    const QString root = settings_.root();
    if (root.isEmpty()) {
        error_ = "Choose the location of " + settings_.gameTitle() + " first";
        status_ = "Failed"; emit changed(); return false;
    }
    const QFileInfo executable(root + '/' + settings_.executableName());
    if (!executable.isFile() || !executable.isExecutable()) {
        error_ = "No native " + settings_.executableName() + " executable in the selected game folder";
        status_ = "Failed"; emit changed(); return false;
    }
    if (!QDir().mkpath(root + "/logs")) {
        error_ = "Cannot create the game's log directory";
        status_ = "Failed"; emit changed(); return false;
    }
    auto environment = QProcessEnvironment::systemEnvironment();
    // Discard inherited development session overrides before launching a game.
    for (const QString& key : environment.keys())
        if (key.startsWith("T6_NET") || key.startsWith("T6_SNAP")) environment.remove(key);
    const auto values = settings_.values();
    if (settings_.gameId() == "tekken6") {
        environment.insert("TEKKEN6_INTERNAL_RESOLUTION", QString("%1x%2").arg(values["sceneWidth"].toInt()).arg(values["sceneHeight"].toInt()));
        environment.insert("TEKKEN6_OFFSCREEN_RESOLUTION", "1024x1024");
        environment.insert("TEKKEN6_YIELD_STARTUP_SPIN", "1");
    } else {
        for (const QString& key : environment.keys())
            if (key.startsWith("TEKKEN6_")) environment.remove(key);
    }
    const QString previous = environment.value("LD_LIBRARY_PATH");
    environment.insert("LD_LIBRARY_PATH", root + (previous.isEmpty() ? QString() : ":" + previous));
    process_.setProcessEnvironment(environment);
    process_.setWorkingDirectory(root);
    process_.setStandardOutputFile(root + "/logs/native-launcher-console.log", QIODevice::Append);
    process_.setProcessChannelMode(QProcess::MergedChannels);
    process_.setProgram(executable.absoluteFilePath());
    process_.setArguments(settings_.arguments());
    status_ = "Starting";
    process_.start(); emit changed(); return true;
}
void GameSession::stop() {
    if (!running()) return;
    stopping_ = true;
    status_ = "Stopping"; process_.terminate(); killTimer_.start(3000); emit changed();
}
