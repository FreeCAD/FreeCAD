// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <algorithm>

#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/geometry.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/ElementNamingUtils.h>
#include <App/Expression.h>
#include <App/FeaturePythonPyImp.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <App/ObjectIdentifier.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/TimeInfo.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/PartPyCXX.h>
#include <Mod/Part/App/GeometryMigrationExtension.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/App/WireJoiner.h>

#include <memory>
#include <numeric>
#include <set>
#include <sstream>

#include <TopoDS_Iterator.hxx>

#include "GeoEnum.h"
#include "SketchObject.h"
#include "Constraint.h"
#include "SketchObjectPy.h"
#include "ExternalGeometryFacade.h"


#undef DEBUG
// #define DEBUG

namespace
{
// inflation applied to every bounding box before disjointness/attribution
// tests; must cover the endpoint-merge tolerance of makeElementWires and the
// intersection tolerance of FaceMaker/WireJoiner (both Precision::Confusion)
constexpr double islandBoxTolerance = 1e-5;

// the single definition of geometric identity used by the buildShape()
// skip-cache and the island splicer: same type, same construction flag, same
// content within OCC modeling tolerance
bool isSameGeometryElement(const Part::Geometry* a, const Part::Geometry* b)
{
    return a->getTypeId() == b->getTypeId()
        && Sketcher::GeometryFacade::getConstruction(a)
        == Sketcher::GeometryFacade::getConstruction(b)
        && a->isSame(*b, Precision::Confusion(), Precision::Angular());
}

// content compare for the buildShape() skip-cache
bool isSameShapeGeometry(
    const std::vector<std::unique_ptr<Part::Geometry>>& cached,
    const std::vector<Part::Geometry*>& current
)
{
    if (cached.size() != current.size()) {
        return false;
    }
    for (std::size_t i = 0; i < cached.size(); ++i) {
        if (!isSameGeometryElement(cached[i].get(), current[i])) {
            return false;
        }
    }
    return true;
}
}  // namespace

// clang-format off
using namespace Sketcher;
using namespace Base;
namespace sp = std::placeholders;
namespace bio = boost::iostreams;

FC_LOG_LEVEL_INIT("Sketch", true, true)

PROPERTY_SOURCE(Sketcher::SketchObject, Part::Part2DObject)

SketchObject::SketchObject() : geoLastId(0)
{
    ADD_PROPERTY_TYPE(
        Geometry, (nullptr), "Sketch", (App::PropertyType)(App::Prop_None), "Sketch geometry");
    ADD_PROPERTY_TYPE(Constraints,
                      (nullptr),
                      "Sketch",
                      (App::PropertyType)(App::Prop_None),
                      "Sketch constraints");
    ADD_PROPERTY_TYPE(ExternalGeometry,
                      (nullptr, nullptr),
                      "Sketch",
                      (App::PropertyType)(App::Prop_None | App::Prop_ReadOnly),
                      "Sketch external geometry");
    ADD_PROPERTY_TYPE(ExternalTypes,
                      ({}),
                      "Sketch",
                      (App::PropertyType)(App::Prop_None | App::Prop_Hidden),
                      "Sketch external geometry type: 0 = projection, 1 = intersection, 2 = both.");
    ADD_PROPERTY_TYPE(FullyConstrained,
                      (false),
                      "Sketch",
                      (App::PropertyType)(App::Prop_Output | App::Prop_ReadOnly | App::Prop_Hidden),
                      "Sketch is fully constrained");
    ADD_PROPERTY_TYPE(Exports,
                      (nullptr),
                      "Sketch",
                      (App::PropertyType)(App::Prop_Hidden),"Sketch export geometry");
    ADD_PROPERTY_TYPE(ExternalGeo,
                      (nullptr),
                      "Sketch",
                      (App::PropertyType)(App::Prop_Hidden),"Sketch external geometry");
    ADD_PROPERTY_TYPE(ArcFitTolerance,
                      (0.0),
                      "Sketch",
                      (App::PropertyType)(App::Prop_None),
                      "Tolerance for fitting arcs of projected external geometry");
    ADD_PROPERTY(InternalShape,
                 (Part::TopoShape()));
    ADD_PROPERTY_TYPE(MakeInternals,
                      (false),
                      "Internal Geometry",
                      App::Prop_None,
                      "Enables selection of closed profiles within a sketch as input for operations");

    Geometry.setOrderRelevant(true);

    allowOtherBody = true;
    allowUnaligned = true;

    initExternalGeo();

    rebuildVertexIndex();

    lastDoF = 0;
    lastHasConflict = false;
    lastHasRedundancies = false;
    lastHasPartialRedundancies = false;
    lastHasMalformedConstraints = false;
    lastSolverStatus = 0;
    lastSolveTime = 0;

    solverNeedsUpdate = false;

    noRecomputes = false;

    //NOLINTBEGIN
    ExpressionEngine.setValidator(
        std::bind(&Sketcher::SketchObject::validateExpression, this, sp::_1, sp::_2));

    constraintsRemovedConn = Constraints.signalConstraintsRemoved.connect(
        std::bind(&Sketcher::SketchObject::constraintsRemoved, this, sp::_1));
    constraintsRenamedConn = Constraints.signalConstraintsRenamed.connect(
        std::bind(&Sketcher::SketchObject::constraintsRenamed, this, sp::_1));
    //NOLINTEND

    analyser = new SketchAnalysis(this);

    internaltransaction = false;
    managedoperation = false;

    registerElementCache(internalPrefix(), &InternalShape);
}

SketchObject::~SketchObject() {
    delete analyser;
}

void SketchObject::setupObject()
{
    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
    ArcFitTolerance.setValue(hGrpp->GetFloat("ArcFitTolerance", Precision::Confusion()*10.0));
    MakeInternals.setValue(hGrpp->GetBool("MakeInternals", true));
    inherited::setupObject();
}

short SketchObject::mustExecute() const
{
    if (Geometry.isTouched())
        return 1;
    if (Constraints.isTouched())
        return 1;
    if (ExternalGeometry.isTouched())
        return 1;
    if (ExternalGeo.isTouched())
        return 1;
    return Part2DObject::mustExecute();
}

