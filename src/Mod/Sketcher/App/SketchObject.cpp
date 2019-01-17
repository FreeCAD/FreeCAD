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
# include <GeomAPI_IntSS.hxx>
# include <BRepProj_Projection.hxx>
# include <GeomConvert_BSplineCurveKnotSplitting.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <GC_MakeCircle.hxx>
# include <Standard_Version.hxx>
# include <cmath>
# include <vector>
//# include <QtGlobal>
#endif

#include <boost/bind.hpp>

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

#include "SketchObject.h"
#include "Sketch.h"
#include <Mod/Sketcher/App/SketchObjectPy.h>


#undef DEBUG
//#define DEBUG

using namespace Sketcher;
using namespace Base;

const int GeoEnum::RtPnt  = -1;
const int GeoEnum::HAxis  = -1;
const int GeoEnum::VAxis  = -2;
const int GeoEnum::RefExt = -3;


PROPERTY_SOURCE(Sketcher::SketchObject, Part::Part2DObject)


SketchObject::SketchObject()
{
    ADD_PROPERTY_TYPE(Geometry,        (0)  ,"Sketch",(App::PropertyType)(App::Prop_None),"Sketch geometry");
    ADD_PROPERTY_TYPE(Constraints,     (0)  ,"Sketch",(App::PropertyType)(App::Prop_None),"Sketch constraints");
    ADD_PROPERTY_TYPE(ExternalGeometry,(0,0),"Sketch",(App::PropertyType)(App::Prop_None),"Sketch external geometry");

    Geometry.setOrderRelevant(true);

    allowOtherBody = true;
    allowUnaligned = true;

    for (std::vector<Part::Geometry *>::iterator it=ExternalGeo.begin(); it != ExternalGeo.end(); ++it)
        if (*it) delete *it;
    ExternalGeo.clear();
    Part::GeomLineSegment *HLine = new Part::GeomLineSegment();
    Part::GeomLineSegment *VLine = new Part::GeomLineSegment();
    HLine->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
    VLine->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(0,1,0));
    HLine->Construction = true;
    VLine->Construction = true;
    ExternalGeo.push_back(HLine);
    ExternalGeo.push_back(VLine);
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
    for (std::vector<Part::Geometry *>::iterator it=ExternalGeo.begin(); it != ExternalGeo.end(); ++it)
        if (*it) delete *it;
    ExternalGeo.clear();

    delete analyser;
}

short SketchObject::mustExecute() const
{
    if (Geometry.isTouched())
        return 1;
    if (Constraints.isTouched())
        return 1;
    if (ExternalGeometry.isTouched())
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
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\nClear constraints to external geometry\n", e.what());
        // we cannot trust the constraints of external geometries, so remove them
        delConstraintsToExternal();
    }

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
    Shape.setValue(solvedSketch.toShape());

    return App::DocumentObject::StdReturn;
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

Base::Vector3d SketchObject::getPoint(int GeoId, PointPos PosId) const
{
    if(!(GeoId == H_Axis || GeoId == V_Axis
         || (GeoId <= getHighestCurveIndex() && GeoId >= -getExternalGeometryCount()) ))
        throw Base::ValueError("SketchObject::getPoint. Invalid GeoId was supplied.");
    const Part::Geometry *geo = getGeometry(GeoId);
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
        if (PosId == mid)
            return circle->getCenter();
    } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse*>(geo);
        if (PosId == mid)
            return ellipse->getCenter();
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
    this->Constraints.setValues(newConstraints);
    for (Constraint* it : newConstraints)
        delete it;
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

int SketchObject::toggleConstruction(int GeoId)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    if(vals[GeoId]->getTypeId() == Part::GeomPoint::getClassTypeId())
        return -1;

    std::vector< Part::Geometry * > newVals(vals);

    Part::Geometry *geoNew = newVals[GeoId]->clone();
    geoNew->Construction = !geoNew->Construction;
    newVals[GeoId]=geoNew;

    this->Geometry.setValues(newVals);
    //this->Constraints.acceptGeometry(getCompleteGeometry()); <= This is not necessary for a toggle. Reducing redundant solving. Abdullah
    solverNeedsUpdate=true;
    return 0;
}

