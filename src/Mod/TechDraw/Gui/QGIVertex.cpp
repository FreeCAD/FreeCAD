/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

# include <QPainter>
# include <QPainterPath>
# include <QStyleOptionGraphicsItem>
#endif

#include "QGIVertex.h"
#include "PreferencesGui.h"
#include "QGIPrimPath.h"


using namespace TechDrawGui;

QGIVertex::QGIVertex(int index) :
    projIndex(index),
    m_radius(2)
{
    QColor vertexColor = PreferencesGui::vertexQColor();
    setFill(vertexColor, Qt::SolidPattern);

    setRadius(m_radius);
}

void QGIVertex::setRadius(float r)
{
    m_radius = r;
    QPainterPath p;
    p.addEllipse(-r/2.0, -r/2.0, r, r);
    setPath(p);
}

void QGIVertex::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->setPen(Qt::blue);
//    painter->drawRect(boundingRect());          //good for debugging

//    m_brush.setColor(m_colCurrent);
//    m_brush.setStyle(m_fill);
//    setBrush(m_brush);
    QGIPrimPath::paint (painter, &myOption, widget);
}
