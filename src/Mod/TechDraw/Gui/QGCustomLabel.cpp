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
#include "QGCustomLabel.h"

using namespace TechDrawGui;

QGCustomLabel::QGCustomLabel()
{
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
}

void QGCustomLabel::centerAt(QPointF centerPos)
{
    QRectF box = boundingRect();
    double width = box.width();
    double height = box.height();
    double newX = centerPos.x() - width/2.;
    double newY = centerPos.y() - height/2.;
    setPos(newX,newY);
}

void QGCustomLabel::centerAt(double cX, double cY)
{
    QRectF box = boundingRect();
    double width = box.width();
    double height = box.height();
    double newX = cX - width/2.;
    double newY = cY - height/2.;
    setPos(newX,newY);
}

void QGCustomLabel::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;
    
    //see QGCustomText for explanation of this code
    double dppt = 3.53;
    double svgMagicX = Rez::guiX(8.0);
    double svgMagicY = Rez::guiX(12.0);
    double svgMagicYoffset = Rez::guiX(3.0);
    double fontSize = Rez::appX(font().pointSizeF());
    double ty = svgMagicY + (svgMagicYoffset*fontSize)/dppt;
    QPointF svgMove(-svgMagicX/dppt,ty);

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    if (svg) {
        painter->scale(Rez::appX(dppt),Rez::appX(dppt));
        painter->translate(svgMove);
    } else {
        painter->scale(1.0,1.0);
    }
    
    QGraphicsTextItem::paint (painter, &myOption, widget);
}
