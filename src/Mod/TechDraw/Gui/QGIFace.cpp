/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPainterPathStroker>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "QGIView.h"
#include "QGIFace.h"

using namespace TechDrawGui;

QGIFace::QGIFace(int index) :
    projIndex(index),
    m_colDefFill(Qt::white),          //Qt::transparent?  paper colour?
    m_styleDef(Qt::SolidPattern),
    m_styleSelect(Qt::SolidPattern)
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


    m_pen.setCosmetic(true);
    m_pen.setColor(m_colNormal);

    m_colNormalFill  = m_colDefFill;
    m_brush.setColor(m_colDefFill);
    m_styleNormal = m_styleDef;
    m_brush.setStyle(m_styleDef);
    setPrettyNormal();
}

QVariant QGIFace::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
       if(isSelected()) {               //this is only QtGui Selected, not FC selected?
            setPrettySel();
        } else {
            setPrettyNormal();
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void QGIFace::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (!isSelected() && !isHighlighted) {
        setPrettyPre();
    }
    QGraphicsPathItem::hoverEnterEvent(event);
}

void QGIFace::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (!isSelected()) {
      setPrettyNormal();
    }
    QGraphicsPathItem::hoverLeaveEvent(event);
}

void QGIFace::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItem::mousePressEvent(event);
}

void QGIFace::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    QGraphicsItem::mouseReleaseEvent(event);
}

void QGIFace::setPrettyNormal() {
    m_colCurrent = m_colNormal;
    m_colCurrFill = m_colNormalFill;
    m_styleCurr = m_styleNormal;
    update();
}

void QGIFace::setPrettyPre() {
    m_colCurrent = m_colPre;
    m_colCurrFill = m_colPre;
    m_styleCurr = m_styleSelect;
    update();
}

void QGIFace::setPrettySel() {
    m_colCurrent = m_colSel;
    m_colCurrFill = m_colSel;
    m_styleCurr = m_styleSelect;
    update();
}

void QGIFace::setHighlighted(bool b)
{
    isHighlighted = b;
    if(isHighlighted && isSelected()) {
        setPrettySel();
    } else if (isHighlighted) {
        setPrettyPre();
    } else {
        setPrettyNormal();
    }
}

void QGIFace::setFill(QColor c, Qt::BrushStyle s) {
    m_colNormalFill = c;
    //m_styleCurr = s;
    m_styleNormal = s;
}

void QGIFace::setFill(QBrush b) {
    m_colNormalFill = b.color();
    //m_styleCurr = b.style();
    m_styleNormal = b.style();
}

void QGIFace::resetFill() {
    m_colNormalFill = m_colDefFill;
    //m_styleCurr = m_styleDef;
    m_styleNormal = m_styleDef;
}

QRectF QGIFace::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGIFace::shape() const
{
    return path();
}

void QGIFace::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    //myOption.state &= ~QStyle::State_Selected;

    m_pen.setColor(m_colCurrent);
    setPen(m_pen);
    m_brush.setStyle(m_styleCurr);
    m_brush.setColor(m_colCurrFill);
    setBrush(m_brush);
    QGraphicsPathItem::paint (painter, &myOption, widget);
}
