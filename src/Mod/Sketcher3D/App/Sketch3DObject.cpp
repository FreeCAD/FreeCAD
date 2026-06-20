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
#include <TopoDS_Shape.hxx>

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

int Sketch3DObject::addGeometry(std::unique_ptr<Part::Geometry> geom)
{
    return addGeometry(std::move(geom), false);
}

int Sketch3DObject::addGeometry(std::unique_ptr<Part::Geometry> geom, bool construction)
{
    if (!geom) {
        return -1;
    }
    int idx = Geometry.getSize();
    Geometry.set1Value(-1, std::move(geom));
    const auto& geos = Geometry.getValues();
    if (idx >= 0 && idx < static_cast<int>(geos.size()) && geos[idx]) {
        Sketcher::GeometryFacade::setConstruction(geos[idx], construction);
    }
    return idx;
}

bool Sketch3DObject::getConstruction(int geoId) const
{
    const Part::Geometry* geo = _getGeometry(geoId);
    return geo && Sketcher::GeometryFacade::getConstruction(geo);
}

int Sketch3DObject::addConstraint(const Constraint3D& c)
{
    const int idx = Constraints.getSize();
    Constraints.setConstraintAt(-1, c);
    return idx;
}

const Part::Geometry* Sketch3DObject::_getGeometry(int geoId) const
{
    const auto& geos = Geometry.getValues();
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
    const int geoId = it->second;

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
    const auto prefix = referencePrefix();

    if (subname.size() >= prefix.size() && subname.compare(0, prefix.size(), prefix) == 0) {
        return resolveSubNameInShape(
            ReferenceShape.getShape(),
            stableToIndex,
            geos,
            subname.substr(prefix.size())
        );
    }

    return resolveSubNameInShape(Shape.getShape(), stableToIndex, geos, subname);
}

TopoDS_Shape Sketch3DObject::getSubShape(const std::string& subname, bool silent) const
{
    if (subname.empty()) {
        return {};
    }

    const auto shapeForSubName =
        [this](const std::string& name, std::string& localSub) -> const Part::TopoShape& {
        const auto& prefix = referencePrefix();
        if (name.size() >= prefix.size() && name.compare(0, prefix.size(), prefix) == 0) {
            localSub = name.substr(prefix.size());
            return ReferenceShape.getShape();
        }
        localSub = name;
        return Shape.getShape();
    };

    std::string localSub;
    const Part::TopoShape& shape = shapeForSubName(subname, localSub);
    if (shape.isNull()) {
        return {};
    }

    return shape.getSubShape(localSub.c_str(), silent);
}

