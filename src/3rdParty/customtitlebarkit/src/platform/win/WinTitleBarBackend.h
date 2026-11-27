// SPDX-FileCopyrightText: 2026 Pierre-Louis Boyer
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef WINTITLEBARBACKEND_H
#define WINTITLEBARBACKEND_H

#include "platform/PlatformTitleBarBackend.h"

#include <QRect>

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

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /// Follow the window into or out of the maximized state, see the definition
    void updateMaximizedState(void *nativeWindow);
    /// Make the OS maximize or restore the window as Qt asked, see the definition
    void syncMaximizedState();
    /// Give Qt the maximized state that the window has, see the definition
    void adoptNativeMaximizedState();

    QWidget *m_window = nullptr;
    int m_titleBarHeight = 0;
    /// Width of the border left to the OS, -1 before it is set
    int m_frameBorder = -1;
    /// Where the window is when it is neither maximized nor minimized, in the
    /// coordinates of WINDOWPLACEMENT; null before it is known
    QRect m_normalPosition;
};

#endif // WINTITLEBARBACKEND_H
