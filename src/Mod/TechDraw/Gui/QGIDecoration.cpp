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
#include <assert.h>
//#include <QGraphicsScene>
//#include <QGraphicsSceneHoverEvent>
//#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"
#include "QGICMark.h"
#include "QGIDecoration.h"

using namespace TechDrawGui;

QGIDecoration::QGIDecoration() :
    m_colCurrent(Qt::black),
    m_styleCurrent(Qt::SolidLine),
    m_brushCurrent(Qt::SolidPattern)
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);

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
    m_colCurrent = c;
    m_pen.setColor(m_colCurrent);
}

void QGIDecoration::makeMark(double x, double y)
{
    QGICMark* cmItem = new QGICMark(-1);
    cmItem->setParentItem(this);
    cmItem->setPos(x,y);
    cmItem->setThick(2.0);
    cmItem->setSize(40.0);
    cmItem->setZValue(ZVALUE::VERTEX);
}

void QGIDecoration::makeMark(Base::Vector3d v)
{
    makeMark(v.x,v.y);
}


