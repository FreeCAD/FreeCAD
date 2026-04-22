// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "customtitlebarkit/FoldableMenuBar.h"

#include <QActionEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QPropertyAnimation>
#include <QTimer>

struct FoldableMenuBar::Impl {
    QHBoxLayout *layout = nullptr;
    QWidget *brandWidget = nullptr;
    QMenuBar *menuBar = nullptr;
    QWidget *menuContainer = nullptr;
    QWidget *menuWrapper = nullptr;   // inside container, has border trick for centering
    QTimer *collapseTimer = nullptr;
    QPropertyAnimation *animation = nullptr;
    QList<QWidget *> maskTargets;
    bool foldable = false;
    bool overlayExpand = true;
    bool expanded = true;
    int revealWidth = 0;

    void updateClip() {
        if (!menuBar) return;
        if (!foldable || revealWidth >= menuBar->sizeHint().width()) {
            menuBar->clearMask();
        }
        else if (revealWidth <= 0) {
            menuBar->setMask(QRegion(-1, -1, 1, 1));
        }
        else {
            menuBar->setMask(QRegion(0, 0, revealWidth, menuBar->height()));
        }
    }

    bool isAnyMenuVisible() const {
        if (!menuBar) return false;
        for (auto *action : menuBar->actions()) {
            if (action->menu() && action->menu()->isVisible())
                return true;
        }
        return false;
    }

    bool isMouseOverOverlay() const {
        return overlayExpand && menuContainer && menuContainer->underMouse();
    }

    void connectMenuCollapse(FoldableMenuBar *self, QMenu *menu) {
        QObject::connect(menu, &QMenu::aboutToHide, self, [this, self]() {
            if (foldable && !self->underMouse() && !isMouseOverOverlay()) {
                collapseTimer->start();
            }
        });
    }

    void positionOverlay(FoldableMenuBar *self) {
        QWidget *win = self->window();
        if (!win || !menuContainer) return;

        QPoint origin = self->mapTo(win, QPoint(0, 0));
        int x = origin.x();
        if (brandWidget)
            x += brandWidget->geometry().right() + 1 + layout->spacing();

        menuContainer->setGeometry(x, origin.y(),
                                   menuBar->sizeHint().width() + 4, self->height());
    }

    /// Mask target widgets to hide their content in the overlay region.
    /// The mask grows with revealWidth so content disappears progressively.
    void updateOverlayMasks(FoldableMenuBar *self) {
        if (maskTargets.isEmpty() || !overlayExpand) return;

        if (revealWidth <= 0 || !menuContainer->isVisible()) {
            for (auto *w : maskTargets)
                w->clearMask();
            return;
        }

        QWidget *win = self->window();
        if (!win) return;

        // Overlay rect in window coordinates, width = current revealWidth
        QPoint overlayTopLeft = menuContainer->mapTo(win, QPoint(0, 0));
        QRect overlayRect(overlayTopLeft, QSize(revealWidth, menuContainer->height()));

        for (auto *target : maskTargets) {
            // Convert overlay rect to target's coordinate system
            QPoint inTarget = target->mapFrom(win, overlayRect.topLeft());
            QRect overlayInTarget(inTarget, overlayRect.size());
            QRect clipped = target->rect().intersected(overlayInTarget);

            if (clipped.isEmpty()) {
                target->clearMask();
            } else {
                target->setMask(QRegion(target->rect()) - QRegion(clipped));
            }
        }
    }

    void clearOverlayMasks() {
        for (auto *w : maskTargets)
            w->clearMask();
    }
};

