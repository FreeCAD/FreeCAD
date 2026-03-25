// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "LinuxTitleBarBackend.h"
#include "LinuxDesktopDetection.h"
#include "LinuxWindowButton.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

// --- LinuxTitleBarBackend implementation ---

void LinuxTitleBarBackend::attach(QWidget *window)
{
    // Pick up system color scheme and icon theme before creating any widgets
    LinuxDesktop::applySystemTheme();

    GenericTitleBarBackend::attach(window);
    m_attachedWindow = window;
}

QString LinuxTitleBarBackend::backendName() const
{
    return QStringLiteral("linux");
}

bool LinuxTitleBarBackend::needsWindowControls() const
{
    return false;
}

QSize LinuxTitleBarBackend::nativeControlsAreaSize()
{
    return {80, 0};
}

PlatformTitleBarBackend::ControlsPosition LinuxTitleBarBackend::nativeControlsPosition() const
{
    LinuxDesktop::ButtonLayout layout = LinuxDesktop::detectButtonLayout();
    // If there are any right-side buttons, place the widget on the right
    if (!layout.right.isEmpty())
        return RightSide;
    return LeftSide;
}

QWidget *LinuxTitleBarBackend::createNativeControlsWidget(QWidget *parent)
{
    auto *container = new QWidget(parent);
    auto *outerLayout = new QVBoxLayout(container);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Button row: vertically centered, caps at 48px so margins stop growing.
    auto *buttonRow = new QWidget(container);
    buttonRow->setMaximumHeight(48);
    auto *layout = new QHBoxLayout(buttonRow);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignVCenter);

    auto *marginFilter = new LinuxSymmetricMarginFilter(
        layout, LinuxWindowButton::ButtonSize, buttonRow);
    buttonRow->installEventFilter(marginFilter);

    // Group hover: hovering any button shows icons on all
    auto *group = new LinuxButtonGroup(buttonRow);

    // Detect button layout from the desktop environment
    LinuxDesktop::ButtonLayout btnLayout = LinuxDesktop::detectButtonLayout();

    auto createButton = [&](const QString &role) -> LinuxWindowButton * {
        if (role == QLatin1String("close"))
            return new LinuxWindowButton(LinuxWindowButton::Close, group, buttonRow);
        if (role == QLatin1String("minimize"))
            return new LinuxWindowButton(LinuxWindowButton::Minimize, group, buttonRow);
        if (role == QLatin1String("maximize"))
            return new LinuxWindowButton(LinuxWindowButton::Maximize, group, buttonRow);
        return nullptr;
    };

    auto connectButton = [container](LinuxWindowButton *btn) {
        switch (btn->role()) {
        case LinuxWindowButton::Close:
            QObject::connect(btn, &QAbstractButton::clicked, container, [container]() {
                if (auto *w = container->window()) w->close();
            });
            break;
        case LinuxWindowButton::Minimize:
            QObject::connect(btn, &QAbstractButton::clicked, container, [container]() {
                if (auto *w = container->window()) w->showMinimized();
            });
            break;
        case LinuxWindowButton::Maximize:
            QObject::connect(btn, &QAbstractButton::clicked, container, [container]() {
                if (auto *w = container->window()) {
                    if (w->isMaximized()) w->showNormal();
                    else w->showMaximized();
                }
            });
            break;
        }
    };

    // Add left-side buttons
    for (const QString &role : btnLayout.left) {
        if (auto *btn = createButton(role)) {
            layout->addWidget(btn);
            connectButton(btn);
        }
    }

    // Spacer between left and right groups
    if (!btnLayout.left.isEmpty() && !btnLayout.right.isEmpty())
        layout->addStretch();

    // Add right-side buttons
    for (const QString &role : btnLayout.right) {
        if (auto *btn = createButton(role)) {
            layout->addWidget(btn);
            connectButton(btn);
        }
    }

    outerLayout->addWidget(buttonRow, 1);
    outerLayout->addStretch();

    return container;
}

int LinuxTitleBarBackend::minimumTitleBarHeight() const
{
    return 38;
}

int LinuxTitleBarBackend::snapTitleBarHeight(int requested) const
{
    if (requested <= 38) return 38;
    if (requested <= 46) return 46;
    return requested;
}

bool LinuxTitleBarBackend::eventFilter(QObject *obj, QEvent *event)
{
    return GenericTitleBarBackend::eventFilter(obj, event);
}

// Platform factory
std::unique_ptr<PlatformTitleBarBackend> PlatformTitleBarBackend::createPlatform()
{
    return std::make_unique<LinuxTitleBarBackend>();
}
