// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "GenericTitleBarBackend.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>
#include <QWindow>

GenericTitleBarBackend::GenericTitleBarBackend() = default;
GenericTitleBarBackend::~GenericTitleBarBackend()
{
    detach();
}

void GenericTitleBarBackend::attach(QWidget *window)
{
    if (!window) return;
    m_window = window;

    // QMenuBar on macOS always tries to use the native global menu bar.
    // In frameless mode we need in-window menu bars, so disable native menus.
    QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);

    window->setWindowFlags(window->windowFlags()
        | Qt::FramelessWindowHint
        | Qt::Window);

    window->installEventFilter(this);
}

void GenericTitleBarBackend::detach()
{
    if (m_window) {
        m_window->removeEventFilter(this);
        m_window = nullptr;
    }
}

QSize GenericTitleBarBackend::nativeControlsAreaSize()
{
    // No native window controls in frameless mode
    return {0, 0};
}

void GenericTitleBarBackend::setTitleBarHeight(int /*height*/)
{
    // No-op for frameless backend — titlebar height is managed by the widget layout.
}

bool GenericTitleBarBackend::needsWindowControls() const
{
    return true;
}

QString GenericTitleBarBackend::backendName() const
{
    return QStringLiteral("generic");
}

int GenericTitleBarBackend::minimumTitleBarHeight() const
{
    return 0;
}

int GenericTitleBarBackend::snapTitleBarHeight(int requested) const
{
    return requested;
}

Qt::Edges GenericTitleBarBackend::edgesAt(const QPoint &pos) const
{
    if (!m_window) return {};

    Qt::Edges edges;
    if (pos.x() < ResizeBorder)
        edges |= Qt::LeftEdge;
    if (pos.x() >= m_window->width() - ResizeBorder)
        edges |= Qt::RightEdge;
    if (pos.y() < ResizeBorder)
        edges |= Qt::TopEdge;
    if (pos.y() >= m_window->height() - ResizeBorder)
        edges |= Qt::BottomEdge;
    return edges;
}

Qt::CursorShape GenericTitleBarBackend::cursorForEdges(Qt::Edges edges) const
{
    if ((edges & (Qt::TopEdge | Qt::LeftEdge)) == (Qt::TopEdge | Qt::LeftEdge))
        return Qt::SizeFDiagCursor;
    if ((edges & (Qt::TopEdge | Qt::RightEdge)) == (Qt::TopEdge | Qt::RightEdge))
        return Qt::SizeBDiagCursor;
    if ((edges & (Qt::BottomEdge | Qt::LeftEdge)) == (Qt::BottomEdge | Qt::LeftEdge))
        return Qt::SizeBDiagCursor;
    if ((edges & (Qt::BottomEdge | Qt::RightEdge)) == (Qt::BottomEdge | Qt::RightEdge))
        return Qt::SizeFDiagCursor;
    if (edges & (Qt::TopEdge | Qt::BottomEdge))
        return Qt::SizeVerCursor;
    if (edges & (Qt::LeftEdge | Qt::RightEdge))
        return Qt::SizeHorCursor;
    return Qt::ArrowCursor;
}

bool GenericTitleBarBackend::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_window)
        return false;

    switch (event->type()) {
    case QEvent::HoverMove:
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent *>(event);
        Qt::Edges edges = edgesAt(me->position().toPoint());
        if (me->buttons() == Qt::NoButton) {
            // Update cursor for resize edges
            if (edges)
                m_window->setCursor(cursorForEdges(edges));
            else
                m_window->unsetCursor();
        }
        break;
    }

    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton)
            break;

        Qt::Edges edges = edgesAt(me->position().toPoint());
        if (edges) {
            if (auto *wh = m_window->windowHandle()) {
                wh->startSystemResize(edges);
                return true;
            }
        }
        break;
    }

    case QEvent::Leave:
        m_window->unsetCursor();
        break;

    default:
        break;
    }

    return false;
}
