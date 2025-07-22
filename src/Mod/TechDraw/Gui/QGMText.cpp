/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
# include <QGraphicsScene>
# include <QPainter>
# include <QStyleOptionGraphicsItem>
#endif

#include "QGMText.h"


using namespace TechDrawGui;

QGMText::QGMText() :
    m_showBox(false),
    m_prettyState("Normal")
{
    setCacheMode(QGCustomText::NoCache);
    setFlag(ItemSendsGeometryChanges, true);
//    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);
}

QVariant QGMText::itemChange(GraphicsItemChange change, const QVariant &value)
{
    //QPointF newPos(0.0, 0.0);
    if(change == ItemPositionHasChanged && scene()) {
        Q_EMIT dragging();
    }

    return QGCustomText::itemChange(change, value);
}

void QGMText::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if(scene() && this == scene()->mouseGrabberItem()) {
        Q_EMIT dragFinished();
    }
    QGCustomText::mouseReleaseEvent(event);
}

void QGMText::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_EMIT hover(true);
    QGCustomText::hoverEnterEvent(event);
}

void QGMText::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_EMIT hover(false);
    QGCustomText::hoverLeaveEvent(event);
}

void QGMText::setPrettySel()
{
    m_prettyState = "Sel";
    QGCustomText::setPrettySel();
}

void QGMText::setPrettyPre()
{
    m_prettyState = "Pre";
    QGCustomText::setPrettyPre();
}

void QGMText::setPrettyNormal()
{
    m_prettyState = "Normal";
    QGCustomText::setPrettyNormal();
}

void QGMText::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging
    //TODO: this should be changed to a rectItem in the parent
    if (showBox()) {
        painter->drawRect(boundingRect().adjusted(1, 1,-1, -1));
    }

    QGCustomText::paint (painter, &myOption, widget);
}

#include <Mod/TechDraw/Gui/moc_QGMText.cpp>
