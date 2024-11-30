/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepBuilderAPI_MakePolygon.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS_Wire.hxx>
#endif

#include "FeaturePartPolygon.h"


PROPERTY_SOURCE(Part::Polygon, Part::Feature)


Part::Polygon::Polygon()
{
    ADD_PROPERTY(Nodes,(Base::Vector3d()));
    ADD_PROPERTY(Close,(false));
}

Part::Polygon::~Polygon() = default;

short Part::Polygon::mustExecute() const
{
    if (Nodes.isTouched() || Close.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Part::Polygon::execute()
{
    BRepBuilderAPI_MakePolygon poly;
    const std::vector<Base::Vector3d> nodes = Nodes.getValues();

    for (const auto & node : nodes) {
        gp_Pnt pnt(node.x, node.y, node.z);
        poly.Add(pnt);
    }

    if (Close.getValue())
        poly.Close();

    if (!poly.IsDone())
        throw Base::CADKernelError("Cannot create polygon because less than two vertices are given");
    TopoDS_Wire wire = poly.Wire();
    this->Shape.setValue(wire);

    return App::DocumentObject::StdReturn;
}
