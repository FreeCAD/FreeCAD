/***************************************************************************
 *   Copyright (c) 2016 Wandererfan <WandererFan@gmail.com>                *
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
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "QGICMark.h"

using namespace TechDrawGui;

QGICMark::QGICMark(int index) : QGIVertex(index)
{
    m_size = 3.0;
    m_width = 0.75;
    draw();
}
void QGICMark::draw(void)
{
    QPainterPath cmPath;
    cmPath.moveTo(0.0,m_size);
    cmPath.lineTo(0.0,-m_size);
    cmPath.moveTo(m_size,0.0);
    cmPath.lineTo(-m_size,0.0);
    setPath(cmPath);
}

void QGICMark::setSize(float s)
{
    m_size = s;
    draw();
}

void QGICMark::setThick(float t)
{
    m_width = t;
    draw();
}

QColor QGICMark::getCMarkColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("CMarkColor", 0x08080800));
    return fcColor.asValue<QColor>();
}

void QGICMark::setPrettyNormal() {
    m_colCurrent = getCMarkColor();
    update();
}

void QGICMark::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QGIVertex::paint (painter, &myOption, widget);
}

QRectF QGICMark::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGICMark::shape() const
{
    QPainterPath outline;
    QPainterPathStroker stroker;
    stroker.setWidth(getMarkFuzz());
    outline = stroker.createStroke(path());
    return outline;
}

 double QGICMark::getMarkFuzz(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double result = hGrp->GetFloat("MarkFuzz",20.0);
    return result;
}

