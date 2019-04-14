/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS.hxx>
# include <TopExp_Explorer.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax3.hxx>
# include <gp_Circ.hxx>
# include <gp_Elips.hxx>
# include <gp_Hypr.hxx>
# include <gp_Parab.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRep_Tool.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Circle.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_Hyperbola.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <Geom_OffsetCurve.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <BRepOffsetAPI_NormalProjection.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <GeomAPI_IntSS.hxx>
# include <BRepProj_Projection.hxx>
# include <GeomConvert_BSplineCurveKnotSplitting.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <GC_MakeCircle.hxx>
# include <Standard_Version.hxx>
# include <cmath>
# include <vector>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
//# include <QtGlobal>
#endif

#include <boost/version.hpp>
#include <boost/config.hpp>
#if defined(BOOST_MSVC) && (BOOST_VERSION == 105500)
// for fixing issue https://svn.boost.org/trac/boost/ticket/9332
#   include "boost_fix/intrusive/detail/memory_util.hpp"
#   include "boost_fix/container/detail/memory_util.hpp"
#endif
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/register/point.hpp>

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/FeaturePythonPyImp.h>
#include <App/Part.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>

#include <App/OriginFeature.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Part/App/PartPyCXX.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

namespace Part {
    PartExport std::list<TopoDS_Edge> sort_Edges2(double tol3d, std::list<TopoDS_Edge>& edges,
            std::deque<int> *hashes);
}

#include "SketchObject.h"
#include "Sketch.h"
#include <Mod/Sketcher/App/SketchObjectPy.h>


#undef DEBUG
//#define DEBUG

using namespace Sketcher;
using namespace Base;

FC_LOG_LEVEL_INIT("Sketcher",true,true);

const int GeoEnum::RtPnt  = -1;
const int GeoEnum::HAxis  = -1;
const int GeoEnum::VAxis  = -2;
const int GeoEnum::RefExt = -3;

PROPERTY_SOURCE(Sketcher::SketchObject, Part::Part2DObject)


SketchObject::SketchObject()
{
    ADD_PROPERTY_TYPE(Geometry,        (0)  ,"Sketch",(App::PropertyType)(App::Prop_None),"Sketch geometry");
    ADD_PROPERTY_TYPE(Constraints,     (0)  ,"Sketch",(App::PropertyType)(App::Prop_None),"Sketch constraints");
    ADD_PROPERTY_TYPE(ExternalGeometry,(0,0),"Sketch",
            (App::PropertyType)(App::Prop_None|App::Prop_ReadOnly),"Sketch external geometry references");
    ADD_PROPERTY_TYPE(Exports,         (0)  ,"Sketch",
            (App::PropertyType)(App::Prop_Hidden),"Sketch export geometry");
    ADD_PROPERTY_TYPE(ExternalGeo,    (0)  ,"Sketch",
            (App::PropertyType)(App::Prop_Hidden),"Sketch external geometry");

    geoLastId = 0;

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    geoHistoryLevel = hGrpp->GetInt("GeometryHistoryLevel",1);

    Geometry.setOrderRelevant(true);
    
    allowOtherBody = true;
    allowUnaligned = true;

    initExternalGeo();

    rebuildVertexIndex();

    lastDoF=0;
    lastHasConflict=false;
    lastHasRedundancies=false;
    lastSolverStatus=0;
    lastSolveTime=0;

    solverNeedsUpdate=false;

    noRecomputes=false;

    ExpressionEngine.setValidator(boost::bind(&Sketcher::SketchObject::validateExpression, this, _1, _2));

    constraintsRemovedConn = Constraints.signalConstraintsRemoved.connect(boost::bind(&Sketcher::SketchObject::constraintsRemoved, this, _1));
    constraintsRenamedConn = Constraints.signalConstraintsRenamed.connect(boost::bind(&Sketcher::SketchObject::constraintsRenamed, this, _1));

    analyser = new SketchAnalysis(this);
}

SketchObject::~SketchObject()
{
    delete analyser;
}

void SketchObject::initExternalGeo() {
    std::vector<Part::Geometry *> geos;
    Part::GeomLineSegment *HLine = new Part::GeomLineSegment();
    Part::GeomLineSegment *VLine = new Part::GeomLineSegment();
    HLine->Id = -1;
    HLine->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
    VLine->Id = -2;
    VLine->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(0,1,0));
    HLine->Construction = true;
    VLine->Construction = true;
    geos.push_back(HLine);
    geos.push_back(VLine);
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

App::DocumentObjectExecReturn *SketchObject::execute(void)
{
    try {
        App::DocumentObjectExecReturn* rtn = Part2DObject::execute();//to positionBySupport
        if(rtn!=App::DocumentObject::StdReturn)
            //error
            return rtn;
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // setup and diagnose the sketch
    rebuildExternalGeometry();

    // This includes a regular solve including full geometry update, except when an error
    // ensues
    int err = this->solve(true);

    if (err == -4) { // over-constrained sketch
        std::string msg="Over-constrained sketch\n";
        appendConflictMsg(lastConflicting, msg);
        return new App::DocumentObjectExecReturn(msg.c_str(),this);
    }
    else if (err == -3) { // conflicting constraints
        std::string msg="Sketch with conflicting constraints\n";
        appendConflictMsg(lastConflicting, msg);
        return new App::DocumentObjectExecReturn(msg.c_str(),this);
    }
    else if (err == -2) { // redundant constraints
        std::string msg="Sketch with redundant constraints\n";
        appendRedundantMsg(lastRedundant, msg);
        return new App::DocumentObjectExecReturn(msg.c_str(),this);
    }
    else if (err == -1) { // Solver failed
        return new App::DocumentObjectExecReturn("Solving the sketch failed",this);
    }

    // this is not necessary for sketch representation in edit mode, unless we want to trigger an update of
    // the objects that depend on this sketch (like pads)
    buildShape();

    return App::DocumentObject::StdReturn;
}

void SketchObject::buildShape() {

    // Shape.setValue(solvedSketch.toShape()); 
    // We use the following instead to map element names

    std::vector<Part::TopoShape> shapes;
    int i=0;
    std::string name("Edge");
    for(auto geo : getInternalGeometry()) {
        ++i;
        if(geo->Construction) 
            continue;
        shapes.push_back(getEdge(geo,convertSubName(
                        name+std::to_string(i),false).c_str()));
    }
    name = "ExternalEdge";
    for(i=2;i<ExternalGeo.getSize();++i) {
        auto geo = ExternalGeo[i];
        if(!geo->testFlag(Part::Geometry::Defining))
            continue;
        shapes.push_back(getEdge(geo,convertSubName(
                        name+std::to_string(i-1),false).c_str()));
    }
    if(shapes.empty())
        Shape.setValue(Part::TopoShape());
    else
        Shape.setValue(Part::TopoShape().makEWires(shapes,TOPOP_SKETCH));
}

static bool hasSketchMarker(const char *name) {
    static std::string marker(Part::TopoShape::elementMapPrefix()+TOPOP_SKETCH);
    return strstr(name,marker.c_str())!=0;
}

int SketchObject::hasConflicts(void) const
{
    if (lastDoF < 0) // over-constrained sketch
        return -2;
    if (solvedSketch.hasConflicts()) // conflicting constraints
        return -1;

    return 0;
}

int SketchObject::solve(bool updateGeoAfterSolving/*=true*/)
{
    // Reset the initial movement in case of a dragging operation was ongoing on the solver.
    solvedSketch.resetInitMove();

    // if updateGeoAfterSolving=false, the solver information is updated, but the Sketch is nothing
    // updated. It is useful to avoid triggering an OnChange when the goeometry did not change but
    // the solver needs to be updated.

    // We should have an updated Sketcher (sketchobject) geometry or this solve() should not have happened
    // therefore we update our sketch solver geometry with the SketchObject one.
    //
    // set up a sketch (including dofs counting and diagnosing of conflicts)
    lastDoF = solvedSketch.setUpSketch(getCompleteGeometry(), Constraints.getValues(),
                                  getExternalGeometryCount());

    // At this point we have the solver information about conflicting/redundant/over-constrained, but the sketch is NOT solved.
    // Some examples:
    // Redundant: a vertical line, a horizontal line and an angle constraint of 90 degrees between the two lines
    // Conflicting: a 80 degrees angle between a vertical line and another line, then adding a horizontal constraint to that other line
    // OverConstrained: a conflicting constraint when all other DoF are already constraint (it has more constrains than parameters and the extra constraints are not redundant)

    solverNeedsUpdate=false;

    lastHasConflict = solvedSketch.hasConflicts();
    lastHasRedundancies = solvedSketch.hasRedundancies();
    lastConflicting=solvedSketch.getConflicting();
    lastRedundant=solvedSketch.getRedundant();
    lastSolveTime=0.0;

    lastSolverStatus=GCS::Failed; // Failure is default for notifying the user unless otherwise proven

    int err=0;

    // redundancy is a lower priority problem than conflict/over-constraint/solver error
    // we set it here because we are indeed going to solve, as we can. However, we still want to
    // provide the right error code.
    if (lastHasRedundancies) { // redundant constraints
        err = -2;
    }

    if (lastDoF < 0) { // over-constrained sketch
        err = -4;
    }
    else if (lastHasConflict) { // conflicting constraints
        // The situation is exactly the same as in the over-constrained situation.
        err = -3;
    }
    else {
        lastSolverStatus=solvedSketch.solve();
        if (lastSolverStatus != 0){ // solving
            err = -1;
        }
    }

    lastSolveTime=solvedSketch.SolveTime;

    if (err == 0 && updateGeoAfterSolving) {
        // set the newly solved geometry
        std::vector<Part::Geometry *> geomlist = solvedSketch.extractGeometry();
        Geometry.setValues(geomlist);
        for (std::vector<Part::Geometry *>::iterator it = geomlist.begin(); it != geomlist.end(); ++it)
            if (*it) delete *it;
    }
    else if(err <0) {
        // if solver failed, invalid constraints were likely added before solving
        // (see solve in addConstraint), so solver information is definitely invalid.
        this->Constraints.touch();
    }

    return err;
}

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

BOOST_GEOMETRY_REGISTER_POINT_3D(
        Base::Vector3d,double,bg::cs::cartesian,x,y,z)

class SketchObject::GeoHistory {
public:
    typedef bgi::linear<16> Parameters;

    typedef std::set<long> IdSet;
    typedef std::pair<IdSet,IdSet> IdSets;
    typedef std::list<IdSet> AdjList;

    //associate a geo with connected ones on both points
    typedef std::map<long, IdSets> AdjMap; 

    // maps start/end points to all existing geo to query and update adjacencies
    typedef std::pair<Base::Vector3d,AdjList::iterator> Value; 

    AdjList adjlist;
    AdjMap adjmap;
    bgi::rtree<Value,Parameters> rtree;

    AdjList::iterator find(const Base::Vector3d &pt,bool strict=true){
        std::vector<Value> ret;
        rtree.query(bgi::nearest(pt,1),std::back_inserter(ret));
        if(ret.size()) {
            // NOTE: we are using square distance here, the 1e-6 threshold is
            // very forgiving. We should have used Precision::SquareConfisuion(), 
            // which is 1e-14. However, there is a problem with current
            // commandGeoCreate. They create new geometry with initial point of
            // the exact mouse position, instead of the pre-selected point
            // position, and rely on auto constraint to snap in the new
            // geometry. So, we cannot use a very strict threshold here.
            double tol = strict?Precision::SquareConfusion()*10:1e-6;
            double d = Base::DistanceP2(ret[0].first,pt);
            if(d<tol) {
                if(!strict) FC_TRACE("hit " << FC_xyz(pt));
                return ret[0].second;
            }
            if(!strict)
                FC_TRACE("miss " << FC_xyz(pt) << ", " << FC_xyz(ret[0].first) << ", " << d);
        }else if(!strict)
            FC_TRACE("miss " << FC_xyz(pt));
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
                auto &v = adjmap[id];
                auto &adj = _id>0?v.first:v.second;
                for(auto it=adj.begin(),itNext=it;it!=adj.end();it=itNext) {
                    ++itNext;
                    long other = *it;
                    if(geomap.find(other) == geomap.end()) {
                        // remember those deleted id's
                        oldset.insert(other);
                        FC_TRACE("insert old " << id << ", " << other);
                    } else if(idset.find(other)==idset.end()) {
                        // delete any existing id's that are no longer in the adj list
                        FC_TRACE("erase " << id << ", " << other);
                        adj.erase(it);
                    }
                }
                // now merge the current ones
                for(long _id2 : idset) {
                    long id2 = abs(_id2);
                    if(id!=id2) {
                        adj.insert(id2);
                        FC_TRACE("insert new " << id << ", " << id2);
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

    if(!geoHistory)
        geoHistory.reset(new GeoHistory);

    FC_TIME_INIT(t);
    const auto &geos = getInternalGeometry();
    geoHistory->clear();
    for(int i=0;i<(int)geos.size();++i) {
        auto geo = geos[i];
        auto pstart = getPoint(geo,start);
        auto pend = getPoint(geo,end);
        geoHistory->update(pstart,geo->Id);
        if(pstart!=pend)
            geoHistory->update(pend,-geo->Id);
    }
    geoHistory->finishUpdate(geoMap);
    FC_TIME_LOG(t,"update geometry history (" << geoHistory->size() << ", " << geoMap.size()<<')');
}

#define GEN_ID(geo) do {\
        generateId(geo); \
        FC_LOG("generate id " << geo->Id << ", " << geoLastId);\
    }while(0);

void SketchObject::generateId(Part::Geometry *geo) {
    if(!geoHistoryLevel) {
        geo->Id = ++geoLastId;
        geoMap[geo->Id] = (long)Geometry.getSize();
        return;
    }

    if(!geoHistory)
        updateGeoHistory();

    // Search geo histroy to see if the start point and end point belongs to
    // some deleted geometries. Prefer matching both start and end point. If
    // can't then try start and then end. Generate new id if none is found.
    auto pstart = getPoint(geo,start);
    auto it = geoHistory->find(pstart,false);
    auto pend = getPoint(geo,end);
    auto it2 = it;
    if(pstart!=pend) {
        it2 = geoHistory->find(pend,false);
        if(it2 == geoHistory->end())
            it2 = it;
    }
    long newId = -1;
    std::vector<long> found;

    if(geoHistoryLevel<=1 && (it==geoHistory->end() || it2==it)) {
        // level<=1 means we only reuse id if both start and end matches
        newId = ++geoLastId;
        goto END;
    }

    if(it!=geoHistory->end()) {
        for(long id  : *it) {
            if(geoMap.find(id)==geoMap.end()) {
                if(it2 == it) {
                    newId = id;
                    goto END;
                }
                found.push_back(id);
            }else
                FC_TRACE("ignore " << id);
        }
    }
    if(it2!=it) {
        if(found.empty()) {
            // no candidate exists
            for(long id : *it2) {
                if(geoMap.find(id)==geoMap.end()) {
                    newId = id;
                    goto END;
                }
                FC_TRACE("ignore " << id);
            }
        }else{
            // already some candidate exists, search for matching of both
            // points
            for(long id : found) {
                if(it2->find(id)!=it2->end()) {
                    newId = id;
                    goto END;
                }
                FC_TRACE("ignore " << id);
            }
        }
    }
    if(found.size()) {
        FC_TRACE("found " << found.front());
        newId = found.front();
    }else 
        newId = ++geoLastId;
END:
    geo->Id = newId;
    geoMap[newId] = (long)Geometry.getSize();
}

int SketchObject::setDatum(int ConstrId, double Datum)
{
    // set the changed value for the constraint
    if (this->Constraints.hasInvalidGeometry())
        return -6;
    const std::vector<Constraint *> &vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;
    ConstraintType type = vals[ConstrId]->Type;
    if (!vals[ConstrId]->isDimensional() &&
        type != Tangent && //for tangent, value==0 is autodecide, value==Pi/2 is external and value==-Pi/2 is internal
        type != Perpendicular)
        return -1;

    if ((type == Distance || type == Radius || type == Diameter) && Datum <= 0)
        return (Datum == 0) ? -5 : -4;

    // copy the list
    std::vector<Constraint *> newVals(vals);
    // clone the changed Constraint
    Constraint *constNew = vals[ConstrId]->clone();
    constNew->setValue(Datum);
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(newVals);
    delete constNew;

    int err = solve();
    if (err)
        this->Constraints.setValues(vals);

    return err;
}

int SketchObject::setDriving(int ConstrId, bool isdriving)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    int ret = testDrivingChange(ConstrId, isdriving);

    if(ret < 0)
        return ret;

    // copy the list
    std::vector<Constraint *> newVals(vals);
    // clone the changed Constraint
    Constraint *constNew = vals[ConstrId]->clone();
    constNew->isDriving = isdriving;
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(newVals);
    if (!isdriving)
        setExpression(Constraints.createPath(ConstrId), boost::shared_ptr<App::Expression>());
    delete constNew;

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::getDriving(int ConstrId, bool &isdriving)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    if (!vals[ConstrId]->isDimensional())
        return -1;

    isdriving=vals[ConstrId]->isDriving;
    return 0;
}

int SketchObject::toggleDriving(int ConstrId)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    int ret = testDrivingChange(ConstrId,!vals[ConstrId]->isDriving);

    if(ret<0)
        return ret;

    const Part::Geometry * geo1 = getGeometry(vals[ConstrId]->First);
    const Part::Geometry * geo2 = getGeometry(vals[ConstrId]->Second);
    const Part::Geometry * geo3 = getGeometry(vals[ConstrId]->Third);

    bool extorconstructionpoint1 = (vals[ConstrId]->First == Constraint::GeoUndef) || (vals[ConstrId]->First < 0) || (geo1 && geo1->getTypeId() == Part::GeomPoint::getClassTypeId() && geo1->Construction == true);
    bool extorconstructionpoint2 = (vals[ConstrId]->Second == Constraint::GeoUndef) || (vals[ConstrId]->Second < 0) || (geo2 && geo2->getTypeId() == Part::GeomPoint::getClassTypeId() && geo2->Construction == true);
    bool extorconstructionpoint3 = (vals[ConstrId]->Third == Constraint::GeoUndef) || (vals[ConstrId]->Third < 0) || (geo3 && geo3->getTypeId() == Part::GeomPoint::getClassTypeId() && geo3->Construction == true);

    if (extorconstructionpoint1 && extorconstructionpoint2 && extorconstructionpoint3 && vals[ConstrId]->isDriving==false)
        return -4;

    // copy the list
    std::vector<Constraint *> newVals(vals);
    // clone the changed Constraint
    Constraint *constNew = vals[ConstrId]->clone();
    constNew->isDriving = !constNew->isDriving;
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(newVals);
    if (!constNew->isDriving)
        setExpression(Constraints.createPath(ConstrId), boost::shared_ptr<App::Expression>());
    delete constNew;

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::testDrivingChange(int ConstrId, bool isdriving)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    if (!vals[ConstrId]->isDimensional())
        return -2;

    if (!(vals[ConstrId]->First>=0 || vals[ConstrId]->Second>=0 || vals[ConstrId]->Third>=0) && isdriving==true)
        return -3; // a constraint that does not have at least one element as not-external-geometry can never be driving.

    return 0;
}


/// Make all dimensionals Driving/non-Driving
int SketchObject::setDatumsDriving(bool isdriving)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();
    std::vector<Constraint *> newVals(vals);

    std::vector< Constraint * > tbd; // list of dynamically allocated memory that need to be deleted;

    for (size_t i=0; i<newVals.size(); i++) {
        if (!testDrivingChange(i, isdriving)) {

            Constraint *constNew = newVals[i]->clone();
            constNew->isDriving = isdriving;
            newVals[i] = constNew;
            tbd.push_back(constNew);
        }
    }
    this->Constraints.setValues(newVals);

    for (size_t i = 0; i < newVals.size(); i++) {
        if (!isdriving && newVals[i]->isDimensional())
            setExpression(Constraints.createPath(i), boost::shared_ptr<App::Expression>());
    }

    for (auto &t : tbd)
        delete t;

    if (noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::moveDatumsToEnd(void)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    std::vector<Constraint *> copy(vals);
    std::vector<Constraint *> newVals(vals.size());

    int addindex= copy.size()-1;

    // add the dimensionals at the end
    for (int i= copy.size()-1 ; i >= 0; i--) {
        if(copy[i]->isDimensional()) {
            newVals[addindex] = copy[i];
            addindex--;
        }
    }

    // add the non-dimensionals
    for (int i = copy.size()-1; i >= 0; i--) {
        if(!copy[i]->isDimensional()) {
            newVals[addindex] = copy[i];
            addindex--;
        }
    }

    this->Constraints.setValues(newVals);

    if (noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::setVirtualSpace(int ConstrId, bool isinvirtualspace)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    // copy the list
    std::vector<Constraint *> newVals(vals);

    // clone the changed Constraint
    Constraint *constNew = vals[ConstrId]->clone();
    constNew->isInVirtualSpace = isinvirtualspace;
    newVals[ConstrId] = constNew;

    this->Constraints.setValues(newVals);

    delete constNew;

    return 0;
}

int SketchObject::getVirtualSpace(int ConstrId, bool &isinvirtualspace) const
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    isinvirtualspace=vals[ConstrId]->isInVirtualSpace;
    return 0;
}

int SketchObject::toggleVirtualSpace(int ConstrId)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    // copy the list
    std::vector<Constraint *> newVals(vals);

    // clone the changed Constraint
    Constraint *constNew = vals[ConstrId]->clone();
    constNew->isInVirtualSpace = !constNew->isInVirtualSpace;
    newVals[ConstrId] = constNew;

    this->Constraints.setValues(newVals);

    delete constNew;

    return 0;
}


int SketchObject::setUpSketch()
{
    lastDoF = solvedSketch.setUpSketch(getCompleteGeometry(), Constraints.getValues(),
                                       getExternalGeometryCount());

    lastHasConflict = solvedSketch.hasConflicts();
    lastHasRedundancies = solvedSketch.hasRedundancies();
    lastConflicting=solvedSketch.getConflicting();
    lastRedundant=solvedSketch.getRedundant();

    if(lastHasRedundancies || lastDoF < 0 || lastHasConflict)
        Constraints.touch();

    return lastDoF;

}

int SketchObject::movePoint(int GeoId, PointPos PosId, const Base::Vector3d& toPoint, bool relative, bool updateGeoBeforeMoving)
{
    // if we are moving a point at SketchObject level, we need to start from a solved sketch
    // if we have conflicts we can forget about moving. However, there is the possibility that we
    // need to do programmatically moves of new geometry that has not been solved yet and that because
    // they were programmatically generated won't generate a conflict. This is the case of Fillet for
    // example. This is why exceptionally, it may be required to update the sketch geometry to that of
    // of SketchObject upon moving. => use updateGeometry parameter = true then


    if(updateGeoBeforeMoving || solverNeedsUpdate) {
        lastDoF = solvedSketch.setUpSketch(getCompleteGeometry(), Constraints.getValues(),
                                    getExternalGeometryCount());

        lastHasConflict = solvedSketch.hasConflicts();
        lastHasRedundancies = solvedSketch.hasRedundancies();
        lastConflicting=solvedSketch.getConflicting();
        lastRedundant=solvedSketch.getRedundant();

        solverNeedsUpdate=false;
    }

    if (lastDoF < 0) // over-constrained sketch
        return -1;
    if (lastHasConflict) // conflicting constraints
        return -1;

    // move the point and solve
    lastSolverStatus = solvedSketch.movePoint(GeoId, PosId, toPoint, relative);

    // moving the point can not result in a conflict that we did not have
    // or a redundancy that we did not have before, or a change of DoF

    if (lastSolverStatus == 0) {
        std::vector<Part::Geometry *> geomlist = solvedSketch.extractGeometry();
        Geometry.setValues(geomlist);
        //Constraints.acceptGeometry(getCompleteGeometry());
        for (std::vector<Part::Geometry *>::iterator it=geomlist.begin(); it != geomlist.end(); ++it) {
            if (*it) delete *it;
        }
    }

    solvedSketch.resetInitMove(); // reset solver point moving mechanism

    return lastSolverStatus;
}

Base::Vector3d SketchObject::getPoint(const Part::Geometry *geo, PointPos PosId) {
    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
        const Part::GeomPoint *p = static_cast<const Part::GeomPoint*>(geo);
        if (PosId == start || PosId == mid || PosId == end)
            return p->getPoint();
    } else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment*>(geo);
        if (PosId == start)
            return lineSeg->getStartPoint();
        else if (PosId == end)
            return lineSeg->getEndPoint();
    } else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle *circle = static_cast<const Part::GeomCircle*>(geo);
        auto pt = circle->getCenter();
        if(PosId != mid)
             pt.x += circle->getRadius();
        return pt;
    } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse*>(geo);
        auto pt = ellipse->getCenter();
        if(PosId != mid) 
            pt = ellipse->getMajorAxisDir()*ellipse->getMajorRadius();
        return pt;
    } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle *aoc = static_cast<const Part::GeomArcOfCircle*>(geo);
        if (PosId == start)
            return aoc->getStartPoint(/*emulateCCW=*/true);
        else if (PosId == end)
            return aoc->getEndPoint(/*emulateCCW=*/true);
        else if (PosId == mid)
            return aoc->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        const Part::GeomArcOfEllipse *aoc = static_cast<const Part::GeomArcOfEllipse*>(geo);
        if (PosId == start)
            return aoc->getStartPoint(/*emulateCCW=*/true);
        else if (PosId == end)
            return aoc->getEndPoint(/*emulateCCW=*/true);
        else if (PosId == mid)
            return aoc->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
        const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);
        if (PosId == start)
            return aoh->getStartPoint();
        else if (PosId == end)
            return aoh->getEndPoint();
        else if (PosId == mid)
            return aoh->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
        const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola*>(geo);
        if (PosId == start)
            return aop->getStartPoint();
        else if (PosId == end)
            return aop->getEndPoint();
        else if (PosId == mid)
            return aop->getCenter();
    } else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
        const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
        if (PosId == start)
            return bsp->getStartPoint();
        else if (PosId == end)
            return bsp->getEndPoint();
    }
    return Base::Vector3d();
}

Base::Vector3d SketchObject::getPoint(int GeoId, PointPos PosId) const
{
    if(!(GeoId == H_Axis || GeoId == V_Axis
         || (GeoId <= getHighestCurveIndex() && GeoId >= -getExternalGeometryCount()) ))
        throw Base::ValueError("SketchObject::getPoint. Invalid GeoId was supplied.");
    const Part::Geometry *geo = getGeometry(GeoId);
    return getPoint(geo,PosId);
}

int SketchObject::getAxisCount(void) const
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    int count=0;
    for (std::vector<Part::Geometry *>::const_iterator geo=vals.begin();
        geo != vals.end(); geo++)
        if ((*geo) && (*geo)->Construction &&
            (*geo)->getTypeId() == Part::GeomLineSegment::getClassTypeId())
            count++;

    return count;
}

Base::Axis SketchObject::getAxis(int axId) const
{
    if (axId == H_Axis || axId == V_Axis || axId == N_Axis)
        return Part::Part2DObject::getAxis(axId);

    const std::vector< Part::Geometry * > &vals = getInternalGeometry();
    int count=0;
    for (std::vector<Part::Geometry *>::const_iterator geo=vals.begin();
        geo != vals.end(); geo++)
        if ((*geo) && (*geo)->Construction &&
            (*geo)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            if (count == axId) {
                Part::GeomLineSegment *lineSeg = static_cast<Part::GeomLineSegment*>(*geo);
                Base::Vector3d start = lineSeg->getStartPoint();
                Base::Vector3d end = lineSeg->getEndPoint();
                return Base::Axis(start, end-start);
            }
            count++;
        }

    return Base::Axis();
}

void SketchObject::acceptGeometry()
{
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
}

bool SketchObject::isSupportedGeometry(const Part::Geometry *geo) const
{
    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId() ||
        geo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
        geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
        geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
        geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        return true;
    }
    if (geo->getTypeId() == Part::GeomTrimmedCurve::getClassTypeId()) {
        Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast(geo->handle());
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trim->BasisCurve());
        Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(trim->BasisCurve());
        if (!circle.IsNull() || !ellipse.IsNull()) {
            return true;
        }
    }
    return false;
}

std::vector<Part::Geometry *> SketchObject::supportedGeometry(const std::vector<Part::Geometry *> &geoList) const
{
    std::vector<Part::Geometry *> supportedGeoList;
    supportedGeoList.reserve(geoList.size());
    // read-in geometry that the sketcher cannot handle
    for (std::vector<Part::Geometry*>::const_iterator it = geoList.begin(); it != geoList.end(); ++it) {
        if (isSupportedGeometry(*it)) {
            supportedGeoList.push_back(*it);
        }
    }

    return supportedGeoList;
}

int SketchObject::addGeometry(const std::vector<Part::Geometry *> &geoList, bool construction/*=false*/)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);
    std::vector< Part::Geometry * > copies;
    copies.reserve(geoList.size());
    for (std::vector<Part::Geometry *>::const_iterator it = geoList.begin(); it != geoList.end(); ++it) {
        Part::Geometry* copy = (*it)->copy();
        GEN_ID(copy);
        if(construction && copy->getTypeId() != Part::GeomPoint::getClassTypeId()) {
            copy->Construction = construction;
        }

        copies.push_back(copy);
    }

    newVals.insert(newVals.end(), copies.begin(), copies.end());
    Geometry.setValues(newVals);
    for (std::vector<Part::Geometry *>::iterator it = copies.begin(); it != copies.end(); ++it)
        delete *it;
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    return Geometry.getSize()-1;
}

int SketchObject::addGeometry(const Part::Geometry *geo, bool construction/*=false*/)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);
    Part::Geometry *geoNew = geo->copy();
    GEN_ID(geoNew);
    
    if(geoNew->getTypeId() != Part::GeomPoint::getClassTypeId())
        geoNew->Construction = construction;

    newVals.push_back(geoNew);
    Geometry.setValues(newVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    delete geoNew;
    rebuildVertexIndex();

    return Geometry.getSize()-1;
}

