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

# include <QGraphicsScene>
# include <QGraphicsSceneHoverEvent>
# include <QPainter>
# include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>

#include "QGIPrimPath.h"
#include "PreferencesGui.h"
#include "QGIView.h"


using namespace TechDrawGui;
using namespace TechDraw;

QGIPrimPath::QGIPrimPath():
    m_width(0),
    m_capStyle(Qt::RoundCap),
    m_fillStyleCurrent (Qt::NoBrush),
//    m_fillStyleCurrent (Qt::SolidPattern),
    m_fillOverride(false)
{
    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);

    isHighlighted = false;

    m_colOverride = false;
    m_colNormal = getNormalColor();
    m_colCurrent = m_colNormal;
    m_styleCurrent = Qt::SolidLine;
    m_pen.setStyle(m_styleCurrent);
    m_capStyle = prefCapStyle();
    m_pen.setCapStyle(m_capStyle);
    m_pen.setWidthF(m_width);

    m_styleDef = Qt::NoBrush;
    m_styleSelect = Qt::SolidPattern;
    m_styleNormal = m_styleDef;
    m_fillStyleCurrent = m_styleNormal;

    m_colDefFill = Qt::white;
//    m_colDefFill = Qt::transparent;
    setFillColor(m_colDefFill);

    setPrettyNormal();
}

QVariant QGIPrimPath::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().Message("QGIPP::itemChange(%d) - type: %d\n", change, type() - QGraphicsItem::UserType);
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
//    Base::Console().Message("QGIPP::hoverEnter() - selected; %d\n", isSelected());
    if (!isSelected()) {
        setPrettyPre();
    }
    QGraphicsPathItem::hoverEnterEvent(event);
}

void QGIPrimPath::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
//    Base::Console().Message("QGIPP::hoverLeave() - selected; %d\n", isSelected());
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
//    Base::Console().Message("QGIPP::setPrettyNormal()\n");
    m_colCurrent = m_colNormal;
    m_fillColorCurrent = m_colNormalFill;
}

void QGIPrimPath::setPrettyPre() {
//    Base::Console().Message("QGIPP::setPrettyPre()\n");
    m_colCurrent = getPreColor();
    if (!m_fillOverride) {
        m_fillColorCurrent = getPreColor();
    }
}

void QGIPrimPath::setPrettySel() {
//    Base::Console().Message("QGIPP::setPrettySel()\n");
    m_colCurrent = getSelectColor();
    if (!m_fillOverride) {
        m_fillColorCurrent = getSelectColor();
    }
}

//wf: why would a face use it's parent's normal colour?
//this always goes to parameter
QColor QGIPrimPath::getNormalColor()
{
    QGIView *parent;

    if (m_colOverride) {
        return m_colNormal;
    }

    QGraphicsItem* qparent = parentItem();
    if (!qparent) {
        parent = nullptr;
    } else {
        parent = dynamic_cast<QGIView *> (qparent);
    }

    if (parent) {
        return parent->getNormalColor();
    }
    return PreferencesGui::normalQColor();
}

QColor QGIPrimPath::getPreColor()
{
    QGIView *parent;
    QGraphicsItem* qparent = parentItem();
    if (!qparent) {
        parent = nullptr;
    } else {
        parent = dynamic_cast<QGIView *> (qparent);
    }

    if (parent) {
        return parent->getPreColor();
    }
    return PreferencesGui::preselectQColor();
}

QColor QGIPrimPath::getSelectColor()
{
    QGIView *parent;
    QGraphicsItem* qparent = parentItem();
    if (!qparent) {
        parent = nullptr;
    } else {
        parent = dynamic_cast<QGIView *> (qparent);
    }

    if (parent) {
        return parent->getSelectColor();
    }
    return PreferencesGui::selectQColor();
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
    m_styleCurrent = static_cast<Qt::PenStyle>(s);
}


void QGIPrimPath::setNormalColor(QColor c)
{
    m_colNormal = c;
    m_colOverride = true;
    m_colCurrent = m_colNormal;
}

void QGIPrimPath::setCapStyle(Qt::PenCapStyle c)
{
    m_capStyle = c;
    m_pen.setCapStyle(c);
}

Base::Reference<ParameterGrp> QGIPrimPath::getParmGroup()
{
    return Preferences::getPreferenceGroup("Colors");
}

//EdgeCapStyle param changed from UInt (Qt::PenCapStyle) to Int (QComboBox index)
Qt::PenCapStyle QGIPrimPath::prefCapStyle()
{
    Qt::PenCapStyle result;
    int newStyle;
    newStyle = Preferences::getPreferenceGroup("General")->GetInt("EdgeCapStyle", 32);    //0x00 FlatCap, 0x10 SquareCap, 0x20 RoundCap
    switch (newStyle) {
        case 0:
            result = static_cast<Qt::PenCapStyle>(0x20);   //round;
            break;
        case 1:
            result = static_cast<Qt::PenCapStyle>(0x10);   //square;
            break;
        case 2:
            result = static_cast<Qt::PenCapStyle>(0x00);   //flat
            break;
        default:
            result = static_cast<Qt::PenCapStyle>(0x20);
    }
    return result;
}

void QGIPrimPath::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    //wf: this seems a bit of a hack. does it mess up selection of QGIPP??
    QGIView *parent;
    QGraphicsItem* qparent = parentItem();
    if (qparent) {
        parent = dynamic_cast<QGIView *> (qparent);
        if (parent) {
//            Base::Console().Message("QGIPP::mousePressEvent - passing event to QGIV parent\n");
            parent->mousePressEvent(event);
        } else {
//            qparent->mousePressEvent(event);  //protected!
            QGraphicsPathItem::mousePressEvent(event);
        }
    } else {
//        Base::Console().Message("QGIPP::mousePressEvent - passing event to ancestor\n");
        QGraphicsPathItem::mousePressEvent(event);
    }
}

void QGIPrimPath::setFill(QColor c, Qt::BrushStyle s) {
    setFillColor(c);
    m_styleNormal = s;
    m_fillStyleCurrent = s;
}

void QGIPrimPath::setFill(QBrush b) {
    setFillColor(b.color());
    m_styleNormal = b.style();
    m_fillStyleCurrent = b.style();
}

void QGIPrimPath::resetFill() {
    m_colNormalFill = m_colDefFill;
    m_styleNormal = m_styleDef;
    m_fillStyleCurrent = m_styleDef;
}

//set PlainFill
void QGIPrimPath::setFillColor(QColor c)
{
    m_colNormalFill = c;
    m_fillColorCurrent = m_colNormalFill;
//    m_colDefFill = c;
}


void QGIPrimPath::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    m_pen.setWidthF(m_width);
    m_pen.setColor(m_colCurrent);
    m_pen.setStyle(m_styleCurrent);
    setPen(m_pen);

    m_brush.setColor(m_fillColorCurrent);
    m_brush.setStyle(m_fillStyleCurrent);
    setBrush(m_brush);

    QGraphicsPathItem::paint (painter, &myOption, widget);
}

