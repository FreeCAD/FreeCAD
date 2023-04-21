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
# include <QPainter>
# include <QRectF>
# include <QStyleOptionGraphicsItem>
#endif

#include "QGCustomSvg.h"


using namespace TechDrawGui;

QGCustomSvg::QGCustomSvg()
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_svgRender = new QSvgRenderer();
}

QGCustomSvg::~QGCustomSvg()
{
    delete m_svgRender;
}

void QGCustomSvg::centerAt(QPointF centerPos)
{
    centerAt(centerPos.x(), centerPos.y());
}

void QGCustomSvg::centerAt(double cX, double cY)
{
    QRectF box = boundingRect();
    double width = box.width();
    double height = box.height();
    double newX = (cX - width/2.) * scale();
    double newY = (cY - height/2.) * scale();
    setPos(newX, newY);
}

bool QGCustomSvg::load(QByteArray *svgBytes)
{
    bool success = m_svgRender->load(*svgBytes);
    prepareGeometryChange();
    setSharedRenderer(m_svgRender);
    return(success);
}

bool QGCustomSvg::load(QString filename)
{
    bool success = m_svgRender->load(filename);
    prepareGeometryChange();
    setSharedRenderer(m_svgRender);
    return(success);
}

void QGCustomSvg::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->setPen(Qt::yellow);
//    painter->drawRect(boundingRect());          //good for debugging

    QGraphicsSvgItem::paint (painter, &myOption, widget);
}
