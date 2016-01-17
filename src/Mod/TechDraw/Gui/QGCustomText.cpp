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

void QGCustomText::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //svg text is much larger than screen text.  scene units(mm) vs points.
    //need to scale text if going to svg.
    //TODO: magic translation happens? approx: right ~8mm  down: 12mm + (3mm per mm of text height)
    //SVG transform matrix translation values are different for same font size + different fonts (osifont vs Ubuntu vs Arial)???
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

    QGraphicsTextItem::paint (painter, &myOption, widget);
}
