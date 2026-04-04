// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Max Wilfinger
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

# include <QGuiApplication>
# include <QMouseEvent>

#include "QGVNavStyleFusion.h"
#include "QGVPage.h"


using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyleFusion::QGVNavStyleFusion(QGVPage* qgvp) :
    QGVNavStyle(qgvp)
{
}

QGVNavStyleFusion::~QGVNavStyleFusion()
{
}

void QGVNavStyleFusion::handleKeyReleaseEvent(QKeyEvent *event)
{
    // zoom mode - stop zooming when Ctrl is released
    if ((event->key() == Qt::Key_Control) && zoomingActive) {
        stopZoom();
        event->accept();
    }
}

void QGVNavStyleFusion::handleMousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

void QGVNavStyleFusion::handleMouseMoveEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        balloonCursorMovement(event);
        return;
    }

    if ((QGuiApplication::mouseButtons() & Qt::MiddleButton) &&
        QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        // zoom mode - Ctrl + MMB
        if (zoomingActive) {
            zoom(mouseZoomFactor(event->pos()));
        } else {
            startZoom(event->pos());
        }
        event->accept();
    } else if (QGuiApplication::mouseButtons() & Qt::MiddleButton) {
        // pan mode - MMB (covers both plain MMB and Shift + MMB,
        // since there is no rotation in the 2D TechDraw view)
        if (panningActive) {
            pan(event->pos());
        } else {
            startPan(event->pos());
        }
        event->accept();
    }
}

void QGVNavStyleFusion::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (event->button() == Qt::MiddleButton) {
        if (zoomingActive) {
            stopZoom();
            event->accept();
        }
        if (panningActive) {
            stopPan();
            event->accept();
        }
    }
}

}  // namespace TechDrawGui
