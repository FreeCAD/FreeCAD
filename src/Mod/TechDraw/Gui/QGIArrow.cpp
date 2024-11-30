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
# include <QPainterPath>
#endif

#include <Mod/TechDraw/App/ArrowPropEnum.h>

#include "QGIArrow.h"
#include "PreferencesGui.h"
#include "Rez.h"


using namespace TechDrawGui;
using namespace TechDraw;

QGIArrow::QGIArrow() :
    m_fill(Qt::SolidPattern),
    m_size(getPrefArrowSize()),
    m_style(0),
    m_dirMode(false),
    m_dir(Base::Vector3d(1.0, 0.0, 0.0))
{
    setFlipped(false);
    setFillStyle(Qt::SolidPattern);
    m_brush.setStyle(m_fill);
    m_colDefFill = getNormalColor();
    m_colNormalFill = m_colDefFill;

    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
}

void QGIArrow::draw() {
    QPainterPath path;
    if (m_style == ArrowType::FILLED_ARROW) {
        if (m_dirMode) {
            path = makeFilledTriangle(getDirection(), m_size, m_size/6.0);
        } else {
            path = makeFilledTriangle(m_size, m_size/6.0, isFlipped());     //"arrow l/w sb 3/1" ??
        }
    } else if (m_style == ArrowType::OPEN_ARROW) {
        if (m_dirMode) {
            path = makeOpenArrow(getDirection(), m_size, m_size/3.0);          //broad arrow?
        } else {
            path = makeOpenArrow(m_size, m_size/3.0, isFlipped());
        }
    } else if (m_style == ArrowType::TICK) {
        if (m_dirMode) {
            path = makeHashMark(getDirection(), m_size/2.0, m_size/2.0);       //big enough?
        } else {
            path = makeHashMark(m_size/2.0, m_size/2.0, isFlipped());       //big enough?
        }
    } else if (m_style == ArrowType::DOT) {
        path = makeDot(m_size/2.0, m_size/2.0, isFlipped());
    } else if (m_style == ArrowType::OPEN_CIRCLE) {
        path = makeOpenDot(m_size/2.0, m_size/2.0, isFlipped());
    } else if (m_style == ArrowType::FORK) {
        if (m_dirMode) {
            path = makeForkArrow(getDirection(), m_size/2.0, m_size/2.0);       //big enough?
        } else {
            path = makeForkArrow(m_size/2.0, m_size/2.0, isFlipped());       //big enough?
        }
    } else if (m_style == ArrowType::FILLED_TRIANGLE){
        if (m_dirMode) {
            path = makePyramid(getDirection(), m_size);
        } else {
            path = makePyramid(m_size, isFlipped());
        }
    }else {
        path = makeFilledTriangle(m_size, m_size/6.0, isFlipped());     //sb a question mark or ???
    }
    setPath(path);
}

void QGIArrow::setSize(double s)
{
    m_size = s;
}


QPainterPath QGIArrow::makeFilledTriangle(double length, double width, bool flipped)
{
//(0, 0) is tip of arrow
    if (!flipped) {
        length *= -1;
    }

    QPainterPath path;
    path.moveTo(QPointF(0., 0.));
    path.lineTo(QPointF(Rez::guiX(length), Rez::guiX(-width)));
    path.lineTo(QPointF(Rez::guiX(length), Rez::guiX(width)));
    path.closeSubpath();
    setFillStyle(Qt::SolidPattern);
    return path;
}

QPainterPath QGIArrow::makeFilledTriangle(Base::Vector3d dir, double length, double width)
{
//(0, 0) is tip of arrow
// dir is direction arrow points
    Base::Vector3d negDir = -dir;
    negDir.Normalize();
    Base::Vector3d perp(-negDir.y, negDir.x, 0.0);
    Base::Vector3d barb1 = negDir * length + perp * width;
    Base::Vector3d barb2 = negDir * length - perp * width;

    QPainterPath path;
    path.moveTo(QPointF(0., 0.));
    path.lineTo(QPointF(Rez::guiX(barb1.x), Rez::guiX(barb1.y)));
    path.lineTo(QPointF(Rez::guiX(barb2.x), Rez::guiX(barb2.y)));
    path.closeSubpath();
    setFillStyle(Qt::SolidPattern);
    return path;
}

QPainterPath QGIArrow::makeOpenArrow(double length, double width, bool flipped)
{
//(0, 0) is tip of arrow
    if (!flipped) {
        length *= -1;
    }

    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(length), Rez::guiX(-width)));
    path.lineTo(QPointF(0., 0.));
    path.lineTo(QPointF(Rez::guiX(length), Rez::guiX(width)));
    setFillStyle(Qt::NoBrush);
    return path;
}

QPainterPath QGIArrow::makeOpenArrow(Base::Vector3d dir, double length, double width)
{
//(0, 0) is tip of arrow
    Base::Vector3d negDir = -dir;
    negDir.Normalize();
    Base::Vector3d perp(-negDir.y, negDir.x, 0.0);
    Base::Vector3d barb1 = negDir * length + perp * width;
    Base::Vector3d barb2 = negDir * length - perp * width;

    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(barb1.x), Rez::guiX(barb1.y)));
    path.lineTo(QPointF(0., 0.));
    path.lineTo(QPointF(Rez::guiX(barb2.x), Rez::guiX(barb2.y)));
    setFillStyle(Qt::NoBrush);
    return path;
}


QPainterPath QGIArrow::makeHashMark(double length, double width, bool flipped)   //Arch tick
{
    double adjWidth = 1.0;
//(0, 0) is tip of arrow
    if (!flipped) {
        length *= -1;
        adjWidth *= -1;
    }
    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(length), Rez::guiX(adjWidth * (-width))));
    path.lineTo(QPointF(Rez::guiX(-length), Rez::guiX(adjWidth * width)));
    setFillStyle(Qt::NoBrush);
    return path;
}

