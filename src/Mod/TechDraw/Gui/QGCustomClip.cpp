/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
#include <QGraphicsSceneHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "ZVALUE.h"
#include "QGICMark.h"
#include "QGCustomClip.h"

using namespace TechDrawGui;

QGCustomClip::QGCustomClip()
{
    setHandlesChildEvents(false);                //not sure if needs to handle events for Views in Group???
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
//    setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);   //good for debugging
    m_rect = QRectF(0.,0.,10.,10.);
}

void QGCustomClip::centerAt(QPointF centerPos)
{
    centerAt(centerPos.x(),centerPos.y());
}

void QGCustomClip::centerAt(double cX, double cY)
{
    QRectF box = boundingRect();
    double width = box.width();
    double height = box.height();
    double newX = cX - width/2.;
    double newY = cY - height/2.;
    setPos(newX,newY);
}

void QGCustomClip::setRect(QRectF r)
{
    //prepareGeometryChange();??
    m_rect = r;
}

void QGCustomClip::setRect(double x, double y, double w, double h)
{
    QRectF r(x,y,w,h);
    setRect(r);
}

QRectF QGCustomClip::rect()
{
    return m_rect;
}

void QGCustomClip::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGraphicsItemGroup::paint (painter, &myOption, widget);
}

QRectF QGCustomClip::boundingRect() const     //sb shape()?
{
    return m_rect;
}

void QGCustomClip::makeMark(double x, double y)
{
    QGICMark* cmItem = new QGICMark(-1);
    cmItem->setParentItem(this);
    cmItem->setPos(x,y);
    cmItem->setThick(1.0);
    cmItem->setSize(40.0);
    cmItem->setZValue(ZVALUE::VERTEX);
}

void QGCustomClip::makeMark(Base::Vector3d v)
{
    makeMark(v.x,v.y);
}


