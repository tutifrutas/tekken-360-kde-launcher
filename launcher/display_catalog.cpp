#include "display_catalog.h"
#include <SDL3/SDL.h>
#include <QGuiApplication>
#include <QScreen>

DisplayCatalog::DisplayCatalog(QObject* parent) : QObject(parent) {
    ownsVideo_ = SDL_InitSubSystem(SDL_INIT_VIDEO);
    if (!ownsVideo_) { error_ = QString::fromUtf8(SDL_GetError()); return; }
    refresh();
    connect(&timer_, &QTimer::timeout, this, &DisplayCatalog::refresh);
    timer_.start(2000);
}
DisplayCatalog::DisplayCatalog(QVariantList fixtureRows, QObject* parent)
    : QObject(parent), rows_(std::move(fixtureRows)) {}
DisplayCatalog::~DisplayCatalog() {
    timer_.stop();
    if (ownsVideo_) SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
int DisplayCatalog::rowForMonitor(int monitor) const {
    int primary = rows_.isEmpty() ? -1 : 0;
    for (qsizetype i = 0; i < rows_.size(); ++i) {
        const auto row = rows_[i].toMap();
        if (row.value("monitor").toInt() == monitor) return int(i);
        if (row.value("primary").toBool()) primary = int(i);
    }
    return primary;
}
void DisplayCatalog::refresh() {
    if (!ownsVideo_) return;
    SDL_Event event;
    for (int i = 0; i < 256 && SDL_PollEvent(&event); ++i) {}
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    if (!displays) {
        rows_.clear(); error_ = QString::fromUtf8(SDL_GetError()); emit changed(); return;
    }
    QVariantList next;
    const auto primary = SDL_GetPrimaryDisplay();
    for (int i = 0; i < count; ++i) {
        const QString name = QString::fromUtf8(SDL_GetDisplayName(displays[i]));
        QString label = name;
        QScreen* match = nullptr;
        int matches = 0;
        for (QScreen* screen : QGuiApplication::screens()) {
            if (!screen->model().isEmpty() && name.contains(screen->model(), Qt::CaseInsensitive)) {
                match = screen; ++matches;
            }
        }
        if (matches == 1 && !match->name().isEmpty()) label += " · " + match->name();
        const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displays[i]);
        const QString modeText = mode ? QString("%1 × %2 · %3 Hz").arg(mode->w).arg(mode->h)
            .arg(QString::number(mode->refresh_rate, 'g', 5)) : QString("Display mode unavailable");
        next.append(QVariantMap{{"monitor", i + 1}, {"label", label}, {"mode", modeText},
            {"primary", displays[i] == primary}});
    }
    SDL_free(displays);
    const bool notify = next != rows_ || !error_.isEmpty();
    rows_ = std::move(next); error_.clear();
    if (notify) emit changed();
}
