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

#include "QGVNavStyleMaya.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleMaya::QGVNavStyleMaya(QGVPage *qgvp) :
    QGVNavStyle(qgvp)
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
    Q_UNUSED(event)
}

void QGVNavStyleMaya::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    //pan mode alt + MMB + mouse movement
    if (QGuiApplication::mouseButtons() & Qt::MiddleButton &&
        QApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    }

    //zoom mode 2 ALT + RMB
    if (QGuiApplication::mouseButtons() & Qt::RightButton &&
        QApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
        if (zoomingActive) {
            zoom(mouseZoomFactor(event->pos()));
        } else {
            startZoom(event->pos());
        }
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

bool QGVNavStyleMaya::allowContextMenu(QContextMenuEvent *event)
{
//    Base::Console().Message("QGVNSM::allowContextMenu()\n");
    if (event->reason() == QContextMenuEvent::Mouse) {
        //must check for a button combination involving context menu button
        if (QApplication::keyboardModifiers() == Qt::AltModifier) {
            //Alt is down, so this is Alt + RMB - don't allow context menu
            return false;
        }
    }
    return true;
}
}  // namespace TechDrawGui
