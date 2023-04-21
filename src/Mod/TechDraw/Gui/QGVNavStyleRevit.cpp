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

#include "QGVNavStyleRevit.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleRevit::QGVNavStyleRevit(QGVPage *qgvp) :
    QGVNavStyle(qgvp)
{
}

QGVNavStyleRevit::~QGVNavStyleRevit()
{
}

void QGVNavStyleRevit::handleMousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        startClick(Qt::RightButton);
    }
}

void QGVNavStyleRevit::handleMouseMoveEvent(QMouseEvent *event)
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
        } else {
            startPan(event->pos());
        }
        event->accept();
    }

    //pan mode 2 - LMB + RMB + move
    if (QGuiApplication::mouseButtons() & Qt::LeftButton &&
        QGuiApplication::mouseButtons() & Qt::RightButton) {
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    }
}

void QGVNavStyleRevit::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if ((event->button() == Qt::RightButton) &&
         m_clickPending &&
        (m_clickButton == Qt::RightButton)) {
        stopClick();
        pseudoContextEvent();
        event->accept();
        return;
    }

    //stop panning if any button released
    if ( (event->button() == Qt::LeftButton) ||
         (event->button() == Qt::RightButton) ||
         (event->button() == Qt::MiddleButton) ){
        if (panningActive) {
            stopPan();
            event->accept();
        }
    }
}

bool QGVNavStyleRevit::allowContextMenu(QContextMenuEvent *event)
{
//    Base::Console().Message("QGVNSRevit::allowContextMenu()\n");
    if (event->reason() == QContextMenuEvent::Mouse) {
        //must check for a button combination involving context menu button
        if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
            //LMB down - don't allow context menu
            return false;
        } else if (m_clickPending) {
            //context menu request to be handled by button release
            return false;
        }
    }
    return true;
}
}  // namespace TechDrawGui
