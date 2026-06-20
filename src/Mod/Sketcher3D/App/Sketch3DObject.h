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
#include <vector>

#include <App/IndexedName.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyGeometryList.h>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

#include "Constraint3D.h"
#include "GeoEnum3D.h"
#include "PropertyConstraint3DList.h"

class TopoDS_Shape;

namespace Part
{
class Geometry;
class TopoShape;
}  // namespace Part

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
    Part::PropertyPartShape ReferenceShape;

    static const std::string& referencePrefix();

    const char* getViewProviderName() const override
    {
        return "Sketcher3DGui::ViewProviderSketch3D";
    }

    /// add a geometry primitive. Returns assigned GeoId.
    int addGeometry(std::unique_ptr<Part::Geometry> geom);
    int addGeometry(std::unique_ptr<Part::Geometry> geom, bool construction);

    bool getConstruction(int geoId) const;

    /// add a constraint. Returns its index in Constraints.
    int addConstraint(const Constraint3D& c);

    template<
        typename GeometryT = Part::Geometry,
        typename = typename std::enable_if<
            std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value>::type>
    const GeometryT* getGeometry(int geoId) const
    {
        return static_cast<const GeometryT*>(_getGeometry(geoId));
    }

    /// Resolve a shape subname like "Edge1" or "RefVertex2" to the owning
    /// sketch geometry. Uses stable element map names so references survive.
    GeoElementId3D resolveSubName(const std::string& subname) const;

    /// Return the OCCT sub shape for normal and reference geometry.
    TopoDS_Shape getSubShape(const std::string& subname, bool silent = true) const;

    /// Write the world position of target to point.
    bool getPointAt(const GeoElementId3D& target, Base::Vector3d& point) const;

    /// Return true when two points are already connected by Coincident3D
    /// constraints, directly or through other coincident points.
    bool arePointsCoincident3D(const GeoElementId3D& a, const GeoElementId3D& b) const;

    /// Run the solver. Writes solved positions back into Geometry
    /// when updateGeo is true. Returns a status code.
    int solve(bool updateGeo = true);

    const std::vector<int>& getLastMalformedConstraints() const
    {
        return lastMalformedConstraints;
    }

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;

    /// when the Geometry property changes we run assignStableIds()
    /// so stable IDs are available always.
    void onChanged(const App::Property* prop) override;

private:
    const Part::Geometry* _getGeometry(int geoId) const;

    /// Drop constraints whose referenced GeoIds are out of range or point
    /// to null geometry slots. Runs at the top of execute().
    void acceptGeometry();

    /// Build a TopoShape for normal or reference geometry.
    Part::TopoShape buildShapeForGeometry(bool construction) const;

    /// Build Shape and ReferenceShape from the current (solved) geometry.
    void buildShapes();

    /// Build a named edge shape and name its endpoint vertices
    Part::TopoShape makeNamedEdge(const Part::Geometry* geo, const std::string& edgeName) const;

    /// Ensure every geometry has a stable ID and rebuild the lookup from
    /// stable ID to current Geometry index.
    void assignStableIds();

    /// Monotonic counter for stable geometry IDs.
    long geoLastId = 0;

    /// stable id map to current positional index in Geometry. Rebuilt by
    /// assignStableIds(). "g{stableId}"
    std::map<long, int> stableToIndex;

    std::vector<int> lastMalformedConstraints;
};

}  // namespace Sketcher3D