int SketchObject::setConstruction(int GeoId, bool on)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    if(vals[GeoId]->getTypeId() == Part::GeomPoint::getClassTypeId())
        return -1;

    std::vector< Part::Geometry * > newVals(vals);

    Part::Geometry *geoNew = newVals[GeoId]->clone();
    geoNew->Construction = on;
    newVals[GeoId]=geoNew;

    this->Geometry.setValues(newVals);
    //this->Constraints.acceptGeometry(getCompleteGeometry()); <= This is not necessary for a toggle. Reducing redundant solving. Abdullah
    solverNeedsUpdate=true;
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
        catch (Base::CADKernelError &e) {
            e.ReportException();
            THROWM(Base::CADKernelError, "Unable to determine the parameter of the first selected curve at the reference point.")
        }

        try {
             if(!curve2->closestParameter(refPnt2,refparam2))
                return -1;
        }
        catch (Base::CADKernelError &e) {
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
                catch (Base::CADKernelError &e) {
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
        catch (Base::CADKernelError &e) {
            e.ReportException();
            THROWM(Base::CADKernelError,"Unable to determine the parameter of the first selected curve at the intersection of the curves.")
        }

        try {
            if(!curve2->closestParameter(interpoints.second,intparam2))
                return -1;
        }
        catch (Base::CADKernelError &e) {
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
        catch (Base::CADKernelError &e) {
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
        catch (Base::CADKernelError &e) {
            e.ReportException();
            THROWM(Base::CADKernelError,"Unable to determine the starting point of the arc.")
        }

        try {
            if(!curve2->closestParameter(filletcenterpoint.second,refoparam2))
                return -1;
        }
        catch (Base::CADKernelError &e) {
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
        (GeoId < 0 && -GeoId > static_cast<int>(ExternalGeo.size())) ||
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
    }
    else { // normal geometry

        newVals[GeoId] = bspline;

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
        THROWMT(Base::ValueError,QT_TRANSLATE_NOOP("Exceptions","The multiplicity cannot be increased beyond the degree of the B-spline."))

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

        const std::vector<DocumentObject*> originalObjects = Objects;
        const std::vector<std::string>     originalSubElements = SubElements;

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

        try {
            rebuildExternalGeometry();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("%s\n", e.what());
            // revert to original values
            ExternalGeometry.setValues(originalObjects,originalSubElements);
            return -1;
        }

        solverNeedsUpdate=true;
    }

    for (std::vector<Part::Geometry *>::const_iterator it=svals.begin(); it != svals.end(); ++it){
        Part::Geometry *geoNew = (*it)->copy();
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

int SketchObject::addExternal(App::DocumentObject *Obj, const char* SubName)
{
    // so far only externals to the support of the sketch and datum features
    if (!isExternalAllowed(Obj->getDocument(), Obj))
       return -1;

    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string>     originalSubElements = SubElements;

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
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects,originalSubElements);
        return -1;
    }

    solverNeedsUpdate=true;
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    return ExternalGeometry.getValues().size()-1;
}

int SketchObject::delExternal(int ExtGeoId)
{
    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();

    if (ExtGeoId < 0 || ExtGeoId >= int(SubElements.size()))
        return -1;

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string>     originalSubElements = SubElements;

    Objects.erase(Objects.begin()+ExtGeoId);
    SubElements.erase(SubElements.begin()+ExtGeoId);

    const std::vector< Constraint * > &constraints = Constraints.getValues();
    std::vector< Constraint * > newConstraints(0);
    int GeoId = GeoEnum::RefExt - ExtGeoId;
    for (std::vector<Constraint *>::const_iterator it = constraints.begin();
         it != constraints.end(); ++it) {
        if ((*it)->First != GeoId && (*it)->Second != GeoId && (*it)->Third != GeoId) {
            Constraint *copiedConstr = (*it)->clone();
            if (copiedConstr->First < GeoId &&
                copiedConstr->First != Constraint::GeoUndef)
                copiedConstr->First += 1;
            if (copiedConstr->Second < GeoId &&
                copiedConstr->Second != Constraint::GeoUndef)
                copiedConstr->Second += 1;
            if (copiedConstr->Third < GeoId &&
                copiedConstr->Third != Constraint::GeoUndef)
                copiedConstr->Third += 1;

            newConstraints.push_back(copiedConstr);
        }
    }

    ExternalGeometry.setValues(Objects,SubElements);
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects,originalSubElements);
        for (Constraint* it : newConstraints)
            delete it;
        return -1;
    }

    solverNeedsUpdate=true;
    Constraints.setValues(newConstraints);
    for (Constraint* it : newConstraints)
        delete it;
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    return 0;
}

int SketchObject::delAllExternal()
{
    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string>     originalSubElements = SubElements;

    Objects.clear();

    SubElements.clear();

    const std::vector< Constraint * > &constraints = Constraints.getValues();
    std::vector< Constraint * > newConstraints(0);

    for (std::vector<Constraint *>::const_iterator it = constraints.begin(); it != constraints.end(); ++it) {
        if ((*it)->First > GeoEnum::RefExt &&
            ((*it)->Second > GeoEnum::RefExt || (*it)->Second == Constraint::GeoUndef ) &&
            ((*it)->Third > GeoEnum::RefExt || (*it)->Third == Constraint::GeoUndef) ) {
            Constraint *copiedConstr = (*it)->clone();

            newConstraints.push_back(copiedConstr);
        }
    }

    ExternalGeometry.setValues(Objects,SubElements);
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects,originalSubElements);
        for (Constraint* it : newConstraints)
            delete it;
        return -1;
    }

    solverNeedsUpdate=true;
    Constraints.setValues(newConstraints);
    for (Constraint* it : newConstraints)
        delete it;
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    return 0;
}

