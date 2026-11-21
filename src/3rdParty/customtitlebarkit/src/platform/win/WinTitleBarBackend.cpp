// SPDX-FileCopyrightText: 2026 Pierre-Louis Boyer
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "WinTitleBarBackend.h"

#include <QWidget>
#include <QTimer>
#include <QEvent>

#include <algorithm>

#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>

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
    window->installEventFilter(this);

    QTimer::singleShot(10, this, [this]() {
        if (!m_window) {
            return;
        }
        HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);

        // Sets the border left to the OS, for the window as it is shown now
        updateMaximizedState(hwnd);
        syncMaximizedState();
    });
}

namespace
{

/**
 * The client area covers the whole window, to draw the title bar. The OS puts a part of
 * a maximized window outside of the monitor: the thickness of the sizing frame on each
 * side, when the window has that frame while it is maximized. Keep the client area in
 * the work area of the monitor, so that nothing of the content is cut.
 */
void keepClientInWorkArea(RECT& client)
{
    MONITORINFO monitor;
    monitor.cbSize = sizeof(MONITORINFO);
    if (!::GetMonitorInfo(::MonitorFromRect(&client, MONITOR_DEFAULTTONEAREST), &monitor)) {
        return;
    }
    const RECT& work = monitor.rcWork;
    client.left = (std::max)(client.left, work.left);
    client.top = (std::max)(client.top, work.top);
    client.right = (std::min)(client.right, work.right);
    client.bottom = (std::min)(client.bottom, work.bottom);

    // A taskbar that hides itself takes nothing of the work area, and does not come back
    // over a window whose client area covers the monitor: leave it a pixel on its edge
    const auto hasHiddenTaskbar = [&monitor](UINT edge) {
        APPBARDATA data = {};
        data.cbSize = sizeof(APPBARDATA);
        data.uEdge = edge;
        data.rc = monitor.rcMonitor;
        return ::SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &data) != 0;
    };
    if (client.left == monitor.rcMonitor.left && hasHiddenTaskbar(ABE_LEFT)) {
        client.left += 1;
    }
    if (client.top == monitor.rcMonitor.top && hasHiddenTaskbar(ABE_TOP)) {
        client.top += 1;
    }
    if (client.right == monitor.rcMonitor.right && hasHiddenTaskbar(ABE_RIGHT)) {
        client.right -= 1;
    }
    if (client.bottom == monitor.rcMonitor.bottom && hasHiddenTaskbar(ABE_BOTTOM)) {
        client.bottom -= 1;
    }
}

}  // namespace

/**
 * Called when the window was moved, resized, maximized or restored, and is in its new
 * state. The state cannot be read while it changes: during WM_GETMINMAXINFO, IsZoomed()
 * still tells the state that the window is leaving.
 */
void WinTitleBarBackend::updateMaximizedState(void* nativeWindow)
{
    HWND hwnd = static_cast<HWND>(nativeWindow);
    if (::IsIconic(hwnd)) {
        return;  // keep the state to which the window is restored
    }
    const bool maximized = ::IsZoomed(hwnd);

    // Remember where the window is restored to, for syncMaximizedState(). Qt tells its
    // state before it moves the window, so the place it maximizes to is not taken.
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);
    const Qt::WindowStates notNormal = Qt::WindowMaximized | Qt::WindowFullScreen;
    if (!maximized && !(m_window->windowState() & notNormal)
        && ::GetWindowPlacement(hwnd, &placement)) {
        const RECT& normal = placement.rcNormalPosition;
        m_normalPosition = QRect(
            QPoint(normal.left, normal.top),
            QPoint(normal.right - 1, normal.bottom - 1)
        );
    }

    // Leave 1 pixel of border so the OS can draw a shadow. A maximized window has neither
    // border nor shadow.
    const int border = maximized ? 0 : 1;
    if (border != m_frameBorder) {
        m_frameBorder = border;
        const MARGINS frame = {border, border, border, border};
        DwmExtendFrameIntoClientArea(hwnd, &frame);
    }
}

/**
 * Qt does not make the OS maximize a frameless window that is shown: it moves the window
 * to the work area of the monitor, and back. For the OS the window is then a normal one
 * that happens to fill the screen: it keeps its border and its rounded corners, it can be
 * dragged away without being restored, and the OS and Qt each have their own idea of the
 * state once the user maximizes or restores by other means (double-click on the title
 * bar, snapping, keyboard). So once Qt changed the state, make the OS agree.
 */
