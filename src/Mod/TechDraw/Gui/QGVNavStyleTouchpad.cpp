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

#include "QGVNavStyleTouchpad.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleTouchpad::QGVNavStyleTouchpad(QGVPage *qgvp) :
    QGVNavStyle(qgvp)
{
}

QGVNavStyleTouchpad::~QGVNavStyleTouchpad()
{
}

void QGVNavStyleTouchpad::handleKeyPressEvent(QKeyEvent *event)
{
//    Q_UNUSED(event)
    if (event->key() == Qt::Key_PageUp) {
        zoomIn();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_PageDown) {
        zoomOut();
        event->accept();
        return;
    }

    QGVNavStyle::handleKeyPressEvent(event);
}

void QGVNavStyleTouchpad::handleKeyReleaseEvent(QKeyEvent *event)
{
//    Q_UNUSED(event)
    if (event->key() == Qt::Key_Shift) {
        if (panningActive) {
            stopPan();
        }
        if (zoomingActive) {
            stopZoom();
        }
        event->accept();
    }

    if (event->key() == Qt::Key_Control) {
        stopZoom();
        event->accept();
    }

}

void QGVNavStyleTouchpad::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    if (QApplication::keyboardModifiers() == Qt::ShiftModifier) {
        //if shift is down then we are panning
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
        return;
    }

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
        QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) ) {
        //if control and shift are down, then we are zooming
        if (zoomingActive) {
            zoom(mouseZoomFactor(event->pos()));
        } else {
            startZoom(event->pos());
        }
        event->accept();
        return;
    }

    // if the mouse moves, but we are not zooming or panning, then we should make
    // sure that zoom and pan are turned off.
    stopPan();
    stopZoom();
}

void QGVNavStyleTouchpad::setAnchor()
{
    //this navigation style can not anchor under mouse since mouse is moving as part of zoom action
    if (m_viewer) {
        m_viewer->setResizeAnchor(QGraphicsView::AnchorViewCenter);
        m_viewer->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    }
}

}  //namespace TechDrawGui
