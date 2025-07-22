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

#include <Gui/Selection/Selection.h>

#include <Mod/TechDraw/App/DrawView.h>

#include "QGIPrimPath.h"
#include "PreferencesGui.h"
#include "QGIView.h"
#include "DrawGuiUtil.h"


using namespace TechDrawGui;
using namespace TechDraw;
using DGU = DrawGuiUtil;

QGIPrimPath::QGIPrimPath():
    m_brush(Qt::NoBrush)
{
    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, true);      // to get key press events

    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);

    multiselectActivated = false;

    m_colNormal = getNormalColor();
    m_pen.setColor(m_colNormal);
    m_styleNormal = Qt::SolidLine;
    m_pen.setStyle(m_styleNormal);
    m_pen.setCapStyle(prefCapStyle());
    m_pen.setWidthF(0);

    m_fillNormal = getDefaultFillStyle();
    m_brush.setStyle(m_fillNormal);

    setFillColor(getDefaultFillColor());

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

    m_pen.setColor(m_colNormal);
    m_brush.setColor(m_colNormalFill);
}

void QGIPrimPath::setPrettyPre() {
    m_pen.setColor(getPreColor());
    m_brush.setColor(getPreColor());
}

void QGIPrimPath::setPrettySel() {
    m_pen.setColor(getSelectColor());
    m_brush.setColor(getSelectColor());
}

//wf: why would a face use its parent's normal colour?
//this always goes to parameter
QColor QGIPrimPath::getNormalColor()
{
    QGIView *parent;
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
//    Base::Console().message("QGIPP::setWidth(%.3f)\n", w);
    m_pen.setWidthF(w);
}

void QGIPrimPath::setStyle(Qt::PenStyle s)
{
// TODO: edge lines for faces are drawn with setStyle(Qt::NoPen) and trigger this message.
//    Base::Console().warning("QGIPP::setStyle(Qt: %d) is deprecated. Use setLinePen instead\n", s);
    m_styleNormal = s;
    m_pen.setStyle(s);
}

void QGIPrimPath::setStyle(int s)
{
// TODO: edge lines for faces are drawn with setStyle(Qt::NoPen) and trigger this message.
//    Base::Console().warning("QGIPP::setStyle(int: %d) is deprecated. Use setLinePen instead\n", s);
    m_styleNormal = static_cast<Qt::PenStyle>(s);
    m_pen.setStyle(m_styleNormal);
}

void QGIPrimPath::setNormalColor(QColor c)
{
    m_colNormal = c;
    m_pen.setColor(m_colNormal);
}

void QGIPrimPath::setCapStyle(Qt::PenCapStyle c)
{
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
    m_brush.setStyle(s);
}

void QGIPrimPath::setFill(QBrush b) {
    m_fillNormal = b.style();
    setFillColor(b.color());
    m_brush.setStyle(b.style());
}

void QGIPrimPath::resetFill() {
    m_colNormalFill = getDefaultFillColor();
    m_fillNormal = getDefaultFillStyle();
    m_brush.setStyle(m_fillNormal);
}

//set PlainFill
void QGIPrimPath::setFillColor(QColor c)
{
    m_colNormalFill = c;
}

void QGIPrimPath::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    setPen(m_pen);
    setBrush(m_brush);

    QGraphicsPathItem::paint (painter, &myOption, widget);
}

