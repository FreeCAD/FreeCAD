// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef TITLEBARWIDGET_H
#define TITLEBARWIDGET_H

#include <QWidget>
#include <memory>

#include "customtitlebarkit/export.h"

class CUSTOMTITLEBARKIT_EXPORT TitleBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TitleBarWidget(QWidget *parent = nullptr);
    ~TitleBarWidget() override;

    /// Left content area — place toolbars, labels, etc. here.
    QWidget *leftArea() const;

    /// Right content area — place toolbars, workbench selectors, etc. here.
    QWidget *rightArea() const;

    /// Spacer for native window controls (only visible on native titlebar backends).
    QWidget *nativeControlsSpacer() const;

    /// Update the spacer to match the native window controls area size.
    void setNativeControlsSpacerSize(const QSize &size);

    /// Replace the native controls spacer with a custom widget (e.g. traffic light buttons).
    /// Takes ownership of the widget. Placed on the left side (index 0).
    void setNativeControlsWidget(QWidget *widget);

    /// Place a custom native controls widget on the right side of the titlebar,
    /// replacing the generic window control buttons. Takes ownership.
    void setNativeControlsWidgetRight(QWidget *widget);

    /// Show or hide the window control buttons (min/max/close).
    void setWindowControlsVisible(bool visible);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // TITLEBARWIDGET_H
