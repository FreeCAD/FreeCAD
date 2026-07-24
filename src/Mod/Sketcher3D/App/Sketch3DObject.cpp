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

#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>

#include <App/ElementMap.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Sketcher/App/GeometryFacade.h>

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
    ADD_PROPERTY_TYPE(
        ReferenceShape,
        (Part::TopoShape()),
        "Sketcher3D",
        (App::PropertyType)(App::Prop_Output | App::Prop_Hidden),
        "Reference geometry shape"
    );
    registerElementCache(referencePrefix(), &ReferenceShape);
}

const std::string& Sketch3DObject::referencePrefix()
{
    static const std::string prefix = "Ref";
    return prefix;
}

Sketch3DObject::~Sketch3DObject() = default;

int Sketch3DObject::addGeometry(std::unique_ptr<Part::Geometry> geom, bool construction)
{
    if (!geom) {
        return -1;
    }
    int idx = Geometry.getSize();
    Geometry.set1Value(-1, std::move(geom));
    Sketcher::GeometryFacade::setConstruction(Geometry.getValues()[idx], construction);
    return idx;
}

bool Sketch3DObject::getConstruction(int geoId) const
{
    auto* geo = _getGeometry(geoId);
    return geo && Sketcher::GeometryFacade::getConstruction(geo);
}

int Sketch3DObject::addConstraint(const Constraint3D& c)
{
    int idx = Constraints.getSize();
    Constraints.setConstraintAt(-1, c);
    return idx;
}

const Part::Geometry* Sketch3DObject::_getGeometry(int geoId) const
{
    auto& geos = Geometry.getValues();
    if (geoId >= 0 && geoId < static_cast<int>(geos.size())) {
        return geos[geoId];
    }
    return nullptr;
}

