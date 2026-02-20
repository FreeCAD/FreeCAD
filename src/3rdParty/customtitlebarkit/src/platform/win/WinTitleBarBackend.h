// SPDX-FileCopyrightText: 2026 Pierre-Louis Boyer
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef WINTITLEBARBACKEND_H
#define WINTITLEBARBACKEND_H

#include "platform/PlatformTitleBarBackend.h"

class QWidget;

class WinTitleBarBackend : public PlatformTitleBarBackend {
    Q_OBJECT

public:
    WinTitleBarBackend();
    ~WinTitleBarBackend() override;

    void attach(QWidget *window) override;
    void detach() override;
    QSize nativeControlsAreaSize() override;
    void setTitleBarHeight(int height) override;
    bool needsWindowControls() const override;
    QString backendName() const override;
    int minimumTitleBarHeight() const override;
    int snapTitleBarHeight(int requested) const override;
    bool handleNativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    QWidget *m_window = nullptr;
    int m_titleBarHeight = 0;
};

#endif // WINTITLEBARBACKEND_H
