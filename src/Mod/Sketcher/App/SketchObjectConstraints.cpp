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

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

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
            break;
        }
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

            break;
        }
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

            break;
        }
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
        }
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
                                               std::vector<PointPos>& PosIdList)
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
                                               std::vector<PointPos>& PosIdList)
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