int SketchObject::delGeometry(int GeoId, bool deleteinternalgeo)
{
    if (GeoId < 0) {
        if(GeoId > GeoEnum::RefExt)
            return -1;
        return delExternal(-GeoId-1);
    }

    const std::vector< Part::Geometry * > &vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    const Part::Geometry *geo = getGeometry(GeoId);
    // Only for supported types
    if ((geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
        geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId())) {

        if(deleteinternalgeo) {
            this->deleteUnusedInternalGeometry(GeoId, true);
            return 0;
        }
    }

    std::vector< Part::Geometry * > newVals(vals);
    newVals.erase(newVals.begin()+GeoId);

    // Find coincident points to replace the points of the deleted geometry
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    for (PointPos PosId = start; PosId != mid; ) {
        getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
        if (GeoIdList.size() > 1) {
            delConstraintOnPoint(GeoId, PosId, true /* only coincidence */);
            transferConstraints(GeoIdList[0], PosIdList[0], GeoIdList[1], PosIdList[1]);
        }
        PosId = (PosId == start) ? end : mid; // loop through [start, end, mid]
    }

    const std::vector< Constraint * > &constraints = this->Constraints.getValues();
    std::vector< Constraint * > newConstraints(0);
    for (std::vector<Constraint *>::const_iterator it = constraints.begin();
         it != constraints.end(); ++it) {
        if ((*it)->First != GeoId && (*it)->Second != GeoId && (*it)->Third != GeoId) {
            Constraint *copiedConstr = (*it)->clone();
            if (copiedConstr->First > GeoId)
                copiedConstr->First -= 1;
            if (copiedConstr->Second > GeoId)
                copiedConstr->Second -= 1;
            if (copiedConstr->Third > GeoId)
                copiedConstr->Third -= 1;
            newConstraints.push_back(copiedConstr);
        }
    }

    this->Geometry.setValues(newVals);
    this->Constraints.setValues(std::move(newConstraints));
    this->Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::deleteAllGeometry()
{
    std::vector< Part::Geometry * > newVals(0);
    std::vector< Constraint * > newConstraints(0);

    this->Geometry.setValues(newVals);
    this->Constraints.setValues(newConstraints);

    this->Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::deleteAllConstraints()
{
    std::vector< Constraint * > newConstraints(0);

    this->Constraints.setValues(newConstraints);

    this->Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::toggleConstructions(const std::vector<int> &GeoIds)
{
    std::set<int> idSet(GeoIds.begin(),GeoIds.end());
    for(int GeoId : idSet) {
        if(GeoId >= Geometry.getSize() || 
           (GeoId < 0 && (GeoId > GeoEnum::RefExt || -GeoId-1 >= ExternalGeo.getSize())))
           return -1;
    }

    bool geometryTouched = false;
    std::vector<Part::Geometry*> geos;
    bool externalTouched = false;
    std::vector<Part::Geometry*> extGeos;
    for(int GeoId : idSet) {
        if (GeoId >= 0) {
            if(geos.empty())
                geos = Geometry.getValues();
            auto &geo = geos[GeoId];
            geo = geo->clone();
            geo->Construction = !geo->Construction;
            geometryTouched = true;
        } else {
            if(extGeos.empty())
                extGeos = ExternalGeo.getValues();
            auto &geo = extGeos[-GeoId-1];
            geo = geo->clone();
            geo->setFlag(Part::Geometry::Defining,!geo->testFlag(Part::Geometry::Defining));
            externalTouched = true;
        }
    }
    if(geometryTouched)
        Geometry.setValues(std::move(geos));
    if(externalTouched)
        ExternalGeo.setValues(std::move(extGeos));

    //this->Constraints.acceptGeometry(getCompleteGeometry()); <= This is not necessary for a toggle. Reducing redundant solving. Abdullah
    solverNeedsUpdate=true;
    return 0;
}

int SketchObject::setConstruction(int GeoId, bool on)
{
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

    if(prop->getValues()[idx]->getTypeId() == Part::GeomPoint::getClassTypeId())
        return -1;

    std::unique_ptr<Part::Geometry> geo(prop->getValues()[idx]->clone());
    if(prop == &Geometry) 
        geo->Construction = on;
    else 
        geo->setFlag(Part::Geometry::Defining, on);

    prop->set1Value(idx,std::move(geo));

    //this->Constraints.acceptGeometry(getCompleteGeometry()); <= This is not necessary for a toggle. Reducing redundant solving. Abdullah
    solverNeedsUpdate=true;
    return 0;
}

int SketchObject::toggleFreeze(const std::vector<int> &geoIds) {
    bool touched = false;
    auto geos = ExternalGeo.getValues();
    std::set<int> idSet(geoIds.begin(),geoIds.end());
    for(auto geoId : geoIds) {
        if(geoId > GeoEnum::RefExt || -geoId-1>=ExternalGeo.getSize())
            continue;
        if(!idSet.count(geoId))
            continue;
        idSet.erase(geoId);
        int idx = -geoId-1;
        auto &geo = geos[idx];
        bool frozen = !geo->testFlag(Part::Geometry::Frozen);
        if(geo->Ref.size()) {
            for(auto gid : getRelatedGeometry(geoId)) {
                if(gid == geoId)
                    continue;
                int idx = -gid-1;
                auto &g = geos[idx];
                g = g->clone();
                g->setFlag(Part::Geometry::Frozen,frozen);
                idSet.erase(gid);
            }
        }
        geo = geo->clone();
        geo->setFlag(Part::Geometry::Frozen,frozen);
        touched = true;
    }

    if(!touched)
        return -1;
    ExternalGeo.setValues(geos);
    return 0;
}

int SketchObject::detachExternal(const std::vector<int> &geoIds) {
    bool touched = false;
    auto geos = ExternalGeo.getValues();
    for(int geoId : geoIds) {
        if(geoId > GeoEnum::RefExt || -geoId-1>=ExternalGeo.getSize())
            continue;
        auto &geo = geos[-geoId-1];
        geo = geo->clone();
        geo->setFlag(Part::Geometry::Detached);
        touched = true;
    }
    if(!touched)
        return -1;
    ExternalGeo.setValues(std::move(geos));
    return 0;
}

//ConstraintList is used only to make copies.
int SketchObject::addConstraints(const std::vector<Constraint *> &ConstraintList)
{
    const std::vector< Constraint * > &vals = this->Constraints.getValues();

    std::vector< Constraint * > newVals(vals);
    newVals.insert(newVals.end(), ConstraintList.begin(), ConstraintList.end());

    //test if tangent constraints have been added; AutoLockTangency.
    std::vector< Constraint * > tbd;//list of temporary copies that need to be deleted
    for(std::size_t i = newVals.size()-ConstraintList.size(); i<newVals.size(); i++){
        if( newVals[i]->Type == Tangent || newVals[i]->Type == Perpendicular ){
            Constraint *constNew = newVals[i]->clone();
            AutoLockTangencyAndPerpty(constNew);
            tbd.push_back(constNew);
            newVals[i] = constNew;
        }
    }

    this->Constraints.setValues(newVals);

    //clean up - delete temporary copies of constraints that were made to affect the constraints
    for(std::size_t i=0; i<tbd.size(); i++){
        delete (tbd[i]);
    }

    return this->Constraints.getSize()-1;
}

int SketchObject::addCopyOfConstraints(const SketchObject &orig)
{
    const std::vector< Constraint * > &vals = this->Constraints.getValues();

    const std::vector< Constraint * > &origvals = orig.Constraints.getValues();

    std::vector< Constraint * > newVals(vals);

    for(std::size_t j = 0; j<origvals.size(); j++)
        newVals.push_back(origvals[j]->copy());

    std::size_t valssize = vals.size();

    this->Constraints.setValues(newVals);

    for(std::size_t i = valssize, j = 0; i<newVals.size(); i++,j++){
        if ( newVals[i]->isDriving && newVals[i]->isDimensional()) {

            App::ObjectIdentifier spath = orig.Constraints.createPath(j);

            App::PropertyExpressionEngine::ExpressionInfo expr_info = orig.getExpression(spath);

            if (expr_info.expression) { // if there is an expression on the source dimensional
                App::ObjectIdentifier dpath = this->Constraints.createPath(i);
                setExpression(dpath, boost::shared_ptr<App::Expression>(expr_info.expression->copy()));
            }

        }
    }

    return this->Constraints.getSize()-1;
}

int SketchObject::addConstraint(const Constraint *constraint)
{
    const std::vector< Constraint * > &vals = this->Constraints.getValues();

    std::vector< Constraint * > newVals(vals);
    Constraint *constNew = constraint->clone();

    if (constNew->Type == Tangent || constNew->Type == Perpendicular)
        AutoLockTangencyAndPerpty(constNew);

    newVals.push_back(constNew);
    this->Constraints.setValues(newVals);
    delete constNew;
    return this->Constraints.getSize()-1;
}

int SketchObject::delConstraint(int ConstrId)
{
    const std::vector< Constraint * > &vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    std::vector< Constraint * > newVals(vals);
    newVals.erase(newVals.begin()+ConstrId);
    this->Constraints.setValues(newVals);

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::delConstraints(std::vector<int> ConstrIds, bool updategeometry)
{
    const std::vector< Constraint * > &vals = this->Constraints.getValues();

    std::vector< Constraint * > newVals(vals);

    std::sort(ConstrIds.begin(),ConstrIds.end());

    if (*ConstrIds.begin() < 0 || *std::prev(ConstrIds.end()) >= int(vals.size()))
        return -1;

    for(auto rit = ConstrIds.rbegin(); rit!=ConstrIds.rend(); rit++)
        newVals.erase(newVals.begin()+*rit);

    this->Constraints.setValues(newVals);

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve(updategeometry);

    return 0;
}

int SketchObject::delConstraintOnPoint(int VertexId, bool onlyCoincident)
{
    int GeoId;
    PointPos PosId;
    if (VertexId == GeoEnum::RtPnt) { // RootPoint
        GeoId = Sketcher::GeoEnum::RtPnt;
        PosId = start;
    } else
        getGeoVertexIndex(VertexId, GeoId, PosId);

    return delConstraintOnPoint(GeoId, PosId, onlyCoincident);
}

int SketchObject::delConstraintOnPoint(int GeoId, PointPos PosId, bool onlyCoincident)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    // check if constraints can be redirected to some other point
    int replaceGeoId=Constraint::GeoUndef;
    PointPos replacePosId=Sketcher::none;
    if (!onlyCoincident) {
        for (std::vector<Constraint *>::const_iterator it = vals.begin(); it != vals.end(); ++it) {
            if ((*it)->Type == Sketcher::Coincident) {
                if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                    replaceGeoId = (*it)->Second;
                    replacePosId = (*it)->SecondPos;
                    break;
                }
                else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                    replaceGeoId = (*it)->First;
                    replacePosId = (*it)->FirstPos;
                    break;
                }
            }
        }
    }

    // remove or redirect any constraints associated with the given point
    std::vector<Constraint *> newVals(0);
    for (std::vector<Constraint *>::const_iterator it = vals.begin(); it != vals.end(); ++it) {
        if ((*it)->Type == Sketcher::Coincident) {
            if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                if (replaceGeoId != Constraint::GeoUndef &&
                    (replaceGeoId != (*it)->Second || replacePosId != (*it)->SecondPos)) { // redirect this constraint
                    (*it)->First = replaceGeoId;
                    (*it)->FirstPos = replacePosId;
                }
                else
                    continue; // skip this constraint
            }
            else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                if (replaceGeoId != Constraint::GeoUndef &&
                    (replaceGeoId != (*it)->First || replacePosId != (*it)->FirstPos)) { // redirect this constraint
                    (*it)->Second = replaceGeoId;
                    (*it)->SecondPos = replacePosId;
                }
                else
                    continue; // skip this constraint
            }
        }
        else if (!onlyCoincident) {
            if ((*it)->Type == Sketcher::Distance ||
                (*it)->Type == Sketcher::DistanceX || (*it)->Type == Sketcher::DistanceY) {
                if ((*it)->First == GeoId && (*it)->FirstPos == none &&
                    (PosId == start || PosId == end)) {
                    // remove the constraint even if it is not directly associated
                    // with the given point
                    continue; // skip this constraint
                }
                else if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                    if (replaceGeoId != Constraint::GeoUndef) { // redirect this constraint
                        (*it)->First = replaceGeoId;
                        (*it)->FirstPos = replacePosId;
                    }
                    else
                        continue; // skip this constraint
                }
                else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                    if (replaceGeoId != Constraint::GeoUndef) { // redirect this constraint
                        (*it)->Second = replaceGeoId;
                        (*it)->SecondPos = replacePosId;
                    }
                    else
                        continue; // skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::PointOnObject) {
                if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                    if (replaceGeoId != Constraint::GeoUndef) { // redirect this constraint
                        (*it)->First = replaceGeoId;
                        (*it)->FirstPos = replacePosId;
                    }
                    else
                        continue; // skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::Tangent || (*it)->Type == Sketcher::Perpendicular) {
                if (((*it)->First == GeoId && (*it)->FirstPos == PosId) ||
                    ((*it)->Second == GeoId && (*it)->SecondPos == PosId)) {
                    // we could keep the tangency constraint by converting it
                    // to a simple one but it is not really worth
                    continue; // skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::Symmetric) {
                if (((*it)->First == GeoId && (*it)->FirstPos == PosId) ||
                    ((*it)->Second == GeoId && (*it)->SecondPos == PosId)) {
                    continue; // skip this constraint
                }
            }
        }
        newVals.push_back(*it);
    }
    if (newVals.size() < vals.size()) {
        this->Constraints.setValues(newVals);

        return 0;
    }

    return -1; // no such constraint
}

int SketchObject::transferConstraints(int fromGeoId, PointPos fromPosId, int toGeoId, PointPos toPosId)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();
    std::vector<Constraint *> newVals(vals);
    std::vector<Constraint *> changed;
    for (int i=0; i < int(newVals.size()); i++) {
        if (vals[i]->First == fromGeoId && vals[i]->FirstPos == fromPosId &&
            !(vals[i]->Second == toGeoId && vals[i]->SecondPos == toPosId) &&
            !(toGeoId < 0 && vals[i]->Second <0) ) {
            // Nothing guarantees that a tangent can be freely transferred to another coincident point, as
            // the transfer destination edge most likely won't be intended to be tangent. However, if it is
            // an end to end point tangency, the user expects it to be substituted by a coincidence constraint.
            Constraint *constNew = newVals[i]->clone();
            constNew->First = toGeoId;
            constNew->FirstPos = toPosId;

            if(vals[i]->Type == Sketcher::Tangent || vals[i]->Type == Sketcher::Perpendicular){
                constNew->Type = Sketcher::Coincident;
            }
            // With respect to angle constraints, if it is a DeepSOIC style angle constraint (segment+segment+point),
            // then no problem arises as the segments are PosId=none. In this case there is no call to this function.
            //
            // However, other angle constraints are problematic because they are created on segments, but internally
            // operate on vertices, PosId=start
            // Such constraint may not be successfully transferred on deletion of the segments.
            else if(vals[i]->Type == Sketcher::Angle) {
                continue;
            }

            newVals[i] = constNew;
            changed.push_back(constNew);
        }
        else if (vals[i]->Second == fromGeoId && vals[i]->SecondPos == fromPosId &&
                 !(vals[i]->First == toGeoId && vals[i]->FirstPos == toPosId) &&
                 !(toGeoId < 0 && vals[i]->First< 0)) {

            Constraint *constNew = newVals[i]->clone();
            constNew->Second = toGeoId;
            constNew->SecondPos = toPosId;
            // Nothing guarantees that a tangent can be freely transferred to another coincident point, as
            // the transfer destination edge most likely won't be intended to be tangent. However, if it is
            // an end to end point tangency, the user expects it to be substituted by a coincidence constraint.
            if(vals[i]->Type == Sketcher::Tangent || vals[i]->Type == Sketcher::Perpendicular) {
                constNew->Type = Sketcher::Coincident;
            }
            else if(vals[i]->Type == Sketcher::Angle) {
                continue;
            }

            newVals[i] = constNew;
            changed.push_back(constNew);
        }
    }

    // assign the new values only if something has changed
    if (!changed.empty()) {
        this->Constraints.setValues(newVals);
        // free memory
        for (Constraint* it : changed)
            delete it;
    }
    return 0;
}

int SketchObject::fillet(int GeoId, PointPos PosId, double radius, bool trim)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    // Find the other geometry Id associated with the coincident point
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);

    // only coincident points between two (non-external) edges can be filleted
    if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
        const Part::Geometry *geo1 = getGeometry(GeoIdList[0]);
        const Part::Geometry *geo2 = getGeometry(GeoIdList[1]);
        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
            geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {
            const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo1);
            const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);

            Base::Vector3d midPnt1 = (lineSeg1->getStartPoint() + lineSeg1->getEndPoint()) / 2 ;
            Base::Vector3d midPnt2 = (lineSeg2->getStartPoint() + lineSeg2->getEndPoint()) / 2 ;
            return fillet(GeoIdList[0], GeoIdList[1], midPnt1, midPnt2, radius, trim);
        }
    }

    return -1;
}

int SketchObject::fillet(int GeoId1, int GeoId2,
                         const Base::Vector3d& refPnt1, const Base::Vector3d& refPnt2,
                         double radius, bool trim)
{
    if (GeoId1 < 0 || GeoId1 > getHighestCurveIndex() ||
        GeoId2 < 0 || GeoId2 > getHighestCurveIndex())
        return -1;

    const Part::Geometry *geo1 = getGeometry(GeoId1);
    const Part::Geometry *geo2 = getGeometry(GeoId2);
    if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
        geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {
        const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo1);
        const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);

        Base::Vector3d filletCenter;
        if (!Part::findFilletCenter(lineSeg1, lineSeg2, radius, refPnt1, refPnt2, filletCenter))
            return -1;
        Base::Vector3d dir1 = lineSeg1->getEndPoint() - lineSeg1->getStartPoint();
        Base::Vector3d dir2 = lineSeg2->getEndPoint() - lineSeg2->getStartPoint();

        // the intersection point will and two distances will be necessary later for trimming the lines
        Base::Vector3d intersection, dist1, dist2;

        // create arc from known parameters and lines
        int filletId;
        Part::GeomArcOfCircle *arc = Part::createFilletGeometry(lineSeg1, lineSeg2, filletCenter, radius);
        if (arc) {
            // calculate intersection and distances before we invalidate lineSeg1 and lineSeg2
            if (!find2DLinesIntersection(lineSeg1, lineSeg2, intersection)) {
                delete arc;
                return -1;
            }
            dist1.ProjectToLine(arc->getStartPoint(/*emulateCCW=*/true)-intersection, dir1);
            dist2.ProjectToLine(arc->getStartPoint(/*emulateCCW=*/true)-intersection, dir2);
            Part::Geometry *newgeo = arc;
            filletId = addGeometry(newgeo);
            if (filletId < 0) {
                delete arc;
                return -1;
            }
        }
        else
            return -1;

        if (trim) {
            PointPos PosId1 = (filletCenter-intersection)*dir1 > 0 ? start : end;
            PointPos PosId2 = (filletCenter-intersection)*dir2 > 0 ? start : end;

            delConstraintOnPoint(GeoId1, PosId1, false);
            delConstraintOnPoint(GeoId2, PosId2, false);
            Sketcher::Constraint *tangent1 = new Sketcher::Constraint();
            Sketcher::Constraint *tangent2 = new Sketcher::Constraint();

            tangent1->Type = Sketcher::Tangent;
            tangent1->First = GeoId1;
            tangent1->FirstPos = PosId1;
            tangent1->Second = filletId;

            tangent2->Type = Sketcher::Tangent;
            tangent2->First = GeoId2;
            tangent2->FirstPos = PosId2;
            tangent2->Second = filletId;

            if (dist1.Length() < dist2.Length()) {
                tangent1->SecondPos = start;
                tangent2->SecondPos = end;
                movePoint(GeoId1, PosId1, arc->getStartPoint(/*emulateCCW=*/true),false,true);
                movePoint(GeoId2, PosId2, arc->getEndPoint(/*emulateCCW=*/true),false,true);
            }
            else {
                tangent1->SecondPos = end;
                tangent2->SecondPos = start;
                movePoint(GeoId1, PosId1, arc->getEndPoint(/*emulateCCW=*/true),false,true);
                movePoint(GeoId2, PosId2, arc->getStartPoint(/*emulateCCW=*/true),false,true);
            }

            addConstraint(tangent1);
            addConstraint(tangent2);
            delete tangent1;
            delete tangent2;
        }
        delete arc;

        if (noRecomputes) // if we do not have a recompute after the geometry creation, the sketch must be solved to update the DoF of the solver
            solve();

        return 0;
    }
    else if(geo1->isDerivedFrom(Part::GeomBoundedCurve::getClassTypeId())  &&
            geo2->isDerivedFrom(Part::GeomBoundedCurve::getClassTypeId())) {

        auto distancetorefpoints = [](Base::Vector3d ip1, Base::Vector3d ip2, Base::Vector3d ref1, Base::Vector3d ref2) {
                    return (ip1 - ref1).Length() + (ip2 - ref2).Length();
                };

        auto selectintersection = [&distancetorefpoints](std::vector<std::pair<Base::Vector3d, Base::Vector3d>> & points,
                                     std::pair<Base::Vector3d, Base::Vector3d>& interpoints,
                                     const Base::Vector3d& refPnt1, const Base::Vector3d& refPnt2) {

            if (points.empty()) {
                return -1;
            }
            else {
                double dist = distancetorefpoints(points[0].first, points[0].second, refPnt1, refPnt2);
                int i = 0, si = 0;

                for (auto ipoints : points) {
                    double d = distancetorefpoints(ipoints.first, ipoints.second, refPnt1, refPnt2);

                    if (d<dist) {
                        si = i;
                        dist = d;
                    }

                    i++;
                }

                interpoints = points[si];

                return 0;
            }
        };

        // NOTE: While it is not a requirement that the endpoints of the corner to trim are coincident
        //       for GeomTrimmedCurves, it is for GeomBoundedCurves. The reason is that there is no basiscurve
        //       that can be extended to find an intersection.
        //
        //       However, GeomTrimmedCurves sometimes run into problems when trying to calculate the intersection
        //       of basis curves, for example in the case of hyperbola sometimes the cosh goes out of range while
        //       calculating this intersection of basis curves.
        //
        //        Consequently:
        //        i. for GeomBoundedCurves, other than GeomTrimmedCurves, a coincident endpoint is mandatory.
        //        ii. for GeomTrimmedCurves, if there is a coincident endpoint, it is used for the fillet,
        //        iii. for GeomTrimmedCurves, if there is not a coincident endpoint, an intersection of basis curves
        //             is attempted.

        const Part::GeomCurve *curve1 = static_cast<const Part::GeomCurve*>(geo1);
        const Part::GeomCurve *curve2 = static_cast<const Part::GeomCurve*>(geo2);

        double refparam1;
        double refparam2;

        try {
            if(!curve1->closestParameter(refPnt1,refparam1))
                return -1;
        }
        catch (Base::CADKernelError e) {
            e.ReportException();
            THROWM(Base::CADKernelError, "Unable to determine the parameter of the first selected curve at the reference point.")
        }

        try {
             if(!curve2->closestParameter(refPnt2,refparam2))
                return -1;
        }
        catch (Base::CADKernelError e) {
            e.ReportException();
            THROWM(Base::CADKernelError, "Unable to determine the parameter of the second selected curve at the reference point.")
        }

#ifdef DEBUG
        Base::Console().Log("\n\nFILLET DEBUG\n\n");
        Base::Console().Log("Ref param: (%f);(%f)",refparam1,refparam2);
#endif

        std::pair<Base::Vector3d, Base::Vector3d> interpoints;
        std::vector<std::pair<Base::Vector3d, Base::Vector3d>> points;


        // look for coincident constraints between curves, take the coincident closest to the refpoints
        Sketcher::PointPos curve1PosId = Sketcher::none;
        Sketcher::PointPos curve2PosId = Sketcher::none;

        double dist=INFINITY;

        const std::vector<Constraint *> &constraints = this->Constraints.getValues();

        for (std::vector<Constraint *>::const_iterator it=constraints.begin(); it != constraints.end(); ++it) {
            if ((*it)->Type == Sketcher::Coincident || (*it)->Type == Sketcher::Perpendicular || (*it)->Type == Sketcher::Tangent) {
                if ((*it)->First == GeoId1 && (*it)->Second == GeoId2 &&
                    (*it)->FirstPos != Sketcher::none && (*it)->SecondPos != Sketcher::none ) {
                    Base::Vector3d tmpp1 = getPoint((*it)->First,(*it)->FirstPos);
                    Base::Vector3d tmpp2 = getPoint((*it)->Second,(*it)->SecondPos);
                    double tmpdist = distancetorefpoints(tmpp1,
                                                         tmpp2,
                                                         refPnt1,
                                                         refPnt2);
                    if(tmpdist < dist) {
                        curve1PosId = (*it)->FirstPos;
                        curve2PosId = (*it)->SecondPos;
                        dist = tmpdist;
                        interpoints = std::make_pair(tmpp1,tmpp2);
                    }
                }
                else if ((*it)->First == GeoId2 && (*it)->Second == GeoId1 &&
                         (*it)->FirstPos != Sketcher::none && (*it)->SecondPos != Sketcher::none ) {
                    Base::Vector3d tmpp2 = getPoint((*it)->First,(*it)->FirstPos);
                    Base::Vector3d tmpp1 = getPoint((*it)->Second,(*it)->SecondPos);
                    double tmpdist = distancetorefpoints(tmpp1,
                                                         tmpp2,
                                                         refPnt1,
                                                         refPnt2);
                    if(tmpdist < dist) {
                        curve2PosId = (*it)->FirstPos;
                        curve1PosId = (*it)->SecondPos;
                        dist = tmpdist;
                        interpoints = std::make_pair(tmpp1,tmpp2);
                    }
                }
            }
        }

        if( curve1PosId == Sketcher::none ) {
            // no coincident was found, try basis curve intersection if GeomTrimmedCurve
            if( geo1->isDerivedFrom(Part::GeomTrimmedCurve::getClassTypeId()) &&
                geo2->isDerivedFrom(Part::GeomTrimmedCurve::getClassTypeId())) {

                const Part::GeomTrimmedCurve *tcurve1 = static_cast<const Part::GeomTrimmedCurve*>(geo1);
                const Part::GeomTrimmedCurve *tcurve2 = static_cast<const Part::GeomTrimmedCurve*>(geo2);

                try {
                    if(!tcurve1->intersectBasisCurves(tcurve2,points))
                        return -1;
                }
                catch (Base::CADKernelError e) {
                    e.ReportException();
                    THROWMT(Base::CADKernelError,QT_TRANSLATE_NOOP("Exceptions", "Unable to guess intersection of curves. Try adding a coincident constraint between the vertices of the curves you are intending to fillet."))
                }

                int res = selectintersection(points,interpoints,refPnt1, refPnt2);

                if (res != 0)
                    return res;
            }
            else
                return -1; // not a GeomTrimmedCurve and no coincident point.
        }

        // Now that we know where the curves intersect, get the parameters in the curves of those points
        double intparam1;
        double intparam2;

        try {
            if(!curve1->closestParameter(interpoints.first,intparam1))
                return -1;
        }
        catch (Base::CADKernelError e) {
            e.ReportException();
            THROWM(Base::CADKernelError,"Unable to determine the parameter of the first selected curve at the intersection of the curves.")
        }

        try {
            if(!curve2->closestParameter(interpoints.second,intparam2))
                return -1;
        }
        catch (Base::CADKernelError e) {
            e.ReportException();
            THROWM(Base::CADKernelError,"Unable to determine the parameter of the second selected curve at the intersection of the curves.")
        }

        // get the starting parameters of each curve
        double spc1 = curve1->getFirstParameter();
        double spc2 = curve2->getFirstParameter();

        // get a fillet radius if zero was given
        Base::Vector3d ref21 = refPnt2 - refPnt1;

        if (radius == .0f) {
            // guess a radius
            // https://forum.freecadweb.org/viewtopic.php?f=3&t=31594&start=50#p266658
            //
            // We do not know the actual tangency points until we intersect the offset curves, but
            // we do not have offset curves before with decide on a radius.
            //
            // This estimation guesses a radius as the average of the distances from the reference points
            // with respect to the intersection of the normals at those reference points.

            try {
                Base::Vector3d tdir1;
                Base::Vector3d tdir2;

                // We want normals, but OCCT normals require curves to be 2 times derivable, and lines are not
                // tangency calculation requires 1 time derivable.

                if(!curve1->tangent(refparam1, tdir1))
                    return -1;

                if(!curve2->tangent(refparam2, tdir2))
                    return -1;

                Base::Vector3d dir1(tdir1.y,-tdir1.x,0);
                Base::Vector3d dir2(tdir2.y,-tdir2.x,0);

                double det = -dir1.x*dir2.y + dir2.x*dir1.y;

                if (std::abs(det) < Precision::Confusion())
                    throw Base::RuntimeError("No intersection of normals"); // no intersection of normals

                Base::Vector3d refp1 = curve1->pointAtParameter(refparam1);
                Base::Vector3d refp2 = curve2->pointAtParameter(refparam2);

                //Base::Console().Log("refpoints: (%f,%f,%f);(%f,%f,%f)",refp1.x,refp1.y,refp1.z,refp2.x,refp2.y,refp2.z);

                Base::Vector3d normalintersect(
                    (-dir1.x*dir2.x*refp1.y + dir1.x*dir2.x*refp2.y - dir1.x*dir2.y*refp2.x + dir2.x*dir1.y*refp1.x)/det,
                    (-dir1.x*dir2.y*refp1.y + dir2.x*dir1.y*refp2.y + dir1.y*dir2.y*refp1.x - dir1.y*dir2.y*refp2.x)/det,0);

                radius = ((refp1 - normalintersect).Length() + (refp2 - normalintersect).Length())/2;
            }
            catch(const Base::Exception&) {
                radius = ref21.Length(); // fall-back to simplest estimation.
            }
        }


#ifdef DEBUG
        Base::Console().Log("Start param: (%f);(%f)\n",spc1,spc2);

        Base::Vector3d c1pf = curve1->pointAtParameter(spc1);
        Base::Vector3d c2pf = curve2->pointAtParameter(spc2);

        Base::Console().Log("start point curves: (%f,%f,%f);(%f,%f,%f)\n",c1pf.x,c1pf.y,c1pf.z,c2pf.x,c2pf.y,c2pf.z);
#endif
        // We create Offset curves at the suggested radius, the direction of offset is estimated from the tangency vector
        Base::Vector3d tdir1 = curve1->firstDerivativeAtParameter(refparam1);
        Base::Vector3d tdir2 = curve2->firstDerivativeAtParameter(refparam2);

#ifdef DEBUG
        Base::Console().Log("tangent vectors: (%f,%f,%f);(%f,%f,%f)\n",tdir1.x,tdir1.y,tdir1.z,tdir2.x,tdir2.y,tdir2.z);
        Base::Console().Log("inter-ref vector: (%f,%f,%f)\n",ref21.x,ref21.y,ref21.z);
#endif

        Base::Vector3d vn(0,0,1);

        double sdir1 = tdir1.Cross(ref21).Dot(vn);
        double sdir2 = tdir2.Cross(-ref21).Dot(vn);

#ifdef DEBUG
        Base::Console().Log("sign of offset: (%f,%f)\n",sdir1,sdir2);
        Base::Console().Log("radius: %f\n",radius);
#endif

        Part::GeomOffsetCurve * ocurve1 = new Part::GeomOffsetCurve(Handle(Geom_Curve)::DownCast(curve1->handle()), (sdir1<0)?radius:-radius, vn);

        Part::GeomOffsetCurve * ocurve2 = new Part::GeomOffsetCurve(Handle(Geom_Curve)::DownCast(curve2->handle()), (sdir2<0)?radius:-radius, vn);

#ifdef DEBUG
        Base::Vector3d oc1pf = ocurve1->pointAtParameter(ocurve1->getFirstParameter());
        Base::Vector3d oc2pf = ocurve2->pointAtParameter(ocurve2->getFirstParameter());

        Base::Console().Log("start point offset curves: (%f,%f,%f);(%f,%f,%f)\n",oc1pf.x,oc1pf.y,oc1pf.z,oc2pf.x,oc2pf.y,oc2pf.z);

        /*auto printoffsetcurve = [](Part::GeomOffsetCurve *c) {

            for(double param = c->getFirstParameter(); param < c->getLastParameter(); param = param + (c->getLastParameter()-c->getFirstParameter())/10)
                Base::Console().Log("\n%f: (%f,%f,0)\n", param, c->pointAtParameter(param).x,c->pointAtParameter(param).y);

        };

        printoffsetcurve(ocurve1);
        printoffsetcurve(ocurve2);*/
#endif

        // Next we calculate the intersection of offset curves to get the center of the fillet
        std::pair<Base::Vector3d, Base::Vector3d> filletcenterpoint;
        std::vector<std::pair<Base::Vector3d, Base::Vector3d>> offsetintersectionpoints;

        try {
            if(!ocurve1->intersect(ocurve2,offsetintersectionpoints)) {
#ifdef DEBUG
                Base::Console().Log("No intersection between offset curves\n");
#endif
                return -1;

            }
        }
        catch (Base::CADKernelError e) {
            e.ReportException();
            THROWM(Base::CADKernelError,"Unable to find intersection between offset curves.")
        }

#ifdef DEBUG
        for(auto inter:offsetintersectionpoints) {
                Base::Console().Log("offset int(%f,%f,0)\n",inter.first.x,inter.first.y);
        }
#endif
        
        int res = selectintersection(offsetintersectionpoints,filletcenterpoint,refPnt1, refPnt2);

        if(res != 0)
            return res;

#ifdef DEBUG
        Base::Console().Log("selected offset int(%f,%f,0)\n",filletcenterpoint.first.x,filletcenterpoint.first.y);
#endif        
        
        double refoparam1;
        double refoparam2;

        try {
            if(!curve1->closestParameter(filletcenterpoint.first,refoparam1))
                return -1;
        }
        catch (Base::CADKernelError e) {
            e.ReportException();
            THROWM(Base::CADKernelError,"Unable to determine the starting point of the arc.")
        }

        try {
            if(!curve2->closestParameter(filletcenterpoint.second,refoparam2))
                return -1;
        }
        catch (Base::CADKernelError e) {
            e.ReportException();
            THROWM(Base::CADKernelError,"Unable to determine the end point of the arc.")
        }

        // Next we calculate the closest points to the fillet center, so the points where tangency is to be applied
        Base::Vector3d refp1 = curve1->pointAtParameter(refoparam1);
        Base::Vector3d refp2 = curve2->pointAtParameter(refoparam2);

#ifdef DEBUG
        Base::Console().Log("refpoints: (%f,%f,%f);(%f,%f,%f)",refp1.x,refp1.y,refp1.z,refp2.x,refp2.y,refp2.z);
#endif
        // Now we create arc for the fillet
        double startAngle, endAngle, range;

        Base::Vector3d radDir1 = refp1 - filletcenterpoint.first;
        Base::Vector3d radDir2 = refp2 - filletcenterpoint.first;

        startAngle = atan2(radDir1.y, radDir1.x);

        range = atan2(-radDir1.y*radDir2.x+radDir1.x*radDir2.y,
                    radDir1.x*radDir2.x+radDir1.y*radDir2.y);

        endAngle = startAngle + range;

        if (endAngle < startAngle)
            std::swap(startAngle, endAngle);

        if (endAngle > 2*M_PI )
            endAngle -= 2*M_PI;

        if (startAngle < 0 )
            endAngle += 2*M_PI;

        // Create Arc Segment
        Part::GeomArcOfCircle *arc = new Part::GeomArcOfCircle();
        arc->setRadius(radDir1.Length());
        arc->setCenter(filletcenterpoint.first);
        arc->setRange(startAngle, endAngle, /*emulateCCWXY=*/true);

        // add arc to sketch geometry
        int filletId;
        Part::Geometry *newgeo = arc;
        filletId = addGeometry(newgeo);
        if (filletId < 0) {
            delete arc;
            return -1;
        }

        if (trim) {
            auto selectend = [](double intparam, double refparam, double startparam) {
                if( (intparam>refparam && startparam >= refparam) ||
                    (intparam<refparam && startparam <= refparam) ) {
                        return start;
                }
                else {
                        return end;
                }
            };

            // Two cases:
            // a) there as a coincidence constraint
            // b) we used the basis curve intersection


            if( curve1PosId == Sketcher::none ) {
                curve1PosId = selectend(intparam1,refoparam1,spc1);
                curve2PosId = selectend(intparam2,refoparam2,spc2);
            }


            delConstraintOnPoint(GeoId1, curve1PosId, false);
            delConstraintOnPoint(GeoId2, curve2PosId, false);


            Sketcher::Constraint *tangent1 = new Sketcher::Constraint();
            Sketcher::Constraint *tangent2 = new Sketcher::Constraint();

            tangent1->Type = Sketcher::Tangent;
            tangent1->First = GeoId1;
            tangent1->FirstPos = curve1PosId;
            tangent1->Second = filletId;

            tangent2->Type = Sketcher::Tangent;
            tangent2->First = GeoId2;
            tangent2->FirstPos = curve2PosId;
            tangent2->Second = filletId;

            double dist1 = (refp1 - arc->getStartPoint(true)).Length();
            double dist2 = (refp1 - arc->getEndPoint(true)).Length();

            //Base::Console().Log("dists_refpoint_to_arc_sp_ep: (%f);(%f)",dist1,dist2);

            if (dist1 < dist2) {
                tangent1->SecondPos = start;
                tangent2->SecondPos = end;
                movePoint(GeoId1, curve1PosId, arc->getStartPoint(true),false,true);
                movePoint(GeoId2, curve2PosId, arc->getEndPoint(true),false,true);
            }
            else {
                tangent1->SecondPos = end;
                tangent2->SecondPos = start;
                movePoint(GeoId1, curve1PosId, arc->getEndPoint(true),false,true);
                movePoint(GeoId2, curve2PosId, arc->getStartPoint(true),false,true);
            }

            addConstraint(tangent1);
            addConstraint(tangent2);
            delete tangent1;
            delete tangent2;
        }
        delete arc;
        delete ocurve1;
        delete ocurve2;

#ifdef DEBUG
        Base::Console().Log("\n\nEND OF FILLET DEBUG\n\n");
#endif

        if(noRecomputes) // if we do not have a recompute after the geometry creation, the sketch must be solved to update the DoF of the solver
            solve();

        return 0;
    }
    return -1;
}

