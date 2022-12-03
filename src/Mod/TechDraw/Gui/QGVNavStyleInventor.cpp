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

#include "QGVNavStyleInventor.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

//**********
// Issue: select should be Shift + LMB
//        currently is just LMB like all the other styles
//        need to just accept LMB w/o Shift and pass Shift + LMB to
//        QGraphicsView?
//**********

QGVNavStyleInventor::QGVNavStyleInventor(QGVPage *qgvp) :
    QGVNavStyle(qgvp)
{
}

QGVNavStyleInventor::~QGVNavStyleInventor()
{
}

void QGVNavStyleInventor::handleMousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

void QGVNavStyleInventor::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    if ((QGuiApplication::mouseButtons() & Qt::LeftButton) &&
        (QGuiApplication::mouseButtons() & Qt::MiddleButton)) {
        //zoom mode 2 - LMB + MMB
        if (zoomingActive) {
            zoom(mouseZoomFactor(event->pos()));
        } else {
            startZoom(event->pos());
        }
        event->accept();
    } else if (QGuiApplication::mouseButtons() & Qt::MiddleButton)  {
        //pan mode - MMB + move
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    }
}

void QGVNavStyleInventor::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (event->button() == Qt::MiddleButton) {
        //pan mode MMB
        if (panningActive) {
            stopPan();
            event->accept();
        }
    }

    if ((event->button() == Qt::LeftButton) ||
        (event->button() == Qt::MiddleButton) ){
        //zoom mode 2 LMB + MMB
        if (zoomingActive) {
            zoomingActive = false;
            event->accept();
        }
    }
}

}  // namespace TechDrawGui
