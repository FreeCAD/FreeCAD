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
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>

#include <App/Document.h>
#include <App/ElementMap.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Sketcher/App/GeometryFacade.h>

#include "GeomReferencePlane3D.h"
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
    const Part::Geometry* geo = _getGeometry(geoId);
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


namespace
{
// extract the stable geometry id and vertex position from names
bool parseMappedName(const char* mapped, long& stableId, PointPos& pos)
{
    if (!mapped || mapped[0] != 'g') {
        return false;
    }
    const char* p = mapped + 1;
    char* end = nullptr;
    const long g = std::strtol(p, &end, 10);
    if (end == p) {
        return false;
    }
    stableId = g;
    pos = PointPos::none;
    if (*end == 'v') {
        const char* q = end + 1;
        const long v = std::strtol(q, &end, 10);
        if (end == q || v < 0 || v > 3) {
            return false;
        }
        pos = static_cast<PointPos>(v);
    }
    return true;
}

GeoKind kindFromGeometry(const Part::Geometry* g)
{
    if (!g) {
        return GeoKind::Unknown;
    }
    if (g->is<Part::GeomPoint>()) {
        return GeoKind::Point;
    }
    if (g->is<Part::GeomLineSegment>()) {
        return GeoKind::Line;
    }
    if (g->is<GeomReferencePlane3D>()) {
        return GeoKind::Plane;
    }
    // Extend with Circle/Arc once they're wired through buildShape.
    return GeoKind::Unknown;
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

    long stableId = -1;
    PointPos pos = PointPos::none;
    const std::string mapped = el.name.toString();
    if (!parseMappedName(mapped.c_str(), stableId, pos)) {
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
    return {geoId, pos, kindFromGeometry(geos[geoId])};
}

}  // namespace

GeoElementId3D Sketch3DObject::resolveSubName(const std::string& subname) const
{
    if (subname.empty()) {
        return {};
    }

    const auto& geos = Geometry.getValues();
    const auto& prefix = referencePrefix();

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

    const auto& prefix = referencePrefix();

    if (!subname.starts_with(prefix)) {
        const auto& shape = Shape.getShape();
        return shape.isNull() ? TopoDS_Shape() : shape.getSubShape(subname.c_str(), silent);
    }

    const char* local = subname.c_str() + prefix.size();
    auto type = Part::TopoShape::getElementTypeAndIndex(local);

    if (type.first == "Face") {
        int geoId = static_cast<int>(type.second) - 1;
        if (!_getGeometry(geoId)) {
            return TopoDS_Shape();
        }
        return Geometry.getValues()[geoId]->toShape();
    }

    const Part::TopoShape& ref = ReferenceShape.getShape();
    return ref.isNull() ? TopoDS_Shape() : ref.getSubShape(local, silent);
}

bool Sketch3DObject::getPointAt(const GeoElementId3D& target, Base::Vector3d& point) const
{
    if (target.isRootPoint()) {
        point = Base::Vector3d(0.0, 0.0, 0.0);
        return true;
    }

    if (!_getGeometry(target.GeoId)) {
        return false;
    }

    Part::Geometry* geo = Geometry.getValues()[target.GeoId];

    if (auto* pt = dynamic_cast<Part::GeomPoint*>(geo)) {
        if (target.Pos != PointPos::none) {
            return false;
        }
        point = pt->getPoint();
        return true;
    }

    if (auto* seg = dynamic_cast<Part::GeomLineSegment*>(geo)) {
        switch (target.Pos) {
            case PointPos::start:
                point = seg->getStartPoint();
                return true;
            case PointPos::end:
                point = seg->getEndPoint();
                return true;
            default:
                return false;
        }
    }

    return false;
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
    for (const Constraint3D& c : Constraints.getConstraints()) {
        if (c.Type != Constraint3D::Coincident3D || c.getElements().size() < 2) {
            continue;
        }

        const auto& elements = c.getElements();
        coincident[elements[0]].push_back(elements[1]);
        coincident[elements[1]].push_back(elements[0]);
    }

    std::vector<GeoElementId3D> pending {a};
    std::set<GeoElementId3D> visited;
    while (!pending.empty()) {
        const GeoElementId3D current = pending.back();
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
    const auto& geos = Geometry.getValues();
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
    const auto& geos = Geometry.getValues();
    int n = static_cast<int>(geos.size());

    auto current = Constraints.getConstraints();
    std::vector<Constraint3D> kept;
    kept.reserve(current.size());

    for (const Constraint3D& c : current) {
        bool allResolve = true;
        for (const GeoElementId3D& ref : c.getElements()) {
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

    if (updateGeo && status != Solver3D::OK && status != Solver3D::Redundant) {
        Base::Console().warning(
            "Sketcher3D debug: skipping geometry writeback because solver status=%d\n",
            status
        );
    }

    // write back on Redundant too, the GCS numeric solve succeeded
    // as they are not a failure.
    bool solved = (status == Solver3D::OK || status == Solver3D::Redundant);
    if (updateGeo && solved) {
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

namespace
{

Part::TopoShape makeNamedShape(const TopoDS_Shape& ocShape)
{
    Part::TopoShape shape(ocShape);
    shape.resetElementMap(std::make_shared<Data::ElementMap>());
    return shape;
}

void setElementName(Part::TopoShape& shape, const char* type, int index, const std::string& name)
{
    shape.setElementName(
        Data::IndexedName::fromConst(type, index),
        Data::MappedName::fromRawData(name.c_str()),
        0L
    );
}

Part::TopoShape makeNamedVertex(const TopoDS_Shape& ocShape, const std::string& gname)
{
    Part::TopoShape shape = makeNamedShape(ocShape);
    setElementName(shape, "Vertex", 1, gname);
    return shape;
}

Part::TopoShape makeNamedEdge(const TopoDS_Shape& ocShape, const std::string& gname)
{
    Part::TopoShape shape = makeNamedShape(ocShape);
    setElementName(shape, "Edge", 1, gname);

    auto edge = TopoDS::Edge(shape.getShape());
    auto vStart = TopExp::FirstVertex(edge);
    auto vEnd = TopExp::LastVertex(edge);

    auto nameVertex = [&](const TopoDS_Vertex& vertex, PointPos pos) {
        setElementName(
            shape,
            "Vertex",
            shape.findShape(vertex),
            gname + 'v' + std::to_string(static_cast<int>(pos))
        );
    };

    nameVertex(vStart, PointPos::start);
    if (!vStart.IsSame(vEnd)) {
        nameVertex(vEnd, PointPos::end);
    }

    return shape;
}

}  // namespace

Part::TopoShape Sketch3DObject::buildShapeForGeometry(bool construction) const
{
    std::vector<Part::TopoShape> shapes;
    const auto& geos = Geometry.getValues();
    shapes.reserve(geos.size());

    for (auto& g : geos) {
        if (!g || Sketcher::GeometryFacade::getConstruction(g) != construction) {
            continue;
        }

        auto shape = g->toShape();
        if (shape.IsNull()) {
            continue;
        }

        auto gname = "g" + std::to_string(Sketcher::GeometryFacade::getFacade(g)->getId());

        // Reference planes are in view only
        if (g->is<Sketcher3D::GeomReferencePlane3D>()) {
            continue;
        }

        if (g->is<Part::GeomLineSegment>()) {
            shapes.push_back(makeNamedEdge(shape, gname));
        }
        else if (g->is<Part::GeomPoint>()) {
            shapes.push_back(makeNamedVertex(shape, gname));
        }
        // Extend with Circle/Arc/Ellipse/BSpline.
    }

    if (shapes.empty()) {
        return Part::TopoShape();
    }

    Part::TopoShape result;
    result.makeElementCompound(shapes, Part::OpCodes::Sketch3D);
    result.Tag = getID();
    return result;
}

void Sketch3DObject::buildShapes()
{
    Shape.setValue(buildShapeForGeometry(/*construction=*/false));
    ReferenceShape.setValue(buildShapeForGeometry(/*construction=*/true));
}
