/* SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <algorithm>
# ifdef FC_OS_WIN32
#  include <windows.h>
#  include <QTimer>
# endif
#endif

#ifdef FC_OS_WIN32
# include <WinUser.h>
# include <windowsx.h>
# include <dwmapi.h>
# include <objidl.h>  // Fixes error C2504: 'IUnknown' : base class undefined
namespace Gdiplus
{
using std::max;
using std::min;
};  // namespace Gdiplus
# include <gdiplus.h>
#endif

#include <App/Application.h>

#include "FramelessWindow.h"

#include "CustomTitleBar.h"

using namespace Gui;
using namespace std;


FramelessWindow::FramelessWindow(QWidget* parent, Qt::WindowFlags f)
    : QMainWindow(parent, f /*WDestructiveClose*/)
#ifdef EXPERIMENTAL_EDGE_RESIZE
    , m_resizing(false)
    , m_resizeEdge(Qt::Edges())
#endif
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow"
    );
    frameless = hGrp->GetBool("FramelessWindow", false);
    if (frameless) {
        setWindowFrameless();
    }
    else {
        setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
    }
}

void FramelessWindow::setWindowFrameless()
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Window);

    auto* titleBar = new CustomTitleBar(this);
    setMenuWidget(titleBar);

#ifdef FC_OS_WIN32
    QTimer::singleShot(10, this, [this]() {  // see https://bugreports.qt.io/browse/QTBUG-134283
        // this line will get titlebar/thick frame/Aero back, which is exactly what we want
        // we will get rid of titlebar and thick frame again in nativeEvent() later
        HWND hwnd = (HWND)this->winId();
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);

# if _WIN32_WINNT >= 0x0600  // Only enable DWM features on Windows Vista or later
        // we better left 1 pixel width of border untouch, so OS can draw nice shadow around it
        const MARGINS shadow = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea(HWND(winId()), &shadow);
# endif
    });
#endif
}

#ifdef FC_OS_WIN32
# if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
bool FramelessWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
# else
bool FramelessWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
# endif
{
    if (!frameless) {
        return QMainWindow::nativeEvent(eventType, message, result);
    }

    MSG* msg = reinterpret_cast<MSG*>(message);

    switch (msg->message) {
# if _WIN32_WINNT >= 0x0600
        case WM_NCCALCSIZE: {
            if (msg->wParam == TRUE) {
                NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);

                // Fix a visual bug when resizing. Without it ugly white bands are drawn while resizing.
                if (params.rgrc[0].top != 0) {
                    params.rgrc[0].top -= 1;
                }

                *result = WVR_REDRAW;
                return true;
            }
        }
        case WM_NCHITTEST: {
            auto titleBar = menuWidget();
            if (!titleBar) {
                return QMainWindow::nativeEvent(eventType, message, result);
            }

            *result = 0;

            const LONG border_width = 5;
            RECT winrect;
            GetWindowRect(HWND(winId()), &winrect);

            long x = GET_X_LPARAM(msg->lParam);
            long y = GET_Y_LPARAM(msg->lParam);

            bool resizeWidth = minimumWidth() != maximumWidth();
            bool resizeHeight = minimumHeight() != maximumHeight();

            if (resizeWidth) {
                // left border
                if (x >= winrect.left && x < winrect.left + border_width) {
                    *result = HTLEFT;
                }
                // right border
                if (x < winrect.right && x >= winrect.right - border_width) {
                    *result = HTRIGHT;
                }
            }
            if (resizeHeight) {
                // bottom border
                if (y < winrect.bottom && y >= winrect.bottom - border_width) {
                    *result = HTBOTTOM;
                }
                // top border
                if (y >= winrect.top && y < winrect.top + border_width) {
                    *result = HTTOP;
                }
            }
            if (resizeWidth && resizeHeight) {
                // bottom left corner
                if (x >= winrect.left && x < winrect.left + border_width && y < winrect.bottom
                    && y >= winrect.bottom - border_width) {
                    *result = HTBOTTOMLEFT;
                }
                // bottom right corner
                if (x < winrect.right && x >= winrect.right - border_width && y < winrect.bottom
                    && y >= winrect.bottom - border_width) {
                    *result = HTBOTTOMRIGHT;
                }
                // top left corner
                if (x >= winrect.left && x < winrect.left + border_width && y >= winrect.top
                    && y < winrect.top + border_width) {
                    *result = HTTOPLEFT;
                }
                // top right corner
                if (x < winrect.right && x >= winrect.right - border_width && y >= winrect.top
                    && y < winrect.top + border_width) {
                    *result = HTTOPRIGHT;
                }
            }

            if (0 != *result) {
                return true;
            }

            //*result still equals 0, that means the cursor locate OUTSIDE the frame area
            // but it may locate in titlebar area

            // support highdpi
            double dpr = this->devicePixelRatioF();
            QPoint pos = titleBar->mapFromGlobal(QPoint(x / dpr, y / dpr));

            if (!titleBar->rect().contains(pos)) {
                return false;
            }
            QWidget* child = titleBar->childAt(pos);
            if (!child) {
                *result = HTCAPTION;
                return true;
            }
            return false;
        }  // end case WM_NCHITTEST
        case WM_GETMINMAXINFO: {
            if (::IsZoomed(msg->hwnd)) {
                RECT frame = {0, 0, 0, 0};
                AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

                // record frame area data
                double dpr = this->devicePixelRatioF();

                QMargins m_frames;
                m_frames.setLeft(abs(frame.left) / dpr + 0.5);
                m_frames.setTop(abs(frame.bottom) / dpr + 0.5);
                m_frames.setRight(abs(frame.right) / dpr + 0.5);
                m_frames.setBottom(abs(frame.bottom) / dpr + 0.5);

                QMainWindow::setContentsMargins(m_frames);
            }
            else {
                QMainWindow::setContentsMargins(QMargins());
            }
            return false;
        }
# endif
        default:
            return QMainWindow::nativeEvent(eventType, message, result);
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif


#ifdef EXPERIMENTAL_EDGE_RESIZE
void FramelessWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_resizeEdge = calculateEdges(event->pos());
        if (m_resizeEdge != Qt::Edges()) {
            m_resizing = true;
            m_dragStart = event->globalPos();
            m_startGeometry = geometry();
        }
    }
    QMainWindow::mousePressEvent(event);
}

void FramelessWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_resizing) {
        setCursorShape(event->pos());
    }
    else {
        QPoint delta = event->globalPos() - m_dragStart;
        QRect newGeometry = m_startGeometry;

        if (m_resizeEdge & Qt::LeftEdge) {
            newGeometry.setLeft(newGeometry.left() + delta.x());
            if (newGeometry.width() < minimumWidth()) {
                newGeometry.setLeft(newGeometry.right() - minimumWidth());
            }
        }
        if (m_resizeEdge & Qt::RightEdge) {
            newGeometry.setRight(newGeometry.right() + delta.x());
            if (newGeometry.width() < minimumWidth()) {
                newGeometry.setRight(newGeometry.left() + minimumWidth());
            }
        }
        if (m_resizeEdge & Qt::TopEdge) {
            newGeometry.setTop(newGeometry.top() + delta.y());
            if (newGeometry.height() < minimumHeight()) {
                newGeometry.setTop(newGeometry.bottom() - minimumHeight());
            }
        }
        if (m_resizeEdge & Qt::BottomEdge) {
            newGeometry.setBottom(newGeometry.bottom() + delta.y());
            if (newGeometry.height() < minimumHeight()) {
                newGeometry.setBottom(newGeometry.top() + minimumHeight());
            }
        }

        setGeometry(newGeometry);
    }
    QMainWindow::mouseMoveEvent(event);
}

void FramelessWindow::mouseReleaseEvent(QMouseEvent* event)
{
    m_resizing = false;
    m_resizeEdge = Qt::Edges();
    QMainWindow::mouseReleaseEvent(event);
}

Qt::Edges FramelessWindow::calculateEdges(const QPoint& pos)
{
    const int borderWidth = 5;
    Qt::Edges edges;

    bool onTitleBar = false;
    if (QWidget* titleBar = menuWidget()) {
        onTitleBar = titleBar->geometry().contains(pos);
    }

    if (pos.x() <= borderWidth) {
        edges |= Qt::LeftEdge;
    }
    if (pos.x() >= width() - borderWidth) {
        edges |= Qt::RightEdge;
    }
    if (pos.y() >= height() - borderWidth) {
        edges |= Qt::BottomEdge;
    }
    if (pos.y() <= borderWidth && !onTitleBar) {
        edges |= Qt::TopEdge;
    }

    Base::Console().warning("width() - borderWidth %d\n", width() - borderWidth);
    Base::Console().warning("height() - borderWidth %d\n", height() - borderWidth);
    Base::Console().warning("cursor pos %d %d\n", pos.x(), pos.y());
    return edges;
}

void FramelessWindow::setCursorShape(const QPoint& pos)
{
    Qt::Edges edges = calculateEdges(pos);

    if ((edges & Qt::LeftEdge) && (edges & Qt::TopEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    }
    else if ((edges & Qt::RightEdge) && (edges & Qt::BottomEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    }
    else if ((edges & Qt::RightEdge) && (edges & Qt::TopEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    }
    else if ((edges & Qt::LeftEdge) && (edges & Qt::BottomEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    }
    else if (edges & Qt::LeftEdge || edges & Qt::RightEdge) {
        setCursor(Qt::SizeHorCursor);
    }
    else if (edges & Qt::TopEdge || edges & Qt::BottomEdge) {
        setCursor(Qt::SizeVerCursor);
    }
    else {
        unsetCursor();
    }
}
#endif
