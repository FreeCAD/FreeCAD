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

#include "PreCompiled.h"

#include <Geom_Plane.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>

#include <Base/Reader.h>
#include <Base/Writer.h>

#include "GeomReferencePlane3D.h"

using namespace Sketcher3D;

namespace
{

gp_Ax3 makePlaneAx3(const Base::Vector3d& origin, const Base::Vector3d& xDir, const Base::Vector3d& normal)
{
    return gp_Ax3(
        gp_Pnt(origin.x, origin.y, origin.z),
        gp_Dir(normal.x, normal.y, normal.z),
        gp_Dir(xDir.x, xDir.y, xDir.z)
    );
}

}  // namespace

TYPESYSTEM_SOURCE(Sketcher3D::GeomReferencePlane3D, Part::GeomPlane)

std::unique_ptr<GeomReferencePlane3D> GeomReferencePlane3D::fromThreePoints(
    const Base::Vector3d& origin,
    const Base::Vector3d& xAxisPoint,
    const Base::Vector3d& thirdPoint
)
{
    Base::Vector3d xDir = (xAxisPoint - origin).Normalized();
    Base::Vector3d normal = xDir.Cross(thirdPoint - origin).Normalized();
    return std::make_unique<GeomReferencePlane3D>(origin, xDir, normal);
}

Part::Geometry* GeomReferencePlane3D::copy() const
{
    auto* copied = new GeomReferencePlane3D(Handle(Geom_Plane)::DownCast(handle()->Copy()));
    copied->copyNonTag(this);
    return copied;
}

TopoDS_Shape GeomReferencePlane3D::toShape() const
{
    Handle(Geom_Plane) occPlane = Handle(Geom_Plane)::DownCast(handle());
    if (occPlane.IsNull()) {
        return {};
    }
    auto hSize = kReferencePlaneHalfSize;
    BRepBuilderAPI_MakeFace mkFace(occPlane->Pln(), -hSize, hSize, -hSize, hSize);
    return mkFace.IsDone() ? mkFace.Shape() : TopoDS_Shape();
}

unsigned int GeomReferencePlane3D::getMemSize() const
{
    return sizeof(GeomReferencePlane3D);
}

void GeomReferencePlane3D::Save(Base::Writer& writer) const
{
    Part::Geometry::Save(writer);

    Base::Vector3d origin = getLocation();
    Base::Vector3d normal = getDir();
    Base::Vector3d xDir = getXDir();

    writer.Stream() << writer.ind() << "<ReferencePlane3D "
                    << "Ox=\"" << origin.x << "\" Oy=\"" << origin.y << "\" Oz=\"" << origin.z
                    << "\" Nx=\"" << normal.x << "\" Ny=\"" << normal.y << "\" Nz=\"" << normal.z
                    << "\" Xx=\"" << xDir.x << "\" Xy=\"" << xDir.y << "\" Xz=\"" << xDir.z
                    << "\"/>" << std::endl;
}

void GeomReferencePlane3D::Restore(Base::XMLReader& reader)
{
    Part::Geometry::Restore(reader);

    reader.readElement("ReferencePlane3D");
    Base::Vector3d origin {
        reader.getAttribute<double>("Ox"),
        reader.getAttribute<double>("Oy"),
        reader.getAttribute<double>("Oz"),
    };
    Base::Vector3d normal {
        reader.getAttribute<double>("Nx"),
        reader.getAttribute<double>("Ny"),
        reader.getAttribute<double>("Nz"),
    };
    Base::Vector3d xDir {
        reader.getAttribute<double>("Xx"),
        reader.getAttribute<double>("Xy"),
        reader.getAttribute<double>("Xz"),
    };

    setHandle(new Geom_Plane(makePlaneAx3(origin, xDir, normal)));
}