App::DocumentObjectExecReturn* SketchObject::execute()
{
    try {
        App::DocumentObjectExecReturn* rtn = Part2DObject::execute();// to positionBySupport
        if (rtn != App::DocumentObject::StdReturn)
            // error
            return rtn;
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // setup and diagnose the sketch
    try {
        rebuildExternalGeometry();
        Constraints.acceptGeometry(getCompleteGeometry());
    }
    catch (const Base::Exception&) {
        // 9/16/24: We used to clear the constraints here, but we no longer want to do that
        // as missing reference geometry is not considered an error while we sort out sketcher UI.
        // Base::Console().error("%s\nClear constraints to external geometry\n", e.what());
        // we cannot trust the constraints of external geometries, so remove them
        //  delConstraintsToExternal();
    }

    // This includes a regular solve including full geometry update, except when an error
    // ensues
    int err = this->solve(true);

    if (err == -4) {// over-constrained sketch
        std::string msg = "Over-constrained sketch\n";
        appendConflictMsg(lastConflicting, msg);
        return new App::DocumentObjectExecReturn(msg.c_str(), this);
    }
    else if (err == -3) {// conflicting constraints
        std::string msg = "Sketch with conflicting constraints\n";
        appendConflictMsg(lastConflicting, msg);
        return new App::DocumentObjectExecReturn(msg.c_str(), this);
    }
    else if (err == -2) {// redundant constraints
        std::string msg = "Sketch with redundant constraints\n";
        appendRedundantMsg(lastRedundant, msg);
        return new App::DocumentObjectExecReturn(msg.c_str(), this);
    }
    else if (err == -5) {
        std::string msg = "Sketch with malformed constraints\n";
        appendMalformedConstraintsMsg(lastMalformedConstraints, msg);
        return new App::DocumentObjectExecReturn(msg.c_str(), this);
    }
    else if (err == -1) {// Solver failed
        return new App::DocumentObjectExecReturn("Solving the sketch failed", this);
    }

    // this is not necessary for sketch representation in edit mode, unless we want to trigger an
    // update of the objects that depend on this sketch (like pads)
    buildShape();

    return App::DocumentObject::StdReturn;
}

static bool inline checkSmallEdge(const Part::TopoShape &s) {
    if (s.shapeType() != TopAbs_EDGE)
        return false;
    BRepAdaptor_Curve adapt(TopoDS::Edge(s.getShape()));
    return GCPnts_AbscissaPoint::Length(adapt, Precision::Confusion()) <= Precision::Confusion();
}

// clang-format on
void SketchObject::buildShape()
{
    // We use the following instead to map element names

    std::vector<Part::TopoShape> shapes;
    std::vector<Part::TopoShape> vertices;
    int geoId = 0;

    auto addVertex = [&vertices](auto vertex, auto name) {
        if (!vertex.hasElementMap()) {
            vertex.resetElementMap(std::make_shared<Data::ElementMap>());
        }
        vertex.setElementName(
            Data::IndexedName::fromConst("Vertex", 1),
            Data::MappedName::fromRawData(name.c_str()),
            0L
        );
        vertices.push_back(vertex);
        vertices.back().copyElementMap(vertex, Part::OpCodes::Sketch);
    };

    auto addEdge = [this, &shapes](auto geo, auto indexedName) {
        shapes.push_back(getEdge(geo, convertSubName(indexedName, false).c_str()));
        if (checkSmallEdge(shapes.back())) {
            FC_WARN("Edge too small: " << indexedName);
        }
    };

    // get the geometry after running the solver
    auto geometries = solvedSketch.extractGeometry();

    auto isSameExternalGeometry = [this]() {
        if (static_cast<int>(builtShapeExternal.size()) != std::max(ExternalGeo.getSize() - 2, 0)) {
            return false;
        }
        for (int i = 2; i < ExternalGeo.getSize(); ++i) {
            auto geo = ExternalGeo[i];
            auto egf = ExternalGeometryFacade::getFacade(geo);
            const auto& [cachedGeo, cachedDefining] = builtShapeExternal[i - 2];
            if (egf->testFlag(ExternalGeometryExtension::Defining) != cachedDefining
                || geo->getTypeId() != cachedGeo->getTypeId()
                || !cachedGeo->isSame(*geo, Precision::Confusion(), Precision::Angular())) {
                return false;
            }
        }
        return true;
    };

    const bool shapeCacheHit = builtShapeValid && builtShapeMakeInternals == MakeInternals.getValue()
        && isSameShapeGeometry(builtShapeGeometry, geometries) && isSameExternalGeometry();

    if (shapeCacheHit) {
        // The solved geometry is content-identical to what Shape/InternalShape
        // were built from: the OCC rebuild (edges/wires/FaceMaker/WireJoiner)
        // would reproduce the same shapes, so keep the current property values.
        for (auto geo : geometries) {
            delete geo;
        }
        return;
    }

    // Record the geometry the shape is being built from (takes ownership of the
    // extracted clones). This deliberately leaves builtShapeValid == false: the
    // cache is marked valid only once Shape and InternalShape have both been
    // successfully assigned (each commit point sets builtShapeValid = true
    // below), so an exception during the OCC rebuild cannot leave a "valid"
    // cache describing a shape that was never committed.
    auto recordGeometryCache = [this, &geometries]() {
        builtShapeGeometry.clear();
        builtShapeGeometry.reserve(geometries.size());
        for (auto geo : geometries) {
            builtShapeGeometry.emplace_back(geo);
        }
        builtShapeExternal.clear();
        for (int i = 2; i < ExternalGeo.getSize(); ++i) {
            auto geo = ExternalGeo[i];
            auto egf = ExternalGeometryFacade::getFacade(geo);
            builtShapeExternal.emplace_back(
                std::unique_ptr<Part::Geometry>(geo->clone()),
                egf->testFlag(ExternalGeometryExtension::Defining)
            );
        }
        builtShapeMakeInternals = MakeInternals.getValue();
        builtShapeValid = false;
    };

    // island-local rebuild: when only some disjoint clusters moved, rebuild
    // just those and splice into the previous compounds
    Part::TopoShape splicedResult;
    Part::TopoShape splicedInternal;
    bool spliced = false;
    if (!shapeCacheHit) {
        spliced = trySpliceIslands(geometries, splicedResult, splicedInternal);
    }

    if (spliced) {
        recordGeometryCache();
        internalElementMap.clear();
        InternalShape.setValue(splicedInternal);
        Shape.setValue(splicedResult);
        builtShapeValid = true;  // commit only after both properties are assigned
        return;
    }

    bool islandSpliceable = true;
    std::vector<std::pair<int, Base::BoundBox3d>> geoBoxes;
    geoBoxes.reserve(geometries.size());

    for (auto geo : geometries) {
        ++geoId;
        if (GeometryFacade::getConstruction(geo)) {
            continue;
        }
        if (geo->isDerivedFrom<Part::GeomPoint>()) {
            islandSpliceable = false;
            int idx = getVertexIndexGeoPos(geoId - 1, Sketcher::PointPos::start);
            addVertex(
                Part::TopoShape {TopoDS::Vertex(geo->toShape())},
                convertSubName(Data::IndexedName::fromConst("Vertex", idx + 1), false)
            );
        }
        else {
            auto indexedName = Data::IndexedName::fromConst("Edge", geoId);
            addEdge(geo, indexedName);
            Base::BoundBox3d box = shapes.back().getBoundBox();
            box.Enlarge(islandBoxTolerance);
            geoBoxes.emplace_back(geoId - 1, box);
        }
    }

    recordGeometryCache();

    for (int i = 2; i < ExternalGeo.getSize(); ++i) {
        auto geo = ExternalGeo[i];
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if (!egf->testFlag(ExternalGeometryExtension::Defining)) {
            continue;
        }

        islandSpliceable = false;

        auto indexedName = Data::IndexedName::fromConst("ExternalEdge", i - 1);

        if (geo->isDerivedFrom<Part::GeomPoint>()) {
            addVertex(
                Part::TopoShape {TopoDS::Vertex(geo->toShape())},
                convertSubName(indexedName, false)
            );
        }
        else {
            addEdge(geo, indexedName);
        }
    }

    internalElementMap.clear();

    if (shapes.empty() && vertices.empty()) {
        islandCache = ShapeIslandCache {};
        InternalShape.setValue(Part::TopoShape());
        Shape.setValue(Part::TopoShape());
        builtShapeValid = true;  // commit only after both properties are assigned
        return;
    }

    Part::TopoShape result(0, getDocument()->getStringHasher());
    if (vertices.empty()) {
        // Notice here we supply op code Part::OpCodes::Sketch to makeElementWires().
        result.makeElementWires(shapes, Part::OpCodes::Sketch);
    }
    else {
        std::vector<Part::TopoShape> results;
        if (!shapes.empty()) {
            // Note, that we HAVE TO add the Part::OpCodes::Sketch op code to all
            // geometry exposed through the Shape property, because
            // SketchObject::getElementName() relies on this op code to
            // differentiate geometries that are exposed with those in edit
            // mode.
            auto wires = Part::TopoShape().makeElementWires(shapes, Part::OpCodes::Sketch);
            for (const auto& wire : wires.getSubTopoShapes(TopAbs_WIRE)) {
                results.push_back(wire);
            }
        }
        results.insert(results.end(), vertices.begin(), vertices.end());
        result.makeElementCompound(results);
    }
    result.Tag = getID();

    Part::TopoShape internal = buildInternals(result.located(TopLoc_Location()));
    InternalShape.setValue(internal);

    // Must set Shape property after InternalShape so that
    // GeoFeature::updateElementReference() can run properly on change of Shape
    // property, because some reference may pointing to the InternalShape
    Shape.setValue(result);

    rebuildIslandCache(geoBoxes, result, internal, islandSpliceable && vertices.empty());

    // commit the skip-cache only now that both Shape and InternalShape are
    // assigned and the island cache has been rebuilt
    builtShapeValid = true;
}
// clang-format off

const std::map<std::string,std::string> SketchObject::getInternalElementMap() const
{
    if (!internalElementMap.empty() || !MakeInternals.getValue())
        return internalElementMap;

    const auto& internalShape = InternalShape.getShape();
    auto shape = Shape.getShape().located(TopLoc_Location());
    if (!internalShape.isNull() && !shape.isNull()) {
        std::vector<std::string> names;
        std::string prefix;
        const std::array<TopAbs_ShapeEnum, 2> types = {TopAbs_VERTEX, TopAbs_EDGE};
        for (const auto &type : types) {
            prefix = internalPrefix() + Part::TopoShape::shapeName(type);
            std::size_t len = prefix.size();
            int i=0;
            for (const auto &v : internalShape.getSubTopoShapes(type)) {
                ++i;
                shape.findSubShapesWithSharedVertex(v, &names, Data::SearchOption::CheckGeometry
                                                |Data::SearchOption::SingleResult);
                if (names.empty())
                    continue;
                prefix += std::to_string(i);
                internalElementMap[prefix] = names.front();
                internalElementMap[names.front()] = prefix;
                prefix.resize(len);
                names.clear();
            }
        }
    }
    return internalElementMap;
}

Part::TopoShape SketchObject::buildInternals(const Part::TopoShape &edges) const {
    if (!MakeInternals.getValue())
        return Part::TopoShape();

    try {
        const auto wires = edges.getSubTopoShapes(TopAbs_WIRE);

        Part::TopoShape result(getID(), getDocument()->getStringHasher());
        try {
            result = result.makeElementFace(wires,
                    /*op*/"",
                    /*maker*/"Part::FaceMakerBuildFace",
                    /*pln*/nullptr
            );
        }
        catch (const Part::NullShapeException&) {
            // An open-only sketch has no bounded regions, so a null face result is expected.
        }

        // NOTE: do not try to skip the WireJoiner when all wires report
        // isClosed() — TestSketcherApp has sketches (self-intersecting /
        // degenerate closed wires whose face fails to build) where the
        // tight-bound analysis still yields open wires.

        // Append open wires (edges not part of any closed face)
        Part::WireJoiner joiner;
        joiner.setTightBound(true);
        joiner.setMergeEdges(true);
        joiner.addShape(edges);
        Part::TopoShape openWires(getID(), getDocument()->getStringHasher());
        joiner.getOpenWires(openWires, "SKF");

        if (openWires.isNull()) {
            return result;  // No open wires, return either face or empty toposhape
        }
        if (result.isNull()) {
            return openWires;   // No face, but we have open wires to return as a shape
        }
        return result.makeElementCompound({result, openWires}); // Compound and return both
    } catch (Base::Exception &e) {
        FC_WARN("Failed to make face for sketch: " << e.what());
    } catch (Standard_Failure &e) {
        FC_WARN("Failed to make face for sketch: " << e.GetMessageString());
    }
    return Part::TopoShape();
}

// clang-format on
void SketchObject::rebuildIslandCache(
    const std::vector<std::pair<int, Base::BoundBox3d>>& geoBoxes,
    const Part::TopoShape& result,
    const Part::TopoShape& internal,
    bool spliceable
)
{
    islandCache = ShapeIslandCache {};

    if (!spliceable || geoBoxes.size() < 2 || result.isNull() || internal.isNull()) {
        return;
    }

    // InternalShape must be faces only (single face or compound of faces): the
    // splice reproduces exactly that structure. Open wires disable splicing.
    if (internal.shapeType() != TopAbs_FACE) {
        if (internal.shapeType() != TopAbs_COMPOUND) {
            return;
        }
        for (TopoDS_Iterator it(internal.getShape()); it.More(); it.Next()) {
            if (it.Value().ShapeType() != TopAbs_FACE) {
                return;
            }
        }
    }

    // union-find over geometry bounding boxes
    const int n = static_cast<int>(geoBoxes.size());
    std::vector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);
    auto find = [&parent](int i) {
        while (parent[i] != i) {
            parent[i] = parent[parent[i]];
            i = parent[i];
        }
        return i;
    };
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (geoBoxes[i].second.Intersect(geoBoxes[j].second)) {
                int a = find(i);
                int b = find(j);
                if (a != b) {
                    parent[b] = a;
                }
            }
        }
    }

    // initial clusters
    std::map<int, int> rootToCluster;
    std::vector<std::vector<int>> memberIdx;  // indices into geoBoxes
    std::vector<Base::BoundBox3d> boxes;
    for (int i = 0; i < n; ++i) {
        int root = find(i);
        auto [it, isNew] = rootToCluster.emplace(root, static_cast<int>(memberIdx.size()));
        if (isNew) {
            memberIdx.emplace_back();
            boxes.emplace_back();
        }
        memberIdx[it->second].push_back(i);
        boxes[it->second].Add(geoBoxes[i].second);
    }

    // merge clusters whose UNION boxes overlap (covers nesting, e.g. a small
    // profile inside a bigger one, which has no pairwise edge-box overlap)
    bool merged = true;
    while (merged && memberIdx.size() > 1) {
        merged = false;
        for (std::size_t a = 0; a < memberIdx.size() && !merged; ++a) {
            for (std::size_t b = a + 1; b < memberIdx.size() && !merged; ++b) {
                if (boxes[a].Intersect(boxes[b])) {
                    memberIdx[a].insert(memberIdx[a].end(), memberIdx[b].begin(), memberIdx[b].end());
                    boxes[a].Add(boxes[b]);
                    memberIdx.erase(memberIdx.begin() + b);
                    boxes.erase(boxes.begin() + b);
                    merged = true;
                }
            }
        }
    }

    if (memberIdx.size() < 2) {
        return;  // single cluster: nothing to splice
    }

    ShapeIslandCache cache;
    cache.clusterGeos.resize(memberIdx.size());
    cache.clusterBoxes = boxes;
    for (std::size_t c = 0; c < memberIdx.size(); ++c) {
        for (int i : memberIdx[c]) {
            cache.clusterGeos[c].push_back(geoBoxes[i].first);
            cache.geoToCluster[geoBoxes[i].first] = static_cast<int>(c);
        }
        std::sort(cache.clusterGeos[c].begin(), cache.clusterGeos[c].end());
    }

    // attribute the compounds' children to clusters; every child must land in
    // exactly one cluster box (they are pairwise disjoint)
    auto attribute = [&cache](
                         const std::vector<Part::TopoShape>& children,
                         std::vector<std::vector<int>>& clusterIdx
                     ) {
        clusterIdx.assign(cache.clusterBoxes.size(), {});
        for (std::size_t i = 0; i < children.size(); ++i) {
            Base::BoundBox3d box = children[i].getBoundBox();
            int owner = -1;
            for (std::size_t c = 0; c < cache.clusterBoxes.size(); ++c) {
                if (box.Intersect(cache.clusterBoxes[c])) {
                    if (owner >= 0) {
                        return false;  // ambiguous
                    }
                    owner = static_cast<int>(c);
                }
            }
            if (owner < 0) {
                return false;
            }
            clusterIdx[owner].push_back(static_cast<int>(i));
        }
        return true;
    };

    cache.wireShapes = result.getSubTopoShapes(TopAbs_WIRE);
    cache.faceShapes = internal.getSubTopoShapes(TopAbs_FACE);
    if (cache.wireShapes.size() < 2 || cache.faceShapes.empty()) {
        return;
    }

    // Multi-wire faces (nested profiles/holes) are excluded: FaceMaker's
    // postBuild() threads name-collision state across faces and maps hole
    // wires at the compound level, which a spliced compound cannot reproduce
    // exactly (e.g. a rect-in-rect sketch).
    for (const auto& face : cache.faceShapes) {
        if (face.countSubShapes(TopAbs_WIRE) != 1) {
            return;
        }
    }

    if (!attribute(cache.wireShapes, cache.clusterWires)
        || !attribute(cache.faceShapes, cache.clusterFaces)) {
        return;
    }

    cache.valid = true;
    islandCache = std::move(cache);
}

