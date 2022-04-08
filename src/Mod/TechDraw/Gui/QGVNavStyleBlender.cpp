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
#include "QGVNavStyleBlender.h"

using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleBlender::QGVNavStyleBlender()
{
}

QGVNavStyleBlender::~QGVNavStyleBlender()
{
}

void QGVNavStyleBlender::handleKeyReleaseEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Shift) && panningActive) {
        stopPan();
        event->accept();
    }
}

void QGVNavStyleBlender::handleMousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == (Qt::LeftButton | Qt::RightButton)) {
        //pan mode 1 - LMB + RMB + mouse move
        startPan(event->pos());
        event->accept();
    } else if ((event->button() == Qt::MiddleButton) &&
                QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
        //pan mode 2 - Shift + MMB
        startPan(event->pos());
        event->accept();
    }
}

void QGVNavStyleBlender::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    if (panningActive) {
        pan(event->pos());
        event->accept();
    }
}

void QGVNavStyleBlender::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (panningActive) {
        //pan mode 1 - LMB + RMB + mouse move
        //stop panning if either button release
        if ( (event->button() == Qt::LeftButton) ||
             (event->button() == Qt::RightButton)) {
            stopPan();
            event->accept();
        }
        //pan mode 2 - Shift + MMB
        //stop panning if MMB released
        if (event->button() == Qt::MiddleButton) {
            stopPan();
            event->accept();
        }
    }
}
}  // namespace TechDrawGui