int SketchObject::extend(int GeoId, double increment, int endpoint) {
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const std::vector<Part::Geometry *> &geomList = getInternalGeometry();
    Part::Geometry *geom = geomList[GeoId];
    int retcode = -1;
    if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        Part::GeomLineSegment *seg = static_cast<Part::GeomLineSegment *>(geom);
        Base::Vector3d startVec = seg->getStartPoint();
        Base::Vector3d endVec = seg->getEndPoint();
        if (endpoint == start) {
            Base::Vector3d newPoint = startVec - endVec;
            double scaleFactor = newPoint.Length() + increment;
            newPoint.Normalize();
            newPoint.Scale(scaleFactor, scaleFactor, scaleFactor);
            newPoint = newPoint + endVec;
            retcode = movePoint(GeoId, Sketcher::start, newPoint, false, true);
        } else if (endpoint == end) {
            Base::Vector3d newPoint = endVec - startVec;
            double scaleFactor = newPoint.Length() + increment;
            newPoint.Normalize();
            newPoint.Scale(scaleFactor, scaleFactor, scaleFactor);
            newPoint = newPoint + startVec;
            retcode = movePoint(GeoId, Sketcher::end, newPoint, false, true);
        }
    } else if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        Part::GeomArcOfCircle *arc = static_cast<Part::GeomArcOfCircle *>(geom);
        double startArc, endArc;
        arc->getRange(startArc, endArc, true);
        if (endpoint == start) {
            arc->setRange(startArc - increment, endArc, true);
            retcode = 0;
        } else if (endpoint == end) {
            arc->setRange(startArc, endArc + increment, true);
            retcode = 0;
        }
    }
    if (retcode == 0 && noRecomputes) {
        solve();
    }
    return retcode;
}

int SketchObject::trim(int GeoId, const Base::Vector3d& point)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const std::vector<Part::Geometry *> &geomlist = getInternalGeometry();
    const std::vector<Constraint *> &constraints = this->Constraints.getValues();

    int GeoId1=Constraint::GeoUndef, GeoId2=Constraint::GeoUndef;
    Base::Vector3d point1, point2;
    Part2DObject::seekTrimPoints(geomlist, GeoId, point, GeoId1, point1, GeoId2, point2);
    if (GeoId1 < 0 && GeoId2 >= 0) {
        std::swap(GeoId1,GeoId2);
        std::swap(point1,point2);
    }
    
    auto handlemultipleintersection = [this] (Constraint * constr, int GeoId, PointPos pos, PointPos & secondPos) {
        
        Base::Vector3d cp = getPoint(constr->First,constr->FirstPos);
    
        Base::Vector3d ee = getPoint(GeoId,pos);
    
        if( (ee-cp).Length() < Precision::Confusion() ) {
            secondPos = constr->FirstPos;
        }
    };

    Part::Geometry *geo = geomlist[GeoId];
    if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment*>(geo);
        Base::Vector3d startPnt = lineSeg->getStartPoint();
        Base::Vector3d endPnt = lineSeg->getEndPoint();
        Base::Vector3d dir = (endPnt - startPnt).Normalize();
        double length = (endPnt - startPnt)*dir;
        double x0 = (point - startPnt)*dir;
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double x1 = (point1 - startPnt)*dir;
            double x2 = (point2 - startPnt)*dir;
            if (x1 > x2) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(x1,x2);
            }
            if (x1 >= 0.001*length && x2 <= 0.999*length) {
                if (x1 < x0 && x2 > x0) {
                    int newGeoId = addGeometry(geo);
                    // go through all constraints and replace the point (GeoId,end) with (newGeoId,end)
                    transferConstraints(GeoId, end, newGeoId, end);

                    movePoint(GeoId, end, point1,false,true);
                    movePoint(newGeoId, start, point2,false,true);

                    PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
                    ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;
                    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                         it != constraints.end(); ++it) {
                        Constraint *constr = *(it);
                        if (secondPos1 == Sketcher::none && (constr->First == GeoId1  && constr->Second == GeoId)) {
                            constrType1= Sketcher::Coincident;
                            secondPos1 = constr->FirstPos;
                        } else if (secondPos2 == Sketcher::none && (constr->First == GeoId2  && constr->Second == GeoId)) {
                            constrType2 = Sketcher::Coincident;
                            secondPos2 = constr->FirstPos;
                        }
                    }

                    // constrain the trimming points on the corresponding geometries
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType1;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType1 == Sketcher::Coincident) {
                        newConstr->SecondPos = secondPos1;
                        delConstraintOnPoint(GeoId1, secondPos1, false);
                    }

                    addConstraint(newConstr);

                    // Reset the second pos
                    newConstr->SecondPos = Sketcher::none;

                    newConstr->Type = constrType2;
                    newConstr->First = newGeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId2;

                    if (constrType2 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos2;
                      delConstraintOnPoint(GeoId2, secondPos2, false);
                    }

                    addConstraint(newConstr);

                    // Reset the second pos
                    newConstr->SecondPos = Sketcher::none;

                    // new line segments colinear
                    newConstr->Type = Sketcher::Tangent;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = none;
                    newConstr->Second = newGeoId;
                    addConstraint(newConstr);
                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();

                    return 0;
                }
            } else if (x1 < 0.001*length) { // drop the first intersection point
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
            } else if (x2 > 0.999*length) { // drop the second intersection point
            }
            else
              return -1;
        }

        if (GeoId1 >= 0) {
            double x1 = (point1 - startPnt)*dir;
            if (x1 >= 0.001*length && x1 <= 0.999*length) {

                ConstraintType constrType = Sketcher::PointOnObject;
                PointPos secondPos = Sketcher::none;
                for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                     it != constraints.end(); ++it) {
                    Constraint *constr = *(it);
                    if ((constr->First == GeoId1  && constr->Second == GeoId)) {
                        constrType = Sketcher::Coincident;
                        secondPos = constr->FirstPos;
                        delConstraintOnPoint(GeoId1, constr->FirstPos, false);
                        break;
                    }
                }

                if (x1 > x0) { // trim line start
                    delConstraintOnPoint(GeoId, start, false);
                    movePoint(GeoId, start, point1,false,true);

                    // constrain the trimming point on the corresponding geometry
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();

                    return 0;
                }
                else if (x1 < x0) { // trim line end
                    delConstraintOnPoint(GeoId, end, false);
                    movePoint(GeoId, end, point1,false,true);
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();

                    return 0;
                }
            }
        }
    } else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle *circle = static_cast<const Part::GeomCircle*>(geo);
        Base::Vector3d center = circle->getCenter();
        double theta0 = Base::fmod(atan2(point.y - center.y,point.x - center.x), 2.f*M_PI);
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double theta1 = Base::fmod(atan2(point1.y - center.y, point1.x - center.x), 2.f*M_PI);
            double theta2 = Base::fmod(atan2(point2.y - center.y, point2.x - center.x), 2.f*M_PI);
            if (Base::fmod(theta1 - theta0, 2.f*M_PI) > Base::fmod(theta2 - theta0, 2.f*M_PI)) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(theta1,theta2);
            }
            if (theta1 == theta0 || theta1 == theta2)
                return -1;
            else if (theta1 > theta2)
                theta2 += 2.f*M_PI;

            // Trim Point between intersection points

            // Create a new arc to substitute Circle in geometry list and set parameters
            Part::GeomArcOfCircle *geoNew = new Part::GeomArcOfCircle();
            geoNew->setCenter(center);
            geoNew->setRadius(circle->getRadius());
            geoNew->setRange(theta1, theta2,/*emulateCCW=*/true);

            std::vector< Part::Geometry * > newVals(geomlist);
            newVals[GeoId] = geoNew;
            Geometry.setValues(newVals);
            Constraints.acceptGeometry(getCompleteGeometry());
            delete geoNew;
            rebuildVertexIndex();

            PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
            ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;
            for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                 it != constraints.end(); ++it) {
                Constraint *constr = *(it);
                if (secondPos1 == Sketcher::none && (constr->First == GeoId1  && constr->Second == GeoId)) {
                    constrType1= Sketcher::Coincident;
                    handlemultipleintersection(constr, GeoId, start, secondPos1);
                } else if(secondPos2 == Sketcher::none && (constr->First == GeoId2  && constr->Second == GeoId)) {
                    constrType2 = Sketcher::Coincident;
                    handlemultipleintersection(constr, GeoId, end, secondPos2);
                }
            }
            
            if( (constrType1 == Sketcher::Coincident && secondPos1 == Sketcher::none) || 
                (constrType2 == Sketcher::Coincident && secondPos2 == Sketcher::none))
                THROWM(ValueError,"Invalid position Sketcher::none when creating a Coincident constraint")

            // constrain the trimming points on the corresponding geometries
            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = constrType1;
            newConstr->First = GeoId;
            newConstr->FirstPos = start;
            newConstr->Second = GeoId1;

            if (constrType1 == Sketcher::Coincident) {
                newConstr->SecondPos = secondPos1;
                delConstraintOnPoint(GeoId1, secondPos1, false);
            }

            addConstraint(newConstr);

            // Reset secondpos in case it was set previously
            newConstr->SecondPos = Sketcher::none;

            // Add Second Constraint
            newConstr->First = GeoId;
            newConstr->FirstPos = end;
            newConstr->Second = GeoId2;

            if (constrType2 == Sketcher::Coincident) {
                newConstr->SecondPos = secondPos2;
                delConstraintOnPoint(GeoId2, secondPos2, false);
            }

            addConstraint(newConstr);

            delete newConstr;

            if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                solve();

            return 0;
        }
    } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse*>(geo);
        Base::Vector3d center = ellipse->getCenter();
        double theta0;
        ellipse->closestParameter(point,theta0);
        theta0 = Base::fmod(theta0, 2.f*M_PI);
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double theta1;
            ellipse->closestParameter(point1,theta1);
            theta1 = Base::fmod(theta1, 2.f*M_PI);
            double theta2;
            ellipse->closestParameter(point2,theta2);
            theta2 = Base::fmod(theta2, 2.f*M_PI);
            if (Base::fmod(theta1 - theta0, 2.f*M_PI) > Base::fmod(theta2 - theta0, 2.f*M_PI)) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(theta1,theta2);
            }
            if (theta1 == theta0 || theta1 == theta2)
                return -1;
            else if (theta1 > theta2)
                theta2 += 2.f*M_PI;

            // Trim Point between intersection points

            // Create a new arc to substitute Circle in geometry list and set parameters
            Part::GeomArcOfEllipse *geoNew = new Part::GeomArcOfEllipse();
            geoNew->setCenter(center);
            geoNew->setMajorRadius(ellipse->getMajorRadius());
            geoNew->setMinorRadius(ellipse->getMinorRadius());
            geoNew->setMajorAxisDir(ellipse->getMajorAxisDir());
            geoNew->setRange(theta1, theta2, /*emulateCCW=*/true);

            std::vector< Part::Geometry * > newVals(geomlist);
            newVals[GeoId] = geoNew;
            Geometry.setValues(newVals);
            Constraints.acceptGeometry(getCompleteGeometry());
            delete geoNew;
            rebuildVertexIndex();

            
            auto handleinternalalignment = [this] (Constraint * constr, int GeoId, PointPos & secondPos) {
                if( constr->Type == Sketcher::InternalAlignment && 
                    ( constr->AlignmentType == Sketcher::EllipseMajorDiameter ||   
                        constr->AlignmentType == Sketcher::EllipseMinorDiameter ) ) {
                    
                    Base::Vector3d sp = getPoint(constr->First,start);
                    Base::Vector3d ep = getPoint(constr->First,end);
                
                    Base::Vector3d ee = getPoint(GeoId,start);
                
                    if( (ee-sp).Length() < (ee-ep).Length() ) {
                        secondPos = Sketcher::start;
                    }
                    else {
                        secondPos = Sketcher::end;
                    }
                }
            };            
            
            PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
            ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;
            for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                 it != constraints.end(); ++it) {
                Constraint *constr = *(it);
                if (secondPos1 == Sketcher::none && (constr->First == GeoId1  && constr->Second == GeoId)) {
                    constrType1= Sketcher::Coincident;
                    if(constr->FirstPos == Sketcher::none){
                        handleinternalalignment(constr, GeoId, secondPos1);
                    }
                    else {
                        handlemultipleintersection(constr, GeoId, start, secondPos1);
                    }
                    
                } else if(secondPos2 == Sketcher::none && (constr->First == GeoId2  && constr->Second == GeoId)) {
                    constrType2 = Sketcher::Coincident;
                    
                    if(constr->FirstPos == Sketcher::none){
                        handleinternalalignment(constr, GeoId, secondPos2);
                    }
                    else {
                        handlemultipleintersection(constr, GeoId, end, secondPos2);
                    }
                }
            }
            
            if( (constrType1 == Sketcher::Coincident && secondPos1 == Sketcher::none) || 
                (constrType2 == Sketcher::Coincident && secondPos2 == Sketcher::none))
                THROWM(ValueError,"Invalid position Sketcher::none when creating a Coincident constraint")

            // constrain the trimming points on the corresponding geometries
            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = constrType1;
            newConstr->First = GeoId;
            newConstr->FirstPos = start;
            newConstr->Second = GeoId1;

            if (constrType1 == Sketcher::Coincident) {
                newConstr->SecondPos = secondPos1;
                delConstraintOnPoint(GeoId1, secondPos1, false);
            }

            addConstraint(newConstr);

            // Reset secondpos in case it was set previously
            newConstr->SecondPos = Sketcher::none;

            // Add Second Constraint
            newConstr->First = GeoId;
            newConstr->FirstPos = end;
            newConstr->Second = GeoId2;

            if (constrType2 == Sketcher::Coincident) {
                newConstr->SecondPos = secondPos2;
                delConstraintOnPoint(GeoId2, secondPos2, false);
            }

            addConstraint(newConstr);

            delete newConstr;

            if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                solve();

            return 0;
        }
    } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle *aoc = static_cast<const Part::GeomArcOfCircle*>(geo);
        Base::Vector3d center = aoc->getCenter();
        double startAngle, endAngle;
        aoc->getRange(startAngle, endAngle, /*emulateCCW=*/true);
        double dir = (startAngle < endAngle) ? 1 : -1; // this is always == 1
        double arcLength = (endAngle - startAngle)*dir;
        double theta0 = Base::fmod(atan2(point.y - center.y, point.x - center.x) - startAngle, 2.f*M_PI); // x0
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double theta1 = Base::fmod(atan2(point1.y - center.y, point1.x - center.x) - startAngle, 2.f*M_PI) * dir; // x1
            double theta2 = Base::fmod(atan2(point2.y - center.y, point2.x - center.x) - startAngle, 2.f*M_PI) * dir; // x2
            if (theta1 > theta2) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(theta1,theta2);
            }
            if (theta1 >= 0.001*arcLength && theta2 <= 0.999*arcLength) {
                // Trim Point between intersection points
                if (theta1 < theta0 && theta2 > theta0) {
                    int newGeoId = addGeometry(geo);
                    // go through all constraints and replace the point (GeoId,end) with (newGeoId,end)
                    transferConstraints(GeoId, end, newGeoId, end);

                    Part::GeomArcOfCircle *aoc1 = static_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    Part::GeomArcOfCircle *aoc2 = static_cast<Part::GeomArcOfCircle*>(geomlist[newGeoId]);
                    aoc1->setRange(startAngle, startAngle + theta1, /*emulateCCW=*/true);
                    aoc2->setRange(startAngle + theta2, endAngle, /*emulateCCW=*/true);

                    // constrain the trimming points on the corresponding geometries
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();

                    // Build Constraints associated with new pair of arcs
                    newConstr->Type = Sketcher::Equal;
                    newConstr->First = GeoId;
                    newConstr->Second = newGeoId;
                    addConstraint(newConstr);

                    PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
                    ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;

                    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                         it != constraints.end(); ++it) {
                        Constraint *constr = *(it);
                        if (secondPos1 == Sketcher::none &&
                            (constr->First == GeoId1  && constr->Second == GeoId)) {
                            constrType1= Sketcher::Coincident;
                            secondPos1 = constr->FirstPos;
                        } else if (secondPos2 == Sketcher::none &&
                                   (constr->First == GeoId2  && constr->Second == GeoId)) {
                            constrType2 = Sketcher::Coincident;
                            secondPos2 = constr->FirstPos;
                        }
                    }

                    newConstr->Type = constrType1;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType1 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos1;
                      delConstraintOnPoint(GeoId1, secondPos1, false);
                    }

                    addConstraint(newConstr);

                    // Reset secondpos in case it was set previously
                    newConstr->SecondPos = Sketcher::none;

                    newConstr->Type = constrType2;
                    newConstr->First = newGeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId2;

                    if (constrType2 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos2;
                      delConstraintOnPoint(GeoId2, secondPos2, false);
                    }

                    addConstraint(newConstr);

                    newConstr->Type = Sketcher::Coincident;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = Sketcher::mid;
                    newConstr->Second = newGeoId;
                    newConstr->SecondPos = Sketcher::mid;
                    addConstraint(newConstr);

                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();

                    return 0;
                } else
                    return -1;
            } else if (theta1 < 0.001*arcLength) { // drop the second intersection point
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
            } else if (theta2 > 0.999*arcLength) {
            }
            else
                return -1;
        }

        if (GeoId1 >= 0) {

            ConstraintType constrType = Sketcher::PointOnObject;
            PointPos secondPos = Sketcher::none;
            for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                 it != constraints.end(); ++it) {
                Constraint *constr = *(it);
                if ((constr->First == GeoId1  && constr->Second == GeoId)) {
                    constrType = Sketcher::Coincident;
                    secondPos = constr->FirstPos;
                    delConstraintOnPoint(GeoId1, constr->FirstPos, false);
                    break;
                }
            }

            double theta1 = Base::fmod(atan2(point1.y - center.y, point1.x - center.x) - startAngle, 2.f*M_PI) * dir; // x1
            if (theta1 >= 0.001*arcLength && theta1 <= 0.999*arcLength) {
                if (theta1 > theta0) { // trim arc start
                    delConstraintOnPoint(GeoId, start, false);
                    Part::GeomArcOfCircle *aoc1 = static_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    aoc1->setRange(startAngle + theta1, endAngle, /*emulateCCW=*/true);
                    // constrain the trimming point on the corresponding geometry
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();

                    return 0;
                }
                else { // trim arc end
                    delConstraintOnPoint(GeoId, end, false);
                    Part::GeomArcOfCircle *aoc1 = static_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    aoc1->setRange(startAngle, startAngle + theta1, /*emulateCCW=*/true);
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();

                    return 0;
                }
            }
        }
    } else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse*>(geo);
        Base::Vector3d center = aoe->getCenter();
        double startAngle, endAngle;
        aoe->getRange(startAngle, endAngle,/*emulateCCW=*/true);
        double dir = (startAngle < endAngle) ? 1 : -1; // this is always == 1
        double arcLength = (endAngle - startAngle)*dir;
        double theta0 = Base::fmod(
                atan2(-aoe->getMajorRadius()*((point.x-center.x)*aoe->getMajorAxisDir().y-(point.y-center.y)*aoe->getMajorAxisDir().x),
                            aoe->getMinorRadius()*((point.x-center.x)*aoe->getMajorAxisDir().x+(point.y-center.y)*aoe->getMajorAxisDir().y)
                )- startAngle, 2.f*M_PI); // x0
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double theta1 = Base::fmod(
                atan2(-aoe->getMajorRadius()*((point1.x-center.x)*aoe->getMajorAxisDir().y-(point1.y-center.y)*aoe->getMajorAxisDir().x),
                            aoe->getMinorRadius()*((point1.x-center.x)*aoe->getMajorAxisDir().x+(point1.y-center.y)*aoe->getMajorAxisDir().y)
                )- startAngle, 2.f*M_PI) * dir; // x1
            double theta2 = Base::fmod(
                atan2(-aoe->getMajorRadius()*((point2.x-center.x)*aoe->getMajorAxisDir().y-(point2.y-center.y)*aoe->getMajorAxisDir().x),
                            aoe->getMinorRadius()*((point2.x-center.x)*aoe->getMajorAxisDir().x+(point2.y-center.y)*aoe->getMajorAxisDir().y)
                )- startAngle, 2.f*M_PI) * dir; // x2

            if (theta1 > theta2) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(theta1,theta2);
            }
            if (theta1 >= 0.001*arcLength && theta2 <= 0.999*arcLength) {
                // Trim Point between intersection points
                if (theta1 < theta0 && theta2 > theta0) {
                    int newGeoId = addGeometry(geo);
                    // go through all constraints and replace the point (GeoId,end) with (newGeoId,end)
                    transferConstraints(GeoId, end, newGeoId, end);

                    Part::GeomArcOfEllipse *aoe1 = static_cast<Part::GeomArcOfEllipse*>(geomlist[GeoId]);
                    Part::GeomArcOfEllipse *aoe2 = static_cast<Part::GeomArcOfEllipse*>(geomlist[newGeoId]);
                    aoe1->setRange(startAngle, startAngle + theta1, /*emulateCCW=*/true);
                    aoe2->setRange(startAngle + theta2, endAngle, /*emulateCCW=*/true);

                    // constrain the trimming points on the corresponding geometries
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();

                    // Build Constraints associated with new pair of arcs
                    newConstr->Type = Sketcher::Equal;
                    newConstr->First = GeoId;
                    newConstr->Second = newGeoId;
                    addConstraint(newConstr);

                    PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
                    ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;

                    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                         it != constraints.end(); ++it) {
                        Constraint *constr = *(it);
                        if (secondPos1 == Sketcher::none &&
                            (constr->First == GeoId1  && constr->Second == GeoId)) {
                            constrType1= Sketcher::Coincident;
                            secondPos1 = constr->FirstPos;
                        } else if (secondPos2 == Sketcher::none &&
                                   (constr->First == GeoId2  && constr->Second == GeoId)) {
                            constrType2 = Sketcher::Coincident;
                            secondPos2 = constr->FirstPos;
                        }
                    }

                    newConstr->Type = constrType1;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType1 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos1;
                      delConstraintOnPoint(GeoId1, secondPos1, false);
                    }

                    addConstraint(newConstr);

                    // Reset secondpos in case it was set previously
                    newConstr->SecondPos = Sketcher::none;

                    newConstr->Type = constrType2;
                    newConstr->First = newGeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId2;

                    if (constrType2 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos2;
                      delConstraintOnPoint(GeoId2, secondPos2, false);
                    }

                    addConstraint(newConstr);

                    newConstr->Type = Sketcher::Coincident;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = Sketcher::mid;
                    newConstr->Second = newGeoId;
                    newConstr->SecondPos = Sketcher::mid;
                    addConstraint(newConstr);

                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();

                    return 0;
                } else
                    return -1;
            } else if (theta1 < 0.001*arcLength) { // drop the second intersection point
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
            } else if (theta2 > 0.999*arcLength) {
            } else
                return -1;
        }

        if (GeoId1 >= 0) {

            ConstraintType constrType = Sketcher::PointOnObject;
            PointPos secondPos = Sketcher::none;
            for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                 it != constraints.end(); ++it) {
                Constraint *constr = *(it);
                if ((constr->First == GeoId1  && constr->Second == GeoId)) {
                    constrType = Sketcher::Coincident;
                    secondPos = constr->FirstPos;
                    delConstraintOnPoint(GeoId1, constr->FirstPos, false);
                    break;
                }
            }

            double theta1 = Base::fmod(
                        atan2(-aoe->getMajorRadius()*((point1.x-center.x)*aoe->getMajorAxisDir().y-(point1.y-center.y)*aoe->getMajorAxisDir().x),
                              aoe->getMinorRadius()*((point1.x-center.x)*aoe->getMajorAxisDir().x+(point1.y-center.y)*aoe->getMajorAxisDir().y)
                             )- startAngle, 2.f*M_PI) * dir; // x1

            if (theta1 >= 0.001*arcLength && theta1 <= 0.999*arcLength) {
                if (theta1 > theta0) { // trim arc start
                    delConstraintOnPoint(GeoId, start, false);
                    Part::GeomArcOfEllipse *aoe1 = static_cast<Part::GeomArcOfEllipse*>(geomlist[GeoId]);
                    aoe1->setRange(startAngle + theta1, endAngle, /*emulateCCW=*/true);
                    // constrain the trimming point on the corresponding geometry
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();

                    return 0;
                }
                else { // trim arc end
                    delConstraintOnPoint(GeoId, end, false);
                    Part::GeomArcOfEllipse *aoe1 = static_cast<Part::GeomArcOfEllipse*>(geomlist[GeoId]);
                    aoe1->setRange(startAngle, startAngle + theta1, /*emulateCCW=*/true);

                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;

                    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
                        solve();
                    return 0;
                }
            }
        }

    } else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
        const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);
        Base::Vector3d center = aoh->getCenter();
        double startAngle, endAngle;
        aoh->getRange(startAngle, endAngle, /*emulateCCW=*/true);
        double dir = (startAngle < endAngle) ? 1 : -1; // this is always == 1
        double arcLength = (endAngle - startAngle)*dir;
        double theta0 = Base::fmod(
                atan2(-aoh->getMajorRadius()*((point.x-center.x)*sin(aoh->getAngleXU())-(point.y-center.y)*cos(aoh->getAngleXU())),
                            aoh->getMinorRadius()*((point.x-center.x)*cos(aoh->getAngleXU())+(point.y-center.y)*sin(aoh->getAngleXU()))
                )- startAngle, 2.f*M_PI); // x0
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double theta1 = Base::fmod(
                atan2(-aoh->getMajorRadius()*((point1.x-center.x)*sin(aoh->getAngleXU())-(point1.y-center.y)*cos(aoh->getAngleXU())),
                            aoh->getMinorRadius()*((point1.x-center.x)*cos(aoh->getAngleXU())+(point1.y-center.y)*sin(aoh->getAngleXU()))
                )- startAngle, 2.f*M_PI) * dir; // x1
            double theta2 = Base::fmod(
                atan2(-aoh->getMajorRadius()*((point2.x-center.x)*sin(aoh->getAngleXU())-(point2.y-center.y)*cos(aoh->getAngleXU())),
                            aoh->getMinorRadius()*((point2.x-center.x)*cos(aoh->getAngleXU())+(point2.y-center.y)*sin(aoh->getAngleXU()))
                )- startAngle, 2.f*M_PI) * dir; // x2

            if (theta1 > theta2) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(theta1,theta2);
            }
            if (theta1 >= 0.001*arcLength && theta2 <= 0.999*arcLength) {
                // Trim Point between intersection points
                if (theta1 < theta0 && theta2 > theta0) {
                    int newGeoId = addGeometry(geo);
                    // go through all constraints and replace the point (GeoId,end) with (newGeoId,end)
                    transferConstraints(GeoId, end, newGeoId, end);

                    Part::GeomArcOfHyperbola *aoh1 = static_cast<Part::GeomArcOfHyperbola*>(geomlist[GeoId]);
                    Part::GeomArcOfHyperbola *aoh2 = static_cast<Part::GeomArcOfHyperbola*>(geomlist[newGeoId]);
                    aoh1->setRange(startAngle, startAngle + theta1, /*emulateCCW=*/true);
                    aoh2->setRange(startAngle + theta2, endAngle, /*emulateCCW=*/true);

                    // constrain the trimming points on the corresponding geometries
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();

                    // Build Constraints associated with new pair of arcs
                    newConstr->Type = Sketcher::Equal;
                    newConstr->First = GeoId;
                    newConstr->Second = newGeoId;
                    addConstraint(newConstr);

                    PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
                    ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;

                    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                         it != constraints.end(); ++it) {
                        Constraint *constr = *(it);
                        if (secondPos1 == Sketcher::none &&
                            (constr->First == GeoId1  && constr->Second == GeoId)) {
                            constrType1= Sketcher::Coincident;
                            secondPos1 = constr->FirstPos;
                        } else if (secondPos2 == Sketcher::none &&
                                   (constr->First == GeoId2  && constr->Second == GeoId)) {
                            constrType2 = Sketcher::Coincident;
                            secondPos2 = constr->FirstPos;
                        }
                    }

                    newConstr->Type = constrType1;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType1 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos1;
                      delConstraintOnPoint(GeoId1, secondPos1, false);
                    }

                    addConstraint(newConstr);

                    // Reset secondpos in case it was set previously
                    newConstr->SecondPos = Sketcher::none;

                    newConstr->Type = constrType2;
                    newConstr->First = newGeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId2;

                    if (constrType2 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos2;
                      delConstraintOnPoint(GeoId2, secondPos2, false);
                    }

                    addConstraint(newConstr);

                    newConstr->Type = Sketcher::Coincident;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = Sketcher::mid;
                    newConstr->Second = newGeoId;
                    newConstr->SecondPos = Sketcher::mid;
                    addConstraint(newConstr);

                    delete newConstr;

                    return 0;
                } else
                    return -1;
            } else if (theta1 < 0.001*arcLength) { // drop the second intersection point
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
            } else if (theta2 > 0.999*arcLength) {
            } else
                return -1;
        }

        if (GeoId1 >= 0) {

            ConstraintType constrType = Sketcher::PointOnObject;
            PointPos secondPos = Sketcher::none;
            for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                 it != constraints.end(); ++it) {
                Constraint *constr = *(it);
                if ((constr->First == GeoId1  && constr->Second == GeoId)) {
                    constrType = Sketcher::Coincident;
                    secondPos = constr->FirstPos;
                    delConstraintOnPoint(GeoId1, constr->FirstPos, false);
                    break;
                }
            }

            double theta1 = Base::fmod(
                        atan2(-aoh->getMajorRadius()*((point1.x-center.x)*sin(aoh->getAngleXU())-(point1.y-center.y)*cos(aoh->getAngleXU())),
                              aoh->getMinorRadius()*((point1.x-center.x)*cos(aoh->getAngleXU())+(point1.y-center.y)*sin(aoh->getAngleXU()))
                             )- startAngle, 2.f*M_PI) * dir; // x1

            if (theta1 >= 0.001*arcLength && theta1 <= 0.999*arcLength) {
                if (theta1 > theta0) { // trim arc start
                    delConstraintOnPoint(GeoId, start, false);
                    Part::GeomArcOfHyperbola *aoe1 = static_cast<Part::GeomArcOfHyperbola*>(geomlist[GeoId]);
                    aoe1->setRange(startAngle + theta1, endAngle, /*emulateCCW=*/true);
                    // constrain the trimming point on the corresponding geometry
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;
                    return 0;
                }
                else { // trim arc end
                    delConstraintOnPoint(GeoId, end, false);
                    Part::GeomArcOfHyperbola *aoe1 = static_cast<Part::GeomArcOfHyperbola*>(geomlist[GeoId]);
                    aoe1->setRange(startAngle, startAngle + theta1, /*emulateCCW=*/true);
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;
                    return 0;
                }
            }
        }
    }

    return -1;
}

