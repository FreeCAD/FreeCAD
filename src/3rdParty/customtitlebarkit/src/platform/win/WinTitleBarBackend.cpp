// SPDX-FileCopyrightText: 2026 Pierre-Louis Boyer
// SPDX-License-Identifier: LGPL-2.1-or-later

// Based on FramelessWindow implementation by Pierre-Louis Boyer (FreeCAD PR #26766)
// Adapted for customtitlebarkit by using DWM to extend client area while preserving
// native window controls (min/max/close buttons).

#include "WinTitleBarBackend.h"

#include <QWidget>
#include <QTimer>
#include <QMargins>

#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <dwmapi.h>

WinTitleBarBackend::WinTitleBarBackend() = default;
WinTitleBarBackend::~WinTitleBarBackend() = default;

void WinTitleBarBackend::attach(QWidget *window)
{
    m_window = window;

    // Set frameless window hint to remove native titlebar
    window->setWindowFlags(window->windowFlags() | Qt::FramelessWindowHint | Qt::Window);

    // After a short delay, restore key window styles so we get native controls and
    // Aero features back. The FramelessWindowHint removes them, but we want DWM
    // snap/maximize/shadow to work. See https://bugreports.qt.io/browse/QTBUG-134283
    QTimer::singleShot(10, window, [window]() {
        HWND hwnd = reinterpret_cast<HWND>(window->winId());
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);

        // Leave 1 pixel of border so the OS can draw a shadow
        const MARGINS shadow = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea(hwnd, &shadow);
    });
}

void WinTitleBarBackend::detach()
{
    m_window = nullptr;
}

QSize WinTitleBarBackend::nativeControlsAreaSize()
{
    // Estimate the size of Windows caption buttons (min/max/close).
    // Each button is typically 46px wide, 3 buttons = 138px.
    // Height matches the titlebar.
    int h = m_titleBarHeight > 0 ? m_titleBarHeight : 32;
    return {138, h};
}

void WinTitleBarBackend::setTitleBarHeight(int height)
{
    m_titleBarHeight = height;
}

bool WinTitleBarBackend::needsWindowControls() const
{
    // DWM preserves native min/max/close buttons, so we don't need custom ones
    return false;
}

QString WinTitleBarBackend::backendName() const
{
    return QStringLiteral("win");
}

int WinTitleBarBackend::minimumTitleBarHeight() const
{
    return 0;
}

int WinTitleBarBackend::snapTitleBarHeight(int requested) const
{
    return requested;
}

bool WinTitleBarBackend::handleNativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);

    if (!m_window) {
        return false;
    }

    MSG *msg = reinterpret_cast<MSG *>(message);

    switch (msg->message) {
    case WM_NCCALCSIZE: {
        if (msg->wParam == TRUE) {
            NCCALCSIZE_PARAMS &params = *reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);

            // Fix a visual bug when resizing: without this, ugly white bands appear
            if (params.rgrc[0].top != 0) {
                params.rgrc[0].top -= 1;
            }

            *result = WVR_REDRAW;
            return true;
        }
        return false;
    }

    case WM_NCHITTEST: {
        *result = 0;

        const LONG border_width = 5;
        RECT winrect;
        GetWindowRect(reinterpret_cast<HWND>(m_window->winId()), &winrect);

        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        bool resizeWidth = m_window->minimumWidth() != m_window->maximumWidth();
        bool resizeHeight = m_window->minimumHeight() != m_window->maximumHeight();

        if (resizeWidth) {
            if (x >= winrect.left && x < winrect.left + border_width) {
                *result = HTLEFT;
            }
            if (x < winrect.right && x >= winrect.right - border_width) {
                *result = HTRIGHT;
            }
        }
        if (resizeHeight) {
            if (y < winrect.bottom && y >= winrect.bottom - border_width) {
                *result = HTBOTTOM;
            }
            if (y >= winrect.top && y < winrect.top + border_width) {
                *result = HTTOP;
            }
        }
        if (resizeWidth && resizeHeight) {
            if (x >= winrect.left && x < winrect.left + border_width
                && y < winrect.bottom && y >= winrect.bottom - border_width) {
                *result = HTBOTTOMLEFT;
            }
            if (x < winrect.right && x >= winrect.right - border_width
                && y < winrect.bottom && y >= winrect.bottom - border_width) {
                *result = HTBOTTOMRIGHT;
            }
            if (x >= winrect.left && x < winrect.left + border_width
                && y >= winrect.top && y < winrect.top + border_width) {
                *result = HTTOPLEFT;
            }
            if (x < winrect.right && x >= winrect.right - border_width
                && y >= winrect.top && y < winrect.top + border_width) {
                *result = HTTOPRIGHT;
            }
        }

        if (0 != *result) {
            return true;
        }

        // Cursor is outside the resize frame area but may be in the titlebar
        double dpr = m_window->devicePixelRatioF();
        QPoint pos = m_window->mapFromGlobal(QPoint(static_cast<int>(x / dpr),
                                                      static_cast<int>(y / dpr)));

        // Check if we're in the titlebar region (top portion of the window)
        if (pos.y() >= 0 && pos.y() < m_titleBarHeight) {
            QWidget *child = m_window->childAt(pos);
            if (!child || child->property("titleBarDragArea").toBool()) {
                *result = HTCAPTION;
                return true;
            }
        }
        return false;
    }

    case WM_GETMINMAXINFO: {
        if (::IsZoomed(msg->hwnd)) {
            RECT frame = {0, 0, 0, 0};
            AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

            double dpr = m_window->devicePixelRatioF();

            QMargins margins;
            margins.setLeft(static_cast<int>(abs(frame.left) / dpr + 0.5));
            margins.setTop(static_cast<int>(abs(frame.bottom) / dpr + 0.5));
            margins.setRight(static_cast<int>(abs(frame.right) / dpr + 0.5));
            margins.setBottom(static_cast<int>(abs(frame.bottom) / dpr + 0.5));

            m_window->setContentsMargins(margins);
        }
        else {
            m_window->setContentsMargins(QMargins());
        }
        return false;
    }

    default:
        return false;
    }
}

std::unique_ptr<PlatformTitleBarBackend> PlatformTitleBarBackend::createPlatform()
{
    return std::make_unique<WinTitleBarBackend>();
}