QPainterPath QGIArrow::makeHashMark(Base::Vector3d dir, double length, double width)   //Arch tick
{
    double adjWidth = 1.0;
    Base::Vector3d negDir = -dir;
    Base::Vector3d normDir = dir;
    negDir.Normalize();
    normDir.Normalize();
    Base::Vector3d perp(-negDir.y, negDir.x, 0.0);
    Base::Vector3d barb1 = negDir * length - perp * (adjWidth * width);
    Base::Vector3d barb2 = normDir * length + perp * (adjWidth * width);

    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(barb1.x), Rez::guiX(barb1.y)));
    path.lineTo(QPointF(Rez::guiX(barb2.x), Rez::guiX(barb2.y)));
    setFillStyle(Qt::NoBrush);
    return path;
}

QPainterPath QGIArrow::makeDot(double length, double width, bool flipped)   //closed dot
{
    Q_UNUSED(flipped);
    QPainterPath path;
    path.moveTo(0.0, 0.0);                                  ////(0, 0) is Center of dot
    path.addEllipse(Rez::guiX(-length/2.0), Rez::guiX(-width/2.0), Rez::guiX(length), Rez::guiX(width));
    setFillStyle(Qt::SolidPattern);
    return path;
}

QPainterPath QGIArrow::makeOpenDot(double length, double width, bool flipped)
{
    Q_UNUSED(flipped);
    QPainterPath path;
    path.moveTo(0.0, 0.0);                                  ////(0, 0) is Center of dot
    path.addEllipse(Rez::guiX(-length/2.0), Rez::guiX(-width/2.0), Rez::guiX(length), Rez::guiX(width));
    setFillStyle(Qt::NoBrush);
    return path;
}

QPainterPath QGIArrow::makeForkArrow(double length, double width, bool flipped)
{
//(0, 0) is tip of arrow
    if (flipped) {
        length *= -1;
    }

    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(length), Rez::guiX(-width)));
    path.lineTo(QPointF(0., 0.));
    path.lineTo(QPointF(Rez::guiX(length), Rez::guiX(width)));
    setFillStyle(Qt::NoBrush);
    return path;
}

QPainterPath QGIArrow::makeForkArrow(Base::Vector3d dir, double length, double width)
{
//(0, 0) is tip of arrow
    Base::Vector3d negDir = -dir;
    Base::Vector3d normDir = dir;
    negDir.Normalize();
    normDir.Normalize();
    Base::Vector3d perp(-normDir.y, normDir.x, 0.0);
    Base::Vector3d barb1 = normDir * length + perp * width;
    Base::Vector3d barb2 = normDir * length - perp * width;

    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(barb1.x), Rez::guiX(barb1.y)));
    path.lineTo(QPointF(0., 0.));
    path.lineTo(QPointF(Rez::guiX(barb2.x), Rez::guiX(barb2.y)));
    setFillStyle(Qt::NoBrush);
    return path;
}

QPainterPath QGIArrow::makePyramid(double length, bool flipped)
{
    double half_width = length/2.;
    double top = -length;
    double base = 0.;
    // [(0, -width), (0, width)] is base of arrow
    if (flipped) {
        top = 0.;
        base = -length;
    }
    top = Rez::guiX(top);
    base = Rez::guiX(base);
    QPainterPath path;
    path.moveTo(QPointF(top, 0.));
    path.lineTo(QPointF(base, Rez::guiX(-half_width)));
    path.lineTo(QPointF(base, Rez::guiX(half_width)));
    path.closeSubpath();
    setFillStyle(Qt::SolidPattern);
    return path;
}

QPainterPath QGIArrow::makePyramid(Base::Vector3d dir, double length)
{
    //(0, 0) is tip of arrow
    // dir is direction arrow points
    Base::Vector3d negDir = -dir;
    negDir.Normalize();
    double width = length / 2.;
    Base::Vector3d perp(-negDir.y, negDir.x, 0.0);
    Base::Vector3d barb1 = perp * width;
    Base::Vector3d barb2 = perp * -width;
    Base::Vector3d top = negDir * length;

    QPainterPath path;
    path.moveTo(QPointF(Rez::guiX(top.x), Rez::guiX(top.y)));
    path.lineTo(QPointF(Rez::guiX(barb1.x), Rez::guiX(barb1.y)));
    path.lineTo(QPointF(Rez::guiX(barb2.x), Rez::guiX(barb2.y)));
    path.closeSubpath();
    setFillStyle(Qt::SolidPattern);
    return path;
}

int QGIArrow::getPrefArrowStyle()
{
    return PreferencesGui::dimArrowStyle();
}

double QGIArrow::getPrefArrowSize()
{
    return PreferencesGui::dimArrowSize();
}

double QGIArrow::getOverlapAdjust(int style, double size)
{
    // adjustment required depends on arrow size and type! :(
    // ex for fork and tick, adjustment sb zero. 0.25 is good for filled triangle, 0.1 for open arrow.
    // open circle sb = radius
    // NOTE: this may need to be adjusted to account for line thickness too.
//    Base::Console().Message("QGIA::getOverlapAdjust(%d, %.3f) \n", style, size);
    switch(style) {
        case FILLED_ARROW:
            return 0.50 * size;
        case OPEN_ARROW:
            return 0.10 * size;
        case TICK:
            return 0.0;
        case DOT:
            return 0.0;
        case OPEN_CIRCLE:
                        //diameter is size/2 so radius is size/4
            return 0.25 * size;
        case FORK:
            return 0.0;
        case FILLED_TRIANGLE:
            return size;
        case NONE:
            return 0.0;
    }
    return 1.0;  // Unknown
}