bool SketchObject::trySpliceIslandDeletion(
    const std::vector<Part::Geometry*>& geometries,
    Part::TopoShape& newResult,
    Part::TopoShape& newInternal
)
{
    // Align: the new geometry list must be an ordered subsequence of the
    // cached one (deletion preserves relative order). Any surviving geometry
    // that moved or changed type breaks the alignment -> full rebuild.
    std::vector<int> deletedOld;
    std::size_t j = 0;
    for (std::size_t i = 0; i < builtShapeGeometry.size(); ++i) {
        const Part::Geometry* cached = builtShapeGeometry[i].get();
        if (j < geometries.size() && isSameGeometryElement(cached, geometries[j])) {
            ++j;
        }
        else {
            deletedOld.push_back(static_cast<int>(i));
        }
    }
    if (j != geometries.size()) {
        return false;  // not a pure ordered deletion
    }

    // deleted construction geometry has no shape; collect the clusters of the
    // shape-relevant deleted geometry
    std::set<int> deletedClusters;
    std::set<int> deletedShapeGeos;
    for (int gi : deletedOld) {
        if (GeometryFacade::getConstruction(builtShapeGeometry[gi].get())) {
            continue;
        }
        auto it = islandCache.geoToCluster.find(gi);
        if (it == islandCache.geoToCluster.end()) {
            return false;  // unattributed shape-relevant geometry
        }
        deletedClusters.insert(it->second);
        deletedShapeGeos.insert(gi);
    }

    if (deletedClusters.empty()) {
        // Only construction geometry was deleted: the shapes are unchanged,
        // but the surviving geometries' absolute indices shifted, so the
        // cache's geo indexing must be re-keyed (the caller re-bases
        // builtShapeGeometry to the new list).
        std::map<int, int> oldToNew;
        {
            std::set<int> deletedSet(deletedOld.begin(), deletedOld.end());
            int nj = 0;
            for (std::size_t i = 0; i < builtShapeGeometry.size(); ++i) {
                if (deletedSet.find(static_cast<int>(i)) == deletedSet.end()) {
                    oldToNew[static_cast<int>(i)] = nj++;
                }
            }
        }
        std::map<int, int> geoToCluster;
        std::vector<std::vector<int>> clusterGeos(islandCache.clusterGeos.size());
        for (std::size_t c = 0; c < islandCache.clusterGeos.size(); ++c) {
            for (int gi : islandCache.clusterGeos[c]) {
                auto it = oldToNew.find(gi);
                if (it == oldToNew.end()) {
                    // a cluster references a deleted geometry; cannot happen for
                    // construction-only deletions, but never keep a stale cache
                    islandCache = ShapeIslandCache {};
                    return false;
                }
                geoToCluster[it->second] = static_cast<int>(c);
                clusterGeos[c].push_back(it->second);
            }
        }
        islandCache.geoToCluster = std::move(geoToCluster);
        islandCache.clusterGeos = std::move(clusterGeos);

        newResult = Shape.getShape();
        newInternal = InternalShape.getShape();
        return true;
    }

    // every touched cluster must be deleted entirely: a partially deleted
    // island changes its wire/face and needs the full pipeline
    for (int c : deletedClusters) {
        for (int gi : islandCache.clusterGeos[c]) {
            if (deletedShapeGeos.find(gi) == deletedShapeGeos.end()) {
                return false;
            }
        }
    }
    if (deletedClusters.size() >= islandCache.clusterBoxes.size()) {
        return false;  // nothing left to reuse
    }

    // positions of the deleted clusters' children in the compounds
    std::set<int> deadWires;
    std::set<int> deadFaces;
    for (int c : deletedClusters) {
        if (islandCache.clusterWires[c].size() != 1 || islandCache.clusterFaces[c].size() != 1) {
            return false;
        }
        deadWires.insert(islandCache.clusterWires[c].front());
        deadFaces.insert(islandCache.clusterFaces[c].front());
    }

    std::vector<Part::TopoShape> wires;
    std::vector<Part::TopoShape> faces;
    for (std::size_t i = 0; i < islandCache.wireShapes.size(); ++i) {
        if (deadWires.find(static_cast<int>(i)) == deadWires.end()) {
            wires.push_back(islandCache.wireShapes[i]);
        }
    }
    for (std::size_t i = 0; i < islandCache.faceShapes.size(); ++i) {
        if (deadFaces.find(static_cast<int>(i)) == deadFaces.end()) {
            faces.push_back(islandCache.faceShapes[i]);
        }
    }
    if (wires.empty() || faces.empty()) {
        return false;
    }

    try {
        Part::TopoShape result(0, getDocument()->getStringHasher());
        result.makeElementCompound(wires);
        result.Tag = getID();

        Part::TopoShape internal(getID(), getDocument()->getStringHasher());
        internal.makeElementCompound(faces);

        // re-key the cache against the new geometry indexing
        std::map<int, int> oldToNew;
        {
            std::set<int> deletedSet(deletedOld.begin(), deletedOld.end());
            int nj = 0;
            for (std::size_t i = 0; i < builtShapeGeometry.size(); ++i) {
                if (deletedSet.find(static_cast<int>(i)) == deletedSet.end()) {
                    oldToNew[static_cast<int>(i)] = nj++;
                }
            }
        }
        auto newPos = [](const std::set<int>& dead, int old) {
            int shift = 0;
            for (int d : dead) {
                if (d < old) {
                    ++shift;
                }
                else {
                    break;
                }
            }
            return old - shift;
        };

        ShapeIslandCache cache;
        cache.wireShapes = wires;
        cache.faceShapes = faces;
        for (std::size_t c = 0; c < islandCache.clusterGeos.size(); ++c) {
            if (deletedClusters.find(static_cast<int>(c)) != deletedClusters.end()) {
                continue;
            }
            std::vector<int> geos;
            for (int gi : islandCache.clusterGeos[c]) {
                auto it = oldToNew.find(gi);
                if (it == oldToNew.end()) {
                    return false;
                }
                geos.push_back(it->second);
            }
            const int nc = static_cast<int>(cache.clusterGeos.size());
            for (int g : geos) {
                cache.geoToCluster[g] = nc;
            }
            cache.clusterGeos.push_back(std::move(geos));
            cache.clusterBoxes.push_back(islandCache.clusterBoxes[c]);
            std::vector<int> cw;
            std::vector<int> cf;
            for (int w : islandCache.clusterWires[c]) {
                cw.push_back(newPos(deadWires, w));
            }
            for (int f : islandCache.clusterFaces[c]) {
                cf.push_back(newPos(deadFaces, f));
            }
            cache.clusterWires.push_back(std::move(cw));
            cache.clusterFaces.push_back(std::move(cf));
        }
        cache.valid = cache.clusterBoxes.size() >= 2;
        islandCache = std::move(cache);

        newResult = result;
        newInternal = internal;
        return true;
    }
    catch (Base::Exception& e) {
        FC_WARN("island deletion splice failed, falling back to full rebuild: " << e.what());
    }
    catch (Standard_Failure& e) {
        FC_WARN(
            "island deletion splice failed, falling back to full rebuild: " << e.GetMessageString()
        );
    }
    islandCache = ShapeIslandCache {};
    return false;
}

bool SketchObject::trySpliceIslands(
    const std::vector<Part::Geometry*>& geometries,
    Part::TopoShape& newResult,
    Part::TopoShape& newInternal
)
{
    if (!islandCache.valid || !builtShapeValid
        || builtShapeMakeInternals != MakeInternals.getValue() || !MakeInternals.getValue()) {
        return false;
    }
    // strict scope: no external geometry beyond the axes
    if (ExternalGeo.getSize() > 2 || !builtShapeExternal.empty()) {
        return false;
    }
    if (builtShapeGeometry.size() != geometries.size()) {
        if (geometries.size() < builtShapeGeometry.size()) {
            // pure deletion of whole islands can be spliced too
            return trySpliceIslandDeletion(geometries, newResult, newInternal);
        }
        return false;
    }

    // diff against the geometry the current shapes were built from
    std::vector<int> changed;
    for (std::size_t i = 0; i < geometries.size(); ++i) {
        const Part::Geometry* cached = builtShapeGeometry[i].get();
        const Part::Geometry* current = geometries[i];
        if (cached->getTypeId() != current->getTypeId()
            || GeometryFacade::getConstruction(cached) != GeometryFacade::getConstruction(current)) {
            return false;  // structural change
        }
        if (!cached->isSame(*current, Precision::Confusion(), Precision::Angular())) {
            changed.push_back(static_cast<int>(i));
        }
    }
    if (changed.empty()) {
        return false;  // the skip-cache handles this
    }

    std::set<int> changedClusters;
    for (int gi : changed) {
        if (GeometryFacade::getConstruction(geometries[gi])) {
            continue;  // construction geometry has no shape
        }
        auto it = islandCache.geoToCluster.find(gi);
        if (it == islandCache.geoToCluster.end()) {
            return false;  // unattributed shape-relevant geometry
        }
        changedClusters.insert(it->second);
    }
    if (changedClusters.empty()) {
        // only construction geometry moved: the shapes are unchanged
        newResult = Shape.getShape();
        newInternal = InternalShape.getShape();
        return true;
    }
    if (changedClusters.size() >= islandCache.clusterBoxes.size()) {
        return false;  // nothing to reuse
    }

    // rebuild each changed cluster in isolation
    struct RebuiltCluster
    {
        int cluster;
        Part::TopoShape wire;
        Part::TopoShape face;
        Base::BoundBox3d box;
    };
    std::vector<RebuiltCluster> rebuilt;
    rebuilt.reserve(changedClusters.size());

    try {
        for (int c : changedClusters) {
            // strict pairing: exactly one wire and one face to replace
            if (islandCache.clusterWires[c].size() != 1 || islandCache.clusterFaces[c].size() != 1) {
                return false;
            }

            std::vector<Part::TopoShape> clusterEdges;
            Base::BoundBox3d box;
            for (int gi : islandCache.clusterGeos[c]) {
                const Part::Geometry* geo = geometries[gi];
                auto indexedName = Data::IndexedName::fromConst("Edge", gi + 1);
                Part::TopoShape edge = getEdge(geo, convertSubName(indexedName, false).c_str());
                if (edge.isNull()) {
                    return false;
                }
                clusterEdges.push_back(edge);
                box.Add(edge.getBoundBox());
            }
            box.Enlarge(islandBoxTolerance);

            Part::TopoShape wireComp(0, getDocument()->getStringHasher());
            wireComp.makeElementWires(clusterEdges, Part::OpCodes::Sketch);
            auto newWires = wireComp.getSubTopoShapes(TopAbs_WIRE);
            if (newWires.size() != 1) {
                return false;
            }

            Part::TopoShape faceShape(getID(), getDocument()->getStringHasher());
            faceShape = faceShape.makeElementFace(
                newWires,
                /*op*/ "",
                /*maker*/ "Part::FaceMakerBuildFace",
                /*pln*/ nullptr
            );
            auto newFaces = faceShape.getSubTopoShapes(TopAbs_FACE);
            if (newFaces.size() != 1) {
                return false;
            }

            // no open wires may appear
            Part::WireJoiner joiner;
            joiner.setTightBound(true);
            joiner.setMergeEdges(true);
            joiner.addShape(clusterEdges);
            Part::TopoShape openWires(getID(), getDocument()->getStringHasher());
            joiner.getOpenWires(openWires, "SKF");
            if (!openWires.isNull()) {
                return false;
            }

            rebuilt.push_back({c, newWires.front(), newFaces.front(), box});
        }

        // the moved clusters must remain disjoint from everything else
        for (const auto& rb : rebuilt) {
            for (std::size_t c = 0; c < islandCache.clusterBoxes.size(); ++c) {
                if (static_cast<int>(c) == rb.cluster) {
                    continue;
                }
                const Base::BoundBox3d* other = &islandCache.clusterBoxes[c];
                for (const auto& rb2 : rebuilt) {
                    if (rb2.cluster == static_cast<int>(c)) {
                        other = &rb2.box;
                        break;
                    }
                }
                if (rb.box.Intersect(*other)) {
                    return false;
                }
            }
        }

        // positional splice
        std::vector<Part::TopoShape> wires = islandCache.wireShapes;
        std::vector<Part::TopoShape> faces = islandCache.faceShapes;
        for (const auto& rb : rebuilt) {
            wires[islandCache.clusterWires[rb.cluster].front()] = rb.wire;
            faces[islandCache.clusterFaces[rb.cluster].front()] = rb.face;
        }

        Part::TopoShape result(0, getDocument()->getStringHasher());
        result.makeElementCompound(wires);
        result.Tag = getID();

        Part::TopoShape internal(getID(), getDocument()->getStringHasher());
        internal.makeElementCompound(faces);

        // commit to the cache
        for (const auto& rb : rebuilt) {
            islandCache.wireShapes[islandCache.clusterWires[rb.cluster].front()] = rb.wire;
            islandCache.faceShapes[islandCache.clusterFaces[rb.cluster].front()] = rb.face;
            islandCache.clusterBoxes[rb.cluster] = rb.box;
        }

        newResult = result;
        newInternal = internal;
        return true;
    }
    catch (Base::Exception& e) {
        FC_WARN("island splice failed, falling back to full rebuild: " << e.what());
    }
    catch (Standard_Failure& e) {
        FC_WARN("island splice failed, falling back to full rebuild: " << e.GetMessageString());
    }
    return false;
}
// clang-format off

