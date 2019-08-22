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

#include "QGIPrimPath.h"
#include "QGIView.h"

using namespace TechDrawGui;

QGIPrimPath::QGIPrimPath():
    m_width(0),
    m_capStyle(Qt::RoundCap)
{
    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    setAcceptHoverEvents(true);

    isHighlighted = false;

    m_colNormal = Qt::white;
    m_colOverride = false;
    m_colCurrent = getNormalColor();
    m_styleCurrent = Qt::SolidLine;
    m_pen.setStyle(m_styleCurrent);
    m_capStyle = prefCapStyle();
    m_pen.setCapStyle(m_capStyle);
//    m_pen.setCapStyle(Qt::FlatCap);
    m_pen.setWidthF(m_width);

    setPrettyNormal();
}

QVariant QGIPrimPath::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().Message("QGIPP::itemChange(%d) - type: %d\n", change,type() - QGraphicsItem::UserType);
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
    if(!isSelected()) {
        setPrettyNormal();
    }
    QGraphicsPathItem::hoverLeaveEvent(event);
}

//set highlighted is obsolete
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
    m_colCurrent = getNormalColor();
    update();
}

void QGIPrimPath::setPrettyPre() {
    m_colCurrent = getPreColor();
    update();
}

void QGIPrimPath::setPrettySel() {
    m_colCurrent = getSelectColor();
    update();
}

void QGIPrimPath::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    m_pen.setWidthF(m_width);
    m_pen.setColor(m_colCurrent);
    m_pen.setStyle(m_styleCurrent);
    setPen(m_pen);
    QGraphicsPathItem::paint (painter, &myOption, widget);
}

QColor QGIPrimPath::getNormalColor()
{

    QColor result;
    QGIView *parent;

    if (m_colOverride) {
        result = m_colNormal;
        return result;
    }

    QGraphicsItem* qparent = parentItem();
    if (qparent == nullptr) {
        parent = nullptr;
    } else {
        parent = dynamic_cast<QGIView *> (qparent);
    }

    if (parent != nullptr) {
        result = parent->getNormalColor();
    } else {
        Base::Reference<ParameterGrp> hGrp = getParmGroup();
        App::Color fcColor;
        fcColor.setPackedValue(hGrp->GetUnsigned("NormalColor", 0x00000000));
        result = fcColor.asValue<QColor>();
    }
    return result;
}

QColor QGIPrimPath::getPreColor()
{
    QColor result;
    QGIView *parent;
    QGraphicsItem* qparent = parentItem();
    if (qparent == nullptr) {
        parent = nullptr;
    } else {
        parent = dynamic_cast<QGIView *> (qparent);
    }

    if (parent != nullptr) {
        result = parent->getPreColor();
    } else {
        Base::Reference<ParameterGrp> hGrp = getParmGroup();
        App::Color fcColor;
        fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0xFFFF0000));
        result = fcColor.asValue<QColor>();
    }
    return result;
}

QColor QGIPrimPath::getSelectColor()
{
    QColor result;
    QGIView *parent;
    QGraphicsItem* qparent = parentItem();
    if (qparent == nullptr) {
        parent = nullptr;
    } else {
        parent = dynamic_cast<QGIView *> (qparent);
    }

    if (parent != nullptr) {
        result = parent->getSelectColor();
    } else {
        Base::Reference<ParameterGrp> hGrp = getParmGroup();
        App::Color fcColor;
        fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x00FF0000));
        result = fcColor.asValue<QColor>();
    }
    return result;
}

void QGIPrimPath::setWidth(double w)
{
//    Base::Console().Message("QGIPP::setWidth(%.3f)\n", w);
    m_width = w;
    m_pen.setWidthF(m_width);
}

void QGIPrimPath::setStyle(Qt::PenStyle s)
{
//    Base::Console().Message("QGIPP::setStyle(QTPS: %d)\n", s);
    m_styleCurrent = s;
}

void QGIPrimPath::setStyle(int s)
{
//    Base::Console().Message("QGIPP::setStyle(int: %d)\n", s);
    m_styleCurrent = (Qt::PenStyle) s;
}


void QGIPrimPath::setNormalColor(QColor c)
{
    m_colNormal = c;
    m_colOverride = true;
}

void QGIPrimPath::setCapStyle(Qt::PenCapStyle c)
{
    m_capStyle = c;
    m_pen.setCapStyle(c);
}

Base::Reference<ParameterGrp> QGIPrimPath::getParmGroup()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    return hGrp;
}

Qt::PenCapStyle QGIPrimPath::prefCapStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    Qt::PenCapStyle result;
    unsigned int cap = hGrp->GetUnsigned("EdgeCapStyle", 0x20);    //0x00 FlatCap, 0x10 SquareCap, 0x20 RoundCap
    result = (Qt::PenCapStyle) cap;
    return result;
}

void QGIPrimPath::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    QGIView *parent;
    QGraphicsItem* qparent = parentItem();
    if (qparent != nullptr) {
        parent = dynamic_cast<QGIView *> (qparent);
        if (parent != nullptr) {
            parent->mousePressEvent(event);
        } else {
            Base::Console().Log("QGIPP::mousePressEvent - no QGIView parent\n");
        }
    }
    QGraphicsPathItem::mousePressEvent(event);
}
