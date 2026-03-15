// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef SKETCHER_GEOMETRY_H
#define SKETCHER_GEOMETRY_H

#include <memory>
#include <list>
#include "GeoEnum.h"
#include <Base/Vector3D.h>

namespace Part
{
class Geometry;
}

namespace Sketcher
{

class SketcherExport SketchGeometry
{
public:
    SketchGeometry() = default;
    SketchGeometry(const SketchGeometry&) = default;
    SketchGeometry(SketchGeometry&&) = default;
    SketchGeometry& operator=(const SketchGeometry&) = default;
    SketchGeometry& operator=(SketchGeometry&&) = default;
    virtual ~SketchGeometry() = default;

    virtual bool supports(const Part::Geometry* geo) const = 0;
    virtual Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const = 0;
};

using SketchGeometryPtr = std::shared_ptr<SketchGeometry>;

class SketchGeometryType
{
public:
    static void init();
    static void addType(const SketchGeometryPtr& type);
    static Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId);

private:
    static std::list<SketchGeometryPtr> sketchGeoms;  // NOLINT
};

}  // namespace Sketcher

#endif  // SKETCHER_GEOMETRY_H
