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
#include <cassert>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "PreferencesGui.h"
#include "QGIEdge.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIEdge::QGIEdge(int index) :
    projIndex(index),
    isCosmetic(false),
    isHiddenEdge(false),
    isSmoothEdge(false)
{
    m_width = 1.0;
    setCosmetic(isCosmetic);
    setFill(Qt::NoBrush);
}

//NOTE this refers to Qt cosmetic lines
void QGIEdge::setCosmetic(bool state)
{
//    Base::Console().Message("QGIE::setCosmetic(%d)\n", state);
    isCosmetic = state;
    if (state) {
        setWidth(0.0);
    }
}

void QGIEdge::setHiddenEdge(bool b) {
    isHiddenEdge = b;
    if (b) {
        m_styleCurrent = getHiddenStyle();
    } else {
        m_styleCurrent = Qt::SolidLine;
    }
}

void QGIEdge::setPrettyNormal() {
//    Base::Console().Message("QGIE::setPrettyNormal()\n");
    if (isHiddenEdge) {
        m_colCurrent = getHiddenColor();
    } else {
        m_colCurrent = getNormalColor();
    }
    //should call QGIPP::setPrettyNormal()?
}

QColor QGIEdge::getHiddenColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("HiddenColor", 0x000000FF));
    return fcColor.asValue<QColor>();
}

Qt::PenStyle QGIEdge::getHiddenStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    //Qt::PenStyle - NoPen, Solid, Dashed, ...
    //Preferences::General - Solid, Dashed                                     
    Qt::PenStyle hidStyle = static_cast<Qt::PenStyle> (hGrp->GetInt("HiddenLine",0) + 1);
    return hidStyle;
}

 double QGIEdge::getEdgeFuzz(void) const
{
    return PreferencesGui::edgeFuzz();
}


QRectF QGIEdge::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGIEdge::shape() const
{
    QPainterPath outline;
    QPainterPathStroker stroker;
    stroker.setWidth(getEdgeFuzz());
    outline = stroker.createStroke(path());
    return outline;
}

void QGIEdge::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //~ painter->drawRect(boundingRect());          //good for debugging

    QGIPrimPath::paint (painter, &myOption, widget);
}
