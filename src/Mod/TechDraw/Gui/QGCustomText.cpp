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
#include "QGCustomText.h"

using namespace TechDrawGui;

QGCustomText::QGCustomText()
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    isHighlighted = false;
    m_colCurrent = getNormalColor();
}

void QGCustomText::centerAt(QPointF centerPos)
{
    QRectF box = boundingRect();
    double width = box.width();
    double height = box.height();
    double newX = centerPos.x() - width/2.;
    double newY = centerPos.y() - height/2.;
    setPos(newX,newY);
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

QVariant QGCustomText::itemChange(GraphicsItemChange change, const QVariant &value)
{
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
    m_colCurrent = getNormalColor();
    update();
}

void QGCustomText::setPrettyPre() {
    m_colCurrent = getPreColor();
    update();
}

void QGCustomText::setPrettySel() {
    m_colCurrent = getSelectColor();
    update();
}

void QGCustomText::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //svg text is much larger than screen text.  scene units(mm) vs points.
    //need to scale text if going to svg.
    //TODO: magic translation happens? approx: right ~8mm  down: 12mm + (3mm per mm of text height)
    //SVG transform matrix translation values are different for same font size + different fonts (Sans vs Ubuntu vs Arial)???
    //                     scale values are same for same font size + different fonts.
    //double svgScale = 2.835;      //72dpi/(25.4mm/in)
    //double svgScale = 3.84;       //96dpi/(25mm/in)
    double svgScale = 2.88;         //72dpi/(25mm/in)   Qt logicalDpiY() is int
    double svgMagicX = 8.0;
    //double svgMagicY = 7.5;        //idk
    double fontSize = font().pointSizeF();
    //double ty = (12.0/svgScale + 3.0*fontSize/svgScale) + (svgMagicY/svgScale);
    double ty = (12.0/svgScale + 3.0*fontSize/svgScale);
    QPointF svgMove(-svgMagicX/svgScale,-ty);

    QPaintDevice* hw = painter->device();
    //QPaintDeviceMetrics hwm(hw);
    //QPrinter* pr = dynamic_cast<QPrinter*>(hw);                      //printer does not rescale vs screen?
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    if (svg) {
        painter->scale(svgScale,svgScale);
        painter->translate(svgMove);
    } else {
        painter->scale(1.0,1.0);
    }

    setDefaultTextColor(m_colCurrent);
    QGraphicsTextItem::paint (painter, &myOption, widget);
}

QColor QGCustomText::getNormalColor()
{
    Base::Reference<ParameterGrp> hGrp = getParmGroup();
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("NormalColor", 0x00000000));
    return fcColor.asValue<QColor>();
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
