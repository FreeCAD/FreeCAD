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
#include <QGraphicsScene>
#include <QMenu>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainterPathStroker>
#include <QPainter>
#include <QPainterPath>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/Console.h>

#include "PreferencesGui.h"
#include "QGIDimLines.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIDimLines::QGIDimLines()
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    m_width = 0.5;
}

void QGIDimLines::draw()
{
}

QPainterPath QGIDimLines::shape() const
{
    QPainterPath outline;
    QPainterPathStroker stroker;
    stroker.setWidth(getEdgeFuzz());
    outline = stroker.createStroke(path());
    return outline;
}

double QGIDimLines::getEdgeFuzz(void) const
{
    return PreferencesGui::edgeFuzz();
}


QRectF QGIDimLines::boundingRect() const
{
    return shape().controlPointRect().adjusted(-3, -3, 3, 3);
}

void QGIDimLines::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());   //good for debugging
//    painter->drawPath(shape());          //good for debugging

    QGIPrimPath::paint (painter, &myOption, widget);
}


