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

#include <BRep_Tool.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>

#include "GeometryMapper3D.h"
#include "Sketch3DObject.h"
#include "Solver3D.h"


using namespace Sketcher3D;

PROPERTY_SOURCE(Sketcher3D::Sketch3DObject, Part::Feature)

Sketch3DObject::Sketch3DObject()
{
    ADD_PROPERTY_TYPE(
        Geometry,
        (nullptr),
        "Sketcher3D",
        (App::PropertyType)(App::Prop_None),
        "Sketcher3D geometry"
    );
    ADD_PROPERTY_TYPE(
        Constraints,
        (nullptr),
        "Sketcher3D",
        (App::PropertyType)(App::Prop_None),
        "Sketcher3D constraints"
    );
}

Sketch3DObject::~Sketch3DObject() = default;

int Sketch3DObject::addGeometry(std::unique_ptr<Part::Geometry> geom)
{
    if (!geom) {
        return -1;
    }
    int idx = Geometry.getSize();
    Geometry.set1Value(-1, std::move(geom));
    return idx;
}

int Sketch3DObject::addConstraint(const Constraint3D& c)
{
    const int idx = Constraints.getSize();
    Constraints.setConstraintAt(-1, c);
    return idx;
}

GeoElementId3D Sketch3DObject::resolvePickedVertex(const TopoDS_Vertex& vertex) const
{
    if (vertex.IsNull()) {
        return {};
    }
    const gp_Pnt pnt = BRep_Tool::Pnt(vertex);
    const Base::Vector3d target(pnt.X(), pnt.Y(), pnt.Z());

    const auto& geos = Geometry.getValues();
    constexpr double tol = 1e-6;
    for (std::size_t i = 0; i < geos.size(); ++i) {
        const Part::Geometry* g = geos[i];
        if (!g) {
            continue;
        }
        const int geoId = static_cast<int>(i);
        if (const auto* gp = dynamic_cast<const Part::GeomPoint*>(g)) {
            if ((gp->getPoint() - target).Length() < tol) {
                return {geoId, PointPos::none, GeoKind::Point};
            }
        }
        else if (const auto* ls = dynamic_cast<const Part::GeomLineSegment*>(g)) {
            if ((ls->getStartPoint() - target).Length() < tol) {
                return {geoId, PointPos::start, GeoKind::Line};
            }
            if ((ls->getEndPoint() - target).Length() < tol) {
                return {geoId, PointPos::end, GeoKind::Line};
            }
        }
    }
    return {};
}

short Sketch3DObject::mustExecute() const
{
    if (Geometry.isTouched() || Constraints.isTouched()) {
        return 1;
    }
    return Part::Feature::mustExecute();
}

void Sketch3DObject::acceptGeometry()
{
    const auto& geos = Geometry.getValues();
    const int n = static_cast<int>(geos.size());

    auto current = Constraints.getConstraints();
    std::vector<Constraint3D> kept;
    kept.reserve(current.size());

    for (const Constraint3D& c : current) {
        bool allResolve = true;
        for (const GeoElementId3D& ref : c.getElements()) {
            if (ref.GeoId < 0 || ref.GeoId >= n || geos[ref.GeoId] == nullptr) {
                allResolve = false;
                break;
            }
        }
        if (allResolve) {
            kept.push_back(c);
        }
    }

    if (kept.size() != current.size()) {
        Constraints.setConstraints(kept);
    }
}

int Sketch3DObject::solve(bool updateGeo)
{
    Solver3D solver;
    GeometryMapper3D mapper;

    mapper.push(Geometry.getValues(), Constraints.getConstraints(), solver);
    const int status = solver.solve();

    if (updateGeo && status == Solver3D::OK) {
        // Clone the list, update values in place, then reassign so the
        // property transaction captures the change.
        std::vector<Part::Geometry*> updated;
        updated.reserve(Geometry.getSize());
        for (Part::Geometry* g : Geometry.getValues()) {
            updated.push_back(g ? g->clone() : nullptr);
        }
        mapper.writeBack(solver, updated);
        Geometry.setValues(std::move(updated));
    }

    return status;
}

App::DocumentObjectExecReturn* Sketch3DObject::execute()
{
    try {
        acceptGeometry();

        const int status = solve(true);

        switch (status) {
            case Solver3D::OverConstrained:
                return new App::DocumentObjectExecReturn("Over-constrained 3D sketch", this);
            case Solver3D::Conflicting:
                return new App::DocumentObjectExecReturn("3D sketch has conflicting constraints", this);
            case Solver3D::Redundant:
                Base::Console().warning("3D sketch has redundant constraints");
                break;
            case Solver3D::Malformed:
                return new App::DocumentObjectExecReturn("Malformed 3D sketch constraints", this);
            case Solver3D::SolverFailed:
                return new App::DocumentObjectExecReturn("3D solver failed", this);
            default:
                break;
        }

        buildShape();
        return Part::Feature::StdReturn;
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

void Sketch3DObject::buildShape()
{
    std::vector<Part::TopoShape> shapes;
    const auto& geos = Geometry.getValues();
    shapes.reserve(geos.size());

    for (const Part::Geometry* g : geos) {
        if (!g) {
            continue;
        }
        TopoDS_Shape occtShape = g->toShape();
        if (occtShape.IsNull()) {
            continue;
        }
        shapes.emplace_back(occtShape);
    }

    if (shapes.empty()) {
        Shape.setValue(Part::TopoShape());
        return;
    }

    Shape.setValue(Part::TopoShape().makeElementCompound(shapes));
}
