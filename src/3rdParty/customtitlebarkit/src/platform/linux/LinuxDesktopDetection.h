// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef LINUXDESKTOPDETECTION_H
#define LINUXDESKTOPDETECTION_H

#include <QString>
#include <QStringList>

namespace LinuxDesktop {

/// Whether the app is running on Wayland.
bool isWayland();

/// Color scheme: "prefer-dark", "prefer-light", or "default".
/// Uses org.freedesktop.portal.Settings DBus, then gsettings/KDE fallbacks.
QString detectColorScheme();

/// Button layout as an ordered list of roles: "close", "minimize", "maximize".
/// Respects GNOME gsettings and KDE kwinrc, falls back to right-side layout.
struct ButtonLayout {
    QStringList left;   // buttons on the left side of the titlebar
    QStringList right;  // buttons on the right side of the titlebar
};

ButtonLayout detectButtonLayout();

/// Detect the system icon theme name (e.g. "Yaru", "Adwaita", "breeze").
QString detectIconTheme();

/// Apply system color scheme and icon theme to the Qt application,
/// if the platform theme plugin hasn't already done so.
/// Safe to call multiple times; only acts on first call.
void applySystemTheme();

} // namespace LinuxDesktop

#endif // LINUXDESKTOPDETECTION_H
