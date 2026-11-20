// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "customtitlebarkit/FoldableMenuBar.h"

#include <QAbstractButton>
#include <QActionEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>

struct FoldableMenuBar::Impl {
    QHBoxLayout *layout = nullptr;
    QWidget *brandWidget = nullptr;
    QMenuBar *menuBar = nullptr;
    QWidget *menuContainer = nullptr;
    QWidget *menuWrapper = nullptr;   // inside container, has border trick for centering
    QPointer<QAbstractButton> pinButton;   // inside container, right after menuWrapper
    QWidget *pinnedSeparator = nullptr;    // in the layout after the pinned container
    QTimer *collapseTimer = nullptr;
    QPropertyAnimation *animation = nullptr;
    QList<QWidget *> maskTargets;
    bool foldable = false;
    bool overlayExpand = true;
    bool expanded = true;
    bool pinned = false;
    int revealWidth = 0;

    /// The menu items fold and unfold: foldable and not pinned
    bool folds() const {
        return foldable && !pinned;
    }

    /// Extra width of the overlay, given to the menu bar wrapper
    static constexpr int overlaySlack = 4;

    bool hasPinButton() const {
        return pinButton && !pinButton->isHidden();
    }

    /// Width uncovered by the reveal: the menu items, then the pin button,
    /// which comes after the slack of the overlay
    int contentWidth() const {
        int width = menuBar->sizeHint().width();
        if (hasPinButton())
            width += overlaySlack + pinButton->sizeHint().width();
        return width;
    }

