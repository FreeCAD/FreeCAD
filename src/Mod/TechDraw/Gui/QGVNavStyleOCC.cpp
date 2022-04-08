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
#include <QApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#endif

#include "QGVPage.h"
#include "QGVNavStyleOCC.h"

using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleOCC::QGVNavStyleOCC()
{
}

QGVNavStyleOCC::~QGVNavStyleOCC()
{
}

void QGVNavStyleOCC::handleKeyReleaseEvent(QKeyEvent *event)
{
    //zoom mode 2
    if ( (event->key() == Qt::Key_Control) && zoomingActive) {
        zoomingActive = false;
        event->accept();
    }
}
void QGVNavStyleOCC::handleMousePressEvent(QMouseEvent *event)
{
    //pan mode MMB + mouse movement
    //also Control + MMB + mouse movement, but this is redundant for our purposes
    if (event->button() == Qt::MiddleButton) {
        startPan(event->pos());
        event->accept();
    }

    //zoom mode 2 Control + LMB
    if ((event->button() == Qt::LeftButton) &&
        (QApplication::keyboardModifiers() == Qt::ControlModifier)) {
        startZoom(event->pos());
        event->accept();
    }
}

void QGVNavStyleOCC::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    if (panningActive) {
        pan(event->pos());
        event->accept();
    }

    if (zoomingActive) {
        zoom(mouseZoomFactor(event->pos()));
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
            zoomingActive = false;
            event->accept();
        }
    }
}

}  // namespace TechDrawGui
