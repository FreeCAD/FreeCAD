// SPDX-FileCopyrightText: 2026 Pierre-Louis Boyer
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "WinTitleBarBackend.h"

#include <QWidget>
#include <QTimer>
#include <QMargins>

#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <dwmapi.h>

// Initialize height to 35 to ensure WM_NCHITTEST works before the first resize event
WinTitleBarBackend::WinTitleBarBackend()
    : m_titleBarHeight(minimumTitleBarHeight())
{}

WinTitleBarBackend::~WinTitleBarBackend() = default;

void WinTitleBarBackend::attach(QWidget* window)
{
    m_window = window;

    // Set frameless window hint to remove native titlebar
    window->setWindowFlags(window->windowFlags() | Qt::FramelessWindowHint | Qt::Window);

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
    // Return {0,0} because the TitleBarWidget manages the Windows control buttons
    // on the right side automatically via its layout. Returning a width here would
    // incorrectly add a macOS-style spacer to the *left* side of the title bar!
    return {0, 0};
}

void WinTitleBarBackend::setTitleBarHeight(int height)
{
    m_titleBarHeight = height;
}

bool WinTitleBarBackend::needsWindowControls() const
{
    // The native titlebar and buttons are hidden by WM_NCCALCSIZE extending
    // the client area. We MUST return true to draw the custom Qt buttons.
    return true;
}

QString WinTitleBarBackend::backendName() const
{
    return QStringLiteral("win");
}

int WinTitleBarBackend::minimumTitleBarHeight() const
{
    return 35;  // Match original FreeCAD implementation
}

int WinTitleBarBackend::snapTitleBarHeight(int requested) const
{
    return requested > 0 ? requested : 35;
}

bool WinTitleBarBackend::handleNativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(eventType);

    if (!m_window) {
        return false;
    }

    MSG* msg = reinterpret_cast<MSG*>(message);

    switch (msg->message) {
        case WM_NCCALCSIZE: {
            if (msg->wParam == TRUE) {
                NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);

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
                if (x >= winrect.left && x < winrect.left + border_width && y < winrect.bottom
                    && y >= winrect.bottom - border_width) {
                    *result = HTBOTTOMLEFT;
                }
                if (x < winrect.right && x >= winrect.right - border_width && y < winrect.bottom
                    && y >= winrect.bottom - border_width) {
                    *result = HTBOTTOMRIGHT;
                }
                if (x >= winrect.left && x < winrect.left + border_width && y >= winrect.top
                    && y < winrect.top + border_width) {
                    *result = HTTOPLEFT;
                }
                if (x < winrect.right && x >= winrect.right - border_width && y >= winrect.top
                    && y < winrect.top + border_width) {
                    *result = HTTOPRIGHT;
                }
            }

            if (0 != *result) {
                return true;
            }

            double dpr = m_window->devicePixelRatioF();
            QPoint pos = m_window->mapFromGlobal(
                QPoint(static_cast<int>(x / dpr), static_cast<int>(y / dpr))
            );

            // Check if we're in the titlebar region
            if (pos.y() >= 0 && pos.y() < m_titleBarHeight) {
                QWidget* child = m_window->childAt(pos);

                // Assume the area is draggable unless we hit an interactive widget
                bool isDragArea = true;
                if (child) {
                    if (child->property("titleBarDragArea").isValid()) {
                        isDragArea = child->property("titleBarDragArea").toBool();
                    }
                    else if (child->inherits("QAbstractButton") || child->inherits("QTabBar")
                             || child->inherits("QMenuBar") || child->inherits("QComboBox")
                             || child->inherits("QSpinBox") || child->inherits("QLineEdit")
                             || child->inherits("QSlider") || child->inherits("QScrollBar")) {
                        isDragArea = false;  // Block drag so the widget can receive mouse clicks
                    }
                }

                if (isDragArea) {
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