    void updateClip() {
        if (!menuBar) return;
        if (!folds() || revealWidth >= contentWidth()) {
            menuBar->clearMask();
            if (pinButton)
                pinButton->clearMask();
        }
        else if (revealWidth <= 0) {
            menuBar->setMask(QRegion(-1, -1, 1, 1));
            if (pinButton)
                pinButton->setMask(QRegion(-1, -1, 1, 1));
        }
        else {
            menuBar->setMask(QRegion(0, 0, revealWidth, menuBar->height()));
            if (pinButton) {
                // The menu bar starts the container, the pin button ends it
                int pinReveal = revealWidth - pinButton->x();
                if (pinReveal <= 0)
                    pinButton->setMask(QRegion(-1, -1, 1, 1));
                else
                    pinButton->setMask(QRegion(0, 0, pinReveal, pinButton->height()));
            }
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

    /// The menu bar is being driven from the keyboard (Alt, then the arrow keys)
    bool isKeyboardActive() const {
        return menuBar && menuBar->hasFocus();
    }

    /// Follow a menu bar: the menus added to it, and its activation from the keyboard.
    void watchMenuBar(FoldableMenuBar *self) {
        menuBar->installEventFilter(self);
        // Alt and Alt+mnemonic make an item current before the menu bar takes the focus
        // or opens the menu: unfold first, so that both happen on a menu bar that shows.
        // A folded menu bar is masked out, so the mouse cannot make an item current.
        QObject::connect(menuBar, &QMenuBar::hovered, self, [this, self]() {
            if (!folds() || expanded) return;
            collapseTimer->stop();
            self->setExpanded(true);
            // The menu that Alt+mnemonic opens is placed right after this
            menuContainer->layout()->activate();
            menuWrapper->layout()->activate();
        });
    }

    void connectMenuCollapse(FoldableMenuBar *self, QMenu *menu) {
        QObject::connect(menu, &QMenu::aboutToHide, self, [this, self]() {
            if (folds() && !self->underMouse() && !isMouseOverOverlay()) {
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

        int width = contentWidth() + (hasPinButton() ? 0 : overlaySlack);
        menuContainer->setGeometry(x, origin.y(), width, self->height());
    }

    /// Pinned menu items are followed by what the menu bar covers when it unfolds:
    /// a line tells them apart. It ends the layout, after the container.
    void updatePinnedSeparator() {
        layout->removeWidget(pinnedSeparator);
        if (foldable && pinned) {
            layout->addWidget(pinnedSeparator);
            pinnedSeparator->show();
        }
        else {
            pinnedSeparator->hide();
        }
    }

    /// Pinned: the container leaves the overlay for the layout, after the brand,
    /// and shows the menu items whole, without mask and without folding.
    /// Returns whether the menu items were folded.
    bool applyPinned(FoldableMenuBar *self) {
        collapseTimer->stop();
        animation->stop();
        clearOverlayMasks();
        if (menuContainer->parent() != self)
            menuContainer->setParent(self);
        if (layout->indexOf(menuContainer) < 0)
            layout->addWidget(menuContainer);
        menuContainer->setMaximumWidth(QWIDGETSIZE_MAX);
        menuContainer->show();
        updatePinnedSeparator();
        revealWidth = contentWidth();
        updateClip();
        bool wasFolded = !expanded;
        expanded = true;
        return wasFolded;
    }

    /// Unpinned: the menu items, still shown, fold like after a click on the brand.
    /// In overlay mode, the container goes back from the layout to the overlay first,
    /// so that the widgets after it take their place again under the folding menu.
    void releasePinned(FoldableMenuBar *self) {
        updatePinnedSeparator();
        if (overlayExpand) {
            layout->removeWidget(menuContainer);
            if (QWidget *win = self->window()) {
                menuContainer->setParent(win);
                positionOverlay(self);
                menuContainer->show();
                menuContainer->raise();
            }
        }
        expanded = true;
        self->setRevealWidth(contentWidth());
        self->setExpanded(false);
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

    // Horizontal: the menu bar wrapper, then the pin button if any (setPinButton()).
    auto *containerLayout = new QHBoxLayout(d->menuContainer);
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

    // Line after the pinned menu items (updatePinnedSeparator()), over the whole height
    // of the title bar. Its holder spaces it; style sheets colour the line by its name.
    d->pinnedSeparator = new QWidget(this);
    auto *separatorLayout = new QHBoxLayout(d->pinnedSeparator);
    separatorLayout->setContentsMargins(4, 0, 6, 0);
    separatorLayout->setSpacing(0);
    auto *separatorLine = new QFrame(d->pinnedSeparator);
    separatorLine->setObjectName(QStringLiteral("titleBarMenuSeparator"));
    separatorLine->setFrameShape(QFrame::NoFrame);
    separatorLine->setFixedWidth(1);
    separatorLine->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    separatorLine->setBackgroundRole(QPalette::Mid);
    separatorLine->setAutoFillBackground(true);
    separatorLayout->addWidget(separatorLine);
    d->pinnedSeparator->hide();

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
            if (d->pinButton)
                d->pinButton->clearMask();
        }
    });

    // Timer for delayed collapse when mouse leaves
    d->collapseTimer = new QTimer(this);
    d->collapseTimer->setSingleShot(true);
    d->collapseTimer->setInterval(400);
    connect(d->collapseTimer, &QTimer::timeout, this, [this]() {
        if (d->folds() && !underMouse() && !d->isMouseOverOverlay()
            && !d->isAnyMenuVisible() && !d->isKeyboardActive()) {
            setExpanded(false);
        }
    });

    d->watchMenuBar(this);
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

    // Watch for future menus added to this QMenuBar, and for its keyboard activation
    d->watchMenuBar(this);

    // Apply current foldable state
    if (d->foldable && !d->expanded) {
        d->updateClip();
    }
}

QMenu *FoldableMenuBar::addMenu(const QString &title)
{
    // Its collapse is connected when the menu bar reports the new action (eventFilter)
    return d->menuBar->addMenu(title);
}

void FoldableMenuBar::setBrandWidget(QWidget *brand)
{
    if (d->brandWidget) {
        d->brandWidget->removeEventFilter(this);
        d->layout->removeWidget(d->brandWidget);
        d->brandWidget->setParent(nullptr);
    }
    d->brandWidget = brand;
    if (brand) {
        brand->setParent(this);
        brand->setAttribute(Qt::WA_NoMousePropagation, true);
        brand->installEventFilter(this);
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

    // Pinning is a choice between folded and unfolded: only foldable menu items offer it
    if (d->pinButton)
        d->pinButton->setVisible(foldable);
    if (foldable && d->pinned)
        d->applyPinned(this);
    else
        d->updatePinnedSeparator();
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

    // Pinned menu items stay in the layout in both modes
    if (d->foldable && d->pinned)
        d->applyPinned(this);
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
    // Pinned menu items do not fold: unpin them instead
    if (!expanded && d->foldable && d->pinned) return;
    d->expanded = expanded;

    if (d->foldable) {
        if (d->animation->state() == QAbstractAnimation::Running)
            d->animation->stop();

        int targetWidth = d->contentWidth();

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

void FoldableMenuBar::setPinButton(QAbstractButton *button)
{
    if (d->pinButton == button) return;

    if (d->pinButton) {
        disconnect(d->pinButton.data(), nullptr, this, nullptr);
        d->menuContainer->layout()->removeWidget(d->pinButton.data());
        d->pinButton->clearMask();
        d->pinButton->setParent(nullptr);
    }
    d->pinButton = button;
    if (button) {
        button->setCheckable(true);
        button->setParent(d->menuContainer);
        button->setVisible(d->foldable);
        static_cast<QHBoxLayout *>(d->menuContainer->layout())
            ->addWidget(button, 0, Qt::AlignVCenter);
        // Queued: the button moves with the container when the state changes,
        // which is better not done inside its own mouse release
        connect(button, &QAbstractButton::toggled, this, &FoldableMenuBar::setPinned,
                Qt::QueuedConnection);
        setPinned(button->isChecked());
        d->updateClip();
    }
}

QAbstractButton *FoldableMenuBar::pinButton() const
{
    return d->pinButton;
}

void FoldableMenuBar::setPinned(bool pinned)
{
    if (d->pinned == pinned) return;
    d->pinned = pinned;
    if (d->pinButton && d->pinButton->isChecked() != pinned)
        d->pinButton->setChecked(pinned);

    if (d->foldable) {
        d->collapseTimer->stop();
        d->animation->stop();
        if (pinned) {
            if (d->applyPinned(this))
                Q_EMIT expandedChanged(true);
        }
        else {
            d->releasePinned(this);
        }
    }

    Q_EMIT pinnedChanged(pinned);
}

bool FoldableMenuBar::isPinned() const
{
    return d->pinned;
}

void FoldableMenuBar::enterEvent(QEnterEvent *event)
{
    d->collapseTimer->stop();
    QWidget::enterEvent(event);
}

void FoldableMenuBar::leaveEvent(QEvent *event)
{
    if (d->folds()) {
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
    if (obj == d->brandWidget && event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton && d->folds()) {
            d->collapseTimer->stop();
            mouseEvent->accept();
            return true;
        }
    }

    if (obj == d->brandWidget && event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton
            && d->brandWidget->rect().contains(mouseEvent->position().toPoint())
            && d->folds()) {
            d->collapseTimer->stop();
            setExpanded(!d->expanded);
            mouseEvent->accept();
            return true;
        }
    }

    // Watch for menus being added to the external QMenuBar
    if (obj == d->menuBar && event->type() == QEvent::ActionAdded) {
        auto *actionEvent = static_cast<QActionEvent *>(event);
        if (auto *menu = actionEvent->action()->menu())
            d->connectMenuCollapse(this, menu);
    }

    // Keyboard navigation ended (Esc, Alt again, a click elsewhere): fold again
    if (obj == d->menuBar && event->type() == QEvent::FocusOut && d->folds()) {
        d->collapseTimer->start();
    }

    // In overlay mode, track mouse on the floating container
    if (d->overlayExpand && obj == d->menuContainer) {
        if (event->type() == QEvent::Enter) {
            d->collapseTimer->stop();
        } else if (event->type() == QEvent::Leave) {
            if (d->folds() && !d->isAnyMenuVisible())
                d->collapseTimer->start();
        }
    }

    return QWidget::eventFilter(obj, event);
}