bool SketchObject::isExternalAllowed(App::Document *pDoc, App::DocumentObject *pObj, eReasonList* rsn) const
{
    if (rsn)
        *rsn = rlAllowed;

    // Externals outside of the Document are NOT allowed
    if (this->getDocument() != pDoc){
        if (rsn)
            *rsn = rlOtherDoc;
        return false;
    }

    //circular reference prevention
    try {
        if (!(this->testIfLinkDAGCompatible(pObj))){
            if (rsn)
                *rsn = rlCircularReference;
            return false;
        }
    } catch (Base::Exception &e) {
        Base::Console().Warning("Probably, there is a circular reference in the document. Error: %s\n", e.what());
        return true; //prohibiting this reference won't remove the problem anyway...
    }


    // Note: Checking for the body of the support doesn't work when the support are the three base planes
    //App::DocumentObject *support = this->Support.getValue();
    Part::BodyBase* body_this = Part::BodyBase::findBodyOf(this);
    Part::BodyBase* body_obj = Part::BodyBase::findBodyOf(pObj);
    App::Part* part_this = App::Part::getPartOfObject(this);
    App::Part* part_obj = App::Part::getPartOfObject(pObj);
    if (part_this == part_obj){ //either in the same part, or in the root of document
        if (body_this == NULL) {
            return true;
        } else if (body_this == body_obj) {
            return true;
        } else {
            if (rsn)
                *rsn = rlOtherBody;
            return false;
        }
    } else {
        // cross-part link. Disallow, should be done via shapebinders only
        if (rsn)
            *rsn = rlOtherPart;
        return false;
    }

    assert(0);
    return true;
}

bool SketchObject::isCarbonCopyAllowed(App::Document *pDoc, App::DocumentObject *pObj, bool & xinv, bool & yinv, eReasonList* rsn) const
{
    if (rsn)
        *rsn = rlAllowed;

    // Only applicable to sketches
    if (pObj->getTypeId() != Sketcher::SketchObject::getClassTypeId()) {
        if (rsn)
            *rsn = rlNotASketch;
        return false;
    }

    SketchObject * psObj = static_cast<SketchObject *>(pObj);

    // Sketches outside of the Document are NOT allowed
    if (this->getDocument() != pDoc){
        if (rsn)
            *rsn = rlOtherDoc;
        return false;
    }

    //circular reference prevention
    try {
        if (!(this->testIfLinkDAGCompatible(pObj))){
            if (rsn)
                *rsn = rlCircularReference;
            return false;
        }
    } catch (Base::Exception &e) {
        Base::Console().Warning("Probably, there is a circular reference in the document. Error: %s\n", e.what());
        return true; //prohibiting this reference won't remove the problem anyway...
    }


    // Note: Checking for the body of the support doesn't work when the support are the three base planes
    //App::DocumentObject *support = this->Support.getValue();
    Part::BodyBase* body_this = Part::BodyBase::findBodyOf(this);
    Part::BodyBase* body_obj = Part::BodyBase::findBodyOf(pObj);
    App::Part* part_this = App::Part::getPartOfObject(this);
    App::Part* part_obj = App::Part::getPartOfObject(pObj);
    if (part_this == part_obj){ //either in the same part, or in the root of document
        if (body_this != NULL) {
            if (body_this != body_obj) {
                if (!this->allowOtherBody) {
                    if (rsn)
                        *rsn = rlOtherBody;
                    return false;
                }
                else if (psObj->getExternalGeometryCount()>2){ // if the original sketch has external geometry AND it is not in this body prevent link
                    if (rsn)
                        *rsn = rlOtherBodyWithLinks;
                    return false;
                }
            }
        }
    } else {
        // cross-part relation. Disallow, should be done via shapebinders only
        if (rsn)
            *rsn = rlOtherPart;
        return false;
    }



    const Rotation & srot = psObj->Placement.getValue().getRotation();
    const Rotation & lrot = this->Placement.getValue().getRotation();

    Base::Vector3d snormal(0,0,1);
    Base::Vector3d sx(1,0,0);
    Base::Vector3d sy(0,1,0);
    srot.multVec(snormal, snormal);
    srot.multVec(sx, sx);
    srot.multVec(sy, sy);

    Base::Vector3d lnormal(0,0,1);
    Base::Vector3d lx(1,0,0);
    Base::Vector3d ly(0,1,0);
    lrot.multVec(lnormal, lnormal);
    lrot.multVec(lx, lx);
    lrot.multVec(ly, ly);

    double dot = snormal*lnormal;
    double dotx = sx * lx;
    double doty = sy * ly;

    // the planes of the sketches must be parallel
    if(!allowUnaligned && dot != 1.0 && dot != -1.0) {
        if (rsn)
            *rsn = rlNonParallel;
        return false;
    }

    // the axis must be aligned
    if(!allowUnaligned && ((dotx != 1.0 && dotx != -1.0) || (doty != 1.0 && doty != -1.0))) {
        if (rsn)
            *rsn = rlAxesMisaligned;
        return false;
    }


    // the origins of the sketches must be aligned or be the same
    Base::Vector3d ddir = (psObj->Placement.getValue().getPosition() - this->Placement.getValue().getPosition()).Normalize();

    double alignment = ddir * lnormal;

    if(!allowUnaligned && (alignment != 1.0 && alignment != -1.0) && (psObj->Placement.getValue().getPosition() != this->Placement.getValue().getPosition()) ){
        if (rsn)
            *rsn = rlOriginsMisaligned;
        return false;
    }

    xinv = allowUnaligned?false:(dotx != 1.0);
    yinv = allowUnaligned?false:(doty != 1.0);

    return true;
}

