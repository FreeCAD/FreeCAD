/***************************************************************************
 *   Copyright (c) 2015 Pavel Strakhov <ri@idzaaus.org>                    *
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

/*
 * Based on
 *
 * http://stackoverflow.com/questions/19113532/qgraphicsview-zooming-in-and-out-under-mouse-position-using-mouse-wheel
 *
 * Re-licensed to LGPL after having contacted original author by e-mail.
 *
 */

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QGraphicsView>
# include <QMouseEvent>
# include <QApplication>
# include <QScrollBar>
# include <qmath.h>
#endif

#include <GraphicsViewZoom.h>

GraphicsViewZoom::GraphicsViewZoom(QGraphicsView* view)
  : QObject(view), _view(view), m_invert_zoom(false)
{
  _view->viewport()->installEventFilter(this);
  _view->setMouseTracking(true);
  _modifiers = Qt::ControlModifier;
  _zoom_factor_base = 1.0015;
}

void GraphicsViewZoom::gentle_zoom(double factor) {
  _view->scale(factor, factor);
  _view->centerOn(target_scene_pos);
  QPointF delta_viewport_pos = target_viewport_pos - QPointF(_view->viewport()->width() / 2.0,
                                                             _view->viewport()->height() / 2.0);
  QPointF viewport_center = _view->mapFromScene(target_scene_pos) - delta_viewport_pos;
  _view->centerOn(_view->mapToScene(viewport_center.toPoint()));
}

void GraphicsViewZoom::set_modifiers(Qt::KeyboardModifiers modifiers) {
  _modifiers = modifiers;

}

void GraphicsViewZoom::set_zoom_factor_base(double value) {
  _zoom_factor_base = value;
}

bool GraphicsViewZoom::eventFilter(QObject *object, QEvent *event) {
  if (event->type() == QEvent::MouseMove) {
    QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
    QPointF delta = target_viewport_pos - mouse_event->pos();
    if (qAbs(delta.x()) > 5 || qAbs(delta.y()) > 5) {
      target_viewport_pos = mouse_event->pos();
      target_scene_pos = _view->mapToScene(mouse_event->pos());
    }
  } else if (event->type() == QEvent::Wheel) {
    QWheelEvent* wheel_event = static_cast<QWheelEvent*>(event);
    if (QApplication::keyboardModifiers() == _modifiers) {
      if (wheel_event->orientation() == Qt::Vertical) {
        double angle = -wheel_event->delta();
        if (m_invert_zoom)
          angle = -angle;
        double factor = qPow(_zoom_factor_base, angle);
        gentle_zoom(factor);
        return true;
      }
    }
  }
  Q_UNUSED(object);
  return false;
}

#include "moc_GraphicsViewZoom.cpp"
