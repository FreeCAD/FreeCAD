// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MacGenericBackend.h"

#include <QAbstractButton>
#include <QEnterEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QImage>
#include <QPainter>
#include <QVBoxLayout>
#include <QApplication>
#include <QMenuBar>
#include <QWidget>

#include "customtitlebarkit/CustomTitleBarWindow.h"
#include "customtitlebarkit/MenuIntegration.h"

#import <AppKit/AppKit.h>

// --- Border overlay (drawn on top of all Qt content) ---

@interface BorderOverlayView : NSView
@end

@implementation BorderOverlayView
- (NSView *)hitTest:(NSPoint)point {
    return nil; // transparent to all mouse events
}
@end

// --- Traffic light button (self-contained, no QSS dependency) ---

namespace {

/// Tracks hover state for a group of traffic light buttons.
/// When the mouse enters the group widget, all buttons show their icons.
class TrafficLightGroup : public QObject {
public:
    bool hovered = false;

    TrafficLightGroup(QWidget *groupWidget)
        : QObject(groupWidget)
    {
        groupWidget->setAttribute(Qt::WA_Hover);
        groupWidget->installEventFilter(this);
    }

    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::HoverEnter) {
            hovered = true;
            updateButtons();
        } else if (event->type() == QEvent::HoverLeave) {
            hovered = false;
            updateButtons();
        }
        return false;
    }

private:
    void updateButtons()
    {
        for (auto *btn : parent()->findChildren<QAbstractButton *>())
            btn->update();
    }
};

class TrafficLightButton : public QAbstractButton {
public:
    enum Role { Close, Minimize, Maximize };

    TrafficLightButton(Role role, TrafficLightGroup *group, QWidget *parent)
        : QAbstractButton(parent), m_role(role), m_group(group)
    {
        setFixedSize(12, 12);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover);
        setCursor(Qt::ArrowCursor);
    }

    QSize sizeHint() const override { return {12, 12}; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Circle background
        QColor bg;
        if (isDown()) {
            switch (m_role) {
            case Close:    bg = QColor(0xF0, 0x92, 0x89); break;
            case Minimize: bg = QColor(0xFC, 0xEB, 0x74); break;
            case Maximize: bg = QColor(0x87, 0xF2, 0x80); break;
            }
        } else {
            switch (m_role) {
            case Close:    bg = QColor(0xED, 0x6A, 0x5E); break;
            case Minimize: bg = QColor(0xF5, 0xBF, 0x4F); break;
            case Maximize: bg = QColor(0x61, 0xC4, 0x51); break;
            }
        }

        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawEllipse(rect());

        // Show icons when any button in the group is hovered.
        // Draw opaque into a temp image, then composite at desired opacity
        // to avoid overlap darkening from semi-transparent strokes.
        if (!m_group->hovered && !isDown()) return;

        const qreal dpr = devicePixelRatioF();
        QImage icon(QSizeF(size()).toSize() * dpr, QImage::Format_ARGB32_Premultiplied);
        icon.setDevicePixelRatio(dpr);
        icon.fill(Qt::transparent);
        {
            QPainter ip(&icon);
            ip.setRenderHint(QPainter::Antialiasing);

            QColor opaque(0x00, 0x00, 0x00);
            const qreal cx = width() / 2.0;
            const qreal cy = height() / 2.0;

            switch (m_role) {
            case Close: {
                // Thin X
                QPen pen(opaque, 1.25, Qt::SolidLine, Qt::RoundCap);
                ip.setPen(pen);
                const qreal s = 2.5;
                ip.drawLine(QPointF(cx - s, cy - s), QPointF(cx + s, cy + s));
                ip.drawLine(QPointF(cx + s, cy - s), QPointF(cx - s, cy + s));
                break;
            }
            case Minimize: {
                // Short horizontal dash
                QPen pen(opaque, 1.5, Qt::SolidLine, Qt::RoundCap);
                ip.setPen(pen);
                const qreal s = 3;
                ip.drawLine(QPointF(cx - s, cy), QPointF(cx + s, cy));
                break;
            }
            case Maximize: {
                // Two small filled triangles pointing to opposite corners
                QPen pen(opaque, 0.5, Qt::SolidLine, Qt::RoundCap);
                ip.setPen(pen);
                ip.setBrush(opaque);
                const qreal d = 2.75;  // distance from center to corner
                const qreal t = 3.5;  // triangle leg length

                QPointF tl(cx - d, cy - d);
                QPolygonF triTL;
                triTL << tl << QPointF(tl.x() + t, tl.y()) << QPointF(tl.x(), tl.y() + t);
                ip.drawPolygon(triTL);

                QPointF br(cx + d, cy + d);
                QPolygonF triBR;
                triBR << br << QPointF(br.x() - t, br.y()) << QPointF(br.x(), br.y() - t);
                ip.drawPolygon(triBR);
                break;
            }
            }
        }
        p.setOpacity(0.55);
        p.drawImage(0, 0, icon);
    }

private:
    Role m_role;
    TrafficLightGroup *m_group;
};

/// Keeps left/right margins in sync with the computed vertical margin.
/// At height h, top margin from VCenter = (h - 12) / 2. Left/right match that.
class SymmetricMarginFilter : public QObject {
public:
    SymmetricMarginFilter(QHBoxLayout *layout, QObject *parent)
        : QObject(parent), m_layout(layout) {}

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::Resize) {
            int h = static_cast<QWidget *>(obj)->height();
            int margin = (h - 12) / 2;
            m_layout->setContentsMargins(margin, 0, margin, 0);
        }
        return false;
    }