static const char *hasSketchMarker(const char *name) {
    static std::string marker(Part::TopoShape::elementMapPrefix()+Part::OpCodes::Sketch);
    if (!name)
        return nullptr;
    return strstr(name,marker.c_str());
}

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

// NOLINTNEXTLINE
BOOST_GEOMETRY_REGISTER_POINT_3D(Base::Vector3d, double, bg::cs::cartesian, x, y, z)

class SketchObject::GeoHistory
{
private:
    static constexpr int bgiMaxElements = 16;

    using Parameters = bgi::linear<bgiMaxElements>;
    using IdSet = std::set<long>;
    using IdSets = std::pair<IdSet, IdSet>;
    using AdjList = std::list<IdSet>;

    // associate a geo with connected ones on both points
    using AdjMap = std::map<long, IdSets>;

    // maps start/end points to all existing geo to query and update adjacencies
    using Value = std::pair<Base::Vector3d, AdjList::iterator>;

    AdjList adjlist;
    AdjMap adjmap;
    bgi::rtree<Value,Parameters> rtree;

public:
    AdjList::iterator find(const Base::Vector3d &pt,bool strict=true){
        std::vector<Value> ret;
        rtree.query(bgi::nearest(pt, 1), std::back_inserter(ret));
        if (!ret.empty()) {
            // NOTE: we are using square distance here, the 1e-6 threshold is
            // very forgiving. We should have used Precision::SquareConfisuion(),
            // which is 1e-14. However, there is a problem with current
            // commandGeoCreate. They create new geometry with initial point of
            // the exact mouse position, instead of the preselected point
            // position, and rely on auto constraint to snap in the new
            // geometry. So, we cannot use a very strict threshold here.
            double tol = strict?Precision::SquareConfusion()*10:1e-6;
            double d = Base::DistanceP2(ret[0].first,pt);
            if(d<tol) {
                return ret[0].second;
            }
        }
        return adjlist.end();
    }

    void clear() {
        rtree.clear();
        adjlist.clear();
    }

    void update(const Base::Vector3d &pt, long id) {
        FC_TRACE("update " << id << ", " << FC_xyz(pt));
        auto it = find(pt);
        if(it==adjlist.end()) {
            adjlist.emplace_back();
            it = adjlist.end();
            --it;
            rtree.insert(std::make_pair(pt,it));
        }
        it->insert(id);
    }

    void finishUpdate(const std::map<long,int> &geomap) {
        IdSet oldset;
        for(auto &idset : adjlist) {
            oldset.clear();
            for(long _id : idset) {
                long id = abs(_id);
                auto& v = adjmap[id];
                auto& adj = _id > 0 ? v.first : v.second;
                for (auto it = adj.begin(); it != adj.end(); /* don't advance here */) {
                    long other = *it;
                    auto removeId = it++;  // grab ID we might erase, and advance
                    if (geomap.find(other) == geomap.end()) {
                        // remember those deleted IDs to swap in below
                        oldset.insert(other);
                    }
                    else if (idset.find(other) == idset.end()) {
                        // delete any existing IDs that are no longer in the adj list
                        adj.erase(removeId);
                    }
                }
                // now merge the current ones
                for(long _id2 : idset) {
                    long id2 = abs(_id2);
                    if(id!=id2) {
                        adj.insert(id2);
                    }
                }
            }
            // now reset the adjacency list with only those deleted id's,
            // because the whole purpose of this history is to try to reuse
            // deleted id.
            idset.swap(oldset);
        }
    }

    AdjList::iterator end() {
        return adjlist.end();
    }

    size_t size() {
        return rtree.size();
    }
};

void SketchObject::updateGeoHistory() {
    if(!geoHistoryLevel) return;

    if (!geoHistory) {
        geoHistory = std::make_unique<GeoHistory>();
    }

    Base::TimeTracker tracker("updateGeoHistory");
    const auto &geos = getInternalGeometry();
    geoHistory->clear();
    for (auto geo : geos) {
        auto pstart = getPoint(geo, PointPos::start);
        auto pend = getPoint(geo, PointPos::end);
        int id = GeometryFacade::getId(geo);
        geoHistory->update(pstart,id);
        if(pstart!=pend)
            geoHistory->update(pend,-id);
    }
    geoHistory->finishUpdate(geoMap);
}

// clang-format on
void SketchObject::generateId(const Part::Geometry* geo)
{
    auto preReturn = [this, &geo](auto& newId) {
        GeometryFacade::setId(geo, newId);
        geoMap[Sketcher::GeometryFacade::getId(geo)] = (long)Geometry.getSize();
    };

    auto isNotInGeoMap = [this](auto& id) {
        if (geoMap.find(id) == geoMap.end()) {
            return true;
        }
        FC_TRACE("ignore " << id);
        return false;
    };

    if (geoHistoryLevel == 0) {
        preReturn(++geoLastId);
        return;
    }

    if (!geoHistory) {
        updateGeoHistory();
    }

    // Search geo history to see if the start point and end point belongs to
    // some deleted geometries. Prefer matching both start and end point. If
    // can't then try start and then end. Generate new id if none is found.
    auto pstart = getPoint(geo, PointPos::start);
    auto it = geoHistory->find(pstart, false);
    auto pend = getPoint(geo, PointPos::end);
    auto it2 = it;
    if (pstart != pend) {
        it2 = geoHistory->find(pend, false);
        if (it2 == geoHistory->end()) {
            it2 = it;
        }
    }
    std::vector<long> found;

    if (geoHistoryLevel <= 1 && (it == geoHistory->end() || it2 == it)) {
        // level <= 1 means we only reuse id if both start and end matches
        preReturn(++geoLastId);
        return;
    }

    if (it != geoHistory->end()) {
        // `find_if` avoids checking twice
        auto iterOfId = std::ranges::find_if(*it, isNotInGeoMap);
        if (iterOfId != it->end() && it2 == it) {
            preReturn(*iterOfId);
            return;
        }
        std::copy_if(iterOfId, it->end(), std::back_inserter(found), isNotInGeoMap);
    }
    if (found.empty()) {
        // no candidate exists
        if (it2 == it) {
            preReturn(++geoLastId);
            return;
        }
        auto iterOfId = std::ranges::find_if(*it, isNotInGeoMap);
        if (iterOfId != it->end()) {
            preReturn(*iterOfId);
            return;
        }
        preReturn(++geoLastId);
        return;
    }

    auto isInIt2 = [&it2](auto& id) {
        if (it2->find(id) != it2->end()) {
            return true;
        }
        FC_TRACE("ignore " << id);
        return false;
    };

    // already some candidate exists, search for matching of both
    // points
    if (it2 != it) {
        auto iterOfId = std::ranges::find_if(found, isInIt2);
        if (iterOfId != found.end()) {
            preReturn(*iterOfId);
            return;
        }
    }
    FC_TRACE("found " << found.front());
    preReturn(found.front());
}
// clang-format off

int SketchObject::setTextAndFont(int ConstrId, std::string& newText, std::string& newFont, bool isHeight, bool isConstruction)
{
;    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // set the changed value for the constraint
    if (this->Constraints.hasInvalidGeometry()) {
        return -6;
    }
    const std::vector<Constraint*>& vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(vals.size())) {
        return -1;
    }

    auto* constr = vals[ConstrId];
    if (constr->Type != Text || !constr->hasElement(0)) {
        return -1;
    }

    // First we replace the old geometries by the new text.
    const std::string oldText = constr->getText();
    const std::string oldFont = constr->getFont();
    const bool oldIsHeight = constr->getIsTextHeight();
    int handleGeoId = constr->getGeoId(0);
    int firstTextGeoId = constr->getGeoId(1);
    bool hasExistingText = firstTextGeoId != GeoEnum::GeoUndef;
    bool handleLast = handleGeoId > firstTextGeoId;

    if (hasExistingText) {
        // Check if text is construction or normal geos
        auto* geo1 = getGeometry(firstTextGeoId);
        isConstruction = GeometryFacade::getConstruction(geo1);

        // Delete all the old text geos. Not the handle!
        std::vector<int> geoIdsToDelete;
        for (int i = 1; constr->hasElement(i); ++i) {
            if (constr->getGeoId(i) == GeoEnum::GeoUndef) {
                continue;
            }
            geoIdsToDelete.push_back(constr->getGeoId(i));
            if (handleLast) {
                --handleGeoId; // handle line is added after all text geos.
            }
        }

        delGeometries(geoIdsToDelete);
    }

    auto* line = dynamic_cast<const Part::GeomLineSegment*>(getGeometry(handleGeoId));
    if (!line) {
        return -1;
    }

    // Generate text geos based on new text/font :
    std::vector<std::unique_ptr<Part::Geometry>> newGeos;
    std::vector<TopoDS_Shape> shapes = Part::makeTextWires(newText, newFont);
    Part::transformAndConvertToGeometry(newGeos,
                                    shapes,
                                    line->getStartPoint(),
                                    line->getEndPoint(),
                                    isHeight);

    // Add the geometries to sketch
    int lastGeoid = getHighestCurveIndex();
    std::vector<Part::Geometry*> newGeosRawPtrs;
    newGeosRawPtrs.reserve(newGeos.size());

    // Populate the raw pointer vector and release ownership from the unique_ptrs.
    for (auto& geo_ptr : newGeos) {
        if (isConstruction) {
            Sketcher::GeometryFacade::setConstruction(geo_ptr.get(), isConstruction);
        }
        // Add the raw pointer to the new vector.
        newGeosRawPtrs.push_back(geo_ptr.get());
        // Release ownership from the unique_ptr. The SketchObject will now manage this memory.
        geo_ptr.release();
    }
    newGeos.clear();
    addGeometry(newGeosRawPtrs);

    int newLastGeoid = getHighestCurveIndex();

    // If there was text geos, they were deleted, which deleted the text constraint.
    // In this case create a new constraint to replace it.
    if (hasExistingText) {
        constr = new Constraint();
        constr->Type = Text;
        constr->truncateElements(0); // remove the First/Second/Third that are created automatically
        constr->addElement(GeoElementId(handleGeoId));
    }
    for (int i = lastGeoid + 1; i <= newLastGeoid; ++i) {
        constr->addElement(GeoElementId(i));
    }
    constr->setText(newText);
    constr->setFont(newFont);
    constr->setIsTextHeight(isHeight);

    if (hasExistingText) {
        addConstraint(constr);
    }

    int err = solve();

    if (err) {
        constr->setText(oldText);
        constr->setFont(oldFont);
        constr->setIsTextHeight(oldIsHeight);
    }

    return err;
}

