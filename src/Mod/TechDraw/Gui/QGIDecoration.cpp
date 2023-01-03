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
# include <QStyleOptionGraphicsItem>
#endif

#include "QGIDecoration.h"
#include "QGICMark.h"
#include "PreferencesGui.h"
#include "ZVALUE.h"


using namespace TechDrawGui;
using namespace TechDraw;

QGIDecoration::QGIDecoration() :
    m_colCurrent(Qt::black),
    m_styleCurrent(Qt::SolidLine),
    m_brushCurrent(Qt::SolidPattern),
    m_dragState(DECORNODRAG)
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setWidth(1.0);
}

void QGIDecoration::draw()
{
}

void QGIDecoration::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QGraphicsItemGroup::paint (painter, &myOption, widget);
}

void QGIDecoration::setWidth(double w)
{
    m_width = w;
    m_pen.setWidthF(m_width);
}

void QGIDecoration::setStyle(Qt::PenStyle s)
{
    m_styleCurrent = s;
    m_pen.setStyle(m_styleCurrent);
}

void QGIDecoration::setColor(QColor c)
{
    m_colNormal = c;
    m_colCurrent = c;
    m_pen.setColor(m_colCurrent);
    m_brush.setColor(m_colCurrent);
}

QColor QGIDecoration::prefNormalColor()
{
    return PreferencesGui::normalQColor();
}

QColor QGIDecoration::prefPreColor()
{
    return PreferencesGui::preselectQColor();
}

QColor QGIDecoration::prefSelectColor()
{
    return PreferencesGui::selectQColor();
}

QRectF QGIDecoration::boundingRect() const
{
    return childrenBoundingRect();
}


void QGIDecoration::makeMark(double x, double y)
{
    QGICMark* cmItem = new QGICMark(-1);
    cmItem->setParentItem(this);
    cmItem->setPos(x, y);
    cmItem->setThick(2.0);
    cmItem->setSize(40.0);
    cmItem->setZValue(ZVALUE::VERTEX);
}

void QGIDecoration::makeMark(Base::Vector3d v)
{
    makeMark(v.x, v.y);
}

void QGIDecoration::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGID::mousePressEvent() - %s\n", getViewName());
    m_dragState = DECORDRAGSTARTED;

    QGraphicsItem::mousePressEvent(event);
}

void QGIDecoration::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    if (m_dragState == DECORDRAGSTARTED) {
        m_dragState = DECORDRAGGING;
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void QGIDecoration::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGID::mouseReleaseEvent() - %s\n", getViewName());
    if (m_dragState == DECORDRAGGING) {
        onDragFinished();
    }
    m_dragState = DECORNODRAG;

    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIDecoration::onDragFinished()
{
    //override this
}
