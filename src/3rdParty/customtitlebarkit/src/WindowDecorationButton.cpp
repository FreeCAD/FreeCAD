// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "WindowDecorationButton.h"
#include <QStyle>
#include <QIcon>

WindowDecorationButton::WindowDecorationButton(Role role, QWidget* parent)
    : QPushButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setFlat(true);

    // Set a default icon size (QSS can override this via qproperty-iconSize)
    setIconSize(QSize(16, 16));
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    switch (role) {
        case Minimize:
            setObjectName(QStringLiteral("minimizeButton"));
            setIcon(QIcon(":/customtitlebarkit/resources/icons/window-minimize.svg"));
            break;
        case Maximize:
            setObjectName(QStringLiteral("maximizeButton"));
            setIcon(QIcon(":/customtitlebarkit/resources/icons/window-maximize.svg"));
            break;
        case Restore:
            setObjectName(QStringLiteral("restoreButton"));
            setIcon(QIcon(":/customtitlebarkit/resources/icons/window-restore.svg"));
            break;
        case Close:
            setObjectName(QStringLiteral("closeButton"));
            setIcon(QIcon(":/customtitlebarkit/resources/icons/window-close.svg"));
            break;
    }
}
