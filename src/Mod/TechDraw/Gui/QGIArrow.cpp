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
#include <QMenu>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainterPathStroker>
#include <QPainter>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/Console.h>

#include "Rez.h"
#include "QGIArrow.h"

using namespace TechDrawGui;

QGIArrow::QGIArrow() :
    m_fill(Qt::SolidPattern),
    m_size(5.0),
    m_style(0),
    m_dirMode(false),
    m_dir(Base::Vector3d(1.0,0.0,0.0))
{
    isFlipped = false;
    m_brush.setStyle(m_fill);

    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
}


void QGIArrow::flip(bool state) {
    isFlipped = state;
}

void QGIArrow::draw() {
    QPainterPath path;
    if (m_style == 0) {
        if (m_dirMode) {
            path = makeFilledTriangle(getDirection(), m_size,m_size/6.0);
        } else {
            path = makeFilledTriangle(m_size,m_size/6.0,isFlipped);     //"arrow l/w sb 3/1" ??
        }
    } else if (m_style == 1) {
        if (m_dirMode) {
            path = makeOpenArrow(getDirection(), m_size,m_size/3.0);          //broad arrow?
        } else {
            path = makeOpenArrow(m_size,m_size/3.0,isFlipped);
        }
    } else if (m_style == 2) {
        if (m_dirMode) {
            path = makeHashMark(getDirection(), m_size/2.0,m_size/2.0);       //big enough?
        } else {
            path = makeHashMark(m_size/2.0,m_size/2.0,isFlipped);       //big enough?
        }
    } else if (m_style == 3) {
        path = makeDot(m_size/2.0,m_size/2.0,isFlipped);
    } else if (m_style == 4) {
        path = makeOpenDot(m_size/2.0,m_size/2.0,isFlipped);
    } else if (m_style == 5) {
        if (m_dirMode) {
            path = makeForkArrow(getDirection(), m_size/2.0,m_size/2.0);       //big enough?
        } else {
            path = makeForkArrow(m_size/2.0,m_size/2.0,isFlipped);       //big enough?
        }
    } else {
        path = makeFilledTriangle(m_size,m_size/6.0,isFlipped);     //sb a question mark or ???
    }
    setPath(path);
}

void QGIArrow::setSize(double s)
{
    m_size = s;
}


QPainterPath QGIArrow::makeFilledTriangle(double length, double width, bool flipped)
{
//(0,0) is tip of arrow
    if (!flipped) {
        length *= -1;
    }

    QPainterPath path;
    path.moveTo(QPointF(0.,0.));
    path.lineTo(QPointF(Rez::guiX(length),Rez::guiX(-width)));
    path.lineTo(QPointF(Rez::guiX(length),Rez::guiX(width)));
    path.closeSubpath();
    m_fill = Qt::SolidPattern;
    return path;
}

QPainterPath QGIArrow::makeFilledTriangle(Base::Vector3d dir, double length, double width)
{
//(0,0) is tip of arrow
// dir is direction arrow points
    Base::Vector3d negDir = -dir;
    negDir.Normalize();
    Base::Vector3d perp(-negDir.y,negDir.x, 0.0);
    Base::Vector3d barb1 = negDir * length + perp * width;
    Base::Vector3d barb2 = negDir * length - perp * width;
    
    QPainterPath path;
    path.moveTo(QPointF(0.,0.));
    path.lineTo(QPointF(Rez::guiX(barb1.x),Rez::guiX(barb1.y)));
    path.lineTo(QPointF(Rez::guiX(barb2.x),Rez::guiX(barb2.y)));
    path.closeSubpath();
    m_fill = Qt::SolidPattern;
    return path;
}

QPainterPath QGIArrow::makeOpenArrow(double length, double width, bool flipped)
{
//(0,0) is tip of arrow
    if (!flipped) {
        length *= -1;
    }

    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(length),Rez::guiX(-width)));
    path.lineTo(QPointF(0.,0.));
    path.lineTo(QPointF(Rez::guiX(length),Rez::guiX(width)));
    m_fill = Qt::NoBrush;
    return path;
}

QPainterPath QGIArrow::makeOpenArrow(Base::Vector3d dir, double length, double width)
{
//(0,0) is tip of arrow
    Base::Vector3d negDir = -dir;
    negDir.Normalize();
    Base::Vector3d perp(-negDir.y,negDir.x, 0.0);
    Base::Vector3d barb1 = negDir * length + perp * width;
    Base::Vector3d barb2 = negDir * length - perp * width;
    
    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(barb1.x),Rez::guiX(barb1.y)));
    path.lineTo(QPointF(0.,0.));
    path.lineTo(QPointF(Rez::guiX(barb2.x),Rez::guiX(barb2.y)));
    m_fill = Qt::NoBrush;
    return path;
}


