// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef MACTITLEBARBACKEND_H
#define MACTITLEBARBACKEND_H

#include "platform/PlatformTitleBarBackend.h"

class MacTitleBarBackend : public PlatformTitleBarBackend {
    Q_OBJECT
public:
    MacTitleBarBackend();
    ~MacTitleBarBackend() override;

    void attach(QWidget *window) override;
    void detach() override;
    QSize nativeControlsAreaSize() override;
    void setTitleBarHeight(int height) override;
    bool needsWindowControls() const override;
    QString backendName() const override;
    int minimumTitleBarHeight() const override;
    int snapTitleBarHeight(int requested) const override;

    bool isFullscreen() const override;

    MenuIntegration *createDefaultMenuIntegration(QObject *parent) override;

private:
    void setNativeControlsHidden(bool hidden);
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // MACTITLEBARBACKEND_H
