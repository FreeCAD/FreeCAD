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

#include "QGVNavStyleBlender.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleBlender::QGVNavStyleBlender(QGVPage* qgvp) :
    QGVNavStyle(qgvp)
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
    Q_UNUSED(event)
//    Base::Console().Message("QGVNSBlender::handleMousePressEvent() - button: %d buttons: %d\n", event->button(), event->buttons());
}

void QGVNavStyleBlender::handleMouseMoveEvent(QMouseEvent *event)
{
//    Base::Console().Message("QGVNSBlender::handleMouseMoveEvent() - buttons: %d modifiers: %X\n",
//                            QGuiApplication::mouseButtons() & Qt::MiddleButton,
//                            QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier));

    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    if ((QGuiApplication::mouseButtons() & Qt::LeftButton) &&
        (QGuiApplication::mouseButtons() & Qt::RightButton)) {
        //pan mode 1 - LMB + RMB
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    } else if ((QGuiApplication::mouseButtons() & Qt::MiddleButton) &&
               (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) ) {
        //pan mode 2 - Shift + MMB
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    }
}

void QGVNavStyleBlender::handleMouseReleaseEvent(QMouseEvent *event)
{
//    Base::Console().Message("QGVNSBlender::handleMouseReleaseEvent() - button: %d buttons: %d\n", event->button(), event->buttons());
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (panningActive) {
        //pan mode 1 - LMB + RMB + mouse move
        //stop panning if either button released
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

bool QGVNavStyleBlender::allowContextMenu(QContextMenuEvent *event)
{
//    Base::Console().Message("QGVNSBlender::allowContextMenu()\n");
    if (event->reason() == QContextMenuEvent::Mouse) {
        //must check for a button combination involving context menu button
        if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
            //LeftButton is down, so this is LMB + RMB - don't allow context menu
            return false;
        }
    }
    return true;
}

}  // namespace TechDrawGui
