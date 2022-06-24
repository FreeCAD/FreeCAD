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
#include "QGVNavStyleRevit.h"

using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleRevit::QGVNavStyleRevit()
{
}

QGVNavStyleRevit::~QGVNavStyleRevit()
{
}

void QGVNavStyleRevit::handleMousePressEvent(QMouseEvent *event)
{
    //pan mode 1 - LMB + RMB
    if (event->buttons() == (Qt::LeftButton & Qt::RightButton)) {
        startPan(event->pos());
        event->accept();
    } 
    
    //pan mode 2 - MMB
    if (event->button() == Qt::MiddleButton) {
        startPan(event->pos());
        event->accept();
    }
}

void QGVNavStyleRevit::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    if (panningActive) {
        pan(event->pos());
        event->accept();
    }
}

void QGVNavStyleRevit::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (panningActive) {
        //stop panning if any button release
        if ( (event->button() == Qt::LeftButton) ||
             (event->button() == Qt::RightButton) ||
             (event->button() == Qt::MiddleButton) ){
            stopPan();
            event->accept();
        }
    }
}

}  // namespace TechDrawGui