// naming g{stableId}, g{stableId}v{pos} and reference planes use RefFace{geoId+1}.
namespace
{

std::string mappedName(const Part::Geometry* geo, PointPos pos)
{
    std::string name = "g" + std::to_string(Sketcher::GeometryFacade::getFacade(geo)->getId());
    if (pos == PointPos::none) {
        return name;
    }
    return name + 'v' + std::to_string(static_cast<int>(pos));
}

bool parseMappedName(const char* mapped, long& stableId, PointPos& pos)
{
    if (!mapped || mapped[0] != 'g') {
        return false;
    }
    const char* p = mapped + 1;
    char* end = nullptr;
    long g = std::strtol(p, &end, 10);
    if (end == p) {
        return false;
    }
    stableId = g;
    pos = PointPos::none;
    if (*end == 'v') {
        char* q = end + 1;
        long v = std::strtol(q, &end, 10);
        if (end == q || v < 0 || v > 3) {
            return false;
        }
        pos = static_cast<PointPos>(v);
    }
    return true;
}

GeoElementId3D resolveSubNameInShape(
    const Part::TopoShape& shape,
    const std::map<long, int>& stableToIndex,
    const std::vector<Part::Geometry*>& geos,
    const std::string& subname
)
{
    if (subname.empty() || shape.isNull()) {
        return {};
    }

    Data::MappedElement el = shape.getElementName(subname.c_str());
    if (!el.name) {
        return {};
    }

    if (el.name.toString() == "RootPoint") {
        return GeoElementId3D::RtPnt;
    }

    long stableId = -1;
    PointPos pos = PointPos::none;
    if (!parseMappedName(el.name.toString().c_str(), stableId, pos)) {
        return {};
    }

    auto it = stableToIndex.find(stableId);
    if (it == stableToIndex.end()) {
        return {};
    }
    int geoId = it->second;
    if (geoId < 0 || geoId >= static_cast<int>(geos.size())) {
        return {};
    }
    return {geoId, pos, kindOfGeometry(geos[geoId])};
}

void setElementName(Part::TopoShape& shape, const char* type, int index, const std::string& name)
{
    shape.setElementName(
        Data::IndexedName::fromConst(type, index),
        Data::MappedName::fromRawData(name.c_str()),
        0L
    );
}

Part::TopoShape makeNamedVertex(
    const TopoDS_Shape& ocShape,
    const Part::Geometry* geo,
    PointPos pos = PointPos::none
)
{
    Part::TopoShape shape(ocShape);
    shape.resetElementMap(std::make_shared<Data::ElementMap>());
    setElementName(shape, "Vertex", 1, mappedName(geo, pos));
    return shape;
}

Part::TopoShape makeNamedEdge(const TopoDS_Shape& ocShape, const Part::Geometry* geo)
{
    Part::TopoShape shape(ocShape);
    shape.resetElementMap(std::make_shared<Data::ElementMap>());
    setElementName(shape, "Edge", 1, mappedName(geo, PointPos::none));

    auto edge = TopoDS::Edge(shape.getShape());
    auto vStart = TopExp::FirstVertex(edge);
    auto vEnd = TopExp::LastVertex(edge);

    setElementName(shape, "Vertex", shape.findShape(vStart), mappedName(geo, PointPos::start));
    if (!vStart.IsSame(vEnd)) {
        setElementName(shape, "Vertex", shape.findShape(vEnd), mappedName(geo, PointPos::end));
    }

    return shape;
}

bool pointAtGeometry(const Part::Geometry* geo, PointPos pos, Base::Vector3d& point)
{
    if (!geo) {
        return false;
    }

    switch (pos) {
        case PointPos::none:
            if (auto* pt = dynamic_cast<const Part::GeomPoint*>(geo)) {
                point = pt->getPoint();
                return true;
            }
            return false;
        case PointPos::start:
        case PointPos::end:
            if (auto* curve = dynamic_cast<const Part::GeomBoundedCurve*>(geo)) {
                point = (pos == PointPos::start) ? curve->getStartPoint() : curve->getEndPoint();
                return true;
            }
            return false;
        case PointPos::mid:
            if (auto* arc = dynamic_cast<const Part::GeomArcOfConic*>(geo)) {
                point = arc->getCenter();
                return true;
            }
            if (auto* conic = dynamic_cast<const Part::GeomConic*>(geo)) {
                point = conic->getCenter();
                return true;
            }
            return false;
    }
    return false;
}

Part::TopoShape makeNamedCenterVertex(const Part::Geometry* geo)
{
    Base::Vector3d center;
    if (!pointAtGeometry(geo, PointPos::mid, center)) {
        return {};
    }

    Part::GeomPoint pt(center);
    return makeNamedVertex(pt.toShape(), geo, PointPos::mid);
}

// The root/origin point is fixed solver geometry (GeoId -1). ReferenceShape
// gives it a selectable edit-time representation without adding it to Shape.
Part::TopoShape makeRootPointVertex()
{
    Part::GeomPoint origin(Base::Vector3d(0.0, 0.0, 0.0));
    auto ocShape = origin.toShape();
    if (ocShape.IsNull()) {
        return {};
    }

    Part::TopoShape shape(ocShape);
    shape.resetElementMap(std::make_shared<Data::ElementMap>());
    setElementName(shape, "Vertex", 1, "RootPoint");
    return shape;
}

Part::TopoShape makeNamedGeometryShape(const Part::Geometry* geo)
{
    if (!geo) {
        return {};
    }

    const GeoKind kind = kindOfGeometry(geo);
    if (kind == GeoKind::Plane || kind == GeoKind::Unknown) {
        return {};
    }

    auto ocShape = geo->toShape();
    if (ocShape.IsNull()) {
        return {};
    }

    switch (kind) {
        case GeoKind::Point:
            return makeNamedVertex(ocShape, geo);
        case GeoKind::Line:
        case GeoKind::Arc:
        case GeoKind::Circle:
            return makeNamedEdge(ocShape, geo);
        case GeoKind::Plane:
        case GeoKind::Unknown:
            return {};
    }
    return {};
}

void appendShape(std::vector<Part::TopoShape>& shapes, Part::TopoShape shape)
{
    if (!shape.isNull()) {
        shapes.push_back(std::move(shape));
    }
}

Part::TopoShape makeShapeCompound(const std::vector<Part::TopoShape>& shapes, long tag)
{
    Part::TopoShape result;
    result.makeElementCompound(shapes, Part::OpCodes::Sketch3D);
    result.Tag = tag;
    return result;
}

}  // namespace

GeoElementId3D Sketch3DObject::resolveSubName(const std::string& subname) const
{
    if (subname.empty()) {
        return {};
    }

    auto& geos = Geometry.getValues();
    auto& prefix = referencePrefix();

    if (!subname.starts_with(prefix)) {
        return resolveSubNameInShape(Shape.getShape(), stableToIndex, geos, subname);
    }

    auto* local = subname.c_str() + prefix.size();
    auto type = Part::TopoShape::getElementTypeAndIndex(local);

    if (type.first == "Face") {
        return {static_cast<int>(type.second) - 1, PointPos::none, GeoKind::Plane};
    }

    return resolveSubNameInShape(ReferenceShape.getShape(), stableToIndex, geos, local);
}

TopoDS_Shape Sketch3DObject::getSubShape(const std::string& subname, bool silent) const
{
    if (subname.empty()) {
        return {};
    }

    auto& prefix = referencePrefix();

    if (!subname.starts_with(prefix)) {
        auto& shape = Shape.getShape();
        return shape.isNull() ? TopoDS_Shape() : shape.getSubShape(subname.c_str(), silent);
    }

    const char* local = subname.c_str() + prefix.size();
    auto type = Part::TopoShape::getElementTypeAndIndex(local);

    if (type.first == "Face") {
        int geoId = static_cast<int>(type.second) - 1;
        auto* geo = _getGeometry(geoId);
        if (!geo) {
            return {};
        }
        return geo->toShape();
    }

    auto& ref = ReferenceShape.getShape();
    return ref.isNull() ? TopoDS_Shape() : ref.getSubShape(local, silent);
}

