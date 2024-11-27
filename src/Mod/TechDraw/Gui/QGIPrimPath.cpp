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

#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawView.h>

#include "QGIPrimPath.h"
#include "PreferencesGui.h"
#include "QGIView.h"
#include "DrawGuiUtil.h"


using namespace TechDrawGui;
using namespace TechDraw;
using DGU = DrawGuiUtil;

QGIPrimPath::QGIPrimPath():
    m_width(0),
    m_capStyle(Qt::RoundCap),
    m_fillStyleCurrent (Qt::NoBrush),
    m_fillOverride(false)
{
    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, true);      // to get key press events

    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);

    isHighlighted = false;
    multiselectActivated = false;

    m_colOverride = false;
    m_colNormal = getNormalColor();
    m_colCurrent = m_colNormal;
    m_styleNormal = Qt::SolidLine;
    m_styleCurrent = m_styleNormal;
    m_pen.setStyle(m_styleCurrent);
    m_capStyle = prefCapStyle();
    m_pen.setCapStyle(m_capStyle);
    m_pen.setWidthF(m_width);

    m_fillDef = Qt::NoBrush;
    m_fillSelect = Qt::SolidPattern;
    m_fillNormal = m_fillDef;
    m_fillStyleCurrent = m_fillNormal;

    m_colDefFill = Qt::white;
    setFillColor(m_colDefFill);

    setPrettyNormal();
}

QVariant QGIPrimPath::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            setPrettySel();
            setFocus();
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
    setFocus();
    QGraphicsPathItem::hoverEnterEvent(event);
}

void QGIPrimPath::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if(!isSelected()) {
        setPrettyNormal();
    }

    QGraphicsPathItem::hoverLeaveEvent(event);
}


void QGIPrimPath::setPrettyNormal() {

    m_colCurrent = m_colNormal;
    m_fillColorCurrent = m_colNormalFill;
}

void QGIPrimPath::setPrettyPre() {
    m_colCurrent = getPreColor();
    if (!m_fillOverride) {
        m_fillColorCurrent = getPreColor();
    }
}

void QGIPrimPath::setPrettySel() {
    m_colCurrent = getSelectColor();
    if (!m_fillOverride) {
        m_fillColorCurrent = getSelectColor();
    }
}

//wf: why would a face use its parent's normal colour?
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
// TODO: edge lines for faces are drawn with setStyle(Qt::NoPen) and trigger this message.
//    Base::Console().Warning("QGIPP::setStyle(Qt: %d) is deprecated. Use setLinePen instead\n", s);
    m_styleNormal = s;
    m_styleCurrent = s;
}

void QGIPrimPath::setStyle(int s)
{
// TODO: edge lines for faces are drawn with setStyle(Qt::NoPen) and trigger this message.
//    Base::Console().Warning("QGIPP::setStyle(int: %d) is deprecated. Use setLinePen instead\n", s);
    m_styleCurrent = static_cast<Qt::PenStyle>(s);
    m_styleNormal = static_cast<Qt::PenStyle>(s);
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
    return (Qt::PenCapStyle)Preferences::LineCapStyle();
}

void QGIPrimPath::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Qt::KeyboardModifiers originalModifiers = event->modifiers();
    if (event->button()&Qt::LeftButton) {
        multiselectActivated = false;
    }

    if (event->button() == Qt::LeftButton
        && multiselectEligible()
        && PreferencesGui::multiSelection()) {

        auto parent = dynamic_cast<QGIView *>(parentItem());
        if (parent) {
            std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
            if (DGU::findObjectInSelection(selection, *(parent->getViewObject()))) {
                // if our parent is already in the selection, then allow addition
                // primitives to be selected.
                multiselectActivated = true;
                event->setModifiers(originalModifiers | Qt::ControlModifier);
            }
        }
    }

    QGraphicsPathItem::mousePressEvent(event);

    event->setModifiers(originalModifiers);
}

void QGIPrimPath::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Qt::KeyboardModifiers originalModifiers = event->modifiers();
    if ((event->button()&Qt::LeftButton) && multiselectActivated) {
        if (PreferencesGui::multiSelection()) {
            event->setModifiers(originalModifiers | Qt::ControlModifier);
        }

        multiselectActivated = false;
    }

    QGraphicsPathItem::mouseReleaseEvent(event);

    event->setModifiers(originalModifiers);
}

void QGIPrimPath::setFill(QColor c, Qt::BrushStyle s) {
    setFillColor(c);
    m_fillNormal = s;
    m_fillStyleCurrent = s;
}

void QGIPrimPath::setFill(QBrush b) {
    setFillColor(b.color());
    m_fillNormal = b.style();
    m_fillStyleCurrent = b.style();
}

void QGIPrimPath::resetFill() {
    m_colNormalFill = m_colDefFill;
    m_fillNormal = m_fillDef;
    m_fillStyleCurrent = m_fillDef;
}

//set PlainFill
void QGIPrimPath::setFillColor(QColor c)
{
    m_colNormalFill = c;
    m_fillColorCurrent = m_colNormalFill;
}

void QGIPrimPath::setCurrentPen()
{
    m_pen.setWidthF(m_width);
    m_pen.setColor(m_colCurrent);
    m_pen.setStyle(m_styleCurrent);
}

void QGIPrimPath::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    setCurrentPen();
    setPen(m_pen);

    m_brush.setColor(m_fillColorCurrent);
    m_brush.setStyle(m_fillStyleCurrent);
    setBrush(m_brush);

    QGraphicsPathItem::paint (painter, &myOption, widget);
}