int SketchObject::addSymmetric(const std::vector<int> &geoIdList, int refGeoId, Sketcher::PointPos refPosId/*=Sketcher::none*/)
{
    const std::vector< Part::Geometry * > &geovals = getInternalGeometry();
    std::vector< Part::Geometry * > newgeoVals(geovals);

    const std::vector< Constraint * > &constrvals = this->Constraints.getValues();
    std::vector< Constraint * > newconstrVals(constrvals);

    int cgeoid = getHighestCurveIndex()+1;

    std::map<int, int> geoIdMap;
    std::map<int, bool> isStartEndInverted;

    // reference is a line
    if(refPosId == Sketcher::none) {
        const Part::Geometry *georef = getGeometry(refGeoId);
        if(georef->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
            Base::Console().Error("Reference for symmetric is neither a point nor a line.\n");
            return -1;
        }

        const Part::GeomLineSegment *refGeoLine = static_cast<const Part::GeomLineSegment *>(georef);
        //line
        Base::Vector3d refstart = refGeoLine->getStartPoint();
        Base::Vector3d vectline = refGeoLine->getEndPoint()-refstart;

        for (std::vector<int>::const_iterator it = geoIdList.begin(); it != geoIdList.end(); ++it) {
            const Part::Geometry *geo = getGeometry(*it);
            Part::Geometry *geosym = geo->copy();
            GEN_ID(geosym);

            // Handle Geometry
            if(geosym->getTypeId() == Part::GeomLineSegment::getClassTypeId()){
                Part::GeomLineSegment *geosymline = static_cast<Part::GeomLineSegment *>(geosym);
                Base::Vector3d sp = geosymline->getStartPoint();
                Base::Vector3d ep = geosymline->getEndPoint();

                geosymline->setPoints(sp+2.0*(sp.Perpendicular(refGeoLine->getStartPoint(),vectline)-sp),
                        ep+2.0*(ep.Perpendicular(refGeoLine->getStartPoint(),vectline)-ep));
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomCircle::getClassTypeId()){
                Part::GeomCircle *geosymcircle = static_cast<Part::GeomCircle *>(geosym);
                Base::Vector3d cp = geosymcircle->getCenter();

                geosymcircle->setCenter(cp+2.0*(cp.Perpendicular(refGeoLine->getStartPoint(),vectline)-cp));
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()){
                Part::GeomArcOfCircle *geoaoc = static_cast<Part::GeomArcOfCircle *>(geosym);
                Base::Vector3d sp = geoaoc->getStartPoint(true);
                Base::Vector3d ep = geoaoc->getEndPoint(true);
                Base::Vector3d cp = geoaoc->getCenter();

                Base::Vector3d ssp = sp+2.0*(sp.Perpendicular(refGeoLine->getStartPoint(),vectline)-sp);
                Base::Vector3d sep = ep+2.0*(ep.Perpendicular(refGeoLine->getStartPoint(),vectline)-ep);
                Base::Vector3d scp = cp+2.0*(cp.Perpendicular(refGeoLine->getStartPoint(),vectline)-cp);

                double theta1 = Base::fmod(atan2(sep.y - scp.y, sep.x - scp.x), 2.f*M_PI);
                double theta2 = Base::fmod(atan2(ssp.y - scp.y, ssp.x - scp.x), 2.f*M_PI);

                geoaoc->setCenter(scp);
                geoaoc->setRange(theta1,theta2,true);
                isStartEndInverted.insert(std::make_pair(*it, true));
            }
            else if(geosym->getTypeId() == Part::GeomEllipse::getClassTypeId()){
                Part::GeomEllipse *geosymellipse = static_cast<Part::GeomEllipse *>(geosym);
                Base::Vector3d cp = geosymellipse->getCenter();

                Base::Vector3d majdir = geosymellipse->getMajorAxisDir();
                double majord=geosymellipse->getMajorRadius();
                double minord=geosymellipse->getMinorRadius();
                double df= sqrt(majord*majord-minord*minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1+2.0*(f1.Perpendicular(refGeoLine->getStartPoint(),vectline)-f1);
                Base::Vector3d scp = cp+2.0*(cp.Perpendicular(refGeoLine->getStartPoint(),vectline)-cp);

                geosymellipse->setMajorAxisDir(sf1-scp);

                geosymellipse->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()){
                Part::GeomArcOfEllipse *geosymaoe = static_cast<Part::GeomArcOfEllipse *>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord=geosymaoe->getMajorRadius();
                double minord=geosymaoe->getMinorRadius();
                double df= sqrt(majord*majord-minord*minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1+2.0*(f1.Perpendicular(refGeoLine->getStartPoint(),vectline)-f1);
                Base::Vector3d scp = cp+2.0*(cp.Perpendicular(refGeoLine->getStartPoint(),vectline)-cp);

                geosymaoe->setMajorAxisDir(sf1-scp);

                geosymaoe->setCenter(scp);

                double theta1,theta2;
                geosymaoe->getRange(theta1,theta2,true);
                theta1 = 2.0*M_PI - theta1;
                theta2 = 2.0*M_PI - theta2;
                std::swap(theta1, theta2);
                if (theta1 < 0) {
                    theta1 += 2.0*M_PI;
                    theta2 += 2.0*M_PI;
                }

                geosymaoe->setRange(theta1,theta2,true);
                isStartEndInverted.insert(std::make_pair(*it, true));
            }
            else if(geosym->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()){
                Part::GeomArcOfHyperbola *geosymaoe = static_cast<Part::GeomArcOfHyperbola *>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord=geosymaoe->getMajorRadius();
                double minord=geosymaoe->getMinorRadius();
                double df= sqrt(majord*majord+minord*minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1+2.0*(f1.Perpendicular(refGeoLine->getStartPoint(),vectline)-f1);
                Base::Vector3d scp = cp+2.0*(cp.Perpendicular(refGeoLine->getStartPoint(),vectline)-cp);

                geosymaoe->setMajorAxisDir(sf1-scp);

                geosymaoe->setCenter(scp);

                double theta1,theta2;
                geosymaoe->getRange(theta1,theta2,true);
                theta1 = -theta1;
                theta2 = -theta2;
                std::swap(theta1, theta2);

                geosymaoe->setRange(theta1,theta2,true);
                isStartEndInverted.insert(std::make_pair(*it, true));
            }
            else if(geosym->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()){
                Part::GeomArcOfParabola *geosymaoe = static_cast<Part::GeomArcOfParabola *>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                //double df= geosymaoe->getFocal();
                Base::Vector3d f1 = geosymaoe->getFocus();

                Base::Vector3d sf1 = f1+2.0*(f1.Perpendicular(refGeoLine->getStartPoint(),vectline)-f1);
                Base::Vector3d scp = cp+2.0*(cp.Perpendicular(refGeoLine->getStartPoint(),vectline)-cp);

                geosymaoe->setXAxisDir(sf1-scp);
                geosymaoe->setCenter(scp);

                double theta1,theta2;
                geosymaoe->getRange(theta1,theta2,true);
                theta1 = -theta1;
                theta2 = -theta2;
                std::swap(theta1, theta2);

                geosymaoe->setRange(theta1,theta2,true);
                isStartEndInverted.insert(std::make_pair(*it, true));
            }
            else if(geosym->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()){
                Part::GeomBSplineCurve *geosymbsp = static_cast<Part::GeomBSplineCurve *>(geosym);

                std::vector<Base::Vector3d> poles = geosymbsp->getPoles();

                for(std::vector<Base::Vector3d>::iterator jt = poles.begin(); jt != poles.end(); ++jt){

                    (*jt) = (*jt) + 2.0*((*jt).Perpendicular(refGeoLine->getStartPoint(),vectline)-(*jt));
                }

                geosymbsp->setPoles(poles);

                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomPoint::getClassTypeId()){
                Part::GeomPoint *geosympoint = static_cast<Part::GeomPoint *>(geosym);
                Base::Vector3d cp = geosympoint->getPoint();

                geosympoint->setPoint(cp+2.0*(cp.Perpendicular(refGeoLine->getStartPoint(),vectline)-cp));
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else {
                Base::Console().Error("Unsupported Geometry!! Just copying it.\n");
                isStartEndInverted.insert(std::make_pair(*it, false));
            }

            newgeoVals.push_back(geosym);
            geoIdMap.insert(std::make_pair(*it, cgeoid));
            cgeoid++;
        }
    }
    else { //reference is a point
        Vector3d refpoint;
        const Part::Geometry *georef = getGeometry(refGeoId);

        if (georef->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            refpoint = static_cast<const Part::GeomPoint *>(georef)->getPoint();
        }
        else if ( refGeoId == -1 && refPosId == Sketcher::start) {
            refpoint = Vector3d(0,0,0);
        }
        else {
            switch(refPosId){
                case Sketcher::start:
                    if(georef->getTypeId() == Part::GeomLineSegment::getClassTypeId()){
                        const Part::GeomLineSegment *geosymline = static_cast<const Part::GeomLineSegment *>(georef);
                        refpoint = geosymline->getStartPoint();
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()){
                        const Part::GeomArcOfCircle *geoaoc = static_cast<const Part::GeomArcOfCircle *>(georef);
                        refpoint = geoaoc->getStartPoint(true);
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()){
                        const Part::GeomArcOfEllipse *geosymaoe = static_cast<const Part::GeomArcOfEllipse *>(georef);
                        refpoint = geosymaoe->getStartPoint(true);
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()){
                        const Part::GeomArcOfHyperbola *geosymaoe = static_cast<const Part::GeomArcOfHyperbola *>(georef);
                        refpoint = geosymaoe->getStartPoint(true);
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()){
                        const Part::GeomArcOfParabola *geosymaoe = static_cast<const Part::GeomArcOfParabola *>(georef);
                        refpoint = geosymaoe->getStartPoint(true);
                    } else if(georef->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()){
                        const Part::GeomBSplineCurve *geosymbsp = static_cast<const Part::GeomBSplineCurve *>(georef);
                        refpoint = geosymbsp->getStartPoint();
                    }
                    break;
                case Sketcher::end:
                    if(georef->getTypeId() == Part::GeomLineSegment::getClassTypeId()){
                        const Part::GeomLineSegment *geosymline = static_cast<const Part::GeomLineSegment *>(georef);
                        refpoint = geosymline->getEndPoint();
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()){
                        const Part::GeomArcOfCircle *geoaoc = static_cast<const Part::GeomArcOfCircle *>(georef);
                        refpoint = geoaoc->getEndPoint(true);
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()){
                        const Part::GeomArcOfEllipse *geosymaoe = static_cast<const Part::GeomArcOfEllipse *>(georef);
                        refpoint = geosymaoe->getEndPoint(true);
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()){
                        const Part::GeomArcOfHyperbola *geosymaoe = static_cast<const Part::GeomArcOfHyperbola *>(georef);
                        refpoint = geosymaoe->getEndPoint(true);
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()){
                        const Part::GeomArcOfParabola *geosymaoe = static_cast<const Part::GeomArcOfParabola *>(georef);
                        refpoint = geosymaoe->getEndPoint(true);
                    }
                    else if(georef->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()){
                        const Part::GeomBSplineCurve *geosymbsp = static_cast<const Part::GeomBSplineCurve *>(georef);
                        refpoint = geosymbsp->getEndPoint();
                    }
                    break;
                case Sketcher::mid:
                    if(georef->getTypeId() == Part::GeomCircle::getClassTypeId()){
                        const Part::GeomCircle *geosymcircle = static_cast<const Part::GeomCircle *>(georef);
                        refpoint = geosymcircle->getCenter();
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()){
                        const Part::GeomArcOfCircle *geoaoc = static_cast<const Part::GeomArcOfCircle *>(georef);
                        refpoint = geoaoc->getCenter();
                    }
                    else if(georef->getTypeId() == Part::GeomEllipse::getClassTypeId()){
                        const Part::GeomEllipse *geosymellipse = static_cast<const Part::GeomEllipse *>(georef);
                        refpoint = geosymellipse->getCenter();
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()){
                        const Part::GeomArcOfEllipse *geosymaoe = static_cast<const Part::GeomArcOfEllipse *>(georef);
                        refpoint = geosymaoe->getCenter();
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()){
                        const Part::GeomArcOfHyperbola *geosymaoe = static_cast<const Part::GeomArcOfHyperbola *>(georef);
                        refpoint = geosymaoe->getCenter();
                    }
                    else if(georef->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()){
                        const Part::GeomArcOfParabola *geosymaoe = static_cast<const Part::GeomArcOfParabola *>(georef);
                        refpoint = geosymaoe->getCenter();
                    }
                    break;
                default:
                    Base::Console().Error("Wrong PointPosId.\n");
                    return -1;
            }
        }

        for (std::vector<int>::const_iterator it = geoIdList.begin(); it != geoIdList.end(); ++it) {
            const Part::Geometry *geo = getGeometry(*it);
            Part::Geometry *geosym = geo->copy();
            GEN_ID(geosym);

            // Handle Geometry
            if(geosym->getTypeId() == Part::GeomLineSegment::getClassTypeId()){
                Part::GeomLineSegment *geosymline = static_cast<Part::GeomLineSegment *>(geosym);
                Base::Vector3d sp = geosymline->getStartPoint();
                Base::Vector3d ep = geosymline->getEndPoint();
                Base::Vector3d ssp = sp + 2.0*(refpoint-sp);
                Base::Vector3d sep = ep + 2.0*(refpoint-ep);

                geosymline->setPoints(ssp, sep);
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomCircle::getClassTypeId()){
                Part::GeomCircle *geosymcircle = static_cast<Part::GeomCircle *>(geosym);
                Base::Vector3d cp = geosymcircle->getCenter();

                geosymcircle->setCenter(cp + 2.0*(refpoint-cp));
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()){
                Part::GeomArcOfCircle *geoaoc = static_cast<Part::GeomArcOfCircle *>(geosym);
                Base::Vector3d sp = geoaoc->getStartPoint(true);
                Base::Vector3d ep = geoaoc->getEndPoint(true);
                Base::Vector3d cp = geoaoc->getCenter();

                Base::Vector3d ssp = sp + 2.0*(refpoint-sp);
                Base::Vector3d sep = ep + 2.0*(refpoint-ep);
                Base::Vector3d scp = cp + 2.0*(refpoint-cp);

                double theta1 = Base::fmod(atan2(ssp.y - scp.y, ssp.x - scp.x), 2.f*M_PI);
                double theta2 = Base::fmod(atan2(sep.y - scp.y, sep.x - scp.x), 2.f*M_PI);

                geoaoc->setCenter(scp);
                geoaoc->setRange(theta1,theta2,true);
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomEllipse::getClassTypeId()){
                Part::GeomEllipse *geosymellipse = static_cast<Part::GeomEllipse *>(geosym);
                Base::Vector3d cp = geosymellipse->getCenter();

                Base::Vector3d majdir = geosymellipse->getMajorAxisDir();
                double majord=geosymellipse->getMajorRadius();
                double minord=geosymellipse->getMinorRadius();
                double df= sqrt(majord*majord-minord*minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0*(refpoint-f1);
                Base::Vector3d scp = cp + 2.0*(refpoint-cp);

                geosymellipse->setMajorAxisDir(sf1-scp);

                geosymellipse->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()){
                Part::GeomArcOfEllipse *geosymaoe = static_cast<Part::GeomArcOfEllipse *>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord=geosymaoe->getMajorRadius();
                double minord=geosymaoe->getMinorRadius();
                double df= sqrt(majord*majord-minord*minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0*(refpoint-f1);
                Base::Vector3d scp = cp + 2.0*(refpoint-cp);

                geosymaoe->setMajorAxisDir(sf1-scp);

                geosymaoe->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()){
                Part::GeomArcOfHyperbola *geosymaoe = static_cast<Part::GeomArcOfHyperbola *>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord=geosymaoe->getMajorRadius();
                double minord=geosymaoe->getMinorRadius();
                double df= sqrt(majord*majord+minord*minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0*(refpoint-f1);
                Base::Vector3d scp = cp + 2.0*(refpoint-cp);

                geosymaoe->setMajorAxisDir(sf1-scp);

                geosymaoe->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()){
                Part::GeomArcOfParabola *geosymaoe = static_cast<Part::GeomArcOfParabola *>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                /*double df= geosymaoe->getFocal();*/
                Base::Vector3d f1 = geosymaoe->getFocus();

                Base::Vector3d sf1 = f1 + 2.0*(refpoint-f1);
                Base::Vector3d scp = cp + 2.0*(refpoint-cp);

                geosymaoe->setXAxisDir(sf1-scp);
                geosymaoe->setCenter(scp);

                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()){
                Part::GeomBSplineCurve *geosymbsp = static_cast<Part::GeomBSplineCurve *>(geosym);

                std::vector<Base::Vector3d> poles = geosymbsp->getPoles();

                for(std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it){
                    (*it) = (*it) + 2.0*(refpoint-(*it));
                }

                geosymbsp->setPoles(poles);

                //isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if(geosym->getTypeId() == Part::GeomPoint::getClassTypeId()){
                Part::GeomPoint *geosympoint = static_cast<Part::GeomPoint *>(geosym);
                Base::Vector3d cp = geosympoint->getPoint();

                geosympoint->setPoint(cp + 2.0*(refpoint-cp));
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else {
                Base::Console().Error("Unsupported Geometry!! Just copying it.\n");
                isStartEndInverted.insert(std::make_pair(*it, false));
            }

            newgeoVals.push_back(geosym);
            geoIdMap.insert(std::make_pair(*it, cgeoid));
            cgeoid++;
        }
    }

    // add the geometry
    Geometry.setValues(newgeoVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    for (std::vector<Constraint *>::const_iterator it = constrvals.begin(); it != constrvals.end(); ++it) {

        std::vector<int>::const_iterator fit=std::find(geoIdList.begin(), geoIdList.end(), (*it)->First);

        if(fit != geoIdList.end()) { // if First of constraint is in geoIdList

            if( (*it)->Second == Constraint::GeoUndef /*&& (*it)->Third == Constraint::GeoUndef*/) {
                if( (*it)->Type != Sketcher::DistanceX &&
                    (*it)->Type != Sketcher::DistanceY) {

                    Constraint *constNew = (*it)->copy();

                    constNew->First = geoIdMap[(*it)->First];
                    newconstrVals.push_back(constNew);
                }
            }
            else { // other geoids intervene in this constraint

                std::vector<int>::const_iterator sit=std::find(geoIdList.begin(), geoIdList.end(), (*it)->Second);

                if(sit != geoIdList.end()) { // Second is also in the list

                    if( (*it)->Third == Constraint::GeoUndef ) {
                        if((*it)->Type ==  Sketcher::Coincident ||
                        (*it)->Type ==  Sketcher::Perpendicular ||
                        (*it)->Type ==  Sketcher::Parallel ||
                        (*it)->Type ==  Sketcher::Tangent ||
                        (*it)->Type ==  Sketcher::Distance ||
                        (*it)->Type ==  Sketcher::Equal ||
                        (*it)->Type ==  Sketcher::Radius ||
                        (*it)->Type ==  Sketcher::Diameter ||
                        (*it)->Type ==  Sketcher::Angle ||
                        (*it)->Type ==  Sketcher::PointOnObject ){
                            Constraint *constNew = (*it)->copy();

                            constNew->First = geoIdMap[(*it)->First];
                            constNew->Second = geoIdMap[(*it)->Second];
                            if(isStartEndInverted[(*it)->First]){
                                if((*it)->FirstPos == Sketcher::start)
                                    constNew->FirstPos = Sketcher::end;
                                else if((*it)->FirstPos == Sketcher::end)
                                    constNew->FirstPos = Sketcher::start;
                            }
                            if(isStartEndInverted[(*it)->Second]){
                                if((*it)->SecondPos == Sketcher::start)
                                    constNew->SecondPos = Sketcher::end;
                                else if((*it)->SecondPos == Sketcher::end)
                                    constNew->SecondPos = Sketcher::start;
                            }

                            if (constNew->Type == Tangent || constNew->Type == Perpendicular)
                                AutoLockTangencyAndPerpty(constNew,true);

                            if( ((*it)->Type ==  Sketcher::Angle) && (refPosId == Sketcher::none)) {
                                constNew->setValue(-(*it)->getValue());
                            }

                            newconstrVals.push_back(constNew);
                        }
                    }
                    else {
                        std::vector<int>::const_iterator tit=std::find(geoIdList.begin(), geoIdList.end(), (*it)->Third);

                        if(tit != geoIdList.end()) { // Third is also in the list
                            Constraint *constNew = (*it)->copy();
                            constNew->First = geoIdMap[(*it)->First];
                            constNew->Second = geoIdMap[(*it)->Second];
                            constNew->Third = geoIdMap[(*it)->Third];
                            if(isStartEndInverted[(*it)->First]){
                                if((*it)->FirstPos == Sketcher::start)
                                    constNew->FirstPos = Sketcher::end;
                                else if((*it)->FirstPos == Sketcher::end)
                                    constNew->FirstPos = Sketcher::start;
                            }
                            if(isStartEndInverted[(*it)->Second]){
                                if((*it)->SecondPos == Sketcher::start)
                                    constNew->SecondPos = Sketcher::end;
                                else if((*it)->SecondPos == Sketcher::end)
                                    constNew->SecondPos = Sketcher::start;
                            }
                            if(isStartEndInverted[(*it)->Third]){
                                if((*it)->ThirdPos == Sketcher::start)
                                    constNew->ThirdPos = Sketcher::end;
                                else if((*it)->ThirdPos == Sketcher::end)
                                    constNew->ThirdPos = Sketcher::start;
                            }
                            newconstrVals.push_back(constNew);
                        }
                    }
                }
            }
        }
    }

    if( newconstrVals.size() > constrvals.size() )
        Constraints.setValues(newconstrVals);

    return Geometry.getSize()-1;
}

int SketchObject::addCopy(const std::vector<int> &geoIdList, const Base::Vector3d& displacement, bool moveonly /*=false*/, bool clone /*=false*/, int csize/*=2*/, int rsize/*=1*/,
                          bool constraindisplacement /*= false*/, double perpscale /*= 1.0*/)
{
    const std::vector< Part::Geometry * > &geovals = getInternalGeometry();
    std::vector< Part::Geometry * > newgeoVals(geovals);

    const std::vector< Constraint * > &constrvals = this->Constraints.getValues();
    std::vector< Constraint * > newconstrVals(constrvals);

    std::vector<int> newgeoIdList(geoIdList);

    if(newgeoIdList.size() == 0) {// default option to operate on all the geometry
        for(int i = 0; i < int(geovals.size()); i++)
            newgeoIdList.push_back(i);
    }

    int cgeoid = getHighestCurveIndex()+1;

    int iterfirstgeoid = -1 ;

    Base::Vector3d iterfirstpoint;

    int refgeoid = -1;

    int colrefgeoid = 0, rowrefgeoid = 0;

    int currentrowfirstgeoid= -1, prevrowstartfirstgeoid = -1, prevfirstgeoid = -1;

    Sketcher::PointPos refposId = Sketcher::none;

    std::map<int, int> geoIdMap;

    Base::Vector3d perpendicularDisplacement = Base::Vector3d(perpscale*displacement.y,perpscale*-displacement.x,0);

    int x,y;

    for (y=0;y<rsize;y++) {
        for (x=0;x<csize;x++) {

            if(x == 0 && y == 0) { // the reference for constraining array elements is the first valid point of the first element
                const Part::Geometry *geo = getGeometry(*(newgeoIdList.begin()));
                refgeoid=*(newgeoIdList.begin());
                currentrowfirstgeoid = refgeoid;
                iterfirstgeoid = refgeoid;
                if(geo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                    geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ){
                    refposId = Sketcher::mid;
                }
                else
                    refposId = Sketcher::start;

                continue; // the first element is already in place
            }
            else {
                prevfirstgeoid = iterfirstgeoid;

                iterfirstgeoid = cgeoid;

                if( x == 0 ) { // if first element of second row
                    prevrowstartfirstgeoid = currentrowfirstgeoid;
                    currentrowfirstgeoid = cgeoid;
                }
            }

            for (std::vector<int>::const_iterator it = newgeoIdList.begin(); it != newgeoIdList.end(); ++it) {
                const Part::Geometry *geo = getGeometry(*it);
                Part::Geometry *geocopy = moveonly?const_cast<Part::Geometry *>(geo):geo->copy();
                if(!moveonly)
                    GEN_ID(geocopy);

                // Handle Geometry
                if(geocopy->getTypeId() == Part::GeomLineSegment::getClassTypeId()){
                    Part::GeomLineSegment *geosymline = static_cast<Part::GeomLineSegment *>(geocopy);
                    Base::Vector3d ep = geosymline->getEndPoint();
                    Base::Vector3d ssp = geosymline->getStartPoint()+double(x)*displacement+double(y)*perpendicularDisplacement;

                    geosymline->setPoints(  ssp,
                                            ep+double(x)*displacement+double(y)*perpendicularDisplacement);

                    if(it == newgeoIdList.begin())
                        iterfirstpoint = ssp;
                }
                else if(geocopy->getTypeId() == Part::GeomCircle::getClassTypeId()){
                    Part::GeomCircle *geosymcircle = static_cast<Part::GeomCircle *>(geocopy);
                    Base::Vector3d cp = geosymcircle->getCenter();
                    Base::Vector3d scp = cp+double(x)*displacement+double(y)*perpendicularDisplacement;

                    geosymcircle->setCenter(scp);

                    if(it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else if(geocopy->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()){
                    Part::GeomArcOfCircle *geoaoc = static_cast<Part::GeomArcOfCircle *>(geocopy);
                    Base::Vector3d cp = geoaoc->getCenter();
                    Base::Vector3d scp = cp+double(x)*displacement+double(y)*perpendicularDisplacement;

                    geoaoc->setCenter(scp);

                    if(it == newgeoIdList.begin())
                        iterfirstpoint = geoaoc->getStartPoint(true);
                }
                else if(geocopy->getTypeId() == Part::GeomEllipse::getClassTypeId()){
                    Part::GeomEllipse *geosymellipse = static_cast<Part::GeomEllipse *>(geocopy);
                    Base::Vector3d cp = geosymellipse->getCenter();
                    Base::Vector3d scp = cp+double(x)*displacement+double(y)*perpendicularDisplacement;

                    geosymellipse->setCenter(scp);

                    if(it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else if(geocopy->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()){
                    Part::GeomArcOfEllipse *geoaoe = static_cast<Part::GeomArcOfEllipse *>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp = cp+double(x)*displacement+double(y)*perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if(it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if(geocopy->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()){
                    Part::GeomArcOfHyperbola *geoaoe = static_cast<Part::GeomArcOfHyperbola *>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp = cp+double(x)*displacement+double(y)*perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if(it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if(geocopy->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()){
                    Part::GeomArcOfParabola *geoaoe = static_cast<Part::GeomArcOfParabola *>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp = cp+double(x)*displacement+double(y)*perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if(it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if(geocopy->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()){
                    Part::GeomBSplineCurve *geobsp = static_cast<Part::GeomBSplineCurve *>(geocopy);

                    std::vector<Base::Vector3d> poles = geobsp->getPoles();

                    for(std::vector<Base::Vector3d>::iterator jt = poles.begin(); jt != poles.end(); ++jt){

                        (*jt) = (*jt) + double(x)*displacement + double(y)*perpendicularDisplacement;
                    }

                    geobsp->setPoles(poles);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geobsp->getStartPoint();
                }
                else if(geocopy->getTypeId() == Part::GeomPoint::getClassTypeId()){
                    Part::GeomPoint *geopoint = static_cast<Part::GeomPoint *>(geocopy);
                    Base::Vector3d cp = geopoint->getPoint();
                    Base::Vector3d scp = cp+double(x)*displacement+double(y)*perpendicularDisplacement;
                    geopoint->setPoint(scp);

                    if(it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else {
                    Base::Console().Error("Unsupported Geometry!! Just skipping it.\n");
                    continue;
                }

                if(!moveonly) {
                    newgeoVals.push_back(geocopy);
                    geoIdMap.insert(std::make_pair(*it, cgeoid));
                    cgeoid++;
                }
            }

            if(!moveonly) {
                // handle geometry constraints
                for (std::vector<Constraint *>::const_iterator it = constrvals.begin(); it != constrvals.end(); ++it) {

                    std::vector<int>::const_iterator fit=std::find(newgeoIdList.begin(), newgeoIdList.end(), (*it)->First);

                    if(fit != newgeoIdList.end()) { // if First of constraint is in geoIdList

                        if( (*it)->Second == Constraint::GeoUndef /*&& (*it)->Third == Constraint::GeoUndef*/) {
                            if( ((*it)->Type != Sketcher::DistanceX && (*it)->Type != Sketcher::DistanceY ) ||
                                (*it)->FirstPos == Sketcher::none ) { // if it is not a point locking DistanceX/Y
                                    if (((*it)->Type == Sketcher::DistanceX ||
                                        (*it)->Type == Sketcher::DistanceY ||
                                        (*it)->Type == Sketcher::Distance  ||
                                        (*it)->Type == Sketcher::Diameter ||
                                        (*it)->Type == Sketcher::Radius ) && clone ) {
                                        // Distances on a single Element are mapped to equality constraints in clone mode
                                        Constraint *constNew = (*it)->copy();
                                        constNew->Type = Sketcher::Equal;
                                        constNew->Second = geoIdMap[(*it)->First]; // first is already (*it->First)
                                        newconstrVals.push_back(constNew);
                                    }
                                    else if ((*it)->Type == Sketcher::Angle && clone){
                                        // Angles on a single Element are mapped to parallel constraints in clone mode
                                        Constraint *constNew = (*it)->copy();
                                        constNew->Type = Sketcher::Parallel;
                                        constNew->Second = geoIdMap[(*it)->First]; // first is already (*it->First)
                                        newconstrVals.push_back(constNew);
                                    }
                                    else {
                                        Constraint *constNew = (*it)->copy();
                                        constNew->First = geoIdMap[(*it)->First];
                                        newconstrVals.push_back(constNew);
                                    }
                            }
                        }
                        else { // other geoids intervene in this constraint

                            std::vector<int>::const_iterator sit=std::find(newgeoIdList.begin(), newgeoIdList.end(), (*it)->Second);

                            if(sit != newgeoIdList.end()) { // Second is also in the list
                                if( (*it)->Third == Constraint::GeoUndef ) {
                                    if (((*it)->Type == Sketcher::DistanceX ||
                                        (*it)->Type == Sketcher::DistanceY ||
                                        (*it)->Type == Sketcher::Distance) && ((*it)->First == (*it)->Second) && clone ) {
                                        // Distances on a two Elements, which must be points of the same line are mapped to equality constraints in clone mode
                                        Constraint *constNew = (*it)->copy();
                                        constNew->Type = Sketcher::Equal;
                                        constNew->FirstPos = Sketcher::none;
                                        constNew->Second = geoIdMap[(*it)->First]; // first is already (*it->First)
                                        constNew->SecondPos = Sketcher::none;
                                        newconstrVals.push_back(constNew);
                                    }
                                    else {
                                        Constraint *constNew = (*it)->copy();
                                        constNew->First = geoIdMap[(*it)->First];
                                        constNew->Second = geoIdMap[(*it)->Second];
                                        newconstrVals.push_back(constNew);
                                    }
                                }
                                else {
                                    std::vector<int>::const_iterator tit=std::find(newgeoIdList.begin(), newgeoIdList.end(), (*it)->Third);

                                    if(tit != newgeoIdList.end()) { // Third is also in the list
                                        Constraint *constNew = (*it)->copy();
                                        constNew->First = geoIdMap[(*it)->First];
                                        constNew->Second = geoIdMap[(*it)->Second];
                                        constNew->Third = geoIdMap[(*it)->Third];

                                        newconstrVals.push_back(constNew);
                                    }
                                }
                            }
                        }
                    }
                }

                // handle inter-geometry constraints
                if(constraindisplacement){

                    // add a construction line
                    Part::GeomLineSegment *constrline= new Part::GeomLineSegment();

                    Base::Vector3d sp = getPoint(refgeoid,refposId)+ ( ( x == 0 )?
                                    (double(x)*displacement+double(y-1)*perpendicularDisplacement):
                                    (double(x-1)*displacement+double(y)*perpendicularDisplacement)); // position of the reference point
                    Base::Vector3d ep = iterfirstpoint; // position of the current instance corresponding point
                    constrline->setPoints(sp,ep);
                    constrline->Construction=true;

                    GEN_ID(constrline);
                    newgeoVals.push_back(constrline);

                    Constraint *constNew;

                    if(x == 0) { // first element of a row

                        // add coincidents for construction line
                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = prevrowstartfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::start;
                        newconstrVals.push_back(constNew);

                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = iterfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::end;
                        newconstrVals.push_back(constNew);

                        if( y == 1 ) { // it is the first added element of this row in the perpendicular to displacementvector direction
                            rowrefgeoid = cgeoid;
                            cgeoid++;

                            // add length (or equal if perpscale==1) and perpendicular
                            if(perpscale==1.0) {
                                constNew = new Constraint();
                                constNew->Type = Sketcher::Equal;
                                constNew->First = rowrefgeoid;
                                constNew->FirstPos = Sketcher::none;
                                constNew->Second = colrefgeoid;
                                constNew->SecondPos = Sketcher::none;
                                newconstrVals.push_back(constNew);
                            } else {
                                constNew = new Constraint();
                                constNew->Type = Sketcher::Distance;
                                constNew->First = rowrefgeoid;
                                constNew->FirstPos = Sketcher::none;
                                constNew->setValue(perpendicularDisplacement.Length());
                                newconstrVals.push_back(constNew);
                            }

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Perpendicular;
                            constNew->First = rowrefgeoid;
                            constNew->FirstPos = Sketcher::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::none;
                            newconstrVals.push_back(constNew);
                        }
                        else { // it is just one more element in the col direction
                            cgeoid++;

                            // all other first rowers get an equality and perpendicular constraint
                            constNew = new Constraint();
                            constNew->Type = Sketcher::Equal;
                            constNew->First = rowrefgeoid;
                            constNew->FirstPos = Sketcher::none;
                            constNew->Second = cgeoid-1;
                            constNew->SecondPos = Sketcher::none;
                            newconstrVals.push_back(constNew);

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Perpendicular;
                            constNew->First = cgeoid-1;
                            constNew->FirstPos = Sketcher::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::none;
                            newconstrVals.push_back(constNew);
                        }
                    }
                    else { // any element not being the first element of a row

                        // add coincidents for construction line
                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = prevfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::start;
                        newconstrVals.push_back(constNew);

                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = iterfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::end;
                        newconstrVals.push_back(constNew);

                        if(y == 0 && x == 1) { // first element of the first row
                                colrefgeoid = cgeoid;
                                cgeoid++;

                                // add length and Angle
                                constNew = new Constraint();
                                constNew->Type = Sketcher::Distance;
                                constNew->First = colrefgeoid;
                                constNew->FirstPos = Sketcher::none;
                                constNew->setValue(displacement.Length());
                                newconstrVals.push_back(constNew);

                                constNew = new Constraint();
                                constNew->Type = Sketcher::Angle;
                                constNew->First = colrefgeoid;
                                constNew->FirstPos = Sketcher::none;
                                constNew->setValue(atan2(displacement.y,displacement.x));
                                newconstrVals.push_back(constNew);
                        }
                        else { // any other element
                            cgeoid++;

                            // all other elements get an equality and parallel constraint
                            constNew = new Constraint();
                            constNew->Type = Sketcher::Equal;
                            constNew->First = colrefgeoid;
                            constNew->FirstPos = Sketcher::none;
                            constNew->Second = cgeoid-1;
                            constNew->SecondPos = Sketcher::none;
                            newconstrVals.push_back(constNew);

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Parallel;
                            constNew->First = cgeoid-1;
                            constNew->FirstPos = Sketcher::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::none;
                            newconstrVals.push_back(constNew);
                        }
                    }
                }

            }

            geoIdMap.clear(); // after each creation reset map so that the key-value is univoque
        }
    }

    Geometry.setValues(newgeoVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    if( newconstrVals.size() > constrvals.size() )
        Constraints.setValues(newconstrVals);

    return Geometry.getSize()-1;

}

int SketchObject::exposeInternalGeometry(int GeoId)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const Part::Geometry *geo = getGeometry(GeoId);
    // Only for supported types
    if(geo->getTypeId() == Part::GeomEllipse::getClassTypeId() || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        // First we search what has to be restored
        bool major=false;
        bool minor=false;
        bool focus1=false;
        bool focus2=false;

        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();

        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::EllipseMajorDiameter:
                        major=true;
                        break;
                    case Sketcher::EllipseMinorDiameter:
                        minor=true;
                        break;
                    case Sketcher::EllipseFocus1:
                        focus1=true;
                        break;
                    case Sketcher::EllipseFocus2:
                        focus2=true;
                        break;
                    default:
                        return -1;
                }
            }
        }

        int currentgeoid= getHighestCurveIndex();
        int incrgeo= 0;

        Base::Vector3d center;
        double majord;
        double minord;
        Base::Vector3d majdir;

        std::vector<Part::Geometry *> igeo;
        std::vector<Constraint *> icon;

        if(geo->getTypeId() == Part::GeomEllipse::getClassTypeId()){
            const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo);

            center=ellipse->getCenter();
            majord=ellipse->getMajorRadius();
            minord=ellipse->getMinorRadius();
            majdir=ellipse->getMajorAxisDir();
        }
        else {
            const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo);

            center=aoe->getCenter();
            majord=aoe->getMajorRadius();
            minord=aoe->getMinorRadius();
            majdir=aoe->getMajorAxisDir();
        }

        Base::Vector3d mindir = Vector3d(-majdir.y, majdir.x);

        Base::Vector3d majorpositiveend = center + majord * majdir;
        Base::Vector3d majornegativeend = center - majord * majdir;
        Base::Vector3d minorpositiveend = center + minord * mindir;
        Base::Vector3d minornegativeend = center - minord * mindir;

        double df= sqrt(majord*majord-minord*minord);

        Base::Vector3d focus1P = center + df * majdir;
        Base::Vector3d focus2P = center - df * majdir;

        if(!major)
        {
            Part::GeomLineSegment *lmajor = new Part::GeomLineSegment();
            lmajor->setPoints(majorpositiveend,majornegativeend);

            igeo.push_back(lmajor);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = EllipseMajorDiameter;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->Second = GeoId;

            icon.push_back(newConstr);
            incrgeo++;
        }
        if(!minor)
        {
            Part::GeomLineSegment *lminor = new Part::GeomLineSegment();
            lminor->setPoints(minorpositiveend,minornegativeend);

            igeo.push_back(lminor);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = EllipseMinorDiameter;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->Second = GeoId;

            icon.push_back(newConstr);
            incrgeo++;
        }
        if(!focus1)
        {
            Part::GeomPoint *pf1 = new Part::GeomPoint();
            pf1->setPoint(focus1P);

            igeo.push_back(pf1);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = EllipseFocus1;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->FirstPos = Sketcher::start;
            newConstr->Second = GeoId;

            icon.push_back(newConstr);
            incrgeo++;
        }
        if(!focus2)
        {
            Part::GeomPoint *pf2 = new Part::GeomPoint();
            pf2->setPoint(focus2P);
            igeo.push_back(pf2);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = EllipseFocus2;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->FirstPos = Sketcher::start;
            newConstr->Second = GeoId;

            icon.push_back(newConstr);
        }

        this->addGeometry(igeo,true);
        this->addConstraints(icon);

        for (std::vector<Part::Geometry *>::iterator it=igeo.begin(); it != igeo.end(); ++it) {
            if (*it)
                delete *it;
        }

        for (std::vector<Constraint *>::iterator it=icon.begin(); it != icon.end(); ++it) {
            if (*it)
                delete *it;
        }

        icon.clear();
        igeo.clear();

        return incrgeo; //number of added elements
    }
    else if(geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
        // First we search what has to be restored
        bool major=false;
        bool minor=false;
        bool focus=false;

        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();

        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
             it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::HyperbolaMajor:
                        major=true;
                        break;
                    case Sketcher::HyperbolaMinor:
                        minor=true;
                        break;
                    case Sketcher::HyperbolaFocus:
                        focus=true;
                        break;
                    default:
                        return -1;
                }
            }
        }

        int currentgeoid= getHighestCurveIndex();
        int incrgeo= 0;

        const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo);

        Base::Vector3d center = aoh->getCenter();
        double majord = aoh->getMajorRadius();
        double minord = aoh->getMinorRadius();
        Base::Vector3d majdir = aoh->getMajorAxisDir();

        std::vector<Part::Geometry *> igeo;
        std::vector<Constraint *> icon;

        Base::Vector3d mindir = Vector3d(-majdir.y, majdir.x);

        Base::Vector3d majorpositiveend = center + majord * majdir;
        Base::Vector3d majornegativeend = center - majord * majdir;
        Base::Vector3d minorpositiveend = majorpositiveend + minord * mindir;
        Base::Vector3d minornegativeend = majorpositiveend - minord * mindir;

        double df= sqrt(majord*majord+minord*minord);

        Base::Vector3d focus1P = center + df * majdir;

        if(!major)
        {
            Part::GeomLineSegment *lmajor = new Part::GeomLineSegment();
            lmajor->setPoints(majorpositiveend,majornegativeend);

            igeo.push_back(lmajor);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = Sketcher::HyperbolaMajor;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->Second = GeoId;

            icon.push_back(newConstr);
            incrgeo++;
        }
        if(!minor)
        {
            Part::GeomLineSegment *lminor = new Part::GeomLineSegment();
            lminor->setPoints(minorpositiveend,minornegativeend);

            igeo.push_back(lminor);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = Sketcher::HyperbolaMinor;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->Second = GeoId;

            icon.push_back(newConstr);

            incrgeo++;
        }
        if(!focus)
        {
            Part::GeomPoint *pf1 = new Part::GeomPoint();
            pf1->setPoint(focus1P);

            igeo.push_back(pf1);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = Sketcher::HyperbolaFocus;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->FirstPos = Sketcher::start;
            newConstr->Second = GeoId;

            icon.push_back(newConstr);
            incrgeo++;
        }

        this->addGeometry(igeo,true);
        this->addConstraints(icon);

        for (std::vector<Part::Geometry *>::iterator it=igeo.begin(); it != igeo.end(); ++it)
            if (*it)
                delete *it;

        for (std::vector<Constraint *>::iterator it=icon.begin(); it != icon.end(); ++it)
            if (*it)
                delete *it;

        icon.clear();
        igeo.clear();

        return incrgeo; //number of added elements
    }
    else if(geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
        // First we search what has to be restored
        bool focus=false;
        int focusgeoid=-1;
        bool focus_to_vertex=false;

        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();

        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
             it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::ParabolaFocus:
                        focus=true;
                        focusgeoid=(*it)->First;
                        break;
                    default:
                        return -1;
                }
            }
        }

        if(focus) {
            // look for a line from focusgeoid:start to Geoid:mid_external
            std::vector<int> focusgeoidlistgeoidlist;
            std::vector<PointPos> focusposidlist;
            getDirectlyCoincidentPoints(focusgeoid, Sketcher::start, focusgeoidlistgeoidlist,
                                           focusposidlist);

            std::vector<int> parabgeoidlistgeoidlist;
            std::vector<PointPos> parabposidlist;
            getDirectlyCoincidentPoints(GeoId, Sketcher::mid, parabgeoidlistgeoidlist,
                                       parabposidlist);

            if (!focusgeoidlistgeoidlist.empty() && !parabgeoidlistgeoidlist.empty()) {
                std::size_t i,j;
                for(i=0;i<focusgeoidlistgeoidlist.size();i++) {
                    for(j=0;j<parabgeoidlistgeoidlist.size();j++) {
                        if(focusgeoidlistgeoidlist[i] == parabgeoidlistgeoidlist[j]) {
                            const Part::Geometry * geo = getGeometry(focusgeoidlistgeoidlist[i]);
                            if (geo && geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                if((focusposidlist[i] == Sketcher::start && parabposidlist[j] == Sketcher::end) ||
                                    (focusposidlist[i] == Sketcher::end && parabposidlist[j] == Sketcher::start))
                                    focus_to_vertex=true;
                            }
                        }
                    }
                }
            }
        }

        int currentgeoid= getHighestCurveIndex();
        int incrgeo= 0;

        const Part::GeomArcOfParabola *aoh = static_cast<const Part::GeomArcOfParabola *>(geo);

        Base::Vector3d center = aoh->getCenter();
        Base::Vector3d focusp = aoh->getFocus();

        std::vector<Part::Geometry *> igeo;
        std::vector<Constraint *> icon;

       if (!focus) {
            Part::GeomPoint *pf1 = new Part::GeomPoint();
            pf1->setPoint(focusp);

            igeo.push_back(pf1);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = Sketcher::ParabolaFocus;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->FirstPos = Sketcher::start;
            newConstr->Second = GeoId;

            focusgeoid = currentgeoid+incrgeo+1;

            icon.push_back(newConstr);
            incrgeo++;
        }

        if(!focus_to_vertex)
        {
            Part::GeomLineSegment *paxis = new Part::GeomLineSegment();
            paxis->setPoints(center,focusp);

            igeo.push_back(paxis);

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::Coincident;
            newConstr->First = focusgeoid;
            newConstr->FirstPos = Sketcher::start;
            newConstr->Second = currentgeoid+incrgeo+1; // just added line
            newConstr->SecondPos = Sketcher::end;

            icon.push_back(newConstr);

            Sketcher::Constraint *newConstr2 = new Sketcher::Constraint();
            newConstr2->Type = Sketcher::Coincident;
            newConstr2->First = GeoId;
            newConstr2->FirstPos = Sketcher::mid;
            newConstr2->Second = currentgeoid+incrgeo+1; // just added line
            newConstr2->SecondPos = Sketcher::start;

            icon.push_back(newConstr2);

            incrgeo++;
        }

        this->addGeometry(igeo,true);
        this->addConstraints(icon);

        for (std::vector<Part::Geometry *>::iterator it=igeo.begin(); it != igeo.end(); ++it) {
            if (*it)
                delete *it;
        }

        for (std::vector<Constraint *>::iterator it=icon.begin(); it != icon.end(); ++it) {
            if (*it)
                delete *it;
        }

        icon.clear();
        igeo.clear();

        return incrgeo; //number of added elements
    }
    else if(geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {

        const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve *>(geo);
        // First we search what has to be restored
        std::vector<bool> controlpoints(bsp->countPoles());
        std::vector<int> controlpointgeoids(bsp->countPoles());

        std::vector<bool> knotpoints(bsp->countKnots());
        std::vector<int> knotgeoids(bsp->countKnots());

        bool isfirstweightconstrained = false;

        std::vector<bool>::iterator itb;
        std::vector<int>::iterator it;

        for(it=controlpointgeoids.begin(), itb=controlpoints.begin(); it!=controlpointgeoids.end() && itb!=controlpoints.end(); ++it, ++itb) {
            (*it)=-1;
            (*itb)=false;
        }

        for(it=knotgeoids.begin(), itb=knotpoints.begin(); it!=knotgeoids.end() && itb!=knotpoints.end(); ++it, ++itb) {
            (*it)=-1;
            (*itb)=false;
        }

        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();

        // search for existing poles
        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
             it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::BSplineControlPoint:
                        controlpoints[(*it)->InternalAlignmentIndex] = true;
                        controlpointgeoids[(*it)->InternalAlignmentIndex] = (*it)->First;
                        break;
                    case Sketcher::BSplineKnotPoint:
                        knotpoints[(*it)->InternalAlignmentIndex] = true;
                        knotgeoids[(*it)->InternalAlignmentIndex] = (*it)->First;
                        break;
                    default:
                        return -1;
                }
            }
        }

        if(controlpoints[0]) {
            // search for first pole weight constraint
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                it != vals.end(); ++it) {
                if((*it)->Type == Sketcher::Radius && (*it)->First == controlpointgeoids[0]) {
                    isfirstweightconstrained = true ;
                }
                else if((*it)->Type == Sketcher::Diameter && (*it)->First == controlpointgeoids[0]) {
                        isfirstweightconstrained = true ;
                }
            }
        }

        int currentgeoid = getHighestCurveIndex();
        int incrgeo = 0;

        std::vector<Part::Geometry *> igeo;
        std::vector<Constraint *> icon;

        std::vector<Base::Vector3d> poles = bsp->getPoles();
        std::vector<double> knots = bsp->getKnots();

        double distance_p0_p1 = (poles[1]-poles[0]).Length(); // for visual purposes only

        int index=0;

        for(it=controlpointgeoids.begin(), itb=controlpoints.begin(); it!=controlpointgeoids.end() && itb!=controlpoints.end(); ++it, ++itb, index++) {

            if(!(*itb)) // if controlpoint not existing
            {
                Part::GeomCircle *pc = new Part::GeomCircle();
                pc->setCenter(poles[index]);
                pc->setRadius(distance_p0_p1/6);

                igeo.push_back(pc);

                Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                newConstr->Type = Sketcher::InternalAlignment;
                newConstr->AlignmentType = Sketcher::BSplineControlPoint;
                newConstr->First = currentgeoid+incrgeo+1;
                newConstr->FirstPos = Sketcher::mid;
                newConstr->Second = GeoId;
                newConstr->InternalAlignmentIndex = index;

                icon.push_back(newConstr);

                if(it != controlpointgeoids.begin()) {
                    // if pole-weight newly created AND first weight is radius-constrained,
                    // make it equal to first weight by default

                    if(isfirstweightconstrained) {
                        Sketcher::Constraint *newConstr2 = new Sketcher::Constraint();
                        newConstr2->Type = Sketcher::Equal;
                        newConstr2->First = currentgeoid+incrgeo+1;
                        newConstr2->FirstPos = Sketcher::none;
                        newConstr2->Second = controlpointgeoids[0];
                        newConstr2->SecondPos = Sketcher::none;

                        icon.push_back(newConstr2);
                    }
                }
                else {
                    controlpointgeoids[0] = currentgeoid+incrgeo+1;
                }

                incrgeo++;
            }
        }

        #if OCC_VERSION_HEX >= 0x060900
        index=0;

        for(it=knotgeoids.begin(), itb=knotpoints.begin(); it!=knotgeoids.end() && itb!=knotpoints.end(); ++it, ++itb, index++) {

            if(!(*itb)) // if knot point not existing
            {
                Part::GeomPoint *kp = new Part::GeomPoint();

                kp->setPoint(bsp->pointAtParameter(knots[index]));

                // a construction point, for now on, is a point that is not handled by the solver and does not contribute to the dofs
                // This is done so as to avoid having to add another data member to GeomPoint that is specific for the sketcher.
                kp->Construction=true;

                igeo.push_back(kp);

                Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                newConstr->Type = Sketcher::InternalAlignment;
                newConstr->AlignmentType = Sketcher::BSplineKnotPoint;
                newConstr->First = currentgeoid+incrgeo+1;
                newConstr->FirstPos = Sketcher::mid;
                newConstr->Second = GeoId;
                newConstr->InternalAlignmentIndex = index;

                icon.push_back(newConstr);

                incrgeo++;
            }
        }
        #endif

        Q_UNUSED(isfirstweightconstrained);
        // constraint the first weight to allow for seamless weight modification and proper visualization
        /*if(!isfirstweightconstrained) {

            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::Radius;
            newConstr->First = controlpointgeoids[0];
            newConstr->FirstPos = Sketcher::none;
            newConstr->setValue( round(distance_p0_p1/6)); // 1/6 is just an estimation for acceptable general visualization

            icon.push_back(newConstr);

        }*/

        this->addGeometry(igeo,true);
        this->addConstraints(icon);

        for (std::vector<Part::Geometry *>::iterator it=igeo.begin(); it != igeo.end(); ++it)
            if (*it)
                delete *it;

        for (std::vector<Constraint *>::iterator it=icon.begin(); it != icon.end(); ++it)
            if (*it)
                delete *it;

        icon.clear();
        igeo.clear();

        return incrgeo; //number of added elements
    }
    else
        return -1; // not supported type
}

int SketchObject::deleteUnusedInternalGeometry(int GeoId, bool delgeoid)
{
   if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const Part::Geometry *geo = getGeometry(GeoId);
    // Only for supported types
    if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
        geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {

        int majorelementindex=-1;
        int minorelementindex=-1;
        int focus1elementindex=-1;
        int focus2elementindex=-1;

        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();

        for (std::vector< Sketcher::Constraint * >::const_iterator it = vals.begin();
                it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::EllipseMajorDiameter:
                    case Sketcher::HyperbolaMajor:
                        majorelementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseMinorDiameter:
                    case Sketcher::HyperbolaMinor:
                        minorelementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseFocus1:
                    case Sketcher::HyperbolaFocus:
                        focus1elementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseFocus2:
                        focus2elementindex=(*it)->First;
                        break;
                    default:
                        return -1;
                }
            }
        }

        // Hide unused geometry here
        int majorconstraints=0; // number of constraints associated to the geoid of the major axis
        int minorconstraints=0;
        int focus1constraints=0;
        int focus2constraints=0;

        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
            it != vals.end(); ++it) {

            if((*it)->Second == majorelementindex || (*it)->First == majorelementindex || (*it)->Third == majorelementindex)
                majorconstraints++;
            else if((*it)->Second == minorelementindex || (*it)->First == minorelementindex || (*it)->Third == minorelementindex)
                minorconstraints++;
            else if((*it)->Second == focus1elementindex || (*it)->First == focus1elementindex || (*it)->Third == focus1elementindex)
                focus1constraints++;
            else if((*it)->Second == focus2elementindex || (*it)->First == focus2elementindex || (*it)->Third == focus2elementindex)
                focus2constraints++;
        }

        std::vector<int> delgeometries;

        // those with less than 2 constraints must be removed
        if (focus2constraints<2)
            delgeometries.push_back(focus2elementindex);

        if (focus1constraints<2)
            delgeometries.push_back(focus1elementindex);

        if (minorconstraints<2)
            delgeometries.push_back(minorelementindex);

        if (majorconstraints<2)
            delgeometries.push_back(majorelementindex);

        if(delgeoid)
            delgeometries.push_back(GeoId);

        std::sort(delgeometries.begin(), delgeometries.end()); // indices over an erased element get automatically updated!!

        if (delgeometries.size()>0) {
            for (std::vector<int>::reverse_iterator it=delgeometries.rbegin(); it!=delgeometries.rend(); ++it) {
                delGeometry(*it,false);
            }
        }

        int ndeleted =  delgeometries.size();

        delgeometries.clear();

        return ndeleted; //number of deleted elements
    }
    else if( geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
        // if the focus-to-vertex line is constrained, then never delete the focus
        // if the line is unconstrained, then the line may be deleted,
        // in this case the focus may be deleted if unconstrained.
        int majorelementindex=-1;
        int focus1elementindex=-1;

        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();

        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin(); it != vals.end(); ++it) {
            if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
                switch ((*it)->AlignmentType) {
                case Sketcher::ParabolaFocus:
                    focus1elementindex = (*it)->First;
                    break;
                default:
                    return -1;
                }
            }
        }

        if (focus1elementindex!=-1) {
            // look for a line from focusgeoid:start to Geoid:mid_external
            std::vector<int> focusgeoidlistgeoidlist;
            std::vector<PointPos> focusposidlist;
            getDirectlyCoincidentPoints(focus1elementindex, Sketcher::start, focusgeoidlistgeoidlist,
                                        focusposidlist);

            std::vector<int> parabgeoidlistgeoidlist;
            std::vector<PointPos> parabposidlist;
            getDirectlyCoincidentPoints(GeoId, Sketcher::mid, parabgeoidlistgeoidlist,
                                       parabposidlist);

            if (!focusgeoidlistgeoidlist.empty() && !parabgeoidlistgeoidlist.empty()) {
                std::size_t i,j;
                for (i=0;i<focusgeoidlistgeoidlist.size();i++) {
                    for (j=0;j<parabgeoidlistgeoidlist.size();j++) {
                        if (focusgeoidlistgeoidlist[i] == parabgeoidlistgeoidlist[j]) {
                            const Part::Geometry * geo = getGeometry(focusgeoidlistgeoidlist[i]);
                            if (geo && geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                if((focusposidlist[i] == Sketcher::start && parabposidlist[j] == Sketcher::end) ||
                                    (focusposidlist[i] == Sketcher::end && parabposidlist[j] == Sketcher::start))
                                    majorelementindex = focusgeoidlistgeoidlist[i];
                            }
                        }
                    }
                }
            }
        }

        // Hide unused geometry here
        int majorconstraints=0; // number of constraints associated to the geoid of the major axis other than the coincident ones
        int focus1constraints=0;

        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin(); it != vals.end(); ++it) {
            if( (*it)->Second == majorelementindex ||
                (*it)->First == majorelementindex  ||
                (*it)->Third == majorelementindex)
                majorconstraints++;
            else if ((*it)->Second == focus1elementindex ||
                (*it)->First == focus1elementindex ||
                (*it)->Third == focus1elementindex)
                focus1constraints++;
        }

        std::vector<int> delgeometries;

        if (majorelementindex !=-1 && majorconstraints<3) { // major as two coincidents to focus and vertex
            delgeometries.push_back(majorelementindex);
            majorelementindex = -1;
        }

        if (majorelementindex == -1 && focus1elementindex !=-1 && focus1constraints<3) // focus has one coincident and one internal align
            delgeometries.push_back(focus1elementindex);

        if(delgeoid)
            delgeometries.push_back(GeoId);

        std::sort(delgeometries.begin(), delgeometries.end()); // indices over an erased element get automatically updated!!

        if (delgeometries.size()>0) {
            for (std::vector<int>::reverse_iterator it=delgeometries.rbegin(); it!=delgeometries.rend(); ++it) {
                delGeometry(*it,false);
            }
        }

        int ndeleted =  delgeometries.size();

        delgeometries.clear();

        return ndeleted; //number of deleted elements
    }
    else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {

        const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve *>(geo);

        // First we search existing IA
        std::vector<int> controlpointgeoids(bsp->countPoles());
        std::vector<int> cpassociatedcontraints(bsp->countPoles());

        std::vector<int> knotgeoids(bsp->countKnots());
        std::vector<int> kassociatedcontraints(bsp->countKnots());

        std::vector<int>::iterator it;
        std::vector<int>::iterator ita;

        for (it=controlpointgeoids.begin(), ita=cpassociatedcontraints.begin(); it!=controlpointgeoids.end() && ita!=cpassociatedcontraints.end(); ++it, ++ita) {
            (*it) = -1;
            (*ita) = 0;
        }

        for (it=knotgeoids.begin(), ita=kassociatedcontraints.begin(); it!=knotgeoids.end() && ita!=kassociatedcontraints.end(); ++it, ++ita) {
            (*it) = -1;
            (*ita) = 0;
        }

        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();

        // search for existing poles
        for (std::vector< Sketcher::Constraint * >::const_iterator jt = vals.begin(); jt != vals.end(); ++jt) {
            if ((*jt)->Type == Sketcher::InternalAlignment && (*jt)->Second == GeoId) {
                switch ((*jt)->AlignmentType) {
                case Sketcher::BSplineControlPoint:
                    controlpointgeoids[(*jt)->InternalAlignmentIndex] = (*jt)->First;
                    break;
                case Sketcher::BSplineKnotPoint:
                    knotgeoids[(*jt)->InternalAlignmentIndex] = (*jt)->First;
                    break;
                default:
                    return -1;
                }
            }
        }

        std::vector<int> delgeometries;

        for (it=controlpointgeoids.begin(), ita=cpassociatedcontraints.begin(); it!=controlpointgeoids.end() && ita!=cpassociatedcontraints.end(); ++it, ++ita) {
            if ((*it) != -1) {
                // look for a circle at geoid index
                for (std::vector< Sketcher::Constraint * >::const_iterator itc= vals.begin(); itc != vals.end(); ++itc) {

                    if ( (*itc)->Type==Sketcher::Equal ) {
                        bool f=false,s=false;
                        for ( std::vector<int>::iterator its=controlpointgeoids.begin(); its!=controlpointgeoids.end(); ++its) {
                            if( (*itc)->First == *its ) {
                                f=true;
                            }
                            else if ( (*itc)->Second == *its ) {
                                s=true;
                            }

                            if (f && s) { // the equality constraint is not interpole
                                break;
                            }
                        }

                        if ( (f && !s) || (!f && s)  ) { // the equality constraint constraints a pole but it is not interpole
                            (*ita)++;
                        }

                    }
                        // ignore radiuses and diameters
                        else if (((*itc)->Type!=Sketcher::Radius && (*itc)->Type!=Sketcher::Diameter) && ( (*itc)->Second == (*it) || (*itc)->First == (*it) || (*itc)->Third == (*it)) )
                        (*ita)++;

                 }

                 if ( (*ita) < 2 ) { // IA
                     delgeometries.push_back((*it));
                 }
            }
        }

        for (it=knotgeoids.begin(), ita=kassociatedcontraints.begin(); it!=knotgeoids.end() && ita!=kassociatedcontraints.end(); ++it, ++ita) {
            if ((*it) != -1) {
                // look for a point at geoid index
                for (std::vector< Sketcher::Constraint * >::const_iterator itc= vals.begin(); itc != vals.end(); ++itc) {
                    if ((*itc)->Second == (*it) || (*itc)->First == (*it) || (*itc)->Third == (*it)) {
                        (*ita)++;
                    }
                }

                if ( (*ita) < 2 ) { // IA
                    delgeometries.push_back((*it));
                }
            }
        }


        if(delgeoid)
            delgeometries.push_back(GeoId);

        std::sort(delgeometries.begin(), delgeometries.end()); // indices over an erased element get automatically updated!!

        if (delgeometries.size()>0) {
            for (std::vector<int>::reverse_iterator it=delgeometries.rbegin(); it!=delgeometries.rend(); ++it) {
                delGeometry(*it,false);
            }
        }

        int ndeleted =  delgeometries.size();
        delgeometries.clear();

        return ndeleted; //number of deleted elements
    }
    else {
        return -1; // not supported type
    }
}

