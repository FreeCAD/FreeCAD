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
#include "QGVNavStyleMaya.h"

using namespace TechDrawGui;

namespace TechDrawGui {

//******
// Issue: on linux mint, ALT + mouse button doesn't get to the application.
//        system eats the event for menus, accessibility magnifier, etc.
//        only wheel zoom is known to work.
//        need to test in different environment.
//******

QGVNavStyleMaya::QGVNavStyleMaya()
{
}

QGVNavStyleMaya::~QGVNavStyleMaya()
{
}

void QGVNavStyleMaya::handleKeyReleaseEvent(QKeyEvent *event)
{
    //zoom mode 2
    if ( (event->key() == Qt::Key_Alt) && zoomingActive) {
        zoomingActive = false;
        event->accept();
    }

    //pan mode
    if ((event->key() == Qt::Key_Alt) && panningActive) {
        stopPan();
        event->accept();
    }
}
void QGVNavStyleMaya::handleMousePressEvent(QMouseEvent *event)
{
    //pan mode alt + MMB + mouse movement
    if ((event->button() == Qt::MiddleButton) && 
        (QApplication::keyboardModifiers() == Qt::AltModifier)) {
        startPan(event->pos());
        event->accept();
    }

    //zoom mode 2 ALT + RMB
    if ((event->button() == Qt::RightButton) &&
        (QApplication::keyboardModifiers() == Qt::AltModifier)) {
        startZoom(event->pos());
        event->accept();
    }
}

void QGVNavStyleMaya::handleMouseMoveEvent(QMouseEvent *event)
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
        event->accept();
    }
}

void QGVNavStyleMaya::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (event->button() == Qt::MiddleButton) {
        //pan mode ALT + MMB
        if (panningActive) {
            stopPan();
            event->accept();
        }
    }

    if (event->button() == Qt::RightButton) {
        //zoom mode 2 ALT + RMB
        if (zoomingActive) {
            zoomingActive = false;
            event->accept();
        }
    }
}

}  // namespace TechDrawGui