int SketchObject::delConstraintsToExternal()
{
    const std::vector< Constraint * > &constraints = Constraints.getValuesForce();
    std::vector< Constraint * > newConstraints(0);
    int GeoId = GeoEnum::RefExt, NullId = Constraint::GeoUndef;
    for (std::vector<Constraint *>::const_iterator it = constraints.begin();
         it != constraints.end(); ++it) {
        if (    (*it)->First > GeoId
                &&
                ((*it)->Second > GeoId || (*it)->Second == NullId)
                &&
                ((*it)->Third > GeoId || (*it)->Third == NullId)) {
            newConstraints.push_back(*it);
        }
    }

    Constraints.setValues(newConstraints);
    Constraints.acceptGeometry(getCompleteGeometry());

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        solve();

    return 0;
}

const Part::Geometry* SketchObject::getGeometry(int GeoId) const
{
    if (GeoId >= 0) {
        const std::vector<Part::Geometry *> &geomlist = getInternalGeometry();
        if (GeoId < int(geomlist.size()))
            return geomlist[GeoId];
    }
    else if (GeoId <= -1 && -GeoId <= int(ExternalGeo.size()))
        return ExternalGeo[-GeoId-1];

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

void SketchObject::validateExternalLinks(void)
{
    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();

    bool rebuild = false;

    for (int i=0; i < int(Objects.size()); i++) {
        const App::DocumentObject *Obj=Objects[i];
        const std::string SubElement=SubElements[i];

        TopoDS_Shape refSubShape;
        try {
            if (Obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
                const Part::Datum* datum = static_cast<const Part::Datum*>(Obj);
                refSubShape = datum->getShape();
            }
            else {
                const Part::Feature *refObj=static_cast<const Part::Feature*>(Obj);
                const Part::TopoShape& refShape=refObj->Shape.getShape();
                refSubShape = refShape.getSubShape(SubElement.c_str());
            }
        }
        catch (Standard_Failure&) {
            rebuild = true ;
            Objects.erase(Objects.begin()+i);
            SubElements.erase(SubElements.begin()+i);

            const std::vector< Constraint * > &constraints = Constraints.getValues();
            std::vector< Constraint * > newConstraints(0);
            int GeoId = GeoEnum::RefExt - i;
            for (std::vector<Constraint *>::const_iterator it = constraints.begin();
                 it != constraints.end(); ++it) {
                if ((*it)->First != GeoId && (*it)->Second != GeoId && (*it)->Third != GeoId) {
                    Constraint *copiedConstr = (*it)->clone();
                    if (copiedConstr->First < GeoId &&
                        copiedConstr->First != Constraint::GeoUndef)
                        copiedConstr->First += 1;
                    if (copiedConstr->Second < GeoId &&
                        copiedConstr->Second != Constraint::GeoUndef)
                        copiedConstr->Second += 1;
                    if (copiedConstr->Third < GeoId &&
                        copiedConstr->Third != Constraint::GeoUndef)
                        copiedConstr->Third += 1;

                    newConstraints.push_back(copiedConstr);
                }
            }

            Constraints.setValues(newConstraints);
            for (Constraint* it : newConstraints)
                delete it;
            i--; // we deleted an item, so the next one took its place
        }
    }

    if (rebuild) {
        ExternalGeometry.setValues(Objects,SubElements);
        rebuildExternalGeometry();
        Constraints.acceptGeometry(getCompleteGeometry());
        rebuildVertexIndex();
        solve(true); // we have to update this sketch and everything depending on it.
    }
}

void SketchObject::rebuildExternalGeometry(void)
{
    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();

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

    for (std::vector<Part::Geometry *>::iterator it=ExternalGeo.begin(); it != ExternalGeo.end(); ++it)
        if (*it) delete *it;
    ExternalGeo.clear();
    Part::GeomLineSegment *HLine = new Part::GeomLineSegment();
    Part::GeomLineSegment *VLine = new Part::GeomLineSegment();
    HLine->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
    VLine->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(0,1,0));
    HLine->Construction = true;
    VLine->Construction = true;
    ExternalGeo.push_back(HLine);
    ExternalGeo.push_back(VLine);
    for (int i=0; i < int(Objects.size()); i++) {
        const App::DocumentObject *Obj=Objects[i];
        const std::string SubElement=SubElements[i];

        TopoDS_Shape refSubShape;

        if (Obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
            const Part::Datum* datum = static_cast<const Part::Datum*>(Obj);
            refSubShape = datum->getShape();
        } else if (Obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            try {
                const Part::Feature *refObj=static_cast<const Part::Feature*>(Obj);
                const Part::TopoShape& refShape=refObj->Shape.getShape();
                refSubShape = refShape.getSubShape(SubElement.c_str());
            }
            catch (Standard_Failure& e) {
                throw Base::CADKernelError(e.GetMessageString());
            }
        } else  if (Obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
            const App::Plane* pl = static_cast<const App::Plane*>(Obj);
            Base::Placement plm = pl->Placement.getValue();
            Base::Vector3d base = plm.getPosition();
            Base::Rotation rot = plm.getRotation();
            Base::Vector3d normal(0,0,1);
            rot.multVec(normal, normal);
            gp_Pln plane(gp_Pnt(base.x,base.y,base.z), gp_Dir(normal.x, normal.y, normal.z));
            BRepBuilderAPI_MakeFace fBuilder(plane);
            if (!fBuilder.IsDone())
                throw Base::RuntimeError("Sketcher: addExternal(): Failed to build face from App::Plane");

            TopoDS_Face f = TopoDS::Face(fBuilder.Shape());
            refSubShape = f;
        } else {
            throw Base::TypeError("Datum feature type is not yet supported as external geometry for a sketch");
        }

        switch (refSubShape.ShapeType())
        {
        case TopAbs_FACE:
            {
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
                                ExternalGeo.push_back(projectLine(curve, gPlane, invPlm));
                            }
                        }

                    } else {
                        throw Base::ValueError("Selected external reference plane must be normal to sketch plane");
                    }
                } else {
                    throw Base::ValueError("Non-planar faces are not yet supported for external geometry of sketches");
                }
            }
            break;
        case TopAbs_EDGE:
            {
                const TopoDS_Edge& edge = TopoDS::Edge(refSubShape);
                BRepAdaptor_Curve curve(edge);
                if (curve.GetType() == GeomAbs_Line) {
                    ExternalGeo.push_back(projectLine(curve, gPlane, invPlm));
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
                            ExternalGeo.push_back(gCircle);
                        }
                        else {
                            Part::GeomArcOfCircle* gArc = new Part::GeomArcOfCircle();
                            Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
                            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(hCircle, curve.FirstParameter(),
                                                                                    curve.LastParameter());
                            gArc->setHandle(tCurve);
                            gArc->Construction = true;
                            ExternalGeo.push_back(gArc);
                        }
                    }
                    else {
                        // creates an ellipse
                        throw Base::NotImplementedError("Not yet supported geometry for external geometry");
                    }
                }
                else {
                    try {
                        BRepOffsetAPI_NormalProjection mkProj(aProjFace);
                        mkProj.Add(edge);
                        mkProj.Build();
                        const TopoDS_Shape& projShape = mkProj.Projection();
                        if (!projShape.IsNull()) {
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
                                        ExternalGeo.push_back(point);
                                    }
                                    else {
                                        Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                        line->setPoints(p1,p2);
                                        line->Construction = true;
                                        ExternalGeo.push_back(line);
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
                                        ExternalGeo.push_back(circle);
                                    }
                                    else {
                                        Part::GeomArcOfCircle* arc = new Part::GeomArcOfCircle();
                                        Handle(Geom_Curve) curve = new Geom_Circle(c);
                                        Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                                projCurve.LastParameter());
                                        arc->setHandle(tCurve);
                                        arc->Construction = true;
                                        ExternalGeo.push_back(arc);
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
                                        ExternalGeo.push_back(circle);
                                    } else {
                                        Part::GeomBSplineCurve* bspline = new Part::GeomBSplineCurve(projCurve.BSpline());
                                        bspline->Construction = true;
                                        ExternalGeo.push_back(bspline);
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
                                        ExternalGeo.push_back(hyperbola);
                                    }
                                    else {
                                        Part::GeomArcOfHyperbola* aoh = new Part::GeomArcOfHyperbola();
                                        Handle(Geom_Curve) curve = new Geom_Hyperbola(e);
                                        Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                                projCurve.LastParameter());
                                        aoh->setHandle(tCurve);
                                        aoh->Construction = true;
                                        ExternalGeo.push_back(aoh);
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
                                        ExternalGeo.push_back(parabola);
                                    }
                                    else {
                                        Part::GeomArcOfParabola* aop = new Part::GeomArcOfParabola();
                                        Handle(Geom_Curve) curve = new Geom_Parabola(e);
                                        Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                                projCurve.LastParameter());
                                        aop->setHandle(tCurve);
                                        aop->Construction = true;
                                        ExternalGeo.push_back(aop);
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
                                        ExternalGeo.push_back(ellipse);
                                    }
                                    else {
                                        Part::GeomArcOfEllipse* aoe = new Part::GeomArcOfEllipse();
                                        Handle(Geom_Curve) curve = new Geom_Ellipse(e);
                                        Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                                projCurve.LastParameter());
                                        aoe->setHandle(tCurve);
                                        aoe->Construction = true;
                                        ExternalGeo.push_back(aoe);
                                    }
                                }
                                else {
                                    throw Base::NotImplementedError("Not yet supported geometry for external geometry");
                                }
                            }
                        }
                    }
                    catch (Standard_Failure& e) {
                        throw Base::CADKernelError(e.GetMessageString());
                    }
                }
            }
            break;
        case TopAbs_VERTEX:
            {
                gp_Pnt P = BRep_Tool::Pnt(TopoDS::Vertex(refSubShape));
                GeomAPI_ProjectPointOnSurf proj(P,gPlane);
                P = proj.NearestPoint();
                Base::Vector3d p(P.X(),P.Y(),P.Z());
                invPlm.multVec(p,p);

                Part::GeomPoint* point = new Part::GeomPoint(p);
                point->Construction = true;
                ExternalGeo.push_back(point);
            }
            break;
        default:
            throw Base::TypeError("Unknown type of geometry");
            break;
        }
    }

    rebuildVertexIndex();
}

