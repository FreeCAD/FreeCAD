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
# include <QGuiApplication>
# include <QMouseEvent>
#endif

#include "QGVNavStyleOCC.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleOCC::QGVNavStyleOCC(QGVPage *qgvp) :
    QGVNavStyle(qgvp)
{
}

QGVNavStyleOCC::~QGVNavStyleOCC()
{
}

void QGVNavStyleOCC::handleKeyReleaseEvent(QKeyEvent *event)
{
    //zoom mode 2
    if ( (event->key() == Qt::Key_Control) && zoomingActive) {
        stopZoom();
        event->accept();
    }

    //pan mode
    if ( (event->key() == Qt::Key_Control) && panningActive) {
        stopPan();
        event->accept();
    }
}

void QGVNavStyleOCC::handleMousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

void QGVNavStyleOCC::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    //pan mode 1 - MMB + mouse movement
    if (QGuiApplication::mouseButtons() & Qt::MiddleButton) {
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    }

    //pan mode 2 - CNTL + MMB + mouse movement
    if (QGuiApplication::mouseButtons() & Qt::MiddleButton &&
        QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ) {
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    }

    //zoom mode 2 Control + LMB
    if ((QGuiApplication::mouseButtons() & Qt::LeftButton) &&
         QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ) {
        if (zoomingActive) {
            zoom(mouseZoomFactor(event->pos()));
        } else {
            startZoom(event->pos());
        }
        event->accept();
    }
}

void QGVNavStyleOCC::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (event->button() == Qt::MiddleButton) {
        //pan mode [Control] + MMB
        if (panningActive) {
            stopPan();
            event->accept();
        }
    }

    if (event->button() == Qt::LeftButton) {
        //zoom mode 2 Control + LMB
        if (zoomingActive) {
            stopZoom();
            event->accept();
        }
    }
}

bool QGVNavStyleOCC::allowContextMenu(QContextMenuEvent *event)
{
//    Base::Console().Message("QGVNSOCC::allowContextMenu()\n");
    if (event->reason() == QContextMenuEvent::Mouse) {
        //must check for a button combination involving context menu button
        if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ) {
            //CNTL is down, so this is CNTL + RMB - don't allow context menu
            return false;
        }
    }
    return true;
}


}  // namespace TechDrawGui