private:
    QHBoxLayout *m_layout;
};

} // anonymous namespace

// --- MacGenericBackend implementation ---

/// macOS-specific integration: uses the native global menu bar.
class NativeMenuIntegration : public MenuIntegration {
public:
    using MenuIntegration::MenuIntegration;

    void install(QMenuBar *menuBar, CustomTitleBarWindow *window) override {
        menuBar->setParent(window);
        auto actions = menuBar->actions();
        for (auto *a : actions) menuBar->removeAction(a);
        menuBar->setNativeMenuBar(true);
        for (auto *a : actions) menuBar->addAction(a);
    }

    void uninstall(CustomTitleBarWindow *) override {}
};

void MacGenericBackend::attach(QWidget *window)
{
    GenericTitleBarBackend::attach(window);

    // GenericTitleBarBackend disables native menus — re-enable for macOS
    QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, false);

    m_attachedWindow = window;
    applyCocoaStyling();
}

void MacGenericBackend::applyCocoaStyling()
{
    if (!m_attachedWindow) return;

    WId wid = m_attachedWindow->winId();
    NSView *view = (__bridge NSView *)(reinterpret_cast<void *>(wid));
    NSWindow *nsWindow = view.window;
    if (!nsWindow) return;

    // Native window shadow
    [nsWindow setHasShadow:YES];

    // Rounded corners via content view layer
    NSView *contentView = nsWindow.contentView;
    contentView.wantsLayer = YES;
    contentView.layer.cornerRadius = 10.0;
    contentView.layer.masksToBounds = YES;

    // Border overlay — sits on top of all Qt content, transparent to mouse
    BorderOverlayView *borderOverlay = [[BorderOverlayView alloc] initWithFrame:contentView.bounds];
    borderOverlay.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    borderOverlay.wantsLayer = YES;
    borderOverlay.layer.cornerRadius = 10.0;
    borderOverlay.layer.borderWidth = 1;
    borderOverlay.layer.borderColor = [NSColor separatorColor].CGColor;
    [contentView addSubview:borderOverlay positioned:NSWindowAbove relativeTo:nil];
}

bool MacGenericBackend::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_attachedWindow && event->type() == QEvent::WinIdChange) {
        applyCocoaStyling();
    }
    return GenericTitleBarBackend::eventFilter(obj, event);
}

QString MacGenericBackend::backendName() const
{
    return QStringLiteral("mac-generic");
}

bool MacGenericBackend::needsWindowControls() const
{
    return false;
}

QSize MacGenericBackend::nativeControlsAreaSize()
{
    return {74, 0};
}

QWidget *MacGenericBackend::createNativeControlsWidget(QWidget *parent)
{
    auto *container = new QWidget(parent);
    auto *outerLayout = new QVBoxLayout(container);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Button row: vertically centered, caps at 48px so margins stop growing.
    // Left/right margins track the vertical margin via SymmetricMarginFilter.
    auto *buttonRow = new QWidget(container);
    buttonRow->setMaximumHeight(48);
    auto *layout = new QHBoxLayout(buttonRow);
    layout->setContentsMargins(8, 0, 8, 0); // initial; updated on resize
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignVCenter);

    auto *marginFilter = new SymmetricMarginFilter(layout, buttonRow);
    buttonRow->installEventFilter(marginFilter);

    // Group hover: hovering any button shows icons on all three
    auto *group = new TrafficLightGroup(buttonRow);

    auto *closeBtn = new TrafficLightButton(TrafficLightButton::Close, group, buttonRow);
    auto *minimizeBtn = new TrafficLightButton(TrafficLightButton::Minimize, group, buttonRow);
    auto *maximizeBtn = new TrafficLightButton(TrafficLightButton::Maximize, group, buttonRow);

    layout->addWidget(closeBtn);
    layout->addWidget(minimizeBtn);
    layout->addWidget(maximizeBtn);

    outerLayout->addWidget(buttonRow, 1); // fills available height up to maxHeight
    outerLayout->addStretch();            // absorbs overflow beyond 48px

    QObject::connect(closeBtn, &QAbstractButton::clicked, container, [container]() {
        if (auto *w = container->window()) w->close();
    });
    QObject::connect(minimizeBtn, &QAbstractButton::clicked, container, [container]() {
        if (auto *w = container->window()) w->showMinimized();
    });
    QObject::connect(maximizeBtn, &QAbstractButton::clicked, container, [container]() {
        if (auto *w = container->window()) {
            if (w->isMaximized()) w->showNormal();
            else w->showMaximized();
        }
    });

    return container;
}

int MacGenericBackend::minimumTitleBarHeight() const
{
    return 28;
}

int MacGenericBackend::snapTitleBarHeight(int requested) const
{
    if (requested <= 28) return 28;
    if (requested <= 38) return 38;
    if (requested <= 46) return 46;
    return requested;
}

MenuIntegration *MacGenericBackend::createDefaultMenuIntegration(QObject *parent)
{
    return new NativeMenuIntegration(parent);
}

// Platform factory — only when MacTitleBarBackend (Qt 6.10) is not available
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
std::unique_ptr<PlatformTitleBarBackend> PlatformTitleBarBackend::createPlatform()
{
    return std::make_unique<MacGenericBackend>();
}
#endif
