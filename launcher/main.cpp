#include "settings.h"
#include "cover_cache.h"
#include "display_catalog.h"
#include "game_session.h"
#include <memory>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTemporaryDir>
#include <QTimer>

int main(int argc, char** argv) {
    QGuiApplication application(argc, argv);
    application.setOrganizationName("TekkenNative");
    application.setApplicationName("TekkenLauncher");
    application.setApplicationDisplayName("Tekken — Native Linux");
    application.setApplicationVersion("0.1.0");
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) QQuickStyle::setStyle("org.kde.desktop");
    const bool smoke = application.arguments().contains("--smoke-test");
    QTemporaryDir scratch;
    if (smoke && !scratch.isValid()) return 1;
    LaunchSettings settings(smoke ? scratch.path() + "/settings.ini" : QString());
    GameSession game(settings);
    auto displays = smoke
        ? std::make_unique<DisplayCatalog>(QVariantList{QVariantMap{{"monitor", 1}, {"label", "Test display"}, {"mode", "1920 × 1080 · 60 Hz"}, {"primary", true}}})
        : std::make_unique<DisplayCatalog>();
    CoverCache covers;
    if (!smoke) covers.load();
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{"settings", QVariant::fromValue(&settings)},
        {"game", QVariant::fromValue(&game)}, {"displays", QVariant::fromValue(displays.get())},
        {"covers", QVariant::fromValue(&covers)}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &application, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    bool qmlFailed = false;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &application,
        [&](const QList<QQmlError>& errors) {
            qmlFailed = true;
            for (const auto& error : errors) std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
        });
    engine.loadFromModule("Tekken.Launcher", "Main");
    if (smoke) {
        if (engine.rootObjects().size() != 1) return 1;
        QTimer::singleShot(350, &application, [&] { application.exit(qmlFailed ? 1 : 0); });
    }
    return application.exec();
}
