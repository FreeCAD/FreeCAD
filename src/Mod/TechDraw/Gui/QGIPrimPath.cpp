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
    m_capStyle(Qt::RoundCap),
//    m_fillStyleCurrent (Qt::SolidPattern),
    m_fillOverride(false)
{
    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);

    setHighlighted(false);

    m_colOverride = false;
    m_colNormal = getNormalColor();
    m_pen.setColor(m_colNormal);
    m_pen.setStyle(Qt::SolidLine);
    m_capStyle = prefCapStyle();
    m_pen.setCapStyle(m_capStyle);
    m_pen.setWidthF(0);

    m_styleDef = Qt::NoBrush;
    m_styleSelect = Qt::SolidPattern;
    m_styleNormal = m_styleDef;
    m_brush.setStyle(m_styleNormal);

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
void QGIPrimPath::setHighlighted(bool isHighlighted)
{
    if(isHighlighted) {
        setPrettySel();
    } else {
        setPrettyNormal();
    }
}

void QGIPrimPath::setPrettyNormal() {
//    Base::Console().Message("QGIPP::setPrettyNormal()\n");
    m_pen.setColor(m_colNormal);
    m_brush.setColor(m_colNormalFill);
}

void QGIPrimPath::setPrettyPre() {
//    Base::Console().Message("QGIPP::setPrettyPre()\n");
    m_pen.setColor(getPreColor());
    if (!m_fillOverride) {
        m_brush.setColor(getPreColor());
    }
}

void QGIPrimPath::setPrettySel() {
//    Base::Console().Message("QGIPP::setPrettySel()\n");
    m_pen.setColor(getSelectColor());
    if (!m_fillOverride) {
        m_brush.setColor(getSelectColor());
    }
}

//wf: why would a face use it's parent's normal colour?
//this always goes to parameter
QColor QGIPrimPath::getNormalColor()
{
    if (m_colOverride) {
        return m_colNormal;
    }

    QGIView* parent = nullptr;
    QGraphicsItem* qparent = parentItem();
    if (qparent) {
        parent = dynamic_cast<QGIView*> (qparent);
    }

    if (!parent) {
        return PreferencesGui::normalQColor();
    }
    return parent->getNormalColor();
}

QColor QGIPrimPath::getPreColor()
{
    QGIView* parent = nullptr;
    QGraphicsItem* qparent = parentItem();
    if (qparent) {
        parent = dynamic_cast<QGIView *> (qparent);
    }
    
    if (!parent) {
        return PreferencesGui::preselectQColor();
    }
    return parent->getPreColor();
}

QColor QGIPrimPath::getSelectColor()
{
    QGIView* parent = nullptr;
    QGraphicsItem* qparent = parentItem();
    if (qparent) {
        parent = dynamic_cast<QGIView *> (qparent);
    }

    if (!parent) {
        return PreferencesGui::selectQColor();
    }
    return parent->getSelectColor();
}

void QGIPrimPath::setWidth(double width)
{
    m_pen.setWidthF(width);
}

void QGIPrimPath::setStyle(Qt::PenStyle style)
{
//    Base::Console().Message("QGIPP::setStyle(QTPS: %d)\n", s);
    m_pen.setStyle(style);
}

void QGIPrimPath::setStyle(int style)
{
//    Base::Console().Message("QGIPP::setStyle(int: %d)\n", s);
    m_pen.setStyle(static_cast<Qt::PenStyle>(style));
}


void QGIPrimPath::setNormalColor(QColor color)
{
    m_colNormal = color;
    m_colOverride = true;
    m_pen.setColor(m_colNormal);
}

void QGIPrimPath::setCapStyle(Qt::PenCapStyle cap)
{
    m_capStyle = cap;
    m_pen.setCapStyle(cap);
}

Base::Reference<ParameterGrp> QGIPrimPath::getParmGroup()
{
    return Preferences::getPreferenceGroup("Colors");
}

//EdgeCapStyle param changed from UInt (Qt::PenCapStyle) to Int (QComboBox index)
Qt::PenCapStyle QGIPrimPath::prefCapStyle()
{
    int newStyle = Preferences::getPreferenceGroup("General")->GetInt("EdgeCapStyle", 32);    //0x00 FlatCap, 0x10 SquareCap, 0x20 RoundCap
    switch (newStyle) {
        case 0:
            return static_cast<Qt::PenCapStyle>(0x20);  // Round
        case 1:
            return static_cast<Qt::PenCapStyle>(0x10);  // Square
        case 2:
            return static_cast<Qt::PenCapStyle>(0x00);  // Flat
    }
    return static_cast<Qt::PenCapStyle>(0x20);
}

void QGIPrimPath::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    //wf: this seems a bit of a hack. does it mess up selection of QGIPP??
    QGraphicsItem* qparent = parentItem();
    QGIView* parent = nullptr;
    if (qparent) {
        parent = dynamic_cast<QGIView*>(qparent);
    }
    
    if (!parent) {
        // qparent->mousePressEvent(event);  //protected!
        // Base::Console().Message("QGIPP::mousePressEvent - passing event to ancestor\n");
        QGraphicsPathItem::mousePressEvent(event);
        return;
    }
    // Base::Console().Message("QGIPP::mousePressEvent - passing event to QGIV parent\n");
    parent->mousePressEvent(event);
}

void QGIPrimPath::setFill(QColor color, Qt::BrushStyle style) {
    setFillColor(color);
    m_styleNormal = style;
    m_brush.setStyle(style);
}

void QGIPrimPath::setFill(QBrush b) {
    setFillColor(b.color());
    m_styleNormal = b.style();
    m_brush.setStyle(b.style());
}

void QGIPrimPath::resetFill() {
    m_colNormalFill = m_colDefFill;
    m_styleNormal = m_styleDef;
    m_brush.setStyle(m_styleDef);
}

//set PlainFill
void QGIPrimPath::setFillColor(QColor color)
{
    m_colNormalFill = color;
    m_brush.setColor(m_colNormalFill);
//    m_colDefFill = c;
}


void QGIPrimPath::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    setPen(m_pen);
    setBrush(m_brush);

    QGraphicsPathItem::paint (painter, &myOption, widget);
}

