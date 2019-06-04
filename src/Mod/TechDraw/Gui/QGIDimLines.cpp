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
#include <QGraphicsScene>
#include <QMenu>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainterPathStroker>
#include <QPainter>
#endif

#include "QGIDimLines.h"

using namespace TechDrawGui;

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

QRectF QGIDimLines::boundingRect() const
{
    return shape().boundingRect().adjusted(-2, -2, 2, 2);  //room for 0.5 line widths? needs Rez::guiX??
//    return childrenBoundingRect().adjusted(-2,-2,2,2);
}

//probably don't need this paint
void QGIDimLines::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging

    QGIPrimPath::paint (painter, &myOption, widget);
}


