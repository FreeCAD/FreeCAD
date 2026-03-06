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


#ifndef GUI_FRAMELESSWINDOW_H
#define GUI_FRAMELESSWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>

#include "FCGlobal.h"

namespace Gui
{

class GuiExport FramelessWindow: public QMainWindow
{
    Q_OBJECT

public:
    explicit FramelessWindow(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::Window);

    void setWindowFrameless();

    bool isFrameless()
    {
        return frameless;
    }

#ifdef FC_OS_WIN32
protected:
# if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray& eventType, void* message, long* result);
# else
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result);
# endif
#endif

#ifdef EXPERIMENTAL_EDGE_RESIZE
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    Qt::Edges calculateEdges(const QPoint& pos);
    void setCursorShape(const QPoint& pos);

    bool frameless;
    bool m_resizing;
    Qt::Edges m_resizeEdge;
    QPoint m_dragStart;
    QRect m_startGeometry;
#endif

private:
    bool frameless;
};

}  // namespace Gui

#endif  // GUI_FRAMELESSWINDOW_H
