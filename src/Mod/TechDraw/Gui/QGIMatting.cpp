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
#include <cassert>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <qmath.h>
#include <QRectF>
#include "PreferencesGui.h"
#include "QGCustomRect.h"
#include "ZVALUE.h"
#include "QGIMatting.h"

using namespace TechDrawGui;

QGIMatting::QGIMatting() :
    m_height(10.0),
    m_width(10.0),
    //m_holeStyle(0),
    m_radius(5.0)

{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    m_mat = new QGraphicsPathItem();
    addToGroup(m_mat);
    m_border = new QGraphicsPathItem();
    addToGroup(m_border);

    m_pen.setColor(Qt::white);
//    m_pen.setColor(Qt::black);
//    m_pen.setStyle(Qt::DashLine);
    m_brush.setColor(Qt::white);
//    m_brush.setColor(Qt::black);
    m_brush.setStyle(Qt::SolidPattern);
//    m_brush.setStyle(Qt::CrossPattern);
//    m_brush.setStyle(Qt::NoBrush);
    m_penB.setColor(Qt::black);
    m_brushB.setStyle(Qt::NoBrush);

    m_mat->setPen(m_pen);
    m_mat->setBrush(m_brush);
    m_border->setPen(m_penB);
    m_border->setBrush(m_brushB);

    setZValue(ZVALUE::MATTING);
}

void QGIMatting::draw()
{
    prepareGeometryChange();
    double radiusFudge = 1.2;       //keep slightly larger than fudge in App/DVDetail (1.1) to prevent bleed through
    m_width = m_radius * radiusFudge;
    m_height = m_radius * radiusFudge;
    QRectF outline(-m_width,-m_height,2.0 * m_width,2.0 * m_height);
    QPainterPath ppOut;
    ppOut.addRect(outline);
    QPainterPath ppCut;
    if (getHoleStyle() == 0) {
        QRectF roundCutout (-m_radius,-m_radius,2.0 * m_radius,2.0 * m_radius);
        ppCut.addEllipse(roundCutout);
    } else {
        double squareSize = m_radius;
        QRectF squareCutout (-squareSize,-squareSize,2.0 * squareSize,2.0 * squareSize);
        ppCut.addRect(squareCutout);
    }
    ppOut.addPath(ppCut);
    m_mat->setPath(ppOut);
    m_border->setPath(ppCut);
    m_mat->setZValue(ZVALUE::MATTING);
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
    result = childrenBoundingRect().adjusted(-1,-1,1,1);
    return result;
}

void QGIMatting::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //painter->drawRect(boundingRect().adjusted(-2.0,-2.0,2.0,2.0));

    QGraphicsItemGroup::paint (painter, &myOption, widget);
}