bool SketchObject::convertToNURBS(int GeoId)
{
    if (GeoId > getHighestCurveIndex() ||
        (GeoId < 0 && -GeoId > ExternalGeo.getSize()) ||
        GeoId == -1 || GeoId == -2)
        return false;

    const Part::Geometry *geo = getGeometry(GeoId);

    if(geo->getTypeId() == Part::GeomPoint::getClassTypeId())
        return false;

    const Part::GeomCurve *geo1 = static_cast<const Part::GeomCurve *>(geo);

    Part::GeomBSplineCurve* bspline;

    try {
        bspline = geo1->toNurbs(geo1->getFirstParameter(), geo1->getLastParameter());

        if(geo1->isDerivedFrom(Part::GeomArcOfConic::getClassTypeId())){
            const Part::GeomArcOfConic * geoaoc = static_cast<const Part::GeomArcOfConic *>(geo1);

            if(geoaoc->isReversed())
                bspline->reverse();
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        // revert to original values
        return false;
    }

    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);

    if (GeoId < 0) { // external geometry
        newVals.push_back(bspline);
        GEN_ID(bspline);
    }
    else { // normal geometry

        newVals[GeoId] = bspline;
        bspline->Id = geo->Id;

        const std::vector< Sketcher::Constraint * > &cvals = Constraints.getValues();

        std::vector< Constraint * > newcVals(cvals);

        int index = cvals.size()-1;
        // delete constraints on this elements other than coincident constraints (bspline does not support them currently)
        for (; index >= 0; index--) {
            if (cvals[index]->Type != Sketcher::Coincident && ( cvals[index]->First == GeoId || cvals[index]->Second == GeoId || cvals[index]->Third == GeoId)) {

                newcVals.erase(newcVals.begin()+index);

            }
        }
        this->Constraints.setValues(newcVals);
    }

    Geometry.setValues(newVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    delete bspline;

    return true;

}

bool SketchObject::increaseBSplineDegree(int GeoId, int degreeincrement /*= 1*/)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return false;

    const Part::Geometry *geo = getGeometry(GeoId);

    if (geo->getTypeId() != Part::GeomBSplineCurve::getClassTypeId())
        return false;

    const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve *>(geo);

    const Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(bsp->handle());

    Part::GeomBSplineCurve *bspline = new Part::GeomBSplineCurve(curve);


    try {
        int cdegree = bspline->getDegree();

        bspline->increaseDegree(cdegree+degreeincrement);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        return false;
    }

    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);

    newVals[GeoId] = bspline;
    bspline->Id = geo->Id;

    Geometry.setValues(newVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    return true;
}

