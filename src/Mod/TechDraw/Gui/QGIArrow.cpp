/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <assert.h>
#include <QGraphicsScene>
#include <QMenu>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainterPathStroker>
#include <QPainter>
#endif

#include "QGIView.h"
#include "QGIArrow.h"

using namespace TechDrawGui;

QGIArrow::QGIArrow(QGraphicsScene *scene)
{
    isFlipped = false;

    if(scene) {
        scene->addItem(this);
    }

    // Set Cache Mode
    setCacheMode(QGraphicsItem::NoCache);

}

QPainterPath QGIArrow::shape() const
{
    return path();
}

void QGIArrow::setHighlighted(bool state)
{
    QPen myPen = pen();
    QBrush myBrush = brush();
    if(state) {
        myPen.setColor(Qt::blue);
        myBrush.setColor(Qt::blue);
    } else {
        myPen.setColor(Qt::black);
        myBrush.setColor(Qt::black);
    }
    setBrush(myBrush);
    setPen(myPen);
}

QVariant QGIArrow::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsPathItem::itemChange(change, value);
}

void QGIArrow::flip(bool state) {
    isFlipped = state;
}

void QGIArrow::draw() {
    // the center is the end point on a dimension
    QPainterPath path;
    QPen pen(Qt::black);
    pen.setWidth(1);

    QBrush brush(Qt::black);
    //setPen(pen);
    setBrush(brush);

    float length = -5.;           //TODO: Arrow heads sb preference? size & type?

    if(isFlipped)
        length *= -1;
    path.moveTo(QPointF(0.,0.));
    path.lineTo(QPointF(length,-0.6));
    path.lineTo(QPointF(length, 0.6));

    path.closeSubpath();
//     path.moveTo(QPointF(-1,1));
//     path.lineTo(QPointF(1,-1));
    setPath(path);

}

void QGIArrow::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;
    QGraphicsPathItem::paint(painter, &myOption, widget);
}
