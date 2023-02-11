/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
# include <cassert>

# include <QPainter>
# include <QPainterPath>
# include <QRectF>
# include <QStyleOptionGraphicsItem>
#endif

#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "QGIMatting.h"
#include "Rez.h"
#include "ZVALUE.h"


using namespace TechDrawGui;

QGIMatting::QGIMatting() :
    m_radius(5.0)

{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    m_border = new QGraphicsPathItem();
    addToGroup(m_border);

    m_pen.setColor(Qt::black);
    m_brush.setStyle(Qt::NoBrush);

    m_border->setPen(m_pen);
    m_border->setBrush(m_brush);

    setZValue(ZVALUE::MATTING);
}

void QGIMatting::draw()
{
    prepareGeometryChange();
    QPainterPath ppCut;
    if (getHoleStyle() == 0) {
        QRectF roundCutout (-m_radius, -m_radius, 2.0 * m_radius, 2.0 * m_radius);
        ppCut.addEllipse(roundCutout);
    } else {
        double squareSize = m_radius;
        QRectF squareCutout (-squareSize, -squareSize, 2.0 * squareSize, 2.0 * squareSize);
        ppCut.addRect(squareCutout);
    }
    m_pen.setWidthF(Rez::guiX(TechDraw::LineGroup::getDefaultWidth("Graphic")));
    m_border->setPen(m_pen);
    m_border->setPath(ppCut);
    m_border->setZValue(ZVALUE::MATTING);
}

int QGIMatting::getHoleStyle()
{
    return TechDraw::Preferences::mattingStyle();
}

//need this because QQGIG only updates BR when items added/deleted.
QRectF QGIMatting::boundingRect() const
{
    QRectF result ;
    result = childrenBoundingRect().adjusted(-1, -1, 1,1);
    return result;
}

void QGIMatting::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //painter->drawRect(boundingRect().adjusted(-2.0, -2.0, 2.0, 2.0));

    QGraphicsItemGroup::paint (painter, &myOption, widget);
}
