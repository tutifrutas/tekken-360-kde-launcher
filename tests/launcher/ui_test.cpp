#include "settings.h"
#include "display_catalog.h"
#include "game_session.h"
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

static QVariantList testDisplays() {
    return {QVariantMap{{"monitor", 1}, {"label", "Test panel · DP-1"}, {"mode", "1920 × 1080 · 60 Hz"}, {"primary", true}},
        QVariantMap{{"monitor", 2}, {"label", "Test OLED · DP-3"}, {"mode", "2560 × 1440 · 144 Hz"}, {"primary", false}}};
}
class LauncherUiTest final : public QObject {
    Q_OBJECT
    void capture(QQuickWindow* window, const QString& name) {
        const QString directory = qEnvironmentVariable("T6_UI_CAPTURE_DIR");
        if (directory.isEmpty()) return;
        QVERIFY(QDir().mkpath(directory));
        QTest::qWait(80);
        const QImage image = window->grabWindow();
        QVERIFY(!image.isNull());
        QVERIFY(image.save(directory + '/' + name + ".png"));
    }
private slots:
    void initTestCase() {
        if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) QQuickStyle::setStyle("org.kde.desktop");
    }
    void displayMapping() {
        DisplayCatalog displays(testDisplays());
        QCOMPARE(displays.rowForMonitor(1), 0); QCOMPARE(displays.rowForMonitor(2), 1);
        QCOMPARE(displays.rowForMonitor(0), 0); QCOMPARE(displays.rowForMonitor(16), 0);
        DisplayCatalog empty(QVariantList{}); QCOMPARE(empty.rowForMonitor(2), -1);
    }
    void playAndConfiguration() {
        QTemporaryDir scratch; QVERIFY(scratch.isValid());
        LaunchSettings settings(scratch.path() + "/settings.ini"); GameSession game(settings);
        DisplayCatalog displays(testDisplays());
        QQmlApplicationEngine engine;
        QSignalSpy warnings(&engine, &QQmlApplicationEngine::warnings);
        engine.setInitialProperties({{"settings", QVariant::fromValue(&settings)},
            {"game", QVariant::fromValue(&game)}, {"displays", QVariant::fromValue(&displays)}});
        engine.load(QUrl::fromLocalFile(QFINDTESTDATA("../../launcher/Main.qml")));
        QCOMPARE(engine.rootObjects().size(), 1);
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().front()); QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QVERIFY(!window->findChild<QObject*>("mainTabs"));
        auto* panel = window->findChild<QQuickItem*>("launchSettingsPanel"); QVERIFY(panel);
        auto* grid = window->findChild<QQuickItem*>("configurationGrid"); QVERIFY(grid);
        QTRY_COMPARE(grid->property("columns").toInt(), 2);
        auto* monitorItem = window->findChild<QQuickItem*>("monitorSelector"); QVERIFY(monitorItem);
        auto* scaleItem = window->findChild<QQuickItem*>("renderScaleSelector"); QVERIFY(scaleItem);
        QTRY_VERIFY(monitorItem->width() > 100);
        QTRY_VERIFY(qAbs(monitorItem->mapToScene(QPointF()).y() - scaleItem->mapToScene(QPointF()).y()) <= 1);
        QTRY_VERIFY(qAbs(monitorItem->width() - scaleItem->width()) <= 1);
        QVERIFY(window->findChild<QQuickItem*>("fullscreenSwitch"));
        QVERIFY(!window->findChild<QObject*>("advancedRenderingButton"));
        QVERIFY(!window->findChild<QObject*>("internalResolutionSummary"));
        QVERIFY(!window->findChild<QObject*>("baseResolution"));
        QVERIFY(!window->findChild<QObject*>("windowResolution"));
        capture(window, "play-desktop");
        QVERIFY(!window->findChild<QObject*>("outputResolutionPreset"));
        QVERIFY(!window->findChild<QObject*>("windowResolutionPreset"));
        QVERIFY(settings.arguments().filter("--window_width=").isEmpty());
        QVERIFY(!window->findChild<QObject*>("launchResolutionSummary"));
        QObject* mode = window->findChild<QObject*>("desktopModeInfo"); QVERIFY(mode);
        QVERIFY(mode->property("text").toString().contains("2560 × 1440"));
        QObject* scale = window->findChild<QObject*>("renderScaleSelector"); QVERIFY(scale);
        scale->setProperty("currentIndex", 2);
        QVERIFY(QMetaObject::invokeMethod(scale, "activated", Q_ARG(int, 2)));
        QCOMPARE(settings.values()["scale"].toInt(), 3);
        QVERIFY(settings.arguments().contains("--draw_resolution_scale_x=3"));
        QObject* monitor = window->findChild<QObject*>("monitorSelector"); QVERIFY(monitor);
        QCOMPARE(monitor->property("count").toInt(), 2);
        monitor->setProperty("currentIndex", 0);
        QVERIFY(QMetaObject::invokeMethod(monitor, "activated", Q_ARG(int, 0)));
        QCOMPARE(settings.values()["monitor"].toInt(), 1);
        auto* fullscreen = window->findChild<QQuickItem*>("fullscreenSwitch"); QVERIFY(fullscreen);
        const QPoint center = fullscreen->mapToScene(QPointF(fullscreen->width() / 2, fullscreen->height() / 2)).toPoint();
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
        QTRY_VERIFY(!settings.values()["fullscreen"].toBool());
        LaunchSettings saved(scratch.path() + "/settings.ini");
        QCOMPARE(saved.values(), settings.values());
        window->resize(640, 600);
        QTRY_COMPARE(grid->property("columns").toInt(), 1);
        QVERIFY(grid->width() <= window->width());
        capture(window, "play-narrow");
        // Exercise the Play handler without launching an installed game or writing a profile.
        QVERIFY(settings.setValue("root", scratch.path() + "/empty-game"));
        QObject* button = window->findChild<QObject*>("playButton"); QVERIFY(button);
        QCOMPARE(button->property("text").toString(), QString("Play"));
        QObject* play = window->findChild<QObject*>("playPage"); QVERIFY(play);
        play->setProperty("folderDraft", "relative-folder");
        QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
        QVERIFY(!game.running()); QCOMPARE(game.status(), QString("Stopped"));
        QVERIFY(!play->property("folderError").toString().isEmpty());
        QVERIFY(settings.setValue("root", scratch.path()));
        QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
        QCOMPARE(game.status(), QString("Failed")); QVERIFY(game.error().contains("No native tekken6"));
        QCOMPARE(warnings.count(), 0);
    }
    void gameSelection() {
        QTemporaryDir scratch; QVERIFY(scratch.isValid());
        LaunchSettings settings(scratch.path() + "/settings.ini"); GameSession game(settings);
        DisplayCatalog displays(testDisplays());
        QQmlApplicationEngine engine;
        QSignalSpy warnings(&engine, &QQmlApplicationEngine::warnings);
        engine.setInitialProperties({{"settings", QVariant::fromValue(&settings)},
            {"game", QVariant::fromValue(&game)}, {"displays", QVariant::fromValue(&displays)}});
        engine.load(QUrl::fromLocalFile(QFINDTESTDATA("../../launcher/Main.qml")));
        QCOMPARE(engine.rootObjects().size(), 1);
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().front()); QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        auto* first = window->findChild<QQuickItem*>("tekken6Cover"); QVERIFY(first);
        auto* second = window->findChild<QQuickItem*>("ttt2Cover"); QVERIFY(second);
        // No artwork or network is required for game selection.
        QCOMPARE(first->property("coverStatus").toInt(), 0); // Image.Null
        QCOMPARE(second->property("coverStatus").toInt(), 0);
        QVERIFY(first->findChild<QQuickItem*>("coverFallback")->isVisible());
        QVERIFY(second->findChild<QQuickItem*>("coverFallback")->isVisible());
        QVERIFY(first->property("selected").toBool());
        QVERIFY(settings.root().isEmpty());
        QVERIFY(settings.setValue("root", scratch.path() + "/t6"));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
            second->mapToScene(QPointF(second->width() / 2, second->height() / 2)).toPoint());
        QVERIFY(second->property("selected").toBool());
        QVERIFY(!first->property("selected").toBool());
        QVERIFY(!game.running());
        QCOMPARE(settings.gameId(), QString("ttt2")); QVERIFY(settings.root().isEmpty());
        auto* folder = window->findChild<QObject*>("gameFolder"); QVERIFY(folder);
        QTRY_COMPARE(folder->property("text").toString(), QString());
        QVERIFY(!window->findChild<QObject*>("baseResolution"));
        QVERIFY(!window->findChild<QObject*>("advancedRenderingButton"));
        QVERIFY(!window->findChild<QObject*>("internalResolutionSummary"));
        QObject* scale = window->findChild<QObject*>("renderScaleSelector"); QVERIFY(scale);
        scale->setProperty("currentIndex", 1);
        QVERIFY(QMetaObject::invokeMethod(scale, "activated", Q_ARG(int, 1)));
        QCOMPARE(settings.values()["scale"].toInt(), 2);
        QVERIFY(settings.arguments().contains("--draw_resolution_scale_x=2"));
        QVERIFY(settings.setValue("root", scratch.path() + "/tag"));
        QTRY_COMPARE(folder->property("text").toString(), settings.root());
        second->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_Left);
        QVERIFY(first->hasActiveFocus());
        QVERIFY(first->property("selected").toBool());
        QCOMPARE(settings.root(), scratch.path() + "/t6");
        QTRY_COMPARE(folder->property("text").toString(), settings.root());
        QTest::keyClick(window, Qt::Key_Right);
        QCOMPARE(settings.gameId(), QString("ttt2"));
        QTest::keyClick(window, Qt::Key_Return);
        QVERIFY(!game.running()); // Enter selects a cover, never launches a game.
        window->resize(640, 600);
        QTRY_VERIFY(first->width() > 100 && second->width() > 100);
        QTRY_VERIFY(second->mapToScene(QPointF(second->width(), 0)).x() <= window->width());
        capture(window, "cover-selection");
        QCOMPARE(warnings.count(), 0);
    }
};
QTEST_MAIN(LauncherUiTest)
#include "ui_test.moc"
