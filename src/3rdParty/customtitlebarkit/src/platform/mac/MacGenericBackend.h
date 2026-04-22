// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef MACGENERICBACKEND_H
#define MACGENERICBACKEND_H

#include "platform/generic/GenericTitleBarBackend.h"

/// macOS frameless backend with traffic-light-style window control buttons.
/// Used when Qt < 6.10 (no native titlebar APIs).
class MacGenericBackend : public GenericTitleBarBackend {
    Q_OBJECT
public:
    void attach(QWidget *window) override;
    QString backendName() const override;
    bool needsWindowControls() const override;
    QSize nativeControlsAreaSize() override;
    QWidget *createNativeControlsWidget(QWidget *parent) override;
    int minimumTitleBarHeight() const override;
    int snapTitleBarHeight(int requested) const override;
    MenuIntegration *createDefaultMenuIntegration(QObject *parent) override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void applyCocoaStyling();
    QWidget *m_attachedWindow = nullptr;
};

#endif // MACGENERICBACKEND_H
