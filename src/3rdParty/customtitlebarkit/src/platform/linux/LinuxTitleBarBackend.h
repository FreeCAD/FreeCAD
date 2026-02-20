// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef LINUXTITLEBARBACKEND_H
#define LINUXTITLEBARBACKEND_H

#include "platform/PlatformTitleBarBackend.h"

class LinuxTitleBarBackend : public PlatformTitleBarBackend {
public:
    void attach(QWidget *window) override;
    void detach() override;
    QSize nativeControlsAreaSize() override;
    void setTitleBarHeight(int height) override;
    bool needsWindowControls() const override;
    QString backendName() const override;
    int minimumTitleBarHeight() const override;
    int snapTitleBarHeight(int requested) const override;
};

#endif // LINUXTITLEBARBACKEND_H
