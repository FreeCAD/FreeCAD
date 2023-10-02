/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

// a class for converting geometry into QPainterPaths


#ifndef PATHBUILDER_H
#define PATHBUILDER_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Mod/TechDraw/App/Geometry.h>

#include "QGIViewPart.h"

namespace TechDrawGui {

class TechDrawGuiExport PathBuilder {
public:
    PathBuilder() {}
    PathBuilder(QGIViewPart* qvp) { m_qvp = qvp; }
    ~PathBuilder() = default;

    QPainterPath geomToPainterPath(TechDraw::BaseGeomPtr baseGeom, double rot) const;
    void pathArc(QPainterPath& path, double rx, double ry, double x_axis_rotation,
                          bool large_arc_flag, bool sweep_flag, double x, double y, double curx,
                          double cury) const;
    void pathArcSegment(QPainterPath& path, double xc, double yc, double th0, double th1,
                                 double rx, double ry, double xAxisRotation) const;

private:
    QGIViewPart* m_qvp;

};

} //end namespace TechDraw
#endif
