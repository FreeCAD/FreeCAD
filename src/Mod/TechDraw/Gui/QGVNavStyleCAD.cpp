/***************************************************************************
 *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QApplication>
# include <QGuiApplication>
# include <QMouseEvent>
#endif

#include "QGVNavStyleCAD.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleCAD::QGVNavStyleCAD(QGVPage* qgvp) :
    QGVNavStyle(qgvp)
{
}

QGVNavStyleCAD::~QGVNavStyleCAD()
{
}

void QGVNavStyleCAD::handleKeyReleaseEvent(QKeyEvent *event)
{
    //zoom mode 2
    if ( ((event->key() == Qt::Key_Control) ||
          (event->key() == Qt::Key_Shift)) && zoomingActive) {
        stopZoom();
        event->accept();
    }

    //pan mode 2
    if ((event->key() == Qt::Key_Control) && panningActive) {
        stopPan();
        event->accept();
    }
}

void QGVNavStyleCAD::handleMousePressEvent(QMouseEvent *event)
{
//    Base::Console().Message("QGVNSCAD::handleMousePressEvent() - button: %d\n", event->button());
    if (event->button() == Qt::MiddleButton) {
        startClick(Qt::MiddleButton);   //for MMB center view
    }

    //zoom mode 2 Control + Shift + RMB click
    if ((event->button() == Qt::RightButton) &&
        QApplication::keyboardModifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        startClick(Qt::RightButton);
    }

    //pan mode 2 Control + RMB click
    if ((event->button() == Qt::RightButton) &&
        QApplication::keyboardModifiers() == Qt::ControlModifier) {
        startClick(Qt::RightButton);
    }
}

void QGVNavStyleCAD::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    //if the mouse moves between press and release, then it isn't a click
    if (m_clickPending) {
        stopClick();
        return;
    }

    //pan mode 1 - MMB + move
    if (QGuiApplication::mouseButtons() & Qt::MiddleButton) {
        if (panningActive) {
            pan(event->pos());
            event->accept();
        } else {
            startPan(event->pos());
            event->accept();
        }
    }

    //pan mode 2 - CNTL + RMB click + move
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
        panningActive) {
        pan(event->pos());
        event->accept();
    } else if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
        m_panPending) {
        startPan(event->pos());
        event->accept();
    }

    //zoom mode 2 - CNTL + SHIFT + RMB click + move
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
        QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) &&
        zoomingActive) {
        zoom(mouseZoomFactor(event->pos()));
        event->accept();
    } else if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
        QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) &&
        m_zoomPending) {
        startZoom(event->pos());
        event->accept();
    }
}

void QGVNavStyleCAD::handleMouseReleaseEvent(QMouseEvent *event)
{
//    Base::Console().Message("QGVNSCAD::handleMouseReleaseEvent() - button: %d\n", event->button());
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (event->button() == Qt::MiddleButton) {
        if (m_clickPending && (m_clickButton == Qt::MiddleButton)) {
            stopClick();
            getViewer()->centerOn(getViewer()->mapToScene(event->pos()));
            event->accept();
        }
        //pan mode 1 hold MMB + mouse move
        if (panningActive) {
            stopPan();
            event->accept();
        }
    }

    //zoom mode 2 Control + Shift + RMB click
    if ((event->button() == Qt::RightButton) &&
         QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
         QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) &&
         m_clickPending &&
        (m_clickButton == Qt::RightButton)) {
        stopClick();
        m_zoomPending = true;
        event->accept();
        return;
    }

    //pan mode 2 starts with Control + RMB click
    if ((event->button() == Qt::RightButton) &&
         QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
         m_clickPending &&
        (m_clickButton == Qt::RightButton)) {
        stopClick();
        m_panPending = true;
        event->accept();
    }
}

bool QGVNavStyleCAD::allowContextMenu(QContextMenuEvent *event)
{
//    Base::Console().Message("QGVNSCAD::allowContextMenu()\n");
    if (event->reason() == QContextMenuEvent::Mouse) {
        //must check for a button combination involving context menu button
        if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ||
            (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
             QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) ) {
            //CNTL or CNTL+Shift down - don't allow context menu
            return false;
        }
    }
    return true;
}

}  // namespace TechDrawGui