void SketchObject::acceptGeometry()
{
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    signalElementsChanged();
}

int SketchObject::setGeometry(int GeoId, const Part::Geometry *geo) {
    std::unique_ptr<Part::Geometry> g(geo->clone());
    if(GeoId>=0 && GeoId <Geometry.getSize()) {
        Geometry.set1Value(GeoId,std::move(g));
    } else if(GeoId <= GeoEnum::RefExt && -GeoId-1 < ExternalGeo.getSize()) {
        ExternalGeo.set1Value(-GeoId-1,std::move(g));
    } else
        return -1;
    return 0;
}

bool SketchObject::evaluateSupport()
{
    // returns false if the shape is broken, null or non-planar
    App::DocumentObject* link = AttachmentSupport.getValue();
    if (!link || !link->isDerivedFrom<Part::Feature>())
        return false;
    return true;
}

bool SketchObject::isInGroup(int geoId, bool includeHandle) const
{
    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (const auto& constr : vals) {
        if (constr->Type == Group || constr->Type == Text) {
            // First is the group construction line. We include it or not in our search.
            int iStart = includeHandle ? 0 : 1;
            for (int i = iStart; constr->hasElement(i); ++i) {
                if (constr->getGeoId(i) == geoId) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool SketchObject::isGroupHandle(int geoId) const
{
    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (const auto& constr : vals) {
        if (constr->Type == Group || constr->Type == Text) {
            if (constr->getGeoId(0) == geoId) {
                return true;
            }
        }
    }
    return false;
}

int SketchObject::getGroupHandleIfInGroup(int geoId)
{
    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (const auto& constr : vals) {
        if (constr->Type == Group || constr->Type == Text) {
            // First is the group construction line.
            int groupHandleGeoId = -1;
            for (int i = 0; constr->hasElement(i); ++i) {
                if (i == 0) {
                    groupHandleGeoId = constr->getGeoId(i);
                }
                else if (constr->getGeoId(i) == geoId) {
                    return groupHandleGeoId;
                }
            }
        }
    }
    return geoId;
}

std::set<int> SketchObject::getGroupGeometries(int handleGeoId) const
{
    std::set<int> geoIds;
    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();
    for (const auto& constr : vals) {
        if (constr->Type == Group || constr->Type == Text) {
            if (constr->getGeoId(0) == handleGeoId) {
                for (int i = 1; constr->hasElement(i); ++i) {
                    geoIds.insert(constr->getElement(i).GeoId);
                }
            }
        }
    }
    return geoIds;
}

PyObject* SketchObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new SketchObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

unsigned int SketchObject::getMemSize() const
{
    return 0;
}

void SketchObject::Save(Writer& writer) const
{
    int index = -1;
    auto &geos = const_cast<Part::PropertyGeometryList&>(ExternalGeo).getValues();
    for(auto geo : geos)
        ExternalGeometryFacade::getFacade(geo)->setRefIndex(-1);

    if(isExporting()) {
        // We cannot export shape with the new topological naming, because it
        // uses hasher indices that are unique only within its owner document.
        // Therefore, we cannot rely on Geometry::Ref as key to map geometry to
        // external object reference. So, before exporting, we pre-calculate
        // the mapping and store them in Geometry::RefIndex. When importing,
        // inside updateGeometryRefs() (called by onDocumentRestore()), we shall
        // regenerate Geometry::Ref based on RefIndex.
        //
        // Note that the regenerated Ref will not be using the new topological
        // naming either, because we didn't export them.  This is exactly the
        // same as if we are opening a legacy file without new names.
        // updateGeometryRefs() will know how to handle the name change thanks
        // to a flag setup in onUpdateElementReference().
        for(auto &key : externalGeoRef) {
            ++index;
            auto iter = externalGeoRefMap.find(key);
            if(iter == externalGeoRefMap.end())
                continue;
            for(auto id : iter->second) {
                auto it = externalGeoMap.find(id);
                if(it != externalGeoMap.end())
                    ExternalGeometryFacade::getFacade(geos[it->second])->setRefIndex(index);
            }
        }
    }

    // save the father classes
    Part::Part2DObject::Save(writer);
}

void SketchObject::Restore(XMLReader& reader)
{
    // read the father classes
    Part::Part2DObject::Restore(reader);
}

void SketchObject::handleChangedPropertyType(Base::XMLReader &reader,
        const char *TypeName, App::Property *prop)
{
    if (prop == &Exports) {
        if(strcmp(TypeName, "App::PropertyLinkList") == 0)
            Exports.Restore(reader);
    }
}

static inline bool checkMigration(Part::PropertyGeometryList &prop)
{
    for (auto g : prop.getValues()) {
        if(g->hasExtension(Part::GeometryMigrationExtension::getClassTypeId())
            || !g->hasExtension(SketchGeometryExtension::getClassTypeId()))
            return true;
    }
    return false;
}

void SketchObject::onChanged(const App::Property* prop)
{
    if (prop == &Geometry) {
        onGeometryChanged();
    }
    else if (prop == &Constraints) {
        onConstraintsChanged();
    }
    else if (prop == &ExternalGeo && !prop->testStatus(App::Property::User3)) {
        onExternalGeoChanged();
    }
    else if (prop == &ExternalGeometry) {
        onExternalGeometryChanged();
    }
    else if (prop == &Placement) {
        onPlacementChanged();
    }
    else if (prop == &ExpressionEngine) {
        onExpressionEngineChanged();
    }
#if 0
    // For now do not delete anything (#0001791). When changing the support
    // face it might be better to check which external geometries can be kept.
    else if (prop == &AttachmentSupport) {
        onAttachmentSupportChanged();
    }
#endif
    Part::Part2DObject::onChanged(prop);
}

void SketchObject::onGeometryChanged()
{
    if (isRestoring() && checkMigration(Geometry)) {
        // Construction migration to extension
        for (auto geometryValue : Geometry.getValues()) {
            if (!geometryValue->hasExtension(
                      Part::GeometryMigrationExtension::getClassTypeId())) {
                continue;
            }

            auto ext = std::static_pointer_cast<Part::GeometryMigrationExtension>(
                geometryValue
                ->getExtension(Part::GeometryMigrationExtension::getClassTypeId())
                .lock());

            // at this point IA geometry is already migrated
            auto gf = GeometryFacade::getFacade(geometryValue);

            if (ext->testMigrationType(Part::GeometryMigrationExtension::Construction)) {
                bool oldconstr = ext->getConstruction()
                    || (geometryValue->is<Part::GeomPoint>() && !gf->isInternalAligned());
                gf->setConstruction(oldconstr);
            }
            if (ext->testMigrationType(Part::GeometryMigrationExtension::GeometryId)) {
                gf->setId(ext->getId());
            }
        }
    }
    geoMap.clear();
    const auto &vals = getInternalGeometry();
    for (long i = 0; i < (long)vals.size(); ++i) {
        auto geo = vals[i];
        auto gf = GeometryFacade::getFacade(geo);
        if (gf->getId() == 0) {
            gf->setId(++geoLastId);
        }
        else if (gf->getId() > geoLastId) {
            geoLastId = gf->getId();
        }
        while (!geoMap.insert(std::make_pair(gf->getId(), i)).second) {
            FC_WARN("duplicate geometry id " << gf->getId() << " -> "
                    << geoLastId + 1);  // NOLINT
            gf->setId(++geoLastId);
        }
    }
    updateGeoHistory();

    auto doc = getDocument();

    if (doc && doc->isPerformingTransaction()) {
        // undo/redo
        setStatus(App::PendingTransactionUpdate, true);
        return;
    }

    if (internaltransaction) {
        solverNeedsUpdate = true;
        return;
    }

    // During an interactive drag, geometry property changes are triggered
    // by the recompute system. The geometry is in a transitional state, so
    // checkConstraintIndices() would fail against the partially-updated
    // geometry. Defer the update until the drag ends.
    if (isDragActive) {
        solverNeedsUpdate = true;
        return;
    }

    // internal sketchobject operations changing both geometry and constraints will
    // explicitly perform an update

    if (managedoperation || isRestoring()) {
        // if geometry changed, the constraint geometry indices must be updated
        acceptGeometry();
        return;
    }

    // External geometry edit — topology may have changed. Invalidate the
    // diagnosis cache HERE (not at the top of the function) so that solver
    // writeback (managedoperation == true) preserves the cache.
    solvedSketch.invalidateDiagnosisCache();

    // this change was not effect via SketchObject, but using direct access to
    // properties, check input data

    // declares constraint invalid if indices go beyond the geometry and any
    // call to getValues with return an empty list until this is fixed.
    bool invalidinput = Constraints.checkConstraintIndices(
        getHighestCurveIndex(), -getExternalGeometryCount());

    if (!invalidinput) {
        acceptGeometry();
    }
    else {
        Base::Console().error(
            this->getFullLabel() + " SketchObject::onChanged ",
            QT_TRANSLATE_NOOP("Notifications", "Unmanaged change of Constraint "
                              "Property results in invalid constraint indices") "\n");
    }
    Base::StateLocker lock(internaltransaction, true);
    setUpSketch();
}

void SketchObject::onConstraintsChanged()
{
    auto doc = getDocument();

    if (doc && doc->isPerformingTransaction()) {
        // undo/redo
        setStatus(App::PendingTransactionUpdate, true);
        return;
    }

    if (internaltransaction) {
        solverNeedsUpdate = true;
        return;
    }

    // During an interactive drag, constraint property changes are triggered
    // by the recompute system, not by user constraint edits. The geometry is
    // in a transitional state (new elements being added), so
    // checkConstraintIndices() would fail and wipe all constraints. Defer
    // the update until the drag ends.
    if (isDragActive) {
        solverNeedsUpdate = true;
        return;
    }

    if (managedoperation || isRestoring()) {
        Constraints.checkGeometry(getCompleteGeometry());
        return;
    }

    // this change was not effect via SketchObject, but using direct access to
    // properties, check input data

    // declares constraint invalid if indices go beyond the geometry and any
    // call to getValues with return an empty list until this is fixed.
    bool invalidinput = Constraints.checkConstraintIndices(
        getHighestCurveIndex(), -getExternalGeometryCount());

    if (!invalidinput) {
        if (Constraints.checkGeometry(getCompleteGeometry())) {
            // if there are invalid geometry indices in the constraints, we need
            // to update them
            acceptGeometry();
        }
    }
    else {
        Base::Console().error(
            this->getFullLabel() + " SketchObject::onChanged ",
            QT_TRANSLATE_NOOP("Notifications", "Unmanaged change of Constraint "
                              "Property results in invalid constraint indices") "\n");
    }
    Base::StateLocker lock(internaltransaction, true);
    setUpSketch();
}

/// not to be confused with `onExternalGeometryChanged`. These names may need fixing.
void SketchObject::onExternalGeoChanged()
{
    if (ExternalGeo.testStatus(App::Property::User3)) {
        return;
    }

    auto doc = getDocument();

    if (doc && doc->isPerformingTransaction()) {
        setStatus(App::PendingTransactionUpdate, true);
    }

    if (isRestoring() && checkMigration(ExternalGeo)) {
        for (auto geometryValue : ExternalGeo.getValues()) {
            if (!geometryValue->hasExtension(
                    Part::GeometryMigrationExtension::getClassTypeId())) {
                continue;
            }

            auto ext = std::static_pointer_cast<Part::GeometryMigrationExtension>(
                geometryValue
                ->getExtension(Part::GeometryMigrationExtension::getClassTypeId())
                .lock());
            std::unique_ptr<ExternalGeometryFacade> egf;
            if (ext->testMigrationType(Part::GeometryMigrationExtension::GeometryId)) {
                egf = ExternalGeometryFacade::getFacade(geometryValue);
                egf->setId(ext->getId());
            }

            if (!ext->testMigrationType(Part::GeometryMigrationExtension::ExternalReference)) {
                continue;
            }

            if (!egf) {
                egf = ExternalGeometryFacade::getFacade(geometryValue);
            }
            egf->setRef(ext->getRef());
            egf->setRefIndex(ext->getRefIndex());
            egf->setFlags(ext->getFlags());
        }
    }
    externalGeoRefMap.clear();
    externalGeoMap.clear();
    std::set<std::string> detached;
    for(int i=0; i<ExternalGeo.getSize(); ++i) {
        auto geo = ExternalGeo[i];
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if (egf->testFlag(ExternalGeometryExtension::Detached)) {
            if (!egf->getRef().empty()) {
                detached.insert(egf->getRef());
                egf->setRef(std::string());
            }
            egf->setFlag(ExternalGeometryExtension::Detached,false);
            egf->setFlag(ExternalGeometryExtension::Missing,false);
        }
        if (egf->getId() > geoLastId) {
            geoLastId = egf->getId();
        }
        if (!externalGeoMap.emplace(egf->getId(), i).second) {
            FC_WARN("duplicate geometry id " << egf->getId() << " -> "
                    << geoLastId + 1);  // NOLINT
            egf->setId(++geoLastId);
            externalGeoMap[egf->getId()] = i;
        }
        if (!egf->getRef().empty()) {
            externalGeoRefMap[egf->getRef()].push_back(egf->getId());
        }
    }
    if (detached.empty()) {
        signalElementsChanged();
        return;
    }

    auto objs = ExternalGeometry.getValues();
    if (externalGeoRef.size() != objs.size()) {
        throw Base::RuntimeError("Inconsistency with external geometries");
    }
    auto itObj = objs.begin();
    auto subs = ExternalGeometry.getSubValues();
    auto itSub = subs.begin();
    for (const auto& i : externalGeoRef) {
        if (detached.count(i) == 0U) {
            ++itObj;
            ++itSub;
            continue;
        }

        itObj = objs.erase(itObj);
        itSub = subs.erase(itSub);
        auto& refs = externalGeoRefMap[i];
        for (long id : refs) {
            auto it = externalGeoMap.find(id);
            if (it!=externalGeoMap.end()) {
                auto geo = ExternalGeo[it->second];
                ExternalGeometryFacade::getFacade(geo)->setRef(std::string());
            }
        }
        refs.clear();
    }
    ExternalGeometry.setValues(objs, subs);
}

void SketchObject::onExternalGeometryChanged()
{
    auto doc = getDocument();

    if (doc && doc->isPerformingTransaction()) {
        setStatus(App::PendingTransactionUpdate, true);
    }

    if(!isRestoring()) {
        // must wait till onDocumentRestored() when shadow references are
        // fully restored
        updateGeometryRefs();
        signalElementsChanged();
    }
}

void SketchObject::onPlacementChanged()
{
    if (ExternalGeometry.getSize() > 0) {
        touch();
    }
}

void SketchObject::onExpressionEngineChanged()
{
    auto doc = getDocument();

    if (!isRestoring() && doc && !doc->isPerformingTransaction() && noRecomputes
        && !managedoperation) {
        // if we do not have a recompute, the sketch must be solved to
        // update the DoF of the solver, constraints and UI
        try {
            auto res = ExpressionEngine.execute();
            if (res) {
                FC_ERR("Failed to recompute " << ExpressionEngine.getFullName() << ": "
                       << res->Why);  // NOLINT
                delete res;
            }
        } catch (Base::Exception &e) {
            e.reportException();
            FC_ERR("Failed to recompute " << ExpressionEngine.getFullName() << ": "
                   << e.what());  // NOLINT
        }
        solve();
    }
}

void SketchObject::onAttachmentSupportChanged()
{
    // make sure not to change anything while restoring this object
    if (isRestoring()) {
        return;
    }

    // if support face has changed then clear the external geometry
    delConstraintsToExternal();
    for (int i=0; i < getExternalGeometryCount(); i++) {
        delExternal(0);
    }
    rebuildExternalGeometry();
}

void SketchObject::onUpdateElementReference(const App::Property *prop)
{
    if(prop == &ExternalGeometry) {
        updateGeoRef = true;
        // Must call updateGeometryRefs() now to avoid the case of recursive
        // property change (e.g. temporary object removal in SubShapeBinder)
        // afterwards causing assertion failure, although this may mean extra
        // call of updateGeometryRefs() later in onChange().
        updateGeometryRefs();
        signalElementsChanged();
    }
}

void SketchObject::onUndoRedoFinished()
{
    // upon undo/redo, PropertyConstraintList does not have updated valid geometry keys, which
    // results in empty constraint lists when using getValues
    //
    // The sketch will also have invalid vertex indices, requiring a call to rebuildVertexIndex
    //
    // Historically this was "solved" by issuing a recompute, which is absolutely unnecessary and
    // prevents solve() from working before such a recompute in case it is redoing an operation with
    // invalid data.
    Constraints.checkConstraintIndices(getHighestCurveIndex(), -getExternalGeometryCount());
    acceptGeometry();
    synchroniseGeometryState();
    solve();
}

void SketchObject::synchroniseGeometryState()
{
    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    for (size_t i = 0; i < vals.size(); i++) {
        auto gf = GeometryFacade::getFacade(vals[i]);

        auto facadeInternalAlignment = gf->getInternalType();
        auto facadeBlockedState = gf->getBlocked();

        Sketcher::InternalType::InternalType constraintInternalAlignment = InternalType::None;
        bool constraintBlockedState = false;

        for (auto cstr : Constraints.getValues()) {
            if (cstr->First == int(i)) {
                getInternalTypeState(cstr, constraintInternalAlignment);
                getBlockedState(cstr, constraintBlockedState);
            }
        }

        if (constraintInternalAlignment != facadeInternalAlignment)
            gf->setInternalType(constraintInternalAlignment);

        if (constraintBlockedState != facadeBlockedState)
            gf->setBlocked(constraintBlockedState);
    }
}

void SketchObject::onDocumentRestored()
{
    try {
        onSketchRestore();
        Part::Part2DObject::onDocumentRestored();
    }
    catch (...) {
    }
}

void SketchObject::restoreFinished()
{
    App::DocumentObject::restoreFinished();
    onSketchRestore();
}

void SketchObject::onSketchRestore()
{
    try {
        migrateSketch();

        updateGeometryRefs();

        fixMissingAxisInExternalGeo();

        if(ExternalGeo.getSize()<=2) {
            for(auto &key : externalGeoRef) {
                long id = getDocument()->getStringHasher()->getID(key.c_str()).value();
                if(geoLastId < id)
                    geoLastId = id;
                externalGeoRefMap[key].push_back(id);
            }
            rebuildExternalGeometry();
            if(ExternalGeometry.getSize()+2!=ExternalGeo.getSize())
                FC_WARN("Failed to restore some external geometry in " << getFullName());
        }else
            acceptGeometry();

        synchroniseGeometryState();
        // this may happen when saving a sketch directly in edit mode
        // but never performed a recompute before
        if (Shape.getValue().IsNull() && hasConflicts() == 0) {
            if (this->solve(true) == 0)
                Shape.setValue(solvedSketch.toShape());
        }

        // Sanity check on constraints with expression. It is added because the
        // way SketchObject syncs expression and constraints heavily relies on
        // proper setup of undo/redo transactions. The missing transaction in
        // EditDatumDialog may cause stray or worse wrongly bound expressions.
        for (auto &v : ExpressionEngine.getExpressions()) {
            if (v.first.getProperty() != &Constraints)
                continue;
            const Constraint * cstr = nullptr;
            try {
                cstr = Constraints.getConstraint(v.first);
            } catch (Base::Exception &) {
            }
            if (!cstr || !cstr->isDimensional()) {
                FC_WARN((cstr ? "Invalid" : "Orphan")
                        << " constraint expression in "
                        << getFullName() << "."
                        << v.first.toString()
                        << ": " << v.second->toString());
                ExpressionEngine.setValue(v.first, nullptr);
            }
        }
    } catch (Base::Exception &e) {
        e.reportException();
        FC_ERR("Error while restoring " << getFullName());
    } catch (...) {
    }
}

// clang-format on
void SketchObject::migrateSketch()
{

    const auto& allGeoms = getInternalGeometry();
    bool noextensions = std::ranges::any_of(allGeoms, [](const auto& geo) {
        return !geo->hasExtension(SketchGeometryExtension::getClassTypeId());
    });

    if (noextensions) {
        for (const auto& c : Constraints.getValues()) {
            addGeometryState(c);

            // Convert B-Spline controlpoints radius/diameter constraints to Weight constraints
            if (c->Type != InternalAlignment || c->AlignmentType != BSplineControlPoint) {
                continue;
            }

            int circleGeoId = c->First;
            int bSplineGeoId = c->Second;

            auto bsp = static_cast<const Part::GeomBSplineCurve*>(getGeometry(bSplineGeoId));

            std::vector<double> weights = bsp->getWeights();

            if (!(c->InternalAlignmentIndex < int(weights.size()))) {
                continue;
            }

            for (auto& ccp : Constraints.getValues()) {
                if ((ccp->Type == Radius || ccp->Type == Diameter) && ccp->First == circleGeoId) {
                    ccp->Type = Weight;
                    ccp->setValue(weights[c->InternalAlignmentIndex]);
                }
            }
        }

        // Construction migration to extension
        for (auto& g : Geometry.getValues()) {
            if (!g->hasExtension(Part::GeometryMigrationExtension::getClassTypeId())) {
                continue;
            }

            auto ext = std::static_pointer_cast<Part::GeometryMigrationExtension>(
                g->getExtension(Part::GeometryMigrationExtension::getClassTypeId()).lock()
            );

            if (!ext->testMigrationType(Part::GeometryMigrationExtension::Construction)) {
                continue;
            }
            // at this point IA geometry is already migrated
            auto gf = GeometryFacade::getFacade(g);

            bool oldConstr = ext->getConstruction()
                || (g->is<Part::GeomPoint>() && !gf->isInternalAligned());

            GeometryFacade::setConstruction(g, oldConstr);

            g->deleteExtension(Part::GeometryMigrationExtension::getClassTypeId());
        }
    }

    {
        // Migrate point-line, circle-circle and circle-line distance from abs to signed
        auto constraints = Constraints.getValues();
        for (auto& constr : constraints) {
            setOrientation(constr, false);
        }

        Constraints.setValues(std::move(constraints));
    }

    /* parabola axis as internal geometry */
    auto constraints = Constraints.getValues();
    auto geometries = getInternalGeometry();

    bool parabolaFound = std::ranges::any_of(geometries, &Part::Geometry::is<Part::GeomArcOfParabola>);

    if (!parabolaFound) {
        return;
    }

    auto focalAxisFound = std::ranges::any_of(constraints, [](auto c) {
        return c->Type == InternalAlignment && c->AlignmentType == ParabolaFocalAxis;
    });

    if (focalAxisFound) {
        return;
    }

    // There are parabolas and there isn't an IA axis. (1) there are no axis or (2) there is a
    // legacy construction line

    // maps parabola geoid to focusGeoId
    std::map<int, int> parabolaGeoId2FocusGeoId;

    // populate parabola and focus geoids
    for (const auto& c : constraints) {
        if (c->Type == InternalAlignment && c->AlignmentType == ParabolaFocus) {
            parabolaGeoId2FocusGeoId[c->Second] = {c->First};
        }
    }

    // maps axis geoid to parabolaGeoId
    std::map<int, int> axisGeoId2ParabolaGeoId;

    // populate axis geoid
    for (const auto& [parabolaGeoId, focusGeoId] : parabolaGeoId2FocusGeoId) {
        // look for a line from focusGeoId:start to Geoid:mid_external
        std::vector<int> focusGeoIdListGeoIdList;
        std::vector<PointPos> focusPosIdList;
        getDirectlyCoincidentPoints(
            focusGeoId,
            Sketcher::PointPos::start,
            focusGeoIdListGeoIdList,
            focusPosIdList
        );

        std::vector<int> parabGeoIdListGeoIdList;
        std::vector<PointPos> parabposidlist;
        getDirectlyCoincidentPoints(
            parabolaGeoId,
            Sketcher::PointPos::mid,
            parabGeoIdListGeoIdList,
            parabposidlist
        );

        for (const auto& parabGeoIdListGeoId : parabGeoIdListGeoIdList) {
            auto iterParabolaGeoId = std::ranges::find(focusGeoIdListGeoIdList, parabGeoIdListGeoId);
            if (iterParabolaGeoId != focusGeoIdListGeoIdList.end()) {
                axisGeoId2ParabolaGeoId[*iterParabolaGeoId] = parabolaGeoId;
            }
        }
    }

    std::vector<Constraint*> newConstraints;
    newConstraints.reserve(constraints.size());

    for (const auto& c : constraints) {
        if (c->Type != Coincident) {
            newConstraints.push_back(c);
            continue;
        }

        auto axisMajorCoincidentFound
            = std::ranges::any_of(axisGeoId2ParabolaGeoId, [&](const auto& pair) {
                  auto parabolaGeoId = pair.second;
                  auto axisgeoid = pair.first;
                  return (c->First == axisgeoid && c->Second == parabolaGeoId
                          && c->SecondPos == PointPos::mid)
                      || (c->Second == axisgeoid && c->First == parabolaGeoId
                          && c->FirstPos == PointPos::mid);
              });

        if (axisMajorCoincidentFound) {
            // we skip this coincident, the other coincident on axis will be substituted
            // by internal geometry constraint
            continue;
        }

        auto focusCoincidentFound = std::ranges::find_if(axisGeoId2ParabolaGeoId, [&](const auto& pair) {
            auto parabolaGeoId = pair.second;
            auto axisgeoid = pair.first;
            auto focusGeoId = parabolaGeoId2FocusGeoId[parabolaGeoId];
            return (c->First == axisgeoid && c->Second == focusGeoId
                    && c->SecondPos == PointPos::start)
                || (c->Second == axisgeoid && c->First == focusGeoId
                    && c->FirstPos == PointPos::start);
        });

        if (focusCoincidentFound != axisGeoId2ParabolaGeoId.end()) {
            auto* newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = Sketcher::ParabolaFocalAxis;
            newConstr->First = focusCoincidentFound->first;  // axis geoid
            newConstr->FirstPos = Sketcher::PointPos::none;
            newConstr->Second = focusCoincidentFound->second;  // parabola geoid
            newConstr->SecondPos = Sketcher::PointPos::none;
            newConstraints.push_back(newConstr);

            addGeometryState(newConstr);

            // we skip the coincident, as we have substituted it by internal geometry
            // constraint
            continue;
        }

        newConstraints.push_back(c);
    }

    Constraints.setValues(std::move(newConstraints));

    Base::Console().critical(
        this->getFullName(),
        QT_TRANSLATE_NOOP(
            "Notifications",
            "Parabolas were migrated. Migrated files won't open in previous "
            "versions of FreeCAD!!\n"
        )
    );
}
// clang-format off

App::DocumentObject *SketchObject::getSubObject(
        const char *subname, PyObject **pyObj,
        Base::Matrix4D *pmat, bool transform, int depth) const
{
    while(subname && *subname=='.') ++subname; // skip leading .
    std::string sub;
    const char *mapped = Data::isMappedElement(subname);
    if(!subname || !subname[0]) {
        return Part2DObject::getSubObject(subname,pyObj,pmat,transform,depth);
    }
    const char *element = Data::findElementName(subname);
    if(element != subname) {
        const char *dot = strchr(subname,'.');
        if(!dot)
            return 0;
        std::string name(subname,dot-subname);
        auto child = Exports.find(name.c_str());
        if(!child)
            return 0;
        return child->getSubObject(dot+1,pyObj,pmat,true,depth+1);
    }

    Data::IndexedName indexedName = checkSubName(subname);
    int index = indexedName.getIndex();
    const char * shapetype = indexedName.getType();
    const Part::Geometry *geo = 0;
    Part::TopoShape subshape;
    Base::Vector3d point;

    if (auto realType = convertInternalName(indexedName.getType())) {
        if (realType[0] == '\0')
            subshape = InternalShape.getShape();
        else {
            auto shapeType = Part::TopoShape::shapeType(realType, true);
            if (shapeType != TopAbs_SHAPE)
                subshape = InternalShape.getShape().getSubTopoShape(shapeType, indexedName.getIndex(), true);
        }
        if (subshape.isNull())
            return nullptr;
    }
    else if (!pyObj || !mapped) {
        if (!pyObj
            || (index > 0
                && !boost::algorithm::contains(subname, "edge")
                && !boost::algorithm::contains(subname, "vertex")))
            return Part2DObject::getSubObject(subname,pyObj,pmat,transform,depth);
    }
    else {
        subshape = Shape.getShape().getSubTopoShape(subname, true);
        if (!subshape.isNull())
            return Part2DObject::getSubObject(subname,pyObj,pmat,transform,depth);
    }

    if (subshape.isNull()) {
        if (boost::equals(shapetype,"Edge") ||
            boost::equals(shapetype,"edge")) {
            geo = getGeometry(index - 1);
            if (!geo)
                return nullptr;
        }
        else if (boost::equals(shapetype,"ExternalEdge")) {
            int GeoId = index - 1;
            GeoId = -GeoId - 3;
            geo = getGeometry(GeoId);
            if(!geo)
                return nullptr;
        }
        else if (boost::equals(shapetype,"Vertex") ||
                 boost::equals(shapetype,"vertex")) {
            int VtId = index- 1;
            int GeoId;
            PointPos PosId;
            getGeoVertexIndex(VtId,GeoId,PosId);
            if (PosId==PointPos::none)
                return nullptr;
            point = getPoint(GeoId,PosId);
        }
        else if (boost::equals(shapetype,"RootPoint"))
            point = getPoint(Sketcher::GeoEnum::RtPnt,PointPos::start);
        else if (boost::equals(shapetype,"H_Axis"))
            geo = getGeometry(Sketcher::GeoEnum::HAxis);
        else if (boost::equals(shapetype,"V_Axis"))
            geo = getGeometry(Sketcher::GeoEnum::VAxis);
        else if (boost::equals(shapetype,"Constraint")) {
            int ConstrId = PropertyConstraintList::getIndexFromConstraintName(shapetype);
            const std::vector< Constraint * > &vals = this->Constraints.getValues();
            if (ConstrId < 0 || ConstrId >= int(vals.size()))
                return nullptr;
            if(pyObj)
                *pyObj = vals[ConstrId]->getPyObject();
            return const_cast<SketchObject*>(this);
        }
        else {
            return nullptr;
        }
    }

    if (pmat && transform)
        *pmat *= Placement.getValue().toMatrix();

    if (!pyObj) {
        return const_cast<SketchObject*>(this);
    }

    // pyObj exists from here
    Part::TopoShape shape;
    std::string name = convertSubName(indexedName,false);
    if (geo) {
        shape = getEdge(geo,name.c_str());
        if(pmat && !shape.isNull()) {
            shape.transformShape(*pmat,false,true);
        }
    }
    else if (!subshape.isNull()) {
        shape = subshape;
        if (pmat) {
            shape.transformShape(*pmat,false,true);
        }
    }
    else {
        if(pmat) {
            point = (*pmat)*point;
        }
        shape = BRepBuilderAPI_MakeVertex(gp_Pnt(point.x,point.y,point.z)).Vertex();
        // Originally in ComplexGeoData::setElementName
        // LinkStable/src/App/ComplexGeoData.cpp#L1631
        // No longer possible after map separated in ElementMap.cpp
        if (!shape.hasElementMap()) {
            shape.resetElementMap(std::make_shared<Data::ElementMap>());
        }
        shape.setElementName(Data::IndexedName::fromConst("Vertex", 1),
                             Data::MappedName::fromRawData(name.c_str()),0);
    }
    shape.Tag = getID();
    *pyObj = Py::new_reference_to(Part::shape2pyshape(shape));

    return const_cast<SketchObject*>(this);
}

std::vector<Data::IndexedName>
SketchObject::getHigherElements(const char *element, bool silent) const
{
    std::vector<Data::IndexedName> res;
    // App::ObjEditing is not in main yet. Only in LinkStage.
    // It is not a problem yet because getHigherElements is still unused.
    // see https://github.com/FreeCAD/FreeCAD/issues/20753
    if (false /*testStatus(App::ObjEditing)*/) {
        if (boost::istarts_with(element, "vertex")) {
            int n = 0;
            int index = atoi(element+6);
            for (auto cstr : Constraints.getValues()) {
                ++n;
                if (cstr->Type != Sketcher::Coincident) {
                    continue;
                }
                for (int i=0; i<2; ++i) {
                    int geoid = i ? cstr->Second : cstr->First;
                    const Sketcher::PointPos &pos = i ? cstr->SecondPos : cstr->FirstPos;
                    if(geoid >= 0 && index == getSolvedSketch().getPointId(geoid, pos) + 1)
                        res.push_back(Data::IndexedName::fromConst("Constraint", n));
                };
            }
        }
        return res;
    }

    auto getNames = [this, &silent, &res](const char *element) {
        bool internal = boost::starts_with(element, internalPrefix());
        const auto &shape = internal ? InternalShape.getShape() : Shape.getShape();
        for (const auto &indexedName : shape.getHigherElements(element+(internal?internalPrefix().size() : 0), silent)) {
            if (!internal) {
                res.push_back(indexedName);
            }
            else if (boost::equals(indexedName.getType(), "Face")
                    || boost::equals(indexedName.getType(), "Edge")
                    || boost::equals(indexedName.getType(), "Wire")) {
                res.emplace_back((internalPrefix() + indexedName.getType()).c_str(), indexedName.getIndex());
            }
        }
    };
    getNames(element);
    const auto &elementMap = getInternalElementMap();
    auto it = elementMap.find(element);
    if (it != elementMap.end()) {
        res.emplace_back(it->second.c_str());
        getNames(it->second.c_str());
    }
    return res;
}

std::vector<const char *> SketchObject::getElementTypes(bool all) const
{
    if (!all)
        return Part::Part2DObject::getElementTypes();
    static std::vector<const char *> res { Part::TopoShape::shapeName(TopAbs_VERTEX).c_str(),
                Part::TopoShape::shapeName(TopAbs_EDGE).c_str(),
                "ExternalEdge",
                "Constraint",
                "InternalEdge",
                "InternalFace",
                "InternalVertex",
              };
    return res;
}

void SketchObject::setExpression(const App::ObjectIdentifier& path,
                                 std::shared_ptr<App::Expression> expr)
{
    DocumentObject::setExpression(path, std::move(expr));

    if (noRecomputes) {
        // if we do not have a recompute, the sketch must be solved to update the DoF of the solver,
        // constraints and UI
        try {
            auto res = ExpressionEngine.execute();
            if (res) {
                FC_ERR("Failed to recompute " << ExpressionEngine.getFullName() << ": "
                                              << res->Why);
                delete res;
            }
        }
        catch (Base::Exception& e) {
            e.reportException();
            FC_ERR("Failed to recompute " << ExpressionEngine.getFullName() << ": " << e.what());
        }
        solve();
    }
}

const std::string &SketchObject::internalPrefix()
{
    static std::string _prefix("Internal");
    return _prefix;
}

const char *SketchObject::convertInternalName(const char *name)
{
    if (name && boost::starts_with(name, internalPrefix()))
        return name + internalPrefix().size();
    return nullptr;
}

App::ElementNamePair SketchObject::getElementName(
        const char *name, ElementNameType type) const
{
    App::ElementNamePair ret;
    if(!name) return ret;

    if(hasSketchMarker(name))
        return Part2DObject::getElementName(name,type);

    const char *mapped = Data::isMappedElement(name);
    Data::IndexedName index = checkSubName(name);
    index.appendToStringBuffer(ret.oldName);
    if (auto realName = convertInternalName(ret.oldName.c_str())) {
        Data::MappedElement mappedElement;
        if (mapped)
            mappedElement = InternalShape.getShape().getElementName(name);
        else if (type == ElementNameType::Export)
            ret.newName = getExportElementName(InternalShape.getShape(), realName).newName;
        else
            mappedElement = InternalShape.getShape().getElementName(realName);

        if (mapped || type != ElementNameType::Export) {
            if (mappedElement.index) {
                ret.oldName = internalPrefix();
                mappedElement.index.appendToStringBuffer(ret.oldName);
            }
            if (mappedElement.name) {
                ret.newName = Data::ComplexGeoData::elementMapPrefix();
                mappedElement.name.appendToBuffer(ret.newName);
            }
            else if (mapped)
                ret.newName = name;
        }

        if (ret.newName.size()) {
            if (auto dot = strrchr(ret.newName.c_str(), '.'))
                ret.newName.resize(dot+1-ret.newName.c_str());
            else
                ret.newName += ".";
            ret.newName += ret.oldName;
        }
        if (mapped && (!mappedElement.index || !mappedElement.name))
            ret.oldName.insert(0, Data::MISSING_PREFIX);
        return ret;
    }

    if(!mapped) {
        auto occindex = Part::TopoShape::shapeTypeAndIndex(name);
        if (occindex.second)
            return Part2DObject::getElementName(name,type);
    }
    if(index && type==ElementNameType::Export) {
        if(boost::starts_with(ret.oldName,"Vertex"))
            ret.oldName[0] = 'v';
        else if(boost::starts_with(ret.oldName,"Edge"))
            ret.oldName[0] = 'e';
    }
    ret.newName = convertSubName(index, true);
    if(!Data::isMappedElement(ret.newName.c_str()))
        ret.newName.clear();
    return ret;
}

Data::IndexedName SketchObject::checkSubName(const char *subname) const
{
    static std::vector<const char *> types = {
        "Edge",
        "Vertex",
        "edge",
        "vertex",
        "ExternalEdge",
        "RootPoint",
        "H_Axis",
        "V_Axis",
        "Constraint",

        // other feature from LS3 not related to TNP
        "InternalEdge",
        "InternalFace",
        "InternalVertex",
    };

    if(!subname) return Data::IndexedName();
    const char *mappedSubname = Data::isMappedElement(subname);

    // if not a mapped name parse the indexed name directly, uppercasing "edge" and "vertex"
    if(!mappedSubname)  {
        Data::IndexedName result(subname, types, true);
        if (boost::equals(result.getType(), "edge"))
            return Data::IndexedName("Edge", result.getIndex());
        if (boost::equals(result.getType(), "vertex"))
            return Data::IndexedName("Vertex", result.getIndex());
        return result;
    }

    bio::stream<bio::array_source> iss(mappedSubname+1, std::strlen(mappedSubname+1));
    int id = -1;
    bool valid = false;
    switch (mappedSubname[0]) {
        case '\0':  // check length != 0
            break;

        case 'g':  // = geometry
        case 'e':  // = external geometry
            if (iss >> id) {
                valid = true;
            }
            break;

        // for RootPoint, H_Axis, V_Axis
        default: {
            const char* dot = strchr(mappedSubname, '.');
            if (dot) {
                mappedSubname = dot + 1;
            }
            return Data::IndexedName(mappedSubname, types, false);
        }
    }

    if (!valid) {
        FC_ERR("invalid subname " << subname);
        return Data::IndexedName();
    }

    int geoId;
    const Part::Geometry* geo = 0;
    switch (mappedSubname[0]) {
        case 'g': {
            auto it = geoMap.find(id);
            if (it != geoMap.end()) {
                geoId = it->second;
                geo = getGeometry(geoId);
            }
            break;
        }
        case 'e': {
            auto it = externalGeoMap.find(id);
            if (it != externalGeoMap.end()) {
                geoId = -it->second - 1;
                geo = getGeometry(geoId);
            }
            break;
        }
    }
    if (geo && GeometryFacade::getId(geo) == id) {
        char sep;
        int posId = static_cast<int>(PointPos::none);
        if ((iss >> sep >> posId) && sep == 'v') {
            int idx = getVertexIndexGeoPos(geoId, static_cast<PointPos>(posId));

            // Outside edit-mode circles exposes the seam point but not the center, while in edit-mode we expose the center but not the seam.
            // getVertexIndexGeoPos searching for a circle start point (g1v1 for example) (which happens outside of edit mode) will fail.
            // see https://github.com/FreeCAD/FreeCAD/issues/25089
            // The following fix works because circles have always 1 vertex, whether in or out of edit mode.
            if (idx < 0 && (static_cast<PointPos>(posId) == PointPos::start || static_cast<PointPos>(posId) == PointPos::end)) {
                if (geo->is<Part::GeomCircle>() || geo->is<Part::GeomEllipse>()) {
                    idx = getVertexIndexGeoPos(geoId, PointPos::mid);
                }
            }

            if (idx < 0) {
                FC_ERR("invalid subname " << subname);
                return Data::IndexedName();
            }
            return Data::IndexedName::fromConst("Vertex", idx + 1);
        }
        else if (geoId >= 0) {
            return Data::IndexedName::fromConst("Edge", geoId + 1);
        }
        else {
            return Data::IndexedName::fromConst("ExternalEdge", -geoId - 2);
        }
    }
    FC_ERR("cannot find subname " << subname);

    return Data::IndexedName();
}

std::string SketchObject::convertSubName(const char *subname, bool postfix) const {
    return convertSubName(checkSubName(subname), postfix);
}

std::string SketchObject::convertSubName(const Data::IndexedName &indexedName, bool postfix) const {
    std::ostringstream ss;
    if (auto realType = convertInternalName(indexedName.getType())) {
        auto mapped = InternalShape.getShape().getMappedName(
                Data::IndexedName::fromConst(realType, indexedName.getIndex()));
        if (!mapped) {
            if (postfix)
                ss << indexedName;
        } else if (postfix)
            ss << Data::ComplexGeoData::elementMapPrefix() << mapped << '.' << indexedName;
        else
            ss << mapped;
        return ss.str();
    }
    int geoId;
    PointPos posId;
    if (!geoIdFromShapeType(indexedName, geoId, posId)) {
        ss << indexedName;
        return ss.str();
    }
    if (geoId == Sketcher::GeoEnum::HAxis ||
        geoId == Sketcher::GeoEnum::VAxis ||
        geoId == Sketcher::GeoEnum::RtPnt) {
        if (postfix)
            ss << Data::ELEMENT_MAP_PREFIX;
        ss << indexedName;
        if (postfix)
            ss << '.' << indexedName;
        return ss.str();
    }

    auto geo = getGeometry(geoId);
    if (!geo) {
        std::string res = indexedName.toString();
        return res;
    }
    if (postfix)
        ss << Data::ELEMENT_MAP_PREFIX;
    ss << (geoId >= 0 ? 'g' : 'e') << GeometryFacade::getFacade(geo)->getId();
    if (posId != PointPos::none)
        ss << 'v' << static_cast<int>(posId);
    if (postfix) {
        // rename Edge to edge, and Vertex to vertex to avoid ambiguous of
        // element mapping of the public shape and internal geometry.
        if (indexedName.getIndex() <= 0)
            ss << '.' << indexedName;
        else if (boost::starts_with(indexedName.getType(), "Edge"))
            ss << ".e" << (indexedName.getType() + 1) << indexedName.getIndex();
        else if (boost::starts_with(indexedName.getType(), "Vertex"))
            ss << ".v" << (indexedName.getType() + 1) << indexedName.getIndex();
        else
            ss << '.' << indexedName;
    }
    return ss.str();
}

int SketchObject::autoConstraint(double precision, double angleprecision, bool includeconstruction)
{
    return analyser->autoconstraint(precision, angleprecision, includeconstruction);
}

int SketchObject::detectMissingPointOnPointConstraints(double precision, bool includeconstruction)
{
    return analyser->detectMissingPointOnPointConstraints(precision, includeconstruction);
}

void SketchObject::analyseMissingPointOnPointCoincident(double angleprecision)
{
    analyser->analyseMissingPointOnPointCoincident(angleprecision);
}

int SketchObject::detectMissingVerticalHorizontalConstraints(double angleprecision)
{
    return analyser->detectMissingVerticalHorizontalConstraints(angleprecision);
}

int SketchObject::detectMissingEqualityConstraints(double precision)
{
    return analyser->detectMissingEqualityConstraints(precision);
}

std::vector<ConstraintIds>& SketchObject::getMissingPointOnPointConstraints()
{
    return analyser->getMissingPointOnPointConstraints();
}

std::vector<ConstraintIds>& SketchObject::getMissingVerticalHorizontalConstraints()
{
    return analyser->getMissingVerticalHorizontalConstraints();
}

std::vector<ConstraintIds>& SketchObject::getMissingLineEqualityConstraints()
{
    return analyser->getMissingLineEqualityConstraints();
}

std::vector<ConstraintIds>& SketchObject::getMissingRadiusConstraints()
{
    return analyser->getMissingRadiusConstraints();
}

void SketchObject::setMissingRadiusConstraints(std::vector<ConstraintIds>& cl)
{
    if (analyser)
        analyser->setMissingRadiusConstraints(cl);
}

void SketchObject::setMissingLineEqualityConstraints(std::vector<ConstraintIds>& cl)
{
    if (analyser)
        analyser->setMissingLineEqualityConstraints(cl);
}

void SketchObject::setMissingVerticalHorizontalConstraints(std::vector<ConstraintIds>& cl)
{
    if (analyser)
        analyser->setMissingVerticalHorizontalConstraints(cl);
}

void SketchObject::setMissingPointOnPointConstraints(std::vector<ConstraintIds>& cl)
{
    if (analyser)
        analyser->setMissingPointOnPointConstraints(cl);
}

void SketchObject::makeMissingPointOnPointCoincident(bool onebyone)
{
    if (analyser) {
        onebyone ? analyser->makeMissingPointOnPointCoincidentOneByOne()
                 : analyser->makeMissingPointOnPointCoincident();
    }
}

void SketchObject::makeMissingVerticalHorizontal(bool onebyone)
{
    if (analyser) {
        onebyone ? analyser->makeMissingVerticalHorizontalOneByOne()
                 : analyser->makeMissingVerticalHorizontal();
    }
}

void SketchObject::makeMissingEquality(bool onebyone)
{
    if (analyser) {
        onebyone ? analyser->makeMissingEqualityOneByOne()
                 : analyser->makeMissingEquality();
    }
}

int SketchObject::detectDegeneratedGeometries(double tolerance)
{
    return analyser->detectDegeneratedGeometries(tolerance);
}

int SketchObject::removeDegeneratedGeometries(double tolerance)
{
    return analyser->removeDegeneratedGeometries(tolerance);
}

std::vector<Base::Vector3d> SketchObject::getOpenVertices() const
{
    std::vector<Base::Vector3d> points;

    if (analyser)
        points = analyser->getOpenVertices();

    return points;
}

// Python Sketcher feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Sketcher::SketchObjectPython, Sketcher::SketchObject)
template<>
const char* Sketcher::SketchObjectPython::getViewProviderName() const
{
    return "SketcherGui::ViewProviderPython";
}
template<>
PyObject* Sketcher::SketchObjectPython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<SketchObjectPy>(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class SketcherExport FeaturePythonT<Sketcher::SketchObject>;
}// namespace App

// clang-format on