QPainterPath QGIArrow::makeHashMark(double length, double width, bool flipped)   //Arch tick
{
    double adjWidth = 1.0;
//(0,0) is tip of arrow
    if (!flipped) {
        length *= -1;
        adjWidth *= -1;
    }
    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(length),Rez::guiX(adjWidth * (-width))));
    path.lineTo(QPointF(Rez::guiX(-length),Rez::guiX(adjWidth * width)));
    m_fill = Qt::NoBrush;
    return path;
}

QPainterPath QGIArrow::makeHashMark(Base::Vector3d dir, double length, double width)   //Arch tick
{
    double adjWidth = 1.0;
    Base::Vector3d negDir = -dir;
    Base::Vector3d normDir = dir;
    negDir.Normalize();
    normDir.Normalize();
    Base::Vector3d perp(-negDir.y,negDir.x, 0.0);
    Base::Vector3d barb1 = negDir * length - perp * (adjWidth * width);
    Base::Vector3d barb2 = normDir * length + perp * (adjWidth * width);
    
    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(barb1.x),Rez::guiX(barb1.y)));
    path.lineTo(QPointF(Rez::guiX(barb2.x),Rez::guiX(barb2.y)));
    m_fill = Qt::NoBrush;
    return path;
}

QPainterPath QGIArrow::makeDot(double length, double width, bool flipped)   //closed dot
{
    Q_UNUSED(flipped);
    QPainterPath path;
    path.moveTo(0.0,0.0);                                  ////(0,0) is Center of dot
    path.addEllipse(Rez::guiX(-length/2.0), Rez::guiX(-width/2.0), Rez::guiX(length), Rez::guiX(width));
    m_fill = Qt::SolidPattern;
    return path;
}

QPainterPath QGIArrow::makeOpenDot(double length, double width, bool flipped)
{
    Q_UNUSED(flipped);
    QPainterPath path;
    path.moveTo(0.0,0.0);                                  ////(0,0) is Center of dot
    path.addEllipse(Rez::guiX(-length/2.0), Rez::guiX(-width/2.0), Rez::guiX(length), Rez::guiX(width));
    m_fill = Qt::NoBrush;
    return path;
}

QPainterPath QGIArrow::makeForkArrow(double length, double width, bool flipped)
{
//(0,0) is tip of arrow
    if (flipped) {
        length *= -1;
    }

    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(length),Rez::guiX(-width)));
    path.lineTo(QPointF(0.,0.));
    path.lineTo(QPointF(Rez::guiX(length),Rez::guiX(width)));
    m_fill = Qt::NoBrush;
    return path;
}

QPainterPath QGIArrow::makeForkArrow(Base::Vector3d dir, double length, double width)
{
//(0,0) is tip of arrow
    Base::Vector3d negDir = -dir;
    Base::Vector3d normDir = dir;
    negDir.Normalize();
    normDir.Normalize();
    Base::Vector3d perp(-normDir.y,normDir.x, 0.0);
    Base::Vector3d barb1 = normDir * length + perp * width;
    Base::Vector3d barb2 = normDir * length - perp * width;
    
    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(barb1.x),Rez::guiX(barb1.y)));
    path.lineTo(QPointF(0.,0.));
    path.lineTo(QPointF(Rez::guiX(barb2.x),Rez::guiX(barb2.y)));
    m_fill = Qt::NoBrush;
    return path;
}



int QGIArrow::getPrefArrowStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    int style = hGrp->GetInt("ArrowStyle", 0);
    return style;
}

double QGIArrow::getPrefArrowSize()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    double style = hGrp->GetFloat("ArrowSize", 3.5);
    return style;
}

double QGIArrow::getOverlapAdjust(int style, double size)
{
    // adjustment required depends on arrow size and type! :(
    // ex for fork and tick, adjustment sb zero. 0.25 is good for filled triangle, 0.1 for open arrow.
    // open circle sb = radius
    // NOTE: this may need to be adjusted to account for line thickness too.
//    Base::Console().Message("QGIA::getOverlapAdjust(%d, %.3f) \n",style, size);
    double result = 1.0;
    switch(style) {
        case 0:         //filled triangle
            result = 0.50 * size;
            break;
        case 1:         //open arrow
            result = 0.10 * size;
            break;
        case 2:         //hash mark
            result = 0.0;
            break;
        case 3:         //dot
            result = 0.0;
            break;
        case 4:         //open circle
                        //diameter is size/2 so radius is size/4
            result = 0.25 * size;
            break;
        case 5:         //fork
            result = 0.0;
            break;
        default:        //unknown
            result = 1.0;
    }
    return result;
}

void QGIArrow::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    setPen(m_pen);
    m_brush.setColor(m_colCurrent);
    m_brush.setStyle(m_fill);
    setBrush(m_brush);
    QGIPrimPath::paint (painter, &myOption, widget);
}
