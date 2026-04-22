// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "customtitlebarkit/CustomTitleBarWindow.h"
#include "customtitlebarkit/TitleBarWidget.h"
#include "customtitlebarkit/MenuIntegration.h"
#include "platform/PlatformTitleBarBackend.h"

#include <QEvent>
#include <QMenuBar>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVariantAnimation>
#include <QWindow>

struct CustomTitleBarWindow::Impl {
    CustomTitleBarWindow::Mode mode = Mode::Custom;
    TitleBarWidget *titleBar = nullptr;
    QWidget *menuSpacer = nullptr;   // plain spacer used as setMenuWidget()
    std::unique_ptr<PlatformTitleBarBackend> backend;
    QMenuBar *appMenuBar = nullptr;
    MenuIntegration *menuIntegration = nullptr;
    int titleBarHeight = 0;
    bool titleBarVisible = true;
    bool attached = false;
    bool safeAreaConnected = false;
    int cachedSpacerWidth = 0;
    QVariantAnimation *spacerAnim = nullptr;

    void updateNativeControlsSpacer() {
        int w = backend->nativeControlsAreaSize().width();
        // Only grow, never shrink — button positions can fluctuate during layout
        if (w > cachedSpacerWidth)
            cachedSpacerWidth = w;
        titleBar->setNativeControlsSpacerSize({cachedSpacerWidth, 0});
    }

    void layoutOverlay(CustomTitleBarWindow *window) {
        if (!titleBar) return;
        int overlayH = backend->snapTitleBarHeight(
            qMax(titleBar->minimumHeight(), titleBar->sizeHint().height()));
        int spacerH = titleBarVisible ? overlayH : 0;
        // Spacer reserves space in QMainWindow's internal layout
        menuSpacer->setFixedHeight(spacerH);
        // Overlay renders on top at (0,0)
        titleBar->setGeometry(0, 0, window->width(), overlayH);
        titleBar->raise();
    }

    void animateSpacerWidth(int targetWidth) {
        if (spacerAnim->state() == QAbstractAnimation::Running)
            spacerAnim->stop();
        spacerAnim->setStartValue(titleBar->nativeControlsSpacer()->width());
        spacerAnim->setEndValue(targetWidth);
        spacerAnim->start();
    }
};

CustomTitleBarWindow::CustomTitleBarWindow(Mode mode, QWidget *parent)
    : QMainWindow(parent)
    , d(std::make_unique<Impl>())
{
    d->mode = mode;

    // Check env var override
    QByteArray envMode = qgetenv("CUSTOMTITLEBARKIT_MODE");
    if (envMode.toLower() == "native")
        d->mode = Mode::Native;

    if (d->mode == Mode::Native)
        return;  // plain QMainWindow — nothing to set up

    d->backend = PlatformTitleBarBackend::create();

    d->titleBar = new TitleBarWidget(this);
    d->titleBar->setMinimumHeight(d->backend->snapTitleBarHeight(0));

    // Show window controls if backend is frameless (no native titlebar)
    d->titleBar->setWindowControlsVisible(d->backend->needsWindowControls());

    // Replace the native controls spacer with a custom widget if the backend provides one
    if (auto *ctrlWidget = d->backend->createNativeControlsWidget(d->titleBar)) {
        if (d->backend->nativeControlsPosition() == PlatformTitleBarBackend::RightSide)
            d->titleBar->setNativeControlsWidgetRight(ctrlWidget);
        else
            d->titleBar->setNativeControlsWidget(ctrlWidget);
    }

    // Plain spacer as menu widget — reserves space in QMainWindow's layout
    // without containing QPushButtons that would affect native titlebar sizing.
    d->menuSpacer = new QWidget(this);
    d->menuSpacer->setFixedHeight(d->titleBar->minimumHeight());
    setMenuWidget(d->menuSpacer);

    // Animate native controls spacer width on fullscreen transitions
    d->spacerAnim = new QVariantAnimation(this);
    d->spacerAnim->setDuration(200);
    d->spacerAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->spacerAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        d->titleBar->setNativeControlsSpacerSize({value.toInt(), 0});
    });

    // Watch for titlebar resize to auto-notify the backend (e.g. Mac NSToolbar)
    d->titleBar->installEventFilter(this);

    // Force native window creation and apply styling eagerly
    winId();
    d->backend->attach(this);
    d->attached = true;

    // Position overlay and set content margins AFTER attach() so that
    // setWindowFlags/WA_LayoutOnEntireRect are already in effect.
    d->layoutOverlay(this);

    // Create default menu integration strategy from the platform backend
    d->menuIntegration = d->backend->createDefaultMenuIntegration(this);

    connect(d->backend.get(), &PlatformTitleBarBackend::fullscreenChanged, this, [this](bool fullscreen) {
        d->animateSpacerWidth(fullscreen ? 0 : d->cachedSpacerWidth);
        d->layoutOverlay(this);
    });
}

