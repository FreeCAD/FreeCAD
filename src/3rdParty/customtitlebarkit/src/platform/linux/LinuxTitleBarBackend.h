// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef LINUXTITLEBARBACKEND_H
#define LINUXTITLEBARBACKEND_H

#include "platform/generic/GenericTitleBarBackend.h"

/// Linux frameless backend with system-themed window control buttons.
/// Extends GenericTitleBarBackend with native-looking buttons that adapt
/// to the current desktop theme (GNOME, KDE, XFCE, etc.) via QPalette
/// and QIcon::fromTheme.
class LinuxTitleBarBackend : public GenericTitleBarBackend {
    Q_OBJECT
public:
    void attach(QWidget *window) override;
    QString backendName() const override;
    bool needsWindowControls() const override;
    QSize nativeControlsAreaSize() override;
    ControlsPosition nativeControlsPosition() const override;
    QWidget *createNativeControlsWidget(QWidget *parent) override;
    int minimumTitleBarHeight() const override;
    int snapTitleBarHeight(int requested) const override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QWidget *m_attachedWindow = nullptr;
};

#endif // LINUXTITLEBARBACKEND_H
