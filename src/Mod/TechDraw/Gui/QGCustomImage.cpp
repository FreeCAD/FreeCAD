/***************************************************************************
 *   Copyright (c) 2016 WandererFan   (wandererfan@gmail.com)              *
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
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtGlobal>
#endif

#include <Base/Console.h>

#include <QRectF>
#include <QPixmap>

#include "QGCustomImage.h"

using namespace TechDrawGui;

QGCustomImage::QGCustomImage() 
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

QGCustomImage::~QGCustomImage()
{
}

void QGCustomImage::centerAt(QPointF centerPos)
{
    centerAt(centerPos.x(),centerPos.y());
}

void QGCustomImage::centerAt(double cX, double cY)
{
    QRectF br = boundingRect();
    double width = br.width() * scale();
    double height = br.height() * scale();
    double newX = cX - width/2.;
    double newY = cY - height/2.;
    setPos(newX, newY);
}

bool QGCustomImage::load(QString fileSpec)
{
    bool success = true;
    QPixmap px(fileSpec);
    m_px = px;
    prepareGeometryChange();
    setPixmap(m_px);
    return(success);
}

QSize QGCustomImage::imageSize(void)
{
    QSize result = m_px.size();
    return result;
}

void QGCustomImage::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //painter->drawRect(boundingRect());          //good for debugging

    QGraphicsPixmapItem::paint (painter, &myOption, widget);
}

