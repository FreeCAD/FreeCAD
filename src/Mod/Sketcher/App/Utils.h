/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHER_Utils_H
#define SKETCHER_Utils_H

#include <Base/Tools.h>
#include <Base/Tools2D.h>


namespace Part
{
class Geometry;
}

namespace Sketcher
{
enum class PointPos : int;
class SketchObject;

bool isCircle(const Part::Geometry&);
bool isArcOfCircle(const Part::Geometry&);
bool isEllipse(const Part::Geometry&);
bool isArcOfEllipse(const Part::Geometry&);
bool isLineSegment(const Part::Geometry&);
bool isArcOfHyperbola(const Part::Geometry&);
bool isArcOfParabola(const Part::Geometry&);
bool isBSplineCurve(const Part::Geometry&);
bool isPoint(const Part::Geometry&);

Base::Vector3d getPoint3d(const Part::Geometry* geo, PointPos PosId);
Base::Vector2d getPoint2d(const Part::Geometry* geo, PointPos PosId);
Base::Vector2d startPoint2d(const Part::Geometry* geo);
Base::Vector2d endPoint2d(const Part::Geometry* geo);
Base::Vector2d centerPoint2d(const Part::Geometry* geo);
Base::Vector3d startPoint3d(const Part::Geometry* geo);
Base::Vector3d endPoint3d(const Part::Geometry* geo);
Base::Vector3d centerPoint3d(const Part::Geometry* geo);

double getRadius(const Part::Geometry* geo);

}  // namespace Sketcher

/// converts a 2D vector into a 3D vector in the XY plane
inline Base::Vector3d toVector3d(const Base::Vector2d& vector2d)
{
    return Base::Vector3d(vector2d.x, vector2d.y, 0.);
}

/// converts a 3D vector in the XY plane into a 2D vector
inline Base::Vector2d toVector2d(const Base::Vector3d& vector3d)
{
    return Base::Vector2d(vector3d.x, vector3d.y);
}

#endif  // SKETCHER_Utils_H
