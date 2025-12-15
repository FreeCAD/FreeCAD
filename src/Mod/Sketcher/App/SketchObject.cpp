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
#include <cmath>
#include <limits>
#include <vector>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Section.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepOffsetAPI_NormalProjection.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Tool.hxx>
#include <ElCLib.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeCircle.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomConvert_BSplineCurveKnotSplitting.hxx>
#include <GeomLProp_CLProps.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Line.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_Plane.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Standard_Version.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax3.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Hypr.hxx>
#include <gp_Parab.hxx>
#include <gp_Pln.hxx>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/geometry.hpp>

#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/ElementNamingUtils.h>
#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <App/FeaturePythonPyImp.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <App/ObjectIdentifier.h>
#include <App/Datums.h>
#include <App/Part.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Base/Writer.h>
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Part/App/PartPyCXX.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/GeometryMigrationExtension.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/App/WireJoiner.h>

#include <memory>

#include "GeoEnum.h"
#include "SketchObject.h"
#include "SketchObjectPy.h"
#include "SolverGeometryExtension.h"
#include "ExternalGeometryFacade.h"
#include <Mod/Part/App/Datums.h>


#undef DEBUG
// #define DEBUG

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
    MakeInternals.setValue(hGrpp->GetBool("MakeInternals", false));
    inherited::setupObject();
}

void SketchObject::initExternalGeo() {
    std::vector<Part::Geometry *> geos;
    auto HLine = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade();
    auto VLine = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade();
    HLine->getTypedGeometry()->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
    VLine->getTypedGeometry()->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(0,1,0));
    HLine->setConstruction(true);
    HLine->setId(-1);
    VLine->setConstruction(true);
    VLine->setId(-2);
    geos.push_back(HLine->getGeometry());
    geos.push_back(VLine->getGeometry());
    HLine->setOwner(false); // we have transferred the ownership to ExternalGeo
    VLine->setOwner(false); // we have transferred the ownership to ExternalGeo
    ExternalGeo.setValues(std::move(geos));
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
    for (auto geo : geometries) {
        ++geoId;
        if (GeometryFacade::getConstruction(geo)) {
            continue;
        }
        if (geo->isDerivedFrom<Part::GeomPoint>()) {
            int idx = getVertexIndexGeoPos(geoId - 1, Sketcher::PointPos::start);
            addVertex(
                Part::TopoShape {TopoDS::Vertex(geo->toShape())},
                convertSubName(Data::IndexedName::fromConst("Vertex", idx + 1), false)
            );
        }
        else {
            auto indexedName = Data::IndexedName::fromConst("Edge", geoId);
            addEdge(geo, indexedName);
        }
    }

    for (auto geo : geometries) {
        delete geo;
    }

    for (int i = 2; i < ExternalGeo.getSize(); ++i) {
        auto geo = ExternalGeo[i];
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if (!egf->testFlag(ExternalGeometryExtension::Defining)) {
            continue;
        }

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
        InternalShape.setValue(Part::TopoShape());
        Shape.setValue(Part::TopoShape());
        return;
    }
    Part::TopoShape result(0, getDocument()->getStringHasher());
    if (vertices.empty()) {
        // Notice here we supply op code Part::OpCodes::Sketch to makEWires().
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
    InternalShape.setValue(buildInternals(result.located(TopLoc_Location())));
    // Must set Shape property after InternalShape so that
    // GeoFeature::updateElementReference() can run properly on change of Shape
    // property, because some reference may pointing to the InternalShape
    Shape.setValue(result);
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
        Part::WireJoiner joiner;
        joiner.setTightBound(true);
        joiner.setMergeEdges(true);
        joiner.addShape(edges);
        Part::TopoShape result(getID(), getDocument()->getStringHasher());
        if (!joiner.Shape().IsNull()) {
            joiner.getResultWires(result, "SKF");
            result = result.makeElementFace(result.getSubTopoShapes(TopAbs_WIRE),
                    /*op*/"",
                    /*maker*/"Part::FaceMakerRing",
                    /*pln*/nullptr
            );
        }
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

static const char *hasSketchMarker(const char *name) {
    static std::string marker(Part::TopoShape::elementMapPrefix()+Part::OpCodes::Sketch);
    if (!name)
        return nullptr;
    return strstr(name,marker.c_str());
}

int SketchObject::hasConflicts() const
{
    if (lastDoF < 0)// over-constrained sketch
        return -2;
    if (solvedSketch.hasConflicts())// conflicting constraints
        return -1;

    return 0;
}

void SketchObject::retrieveSolverDiagnostics()
{
    lastHasConflict = solvedSketch.hasConflicts();
    lastHasRedundancies = solvedSketch.hasRedundancies();
    lastHasPartialRedundancies = solvedSketch.hasPartialRedundancies();
    lastHasMalformedConstraints = solvedSketch.hasMalformedConstraints();
    lastConflicting = solvedSketch.getConflicting();
    lastRedundant = solvedSketch.getRedundant();
    lastPartiallyRedundant = solvedSketch.getPartiallyRedundant();
    lastMalformedConstraints = solvedSketch.getMalformedConstraints();
}

int SketchObject::solve(bool updateGeoAfterSolving /*=true*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // Reset the initial movement in case of a dragging operation was ongoing on the solver.
    solvedSketch.resetInitMove();

    // if updateGeoAfterSolving=false, the solver information is updated, but the Sketch is nothing
    // updated. It is useful to avoid triggering an OnChange when the goeometry did not change but
    // the solver needs to be updated.

    // We should have an updated Sketcher (sketchobject) geometry or this solve() should not have
    // happened therefore we update our sketch solver geometry with the SketchObject one.
    //
    // set up a sketch (including dofs counting and diagnosing of conflicts)
    lastDoF = solvedSketch.setUpSketch(
        getCompleteGeometry(), Constraints.getValues(), getExternalGeometryCount());

    // At this point we have the solver information about conflicting/redundant/over-constrained,
    // but the sketch is NOT solved. Some examples: Redundant: a vertical line, a horizontal line
    // and an angle constraint of 90 degrees between the two lines Conflicting: a 80 degrees angle
    // between a vertical line and another line, then adding a horizontal constraint to that other
    // line OverConstrained: a conflicting constraint when all other DoF are already constrained (it
    // has more constraints than parameters and the extra constraints are not redundant)

    solverNeedsUpdate = false;

    retrieveSolverDiagnostics();

    lastSolveTime = 0.0;

    // Failure is default for notifying the user unless otherwise proven
    lastSolverStatus = GCS::Failed;

    int err = 0;

    // redundancy is a lower priority problem than conflict/over-constraint/solver error
    // we set it here because we are indeed going to solve, as we can. However, we still want to
    // provide the right error code.
    if (lastHasRedundancies) {// redundant constraints
        err = -2;
    }

    if (lastDoF < 0) {// over-constrained sketch
        err = -4;
    }
    else if (lastHasConflict) {// conflicting constraints
        // The situation is exactly the same as in the over-constrained situation.
        err = -3;
    }
    else if (lastHasMalformedConstraints) {
        err = -5;
    }
    else {
        lastSolverStatus = solvedSketch.solve();
        if (lastSolverStatus != 0) {// solving
            err = -1;
        }
    }

    if (lastHasMalformedConstraints) {
        Base::Console().error(
            this->getFullLabel(),
            QT_TRANSLATE_NOOP("Notifications", "The Sketch has malformed constraints!") "\n");
    }

    if (lastHasPartialRedundancies) {
        Base::Console().warning(
            this->getFullLabel(),
            QT_TRANSLATE_NOOP("Notifications",
                              "The Sketch has partially redundant constraints!") "\n");
    }

    lastSolveTime = solvedSketch.getSolveTime();

    // In uncommon situations, the analysis of QR decomposition leads to full rank, but the result
    // does not converge. We avoid marking a sketch as fully constrained when no convergence is
    // achieved.
    if (err == 0) {
        FullyConstrained.setValue(lastDoF == 0);
        if (updateGeoAfterSolving) {
            // set the newly solved geometry
            std::vector<Part::Geometry*> geomlist = solvedSketch.extractGeometry();
            Part::PropertyGeometryList tmp;
            tmp.setValues(std::move(geomlist));
            // Only set values if there is actual changes
            if (Constraints.isTouched() || !Geometry.isSame(tmp)) {
                Geometry.moveValues(std::move(tmp));
            }
        }
    }

    signalSolverUpdate();

    return err;
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

    FC_TIME_INIT(t);
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
    FC_TIME_LOG(t,"update geometry history (" << geoHistory->size() << ", " << geoMap.size()<<')');
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

int SketchObject::setDatum(int ConstrId, double Datum)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // set the changed value for the constraint
    if (this->Constraints.hasInvalidGeometry())
        return -6;
    const std::vector<Constraint*>& vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;
    ConstraintType type = vals[ConstrId]->Type;
    // for tangent, value==0 is autodecide, value==Pi/2 is external and value==-Pi/2 is internal
    if (!vals[ConstrId]->isDimensional() && type != Tangent && type != Perpendicular)
        return -1;

    if ((type == Radius || type == Diameter || type == Weight) && Datum <= 0)
        return (Datum == 0) ? -5 : -4;

    if (type == Distance && Datum == 0)
            return -5;

    // copy the list
    std::vector<Constraint*> newVals(vals);
    double oldDatum = newVals[ConstrId]->getValue();
    newVals[ConstrId] = newVals[ConstrId]->clone();
    newVals[ConstrId]->setValue(Datum);

    this->Constraints.setValues(std::move(newVals));

    int err = solve();

    if (err)
        this->Constraints.getValues()[ConstrId]->setValue(oldDatum);// newVals is a shell now

    return err;
}
double SketchObject::getDatum(int ConstrId) const
{
    if (!this->Constraints[ConstrId]->isDimensional()) {
        return 0.0;
    }
    return this->Constraints[ConstrId]->getValue();
}

int SketchObject::setDriving(int ConstrId, bool isdriving)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    int ret = testDrivingChange(ConstrId, isdriving);

    if (ret < 0)
        return ret;

    // copy the list
    std::vector<Constraint*> newVals(vals);
    newVals[ConstrId] = newVals[ConstrId]->clone();
    newVals[ConstrId]->isDriving = isdriving;

    this->Constraints.setValues(std::move(newVals));

    if (!isdriving)
        setExpression(Constraints.createPath(ConstrId), std::shared_ptr<App::Expression>());

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::getDriving(int ConstrId, bool& isdriving)
{
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    if (!vals[ConstrId]->isDimensional())
        return -1;

    isdriving = vals[ConstrId]->isDriving;
    return 0;
}

int SketchObject::testDrivingChange(int ConstrId, bool isdriving)
{
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    if (!vals[ConstrId]->isDimensional())
        return -2;

    if (!(vals[ConstrId]->First >= 0 || vals[ConstrId]->Second >= 0 || vals[ConstrId]->Third >= 0)
        && isdriving) {
        // a constraint that does not have at least one element as not-external-geometry can never
        // be driving.
        return -3;
    }

    return 0;
}

int SketchObject::setActive(int ConstrId, bool isactive)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    // copy the list
    std::vector<Constraint*> newVals(vals);
    // clone the changed Constraint
    Constraint* constNew = vals[ConstrId]->clone();
    constNew->isActive = isactive;
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(std::move(newVals));

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::getActive(int ConstrId, bool& isactive)
{
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    isactive = vals[ConstrId]->isActive;

    return 0;
}

int SketchObject::toggleActive(int ConstrId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    // copy the list
    std::vector<Constraint*> newVals(vals);
    // clone the changed Constraint
    Constraint* constNew = vals[ConstrId]->clone();
    constNew->isActive = !constNew->isActive;
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(std::move(newVals));

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::setLabelPosition(int ConstrId, float value)
{
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size())) {
        return -1;
    }

    // copy the list
    std::vector<Constraint*> newVals(vals);
    // clone the changed Constraint
    Constraint* constNew = vals[ConstrId]->clone();
    constNew->LabelPosition = value;
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(std::move(newVals));

    return 0;
}

int SketchObject::getLabelPosition(int ConstrId, float& value)
{
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size())) {
        return -1;
    }

    value = vals[ConstrId]->LabelPosition;

    return 0;
}

int SketchObject::setLabelDistance(int ConstrId, float value)
{
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size())) {
        return -1;
    }

    // copy the list
    std::vector<Constraint*> newVals(vals);
    // clone the changed Constraint
    Constraint* constNew = vals[ConstrId]->clone();
    constNew->LabelDistance = value;
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(std::move(newVals));

    return 0;
}

int SketchObject::getLabelDistance(int ConstrId, float& value)
{
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size())) {
        return -1;
    }

    value = vals[ConstrId]->LabelDistance;

    return 0;
}


/// Make all dimensionals Driving/non-Driving
int SketchObject::setDatumsDriving(bool isdriving)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();
    std::vector<Constraint*> newVals(vals);

    for (size_t i = 0; i < newVals.size(); i++) {
        if (!testDrivingChange(i, isdriving)) {
            newVals[i] = newVals[i]->clone();
            newVals[i]->isDriving = isdriving;
        }
    }

    this->Constraints.setValues(std::move(newVals));

    // newVals is a shell now
    const std::vector<Constraint*>& uvals = this->Constraints.getValues();

    for (size_t i = 0; i < uvals.size(); i++) {
        if (!isdriving && uvals[i]->isDimensional())
            setExpression(Constraints.createPath(i), std::shared_ptr<App::Expression>());
    }

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::moveDatumsToEnd()
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    std::vector<Constraint*> copy(vals);
    std::vector<Constraint*> newVals(vals.size());

    int addindex = copy.size() - 1;

    // add the dimensionals at the end
    for (int i = copy.size() - 1; i >= 0; i--) {
        if (copy[i]->isDimensional()) {
            newVals[addindex] = copy[i];
            addindex--;
        }
    }

    // add the non-dimensionals
    for (int i = copy.size() - 1; i >= 0; i--) {
        if (!copy[i]->isDimensional()) {
            newVals[addindex] = copy[i];
            addindex--;
        }
    }

    this->Constraints.setValues(std::move(newVals));

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

void SketchObject::reverseAngleConstraintToSupplementary(Constraint* constr, int constNum)
{
    std::swap(constr->First, constr->Second);
    std::swap(constr->FirstPos, constr->SecondPos);
    constr->FirstPos = (constr->FirstPos == Sketcher::PointPos::start) ? Sketcher::PointPos::end : Sketcher::PointPos::start;

    // Edit the expression if any, else modify constraint value directly
    if (constraintHasExpression(constNum)) {
        std::string expression = getConstraintExpression(constNum);
        setConstraintExpression(constNum, std::move(reverseAngleConstraintExpression(expression)));
    }
    else {
        double actAngle = constr->getValue();
        constr->setValue(std::numbers::pi - actAngle);
    }
}

void SketchObject::inverseAngleConstraint(Constraint* constr)
{
    constr->FirstPos = (constr->FirstPos == Sketcher::PointPos::start) ? Sketcher::PointPos::end : Sketcher::PointPos::start;
    constr->SecondPos = (constr->SecondPos == Sketcher::PointPos::start) ? Sketcher::PointPos::end : Sketcher::PointPos::start;
}

bool SketchObject::constraintHasExpression(int constNum) const
{
    App::ObjectIdentifier path = Constraints.createPath(constNum);
    auto info = getExpression(path);
    if (info.expression) {
        return true;
    }
    return false;
}

std::string SketchObject::getConstraintExpression(int constNum) const
{
    App::ObjectIdentifier path = Constraints.createPath(constNum);
    auto info = getExpression(path);
    if (info.expression) {
        std::string expression = info.expression->toString();
        return expression;
    }

    return {};
}

void SketchObject::setConstraintExpression(int constNum, const std::string& newExpression)
{
    App::ObjectIdentifier path = Constraints.createPath(constNum);
    auto info = getExpression(path);
    if (info.expression) {
        try {
            std::shared_ptr<App::Expression> expr(App::Expression::parse(this, newExpression));
            setExpression(path, std::move(expr));
        }
        catch (const Base::Exception&) {
            Base::Console().error("Failed to set constraint expression.");
        }
    }
}

std::string SketchObject::reverseAngleConstraintExpression(std::string expression)
{
    // Check if expression contains units (°, deg, rad)
    if (expression.find("°") != std::string::npos
        || expression.find("deg") != std::string::npos
        || expression.find("rad") != std::string::npos) {
        if (expression.substr(0, 9) == "180 ° - ") {
            expression = expression.substr(9, expression.size() - 9);
        }
        else {
            expression = "180 ° - (" + expression + ")";
        }
    }
    else {
        if (expression.substr(0, 6) == "180 - ") {
            expression = expression.substr(6, expression.size() - 6);
        }
        else {
            expression = "180 - (" + expression + ")";
        }
    }
    return expression;
}

int SketchObject::setVirtualSpace(int ConstrId, bool isinvirtualspace)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    // copy the list
    std::vector<Constraint*> newVals(vals);

    // clone the changed Constraint
    Constraint* constNew = vals[ConstrId]->clone();
    constNew->isInVirtualSpace = isinvirtualspace;
    newVals[ConstrId] = constNew;

    this->Constraints.setValues(std::move(newVals));

    // Solver didn't actually update, but we need this to inform view provider
    // to redraw
    signalSolverUpdate();

    return 0;
}

int SketchObject::setVirtualSpace(std::vector<int> constrIds, bool isinvirtualspace)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (constrIds.empty())
        return 0;

    std::sort(constrIds.begin(), constrIds.end());

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (constrIds.front() < 0 || constrIds.back() >= int(vals.size()))
        return -1;

    std::vector<Constraint*> newVals(vals);

    for (auto cid : constrIds) {
        // clone the changed Constraint
        if (vals[cid]->isInVirtualSpace != isinvirtualspace) {
            Constraint* constNew = vals[cid]->clone();
            constNew->isInVirtualSpace = isinvirtualspace;
            newVals[cid] = constNew;
        }
    }

    this->Constraints.setValues(std::move(newVals));

    // Solver didn't actually update, but we need this to inform view provider
    // to redraw
    signalSolverUpdate();

    return 0;
}


int SketchObject::getVirtualSpace(int ConstrId, bool& isinvirtualspace) const
{
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    isinvirtualspace = vals[ConstrId]->isInVirtualSpace;
    return 0;
}

int SketchObject::toggleVirtualSpace(int ConstrId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    // copy the list
    std::vector<Constraint*> newVals(vals);

    // clone the changed Constraint
    Constraint* constNew = vals[ConstrId]->clone();
    constNew->isInVirtualSpace = !constNew->isInVirtualSpace;
    newVals[ConstrId] = constNew;

    this->Constraints.setValues(std::move(newVals));

    // Solver didn't actually update, but we need this to inform view provider
    // to redraw
    signalSolverUpdate();

    return 0;
}


int SketchObject::setVisibility(std::vector<int> constrIds, bool isVisible)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (constrIds.empty())
        return 0;

    std::sort(constrIds.begin(), constrIds.end());

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (constrIds.front() < 0 || constrIds.back() >= int(vals.size()))
        return -1;

    std::vector<Constraint*> newVals(vals);

    for (auto cid : constrIds) {
        // clone the changed Constraint
        if (vals[cid]->isVisible != isVisible) {
            Constraint* constNew = vals[cid]->clone();
            constNew->isVisible = isVisible;
            newVals[cid] = constNew;
        }
    }

    this->Constraints.setValues(std::move(newVals));

    // Solver didn't actually update, but we need this to inform view provider
    // to redraw
    signalSolverUpdate();

    return 0;
}

int SketchObject::setVisibility(int ConstrId, bool isVisible)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    // copy the list
    std::vector<Constraint*> newVals(vals);

    // clone the changed Constraint
    Constraint* constNew = vals[ConstrId]->clone();
    constNew->isVisible = isVisible;
    newVals[ConstrId] = constNew;

    this->Constraints.setValues(std::move(newVals));

    // Solver didn't actually update, but we need this to inform view provider
    // to redraw
    signalSolverUpdate();

    return 0;
}


int SketchObject::setUpSketch()
{
    lastDoF = solvedSketch.setUpSketch(
        getCompleteGeometry(), Constraints.getValues(), getExternalGeometryCount());

    retrieveSolverDiagnostics();

    if (lastHasRedundancies || lastDoF < 0 || lastHasConflict || lastHasMalformedConstraints
        || lastHasPartialRedundancies)
        Constraints.touch();

    return lastDoF;
}

int SketchObject::diagnoseAdditionalConstraints(
    std::vector<Sketcher::Constraint*> additionalconstraints)
{
    auto objectconstraints = Constraints.getValues();

    std::vector<Sketcher::Constraint*> allconstraints;
    allconstraints.reserve(objectconstraints.size() + additionalconstraints.size());

    std::ranges::copy(objectconstraints, back_inserter(allconstraints));
    std::ranges::copy(additionalconstraints, back_inserter(allconstraints));

    lastDoF =
        solvedSketch.setUpSketch(getCompleteGeometry(), allconstraints, getExternalGeometryCount());

    retrieveSolverDiagnostics();

    return lastDoF;
}

int SketchObject::moveGeometries(const std::vector<GeoElementId>& geoEltIds, const Base::Vector3d& toPoint, bool relative,
                            bool updateGeoBeforeMoving)
{

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // if we are moving a point at SketchObject level, we need to start from a solved sketch
    // if we have conflicts we can forget about moving. However, there is the possibility that we
    // need to do programmatically moves of new geometry that has not been solved yet and that
    // because they were programmatically generated won't generate a conflict. This is the case of
    // Fillet for example. This is why exceptionally, it may be required to update the sketch
    // geometry to that of of SketchObject upon moving. => use updateGeometry parameter = true then


    if (updateGeoBeforeMoving || solverNeedsUpdate) {
        lastDoF = solvedSketch.setUpSketch(
            getCompleteGeometry(), Constraints.getValues(), getExternalGeometryCount());

        retrieveSolverDiagnostics();

        solverNeedsUpdate = false;
    }

    if (lastDoF < 0)// over-constrained sketch
        return -1;
    if (lastHasConflict)// conflicting constraints
        return -1;

    // move the point and solve
    lastSolverStatus = solvedSketch.moveGeometries(geoEltIds, toPoint, relative);

    // moving the point can not result in a conflict that we did not have
    // or a redundancy that we did not have before, or a change of DoF

    if (lastSolverStatus == 0) {
        std::vector<Part::Geometry*> geomlist = solvedSketch.extractGeometry();
        Geometry.setValues(geomlist);
        for (auto* geo :  geomlist) {
            if (geo){
                delete geo;
            }
        }
    }

    solvedSketch.resetInitMove();// reset solver point moving mechanism

    return lastSolverStatus;
}

int SketchObject::moveGeometry(int geoId, PointPos pos, const Base::Vector3d& toPoint, bool relative,
    bool updateGeoBeforeMoving)
{
    std::vector<GeoElementId> geoEltIds = { GeoElementId(geoId, pos) };
    return moveGeometries(geoEltIds, toPoint, relative, updateGeoBeforeMoving);
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomPoint *geomPoint, PointPos PosId)
{
    if (PosId == PointPos::start || PosId == PointPos::mid || PosId == PointPos::end)
        return geomPoint->getPoint();
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomLineSegment *lineSeg, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return lineSeg->getStartPoint();
    }
    case PointPos::end: {
        return lineSeg->getEndPoint();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomCircle *circle, PointPos PosId)
{
    auto pt = circle->getCenter();
    if(PosId != PointPos::mid)
        pt.x += circle->getRadius();
    return pt;
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomEllipse *ellipse, PointPos PosId)
{
    auto pt = ellipse->getCenter();
    if(PosId != PointPos::mid)
        pt += ellipse->getMajorAxisDir()*ellipse->getMajorRadius();
    return pt;
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomArcOfCircle *aoc, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return aoc->getStartPoint(/*emulateCCW=*/true);
    }
    case PointPos::end: {
        return aoc->getEndPoint(/*emulateCCW=*/true);
    }
    case PointPos::mid: {
        return aoc->getCenter();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomArcOfEllipse *aoe, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return aoe->getStartPoint(/*emulateCCW=*/true);
    }
    case PointPos::end: {
        return aoe->getEndPoint(/*emulateCCW=*/true);
    }
    case PointPos::mid: {
        return aoe->getCenter();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomArcOfHyperbola *aoh, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return aoh->getStartPoint();
    }
    case PointPos::end: {
        return aoh->getEndPoint();
    }
    case PointPos::mid: {
        return aoh->getCenter();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomArcOfParabola *aop, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return aop->getStartPoint();
    }
    case PointPos::end: {
        return aop->getEndPoint();
    }
    case PointPos::mid: {
        return aop->getCenter();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomBSplineCurve *bsp, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return bsp->getStartPoint();
    }
    case PointPos::end: {
        return bsp->getEndPoint();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

Base::Vector3d SketchObject::getPoint(const Part::Geometry *geo, PointPos PosId)
{
    if (auto point = freecad_cast<Part::GeomPoint*>(geo)) {
        return getPointForGeometry<Part::GeomPoint>(point, PosId);
    }
    else if (auto lineSegment = freecad_cast<Part::GeomLineSegment*>(geo)) {
        return getPointForGeometry<Part::GeomLineSegment>(lineSegment, PosId);
    }
    else if (auto circle = freecad_cast<Part::GeomCircle*>(geo)) {
        return getPointForGeometry<Part::GeomCircle>(circle, PosId);
    }
    else if (auto ellipse = freecad_cast<Part::GeomEllipse*>(geo)) {
        return getPointForGeometry<Part::GeomEllipse>(ellipse, PosId);
    }
    else if (auto arcOfCircle = freecad_cast<Part::GeomArcOfCircle*>(geo)) {
        return getPointForGeometry<Part::GeomArcOfCircle>(arcOfCircle, PosId);
    }
    else if (auto arcOfEllipse = freecad_cast<Part::GeomArcOfEllipse*>(geo)) {
        return getPointForGeometry<Part::GeomArcOfEllipse>(arcOfEllipse, PosId);
    }
    else if (auto arcOfHyperbola = freecad_cast<Part::GeomArcOfHyperbola*>(geo)) {
        return getPointForGeometry<Part::GeomArcOfHyperbola>(arcOfHyperbola, PosId);
    }
    else if (auto arcOfParabola = freecad_cast<Part::GeomArcOfParabola*>(geo)) {
        return getPointForGeometry<Part::GeomArcOfParabola>(arcOfParabola, PosId);
    }
    else if (auto bSplineCurve = freecad_cast<Part::GeomBSplineCurve*>(geo)) {
        return getPointForGeometry<Part::GeomBSplineCurve>(bSplineCurve, PosId);
    }
    return Base::Vector3d();
}

Base::Vector3d SketchObject::getPoint(int GeoId, PointPos PosId) const
{
    if (!(GeoId == H_Axis || GeoId == V_Axis
          || (GeoId <= getHighestCurveIndex() && GeoId >= -getExternalGeometryCount())))
        throw Base::ValueError("SketchObject::getPoint. Invalid GeoId was supplied.");
    const Part::Geometry* geo = getGeometry(GeoId);
    return getPoint(geo,PosId);
}

int SketchObject::getAxisCount() const
{
    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    int count = 0;
    for (const auto& geo : vals) {
        if (geo && GeometryFacade::getConstruction(geo)
            && geo->is<Part::GeomLineSegment>()) {
            count++;
        }
    }

    return count;
}

Base::Axis SketchObject::getAxis(int axId) const
{
    if (axId == H_Axis || axId == V_Axis || axId == N_Axis)
        return Part::Part2DObject::getAxis(axId);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    int count = 0;
    for (const auto& geo : vals) {
        if (geo && GeometryFacade::getConstruction(geo)
            && geo->is<Part::GeomLineSegment>()) {
            if (count == axId) {
                auto* lineSeg = static_cast<Part::GeomLineSegment*>(geo);
                Base::Vector3d start = lineSeg->getStartPoint();
                Base::Vector3d end = lineSeg->getEndPoint();
                return Base::Axis(start, end - start);
            }
            count++;
        }
    }

    return Base::Axis();
}

void SketchObject::acceptGeometry()
{
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    signalElementsChanged();
}

bool SketchObject::isSupportedGeometry(const Part::Geometry* geo) const
{
    if (geo->is<Part::GeomPoint>()
        || geo->is<Part::GeomCircle>()
        || geo->is<Part::GeomEllipse>()
        || geo->is<Part::GeomArcOfCircle>()
        || geo->is<Part::GeomArcOfEllipse>()
        || geo->is<Part::GeomArcOfHyperbola>()
        || geo->is<Part::GeomArcOfParabola>()
        || geo->is<Part::GeomBSplineCurve>()
        || geo->is<Part::GeomLineSegment>()) {
        return true;
    }
    if (geo->is<Part::GeomTrimmedCurve>()) {
        Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast(geo->handle());
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trim->BasisCurve());
        Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(trim->BasisCurve());
        if (!circle.IsNull() || !ellipse.IsNull()) {
            return true;
        }
    }
    return false;
}

std::vector<Part::Geometry*>
SketchObject::supportedGeometry(const std::vector<Part::Geometry*>& geoList) const
{
    std::vector<Part::Geometry*> supportedGeoList;
    supportedGeoList.reserve(geoList.size());
    // read-in geometry that the sketcher cannot handle
    for (const auto& geo : geoList) {
        if (isSupportedGeometry(geo)) {
            supportedGeoList.push_back(geo);
        }
    }

    return supportedGeoList;
}

int SketchObject::addGeometry(const std::vector<Part::Geometry*>& geoList,
                              bool construction /*=false*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);
    newVals.reserve(newVals.size() + geoList.size());
    for (auto& v : geoList) {
        Part::Geometry* copy = v->copy();
        generateId(copy);

        if (construction) {
            GeometryFacade::setConstruction(copy, construction);
        }

        newVals.push_back(copy);
    }

    // On setting geometry the onChanged method will call acceptGeometry(), thereby updating
    // constraint geometry indices and rebuilding the vertex index
    Geometry.setValues(std::move(newVals));

    return Geometry.getSize() - 1;
}

int SketchObject::addGeometry(const Part::Geometry* geo, bool construction /*=false*/)
{
    // this copy has a new random tag (see copy() vs clone())
    auto geoNew = std::unique_ptr<Part::Geometry>(geo->copy());

    return addGeometry(std::move(geoNew), construction);
}

int SketchObject::addGeometry(std::unique_ptr<Part::Geometry> newgeo, bool construction /*=false*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    auto* geoNew = newgeo.release();
    generateId(geoNew);

    if (construction) {
        GeometryFacade::setConstruction(geoNew, construction);
    }

    newVals.push_back(geoNew);

    // On setting geometry the onChanged method will call acceptGeometry(), thereby updating
    // constraint geometry indices and rebuilding the vertex index
    Geometry.setValues(std::move(newVals));

    return Geometry.getSize() - 1;
}

bool SketchObject::isClosedCurve(const Part::Geometry* geo)
{
    return (geo->is<Part::GeomCircle>()
            || geo->is<Part::GeomEllipse>()
            || (geo->is<Part::GeomBSplineCurve>()
                && static_cast<const Part::GeomBSplineCurve*>(geo)->isPeriodic()));
}

bool SketchObject::hasInternalGeometry(const Part::Geometry* geo)
{
    return (geo->is<Part::GeomEllipse>()
            || geo->is<Part::GeomArcOfEllipse>()
            || geo->is<Part::GeomArcOfHyperbola>()
            || geo->is<Part::GeomArcOfParabola>()
            || geo->is<Part::GeomBSplineCurve>());
}

int SketchObject::delGeometry(int GeoId, DeleteOptions options)
{
    if (GeoId < 0) {
        if(GeoId > GeoEnum::RefExt)
            return -1;
        return delExternal(-GeoId-1);
    }

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    if (GeoId >= int(vals.size())) {
        return -1;
    }

    if (options.testFlag(DeleteOption::IncludeInternalGeometry) && hasInternalGeometry(getGeometry(GeoId))) {
        // Only for supported types
        this->deleteUnusedInternalGeometry(GeoId, true);
        return 0;
    }

    std::vector<Part::Geometry*> newVals(vals);
    newVals.erase(newVals.begin() + GeoId);

    // Find coincident points to replace the points of the deleted geometry
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    for (PointPos PosId : {PointPos::start, PointPos::end, PointPos::mid}) {
        getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
        if (GeoIdList.size() > 1) {
            delConstraintOnPoint(GeoId, PosId, true /* only coincidence */);
            transferConstraints(GeoIdList[0], PosIdList[0], GeoIdList[1], PosIdList[1]);
        }
    }

    const std::vector<Constraint*>& constraints = this->Constraints.getValues();
    std::vector<Constraint*> newConstraints;
    newConstraints.reserve(constraints.size());
    for (const auto& constr : constraints) {
        if (auto newConstr = getConstraintAfterDeletingGeo(constr, GeoId)) {
            newConstraints.push_back(newConstr.release());
        }
    }

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(std::move(newVals));
        this->Constraints.setValues(std::move(newConstraints));
    }

    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::delGeometries(const std::vector<int>& GeoIds, DeleteOptions options)
{
    return delGeometries(GeoIds.begin(), GeoIds.end(), options);
}

template <class InputIt>
int SketchObject::delGeometries(InputIt first, InputIt last, DeleteOptions options)
{
    std::vector<int> sGeoIds;
    std::vector<int> negativeGeoIds;

    // Separate GeoIds into negative (external) and non-negative GeoIds
    for (auto it = first; it != last; ++it) {
        int geoId = *it;
        if (geoId < 0 && geoId <= GeoEnum::RefExt) {
            negativeGeoIds.push_back(geoId);
        }
        else if (geoId >= 0){
            sGeoIds.push_back(geoId);
        }
    }

    // Handle negative GeoIds by calling delExternal
    if (!negativeGeoIds.empty()) {
        int result = delExternal(negativeGeoIds);
        if (result != 0) {
            return result; // Return if deletion of external geometries failed
        }
    }

    // Proceed with non-negative GeoIds
    if (sGeoIds.empty()) {
        return 0; // No positive GeoIds to delete
    }

    // if a GeoId has internal geometry, it must delete internal geometries too
    for (auto c : Constraints.getValues()) {
        if (c->Type == InternalAlignment) {
            auto pos = std::ranges::find(sGeoIds, c->Second);

            if (pos != sGeoIds.end()) {
                sGeoIds.push_back(c->First);
            }
        }
    }

    std::ranges::sort(sGeoIds);
    // eliminate duplicates
    auto newend = std::unique(sGeoIds.begin(), sGeoIds.end());
    sGeoIds.resize(std::distance(sGeoIds.begin(), newend));

    return delGeometriesExclusiveList(sGeoIds, options);
}

int SketchObject::delGeometriesExclusiveList(const std::vector<int>& GeoIds, DeleteOptions options)
{
    std::vector<int> sGeoIds(GeoIds);

    std::ranges::sort(sGeoIds);
    if (sGeoIds.empty()) {
        return 0;
    }

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    if (sGeoIds.front() < 0 || sGeoIds.back() >= int(vals.size())) {
        return -1;
    }

    std::vector<Part::Geometry*> newVals(vals);
    for (auto it = sGeoIds.rbegin(); it != sGeoIds.rend(); ++it) {
        int GeoId = *it;
        newVals.erase(newVals.begin() + GeoId);

        // Find coincident points to replace the points of the deleted geometry
        std::vector<int> GeoIdList;
        std::vector<PointPos> PosIdList;
        for (PointPos PosId : {PointPos::start, PointPos::end, PointPos::mid}) {
            getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
            if (GeoIdList.size() > 1) {
                delConstraintOnPoint(GeoId, PosId, true /* only coincidence */);
                transferConstraints(GeoIdList[0], PosIdList[0], GeoIdList[1], PosIdList[1]);
            }
        }
    }

    // Copy the original constraints
    std::vector<Constraint*> constraints;
    for (const auto& ptr : this->Constraints.getValues()) {
        constraints.push_back(ptr->clone());
    }
    for (auto itGeo = sGeoIds.rbegin(); itGeo != sGeoIds.rend(); ++itGeo) {
        const int GeoId = *itGeo;
        for (auto& constr : constraints) {
            changeConstraintAfterDeletingGeo(constr, GeoId);
        }
    }

    constraints.erase(std::remove_if(constraints.begin(),
                                     constraints.end(),
                                     [](const auto& constr) {
                                         return constr->Type == ConstraintType::None;
                                     }),
                      constraints.end());

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(newVals);
        this->Constraints.setValues(std::move(constraints));
    }
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

void SketchObject::replaceGeometries(std::vector<int> oldGeoIds,
                                     std::vector<Part::Geometry*>& newGeos)
{
    auto& vals = getInternalGeometry();
    auto newVals(vals);

    if (std::ranges::any_of(oldGeoIds, [](auto geoId) {
            return geoId < 0;
        })) {
        THROWM(ValueError, "Cannot replace external geometries and axes.");
    }

    auto oldGeoIdIter = oldGeoIds.begin();
    auto newGeoIter = newGeos.begin();

    for (; oldGeoIdIter != oldGeoIds.end() && newGeoIter != newGeos.end();
         ++oldGeoIdIter, ++newGeoIter) {
        GeometryFacade::copyId(getGeometry(*oldGeoIdIter), *newGeoIter);
        newVals[*oldGeoIdIter] = *newGeoIter;
    }

    if (newGeoIter != newGeos.end()) {
        for (; newGeoIter != newGeos.end(); ++newGeoIter) {
            generateId(*newGeoIter);
            newVals.push_back(*newGeoIter);
        }
    }
    else {
        delGeometries(oldGeoIdIter, oldGeoIds.end());
    }

    Geometry.setValues(std::move(newVals));
}

int SketchObject::deleteAllGeometry(DeleteOptions options)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<Part::Geometry*> newVals(0);
    std::vector<Constraint*> newConstraints(0);

    // Avoid unnecessary updates and checks as this is a transaction
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(newVals);
        this->Constraints.setValues(newConstraints);
    }
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::deleteAllConstraints(DeleteOptions options)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<Constraint*> newConstraints(0);

    this->Constraints.setValues(newConstraints);

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::toggleConstruction(int GeoId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId >= 0) {
        const std::vector<Part::Geometry*>& vals = getInternalGeometry();
        if (GeoId >= int(vals.size())) {
            return -1;
        }

        if (getGeometryFacade(GeoId)->isInternalAligned()) {
            return -1;
        }

        // While it may seem that there is not a need to trigger an update at this time, because the
        // solver has its own copy of the geometry, and updateColors of the viewprovider may be
        // triggered by the clearselection of the UI command, this won't update the elements widget, in
        // the accumulative of actions it is judged that it is worth to trigger an update here.

        std::unique_ptr<Part::Geometry> geo(vals[GeoId]->clone());
        auto gft = GeometryFacade::getFacade(geo.get());
        gft->setConstruction(!gft->getConstruction());
        this->Geometry.set1Value(GeoId, std::move(geo));
    }
    else {
        if (GeoId > GeoEnum::RefExt) {
            return -1;
        }

        const std::vector<Part::Geometry*>& extGeos = getExternalGeometry();
        std::unique_ptr<Part::Geometry> geo(extGeos[-GeoId - 1]->clone());
        auto egf = ExternalGeometryFacade::getFacade(geo.get());
        egf->setFlag(ExternalGeometryExtension::Defining, !egf->testFlag(ExternalGeometryExtension::Defining));
        this->ExternalGeo.set1Value(-GeoId - 1, std::move(geo));
    }

    solverNeedsUpdate = true;
    signalSolverUpdate();  // FIXME:  In theory this is totally redundant, but now seems required
                           // for UI to update.
    return 0;
}

int SketchObject::setConstruction(int GeoId, bool on)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

   Part::PropertyGeometryList *prop;
    int idx;
    if (GeoId >= 0) {
        prop = &Geometry;
        if (GeoId < Geometry.getSize())
            idx = GeoId;
        else
            return -1;
    }else if (GeoId <= GeoEnum::RefExt && -GeoId-1 < ExternalGeo.getSize()) {
        prop = &ExternalGeo;
        idx = -GeoId-1;
    }else
        return -1;

    // While it may seem that there is not a need to trigger an update at this time, because the
    // solver has its own copy of the geometry, and updateColors of the viewprovider may be
    // triggered by the clearselection of the UI command, this won't update the elements widget, in
    // the accumulative of actions it is judged that it is worth to trigger an update here.

    std::unique_ptr<Part::Geometry> geo(prop->getValues()[idx]->clone());
    if(prop == &Geometry)
        GeometryFacade::setConstruction(geo.get(), on);
    else {
        auto egf = ExternalGeometryFacade::getFacade(geo.get());
        egf->setFlag(ExternalGeometryExtension::Defining, on);
    }

    prop->set1Value(idx,std::move(geo));
    solverNeedsUpdate = true;
    return 0;
}

// clang-format on
int SketchObject::toggleExternalGeometryFlag(
    const std::vector<int>& geoIds,
    const std::vector<ExternalGeometryExtension::Flag>& flags
)
{
    if (flags.empty()) {
        return 0;
    }
    auto flag = flags.front();

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    bool update = false;
    bool touched = false;
    auto geos = ExternalGeo.getValues();
    std::set<int> idSet(geoIds.begin(), geoIds.end());
    for (auto geoId : geoIds) {
        if (geoId > GeoEnum::RefExt || -geoId - 1 >= ExternalGeo.getSize()) {
            continue;
        }
        if (!idSet.contains(geoId)) {
            continue;
        }
        idSet.erase(geoId);
        const int idx = -geoId - 1;
        auto& geo = geos[idx];
        const auto egf = ExternalGeometryFacade::getFacade(geo);
        const bool value = !egf->testFlag(flag);
        if (!egf->getRef().empty()) {
            for (auto relatedGeoId : getRelatedGeometry(geoId)) {
                if (relatedGeoId == geoId) {
                    continue;
                }
                int relatedIndex = -relatedGeoId - 1;
                auto& relatedGeometry = geos[relatedIndex];
                relatedGeometry = relatedGeometry->clone();
                auto relatedFacade = ExternalGeometryFacade::getFacade(relatedGeometry);
                for (auto& _flag : flags) {
                    relatedFacade->setFlag(_flag, value);
                }
                idSet.erase(relatedGeoId);
            }
        }
        geo = geo->clone();
        egf->setGeometry(geo);
        for (auto& _flag : flags) {
            egf->setFlag(_flag, value);
        }
        update = update || (value || flag != ExternalGeometryExtension::Frozen);
        touched = true;
    }

    if (!touched) {
        return -1;
    }
    ExternalGeo.setValues(geos);
    if (update) {
        rebuildExternalGeometry();
    }
    return 0;
}
// clang-format off

void SketchObject::addGeometryState(const Constraint* cstr) const
{
    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    Sketcher::InternalType::InternalType constraintInternalAlignment = InternalType::None;
    bool constraintBlockedState = false;

    if (getInternalTypeState(cstr, constraintInternalAlignment)) {
        auto gf = GeometryFacade::getFacade(vals[cstr->First]);
        gf->setInternalType(constraintInternalAlignment);
    }
    else if (getBlockedState(cstr, constraintBlockedState)) {
        auto gf = GeometryFacade::getFacade(vals[cstr->First]);
        gf->setBlocked(constraintBlockedState);
    }
}

void SketchObject::removeGeometryState(const Constraint* cstr) const
{
    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    // Assign correct Internal Geometry Type (see SketchGeometryExtension)
    if (cstr->Type == InternalAlignment) {
        auto gf = GeometryFacade::getFacade(vals[cstr->First]);
        gf->setInternalType(InternalType::None);
    }

    // Assign Blocked geometry mode (see SketchGeometryExtension)
    if (cstr->Type == Block) {
        auto gf = GeometryFacade::getFacade(vals[cstr->First]);
        gf->setBlocked(false);
    }
}

// ConstraintList is used only to make copies.
int SketchObject::addConstraints(const std::vector<Constraint*>& ConstraintList)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    std::vector<Constraint*> newVals(vals);
    newVals.insert(newVals.end(), ConstraintList.begin(), ConstraintList.end());
    for (std::size_t i = newVals.size() - ConstraintList.size(); i < newVals.size(); i++) {
        Constraint* cnew = newVals[i]->clone();
        newVals[i] = cnew;

        if (cnew->Type == Tangent || cnew->Type == Perpendicular) {
            AutoLockTangencyAndPerpty(cnew);
        }

        addGeometryState(cnew);
    }

    this->Constraints.setValues(std::move(newVals));

    return this->Constraints.getSize() - 1;
}

int SketchObject::addCopyOfConstraints(const SketchObject& orig)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    const std::vector<Constraint*>& origvals = orig.Constraints.getValues();

    std::vector<Constraint*> newVals(vals);

    newVals.reserve(vals.size() + origvals.size());

    for (auto& v : origvals)
        newVals.push_back(v->copy());

    this->Constraints.setValues(std::move(newVals));

    auto& uvals = this->Constraints.getValues();

    std::size_t uvalssize = uvals.size();

    for (std::size_t i = uvalssize, j = 0; i < uvals.size(); i++, j++) {
        if (uvals[i]->isDriving && uvals[i]->isDimensional()) {

            App::ObjectIdentifier spath = orig.Constraints.createPath(j);

            App::PropertyExpressionEngine::ExpressionInfo expr_info = orig.getExpression(spath);

            if (expr_info.expression) {// if there is an expression on the source dimensional
                App::ObjectIdentifier dpath = this->Constraints.createPath(i);
                setExpression(dpath,
                              std::shared_ptr<App::Expression>(expr_info.expression->copy()));
            }
        }
    }

    if (noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return this->Constraints.getSize() - 1;
}

int SketchObject::addConstraint(const Constraint* constraint)
{
    auto constraint_ptr = std::unique_ptr<Constraint>(constraint->clone());

    return addConstraint(std::move(constraint_ptr));
}

int SketchObject::addConstraint(std::unique_ptr<Constraint> constraint)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    std::vector<Constraint*> newVals(vals);

    Constraint* constNew = constraint.release();

    if (constNew->Type == Tangent || constNew->Type == Perpendicular)
        AutoLockTangencyAndPerpty(constNew);

    addGeometryState(constNew);

    newVals.push_back(constNew);// add new constraint at the back

    this->Constraints.setValues(std::move(newVals));

    return this->Constraints.getSize() - 1;
}

int SketchObject::delConstraint(int ConstrId, DeleteOptions options)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(vals.size())) {
        return -1;
    }

    std::vector<Constraint*> newVals(vals);
    auto ctriter = newVals.begin() + ConstrId;
    removeGeometryState(*ctriter);
    newVals.erase(ctriter);
    this->Constraints.setValues(std::move(newVals));

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::delConstraints(std::vector<int> ConstrIds, DeleteOptions options)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);
    if (ConstrIds.empty()) {
        return 0;
    }

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    std::vector<Constraint*> newVals(vals);

    std::sort(ConstrIds.begin(), ConstrIds.end());

    if (ConstrIds.front() < 0 || ConstrIds.back() >= int(vals.size()))
        return -1;

    for (auto rit = ConstrIds.rbegin(); rit != ConstrIds.rend(); rit++) {
        auto ctriter = newVals.begin() + *rit;
        removeGeometryState(*ctriter);
        newVals.erase(ctriter);
    }

    this->Constraints.setValues(std::move(newVals));

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::delConstraintOnPoint(int VertexId, bool onlyCoincident)
{
    int GeoId;
    PointPos PosId;
    if (VertexId == GeoEnum::RtPnt) {// RootPoint
        GeoId = Sketcher::GeoEnum::RtPnt;
        PosId = PointPos::start;
    }
    else
        getGeoVertexIndex(VertexId, GeoId, PosId);

    return delConstraintOnPoint(GeoId, PosId, onlyCoincident);
}

// clang-format on
int SketchObject::delConstraintOnPoint(int geoId, PointPos posId, bool onlyCoincident)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();
    std::vector<Constraint*> newVals;
    newVals.reserve(vals.size());

    // check if constraints can be redirected to some other point
    int replaceGeoId = GeoEnum::GeoUndef;
    PointPos replacePosId = Sketcher::PointPos::none;
    auto findReplacement = [geoId, posId, &replaceGeoId, &replacePosId, &vals]() {
        auto it = std::ranges::find_if(vals, [geoId, posId](auto& constr) {
            return constr->Type == Sketcher::Coincident
                && constr->involvesGeoIdAndPosId(geoId, posId);
        });

        if (it == vals.end()) {
            return;
        }

        if ((*it)->First == geoId && (*it)->FirstPos == posId) {
            replaceGeoId = (*it)->Second;
            replacePosId = (*it)->SecondPos;
        }
        else {
            replaceGeoId = (*it)->First;
            replacePosId = (*it)->FirstPos;
        }
    };

    auto transferToReplacement =
        [&geoId, &posId, &replaceGeoId, &replacePosId](int& constrGeoId, PointPos& constrPosId) {
            if (replaceGeoId == GeoEnum::GeoUndef) {
                return false;
            }
            if (geoId != constrGeoId || posId != constrPosId) {
                return false;
            }
            constrGeoId = replaceGeoId;
            constrPosId = replacePosId;
            return true;
        };

    findReplacement();

    auto performCoincidenceChecksOrChanges = [&](auto& constr) -> bool {
        if (replaceGeoId == GeoEnum::GeoUndef) {
            return false;
        }
        if (constr->involvesGeoIdAndPosId(replaceGeoId, replacePosId)) {
            return false;
        }
        // Assuming `constr` already involves geoId and posId, all conditions are already met
        constr->substituteIndexAndPos(geoId, posId, replaceGeoId, replacePosId);
        return true;
    };

    auto performAllConstraintChecksOrChanges = [&](auto& constr) -> std::optional<bool> {
        if (constr->Type != Sketcher::Coincident && onlyCoincident) {
            return true;
        }
        switch (constr->Type) {
            case Sketcher::Coincident:
                return performCoincidenceChecksOrChanges(constr);
            case Sketcher::Distance:
            case Sketcher::DistanceX:
            case Sketcher::DistanceY: {
                return (
                    transferToReplacement(constr->First, constr->FirstPos)
                    || transferToReplacement(constr->Second, constr->SecondPos)
                );
            }
            case Sketcher::PointOnObject: {
                return transferToReplacement(constr->First, constr->FirstPos);
            }
            case Sketcher::Tangent:
            case Sketcher::Perpendicular: {
                // we could keep this constraint by converting it to a simple one, but that doesn't
                // always work (for example if tangent-via-point is necessary), and it is not really
                // worth it
                return false;
            }
            case Sketcher::Vertical:
            case Sketcher::Horizontal:
            case Sketcher::Symmetric: {
                return false;
            }
            default:
                return std::nullopt;
        }
    };

    // remove or redirect any constraints associated with the given point
    for (auto& constr : vals) {
        // keep the constraint if it doesn't involve the point
        if (!constr->involvesGeoIdAndPosId(geoId, posId)) {
            // for these constraints remove the constraint even if it is not directly associated
            // with the given point
            const bool isOneOfDistanceTypes = constr->Type == Sketcher::Distance
                || constr->Type == Sketcher::DistanceX || constr->Type == Sketcher::DistanceY;
            const bool involvesEntireCurve = constr->First == geoId
                && constr->FirstPos == PointPos::none;
            const bool isPosAnEndpoint = posId == PointPos::start || posId == PointPos::end;
            if (isOneOfDistanceTypes && involvesEntireCurve && isPosAnEndpoint) {
                continue;
            }
            newVals.push_back(constr);
            continue;
        }
        if (performAllConstraintChecksOrChanges(constr).value_or(true)) {
            newVals.push_back(constr);
        }
    }

    if (newVals.size() < vals.size()) {
        this->Constraints.setValues(std::move(newVals));

        return 0;
    }

    return -1;  // no such constraint
}
// clang-format off

void SketchObject::transferFilletConstraints(int geoId1, PointPos posId1, int geoId2,
                                             PointPos posId2)
{
    // If the lines don't intersect, there's no original corner to work with so
    // don't try to transfer the constraints. But we should delete line length and equal
    // constraints and constraints on the affected endpoints because they're about
    // to move unpredictably.
    if (!arePointsCoincident(geoId1, posId1, geoId2, posId2)) {
        // Delete constraints on the endpoints
        delConstraintOnPoint(geoId1, posId1, false);
        delConstraintOnPoint(geoId2, posId2, false);

        // Delete line length and equal constraints
        const std::vector<Constraint*>& constraints = this->Constraints.getValues();
        std::vector<int> deleteme;
        for (int i = 0; i < int(constraints.size()); i++) {
            const Constraint* c = constraints[i];
            if (c->Type == Sketcher::Distance || c->Type == Sketcher::Equal) {
                bool line1 = c->First == geoId1 && c->FirstPos == PointPos::none;
                bool line2 = c->First == geoId2 && c->FirstPos == PointPos::none;
                if (line1 || line2) {
                    deleteme.push_back(i);
                }
            }
        }
        delConstraints(std::move(deleteme), DeleteOption::NoFlag);
        return;
    }

    // If the lines aren't straight, don't try to transfer the constraints.
    // TODO: Add support for curved lines.
    const Part::Geometry* geo1 = getGeometry(geoId1);
    const Part::Geometry* geo2 = getGeometry(geoId2);
    if (!geo1->is<Part::GeomLineSegment>()
        || !geo2->is<Part::GeomLineSegment>()) {
        delConstraintOnPoint(geoId1, posId1, false);
        delConstraintOnPoint(geoId2, posId2, false);
        return;
    }

    // Add a vertex to preserve the original intersection of the filleted lines
    Part::GeomPoint* originalCorner = new Part::GeomPoint(getPoint(geoId1, posId1));
    int originalCornerId = addGeometry(originalCorner, true);
    delete originalCorner;

    // Constrain the vertex to the two lines
    Sketcher::Constraint* cornerToLine1 = new Sketcher::Constraint();
    cornerToLine1->Type = Sketcher::PointOnObject;
    cornerToLine1->First = originalCornerId;
    cornerToLine1->FirstPos = PointPos::start;
    cornerToLine1->Second = geoId1;
    cornerToLine1->SecondPos = PointPos::none;
    addConstraint(cornerToLine1);
    delete cornerToLine1;
    Sketcher::Constraint* cornerToLine2 = new Sketcher::Constraint();
    cornerToLine2->Type = Sketcher::PointOnObject;
    cornerToLine2->First = originalCornerId;
    cornerToLine2->FirstPos = PointPos::start;
    cornerToLine2->Second = geoId2;
    cornerToLine2->SecondPos = PointPos::none;
    addConstraint(cornerToLine2);
    delete cornerToLine2;

    Base::StateLocker lock(managedoperation, true);

    // Loop through all the constraints and try to do reasonable things with the affected ones
    std::vector<Constraint*> newConstraints;
    for (auto c : this->Constraints.getValues()) {
        // Keep track of whether the affected lines and endpoints appear in this constraint
        bool point1First = c->First == geoId1 && c->FirstPos == posId1;
        bool point2First = c->First == geoId2 && c->FirstPos == posId2;
        bool point1Second = c->Second == geoId1 && c->SecondPos == posId1;
        bool point2Second = c->Second == geoId2 && c->SecondPos == posId2;
        bool point1Third = c->Third == geoId1 && c->ThirdPos == posId1;
        bool point2Third = c->Third == geoId2 && c->ThirdPos == posId2;
        bool line1First = c->First == geoId1 && c->FirstPos == PointPos::none;
        bool line2First = c->First == geoId2 && c->FirstPos == PointPos::none;
        bool line1Second = c->Second == geoId1 && c->SecondPos == PointPos::none;
        bool line2Second = c->Second == geoId2 && c->SecondPos == PointPos::none;

        if (c->Type == Sketcher::Coincident) {
            if ((point1First && point2Second) || (point2First && point1Second)) {
                // This is the constraint holding the two edges together that are about to be
                // filleted.  This constraint goes away because the edges will touch the fillet
                // instead.
                continue;
            }
            if (point1First || point2First) {
                // Move the coincident constraint to the new corner point
                c->First = originalCornerId;
                c->FirstPos = PointPos::start;
            }
            if (point1Second || point2Second) {
                // Move the coincident constraint to the new corner point
                c->Second = originalCornerId;
                c->SecondPos = PointPos::start;
            }
        }
        else if (c->Type == Sketcher::Horizontal || c->Type == Sketcher::Vertical) {
            // Point-to-point horizontal or vertical constraint, move to new corner point
            if (point1First || point2First) {
                c->First = originalCornerId;
                c->FirstPos = PointPos::start;
            }
            if (point1Second || point2Second) {
                c->Second = originalCornerId;
                c->SecondPos = PointPos::start;
            }
        }
        else if (c->Type == Sketcher::Distance || c->Type == Sketcher::DistanceX
                 || c->Type == Sketcher::DistanceY) {
            // Point-to-point distance constraint.  Move it to the new corner point
            if (point1First || point2First) {
                c->First = originalCornerId;
                c->FirstPos = PointPos::start;
            }
            if (point1Second || point2Second) {
                c->Second = originalCornerId;
                c->SecondPos = PointPos::start;
            }

            // Distance constraint on the line itself. Change it to point-point between the far end
            // of the line and the new corner
            if (line1First) {
                c->FirstPos = (posId1 == PointPos::start) ? PointPos::end : PointPos::start;
                c->Second = originalCornerId;
                c->SecondPos = PointPos::start;
            }
            if (line2First) {
                c->FirstPos = (posId2 == PointPos::start) ? PointPos::end : PointPos::start;
                c->Second = originalCornerId;
                c->SecondPos = PointPos::start;
            }
        }
        else if (c->Type == Sketcher::PointOnObject) {
            // The corner to be filleted was touching some other object.
            if (point1First || point2First) {
                c->First = originalCornerId;
                c->FirstPos = PointPos::start;
            }
        }
        else if (c->Type == Sketcher::Equal) {
            // Equal length constraints are dicey because the lines are getting shorter.  Safer to
            // delete them and let the user notice the underconstraint.
            if (line1First || line2First || line1Second || line2Second) {
                continue;
            }
        }
        else if (c->Type == Sketcher::Symmetric) {
            // Symmetries should probably be preserved relative to the original corner
            if (point1First || point2First) {
                c->First = originalCornerId;
                c->FirstPos = PointPos::start;
            }
            else if (point1Second || point2Second) {
                c->Second = originalCornerId;
                c->SecondPos = PointPos::start;
            }
            else if (point1Third || point2Third) {
                c->Third = originalCornerId;
                c->ThirdPos = PointPos::start;
            }
        }
        else if (c->Type == Sketcher::SnellsLaw) {
            // Can't imagine any cases where you'd fillet a vertex going through a lens, so let's
            // delete to be safe.
            continue;
        }
        else if (point1First || point2First || point1Second || point2Second || point1Third
                 || point2Third) {
            // Delete any other point-based constraints on the relevant points
            continue;
        }

        // Default: keep all other constraints
        newConstraints.push_back(c->clone());
    }
    this->Constraints.setValues(std::move(newConstraints));
}

// clang-format on
int SketchObject::transferConstraints(
    int fromGeoId,
    PointPos fromPosId,
    int toGeoId,
    PointPos toPosId,
    bool doNotTransformTangencies
)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();
    std::vector<Constraint*> newVals(vals);
    bool changed = false;
    for (int i = 0; i < int(newVals.size()); i++) {
        if (vals[i]->Type == Sketcher::InternalAlignment) {
            // Transferring internal alignment constraint can cause malformed constraints.
            // For example a B-spline pole being a point instead of a circle.
            continue;
        }
        else if (vals[i]->involvesGeoIdAndPosId(fromGeoId, fromPosId)
                 && !vals[i]->involvesGeoIdAndPosId(toGeoId, toPosId)) {
            std::unique_ptr<Constraint> constNew(newVals[i]->clone());
            constNew->substituteIndexAndPos(fromGeoId, fromPosId, toGeoId, toPosId);
            if (vals[i]->First < 0 && vals[i]->Second < 0) {
                // TODO: Can `vals[i]->Third` be involved as well?
                // If it is, we need to be sure at most ONE of these is external
                continue;
            }

            switch (vals[i]->Type) {
                case Sketcher::Tangent:
                case Sketcher::Perpendicular: {
                    // If not explicitly confirmed, nothing guarantees that a tangent can be freely
                    // transferred to another coincident point, as the transfer destination edge
                    // most likely won't be intended to be tangent. However, if it is an end to end
                    // point tangency, the user expects it to be substituted by a coincidence
                    // constraint.
                    if (!doNotTransformTangencies) {
                        constNew->Type = Sketcher::Coincident;
                    }
                    break;
                }
                case Sketcher::Angle:
                    // With respect to angle constraints, if it is a DeepSOIC style angle constraint
                    // (segment+segment+point), then no problem arises as the segments are
                    // PosId=none. In this case there is no call to this function.
                    //
                    // However, other angle constraints are problematic because they are created on
                    // segments, but internally operate on vertices, PosId=start Such constraint may
                    // not be successfully transferred on deletion of the segments.
                    continue;
                default:
                    break;
            }

            Constraint* constPtr = constNew.release();
            newVals[i] = constPtr;
            changed = true;
        }
    }

    // assign the new values only if something has changed
    if (changed) {
        this->Constraints.setValues(std::move(newVals));
    }
    return 0;
}
// clang-format off

std::vector<int> SketchObject::chooseFilletsEdges(const std::vector<int>& GeoIdList) const
{
    if (GeoIdList.size() == 2) {
        return GeoIdList;
    }

    std::vector<int> dst;
    for (auto id : GeoIdList) {
        if (!GeometryFacade::getFacade(getGeometry(id))->getConstruction()) {
            dst.push_back(id);

            if (dst.size() > 2) {
                return {};
            }
        }
    }
    return dst;
}
int SketchObject::fillet(int GeoId, PointPos PosId, double radius, bool trim, bool createCorner, bool chamfer)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    // Find the other geometry Id associated with the coincident point
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);

    GeoIdList = chooseFilletsEdges(GeoIdList);

    // only coincident points between two (non-external) edges can be filleted
    if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
        const Part::Geometry* geo1 = getGeometry(GeoIdList[0]);
        const Part::Geometry* geo2 = getGeometry(GeoIdList[1]);
        if (geo1->is<Part::GeomLineSegment>()
            && geo2->is<Part::GeomLineSegment>()) {
            auto* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo1);
            auto* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);

            Base::Vector3d midPnt1 = (lineSeg1->getStartPoint() + lineSeg1->getEndPoint()) / 2;
            Base::Vector3d midPnt2 = (lineSeg2->getStartPoint() + lineSeg2->getEndPoint()) / 2;
            return fillet(GeoIdList[0], GeoIdList[1], midPnt1, midPnt2, radius, trim, createCorner, chamfer);
        }
    }

    return -1;
}

int SketchObject::fillet(int GeoId1, int GeoId2, const Base::Vector3d& refPnt1,
                         const Base::Vector3d& refPnt2, double radius, bool trim, bool createCorner, bool chamfer)
{
    if (GeoId1 < 0 || GeoId1 > getHighestCurveIndex() || GeoId2 < 0 || GeoId2 > getHighestCurveIndex()) {
        return -1;
    }

    // If either of the two input lines are locked, don't try to trim since it won't work anyway
    const Part::Geometry* geo1 = getGeometry(GeoId1);
    const Part::Geometry* geo2 = getGeometry(GeoId2);
    if (trim && (GeometryFacade::getBlocked(geo1) || GeometryFacade::getBlocked(geo2))) {
        trim = false;
    }

    int pos1 = 0;
    int pos2 = 0;
    bool reverse = false;
    std::unique_ptr<Part::GeomArcOfCircle> arc(createFilletGeometry(geo1, geo2, refPnt1, refPnt2, radius, pos1, pos2, reverse));
    if (!arc) {
        return -1;
    }

    int filletId = addGeometry(arc.get());
    if (filletId < 0) {
        return -1;
    }

    PointPos PosId1 = static_cast<PointPos>(pos1);
    PointPos PosId2= static_cast<PointPos>(pos2);
    PointPos filletPosId1 = PointPos::none;
    PointPos filletPosId2 = PointPos::none;

    Base::Vector3d p1 = arc->getStartPoint(true);
    Base::Vector3d p2 = arc->getEndPoint(true);

    if (trim) {
        if (createCorner && geo1->is<Part::GeomLineSegment>() && geo2->is<Part::GeomLineSegment>()) {
            transferFilletConstraints(GeoId1, PosId1, GeoId2, PosId2);
        }
        else {
            delConstraintOnPoint(GeoId1, PosId1, false);
            delConstraintOnPoint(GeoId2, PosId2, false);
        }

        if (reverse) {
            filletPosId1 = PointPos::start;
            filletPosId2 = PointPos::end;
            moveGeometry(GeoId1, PosId1, p1, false, true);
            moveGeometry(GeoId2, PosId2, p2, false, true);
        }
        else {
            filletPosId1 = PointPos::end;
            filletPosId2 = PointPos::start;
            moveGeometry(GeoId1, PosId1, p2, false, true);
            moveGeometry(GeoId2, PosId2, p1, false, true);
        }

        auto tangent1 = std::make_unique<Sketcher::Constraint>();
        auto tangent2 = std::make_unique<Sketcher::Constraint>();

        tangent1->Type = Sketcher::Tangent;
        tangent1->First = GeoId1;
        tangent1->FirstPos = PosId1;
        tangent1->Second = filletId;
        tangent1->SecondPos = filletPosId1;

        tangent2->Type = Sketcher::Tangent;
        tangent2->First = GeoId2;
        tangent2->FirstPos = PosId2;
        tangent2->Second = filletId;
        tangent2->SecondPos = filletPosId2;

        addConstraint(std::move(tangent1));
        addConstraint(std::move(tangent2));
    }

    if (chamfer) {
        auto line = std::make_unique<Part::GeomLineSegment>();
        line->setPoints(p1, p2);
        int lineGeoId = addGeometry(line.get());


        auto coinc1 = std::make_unique<Sketcher::Constraint>();
        auto coinc2 = std::make_unique<Sketcher::Constraint>();

        coinc1->Type = Sketcher::Coincident;
        coinc1->First = lineGeoId;
        coinc1->FirstPos = filletPosId1;

        coinc2->Type = Sketcher::Coincident;
        coinc2->First = lineGeoId;
        coinc2->FirstPos = filletPosId2;

        if (trim) {
            coinc1->Second = GeoId1;
            coinc1->SecondPos = PosId1;
            coinc2->Second = GeoId2;
            coinc2->SecondPos = PosId2;
        }
        else {
            coinc1->Second = filletId;
            coinc1->SecondPos = PointPos::start;
            coinc2->Second = filletId;
            coinc2->SecondPos = PointPos::end;
        }

        addConstraint(std::move(coinc1));
        addConstraint(std::move(coinc2));

        setConstruction(filletId, true);
    }

    // if we do not have a recompute after the geometry creation, the sketch must be solved to
    // update the DoF of the solver
    if (noRecomputes) {
        solve();
    }

    return 0;
}

int SketchObject::extend(int GeoId, double increment, PointPos endpoint)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const std::vector<Part::Geometry*>& geomList = getInternalGeometry();
    Part::Geometry* geom = geomList[GeoId];
    int retcode = -1;
    if (geom->is<Part::GeomLineSegment>()) {
        Part::GeomLineSegment* seg = static_cast<Part::GeomLineSegment*>(geom);
        Base::Vector3d startVec = seg->getStartPoint();
        Base::Vector3d endVec = seg->getEndPoint();
        if (endpoint == PointPos::start) {
            Base::Vector3d newPoint = startVec - endVec;
            double scaleFactor = newPoint.Length() + increment;
            newPoint.Normalize();
            newPoint.Scale(scaleFactor, scaleFactor, scaleFactor);
            newPoint = newPoint + endVec;
            retcode = moveGeometry(GeoId, Sketcher::PointPos::start, newPoint, false, true);
        }
        else if (endpoint == PointPos::end) {
            Base::Vector3d newPoint = endVec - startVec;
            double scaleFactor = newPoint.Length() + increment;
            newPoint.Normalize();
            newPoint.Scale(scaleFactor, scaleFactor, scaleFactor);
            newPoint = newPoint + startVec;
            retcode = moveGeometry(GeoId, Sketcher::PointPos::end, newPoint, false, true);
        }
    }
    else if (geom->is<Part::GeomArcOfCircle>()) {
        Part::GeomArcOfCircle* arc = static_cast<Part::GeomArcOfCircle*>(geom);
        double startArc, endArc;
        arc->getRange(startArc, endArc, true);
        if (endpoint == PointPos::start) {
            arc->setRange(startArc - increment, endArc, true);
            retcode = 0;
        }
        else if (endpoint == PointPos::end) {
            arc->setRange(startArc, endArc + increment, true);
            retcode = 0;
        }
    }
    if (retcode == 0 && noRecomputes) {
        solve();
    }
    return retcode;
}

std::unique_ptr<Constraint> SketchObject::createConstraint(
    Sketcher::ConstraintType constrType, int firstGeoId, Sketcher::PointPos firstPos,
    int secondGeoId, Sketcher::PointPos secondPos, int thirdGeoId, Sketcher::PointPos thirdPos)
{
    auto newConstr = std::make_unique<Sketcher::Constraint>();

    newConstr->Type = constrType;
    newConstr->First = firstGeoId;
    newConstr->FirstPos = firstPos;
    newConstr->Second = secondGeoId;
    newConstr->SecondPos = secondPos;
    newConstr->Third = thirdGeoId;
    newConstr->ThirdPos = thirdPos;
    return newConstr;
}

void SketchObject::addConstraint(Sketcher::ConstraintType constrType, int firstGeoId,
                                 Sketcher::PointPos firstPos, int secondGeoId,
                                 Sketcher::PointPos secondPos, int thirdGeoId,
                                 Sketcher::PointPos thirdPos)
{
    auto newConstr = createConstraint(
        constrType, firstGeoId, firstPos, secondGeoId, secondPos, thirdGeoId, thirdPos);

    this->addConstraint(std::move(newConstr));
}

std::unique_ptr<Constraint>
SketchObject::getConstraintAfterDeletingGeo(const Constraint* constr,
                                            const int deletedGeoId) const
{
    if (!constr) {
        return nullptr;
    }

    // TODO: While this is not incorrect, it recreates all constraints regardless of whether or not we need to.
    auto newConstr = std::unique_ptr<Constraint>(constr->clone());

    changeConstraintAfterDeletingGeo(newConstr.get(), deletedGeoId);

    if (newConstr->Type == ConstraintType::None) {
        return nullptr;
    }

    return newConstr;
}

void SketchObject::changeConstraintAfterDeletingGeo(Constraint* constr,
                                                    const int deletedGeoId) const
{
    if (!constr) {
        return;
    }

    if (constr->First == deletedGeoId ||
        constr->Second == deletedGeoId ||
        constr->Third == deletedGeoId) {
        constr->Type = ConstraintType::None;
        return;
    }

    int step = 1;
    std::function<bool (const int&)> needsUpdate = [&deletedGeoId](const int& givenId) -> bool {
        return givenId > deletedGeoId;
    };
    if (deletedGeoId < 0) {
        step = -1;
        needsUpdate = [&deletedGeoId](const int& givenId) -> bool {
            return givenId < deletedGeoId && givenId != GeoEnum::GeoUndef;
        };
    }

    if (needsUpdate(constr->First)) {
        constr->First -= step;
    }
    if (needsUpdate(constr->Second)) {
        constr->Second -= step;
    }
    if (needsUpdate(constr->Third)) {
        constr->Third -= step;
    }
}

// clang-format on
bool SketchObject::seekTrimPoints(
    int GeoId,
    const Base::Vector3d& point,
    int& GeoId1,
    Base::Vector3d& intersect1,
    int& GeoId2,
    Base::Vector3d& intersect2
)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        return false;
    }

    auto geos = getCompleteGeometry();  // this includes the axes too

    geos.resize(geos.size() - 2);  // remove the axes to avoid intersections with the axes

    int localindex1, localindex2;

    // Not found in will be returned as -1, not as GeoUndef, Part WB is agnostic to the concept of
    // GeoUndef
    if (!Part2DObject::seekTrimPoints(geos, GeoId, point, localindex1, intersect1, localindex2, intersect2)) {
        return false;
    }

    // invalid complete geometry indices are mapped to GeoUndef
    GeoId1 = getGeoIdFromCompleteGeometryIndex(localindex1);
    GeoId2 = getGeoIdFromCompleteGeometryIndex(localindex2);

    return true;
}

// given a geometry and a point, returns the corresponding parameter of the geometry point
// closest to the point. Wrapped around a try-catch so the calling operation can fail without
// throwing an exception.
bool getIntersectionParameter(const Part::Geometry* geo, const Base::Vector3d point, double& pointParam)
{
    const auto* curve = static_cast<const Part::GeomCurve*>(geo);

    try {
        curve->closestParameter(point, pointParam);
    }
    catch (Base::CADKernelError& e) {
        e.reportException();
        return false;
    }

    return true;
}

bool arePointsWithinPrecision(const Base::Vector3d& point1, const Base::Vector3d& point2)
{
    // From testing: 500x (or 0.000050) is needed in order to not falsely distinguish points
    // calculated with seekTrimPoints
    return ((point1 - point2).Length() < 500 * Precision::Confusion());
}

bool areParamsWithinApproximation(double param1, double param2)
{
    // From testing: 500x (or 0.000050) is needed in order to not falsely distinguish points
    // calculated with seekTrimPoints
    return (std::abs(param1 - param2) < Precision::PApproximation());
}

// returns true if the point defined by (GeoId1, pos1) can be considered to be coincident with
// point.
bool isPointAtPosition(const SketchObject* obj, int GeoId1, PointPos pos1, const Base::Vector3d& point)
{
    Base::Vector3d pp = obj->getPoint(GeoId1, pos1);

    return arePointsWithinPrecision(point, pp);
}

// Checks whether preexisting constraints must be converted to new constraints.
// Preexisting point on object constraints get converted to coincidents.
// Returns:
//     - The constraint that should be used to constraint GeoId and cuttingGeoId
std::unique_ptr<Constraint> transformPreexistingConstraintForTrim(
    const SketchObject* obj,
    const Constraint* constr,
    int GeoId,
    int cuttingGeoId,
    const Base::Vector3d& cutPointVec,
    int newGeoId,
    PointPos newPosId
)
{
    /* TODO: It is possible that the trimming entity has both a PointOnObject constraint to the
     * trimmed entity, and a simple Tangent constraint to the trimmed entity. In this case we
     * want to change to a single end-to-end tangency, i.e we want to ensure that constrType1
     * is set to Sketcher::Tangent, that the secondPos1 is captured from the PointOnObject,
     * and also make sure that the PointOnObject constraint is deleted.
     */
    // TODO: Symmetric and distance constraints (sometimes together) can be changed to something
    std::unique_ptr<Constraint> newConstr;
    if (cuttingGeoId == GeoEnum::GeoUndef || !constr->involvesGeoId(cuttingGeoId)
        || !constr->involvesGeoIdAndPosId(GeoId, PointPos::none)) {
        return newConstr;
    }
    switch (constr->Type) {
        case PointOnObject: {
            // we might want to transform this (and the new point-on-object constraints) into a
            // coincidence At this stage of the check the point has to be an end of `cuttingGeoId`
            // on the edge of `GeoId`.
            if (isPointAtPosition(obj, constr->First, constr->FirstPos, cutPointVec)) {
                // We already know the point-on-object is on the whole of GeoId
                newConstr.reset(constr->copy());
                newConstr->Type = Sketcher::Coincident;
                newConstr->Second = newGeoId;
                newConstr->SecondPos = newPosId;
            }
            break;
        }
        case Tangent:
        case Perpendicular: {
            // These may have to be turned into endpoint-to-endpoint or endpoint-to-edge
            // TODO: could there be tangent/perpendicular constraints not involving the trim that
            // are modified below?
            newConstr.reset(constr->copy());
            newConstr->substituteIndexAndPos(GeoId, PointPos::none, newGeoId, newPosId);
            // make sure the first position is a point
            if (newConstr->FirstPos == PointPos::none) {
                std::swap(newConstr->First, newConstr->Second);
                std::swap(newConstr->FirstPos, newConstr->SecondPos);
            }
            // there is no need for the third point if it exists
            newConstr->Third = GeoEnum::GeoUndef;
            newConstr->ThirdPos = PointPos::none;
            break;
        }
        default:
            break;
    }
    return newConstr;
}

std::unique_ptr<Constraint> getNewConstraintAtTrimCut(
    const SketchObject* obj,
    int cuttingGeoId,
    int cutGeoId,
    PointPos cutPosId,
    const Base::Vector3d& cutPointVec
)
{
    auto newConstr = std::make_unique<Sketcher::Constraint>();
    newConstr->First = cutGeoId;
    newConstr->FirstPos = cutPosId;
    newConstr->Second = cuttingGeoId;
    if (isPointAtPosition(obj, cuttingGeoId, PointPos::start, cutPointVec)) {
        newConstr->Type = Sketcher::Coincident;
        newConstr->SecondPos = PointPos::start;
    }
    else if (isPointAtPosition(obj, cuttingGeoId, PointPos::end, cutPointVec)) {
        newConstr->Type = Sketcher::Coincident;
        newConstr->SecondPos = PointPos::end;
    }
    else {
        // Points are sufficiently far apart: use point-on-object
        newConstr->Type = Sketcher::PointOnObject;
        newConstr->SecondPos = PointPos::none;
    }
    return newConstr;
}

bool isGeoIdAllowedForTrim(const SketchObject* obj, int GeoId)
{
    const auto* geo = obj->getGeometry(GeoId);

    return GeoId >= 0 && GeoId <= obj->getHighestCurveIndex()
        && GeometryFacade::isInternalType(geo, InternalType::None);
}

bool getParamLimitsOfNewGeosForTrim(
    const SketchObject* obj,
    int GeoId,
    std::array<int, 2>& cuttingGeoIds,
    std::array<Base::Vector3d, 2>& cutPoints,
    std::vector<std::pair<double, double>>& paramsOfNewGeos
)
{
    const auto* geoAsCurve = obj->getGeometry<Part::GeomCurve>(GeoId);
    double firstParam = geoAsCurve->getFirstParameter();
    double lastParam = geoAsCurve->getLastParameter();
    double cut0Param {firstParam}, cut1Param {lastParam};

    bool allParamsFound = getIntersectionParameter(geoAsCurve, cutPoints[0], cut0Param)
        && getIntersectionParameter(geoAsCurve, cutPoints[1], cut1Param);
    if (!allParamsFound) {
        return false;
    }

    if (!obj->isClosedCurve(geoAsCurve) && areParamsWithinApproximation(firstParam, cut0Param)) {
        cuttingGeoIds[0] = GeoEnum::GeoUndef;
    }

    if (!obj->isClosedCurve(geoAsCurve) && areParamsWithinApproximation(lastParam, cut1Param)) {
        cuttingGeoIds[1] = GeoEnum::GeoUndef;
    }

    size_t numUndefs = std::count(cuttingGeoIds.begin(), cuttingGeoIds.end(), GeoEnum::GeoUndef);

    if (numUndefs == 0 && arePointsWithinPrecision(cutPoints[0], cutPoints[1])) {
        // If both points are detected and are coincident, deletion is the only option.
        paramsOfNewGeos.clear();
        return true;
    }

    paramsOfNewGeos.assign(2 - numUndefs, {firstParam, lastParam});

    if (paramsOfNewGeos.empty()) {
        return true;
    }

    if (obj->isClosedCurve(geoAsCurve)) {
        paramsOfNewGeos.pop_back();
    }

    if (cuttingGeoIds[0] != GeoEnum::GeoUndef) {
        paramsOfNewGeos.front().second = cut0Param;
    }
    if (cuttingGeoIds[1] != GeoEnum::GeoUndef) {
        paramsOfNewGeos.back().first = cut1Param;
    }

    return true;
}

void createArcsFromGeoWithLimits(
    const Part::GeomCurve* geo,
    const std::vector<std::pair<double, double>>& paramsOfNewGeos,
    std::vector<Part::Geometry*>& newGeos
)
{
    for (auto& [u1, u2] : paramsOfNewGeos) {
        auto newGeo = static_cast<const Part::GeomCurve*>(geo)->createArc(u1, u2);
        assert(newGeo);
        newGeos.emplace_back(newGeo);
    }
}

void createNewConstraintsForTrim(
    const SketchObject* obj,
    const int GeoId,
    const std::array<int, 2>& cuttingGeoIds,
    const std::array<Base::Vector3d, 2>& cutPoints,
    const std::vector<int>& newIds,
    const std::vector<const Part::Geometry*> newGeos,
    std::vector<int>& idsOfOldConstraints,
    std::vector<Constraint*>& newConstraints,
    std::set<int, std::greater<>>& geoIdsToBeDeleted
)
{
    const auto& allConstraints = obj->Constraints.getValues();

    bool isPoint1ConstrainedOnGeoId1 = false;
    bool isPoint2ConstrainedOnGeoId2 = false;

    for (const auto& oldConstrId : idsOfOldConstraints) {
        // trim-specific changes first
        const Constraint* con = allConstraints[oldConstrId];
        if (con->Type == InternalAlignment) {
            geoIdsToBeDeleted.insert(con->First);
            continue;
        }
        if (auto newConstr = transformPreexistingConstraintForTrim(
                obj,
                con,
                GeoId,
                cuttingGeoIds[0],
                cutPoints[0],
                newIds.front(),
                PointPos::end
            )) {
            newConstraints.push_back(newConstr.release());
            isPoint1ConstrainedOnGeoId1 = true;
            continue;
        }
        if (auto newConstr = transformPreexistingConstraintForTrim(
                obj,
                con,
                GeoId,
                cuttingGeoIds[1],
                cutPoints[1],
                newIds.back(),
                PointPos::start
            )) {
            newConstraints.push_back(newConstr.release());
            isPoint2ConstrainedOnGeoId2 = true;
            continue;
        }
        // We have already transferred all constraints on endpoints to the new pieces.
        // If there is still any left, this means one of the remaining pieces was degenerate.
        if (!(con->Type == Angle || con->involvesGeoIdAndPosId(GeoId, PointPos::none))) {
            continue;
        }
        // constraint has not yet been changed
        obj->deriveConstraintsForPieces(GeoId, newIds, newGeos, con, newConstraints);
    }

    // Add point-on-object/coincidence constraints with the newly exposed points.
    // This will need to account for the constraints that were already converted
    // to coincident or end-to-end tangency/perpendicularity.
    // TODO: Tangent/perpendicular not yet covered

    if (cuttingGeoIds[0] != GeoEnum::GeoUndef && !isPoint1ConstrainedOnGeoId1) {
        newConstraints.emplace_back(
            getNewConstraintAtTrimCut(obj, cuttingGeoIds[0], newIds.front(), PointPos::end, cutPoints[0])
                .release()
        );
    }

    if (cuttingGeoIds[1] != GeoEnum::GeoUndef && !isPoint2ConstrainedOnGeoId2) {
        newConstraints.emplace_back(
            getNewConstraintAtTrimCut(obj, cuttingGeoIds[1], newIds.back(), PointPos::start, cutPoints[1])
                .release()
        );
    }
}

std::optional<size_t> findPieceContainingPoint(
    const SketchObject* obj,
    const Part::Geometry* geo,
    const Base::Vector3d& point,
    const std::vector<int>& newIds,
    const std::vector<const Part::Geometry*>& newGeos
)
{
    double conParam;
    auto* geoAsCurve = static_cast<const Part::GeomCurve*>(geo);
    geoAsCurve->closestParameter(point, conParam);
    // Choose based on where the closest point lies
    // If it's not there, just leave this constraint out
    for (size_t i = 0; i < newIds.size(); ++i) {
        double newGeoFirstParam = static_cast<const Part::GeomCurve*>(newGeos[i])->getFirstParameter();
        double newGeoLastParam = static_cast<const Part::GeomCurve*>(newGeos[i])->getLastParameter();
        // For periodic curves the point may need a full revolution
        if ((newGeoFirstParam - conParam) > Precision::PApproximation() && obj->isClosedCurve(geo)) {
            conParam += (geoAsCurve->getLastParameter() - geoAsCurve->getFirstParameter());
        }
        if ((newGeoFirstParam - conParam) <= Precision::PApproximation()
            && (conParam - newGeoLastParam) <= Precision::PApproximation()) {
            return i;
        }
    }
    return std::nullopt;
}

int SketchObject::trim(int GeoId, const Base::Vector3d& point)
{
    if (!isGeoIdAllowedForTrim(this, GeoId)) {
        return -1;
    }
    // Remove internal geometry beforehand for now
    // FIXME: we should be able to transfer these to new curves smoothly
    // auto geo = getGeometry(GeoId);
    const auto* geoAsCurve = getGeometry<Part::GeomCurve>(GeoId);

    if (geoAsCurve == nullptr) {
        return -1;
    }

    bool isOriginalCurveConstruction = GeometryFacade::getConstruction(geoAsCurve);
    bool isOriginalCurvePeriodic = isClosedCurve(geoAsCurve);

    //******************* Step A => Detection of intersection - Common to all Geometries
    //****************************************//
    // GeoIds intersecting the curve around `point`
    std::array<int, 2> cuttingGeoIds {GeoEnum::GeoUndef, GeoEnum::GeoUndef};
    // Points at the intersection
    std::array<Base::Vector3d, 2> cutPoints;

    // Using SketchObject wrapper, as Part2DObject version returns GeoId = -1 when intersection not
    // found, which is wrong for a GeoId (axis). seekTrimPoints returns:
    // - For a parameter associated with "point" between an intersection and the end point
    // (non-periodic case) cuttingGeoIds[0] != GeoUndef and cuttingGeoIds[1] == GeoUndef
    // - For a parameter associated with "point" between the start point and an intersection
    // (non-periodic case) cuttingGeoIds[1] != GeoUndef and cuttingGeoIds[0] == GeoUndef
    // - For a parameter associated with "point" between two intersection points, cuttingGeoIds[0]
    // != GeoUndef and cuttingGeoIds[1] != GeoUndef
    //
    // FirstParam < point1param < point2param < LastParam
    if (!SketchObject::seekTrimPoints(
            GeoId,
            point,
            cuttingGeoIds[0],
            cutPoints[0],
            cuttingGeoIds[1],
            cutPoints[1]
        )) {
        // If no suitable trim points are found, then trim defaults to deleting the geometry
        delGeometry(GeoId);
        return 0;
    }

    // TODO: find trim parameters
    std::vector<std::pair<double, double>> paramsOfNewGeos;
    paramsOfNewGeos.reserve(2);
    if (!getParamLimitsOfNewGeosForTrim(this, GeoId, cuttingGeoIds, cutPoints, paramsOfNewGeos)) {
        return -1;
    }

    //******************* Step B => Creation of new geometries
    //****************************************//
    std::vector<int> newIds;
    std::vector<Part::Geometry*> newGeos;
    std::vector<const Part::Geometry*> newGeosAsConsts;

    switch (paramsOfNewGeos.size()) {
        case 0: {
            delGeometry(GeoId);
            return 0;
        }
        case 1: {
            newIds.push_back(GeoId);
            break;
        }
        case 2: {
            newIds.push_back(GeoId);
            newIds.push_back(getHighestCurveIndex() + 1);
            break;
        }
        default: {
            return -1;
        }
    }

    createArcsFromGeoWithLimits(geoAsCurve, paramsOfNewGeos, newGeos);
    for (const auto* geo : newGeos) {
        newGeosAsConsts.push_back(geo);
    }

    //******************* Step C => Creation of new constraints
    //****************************************//
    // Now that we have the new curves, change constraints as needed
    // Some are covered with `deriveConstraintsForPieces`, others are specific to trim
    // FIXME: We are using non-smart pointers since that's what's needed in `addConstraints`.
    const auto& allConstraints = this->Constraints.getValues();
    std::vector<Constraint*> newConstraints;
    std::vector<int> idsOfOldConstraints;
    std::set<int, std::greater<>> geoIdsToBeDeleted;
    getConstraintIndices(GeoId, idsOfOldConstraints);
    // remove the constraints that we want to manually transfer
    // We could transfer beforehand but in case of exception that transfer is permanent
    if (!isOriginalCurvePeriodic) {
        std::erase_if(idsOfOldConstraints, [&GeoId, &allConstraints, &cuttingGeoIds](const auto& i) {
            auto* constr = allConstraints[i];
            bool involvesStart = constr->involvesGeoIdAndPosId(GeoId, PointPos::start);
            bool involvesEnd = constr->involvesGeoIdAndPosId(GeoId, PointPos::end);
            bool keepStart = cuttingGeoIds[0] != GeoEnum::GeoUndef;
            bool keepEnd = cuttingGeoIds[1] != GeoEnum::GeoUndef;
            bool involvesBothButNotBothKept = involvesStart && involvesEnd && !(keepStart && keepEnd);
            return !involvesBothButNotBothKept
                && ((involvesStart && keepStart) || (involvesEnd && keepEnd));
        });
    }
    std::erase_if(idsOfOldConstraints, [&GeoId, &allConstraints](const auto& i) {
        return (allConstraints[i]->involvesGeoIdAndPosId(GeoId, PointPos::mid));
    });

    createNewConstraintsForTrim(
        this,
        GeoId,
        cuttingGeoIds,
        cutPoints,
        newIds,
        newGeosAsConsts,
        idsOfOldConstraints,
        newConstraints,
        geoIdsToBeDeleted
    );

    //******************* Step D => Replacing geometries and constraints
    //****************************************//

    // Constraints related to start/mid/end points of original
    [[maybe_unused]] auto constrainAsEqual = [this](int GeoId1, int GeoId2) {
        auto newConstr = std::make_unique<Sketcher::Constraint>();

        // Build Constraints associated with new pair of arcs
        newConstr->Type = Sketcher::Equal;
        newConstr->First = GeoId1;
        newConstr->FirstPos = Sketcher::PointPos::none;
        newConstr->Second = GeoId2;
        newConstr->SecondPos = Sketcher::PointPos::none;
        addConstraint(std::move(newConstr));
    };

    delConstraints(std::move(idsOfOldConstraints), DeleteOption::NoFlag);

    if (!isOriginalCurvePeriodic) {
        transferConstraints(GeoId, PointPos::start, newIds.front(), PointPos::start, true);
        transferConstraints(GeoId, PointPos::end, newIds.back(), PointPos::end, true);
    }
    bool geomHasMid = geoAsCurve->isDerivedFrom<Part::GeomConic>()
        || geoAsCurve->isDerivedFrom<Part::GeomArcOfConic>();
    if (geomHasMid) {
        transferConstraints(GeoId, PointPos::mid, newIds.front(), PointPos::mid, true);
        // Make centers coincident
        if (newIds.size() > 1) {
            auto* joint = new Constraint();
            joint->Type = Coincident;
            joint->First = newIds.front();
            joint->FirstPos = PointPos::mid;
            joint->Second = newIds.back();
            joint->SecondPos = PointPos::mid;
            newConstraints.push_back(joint);

            // Any radius etc. equality constraints here
            // TODO: There could be some form of equality between the constraints here. However, it
            // may happen that this is imposed by an elaborate set of additional constraints. When
            // that happens, this causes redundant constraints, and in worse cases (incorrect)
            // complaints of over-constraint and solver failures.

            // if (std::ranges::none_of(newConstraints, [](const auto& constr) {
            //         return constr->Type == ConstraintType::Equal;
            //     })) {
            //     constrainAsEqual(newIds.front(), newIds.back());
            // }
            // TODO: ensure alignment as well?
        }
    }

    replaceGeometries({GeoId}, newGeos);
    for (auto newId : newIds) {
        setConstruction(newId, isOriginalCurveConstruction);
    }

    if (noRecomputes) {
        solve();
    }

    for (auto& deletedGeoId : geoIdsToBeDeleted) {
        for (auto& cons : newConstraints) {
            changeConstraintAfterDeletingGeo(cons, deletedGeoId);
        }
    }
    std::erase_if(newConstraints, [](const auto& constr) {
        return constr->Type == ConstraintType::None;
    });
    delGeometries(geoIdsToBeDeleted.begin(), geoIdsToBeDeleted.end());
    addConstraints(newConstraints);

    if (noRecomputes) {
        solve();
    }

    //******************* Cleanup
    //****************************************//

    // Since we used regular "non-smart" pointers, we have to handle cleanup
    for (auto& cons : newConstraints) {
        delete cons;
    }

    return 0;
}

bool SketchObject::deriveConstraintsForPieces(
    const int oldId,
    const std::vector<int>& newIds,
    const Constraint* con,
    std::vector<Constraint*>& newConstraints
) const
{
    std::vector<const Part::Geometry*> newGeos;
    for (auto& newId : newIds) {
        newGeos.push_back(getGeometry(newId));
    }

    return deriveConstraintsForPieces(oldId, newIds, newGeos, con, newConstraints);
}

bool SketchObject::deriveConstraintsForPieces(
    const int oldId,
    const std::vector<int>& newIds,
    const std::vector<const Part::Geometry*>& newGeos,
    const Constraint* con,
    std::vector<Constraint*>& newConstraints
) const
{
    const Part::Geometry* geo = getGeometry(oldId);
    int conId = con->First;
    PointPos conPos = con->FirstPos;
    if (conId == oldId) {
        conId = con->Second;
        conPos = con->SecondPos;
    }

    bool newGeosLikelyNotCreated = std::ranges::find(newGeos, nullptr) != newGeos.end();

    bool transferToAll = false;
    switch (con->Type) {
        case Horizontal:
        case Vertical:
        case Parallel: {
            transferToAll = geo->is<Part::GeomLineSegment>();
        } break;
        case Tangent:
        case Perpendicular: {
            if (geo->is<Part::GeomLineSegment>()) {
                transferToAll = true;
                break;
            }

            const Part::Geometry* conGeo = getGeometry(conId);
            if (!(conGeo && conGeo->isDerivedFrom<Part::GeomCurve>())) {
                return false;
            }

            // no use going forward if newGeos aren't ready
            if (newGeosLikelyNotCreated) {
                break;
            }

            // For now: just transfer to the first intersection
            // TODO: Actually check that there was perpendicularity earlier
            // TODO: Choose piece based on parameters ("values" of the constraint)
            for (size_t i = 0; i < newIds.size(); ++i) {
                std::vector<std::pair<Base::Vector3d, Base::Vector3d>> intersections;
                bool intersects
                    = static_cast<const Part::GeomCurve*>(newGeos[i])
                          ->intersect(static_cast<const Part::GeomCurve*>(conGeo), intersections);

                if (intersects) {
                    Constraint* trans = con->copy();
                    trans->substituteIndex(oldId, newIds[i]);
                    newConstraints.push_back(trans);
                    return true;
                }
            }
        } break;
        case Angle: {
            const auto [thirdGeo, thirdPos] = con->getElement(2);
            if (thirdGeo == oldId) {
                // TODO: transfer to a coincident point,
                // is it possible to do it somewhere else and avoid?
                std::vector<int> GeoIdList;
                std::vector<PointPos> PosIdList;
                getDirectlyCoincidentPoints(thirdGeo, thirdPos, GeoIdList, PosIdList);
                if (GeoIdList.size() <= 1) {
                    // TODO: Even in this case we can add a point
                    return false;
                }

                // transfer only to the curve that actually intersects
                Base::Vector3d point(getPoint(thirdGeo, thirdPos));
                std::optional<size_t> idx = findPieceContainingPoint(this, geo, point, newIds, newGeos);

                if (idx.has_value()) {
                    Constraint* trans = con->copy();
                    trans->substituteIndexAndPos(GeoIdList[0], PosIdList[0], GeoIdList[1], PosIdList[1]);
                    trans->substituteIndex(oldId, newIds[idx.value()]);
                    newConstraints.push_back(trans);
                    return true;
                }
            }
            else if (thirdGeo != GeoEnum::GeoUndef) {
                // Angle via point but the point won't change, can transfer to all or first
                // transfer only to the curve that actually intersects
                Base::Vector3d point(getPoint(thirdGeo, thirdPos));
                std::optional<size_t> idx = findPieceContainingPoint(this, geo, point, newIds, newGeos);

                if (idx.has_value()) {
                    Constraint* trans = con->copy();
                    trans->substituteIndex(oldId, newIds[idx.value()]);
                    newConstraints.push_back(trans);
                    return true;
                }
                break;
            }
            else if (std::ranges::any_of(newGeos, [](const Part::Geometry* geo) {
                         return !geo->is<Part::GeomLineSegment>();
                     })) {
                // Angle without a specific point is only supported when _all_ geometries are lines.
                // If the original was a line, we may reach this point, for example, when converting
                // it to NURBS.

                // NOTE: We may decide to change this logic in the future. Follows
                // `Sketch::addConstraint`.
                return false;
            }
            else {
                // Straight up angle, can transfer to all or first
                transferToAll = true;
                break;
            }
        } break;
        case Distance:
        case DistanceX:
        case DistanceY:
        case PointOnObject: {
            if (con->FirstPos == PointPos::none && con->SecondPos == PointPos::none
                && newIds.size() > 1) {
                Constraint* dist = con->copy();
                dist->First = newIds.front();
                dist->FirstPos = PointPos::start;
                dist->Second = newIds.back();
                dist->SecondPos = PointPos::end;
                newConstraints.push_back(dist);
                return true;
            }

            if (conId == GeoEnum::GeoUndef || newGeosLikelyNotCreated) {
                // nothing further to do
                return false;
            }

            Base::Vector3d conPoint(getPoint(conId, conPos));
            double conParam;
            auto* geoAsCurve = static_cast<const Part::GeomCurve*>(geo);
            geoAsCurve->closestParameter(conPoint, conParam);
            // Choose based on where the closest point lies
            // If it's not there, just leave this constraint out
            for (size_t i = 0; i < newIds.size(); ++i) {
                double newGeoFirstParam
                    = static_cast<const Part::GeomCurve*>(newGeos[i])->getFirstParameter();
                double newGeoLastParam
                    = static_cast<const Part::GeomCurve*>(newGeos[i])->getLastParameter();
                // For periodic curves the point may need a full revolution
                if ((newGeoFirstParam - conParam) > Precision::PApproximation()
                    && isClosedCurve(geo)) {
                    conParam += (geoAsCurve->getLastParameter() - geoAsCurve->getFirstParameter());
                }
                if ((newGeoFirstParam - conParam) <= Precision::PApproximation()
                    && (conParam - newGeoLastParam) <= Precision::PApproximation()) {
                    Constraint* trans = con->copy();
                    trans->First = conId;
                    trans->FirstPos = conPos;
                    trans->Second = newIds[i];
                    trans->SecondPos = PointPos::none;
                    newConstraints.push_back(trans);
                    return true;
                }
            }
        } break;
        case Radius:
        case Diameter:
        case Equal: {
            // Only transfer to one of them (arbitrarily chosen here as the first) and only if the
            // curve is a conic or its arc
            // TODO: Some equalities may be transferred, using something along the lines of
            // `getDirectlyCoincidentPoints`
            if (geo->isDerivedFrom<Part::GeomConic>() || geo->isDerivedFrom<Part::GeomArcOfConic>()) {
                Constraint* trans = con->copy();
                trans->substituteIndex(oldId, newIds.front());
                newConstraints.push_back(trans);
                break;
            }
        } break;
        default:
            // Release other constraints
            break;
    }

    if (transferToAll) {
        for (auto& newId : newIds) {
            Constraint* trans = con->copy();
            trans->substituteIndex(oldId, newId);
            newConstraints.push_back(trans);
        }

        return true;
    }

    return false;
}

int SketchObject::split(int GeoId, const Base::Vector3d& point)
{
    // No need to check input data validity as this is an sketchobject managed operation

    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        return -1;
    }

    // FIXME: we should be able to transfer these to new curves smoothly
    deleteUnusedInternalGeometryAndUpdateGeoId(GeoId);
    const auto* geoAsCurve = getGeometry<Part::GeomCurve>(GeoId);

    bool isOriginalCurvePeriodic = isClosedCurve(geoAsCurve);
    std::vector<int> newIds;
    std::vector<Part::Geometry*> newGeos;
    std::vector<Constraint*> newConstraints;

    double splitParam;
    geoAsCurve->closestParameter(point, splitParam);

    // TODO: find trim parameters
    std::vector<std::pair<double, double>> paramsOfNewGeos(
        isOriginalCurvePeriodic ? 1 : 2,
        {geoAsCurve->getFirstParameter(), geoAsCurve->getLastParameter()}
    );
    paramsOfNewGeos.front().second = isOriginalCurvePeriodic
        ? (splitParam + geoAsCurve->getLastParameter() - geoAsCurve->getFirstParameter())
        : splitParam;
    paramsOfNewGeos.back().first = splitParam;

    switch (paramsOfNewGeos.size()) {
        case 0: {
            delGeometry(GeoId);
            return 0;
        }
        case 1: {
            newIds.push_back(GeoId);
            break;
        }
        case 2: {
            newIds.push_back(GeoId);
            newIds.push_back(getHighestCurveIndex() + 1);
            break;
        }
        default: {
            return -1;
        }
    }

    createArcsFromGeoWithLimits(geoAsCurve, paramsOfNewGeos, newGeos);

    std::vector<int> idsOfOldConstraints;
    getConstraintIndices(GeoId, idsOfOldConstraints);

    const auto& allConstraints = this->Constraints.getValues();

    std::erase_if(idsOfOldConstraints, [&GeoId, &allConstraints](const auto& i) {
        return !allConstraints[i]->involvesGeoIdAndPosId(GeoId, PointPos::none);
    });

    for (const auto& oldConstrId : idsOfOldConstraints) {
        Constraint* con = allConstraints[oldConstrId];
        deriveConstraintsForPieces(GeoId, newIds, con, newConstraints);
    }

    // This also seems to reset SketchObject::Geometry.
    // TODO: figure out why, and if that check must be used
    geoAsCurve = getGeometry<Part::GeomCurve>(GeoId);

    if (!isOriginalCurvePeriodic) {
        Constraint* joint = new Constraint();
        joint->Type = Coincident;
        joint->First = newIds.front();
        joint->FirstPos = PointPos::end;
        joint->Second = newIds.back();
        joint->SecondPos = PointPos::start;
        newConstraints.push_back(joint);

        transferConstraints(GeoId, PointPos::start, newIds.front(), PointPos::start);
        transferConstraints(GeoId, PointPos::end, newIds.back(), PointPos::end);
    }

    // This additional constraint is there to maintain existing behavior.
    // TODO: Decide whether to remove it altogether or also apply to other curves with centers.
    if (geoAsCurve->is<Part::GeomArcOfCircle>()) {
        Constraint* joint = new Constraint();
        joint->Type = Coincident;
        joint->First = newIds.front();
        joint->FirstPos = PointPos::mid;
        joint->Second = newIds.back();
        joint->SecondPos = PointPos::mid;
        newConstraints.push_back(joint);
    }

    if (geoAsCurve->isDerivedFrom<Part::GeomConic>()
        || geoAsCurve->isDerivedFrom<Part::GeomArcOfConic>()) {
        transferConstraints(GeoId, PointPos::mid, newIds.front(), PointPos::mid);
    }

    delConstraints(std::move(idsOfOldConstraints), DeleteOption::NoSolve);
    replaceGeometries({GeoId}, newGeos);
    addConstraints(newConstraints);

    // `if (noRecomputes)` results in a failed test (`testPD_TNPSketchPadSketchSplit(self)`)
    // TODO: figure out why, and if that check must be used
    solve();

    for (auto& cons : newConstraints) {
        delete cons;
    }

    return 0;
}

// clang-format off

int SketchObject::join(int geoId1, Sketcher::PointPos posId1, int geoId2, Sketcher::PointPos posId2, int continuity)
{
    // No need to check input data validity as this is an sketchobject managed operation

    Base::StateLocker lock(managedoperation, true);

    if (Sketcher::PointPos::start != posId1 && Sketcher::PointPos::end != posId1
        && Sketcher::PointPos::start != posId2 && Sketcher::PointPos::end != posId2) {
        THROWM(ValueError, "Invalid positions: points must be start or end points of a curve.");
        return -1;
    }

    if (geoId1 == geoId2) {
        THROWM(ValueError, "Connecting the end points of the same curve is not yet supported.");
        return -1;
    }

    if (geoId1 < 0 || geoId1 > getHighestCurveIndex() || geoId2 < 0
        || geoId2 > getHighestCurveIndex()) {
        return -1;
    }

    // get the old splines
    auto* geo1 = dynamic_cast<const Part::GeomCurve*>(getGeometry(geoId1));
    auto* geo2 = dynamic_cast<const Part::GeomCurve*>(getGeometry(geoId2));

    if (GeometryFacade::getConstruction(geo1) != GeometryFacade::getConstruction(geo2)) {
        THROWM(ValueError, "Cannot join construction and non-construction geometries.");
        return -1;
    }

    // TODO: make both curves b-splines here itself
    if (!geo1 || !geo2) {
        return -1;
    }

    // TODO: is there a cleaner way to get our mutable bsp's?
    // we need the splines to be mutable because we may reverse them
    // and/or change their degree
    std::unique_ptr<Part::GeomBSplineCurve> bsp1(
        geo1->toNurbs(geo1->getFirstParameter(), geo1->getLastParameter()));
    std::unique_ptr<Part::GeomBSplineCurve> bsp2(
        geo2->toNurbs(geo2->getFirstParameter(), geo2->getLastParameter()));

    if (bsp1->isPeriodic() || bsp2->isPeriodic()) {
        THROWM(ValueError, "It is only possible to join non-periodic curves.");
        return -1;
    }

    // reverse the splines if needed: join end of 1st to start of 2nd
    if (Sketcher::PointPos::start == posId1)
        bsp1->reverse();
    if (Sketcher::PointPos::end == posId2)
        bsp2->reverse();

    // ensure the degrees of both curves are the same
    if (bsp1->getDegree() < bsp2->getDegree())
        bsp1->increaseDegree(bsp2->getDegree());
    else if (bsp2->getDegree() < bsp1->getDegree())
        bsp2->increaseDegree(bsp1->getDegree());

    // TODO: Check for tangent constraint here
    bool makeC1Continuous = (continuity >= 1);

    // TODO: Rescale one or both sections to fulfill some purpose.
    // This could include making param between [0,1], and/or making
    // C1 continuity possible.
    if (makeC1Continuous) {
        // We assume here that there is already G1 continuity.
        // Just scale parameters to get C1.
        Base::Vector3d slope1 = bsp1->firstDerivativeAtParameter(bsp1->getLastParameter());
        Base::Vector3d slope2 = bsp2->firstDerivativeAtParameter(bsp2->getFirstParameter());
        // TODO: slope2 can technically be a zero vector
        // But that seems not possible unless the spline is trivial.
        // Prove or account for the possibility.
        double scale = slope2.Length() / slope1.Length();
        bsp2->scaleKnotsToBounds(0, scale * (bsp2->getLastParameter() - bsp2->getFirstParameter()));
    }

    // set up vectors for new poles, knots, mults
    std::vector<Base::Vector3d> poles1 = bsp1->getPoles();
    std::vector<double> weights1 = bsp1->getWeights();
    std::vector<double> knots1 = bsp1->getKnots();
    std::vector<int> mults1 = bsp1->getMultiplicities();
    std::vector<Base::Vector3d> poles2 = bsp2->getPoles();
    std::vector<double> weights2 = bsp2->getWeights();
    std::vector<double> knots2 = bsp2->getKnots();
    std::vector<int> mults2 = bsp2->getMultiplicities();

    std::vector<Base::Vector3d> newPoles(std::move(poles1));
    std::vector<double> newWeights(std::move(weights1));
    std::vector<double> newKnots(std::move(knots1));
    std::vector<int> newMults(std::move(mults1));

    poles2.erase(poles2.begin());
    if (makeC1Continuous)
        newPoles.erase(newPoles.end()-1);
    newPoles.insert(newPoles.end(),
                    std::make_move_iterator(poles2.begin()),
                    std::make_move_iterator(poles2.end()));

    // TODO: Weights might need to be scaled
    weights2.erase(weights2.begin());
    if (makeC1Continuous)
        newWeights.erase(newWeights.end()-1);
    newWeights.insert(newWeights.end(),
                      std::make_move_iterator(weights2.begin()),
                      std::make_move_iterator(weights2.end()));

    // knots of the second spline come after all of the first
    double offset = newKnots.back() - knots2.front();
    knots2.erase(knots2.begin());
    for (auto& knot : knots2)
        knot += offset;
    newKnots.insert(newKnots.end(),
                    std::make_move_iterator(knots2.begin()),
                    std::make_move_iterator(knots2.end()));

    // end knots can have a multiplicity of (degree + 1)
    if (bsp1->getDegree() < newMults.back()) {
        newMults.back() = bsp1->getDegree();
        if (makeC1Continuous) {
            newMults.back() -= 1;
        }
    }

    mults2.erase(mults2.begin());
    newMults.insert(newMults.end(),
                    std::make_move_iterator(mults2.begin()),
                    std::make_move_iterator(mults2.end()));

    Part::GeomBSplineCurve* newSpline = new Part::GeomBSplineCurve(
        newPoles, newWeights, newKnots, newMults, bsp1->getDegree(), false, true);

    int newGeoId = addGeometry(newSpline);

    if (newGeoId < 0) {
        THROWM(ValueError, "Failed to create joined curve.");
        return -1;
    }

    exposeInternalGeometry(newGeoId);
    setConstruction(newGeoId, GeometryFacade::getConstruction(geo1));

    // TODO: transfer constraints on the non-connected ends
    auto otherPosId1 = (Sketcher::PointPos::start == posId1) ? Sketcher::PointPos::end
        : Sketcher::PointPos::start;
    auto otherPosId2 = (Sketcher::PointPos::start == posId2) ? Sketcher::PointPos::end
        : Sketcher::PointPos::start;

    transferConstraints(geoId1, otherPosId1, newGeoId, PointPos::start, true);
    transferConstraints(geoId2, otherPosId2, newGeoId, PointPos::end, true);

    delGeometries({geoId1, geoId2});

    return 0;
}

bool SketchObject::isExternalAllowed(App::Document* pDoc, App::DocumentObject* pObj,
                                     eReasonList* rsn) const
{
    if (rsn)
        *rsn = rlAllowed;

    // Externals outside of the Document are NOT allowed
    if (this->getDocument() != pDoc) {
        if (rsn)
            *rsn = rlOtherDoc;
        return false;
    }

    // circular reference prevention
    try {
        if (!(this->testIfLinkDAGCompatible(pObj))) {
            if (rsn)
                *rsn = rlCircularReference;
            return false;
        }
    }
    catch (Base::Exception& e) {
        Base::Console().warning(
            "Probably, there is a circular reference in the document. Error: %s\n", e.what());
        return true;// prohibiting this reference won't remove the problem anyway...
    }


    // Note: Checking for the body of the support doesn't work when the support are the three base
    // planes
    Part::BodyBase* body_this = Part::BodyBase::findBodyOf(this);
    Part::BodyBase* body_obj = Part::BodyBase::findBodyOf(pObj);
    App::Part* part_this = App::Part::getPartOfObject(this);
    App::Part* part_obj = App::Part::getPartOfObject(pObj);
    if (part_this == part_obj) {// either in the same part, or in the root of document
        if (!body_this) {
            return true;
        }
        else if (body_this == body_obj) {
            return true;
        }
        else {
            if (rsn)
                *rsn = rlOtherBody;
            return false;
        }
    }
    else {
        // cross-part link. Disallow, should be done via shapebinders only
        if (rsn)
            *rsn = rlOtherPart;
        return false;
    }
}

bool SketchObject::isCarbonCopyAllowed(App::Document* pDoc, App::DocumentObject* pObj, bool& xinv,
                                       bool& yinv, eReasonList* rsn) const
{
    if (rsn) {
        *rsn = rlAllowed;
    }

    std::string sketchArchType ("Sketcher::SketchObjectPython");

    // Only applicable to sketches
    if (!pObj->is<Sketcher::SketchObject>()
        && sketchArchType != pObj->getTypeId().getName()) {
        if (rsn) {
            *rsn = rlNotASketch;
        }
        return false;
    }


    auto* psObj = static_cast<SketchObject*>(pObj);

    // Sketches outside of the Document are NOT allowed
    if (this->getDocument() != pDoc) {
        if (rsn) {
            *rsn = rlOtherDoc;
        }
        return false;
    }

    // circular reference prevention
    try {
        if (!(this->testIfLinkDAGCompatible(pObj))) {
            if (rsn) {
                *rsn = rlCircularReference;
            }
            return false;
        }
    }
    catch (Base::Exception& e) {
        Base::Console().warning(
            "Probably, there is a circular reference in the document. Error: %s\n", e.what());
        return true;// prohibiting this reference won't remove the problem anyway...
    }


    // Note: Checking for the body of the support doesn't work when the support are the three base
    // planes
    Part::BodyBase* body_this = Part::BodyBase::findBodyOf(this);
    Part::BodyBase* body_obj = Part::BodyBase::findBodyOf(pObj);
    App::Part* part_this = App::Part::getPartOfObject(this);
    App::Part* part_obj = App::Part::getPartOfObject(pObj);
    if (part_this == part_obj) {// either in the same part, or in the root of document
        if (body_this) {
            if (body_this != body_obj) {
                if (!this->allowOtherBody) {
                    if (rsn)
                        *rsn = rlOtherBody;
                    return false;
                }
                // if the original sketch has external geometry AND it is not in this body prevent
                // link
                else if (psObj->getExternalGeometryCount() > 2) {
                    if (rsn)
                        *rsn = rlOtherBodyWithLinks;
                    return false;
                }
            }
        }
    }
    else {
        // cross-part relation. Disallow, should be done via shapebinders only
        if (rsn)
            *rsn = rlOtherPart;
        return false;
    }


    const Rotation& srot = psObj->Placement.getValue().getRotation();
    const Rotation& lrot = this->Placement.getValue().getRotation();

    Base::Vector3d snormal(0, 0, 1);
    Base::Vector3d sx(1, 0, 0);
    Base::Vector3d sy(0, 1, 0);
    srot.multVec(snormal, snormal);
    srot.multVec(sx, sx);
    srot.multVec(sy, sy);

    Base::Vector3d lnormal(0, 0, 1);
    Base::Vector3d lx(1, 0, 0);
    Base::Vector3d ly(0, 1, 0);
    lrot.multVec(lnormal, lnormal);
    lrot.multVec(lx, lx);
    lrot.multVec(ly, ly);

    double dot = snormal * lnormal;
    double dotx = sx * lx;
    double doty = sy * ly;

    // the planes of the sketches must be parallel
    if (!allowUnaligned && fabs(fabs(dot) - 1) > Precision::Confusion()) {
        if (rsn)
            *rsn = rlNonParallel;
        return false;
    }

    // the axis must be aligned
    if (!allowUnaligned
        && ((fabs(fabs(dotx) - 1) > Precision::Confusion())
            || (fabs(fabs(doty) - 1) > Precision::Confusion()))) {
        if (rsn)
            *rsn = rlAxesMisaligned;
        return false;
    }


    // the origins of the sketches must be aligned or be the same
    Base::Vector3d ddir =
        (psObj->Placement.getValue().getPosition() - this->Placement.getValue().getPosition())
            .Normalize();

    double alignment = ddir * lnormal;

    if (!allowUnaligned && (fabs(fabs(alignment) - 1) > Precision::Confusion())
        && (psObj->Placement.getValue().getPosition()
            != this->Placement.getValue().getPosition())) {
        if (rsn)
            *rsn = rlOriginsMisaligned;
        return false;
    }

    xinv = allowUnaligned ? false : (fabs(dotx - 1) > Precision::Confusion());
    yinv = allowUnaligned ? false : (fabs(doty - 1) > Precision::Confusion());

    return true;
}

int SketchObject::addSymmetric(const std::vector<int>& geoIdList, int refGeoId,
                               Sketcher::PointPos refPosId , bool addSymmetryConstraints )
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& constrvals = this->Constraints.getValues();
    std::vector<Constraint*> newconstrVals(constrvals);

    std::map<int, int> geoIdMap;
    std::map<int, bool> isStartEndInverted;

    // Find out if reference is aligned with V or H axis,
    // if so we can keep Vertical and Horizontal constraints in the mirrored geometry.
    bool refIsLine = refPosId == Sketcher::PointPos::none;
    bool refIsAxisAligned = false;
    if (refGeoId == Sketcher::GeoEnum::VAxis || refGeoId == Sketcher::GeoEnum::HAxis || !refIsLine) {
        refIsAxisAligned = true;
    }
    else {
        for (auto* constr : constrvals) {
            if (constr->First == refGeoId
                && (constr->Type == Sketcher::Vertical || constr->Type == Sketcher::Horizontal)){
                refIsAxisAligned = true;
            }
        }
    }

    std::vector<Part::Geometry*> symgeos = getSymmetric(geoIdList, geoIdMap, isStartEndInverted, refGeoId, refPosId);

    {
        addGeometry(symgeos);

        for (auto* constr :  constrvals) {
            // we look in the map, because we might have skipped internal alignment geometry
            auto fit = geoIdMap.find(constr->First);

            if (fit != geoIdMap.end()) {// if First of constraint is in geoIdList
                if (addSymmetryConstraints && constr->Type != Sketcher::InternalAlignment) {
                    // if we are making symmetric constraints, then we don't want to copy all constraints
                    continue;
                }

                if (constr->Second == GeoEnum::GeoUndef ){
                    if (refIsAxisAligned) {
                        // in this case we want to keep the Vertical, Horizontal constraints
                        // DistanceX ,and DistanceY constraints should also be possible to keep in
                        // this case, but keeping them causes segfault, not sure why.

                        if (constr->Type != Sketcher::DistanceX
                            && constr->Type != Sketcher::DistanceY) {
                            Constraint* constNew = constr->copy();
                            constNew->Name = ""; // Make sure we don't have 2 constraint with same name.
                            constNew->First = fit->second;
                            newconstrVals.push_back(constNew);
                        }
                    }
                    else if (constr->Type != Sketcher::DistanceX
                                && constr->Type != Sketcher::DistanceY
                                && constr->Type != Sketcher::Vertical
                                && constr->Type != Sketcher::Horizontal) {
                        // this includes all non-directional single GeoId constraints, as radius,
                        // diameter, weight,...

                        Constraint* constNew = constr->copy();
                        constNew->Name = "";
                        constNew->First = fit->second;
                        newconstrVals.push_back(constNew);
                    }
                }
                else {// other geoids intervene in this constraint

                    auto sit = geoIdMap.find(constr->Second);

                    if (sit != geoIdMap.end()) {// Second is also in the list

                        if (constr->Third == GeoEnum::GeoUndef) {
                            if (constr->Type == Sketcher::Coincident
                                || constr->Type == Sketcher::Perpendicular
                                || constr->Type == Sketcher::Parallel
                                || constr->Type == Sketcher::Tangent
                                || constr->Type == Sketcher::Distance
                                || constr->Type == Sketcher::Equal || constr->Type == Sketcher::Angle
                                || constr->Type == Sketcher::PointOnObject
                                || constr->Type == Sketcher::InternalAlignment) {
                                Constraint* constNew = constr->copy();
                                constNew->Name = "";
                                constNew->First = fit->second;
                                constNew->Second = sit->second;
                                if (isStartEndInverted[constr->First]) {
                                    if (constr->FirstPos == Sketcher::PointPos::start)
                                        constNew->FirstPos = Sketcher::PointPos::end;
                                    else if (constr->FirstPos == Sketcher::PointPos::end)
                                        constNew->FirstPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[constr->Second]) {
                                    if (constr->SecondPos == Sketcher::PointPos::start)
                                        constNew->SecondPos = Sketcher::PointPos::end;
                                    else if (constr->SecondPos == Sketcher::PointPos::end)
                                        constNew->SecondPos = Sketcher::PointPos::start;
                                }

                                if (constNew->Type == Tangent || constNew->Type == Perpendicular)
                                    AutoLockTangencyAndPerpty(constNew, true);

                                if ((constr->Type == Sketcher::Angle)
                                    && (refPosId == Sketcher::PointPos::none)) {
                                    constNew->setValue(-constr->getValue());
                                }

                                newconstrVals.push_back(constNew);
                            }
                        }
                        else {// three GeoIds intervene in constraint
                            auto tit = geoIdMap.find(constr->Third);

                            if (tit != geoIdMap.end()) {// Third is also in the list
                                Constraint* constNew = constr->copy();
                                constNew->Name = "";
                                constNew->First = fit->second;
                                constNew->Second = sit->second;
                                constNew->Third = tit->second;
                                if (isStartEndInverted[constr->First]) {
                                    if (constr->FirstPos == Sketcher::PointPos::start)
                                        constNew->FirstPos = Sketcher::PointPos::end;
                                    else if (constr->FirstPos == Sketcher::PointPos::end)
                                        constNew->FirstPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[constr->Second]) {
                                    if (constr->SecondPos == Sketcher::PointPos::start)
                                        constNew->SecondPos = Sketcher::PointPos::end;
                                    else if (constr->SecondPos == Sketcher::PointPos::end)
                                        constNew->SecondPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[constr->Third]) {
                                    if (constr->ThirdPos == Sketcher::PointPos::start)
                                        constNew->ThirdPos = Sketcher::PointPos::end;
                                    else if (constr->ThirdPos == Sketcher::PointPos::end)
                                        constNew->ThirdPos = Sketcher::PointPos::start;
                                }
                                newconstrVals.push_back(constNew);
                            }
                        }
                    }
                }
            }
        }

        if (addSymmetryConstraints) {
            auto createSymConstr = [&]
            (int first, int second, Sketcher::PointPos firstPos, Sketcher::PointPos secondPos) {
                auto symConstr = new Constraint();
                symConstr->Type = Symmetric;
                symConstr->First = first;
                symConstr->Second = second;
                symConstr->Third = refGeoId;
                symConstr->FirstPos = firstPos;
                symConstr->SecondPos = secondPos;
                symConstr->ThirdPos = refPosId;
                newconstrVals.push_back(symConstr);
            };
            auto createEqualityConstr = [&]
            (int first, int second) {
                auto symConstr = new Constraint();
                symConstr->Type = Equal;
                symConstr->First = first;
                symConstr->Second = second;
                newconstrVals.push_back(symConstr);
            };

            for (auto geoIdPair : geoIdMap) {
                int geoId1 = geoIdPair.first;
                int geoId2 = geoIdPair.second;
                const Part::Geometry* geo = getGeometry(geoId1);

                if (geo->is<Part::GeomLineSegment>()) {
                    auto gf = GeometryFacade::getFacade(geo);
                    if (!gf->isInternalAligned()) {
                        // Note internal aligned lines (ellipse, parabola, hyperbola) are causing redundant constraint.
                        createSymConstr(geoId1, geoId2, PointPos::start, isStartEndInverted[geoId1] ? PointPos::end : PointPos::start);
                        createSymConstr(geoId1, geoId2, PointPos::end, isStartEndInverted[geoId1] ? PointPos::start : PointPos::end);
                    }
                }
                else if (geo->is<Part::GeomCircle>() || geo->is<Part::GeomEllipse>()) {
                    createEqualityConstr(geoId1, geoId2);
                    createSymConstr(geoId1, geoId2, PointPos::mid, PointPos::mid);
                }
                else if (geo->is<Part::GeomArcOfCircle>()
                    || geo->is<Part::GeomArcOfEllipse>()
                    || geo->is<Part::GeomArcOfHyperbola>()
                    || geo->is<Part::GeomArcOfParabola>()) {
                    createEqualityConstr(geoId1, geoId2);
                    createSymConstr(geoId1, geoId2, PointPos::start, isStartEndInverted[geoId1] ? PointPos::end : PointPos::start);
                    createSymConstr(geoId1, geoId2, PointPos::end, isStartEndInverted[geoId1] ? PointPos::start : PointPos::end);
                }
                else if (geo->is<Part::GeomPoint>()) {
                    auto gf = GeometryFacade::getFacade(geo);
                    if (!gf->isInternalAligned()) {
                        createSymConstr(geoId1, geoId2, PointPos::start, PointPos::start);
                    }
                }
                // Note bspline has symmetric by the internal aligned circles.
            }
        }

        if (newconstrVals.size() > constrvals.size()){
            Constraints.setValues(std::move(newconstrVals));
        }
    }

    // we delayed update, so trigger it now.
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    return Geometry.getSize() - 1;
}

std::vector<Part::Geometry*> SketchObject::getSymmetric(const std::vector<int>& geoIdList,
    std::map<int, int>& geoIdMap,
    std::map<int, bool>& isStartEndInverted,
    int refGeoId,
    Sketcher::PointPos refPosId)
{
    using std::numbers::pi;

    std::vector<Part::Geometry*> symmetricVals;
    bool refIsLine = refPosId == Sketcher::PointPos::none;
    int cgeoid = getHighestCurveIndex() + 1;

    auto shouldCopyGeometry = [&](auto* geo, int geoId) -> bool {
        auto gf = GeometryFacade::getFacade(geo);
        if (gf->isInternalAligned()) {
            // only add if the corresponding geometry it defines is also in the list.
            int definedGeo = GeoEnum::GeoUndef;
            for (auto c : Constraints.getValues()) {
                if (c->Type == Sketcher::InternalAlignment && c->First == geoId) {
                    definedGeo = c->Second;
                    break;
                }
            }
            // Return true if definedGeo is in geoIdList, false otherwise
            return std::ranges::find(geoIdList, definedGeo) != geoIdList.end();
        }
        // Return true if not internal aligned, indicating it should always be copied
        return true;
    };

    if (refIsLine) {
        const Part::Geometry* georef = getGeometry(refGeoId);
        if (!georef->is<Part::GeomLineSegment>()) {
            return {};
        }

        auto* refGeoLine = static_cast<const Part::GeomLineSegment*>(georef);
        // line
        Base::Vector3d refstart = refGeoLine->getStartPoint();
        Base::Vector3d vectline = refGeoLine->getEndPoint() - refstart;

        for (auto geoId : geoIdList) {
            const Part::Geometry* geo = getGeometry(geoId);
            Part::Geometry* geosym;

            if (!shouldCopyGeometry(geo, geoId)) {
                continue;
            }

            geosym = geo->copy();

            // Handle Geometry
            if (geosym->is<Part::GeomLineSegment>()) {
                auto* geosymline = static_cast<Part::GeomLineSegment*>(geosym);
                Base::Vector3d sp = geosymline->getStartPoint();
                Base::Vector3d ep = geosymline->getEndPoint();

                geosymline->setPoints(
                    sp + 2.0 * (sp.Perpendicular(refGeoLine->getStartPoint(), vectline) - sp),
                    ep + 2.0 * (ep.Perpendicular(refGeoLine->getStartPoint(), vectline) - ep));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomCircle>()) {
                auto* geosymcircle = static_cast<Part::GeomCircle*>(geosym);
                Base::Vector3d cp = geosymcircle->getCenter();

                geosymcircle->setCenter(
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfCircle>()) {
                auto* geoaoc = static_cast<Part::GeomArcOfCircle*>(geosym);
                Base::Vector3d sp = geoaoc->getStartPoint(true);
                Base::Vector3d ep = geoaoc->getEndPoint(true);
                Base::Vector3d cp = geoaoc->getCenter();

                Base::Vector3d ssp =
                    sp + 2.0 * (sp.Perpendicular(refGeoLine->getStartPoint(), vectline) - sp);
                Base::Vector3d sep =
                    ep + 2.0 * (ep.Perpendicular(refGeoLine->getStartPoint(), vectline) - ep);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                double theta1 = Base::fmod(atan2(sep.y - scp.y, sep.x - scp.x), 2.f * std::numbers::pi);
                double theta2 = Base::fmod(atan2(ssp.y - scp.y, ssp.x - scp.x), 2.f * std::numbers::pi);

                geoaoc->setCenter(scp);
                geoaoc->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, true));
            }
            else if (geosym->is<Part::GeomEllipse>()) {
                auto* geosymellipse = static_cast<Part::GeomEllipse*>(geosym);
                Base::Vector3d cp = geosymellipse->getCenter();

                Base::Vector3d majdir = geosymellipse->getMajorAxisDir();
                double majord = geosymellipse->getMajorRadius();
                double minord = geosymellipse->getMinorRadius();
                double df = sqrt(majord * majord - minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 =
                    f1 + 2.0 * (f1.Perpendicular(refGeoLine->getStartPoint(), vectline) - f1);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                geosymellipse->setMajorAxisDir(sf1 - scp);

                geosymellipse->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfEllipse>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfEllipse*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord = geosymaoe->getMajorRadius();
                double minord = geosymaoe->getMinorRadius();
                double df = sqrt(majord * majord - minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 =
                    f1 + 2.0 * (f1.Perpendicular(refGeoLine->getStartPoint(), vectline) - f1);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                geosymaoe->setMajorAxisDir(sf1 - scp);

                geosymaoe->setCenter(scp);

                double theta1, theta2;
                geosymaoe->getRange(theta1, theta2, true);
                theta1 = 2.0 * pi - theta1;
                theta2 = 2.0 * pi - theta2;
                std::swap(theta1, theta2);
                if (theta1 < 0) {
                    theta1 += 2.0 * pi;
                    theta2 += 2.0 * pi;
                }

                geosymaoe->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, true));
            }
            else if (geosym->is<Part::GeomArcOfHyperbola>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfHyperbola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord = geosymaoe->getMajorRadius();
                double minord = geosymaoe->getMinorRadius();
                double df = sqrt(majord * majord + minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 =
                    f1 + 2.0 * (f1.Perpendicular(refGeoLine->getStartPoint(), vectline) - f1);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                geosymaoe->setMajorAxisDir(sf1 - scp);

                geosymaoe->setCenter(scp);

                double theta1, theta2;
                geosymaoe->getRange(theta1, theta2, true);
                theta1 = -theta1;
                theta2 = -theta2;
                std::swap(theta1, theta2);

                geosymaoe->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, true));
            }
            else if (geosym->is<Part::GeomArcOfParabola>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfParabola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d f1 = geosymaoe->getFocus();

                Base::Vector3d sf1 =
                    f1 + 2.0 * (f1.Perpendicular(refGeoLine->getStartPoint(), vectline) - f1);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                geosymaoe->setXAxisDir(sf1 - scp);
                geosymaoe->setCenter(scp);

                double theta1, theta2;
                geosymaoe->getRange(theta1, theta2, true);
                theta1 = -theta1;
                theta2 = -theta2;
                std::swap(theta1, theta2);

                geosymaoe->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, true));
            }
            else if (geosym->is<Part::GeomBSplineCurve>()) {
                auto* geosymbsp = static_cast<Part::GeomBSplineCurve*>(geosym);

                std::vector<Base::Vector3d> poles = geosymbsp->getPoles();

                for (auto& pole : poles) {
                    pole = pole
                        + 2.0 * (pole.Perpendicular(refGeoLine->getStartPoint(), vectline) - pole);
                }

                geosymbsp->setPoles(poles);

                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomPoint>()) {
                auto* geosympoint = static_cast<Part::GeomPoint*>(geosym);
                Base::Vector3d cp = geosympoint->getPoint();

                geosympoint->setPoint(
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else {
                Base::Console().error("Unsupported Geometry!! Just copying it.\n");
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }

            symmetricVals.push_back(geosym);
            geoIdMap.insert(std::make_pair(geoId, cgeoid));
            cgeoid++;
        }
    }
    else {// reference is a point
        Vector3d refpoint;
        const Part::Geometry* georef = getGeometry(refGeoId);

        if (georef->is<Part::GeomPoint>()) {
            refpoint = static_cast<const Part::GeomPoint*>(georef)->getPoint();
        }
        else if (refGeoId == -1 && refPosId == Sketcher::PointPos::start) {
            refpoint = Vector3d(0, 0, 0);
        }
        else {
            if (refPosId == Sketcher::PointPos::none) {
                Base::Console().error("Wrong PointPosId.\n");
                return {};
            }
            refpoint = getPoint(georef, refPosId);
        }

        for (auto geoId : geoIdList) {
            const Part::Geometry* geo = getGeometry(geoId);
            Part::Geometry* geosym;

            if (!shouldCopyGeometry(geo, geoId)) {
                continue;
            }

            geosym = geo->copy();

            // Handle Geometry
            if (geosym->is<Part::GeomLineSegment>()) {
                auto* geosymline = static_cast<Part::GeomLineSegment*>(geosym);
                Base::Vector3d sp = geosymline->getStartPoint();
                Base::Vector3d ep = geosymline->getEndPoint();
                Base::Vector3d ssp = sp + 2.0 * (refpoint - sp);
                Base::Vector3d sep = ep + 2.0 * (refpoint - ep);

                geosymline->setPoints(ssp, sep);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomCircle>()) {
                auto* geosymcircle = static_cast<Part::GeomCircle*>(geosym);
                Base::Vector3d cp = geosymcircle->getCenter();

                geosymcircle->setCenter(cp + 2.0 * (refpoint - cp));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfCircle>()) {
                auto* geoaoc = static_cast<Part::GeomArcOfCircle*>(geosym);
                Base::Vector3d sp = geoaoc->getStartPoint(true);
                Base::Vector3d ep = geoaoc->getEndPoint(true);
                Base::Vector3d cp = geoaoc->getCenter();

                Base::Vector3d ssp = sp + 2.0 * (refpoint - sp);
                Base::Vector3d sep = ep + 2.0 * (refpoint - ep);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                double theta1 = Base::fmod(atan2(ssp.y - scp.y, ssp.x - scp.x), 2.f * pi);
                double theta2 = Base::fmod(atan2(sep.y - scp.y, sep.x - scp.x), 2.f * pi);

                geoaoc->setCenter(scp);
                geoaoc->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomEllipse>()) {
                auto* geosymellipse = static_cast<Part::GeomEllipse*>(geosym);
                Base::Vector3d cp = geosymellipse->getCenter();

                Base::Vector3d majdir = geosymellipse->getMajorAxisDir();
                double majord = geosymellipse->getMajorRadius();
                double minord = geosymellipse->getMinorRadius();
                double df = sqrt(majord * majord - minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymellipse->setMajorAxisDir(sf1 - scp);

                geosymellipse->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfEllipse>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfEllipse*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord = geosymaoe->getMajorRadius();
                double minord = geosymaoe->getMinorRadius();
                double df = sqrt(majord * majord - minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymaoe->setMajorAxisDir(sf1 - scp);

                geosymaoe->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfHyperbola>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfHyperbola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord = geosymaoe->getMajorRadius();
                double minord = geosymaoe->getMinorRadius();
                double df = sqrt(majord * majord + minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymaoe->setMajorAxisDir(sf1 - scp);

                geosymaoe->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfParabola>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfParabola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();
                Base::Vector3d f1 = geosymaoe->getFocus();

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymaoe->setXAxisDir(sf1 - scp);
                geosymaoe->setCenter(scp);

                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomBSplineCurve>()) {
                auto* geosymbsp = static_cast<Part::GeomBSplineCurve*>(geosym);

                std::vector<Base::Vector3d> poles = geosymbsp->getPoles();

                for (auto& pole : poles) {
                    pole = pole + 2.0 * (refpoint - pole);
                }

                geosymbsp->setPoles(poles);

            }
            else if (geosym->is<Part::GeomPoint>()) {
                auto* geosympoint = static_cast<Part::GeomPoint*>(geosym);
                Base::Vector3d cp = geosympoint->getPoint();

                geosympoint->setPoint(cp + 2.0 * (refpoint - cp));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else {
                Base::Console().error("Unsupported Geometry!! Just copying it.\n");
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }

            symmetricVals.push_back(geosym);
            geoIdMap.insert(std::make_pair(geoId, cgeoid));
            cgeoid++;
        }
    }
    return symmetricVals;
}

int SketchObject::addCopy(const std::vector<int>& geoIdList, const Base::Vector3d& displacement,
                          bool moveonly , bool clone , int csize , int rsize , bool constraindisplacement ,
                          double perpscale )
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& geovals = getInternalGeometry();
    std::vector<Part::Geometry*> newgeoVals(geovals);

    const std::vector<Constraint*>& constrvals = this->Constraints.getValues();
    std::vector<Constraint*> newconstrVals(constrvals);

    if (!moveonly) {
        newgeoVals.reserve(geovals.size() + geoIdList.size());
    }

    std::vector<int> newgeoIdList(geoIdList);

    if (newgeoIdList.empty()) {// default option to operate on all the geometry
        for (int i = 0; i < int(geovals.size()); i++)
            newgeoIdList.push_back(i);
    }

    int cgeoid = getHighestCurveIndex() + 1;

    int iterfirstgeoid = -1;

    Base::Vector3d iterfirstpoint;

    int refgeoid = -1;

    int colrefgeoid = 0, rowrefgeoid = 0;

    int currentrowfirstgeoid = -1, prevrowstartfirstgeoid = -1, prevfirstgeoid = -1;

    Sketcher::PointPos refposId = Sketcher::PointPos::none;

    std::map<int, int> geoIdMap;

    Base::Vector3d perpendicularDisplacement =
        Base::Vector3d(perpscale * displacement.y, perpscale * -displacement.x, 0);

    int x, y;

    for (y = 0; y < rsize; y++) {
        for (x = 0; x < csize; x++) {
            // the reference for constraining array elements is the first valid point of the first
            // element
            if (x == 0 && y == 0) {
                const Part::Geometry* geo = getGeometry(*(newgeoIdList.begin()));

                auto gf = GeometryFacade::getFacade(geo);

                if (gf->isInternalAligned() && !moveonly) {
                    // only add this geometry if the corresponding geometry it defines is also in
                    // the list.
                    int definedGeo = GeoEnum::GeoUndef;

                    for (auto c : Constraints.getValues()) {
                        if (c->Type == Sketcher::InternalAlignment
                            && c->First == *(newgeoIdList.begin())) {
                            definedGeo = c->Second;
                            break;
                        }
                    }

                    if (std::ranges::find(newgeoIdList, definedGeo) == newgeoIdList.end()) {
                        // the first element setting the reference is an internal alignment
                        // geometry, wherein the geometry it defines is not part of the copy
                        // operation.
                        THROWM(Base::ValueError,
                               "A move/copy/array operation on an internal alignment geometry is "
                               "only possible together with the geometry it defines.")
                    }
                }

                refgeoid = *(newgeoIdList.begin());
                currentrowfirstgeoid = refgeoid;
                iterfirstgeoid = refgeoid;
                if (geo->is<Part::GeomCircle>()
                    || geo->is<Part::GeomEllipse>()) {
                    refposId = Sketcher::PointPos::mid;
                }
                else
                    refposId = Sketcher::PointPos::start;

                continue;// the first element is already in place
            }
            else {
                prevfirstgeoid = iterfirstgeoid;

                iterfirstgeoid = cgeoid;

                if (x == 0) {// if first element of second row
                    prevrowstartfirstgeoid = currentrowfirstgeoid;
                    currentrowfirstgeoid = cgeoid;
                }
            }

            int index = 0;
            for (std::vector<int>::const_iterator it = newgeoIdList.begin();
                 it != newgeoIdList.end();
                 ++it, index++) {
                const Part::Geometry* geo = getGeometry(*it);

                Part::Geometry* geocopy;

                auto gf = GeometryFacade::getFacade(geo);

                if (gf->isInternalAligned() && !moveonly) {
                    // only add this geometry if the corresponding geometry it defines is also in
                    // the list.
                    int definedGeo = GeoEnum::GeoUndef;

                    for (auto c : Constraints.getValues()) {
                        if (c->Type == Sketcher::InternalAlignment && c->First == *it) {
                            definedGeo = c->Second;
                            break;
                        }
                    }

                    if (std::ranges::find(newgeoIdList, definedGeo)
                        == newgeoIdList.end()) {
                        // we should not copy internal alignment geometry, unless the element they
                        // define is also mirrored
                        continue;
                    }
                }

                // We have already cloned all geometry and constraints, we only need a copy if not
                // moving
                if (!moveonly) {
                    geocopy = geo->copy();
                    generateId(geocopy);
                } else
                    geocopy = newgeoVals[*it];

                // Handle Geometry
                if (geocopy->is<Part::GeomLineSegment>()) {
                    Part::GeomLineSegment* geosymline =
                        static_cast<Part::GeomLineSegment*>(geocopy);
                    Base::Vector3d ep = geosymline->getEndPoint();
                    Base::Vector3d ssp = geosymline->getStartPoint() + double(x) * displacement
                        + double(y) * perpendicularDisplacement;

                    geosymline->setPoints(
                        ssp, ep + double(x) * displacement + double(y) * perpendicularDisplacement);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = ssp;
                }
                else if (geocopy->is<Part::GeomCircle>()) {
                    auto* geosymcircle = static_cast<Part::GeomCircle*>(geocopy);
                    Base::Vector3d cp = geosymcircle->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geosymcircle->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else if (geocopy->is<Part::GeomArcOfCircle>()) {
                    auto* geoaoc = static_cast<Part::GeomArcOfCircle*>(geocopy);
                    Base::Vector3d cp = geoaoc->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoc->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoc->getStartPoint(true);
                }
                else if (geocopy->is<Part::GeomEllipse>()) {
                    auto* geosymellipse = static_cast<Part::GeomEllipse*>(geocopy);
                    Base::Vector3d cp = geosymellipse->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geosymellipse->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else if (geocopy->is<Part::GeomArcOfEllipse>()) {
                    auto* geoaoe = static_cast<Part::GeomArcOfEllipse*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->is<Part::GeomArcOfHyperbola>()) {
                    Part::GeomArcOfHyperbola* geoaoe =
                        static_cast<Part::GeomArcOfHyperbola*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->is<Part::GeomArcOfParabola>()) {
                    Part::GeomArcOfParabola* geoaoe =
                        static_cast<Part::GeomArcOfParabola*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->is<Part::GeomBSplineCurve>()) {
                    auto* geobsp = static_cast<Part::GeomBSplineCurve*>(geocopy);

                    std::vector<Base::Vector3d> poles = geobsp->getPoles();

                    for (std::vector<Base::Vector3d>::iterator jt = poles.begin();
                         jt != poles.end();
                         ++jt) {

                        (*jt) = (*jt) + double(x) * displacement
                            + double(y) * perpendicularDisplacement;
                    }

                    geobsp->setPoles(poles);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geobsp->getStartPoint();
                }
                else if (geocopy->is<Part::GeomPoint>()) {
                    auto* geopoint = static_cast<Part::GeomPoint*>(geocopy);
                    Base::Vector3d cp = geopoint->getPoint();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;
                    geopoint->setPoint(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else {
                    Base::Console().error("Unsupported Geometry!! Just skipping it.\n");
                    continue;
                }

                if (!moveonly) {// we are copying
                    newgeoVals.push_back(geocopy);
                    geoIdMap.insert(std::make_pair(*it, cgeoid));
                    cgeoid++;
                }
            }

            if (!moveonly) {
                // handle geometry constraints
                for (std::vector<Constraint*>::const_iterator it = constrvals.begin();
                     it != constrvals.end();
                     ++it) {

                    auto fit = geoIdMap.find((*it)->First);

                    if (fit != geoIdMap.end()) {// if First of constraint is in geoIdList

                        if ((*it)->Second
                            == GeoEnum::GeoUndef /*&& (*it)->Third == GeoEnum::GeoUndef*/) {
                            if (((*it)->Type != Sketcher::DistanceX
                                 && (*it)->Type != Sketcher::DistanceY)
                                || (*it)->FirstPos == Sketcher::PointPos::none) {
                                // if it is not a point locking DistanceX/Y
                                if (((*it)->Type == Sketcher::DistanceX
                                     || (*it)->Type == Sketcher::DistanceY
                                     || (*it)->Type == Sketcher::Distance
                                     || (*it)->Type == Sketcher::Diameter
                                     || (*it)->Type == Sketcher::Weight
                                     || (*it)->Type == Sketcher::Radius)
                                    && clone) {
                                    // Distances on a single Element are mapped to equality
                                    // constraints in clone mode
                                    Constraint* constNew = (*it)->copy();
                                    constNew->Type = Sketcher::Equal;
                                    constNew->isDriving = true;
                                    // first is already (*it->First)
                                    constNew->Second = fit->second;
                                    newconstrVals.push_back(constNew);
                                }
                                else if ((*it)->Type == Sketcher::Angle && clone) {
                                    if (getGeometry((*it)->First)->is<Part::GeomLineSegment>()) {
                                        // Angles on a single Element are mapped to parallel
                                        // constraints in clone mode
                                        Constraint* constNew = (*it)->copy();
                                        constNew->Type = Sketcher::Parallel;
                                        constNew->isDriving = true;
                                        // first is already (*it->First)
                                        constNew->Second = fit->second;
                                        newconstrVals.push_back(constNew);
                                    }
                                }
                                else {
                                    Constraint* constNew = (*it)->copy();
                                    constNew->First = fit->second;
                                    newconstrVals.push_back(constNew);
                                }
                            }
                        }
                        else {// other geoids intervene in this constraint

                            auto sit = geoIdMap.find((*it)->Second);

                            if (sit != geoIdMap.end()) {// Second is also in the list
                                if ((*it)->Third == GeoEnum::GeoUndef) {
                                    if (((*it)->Type == Sketcher::DistanceX
                                         || (*it)->Type == Sketcher::DistanceY
                                         || (*it)->Type == Sketcher::Distance)
                                        && ((*it)->First == (*it)->Second) && clone) {
                                        // Distances on a two Elements, which must be points of the
                                        // same line are mapped to equality constraints in clone
                                        // mode
                                        Constraint* constNew = (*it)->copy();
                                        constNew->Type = Sketcher::Equal;
                                        constNew->isDriving = true;
                                        constNew->FirstPos = Sketcher::PointPos::none;
                                        // first is already (*it->First)
                                        constNew->Second = fit->second;
                                        constNew->SecondPos = Sketcher::PointPos::none;
                                        newconstrVals.push_back(constNew);
                                    }
                                    else {// this includes InternalAlignment constraints
                                        Constraint* constNew = (*it)->copy();
                                        constNew->First = fit->second;
                                        constNew->Second = sit->second;
                                        newconstrVals.push_back(constNew);
                                    }
                                }
                                else {
                                    auto tit = geoIdMap.find((*it)->Third);

                                    if (tit != geoIdMap.end()) {// Third is also in the list
                                        Constraint* constNew = (*it)->copy();
                                        constNew->First = fit->second;
                                        constNew->Second = sit->second;
                                        constNew->Third = tit->second;

                                        newconstrVals.push_back(constNew);
                                    }
                                }
                            }
                        }
                    }
                }

                // handle inter-geometry constraints
                if (constraindisplacement) {

                    // add a construction line
                    Part::GeomLineSegment* constrline = new Part::GeomLineSegment();

                    // position of the reference point
                    Base::Vector3d sp = getPoint(refgeoid, refposId)
                        + ((x == 0) ? (double(x) * displacement
                                       + double(y - 1) * perpendicularDisplacement)
                                    : (double(x - 1) * displacement
                                       + double(y) * perpendicularDisplacement));

                    // position of the current instance corresponding point
                    Base::Vector3d ep = iterfirstpoint;
                    constrline->setPoints(sp, ep);
                    GeometryFacade::setConstruction(constrline, true);

                    generateId(constrline);
                    newgeoVals.push_back(constrline);

                    Constraint* constNew;

                    if (x == 0) {// first element of a row

                        // add coincidents for construction line
                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = prevrowstartfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::PointPos::start;
                        newconstrVals.push_back(constNew);

                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = iterfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::PointPos::end;
                        newconstrVals.push_back(constNew);

                        // it is the first added element of this row in the perpendicular to
                        // displacementvector direction
                        if (y == 1) {
                            rowrefgeoid = cgeoid;
                            cgeoid++;

                            // add length (or equal if perpscale==1) and perpendicular
                            if (perpscale == 1.0) {
                                constNew = new Constraint();
                                constNew->Type = Sketcher::Equal;
                                constNew->First = rowrefgeoid;
                                constNew->FirstPos = Sketcher::PointPos::none;
                                constNew->Second = colrefgeoid;
                                constNew->SecondPos = Sketcher::PointPos::none;
                                newconstrVals.push_back(constNew);
                            }
                            else {
                                constNew = new Constraint();
                                constNew->Type = Sketcher::Distance;
                                constNew->First = rowrefgeoid;
                                constNew->FirstPos = Sketcher::PointPos::none;
                                constNew->setValue(perpendicularDisplacement.Length());
                                newconstrVals.push_back(constNew);
                            }

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Perpendicular;
                            constNew->First = rowrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);
                        }
                        else {// it is just one more element in the col direction
                            cgeoid++;

                            // all other first rowers get an equality and perpendicular constraint
                            constNew = new Constraint();
                            constNew->Type = Sketcher::Equal;
                            constNew->First = rowrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = cgeoid - 1;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Perpendicular;
                            constNew->First = cgeoid - 1;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);
                        }
                    }
                    else {// any element not being the first element of a row

                        // add coincidents for construction line
                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = prevfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::PointPos::start;
                        newconstrVals.push_back(constNew);

                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = iterfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::PointPos::end;
                        newconstrVals.push_back(constNew);

                        if (y == 0 && x == 1) {// first element of the first row
                            colrefgeoid = cgeoid;
                            cgeoid++;

                            // add length and Angle
                            constNew = new Constraint();
                            constNew->Type = Sketcher::Distance;
                            constNew->First = colrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->setValue(displacement.Length());
                            newconstrVals.push_back(constNew);

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Angle;
                            constNew->First = colrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->setValue(atan2(displacement.y, displacement.x));
                            newconstrVals.push_back(constNew);
                        }
                        else {// any other element
                            cgeoid++;

                            // all other elements get an equality and parallel constraint
                            constNew = new Constraint();
                            constNew->Type = Sketcher::Equal;
                            constNew->First = colrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = cgeoid - 1;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Parallel;
                            constNew->First = cgeoid - 1;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);
                        }
                    }
                }
                // after each creation reset map so that the key-value is univoque (only for
                // operations other than move)
                geoIdMap.clear();
            }
        }
    }

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(std::move(newgeoVals));

        if (newconstrVals.size() > constrvals.size())
            Constraints.setValues(std::move(newconstrVals));
    }

    // we inhibited update, so we trigger it now
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    return Geometry.getSize() - 1;
}

int SketchObject::removeAxesAlignment(const std::vector<int>& geoIdList)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& constrvals = this->Constraints.getValues();

    unsigned int nhoriz = 0;
    unsigned int nvert = 0;

    bool changed = false;

    std::vector<std::pair<size_t, Sketcher::ConstraintType>> changeConstraintIndices;

    for (size_t i = 0; i < constrvals.size(); i++) {
        for (auto geoid : geoIdList) {
            if (constrvals[i]->First == geoid || constrvals[i]->Second == geoid
                || constrvals[i]->Third == geoid) {
                switch (constrvals[i]->Type) {
                    case Sketcher::Horizontal:
                        if (constrvals[i]->FirstPos == Sketcher::PointPos::none
                            && constrvals[i]->SecondPos == Sketcher::PointPos::none) {
                            changeConstraintIndices.emplace_back(i, constrvals[i]->Type);
                            nhoriz++;
                        }
                        break;
                    case Sketcher::Vertical:
                        if (constrvals[i]->FirstPos == Sketcher::PointPos::none
                            && constrvals[i]->SecondPos == Sketcher::PointPos::none) {
                            changeConstraintIndices.emplace_back(i, constrvals[i]->Type);
                            nvert++;
                        }
                        break;
                    case Sketcher::Symmetric:// only remove symmetric to axes
                        if ((constrvals[i]->Third == GeoEnum::HAxis
                             || constrvals[i]->Third == GeoEnum::VAxis)
                            && constrvals[i]->ThirdPos == Sketcher::PointPos::none)
                            changeConstraintIndices.emplace_back(i, constrvals[i]->Type);
                        break;
                    case Sketcher::PointOnObject:
                        if ((constrvals[i]->Second == GeoEnum::HAxis
                             || constrvals[i]->Second == GeoEnum::VAxis)
                            && constrvals[i]->SecondPos == Sketcher::PointPos::none)
                            changeConstraintIndices.emplace_back(i, constrvals[i]->Type);
                        break;
                    case Sketcher::DistanceX:
                    case Sketcher::DistanceY:
                        changeConstraintIndices.emplace_back(i, constrvals[i]->Type);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (changeConstraintIndices.empty())
        return 0;// nothing to be done

    std::vector<Constraint*> newconstrVals;
    newconstrVals.reserve(constrvals.size());

    int referenceHorizontal = GeoEnum::GeoUndef;
    int referenceVertical = GeoEnum::GeoUndef;

    int cindex = 0;
    for (size_t i = 0; i < constrvals.size(); i++) {
        if (i == changeConstraintIndices[cindex].first) {
            if (changeConstraintIndices[cindex].second == Sketcher::Horizontal && nhoriz > 0) {
                changed = true;
                if (referenceHorizontal == GeoEnum::GeoUndef) {
                    referenceHorizontal = constrvals[i]->First;
                }
                else {

                    auto newConstr = new Constraint();

                    newConstr->Type = Sketcher::Parallel;
                    newConstr->First = referenceHorizontal;
                    newConstr->Second = constrvals[i]->First;

                    newconstrVals.push_back(newConstr);
                }
            }
            else if (changeConstraintIndices[cindex].second == Sketcher::Vertical && nvert > 0) {
                changed = true;
                if (referenceVertical == GeoEnum::GeoUndef) {
                    referenceVertical = constrvals[i]->First;
                    ;
                }
                else {
                    auto newConstr = new Constraint();

                    newConstr->Type = Sketcher::Parallel;
                    newConstr->First = referenceVertical;
                    newConstr->Second = constrvals[i]->First;

                    newconstrVals.push_back(newConstr);
                }
            }
            else if (changeConstraintIndices[cindex].second == Sketcher::Symmetric
                     || changeConstraintIndices[cindex].second == Sketcher::PointOnObject) {
                changed = true;// We remove symmetric on axes
            }
            else if (changeConstraintIndices[cindex].second == Sketcher::DistanceX
                     || changeConstraintIndices[cindex].second == Sketcher::DistanceY) {
                changed = true;// We remove symmetric on axes
                newconstrVals.push_back(constrvals[i]->clone());
                newconstrVals.back()->Type = Sketcher::Distance;
            }

            cindex++;
        }
        else {
            newconstrVals.push_back(constrvals[i]);
        }
    }

    if (nhoriz > 0 && nvert > 0) {
        auto newConstr = new Constraint();

        newConstr->Type = Sketcher::Perpendicular;
        newConstr->First = referenceVertical;
        newConstr->Second = referenceHorizontal;

        newconstrVals.push_back(newConstr);
    }

    if (changed)
        Constraints.setValues(std::move(newconstrVals));

    return 0;
}

template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomEllipse>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);
    // First we search what has to be restored
    bool major = false;
    bool minor = false;
    bool focus1 = false;
    bool focus2 = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (const auto& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::EllipseMajorDiameter:
            major = true;
            break;
        case Sketcher::EllipseMinorDiameter:
            minor = true;
            break;
        case Sketcher::EllipseFocus1:
            focus1 = true;
            break;
        case Sketcher::EllipseFocus2:
            focus2 = true;
            break;
        default:
            return -1;
        }
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    const auto* ellipse = static_cast<const Part::GeomEllipse*>(geo);

    Base::Vector3d center {ellipse->getCenter()};
    double majord {ellipse->getMajorRadius()};
    double minord {ellipse->getMinorRadius()};
    Base::Vector3d majdir {ellipse->getMajorAxisDir()};

    Base::Vector3d mindir = Vector3d(-majdir.y, majdir.x);

    Base::Vector3d majorpositiveend = center + majord * majdir;
    Base::Vector3d majornegativeend = center - majord * majdir;
    Base::Vector3d minorpositiveend = center + minord * mindir;
    Base::Vector3d minornegativeend = center - minord * mindir;

    double df = sqrt(majord * majord - minord * minord);

    Base::Vector3d focus1P = center + df * majdir;
    Base::Vector3d focus2P = center - df * majdir;

    if (!major) {
        Part::GeomLineSegment* lmajor = new Part::GeomLineSegment();
        lmajor->setPoints(majorpositiveend, majornegativeend);

        igeo.push_back(lmajor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseMajorDiameter;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!minor) {
        Part::GeomLineSegment* lminor = new Part::GeomLineSegment();
        lminor->setPoints(minorpositiveend, minornegativeend);

        igeo.push_back(lminor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseMinorDiameter;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!focus1) {
        Part::GeomPoint* pf1 = new Part::GeomPoint();
        pf1->setPoint(focus1P);

        igeo.push_back(pf1);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseFocus1;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!focus2) {
        Part::GeomPoint* pf2 = new Part::GeomPoint();
        pf2->setPoint(focus2P);
        igeo.push_back(pf2);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseFocus2;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
    }

    addAndCleanup(igeo, icon);
    return incrgeo;
}

void SketchObject::addAndCleanup(std::vector<Part::Geometry*> igeo, std::vector<Constraint*> icon)
{
    this->addGeometry(igeo, true);
    this->addConstraints(icon);

    for (auto& geoToDelete : igeo) {
        delete geoToDelete;
    }

    for (auto& constraintToDelete : icon) {
        delete constraintToDelete;
    }
}

// TODO: This is a repeat of ellipse. Can we do some code reuse?
template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomArcOfEllipse>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);
    // First we search what has to be restored
    bool major = false;
    bool minor = false;
    bool focus1 = false;
    bool focus2 = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (const auto& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::EllipseMajorDiameter:
            major = true;
            break;
        case Sketcher::EllipseMinorDiameter:
            minor = true;
            break;
        case Sketcher::EllipseFocus1:
            focus1 = true;
            break;
        case Sketcher::EllipseFocus2:
            focus2 = true;
            break;
        default:
            return -1;
        }
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    const auto* aoe = static_cast<const Part::GeomArcOfEllipse*>(geo);

    Base::Vector3d center {aoe->getCenter()};
    double majord {aoe->getMajorRadius()};
    double minord {aoe->getMinorRadius()};
    Base::Vector3d majdir {aoe->getMajorAxisDir()};

    Base::Vector3d mindir {-majdir.y, majdir.x};

    Base::Vector3d majorpositiveend {center + majord * majdir};
    Base::Vector3d majornegativeend {center - majord * majdir};
    Base::Vector3d minorpositiveend {center + minord * mindir};
    Base::Vector3d minornegativeend {center - minord * mindir};

    double df = sqrt(majord * majord - minord * minord);

    Base::Vector3d focus1P {center + df * majdir};
    Base::Vector3d focus2P {center - df * majdir};

    if (!major) {
        Part::GeomLineSegment* lmajor = new Part::GeomLineSegment();
        lmajor->setPoints(majorpositiveend, majornegativeend);

        igeo.push_back(lmajor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseMajorDiameter;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!minor) {
        Part::GeomLineSegment* lminor = new Part::GeomLineSegment();
        lminor->setPoints(minorpositiveend, minornegativeend);

        igeo.push_back(lminor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseMinorDiameter;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!focus1) {
        Part::GeomPoint* pf1 = new Part::GeomPoint();
        pf1->setPoint(focus1P);

        igeo.push_back(pf1);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseFocus1;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!focus2) {
        Part::GeomPoint* pf2 = new Part::GeomPoint();
        pf2->setPoint(focus2P);
        igeo.push_back(pf2);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseFocus2;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
    }

    addAndCleanup(igeo, icon);
    return incrgeo;  // number of added elements
}

template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomArcOfHyperbola>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);
    // First we search what has to be restored
    bool major = false;
    bool minor = false;
    bool focus = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::HyperbolaMajor:
            major = true;
            break;
        case Sketcher::HyperbolaMinor:
            minor = true;
            break;
        case Sketcher::HyperbolaFocus:
            focus = true;
            break;
        default:
            return -1;
        }
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    const auto* aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);

    Base::Vector3d center {aoh->getCenter()};
    double majord {aoh->getMajorRadius()};
    double minord {aoh->getMinorRadius()};
    Base::Vector3d majdir {aoh->getMajorAxisDir()};

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    Base::Vector3d mindir = Vector3d(-majdir.y, majdir.x);

    Base::Vector3d majorpositiveend = center + majord * majdir;
    Base::Vector3d majornegativeend = center - majord * majdir;
    Base::Vector3d minorpositiveend = majorpositiveend + minord * mindir;
    Base::Vector3d minornegativeend = majorpositiveend - minord * mindir;

    double df = sqrt(majord * majord + minord * minord);

    Base::Vector3d focus1P = center + df * majdir;

    if (!major) {
        Part::GeomLineSegment* lmajor = new Part::GeomLineSegment();
        lmajor->setPoints(majorpositiveend, majornegativeend);

        igeo.push_back(lmajor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::HyperbolaMajor;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!minor) {
        Part::GeomLineSegment* lminor = new Part::GeomLineSegment();
        lminor->setPoints(minorpositiveend, minornegativeend);

        igeo.push_back(lminor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::HyperbolaMinor;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);

        incrgeo++;
    }
    if (!focus) {
        Part::GeomPoint* pf1 = new Part::GeomPoint();
        pf1->setPoint(focus1P);

        igeo.push_back(pf1);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::HyperbolaFocus;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }

    addAndCleanup(igeo, icon);
    return incrgeo;  // number of added elements
}

template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomArcOfParabola>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);
    // First we search what has to be restored
    bool focus = false;
    bool focus_to_vertex = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::ParabolaFocus:
            focus = true;
            break;
        case Sketcher::ParabolaFocalAxis:
            focus_to_vertex = true;
            break;
        default:
            return -1;
        }
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    const auto* aop = static_cast<const Part::GeomArcOfParabola*>(geo);

    Base::Vector3d center {aop->getCenter()};
    Base::Vector3d focusp {aop->getFocus()};

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    if (!focus) {
        Part::GeomPoint* pf1 = new Part::GeomPoint();
        pf1->setPoint(focusp);

        igeo.push_back(pf1);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::ParabolaFocus;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }

    if (!focus_to_vertex) {
        Part::GeomLineSegment* paxis = new Part::GeomLineSegment();
        paxis->setPoints(center, focusp);

        igeo.push_back(paxis);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::ParabolaFocalAxis;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::none;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);

        incrgeo++;
    }

    addAndCleanup(igeo, icon);
    return incrgeo;  // number of added elements
}

template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomBSplineCurve>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
    // First we search what has to be restored
    std::vector<int> controlpointgeoids(bsp->countPoles(), GeoEnum::GeoUndef);

    std::vector<int> knotgeoids(bsp->countKnots(), GeoEnum::GeoUndef);

    bool isfirstweightconstrained = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    // search for existing poles
    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::BSplineControlPoint:
            controlpointgeoids[constr->InternalAlignmentIndex] = constr->First;
            break;
        case Sketcher::BSplineKnotPoint:
            knotgeoids[constr->InternalAlignmentIndex] = constr->First;
            break;
        default:
            return -1;
        }
    }

    if (controlpointgeoids[0] != GeoEnum::GeoUndef) {
        isfirstweightconstrained =
            std::ranges::any_of(vals, [&controlpointgeoids](const auto& constr) {
                return (constr->Type == Sketcher::Weight && constr->First == controlpointgeoids[0]);
            });
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<double> weights = bsp->getWeights();
    std::vector<double> knots = bsp->getKnots();

    double distance_p0_p1 = (poles[1] - poles[0]).Length();// for visual purposes only

    for (size_t index = 0; index < controlpointgeoids.size(); ++index) {
        auto& cpGeoId = controlpointgeoids.at(index);
        if (cpGeoId != GeoEnum::GeoUndef) {
            continue;
        }

        // if controlpoint not existing
        Part::GeomCircle* pc = new Part::GeomCircle();
        pc->setCenter(poles[index]);
        pc->setRadius(distance_p0_p1 / 6);

        igeo.push_back(pc);
        incrgeo++;

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::BSplineControlPoint;
        newConstr->First = currentgeoid + incrgeo;
        newConstr->FirstPos = Sketcher::PointPos::mid;
        newConstr->Second = GeoId;
        newConstr->InternalAlignmentIndex = index;

        icon.push_back(newConstr);

        if (index == 0) {
            controlpointgeoids[0] = currentgeoid + incrgeo;
            if (weights[0] == 1.0) {
                // if the first weight is 1.0 it's probably going to be non-rational
                Sketcher::Constraint* newConstr3 = new Sketcher::Constraint();
                newConstr3->Type = Sketcher::Weight;
                newConstr3->First = controlpointgeoids[0];
                newConstr3->setValue(weights[0]);

                icon.push_back(newConstr3);

                isfirstweightconstrained = true;
            }

            continue;
        }

        if (isfirstweightconstrained && weights[0] == weights[index]) {
            // if pole-weight newly created AND first weight is radius-constrained,
            // AND these weights are equal, constrain them to be equal
            Sketcher::Constraint* newConstr2 = new Sketcher::Constraint();
            newConstr2->Type = Sketcher::Equal;
            newConstr2->First = currentgeoid + incrgeo;
            newConstr2->FirstPos = Sketcher::PointPos::none;
            newConstr2->Second = controlpointgeoids[0];
            newConstr2->SecondPos = Sketcher::PointPos::none;

            icon.push_back(newConstr2);
        }
    }

    for (size_t index = 0; index < knotgeoids.size(); ++index) {
        auto& kGeoId = knotgeoids.at(index);
        if (kGeoId != GeoEnum::GeoUndef) {
            continue;
        }

        // if knot point not existing
        Part::GeomPoint* kp = new Part::GeomPoint();

        kp->setPoint(bsp->pointAtParameter(knots[index]));

        igeo.push_back(kp);
        incrgeo++;

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::BSplineKnotPoint;
        newConstr->First = currentgeoid + incrgeo;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;
        newConstr->InternalAlignmentIndex = index;

        icon.push_back(newConstr);
    }

    Q_UNUSED(isfirstweightconstrained);

    addAndCleanup(igeo, icon);
    return incrgeo;  // number of added elements
}

int SketchObject::exposeInternalGeometry(int GeoId)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const Part::Geometry* geo = getGeometry(GeoId);
    // Only for supported types
    if (geo->is<Part::GeomEllipse>()) {
        return exposeInternalGeometryForType<Part::GeomEllipse>(GeoId);
    }
    else if (geo->is<Part::GeomArcOfEllipse>()) {
        return exposeInternalGeometryForType<Part::GeomArcOfEllipse>(GeoId);
    }
    else if (geo->is<Part::GeomArcOfHyperbola>()) {
        return exposeInternalGeometryForType<Part::GeomArcOfHyperbola>(GeoId);
    }
    else if (geo->is<Part::GeomArcOfParabola>()) {
        return exposeInternalGeometryForType<Part::GeomArcOfParabola>(GeoId);
    }
    else if (geo->is<Part::GeomBSplineCurve>()) {
        return exposeInternalGeometryForType<Part::GeomBSplineCurve>(GeoId);
    }
    else
        return -1;// not supported type
}

int SketchObject::deleteUnusedInternalGeometry(int GeoId, bool delgeoid)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const Part::Geometry* geo = getGeometry(GeoId);
    // Only for supported types
    if (geo->is<Part::GeomEllipse>()
        || geo->is<Part::GeomArcOfEllipse>()
        || geo->is<Part::GeomArcOfHyperbola>()) {
        return deleteUnusedInternalGeometryWhenTwoFoci(GeoId, delgeoid);
    }

    if (geo->is<Part::GeomArcOfParabola>()) {
        return deleteUnusedInternalGeometryWhenOneFocus(GeoId, delgeoid);
    }

    if (geo->is<Part::GeomBSplineCurve>()) {
        return deleteUnusedInternalGeometryWhenBSpline(GeoId, delgeoid);
    }

    // Default case: type not supported
        return -1;
}

int SketchObject::deleteUnusedInternalGeometryWhenTwoFoci(int GeoId, bool delgeoid)
{
    int majorelementindex = -1;
    int minorelementindex = -1;
    int focus1elementindex = -1;
    int focus2elementindex = -1;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::EllipseMajorDiameter:
        case Sketcher::HyperbolaMajor:
            majorelementindex = constr->First;
            break;
        case Sketcher::EllipseMinorDiameter:
        case Sketcher::HyperbolaMinor:
            minorelementindex = constr->First;
            break;
        case Sketcher::EllipseFocus1:
        case Sketcher::HyperbolaFocus:
            focus1elementindex = constr->First;
            break;
        case Sketcher::EllipseFocus2:
            focus2elementindex = constr->First;
            break;
        default:
            return -1;
        }
    }

    // Hide unused geometry here
    int majorconstraints = 0;// number of constraints associated to the geoid of the major axis
    int minorconstraints = 0;
    int focus1constraints = 0;
    int focus2constraints = 0;

    for (const auto& constr : vals) {
        if (constr->involvesGeoId(majorelementindex))
            majorconstraints++;
        else if (constr->involvesGeoId(minorelementindex))
            minorconstraints++;
        else if (constr->involvesGeoId(focus1elementindex))
            focus1constraints++;
        else if (constr->involvesGeoId(focus2elementindex))
            focus2constraints++;
    }

    std::vector<int> delgeometries;

    // those with less than 2 constraints must be removed
    if (focus2constraints < 2)
        delgeometries.push_back(focus2elementindex);

    if (focus1constraints < 2)
        delgeometries.push_back(focus1elementindex);

    if (minorconstraints < 2)
        delgeometries.push_back(minorelementindex);

    if (majorconstraints < 2)
        delgeometries.push_back(majorelementindex);

    if (delgeoid)
        delgeometries.push_back(GeoId);

    // indices over an erased element get automatically updated!!
    std::sort(delgeometries.begin(), delgeometries.end(), std::greater<>());

    for (auto& dGeoId : delgeometries) {
        delGeometry(dGeoId, DeleteOption::UpdateGeometry);
    }

    int ndeleted = delgeometries.size();

    return ndeleted;// number of deleted elements
}

int SketchObject::deleteUnusedInternalGeometryWhenOneFocus(int GeoId, bool delgeoid)
{
    // if the focus-to-vertex line is constrained, then never delete the focus
    // if the line is unconstrained, then the line may be deleted,
    // in this case the focus may be deleted if unconstrained.
    int majorelementindex = -1;
    int focus1elementindex = -1;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::ParabolaFocus:
            focus1elementindex = constr->First;
            break;
        case Sketcher::ParabolaFocalAxis:
            majorelementindex = constr->First;
            break;
        default:
            return -1;
        }
    }

    // Hide unused geometry here
    // number of constraints associated to the geoid of the major axis other than the coincident
    // ones
    int majorconstraints = 0;
    int focus1constraints = 0;

    for (const auto& constr : vals) {
        if (constr->involvesGeoId(majorelementindex)) {
            majorconstraints++;
        }
        else if (constr->involvesGeoId(focus1elementindex)) {
            focus1constraints++;
        }
    }

    std::vector<int> delgeometries;

    // major has minimum one constraint, the specific internal alignment constraint
    if (majorelementindex != -1 && majorconstraints < 2)
        delgeometries.push_back(majorelementindex);

    // focus has minimum one constraint now, the specific internal alignment constraint
    if (focus1elementindex != -1 && focus1constraints < 2)
        delgeometries.push_back(focus1elementindex);

    if (delgeoid)
        delgeometries.push_back(GeoId);

    // indices over an erased element get automatically updated!!
    std::sort(delgeometries.begin(), delgeometries.end(), std::greater<>());

    for (auto& dGeoId : delgeometries) {
        delGeometry(dGeoId, DeleteOption::UpdateGeometry);
    }

    int ndeleted = delgeometries.size();

    delgeometries.clear();

    return ndeleted;// number of deleted elements
}

int SketchObject::deleteUnusedInternalGeometryWhenBSpline(int GeoId, bool delgeoid)
{
    // First we search existing IA
    std::map<int, int> poleGeoIdsAndConstraints;
    std::map<int, int> knotGeoIdsAndConstraints;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    // search for existing poles
    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::BSplineControlPoint:
            poleGeoIdsAndConstraints[constr->First] = 0;
            break;
        case Sketcher::BSplineKnotPoint:
            knotGeoIdsAndConstraints[constr->First] = 0;
            break;
        default:
            return -1;
        }
    }

    std::vector<int> delgeometries;

    // Update all control point constraint counts.
    // EXCLUDES internal alignment and related constraints.
    for (auto const& constr : vals) {
        // We do not ignore weight constraints as we did with radius constraints,
        // because the radius magnitude no longer makes sense without the B-Spline.
        if (constr->Type == Sketcher::InternalAlignment
            || constr->Type == Sketcher::Weight) {
            continue;
        }
        bool firstIsInCPGeoIds = poleGeoIdsAndConstraints.count(constr->First) == 1;
        bool secondIsInCPGeoIds = poleGeoIdsAndConstraints.count(constr->Second) == 1;
        if (constr->Type == Sketcher::Equal && firstIsInCPGeoIds == secondIsInCPGeoIds) {
            continue;
        }
        // any equality constraint constraining a pole is not interpole
        if (firstIsInCPGeoIds) {
            ++poleGeoIdsAndConstraints[constr->First];
        }
        if (secondIsInCPGeoIds) {
            ++poleGeoIdsAndConstraints[constr->Second];
        }
    }

    for (auto& [cpGeoId, numConstr] : poleGeoIdsAndConstraints) {
        if (numConstr < 1) { // IA
            delgeometries.push_back(cpGeoId);
        }
    }

    for (auto& [kGeoId, numConstr] : knotGeoIdsAndConstraints) {
        // Update all control point constraint counts.
        // INCLUDES internal alignment and related constraints.
        auto tempGeoID = kGeoId;  // C++17 and earlier do not support captured structured bindings
        numConstr = std::count_if(vals.begin(), vals.end(), [&tempGeoID](const auto& constr) {
            return constr->involvesGeoId(tempGeoID);
        });

        if (numConstr < 2) { // IA
            delgeometries.push_back(kGeoId);
        }
    }

    if (delgeoid) {
        delgeometries.push_back(GeoId);
    }

    int ndeleted = delGeometriesExclusiveList(delgeometries);

    return ndeleted;// number of deleted elements
}

int SketchObject::deleteUnusedInternalGeometryAndUpdateGeoId(int& GeoId, bool delgeoid)
{
    const Part::Geometry* geo = getGeometry(GeoId);

    if (!hasInternalGeometry(geo)) {
        return -1;
    }
    // We need to remove the internal geometry of the BSpline, as BSplines change in number
    // of poles and knots We save the tags of the relevant geometry to retrieve the new
    // GeoIds later on.
    boost::uuids::uuid GeoIdTag;

    GeoIdTag = geo->getTag();

    int returnValue = deleteUnusedInternalGeometry(GeoId, delgeoid);

    if (delgeoid) {
        GeoId = GeoEnum::GeoUndef;
        return returnValue;
    }

    auto vals = getCompleteGeometry();

    for (size_t i = 0; i < vals.size(); i++) {
        if (vals[i]->getTag() == GeoIdTag) {
            GeoId = getGeoIdFromCompleteGeometryIndex(i);
            break;
        }
    }

    return returnValue;
}

bool SketchObject::convertToNURBS(int GeoId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId > getHighestCurveIndex()
        || (GeoId < 0 && -GeoId > static_cast<int>(ExternalGeo.getSize())) || GeoId == -1
        || GeoId == -2)
        return false;

    const Part::Geometry* geo = getGeometry(GeoId);

    if (geo->is<Part::GeomPoint>())
        return false;

    const auto* geo1 = static_cast<const Part::GeomCurve*>(geo);

    Part::GeomBSplineCurve* bspline;

    try {
        bspline = geo1->toNurbs(geo1->getFirstParameter(), geo1->getLastParameter());

        if (geo1->isDerivedFrom<Part::GeomArcOfConic>()) {
            const auto* geoaoc = static_cast<const Part::GeomArcOfConic*>(geo1);

            if (geoaoc->isReversed())
                bspline->reverse();
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        // revert to original values
        return false;
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    // Block checks and updates in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);

        if (GeoId < 0) {// external geometry
            newVals.push_back(bspline);
            generateId(bspline);
        }
        else {// normal geometry

            newVals[GeoId] = bspline;
            GeometryFacade::copyId(geo, bspline);

            const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

            std::vector<Constraint*> newcVals(cvals);

            int index = cvals.size() - 1;
            // delete constraints on this elements other than coincident constraints (bspline does
            // not support them currently), except for coincidents on mid point of the
            // to-be-converted curve.
            for (; index >= 0; index--) {
                auto otherthancoincident = cvals[index]->Type != Sketcher::Coincident
                    && (cvals[index]->First == GeoId || cvals[index]->Second == GeoId
                        || cvals[index]->Third == GeoId);

                auto coincidentonmidpoint = cvals[index]->Type == Sketcher::Coincident
                    && ((cvals[index]->First == GeoId
                         && cvals[index]->FirstPos == Sketcher::PointPos::mid)
                        || (cvals[index]->Second == GeoId
                            && cvals[index]->SecondPos == Sketcher::PointPos::mid));

                if (otherthancoincident || coincidentonmidpoint)
                    newcVals.erase(newcVals.begin() + index);
            }

            this->Constraints.setValues(std::move(newcVals));
        }

        Geometry.setValues(std::move(newVals));
    }

    // trigger update now
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    return true;
}

bool SketchObject::increaseBSplineDegree(int GeoId, int degreeincrement /*= 1*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        return false;
    }

    const Part::Geometry* geo = getGeometry(GeoId);

    if (!geo->is<Part::GeomBSplineCurve>()) {
        return false;
    }

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    const Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(bsp->handle());

    std::unique_ptr<Part::GeomBSplineCurve> bspline(new Part::GeomBSplineCurve(curve));

    try {
        int cdegree = bspline->getDegree();

        bspline->increaseDegree(cdegree + degreeincrement);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    GeometryFacade::copyId(geo, bspline.get());
    newVals[GeoId] = bspline.release();

    // AcceptGeometry called from onChanged
    Geometry.setValues(std::move(newVals));

    return true;
}

bool SketchObject::decreaseBSplineDegree(int GeoId, int degreedecrement /*= 1*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return false;

    const Part::Geometry* geo = getGeometry(GeoId);

    if (!geo->is<Part::GeomBSplineCurve>())
        return false;

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    const Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(bsp->handle());

    std::unique_ptr<Part::GeomBSplineCurve> bspline(new Part::GeomBSplineCurve(curve));

    try {
        int cdegree = bspline->getDegree();

        // degree must be >= 1
        int maxdegree = cdegree - degreedecrement;
        if (maxdegree == 0)
            return false;
        bspline->approximate(Precision::Confusion(), 20, maxdegree, GeomAbs_C0);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }

    // FIXME: Avoid to delete the whole geometry but only delete invalid constraints
    // and unused construction geometries
#if 0
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);

    newVals[GeoId] = bspline.release();

    // AcceptGeometry called from onChanged
    Geometry.setValues(newVals);
#else
    delGeometry(GeoId);
    int newId = addGeometry(bspline.release());
    exposeInternalGeometry(newId);
#endif

    return true;
}

// clang-format on
bool SketchObject::modifyBSplineKnotMultiplicity(int GeoId, int knotIndex, int multiplicityincr)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "B-spline Geometry Index (GeoID) is out of bounds.")
        );
    }

    if (multiplicityincr == 0) {
        // no change in multiplicity
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "You are requesting no change in knot multiplicity.")
        );
    }

    const Part::Geometry* geo = getGeometry(GeoId);

    if (!geo->is<Part::GeomBSplineCurve>()) {
        THROWMT(
            Base::TypeError,
            QT_TRANSLATE_NOOP("Exceptions", "The Geometry Index (GeoId) provided is not a B-spline.")
        );
    }

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    int degree = bsp->getDegree();

    if (knotIndex > bsp->countKnots() || knotIndex < 1) {
        // knotindex in OCC 1 -> countKnots
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP(
                "Exceptions",
                "The knot index is out of bounds. Note that in accordance with "
                "OCC notation, the first knot has index 1 and not zero."
            )
        );
    }

    std::unique_ptr<Part::GeomBSplineCurve> bspline;

    int curmult = bsp->getMultiplicity(knotIndex);

    // zero is removing the knot, degree is just positional continuity
    if ((curmult + multiplicityincr) > degree) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP(
                "Exceptions",
                "The multiplicity cannot be increased beyond the degree of the B-spline."
            )
        );
    }

    // zero is removing the knot, degree is just positional continuity
    if ((curmult + multiplicityincr) < 0) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "The multiplicity cannot be decreased beyond zero.")
        );
    }

    try {
        bspline.reset(static_cast<Part::GeomBSplineCurve*>(bsp->clone()));

        if (multiplicityincr > 0) {  // increase multiplicity
            bspline->increaseMultiplicity(knotIndex, curmult + multiplicityincr);
        }
        else {  // decrease multiplicity
            bool result = bspline->removeKnot(knotIndex, curmult + multiplicityincr, 1E6);

            if (!result) {
                THROWMT(
                    Base::CADKernelError,
                    QT_TRANSLATE_NOOP(
                        "Exceptions",
                        "OCC is unable to decrease the multiplicity within the "
                        "maximum tolerance."
                    )
                );
            }
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }

    // we succeeded with the multiplicity modification, so alignment geometry may be
    // invalid/inconsistent for the new bspline
    std::vector<int> delGeoId;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<Base::Vector3d> newPoles = bspline->getPoles();

    // on fully removing a knot the knot geometry changes
    std::vector<double> knots = bsp->getKnots();
    std::vector<double> newKnots = bspline->getKnots();

    std::map<Sketcher::InternalAlignmentType, std::vector<int>> indexInNew {
        {Sketcher::BSplineControlPoint, {}},
        {Sketcher::BSplineKnotPoint, {}}
    };
    indexInNew[Sketcher::BSplineControlPoint].reserve(poles.size());
    indexInNew[Sketcher::BSplineKnotPoint].reserve(knots.size());

    for (const auto& pole : poles) {
        const auto it = std::ranges::find(newPoles, pole);
        indexInNew[Sketcher::BSplineControlPoint].emplace_back(it - newPoles.begin());
    }
    std::ranges::replace(indexInNew[Sketcher::BSplineControlPoint], int(newPoles.size()), -1);

    for (const auto& knot : knots) {
        const auto it = std::ranges::find(newKnots, knot);
        indexInNew[Sketcher::BSplineKnotPoint].emplace_back(it - newKnots.begin());
    }
    std::ranges::replace(indexInNew[Sketcher::BSplineKnotPoint], int(newKnots.size()), -1);

    const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

    std::vector<Constraint*> newcVals(0);

    // modify pole and knot constraints
    for (const auto& constr : cvals) {
        if (!(constr->Type == Sketcher::InternalAlignment && constr->Second == GeoId)) {
            newcVals.push_back(constr);
            continue;
        }

        int index = indexInNew.at(constr->AlignmentType).at(constr->InternalAlignmentIndex);

        if (index == -1) {
            // it is an internal alignment geometry that is no longer valid
            // => delete it and the geometry
            delGeoId.push_back(constr->First);
            continue;
        }

        Constraint* newConstr = constr->clone();
        newConstr->InternalAlignmentIndex = index;
        newcVals.push_back(newConstr);
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    GeometryFacade::copyId(geo, bspline.get());
    newVals[GeoId] = bspline.release();

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(std::move(newVals));

        this->Constraints.setValues(std::move(newcVals));
    }

    // Trigger update now
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    if (!delGeoId.empty()) {
        delGeometriesExclusiveList(delGeoId);
    }
    else {
        Geometry.touch();
    }
    return true;
}

bool SketchObject::insertBSplineKnot(int GeoId, double param, int multiplicity)
{
    // TODO: Check if this is still valid: no need to check input data validity as this is an
    // sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // handling unacceptable cases
    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "B-spline Geometry Index (GeoID) is out of bounds.")
        );
    }

    if (multiplicity == 0) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "Knot cannot have zero multiplicity.")
        );
    }

    const Part::Geometry* geo = getGeometry(GeoId);

    if (!geo->is<Part::GeomBSplineCurve>()) {
        THROWMT(
            Base::TypeError,
            QT_TRANSLATE_NOOP("Exceptions", "The Geometry Index (GeoId) provided is not a B-spline.")
        );
    }

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    int degree = bsp->getDegree();
    double firstParam = bsp->getFirstParameter();
    double lastParam = bsp->getLastParameter();

    if (multiplicity > degree) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP(
                "Exceptions",
                "Knot multiplicity cannot be higher than the degree of the B-spline."
            )
        );
    }

    if (param > lastParam || param < firstParam) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "Knot cannot be inserted outside the B-spline parameter range.")
        );
    }

    std::unique_ptr<Part::GeomBSplineCurve> bspline;

    // run the command
    try {
        bspline.reset(static_cast<Part::GeomBSplineCurve*>(bsp->clone()));

        bspline->insertKnot(param, multiplicity);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }

    // once command is run update the internal geometries
    std::vector<int> delGeoId;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<Base::Vector3d> newPoles = bspline->getPoles();
    std::vector<int> poleIndexInNew(poles.size(), -1);

    for (size_t j = 0; j < poles.size(); j++) {
        const auto it = std::ranges::find(newPoles, poles[j]);
        poleIndexInNew[j] = it - newPoles.begin();
    }
    std::ranges::replace(poleIndexInNew, int(newPoles.size()), -1);

    std::vector<double> knots = bsp->getKnots();
    std::vector<double> newKnots = bspline->getKnots();
    std::vector<int> knotIndexInNew(knots.size(), -1);

    for (size_t j = 0; j < knots.size(); j++) {
        const auto it = std::ranges::find(newKnots, knots[j]);
        knotIndexInNew[j] = it - newKnots.begin();
    }
    std::ranges::replace(knotIndexInNew, int(newKnots.size()), -1);

    const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

    std::vector<Constraint*> newcVals(0);

    // modify pole and knot constraints
    for (const auto& constr : cvals) {
        if (!(constr->Type == Sketcher::InternalAlignment && constr->Second == GeoId)) {
            newcVals.push_back(constr);
            continue;
        }

        std::vector<int>* indexInNew = nullptr;

        if (constr->AlignmentType == Sketcher::BSplineControlPoint) {
            indexInNew = &poleIndexInNew;
        }
        else if (constr->AlignmentType == Sketcher::BSplineKnotPoint) {
            indexInNew = &knotIndexInNew;
        }
        else {
            // it is a bspline geometry, but not a controlpoint or knot
            newcVals.push_back(constr);
            continue;
        }

        if (indexInNew && indexInNew->at(constr->InternalAlignmentIndex) == -1) {
            // it is an internal alignment geometry that is no longer valid
            // => delete it and the pole circle
            delGeoId.push_back(constr->First);
            continue;
        }

        Constraint* newConstr = constr->clone();
        newConstr->InternalAlignmentIndex = indexInNew->at(constr->InternalAlignmentIndex);
        newcVals.push_back(newConstr);
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    GeometryFacade::copyId(geo, bspline.get());
    newVals[GeoId] = bspline.release();

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(std::move(newVals));

        this->Constraints.setValues(std::move(newcVals));
    }

    // Trigger update now
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    if (!delGeoId.empty()) {
        // NOTE: There have been a couple of instances when knot insertion has
        // led to a segmentation fault: see
        // https://forum.freecad.org/viewtopic.php?f=19&t=64962&sid=10272db50a635c633260517b14ecad37.
        // If a segfault happens again and a `Geometry.touch()` here fixes it,
        // it is possible that `delGeometriesExclusiveList` is causing an update
        // in constraint GUI features during an intermediate step.
        // See 247a9f0876a00e08c25b07d1f8802479d8623e87 for suggestions.
        // Geometry.touch();
        delGeometriesExclusiveList(delGeoId);
        return true;
    }

    Geometry.touch();

    return true;
}
// clang-format off

int SketchObject::carbonCopy(App::DocumentObject* pObj, bool construction)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // so far only externals to the support of the sketch and datum features
    bool xinv = false, yinv = false;

    if (!isCarbonCopyAllowed(pObj->getDocument(), pObj, xinv, yinv))
        return -1;

    SketchObject* psObj = static_cast<SketchObject*>(pObj);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

    std::vector<Part::Geometry*> newVals(vals);

    std::vector<Constraint*> newcVals(cvals);

    int nextgeoid = vals.size();

    int nextextgeoid = getExternalGeometryCount();

    int nextcid = cvals.size();

    const std::vector<Part::Geometry*>& svals = psObj->getInternalGeometry();

    const std::vector<Sketcher::Constraint*>& scvals = psObj->Constraints.getValues();

    newVals.reserve(vals.size() + svals.size());
    newcVals.reserve(cvals.size() + scvals.size());

    std::map<int, int> extMap;
    if (psObj->ExternalGeo.getSize() > 1) {
        int i = -1;
        auto geos = this->ExternalGeo.getValues();
        std::string myName(this->getNameInDocument());
        myName += ".";
        for (const auto &geo : psObj->ExternalGeo.getValues()) {
            if (++i < 2) // skip h/v axes
                continue;
            else {
                auto egf = ExternalGeometryFacade::getFacade(geo);
                const auto &ref = egf->getRef();
                if (boost::starts_with(ref, myName)) {
                    int geoId;
                    PointPos posId;
                    if (this->geoIdFromShapeType(ref.c_str()+myName.size(), geoId, posId)) {
                        extMap[-i-1] = geoId;
                        continue;
                    }
                }
            }
            auto copy = geo->copy();
            auto egf = ExternalGeometryFacade::getFacade(copy);
            egf->setId(++geoLastId);
            if (!egf->getRef().empty()) {
                auto &refs = this->externalGeoRefMap[egf->getRef()];
                refs.push_back(geoLastId);
            }
            this->externalGeoMap[geoLastId] = (int)geos.size();
            geos.push_back(copy);
            extMap[-i-1] = -(int)geos.size();
        }
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard(App::Property::User3, &this->ExternalGeo);
        this->ExternalGeo.setValues(std::move(geos));
    }

    if (psObj->ExternalGeometry.getSize() > 0) {
        std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
        std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

        const std::vector<DocumentObject*> originalObjects = Objects;
        const std::vector<std::string> originalSubElements = SubElements;

        std::vector<DocumentObject*> sObjects = psObj->ExternalGeometry.getValues();
        std::vector<std::string> sSubElements = psObj->ExternalGeometry.getSubValues();

        if (Objects.size() != SubElements.size() || sObjects.size() != sSubElements.size()) {
            assert(0 /*counts of objects and subelements in external geometry links do not match*/);
            Base::Console().error("Internal error: counts of objects and subelements in external "
                                  "geometry links do not match\n");
            return -1;
        }

        int si = 0;
        for (auto& sobj : sObjects) {
            int i = 0;
            for (auto& obj : Objects) {
                if (obj == sobj && SubElements[i] == sSubElements[si]) {
                    Base::Console().error(
                        "Link to %s already exists in this sketch. Delete the link and try again\n",
                        sSubElements[si].c_str());
                    return -1;
                }

                i++;
            }

            Objects.push_back(sobj);
            SubElements.push_back(sSubElements[si]);

            si++;
        }

        ExternalGeometry.setValues(Objects, SubElements);

        try {
            rebuildExternalGeometry();
        }
        catch (const Base::Exception& e) {
            Base::Console().error("%s\n", e.what());
            // revert to original values
            ExternalGeometry.setValues(originalObjects, originalSubElements);
            return -1;
        }

        solverNeedsUpdate = true;
    }

    for (std::vector<Part::Geometry*>::const_iterator it = svals.begin(); it != svals.end(); ++it) {
        Part::Geometry* geoNew = (*it)->copy();
        generateId(geoNew);
        if (construction && !geoNew->is<Part::GeomPoint>()) {
            GeometryFacade::setConstruction(geoNew, true);
        }
        newVals.push_back(geoNew);
    }

    for (std::vector<Sketcher::Constraint*>::const_iterator it = scvals.begin(); it != scvals.end();
         ++it) {
        Sketcher::Constraint* newConstr = (*it)->copy();
        if ((*it)->First >= 0)
            newConstr->First += nextgeoid;
        if ((*it)->Second >= 0)
            newConstr->Second += nextgeoid;
        if ((*it)->Third >= 0)
            newConstr->Third += nextgeoid;

        if ((*it)->First < -2 && (*it)->First != GeoEnum::GeoUndef)
            newConstr->First -= (nextextgeoid - 2);
        if ((*it)->Second < -2 && (*it)->Second != GeoEnum::GeoUndef)
            newConstr->Second -= (nextextgeoid - 2);
        if ((*it)->Third < -2 && (*it)->Third != GeoEnum::GeoUndef)
            newConstr->Third -= (nextextgeoid - 2);

        newcVals.push_back(newConstr);
    }

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(std::move(newVals));
        this->Constraints.setValues(std::move(newcVals));
    }
    // we trigger now the update (before dealing with expressions)
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    int sourceid = 0;
    for (std::vector<Sketcher::Constraint*>::const_iterator it = scvals.begin(); it != scvals.end();
         ++it, nextcid++, sourceid++) {

        if ((*it)->isDimensional()) {
            // then we link its value to the parent
            if ((*it)->isDriving) {
                App::ObjectIdentifier spath;
                std::shared_ptr<App::Expression> expr;
                std::string scname = (*it)->Name;
                if (App::ExpressionParser::isTokenAnIndentifier(scname)) {
                    spath = App::ObjectIdentifier(psObj->Constraints)
                        << App::ObjectIdentifier::SimpleComponent(scname);
                    expr = std::shared_ptr<App::Expression>(App::Expression::parse(
                        this, spath.getDocumentObjectName().getString() + spath.toString()));
                }
                else {
                    spath = psObj->Constraints.createPath(sourceid);
                    expr = std::shared_ptr<App::Expression>(
                        App::Expression::parse(this,
                                               spath.getDocumentObjectName().getString()
                                                   + std::string(1, '.') + spath.toString()));
                }
                setExpression(Constraints.createPath(nextcid), std::move(expr));
            }
        }
    }

    // Solve even if `noRecomputes==false`, because recompute may fail, and leave the
    // sketch in an inconsistent state. A concrete example. If the copied sketch
    // has broken external geometry, its recomputation will fail. And because we
    // use expression for copied constraint to add dependency to the copied
    // sketch, this sketch will not be recomputed (because its dependency fails
    // to recompute).
    solve();

    return svals.size();
}

// clang-format on
int SketchObject::addExternal(App::DocumentObject* Obj, const char* SubName, bool defining, bool intersection)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // so far only externals to the support of the sketch and datum features
    if (!isExternalAllowed(Obj->getDocument(), Obj)) {
        return -1;
    }

    auto wholeShape = Part::Feature::getTopoShape(
        Obj,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    auto shape = wholeShape.getSubTopoShape(SubName, /*silent*/ true);
    TopAbs_ShapeEnum shapeType = TopAbs_SHAPE;
    if (shape.shapeType(/*silent*/ true) != TopAbs_FACE) {
        if (shape.hasSubShape(TopAbs_FACE)) {
            shapeType = TopAbs_FACE;
        }
        else if (shape.shapeType(/*silent*/ true) != TopAbs_EDGE && shape.hasSubShape(TopAbs_EDGE)) {
            shapeType = TopAbs_EDGE;
        }
    }

    if (shapeType != TopAbs_SHAPE) {
        std::string element = Part::TopoShape::shapeName(shapeType);
        std::size_t elementNameSize = element.size();
        int geometryCount = ExternalGeometry.getSize();

        gp_Pln sketchPlane;
        if (intersection) {
            Base::Placement Plm = Placement.getValue();
            Base::Vector3d Pos = Plm.getPosition();
            Base::Rotation Rot = Plm.getRotation();
            Base::Vector3d dN(0, 0, 1);
            Rot.multVec(dN, dN);
            Base::Vector3d dX(1, 0, 0);
            Rot.multVec(dX, dX);
            gp_Ax3 sketchAx3(
                gp_Pnt(Pos.x, Pos.y, Pos.z),
                gp_Dir(dN.x, dN.y, dN.z),
                gp_Dir(dX.x, dX.y, dX.z)
            );
            sketchPlane.SetPosition(sketchAx3);
        }
        for (const auto& subShape : shape.getSubShapes(shapeType)) {
            int idx = wholeShape.findShape(subShape);
            if (idx == 0) {
                continue;
            }
            if (intersection) {
                try {
                    FCBRepAlgoAPI_Section maker(subShape, sketchPlane);
                    if (!maker.IsDone() || maker.Shape().IsNull()) {
                        continue;
                    }
                }
                catch (Standard_Failure&) {
                    continue;
                }
            }
            element += std::to_string(idx);
            addExternal(Obj, element.c_str(), defining, intersection);
            element.resize(elementNameSize);
        }
        if (ExternalGeometry.getSize() == geometryCount) {
            return -1;
        }
        return geometryCount;
    }

    // get the actual lists of the externals
    std::vector<long> Types = ExternalTypes.getValues();
    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();
    Types.resize(Objects.size(), static_cast<long>(ExtType::Projection));

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string> originalSubElements = SubElements;

    if (Objects.size() != SubElements.size()) {
        assert(0 /*counts of objects and subelements in external geometry links do not match*/);
        Base::Console().error(
            "Internal error: counts of objects and subelements in external "
            "geometry links do not match\n"
        );
        return -1;
    }

    bool add = true;
    for (size_t i = 0; i < Objects.size(); ++i) {
        if (!(Objects[i] == Obj && std::string(SubName) == SubElements[i])) {
            continue;
        }
        if (Types[i] == static_cast<int>(ExtType::Both)
            || (Types[i] == static_cast<int>(ExtType::Projection) && !intersection)
            || (Types[i] == static_cast<int>(ExtType::Intersection) && intersection)) {
            Base::Console().error("Link to %s already exists in this sketch.\n", SubName);
            return -1;
        }
        // Case where projections are already there when adding intersections.
        add = false;
        Types[i] = static_cast<int>(ExtType::Both);
    }
    if (add) {
        // add the new ones
        Objects.push_back(Obj);
        SubElements.emplace_back(SubName);
        Types.push_back(static_cast<int>(intersection ? ExtType::Intersection : ExtType::Projection));
        if (intersection) {
        }

        // set the Link list.
        ExternalGeometry.setValues(Objects, SubElements);
    }
    ExternalTypes.setValues(Types);

    try {
        ExternalToAdd ext {Obj, std::string(SubName), defining, intersection};
        rebuildExternalGeometry(ext);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects, originalSubElements);
        return -1;
    }

    acceptGeometry();  // This may need to be refactored into onChanged for ExternalGeometry

    solverNeedsUpdate = true;
    return ExternalGeometry.getValues().size() - 1;
}
// clang-format off

int SketchObject::delExternal(int ExtGeoId)
{
    return delExternal(std::vector<int>{ExtGeoId});
}

int SketchObject::delExternal(const std::vector<int>& ExtGeoIds)
{
    std::set<long> geoIds;
    for (int ExtGeoId : ExtGeoIds) {
        int GeoId = ExtGeoId >= 0 ? GeoEnum::RefExt - ExtGeoId : ExtGeoId;
        if (GeoId > GeoEnum::RefExt || -GeoId - 1 >= ExternalGeo.getSize())
            return -1;

        auto geo = getGeometry(GeoId);
        if (!geo)
            return -1;

        auto egf = ExternalGeometryFacade::getFacade(geo);
        geoIds.insert(egf->getId());
        if (egf->getRef().size()) {
            auto& refs = externalGeoRefMap[egf->getRef()];
            geoIds.insert(refs.begin(), refs.end());
        }
    }

    delExternalPrivate(geoIds, true);
    return 0;
}

void SketchObject::delExternalPrivate(const std::set<long> &ids, bool removeRef) {

    Base::StateLocker lock(managedoperation, true); // no need to check input data validity as this is an sketchobject managed operation.

    std::set<std::string> refs;
    // Must sort in reverse order so as to delete geo from back to front to
    // avoid index change
    std::set<int, std::greater<int>> geoIds;

    for(auto id : ids) {
        auto it = externalGeoMap.find(id);
        if(it == externalGeoMap.end())
            continue;

        auto egf = ExternalGeometryFacade::getFacade(ExternalGeo[it->second]);
        if(removeRef && egf->getRef().size())
            refs.insert(egf->getRef());
        geoIds.insert(-it->second-1);
    }

    if(geoIds.empty())
        return;

    std::vector< Constraint * > newConstraints;
    for(auto cstr : Constraints.getValues()) {
        if(!geoIds.count(cstr->First) &&
           (cstr->Second==GeoEnum::GeoUndef || !geoIds.count(cstr->Second)) &&
           (cstr->Third==GeoEnum::GeoUndef || !geoIds.count(cstr->Third)))
        {
            bool cloned = false;
            int offset = 0;
            for(auto GeoId : geoIds) {
                GeoId += offset++;
                bool done = true;
                if (cstr->First < GeoId && cstr->First != GeoEnum::GeoUndef) {
                    if (!cloned) {
                        cloned = true;
                        cstr = cstr->clone();
                    }
                    cstr->First += 1;
                    done = false;
                }
                if (cstr->Second < GeoId && cstr->Second != GeoEnum::GeoUndef) {
                    if (!cloned) {
                        cloned = true;
                        cstr = cstr->clone();
                    }
                    cstr->Second += 1;
                    done = false;
                }
                if (cstr->Third < GeoId && cstr->Third != GeoEnum::GeoUndef) {
                    if (!cloned) {
                        cloned = true;
                        cstr = cstr->clone();
                    }
                    cstr->Third += 1;
                    done = false;
                }
                if(done) break;
            }
            newConstraints.push_back(cstr);
        }
    }

    auto geos = ExternalGeo.getValues();
    int offset = 0;
    for(auto geoId : geoIds) {
        int idx = -geoId-1;
        geos.erase(geos.begin()+idx-offset);
        ++offset;
    }

    if(refs.size()) {
        std::vector<std::string> newSubs;
        std::vector<App::DocumentObject*> newObjs;
        const auto &subs = ExternalGeometry.getSubValues();
        auto itSub = subs.begin();
        const auto &objs = ExternalGeometry.getValues();
        auto itObj = objs.begin();
        bool touched = false;
        assert(externalGeoRef.size() == objs.size());
        assert(externalGeoRef.size() == subs.size());
        for(auto it=externalGeoRef.begin();it!=externalGeoRef.end();++it,++itObj,++itSub) {
            if(refs.count(*it)) {
                if(!touched) {
                    touched = true;
                    if(newObjs.empty()) {
                        newObjs.insert(newObjs.end(),objs.begin(),itObj);
                        newSubs.insert(newSubs.end(),subs.begin(),itSub);
                    }
                }
            }else if(touched) {
                newObjs.push_back(*itObj);
                newSubs.push_back(*itSub);
            }
        }
        if(touched)
            ExternalGeometry.setValues(newObjs,newSubs);
    }

    ExternalGeo.setValues(std::move(geos));

    solverNeedsUpdate = true;
    Constraints.setValues(std::move(newConstraints));
    acceptGeometry(); // This may need to be refactored into OnChanged for ExternalGeometry.
}

// clang-format on
int SketchObject::delAllExternal()
{
    int count = 0;                      // the remaining count of the detached external geometry
    std::map<int, int> indexMap;        // the index map of the remain external geometry
    std::vector<Part::Geometry*> geos;  // the remaining external geometry
    for (int i = 0; i < ExternalGeo.getSize(); ++i) {
        auto geo = ExternalGeo[i];
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if (egf->getRef().empty()) {
            indexMap[i] = count++;
        }
        geos.push_back(geo);
    }
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string> originalSubElements = SubElements;

    Objects.clear();
    SubElements.clear();

    const std::vector<Constraint*>& constraints = Constraints.getValues();
    std::vector<Constraint*> newConstraints(0);

    for (const auto& constr : constraints) {
        if (constr->First > GeoEnum::RefExt
            && (constr->Second > GeoEnum::RefExt || constr->Second == GeoEnum::GeoUndef)
            && (constr->Third > GeoEnum::RefExt || constr->Third == GeoEnum::GeoUndef)) {
            Constraint* copiedConstr = constr->clone();

            newConstraints.push_back(copiedConstr);
        }
    }

    ExternalGeometry.setValues(Objects, SubElements);
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects, originalSubElements);
        for (Constraint* it : newConstraints) {
            delete it;
        }
        return -1;
    }

    ExternalGeometry.setValue(0);
    ExternalGeo.setValues(std::move(geos));
    solverNeedsUpdate = true;
    Constraints.setValues(std::move(newConstraints));
    acceptGeometry();  // This may need to be refactored into OnChanged for ExternalGeometry
    return 0;
}
// clang-format off

int SketchObject::delConstraintsToExternal(DeleteOptions options)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& constraints = Constraints.getValuesForce();
    std::vector<Constraint*> newConstraints(0);
    int GeoId = GeoEnum::RefExt, NullId = GeoEnum::GeoUndef;
    for (std::vector<Constraint*>::const_iterator it = constraints.begin(); it != constraints.end();
         ++it) {
        if ((*it)->First > GeoId && ((*it)->Second > GeoId || (*it)->Second == NullId)
            && ((*it)->Third > GeoId || (*it)->Third == NullId)) {
            newConstraints.push_back(*it);
        }
    }

    Constraints.setValues(std::move(newConstraints));
    Constraints.acceptGeometry(getCompleteGeometry());

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoFlag)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::attachExternal(
        const std::vector<int> &geoIds, App::DocumentObject *Obj, const char* SubName)
{
    if (!isExternalAllowed(Obj->getDocument(), Obj))
       return -1;

    std::set<std::string> detached;
    std::set<int> idSet;
    for (int geoId : geoIds) {
        if (geoId > GeoEnum::RefExt || -geoId - 1 >= ExternalGeo.getSize())
            continue;
        auto geo = getGeometry(geoId);
        if(!geo)
            continue;
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if(egf->getRef().size())
            detached.insert(egf->getRef());
        for(int id : getRelatedGeometry(geoId))
            idSet.insert(id);
    }

    auto geos = ExternalGeo.getValues();

    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    auto itObj = Objects.begin();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();
    auto itSub = SubElements.begin();

    assert(Objects.size()==SubElements.size());
    assert(externalGeoRef.size() == Objects.size());

    for(auto &key : externalGeoRef) {
        if (*itObj == Obj  &&  *itSub == SubName){
            FC_ERR("Duplicate external element reference in " << getFullName() << ": " << key);
            return -1;
        }
        // detach old reference
        if(detached.count(key)) {
            itObj = Objects.erase(itObj);
            itSub = SubElements.erase(itSub);
        }else{
            ++itObj;
            ++itSub;
        }
    }

    // add the new ones
    Objects.push_back(Obj);
    SubElements.push_back(std::string(SubName));

    ExternalGeometry.setValues(Objects,SubElements);
    if(externalGeoRef.size()!=Objects.size())
        return -1;

    std::string ref = externalGeoRef.back();
    for(auto geoId : idSet) {
        auto &geo = geos[-geoId-1];
        geo = geo->clone();
        ExternalGeometryFacade::getFacade(geo)->setRef(ref);
    }

    ExternalGeo.setValues(std::move(geos));
    rebuildExternalGeometry();
    return ExternalGeometry.getSize()-1;
}

std::vector<int> SketchObject::getRelatedGeometry(int GeoId) const {
    std::vector<int> res;
    if(GeoId>GeoEnum::RefExt || -GeoId-1>=ExternalGeo.getSize())
        return res;
    auto geo = getGeometry(GeoId);
    if(!geo)
        return res;
    const std::string &ref = ExternalGeometryFacade::getFacade(geo)->getRef();
    if(!ref.size())
       return {GeoId};
    auto iter = externalGeoRefMap.find(ref);
    if(iter == externalGeoRefMap.end())
        return {GeoId};
    for(auto id : iter->second) {
        auto it = externalGeoMap.find(id);
        if(it!=externalGeoMap.end())
            res.push_back(-it->second-1);
    }
    return res;
}

int SketchObject::syncGeometry(const std::vector<int> &geoIds) {
    bool touched = false;
    auto geos = ExternalGeo.getValues();
    std::set<int> idSet;
    for(int geoId : geoIds) {
        auto geo = getGeometry(geoId);
        if(!geo || !ExternalGeometryFacade::getFacade(geo)->testFlag(ExternalGeometryExtension::Frozen))
            continue;
        for(int gid : getRelatedGeometry(geoId))
            idSet.insert(gid);
    }
    for(int geoId : idSet) {
        if(geoId <= GeoEnum::RefExt && -geoId-1 < ExternalGeo.getSize()) {
            auto &geo = geos[-geoId-1];
            geo = geo->clone();
            ExternalGeometryFacade::getFacade(geo)->setFlag(ExternalGeometryExtension::Sync);
            touched = true;
        }
    }
    if(touched)
        ExternalGeo.setValues(std::move(geos));
    return 0;
}

const Part::Geometry* SketchObject::_getGeometry(int GeoId) const
{
    if (GeoId >= 0) {
        const std::vector<Part::Geometry *> &geomlist = getInternalGeometry();
        if (GeoId < int(geomlist.size()))
            return geomlist[GeoId];
    }
    else if (-GeoId-1 < ExternalGeo.getSize()) {
        return ExternalGeo[-GeoId-1];
    }

    return nullptr;
}

int SketchObject::getCompleteGeometryIndex(int GeoId) const
{
    if (GeoId >= 0) {
        if (GeoId < int(Geometry.getSize()))
            return GeoId;
    }
    else if (-GeoId <= int(ExternalGeo.getSize()))
        return -GeoId - 1;

    return GeoEnum::GeoUndef;
}

int SketchObject::getGeoIdFromCompleteGeometryIndex(int completeGeometryIndex) const
{
    int completeGeometryCount = int(Geometry.getSize() + ExternalGeo.getSize());

    if (completeGeometryIndex < 0 || completeGeometryIndex >= completeGeometryCount)
        return GeoEnum::GeoUndef;

    if (completeGeometryIndex < Geometry.getSize())
        return completeGeometryIndex;
    else
        return (completeGeometryIndex - completeGeometryCount);
}
int SketchObject::getSingleScaleDefiningConstraint() const
{
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    int found = -1;
    for (size_t i = 0; i < vals.size(); ++i) {
        // An angle does not define scale
        if (vals[i]->isDimensional() && vals[i]->Type != Angle) {
            if (found != -1) { // More than one scale defining constraint
                return -1;
            }
            found = i;
        }
    }
    return found;
}

std::unique_ptr<const GeometryFacade> SketchObject::getGeometryFacade(int GeoId) const
{
    return GeometryFacade::getFacade(getGeometry(GeoId));
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

namespace {

// Auxiliary Method: returns vector projection in UV space of plane
static gp_Vec2d ProjVecOnPlane_UV(const gp_Vec& V, const gp_Pln& Pl)
{
    return gp_Vec2d(V.Dot(Pl.Position().XDirection()), V.Dot(Pl.Position().YDirection()));
}

// Auxiliary Method: returns vector projection in UVN space of plane
static gp_Vec ProjVecOnPlane_UVN(const gp_Vec& V, const gp_Pln& Pl)
{
    gp_Vec2d vector = ProjVecOnPlane_UV(V, Pl);
    return gp_Vec(vector.X(), vector.Y(), 0.0);
}


// Auxiliary Method: returns point projection in UV space of plane
static gp_Vec2d ProjPointOnPlane_UV(const gp_Pnt& P, const gp_Pln& Pl)
{
    gp_Vec OP = gp_Vec(Pl.Location(), P);
    return ProjVecOnPlane_UV(OP, Pl);
}

// Auxiliary Method: returns point projection in UVN space of plane
static gp_Vec ProjPointOnPlane_UVN(const gp_Pnt& P, const gp_Pln& Pl)
{
    gp_Vec2d vec2 = ProjPointOnPlane_UV(P, Pl);
    return gp_Vec(vec2.X(), vec2.Y(), 0.0);
}

// Auxiliary Method: returns point projection in XYZ space
static gp_Pnt ProjPointOnPlane_XYZ(const gp_Pnt& P, const gp_Pln& Pl)
{
    gp_Vec positionUVN = ProjPointOnPlane_UVN(P, Pl);
    return gp_Pnt((positionUVN.X() * Pl.Position().XDirection()
                   + positionUVN.Y() * Pl.Position().YDirection() + gp_Vec(Pl.Location().XYZ()))
                      .XYZ());
}

// Auxiliary method
Part::Geometry* projectLine(const BRepAdaptor_Curve& curve, const Handle(Geom_Plane) & gPlane,
                            const Base::Placement& invPlm)
{
    double first = curve.FirstParameter();

    if (fabs(first) > 1E99) {
        // TODO: What is OCE's definition of Infinite?
        // TODO: The clean way to do this is to handle a new sketch geometry Geom::Line
        // but its a lot of work to implement...
        first = -10000;
    }

    double last = curve.LastParameter();
    if (fabs(last) > 1E99) {
        last = +10000;
    }

    gp_Pnt P1 = curve.Value(first);
    gp_Pnt P2 = curve.Value(last);

    GeomAPI_ProjectPointOnSurf proj1(P1, gPlane);
    P1 = proj1.NearestPoint();
    GeomAPI_ProjectPointOnSurf proj2(P2, gPlane);
    P2 = proj2.NearestPoint();

    Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());
    Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());
    invPlm.multVec(p1, p1);
    invPlm.multVec(p2, p2);

    if (Base::Distance(p1, p2) < Precision::Confusion()) {
        Base::Vector3d p = (p1 + p2) / 2;
        Part::GeomPoint* point = new Part::GeomPoint(p);
        GeometryFacade::setConstruction(point, true);
        return point;
    }
    else {
        Part::GeomLineSegment* line = new Part::GeomLineSegment();
        line->setPoints(p1, p2);
        GeometryFacade::setConstruction(line, true);
        return line;
    }
}

}  // anonymous namespace

bool SketchObject::evaluateSupport()
{
    // returns false if the shape is broken, null or non-planar
    App::DocumentObject* link = AttachmentSupport.getValue();
    if (!link || !link->isDerivedFrom<Part::Feature>())
        return false;
    return true;
}

static Part::Geometry *fitArcs(std::vector<std::unique_ptr<Part::Geometry> > &arcs,
                               const gp_Pnt &P1,
                               const gp_Pnt &P2,
                               double tol)
{
    double radius = 0.0;
    double m = 0.0;
    Base::Vector3d center;
    for (auto &geo : arcs) {
        if (auto arc = freecad_cast<Part::GeomArcOfCircle*>(geo.get())) {
            if (radius == 0.0) {
                radius = arc->getRadius();
                center = arc->getCenter();
                double f = arc->getFirstParameter();
                double l = arc->getLastParameter();
                m = (l-f)*0.5 + f; // middle parameter
            } else if (std::abs(radius - arc->getRadius()) > tol)
                return nullptr;
        } else
            return nullptr;
    }
    if (radius == 0.0) {
        return nullptr;
    }
    if (P1.SquareDistance(P2) < Precision::Confusion()) {
        Part::GeomCircle* circle = new Part::GeomCircle();
        circle->setCenter(center);
        circle->setRadius(radius);
        return circle;
    }
    if (arcs.size() == 1) {
        auto res = arcs.front().release();
        arcs.clear();
        return res;
    }

    GeomLProp_CLProps prop(Handle(Geom_Curve)::DownCast(arcs.front()->handle()),m,0,Precision::Confusion());
    gp_Pnt midPoint = prop.Value();
    GC_MakeArcOfCircle arc(P1, midPoint, P2);
    auto geo = new Part::GeomArcOfCircle();
    geo->setHandle(arc.Value());
    return geo;
}

void SketchObject::validateExternalLinks()
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

    bool rebuild = false;

    for (int i = 0; i < int(Objects.size()); i++) {
        const App::DocumentObject* Obj = Objects[i];
        const std::string SubElement = SubElements[i];

        TopoDS_Shape refSubShape;
        bool removeBadLink = false;
        try {
            if (Obj->isDerivedFrom<Part::Datum>()) {
                const Part::Datum* datum = static_cast<const Part::Datum*>(Obj);
                refSubShape = datum->getShape();
            }
            else if (Obj->isDerivedFrom<App::DatumElement>()) {
                // do nothing - shape will be calculated later during rebuild
            }
            else {
                const Part::Feature* refObj = static_cast<const Part::Feature*>(Obj);
                const Part::TopoShape& refShape = refObj->Shape.getShape();
                refSubShape = refShape.getSubShape(SubElement.c_str());
            }
        }
        catch (Base::IndexError& indexError) {
            removeBadLink = true;
            Base::Console().warning(
                this->getFullLabel(), (indexError.getMessage() + "\n").c_str());
        }
        catch (Base::ValueError& valueError) {
            removeBadLink = true;
            Base::Console().warning(
                this->getFullLabel(), (valueError.getMessage() + "\n").c_str());
        }
        catch (Standard_Failure&) {
            removeBadLink = true;
        }
        if (removeBadLink) {
            rebuild = true;
            Objects.erase(Objects.begin() + i);
            SubElements.erase(SubElements.begin() + i);

            const std::vector<Constraint*>& constraints = Constraints.getValues();
            std::vector<Constraint*> newConstraints(0);
            int GeoId = GeoEnum::RefExt - i;
            for (const auto& constr : constraints) {
                auto newConstr = getConstraintAfterDeletingGeo(constr, GeoId);
                if (newConstr) {
                    newConstraints.push_back(newConstr.release());
                }
            }

            Constraints.setValues(std::move(newConstraints));
            i--;// we deleted an item, so the next one took its place
        }
    }

    if (rebuild) {
        ExternalGeometry.setValues(Objects, SubElements);
        rebuildExternalGeometry();
        acceptGeometry();// This may need to be refactor to OnChanged for ExternalGeo
        solve(true);     // we have to update this sketch and everything depending on it.
    }
}

namespace {

void adjustParameterRange(const TopoDS_Edge &edge,
                                 Handle(Geom_Plane) gPlane,
                                 const gp_Trsf &mov,
                                 Handle(Geom_Curve) curve,
                                 double &firstParameter,
                                 double &lastParameter)
{
    // This function is to deal with the ambiguity of trimming a periodic
    // curve, e.g. given two points on a circle, whether to get the upper or
    // lower arc. Because projection orientation may swap the first and last
    // parameter of the original curve.
    //
    // We project the middle point of the original curve to the projected curve
    // to decide whether to flip the parameters.

    Handle(Geom_Curve) origCurve = BRepAdaptor_Curve(edge).Curve().Curve();

    // GeomAPI_ProjectPointOnCurve will project a point to an untransformed
    // curve, so make sure to obtain the point on an untransformed edge.
    auto e = edge.Located(TopLoc_Location());

    gp_Pnt firstPoint = BRep_Tool::Pnt(TopExp::FirstVertex(TopoDS::Edge(e)));
    double f = GeomAPI_ProjectPointOnCurve(firstPoint, origCurve).LowerDistanceParameter();

    gp_Pnt lastPoint = BRep_Tool::Pnt(TopExp::LastVertex(TopoDS::Edge(e)));
    double l = GeomAPI_ProjectPointOnCurve(lastPoint, origCurve).LowerDistanceParameter();

    auto adjustPeriodic = [](Handle(Geom_Curve) curve, double &f, double &l) {
        // Copied from Geom_TrimmedCurve::setTrim()
        if (curve->IsPeriodic()) {
            Standard_Real Udeb = curve->FirstParameter();
            Standard_Real Ufin = curve->LastParameter();
            // set f in the range Udeb , Ufin
            // set l in the range f , f + Period()
            ElCLib::AdjustPeriodic(Udeb, Ufin,
                    std::min(std::abs(f-l)/2,Precision::PConfusion()),
                    f, l);
        }
    };

    // Adjust for periodic curve to deal with orientation
    adjustPeriodic(origCurve, f, l);

    // Obtain the middle parameter in order to get the mid point of the arc
    double m = (l - f) * 0.5 + f;
    GeomLProp_CLProps prop(origCurve,m,0,Precision::Confusion());
    gp_Pnt midPoint = prop.Value();

    // Transform all three points to the world coordinate
    auto trsf = edge.Location().Transformation();
    midPoint.Transform(trsf);
    firstPoint.Transform(trsf);
    lastPoint.Transform(trsf);

    // Project the points to the sketch plane. Note the coordinates are still
    // in world coordinate system.
    gp_Pnt pm = GeomAPI_ProjectPointOnSurf(midPoint, gPlane).NearestPoint();
    gp_Pnt pf = GeomAPI_ProjectPointOnSurf(firstPoint, gPlane).NearestPoint();
    gp_Pnt pl = GeomAPI_ProjectPointOnSurf(lastPoint, gPlane).NearestPoint();

    // Transform the projected points to sketch plane local coordinates
    pm.Transform(mov);
    pf.Transform(mov);
    pl.Transform(mov);

    // Obtain the corresponding parameters for those points in the projected curve
    double f2 = GeomAPI_ProjectPointOnCurve(pf, curve).LowerDistanceParameter();
    double l2 = GeomAPI_ProjectPointOnCurve(pl, curve).LowerDistanceParameter();
    double m2 = GeomAPI_ProjectPointOnCurve(pm, curve).LowerDistanceParameter();

    firstParameter = f2;
    lastParameter = l2;

    adjustPeriodic(curve, f2, l2);
    adjustPeriodic(curve, f2, m2);
    // If the middle point is out of range, it means we need to choose the
    // other half of the arc.
    if (m2 > l2){
        std::swap(firstParameter, lastParameter);
    }
}

void processEdge2(TopoDS_Edge& projEdge, std::vector<std::unique_ptr<Part::Geometry>>& geos)
{
    BRepAdaptor_Curve projCurve(projEdge);
    if (projCurve.GetType() == GeomAbs_Line) {
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());
        Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());
        Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());

        if (Base::Distance(p1, p2) < Precision::Confusion()) {
            Base::Vector3d p = (p1 + p2) / 2;
            auto* point = new Part::GeomPoint(p);
            GeometryFacade::setConstruction(point, true);
            geos.emplace_back(point);
        }
        else {
            auto* line = new Part::GeomLineSegment();
            line->setPoints(p1, p2);
            GeometryFacade::setConstruction(line, true);
            geos.emplace_back(line);
        }
    }
    else if (projCurve.GetType() == GeomAbs_Circle) {
        gp_Circ c = projCurve.Circle();
        gp_Pnt p = c.Location();
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

        if (P1.SquareDistance(P2) < Precision::Confusion()) {
            auto* circle = new Part::GeomCircle();
            circle->setRadius(c.Radius());
            circle->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));

            GeometryFacade::setConstruction(circle, true);
            geos.emplace_back(circle);
        }
        else {
            auto* arc = new Part::GeomArcOfCircle();
            Handle(Geom_Curve) curve = new Geom_Circle(c);
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve,
                    projCurve.FirstParameter(),
                    projCurve.LastParameter());
            arc->setHandle(tCurve);
            GeometryFacade::setConstruction(arc, true);
            geos.emplace_back(arc);
        }
    }
    else if (projCurve.GetType() == GeomAbs_BSplineCurve) {
        // Unfortunately, a normal projection of a circle can also give
        // a Bspline Split the spline into arcs
        GeomConvert_BSplineCurveKnotSplitting bSplineSplitter(projCurve.BSpline(), 2);
        auto* bspline = new Part::GeomBSplineCurve(projCurve.BSpline());
        GeometryFacade::setConstruction(bspline, true);
        geos.emplace_back(bspline);
    }
    else if (projCurve.GetType() == GeomAbs_Hyperbola) {
        gp_Hypr e = projCurve.Hyperbola();
        gp_Pnt p = e.Location();
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

        gp_Dir normal = e.Axis().Direction();
        gp_Dir xdir = e.XAxis().Direction();
        gp_Ax2 xdirref(p, normal);

        if (P1.SquareDistance(P2) < Precision::Confusion()) {
            auto* hyperbola = new Part::GeomHyperbola();
            hyperbola->setMajorRadius(e.MajorRadius());
            hyperbola->setMinorRadius(e.MinorRadius());
            hyperbola->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));
            hyperbola->setAngleXU(-xdir.AngleWithRef(xdirref.XDirection(), normal));
            GeometryFacade::setConstruction(hyperbola, true);
            geos.emplace_back(hyperbola);
        }
        else {
            auto* aoh = new Part::GeomArcOfHyperbola();
            Handle(Geom_Curve) curve = new Geom_Hyperbola(e);
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve,
                    projCurve.FirstParameter(),
                    projCurve.LastParameter());
            aoh->setHandle(tCurve);
            GeometryFacade::setConstruction(aoh, true);
            geos.emplace_back(aoh);
        }
    }
    else if (projCurve.GetType() == GeomAbs_Parabola) {
        gp_Parab e = projCurve.Parabola();
        gp_Pnt p = e.Location();
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

        gp_Dir normal = e.Axis().Direction();
        gp_Dir xdir = e.XAxis().Direction();
        gp_Ax2 xdirref(p, normal);

        if (P1.SquareDistance(P2) < Precision::Confusion()) {
            auto* parabola = new Part::GeomParabola();
            parabola->setFocal(e.Focal());
            parabola->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));
            parabola->setAngleXU(-xdir.AngleWithRef(xdirref.XDirection(), normal));
            GeometryFacade::setConstruction(parabola, true);
            geos.emplace_back(parabola);
        }
        else {
            auto* aop = new Part::GeomArcOfParabola();
            Handle(Geom_Curve) curve = new Geom_Parabola(e);
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve,
                    projCurve.FirstParameter(),
                    projCurve.LastParameter());
            aop->setHandle(tCurve);
            GeometryFacade::setConstruction(aop, true);
            geos.emplace_back(aop);
        }
    }
    else if (projCurve.GetType() == GeomAbs_Ellipse) {
        gp_Elips e = projCurve.Ellipse();
        gp_Pnt p = e.Location();
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

        gp_Dir normal = gp_Dir(0, 0, 1);
        gp_Ax2 xdirref(p, normal);

        if (P1.SquareDistance(P2) < Precision::Confusion()) {
            auto* ellipse = new Part::GeomEllipse();
            Handle(Geom_Ellipse) curve = new Geom_Ellipse(e);
            ellipse->setHandle(curve);
            GeometryFacade::setConstruction(ellipse, true);
            geos.emplace_back(ellipse);
        }
        else {
            auto* aoe = new Part::GeomArcOfEllipse();
            Handle(Geom_Curve) curve = new Geom_Ellipse(e);
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve,
                    projCurve.FirstParameter(),
                    projCurve.LastParameter());
            aoe->setHandle(tCurve);
            GeometryFacade::setConstruction(aoe, true);
            geos.emplace_back(aoe);
        }
    }
    else {
        throw Base::NotImplementedError("Not yet supported geometry for external geometry");
    }
}

void processEdge(const TopoDS_Edge& edge,
                 std::vector<std::unique_ptr<Part::Geometry>>& geos,
                 const Handle(Geom_Plane)& gPlane,
                 const Base::Placement& invPlm,
                 const gp_Trsf& mov,
                 const gp_Pln& sketchPlane,
                 const Base::Rotation& invRot,
                 gp_Ax3& sketchAx3,
                 TopoDS_Shape& aProjFace)
{
    using std::numbers::pi;

    BRepAdaptor_Curve curve(edge);
    if (curve.GetType() == GeomAbs_Line) {
        geos.emplace_back(projectLine(curve, gPlane, invPlm));
    }
    else if (curve.GetType() == GeomAbs_Circle) {
        gp_Dir vec1 = sketchPlane.Axis().Direction();
        gp_Dir vec2 = curve.Circle().Axis().Direction();

        // start point of arc of circle
        gp_Pnt beg = curve.Value(curve.FirstParameter());
        // end point of arc of circle
        gp_Pnt end = curve.Value(curve.LastParameter());

        if (vec1.IsParallel(vec2, Precision::Confusion())) {
            gp_Circ circle = curve.Circle();
            gp_Pnt cnt = circle.Location();

            GeomAPI_ProjectPointOnSurf proj(cnt, gPlane);
            cnt = proj.NearestPoint();
            circle.SetLocation(cnt);
            cnt.Transform(mov);
            circle.Transform(mov);

            if (beg.SquareDistance(end) < Precision::Confusion()) {
                auto* gCircle = new Part::GeomCircle();
                gCircle->setRadius(circle.Radius());
                gCircle->setCenter(Base::Vector3d(cnt.X(), cnt.Y(), cnt.Z()));

                GeometryFacade::setConstruction(gCircle, true);
                geos.emplace_back(gCircle);
            }
            else {
                auto* gArc = new Part::GeomArcOfCircle();
                Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
                Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(
                    hCircle, curve.FirstParameter(), curve.LastParameter());
                gArc->setHandle(tCurve);
                GeometryFacade::setConstruction(gArc, true);
                geos.emplace_back(gArc);
            }
        }
        else {
            // creates an ellipse or a segment
            gp_Circ origCircle = curve.Circle();

            if (vec1.IsNormal(vec2, Precision::Angular())) {
                // circle's normal vector in plane:
                // projection is a line
                // define center by projection
                gp_Pnt cnt = origCircle.Location();
                GeomAPI_ProjectPointOnSurf proj(cnt, gPlane);
                cnt = proj.NearestPoint();

                gp_Dir dirOrientation = gp_Dir(vec1 ^ vec2);
                gp_Dir dirLine(dirOrientation);

                auto* projectedSegment = new Part::GeomLineSegment();
                Geom_Line ligne(cnt, dirLine);// helper object to compute end points
                gp_Pnt P1, P2;                // end points of the segment, OCC style

                ligne.D0(-origCircle.Radius(), P1);
                ligne.D0(origCircle.Radius(), P2);

                if (!curve.IsClosed()) {// arc of circle
                    double alpha =
                        dirOrientation.AngleWithRef(curve.Circle().XAxis().Direction(),
                            curve.Circle().Axis().Direction());

                    double baseAngle = curve.FirstParameter();

                    int tours = 0;
                    double startAngle = baseAngle + alpha;
                    // bring startAngle back in [-pi/2 , 3pi/2[
                    while (startAngle < -pi / 2.0 && tours < 10) {
                        startAngle = baseAngle + ++tours * 2.0 * pi + alpha;
                    }
                    while (startAngle >= 3.0 * pi / 2.0 && tours > -10) {
                        startAngle = baseAngle + --tours * 2.0 * pi + alpha;
                    }

                    // apply same offset to end angle
                    double endAngle = curve.LastParameter() + startAngle - baseAngle;

                    if (startAngle <= 0.0) {
                        if (endAngle <= 0.0) {
                            P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                        }
                        else {
                            if (endAngle <= fabs(startAngle)) {
                                // P2 = P2 already defined
                                P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            }
                            else if (endAngle < pi) {
                                // P2 = P2, already defined
                                P1 = ProjPointOnPlane_XYZ(end, sketchPlane);
                            }
                            else {
                                // P1 = P1, already defined
                                // P2 = P2, already defined
                            }
                        }
                    }
                    else if (startAngle < pi) {
                        if (endAngle < pi) {
                            P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                        }
                        else if (endAngle < 2.0 * pi - startAngle) {
                            P2 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            // P1 = P1, already defined
                        }
                        else if (endAngle < 2.0 * pi) {
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                            // P1 = P1, already defined
                        }
                        else {
                            // P1 = P1, already defined
                            // P2 = P2, already defined
                        }
                    }
                    else {
                        if (endAngle < 2 * pi) {
                            P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                        }
                        else if (endAngle < 4 * pi - startAngle) {
                            P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            // P2 = P2, already defined
                        }
                        else if (endAngle < 3 * pi) {
                            // P1 = P1, already defined
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                        }
                        else {
                            // P1 = P1, already defined
                            // P2 = P2, already defined
                        }
                    }
                }

                Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());// ends of segment FCAD style
                Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());
                invPlm.multVec(p1, p1);
                invPlm.multVec(p2, p2);

                projectedSegment->setPoints(p1, p2);
                GeometryFacade::setConstruction(projectedSegment, true);
                geos.emplace_back(projectedSegment);
            }
            else {// general case, full circle or arc of circle
                gp_Pnt cnt = origCircle.Location();
                GeomAPI_ProjectPointOnSurf proj(cnt, gPlane);
                // projection of circle center on sketch plane, 3D space
                cnt = proj.NearestPoint();
                // converting to FCAD style vector
                Base::Vector3d p(cnt.X(), cnt.Y(), cnt.Z());
                // transforming towards sketch's (x,y) coordinates
                invPlm.multVec(p, p);


                gp_Vec vecMajorAxis = vec1 ^ vec2;// major axis in 3D space

                double minorRadius;// TODO use data type of vectors around...
                double cosTheta;
                // cos of angle between the two planes, assuming vectirs are normalized
                // to 1
                cosTheta = fabs(vec1.Dot(vec2));
                minorRadius = origCircle.Radius() * cosTheta;

                // maj axis into FCAD style vector
                Base::Vector3d vectorMajorAxis(
                    vecMajorAxis.X(), vecMajorAxis.Y(), vecMajorAxis.Z());
                // transforming to sketch's (x,y) coordinates
                invRot.multVec(vectorMajorAxis, vectorMajorAxis);
                // back to OCC
                vecMajorAxis.SetXYZ(
                    gp_XYZ(vectorMajorAxis[0], vectorMajorAxis[1], vectorMajorAxis[2]));

                // NB: force normal of ellipse to be normal of sketch's plane.
                gp_Ax2 refFrameEllipse(
                    gp_Pnt(gp_XYZ(p[0], p[1], p[2])), gp_Vec(0, 0, 1), vecMajorAxis);

                gp_Elips elipsDest;
                elipsDest.SetPosition(refFrameEllipse);
                elipsDest.SetMajorRadius(origCircle.Radius());
                elipsDest.SetMinorRadius(minorRadius);

                Handle(Geom_Ellipse) projCurve = new Geom_Ellipse(elipsDest);

                if (beg.SquareDistance(end) < Precision::Confusion()) {
                    // projection is an ellipse
                    auto* ellipse = new Part::GeomEllipse();
                    ellipse->setHandle(projCurve);
                    GeometryFacade::setConstruction(ellipse, true);
                    geos.emplace_back(ellipse);
                }
                else {
                    // projection is an arc of ellipse
                    auto* aoe = new Part::GeomArcOfEllipse();
                    double firstParam, lastParam;
                    // adjust the parameter range to get the correct arc
                    adjustParameterRange(edge, gPlane, mov, projCurve, firstParam, lastParam);

                    Handle(Geom_TrimmedCurve) trimmedCurve = new Geom_TrimmedCurve(projCurve, firstParam, lastParam);
                    aoe->setHandle(trimmedCurve);
                    GeometryFacade::setConstruction(aoe, true);
                    geos.emplace_back(aoe);
                }
            }
        }
    }
    else if (curve.GetType() == GeomAbs_Ellipse) {

        gp_Pnt P1 = curve.Value(curve.FirstParameter());
        gp_Pnt P2 = curve.Value(curve.LastParameter());
        gp_Elips elipsOrig = curve.Ellipse();
        gp_Elips elipsDest;
        gp_Pnt origCenter = elipsOrig.Location();
        gp_Pnt destCenter = ProjPointOnPlane_UVN(origCenter, sketchPlane).XYZ();

        gp_Dir origAxisMajorDir = elipsOrig.XAxis().Direction();
        gp_Vec origAxisMajor = elipsOrig.MajorRadius() * gp_Vec(origAxisMajorDir);
        gp_Dir origAxisMinorDir = elipsOrig.YAxis().Direction();
        gp_Vec origAxisMinor = elipsOrig.MinorRadius() * gp_Vec(origAxisMinorDir);

        // Here, it used to be a test for parallel direction between the sketchplane and
        // the elipsOrig, in which the original ellipse would be copied and translated
        // to the new position. The problem with that approach is that for the sketcher
        // the normal vector is always (0,0,1). If the original ellipse was not on the
        // XY plane, the copy will not be either. Then, the dimensions would be wrong
        // because of the different major axis direction (which is not projected on the
        // XY plane). So here, we default to the more general ellipse construction
        // algorithm.
        //
        // Doing that solves:
        // https://forum.freecad.org/viewtopic.php?f=3&t=55284#p477522

        // GENERAL ELLIPSE CONSTRUCTION ALGORITHM
        //
        // look for major axis of projected ellipse
        //
        // t is the parameter along the origin ellipse
        //   OM(t) = origCenter
        //           + majorRadius * cos(t) * origAxisMajorDir
        //           + minorRadius * sin(t) * origAxisMinorDir
        gp_Vec2d PA = ProjVecOnPlane_UV(origAxisMajor, sketchPlane);
        gp_Vec2d PB = ProjVecOnPlane_UV(origAxisMinor, sketchPlane);
        double t_max = 2.0 * PA.Dot(PB) / (PA.SquareMagnitude() - PB.SquareMagnitude());
        t_max = 0.5 * atan(t_max);// gives new major axis is most cases, but not all
        double t_min = t_max + 0.5 * pi;

        // ON_max = OM(t_max) gives the point, which projected on the sketch plane,
        //     becomes the apoapse of the projected ellipse.
        gp_Vec ON_max = origAxisMajor * cos(t_max) + origAxisMinor * sin(t_max);
        gp_Vec ON_min = origAxisMajor * cos(t_min) + origAxisMinor * sin(t_min);
        gp_Vec destAxisMajor = ProjVecOnPlane_UVN(ON_max, sketchPlane);
        gp_Vec destAxisMinor = ProjVecOnPlane_UVN(ON_min, sketchPlane);

        double RDest = destAxisMajor.Magnitude();
        double rDest = destAxisMinor.Magnitude();

        if (RDest < rDest) {
            double rTmp = rDest;
            rDest = RDest;
            RDest = rTmp;
            gp_Vec axisTmp = destAxisMajor;
            destAxisMajor = destAxisMinor;
            destAxisMinor = axisTmp;
        }

        double sens = sketchAx3.Direction().Dot(elipsOrig.Position().Direction());
        int flip = sens > 0.0 ? 1.0 : -1.0;
        gp_Ax2 destCurveAx2(destCenter, gp_Dir(0, 0, flip), gp_Dir(destAxisMajor));

        // projection is a circle
        if ((RDest - rDest) < (double)Precision::Confusion()) {
            Handle(Geom_Circle) projCurve = new Geom_Circle(destCurveAx2, 0.5 * (rDest + RDest));
            if (P1.SquareDistance(P2) < Precision::Confusion()) {
                auto* circle = new Part::GeomCircle();
                circle->setHandle(projCurve);
                GeometryFacade::setConstruction(circle, true);
                geos.emplace_back(circle);
            }
            else {
                auto* arc = new Part::GeomArcOfCircle();
                double firstParam, lastParam;
                adjustParameterRange(edge, gPlane, mov, projCurve, firstParam, lastParam);
                Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(projCurve, firstParam, lastParam);
                arc->setHandle(tCurve);
                GeometryFacade::setConstruction(arc, true);
                geos.emplace_back(arc);
            }
        }
        else {
            if (sketchPlane.Position().Direction().IsNormal(
                elipsOrig.Position().Direction(), Precision::Angular())) {
                gp_Vec start = gp_Vec(destCenter.XYZ()) + destAxisMajor;
                gp_Vec end = gp_Vec(destCenter.XYZ()) - destAxisMajor;

                auto* projectedSegment = new Part::GeomLineSegment();
                projectedSegment->setPoints(
                    Base::Vector3d(start.X(), start.Y(), start.Z()),
                    Base::Vector3d(end.X(), end.Y(), end.Z()));
                GeometryFacade::setConstruction(projectedSegment, true);
                geos.emplace_back(projectedSegment);
            }
            else {

                elipsDest.SetPosition(destCurveAx2);
                elipsDest.SetMajorRadius(destAxisMajor.Magnitude());
                elipsDest.SetMinorRadius(destAxisMinor.Magnitude());

                Handle(Geom_Ellipse) projCurve = new Geom_Ellipse(elipsDest);

                if (P1.SquareDistance(P2) < Precision::Confusion()) {
                    auto* ellipse = new Part::GeomEllipse();
                    ellipse->setHandle(projCurve);
                    GeometryFacade::setConstruction(ellipse, true);
                    geos.emplace_back(ellipse);
                }
                else {
                    auto* aoe = new Part::GeomArcOfEllipse();
                    double firstParam, lastParam;
                    adjustParameterRange(edge, gPlane, mov, projCurve, firstParam, lastParam);

                    Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(projCurve, firstParam, lastParam);
                    aoe->setHandle(tCurve);
                    GeometryFacade::setConstruction(aoe, true);
                    geos.emplace_back(aoe);
                }
            }
        }
    }
    else {
        gp_Pln plane;
        auto shape = Part::TopoShape(edge);
        bool planar = shape.findPlane(plane);

        // Check if the edge is planar and plane is perpendicular to the projection plane
        if (planar && plane.Axis().IsNormal(sketchPlane.Axis(), Precision::Angular())) {
            // Project an edge to a line. Only works if the edge is planar and its plane is
            // perpendicular to the projection plane. OCC has trouble handling
            // BSpline projection to a straight line. Although it does correctly projects
            // the line including extreme bounds (not always a case), it will produce a BSpline with degree
            // more than one.
            //
            // The work around here is to use an aligned bounding box of the edge to get
            // the projection of the extremum points to construct the projected line.

            // First, transform the shape to the projection plane local coordinates.
            shape.setPlacement(invPlm * shape.getPlacement());

            // Align the z axis of the edge plane to the y axis of the projection
            // plane,  so that the extreme bound will be a line in the x axis direction
            // of the projection plane.
            double angle = plane.Axis().Direction().Angle(sketchPlane.YAxis().Direction());

            gp_Trsf trsf;
            if (fabs(angle) > Precision::Angular()) {
                trsf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(0, 0, 1)), angle);
                shape.move(trsf);
            }

            // Make a copy to work around OCC circular edge transformation bug
            shape = shape.makeElementCopy();

            // Obtain the bounding box (precise version!) and move the extreme points back
            // to the original location
            auto bbox = shape.getBoundBoxOptimal();
            if (!bbox.IsValid()){
                throw Base::CADKernelError("Invalid bounding box");
            }

            gp_Pnt p1(bbox.MinX, bbox.MinY, 0);
            gp_Pnt p2(bbox.MaxX, bbox.MaxY, 0);
            if (fabs(angle) > Precision::Angular()) {
                trsf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(0, 0, 1)), -angle);
                p1.Transform(trsf);
                p2.Transform(trsf);
            }

            Base::Vector3d P1(p1.X(), p1.Y(), 0);
            Base::Vector3d P2(p2.X(), p2.Y(), 0);

            // check for degenerated case when the line is collapsed to a point
            if (p1.SquareDistance(p2) < Precision::SquareConfusion()) {
                Part::GeomPoint* point = new Part::GeomPoint((P1 + P2) / 2);
                GeometryFacade::setConstruction(point, true);
                geos.emplace_back(point);
            }
            else {
                auto* projectedSegment = new Part::GeomLineSegment();
                projectedSegment->setPoints(P1, P2);
                GeometryFacade::setConstruction(projectedSegment, true);
                geos.emplace_back(projectedSegment);
            }
        }
        else {
            try {
                Part::TopoShape projShape;
                // Projection of the edge on parallel plane to the sketch plane is edge itself
                // all we need to do is match coordinate systems
                // for some reason OCC doesn't like to project a planar B-Spline to a plane parallel to it
                if (planar && plane.Axis().Direction().IsParallel(sketchPlane.Axis().Direction(), Precision::Confusion())) {
                    TopoDS_Edge projEdge = edge;

                    // We need to trim the curve in case we are projecting a B-Spline segment
                    if(curve.GetType() == GeomAbs_BSplineCurve){
                        double Param1 = curve.FirstParameter();
                        double Param2 = curve.LastParameter();

                        if (Param1 > Param2){
                            std::swap(Param1, Param2);
                        }

                        // trim curve in case we are projecting a segment
                        auto bsplineCurve = curve.BSpline();
                        if(Param2 - Param1 > Precision::Confusion()){
                            bsplineCurve->Segment(Param1, Param2);
                            projEdge = BRepBuilderAPI_MakeEdge(bsplineCurve).Edge();
                        }
                    }

                    projShape.setShape(projEdge);

                    // We can't use gp_Pln::Distance() because we need to
                    // know which side the plane is regarding the sketch
                    const gp_Pnt& aP = sketchPlane.Location();
                    const gp_Pnt& aLoc = plane.Location ();
                    const gp_Dir& aDir = plane.Axis().Direction();
                    double d = (aDir.X() * (aP.X() - aLoc.X()) +
                            aDir.Y() * (aP.Y() - aLoc.Y()) +
                            aDir.Z() * (aP.Z() - aLoc.Z()));

                    gp_Trsf trsf;
                    trsf.SetTranslation(gp_Vec(aDir) * d);
                    projShape.transformShape(Part::TopoShape::convert(trsf), /*copy*/false);
                } else {
                    // When planes not parallel or perpendicular, or edge is not planar
                    // normal projection is working just fine
                    BRepOffsetAPI_NormalProjection mkProj(aProjFace);
                    mkProj.Add(edge);
                    mkProj.Build();

                    projShape.setShape(mkProj.Projection());
                }
                if (!projShape.isNull() && projShape.hasSubShape(TopAbs_EDGE)) {
                    for (auto &e : projShape.getSubTopoShapes(TopAbs_EDGE)) {
                        // Transform copy of the edge to the sketch plane local coordinates
                        e.transformShape(invPlm.toMatrix(), /*copy*/true, /*checkScale*/true);
                        TopoDS_Edge projEdge = TopoDS::Edge(e.getShape());
                        processEdge2(projEdge, geos);
                    }
                }
            }
            catch (Standard_Failure& e) {
                throw Base::CADKernelError(e.GetMessageString());
            }
        }
    }
}

std::vector<TopoDS_Shape> projectShape(const TopoDS_Shape& inShape, const gp_Ax3& viewAxis)
{
    std::vector<TopoDS_Shape> res;
    Handle(HLRBRep_Algo) brep_hlr;
    try {
        brep_hlr = new HLRBRep_Algo();
        brep_hlr->Add(inShape);

        gp_Trsf aTrsf;
        aTrsf.SetTransformation(viewAxis);
        HLRAlgo_Projector projector(aTrsf, false, 1);

        brep_hlr->Projector(projector);
        brep_hlr->Update();
        brep_hlr->Hide();
    }
    catch (const Standard_Failure& e) {
        Base::Console().error("GO::projectShape - OCC error - %s - while projecting shape\n",
            e.GetMessageString());
        throw Base::RuntimeError("SketchObject::projectShape - OCC error");
    }
    catch (...) {
        throw Base::RuntimeError("SketchObject::projectShape - unknown error");
    }

    try {
        HLRBRep_HLRToShape hlrToShape(brep_hlr);
        if (!hlrToShape.VCompound().IsNull()) {
            //TopAbs_COMPOUND to TopAbs_EDGE
            res.push_back(hlrToShape.VCompound());
        }

        if (!hlrToShape.Rg1LineVCompound().IsNull()) {
            res.push_back(hlrToShape.Rg1LineVCompound());
        }

        if (!hlrToShape.OutLineVCompound().IsNull()) {
            res.push_back(hlrToShape.OutLineVCompound());
        }

        if (!hlrToShape.IsoLineVCompound().IsNull()) {
            res.push_back(hlrToShape.IsoLineVCompound());
        }

        if (!hlrToShape.HCompound().IsNull()) {
            res.push_back(hlrToShape.HCompound());
        }

        if (!hlrToShape.Rg1LineHCompound().IsNull()) {
            res.push_back(hlrToShape.Rg1LineHCompound());
        }

        if (!hlrToShape.OutLineHCompound().IsNull()) {
            res.push_back(hlrToShape.OutLineHCompound());
        }

        if (!hlrToShape.IsoLineHCompound().IsNull()) {
            res.push_back(hlrToShape.IsoLineHCompound());
        }
    }
    catch (const Standard_Failure&) {
        throw Base::RuntimeError(
            "SketchObject::projectShape - OCC error occurred while extracting edges");
    }
    catch (...) {
        throw Base::RuntimeError(
            "SketchObject::projectShape - unknown error occurred while extracting edges");
    }

    return res;
}

void processFace (const Rotation& invRot,
                  const Placement& invPlm,
                  const gp_Trsf& mov,
                  const gp_Pln& sketchPlane,
                  const Handle(Geom_Plane)& gPlane,
                  gp_Ax3& sketchAx3,
                  TopoDS_Shape& aProjFace,
                  std::vector<std::unique_ptr<Part::Geometry>>& geos,
                  TopoDS_Shape& refSubShape)
{
    const TopoDS_Face& face = TopoDS::Face(refSubShape);
    BRepAdaptor_Surface surface(face);
    if (surface.GetType() == GeomAbs_Plane) {
        // Check that the plane is perpendicular to the sketch plane
        Geom_Plane plane = surface.Plane();
        gp_Dir dnormal = plane.Axis().Direction();
        gp_Dir snormal = sketchPlane.Axis().Direction();

        // Extract all edges from the face
        TopExp_Explorer edgeExp;
        for (edgeExp.Init(face, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(edgeExp.Current());
            // Process each edge
            processEdge(edge, geos, gPlane, invPlm, mov, sketchPlane, invRot, sketchAx3, aProjFace);
        }

        if (fabs(dnormal.Angle(snormal) - std::numbers::pi/2) < Precision::Confusion()) {
            // The face is normal to the sketch plane
            // We don't want to keep the projection of all the edges of the face.
            // We need a single line that goes from min to max of all the projections.
            bool initialized = false;
            Vector3d start, end;
            // Lambda to determine if a point should replace start or end
            auto updateExtremes = [&](const Vector3d& point) {
                if ((point - start).Length() < (point - end).Length()) {
                    // `point` is closer to `start` than `end`, check if it's further out than `start`
                    if ((point - end).Length() > (end - start).Length()) {
                        start = point;
                    }
                }
                else {
                    // `point` is closer to `end`, check if it's further out than `end`
                    if ((point - start).Length() > (end - start).Length()) {
                        end = point;
                    }
                }
            };
            for (auto& geo : geos) {
                auto* line = dynamic_cast<Part::GeomLineSegment*>(geo.get());
                if (!line) {
                    // The face being normal to the sketch, we should have
                    // only lines. This is just a fail-safe in case there's a
                    // straight bspline or something like this.
                    continue;
                }
                if (!initialized) {
                    start = line->getStartPoint();
                    end = line->getEndPoint();
                    initialized = true;
                    continue;
                }

                updateExtremes(line->getStartPoint());
                updateExtremes(line->getEndPoint());
            }
            if (initialized) {
                auto* unifiedLine = new Part::GeomLineSegment();
                unifiedLine->setPoints(start, end);
                geos.clear(); // Clear other segments
                geos.emplace_back(unifiedLine);
            }
            else {
                // In case we have not initialized, perhaps the projections were
                // only straight bsplines.
                // Then we use the old method that will give a line with 20000 length:
                // Get vector that is normal to both sketch plane normal and plane normal.
                // This is the line's direction
                gp_Dir lnormal = dnormal.Crossed(snormal);
                BRepBuilderAPI_MakeEdge builder(gp_Lin(plane.Location(), lnormal));
                builder.Build();
                if (builder.IsDone()) {
                    const TopoDS_Edge& edge = TopoDS::Edge(builder.Shape());
                    BRepAdaptor_Curve curve(edge);
                    if (curve.GetType() == GeomAbs_Line) {
                        geos.emplace_back(projectLine(curve, gPlane, invPlm));
                    }
                }
            }
        }
    }
    else {
        std::vector<TopoDS_Shape> res = projectShape(face, sketchAx3);
        for (auto& resShape : res) {
            TopExp_Explorer explorer(resShape, TopAbs_EDGE);
            while (explorer.More()) {
                TopoDS_Edge projEdge = TopoDS::Edge(explorer.Current());
                processEdge2(projEdge, geos);
                explorer.Next();
            }
        }
    }
}

}

void SketchObject::rebuildExternalGeometry(std::optional<ExternalToAdd> extToAdd)
{
    Base::StateLocker lock(managedoperation, true); // no need to check input data validity as this is an sketchobject managed operation.

    // Analyze the state of existing external geometries to infer the desired state for new ones.
    // If any geometry from a source link is "defining", we'll treat the whole link as "defining".
    std::map<std::string, bool> linkIsDefiningMap;
    for (const auto& geo : ExternalGeo.getValues()) {
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if (!egf->getRef().empty()) {
            bool isDefining = egf->testFlag(ExternalGeometryExtension::Defining);
            if (linkIsDefiningMap.find(egf->getRef()) == linkIsDefiningMap.end()) {
                linkIsDefiningMap[egf->getRef()] = isDefining;
            }
            else {
                linkIsDefiningMap[egf->getRef()] = linkIsDefiningMap[egf->getRef()] && isDefining;
            }
        }
    }

    // get the actual lists of the externals
    auto Types       = ExternalTypes.getValues();
    auto Objects     = ExternalGeometry.getValues();
    auto SubElements = ExternalGeometry.getSubValues();
    assert(externalGeoRef.size() == Objects.size());
    auto keys = externalGeoRef;

    // re-check for any missing geometry element. The code here has a side
    // effect that the linked external geometry will continue to work even if
    // ExternalGeometry is wiped out.
    for(auto &geo : ExternalGeo.getValues()) {
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if(egf->getRef().size() && egf->testFlag(ExternalGeometryExtension::Missing)) {
            const std::string &ref = egf->getRef();
            auto pos = ref.find('.');
            if(pos == std::string::npos)
                continue;
            std::string objName = ref.substr(0,pos);
            auto obj = getDocument()->getObject(objName.c_str());
            if(!obj)
                continue;
            App::ElementNamePair elementName;
            App::GeoFeature::resolveElement(obj,ref.c_str()+pos+1,elementName);
            if(elementName.oldName.size()
                    && !App::GeoFeature::hasMissingElement(elementName.oldName.c_str()))
            {
                Objects.push_back(obj);
                SubElements.push_back(elementName.oldName);
                keys.push_back(ref);
            }
        }
    }

    Base::Placement Plm = Placement.getValue();
    Base::Vector3d Pos = Plm.getPosition();
    Base::Rotation Rot = Plm.getRotation();
    Base::Rotation invRot = Rot.inverse();
    Base::Vector3d dN(0, 0, 1);
    Rot.multVec(dN, dN);
    Base::Vector3d dX(1, 0, 0);
    Rot.multVec(dX, dX);

    Base::Placement invPlm = Plm.inverse();
    Base::Matrix4D invMat = invPlm.toMatrix();
    gp_Trsf mov;
    mov.SetValues(invMat[0][0],
                  invMat[0][1],
                  invMat[0][2],
                  invMat[0][3],
                  invMat[1][0],
                  invMat[1][1],
                  invMat[1][2],
                  invMat[1][3],
                  invMat[2][0],
                  invMat[2][1],
                  invMat[2][2],
                  invMat[2][3]);

    gp_Ax3 sketchAx3(
        gp_Pnt(Pos.x, Pos.y, Pos.z), gp_Dir(dN.x, dN.y, dN.z), gp_Dir(dX.x, dX.y, dX.z));
    gp_Pln sketchPlane(sketchAx3);

    Handle(Geom_Plane) gPlane = new Geom_Plane(sketchPlane);
    BRepBuilderAPI_MakeFace mkFace(sketchPlane);
    TopoDS_Shape aProjFace = mkFace.Shape();

    Types.resize(Objects.size(), static_cast<long>(ExtType::Projection));

    std::set<std::string> refSet;
    // We use a vector here to keep the order (roughly) the same as ExternalGeometry
    std::vector<std::vector<std::unique_ptr<Part::Geometry> > > newGeos;
    newGeos.reserve(Objects.size());
    for (int i=0; i < int(Objects.size()); i++) {
        const App::DocumentObject *Obj=Objects[i];
        const std::string &SubElement=SubElements[i];
        const std::string &key = keys[i];

        bool beingCreated = false;
        if (extToAdd) {
            beingCreated = extToAdd->obj == Obj && extToAdd->subname == SubElement;
        }

        bool projection = Types[i] == (int)ExtType::Projection || Types[i] == (int)ExtType::Both;
        bool intersection = Types[i] == (int)ExtType::Intersection || Types[i] == (int)ExtType::Both;

        // Skip frozen geometries
        bool frozen = false;
        bool sync = false;
        for(auto id : externalGeoRefMap[key]) {
            auto it = externalGeoMap.find(id);
            if(it != externalGeoMap.end()) {
                auto egf = ExternalGeometryFacade::getFacade(ExternalGeo[it->second]);
                if(egf->testFlag(ExternalGeometryExtension::Frozen)) {
                    frozen = true;
                }
                if (egf->testFlag(ExternalGeometryExtension::Sync)) {
                    sync = true;
                }
            }
        }
        if(frozen && !sync) {
            refSet.insert(std::move(key));
            continue;
        }
        if (!Obj || !Obj->getNameInDocument()) {
            continue;
        }

        std::vector<std::unique_ptr<Part::Geometry> > geos;

        auto importVertex = [&](const TopoDS_Shape& refSubShape) {
            gp_Pnt P = BRep_Tool::Pnt(TopoDS::Vertex(refSubShape));
            GeomAPI_ProjectPointOnSurf proj(P, gPlane);
            P = proj.NearestPoint();
            Base::Vector3d p(P.X(), P.Y(), P.Z());
            invPlm.multVec(p, p);

            Part::GeomPoint* point = new Part::GeomPoint(p);
            GeometryFacade::setConstruction(point, true);
            geos.emplace_back(point);
        };

        try {
            TopoDS_Shape refSubShape;

            if (Obj->isDerivedFrom<Part::Datum>()) {
                auto* datum = static_cast<const Part::Datum*>(Obj);
                refSubShape = datum->getShape();
            }
            else if (Obj->isDerivedFrom<Part::Feature>()) {
                auto* refObj = static_cast<const Part::Feature*>(Obj);
                const Part::TopoShape& refShape = refObj->Shape.getShape();
                refSubShape = refShape.getSubShape(SubElement.c_str());
            }
            else if (Obj->isDerivedFrom<App::Plane>()) {
                auto* pl = static_cast<const App::Plane*>(Obj);
                Base::Placement plm = pl->Placement.getValue();
                Base::Vector3d base = plm.getPosition();
                Base::Rotation rot = plm.getRotation();
                Base::Vector3d normal(0, 0, 1);
                rot.multVec(normal, normal);
                gp_Pln plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
                BRepBuilderAPI_MakeFace fBuilder(plane);
                if (!fBuilder.IsDone())
                    throw Base::RuntimeError(
                        "Sketcher: addExternal(): Failed to build face from App::Plane");

                TopoDS_Face f = TopoDS::Face(fBuilder.Shape());
                refSubShape = f;
            }
            else if (Obj->isDerivedFrom<Part::DatumLine>()) {
                auto* line = static_cast<const Part::DatumLine*>(Obj);
                Base::Placement plm = line->Placement.getValue();
                Base::Vector3d base = plm.getPosition();
                Base::Vector3d dir = line->getDirection();
                gp_Lin l(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
                BRepBuilderAPI_MakeEdge eBuilder(l);
                if (!eBuilder.IsDone()) {
                    throw Base::RuntimeError(
                        "Sketcher: addExternal(): Failed to build edge from Part::DatumLine");
                }

                TopoDS_Edge e = TopoDS::Edge(eBuilder.Shape());
                refSubShape = e;
            }
            else if (Obj->isDerivedFrom<Part::DatumPoint>()) {
                auto* point = static_cast<const Part::DatumPoint*>(Obj);
                Base::Placement plm = point->Placement.getValue();
                Base::Vector3d base = plm.getPosition();
                gp_Pnt p(base.x, base.y, base.z);
                BRepBuilderAPI_MakeVertex eBuilder(p);
                if (!eBuilder.IsDone()) {
                    throw Base::RuntimeError(
                        "Sketcher: addExternal(): Failed to build vertex from Part::DatumPoint");
                }

                TopoDS_Vertex v = TopoDS::Vertex(eBuilder.Shape());
                refSubShape = v;
            }
            else {
                throw Base::TypeError(
                    "Datum feature type is not yet supported as external geometry for a sketch");
            }

            if (projection && !refSubShape.IsNull()) {
                switch (refSubShape.ShapeType()) {
                case TopAbs_FACE: {
                    processFace(invRot, invPlm, mov, sketchPlane, gPlane, sketchAx3, aProjFace, geos, refSubShape);
                } break;
                case TopAbs_EDGE: {
                    const TopoDS_Edge& edge = TopoDS::Edge(refSubShape);
                    processEdge(edge, geos, gPlane, invPlm, mov, sketchPlane, invRot, sketchAx3, aProjFace);
                } break;
                case TopAbs_VERTEX: {
                    importVertex(refSubShape);
                } break;
                default:
                    throw Base::TypeError("Unknown type of geometry");
                    break;
                }
                if (beingCreated && !extToAdd->intersection) {
                    // We are adding the projections, so we need to initialize those
                    for (auto& geo : geos) {
                        auto egf = ExternalGeometryFacade::getFacade(geo.get());
                        egf->setFlag(ExternalGeometryExtension::Defining, extToAdd->defining);
                    }
                }
            }
            int projSize = geos.size();

            if (intersection && !refSubShape.IsNull()) {
                FCBRepAlgoAPI_Section maker(refSubShape, sketchPlane);
                maker.Approximation(Standard_True);
                if (!maker.IsDone())
                    FC_THROWM(Base::CADKernelError, "Failed to get intersection");
                Part::TopoShape intersectionShape(maker.Shape());
                auto edges = intersectionShape.getSubTopoShapes(TopAbs_EDGE);
                for (const auto& s : edges) {
                    TopoDS_Edge edge = TopoDS::Edge(s.getShape());
                    processEdge(edge, geos, gPlane, invPlm, mov, sketchPlane, invRot, sketchAx3, aProjFace);
                }
                // Section of some face (e.g. sphere) produce more than one arcs
                // from the same circle. So we try to fit the arcs with a single
                // circle/arc.
                if (refSubShape.ShapeType() == TopAbs_FACE && geos.size() > 1) {
                    auto wires = Part::TopoShape().makeElementWires(edges);
                    if (wires.countSubShapes(TopAbs_WIRE) == 1) {
                        TopoDS_Vertex firstVertex, lastVertex;
                        BRepTools_WireExplorer exp(TopoDS::Wire(wires.getSubShape(TopAbs_WIRE, 1)));
                        firstVertex = exp.CurrentVertex();
                        while (!exp.More())
                            exp.Next();
                        lastVertex = exp.CurrentVertex();
                        gp_Pnt P1 = BRep_Tool::Pnt(firstVertex);
                        gp_Pnt P2 = BRep_Tool::Pnt(lastVertex);
                        if (auto geo = fitArcs(geos, P1, P2, ArcFitTolerance.getValue())) {
                            geos.clear();
                            geos.emplace_back(geo);
                        }
                    }
                }
                for (const auto& s : intersectionShape.getSubShapes(TopAbs_VERTEX, TopAbs_EDGE)) {
                    importVertex(s);
                }

                if (beingCreated && extToAdd->intersection) {
                    // We are adding the projections, so we need to initialize those
                    for (size_t i = projSize; i < geos.size(); ++i) {
                        auto egf = ExternalGeometryFacade::getFacade(geos[i].get());
                        egf->setFlag(ExternalGeometryExtension::Defining, extToAdd->defining);
                    }
                }
            }

        } catch (Base::Exception &e) {
            FC_ERR("Failed to project external geometry in "
                   << getFullName() << ": " << key << std::endl << e.what());
            continue;
        } catch (Standard_Failure &e) {
            FC_ERR("Failed to project external geometry in "
                   << getFullName() << ": " << key << std::endl << e.GetMessageString());
            continue;
        } catch (std::exception &e) {
            FC_ERR("Failed to project external geometry in "
                   << getFullName() << ": " << key << std::endl << e.what());
            continue;
        } catch (...) {
            FC_ERR("Failed to project external geometry in "
                   << getFullName() << ": " << key << std::endl << "Unknown exception");
            continue;
        }
        if (geos.empty()) {
            continue;
        }

        if(!refSet.emplace(key).second) {
            FC_WARN("Duplicated external reference in " << getFullName() << ": " << key);
            continue;
        }

        for (auto& geo : geos) {
            ExternalGeometryFacade::getFacade(geo.get())->setRef(key);
        }
        newGeos.push_back(std::move(geos));
    }

    // allocate unique geometry id
    for(auto &geos : newGeos) {
        auto egf = ExternalGeometryFacade::getFacade(geos.front().get());
        auto &refs = externalGeoRefMap[egf->getRef()];
        while(refs.size() < geos.size())
            refs.push_back(++geoLastId);

        // In case a projection reduces output geometries, delete them
        std::set<long> geoIds;
        geoIds.insert(refs.begin()+geos.size(),refs.end());

        // Sync id and ref of the new geometries
        int i = 0;
        for(auto &geo : geos)
            GeometryFacade::setId(geo.get(), refs[i++]);

        delExternalPrivate(geoIds,false);
    }

    auto geoms = ExternalGeo.getValues();

    // now update the geometries
    for(auto &geos : newGeos) {
        if (geos.empty()) {
            continue;
        }

        // Get the reference key for this group of geometries. All geos in this vector share the same ref.
        const std::string& key = ExternalGeometryFacade::getFacade(geos.front().get())->getRef();
        auto itKey = linkIsDefiningMap.find(key);
        bool hasLinkState = itKey != linkIsDefiningMap.end();
        bool isLinkDefining = hasLinkState ? itKey->second : false;

        for(auto &geo : geos) {
            auto it = externalGeoMap.find(GeometryFacade::getId(geo.get()));
            if(it == externalGeoMap.end()) {
                // This is a new geometries.
                // Set its defining state based on the inferred state of its parent link.
                if (hasLinkState) {
                    ExternalGeometryFacade::getFacade(geo.get())->setFlag(ExternalGeometryExtension::Defining, isLinkDefining);
                }
                geoms.push_back(geo.release());
                continue;
            }
            // This is an existing geometry. Update it while keeping the old flags
            ExternalGeometryFacade::copyFlags(geoms[it->second], geo.get());
            geoms[it->second] = geo.release();
        }
    }

    // Check for any missing references
    bool hasError = false;
    for(auto geo : geoms) {
        auto egf = ExternalGeometryFacade::getFacade(geo);
        egf->setFlag(ExternalGeometryExtension::Sync,false);
        if(egf->getRef().empty())
            continue;
        if(!refSet.count(egf->getRef())) {
            FC_ERR( "External geometry " << getFullName() << ".e" << egf->getId()
                    << " missing reference: " << egf->getRef());
            hasError = true;
            egf->setFlag(ExternalGeometryExtension::Missing,true);
        } else {
            egf->setFlag(ExternalGeometryExtension::Missing,false);
        }
    }

    ExternalGeo.setValues(std::move(geoms));
    rebuildVertexIndex();

    // clean up geometry reference
    if(refSet.size() != (size_t)ExternalGeometry.getSize()) {
        if(refSet.size() < keys.size()) {
            auto itObj = Objects.begin();
            auto itSub = SubElements.begin();
            for(auto &ref : keys) {
                if(!refSet.count(ref)) {
                    itObj = Objects.erase(itObj);
                    itSub = SubElements.erase(itSub);
                }else {
                    ++itObj;
                    ++itSub;
                }
            }
        }
        ExternalGeometry.setValues(Objects,SubElements);
    }

    solverNeedsUpdate=true;
    Constraints.acceptGeometry(getCompleteGeometry());

    if (hasError && this->isRecomputing()) {
        throw Base::RuntimeError("Missing external geometry reference");
    }
}

void SketchObject::fixExternalGeometry(const std::vector<int> &geoIds) {
    std::set<int> idSet(geoIds.begin(),geoIds.end());
    auto geos = ExternalGeo.getValues();
    auto objs = ExternalGeometry.getValues();
    auto subs = ExternalGeometry.getSubValues();
    bool touched = false;
    for(int i=2;i<(int)geos.size();++i) {
        auto &geo = geos[i];
        auto egf = ExternalGeometryFacade::getFacade(geo);
        int GeoId = -i-1;
        if(egf->getRef().empty()
                || !egf->testFlag(ExternalGeometryExtension::Missing)
                || (idSet.size() && !idSet.count(GeoId)))
            continue;
        std::string ref = egf->getRef();
        auto pos = ref.find('.');
        if(pos == std::string::npos) {
            FC_ERR("Invalid geometry reference " << ref);
            continue;
        }
        std::string objName = ref.substr(0,pos);
        auto obj = getDocument()->getObject(objName.c_str());
        if(!obj) {
            FC_ERR("Cannot find object in reference " << ref);
            continue;
        }

        auto elements = Part::Feature::getRelatedElements(obj,ref.c_str()+pos+1);
        if(!elements.size()) {
            FC_ERR("No related reference found for " << ref);
            continue;
        }

        geo = geo->clone();
        egf->setGeometry(geo);
        egf->setFlag(ExternalGeometryExtension::Missing,false);
        ref = objName + "." + Data::ComplexGeoData::elementMapPrefix();
        elements.front().name.appendToBuffer(ref);
        egf->setRef(ref);
        objs.push_back(obj);
        subs.emplace_back();
        elements.front().index.appendToStringBuffer(subs.back());
        touched = true;
    }

    if(touched) {
        ExternalGeo.setValues(geos);
        ExternalGeometry.setValues(objs,subs);
        rebuildExternalGeometry();
    }
}

std::vector<Part::Geometry*> SketchObject::getCompleteGeometry() const
{
    std::vector<Part::Geometry*> vals = getInternalGeometry();
    const auto &geos = getExternalGeometry();
    vals.insert(vals.end(), geos.rbegin(), geos.rend()); // in reverse order
    return vals;
}

GeoListFacade SketchObject::getGeoListFacade() const
{
    std::vector<GeometryFacadeUniquePtr> facade;
    facade.reserve(Geometry.getSize() + ExternalGeo.getSize());

    for (auto geo : Geometry.getValues())
        facade.push_back(GeometryFacade::getFacade(geo));

    const auto &externalGeos = ExternalGeo.getValues();
    for(auto rit = externalGeos.rbegin(); rit != externalGeos.rend(); rit++)
        facade.push_back(GeometryFacade::getFacade(*rit));

    return GeoListFacade::getGeoListModel(std::move(facade), Geometry.getSize());
}

void SketchObject::rebuildVertexIndex()
{
    VertexId2GeoId.resize(0);
    VertexId2PosId.resize(0);
    int imax = getHighestCurveIndex();
    int i = 0;
    const std::vector<Part::Geometry*> geometry = getCompleteGeometry();
    if (geometry.size() <= 2)
        return;
    for (std::vector<Part::Geometry*>::const_iterator it = geometry.begin();
         it != geometry.end() - 2;
         ++it, i++) {
        if (i > imax)
            i = -getExternalGeometryCount();
        if ((*it)->is<Part::GeomPoint>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
        }
        else if ((*it)->is<Part::GeomLineSegment>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
        }
        else if ((*it)->is<Part::GeomCircle>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomEllipse>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomArcOfCircle>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomArcOfEllipse>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomArcOfHyperbola>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomArcOfParabola>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomBSplineCurve>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
        }
    }
}

const std::vector<std::map<int, Sketcher::PointPos>> SketchObject::getCoincidenceGroups()
{
    // this function is different from that in getCoincidentPoints in that:
    // - getCoincidentPoints only considers direct coincidence (the points that are linked via a
    // single coincidence)
    // - this function provides an array of maps of points, each map containing the points that are
    // coincident by virtue
    //   of any number of interrelated coincidence constraints (if coincidence 1-2 and coincidence
    //   2-3, {1,2,3} are in that set)

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    std::vector<std::map<int, Sketcher::PointPos>> coincidenttree;
    // push the constraints
    for (const auto& constr : vals) {
        if (constr->Type != Sketcher::Coincident) {
            continue;
        }

        int firstpresentin = -1;
        int secondpresentin = -1;

        int i = 0;

        for (auto iti = coincidenttree.begin(); iti != coincidenttree.end(); ++iti, ++i) {
            // First
            std::map<int, Sketcher::PointPos>::const_iterator filiterator;
            filiterator = (*iti).find(constr->First);
            if (filiterator != (*iti).end() && constr->FirstPos == (*filiterator).second) {
                firstpresentin = i;
            }
            // Second
            filiterator = (*iti).find(constr->Second);
            if (filiterator != (*iti).end() && constr->SecondPos == (*filiterator).second) {
                secondpresentin = i;
            }
        }

        if (firstpresentin != -1 && secondpresentin != -1) {
            // we have to merge those sets into one
            coincidenttree[firstpresentin].insert(coincidenttree[secondpresentin].begin(),
                                                  coincidenttree[secondpresentin].end());
            coincidenttree.erase(coincidenttree.begin() + secondpresentin);
        }
        else if (firstpresentin == -1 && secondpresentin == -1) {
            // we do not have any of the values, so create a setCursor
            std::map<int, Sketcher::PointPos> tmp;
            tmp.insert(std::pair<int, Sketcher::PointPos>(constr->First, constr->FirstPos));
            tmp.insert(std::pair<int, Sketcher::PointPos>(constr->Second, constr->SecondPos));
            coincidenttree.push_back(std::move(tmp));
        }
        else if (firstpresentin != -1) {
            // add to existing group
            coincidenttree[firstpresentin].insert(
                std::pair<int, Sketcher::PointPos>(constr->Second, constr->SecondPos));
        }
        else {// secondpresentin != -1
            // add to existing group
            coincidenttree[secondpresentin].insert(
                std::pair<int, Sketcher::PointPos>(constr->First, constr->FirstPos));
        }
    }

    return coincidenttree;
}

void SketchObject::isCoincidentWithExternalGeometry(int GeoId, bool& start_external,
                                                    bool& mid_external, bool& end_external)
{
    start_external = false;
    mid_external = false;
    end_external = false;

    const std::vector<std::map<int, Sketcher::PointPos>> coincidenttree = getCoincidenceGroups();

    for (const auto& cGroup : coincidenttree) {
        const auto& geoId1iterator = cGroup.find(GeoId);
        if (geoId1iterator == cGroup.end()) {
            continue;
        }

        if (cGroup.begin()->first >= 0) {
            continue;
        }

        // `GeoId` is in this set and the first key in this ordered element key is external
        if (geoId1iterator->second == Sketcher::PointPos::start)
            start_external = true;
        else if (geoId1iterator->second == Sketcher::PointPos::mid)
            mid_external = true;
        else if (geoId1iterator->second == Sketcher::PointPos::end)
            end_external = true;
    }
}

const std::map<int, Sketcher::PointPos> SketchObject::getAllCoincidentPoints(int GeoId,
                                                                             PointPos PosId)
{
    const std::vector<std::map<int, Sketcher::PointPos>> coincidenttree = getCoincidenceGroups();

    for (const auto& cGroup : coincidenttree) {
        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

        geoId1iterator = cGroup.find(GeoId);

        if (geoId1iterator != cGroup.end()) {
            // If GeoId is in this set

            if (geoId1iterator->second == PosId)// and posId matches
                return cGroup;
        }
    }

    std::map<int, Sketcher::PointPos> empty;

    return empty;
}


void SketchObject::getDirectlyCoincidentPoints(int GeoId, PointPos PosId,
                                               std::vector<int>& GeoIdList,
                                               std::vector<PointPos>& PosIdList) const
{
    const std::vector<Constraint*>& constraints = this->Constraints.getValues();

    GeoIdList.clear();
    PosIdList.clear();
    GeoIdList.push_back(GeoId);
    PosIdList.push_back(PosId);
    for (std::vector<Constraint*>::const_iterator it = constraints.begin(); it != constraints.end();
         ++it) {
        if ((*it)->Type == Sketcher::Coincident) {
            if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                GeoIdList.push_back((*it)->Second);
                PosIdList.push_back((*it)->SecondPos);
            }
            else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                GeoIdList.push_back((*it)->First);
                PosIdList.push_back((*it)->FirstPos);
            }
        }
        if ((*it)->Type == Sketcher::Tangent) {
            if ((*it)->First == GeoId && (*it)->FirstPos == PosId &&
                ((*it)->SecondPos == Sketcher::PointPos::start ||
                 (*it)->SecondPos == Sketcher::PointPos::end)) {
                GeoIdList.push_back((*it)->Second);
                PosIdList.push_back((*it)->SecondPos);
            }
            if ((*it)->Second == GeoId && (*it)->SecondPos == PosId &&
                ((*it)->FirstPos == Sketcher::PointPos::start ||
                 (*it)->FirstPos == Sketcher::PointPos::end)) {
                GeoIdList.push_back((*it)->First);
                PosIdList.push_back((*it)->FirstPos);
            }
        }
    }
    if (GeoIdList.size() == 1) {
        GeoIdList.clear();
        PosIdList.clear();
    }
}

void SketchObject::getDirectlyCoincidentPoints(int VertexId, std::vector<int>& GeoIdList,
                                               std::vector<PointPos>& PosIdList) const
{
    int GeoId;
    PointPos PosId;
    getGeoVertexIndex(VertexId, GeoId, PosId);
    getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
}

bool SketchObject::arePointsCoincident(int GeoId1, PointPos PosId1, int GeoId2, PointPos PosId2)
{
    if (GeoId1 == GeoId2 && PosId1 == PosId2)
        return true;

    const std::vector<std::map<int, Sketcher::PointPos>> coincidenttree = getCoincidenceGroups();

    for (const auto& cGroup : coincidenttree) {
        const auto& geoId1iterator = cGroup.find(GeoId1);

        if (geoId1iterator != cGroup.end()) {
            // If First is in this set
            const auto& geoId2iterator = cGroup.find(GeoId2);

            if (geoId2iterator != cGroup.end()) {
                // If Second is in this set
                if (geoId1iterator->second == PosId1 && geoId2iterator->second == PosId2)
                    return true;
            }
        }
    }

    return false;
}
bool SketchObject::hasBlockConstraint() const
{
    return std::ranges::any_of(Constraints.getValues(), [](auto& c) {
        return c->Type == Block;
    });
}

void SketchObject::getConstraintIndices(int GeoId, std::vector<int>& constraintList)
{
    const std::vector<Constraint*>& constraints = this->Constraints.getValues();
    int i = 0;

    for (const auto& constr : constraints) {
        if (constr->First == GeoId || constr->Second == GeoId || constr->Third == GeoId) {
            constraintList.push_back(i);
        }
        ++i;
    }
}

void SketchObject::appendConflictMsg(const std::vector<int>& conflicting, std::string& msg)
{
    appendConstraintsMsg(conflicting,
                         "Remove the following conflicting constraint:",
                         "Remove at least one of the following conflicting constraints:",
                         msg);
}

void SketchObject::appendRedundantMsg(const std::vector<int>& redundant, std::string& msg)
{
    appendConstraintsMsg(redundant,
                         "Remove the following redundant constraint:",
                         "Remove the following redundant constraints:",
                         msg);
}

void SketchObject::appendMalformedConstraintsMsg(const std::vector<int>& malformed,
                                                 std::string& msg)
{
    appendConstraintsMsg(malformed,
                         "Remove the following malformed constraint:",
                         "Remove the following malformed constraints:",
                         msg);
}

void SketchObject::appendConstraintsMsg(const std::vector<int>& vector,
                                        const std::string& singularmsg,
                                        const std::string& pluralmsg, std::string& msg)
{
    std::stringstream ss;
    if (msg.length() > 0)
        ss << msg;
    if (!vector.empty()) {
        if (vector.size() == 1)
            ss << singularmsg << std::endl;
        else
            ss << pluralmsg;
        ss << vector[0] << std::endl;
        for (unsigned int i = 1; i < vector.size(); i++)
            ss << ", " << vector[i];
        ss << "\n";
    }
    msg = ss.str();
}


void SketchObject::getGeometryWithDependentParameters(
    std::vector<std::pair<int, PointPos>>& geometrymap)
{
    auto geos = getInternalGeometry();

    int geoid = -1;

    for (auto geo : geos) {
        ++geoid;

        if (!geo) {
            continue;
        }

        if (!geo->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {
            continue;
        }

        auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
            geo->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

        if (solvext->getGeometry()
            != Sketcher::SolverGeometryExtension::NotFullyConstraint) {
            continue;
        }

        // The solver differentiates whether the parameters that are dependent are not
        // those of start, end, mid, and assigns them to the edge (edge params = curve
        // params - parms of start, end, mid). The user looking at the UI expects that
        // the edge of a NotFullyConstraint geometry will always move, even if the edge
        // parameters are independent, for example if mid is the only dependent
        // parameter. In other words, the user could reasonably restrict the edge to
        // reach a fully constrained element. Under this understanding, the edge
        // parameter would always be dependent, unless the element is fully constrained.
        //
        // While this is ok from a user visual expectation point of view, it leads to a
        // loss of information of whether restricting the point start, end, mid that is
        // dependent may suffice, or even if such points are restricted, the edge would
        // still need to be restricted.
        //
        // Because Python gets the information in this function, it would lead to Python
        // users having access to a lower amount of detail.
        //
        // For this reason, this function returns edge as dependent parameter if and
        // only if constraining the parameters of the points would not suffice to
        // constraint the element.
        if (solvext->getEdge() == SolverGeometryExtension::Dependent)
            geometrymap.emplace_back(geoid, Sketcher::PointPos::none);
        if (solvext->getStart() == SolverGeometryExtension::Dependent)
            geometrymap.emplace_back(geoid, Sketcher::PointPos::start);
        if (solvext->getEnd() == SolverGeometryExtension::Dependent)
            geometrymap.emplace_back(geoid, Sketcher::PointPos::start);
        if (solvext->getMid() == SolverGeometryExtension::Dependent)
            geometrymap.emplace_back(geoid, Sketcher::PointPos::start);
    }
}

bool SketchObject::evaluateConstraint(const Constraint* constraint) const
{
    // if requireXXX,  GeoUndef is treated as an error. If not requireXXX,
    // GeoUndef is accepted. Index range checking is done on everything regardless.

    // constraints always require a First!!
    bool requireSecond = false;
    bool requireThird = false;

    switch (constraint->Type) {
        case Radius:
        case Diameter:
        case Weight:
        case Horizontal:
        case Vertical:
        case Distance:
        case DistanceX:
        case DistanceY:
        case Coincident:
        case Perpendicular:
        case Parallel:
        case Equal:
        case PointOnObject:
        case Angle:
            break;
        case Tangent:
            requireSecond = true;
            break;
        case Symmetric:
        case SnellsLaw:
            requireSecond = true;
            requireThird = true;
            break;
        default:
            break;
    }

    int intGeoCount = getHighestCurveIndex() + 1;
    int extGeoCount = getExternalGeometryCount();

    // the actual checks
    bool ret = true;
    int geoId;

    // First is always required and GeoId must be within range
    geoId = constraint->First;
    ret = ret && (geoId >= -extGeoCount && geoId < intGeoCount);

    geoId = constraint->Second;
    ret = ret
        && ((geoId == GeoEnum::GeoUndef && !requireSecond)
            || (geoId >= -extGeoCount && geoId < intGeoCount));

    geoId = constraint->Third;
    ret = ret
        && ((geoId == GeoEnum::GeoUndef && !requireThird)
            || (geoId >= -extGeoCount && geoId < intGeoCount));

    return ret;
}

bool SketchObject::evaluateConstraints() const
{
    int intGeoCount = getHighestCurveIndex() + 1;
    int extGeoCount = getExternalGeometryCount();

    std::vector<Part::Geometry*> geometry = getCompleteGeometry();
    const std::vector<Sketcher::Constraint*>& constraints = Constraints.getValuesForce();
    if (static_cast<int>(geometry.size()) != extGeoCount + intGeoCount) {
        return false;
    }
    if (geometry.size() < 2) {
        return false;
    }

    for (auto it : constraints) {
        if (!evaluateConstraint(it)) {
            return false;
        }
    }

    if (!constraints.empty()) {
        if (!Constraints.scanGeometry(geometry)) {
            return false;
        }
    }

    return true;
}

void SketchObject::validateConstraints()
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<Part::Geometry*> geometry = getCompleteGeometry();
    const std::vector<Sketcher::Constraint*>& constraints = Constraints.getValuesForce();

    std::vector<Sketcher::Constraint*> newConstraints;
    newConstraints.reserve(constraints.size());
    std::vector<Sketcher::Constraint*>::const_iterator it;
    for (it = constraints.begin(); it != constraints.end(); ++it) {
        bool valid = evaluateConstraint(*it);
        if (valid) {
            newConstraints.push_back(*it);
        }
    }

    if (newConstraints.size() != constraints.size()) {
        Constraints.setValues(std::move(newConstraints));
        acceptGeometry();
    }
    else if (!Constraints.scanGeometry(geometry)) {
        Constraints.acceptGeometry(geometry);
    }
}

std::string SketchObject::validateExpression(const App::ObjectIdentifier& path,
                                             std::shared_ptr<const App::Expression> expr)
{
    const App::Property* prop = path.getProperty();

    assert(expr);

    if (!prop)
        return "Property not found";

    if (prop == &Constraints) {
        const Constraint* constraint = Constraints.getConstraint(path);

        if (!constraint->isDriving)
            return "Reference constraints cannot be set!";
    }

    auto deps = expr->getDeps();
    auto it = deps.find(this);
    if (it != deps.end()) {
        auto it2 = it->second.find("Constraints");
        if (it2 != it->second.end()) {
            for (auto& oid : it2->second) {
                const Constraint* constraint = Constraints.getConstraint(oid);

                if (!constraint->isDriving)
                    return "Reference constraint from this sketch cannot be used in this "
                           "expression.";
            }
        }
        geoMap.clear();
        const auto &vals = getInternalGeometry();
        for(long i=0;i<(long)vals.size();++i) {
            auto geo = vals[i];
            auto gf = GeometryFacade::getFacade(geo);
            if(!gf->getId())
                gf->setId(++geoLastId);
            else if(gf->getId() > geoLastId)
                geoLastId = gf->getId();
            while(!geoMap.insert(std::make_pair(gf->getId(),i)).second) {
                FC_WARN("duplicate geometry id " << gf->getId() << " -> " << geoLastId+1);
                gf->setId(++geoLastId);
            }
        }
        updateGeoHistory();
    }
    return "";
}

// This function is necessary for precalculation of an angle when adding
//  an angle constraint. It is also used here, in SketchObject, to
//  lock down the type of tangency/perpendicularity.
double SketchObject::calculateAngleViaPoint(int GeoId1, int GeoId2, double px, double py)
{
    // Temporary sketch based calculation. Slow, but guaranteed consistency with constraints.
    Sketcher::Sketch sk;

    const auto* p1 = dynamic_cast<const Part::GeomCurve*>(this->getGeometry(GeoId1));
    const auto* p2 = dynamic_cast<const Part::GeomCurve*>(this->getGeometry(GeoId2));

    if (p1 && p2) {
        // TODO: Check if any of these are B-splines
        int i1 = sk.addGeometry(this->getGeometry(GeoId1));
        int i2 = sk.addGeometry(this->getGeometry(GeoId2));

        if (p1->is<Part::GeomBSplineCurve>() ||
            p2->is<Part::GeomBSplineCurve>()) {
            double p1ClosestParam, p2ClosestParam;
            Base::Vector3d pt(px, py, 0);
            p1->closestParameter(pt, p1ClosestParam);
            p2->closestParameter(pt, p2ClosestParam);

            return sk.calculateAngleViaParams(i1, i2, p1ClosestParam, p2ClosestParam);
        }

        return sk.calculateAngleViaPoint(i1, i2, px, py);
    }
    else
        throw Base::ValueError("Null geometry in calculateAngleViaPoint");
}

void SketchObject::constraintsRenamed(
    const std::map<App::ObjectIdentifier, App::ObjectIdentifier>& renamed)
{
    ExpressionEngine.renameExpressions(renamed);

    for (auto doc : App::GetApplication().getDocuments())
        doc->renameObjectIdentifiers(renamed);
}

void SketchObject::constraintsRemoved(const std::set<App::ObjectIdentifier>& removed)
{
    std::set<App::ObjectIdentifier>::const_iterator i = removed.begin();

    while (i != removed.end()) {
        ExpressionEngine.setValue(*i, std::shared_ptr<App::Expression>());
        ++i;
    }
}

// Tests if the provided point lies exactly in a curve (satisfies
//  point-on-object constraint). It is used to decide whether it is nesessary to
//  constrain a point onto curves when 3-element selection tangent-via-point-like
//  constraints are applied.
bool SketchObject::isPointOnCurve(int geoIdCurve, double px, double py)
{
    // DeepSOIC: this may be slow, but I wanted to reuse the existing code
    Sketcher::Sketch sk;
    int icrv = sk.addGeometry(this->getGeometry(geoIdCurve));
    Base::Vector3d pp;
    pp.x = px;
    pp.y = py;
    Part::GeomPoint p(pp);
    int ipnt = sk.addPoint(p);
    int icstr = sk.addPointOnObjectConstraint(ipnt, Sketcher::PointPos::start, icrv);
    double err = sk.calculateConstraintError(icstr);
    return err * err < 10.0 * sk.getSolverPrecision();
}

// This one was done just for fun to see to what precision the constraints are solved.
double SketchObject::calculateConstraintError(int ConstrId)
{
    Sketcher::Sketch sk;
    const std::vector<Constraint*>& clist = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(clist.size()))
        return std::numeric_limits<double>::quiet_NaN();

    Constraint* cstr = clist[ConstrId]->clone();
    double result = 0.0;
    try {
        std::vector<int> GeoIdList;
        int g;
        GeoIdList.push_back(cstr->First);
        GeoIdList.push_back(cstr->Second);
        GeoIdList.push_back(cstr->Third);

        // add only necessary geometry to the sketch
        for (std::size_t i = 0; i < GeoIdList.size(); i++) {
            g = GeoIdList[i];
            if (g != GeoEnum::GeoUndef) {
                GeoIdList[i] = sk.addGeometry(this->getGeometry(g));
            }
        }

        cstr->First = GeoIdList[0];
        cstr->Second = GeoIdList[1];
        cstr->Third = GeoIdList[2];
        int icstr = sk.addConstraint(cstr);
        result = sk.calculateConstraintError(icstr);
    }
    catch (...) {// cleanup
        delete cstr;
        throw;
    }
    delete cstr;
    return result;
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
        if (isRestoring() && checkMigration(Geometry)) {
            // Construction migration to extension
            for (auto geometryValue : Geometry.getValues()) {
                if (geometryValue->hasExtension(
                        Part::GeometryMigrationExtension::getClassTypeId())) {
                    auto ext = std::static_pointer_cast<Part::GeometryMigrationExtension>(
                        geometryValue
                            ->getExtension(Part::GeometryMigrationExtension::getClassTypeId())
                            .lock());

                    auto gf = GeometryFacade::getFacade(
                        geometryValue);  // at this point IA geometry is already migrated

                    if (ext->testMigrationType(Part::GeometryMigrationExtension::Construction)) {
                        bool oldconstr = ext->getConstruction();
                        if (geometryValue->is<Part::GeomPoint>()
                            && !gf->isInternalAligned()) {
                            oldconstr = true;
                        }
                        gf->setConstruction(oldconstr);
                    }
                    if (ext->testMigrationType(Part::GeometryMigrationExtension::GeometryId)) {
                        gf->setId(ext->getId());
                    }
                }
            }
        }
        geoMap.clear();
        const auto &vals = getInternalGeometry();
        for(long i=0;i<(long)vals.size();++i) {
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
    }

    auto doc = getDocument();

    if (prop == &Geometry || prop == &Constraints) {
        if (doc && doc->isPerformingTransaction()) {// undo/redo
            setStatus(App::PendingTransactionUpdate, true);
        }
        else {
            if (!internaltransaction) {
                // internal sketchobject operations changing both geometry and constraints will
                // explicitly perform an update
                if (prop == &Geometry) {
                    if (managedoperation || isRestoring()) {
                        // if geometry changed, the constraint geometry indices must be updated
                        acceptGeometry();
                    }
                    else {
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
                                QT_TRANSLATE_NOOP("Notifications", "Unmanaged change of Geometry Property "
                                "results in invalid constraint indices") "\n");
                        }
                        Base::StateLocker lock(internaltransaction, true);
                        setUpSketch();
                    }
                }
                else {// Change is in Constraints

                    if (managedoperation || isRestoring()) {
                        Constraints.checkGeometry(getCompleteGeometry());
                    }
                    else {
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
                }
            }
        }
    }
    else if (prop == &ExternalGeo && !prop->testStatus(App::Property::User3)) {
        if (doc && doc->isPerformingTransaction()) {
            setStatus(App::PendingTransactionUpdate, true);
        }

        if (isRestoring() && checkMigration(ExternalGeo)) {
            for (auto geometryValue : ExternalGeo.getValues()) {
                if (geometryValue->hasExtension(
                        Part::GeometryMigrationExtension::getClassTypeId())) {
                    auto ext = std::static_pointer_cast<Part::GeometryMigrationExtension>(
                        geometryValue
                            ->getExtension(Part::GeometryMigrationExtension::getClassTypeId())
                            .lock());
                    std::unique_ptr<ExternalGeometryFacade> egf;
                    if (ext->testMigrationType(Part::GeometryMigrationExtension::GeometryId)) {
                        egf = ExternalGeometryFacade::getFacade(geometryValue);
                        egf->setId(ext->getId());
                    }

                    if (ext->testMigrationType(
                            Part::GeometryMigrationExtension::ExternalReference)) {
                        if (!egf) {
                            egf = ExternalGeometryFacade::getFacade(geometryValue);
                        }
                        egf->setRef(ext->getRef());
                        egf->setRefIndex(ext->getRefIndex());
                        egf->setFlags(ext->getFlags());
                    }
                }
            }
        }
        externalGeoRefMap.clear();
        externalGeoMap.clear();
        std::set<std::string> detached;
        for(int i=0;i<ExternalGeo.getSize();++i) {
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
        if (!detached.empty()) {
            auto objs = ExternalGeometry.getValues();
            assert(externalGeoRef.size() == objs.size());
            auto itObj = objs.begin();
            auto subs = ExternalGeometry.getSubValues();
            auto itSub = subs.begin();
            for (const auto& i : externalGeoRef) {
                if (detached.count(i) != 0U) {
                    itObj = objs.erase(itObj);
                    itSub = subs.erase(itSub);
                    auto& refs = externalGeoRefMap[i];
                    for (long id : refs) {
                        auto it = externalGeoMap.find(id);
                        if(it!=externalGeoMap.end()) {
                            auto geo = ExternalGeo[it->second];
                            ExternalGeometryFacade::getFacade(geo)->setRef(std::string());
                        }
                    }
                    refs.clear();
                } else {
                    ++itObj;
                    ++itSub;
                }
            }
            ExternalGeometry.setValues(objs, subs);
        }
        else {
            signalElementsChanged();
        }
    }
    else if (prop == &ExternalGeometry) {
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
    else if (prop == &Placement) {
        if (ExternalGeometry.getSize() > 0) {
            touch();
        }
    }
    else if (prop == &ExpressionEngine) {
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
#if 0
    // For now do not delete anything (#0001791). When changing the support
    // face it might be better to check which external geometries can be kept.
    else if (prop == &AttachmentSupport) {
        // make sure not to change anything while restoring this object
        if (!isRestoring()) {
            // if support face has changed then clear the external geometry
            delConstraintsToExternal();
            for (int i=0; i < getExternalGeometryCount(); i++) {
                delExternal(0);
            }
            rebuildExternalGeometry();
        }
    }
#endif
    Part::Part2DObject::onChanged(prop);
}

void SketchObject::onUpdateElementReference(const App::Property *prop) {
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

void SketchObject::updateGeometryRefs() {
    const auto &objs = ExternalGeometry.getValues();
    const auto &subs = ExternalGeometry.getSubValues();
    const auto &shadows = ExternalGeometry.getShadowSubs();
    assert(subs.size() == shadows.size());
    std::vector<std::string> originalRefs;
    std::map<std::string,std::string> refMap;
    if(updateGeoRef) {
        assert(externalGeoRef.size() == objs.size());
        updateGeoRef = false;
        originalRefs = std::move(externalGeoRef);
    }
    externalGeoRef.clear();
    std::unordered_map<std::string, int> legacyMap;
    for(int i=0;i<(int)objs.size();++i) {
        auto obj = objs[i];
        const std::string& sub = shadows[i].newName.empty() ? subs[i] : shadows[i].newName;
        externalGeoRef.emplace_back(obj->getNameInDocument());
        auto &key = externalGeoRef.back();
        key += '.';

        legacyMap[key + Data::oldElementName(sub.c_str())] = i;

        if (!obj->isDerivedFrom<Part::Datum>()) {
            key += Data::newElementName(sub.c_str());
        }
        if (!originalRefs.empty() && originalRefs[i] != key) {
            refMap[originalRefs[i]] = key;
        }
    }
    bool touched = false;
    auto geos = ExternalGeo.getValues();
    if(refMap.empty()) {
        for(auto geo : geos) {
            auto egf = ExternalGeometryFacade::getFacade(geo);
            if (egf->getRefIndex() < 0) {
                if (egf->getId() < 0 && !egf->getRef().empty()) {
                    // NOLINTNEXTLINE
                    FC_ERR("External geometry reference corrupted in " << getFullName()
                            << " Please check.");
                    // This could happen if someone saved the sketch containing
                    // external geometries using some rogue releases during the
                    // migration period. As a remedy, We re-initiate the
                    // external geometry here to trigger rebuild later, with
                    // call to rebuildExternalGeometry()
                    initExternalGeo();
                    return;
                }
                auto it = legacyMap.find(egf->getRef());
                if (it != legacyMap.end() && egf->getRef() != externalGeoRef[it->second]) {
                    if(getDocument() && !getDocument()->isPerformingTransaction()) {
                        // FIXME: this is a bug. Find out when and why does this happen
                        //
                        // Amendment: maybe the original bug is because of not
                        // handling external geometry changes during undo/redo,
                        // which should be considered as normal. So warning only
                        // if not undo/redo.
                        //
                        // NOLINTNEXTLINE
                        FC_WARN("Update legacy external reference "
                                << egf->getRef() << " -> " << externalGeoRef[it->second] << " in "
                                << getFullName());
                    }
                    else {
                        // NOLINTNEXTLINE
                        FC_LOG("Update undo/redo external reference "
                               << egf->getRef() << " -> " << externalGeoRef[it->second] << " in "
                               << getFullName());
                    }
                    touched = true;
                    egf->setRef(externalGeoRef[it->second]);
                }
                continue;
            }
            if (egf->getRefIndex() < (int)externalGeoRef.size()
                && egf->getRef() != externalGeoRef[egf->getRefIndex()]) {
                touched = true;
                egf->setRef(externalGeoRef[egf->getRefIndex()]);
            }
            egf->setRefIndex(-1);
        }
    }else{
        for(auto &v : refMap) {
            auto it = externalGeoRefMap.find(v.first);
            if (it == externalGeoRefMap.end()) {
                continue;
            }
            for (long id : it->second) {
                auto iter = externalGeoMap.find(id);
                if(iter!=externalGeoMap.end()) {
                    auto &geo = geos[iter->second];
                    geo = geo->clone();
                    auto egf = ExternalGeometryFacade::getFacade(geo);
                    // NOLINTNEXTLINE
                    FC_LOG(getFullName() << " ref change on ExternalEdge" << iter->second - 1 << ' '
                                         << egf->getRef() << " -> " << v.second);
                    egf->setRef(v.second);
                    touched = true;
                }
            }
        }
    }
    if (touched) {
        ExternalGeo.setValues(std::move(geos));
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

bool SketchObject::getInternalTypeState(
    const Constraint* cstr, Sketcher::InternalType::InternalType& internaltypestate) const
{
    if (cstr->Type == InternalAlignment) {

        switch (cstr->AlignmentType) {
            case Undef:
            case NumInternalAlignmentType:
                internaltypestate = InternalType::None;
                break;
            case EllipseMajorDiameter:
                internaltypestate = InternalType::EllipseMajorDiameter;
                break;
            case EllipseMinorDiameter:
                internaltypestate = InternalType::EllipseMinorDiameter;
                break;
            case EllipseFocus1:
                internaltypestate = InternalType::EllipseFocus1;
                break;
            case EllipseFocus2:
                internaltypestate = InternalType::EllipseFocus2;
                break;
            case HyperbolaMajor:
                internaltypestate = InternalType::HyperbolaMajor;
                break;
            case HyperbolaMinor:
                internaltypestate = InternalType::HyperbolaMinor;
                break;
            case HyperbolaFocus:
                internaltypestate = InternalType::HyperbolaFocus;
                break;
            case ParabolaFocus:
                internaltypestate = InternalType::ParabolaFocus;
                break;
            case BSplineControlPoint:
                internaltypestate = InternalType::BSplineControlPoint;
                break;
            case BSplineKnotPoint:
                internaltypestate = InternalType::BSplineKnotPoint;
                break;
            case ParabolaFocalAxis:
                internaltypestate = InternalType::ParabolaFocalAxis;
                break;
        }

        return true;
    }

    return false;
}

bool SketchObject::getBlockedState(const Constraint* cstr, bool& blockedstate) const
{
    if (cstr->Type == Block) {
        blockedstate = true;
        return true;
    }

    return false;
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
        if(ExternalGeo.getSize()<=2) {
            if (ExternalGeo.getSize() < 2)
                initExternalGeo();
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

void SketchObject::migrateSketch()
{
    bool noextensions = false;

    for (const auto& g : getInternalGeometry())
        // no extension - legacy file
        if (!g->hasExtension(SketchGeometryExtension::getClassTypeId()))
            noextensions = true;

    if (noextensions) {
        for (auto c : Constraints.getValues()) {
            addGeometryState(c);

            // Convert B-Spline controlpoints radius/diameter constraints to Weight constraints
            if (c->Type == InternalAlignment && c->AlignmentType == BSplineControlPoint) {
                int circlegeoid = c->First;
                int bsplinegeoid = c->Second;

                auto bsp = static_cast<const Part::GeomBSplineCurve*>(getGeometry(bsplinegeoid));

                std::vector<double> weights = bsp->getWeights();

                for (auto ccp : Constraints.getValues()) {
                    if ((ccp->Type == Radius || ccp->Type == Diameter)
                        && ccp->First == circlegeoid) {
                        if (c->InternalAlignmentIndex < int(weights.size())) {
                            ccp->Type = Weight;
                            ccp->setValue(weights[c->InternalAlignmentIndex]);
                        }
                    }
                }
            }
        }

        // Construction migration to extension
        for (auto g : Geometry.getValues()) {
            if (g->hasExtension(Part::GeometryMigrationExtension::getClassTypeId())) {
                auto ext = std::static_pointer_cast<Part::GeometryMigrationExtension>(
                    g->getExtension(Part::GeometryMigrationExtension::getClassTypeId()).lock());

                if (ext->testMigrationType(Part::GeometryMigrationExtension::Construction)) {
                    // at this point IA geometry is already migrated
                    auto gf = GeometryFacade::getFacade(g);

                    bool oldconstr = ext->getConstruction();

                    if (g->is<Part::GeomPoint>() && !gf->isInternalAligned())
                        oldconstr = true;

                    GeometryFacade::setConstruction(g, oldconstr);
                }

                g->deleteExtension(Part::GeometryMigrationExtension::getClassTypeId());
            }
        }
    }

    /* parabola axis as internal geometry */
    auto constraints = Constraints.getValues();
    auto geometries = getInternalGeometry();

    bool parabolaFound = std::ranges::any_of(geometries, &Part::Geometry::is<Part::GeomArcOfParabola>);

    if (parabolaFound) {
        bool focalaxisfound = std::ranges::any_of(constraints, [](auto& c) {
            return c->Type == InternalAlignment && c->AlignmentType == ParabolaFocalAxis;
        });

        // There are parabolas and there isn't an IA axis. (1) there are no axis or (2) there is a
        // legacy construction line
        if (!focalaxisfound) {
            // maps parabola geoid to focusgeoid
            std::map<int, int> parabolageoid2focusgeoid;

            // populate parabola and focus geoids
            for (const auto& c : constraints) {
                if (c->Type == InternalAlignment && c->AlignmentType == ParabolaFocus) {
                    parabolageoid2focusgeoid[c->Second] = {c->First};
                }
            }

            // maps axis geoid to parabolageoid
            std::map<int, int> axisgeoid2parabolageoid;

            // populate axis geoid
            for (const auto& [parabolageoid, focusgeoid] : parabolageoid2focusgeoid) {
                // look for a line from focusgeoid:start to Geoid:mid_external
                std::vector<int> focusgeoidlistgeoidlist;
                std::vector<PointPos> focusposidlist;
                getDirectlyCoincidentPoints(
                    focusgeoid, Sketcher::PointPos::start, focusgeoidlistgeoidlist, focusposidlist);

                std::vector<int> parabgeoidlistgeoidlist;
                std::vector<PointPos> parabposidlist;
                getDirectlyCoincidentPoints(parabolageoid,
                                            Sketcher::PointPos::mid,
                                            parabgeoidlistgeoidlist,
                                            parabposidlist);

                if (!focusgeoidlistgeoidlist.empty() && !parabgeoidlistgeoidlist.empty()) {
                    std::size_t i, j;
                    for (i = 0; i < focusgeoidlistgeoidlist.size(); i++) {
                        for (j = 0; j < parabgeoidlistgeoidlist.size(); j++) {
                            if (focusgeoidlistgeoidlist[i] == parabgeoidlistgeoidlist[j]) {
                                axisgeoid2parabolageoid[focusgeoidlistgeoidlist[i]] = parabolageoid;
                            }
                        }
                    }
                }
            }

            std::vector<Constraint*> newconstraints;
            newconstraints.reserve(constraints.size());

            for (const auto& c : constraints) {

                if (c->Type != Coincident) {
                    newconstraints.push_back(c);
                }
                else {
                    auto axismajorcoincidentfound = std::ranges::any_of(axisgeoid2parabolageoid,
                                    [&](const auto& pair) {
                                        auto parabolageoid = pair.second;
                                        auto axisgeoid = pair.first;
                                        return (c->First == axisgeoid && c->Second == parabolageoid
                                                && c->SecondPos == PointPos::mid)
                                            || (c->Second == axisgeoid && c->First == parabolageoid
                                                && c->FirstPos == PointPos::mid);
                                    });

                    if (axismajorcoincidentfound) {
                        // we skip this coincident, the other coincident on axis will be substituted
                        // by internal geometry constraint
                        continue;
                    }

                    auto focuscoincidentfound =
                        std::ranges::find_if(axisgeoid2parabolageoid,
                                     [&](const auto& pair) {
                                         auto parabolageoid = pair.second;
                                         auto axisgeoid = pair.first;
                                         auto focusgeoid = parabolageoid2focusgeoid[parabolageoid];
                                         return (c->First == axisgeoid && c->Second == focusgeoid
                                                 && c->SecondPos == PointPos::start)
                                             || (c->Second == axisgeoid && c->First == focusgeoid
                                                 && c->FirstPos == PointPos::start);
                                     });

                    if (focuscoincidentfound != axisgeoid2parabolageoid.end()) {
                        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
                        newConstr->Type = Sketcher::InternalAlignment;
                        newConstr->AlignmentType = Sketcher::ParabolaFocalAxis;
                        newConstr->First = focuscoincidentfound->first;// axis geoid
                        newConstr->FirstPos = Sketcher::PointPos::none;
                        newConstr->Second = focuscoincidentfound->second;// parabola geoid
                        newConstr->SecondPos = Sketcher::PointPos::none;
                        newconstraints.push_back(newConstr);

                        addGeometryState(newConstr);

                        // we skip the coincident, as we have substituted it by internal geometry
                        // constraint
                        continue;
                    }

                    newconstraints.push_back(c);
                }
            }

            Constraints.setValues(std::move(newconstraints));

            Base::Console().critical(
                this->getFullName(),
                QT_TRANSLATE_NOOP("Notifications",
                                  "Parabolas were migrated. Migrated files won't open in previous "
                                  "versions of FreeCAD!!\n"));
        }
    }
}

void SketchObject::getGeoVertexIndex(int VertexId, int& GeoId, PointPos& PosId) const
{
    if (VertexId < 0 || VertexId >= int(VertexId2GeoId.size())) {
        GeoId = GeoEnum::GeoUndef;
        PosId = PointPos::none;
        return;
    }
    GeoId = VertexId2GeoId[VertexId];
    PosId = VertexId2PosId[VertexId];
}

int SketchObject::getVertexIndexGeoPos(int GeoId, PointPos PosId) const
{
    for (std::size_t i = 0; i < VertexId2GeoId.size(); i++) {
        if (VertexId2GeoId[i] == GeoId && VertexId2PosId[i] == PosId)
            return i;
    }

    return -1;
}

/// changeConstraintsLocking locks or unlocks all tangent and perpendicular
///  constraints. (Constraint locking prevents it from flipping to another valid
///  configuration, when e.g. external geometry is updated from outside.) The
///  sketch solve is not triggered by the function, but the SketchObject is
///  touched (a recompute will be necessary). The geometry should not be affected
///  by the function.
/// The bLock argument specifies, what to do. If true, all constraints are
///  unlocked and locked again. If false, all tangent and perp. constraints are
///  unlocked.
int SketchObject::changeConstraintsLocking(bool bLock)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    int cntSuccess = 0;
    int cntToBeAffected = 0;//==cntSuccess+cntFail
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    std::vector<Constraint*> newVals(vals);// modifiable copy of pointers array

    for (size_t i = 0; i < newVals.size(); i++) {
        if (newVals[i]->Type == Tangent || newVals[i]->Type == Perpendicular) {
            // create a constraint copy, affect it, replace the pointer
            cntToBeAffected++;
            Constraint* constNew = newVals[i]->clone();
            bool ret = AutoLockTangencyAndPerpty(newVals[i], /*bForce=*/true, bLock);

            if (ret)
                cntSuccess++;

            newVals[i] = constNew;
            Base::Console().log("Constraint%i will be affected\n", i + 1);
        }
    }

    this->Constraints.setValues(std::move(newVals));

    Base::Console().log("ChangeConstraintsLocking: affected %i of %i tangent/perp constraints\n",
                        cntSuccess,
                        cntToBeAffected);

    return cntSuccess;
}

/*!
 * \brief SketchObject::port_reversedExternalArcs finds constraints that link to endpoints of
 * external-geometry arcs, and swaps the endpoints in the constraints. This is needed after CCW
 * emulation was introduced, to port old sketches. \param justAnalyze if true, nothing is actually
 * done - only the number of constraints to be affected is returned. \return the number of
 * constraints changed/to be changed.
 */
int SketchObject::port_reversedExternalArcs(bool justAnalyze)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    int cntToBeAffected = 0;
    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    std::vector<Constraint*> newVals(vals);// modifiable copy of pointers array

    for (std::size_t ic = 0; ic < newVals.size(); ic++) {// ic = index of constraint
        bool affected = false;
        Constraint* constNew = nullptr;
        for (int ig = 1; ig <= 3; ig++) {// cycle through constraint.first, second, third
            int geoId = 0;
            Sketcher::PointPos posId = PointPos::none;
            switch (ig) {
                case 1:
                    geoId = newVals[ic]->First;
                    posId = newVals[ic]->FirstPos;
                    break;
                case 2:
                    geoId = newVals[ic]->Second;
                    posId = newVals[ic]->SecondPos;
                    break;
                case 3:
                    geoId = newVals[ic]->Third;
                    posId = newVals[ic]->ThirdPos;
                    break;
            }

            if (geoId <= GeoEnum::RefExt
                && (posId == Sketcher::PointPos::start || posId == Sketcher::PointPos::end)) {
                // we are dealing with a link to an endpoint of external geom
                Part::Geometry* g = this->ExternalGeo[-geoId - 1];
                if (g->is<Part::GeomArcOfCircle>()) {
                    const Part::GeomArcOfCircle* segm =
                        static_cast<const Part::GeomArcOfCircle*>(g);
                    if (segm->isReversed()) {
                        // Gotcha! a link to an endpoint of external arc that is reversed.
                        // create a constraint copy, affect it, replace the pointer
                        if (!affected)
                            constNew = newVals[ic]->clone();
                        affected = true;
                        // Do the fix on temp vars
                        if (posId == Sketcher::PointPos::start)
                            posId = Sketcher::PointPos::end;
                        else if (posId == Sketcher::PointPos::end)
                            posId = Sketcher::PointPos::start;
                    }
                }
            }
            if (!affected)
                continue;
            // Propagate the fix made on temp vars to the constraint
            switch (ig) {
                case 1:
                    constNew->First = geoId;
                    constNew->FirstPos = posId;
                    break;
                case 2:
                    constNew->Second = geoId;
                    constNew->SecondPos = posId;
                    break;
                case 3:
                    constNew->Third = geoId;
                    constNew->ThirdPos = posId;
                    break;
            }
        }
        if (affected) {
            cntToBeAffected++;
            newVals[ic] = constNew;
            Base::Console().log("Constraint%i will be affected\n", ic + 1);
        };
    }

    if (!justAnalyze) {
        this->Constraints.setValues(std::move(newVals));
        Base::Console().log("Swapped start/end of reversed external arcs in %i constraints\n",
                            cntToBeAffected);
    }

    return cntToBeAffected;
}

/// Locks tangency/perpendicularity type of such a constraint.
/// The constraint passed must be writable (i.e. the one that is not
///  yet in the constraint list).
/// Tangency type (internal/external) is derived from current geometry
///  the constraint refers to.
/// Same for perpendicularity type.
///
/// This function catches exceptions, because it's not a reason to
///  not create a constraint if tangency/perp-ty type cannot be determined.
///
/// Arguments:
///  cstr - pointer to a constraint to be locked/unlocked
///  bForce - specifies whether to ignore the already locked constraint or not.
///  bLock - specifies whether to lock the constraint or not (if bForce is
///   true, the constraint gets unlocked, otherwise nothing is done at all).
///
/// Return values:
///  true - success.
///  false - fail (this indicates an error, or that a constraint locking isn't supported).
bool SketchObject::AutoLockTangencyAndPerpty(Constraint* cstr, bool bForce, bool bLock)
{
    using std::numbers::pi;

    try {
        /*tangency type already set. If not bForce - don't touch.*/
        if (cstr->getValue() != 0.0 && !bForce)
            return true;
        if (!bLock) {
            cstr->setValue(0.0);// reset
        }
        else {
            // decide on tangency type. Write the angle value into the datum field of the
            // constraint.
            int geoId1, geoId2, geoIdPt;
            PointPos posPt;
            geoId1 = cstr->First;
            geoId2 = cstr->Second;
            geoIdPt = cstr->Third;
            posPt = cstr->ThirdPos;
            if (geoIdPt == GeoEnum::GeoUndef) {// not tangent-via-point, try endpoint-to-endpoint...

                // First check if it is a tangency at knot constraint, if not continue with checking
                // for endpoints. Endpoint constraints make use of the AngleViaPoint framework at
                // solver level, so they need locking angle calculation, tangency at knot constraint
                // does not.
                auto geof = getGeometryFacade(cstr->First);
                if (geof->isInternalType(InternalType::BSplineKnotPoint)) {
                    // there is point that is a B-Spline knot in a two element constraint
                    // this is not implement using AngleViaPoint (TangencyViaPoint)
                    return false;
                }

                geoIdPt = cstr->First;
                posPt = cstr->FirstPos;
            }
            if (posPt == PointPos::none) {
                // not endpoint-to-curve and not endpoint-to-endpoint tangent (is simple tangency)
                // no tangency lockdown is implemented for simple tangency. Do nothing.
                return false;
            }

            Base::Vector3d p = getPoint(geoIdPt, posPt);

            // this piece of code is also present in Sketch.cpp, correct for offset
            // and to do the autodecision for old sketches.
            // the difference between the datum value and the actual angle to apply.
            // (datum=angle+offset)
            double angleOffset = 0.0;
            // the desired angle value (and we are to decide if 180* should be added to it)
            double angleDesire = 0.0;
            if (cstr->Type == Tangent) {
                angleOffset = -pi / 2;
                angleDesire = 0.0;
            }
            if (cstr->Type == Perpendicular) {
                angleOffset = 0;
                angleDesire = pi / 2;
            }

            double angleErr = calculateAngleViaPoint(geoId1, geoId2, p.x, p.y) - angleDesire;

            // bring angleErr to -pi..pi
            if (angleErr > pi)
                angleErr -= pi * 2;
            if (angleErr < -pi)
                angleErr += pi * 2;

            // the autodetector
            if (fabs(angleErr) > pi / 2)
                angleDesire += pi;

            // external tangency. The angle stored is offset by Pi/2 so that a value of 0.0 is
            // invalid and treated as "undecided".
            cstr->setValue(angleDesire + angleOffset);
        }
    }
    catch (Base::Exception& e) {
        // failure to determine tangency type is not a big deal, so a warning.
        Base::Console().warning("Error in AutoLockTangency. %s \n", e.what());
        return false;
    }
    return true;
}

App::DocumentObject *SketchObject::getSubObject(
        const char *subname, PyObject **pyObj,
        Base::Matrix4D *pmat, bool transform, int depth) const
{
    while(subname && *subname=='.') ++subname; // skip leading .
    std::string sub;
    const char *mapped = Data::isMappedElement(subname);
    if(!subname || !subname[0])
        return Part2DObject::getSubObject(subname,pyObj,pmat,transform,depth);
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
    } else {
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
        } else if (boost::equals(shapetype,"ExternalEdge")) {
            int GeoId = index - 1;
            GeoId = -GeoId - 3;
            geo = getGeometry(GeoId);
            if(!geo)
                return nullptr;
        } else if (boost::equals(shapetype,"Vertex") ||
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
        } else
            return nullptr;
    }

    if (pmat && transform)
        *pmat *= Placement.getValue().toMatrix();

    if (pyObj) {
        Part::TopoShape shape;
        std::string name = convertSubName(indexedName,false);
        if (geo) {
            shape = getEdge(geo,name.c_str());
            if(pmat && !shape.isNull())
                shape.transformShape(*pmat,false,true);
        } else if (!subshape.isNull()) {
            shape = subshape;
            if (pmat)
                shape.transformShape(*pmat,false,true);
        } else {
            if(pmat)
                point = (*pmat)*point;
            shape = BRepBuilderAPI_MakeVertex(gp_Pnt(point.x,point.y,point.z)).Vertex();
            // Originally in ComplexGeoData::setElementName
            // LinkStable/src/App/ComplexGeoData.cpp#L1631
            // No longer possible after map separated in ElementMap.cpp
            if ( !shape.hasElementMap() ) {
                shape.resetElementMap(std::make_shared<Data::ElementMap>());
            }
            shape.setElementName(Data::IndexedName::fromConst("Vertex", 1),
                                 Data::MappedName::fromRawData(name.c_str()),0);
        }
        shape.Tag = getID();
        *pyObj = Py::new_reference_to(Part::shape2pyshape(shape));
    }

    return const_cast<SketchObject*>(this);
}

std::vector<Data::IndexedName>
SketchObject::getHigherElements(const char *element, bool silent) const
{
    std::vector<Data::IndexedName> res;
        if (boost::istarts_with(element, "vertex")) {
            int n = 0;
            int index = atoi(element+6);
            for (auto cstr : Constraints.getValues()) {
                ++n;
                if (cstr->Type != Sketcher::Coincident)
                    continue;
                if(cstr->First >= 0 && index == getSolvedSketch().getPointId(cstr->First, cstr->FirstPos) + 1)
                    res.push_back(Data::IndexedName::fromConst("Constraint", n));
                if(cstr->Second >= 0 && index == getSolvedSketch().getPointId(cstr->Second, cstr->SecondPos) + 1)
                    res.push_back(Data::IndexedName::fromConst("Constraint", n));
            }
        }
        return res;

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

Part::TopoShape SketchObject::getEdge(const Part::Geometry *geo, const char *name) const
{
    Part::TopoShape shape(geo->toShape());
    // Originally in ComplexGeoData::setElementName
    // LinkStable/src/App/ComplexGeoData.cpp#L1631
    // No longer possible after map separated in ElementMap.cpp
    if ( !shape.hasElementMap() ) {
        shape.resetElementMap(std::make_shared<Data::ElementMap>());
    }
    shape.setElementName(Data::IndexedName::fromConst("Edge", 1),
                          Data::MappedName::fromRawData(name),0L);
    TopTools_IndexedMapOfShape vmap;
    TopExp::MapShapes(shape.getShape(), TopAbs_VERTEX, vmap);
    std::ostringstream ss;
    for(int i=1;i<=vmap.Extent();++i) {
        auto gpt = BRep_Tool::Pnt(TopoDS::Vertex(vmap(i)));
        Base::Vector3d pt(gpt.X(),gpt.Y(),gpt.Z());
        PointPos pos[] = {PointPos::start,PointPos::end};
        for(size_t j=0;j<sizeof(pos)/sizeof(pos[0]);++j) {
            if(getPoint(geo,pos[j]) == pt) {
                ss.str("");
                ss << name << 'v' << static_cast<int>(pos[j]);
                shape.setElementName(Data::IndexedName::fromConst("Vertex", i),
                                      Data::MappedName::fromRawData(ss.str().c_str()),0L);
                break;
            }
        }
    }
    return shape;
}

Data::IndexedName SketchObject::checkSubName(const char *subname) const{
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

Data::IndexedName SketchObject::shapeTypeFromGeoId(int geoId, PointPos posId) const
{
    if (geoId == GeoEnum::HAxis) {
        if (posId == PointPos::start) {
            return Data::IndexedName::fromConst("RootPoint", 0);
        }
        return Data::IndexedName::fromConst("H_Axis", 0);
    }
    if (geoId == GeoEnum::VAxis) {
        return Data::IndexedName::fromConst("V_Axis", 0);
    }

    if (posId == PointPos::none) {
        auto geo = getGeometry(geoId);
        if (geo && geo->isDerivedFrom<Part::GeomPoint>()) {
            posId = PointPos::start;
        }
    }
    if(posId != PointPos::none) {
        int idx = getVertexIndexGeoPos(geoId, posId);
        if (idx < 0) {
            return Data::IndexedName();
        }
        return Data::IndexedName::fromConst("Vertex", idx + 1);
    }
    if (geoId >= 0) {
        return Data::IndexedName::fromConst("Edge", geoId + 1);
    }
    return Data::IndexedName::fromConst("ExternalEdge", -geoId - 2);
}

bool SketchObject::geoIdFromShapeType(const Data::IndexedName & indexedName,
                                      int &geoId,
                                      PointPos &posId) const
{
    posId = PointPos::none;
    geoId = Sketcher::GeoEnum::GeoUndef;
    if (!indexedName)
        return false;
    const char *shapetype = indexedName.getType();
    if (boost::equals(shapetype,"Edge") ||
        boost::equals(shapetype,"edge")) {
        geoId = indexedName.getIndex() - 1;
    } else if (boost::equals(shapetype,"ExternalEdge")) {
        geoId = indexedName.getIndex() - 1;
        geoId = Sketcher::GeoEnum::RefExt - geoId;
    } else if (boost::equals(shapetype,"Vertex") ||
               boost::equals(shapetype,"vertex")) {
        int VtId = indexedName.getIndex() - 1;
        getGeoVertexIndex(VtId,geoId,posId);
        if (posId==PointPos::none) return false;
    } else if (boost::equals(shapetype,"H_Axis")) {
        geoId = Sketcher::GeoEnum::HAxis;
    } else if (boost::equals(shapetype,"V_Axis")) {
        geoId = Sketcher::GeoEnum::VAxis;
    } else if (boost::equals(shapetype,"RootPoint")) {
        geoId = Sketcher::GeoEnum::RtPnt;
        posId = PointPos::start;
    } else
        return false;
    return true;
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

std::string SketchObject::getGeometryReference(int GeoId) const {
    auto geo = getGeometry(GeoId);
    if (!geo) {
        return {};
    }
    auto egf = ExternalGeometryFacade::getFacade(geo);
    if (egf->getRef().empty()) {
        return {};
    }

    const std::string &ref = egf->getRef();

    if (egf->testFlag(ExternalGeometryExtension::Missing)) {
        return std::string("? ") + ref;
    }

    auto pos = ref.find('.');
    if (pos == std::string::npos) {
        return ref;
    }
    std::string objName = ref.substr(0, pos);
    auto obj = getDocument()->getObject(objName.c_str());
    if (!obj) {
        return ref;
    }

    App::ElementNamePair elementName;
    App::GeoFeature::resolveElement(obj, ref.c_str() + pos + 1, elementName);
    if (!elementName.oldName.empty()) {
        return objName + "." + elementName.oldName;
    }
    return ref;
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

int SketchObject::autoRemoveRedundants(DeleteOptions options)
{
    auto redundants = getLastRedundant();

    if (redundants.empty()) {
        return 0;
    }

    // getLastRedundant is base 1, while delConstraints is base 0
    for (size_t i = 0; i < redundants.size(); i++)
        redundants[i]--;

    delConstraints(redundants, options);

    return redundants.size();
}

int SketchObject::renameConstraint(int GeoId, std::string name)
{
    // only change the constraint item if the names are different
    const Constraint* item = Constraints[GeoId];

    if (item->Name != name) {
        // no need to check input data validity as this is an sketchobject managed operation.
        Base::StateLocker lock(managedoperation, true);

        Constraint* copy = item->clone();
        copy->Name = std::move(name);

        Constraints.set1Value(GeoId, copy);
        delete copy;

        // make sure any prospective solver access updates the constraint pointer that just got
        // invalidated
        solverNeedsUpdate = true;

        return 0;
    }
    return -1;
}

std::vector<Base::Vector3d> SketchObject::getOpenVertices() const
{
    std::vector<Base::Vector3d> points;

    if (analyser)
        points = analyser->getOpenVertices();

    return points;
}

// SketchGeometryExtension interface

size_t setGeometryIdHelper(int GeoId, long id, std::vector<Part::Geometry*>& newVals, size_t searchOffset = 0, bool returnOnMatch = false)
{
    // deep copy
    for (size_t i = searchOffset; i < newVals.size(); i++) {
        newVals[i] = newVals[i]->clone();

        if ((int)i == GeoId) {
            auto gf = GeometryFacade::getFacade(newVals[i]);

            gf->setId(id);

            if (returnOnMatch) {
                return i;
            }
        }
    }
    return 0;
}
int SketchObject::setGeometryId(int GeoId, long id)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId >= int(Geometry.getValues().size()))
        return -1;

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);
    setGeometryIdHelper(GeoId, id, newVals);

    // There is not actual internal transaction going on here, however neither the geometry indices
    // nor the vertices need to be updated so this is a convenient way of preventing it.
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(std::move(newVals));
    }

    return 0;
}
int SketchObject::setGeometryIds(std::vector<std::pair<int, long>> GeoIdsToIds)
{
    Base::StateLocker lock(managedoperation, true);

    std::sort(GeoIdsToIds.begin(), GeoIdsToIds.end());

    size_t searchOffset = 0;

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    for (size_t i = 0; i < GeoIdsToIds.size(); ++i) {
        int GeoId = GeoIdsToIds[i].first;
        long id = GeoIdsToIds[i].second;
        searchOffset = setGeometryIdHelper(GeoId, id, newVals, searchOffset, i != GeoIdsToIds.size()-1);
    }

    // There is not actual internal transaction going on here, however neither the geometry indices
    // nor the vertices need to be updated so this is a convenient way of preventing it.
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(std::move(newVals));
    }

    return 0;
}

int SketchObject::getGeometryId(int GeoId, long& id) const
{
    if (GeoId < 0 || GeoId >= int(Geometry.getValues().size()))
        return -1;

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    auto gf = GeometryFacade::getFacade(vals[GeoId]);

    id = gf->getId();

    return 0;
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
