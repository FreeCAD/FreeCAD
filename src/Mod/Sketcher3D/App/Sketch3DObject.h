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


#ifndef SKETCHER3D_SKETCH3DOBJECT_H
#define SKETCHER3D_SKETCH3DOBJECT_H

#include <memory>
#include <vector>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyGeometryList.h>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

#include "Constraint3D.h"
#include "PropertyConstraint3DList.h"

class TopoDS_Vertex;

namespace Part
{
class Geometry;
}

namespace Sketcher3D
{

/** 3D sketch document object.
 *  Inherited Part::Feature so downstream part operations can use 
 *  it directly.
 */
class Sketcher3DExport Sketch3DObject: public Part::Feature
{

    PROPERTY_HEADER_WITH_OVERRIDE(Sketcher3D::Sketch3DObject);

public:
    Sketch3DObject();
    ~Sketch3DObject() override;

    Part::PropertyGeometryList Geometry;
    PropertyConstraint3DList Constraints;

    const char* getViewProviderName() const override
    {
        return "Sketcher3DGui::ViewProviderSketch3D";
    }

    /// add a geometry primitive. Returns assigned GeoId.
    int addGeometry(std::unique_ptr<Part::Geometry> geom);

    /// add a constraint. Returns its index in Constraints.
    int addConstraint(const Constraint3D& c);

    /// Resolve a picked 3D vertex back to the Sketcher3D element that
    /// produced it.
    GeoElementId3D resolvePickedVertex(const TopoDS_Vertex& vertex) const;

    /// Run the solver. Writes solved positions back into Geometry 
    /// when updateGeo is true. Returns a status code.
    int solve(bool updateGeo = true);

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;

private:
    /// Drop constraints whose referenced GeoIds are out of range or point
    /// to null geometry slots. Runs at the top of execute().
    void acceptGeometry();

    /// Build the output TopoShape from the current (solved) geometry.
    void buildShape();
};

}  // namespace Sketcher3D

#endif  // SKETCHER3D_SKETCH3DOBJECT_H
