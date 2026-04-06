/***************************************************************************
 *   Copyright (c) 2026 Max Wilfinger                                      *
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

// Onshape navigation style for TechDraw:
// Left mouse:              Select
// Middle mouse drag:       Pan
// Right mouse drag:        Pan
// Ctrl + right mouse drag: Pan
// Scroll wheel:            Zoom in/out

#include <QGuiApplication>
#include <QMouseEvent>

#include "QGVNavStyleOnshape.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleOnshape::QGVNavStyleOnshape(QGVPage* qgvp) :
    QGVNavStyle(qgvp)
{
}

QGVNavStyleOnshape::~QGVNavStyleOnshape()
{
}

void QGVNavStyleOnshape::handleMousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
}

void QGVNavStyleOnshape::handleMouseMoveEvent(QMouseEvent* event)
{
    if (getViewer()->isBalloonPlacing()) {
        balloonCursorMovement(event);
        return;
    }

    if (QGuiApplication::mouseButtons() & Qt::MiddleButton) {
        // Middle mouse drag: Pan
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    } else if (QGuiApplication::mouseButtons() & Qt::RightButton) {
        // Right mouse drag (with or without Ctrl): Pan
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    }
}

void QGVNavStyleOnshape::handleMouseReleaseEvent(QMouseEvent* event)
{
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (panningActive) {
        if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
            stopPan();
            event->accept();
        }
    }
}

}  // namespace TechDrawGui
