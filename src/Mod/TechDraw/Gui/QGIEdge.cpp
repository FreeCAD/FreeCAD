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
#include <assert.h>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <qmath.h>
#include "QGIView.h"
#include "QGIEdge.h"

using namespace TechDrawGui;

QGIEdge::QGIEdge(int index) :
    projIndex(index)
{
    strokeWidth = 1.;

    isCosmetic    = false;
    m_pen.setCosmetic(isCosmetic);
    isHiddenEdge = false;
    isSmoothEdge = false;

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    m_defNormal = m_colNormal;
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("HiddenColor", 0x08080800));
    m_colHid = fcColor.asValue<QColor>();

    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");
    m_styleHid = static_cast<Qt::PenStyle> (hGrp->GetInt("HiddenLine",2));

    //m_pen.setStyle(Qt::SolidLine);
    //m_pen.setCapStyle(Qt::RoundCap);
}

QRectF QGIEdge::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGIEdge::shape() const
{
    QPainterPath outline;
    QPainterPathStroker stroker;
    stroker.setWidth(2.0);
    outline = stroker.createStroke(path());
    return outline;
}


void QGIEdge::setCosmetic(bool state)
{
    isCosmetic = state;
    m_pen.setCosmetic(state);
    update();
}

void QGIEdge::setStrokeWidth(float width) {
    strokeWidth = width;
    update();
}

void QGIEdge::setHiddenEdge(bool b) {
    isHiddenEdge = b;
    if (b) {
        m_pen.setStyle(m_styleHid);
        m_colNormal = m_colHid;
    } else {
        m_pen.setStyle(Qt::SolidLine);
        m_colNormal = m_defNormal;
    }
    update();
}

void QGIEdge::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    m_pen.setWidthF(strokeWidth);
    QGIPrimPath::paint (painter, &myOption, widget);
}
