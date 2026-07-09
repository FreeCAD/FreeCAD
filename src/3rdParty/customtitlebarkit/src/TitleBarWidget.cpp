// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "customtitlebarkit/TitleBarWidget.h"
#include "WindowDecorationButton.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QTimer>
#include <QWindow>

struct TitleBarWidget::Impl {
    QWidget *spacer = nullptr;        // transparent spacer (default)
    QWidget *controlsWidget = nullptr; // custom replacement (e.g. traffic lights)
    QWidget *leftArea = nullptr;
    QWidget *rightArea = nullptr;
    QWidget *windowControls = nullptr;
    WindowDecorationButton *minimizeBtn = nullptr;
    WindowDecorationButton* maximizeBtn = nullptr;
    WindowDecorationButton* restoreBtn = nullptr;
    WindowDecorationButton *closeBtn = nullptr;

    bool blocksWindowDrag(QWidget *child, const TitleBarWidget *titleBar) const
    {
        for (auto *widget = child; widget && widget != titleBar; widget = widget->parentWidget()) {
            if (widget->property("titleBarDragArea").isValid()) {
                return !widget->property("titleBarDragArea").toBool();
            }
            if (widget->inherits("QAbstractButton") || widget->inherits("QTabBar")
                || widget->inherits("QMenuBar") || widget->inherits("QComboBox")
                || widget->inherits("QSpinBox") || widget->inherits("QLineEdit")
                || widget->inherits("QSlider") || widget->inherits("QScrollBar")
                || widget->inherits("QToolBar")) {
                return true;
            }
        }

        return false;
    }
};

TitleBarWidget::TitleBarWidget(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setAttribute(Qt::WA_StyledBackground, true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    setAttribute(Qt::WA_LayoutOnEntireRect, true);
#endif

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Native controls spacer — transparent to mouse so native buttons work
    d->spacer = new QWidget(this);
    d->spacer->setAttribute(Qt::WA_TransparentForMouseEvents);
    d->spacer->setFixedWidth(0);
    layout->addWidget(d->spacer);

    // Left content area — expands to fill space between spacer and right area.
    // Transparent border stylesheet forces Qt's styled rendering path,
    // which is needed for proper vertical centering of child widgets (e.g. QMenuBar).
    d->leftArea = new QWidget(this);
    d->leftArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    layout->addWidget(d->leftArea, 0);

    // Low-priority expanding spacer between left and right areas
    layout->addStretch(1);

    // Right content area
    d->rightArea = new QWidget(this);
    d->rightArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->addWidget(d->rightArea);

    // Window control buttons (hidden by default, shown for frameless backends)
    d->windowControls = new QWidget(this);
    auto *ctrlLayout = new QHBoxLayout(d->windowControls);
    ctrlLayout->setContentsMargins(0, 0, 0, 0);
    ctrlLayout->setSpacing(0);

    d->minimizeBtn = new WindowDecorationButton(WindowDecorationButton::Minimize, d->windowControls);
    d->maximizeBtn = new WindowDecorationButton(WindowDecorationButton::Maximize, d->windowControls);
    d->restoreBtn = new WindowDecorationButton(WindowDecorationButton::Restore, d->windowControls);
    d->closeBtn = new WindowDecorationButton(WindowDecorationButton::Close, d->windowControls);
    ctrlLayout->addWidget(d->minimizeBtn);
    ctrlLayout->addWidget(d->maximizeBtn);
    ctrlLayout->addWidget(d->restoreBtn);
    ctrlLayout->addWidget(d->closeBtn);

    d->restoreBtn->hide();

    layout->addWidget(d->windowControls);
    d->windowControls->setVisible(false);

    // Connect window control buttons
    connect(d->minimizeBtn, &WindowDecorationButton::clicked, this, [this]() {
        if (auto* w = window()) {
            w->showMinimized();
        }
    });
    connect(d->maximizeBtn, &QPushButton::clicked, this, [this]() {
        if (auto* w = window()) {
            w->showMaximized();
// YOUR ORIGINAL QT6 FIX:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# ifdef Q_OS_WIN
            QTimer::singleShot(10, w, [w]() { w->showMaximized(); });
# endif
#endif
        }
    });
    connect(d->restoreBtn, &QPushButton::clicked, this, [this]() {
        if (auto* w = window()) {
            w->showNormal();
        }
    });
    connect(d->closeBtn, &WindowDecorationButton::clicked, this, [this]() {
        if (auto *w = window()) w->close();
    });

    setFixedHeight(35);
}

TitleBarWidget::~TitleBarWidget() = default;

QWidget *TitleBarWidget::leftArea() const
{
    return d->leftArea;
}

QWidget *TitleBarWidget::rightArea() const
{
    return d->rightArea;
}

QWidget *TitleBarWidget::nativeControlsSpacer() const
{
    return d->spacer;
}

void TitleBarWidget::setNativeControlsSpacerSize(const QSize &size)
{
    if (d->controlsWidget) return; // custom widget handles its own sizing
    d->spacer->setFixedWidth(size.width());
}

void TitleBarWidget::setNativeControlsWidget(QWidget *widget)
{
    if (!widget) return;

    auto *lay = static_cast<QHBoxLayout *>(layout());

    // Remove the transparent spacer from the layout and hide it
    lay->removeWidget(d->spacer);
    d->spacer->setVisible(false);

    // Insert the custom widget at index 0 (native controls position)
    widget->setParent(this);
    lay->insertWidget(0, widget);
    d->controlsWidget = widget;
}

void TitleBarWidget::setNativeControlsWidgetRight(QWidget *widget)
{
    if (!widget) return;

    auto *lay = static_cast<QHBoxLayout *>(layout());

    // Hide the generic window control buttons — the custom widget replaces them
    d->windowControls->setVisible(false);

    // Insert right before windowControls (end of visible area)
    widget->setParent(this);
    int idx = lay->indexOf(d->windowControls);
    lay->insertWidget(idx, widget);
    d->controlsWidget = widget;
}

void TitleBarWidget::setWindowControlsVisible(bool visible)
{
    d->windowControls->setVisible(visible);
}

void TitleBarWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (d->blocksWindowDrag(childAt(event->pos()), this)) {
            QWidget::mousePressEvent(event);
            return;
        }
        if (auto *wh = window()->windowHandle()) {
            wh->startSystemMove();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (d->blocksWindowDrag(childAt(event->pos()), this)) {
            QWidget::mouseDoubleClickEvent(event);
            return;
        }
        if (auto* w = window()) {
            if (w->isMaximized()) {
                w->showNormal();
            }
            else {
                w->showMaximized();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# ifdef Q_OS_WIN
                QTimer::singleShot(10, w, [w]() { w->showMaximized(); });
# endif
#endif
            }
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void TitleBarWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (window()) {
        window()->installEventFilter(this);  // Listen to the main window
        // Force an initial sync
        bool isMax = window()->isMaximized();
        d->maximizeBtn->setVisible(!isMax);
        d->restoreBtn->setVisible(isMax);
    }
}

bool TitleBarWidget::eventFilter(QObject* obj, QEvent* event)
{
    // If the window state changes (e.g. user drags window to top of screen)
    if (obj == window() && event->type() == QEvent::WindowStateChange) {
        bool isMax = window()->isMaximized();
        d->maximizeBtn->setVisible(!isMax);
        d->restoreBtn->setVisible(isMax);
    }
    return QWidget::eventFilter(obj, event);
}