// TODO: Need better implementation here.
bool Sketch3DObject::getPointAt(const GeoElementId3D& target, Base::Vector3d& point) const
{
    if (target.isRootPoint()) {
        point = Base::Vector3d(0.0, 0.0, 0.0);
        return true;
    }

    return pointAtGeometry(_getGeometry(target.GeoId), target.Pos, point);
}

bool Sketch3DObject::arePointsCoincident3D(const GeoElementId3D& a, const GeoElementId3D& b) const
{
    if (!a.isValid() || !b.isValid()) {
        return false;
    }
    if (a == b) {
        return true;
    }

    // graph edges means Two points are already coincident if there is any path from one to
    // the other.
    std::map<GeoElementId3D, std::vector<GeoElementId3D>> coincident;
    for (auto& c : Constraints.getConstraints()) {
        if (c.Type != Constraint3D::Coincident3D || c.getElements().size() < 2) {
            continue;
        }

        auto& elements = c.getElements();
        coincident[elements[0]].push_back(elements[1]);
        coincident[elements[1]].push_back(elements[0]);
    }

    std::vector<GeoElementId3D> pending {a};
    std::set<GeoElementId3D> visited;
    while (!pending.empty()) {
        GeoElementId3D current = pending.back();
        pending.pop_back();

        if (current == b) {
            return true;
        }
        if (!visited.insert(current).second) {
            continue;
        }

        auto it = coincident.find(current);
        if (it != coincident.end()) {
            pending.insert(pending.end(), it->second.begin(), it->second.end());
        }
    }

    return false;
}

short Sketch3DObject::mustExecute() const
{
    if (Geometry.isTouched() || Constraints.isTouched()) {
        return 1;
    }
    return Part::Feature::mustExecute();
}

void Sketch3DObject::onChanged(const App::Property* prop)
{
    if (prop == &Geometry) {
        assignStableIds();
    }
    Part::Feature::onChanged(prop);
}

void Sketch3DObject::assignStableIds()
{
    stableToIndex.clear();
    auto& geos = Geometry.getValues();
    for (long i = 0; i < static_cast<long>(geos.size()); ++i) {
        auto* geo = geos[i];
        if (!geo) {
            continue;
        }
        auto gf = Sketcher::GeometryFacade::getFacade(geo);
        if (gf->getId() == 0) {
            gf->setId(++geoLastId);
        }
        else if (gf->getId() > geoLastId) {
            geoLastId = gf->getId();
        }
        // Defensive duplicate handling
        while (!stableToIndex.insert({gf->getId(), static_cast<int>(i)}).second) {
            Base::Console()
                .warning("Sketcher3D: duplicate stable id %ld -> %ld\n", gf->getId(), geoLastId + 1);
            gf->setId(++geoLastId);
        }
    }
}

void Sketch3DObject::acceptGeometry()
{
    auto& geos = Geometry.getValues();
    int n = static_cast<int>(geos.size());

    auto current = Constraints.getConstraints();
    std::vector<Constraint3D> kept;
    kept.reserve(current.size());

    for (auto& c : current) {
        bool allResolve = true;
        for (auto& ref : c.getElements()) {
            if (ref.isRootPoint()) {
                continue;
            }
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

    mapper.setUpSketch(Geometry.getValues(), Constraints.getConstraints(), solver);
    lastMalformedConstraints = mapper.getMalformedConstraints();
    if (mapper.hasMalformedConstraints()) {
        return Solver3D::Malformed;
    }

    int status = solver.solve();
    // TODO: need consumer of lastMalformedConstraints to be added in taskpanel.
    lastMalformedConstraints = solver.getConflicting();


    // write back on Redundant too, the GCS numeric solve succeeded
    // as they are not a failure.
    if (updateGeo && (status == Solver3D::OK || status == Solver3D::Redundant)) {
        mapper.updateGeometry(solver);
        Geometry.setValues(mapper.extractGeometry());
    }

    return status;
}

App::DocumentObjectExecReturn* Sketch3DObject::execute()
{
    try {
        acceptGeometry();

        int status = solve(true);

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

        buildShapes();
        return Part::Feature::StdReturn;
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

void Sketch3DObject::buildShapes()
{
    auto& geos = Geometry.getValues();
    std::vector<Part::TopoShape> normalShapes;
    std::vector<Part::TopoShape> referenceShapes;
    normalShapes.reserve(geos.size());
    referenceShapes.reserve((2 * geos.size()) + 1);

    appendShape(referenceShapes, makeRootPointVertex());

    for (auto& geo : geos) {
        if (!geo) {
            continue;
        }

        GeoKind kind = kindOfGeometry(geo);
        if (kind == GeoKind::Circle || kind == GeoKind::Arc) {
            appendShape(referenceShapes, makeNamedCenterVertex(geo));
        }

        auto& target = Sketcher::GeometryFacade::getConstruction(geo) ? referenceShapes
                                                                      : normalShapes;
        appendShape(target, makeNamedGeometryShape(geo));
    }

    Shape.setValue(makeShapeCompound(normalShapes, getID()));
    ReferenceShape.setValue(makeShapeCompound(referenceShapes, getID()));
}