void WinTitleBarBackend::syncMaximizedState()
{
    const Qt::WindowStates state = m_window->windowState();
    if (!m_window->isVisible() || (state & (Qt::WindowMinimized | Qt::WindowFullScreen))) {
        return;
    }
    HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    const bool maximize = state.testFlag(Qt::WindowMaximized);
    if (maximize == (::IsZoomed(hwnd) != 0)) {
        return;
    }
    if (!maximize) {
        ::ShowWindow(hwnd, SW_RESTORE);
        return;
    }
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);
    if (!::GetWindowPlacement(hwnd, &placement)) {
        return;
    }
    if (!m_normalPosition.isNull()) {
        // The window is where Qt moved it: restore to where it was before
        placement.rcNormalPosition = {
            m_normalPosition.left(),
            m_normalPosition.top(),
            m_normalPosition.right() + 1,
            m_normalPosition.bottom() + 1,
        };
    }
    placement.showCmd = SW_SHOWMAXIMIZED;
    ::SetWindowPlacement(hwnd, &placement);
}

/**
 * When Qt restores a maximized window by moving it, the OS tells the new size of a
 * window that is still maximized, before syncMaximizedState() restores it for real.
 * Qt takes this as the user maximizing the window, and keeps that state. So when the OS
 * told a state, take the one that the window has by now.
 */
void WinTitleBarBackend::adoptNativeMaximizedState()
{
    const Qt::WindowStates state = m_window->windowState();
    if (!m_window->isVisible() || (state & (Qt::WindowMinimized | Qt::WindowFullScreen))) {
        return;
    }
    const bool maximized = ::IsZoomed(reinterpret_cast<HWND>(m_window->winId())) != 0;
    if (state.testFlag(Qt::WindowMaximized) != maximized) {
        m_window->setWindowState(state ^ Qt::WindowMaximized);
    }
}

bool WinTitleBarBackend::eventFilter(QObject* watched, QEvent* event)
{
    if (m_window && watched == m_window && event->type() == QEvent::WindowStateChange) {
        // A change that comes from the OS is spontaneous, one that Qt made is not
        if (event->spontaneous()) {
            adoptNativeMaximizedState();
        }
        else {
            syncMaximizedState();
        }
    }
    return PlatformTitleBarBackend::eventFilter(watched, event);
}

void WinTitleBarBackend::detach()
{
    if (m_window) {
        m_window->removeEventFilter(this);
    }
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

                // In: where the window will be. Out: its client area
                RECT& client = params.rgrc[0];
                if (::IsZoomed(msg->hwnd)) {
                    keepClientInWorkArea(client);
                }
                else if (client.top != 0) {
                    // Fix a visual bug when resizing: without this, ugly white bands appear
                    client.top -= 1;
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

            // A maximized window has no border to resize it, also when its edges are on
            // the screen
            const bool sizable = !::IsZoomed(msg->hwnd);
            bool resizeWidth = sizable && m_window->minimumWidth() != m_window->maximumWidth();
            bool resizeHeight = sizable && m_window->minimumHeight() != m_window->maximumHeight();

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

            // x and y are physical screen coordinates. Dividing them by the
            // device pixel ratio only gives Qt's logical coordinates on a screen
            // whose origin is (0,0): on another monitor with scaling the point
            // lands elsewhere. Convert to physical client coordinates first:
            // those scale to the window's logical coordinates by the ratio alone.
            POINT clientPoint = {x, y};
            ::ScreenToClient(reinterpret_cast<HWND>(m_window->winId()), &clientPoint);
            double dpr = m_window->devicePixelRatioF();
            QPoint pos(
                static_cast<int>(clientPoint.x / dpr),
                static_cast<int>(clientPoint.y / dpr)
            );

            // Check if we're in the titlebar region
            if (pos.y() >= 0 && pos.y() < m_titleBarHeight) {
                QWidget* child = m_window->childAt(pos);

                // Assume the area is draggable unless we hit an interactive widget
                bool isDragArea = true;
                for (auto *widget = child; widget; widget = widget->parentWidget()) {
                    if (widget->property("titleBarDragArea").isValid()) {
                        isDragArea = widget->property("titleBarDragArea").toBool();
                        break;
                    }
                    if (widget->inherits("QAbstractButton") || widget->inherits("QTabBar")
                        || widget->inherits("QMenuBar") || widget->inherits("QComboBox")
                        || widget->inherits("QSpinBox") || widget->inherits("QLineEdit")
                        || widget->inherits("QSlider") || widget->inherits("QScrollBar")
                        || widget->inherits("QToolBar")) {
                        isDragArea = false;  // Block drag so the widget can receive mouse clicks
                        break;
                    }
                }

                if (isDragArea) {
                    *result = HTCAPTION;
                    return true;
                }
            }
            return false;
        }

        case WM_WINDOWPOSCHANGED: {
            // The window was moved, resized, maximized or restored, and is in its new state
            updateMaximizedState(msg->hwnd);
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
