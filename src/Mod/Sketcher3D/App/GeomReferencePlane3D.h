// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <memory>

#include <gp_Ax3.hxx>

#include <Base/Vector3D.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

namespace Sketcher3D
{

constexpr double kReferencePlaneHalfSize = 50.0;

/// Internal Sketcher3D reference plane stored in sketch geometry.
class Sketcher3DExport GeomReferencePlane3D: public Part::GeomPlane
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    GeomReferencePlane3D() = default;
    explicit GeomReferencePlane3D(const Handle(Geom_Plane) & plane)
        : Part::GeomPlane(plane)
    {}
    explicit GeomReferencePlane3D(const gp_Pln& pln)
        : Part::GeomPlane(pln)
    {}
    GeomReferencePlane3D(
        const Base::Vector3d& origin,
        const Base::Vector3d& xDir,
        const Base::Vector3d& normal
    )
        : Part::GeomPlane(gp_Pln(gp_Ax3(
              gp_Pnt(origin.x, origin.y, origin.z),
              gp_Dir(normal.x, normal.y, normal.z),
              gp_Dir(xDir.x, xDir.y, xDir.z)
          )))
    {}
    ~GeomReferencePlane3D() override = default;

    /// Origin, x-axis point, and a third point defining the plane normal.
    static std::unique_ptr<GeomReferencePlane3D> fromThreePoints(
        const Base::Vector3d& origin,
        const Base::Vector3d& xAxisPoint,
        const Base::Vector3d& thirdPoint
    );

    Part::Geometry* copy() const override;

    TopoDS_Shape toShape() const override;

    unsigned int getMemSize() const override;
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
};

}  // namespace Sketcher3D
