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
    m_radius(5.0),
    m_fudge(1.01)       // same as m_fudge in DrawViewDetail

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

    m_mat = new QGraphicsPathItem();
    addToGroup(m_mat);
    m_matPen.setColor(Qt::white);
    m_matPen.setStyle(Qt::SolidLine);
    m_matBrush.setStyle(Qt::SolidPattern);
    m_matBrush.setColor(Qt::white);
    m_mat->setPen(m_matPen);
    m_mat->setBrush(m_matBrush);

    setZValue(ZVALUE::MATTING);
}

void QGIMatting::draw()
{
    prepareGeometryChange();
    double penWidth = Rez::guiX(TechDraw::LineGroup::getDefaultWidth("Graphic"));
    double penWidth_2 = penWidth / 2.0;
    m_pen.setWidthF(penWidth);
    double matSize = m_radius * m_fudge + 2 * penWidth;   // outer bound of mat
    m_matPen.setWidthF(2.0 * penWidth);
    QPainterPath ppCut;
    QPainterPath ppMat;
    if (getHoleStyle() == 0) {
        QRectF roundCutout (-m_radius, -m_radius, 2.0 * m_radius, 2.0 * m_radius);
        ppCut.addEllipse(roundCutout);
        QRectF roundMat(-matSize, -matSize, 2.0 * matSize, 2.0 * matSize);
        ppMat.addEllipse(roundMat);
        ppMat.addEllipse(roundCutout.adjusted(-penWidth_2, -penWidth_2, penWidth_2, penWidth_2));
    } else {
        double squareSize = m_radius;
        QRectF squareCutout (-squareSize, -squareSize, 2.0 * squareSize, 2.0 * squareSize);
        ppCut.addRect(squareCutout);
        QRectF squareMat(-matSize, -matSize, 2.0 * matSize, 2.0 * matSize);
        ppMat.addRect(squareMat);
        ppMat.addRect(squareCutout.adjusted(-penWidth_2, -penWidth_2, penWidth_2, penWidth_2));
    }
    m_border->setPen(m_pen);
    m_border->setPath(ppCut);
    m_border->setZValue(ZVALUE::MATTING);
    m_mat->setPen(m_matPen);
    m_mat->setPath(ppMat);
    m_mat->setZValue(ZVALUE::MATTING - 1.0);
}

int QGIMatting::getHoleStyle()
{
    return TechDraw::Preferences::mattingStyle();
}

//need this because QQGIG only updates BR when items added/deleted.
QRectF QGIMatting::boundingRect() const
{
    return childrenBoundingRect().adjusted(-1, -1, 1,1);
}

void QGIMatting::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //painter->drawRect(boundingRect().adjusted(-2.0, -2.0, 2.0, 2.0));

    QGraphicsItemGroup::paint (painter, &myOption, widget);
}
