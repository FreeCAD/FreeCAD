// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef WINDOWDECORATIONBUTTON_H
#define WINDOWDECORATIONBUTTON_H

#include <QPushButton>

/// A borderless window control button.
/// Themeable via QSS — object names: "minimizeButton", "maximizeButton", "restoreButton", "closeButton".
class WindowDecorationButton: public QPushButton
{
    Q_OBJECT

public:
    enum Role
    {
        Minimize,
        Maximize,
        Restore,
        Close
    };

    explicit WindowDecorationButton(Role role, QWidget* parent = nullptr);
};

#endif  // WINDOWDECORATIONBUTTON_H