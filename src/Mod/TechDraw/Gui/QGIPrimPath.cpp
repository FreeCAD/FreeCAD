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
#include <QGraphicsSceneHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "QGIPrimPath.h"
#include "QGIView.h"

using namespace TechDrawGui;

QGIPrimPath::QGIPrimPath()
{
    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    setAcceptHoverEvents(true);

    isHighlighted = false;

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("NormalColor", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x0000FF00));
    m_colSel = fcColor.asValue<QColor>();
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0x00080800));
    m_colPre = fcColor.asValue<QColor>();

    setPrettyNormal();

    m_pen.setColor(m_colNormal);
    m_pen.setStyle(Qt::SolidLine);
    m_pen.setCapStyle(Qt::RoundCap);
}

QVariant QGIPrimPath::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            setPrettySel();
        } else {
            setPrettyNormal();
        }
    }
    return QGraphicsPathItem::itemChange(change, value);
}

void QGIPrimPath::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (!isSelected()) {
        setPrettyPre();
    }
    QGraphicsPathItem::hoverEnterEvent(event);
}

void QGIPrimPath::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());    //this is temp for debug??
    assert(view != 0);
    if(!isSelected() && !isHighlighted) {
        setPrettyNormal();
    }
    QGraphicsPathItem::hoverLeaveEvent(event);
}

void QGIPrimPath::setHighlighted(bool b)
{
    isHighlighted = b;
    if(isHighlighted) {
        setPrettySel();
    } else {
        setPrettyNormal();
    }
}

void QGIPrimPath::setPrettyNormal() {
    m_colCurrent = m_colNormal;
    update();
}

void QGIPrimPath::setPrettyPre() {
    m_colCurrent = m_colPre;
    update();
}

void QGIPrimPath::setPrettySel() {
    m_colCurrent = m_colSel;
    update();
}

void QGIPrimPath::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    m_pen.setColor(m_colCurrent);
    setPen(m_pen);
    QGraphicsPathItem::paint (painter, &myOption, widget);
}
