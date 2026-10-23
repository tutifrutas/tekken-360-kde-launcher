#include "settings.h"
#include "game_session.h"
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QSignalSpy>

class LauncherTest final : public QObject {
    Q_OBJECT
private slots:
    void settingsPersistence() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const QString path = directory.path() + "/settings.ini";
        {
            LaunchSettings settings(path);
            QCOMPARE(settings.values()["scale"].toInt(), 2);
            QVERIFY(!settings.setValue("root", "relative/path"));
            QVERIFY(!settings.setValue("scale", 0)); QVERIFY(!settings.setValue("monitor", -1));
            QVERIFY(!settings.setValue("unknown", 42));
            QVERIFY(settings.setValue("root", QUrl::fromLocalFile(directory.path() + "/game with + spaces")));
            QVERIFY(settings.setValue("scale", 3)); QVERIFY(settings.setValue("fullscreen", false));
        }
        // Force the disk representation, as read by a cold process.
        { QFile ini(path); QVERIFY(ini.open(QIODevice::Append)); ini.write("\n"); }
        LaunchSettings restored(path);
        QCOMPARE(restored.values()["scale"].toInt(), 3); QCOMPARE(restored.values()["fullscreen"].toBool(), false);
        QCOMPARE(restored.root(), directory.path() + "/game with + spaces");
        QVERIFY(restored.arguments().contains("--window_width=2560"));
        QVERIFY(restored.arguments().contains("--window_height=1440"));
        QVERIFY(restored.setValue("fullscreen", true));
        QVERIFY(restored.arguments().filter("--window_width=").isEmpty());
        QVERIFY(restored.arguments().filter("--window_height=").isEmpty());
        QVERIFY(restored.arguments().contains("--fullscreen_exclusive=false"));
        QVERIFY(restored.arguments().contains("--draw_resolution_scale_x=3"));
        QVERIFY(restored.arguments().contains("--user_data_root=" + restored.root() + "/user"));
    }
    void failedSettingsWrite() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QFile blocker(directory.path() + "/not-a-directory");
        QVERIFY(blocker.open(QIODevice::WriteOnly)); blocker.close();
        LaunchSettings settings(blocker.fileName() + "/settings.ini");
        QSignalSpy errors(&settings, &LaunchSettings::error);
        QVERIFY(!settings.selectGame("ttt2")); QCOMPARE(settings.gameId(), QString("tekken6"));
        QVERIFY(!settings.setValue("root", directory.path())); QVERIFY(settings.root().isEmpty());
        QCOMPARE(errors.count(), 2);
    }
    void perGameSettings() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const QString path = directory.path() + "/settings.ini";
        LaunchSettings settings(path);
        QCOMPARE(settings.root(), QString());
        GameSession missing(settings);
        QVERIFY(!missing.start()); QVERIFY(missing.error().contains("Choose the location"));
        QVERIFY(settings.setValue("root", directory.path() + "/Tekken 6"));
        QVERIFY(settings.setValue("scale", 3));
        QVERIFY(settings.arguments().contains("--video_driver=wayland"));
        QVERIFY(!settings.arguments().contains("--readback_resolve=full"));
        QVERIFY(!settings.selectGame("unknown"));
        QVERIFY(settings.selectGame("ttt2"));
        QCOMPARE(settings.executableName(), QString("ttt2"));
        QVERIFY(!settings.supportsSceneResolution());
        QCOMPARE(settings.root(), QString()); QCOMPARE(settings.values()["scale"].toInt(), 1);
        QVERIFY(!settings.setValue("sceneWidth", 1024));
        QVERIFY(settings.setValue("root", directory.path() + "/Tag 2"));
        QVERIFY(settings.arguments().contains("--readback_resolve=full"));
        QVERIFY(settings.arguments().contains("--cache_root=" + settings.root() + "/cache/rov-1x"));
        QVERIFY(settings.selectGame("tekken6"));
        QCOMPARE(settings.root(), directory.path() + "/Tekken 6");
        QCOMPARE(settings.values()["scale"].toInt(), 3);
        QCOMPARE(settings.rootForGame("ttt2"), directory.path() + "/Tag 2");
        QVERIFY(settings.selectGame("ttt2"));
        LaunchSettings restored(path);
        QCOMPARE(restored.gameId(), QString("ttt2")); QCOMPARE(restored.root(), settings.root());
        QVERIFY(restored.selectGame("tekken6"));
        QCOMPARE(restored.values()["scale"].toInt(), 3);
    }
    void tagProcessIsolation() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        LaunchSettings settings(directory.path() + "/settings.ini");
        QVERIFY(settings.selectGame("ttt2")); QVERIFY(settings.setValue("root", directory.path()));
        QFile script(directory.path() + "/ttt2"); QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\nprintf '%s\\n' \"$@\" > arguments.txt\nprintf '%s\\n' \"${TEKKEN6_INTERNAL_RESOLUTION-unset}\" > scene.txt\nexec sleep 30\n");
        script.close(); QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        const auto previous = qgetenv("TEKKEN6_INTERNAL_RESOLUTION");
        const bool hadPrevious = qEnvironmentVariableIsSet("TEKKEN6_INTERNAL_RESOLUTION");
        qputenv("TEKKEN6_INTERNAL_RESOLUTION", "640x360");
        GameSession game(settings);
        const bool started = game.start();
        if (hadPrevious) qputenv("TEKKEN6_INTERNAL_RESOLUTION", previous);
        else qunsetenv("TEKKEN6_INTERNAL_RESOLUTION");
        QVERIFY(started);
        QVERIFY(!settings.selectGame("tekken6"));
        QTRY_COMPARE_WITH_TIMEOUT(game.status(), QString("Running"), 3000);
        QFile scene(directory.path() + "/scene.txt");
        QTRY_VERIFY_WITH_TIMEOUT(scene.exists() && scene.size() > 0, 3000);
        QVERIFY(scene.open(QIODevice::ReadOnly)); QCOMPARE(scene.readAll(), QByteArray("unset\n"));
        QFile arguments(directory.path() + "/arguments.txt"); QVERIFY(arguments.open(QIODevice::ReadOnly));
        const auto bytes = arguments.readAll();
        QVERIFY(bytes.contains("--readback_resolve=full\n"));
        QVERIFY(bytes.contains("--video_driver=wayland\n"));
        QVERIFY(bytes.contains("--user_data_root=" + directory.path().toUtf8() + "/user\n"));
        game.stop(); QTRY_VERIFY_WITH_TIMEOUT(!game.running(), 5000);
        QVERIFY(settings.selectGame("tekken6"));
    }
    void processLifecycle() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        LaunchSettings settings(directory.path() + "/settings.ini");
        QVERIFY(settings.setValue("root", directory.path()));
        GameSession game(settings);
        QVERIFY(!game.start()); QVERIFY(!game.error().isEmpty()); QVERIFY(!game.running());
        QFile sentinel(directory.path() + "/real-save-sentinel"); QVERIFY(sentinel.open(QIODevice::WriteOnly));
        sentinel.write("preserve-save"); sentinel.close();
        QFile script(directory.path() + "/tekken6"); QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\nprintf '%s\\n' \"$@\" > arguments.txt\nprintf '%s\\n' \"$TEKKEN6_INTERNAL_RESOLUTION\" > scene.txt\nexec sleep 30\n"); script.close();
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(game.start()); QVERIFY(!game.start());
        QTRY_COMPARE_WITH_TIMEOUT(game.status(), QString("Running"), 3000);
        QFile arguments(directory.path() + "/arguments.txt");
        QTRY_VERIFY_WITH_TIMEOUT(arguments.exists() && arguments.size() > 0, 3000);
        QVERIFY(arguments.open(QIODevice::ReadOnly)); const auto bytes = arguments.readAll();
        QVERIFY(bytes.contains("--monitor=2\n")); QVERIFY(bytes.contains("--render_target_path_vulkan=fsi\n"));
        game.stop(); QTRY_VERIFY_WITH_TIMEOUT(!game.running(), 5000);
        QVERIFY(game.error().isEmpty());
        QVERIFY(sentinel.open(QIODevice::ReadOnly)); QCOMPARE(sentinel.readAll(), QByteArray("preserve-save"));
        QVERIFY(game.start()); QTRY_COMPARE_WITH_TIMEOUT(game.status(), QString("Running"), 3000);
        game.stop(); QTRY_VERIFY_WITH_TIMEOUT(!game.running(), 5000);
    }
};
QTEST_GUILESS_MAIN(LauncherTest)
#include "launcher_test.moc"
