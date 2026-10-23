#include "settings.h"
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

namespace {
QString settingsFile(const QString& overridePath) {
    if (!overridePath.isEmpty()) return overridePath;
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(directory);
    return directory + "/launcher.ini";
}
}
LaunchSettings::LaunchSettings(const QString& path, QObject* parent)
    : QObject(parent), settings_(settingsFile(path), QSettings::IniFormat) {
    const QString saved = settings_.value("selectedGame", "tekken6").toString();
    if (saved == "tekken6" || saved == "ttt2") gameId_ = saved;
    loadValues();
}
QString LaunchSettings::gameTitle() const {
    return gameId_ == "tekken6" ? QStringLiteral("Tekken 6") : QStringLiteral("Tekken Tag Tournament 2");
}
QString LaunchSettings::rootForGame(const QString& id) const {
    if (id != "tekken6" && id != "ttt2") return {};
    return settings_.value(id + "/root").toString();
}
void LaunchSettings::loadValues() {
    // Locations are deliberately empty until the user selects each package.
    values_ = gameId_ == "tekken6"
        ? QVariantMap{{"root", ""}, {"width", 2560}, {"height", 1440}, {"monitor", 2},
            {"scale", 2}, {"fullscreen", true}, {"sceneWidth", 1024}, {"sceneHeight", 576}}
        : QVariantMap{{"root", ""}, {"width", 1280}, {"height", 720}, {"monitor", 0},
            {"scale", 1}, {"fullscreen", false}};
    for (auto it = values_.begin(); it != values_.end(); ++it) {
        const QVariant value = normalize(it.key(), settings_.value(gameId_ + '/' + it.key(), it.value()));
        if (value.isValid()) it.value() = value;
    }
}
QVariant LaunchSettings::normalize(const QString& key, const QVariant& value) const {
    if (key == "root") {
        const QString path = (value.metaType().id() == QMetaType::QUrl ? value.toUrl().toLocalFile() : value.toString()).trimmed();
        if (path.isEmpty() || !QDir::isAbsolutePath(path)) return {};
        return QDir::cleanPath(path);
    }
    if (key == "fullscreen") {
        const QString text = value.toString().toLower();
        if (value.metaType().id() != QMetaType::Bool && text != "true" && text != "false") return {};
        return value.toBool();
    }
    bool ok = false;
    const int number = value.toInt(&ok);
    const int minimum = key == "monitor" ? 0 : key == "scale" ? 1 : 320;
    const int maximum = key == "monitor" ? 16 : key == "scale" ? 4 : 8192;
    if (!ok || number < minimum || number > maximum) return {};
    return number;
}
bool LaunchSettings::selectGame(const QString& id) {
    if (id != "tekken6" && id != "ttt2") return false;
    if (id == gameId_) return true;
    if (gameRunning_) return false;
    settings_.setValue("selectedGame", id);
    settings_.sync();
    if (settings_.status() != QSettings::NoError) {
        settings_.setValue("selectedGame", gameId_);
        emit error("Could not save the selected game");
        return false;
    }
    gameId_ = id;
    loadValues();
    emit changed();
    return true;
}
bool LaunchSettings::setValue(const QString& key, const QVariant& value) {
    if (!values_.contains(key)) return false;
    const QVariant normalized = normalize(key, value);
    if (!normalized.isValid()) return false;
    if (values_.value(key) == normalized) return true;
    const auto previous = values_.value(key);
    settings_.setValue(gameId_ + '/' + key, normalized);
    settings_.sync();
    if (settings_.status() != QSettings::NoError) {
        settings_.setValue(gameId_ + '/' + key, previous);
        emit error("Could not save launcher settings");
        return false;
    }
    values_[key] = normalized;
    emit changed();
    return true;
}
QStringList LaunchSettings::arguments() const {
    const QString package = root();
    const auto value = [this](const char* key) { return values_.value(key).toString(); };
    QStringList arguments{"--fullscreen=" + QString(values_.value("fullscreen").toBool() ? "true" : "false"),
        "--fullscreen_exclusive=false", "--video_driver=wayland",
        "--monitor=" + value("monitor"), "--draw_resolution_scale_x=" + value("scale"),
        "--draw_resolution_scale_y=" + value("scale"), "--gpu_backend=vulkan",
        "--render_target_path_vulkan=fsi", "--async_shader_compilation=true",
        "--anisotropic_override=0", "--log_level=info",
        "--game_data_root=" + package + "/game", "--user_data_root=" + package + "/user",
        // Preserve each game's existing cache namespace.
        "--cache_root=" + package + (gameId_ == "ttt2" ? "/cache/rov-" : "/cache/vulkan-") + value("scale") + "x",
        "--log_file=" + package + "/logs/native-launcher-game.log"};
    if (gameId_ == "ttt2") arguments << "--readback_resolve=full";
    if (!values_.value("fullscreen").toBool())
        arguments << "--window_width=" + value("width") << "--window_height=" + value("height");
    return arguments;
}
