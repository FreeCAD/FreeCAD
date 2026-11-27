// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef MENUINTEGRATION_H
#define MENUINTEGRATION_H

#include <QObject>

#include "customtitlebarkit/export.h"

class QAbstractButton;
class QMenuBar;
class QWidget;
class CustomTitleBarWindow;
class FoldableMenuBar;

/// Abstract base for pluggable menu bar integration strategies.
class MenuIntegration : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    /// Install the menu bar into the window (e.g. native global menu or inline in titlebar).
    virtual void install(QMenuBar *menuBar, CustomTitleBarWindow *window) = 0;

    /// Remove the menu bar from the window.
    virtual void uninstall(CustomTitleBarWindow *window) = 0;
};

/// Default integration: inline menu bar in titlebar left area.
class DefaultMenuIntegration : public MenuIntegration {
    Q_OBJECT
public:
    explicit DefaultMenuIntegration(QObject *parent = nullptr);

    void install(QMenuBar *menuBar, CustomTitleBarWindow *window) override;
    void uninstall(CustomTitleBarWindow *window) override;

private:
    QWidget *m_container = nullptr;
};

/// Foldable menu integration: wraps QMenuBar inside a FoldableMenuBar
/// with an optional brand widget. Collapsed shows only the brand; clicking it expands.
class CUSTOMTITLEBARKIT_EXPORT FoldableMenuIntegration : public MenuIntegration {
    Q_OBJECT
public:
    explicit FoldableMenuIntegration(QWidget *brandWidget = nullptr, QObject *parent = nullptr);

    /// Pin button shown after the last menu, see FoldableMenuBar::setPinButton().
    void setPinButton(QAbstractButton *pinButton);

    void install(QMenuBar *menuBar, CustomTitleBarWindow *window) override;
    void uninstall(CustomTitleBarWindow *window) override;

private:
    QWidget *m_brandWidget = nullptr;
    QAbstractButton *m_pinButton = nullptr;
    FoldableMenuBar *m_foldableBar = nullptr;
};

#endif // MENUINTEGRATION_H