bool Sketch3DObject::getPointAt(const GeoElementId3D& target, Base::Vector3d& point) const
{
    if (target.isRootPoint()) {
        point = Base::Vector3d(0.0, 0.0, 0.0);
        return true;
    }

    const auto& geos = Geometry.getValues();
    if (target.GeoId < 0 || target.GeoId >= static_cast<int>(geos.size())) {
        return false;
    }

    const Part::Geometry* geo = geos[target.GeoId];
    if (!geo) {
        return false;
    }

    if (const auto* pt = dynamic_cast<const Part::GeomPoint*>(geo)) {
        if (target.Pos != PointPos::none) {
            return false;
        }
        point = pt->getPoint();
        return true;
    }

    if (const auto* seg = dynamic_cast<const Part::GeomLineSegment*>(geo)) {
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
        const Part::Geometry* geo = geos[i];
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
    const int n = static_cast<int>(geos.size());

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

    const int status = solver.solve();

    if (updateGeo && status != Solver3D::OK && status != Solver3D::Redundant) {
        Base::Console().warning(
            "Sketcher3D debug: skipping geometry writeback because solver status=%d\n",
            status
        );
    }

    // write back on Redundant too, the GCS numeric solve succeeded
    // as they are not a failure.
    const bool solved = (status == Solver3D::OK || status == Solver3D::Redundant);
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

Part::TopoShape Sketch3DObject::makeNamedEdge(const Part::Geometry* geo, const std::string& edgeName) const
{
    Part::TopoShape shape(geo->toShape());
    if (!shape.hasElementMap()) {
        shape.resetElementMap(std::make_shared<Data::ElementMap>());
    }
    shape.setElementName(
        Data::IndexedName::fromConst("Edge", 1),
        Data::MappedName::fromRawData(edgeName.c_str()),
        0L
    );

    const auto* curve = dynamic_cast<const Part::GeomBoundedCurve*>(geo);
    if (!curve) {
        return shape;
    }

    TopTools_IndexedMapOfShape vmap;
    TopExp::MapShapes(shape.getShape(), TopAbs_VERTEX, vmap);
    const Base::Vector3d ends[] = {curve->getStartPoint(), curve->getEndPoint()};
    const PointPos posIds[] = {PointPos::start, PointPos::end};
    for (int i = 1; i <= vmap.Extent(); ++i) {
        const gp_Pnt gpt = BRep_Tool::Pnt(TopoDS::Vertex(vmap(i)));
        const Base::Vector3d pt(gpt.X(), gpt.Y(), gpt.Z());
        for (std::size_t j = 0; j < sizeof(posIds) / sizeof(posIds[0]); ++j) {
            if (ends[j] == pt) {
                const std::string vname = edgeName + 'v'
                    + std::to_string(static_cast<int>(posIds[j]));
                shape.setElementName(
                    Data::IndexedName::fromConst("Vertex", i),
                    Data::MappedName::fromRawData(vname.c_str()),
                    0L
                );
                break;
            }
        }
    }
    return shape;
}

Part::TopoShape Sketch3DObject::buildShapeForGeometry(bool construction) const
{
    std::vector<Part::TopoShape> edges;
    std::vector<Part::TopoShape> vertices;
    const auto& geos = Geometry.getValues();
    edges.reserve(geos.size());

    for (std::size_t i = 0; i < geos.size(); ++i) {
        const Part::Geometry* g = geos[i];
        if (!g) {
            continue;
        }
        if (Sketcher::GeometryFacade::getConstruction(g) != construction) {
            continue;
        }

        TopoDS_Shape occtShape = g->toShape();
        if (occtShape.IsNull()) {
            continue;
        }
        // The mapped name uses the stable id
        const long stableId = Sketcher::GeometryFacade::getFacade(g)->getId();
        const std::string gname = "g" + std::to_string(stableId);

        if (g->is<Part::GeomPoint>()) {
            Part::TopoShape v {TopoDS::Vertex(occtShape)};
            if (!v.hasElementMap()) {
                v.resetElementMap(std::make_shared<Data::ElementMap>());
            }
            v.setElementName(
                Data::IndexedName::fromConst("Vertex", 1),
                Data::MappedName::fromRawData(gname.c_str()),
                0L
            );
            vertices.push_back(v);
            vertices.back().copyElementMap(v, Part::OpCodes::Sketch3D);
        }
        else if (g->is<Part::GeomLineSegment>()) {
            edges.push_back(makeNamedEdge(g, gname));
        }
        // Extend with Circle/Arc/Ellipse/BSpline.
    }

    if (edges.empty() && vertices.empty()) {
        return Part::TopoShape();
    }

    auto* doc = getDocument();
    Part::TopoShape result(0, doc ? doc->getStringHasher() : nullptr);

    std::vector<Part::TopoShape> all;
    if (!edges.empty()) {
        auto wires = Part::TopoShape().makeElementWires(edges, Part::OpCodes::Sketch3D);
        for (const auto& w : wires.getSubTopoShapes(TopAbs_WIRE)) {
            all.push_back(w);
        }
    }
    all.insert(all.end(), vertices.begin(), vertices.end());

    if (all.size() == 1) {
        result = all[0];
    }
    else {
        result.makeElementCompound(all);
    }

    result.Tag = getID();
    return result;
}

void Sketch3DObject::buildShapes()
{
    Shape.setValue(buildShapeForGeometry(/*construction=*/false));
    ReferenceShape.setValue(buildShapeForGeometry(/*construction=*/true));
}
