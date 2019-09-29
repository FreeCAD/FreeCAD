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
#include <assert.h>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMouseEvent>
#include <QPaintDevice>
#include <QPainter>
#include <QPrinter>
#include <QSvgGenerator>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <qmath.h>
#include <QRectF>
#include "Rez.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"
#include "QGICMark.h"
#include "QGIView.h"
#include "QGCustomText.h"

using namespace TechDrawGui;

QGCustomText::QGCustomText(QGraphicsItem* parent) :
    QGraphicsTextItem(parent)
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    isHighlighted = false;
    m_colCurrent = getNormalColor();
    m_colNormal  = m_colCurrent;
}

void QGCustomText::centerAt(QPointF centerPos)
{
      centerAt(centerPos.x(),centerPos.y());
}

void QGCustomText::centerAt(double cX, double cY)
{
    QRectF box = boundingRect();
    double width = box.width();
    double height = box.height();
    double newX = cX - width/2.;
    double newY = cY - height/2.;
    setPos(newX,newY);
}

void QGCustomText::justifyLeftAt(QPointF centerPos, bool vCenter)
{
    justifyLeftAt(centerPos.x(),centerPos.y(), vCenter);
}

void QGCustomText::justifyLeftAt(double cX, double cY, bool vCenter)
{
    QRectF box = boundingRect();
    double height = box.height();
    double newY = cY - height;
    if (vCenter) {
        newY = cY - height/2.;
    }
    setPos(cX,newY);
}

void QGCustomText::justifyRightAt(QPointF centerPos, bool vCenter)
{
    justifyRightAt(centerPos.x(),centerPos.y(), vCenter);
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
    setPos(newX,newY);
}

double QGCustomText::getHeight(void)
{
    return boundingRect().height();
}

double QGCustomText::getWidth(void)
{
    return boundingRect().width();
}
QVariant QGCustomText::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().Message("QGCT::itemChange - this: %X change: %d\n", this, change);
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
    if(!isSelected() && !isHighlighted) {
        setPrettyNormal();
    }
    QGraphicsTextItem::hoverLeaveEvent(event);
}

void QGCustomText::setPrettyNormal() {
    m_colCurrent = m_colNormal;
    setDefaultTextColor(m_colCurrent);
    update();
}

void QGCustomText::setPrettyPre() {
    m_colCurrent = getPreColor();
    setDefaultTextColor(m_colCurrent);
    update();
}

void QGCustomText::setPrettySel() {
    m_colCurrent = getSelectColor();
    setDefaultTextColor(m_colCurrent);
    update();
}

void QGCustomText::setColor(QColor c)
{
    m_colNormal = c;
    m_colCurrent = c;
    QGraphicsTextItem::setDefaultTextColor(c);
 }

void QGCustomText::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->setPen(Qt::green);
//    painter->drawRect(boundingRect());          //good for debugging

    QGraphicsTextItem::paint (painter, &myOption, widget);
}

QColor QGCustomText::getNormalColor()    //preference!
{
    QColor result;
    Base::Reference<ParameterGrp> hGrp = getParmGroup();
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("NormalColor", 0x00000000));
    result = fcColor.asValue<QColor>();
    return result;
}

QColor QGCustomText::getPreColor()
{
    Base::Reference<ParameterGrp> hGrp = getParmGroup();
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", 0xFFFF0000));
    return fcColor.asValue<QColor>();
}

QColor QGCustomText::getSelectColor()
{
    Base::Reference<ParameterGrp> hGrp = getParmGroup();
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", 0x00FF0000));
    return fcColor.asValue<QColor>();
}

Base::Reference<ParameterGrp> QGCustomText::getParmGroup()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    return hGrp;
}

void QGCustomText::makeMark(double x, double y)
{
    QGICMark* cmItem = new QGICMark(-1);
    cmItem->setParentItem(this);
    cmItem->setPos(x,y);
    cmItem->setThick(1.0);
    cmItem->setSize(40.0);
    cmItem->setZValue(ZVALUE::VERTEX);
}

void QGCustomText::makeMark(Base::Vector3d v)
{
    makeMark(v.x,v.y);
}