CustomTitleBarWindow::~CustomTitleBarWindow() = default;

CustomTitleBarWindow::Mode CustomTitleBarWindow::mode() const
{
    return d->mode;
}

QString CustomTitleBarWindow::backendName() const
{
    return d->backend ? d->backend->backendName() : QString();
}

int CustomTitleBarWindow::titleBarHeight() const
{
    return d->titleBarHeight;
}

void CustomTitleBarWindow::setTitleBarHeight(int height)
{
    if (d->mode == Mode::Native) return;
    if (d->titleBarHeight == height)
        return;

    d->titleBarHeight = height;
    d->titleBar->setMinimumHeight(d->backend->snapTitleBarHeight(height));
    d->layoutOverlay(this);
    Q_EMIT titleBarHeightChanged(height);
}

QWidget *CustomTitleBarWindow::leftArea() const
{
    if (d->mode == Mode::Native) return nullptr;
    return d->titleBar->leftArea();
}

QWidget *CustomTitleBarWindow::rightArea() const
{
    if (d->mode == Mode::Native) return nullptr;
    return d->titleBar->rightArea();
}

QWidget *CustomTitleBarWindow::nativeControlsWidget() const
{
    if (d->mode == Mode::Native) return nullptr;
    return d->titleBar->nativeControlsSpacer();
}

QMenuBar *CustomTitleBarWindow::menuBar()
{
    if (d->mode == Mode::Native)
        return QMainWindow::menuBar();
    if (!d->appMenuBar) {
        d->appMenuBar = new QMenuBar(this);
        d->menuIntegration->install(d->appMenuBar, this);
    }
    return d->appMenuBar;
}

void CustomTitleBarWindow::setMenuBar(QMenuBar *mb)
{
    if (d->mode == Mode::Native) {
        QMainWindow::setMenuBar(mb);
        return;
    }
    if (d->appMenuBar) {
        d->menuIntegration->uninstall(this);
        if (d->appMenuBar->parent() == this)
            delete d->appMenuBar;
    }
    d->appMenuBar = mb;
    if (mb)
        d->menuIntegration->install(mb, this);
}

void CustomTitleBarWindow::setMenuIntegration(MenuIntegration *integration)
{
    if (d->mode == Mode::Native) {
        delete integration;
        return;
    }
    if (d->appMenuBar && d->menuIntegration)
        d->menuIntegration->uninstall(this);
    delete d->menuIntegration;
    // nullptr resets to the platform default
    d->menuIntegration = integration ? integration
                                     : d->backend->createDefaultMenuIntegration(this);
    if (d->appMenuBar)
        d->menuIntegration->install(d->appMenuBar, this);
}

bool CustomTitleBarWindow::isTitleBarVisible() const
{
    return d->titleBarVisible;
}

void CustomTitleBarWindow::setTitleBarVisible(bool visible)
{
    if (d->mode == Mode::Native) return;
    if (d->titleBarVisible == visible)
        return;

    d->titleBarVisible = visible;
    if (visible) {
        d->titleBar->setMinimumHeight(d->backend->snapTitleBarHeight(d->titleBarHeight));
        d->titleBar->setVisible(true);
    } else {
        d->titleBar->setMinimumHeight(0);
        d->titleBar->setVisible(false);
    }
    d->layoutOverlay(this);
    Q_EMIT titleBarVisibleChanged(visible);
}

void CustomTitleBarWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (d->mode == Mode::Native) return;

    d->layoutOverlay(this);

    d->updateNativeControlsSpacer();

    // Connect to safeAreaMarginsChanged for dynamic spacer updates (once)
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    if (!d->safeAreaConnected) {
        if (auto *wh = windowHandle()) {
            connect(wh, &QWindow::safeAreaMarginsChanged, this, [this]() {
                if (!d->backend->isFullscreen())
                    d->updateNativeControlsSpacer();
            });
            d->safeAreaConnected = true;
        }
    }
#endif
}

void CustomTitleBarWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (d->mode == Mode::Native) return;
    d->layoutOverlay(this);
}

bool CustomTitleBarWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (d->mode != Mode::Native && d->backend) {
        if (d->backend->handleNativeEvent(eventType, message, result))
            return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

bool CustomTitleBarWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (d->mode == Mode::Native)
        return QMainWindow::eventFilter(obj, event);
    if (obj == d->titleBar && event->type() == QEvent::Resize && !d->backend->isFullscreen()) {
        d->backend->setTitleBarHeight(d->titleBar->height());
        d->updateNativeControlsSpacer();
    }
    return QMainWindow::eventFilter(obj, event);
}
