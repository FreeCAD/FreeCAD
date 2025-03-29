/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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

# include <QGraphicsSceneHoverEvent>
# include <QPainter>
# include <QRectF>
# include <QStyleOptionGraphicsItem>
#endif

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "QGCustomText.h"
#include "PreferencesGui.h"
#include "QGICMark.h"
#include "ZVALUE.h"

using namespace TechDraw;
using namespace TechDrawGui;

QGCustomText::QGCustomText(QGraphicsItem* parent) :
    QGraphicsTextItem(parent)
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    m_colNormal  = getNormalColor();
    tightBounding = false;
}

void QGCustomText::centerAt(QPointF centerPos)
{
      centerAt(centerPos.x(), centerPos.y());
}

void QGCustomText::centerAt(double cX, double cY)
{
    QRectF box = boundingRect();
    double width = box.width();
    double height = box.height();
    double newX = cX - width/2.;
    double newY = cY - height/2.;
    setPos(newX, newY);
}

void QGCustomText::justifyLeftAt(QPointF centerPos, bool vCenter)
{
    justifyLeftAt(centerPos.x(), centerPos.y(), vCenter);
}

void QGCustomText::justifyLeftAt(double cX, double cY, bool vCenter)
{
    QRectF box = boundingRect();
    double height = box.height();
    double newY = cY - height;
    if (vCenter) {
        newY = cY - height/2.;
    }
    setPos(cX, newY);
}

void QGCustomText::justifyRightAt(QPointF centerPos, bool vCenter)
{
    justifyRightAt(centerPos.x(), centerPos.y(), vCenter);
}

void QGCustomText::justifyRightAt(double cX, double cY, bool vCenter)
{
    QRectF box = boundingRect();
    double width = box.width();
    double height = box.height();
    double newX = cX - width;
    double newY = cY - height;
    if (vCenter) {
        newY = cY - height/2.;
    }
    setPos(newX, newY);
}

double QGCustomText::getHeight()
{
    return boundingRect().height();
}

double QGCustomText::getWidth()
{
    return boundingRect().width();
}
QVariant QGCustomText::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().message("QGCT::itemChange - this: %X change: %d\n", this, change);
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            setPrettySel();
        } else {
            setPrettyNormal();
        }
    }
    return QGraphicsTextItem::itemChange(change, value);
}


void QGCustomText::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (!isSelected()) {
        setPrettyPre();
    }
    QGraphicsTextItem::hoverEnterEvent(event);
}

void QGCustomText::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if(!isSelected()) {
        setPrettyNormal();
    }
    QGraphicsTextItem::hoverLeaveEvent(event);
}

void QGCustomText::setPrettyNormal() {
    setDefaultTextColor(m_colNormal);
    update();
}

void QGCustomText::setPrettyPre() {
    setDefaultTextColor(getPreColor());
    update();
}

void QGCustomText::setPrettySel() {
    setDefaultTextColor(getSelectColor());
    update();
}

void QGCustomText::setColor(QColor c)
{
    m_colNormal = c;
    QGraphicsTextItem::setDefaultTextColor(c);
}

void QGCustomText::setTightBounding(bool tight)
{
    tightBounding = tight;
}

void QGCustomText::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->setPen(Qt::green);
//    painter->drawRect(alignmentRect());          //good for debugging

    QGraphicsTextItem::paint (painter, &myOption, widget);
}

QRectF QGCustomText::tightBoundingRect() const
{
    QFontMetrics qfm(font());
    QRectF result = QGraphicsTextItem::boundingRect();
    QRectF tight = qfm.tightBoundingRect(toPlainText());
    qreal x_adj = (result.width() - tight.width())/4.0;
    qreal y_adj = (result.height() - tight.height())/4.0;

    // Adjust the bounding box 50% towards the Qt tightBoundingRect(),
    // except chomp some extra empty space above the font (1.75*y_adj)
    // wf: this does not work with all fonts.  it depends on where the glyph is located within
    //     the em square.  see https://github.com/FreeCAD/FreeCAD/issues/11452
    // TODO: how to know where the glyph is going to be placed?
    result.adjust(x_adj, 1.75*y_adj, -x_adj, -y_adj);

    return result;
}

//! a boundingRect for text alignment, that does not adversely affect rendering.
QRectF QGCustomText::alignmentRect() const
{
    if (tightBounding) {
        return tightBoundingRect();
    } else {
        return boundingRect();
    }
}

// Calculate the amount of difference between tight and relaxed bounding boxes
QPointF QGCustomText::tightBoundingAdjust() const
{
    QRectF original = QGraphicsTextItem::boundingRect();
    QRectF tight = tightBoundingRect();

    return QPointF(tight.x()-original.x(), tight.y()-original.y());
}

// TODO: when setting position, it doesn't take into account the tight bounding rect
// Meaning top left corner has distance to pos(0, 0)
// Here is a sketch for a fix
// Note that the position adjustment will have to carried out every time the font changes
// void QGCustomText::setPos(const QPointF &pos) {
//     if(tightBounding) {
//         QGraphicsTextItem::setPos(pos.x() - tightBoundingAdjust().x(), pos.y() - tightBoundingAdjust().y());
//         return;
//     }
//     QGraphicsTextItem::setPos(pos);
// }

// void QGCustomText::setPos(qreal x, qreal y) {
//     setPos(QPointF(x, y));
// }

// QPointF QGCustomText::pos() const
// {
//     // Native Qt pos function doesn't take into account the tight bounding rect
//     return boundingRect().topLeft();
// }

QColor QGCustomText::getNormalColor()    //preference!
{
    return PreferencesGui::normalQColor();
}

QColor QGCustomText::getPreColor()
{
    return PreferencesGui::preselectQColor();
}

QColor QGCustomText::getSelectColor()
{
    return PreferencesGui::selectQColor();
}

Base::Reference<ParameterGrp> QGCustomText::getParmGroup()
{
    return Preferences::getPreferenceGroup("Colors");
}

void QGCustomText::makeMark(double x, double y)
{
    QGICMark* cmItem = new QGICMark(-1);
    cmItem->setParentItem(this);
    cmItem->setPos(x, y);
    cmItem->setThick(1.0);
    cmItem->setSize(40.0);
    cmItem->setZValue(ZVALUE::VERTEX);
}

void QGCustomText::makeMark(Base::Vector3d v)
{
    makeMark(v.x, v.y);
}

