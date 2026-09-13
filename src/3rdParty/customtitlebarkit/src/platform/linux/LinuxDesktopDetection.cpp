// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "LinuxDesktopDetection.h"

#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QApplication>
#include <QStyle>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>


namespace LinuxDesktop {

// CUSTOMTITLEBARKIT_LINUX_DESKTOP env var: "gtk", "kde", or "fallback".
// When set, only that detection path runs. When unset, all paths cascade.
enum class DesktopOverride { None, Gtk, Kde, Fallback };

static DesktopOverride desktopOverride()
{
    static const DesktopOverride override = [] {
        QByteArray env = qgetenv("CUSTOMTITLEBARKIT_LINUX_DESKTOP");
        if (env.isEmpty()) return DesktopOverride::None;
        if (env.toLower() == "gtk")      return DesktopOverride::Gtk;
        if (env.toLower() == "kde")      return DesktopOverride::Kde;
        if (env.toLower() == "fallback") return DesktopOverride::Fallback;
        return DesktopOverride::None;
    }();
    return override;
}

static bool tryGtk() { auto o = desktopOverride(); return o == DesktopOverride::None || o == DesktopOverride::Gtk; }
static bool tryKde() { auto o = desktopOverride(); return o == DesktopOverride::None || o == DesktopOverride::Kde; }

// Find the system gsettings binary, avoiding any conda/pixi-bundled copy.
static QString systemGsettings()
{
    static const QString path = [] {
        // Prefer the system binary at well-known paths
        for (const auto *candidate : {"/usr/bin/gsettings", "/usr/local/bin/gsettings"}) {
            if (QFile::exists(QLatin1String(candidate)))
                return QString::fromLatin1(candidate);
        }
        // Last resort: PATH lookup (may find conda's copy)
        return QStringLiteral("gsettings");
    }();
    return path;
}

bool isWayland()
{
    static const bool wayland = QGuiApplication::platformName() == QLatin1String("wayland");
    return wayland;
}

QString detectColorScheme()
{
    // GTK/GNOME: gsettings
    if (tryGtk()) {
        QProcess proc;
        proc.start(systemGsettings(),
                   {QStringLiteral("get"),
                    QStringLiteral("org.gnome.desktop.interface"),
                    QStringLiteral("color-scheme")});
        if (proc.waitForFinished(500)) {
            QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            output.remove(QLatin1Char('\''));
            if (output.contains(QLatin1String("dark")))
                return QStringLiteral("prefer-dark");
            if (output.contains(QLatin1String("light")))
                return QStringLiteral("prefer-light");
        }
    }

    // KDE: ~/.config/kdeglobals
    if (tryKde()) {
        QString kdePath = QStandardPaths::locate(
            QStandardPaths::GenericConfigLocation, QStringLiteral("kdeglobals"));
        if (!kdePath.isEmpty()) {
            QSettings kdeSettings(kdePath, QSettings::IniFormat);
            kdeSettings.beginGroup(QStringLiteral("General"));
            QString colorScheme = kdeSettings.value(QStringLiteral("ColorScheme")).toString();
            if (colorScheme.contains(QLatin1String("Dark"), Qt::CaseInsensitive))
                return QStringLiteral("prefer-dark");
            if (!colorScheme.isEmpty())
                return QStringLiteral("prefer-light");
        }
    }

    return QStringLiteral("default");
}

static ButtonLayout parseGnomeButtonLayout(const QString &layout)
{
    ButtonLayout result;
    QString cleaned = layout;
    cleaned.remove(QLatin1Char('\''));
    cleaned.remove(QLatin1Char('"'));

    QStringList sides = cleaned.split(QLatin1Char(':'));
    if (sides.size() >= 1) {
        for (const QString &btn : sides[0].split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            QString b = btn.trimmed();
            if (b == QLatin1String("close") || b == QLatin1String("minimize")
                || b == QLatin1String("maximize"))
                result.left.append(b);
        }
    }
    if (sides.size() >= 2) {
        for (const QString &btn : sides[1].split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            QString b = btn.trimmed();
            if (b == QLatin1String("close") || b == QLatin1String("minimize")
                || b == QLatin1String("maximize"))
                result.right.append(b);
        }
    }
    return result;
}

ButtonLayout detectButtonLayout()
{
    // GTK/GNOME: gsettings
    if (tryGtk()) {
        QProcess proc;
        proc.start(systemGsettings(),
                   {QStringLiteral("get"),
                    QStringLiteral("org.gnome.desktop.wm.preferences"),
                    QStringLiteral("button-layout")});
        if (proc.waitForFinished(500)) {
            QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            if (!output.isEmpty()) {
                ButtonLayout layout = parseGnomeButtonLayout(output);
                if (!layout.left.isEmpty() || !layout.right.isEmpty())
                    return layout;
            }
        }
    }

    // KDE: ~/.config/kwinrc
    if (tryKde()) {
        QString kwinPath = QStandardPaths::locate(
            QStandardPaths::GenericConfigLocation, QStringLiteral("kwinrc"));
        if (!kwinPath.isEmpty()) {
            QSettings kwinSettings(kwinPath, QSettings::IniFormat);
            kwinSettings.beginGroup(QStringLiteral("org.kde.kdecoration2"));

            auto parseKdeButtons = [](const QString &str) -> QStringList {
                QStringList buttons;
                for (QChar c : str) {
                    if (c == QLatin1Char('X'))
                        buttons.append(QStringLiteral("close"));
                    else if (c == QLatin1Char('I'))
                        buttons.append(QStringLiteral("minimize"));
                    else if (c == QLatin1Char('A'))
                        buttons.append(QStringLiteral("maximize"));
                }
                return buttons;
            };

            QString leftStr = kwinSettings.value(QStringLiteral("ButtonsOnLeft")).toString();
            QString rightStr = kwinSettings.value(QStringLiteral("ButtonsOnRight")).toString();

            ButtonLayout result;
            if (!leftStr.isEmpty())
                result.left = parseKdeButtons(leftStr);
            if (!rightStr.isEmpty())
                result.right = parseKdeButtons(rightStr);

            if (!result.left.isEmpty() || !result.right.isEmpty())
                return result;
        }
    }

    // Fallback: right side
    return {{}, {QStringLiteral("minimize"), QStringLiteral("maximize"), QStringLiteral("close")}};
}

QString detectIconTheme()
{
    // GTK/GNOME: gsettings
    if (tryGtk()) {
        QProcess proc;
        proc.start(systemGsettings(),
                   {QStringLiteral("get"),
                    QStringLiteral("org.gnome.desktop.interface"),
                    QStringLiteral("icon-theme")});
        if (proc.waitForFinished(500)) {
            QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            output.remove(QLatin1Char('\''));
            output.remove(QLatin1Char('"'));
            if (!output.isEmpty())
                return output;
        }
    }

    // KDE: ~/.config/kdeglobals
    if (tryKde()) {
        QString kdePath = QStandardPaths::locate(
            QStandardPaths::GenericConfigLocation, QStringLiteral("kdeglobals"));
        if (!kdePath.isEmpty()) {
            QSettings kdeSettings(kdePath, QSettings::IniFormat);
            kdeSettings.beginGroup(QStringLiteral("Icons"));
            QString theme = kdeSettings.value(QStringLiteral("Theme")).toString();
            if (!theme.isEmpty())
                return theme;
        }
    }

    return {};
}

void applySystemTheme()
{
    static bool applied = false;
    if (applied) return;
    applied = true;

    QString scheme = detectColorScheme();
    if (scheme == QLatin1String("prefer-dark"))
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    else if (scheme == QLatin1String("prefer-light"))
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);

    // Force the style to regenerate its palette from the new color scheme,
    // then apply it application-wide so all widgets pick it up.
    if (auto *app = qobject_cast<QApplication *>(QCoreApplication::instance())) {
        if (auto *style = app->style())
            app->setPalette(style->standardPalette());
    }

    // Apply system icon theme if Qt doesn't have a platform theme plugin
    // providing the real one.
    QString systemTheme = detectIconTheme();
    if (!systemTheme.isEmpty() && systemTheme != QIcon::themeName())
        QIcon::setThemeName(systemTheme);
}

} // namespace LinuxDesktop