FoldableMenuBar::FoldableMenuBar(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    setAttribute(Qt::WA_LayoutOnEntireRect, true);
#endif
    d->layout = new QHBoxLayout(this);
    d->layout->setContentsMargins(2, 0, 2, 0);
    d->layout->setSpacing(0);

    // Real QMenuBar — gets tracking, keyboard nav, mnemonics for free.
    // Wrapped in a container with VBox to force vertical centering,
    // since QMenuBar overrides size policy internally.
    d->menuBar = new QMenuBar(this);
    d->menuBar->setNativeMenuBar(false);
    d->menuBar->setProperty("foldable", true);

    // Outer container — transparent so the titlebar background (gradient, etc.)
    // shows through. Mask targets (leftArea/rightArea) clip their content
    // underneath so only menu items are visible in the overlay region.
    d->menuContainer = new QWidget(this);

    auto *containerLayout = new QVBoxLayout(d->menuContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    // Inner wrapper — transparent border forces Qt's styled rendering path,
    // needed for proper vertical centering of QMenuBar (platform-native ignores layout).
    d->menuWrapper = new QWidget(d->menuContainer);
    d->menuWrapper->setStyleSheet("border: 0px solid transparent;");
    auto *wrapperLayout = new QVBoxLayout(d->menuWrapper);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->setSpacing(0);
    wrapperLayout->addWidget(d->menuBar, 0, Qt::AlignVCenter);
    containerLayout->addWidget(d->menuWrapper);

    // Overlay is default — menuContainer is NOT in the layout (floats over content).
    // In non-overlay mode it gets added to the layout in setOverlayExpand(false).

    // Track enter/leave on the overlay container for hover detection
    d->menuContainer->installEventFilter(this);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // Animate revealWidth — only changes a clip mask, no layout involved
    d->animation = new QPropertyAnimation(this, "revealWidth", this);
    d->animation->setDuration(150);
    d->animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(d->animation, &QPropertyAnimation::finished, this, [this]() {
        if (!d->expanded) {
            d->clearOverlayMasks();
            if (d->overlayExpand && d->menuContainer->parent() != this) {
                d->menuContainer->setParent(this);
                d->menuContainer->show();
                d->menuContainer->setGeometry(0, 0, 0, 0);
            }
            else if (!d->overlayExpand) {
                d->menuContainer->setMaximumWidth(0);
            }
        }
        if (d->expanded) {
            d->menuBar->clearMask();
        }
    });

    // Timer for delayed collapse when mouse leaves
    d->collapseTimer = new QTimer(this);
    d->collapseTimer->setSingleShot(true);
    d->collapseTimer->setInterval(400);
    connect(d->collapseTimer, &QTimer::timeout, this, [this]() {
        if (d->foldable && !underMouse() && !d->isMouseOverOverlay()
            && !d->isAnyMenuVisible()) {
            setExpanded(false);
        }
    });
}

FoldableMenuBar::~FoldableMenuBar()
{
    d->clearOverlayMasks();
    // In overlay mode, menuContainer may have been reparented to window()
    if (d->menuContainer && d->menuContainer->parent() != this)
        delete d->menuContainer;
}

int FoldableMenuBar::revealWidth() const
{
    return d->revealWidth;
}

void FoldableMenuBar::setRevealWidth(int w)
{
    d->revealWidth = w;
    d->updateClip();
    d->updateOverlayMasks(this);
}

void FoldableMenuBar::setMenuBar(QMenuBar *menuBar)
{
    if (menuBar == d->menuBar) return;

    // Remove old menuBar from wrapper
    auto *wrapperLayout = d->menuWrapper->layout();
    if (d->menuBar) {
        d->menuBar->removeEventFilter(this);
        wrapperLayout->removeWidget(d->menuBar);
        delete d->menuBar;
    }

    // Install new one
    d->menuBar = menuBar;
    d->menuBar->setNativeMenuBar(false);
    d->menuBar->setProperty("foldable", true);
    d->menuBar->setParent(d->menuWrapper);
    static_cast<QVBoxLayout *>(wrapperLayout)->addWidget(d->menuBar, 0, Qt::AlignVCenter);

    // Connect collapse for existing menus
    for (auto *action : d->menuBar->actions()) {
        if (auto *menu = action->menu())
            d->connectMenuCollapse(this, menu);
    }

    // Watch for future menus added to this QMenuBar
    d->menuBar->installEventFilter(this);

    // Apply current foldable state
    if (d->foldable && !d->expanded) {
        d->updateClip();
    }
}

QMenu *FoldableMenuBar::addMenu(const QString &title)
{
    auto *menu = d->menuBar->addMenu(title);
    d->connectMenuCollapse(this, menu);
    return menu;
}

void FoldableMenuBar::setBrandWidget(QWidget *brand)
{
    if (d->brandWidget) {
        d->layout->removeWidget(d->brandWidget);
        d->brandWidget->setParent(nullptr);
    }
    d->brandWidget = brand;
    if (brand) {
        brand->setParent(this);
        d->layout->insertWidget(0, brand);
    }
}

QWidget *FoldableMenuBar::brandWidget() const
{
    return d->brandWidget;
}

void FoldableMenuBar::setFoldable(bool foldable)
{
    if (d->foldable == foldable) return;
    d->foldable = foldable;
    if (foldable) {
        d->expanded = false;
        d->revealWidth = 0;
        d->updateClip();
        if (d->overlayExpand) {
            d->menuContainer->setGeometry(0, 0, 0, 0);
        }
        else {
            d->menuContainer->setMaximumWidth(0);
        }
    }
    else {
        d->expanded = true;
        d->menuBar->clearMask();
        if (!d->overlayExpand) {
            d->menuContainer->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    }
}

bool FoldableMenuBar::isFoldable() const
{
    return d->foldable;
}

void FoldableMenuBar::setOverlayExpand(bool overlay)
{
    if (d->overlayExpand == overlay) return;
    d->overlayExpand = overlay;

    if (overlay) {
        // Remove from layout — it will float as an overlay when expanded
        d->layout->removeWidget(d->menuContainer);
        d->menuContainer->setMaximumWidth(QWIDGETSIZE_MAX);
        if (d->foldable && !d->expanded) {
            d->menuContainer->setGeometry(0, 0, 0, 0);
        }
    }
    else {
        d->clearOverlayMasks();
        // Ensure menuContainer is parented to this and add to layout
        if (d->menuContainer->parent() != this) {
            d->menuContainer->setParent(this);
            d->menuContainer->show();
        }
        d->layout->addWidget(d->menuContainer);
        if (d->foldable && !d->expanded) {
            d->menuContainer->setMaximumWidth(0);
        }
        else {
            d->menuContainer->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    }
}

bool FoldableMenuBar::isOverlayExpand() const
{
    return d->overlayExpand;
}

void FoldableMenuBar::setOverlayMaskTargets(const QList<QWidget *> &targets)
{
    d->clearOverlayMasks();
    d->maskTargets = targets;
}

void FoldableMenuBar::setExpanded(bool expanded)
{
    if (d->expanded == expanded) return;
    d->expanded = expanded;

    if (d->foldable) {
        if (d->animation->state() == QAbstractAnimation::Running)
            d->animation->stop();

        int targetWidth = d->menuBar->sizeHint().width();

        if (expanded) {
            if (d->overlayExpand) {
                // Reparent to window so menu floats above all titlebar content
                QWidget *win = window();
                if (win) {
                    d->menuContainer->setParent(win);
                    d->positionOverlay(this);
                    d->menuContainer->show();
                    d->menuContainer->raise();
                }
            }
            else {
                d->menuContainer->setMaximumWidth(QWIDGETSIZE_MAX);
            }

            d->animation->setStartValue(0);
            d->animation->setEndValue(targetWidth);
        } else {
            d->animation->setStartValue(d->revealWidth);
            d->animation->setEndValue(0);
        }
        d->animation->start();
    }
    else {
        if (expanded) {
            d->menuBar->clearMask();
        }
        else {
            d->menuBar->setMask(QRegion(-1, -1, 1, 1));
        }
    }

    Q_EMIT expandedChanged(expanded);
}

bool FoldableMenuBar::isExpanded() const
{
    return d->expanded;
}

void FoldableMenuBar::enterEvent(QEnterEvent *event)
{
    d->collapseTimer->stop();
    if (d->foldable && !d->expanded) {
        setExpanded(true);
    }
    QWidget::enterEvent(event);
}

void FoldableMenuBar::leaveEvent(QEvent *event)
{
    if (d->foldable) {
        d->collapseTimer->start();
    }
    QWidget::leaveEvent(event);
}

void FoldableMenuBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Update mask if needed (menuBar height may have changed)
    if (d->foldable && !d->expanded) {
        d->updateClip();
    }
}

bool FoldableMenuBar::eventFilter(QObject *obj, QEvent *event)
{
    // Watch for menus being added to the external QMenuBar
    if (obj == d->menuBar && event->type() == QEvent::ActionAdded) {
        auto *actionEvent = static_cast<QActionEvent *>(event);
        if (auto *menu = actionEvent->action()->menu())
            d->connectMenuCollapse(this, menu);
    }

    // In overlay mode, track mouse on the floating container
    if (d->overlayExpand && obj == d->menuContainer) {
        if (event->type() == QEvent::Enter) {
            d->collapseTimer->stop();
        } else if (event->type() == QEvent::Leave) {
            if (d->foldable && !d->isAnyMenuVisible())
                d->collapseTimer->start();
        }
    }

    return QWidget::eventFilter(obj, event);
}