bool SketchObject::modifyBSplineKnotMultiplicity(int GeoId, int knotIndex, int multiplicityincr)
{
    #if OCC_VERSION_HEX < 0x060900
        THROWMT(Base::NotImplementedError, QT_TRANSLATE_NOOP("Exceptions", "This version of OCE/OCC does not support knot operation. You need 6.9.0 or higher."))
    #endif

    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        THROWMT(Base::ValueError,QT_TRANSLATE_NOOP("Exceptions", "BSpline GeoId is out of bounds."))

    if (multiplicityincr == 0) // no change in multiplicity
        THROWMT(Base::ValueError,QT_TRANSLATE_NOOP("Exceptions", "You are requesting no change in knot multiplicity."))

    const Part::Geometry *geo = getGeometry(GeoId);

    if(geo->getTypeId() != Part::GeomBSplineCurve::getClassTypeId())
        THROWMT(Base::TypeError,QT_TRANSLATE_NOOP("Exceptions", "The GeoId provided is not a B-spline curve."))

    const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve *>(geo);

    int degree = bsp->getDegree();

    if( knotIndex > bsp->countKnots() || knotIndex < 1 ) // knotindex in OCC 1 -> countKnots
        THROWMT(Base::ValueError,QT_TRANSLATE_NOOP("Exceptions", "The knot index is out of bounds. Note that in accordance with OCC notation, the first knot has index 1 and not zero."))

    Part::GeomBSplineCurve *bspline;

    int curmult = bsp->getMultiplicity(knotIndex);

    if ( (curmult + multiplicityincr) > degree ) // zero is removing the knot, degree is just positional continuity
        THROWMT(Base::ValueError,QT_TRANSLATE_NOOP("Exceptions","The multiplicity cannot be increased beyond the degree of the b-spline."))

    if ( (curmult + multiplicityincr) < 0) // zero is removing the knot, degree is just positional continuity
        THROWMT(Base::ValueError,QT_TRANSLATE_NOOP("Exceptions", "The multiplicity cannot be decreased beyond zero."))

    try {

        bspline = static_cast<Part::GeomBSplineCurve *>(bsp->clone());

        if(multiplicityincr > 0) { // increase multiplicity
            bspline->increaseMultiplicity(knotIndex, curmult + multiplicityincr);
        }
        else { // decrease multiplicity
            bool result = bspline->removeKnot(knotIndex, curmult + multiplicityincr,1E6);

            if(!result)
                THROWMT(Base::CADKernelError, QT_TRANSLATE_NOOP("Exceptions", "OCC is unable to decrease the multiplicity within the maximum tolerance."))
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        return false;
    }

    // we succeeded with the multiplicity modification, so alignment geometry may be invalid/inconsistent for the new bspline

    std::vector<int> delGeoId;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<Base::Vector3d> newpoles = bspline->getPoles();
    std::vector<int> prevpole(bsp->countPoles());

    for(int i = 0; i < int(poles.size()); i++)
        prevpole[i] = -1;

    int taken = 0;
    for(int j = 0; j < int(poles.size()); j++){
        for(int i = taken; i < int(newpoles.size()); i++){
            if( newpoles[i] == poles[j] ) {
                prevpole[j] = i;
                taken++;
                break;
            }
        }
    }

    // on fully removing a knot the knot geometry changes
    std::vector<double> knots = bsp->getKnots();
    std::vector<double> newknots = bspline->getKnots();
    std::vector<int> prevknot(bsp->countKnots());

    for(int i = 0; i < int(knots.size()); i++)
        prevknot[i] = -1;

    taken = 0;
    for(int j = 0; j < int(knots.size()); j++){
        for(int i = taken; i < int(newknots.size()); i++){
            if( newknots[i] == knots[j] ) {
                prevknot[j] = i;
                taken++;
                break;
            }
        }
    }

    const std::vector< Sketcher::Constraint * > &cvals = Constraints.getValues();

    std::vector< Constraint * > newcVals(0);

    // modify pole constraints
    for (std::vector< Sketcher::Constraint * >::const_iterator it= cvals.begin(); it != cvals.end(); ++it) {
        if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
        {
            if((*it)->AlignmentType == Sketcher::BSplineControlPoint) {
                if (prevpole[(*it)->InternalAlignmentIndex]!=-1) {
                    assert(prevpole[(*it)->InternalAlignmentIndex] < bspline->countPoles());
                    Constraint * newConstr = (*it)->clone();
                    newConstr->InternalAlignmentIndex = prevpole[(*it)->InternalAlignmentIndex];
                    newcVals.push_back(newConstr);
                }
                else { // it is an internal alignment geometry that is no longer valid => delete it and the pole circle
                    delGeoId.push_back((*it)->First);
                }
            }
            else if((*it)->AlignmentType == Sketcher::BSplineKnotPoint) {
                if (prevknot[(*it)->InternalAlignmentIndex]!=-1) {
                    assert(prevknot[(*it)->InternalAlignmentIndex] < bspline->countKnots());
                    Constraint * newConstr = (*it)->clone();
                    newConstr->InternalAlignmentIndex = prevknot[(*it)->InternalAlignmentIndex];
                    newcVals.push_back(newConstr);
                }
                else { // it is an internal alignment geometry that is no longer valid => delete it and the knot point
                    delGeoId.push_back((*it)->First);
                }
            }
            else { // it is a bspline geometry, but not a controlpoint or knot
                newcVals.push_back(*it);
            }
        }
        else {
            newcVals.push_back(*it);
        }
    }

    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);

    newVals[GeoId] = bspline;
    bspline->Id = geo->Id;
    
    Geometry.setValues(newVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    this->Constraints.setValues(newcVals);

    std::sort (delGeoId.begin(), delGeoId.end());

    if (delGeoId.size()>0) {
        for (std::vector<int>::reverse_iterator it=delGeoId.rbegin(); it!=delGeoId.rend(); ++it) {
            delGeometry(*it,false);
        }
    }

    // * DOCUMENTING OCC ISSUE OCC < 6.9.0
    // https://forum.freecadweb.org/viewtopic.php?f=10&t=9364&start=330#p162528
    //
    // A segmentation fault is generated:
    //Program received signal SIGSEGV, Segmentation fault.
    //#0 /lib/x86_64-linux-gnu/libc.so.6(+0x36cb0) [0x7f4b933bbcb0]
    //#1  0x7f4b0300ea14 in BSplCLib::BuildCache(double, double, bool, int, TColStd_Array1OfReal const&, TColgp_Array1OfPnt const&, TColStd_Array1OfReal const&, TColgp_Array1OfPnt&, TColStd_Array1OfReal&) from /usr/lib/x86_64-linux-gnu/libTKMath.so.10+0x484
    //#2  0x7f4b033f9582 in Geom_BSplineCurve::ValidateCache(double) from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0x202
    //#3  0x7f4b033f2a7e in Geom_BSplineCurve::D0(double, gp_Pnt&) const from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0xde
    //#4  0x7f4b033de1b5 in Geom_Curve::Value(double) const from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0x25
    //#5  0x7f4b03423d73 in GeomLProp_CurveTool::Value(Handle(Geom_Curve) const&, double, gp_Pnt&) from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0x13
    //#6  0x7f4b03427175 in GeomLProp_CLProps::SetParameter(double) from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0x75
    //#7  0x7f4b0342727d in GeomLProp_CLProps::GeomLProp_CLProps(Handle(Geom_Curve) const&, double, int, double) from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0xcd
    //#8  0x7f4b11924b53 in Part::GeomCurve::pointAtParameter(double) const from /home/abdullah/github/freecad-build/Mod/Part/Part.so+0xa7



    return true;
}

int SketchObject::carbonCopy(App::DocumentObject * pObj, bool construction)
{
    // so far only externals to the support of the sketch and datum features
    bool xinv = false, yinv = false;

    if (!isCarbonCopyAllowed(pObj->getDocument(), pObj, xinv, yinv))
        return -1;

    SketchObject * psObj = static_cast<SketchObject *>(pObj);

    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    const std::vector< Sketcher::Constraint * > &cvals = Constraints.getValues();

    std::vector< Part::Geometry * > newVals(vals);

    std::vector< Constraint * > newcVals(cvals);

    int nextgeoid = vals.size();

    int nextextgeoid = getExternalGeometryCount();

    int nextcid = cvals.size();

    const std::vector< Part::Geometry * > &svals = psObj->getInternalGeometry();

    const std::vector< Sketcher::Constraint * > &scvals = psObj->Constraints.getValues();

    if(psObj->ExternalGeometry.getSize()>0) {
        std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
        std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();

        std::vector<DocumentObject*> sObjects     = psObj->ExternalGeometry.getValues();
        std::vector<std::string>     sSubElements = psObj->ExternalGeometry.getSubValues();

        if (Objects.size() != SubElements.size() || sObjects.size() != sSubElements.size()) {
            assert(0 /*counts of objects and subelements in external geometry links do not match*/);
            Base::Console().Error("Internal error: counts of objects and subelements in external geometry links do not match\n");
            return -1;
        }

        int si=0;
        for (auto & sobj : sObjects) {
            int i=0;
            for (auto & obj : Objects){
                if (obj == sobj && SubElements[i] == sSubElements[si]){
                    Base::Console().Error("Link to %s already exists in this sketch. Delete the link and try again\n",sSubElements[si].c_str());
                    return -1;
                }

                i++;
            }

            Objects.push_back(sobj);
            SubElements.push_back(sSubElements[si]);

            si++;
        }

        ExternalGeometry.setValues(Objects,SubElements);
        rebuildExternalGeometry();
        solverNeedsUpdate=true;
    }

    for (std::vector<Part::Geometry *>::const_iterator it=svals.begin(); it != svals.end(); ++it){
        Part::Geometry *geoNew = (*it)->copy();
        GEN_ID(geoNew);
        if(construction) {
            geoNew->Construction = true;
        }
        newVals.push_back(geoNew);
    }

    for (std::vector< Sketcher::Constraint * >::const_iterator it= scvals.begin(); it != scvals.end(); ++it) {
        Sketcher::Constraint *newConstr = (*it)->copy();
        if( (*it)->First>=0 )
            newConstr->First += nextgeoid;
        if( (*it)->Second>=0 )
            newConstr->Second += nextgeoid;
        if( (*it)->Third>=0 )
            newConstr->Third += nextgeoid;

        if( (*it)->First<-2 && (*it)->First != Constraint::GeoUndef )
            newConstr->First -= (nextextgeoid-2);
        if( (*it)->Second<-2 && (*it)->Second != Constraint::GeoUndef)
            newConstr->Second -= (nextextgeoid-2);
        if( (*it)->Third<-2 && (*it)->Third != Constraint::GeoUndef)
            newConstr->Third -= (nextextgeoid-2);

        newcVals.push_back(newConstr);
    }

    Geometry.setValues(newVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();

    this->Constraints.setValues(newcVals);

    int sourceid = 0;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= scvals.begin(); it != scvals.end(); ++it,nextcid++,sourceid++) {

        if ((*it)->Type == Sketcher::Distance ||
            (*it)->Type == Sketcher::Radius ||
            (*it)->Type == Sketcher::Diameter ||
            (*it)->Type == Sketcher::Angle ||
            (*it)->Type == Sketcher::SnellsLaw) {
            // then we link its value to the parent
            // (there is a plausible alternative for a slightly different use case to copy the expression of the parent if one is existing)
            if ((*it)->isDriving) {
                App::ObjectIdentifier spath = psObj->Constraints.createPath(sourceid);
                /*
                 *           App::PropertyExpressionEngine::ExpressionInfo expr_info = psObj->getExpression(path);
                 *
                 *           if (expr_info.expression)*/
                //App::Expression * expr = parse(this, const std::string& buffer);

                boost::shared_ptr<App::Expression> expr(App::Expression::parse(this, spath.getDocumentObjectName().getString() +std::string(1,'.') + spath.toString()));
                setExpression(Constraints.createPath(nextcid), expr);


            }

        }
        else if ((*it)->Type == Sketcher::DistanceX) {
            // then we link its value to the parent
            // (there is a plausible alternative for a slightly different use case to copy the expression of the parent if one is existing)
            if ((*it)->isDriving) {
                App::ObjectIdentifier spath = psObj->Constraints.createPath(sourceid);

                if(xinv) {
                    boost::shared_ptr<App::Expression> expr(App::Expression::parse(this, std::string(1,'-') + spath.getDocumentObjectName().getString() +std::string(1,'.') + spath.toString()));
                    setExpression(Constraints.createPath(nextcid), expr);
                }
                else {
                    boost::shared_ptr<App::Expression> expr(App::Expression::parse(this, spath.getDocumentObjectName().getString() +std::string(1,'.') + spath.toString()));

                    setExpression(Constraints.createPath(nextcid), expr);
                }
            }

        }
        else if ((*it)->Type == Sketcher::DistanceY ) {
            // then we link its value to the parent
            // (there is a plausible alternative for a slightly different use case to copy the expression of the parent if one is existing)
            if ((*it)->isDriving) {
                App::ObjectIdentifier spath = psObj->Constraints.createPath(sourceid);

                if(yinv) {
                    boost::shared_ptr<App::Expression> expr(App::Expression::parse(this, std::string(1,'-') + spath.getDocumentObjectName().getString() +std::string(1,'.') + spath.toString()));
                    setExpression(Constraints.createPath(nextcid), expr);
                }
                else {
                    boost::shared_ptr<App::Expression> expr(App::Expression::parse(this, spath.getDocumentObjectName().getString() +std::string(1,'.') + spath.toString()));
                    setExpression(Constraints.createPath(nextcid), expr);
                }
            }

        }
    }

    return svals.size();
}

int SketchObject::addExternal(App::DocumentObject *Obj, const char* SubName, bool defining)
{
    // so far only externals to the support of the sketch and datum features
    if (!isExternalAllowed(Obj->getDocument(), Obj))
       return -1;

    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();

    if (Objects.size() != SubElements.size()) {
        assert(0 /*counts of objects and subelements in external geometry links do not match*/);
        Base::Console().Error("Internal error: counts of objects and subelements in external geometry links do not match\n");
        return -1;
    }

    for (size_t i = 0  ;  i < Objects.size()  ;  ++i){
        if (Objects[i] == Obj   &&   std::string(SubName) == SubElements[i]){
            Base::Console().Error("Link to %s already exists in this sketch.\n",SubName);
            return -1;
        }
    }

    // add the new ones
    Objects.push_back(Obj);
    SubElements.push_back(std::string(SubName));

    // set the Link list.
    ExternalGeometry.setValues(Objects,SubElements);
    rebuildExternalGeometry(defining);
    if(ExternalGeometry.getSize() == (int)Objects.size())
        return ExternalGeometry.getSize()-1;
    return -1;
}

int SketchObject::delExternal(int ExtGeoId)
{
    int GeoId = GeoEnum::RefExt - ExtGeoId;
    if(GeoId > GeoEnum::RefExt || -GeoId-1 >= ExternalGeo.getSize())
        return -1;

    auto geo = getGeometry(GeoId);
    if(!geo)
        return -1;

    std::set<long> geoIds;
    geoIds.insert(geo->Id);
    if(geo->Ref.size()) {
        auto &refs = externalGeoRefMap[geo->Ref];
        geoIds.insert(refs.begin(),refs.end());
    }
    delExternalPrivate(geoIds,true);
    return 0;
}

void SketchObject::delExternalPrivate(const std::set<long> &ids, bool removeRef) {

    std::set<std::string> refs;
    std::set<int> geoIds;

    for(auto id : ids) {
        auto it = externalGeoMap.find(id);
        if(it == externalGeoMap.end())
            continue;

        auto geo = ExternalGeo[it->second];
        if(removeRef && geo->Ref.size())
            refs.insert(geo->Ref);
        geoIds.insert(-it->second-1);
    }

    if(geoIds.empty())
        return;

    std::vector< Constraint * > newConstraints;
    for(auto cstr : Constraints.getValues()) {
        if(!geoIds.count(cstr->First) &&
           (cstr->Second==Constraint::GeoUndef || !geoIds.count(cstr->Second)) &&
           (cstr->Third==Constraint::GeoUndef || !geoIds.count(cstr->Third)))
        {
            cstr = cstr->clone();
            int offset = 0;
            for(auto GeoId : geoIds) {
                GeoId += offset++;
                bool done = true;
                if (cstr->First < GeoId && cstr->First != Constraint::GeoUndef) {
                    cstr->First += 1;
                    done = false;
                }
                if (cstr->Second < GeoId && cstr->Second != Constraint::GeoUndef) {
                    cstr->Second += 1;
                    done = false;
                }
                if (cstr->Third < GeoId && cstr->Third != Constraint::GeoUndef) {
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

    solverNeedsUpdate=true;
    Constraints.setValues(std::move(newConstraints));
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
}

int SketchObject::delAllExternal()
{
    int count = 0; // the remaining count of the detached external geometry
    std::map<int,int> indexMap; // the index map of the remain external geometry
    std::vector<Part::Geometry*> geos; // the remaining external geometry
    for(int i=0;i<ExternalGeo.getSize();++i) {
        auto geo = ExternalGeo[i];
        if(geo->Ref.empty())
            indexMap[i] = count++;
        geos.push_back(geo);
    }

    const std::vector< Constraint * > &constraints = Constraints.getValues();
    std::vector< Constraint * > newConstraints(0);
    for(auto cstr : constraints) {
        std::unique_ptr<Constraint> clone;
        if(cstr->First <= GeoEnum::RefExt) {
           auto it = indexMap.find(cstr->First);
           if(it==indexMap.end())
               continue;
            if(!clone)
                clone.reset(cstr->clone());
            clone->First = it->second;
        }
        if(cstr->Second <= GeoEnum::RefExt && cstr->Second != Constraint::GeoUndef) {
           auto it = indexMap.find(cstr->Second);
           if(it==indexMap.end())
               continue;
            if(!clone)
                clone.reset(cstr->clone());
            clone->Second = it->second;
        }
        if(cstr->Third <= GeoEnum::RefExt && cstr->Third != Constraint::GeoUndef) {
           auto it = indexMap.find(cstr->Third);
           if(it==indexMap.end())
               continue;
            if(!clone)
                clone.reset(cstr->clone());
            clone->Third = it->second;
        }
        if(!clone)
            clone.reset(cstr->clone());

        newConstraints.push_back(clone.release());
    }

    ExternalGeometry.setValue(0);
    ExternalGeo.setValues(std::move(geos));
    solverNeedsUpdate=true;
    Constraints.setValues(std::move(newConstraints));
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    return 0;
}

int SketchObject::delConstraintsToExternal()
{
    const std::vector< Constraint * > &constraints = Constraints.getValuesForce();
    std::vector< Constraint * > newConstraints(0);
    int GeoId = GeoEnum::RefExt, NullId = Constraint::GeoUndef;
    for(auto cstr : constraints) {
        if(cstr->First <= GeoId) {
            auto geo = getGeometry(cstr->First);
            if(geo && geo->Ref.size())
                continue;
        } else if(cstr->Second <= GeoId && cstr->Second != NullId) {
            auto geo = getGeometry(cstr->Second);
            if(geo && geo->Ref.size())
                continue;
        } else if(cstr->Third <= GeoId && cstr->Third != NullId) {
            auto geo = getGeometry(cstr->Third);
            if(geo && geo->Ref.size())
                continue;
        }
        newConstraints.push_back(cstr);
    }

    Constraints.setValues(std::move(newConstraints));
    Constraints.acceptGeometry(getCompleteGeometry());

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

int SketchObject::attachExternal(
        const std::vector<int> &geoIds, App::DocumentObject *Obj, const char* SubName) 
{
    if (!isExternalAllowed(Obj->getDocument(), Obj))
       return -1;

    std::set<std::string> detached;
    std::set<int> idSet;
    for(int geoId : geoIds) {
        if(geoId > GeoEnum::RefExt || -geoId-1 >= ExternalGeo.getSize())
            continue;
        auto geo = getGeometry(geoId);
        if(!geo)
            continue;
        if(geo->Ref.size())
            detached.insert(geo->Ref);
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
            FC_ERR("Duplicdate external element reference in " << getFullName() << ": " << key);
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
        geo->Ref = ref;
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
    if(!geo->Ref.size())
       return {GeoId}; 
    auto iter = externalGeoRefMap.find(geo->Ref);
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
        if(!geo || !geo->testFlag(Part::Geometry::Frozen))
            continue;
        for(int gid : getRelatedGeometry(geoId))
            idSet.insert(gid);
    }
    for(int geoId : idSet) {
        if(geoId <= GeoEnum::RefExt && -geoId-1 < ExternalGeo.getSize()) {
            auto &geo = geos[-geoId-1];
            geo = geo->clone();
            geo->setFlag(Part::Geometry::Sync);
            touched = true;
        }
    }
    if(touched) 
        ExternalGeo.setValues(std::move(geos));
    return 0;
}

const Part::Geometry* SketchObject::getGeometry(int GeoId) const
{
    if (GeoId >= 0) {
        const std::vector<Part::Geometry *> &geomlist = getInternalGeometry();
        if (GeoId < int(geomlist.size()))
            return geomlist[GeoId];
    }
    else if (GeoId < 0 && -GeoId-1 < ExternalGeo.getSize())
        return ExternalGeo[-GeoId-1];

    return 0;
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

// Auxiliary method
Part::Geometry* projectLine(const BRepAdaptor_Curve& curve, const Handle(Geom_Plane)& gPlane, const Base::Placement& invPlm)
{
    double first = curve.FirstParameter();
    bool infinite = false;
    if (fabs(first) > 1E99) {
        // TODO: What is OCE's definition of Infinite?
        // TODO: The clean way to do this is to handle a new sketch geometry Geom::Line
        // but its a lot of work to implement...
        first = -10000;
        //infinite = true;
    }
    double last = curve.LastParameter();
    if (fabs(last) > 1E99) {
        last = +10000;
        //infinite = true;
    }

    gp_Pnt P1 = curve.Value(first);
    gp_Pnt P2 = curve.Value(last);

    GeomAPI_ProjectPointOnSurf proj1(P1,gPlane);
    P1 = proj1.NearestPoint();
    GeomAPI_ProjectPointOnSurf proj2(P2,gPlane);
    P2 = proj2.NearestPoint();

    Base::Vector3d p1(P1.X(),P1.Y(),P1.Z());
    Base::Vector3d p2(P2.X(),P2.Y(),P2.Z());
    invPlm.multVec(p1,p1);
    invPlm.multVec(p2,p2);

    if (Base::Distance(p1,p2) < Precision::Confusion()) {
        Base::Vector3d p = (p1 + p2) / 2;
        Part::GeomPoint* point = new Part::GeomPoint(p);
        point->Construction = true;
        return point;
    }
    else if (!infinite) {
        Part::GeomLineSegment* line = new Part::GeomLineSegment();
        line->setPoints(p1,p2);
        line->Construction = true;
        return line;
    } else {
        Part::GeomLine* line = new Part::GeomLine();
        line->setLine(p1, p2 - p1);
        line->Construction = true;
        return line;
    }
}

bool SketchObject::evaluateSupport(void)
{
    // returns false if the shape if broken, null or non-planar
    Part::Feature *part = static_cast<Part::Feature*>(Support.getValue());
    if (!part || !part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return false;
    return true;
}

void SketchObject::rebuildExternalGeometry(bool defining)
{
    // get the actual lists of the externals
    auto Objects     = ExternalGeometry.getValues();
    auto SubElements = ExternalGeometry.getSubValues();
    assert(externalGeoRef.size() == Objects.size());
    auto keys = externalGeoRef;

    auto geoms = ExternalGeo.getValues();
    // re-check for any missing geometry element. The code here has a side
    // effect that the linked external geometry will continue to work even if
    // ExternalGeometry is wiped out.
    for(auto &geo : geoms) {
        if(geo->Ref.size() && geo->testFlag(Part::Geometry::Missing)) {
            auto pos = geo->Ref.find('.');
            if(pos == std::string::npos)
                continue;
            std::string objName = geo->Ref.substr(0,pos);
            auto obj = getDocument()->getObject(objName.c_str());
            if(!obj)
                continue;
            std::pair<std::string,std::string> elementName;
            App::GeoFeature::resolveElement(obj,geo->Ref.c_str()+pos+1,elementName);
            if(elementName.second.size() 
                    && !App::GeoFeature::hasMissingElement(elementName.second.c_str())) 
            {
                Objects.push_back(obj);
                SubElements.push_back(elementName.second);
                keys.push_back(geo->Ref);
            }
        }
    }
    
    Base::Placement Plm = Placement.getValue();
    Base::Vector3d Pos = Plm.getPosition();
    Base::Rotation Rot = Plm.getRotation();
    Base::Vector3d dN(0,0,1);
    Rot.multVec(dN,dN);
    Base::Vector3d dX(1,0,0);
    Rot.multVec(dX,dX);

    Base::Placement invPlm = Plm.inverse();
    Base::Matrix4D invMat = invPlm.toMatrix();
    gp_Trsf mov;
    mov.SetValues(invMat[0][0],invMat[0][1],invMat[0][2],invMat[0][3],
                  invMat[1][0],invMat[1][1],invMat[1][2],invMat[1][3],
                  invMat[2][0],invMat[2][1],invMat[2][2],invMat[2][3]
#if OCC_VERSION_HEX < 0x060800
                  , 0.00001, 0.00001
#endif
                  ); //precision was removed in OCCT CR0025194

    gp_Ax3 sketchAx3(gp_Pnt(Pos.x,Pos.y,Pos.z),
                     gp_Dir(dN.x,dN.y,dN.z),
                     gp_Dir(dX.x,dX.y,dX.z));
    gp_Pln sketchPlane(sketchAx3);

    Handle(Geom_Plane) gPlane = new Geom_Plane(sketchPlane);
    BRepBuilderAPI_MakeFace mkFace(sketchPlane);
    TopoDS_Shape aProjFace = mkFace.Shape();

    std::set<std::string> refSet;
    // We use a vector here to keep the order (roughly) the same as ExternalGeometry
    std::vector<std::vector<std::unique_ptr<Part::Geometry> > > newGeos;
    newGeos.reserve(Objects.size());

    for (int i=0; i < int(Objects.size()); i++) {
        const App::DocumentObject *Obj=Objects[i];
        const std::string &SubElement=SubElements[i];        
        const std::string &key = keys[i];

        // Skip frozen geometries
        bool frozen = false;
        bool sync = false;
        for(auto id : externalGeoRefMap[key]) {
            auto it = externalGeoMap.find(id);
            if(it != externalGeoMap.end()) {
                auto geo = ExternalGeo[it->second];
                if(geo->testFlag(Part::Geometry::Frozen))
                    frozen = true;
                if(geo->testFlag(Part::Geometry::Sync)) 
                    sync = true;
            }
        }
        if(frozen && !sync) {
            refSet.insert(std::move(key));
            continue;
        }

        if(!Obj || !Obj->getNameInDocument())
            continue;

        std::vector<std::unique_ptr<Part::Geometry> > geos;

        try {
            TopoDS_Shape refSubShape;
            if (Obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
                const Part::Datum* datum = static_cast<const Part::Datum*>(Obj);
                refSubShape = datum->getShape();
            } else  if (Obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                const App::Plane* pl = static_cast<const App::Plane*>(Obj);
                Base::Placement plm = pl->Placement.getValue();
                Base::Vector3d base = plm.getPosition();
                Base::Rotation rot = plm.getRotation();
                Base::Vector3d normal(0,0,1);
                rot.multVec(normal, normal);
                gp_Pln plane(gp_Pnt(base.x,base.y,base.z), gp_Dir(normal.x, normal.y, normal.z));
                BRepBuilderAPI_MakeFace fBuilder(plane);
                if (fBuilder.IsDone()) {
                    TopoDS_Face f = TopoDS::Face(fBuilder.Shape());
                    refSubShape = f;
                }
            } else {
                refSubShape = Part::Feature::getShape(Obj,SubElement.c_str(),true);
            }

            if(refSubShape.IsNull()) {
                FC_WARN("Null shape from geometry reference in " << getFullName() << ": " << key);
                continue;
            }

            switch (refSubShape.ShapeType())
            {
            case TopAbs_FACE: {
                const TopoDS_Face& face = TopoDS::Face(refSubShape);
                BRepAdaptor_Surface surface(face);
                if (surface.GetType() == GeomAbs_Plane) {
                    // Check that the plane is perpendicular to the sketch plane
                    Geom_Plane plane = surface.Plane();
                    gp_Dir dnormal = plane.Axis().Direction();
                    gp_Dir snormal = sketchPlane.Axis().Direction();
                    if (fabs(dnormal.Angle(snormal) - M_PI_2) < Precision::Confusion()) {
                        // Get vector that is normal to both sketch plane normal and plane normal. This is the line's direction
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

                    } else {
                        FC_ERR("Selected external reference plane must be normal to sketch plane in "
                                << getFullName() << ": " << key);
                        continue;
                    }
                } else {
                    FC_ERR("Non-planar faces are not yet supported in sketch "
                                << getFullName() << ": " << key);
                    continue;
                }
                break;
            }
            case TopAbs_EDGE: {
                const TopoDS_Edge& edge = TopoDS::Edge(refSubShape);
                BRepAdaptor_Curve curve(edge);
                if (curve.GetType() == GeomAbs_Line) {
                    geos.emplace_back(projectLine(curve, gPlane, invPlm));
                }
                else if (curve.GetType() == GeomAbs_Circle) {
                    gp_Dir vec1 = sketchPlane.Axis().Direction();
                    gp_Dir vec2 = curve.Circle().Axis().Direction();
                    if (vec1.IsParallel(vec2, Precision::Confusion())) {
                        gp_Circ circle = curve.Circle();
                        gp_Pnt cnt = circle.Location();
                        gp_Pnt beg = curve.Value(curve.FirstParameter());
                        gp_Pnt end = curve.Value(curve.LastParameter());

                        GeomAPI_ProjectPointOnSurf proj(cnt,gPlane);
                        cnt = proj.NearestPoint();
                        circle.SetLocation(cnt);
                        cnt.Transform(mov);
                        circle.Transform(mov);

                        if (beg.SquareDistance(end) < Precision::Confusion()) {
                            Part::GeomCircle* gCircle = new Part::GeomCircle();
                            gCircle->setRadius(circle.Radius());
                            gCircle->setCenter(Base::Vector3d(cnt.X(),cnt.Y(),cnt.Z()));

                            gCircle->Construction = true;
                            geos.emplace_back(gCircle);
                        }
                        else {
                            Part::GeomArcOfCircle* gArc = new Part::GeomArcOfCircle();
                            Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
                            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(hCircle, curve.FirstParameter(),
                                                                                    curve.LastParameter());
                            gArc->setHandle(tCurve);
                            gArc->Construction = true;
                            geos.emplace_back(gArc);
                        }
                    }
                    else {
                        // creates an ellipse
                        FC_ERR("Not yet supported geometry in sketch " << getFullName() << ": " << key);
                        continue;
                    }
                }
                else {
                    BRepOffsetAPI_NormalProjection mkProj(aProjFace);
                    mkProj.Add(edge);
                    mkProj.Build();
                    const TopoDS_Shape& projShape = mkProj.Projection();
                    if (projShape.IsNull()) {
                        FC_ERR("Invalid geometry in sketch " << getFullName() << ": " << key);
                        continue;
                    }
                    TopExp_Explorer xp;
                    for (xp.Init(projShape, TopAbs_EDGE); xp.More(); xp.Next()) {
                        TopoDS_Edge projEdge = TopoDS::Edge(xp.Current());
                        TopLoc_Location loc(mov);
                        projEdge.Location(loc);
                        BRepAdaptor_Curve projCurve(projEdge);
                        if (projCurve.GetType() == GeomAbs_Line) {
                            gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
                            gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());
                            Base::Vector3d p1(P1.X(),P1.Y(),P1.Z());
                            Base::Vector3d p2(P2.X(),P2.Y(),P2.Z());

                            if (Base::Distance(p1,p2) < Precision::Confusion()) {
                                Base::Vector3d p = (p1 + p2) / 2;
                                Part::GeomPoint* point = new Part::GeomPoint(p);
                                point->Construction = true;
                                geos.emplace_back(point);
                            }
                            else {
                                Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                line->setPoints(p1,p2);
                                line->Construction = true;
                                geos.emplace_back(line);
                            }
                        }
                        else if (projCurve.GetType() == GeomAbs_Circle) {
                            gp_Circ c = projCurve.Circle();
                            gp_Pnt p = c.Location();
                            gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
                            gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

                            if (P1.SquareDistance(P2) < Precision::Confusion()) {
                                Part::GeomCircle* circle = new Part::GeomCircle();
                                circle->setRadius(c.Radius());
                                circle->setCenter(Base::Vector3d(p.X(),p.Y(),p.Z()));

                                circle->Construction = true;
                                geos.emplace_back(circle);
                            }
                            else {
                                Part::GeomArcOfCircle* arc = new Part::GeomArcOfCircle();
                                Handle(Geom_Curve) curve = new Geom_Circle(c);
                                Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                        projCurve.LastParameter());
                                arc->setHandle(tCurve);
                                arc->Construction = true;
                                geos.emplace_back(arc);
                            }
                        } else if (projCurve.GetType() == GeomAbs_BSplineCurve) {
                            // Unfortunately, a normal projection of a circle can also give a Bspline
                            // Split the spline into arcs
                            GeomConvert_BSplineCurveKnotSplitting bSplineSplitter(projCurve.BSpline(), 2);
                            //int s = bSplineSplitter.NbSplits();
                            if ((curve.GetType() == GeomAbs_Circle) && (bSplineSplitter.NbSplits() == 2)) {
                                // Result of projection is actually a circle...
                                TColStd_Array1OfInteger splits(1, 2);
                                bSplineSplitter.Splitting(splits);
                                gp_Pnt p1 = projCurve.Value(splits(1));
                                gp_Pnt p2 = projCurve.Value(splits(2));
                                gp_Pnt p3 = projCurve.Value(0.5 * (splits(1) + splits(2)));
                                GC_MakeCircle circleMaker(p1, p2, p3);
                                Handle(Geom_Circle) circ = circleMaker.Value();
                                Part::GeomCircle* circle = new Part::GeomCircle();
                                circle->setRadius(circ->Radius());
                                gp_Pnt center = circ->Axis().Location();
                                circle->setCenter(Base::Vector3d(center.X(), center.Y(), center.Z()));

                                circle->Construction = true;
                                geos.emplace_back(circle);
                            } else {
                                Part::GeomBSplineCurve* bspline = new Part::GeomBSplineCurve(projCurve.BSpline());
                                bspline->Construction = true;
                                geos.emplace_back(bspline);
                            }
                        } else if (projCurve.GetType() == GeomAbs_Hyperbola) {
                            gp_Hypr e = projCurve.Hyperbola();
                            gp_Pnt p = e.Location();
                            gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
                            gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

                            gp_Dir normal = e.Axis().Direction();
                            gp_Dir xdir = e.XAxis().Direction();
                            gp_Ax2 xdirref(p, normal);

                            if (P1.SquareDistance(P2) < Precision::Confusion()) {
                                Part::GeomHyperbola* hyperbola = new Part::GeomHyperbola();
                                hyperbola->setMajorRadius(e.MajorRadius());
                                hyperbola->setMinorRadius(e.MinorRadius());
                                hyperbola->setCenter(Base::Vector3d(p.X(),p.Y(),p.Z()));
                                hyperbola->setAngleXU(-xdir.AngleWithRef(xdirref.XDirection(),normal));
                                hyperbola->Construction = true;
                                geos.emplace_back(hyperbola);
                            }
                            else {
                                Part::GeomArcOfHyperbola* aoh = new Part::GeomArcOfHyperbola();
                                Handle(Geom_Curve) curve = new Geom_Hyperbola(e);
                                Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                        projCurve.LastParameter());
                                aoh->setHandle(tCurve);
                                aoh->Construction = true;
                                geos.emplace_back(aoh);
                            }
                        } else if (projCurve.GetType() == GeomAbs_Parabola) {
                            gp_Parab e = projCurve.Parabola();
                            gp_Pnt p = e.Location();
                            gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
                            gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

                            gp_Dir normal = e.Axis().Direction();
                            gp_Dir xdir = e.XAxis().Direction();
                            gp_Ax2 xdirref(p, normal);

                            if (P1.SquareDistance(P2) < Precision::Confusion()) {
                                Part::GeomParabola* parabola = new Part::GeomParabola();
                                parabola->setFocal(e.Focal());
                                parabola->setCenter(Base::Vector3d(p.X(),p.Y(),p.Z()));
                                parabola->setAngleXU(-xdir.AngleWithRef(xdirref.XDirection(),normal));
                                parabola->Construction = true;
                                geos.emplace_back(parabola);
                            }
                            else {
                                Part::GeomArcOfParabola* aop = new Part::GeomArcOfParabola();
                                Handle(Geom_Curve) curve = new Geom_Parabola(e);
                                Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                        projCurve.LastParameter());
                                aop->setHandle(tCurve);
                                aop->Construction = true;
                                geos.emplace_back(aop);
                            }
                        }
                        else if (projCurve.GetType() == GeomAbs_Ellipse) {
                            gp_Elips e = projCurve.Ellipse();
                            gp_Pnt p = e.Location();
                            gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
                            gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

                            //gp_Dir normal = e.Axis().Direction();
                            gp_Dir normal = gp_Dir(0,0,1);
                            gp_Ax2 xdirref(p, normal);

                            if (P1.SquareDistance(P2) < Precision::Confusion()) {
                                Part::GeomEllipse* ellipse = new Part::GeomEllipse();
                                Handle(Geom_Ellipse) curve = new Geom_Ellipse(e);
                                ellipse->setHandle(curve);
                                ellipse->Construction = true;
                                geos.emplace_back(ellipse);
                            }
                            else {
                                Part::GeomArcOfEllipse* aoe = new Part::GeomArcOfEllipse();
                                Handle(Geom_Curve) curve = new Geom_Ellipse(e);
                                Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                        projCurve.LastParameter());
                                aoe->setHandle(tCurve);
                                aoe->Construction = true;
                                geos.emplace_back(aoe);
                            }
                        }
                        else {
                            FC_ERR("Not supported projected geometry in sketch " << getFullName() << ": " << key);
                            geos.clear();
                            break;
                        }
                    }
                }
                break;
            }
            case TopAbs_VERTEX: {
                gp_Pnt P = BRep_Tool::Pnt(TopoDS::Vertex(refSubShape));
                GeomAPI_ProjectPointOnSurf proj(P,gPlane);
                P = proj.NearestPoint();
                Base::Vector3d p(P.X(),P.Y(),P.Z());
                invPlm.multVec(p,p);

                Part::GeomPoint* point = new Part::GeomPoint(p);
                point->Construction = true;
                geos.emplace_back(point);
                break;
            }
            default:
                FC_ERR("Unknown type of geometry in " << getFullName() << ": " << key);
                break;
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
        if(geos.empty())
            continue;

        if(!refSet.emplace(key).second) {
            FC_WARN("Duplicated external reference in " << getFullName() << ": " << key);
            continue;
        }
        if(defining && i+1==(int)Objects.size()) {
            for(auto &geo : geos)
                geo->setFlag(Part::Geometry::Defining);
        }
        for(auto &geo : geos)
            geo->Ref = key;
        newGeos.push_back(std::move(geos));
    }

    // allocate unique geometry id
    for(auto &geos : newGeos) {
        auto &refs = externalGeoRefMap[geos.front()->Ref];
        while(refs.size() < geos.size()) 
            refs.push_back(++geoLastId);

        // In case a projection reduces output geometries, delete them
        std::set<long> geoIds;
        geoIds.insert(refs.begin()+geos.size(),refs.end());
        delExternalPrivate(geoIds,false);

        // Sync id and ref of the new geometries
        int i = 0;
        for(auto &geo : geos)
            geo->Id = refs[i++];
    }

    // now update the geometries
    for(auto &geos : newGeos) {
        for(auto &geo : geos) {
            auto it = externalGeoMap.find(geo->Id);
            if(it == externalGeoMap.end()) {
                // This is a new geometries.
                geoms.push_back(geo.release());
                continue;
            }
            // This is an existing geometry. Update it while keeping the old flags
            geo->Flags = geoms[it->second]->Flags;
            geoms[it->second] = geo.release();
        }
    }

    // Check for any missing references
    bool hasError = false;
    for(auto geo : geoms) {
        geo->setFlag(Part::Geometry::Sync,false);
        if(geo->Ref.empty())
            continue;
        if(!refSet.count(geo->Ref)) {
            FC_ERR( "External geometry " << getFullName() << ".e" << geo->Id
                    << " missing reference: " << geo->Ref);
            hasError = true;
            geo->setFlag(Part::Geometry::Missing);
        } else {
            geo->setFlag(Part::Geometry::Missing,false);
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

    if(hasError) 
        throw Base::RuntimeError("Missing external geometry reference");
}

void SketchObject::fixExternalGeometry(const std::vector<int> &geoIds) {
    std::set<int> idSet(geoIds.begin(),geoIds.end());
    auto geos = ExternalGeo.getValues();
    auto objs = ExternalGeometry.getValues();
    auto subs = ExternalGeometry.getSubValues();
    bool touched = false;
    for(int i=2;i<(int)geos.size();++i) {
        auto &geo = geos[i];
        int GeoId = -i-1;
        if(geo->Ref.empty() 
                || !geo->testFlag(Part::Geometry::Missing) 
                || (idSet.size() && !idSet.count(GeoId)))
            continue;
        auto pos = geo->Ref.find('.');
        if(pos == std::string::npos) {
            FC_ERR("Invalid geometry reference " << geo->Ref);
            continue;
        }
        std::string objName = geo->Ref.substr(0,pos);
        auto obj = getDocument()->getObject(objName.c_str());
        if(!obj) {
            FC_ERR("Cannot find object in reference " << geo->Ref);
            continue;
        }
        
        auto elements = Part::Feature::getRelatedElements(obj,geo->Ref.c_str()+pos+1);
        if(elements.empty()) {
            FC_ERR("No related reference found for " << geo->Ref);
            continue;
        }

        geo = geo->clone();
        geo->setFlag(Part::Geometry::Missing,false);
        geo->Ref = objName + "." 
            + Data::ComplexGeoData::elementMapPrefix() + elements.front().first;
        objs.push_back(obj);
        subs.push_back(elements.front().second);
        touched = true;
    }

    if(touched) {
        ExternalGeo.setValues(geos);
        ExternalGeometry.setValues(objs,subs);
        rebuildExternalGeometry();
    }
}

std::vector<Part::Geometry*> SketchObject::getCompleteGeometry(void) const
{
    std::vector<Part::Geometry*> vals=getInternalGeometry();
    const auto &geos = getExternalGeometry();
    vals.insert(vals.end(), geos.rbegin(), geos.rend()); // in reverse order
    return vals;
}

void SketchObject::rebuildVertexIndex(void)
{
    GeoPos2VertexId.clear();
    VertexId2GeoId.resize(0);
    VertexId2PosId.resize(0);
    int imax=getHighestCurveIndex();
    int i=0;
    const std::vector< Part::Geometry * > geometry = getCompleteGeometry();
    if (geometry.size() <= 2)
        return;
    for (std::vector< Part::Geometry * >::const_iterator it = geometry.begin();
         it != geometry.end()-2; ++it, i++) {
        if (i > imax)
              i = -getExternalGeometryCount();
        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,start)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
        } else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,start)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            GeoPos2VertexId[std::make_pair(i,end)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
        } else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,mid)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,mid)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,start)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            GeoPos2VertexId[std::make_pair(i,end)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
            GeoPos2VertexId[std::make_pair(i,mid)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,start)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            GeoPos2VertexId[std::make_pair(i,end)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
            GeoPos2VertexId[std::make_pair(i,mid)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,start)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            GeoPos2VertexId[std::make_pair(i,end)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
            GeoPos2VertexId[std::make_pair(i,mid)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,start)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            GeoPos2VertexId[std::make_pair(i,end)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
            GeoPos2VertexId[std::make_pair(i,mid)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            GeoPos2VertexId[std::make_pair(i,start)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            GeoPos2VertexId[std::make_pair(i,end)] = VertexId2GeoId.size();
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
        }
    }
}

const std::vector< std::map<int, Sketcher::PointPos> > SketchObject::getCoincidenceGroups()
{
    // this function is different from that in getCoincidentPoints in that:
    // - getCoincidentPoints only considers direct coincidence (the points that are linked via a single coincidence)
    // - this function provides an array of maps of points, each map containing the points that are coincident by virtue
    //   of any number of interrelated coincidence constraints (if coincidence 1-2 and coincidence 2-3, {1,2,3} are in that set)

    const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();

    std::vector< std::map<int, Sketcher::PointPos> > coincidenttree;
    // push the constraints
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it) {
        if( (*it)->Type == Sketcher::Coincident ) {
            int firstpresentin=-1;
            int secondpresentin=-1;

            int i=0;

            for(std::vector< std::map<int, Sketcher::PointPos> >::const_iterator iti = coincidenttree.begin(); iti != coincidenttree.end(); ++iti,i++) {
                // First
                std::map<int, Sketcher::PointPos>::const_iterator filiterator;
                filiterator = (*iti).find((*it)->First);
                if( filiterator != (*iti).end()) {
                    if((*it)->FirstPos == (*filiterator).second)
                        firstpresentin = i;
                }
                // Second
                filiterator = (*iti).find((*it)->Second);
                if( filiterator != (*iti).end()) {
                    if((*it)->SecondPos == (*filiterator).second)
                        secondpresentin = i;
                }
            }

            if ( firstpresentin!=-1 && secondpresentin!=-1) {
                // we have to merge those sets into one
                coincidenttree[firstpresentin].insert(coincidenttree[secondpresentin].begin(), coincidenttree[secondpresentin].end());
                coincidenttree.erase(coincidenttree.begin()+secondpresentin);
            }
            else if ( firstpresentin==-1 && secondpresentin==-1 ) {
                // we do not have any of the values, so create a setCursor
                std::map<int, Sketcher::PointPos> tmp;
                tmp.insert(std::pair<int, Sketcher::PointPos>((*it)->First,(*it)->FirstPos));
                tmp.insert(std::pair<int, Sketcher::PointPos>((*it)->Second,(*it)->SecondPos));
                coincidenttree.push_back(tmp);
            }
            else if ( firstpresentin != -1 ) {
                // add to existing group
                coincidenttree[firstpresentin].insert(std::pair<int, Sketcher::PointPos>((*it)->Second,(*it)->SecondPos));
            }
            else { // secondpresentin != -1
                // add to existing group
                coincidenttree[secondpresentin].insert(std::pair<int, Sketcher::PointPos>((*it)->First,(*it)->FirstPos));
            }

        }
    }

    return coincidenttree;
}

void SketchObject::isCoincidentWithExternalGeometry(int GeoId, bool &start_external, bool &mid_external, bool &end_external) {

    start_external=false;
    mid_external=false;
    end_external=false;

    const std::vector< std::map<int, Sketcher::PointPos> > coincidenttree = getCoincidenceGroups();

    for(std::vector< std::map<int, Sketcher::PointPos> >::const_iterator it = coincidenttree.begin(); it != coincidenttree.end(); ++it) {

        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

        geoId1iterator = (*it).find(GeoId);

        if( geoId1iterator != (*it).end()) {
            // If First is in this set and the first key in this ordered element key is external
            if( (*it).begin()->first < 0 ) {
                if( (*geoId1iterator).second == Sketcher::start )
                    start_external=true;
                else if ( (*geoId1iterator).second == Sketcher::mid )
                    mid_external=true;
                else if ( (*geoId1iterator).second == Sketcher::end )
                    end_external=true;
            }
        }
    }
}

const std::map<int, Sketcher::PointPos> SketchObject::getAllCoincidentPoints(int GeoId, PointPos PosId) {

    const std::vector< std::map<int, Sketcher::PointPos> > coincidenttree = getCoincidenceGroups();

    for(std::vector< std::map<int, Sketcher::PointPos> >::const_iterator it = coincidenttree.begin(); it != coincidenttree.end(); ++it) {

        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

        geoId1iterator = (*it).find(GeoId);

        if( geoId1iterator != (*it).end()) {
            // If GeoId is in this set

            if ((*geoId1iterator).second == PosId) // and posId matches
                return (*it);
        }
    }

    std::map<int, Sketcher::PointPos> empty;

    return empty;
}


void SketchObject::getDirectlyCoincidentPoints(int GeoId, PointPos PosId, std::vector<int> &GeoIdList,
                                       std::vector<PointPos> &PosIdList)
{
    const std::vector<Constraint *> &constraints = this->Constraints.getValues();

    GeoIdList.clear();
    PosIdList.clear();
    GeoIdList.push_back(GeoId);
    PosIdList.push_back(PosId);
    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
         it != constraints.end(); ++it) {
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
    }
    if (GeoIdList.size() == 1) {
        GeoIdList.clear();
        PosIdList.clear();
    }
}

void SketchObject::getDirectlyCoincidentPoints(int VertexId, std::vector<int> &GeoIdList,
                                       std::vector<PointPos> &PosIdList)
{
    int GeoId;
    PointPos PosId;
    getGeoVertexIndex(VertexId, GeoId, PosId);
    getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
}

bool SketchObject::arePointsCoincident(int GeoId1, PointPos PosId1,
                                       int GeoId2, PointPos PosId2)
{
    if (GeoId1 == GeoId2 && PosId1 == PosId2)
        return true;

    const std::vector< std::map<int, Sketcher::PointPos> > coincidenttree = getCoincidenceGroups();

    for(std::vector< std::map<int, Sketcher::PointPos> >::const_iterator it = coincidenttree.begin(); it != coincidenttree.end(); ++it) {

        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

        geoId1iterator = (*it).find(GeoId1);

        if( geoId1iterator != (*it).end()) {
            // If First is in this set
            std::map<int, Sketcher::PointPos>::const_iterator geoId2iterator;

            geoId2iterator = (*it).find(GeoId2);

            if( geoId2iterator != (*it).end()) {
                // If Second is in this set
                if ((*geoId1iterator).second == PosId1 &&
                    (*geoId2iterator).second == PosId2)
                    return true;
            }
        }
    }

    return false;
}

void SketchObject::appendConflictMsg(const std::vector<int> &conflicting, std::string &msg)
{
    std::stringstream ss;
    if (msg.length() > 0)
        ss << msg;
    if (conflicting.size() > 0) {
        if (conflicting.size() == 1)
            ss << "Please remove the following constraint:\n";
        else
            ss << "Please remove at least one of the following constraints:\n";
        ss << conflicting[0];
        for (unsigned int i=1; i < conflicting.size(); i++)
            ss << ", " << conflicting[i];
        ss << "\n";
    }
    msg = ss.str();
}

void SketchObject::appendRedundantMsg(const std::vector<int> &redundant, std::string &msg)
{
    std::stringstream ss;
    if (msg.length() > 0)
        ss << msg;
    if (redundant.size() > 0) {
        if (redundant.size() == 1)
            ss << "Please remove the following redundant constraint:\n";
        else
            ss << "Please remove the following redundant constraints:\n";
        ss << redundant[0];
        for (unsigned int i=1; i < redundant.size(); i++)
            ss << ", " << redundant[i];
        ss << "\n";
    }
    msg = ss.str();
}

void SketchObject::getGeometryWithDependentParameters(std::vector<std::pair<int,PointPos>>& geometrymap)
{
    auto geos = getInternalGeometry();

    GCS::QRAlgorithm curQRAlg = getSolvedSketch().getQRAlgorithm();

    if(curQRAlg == GCS::EigenSparseQR) {
        getSolvedSketch().setQRAlgorithm(GCS::EigenDenseQR);
        solve(false);
    }

    auto addelement = [this,&geometrymap](int geoId, PointPos pos){
        if(getSolvedSketch().hasDependentParameters(geoId, pos))
            geometrymap.emplace_back(geoId,pos);
    };


    int geoid = 0;

    for(auto geo : geos) {
        if(geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            addelement(geoid, Sketcher::start);
        }
        else if(geo->getTypeId() == Part::GeomLineSegment::getClassTypeId() ||
            geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {

            addelement(geoid, Sketcher::start);
            addelement(geoid, Sketcher::end);
            addelement(geoid, Sketcher::none);
        }
        else if(geo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ) {

            addelement(geoid, Sketcher::mid);
            addelement(geoid, Sketcher::none);
        }
        else if(geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
            geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
            geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
            geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {

            addelement(geoid, Sketcher::start);
            addelement(geoid, Sketcher::end);
            addelement(geoid, Sketcher::mid);
            addelement(geoid, Sketcher::none);
        }

        geoid++;
    }

    if(curQRAlg == GCS::EigenSparseQR) {
        getSolvedSketch().setQRAlgorithm(GCS::EigenSparseQR);
    }
}

bool SketchObject::evaluateConstraint(const Constraint *constraint) const
{
    //if requireXXX,  GeoUndef is treated as an error. If not requireXXX,
    //GeoUndef is accepted. Index range checking is done on everything regardless.
    bool requireFirst = true;
    bool requireSecond = false;
    bool requireThird = false;

    switch (constraint->Type) {
        case Radius:
            requireFirst = true;
            break;
        case Diameter:
            requireFirst = true;
            break;
        case Horizontal:
        case Vertical:
            requireFirst = true;
            break;
        case Distance:
        case DistanceX:
        case DistanceY:
            requireFirst = true;
            break;
        case Coincident:
        case Perpendicular:
        case Parallel:
        case Equal:
        case PointOnObject:
        case Tangent:
            requireFirst = true;
            requireSecond = true;
            break;
        case Symmetric:
            requireFirst = true;
            requireSecond = true;
            requireThird = true;
            break;
        case Angle:
            requireFirst = true;
            break;
        case SnellsLaw:
            requireFirst = true;
            requireSecond = true;
            requireThird = true;
            break;
        default:
            break;
    }

    int intGeoCount = getHighestCurveIndex() + 1;
    int extGeoCount = getExternalGeometryCount();

    //the actual checks
    bool ret = true;
    int geoId;
    geoId = constraint->First;
    ret = ret && ((geoId == Constraint::GeoUndef && !requireFirst)
                  ||
                  (geoId >= -extGeoCount && geoId < intGeoCount) );

    geoId = constraint->Second;
    ret = ret && ((geoId == Constraint::GeoUndef && !requireSecond)
                  ||
                  (geoId >= -extGeoCount && geoId < intGeoCount) );

    geoId = constraint->Third;
    ret = ret && ((geoId == Constraint::GeoUndef && !requireThird)
                  ||
                  (geoId >= -extGeoCount && geoId < intGeoCount) );

    return ret;
}

bool SketchObject::evaluateConstraints() const
{
    int intGeoCount = getHighestCurveIndex() + 1;
    int extGeoCount = getExternalGeometryCount();

    std::vector<Part::Geometry *> geometry = getCompleteGeometry();
    const std::vector<Sketcher::Constraint *>& constraints = Constraints.getValuesForce();
    if (static_cast<int>(geometry.size()) != extGeoCount + intGeoCount)
        return false;
    if (geometry.size() < 2)
        return false;

    std::vector<Sketcher::Constraint *>::const_iterator it;
    for (it = constraints.begin(); it != constraints.end(); ++it) {
        if (!evaluateConstraint(*it))
            return false;
    }

    if (!constraints.empty()) {
        if (!Constraints.scanGeometry(geometry))
            return false;
    }

    return true;
}

void SketchObject::validateConstraints()
{
    std::vector<Part::Geometry *> geometry = getCompleteGeometry();
    const std::vector<Sketcher::Constraint *>& constraints = Constraints.getValuesForce();

    std::vector<Sketcher::Constraint *> newConstraints;
    std::vector<Sketcher::Constraint *>::const_iterator it;
    for (it = constraints.begin(); it != constraints.end(); ++it) {
        bool valid = evaluateConstraint(*it);
        if (valid)
            newConstraints.push_back(*it);
    }

    if (newConstraints.size() != constraints.size()) {
        Constraints.setValues(newConstraints);
        acceptGeometry();
    }
    else if (!Constraints.scanGeometry(geometry)) {
        Constraints.acceptGeometry(geometry);
    }
}

std::string SketchObject::validateExpression(const App::ObjectIdentifier &path, boost::shared_ptr<const App::Expression> expr)
{
    const App::Property * prop = path.getProperty();

    assert(expr != 0);

    if (!prop)
        return "Property not found";

    if (prop == &Constraints) {
        const Constraint * constraint = Constraints.getConstraint(path);

        if (!constraint->isDriving)
            return "Reference constraints cannot be set!";
    }

    auto deps = expr->getDeps();
    auto it = deps.find(this);
    if(it!=deps.end()) {
        auto it2 = it->second.find("Constraints");
        if(it2 != it->second.end()) {
            for(auto &oid : it2->second) {
                const Constraint * constraint = Constraints.getConstraint(oid);

                if (!constraint->isDriving)
                    return "Reference constraint from this sketch cannot be used in this expression.";
            }
        }
    }
    return "";
}

//This function is necessary for precalculation of an angle when adding
// an angle constraint. It is also used here, in SketchObject, to
// lock down the type of tangency/perpendicularity.
double SketchObject::calculateAngleViaPoint(int GeoId1, int GeoId2, double px, double py)
{
    // Temporary sketch based calculation. Slow, but guaranteed consistency with constraints.
    Sketcher::Sketch sk;

    const Part::Geometry *p1=this->getGeometry(GeoId1);
    const Part::Geometry *p2=this->getGeometry(GeoId2);

    if(p1!=0 && p2!=0) {
        int i1 = sk.addGeometry(this->getGeometry(GeoId1));
        int i2 = sk.addGeometry(this->getGeometry(GeoId2));

        return sk.calculateAngleViaPoint(i1,i2,px,py);
    }
    else
        throw Base::ValueError("Null geometry in calculateAngleViaPoint");

/*
    // OCC-based calculation. It is faster, but it was removed due to problems
    // with reversed geometry (clockwise arcs). More info in "Sketch: how to
    // handle reversed external arcs?" forum thread
    // http://forum.freecadweb.org/viewtopic.php?f=10&t=9130&sid=1b994fa1236db5ac2371eeb9a53de23f

    const Part::GeomCurve &g1 = *(dynamic_cast<const Part::GeomCurve*>(this->getGeometry(GeoId1)));
    const Part::GeomCurve &g2 = *(dynamic_cast<const Part::GeomCurve*>(this->getGeometry(GeoId2)));
    Base::Vector3d p(px, py, 0.0);

    double u1 = 0.0;
    double u2 = 0.0;
    if (! g1.closestParameterToBasicCurve(p, u1) ) throw Base::ValueError("SketchObject::calculateAngleViaPoint: closestParameter(curve1) failed!");
    if (! g2.closestParameterToBasicCurve(p, u2) ) throw Base::ValueError("SketchObject::calculateAngleViaPoint: closestParameter(curve2) failed!");

    gp_Dir tan1, tan2;
    if (! g1.tangent(u1,tan1) ) throw Base::ValueError("SketchObject::calculateAngleViaPoint: tangent1 failed!");
    if (! g2.tangent(u2,tan2) ) throw Base::ValueError("SketchObject::calculateAngleViaPoint: tangent2 failed!");

    assert(abs(tan1.Z())<0.0001);
    assert(abs(tan2.Z())<0.0001);

    double ang = atan2(-tan2.X()*tan1.Y()+tan2.Y()*tan1.X(), tan2.X()*tan1.X() + tan2.Y()*tan1.Y());
    return ang;
*/
}

void SketchObject::constraintsRenamed(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &renamed)
{
    ExpressionEngine.renameExpressions(renamed);

    getDocument()->renameObjectIdentifiers(renamed);
}

void SketchObject::constraintsRemoved(const std::set<App::ObjectIdentifier> &removed)
{
    std::set<App::ObjectIdentifier>::const_iterator i = removed.begin();

    while (i != removed.end()) {
        ExpressionEngine.setValue(*i, boost::shared_ptr<App::Expression>());
        ++i;
    }
}

//Tests if the provided point lies exactly in a curve (satisfies
// point-on-object constraint). It is used to decide whether it is nesessary to
// constrain a point onto curves when 3-element selection tangent-via-point-like
// constraints are applied.
bool SketchObject::isPointOnCurve(int geoIdCurve, double px, double py)
{
    //DeepSOIC: this may be slow, but I wanted to reuse the existing code
    Sketcher::Sketch sk;
    int icrv = sk.addGeometry(this->getGeometry(geoIdCurve));
    Base::Vector3d pp;
    pp.x = px; pp.y = py;
    Part::GeomPoint p(pp);
    int ipnt = sk.addPoint(p);
    int icstr = sk.addPointOnObjectConstraint(ipnt, Sketcher::start, icrv);
    double err = sk.calculateConstraintError(icstr);
    return err*err < 10.0*sk.getSolverPrecision();
}

//This one was done just for fun to see to what precision the constraints are solved.
double SketchObject::calculateConstraintError(int ConstrId)
{
    Sketcher::Sketch sk;
    const std::vector<Constraint *> &clist = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(clist.size()))
        return std::numeric_limits<double>::quiet_NaN();

    Constraint* cstr = clist[ConstrId]->clone();
    double result=0.0;
    try{
        std::vector<int> GeoIdList;
        int g;
        GeoIdList.push_back(cstr->First);
        GeoIdList.push_back(cstr->Second);
        GeoIdList.push_back(cstr->Third);

        //add only necessary geometry to the sketch
        for(std::size_t i=0; i<GeoIdList.size(); i++){
            g = GeoIdList[i];
            if (g != Constraint::GeoUndef){
                GeoIdList[i] = sk.addGeometry(this->getGeometry(g));
            }
        }

        cstr->First = GeoIdList[0];
        cstr->Second = GeoIdList[1];
        cstr->Third = GeoIdList[2];
        int icstr = sk.addConstraint(cstr);
        result = sk.calculateConstraintError(icstr);
    } catch(...) {//cleanup
        delete cstr;
        throw;
    }
    delete cstr;
    return result;
}

PyObject *SketchObject::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new SketchObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

unsigned int SketchObject::getMemSize(void) const
{
    return 0;
}

void SketchObject::Save(Writer &writer) const
{
    // save the father classes
    Part::Part2DObject::Save(writer);
}

void SketchObject::Restore(XMLReader &reader)
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

void SketchObject::onChanged(const App::Property* prop)
{
    if (prop == &Geometry) {
        geoMap.clear();
        const auto &vals = getInternalGeometry();
        for(long i=0;i<(long)vals.size();++i) {
            auto geo = vals[i];
            if(!geo->Id) 
                geo->Id = ++geoLastId;
            else if(geo->Id > geoLastId)
                geoLastId = geo->Id;
            while(!geoMap.insert(std::make_pair(geo->Id,i)).second) {
                FC_WARN("duplicate geometry id " << geo->Id << " -> " << geoLastId+1);
                geo->Id = ++geoLastId;
            }
        }
        updateGeoHistory();

        if (isRestoring()) {
            std::vector<Part::Geometry*> geom = Geometry.getValues();
            std::vector<Part::Geometry*> supportedGeom = supportedGeometry(geom);
            // To keep upward compatibility ignore unsupported geometry types
            if (supportedGeom.size() != geom.size()) {
                Geometry.setValues(supportedGeom);
                return;
            }
        }

        Constraints.checkGeometry(getCompleteGeometry());

    } else if ( prop == &Constraints) {
        Constraints.checkGeometry(getCompleteGeometry());
    } else if ( prop == &ExternalGeo) {
        externalGeoRefMap.clear();
        externalGeoMap.clear();
        std::set<std::string> detached;
        for(int i=0;i<ExternalGeo.getSize();++i) {
            auto geo = ExternalGeo[i];
            if(geo->testFlag(Part::Geometry::Detached)) {
                if(geo->Ref.size()) {
                    detached.insert(geo->Ref);
                    geo->Ref.clear();
                }
                geo->setFlag(Part::Geometry::Detached,false);
                geo->setFlag(Part::Geometry::Missing,false);
            }
            if(geo->Id > geoLastId)
                geoLastId = geo->Id;
            if(!externalGeoMap.emplace(geo->Id,i).second) {
                FC_WARN("duplicate geometry id " << geo->Id << " -> " << geoLastId+1);
                geo->Id = ++geoLastId;
                externalGeoMap[geo->Id] = i;
            }
            if(geo->Ref.size())
                externalGeoRefMap[geo->Ref].push_back(geo->Id);
        }
        if(detached.size()) {
            auto objs = ExternalGeometry.getValues();
            assert(externalGeoRef.size() == objs.size());
            auto itObj = objs.begin();
            auto subs = ExternalGeometry.getSubValues();
            auto itSub = subs.begin();
            for(size_t i=0;i<externalGeoRef.size();++i) {
                if(detached.count(externalGeoRef[i])) {
                    itObj = objs.erase(itObj);
                    itSub = subs.erase(itSub);
                    auto &refs = externalGeoRefMap[externalGeoRef[i]];
                    for(long id : refs) {
                        auto it = externalGeoMap.find(id);
                        if(it!=externalGeoMap.end()) {
                            auto geo = ExternalGeo[it->second];
                            geo->Ref.clear();
                        }
                    }
                    refs.clear();
                } else {
                    ++itObj;
                    ++itSub;
                }
            }
            ExternalGeometry.setValues(objs,subs);
        }
    } else if( prop == &ExternalGeometry ) {
        if(!isRestoring()) {
            // must wait till onDocumentRestored() when shadow references are
            // fully restored
            updateGeometryRefs();
        }
    }
    Part::Part2DObject::onChanged(prop);
}

void SketchObject::onUpdateElementReference(const App::Property *prop) {
    if(prop == &ExternalGeometry)
        updateGeoRef = true;
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
    for(int i=0;i<(int)objs.size();++i) {
        auto obj = objs[i];
        const std::string &sub=shadows[i].first.size()?shadows[i].first:subs[i];        
        externalGeoRef.emplace_back(obj->getNameInDocument());
        auto &key = externalGeoRef.back();
        key += '.';

        if (!obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()) &&
                obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) 
        {
            key += Data::ComplexGeoData::newElementName(sub.c_str());
        }
        if(originalRefs.size() && originalRefs[i]!=key) 
            refMap[originalRefs[i]] = key;
    }
    if(refMap.size()) {
        auto geos = ExternalGeo.getValues();
        bool touched = false;
        for(auto &v : refMap) {
            auto it = externalGeoRefMap.find(v.first);
            if(it == externalGeoRefMap.end())
                continue;
            for(long id : it->second) {
                auto iter = externalGeoMap.find(id);
                if(iter!=externalGeoMap.end()) {
                    auto &geo = geos[iter->second];
                    geo = geo->clone();
                    FC_LOG(getFullName() << " ref change on ExternalEdge"
                            << iter->second-1 << ' ' << geo->Ref << " -> " << v.second);
                    geo->Ref = v.second;
                    touched = true;
                }
            }
        }
        if(touched)
            ExternalGeo.setValues(std::move(geos));
    }
}

void SketchObject::onDocumentRestored()
{
    bool migrate = false;
    updateGeometryRefs();
    if(ExternalGeo.getSize()<=2) {
        migrate = true;
        for(auto &key : externalGeoRef) {
            long id = getDocument()->Hasher->getID(key.c_str())->value();
            if(geoLastId < id)
                geoLastId = id;
            externalGeoRefMap[key].push_back(id);
        }
        rebuildExternalGeometry();
    }else
        acceptGeometry();

    // this may happen when saving a sketch directly in edit mode
    // but never performed a recompute before
    if (Shape.getValue().IsNull() && hasConflicts() == 0) {
        if (this->solve(true) == 0)
            buildShape();
    }

    Part::Part2DObject::onDocumentRestored();

    if(migrate && ExternalGeometry.getSize()+2!=ExternalGeo.getSize()) {
        delConstraintsToExternal();
        throw Base::RuntimeError("Failed to restore external geometry");
    }
}

void SketchObject::getGeoVertexIndex(int VertexId, int &GeoId, PointPos &PosId) const
{
    if (VertexId < 0 || VertexId >= int(VertexId2GeoId.size())) {
        GeoId = Constraint::GeoUndef;
        PosId = none;
        return;
    }
    GeoId = VertexId2GeoId[VertexId];
    PosId = VertexId2PosId[VertexId];
}

int SketchObject::getVertexIndexGeoPos(int GeoId, PointPos PosId) const
{
    auto it = GeoPos2VertexId.find(std::make_pair(GeoId,PosId));
    if(it != GeoPos2VertexId.end())
        return (int)it->second;
    return -1;
}

///changeConstraintsLocking locks or unlocks all tangent and perpendicular
/// constraints. (Constraint locking prevents it from flipping to another valid
/// configuration, when e.g. external geometry is updated from outside.) The
/// sketch solve is not triggered by the function, but the SketchObject is
/// touched (a recompute will be necessary). The geometry should not be affected
/// by the function.
///The bLock argument specifies, what to do. If true, all constraints are
/// unlocked and locked again. If false, all tangent and perp. constraints are
/// unlocked.
int SketchObject::changeConstraintsLocking(bool bLock)
{
    int cntSuccess = 0;
    int cntToBeAffected = 0;//==cntSuccess+cntFail
    const std::vector< Constraint * > &vals = this->Constraints.getValues();

    std::vector< Constraint * > newVals(vals);//modifiable copy of pointers array

    std::vector< Constraint * > tbd;//list of temporary Constraint copies that need to be deleted later

    for(std::size_t i = 0; i<newVals.size(); i++){
        if( newVals[i]->Type == Tangent || newVals[i]->Type == Perpendicular ){
            //create a constraint copy, affect it, replace the pointer
            cntToBeAffected++;
            Constraint *constNew = newVals[i]->clone();
            bool ret = AutoLockTangencyAndPerpty(constNew, /*bForce=*/true, bLock);
            if (ret) cntSuccess++;
            tbd.push_back(constNew);
            newVals[i] = constNew;
            Base::Console().Log("Constraint%i will be affected\n",
                                i+1);
        }
    }

    this->Constraints.setValues(newVals);

    //clean up - delete temporary copies of constraints that were made to affect the constraints
    for(std::size_t i=0; i<tbd.size(); i++){
        delete (tbd[i]);
    }

    Base::Console().Log("ChangeConstraintsLocking: affected %i of %i tangent/perp constraints\n",
                        cntSuccess, cntToBeAffected);

    return cntSuccess;
}

bool SketchObject::constraintHasExpression(int constrid) const
{
    App::ObjectIdentifier spath = this->Constraints.createPath(constrid);

    App::PropertyExpressionEngine::ExpressionInfo expr_info = this->getExpression(spath);

    return (expr_info.expression != 0);
}


/*!
 * \brief SketchObject::port_reversedExternalArcs finds constraints that link to endpoints of external-geometry arcs,
 *  and swaps the endpoints in the constraints. This is needed after CCW emulation was introduced, to port old sketches.
 * \param justAnalyze if true, nothing is actually done - only the number of constraints to be affected is returned.
 * \return the number of constraints changed/to be changed.
 */
int SketchObject::port_reversedExternalArcs(bool justAnalyze)
{
    int cntToBeAffected = 0;//==cntSuccess+cntFail
    const std::vector< Constraint * > &vals = this->Constraints.getValues();

    std::vector< Constraint * > newVals(vals);//modifiable copy of pointers array

    std::vector< Constraint * > tbd;//list of temporary Constraint copies that need to be deleted later

    for(std::size_t ic = 0; ic<newVals.size(); ic++){//ic = index of constraint
        bool affected=false;
        Constraint *constNew = 0;
        for(int ig=1; ig<=3; ig++){//cycle through constraint.first, second, third
            int geoId = 0;
            Sketcher::PointPos posId = none;
            switch (ig){
                case 1: geoId=newVals[ic]->First; posId = newVals[ic]->FirstPos; break;
                case 2: geoId=newVals[ic]->Second; posId = newVals[ic]->SecondPos; break;
                case 3: geoId=newVals[ic]->Third; posId = newVals[ic]->ThirdPos; break;
            }

            if ( geoId <= GeoEnum::RefExt &&
                 (posId==Sketcher::start || posId==Sketcher::end)){
                //we are dealing with a link to an endpoint of external geom
                Part::Geometry* g = this->ExternalGeo[-geoId-1];
                if (g->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()){
                    const Part::GeomArcOfCircle *segm = static_cast<const Part::GeomArcOfCircle*>(g);
                    if (segm->isReversed()){
                        //Gotcha! a link to an endpoint of external arc that is reversed.
                        //create a constraint copy, affect it, replace the pointer
                        if (!affected)
                            constNew = newVals[ic]->clone();
                        affected=true;
                        //Do the fix on temp vars
                        if(posId == Sketcher::start)
                            posId = Sketcher::end;
                        else if (posId == Sketcher::end)
                            posId = Sketcher::start;
                    }
                }
            }
            if (!affected) continue;
            //Propagate the fix made on temp vars to the constraint
            switch (ig){
                case 1: constNew->First = geoId; constNew->FirstPos = posId; break;
                case 2: constNew->Second = geoId; constNew->SecondPos = posId; break;
                case 3: constNew->Third = geoId; constNew->ThirdPos = posId; break;
            }
        }
        if (affected){
            cntToBeAffected++;
            tbd.push_back(constNew);
            newVals[ic] = constNew;
            Base::Console().Log("Constraint%i will be affected\n",
                                ic+1);
        };
    }

    if(!justAnalyze){
        this->Constraints.setValues(newVals);
        Base::Console().Log("Swapped start/end of reversed external arcs in %i constraints\n",
                            cntToBeAffected);
    }

    //clean up - delete temporary copies of constraints that were made to affect the constraints
    for(std::size_t i=0; i<tbd.size(); i++){
        delete (tbd[i]);
    }


    return cntToBeAffected;
}

///Locks tangency/perpendicularity type of such a constraint.
///The constraint passed must be writable (i.e. the one that is not
/// yet in the constraint list).
///Tangency type (internal/external) is derived from current geometry
/// the constraint refers to.
///Same for perpendicularity type.
///
///This function catches exceptions, because it's not a reason to
/// not create a constraint if tangency/perp-ty type cannot be determined.
///
///Arguments:
/// cstr - pointer to a constraint to be locked/unlocked
/// bForce - specifies whether to ignore the already locked constraint or not.
/// bLock - specifies whether to lock the constraint or not (if bForce is
///  true, the constraint gets unlocked, otherwise nothing is done at all).
///
///Return values:
/// true - success.
/// false - fail (this indicates an error, or that a constraint locking isn't supported).
bool SketchObject::AutoLockTangencyAndPerpty(Constraint *cstr, bool bForce, bool bLock)
{
    try{
        //assert ( cstr->Type == Tangent  ||  cstr->Type == Perpendicular);
        if(cstr->getValue() != 0.0 && ! bForce) /*tangency type already set. If not bForce - don't touch.*/
            return true;
        if(!bLock){
            cstr->setValue(0.0);//reset
        } else {
            //decide on tangency type. Write the angle value into the datum field of the constraint.
            int geoId1, geoId2, geoIdPt;
            PointPos posPt;
            geoId1 = cstr->First;
            geoId2 = cstr->Second;
            geoIdPt = cstr->Third;
            posPt = cstr->ThirdPos;
            if (geoIdPt == Constraint::GeoUndef){//not tangent-via-point, try endpoint-to-endpoint...
                geoIdPt = cstr->First;
                posPt = cstr->FirstPos;
            }
            if (posPt == none){//not endpoint-to-curve and not endpoint-to-endpoint tangent (is simple tangency)
                //no tangency lockdown is implemented for simple tangency. Do nothing.
                return false;
            } else {
                Base::Vector3d p = getPoint(geoIdPt, posPt);

                //this piece of code is also present in Sketch.cpp, correct for offset
                //and to do the autodecision for old sketches.
                double angleOffset = 0.0;//the difference between the datum value and the actual angle to apply. (datum=angle+offset)
                double angleDesire = 0.0;//the desired angle value (and we are to decide if 180* should be added to it)
                if (cstr->Type == Tangent) {angleOffset = -M_PI/2; angleDesire = 0.0;}
                if (cstr->Type == Perpendicular) {angleOffset = 0; angleDesire = M_PI/2;}

                double angleErr = calculateAngleViaPoint(geoId1, geoId2, p.x, p.y) - angleDesire;

                //bring angleErr to -pi..pi
                if (angleErr > M_PI) angleErr -= M_PI*2;
                if (angleErr < -M_PI) angleErr += M_PI*2;

                //the autodetector
                if(fabs(angleErr) > M_PI/2 )
                    angleDesire += M_PI;

                cstr->setValue(angleDesire + angleOffset); //external tangency. The angle stored is offset by Pi/2 so that a value of 0.0 is invalid and threated as "undecided".
            }
        }
    } catch (Base::Exception& e){
        //failure to determine tangency type is not a big deal, so a warning.
        Base::Console().Warning("Error in AutoLockTangency. %s \n", e.what());
        return false;
    }
    return true;
}

void SketchObject::setExpression(const App::ObjectIdentifier &path, boost::shared_ptr<App::Expression> expr)
{
    DocumentObject::setExpression(path, expr);

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver, constraints and UI
        solve();
}

App::DocumentObject *SketchObject::getSubObject(
        const char *subname, PyObject **pyObj, 
        Base::Matrix4D *pmat, bool transform, int depth) const
{
    const char *mapped = Data::ComplexGeoData::isMappedElement(subname);
    if(!subname || !subname[0] || (mapped && hasSketchMarker(mapped))) {
        return Part2DObject::getSubObject(subname,pyObj,pmat,transform,depth);
    }
    if(!mapped) {
        const char *dot = strchr(subname,'.');
        if(dot) {
            std::string name(subname,dot-subname);
            auto child = Exports.find(name.c_str());
            if(!child)
                return 0;
            return child->getSubObject(dot+1,pyObj,pmat,true,depth+1);
        }else if(boost::starts_with(subname,"Vertex") || boost::starts_with(subname,"Edge"))
            return Part2DObject::getSubObject(subname,pyObj,pmat,transform,depth);
    }

    std::string sub = checkSubName(subname);
    const char *shapetype = sub.c_str();
    const Part::Geometry *geo = 0;
    Base::Vector3d point;
    if (boost::starts_with(shapetype,"Edge") ||
        boost::starts_with(shapetype,"edge")) {
        geo = getGeometry(std::atoi(&shapetype[4]) - 1);
        if (!geo) return 0;
    } else if (boost::starts_with(shapetype,"ExternalEdge")) {
        int GeoId = std::atoi(&shapetype[12]) - 1;
        GeoId = -GeoId - 3;
        geo = getGeometry(GeoId);
        if(!geo) return 0;
    } else if (boost::starts_with(shapetype,"Vertex") ||
               boost::starts_with(shapetype,"vertex")) {
        int VtId = std::atoi(&shapetype[6]) - 1;
        int GeoId;
        PointPos PosId;
        getGeoVertexIndex(VtId,GeoId,PosId);
        if (PosId==none) return 0;
        point = getPoint(GeoId,PosId);
    }
    else if (strcmp(shapetype,"RootPoint")==0) 
        point = getPoint(Sketcher::GeoEnum::RtPnt,start);
    else if (strcmp(shapetype,"H_Axis")==0)
        geo = getGeometry(Sketcher::GeoEnum::HAxis);
    else if (strcmp(shapetype,"V_Axis")==0)
        geo = getGeometry(Sketcher::GeoEnum::VAxis);
    else if (boost::starts_with(shapetype,"Constraint")) {
        int ConstrId = PropertyConstraintList::getIndexFromConstraintName(shapetype);
        const std::vector< Constraint * > &vals = this->Constraints.getValues();
        if (ConstrId < 0 || ConstrId >= int(vals.size()))
            return 0;
        if(pyObj) 
            *pyObj = vals[ConstrId]->getPyObject();
        return const_cast<SketchObject*>(this);
    }else
        return 0;

    if (pmat && transform)
        *pmat *= Placement.getValue().toMatrix();

    if (pyObj) {
        Part::TopoShape shape;
        if (geo) {
            std::string name = convertSubName(shapetype,false);
            shape = getEdge(geo,name.c_str());
            if(pmat && !shape.isNull()) 
                shape.transformShape(*pmat,false,true);
        }else {
            if(pmat)
                point = (*pmat)*point;
            shape = BRepBuilderAPI_MakeVertex(gp_Pnt(point.x,point.y,point.z)).Vertex();
            shape.setElementName("Vertex1",convertSubName(shapetype,false).c_str());
        }
        shape.Tag = getID();
        *pyObj = Py::new_reference_to(Part::shape2pyshape(shape));
    }

    return const_cast<SketchObject*>(this);
}

std::pair<std::string,std::string> SketchObject::getElementName(
        const char *name, ElementNameType type) const
{
    std::pair<std::string,std::string> ret;
    if(!name) return ret;
    const char *mapped = Data::ComplexGeoData::isMappedElement(name);
    if(!mapped) {
        if(boost::starts_with(name,"Vertex") || 
           boost::starts_with(name,"Edge"))
            return Part2DObject::getElementName(name,type);

        ret.first = convertSubName(name,true);
        if(!Data::ComplexGeoData::isMappedElement(ret.first.c_str()))
            ret.first.clear();
        ret.second = name;
        return ret;
    }

    if(hasSketchMarker(mapped))
        return Part2DObject::getElementName(name,type);

    ret.second = checkSubName(name);
    if(ret.second.size()) {
        ret.first = convertSubName(ret.second.c_str(),true);
        if(type==ElementNameType::Export) {
            if(boost::starts_with(ret.second,"Vertex"))
                ret.second[0] = 'v';
            else if(boost::starts_with(ret.second,"Edge"))
                ret.second[0] = 'e';
        }
    }
    return ret;
}

Part::TopoShape SketchObject::getEdge(const Part::Geometry *geo, const char *name) const {
    Part::TopoShape shape(geo->toShape());
    shape.setElementName("Edge1",name);
    TopTools_IndexedMapOfShape vmap;
    TopExp::MapShapes(shape.getShape(), TopAbs_VERTEX, vmap);
    for(int i=1;i<=vmap.Extent();++i) {
        auto gpt = BRep_Tool::Pnt(TopoDS::Vertex(vmap(i)));
        Base::Vector3d pt(gpt.X(),gpt.Y(),gpt.Z());
        PointPos pos[] = {start,end};
        for(size_t j=0;j<sizeof(pos)/sizeof(pos[0]);++j) {
            if(getPoint(geo,pos[j]) == pt) {
                std::ostringstream ss;
                ss << name << 'v' << pos[j];
                std::string element("Vertex");
                element += std::to_string(i);
                shape.setElementName(element.c_str(),ss.str().c_str());
                break;
            }
        }
    }
    return shape;
}

std::vector<std::string> SketchObject::checkSubNames(const std::vector<std::string> &subnames) const{
    std::vector<std::string> ret;
    ret.reserve(subnames.size());
    for(const auto &subname : subnames) 
        ret.push_back(checkSubName(subname.c_str()));
    return ret;
}

std::string SketchObject::checkSubName(const char *sub) const{
    if(!sub) return std::string();
    const char *subname = Data::ComplexGeoData::isMappedElement(sub);
    if(!subname)
        return sub;
    if(!subname[0]) {
        FC_ERR("invalid subname " << sub);
        return sub;
    }
    std::istringstream iss(subname+1);
    int id = -1;
    bool valid = false;
    switch(subname[0]) {
    case 'g':
    case 'e': 
        if(iss>>id) 
            valid = true;
        break;
    default: {
        // for RootPoint,H_Axis,V_Axis
        const char *dot = strchr(subname,'.');
        if(dot)
            return dot+1;
        return subname;
    }}
    if(!valid) {
        FC_ERR("invalid subname " << sub);
        return sub;
    }

    int geoId;
    const Part::Geometry *geo = 0;
    switch(subname[0]) {
    case 'g': {
        auto it = geoMap.find(id);
        if(it!=geoMap.end()) {
            std::ostringstream ss;
            geoId = it->second;
            geo = getGeometry(geoId);
        }
        break;
    } case 'e': {
        auto it = externalGeoMap.find(id);
        if(it!=externalGeoMap.end()) {
            geoId = -it->second - 1;
            geo  = getGeometry(geoId);
        }
        break;
    }}
    if(geo && geo->Id == id) {
        char sep;
        int posId = none;
        std::ostringstream ss;
        if((iss >> sep >> posId) && sep=='v') {
            int idx = getVertexIndexGeoPos(geoId,static_cast<PointPos>(posId));
            if(idx < 0) {
                FC_ERR("invalid subname " << sub);
                return sub;
            }
            ss << "Vertex" << idx+1;
        }else if(geoId>=0)
            ss << "Edge" << geoId+1;
        else
            ss << "ExternalEdge" << (-geoId-3)+1;
        return ss.str();
    }
    FC_ERR("cannot find subname " << sub);
    return sub;
}

std::string SketchObject::shapeTypeFromGeoId(int geoId, PointPos posId) const {
    if(geoId == GeoEnum::HAxis) {
        if(posId == start)
            return "RootPoint";
        return "H_Axis";
    }else if(geoId == GeoEnum::VAxis)
        return "V_Axis";
    if(posId != none) {
        int idx = getVertexIndexGeoPos(geoId, posId);
        if(idx < 0)
            return std::string();
        return std::string("Vertex") + std::to_string(idx+1);
    }
    if(geoId >= 0)
        return std::string("Edge") + std::to_string(geoId+1);
    else
        return std::string("ExternalEdge") + std::to_string(-geoId-2);
}

bool SketchObject::geoIdFromShapeType(const char *_shapetype, int &geoId, PointPos &posId) const {
    posId = none;
    auto shapetype = checkSubName(_shapetype);
    if (boost::starts_with(shapetype,"Edge") ||
        boost::starts_with(shapetype,"edge")) {
        geoId = std::atoi(&shapetype[4]) - 1;
    } else if (boost::starts_with(shapetype,"ExternalEdge")) {
        geoId = std::atoi(&shapetype[12]) - 1;
        geoId = -geoId - 3;
    } else if (boost::starts_with(shapetype,"Vertex") ||
               boost::starts_with(shapetype,"vertex")) {
        int VtId = std::atoi(&shapetype[6]) - 1;
        getGeoVertexIndex(VtId,geoId,posId);
        if (posId==none) return false;
    } else if (shapetype=="H_Axis") {
        geoId = Sketcher::GeoEnum::HAxis;
    } else if (shapetype=="V_Axis") {
        geoId = Sketcher::GeoEnum::VAxis;
    } else if (shapetype=="RootPoint") {
        geoId = Sketcher::GeoEnum::RtPnt;
        posId = start;
    } else
        return false;
    return true;
}

std::string SketchObject::convertSubName(const char *shapetype, bool postfix) const{
    std::ostringstream ss;
    int geoId;
    PointPos posId;
    if(!geoIdFromShapeType(shapetype,geoId,posId))
        return shapetype;
    if(geoId == Sketcher::GeoEnum::HAxis ||
       geoId == Sketcher::GeoEnum::VAxis ||
       geoId == Sketcher::GeoEnum::RtPnt) {
        ss << Data::ComplexGeoData::elementMapPrefix() << shapetype;
        if(postfix)
           ss  << '.' << shapetype;
        return ss.str();
    }

    auto geo = getGeometry(geoId);
    if(!geo)
        return shapetype;
    ss << Data::ComplexGeoData::elementMapPrefix() << (geoId>=0?'g':'e') << geo->Id;
    if(posId!=none)
        ss << 'v' << posId;
    if(postfix) {
        // rename Edge to edge, and Vertex to vertex to avoid ambiguous of
        // element mapping of the public shape and internal geometry.
        if(boost::starts_with(shapetype,"Edge"))
            ss << ".e" << shapetype+1;
        else if(boost::starts_with(shapetype,"Vertex"))
            ss << ".v" << shapetype+1;
        else
            ss << '.' << shapetype;
    }
    return ss.str();
}

std::string SketchObject::getGeometryReference(int GeoId) const {
    auto geo = getGeometry(GeoId);
    if(!geo || geo->Ref.empty())
        return std::string();

    if(geo->testFlag(Part::Geometry::Missing))
        return std::string("? ") + geo->Ref;

    auto pos = geo->Ref.find('.');
    if(pos == std::string::npos)
        return geo->Ref;
    std::string objName = geo->Ref.substr(0,pos);
    auto obj = getDocument()->getObject(objName.c_str());
    if(!obj)
        return geo->Ref;

    std::pair<std::string,std::string> elementName;
    App::GeoFeature::resolveElement(obj,geo->Ref.c_str()+pos+1,elementName);
    if(elementName.second.size())
        return objName + "." + elementName.second;
    return geo->Ref;
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

std::vector<ConstraintIds> & SketchObject::getMissingPointOnPointConstraints(void)
{
    return analyser->getMissingPointOnPointConstraints();
}

std::vector<ConstraintIds> & SketchObject::getMissingVerticalHorizontalConstraints(void)
{
    return analyser->getMissingVerticalHorizontalConstraints();
}

std::vector<ConstraintIds> & SketchObject::getMissingLineEqualityConstraints(void)
{
    return analyser->getMissingLineEqualityConstraints();
}

std::vector<ConstraintIds> & SketchObject::getMissingRadiusConstraints(void)
{
    return analyser->getMissingRadiusConstraints();
}

void SketchObject::setMissingRadiusConstraints(std::vector<ConstraintIds> &cl)
{
    if(analyser)
        analyser->setMissingRadiusConstraints(cl);
}

void SketchObject::setMissingLineEqualityConstraints(std::vector<ConstraintIds>& cl)
{
    if(analyser)
        analyser->setMissingLineEqualityConstraints(cl);
}

void SketchObject::setMissingVerticalHorizontalConstraints(std::vector<ConstraintIds>& cl)
{
    if(analyser)
        analyser->setMissingVerticalHorizontalConstraints(cl);
}

void SketchObject::setMissingPointOnPointConstraints(std::vector<ConstraintIds>& cl)
{
    if(analyser)
        analyser->setMissingPointOnPointConstraints(cl);
}

void SketchObject::makeMissingPointOnPointCoincident(bool onebyone)
{
    if(analyser)
        analyser->makeMissingPointOnPointCoincident(onebyone);
}

void SketchObject::makeMissingVerticalHorizontal(bool onebyone)
{
    if(analyser)
        analyser->makeMissingVerticalHorizontal(onebyone);
}

void SketchObject::makeMissingEquality(bool onebyone)
{
    if(analyser)
        analyser->makeMissingEquality(onebyone);
}

int SketchObject::autoRemoveRedundants(bool updategeo)
{
    auto redundants = getLastRedundant();

    if(redundants.size() == 0)
        return 0;

    for(size_t i=0;i<redundants.size();i++) // getLastRedundant is base 1, while delConstraints is base 0
        redundants[i]--;

    delConstraints(redundants,updategeo);

    return redundants.size();
}

// Python Sketcher feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Sketcher::SketchObjectPython, Sketcher::SketchObject)
template<> const char* Sketcher::SketchObjectPython::getViewProviderName(void) const {
    return "SketcherGui::ViewProviderPython";
}
template<> PyObject* Sketcher::SketchObjectPython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<SketchObjectPy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class SketcherExport FeaturePythonT<Sketcher::SketchObject>;
}

// ---------------------------------------------------------

PROPERTY_SOURCE(Sketcher::SketchExport, Part::Part2DObject)

SketchExport::SketchExport() {
    ADD_PROPERTY_TYPE(Base,(0),"",App::Prop_Hidden,"Base sketch object name");
    ADD_PROPERTY_TYPE(Refs,(),"",App::Prop_None,"Sketch geometry references");
    ADD_PROPERTY_TYPE(SyncPlacement,(true),"",App::Prop_None,"Synchronize placement with parent sketch if not attached");
}

SketchExport::~SketchExport()
{}

App::DocumentObject *SketchExport::getBase() const {
    return Base.getValue();
}

App::DocumentObjectExecReturn *SketchExport::execute(void) {
    try {
        App::DocumentObjectExecReturn* rtn = Part2DObject::execute();//to positionBySupport
        if(rtn!=App::DocumentObject::StdReturn)
            //error
            return rtn;
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    auto base = dynamic_cast<SketchObject*>(getBase());
    if(!base)
        return new App::DocumentObjectExecReturn("Missing parent sketch");
    if(update() && SyncPlacement.getValue() && !positionBySupport())
        Placement.setValue(base->Placement.getValue());
    return App::DocumentObject::StdReturn;
}

void SketchExport::onChanged(const App::Property* prop) {
    if(prop == &Refs) {
        if(!isRestoring() && !Refs.testStatus(App::Property::User3))
            update();
    } else if(prop == &Shape) {
        // bypass Part::Feature logic, 'cause we don't want to transform the
        // shape and mess up the element map.
        DocumentObject::onChanged(prop);
        return;
    }
    Part2DObject::onChanged(prop);
}

std::set<std::string> SketchExport::getRefs() const {
    std::set<std::string> refSet;
    const auto &refs = Refs.getValues();
    refSet.insert(refs.begin(),refs.end());
    if(refSet.size()>1)
        refSet.erase("");
    return refSet;
}

bool SketchExport::update() {
    auto base = getBase();
    if(!base) 
        return false;
    std::vector<Part::TopoShape> points;
    std::vector<Part::TopoShape> shapes;
    for(const auto &ref : getRefs()) {
        // Obtain the shape without feature's placement transformation, because
        // we may have our own support.
        auto shape = Part::Feature::getTopoShape(base,ref.c_str(),true,0,0,false,false);
        if(shape.isNull()) {
            FC_ERR("Invalid element reference: " << ref);
            throw Base::RuntimeError("Invalid element reference");
        }
        if(!shape.hasSubShape(TopAbs_EDGE))
            points.push_back(shape.makECopy(TOPOP_SKETCH_EXPORT));
        else
            shapes.push_back(shape.makECopy());
    }
    Part::TopoShape res;
    if(shapes.size()) {
        res.makEWires(shapes,TOPOP_SKETCH_EXPORT);
        shapes.clear();
        if(points.size()) {
            // Check if the vertex is already included in the wires
            for(auto &point : points) {
                auto name = point.getElementName("Vertex1",true);
                if(res.getElementName(name,2)!=name)
                    continue;
                shapes.push_back(point);
            }
            if(shapes.size()) {
                shapes.push_back(res);
                res.makECompound(shapes);
            }
        }
    }else if(points.empty())
        return false;
    else
        res.makECompound(points,0,false);
    Shape.setValue(res);
    return true;
}

void SketchExport::handleChangedPropertyType(Base::XMLReader &reader, 
        const char *TypeName, App::Property *prop)
{
    if (prop == &Base && strcmp(TypeName, "App::PropertyString") == 0) {
        App::PropertyString p;
        p.Restore(reader);
        auto obj = getDocument()->getObject(p.getValue());
        if(!obj) {
            FC_ERR("Cannot find parent sketch '" << p.getValue() << "' of " << getFullName());
            return;
        }
        Base.setValue(obj);
    }
}


