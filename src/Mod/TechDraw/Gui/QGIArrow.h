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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>

#include <Base/Vector3D.h>

#include "QGIPrimPath.h"
#include "QGIUserTypes.h"

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGIArrow : public QGIPrimPath
{
public:
    explicit QGIArrow();
    ~QGIArrow() override {}

    enum {Type = UserType::QGIArrow};
    int type() const override { return Type;}

public:
    void draw();
    bool isFlipped() { return m_flipped; }
    void setFlipped(bool flipped) { m_flipped = flipped; }
    void flip() { m_flipped = !m_flipped; }
    double getSize() { return m_size; }
    void setSize(double s);
    TechDraw::ArrowType getStyle() { return m_style; }
    void setStyle(TechDraw::ArrowType s) { m_style = s; }
    bool getDirMode() { return m_dirMode; }
    void setDirMode(bool b) { m_dirMode = b; }
    Base::Vector3d getDirection(void) { return m_flipped ? -m_dir : m_dir; }
    void setDirection(Base::Vector3d v) { m_dir = v; }
    void setDirection(double angle) { m_dir = Base::Vector3d(cos(angle), sin(angle), 0.0); }
    static TechDraw::ArrowType getPrefArrowStyle();
    static double getPrefArrowSize();
    static double getOverlapAdjust(TechDraw::ArrowType style, double size);

protected:
    QPainterPath makeFilledTriangle(double length, double width, bool flipped);
    QPainterPath makeFilledTriangle(Base::Vector3d dir, double length, double width);
    QPainterPath makeOpenArrow(double length, double width, bool flipped);
    QPainterPath makeOpenArrow(Base::Vector3d dir, double length, double width);
    QPainterPath makeHashMark(double length, double width, bool flipped);
    QPainterPath makeHashMark(Base::Vector3d dir, double length, double width);
    QPainterPath makeDot(double length, double width, bool flipped);
    QPainterPath makeOpenDot(double length, double width, bool flipped);
    QPainterPath makeForkArrow(double length, double width, bool flipped);
    QPainterPath makeForkArrow(Base::Vector3d dir, double length, double width);
    QPainterPath makePyramid(double length, bool flipped);
    QPainterPath makePyramid(Base::Vector3d dir, double length);

private:
    QBrush m_brush;
    Qt::BrushStyle m_fill;
    double m_size;
    TechDraw::ArrowType m_style;
    bool m_flipped;
    bool m_dirMode;
    Base::Vector3d m_dir;
};

}