std::vector<Part::Geometry*> SketchObject::getCompleteGeometry(void) const
{
    std::vector<Part::Geometry*> vals=getInternalGeometry();
    vals.insert(vals.end(), ExternalGeo.rbegin(), ExternalGeo.rend()); // in reverse order
    return vals;
}

void SketchObject::rebuildVertexIndex(void)
{
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
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
        } else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
        } else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
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

    std::set<App::ObjectIdentifier> deps;
    expr->getDeps(deps);

    for (std::set<App::ObjectIdentifier>::const_iterator i = deps.begin(); i != deps.end(); ++i) {
        const App::Property * prop = (*i).getProperty();

        if (prop == &Constraints) {
            const Constraint * constraint = Constraints.getConstraint(*i);

            if (!constraint->isDriving)
                return "Reference constraint from this sketch cannot be used in this expression.";
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
        ExpressionEngine.setValue(*i, boost::shared_ptr<App::Expression>(), 0);
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

void SketchObject::onChanged(const App::Property* prop)
{
    if (isRestoring() && prop == &Geometry) {
        std::vector<Part::Geometry*> geom = Geometry.getValues();
        std::vector<Part::Geometry*> supportedGeom = supportedGeometry(geom);
        // To keep upward compatibility ignore unsupported geometry types
        if (supportedGeom.size() != geom.size()) {
            Geometry.setValues(supportedGeom);
            return;
        }
    }
    if (prop == &Geometry || prop == &Constraints) {
        Constraints.checkGeometry(getCompleteGeometry());
    }
    else if (prop == &ExternalGeometry) {
        // make sure not to change anything while restoring this object
        if (!isRestoring()) {
            // external geometry was cleared
            if (ExternalGeometry.getSize() == 0) {
                delConstraintsToExternal();
            }
        }
    }
#if 0
    // For now do not delete anything (#0001791). When changing the support
    // face it might be better to check which external geometries can be kept.
    else if (prop == &Support) {
        // make sure not to change anything while restoring this object
        if (!isRestoring()) {
            // if support face has changed then clear the external geometry
            delConstraintsToExternal();
            for (int i=0; i < getExternalGeometryCount(); i++) {
                delExternal(0);
            }
        }
    }
#endif
    Part::Part2DObject::onChanged(prop);
}

void SketchObject::onDocumentRestored()
{
    try {
        validateExternalLinks();
        rebuildExternalGeometry();
        Constraints.acceptGeometry(getCompleteGeometry());
        // this may happen when saving a sketch directly in edit mode
        // but never performed a recompute before
        if (Shape.getValue().IsNull() && hasConflicts() == 0) {
            if (this->solve(true) == 0)
                Shape.setValue(solvedSketch.toShape());
        }

        Part::Part2DObject::onDocumentRestored();
    }
    catch (...) {
    }
}

void SketchObject::restoreFinished()
{
    try {
        Constraints.acceptGeometry(getCompleteGeometry());
        // this may happen when saving a sketch directly in edit mode
        // but never performed a recompute before
        if (Shape.getValue().IsNull() && hasConflicts() == 0) {
            if (this->solve(true) == 0)
                Shape.setValue(solvedSketch.toShape());
        }
    }
    catch (...) {
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
    for(std::size_t i=0;i<VertexId2GeoId.size();i++) {
        if(VertexId2GeoId[i]==GeoId && VertexId2PosId[i]==PosId)
            return i;
    }

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

void SketchObject::setExpression(const App::ObjectIdentifier &path, boost::shared_ptr<App::Expression> expr, const char * comment)
{
    DocumentObject::setExpression(path, expr, comment);

    if(noRecomputes) // if we do not have a recompute, the sketch must be solved to update the DoF of the solver, constraints and UI
        solve();
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

std::vector<Base::Vector3d> SketchObject::getOpenVertices(void) const
{
    std::vector<Base::Vector3d> points;

    if(analyser)
        points = analyser->getOpenVertices();

    return points;
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
