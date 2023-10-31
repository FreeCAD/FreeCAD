/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <cmath>
#include <vector>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepOffsetAPI_NormalProjection.hxx>
#include <BRep_Tool.hxx>
#include <GC_MakeCircle.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomConvert_BSplineCurveKnotSplitting.hxx>
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
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/ElementNamingUtils.h>
#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <App/FeaturePythonPyImp.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <App/ObjectIdentifier.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Base/Writer.h>
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/GeometryMigrationExtension.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "SketchObject.h"
#include "SketchObjectPy.h"
#include "SolverGeometryExtension.h"


#undef DEBUG
// #define DEBUG

// clang-format off
using namespace Sketcher;
using namespace Base;
namespace sp = std::placeholders;

FC_LOG_LEVEL_INIT("Sketch", true, true)

PROPERTY_SOURCE(Sketcher::SketchObject, Part::Part2DObject)

SketchObject::SketchObject()
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
                      (App::PropertyType)(App::Prop_None),
                      "Sketch external geometry");
    ADD_PROPERTY_TYPE(FullyConstrained,
                      (false),
                      "Sketch",
                      (App::PropertyType)(App::Prop_Output | App::Prop_ReadOnly | App::Prop_Hidden),
                      "Sketch is fully constrained");

    Geometry.setOrderRelevant(true);

    allowOtherBody = true;
    allowUnaligned = true;

    for (std::vector<Part::Geometry*>::iterator it = ExternalGeo.begin(); it != ExternalGeo.end();
         ++it)
        if (*it)
            delete *it;
    ExternalGeo.clear();
    auto HLine = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade();
    auto VLine = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade();
    HLine->getTypedGeometry()->setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(1, 0, 0));
    VLine->getTypedGeometry()->setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(0, 1, 0));
    HLine->setConstruction(true);
    VLine->setConstruction(true);
    ExternalGeo.push_back(HLine->getGeometry());
    ExternalGeo.push_back(VLine->getGeometry());
    HLine->setOwner(false);// we have transferred the ownership to ExternalGeo
    VLine->setOwner(false);// we have transferred the ownership to ExternalGeo
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
}

SketchObject::~SketchObject()
{
    for (std::vector<Part::Geometry*>::iterator it = ExternalGeo.begin(); it != ExternalGeo.end();
         ++it)
        if (*it)
            delete *it;
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
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\nClear constraints to external geometry\n", e.what());
        // we cannot trust the constraints of external geometries, so remove them
        delConstraintsToExternal();
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
    Shape.setValue(solvedSketch.toShape());

    return App::DocumentObject::StdReturn;
}

void SketchObject::buildShape()
{
    // Shape.setValue(solvedSketch.toShape());
    // We use the following instead to map element names

    std::vector<Part::TopoShape> shapes;
    std::vector<Part::TopoShape> vertices;
    int i=0;
    for(auto geo : getInternalGeometry()) {
        ++i;
        if(GeometryFacade::getConstruction(geo))
            continue;
        if (geo->isDerivedFrom(Part::GeomPoint::getClassTypeId())) {
            vertices.emplace_back(TopoDS::Vertex(geo->toShape()));
            int idx = getVertexIndexGeoPos(i-1, PointPos::start);
            std::string name = convertSubName(Data::IndexedName::fromConst("Vertex", idx+1), false);
            // vertices.back().setElementName(Data::IndexedName::fromConst("Vertex", 1),
            //                                Data::MappedName::fromRawData(name.c_str()));
        } else
            shapes.push_back(getEdge(geo,convertSubName(
                        Data::IndexedName::fromConst("Edge", i), false).c_str()));
    }

    // FIXME: Commented since ExternalGeometryFacade is not added
    // for(i=2;i<ExternalGeo.getSize();++i) {
    //     auto geo = ExternalGeo[i];
    //     auto egf = ExternalGeometryFacade::getFacade(geo);
    //     if(!egf->testFlag(ExternalGeometryExtension::Defining))
    //         continue;
    //     shapes.push_back(getEdge(geo, convertSubName(
    //                     Data::IndexedName::fromConst("ExternalEdge", i-1), false).c_str()));
    // }
    // if(shapes.empty() && vertices.empty())
    //     Shape.setValue(Part::TopoShape());
    // else if (vertices.empty()) {
    //     // Notice here we supply op code Part::OpCodes::Sketch to makEWires().
    //     Shape.setValue(Part::TopoShape().makEWires(shapes,Part::OpCodes::Sketch));
    // } else {
    //     std::vector<Part::TopoShape> results;
    //     if (!shapes.empty()) {
    //         // This call of makEWires() does not have the op code, in order to
    //         // avoid duplication. Because we'll going to make a compound (to
    //         // include the vertices) below with the same op code.
    //         //
    //         // Note, that we HAVE TO add the Part::OpCodes::Sketch op code to all
    //         // geometry exposed through the Shape property, because
    //         // SketchObject::getElementName() relies on this op code to
    //         // differentiate geometries that are exposed with those in edit
    //         // mode.
    //         auto wires = Part::TopoShape().makEWires(shapes);
    //         for (const auto &wire : wires.getSubTopoShapes(TopAbs_WIRE))
    //             results.push_back(wire);
    //     }
    //     results.insert(results.end(), vertices.begin(), vertices.end());
    //     Shape.setValue(Part::TopoShape().makECompound(results, Part::OpCodes::Sketch));
    // }
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
        Base::Console().Error(
            this->getFullLabel(),
            QT_TRANSLATE_NOOP("Notifications", "The Sketch has malformed constraints!") "\n");
    }

    if (lastHasPartialRedundancies) {
        Base::Console().Warning(
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
    }

    if (err == 0 && updateGeoAfterSolving) {
        // set the newly solved geometry
        std::vector<Part::Geometry*> geomlist = solvedSketch.extractGeometry();
        Geometry.setValues(geomlist);
        for (std::vector<Part::Geometry*>::iterator it = geomlist.begin(); it != geomlist.end();
             ++it)
            if (*it)
                delete *it;
    }
    else if (err < 0) {
        // if solver failed, invalid constraints were likely added before solving
        // (see solve in addConstraint), so solver information is definitely invalid.
        this->Constraints.touch();
    }

    return err;
}

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

    if ((type == Distance || type == Radius || type == Diameter || type == Weight) && Datum <= 0)
        return (Datum == 0) ? -5 : -4;

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
        setConstraintExpression(constNum, reverseAngleConstraintExpression(expression));
    }
    else {
        double actAngle = constr->getValue();
        constr->setValue(M_PI - actAngle);
    }
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
            setExpression(path, expr);
        }
        catch (const Base::Exception&) {
            Base::Console().Error("Failed to set constraint expression.");
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

    std::copy(objectconstraints.begin(), objectconstraints.end(), back_inserter(allconstraints));
    std::copy(
        additionalconstraints.begin(), additionalconstraints.end(), back_inserter(allconstraints));

    lastDoF =
        solvedSketch.setUpSketch(getCompleteGeometry(), allconstraints, getExternalGeometryCount());

    retrieveSolverDiagnostics();

    return lastDoF;
}

int SketchObject::movePoint(int GeoId, PointPos PosId, const Base::Vector3d& toPoint, bool relative,
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
    lastSolverStatus = solvedSketch.movePoint(GeoId, PosId, toPoint, relative);

    // moving the point can not result in a conflict that we did not have
    // or a redundancy that we did not have before, or a change of DoF

    if (lastSolverStatus == 0) {
        std::vector<Part::Geometry*> geomlist = solvedSketch.extractGeometry();
        Geometry.setValues(geomlist);
        // Constraints.acceptGeometry(getCompleteGeometry());
        for (std::vector<Part::Geometry*>::iterator it = geomlist.begin(); it != geomlist.end();
             ++it) {
            if (*it)
                delete *it;
        }
    }

    solvedSketch.resetInitMove();// reset solver point moving mechanism

    return lastSolverStatus;
}

Base::Vector3d SketchObject::getPoint(const Part::Geometry *geo, PointPos PosId)
{
    if (geo->is<Part::GeomPoint>()) {
        const Part::GeomPoint *p = static_cast<const Part::GeomPoint*>(geo);
        if (PosId == PointPos::start || PosId == PointPos::mid || PosId == PointPos::end)
            return p->getPoint();
    } else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment*>(geo);
        if (PosId == PointPos::start)
            return lineSeg->getStartPoint();
        else if (PosId == PointPos::end)
            return lineSeg->getEndPoint();
    } else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle *circle = static_cast<const Part::GeomCircle*>(geo);
        auto pt = circle->getCenter();
        if(PosId != PointPos::mid)
             pt.x += circle->getRadius();
        return pt;
    } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse*>(geo);
        auto pt = ellipse->getCenter();
        if(PosId != PointPos::mid)
            pt += ellipse->getMajorAxisDir()*ellipse->getMajorRadius();
        return pt;
    } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle *aoc = static_cast<const Part::GeomArcOfCircle*>(geo);
        if (PosId == PointPos::start)
            return aoc->getStartPoint(/*emulateCCW=*/true);
        else if (PosId == PointPos::end)
            return aoc->getEndPoint(/*emulateCCW=*/true);
        else if (PosId == PointPos::mid)
            return aoc->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        const Part::GeomArcOfEllipse *aoc = static_cast<const Part::GeomArcOfEllipse*>(geo);
        if (PosId == PointPos::start)
            return aoc->getStartPoint(/*emulateCCW=*/true);
        else if (PosId == PointPos::end)
            return aoc->getEndPoint(/*emulateCCW=*/true);
        else if (PosId == PointPos::mid)
            return aoc->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
        const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);
        if (PosId == PointPos::start)
            return aoh->getStartPoint();
        else if (PosId == PointPos::end)
            return aoh->getEndPoint();
        else if (PosId == PointPos::mid)
            return aoh->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
        const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola*>(geo);
        if (PosId == PointPos::start)
            return aop->getStartPoint();
        else if (PosId == PointPos::end)
            return aop->getEndPoint();
        else if (PosId == PointPos::mid)
            return aop->getCenter();
    } else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
        const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
        if (PosId == PointPos::start)
            return bsp->getStartPoint();
        else if (PosId == PointPos::end)
            return bsp->getEndPoint();
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
    for (std::vector<Part::Geometry*>::const_iterator geo = vals.begin(); geo != vals.end(); geo++)
        if ((*geo) && GeometryFacade::getConstruction(*geo)
            && (*geo)->getTypeId() == Part::GeomLineSegment::getClassTypeId())
            count++;

    return count;
}

Base::Axis SketchObject::getAxis(int axId) const
{
    if (axId == H_Axis || axId == V_Axis || axId == N_Axis)
        return Part::Part2DObject::getAxis(axId);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    int count = 0;
    for (std::vector<Part::Geometry*>::const_iterator geo = vals.begin(); geo != vals.end(); geo++)
        if ((*geo) && GeometryFacade::getConstruction(*geo)
            && (*geo)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            if (count == axId) {
                Part::GeomLineSegment* lineSeg = static_cast<Part::GeomLineSegment*>(*geo);
                Base::Vector3d start = lineSeg->getStartPoint();
                Base::Vector3d end = lineSeg->getEndPoint();
                return Base::Axis(start, end - start);
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

bool SketchObject::isSupportedGeometry(const Part::Geometry* geo) const
{
    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId()
        || geo->getTypeId() == Part::GeomCircle::getClassTypeId()
        || geo->getTypeId() == Part::GeomEllipse::getClassTypeId()
        || geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
        || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
        || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
        || geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()
        || geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()
        || geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
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

std::vector<Part::Geometry*>
SketchObject::supportedGeometry(const std::vector<Part::Geometry*>& geoList) const
{
    std::vector<Part::Geometry*> supportedGeoList;
    supportedGeoList.reserve(geoList.size());
    // read-in geometry that the sketcher cannot handle
    for (std::vector<Part::Geometry*>::const_iterator it = geoList.begin(); it != geoList.end();
         ++it) {
        if (isSupportedGeometry(*it)) {
            supportedGeoList.push_back(*it);
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

        if (copy->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            // creation mode for points is always construction not to
            // break legacy code
            GeometryFacade::setConstruction(copy, true);
        }
        else if (construction) {
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

    if (geoNew->getTypeId() == Part::GeomPoint::getClassTypeId()) {
        // creation mode for points is always construction not to
        // break legacy code
        GeometryFacade::setConstruction(geoNew, true);
    }
    else if (construction) {
        GeometryFacade::setConstruction(geoNew, construction);
    }

    newVals.push_back(geoNew);

    // On setting geometry the onChanged method will call acceptGeometry(), thereby updating
    // constraint geometry indices and rebuilding the vertex index
    Geometry.setValues(std::move(newVals));

    return Geometry.getSize() - 1;
}

int SketchObject::delGeometry(int GeoId, bool deleteinternalgeo)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    if (deleteinternalgeo) {
        const Part::Geometry* geo = getGeometry(GeoId);
        // Only for supported types
        if ((geo->getTypeId() == Part::GeomEllipse::getClassTypeId()
             || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
             || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
             || geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()
             || geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId())) {

            this->deleteUnusedInternalGeometry(GeoId, true);

            return 0;
        }
    }

    std::vector<Part::Geometry*> newVals(vals);
    newVals.erase(newVals.begin() + GeoId);

    // Find coincident points to replace the points of the deleted geometry
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    for (PointPos PosId = PointPos::start; PosId != PointPos::mid;) {
        getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
        if (GeoIdList.size() > 1) {
            delConstraintOnPoint(GeoId, PosId, true /* only coincidence */);
            transferConstraints(GeoIdList[0], PosIdList[0], GeoIdList[1], PosIdList[1]);
        }
        // loop through [start, end, mid]
        PosId = (PosId == PointPos::start) ? PointPos::end : PointPos::mid;
    }

    const std::vector<Constraint*>& constraints = this->Constraints.getValues();
    std::vector<Constraint*> newConstraints;
    newConstraints.reserve(constraints.size());
    for (auto cstr : constraints) {
        if (cstr->First == GeoId || cstr->Second == GeoId || cstr->Third == GeoId)
            continue;
        if (cstr->First > GeoId || cstr->Second > GeoId || cstr->Third > GeoId) {
            cstr = cstr->clone();
            if (cstr->First > GeoId)
                cstr->First -= 1;
            if (cstr->Second > GeoId)
                cstr->Second -= 1;
            if (cstr->Third > GeoId)
                cstr->Third -= 1;
        }
        newConstraints.push_back(cstr);
    }

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker lock(internaltransaction, true);
        this->Geometry.setValues(std::move(newVals));
        this->Constraints.setValues(std::move(newConstraints));
    }

    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}


int SketchObject::delGeometries(const std::vector<int>& GeoIds)
{
    std::vector<int> sGeoIds(GeoIds);

    // if a GeoId has internal geometry, it must delete internal geometries too
    for (auto c : Constraints.getValues()) {
        if (c->Type == InternalAlignment) {
            auto pos = std::find(sGeoIds.begin(), sGeoIds.end(), c->Second);

            if (pos != sGeoIds.end()) {
                sGeoIds.push_back(c->First);
            }
        }
    }

    std::sort(sGeoIds.begin(), sGeoIds.end());
    // eliminate duplicates
    auto newend = std::unique(sGeoIds.begin(), sGeoIds.end());
    sGeoIds.resize(std::distance(sGeoIds.begin(), newend));

    return delGeometriesExclusiveList(sGeoIds);
}

int SketchObject::delGeometriesExclusiveList(const std::vector<int>& GeoIds)
{
    std::vector<int> sGeoIds(GeoIds);

    std::sort(sGeoIds.begin(), sGeoIds.end());
    if (sGeoIds.empty())
        return 0;

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    if (sGeoIds.front() < 0 || sGeoIds.back() >= int(vals.size()))
        return -1;


    std::vector<Part::Geometry*> newVals(vals);
    for (auto it = sGeoIds.rbegin(); it != sGeoIds.rend(); ++it) {
        int GeoId = *it;
        newVals.erase(newVals.begin() + GeoId);

        // Find coincident points to replace the points of the deleted geometry
        std::vector<int> GeoIdList;
        std::vector<PointPos> PosIdList;
        for (PointPos PosId = PointPos::start; PosId != PointPos::mid;) {
            getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
            if (GeoIdList.size() > 1) {
                delConstraintOnPoint(GeoId, PosId, true /* only coincidence */);
                transferConstraints(GeoIdList[0], PosIdList[0], GeoIdList[1], PosIdList[1]);
            }
            // loop through [start, end, mid]
            PosId = (PosId == PointPos::start) ? PointPos::end : PointPos::mid;
        }
    }

    // Copy the original constraints
    std::vector<Constraint*> constraints;
    for (const auto ptr : this->Constraints.getValues())
        constraints.push_back(ptr->clone());
    std::vector<Constraint*> filteredConstraints(0);
    for (auto itGeo = sGeoIds.rbegin(); itGeo != sGeoIds.rend(); ++itGeo) {
        int GeoId = *itGeo;
        for (std::vector<Constraint*>::const_iterator it = constraints.begin();
             it != constraints.end();
             ++it) {

            Constraint* copiedConstr(*it);
            if ((*it)->First != GeoId && (*it)->Second != GeoId && (*it)->Third != GeoId) {
                if (copiedConstr->First > GeoId)
                    copiedConstr->First -= 1;
                if (copiedConstr->Second > GeoId)
                    copiedConstr->Second -= 1;
                if (copiedConstr->Third > GeoId)
                    copiedConstr->Third -= 1;
                filteredConstraints.push_back(copiedConstr);
            }
            else {
                delete copiedConstr;
            }
        }

        constraints = filteredConstraints;
        filteredConstraints.clear();
    }

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker lock(internaltransaction, true);
        this->Geometry.setValues(newVals);
        this->Constraints.setValues(std::move(constraints));
    }
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::deleteAllGeometry()
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<Part::Geometry*> newVals(0);
    std::vector<Constraint*> newConstraints(0);

    // Avoid unnecessary updates and checks as this is a transaction
    {
        Base::StateLocker lock(internaltransaction, true);
        this->Geometry.setValues(newVals);
        this->Constraints.setValues(newConstraints);
    }
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::deleteAllConstraints()
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<Constraint*> newConstraints(0);

    this->Constraints.setValues(newConstraints);

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::toggleConstruction(int GeoId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    if (getGeometryFacade(GeoId)->isInternalAligned())
        return -1;

    // While it may seem that there is not a need to trigger an update at this time, because the
    // solver has its own copy of the geometry, and updateColors of the viewprovider may be
    // triggered by the clearselection of the UI command, this won't update the elements widget, in
    // the accumulative of actions it is judged that it is worth to trigger an update here.

    std::unique_ptr<Part::Geometry> geo(vals[GeoId]->clone());
    auto gft = GeometryFacade::getFacade(geo.get());
    gft->setConstruction(!gft->getConstruction());
    this->Geometry.set1Value(GeoId, std::move(geo));

    solverNeedsUpdate = true;
    return 0;
}

int SketchObject::setConstruction(int GeoId, bool on)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    if (getGeometryFacade(GeoId)->isInternalAligned())
        return -1;

    // While it may seem that there is not a need to trigger an update at this time, because the
    // solver has its own copy of the geometry, and updateColors of the viewprovider may be
    // triggered by the clearselection of the UI command, this won't update the elements widget, in
    // the accumulative of actions it is judged that it is worth to trigger an update here.

    std::unique_ptr<Part::Geometry> geo(vals[GeoId]->clone());
    GeometryFacade::setConstruction(geo.get(), on);
    this->Geometry.set1Value(GeoId, std::move(geo));

    solverNeedsUpdate = true;
    return 0;
}

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

int SketchObject::delConstraint(int ConstrId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;

    std::vector<Constraint*> newVals(vals);
    auto ctriter = newVals.begin() + ConstrId;
    removeGeometryState(*ctriter);
    newVals.erase(ctriter);
    this->Constraints.setValues(std::move(newVals));

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::delConstraints(std::vector<int> ConstrIds, bool updategeometry)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);
    if (ConstrIds.empty())
        return 0;

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
    if (noRecomputes)
        solve(updategeometry);

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

int SketchObject::delConstraintOnPoint(int GeoId, PointPos PosId, bool onlyCoincident)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();

    // check if constraints can be redirected to some other point
    int replaceGeoId = GeoEnum::GeoUndef;
    PointPos replacePosId = Sketcher::PointPos::none;
    if (!onlyCoincident) {
        for (std::vector<Constraint*>::const_iterator it = vals.begin(); it != vals.end(); ++it) {
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
    std::vector<Constraint*> newVals(0);
    for (std::vector<Constraint*>::const_iterator it = vals.begin(); it != vals.end(); ++it) {
        if ((*it)->Type == Sketcher::Coincident) {
            if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                // redirect this constraint
                if (replaceGeoId != GeoEnum::GeoUndef
                    && (replaceGeoId != (*it)->Second || replacePosId != (*it)->SecondPos)) {
                    (*it)->First = replaceGeoId;
                    (*it)->FirstPos = replacePosId;
                }
                else
                    continue;// skip this constraint
            }
            else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                // redirect this constraint
                if (replaceGeoId != GeoEnum::GeoUndef
                    && (replaceGeoId != (*it)->First || replacePosId != (*it)->FirstPos)) {
                    (*it)->Second = replaceGeoId;
                    (*it)->SecondPos = replacePosId;
                }
                else
                    continue;// skip this constraint
            }
        }
        else if (!onlyCoincident) {
            if ((*it)->Type == Sketcher::Distance || (*it)->Type == Sketcher::DistanceX
                || (*it)->Type == Sketcher::DistanceY) {
                if ((*it)->First == GeoId && (*it)->FirstPos == PointPos::none
                    && (PosId == PointPos::start || PosId == PointPos::end)) {
                    // remove the constraint even if it is not directly associated
                    // with the given point
                    continue;// skip this constraint
                }
                else if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                    if (replaceGeoId != GeoEnum::GeoUndef) {// redirect this constraint
                        (*it)->First = replaceGeoId;
                        (*it)->FirstPos = replacePosId;
                    }
                    else
                        continue;// skip this constraint
                }
                else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                    if (replaceGeoId != GeoEnum::GeoUndef) {// redirect this constraint
                        (*it)->Second = replaceGeoId;
                        (*it)->SecondPos = replacePosId;
                    }
                    else
                        continue;// skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::PointOnObject) {
                if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                    if (replaceGeoId != GeoEnum::GeoUndef) {// redirect this constraint
                        (*it)->First = replaceGeoId;
                        (*it)->FirstPos = replacePosId;
                    }
                    else
                        continue;// skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::Tangent || (*it)->Type == Sketcher::Perpendicular) {
                if (((*it)->First == GeoId && (*it)->FirstPos == PosId)
                    || ((*it)->Second == GeoId && (*it)->SecondPos == PosId)) {
                    // we could keep the tangency constraint by converting it
                    // to a simple one but it is not really worth
                    continue;// skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::Symmetric) {
                if (((*it)->First == GeoId && (*it)->FirstPos == PosId)
                    || ((*it)->Second == GeoId && (*it)->SecondPos == PosId)) {
                    continue;// skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::Vertical || (*it)->Type == Sketcher::Horizontal) {
                if (((*it)->First == GeoId && (*it)->FirstPos == PosId)
                    || ((*it)->Second == GeoId && (*it)->SecondPos == PosId)) {
                    continue;// skip this constraint
                }
            }
        }
        newVals.push_back(*it);
    }
    if (newVals.size() < vals.size()) {
        this->Constraints.setValues(std::move(newVals));

        return 0;
    }

    return -1;// no such constraint
}

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
        delConstraints(deleteme, false);
        return;
    }

    // If the lines aren't straight, don't try to transfer the constraints.
    // TODO: Add support for curved lines.
    const Part::Geometry* geo1 = getGeometry(geoId1);
    const Part::Geometry* geo2 = getGeometry(geoId2);
    if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId()
        || geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
        delConstraintOnPoint(geoId1, posId1, false);
        delConstraintOnPoint(geoId2, posId2, false);
        return;
    }

    // Add a vertex to preserve the original intersection of the filleted lines
    Part::GeomPoint* originalCorner = new Part::GeomPoint(getPoint(geoId1, posId1));
    int originalCornerId = addGeometry(originalCorner);
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

int SketchObject::transferConstraints(int fromGeoId, PointPos fromPosId, int toGeoId,
                                      PointPos toPosId, bool doNotTransformTangencies)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& vals = this->Constraints.getValues();
    std::vector<Constraint*> newVals(vals);
    bool changed = false;
    for (int i = 0; i < int(newVals.size()); i++) {
        if (vals[i]->First == fromGeoId && vals[i]->FirstPos == fromPosId
            && !(vals[i]->Second == toGeoId && vals[i]->SecondPos == toPosId)
            && !(toGeoId < 0 && vals[i]->Second < 0)) {

            std::unique_ptr<Constraint> constNew(newVals[i]->clone());
            constNew->First = toGeoId;
            constNew->FirstPos = toPosId;

            // If not explicitly confirmed, nothing guarantees that a tangent can be freely
            // transferred to another coincident point, as the transfer destination edge most likely
            // won't be intended to be tangent. However, if it is an end to end point tangency, the
            // user expects it to be substituted by a coincidence constraint.
            if (vals[i]->Type == Sketcher::Tangent || vals[i]->Type == Sketcher::Perpendicular) {
                if (!doNotTransformTangencies) {
                    constNew->Type = Sketcher::Coincident;
                }
            }
            // With respect to angle constraints, if it is a DeepSOIC style angle constraint
            // (segment+segment+point), then no problem arises as the segments are PosId=none. In
            // this case there is no call to this function.
            //
            // However, other angle constraints are problematic because they are created on
            // segments, but internally operate on vertices, PosId=start Such constraint may not be
            // successfully transferred on deletion of the segments.
            else if (vals[i]->Type == Sketcher::Angle) {
                continue;
            }

            Constraint* constPtr = constNew.release();
            newVals[i] = constPtr;
            changed = true;
        }
        else if (vals[i]->Second == fromGeoId && vals[i]->SecondPos == fromPosId
                 && !(vals[i]->First == toGeoId && vals[i]->FirstPos == toPosId)
                 && !(toGeoId < 0 && vals[i]->First < 0)) {

            std::unique_ptr<Constraint> constNew(newVals[i]->clone());
            constNew->Second = toGeoId;
            constNew->SecondPos = toPosId;
            // If not explicitly confirmed, nothing guarantees that a tangent can be freely
            // transferred to another coincident point, as the transfer destination edge most likely
            // won't be intended to be tangent. However, if it is an end to end point tangency, the
            // user expects it to be substituted by a coincidence constraint.
            if (vals[i]->Type == Sketcher::Tangent || vals[i]->Type == Sketcher::Perpendicular) {
                if (!doNotTransformTangencies) {
                    constNew->Type = Sketcher::Coincident;
                }
            }
            else if (vals[i]->Type == Sketcher::Angle) {
                continue;
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

int SketchObject::fillet(int GeoId, PointPos PosId, double radius, bool trim, bool createCorner)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    // Find the other geometry Id associated with the coincident point
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);

    // only coincident points between two (non-external) edges can be filleted
    if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
        const Part::Geometry* geo1 = getGeometry(GeoIdList[0]);
        const Part::Geometry* geo2 = getGeometry(GeoIdList[1]);
        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()
            && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo1);
            const Part::GeomLineSegment* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);

            Base::Vector3d midPnt1 = (lineSeg1->getStartPoint() + lineSeg1->getEndPoint()) / 2;
            Base::Vector3d midPnt2 = (lineSeg2->getStartPoint() + lineSeg2->getEndPoint()) / 2;
            return fillet(GeoIdList[0], GeoIdList[1], midPnt1, midPnt2, radius, trim, createCorner);
        }
    }

    return -1;
}

int SketchObject::fillet(int GeoId1, int GeoId2, const Base::Vector3d& refPnt1,
                         const Base::Vector3d& refPnt2, double radius, bool trim, bool createCorner)
{
    if (GeoId1 < 0 || GeoId1 > getHighestCurveIndex() || GeoId2 < 0
        || GeoId2 > getHighestCurveIndex())
        return -1;

    // If either of the two input lines are locked, don't try to trim since it won't work anyway
    const Part::Geometry* geo1 = getGeometry(GeoId1);
    const Part::Geometry* geo2 = getGeometry(GeoId2);
    if (trim && (GeometryFacade::getBlocked(geo1) || GeometryFacade::getBlocked(geo2))) {
        trim = false;
    }

    if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()
        && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo1);
        const Part::GeomLineSegment* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);

        Base::Vector3d filletCenter;
        if (!Part::findFilletCenter(lineSeg1, lineSeg2, radius, refPnt1, refPnt2, filletCenter))
            return -1;
        Base::Vector3d dir1 = lineSeg1->getEndPoint() - lineSeg1->getStartPoint();
        Base::Vector3d dir2 = lineSeg2->getEndPoint() - lineSeg2->getStartPoint();

        // the intersection point will and two distances will be necessary later for trimming the
        // lines
        Base::Vector3d intersection, dist1, dist2;

        // create arc from known parameters and lines
        int filletId;
        std::unique_ptr<Part::GeomArcOfCircle> arc(
            Part::createFilletGeometry(lineSeg1, lineSeg2, filletCenter, radius));
        if (!arc)
            return -1;

        // calculate intersection and distances before we invalidate lineSeg1 and lineSeg2
        if (!find2DLinesIntersection(lineSeg1, lineSeg2, intersection)) {
            return -1;
        }

        dist1.ProjectToLine(arc->getStartPoint(/*emulateCCW=*/true) - intersection, dir1);
        dist2.ProjectToLine(arc->getStartPoint(/*emulateCCW=*/true) - intersection, dir2);
        Part::Geometry* newgeo = arc.get();
        filletId = addGeometry(newgeo);

        if (trim) {
            PointPos PosId1 =
                (filletCenter - intersection) * dir1 > 0 ? PointPos::start : PointPos::end;
            PointPos PosId2 =
                (filletCenter - intersection) * dir2 > 0 ? PointPos::start : PointPos::end;

            if (createCorner) {
                transferFilletConstraints(GeoId1, PosId1, GeoId2, PosId2);
            }
            else {
                delConstraintOnPoint(GeoId1, PosId1, false);
                delConstraintOnPoint(GeoId2, PosId2, false);
            }

            auto tangent1 = std::make_unique<Sketcher::Constraint>();
            auto tangent2 = std::make_unique<Sketcher::Constraint>();

            tangent1->Type = Sketcher::Tangent;
            tangent1->First = GeoId1;
            tangent1->FirstPos = PosId1;
            tangent1->Second = filletId;

            tangent2->Type = Sketcher::Tangent;
            tangent2->First = GeoId2;
            tangent2->FirstPos = PosId2;
            tangent2->Second = filletId;

            if (dist1.Length() < dist2.Length()) {
                tangent1->SecondPos = PointPos::start;
                tangent2->SecondPos = PointPos::end;
                movePoint(GeoId1, PosId1, arc->getStartPoint(/*emulateCCW=*/true), false, true);
                movePoint(GeoId2, PosId2, arc->getEndPoint(/*emulateCCW=*/true), false, true);
            }
            else {
                tangent1->SecondPos = PointPos::end;
                tangent2->SecondPos = PointPos::start;
                movePoint(GeoId1, PosId1, arc->getEndPoint(/*emulateCCW=*/true), false, true);
                movePoint(GeoId2, PosId2, arc->getStartPoint(/*emulateCCW=*/true), false, true);
            }

            addConstraint(std::move(tangent1));
            addConstraint(std::move(tangent2));
        }

        // if we do not have a recompute after the geometry creation, the sketch must be solved to
        // update the DoF of the solver
        if (noRecomputes)
            solve();

        return 0;
    }
    else if (geo1->isDerivedFrom<Part::GeomBoundedCurve>()
             && geo2->isDerivedFrom<Part::GeomBoundedCurve>()) {

        auto distancetorefpoints =
            [](Base::Vector3d ip1, Base::Vector3d ip2, Base::Vector3d ref1, Base::Vector3d ref2) {
                return (ip1 - ref1).Length() + (ip2 - ref2).Length();
            };

        auto selectintersection =
            [&distancetorefpoints](std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                                   std::pair<Base::Vector3d, Base::Vector3d>& interpoints,
                                   const Base::Vector3d& refPnt1,
                                   const Base::Vector3d& refPnt2) {
                if (points.empty()) {
                    return -1;
                }
                else {
                    double dist =
                        distancetorefpoints(points[0].first, points[0].second, refPnt1, refPnt2);
                    int i = 0, si = 0;

                    for (auto ipoints : points) {
                        double d =
                            distancetorefpoints(ipoints.first, ipoints.second, refPnt1, refPnt2);

                        if (d < dist) {
                            si = i;
                            dist = d;
                        }

                        i++;
                    }

                    interpoints = points[si];

                    return 0;
                }
            };

        // NOTE: While it is not a requirement that the endpoints of the corner to trim are
        // coincident
        //       for GeomTrimmedCurves, it is for GeomBoundedCurves. The reason is that there is no
        //       basiscurve that can be extended to find an intersection.
        //
        //       However, GeomTrimmedCurves sometimes run into problems when trying to calculate the
        //       intersection of basis curves, for example in the case of hyperbola sometimes the
        //       cosh goes out of range while calculating this intersection of basis curves.
        //
        //        Consequently:
        //        i. for GeomBoundedCurves, other than GeomTrimmedCurves, a coincident endpoint is
        //        mandatory. ii. for GeomTrimmedCurves, if there is a coincident endpoint, it is
        //        used for the fillet, iii. for GeomTrimmedCurves, if there is not a coincident
        //        endpoint, an intersection of basis curves
        //             is attempted.

        const Part::GeomCurve* curve1 = static_cast<const Part::GeomCurve*>(geo1);
        const Part::GeomCurve* curve2 = static_cast<const Part::GeomCurve*>(geo2);

        double refparam1;
        double refparam2;

        try {
            if (!curve1->closestParameter(refPnt1, refparam1))
                return -1;
        }
        catch (Base::CADKernelError& e) {
            e.ReportException();
            THROWM(Base::CADKernelError,
                   "Unable to determine the parameter of the first selected curve at the reference "
                   "point.")
        }

        try {
            if (!curve2->closestParameter(refPnt2, refparam2))
                return -1;
        }
        catch (Base::CADKernelError& e) {
            e.ReportException();
            THROWM(Base::CADKernelError,
                   "Unable to determine the parameter of the second selected curve at the "
                   "reference point.")
        }

#ifdef DEBUG
        Base::Console().Log("\n\nFILLET DEBUG\n\n");
        Base::Console().Log("Ref param: (%f);(%f)", refparam1, refparam2);
#endif

        std::pair<Base::Vector3d, Base::Vector3d> interpoints;
        std::vector<std::pair<Base::Vector3d, Base::Vector3d>> points;


        // look for coincident constraints between curves, take the coincident closest to the
        // refpoints
        Sketcher::PointPos curve1PosId = Sketcher::PointPos::none;
        Sketcher::PointPos curve2PosId = Sketcher::PointPos::none;

        double dist = INFINITY;

        const std::vector<Constraint*>& constraints = this->Constraints.getValues();

        for (std::vector<Constraint*>::const_iterator it = constraints.begin();
             it != constraints.end();
             ++it) {
            if ((*it)->Type == Sketcher::Coincident || (*it)->Type == Sketcher::Perpendicular
                || (*it)->Type == Sketcher::Tangent) {
                if ((*it)->First == GeoId1 && (*it)->Second == GeoId2
                    && (*it)->FirstPos != Sketcher::PointPos::none
                    && (*it)->SecondPos != Sketcher::PointPos::none) {
                    Base::Vector3d tmpp1 = getPoint((*it)->First, (*it)->FirstPos);
                    Base::Vector3d tmpp2 = getPoint((*it)->Second, (*it)->SecondPos);
                    double tmpdist = distancetorefpoints(tmpp1, tmpp2, refPnt1, refPnt2);
                    if (tmpdist < dist) {
                        curve1PosId = (*it)->FirstPos;
                        curve2PosId = (*it)->SecondPos;
                        dist = tmpdist;
                        interpoints = std::make_pair(tmpp1, tmpp2);
                    }
                }
                else if ((*it)->First == GeoId2 && (*it)->Second == GeoId1
                         && (*it)->FirstPos != Sketcher::PointPos::none
                         && (*it)->SecondPos != Sketcher::PointPos::none) {
                    Base::Vector3d tmpp2 = getPoint((*it)->First, (*it)->FirstPos);
                    Base::Vector3d tmpp1 = getPoint((*it)->Second, (*it)->SecondPos);
                    double tmpdist = distancetorefpoints(tmpp1, tmpp2, refPnt1, refPnt2);
                    if (tmpdist < dist) {
                        curve2PosId = (*it)->FirstPos;
                        curve1PosId = (*it)->SecondPos;
                        dist = tmpdist;
                        interpoints = std::make_pair(tmpp1, tmpp2);
                    }
                }
            }
        }

        if (curve1PosId == Sketcher::PointPos::none) {
            // no coincident was found, try basis curve intersection if GeomTrimmedCurve
            if (geo1->isDerivedFrom(Part::GeomTrimmedCurve::getClassTypeId())
                && geo2->isDerivedFrom(Part::GeomTrimmedCurve::getClassTypeId())) {

                const Part::GeomTrimmedCurve* tcurve1 =
                    static_cast<const Part::GeomTrimmedCurve*>(geo1);
                const Part::GeomTrimmedCurve* tcurve2 =
                    static_cast<const Part::GeomTrimmedCurve*>(geo2);

                try {
                    if (!tcurve1->intersectBasisCurves(tcurve2, points))
                        return -1;
                }
                catch (Base::CADKernelError& e) {
                    e.ReportException();
                    THROWMT(Base::CADKernelError,
                            QT_TRANSLATE_NOOP("Exceptions",
                                              "Unable to guess intersection of curves. Try adding "
                                              "a coincident constraint between the vertices of the "
                                              "curves you are intending to fillet."))
                }

                int res = selectintersection(points, interpoints, refPnt1, refPnt2);

                if (res != 0)
                    return res;
            }
            else
                return -1;// not a GeomTrimmedCurve and no coincident point.
        }

        // Now that we know where the curves intersect, get the parameters in the curves of those
        // points
        double intparam1;
        double intparam2;

        try {
            if (!curve1->closestParameter(interpoints.first, intparam1))
                return -1;
        }
        catch (Base::CADKernelError& e) {
            e.ReportException();
            THROWM(Base::CADKernelError,
                   "Unable to determine the parameter of the first selected curve at the "
                   "intersection of the curves.")
        }

        try {
            if (!curve2->closestParameter(interpoints.second, intparam2))
                return -1;
        }
        catch (Base::CADKernelError& e) {
            e.ReportException();
            THROWM(Base::CADKernelError,
                   "Unable to determine the parameter of the second selected curve at the "
                   "intersection of the curves.")
        }

        // get the starting parameters of each curve
        double spc1 = curve1->getFirstParameter();
        double spc2 = curve2->getFirstParameter();

        // get a fillet radius if zero was given
        Base::Vector3d ref21 = refPnt2 - refPnt1;

        if (radius == .0f) {
            // guess a radius
            // https://forum.freecad.org/viewtopic.php?f=3&t=31594&start=50#p266658
            //
            // We do not know the actual tangency points until we intersect the offset curves, but
            // we do not have offset curves before with decide on a radius.
            //
            // This estimation guesses a radius as the average of the distances from the reference
            // points with respect to the intersection of the normals at those reference points.

            try {
                Base::Vector3d tdir1;
                Base::Vector3d tdir2;

                // We want normals, but OCCT normals require curves to be 2 times derivable, and
                // lines are not tangency calculation requires 1 time derivable.

                if (!curve1->tangent(refparam1, tdir1))
                    return -1;

                if (!curve2->tangent(refparam2, tdir2))
                    return -1;

                Base::Vector3d dir1(tdir1.y, -tdir1.x, 0);
                Base::Vector3d dir2(tdir2.y, -tdir2.x, 0);

                double det = -dir1.x * dir2.y + dir2.x * dir1.y;

                if (std::abs(det) < Precision::Confusion()) {
                    // no intersection of normals
                    throw Base::RuntimeError("No intersection of normals");
                }

                Base::Vector3d refp1 = curve1->pointAtParameter(refparam1);
                Base::Vector3d refp2 = curve2->pointAtParameter(refparam2);

                // Base::Console().Log("refpoints:
                // (%f,%f,%f);(%f,%f,%f)",refp1.x,refp1.y,refp1.z,refp2.x,refp2.y,refp2.z);

                Base::Vector3d normalintersect(
                    (-dir1.x * dir2.x * refp1.y + dir1.x * dir2.x * refp2.y
                     - dir1.x * dir2.y * refp2.x + dir2.x * dir1.y * refp1.x)
                        / det,
                    (-dir1.x * dir2.y * refp1.y + dir2.x * dir1.y * refp2.y
                     + dir1.y * dir2.y * refp1.x - dir1.y * dir2.y * refp2.x)
                        / det,
                    0);

                radius =
                    ((refp1 - normalintersect).Length() + (refp2 - normalintersect).Length()) / 2;
            }
            catch (const Base::Exception&) {
                radius = ref21.Length();// fall-back to simplest estimation.
            }
        }


#ifdef DEBUG
        Base::Console().Log("Start param: (%f);(%f)\n", spc1, spc2);

        Base::Vector3d c1pf = curve1->pointAtParameter(spc1);
        Base::Vector3d c2pf = curve2->pointAtParameter(spc2);

        Base::Console().Log("start point curves: (%f,%f,%f);(%f,%f,%f)\n",
                            c1pf.x,
                            c1pf.y,
                            c1pf.z,
                            c2pf.x,
                            c2pf.y,
                            c2pf.z);
#endif
        // We create Offset curves at the suggested radius, the direction of offset is estimated
        // from the tangency vector
        Base::Vector3d tdir1 = curve1->firstDerivativeAtParameter(refparam1);
        Base::Vector3d tdir2 = curve2->firstDerivativeAtParameter(refparam2);

#ifdef DEBUG
        Base::Console().Log("tangent vectors: (%f,%f,%f);(%f,%f,%f)\n",
                            tdir1.x,
                            tdir1.y,
                            tdir1.z,
                            tdir2.x,
                            tdir2.y,
                            tdir2.z);
        Base::Console().Log("inter-ref vector: (%f,%f,%f)\n", ref21.x, ref21.y, ref21.z);
#endif

        Base::Vector3d vn(0, 0, 1);

        double sdir1 = tdir1.Cross(ref21).Dot(vn);
        double sdir2 = tdir2.Cross(-ref21).Dot(vn);

#ifdef DEBUG
        Base::Console().Log("sign of offset: (%f,%f)\n", sdir1, sdir2);
        Base::Console().Log("radius: %f\n", radius);
#endif

        Part::GeomOffsetCurve* ocurve1 = new Part::GeomOffsetCurve(
            Handle(Geom_Curve)::DownCast(curve1->handle()), (sdir1 < 0) ? radius : -radius, vn);

        Part::GeomOffsetCurve* ocurve2 = new Part::GeomOffsetCurve(
            Handle(Geom_Curve)::DownCast(curve2->handle()), (sdir2 < 0) ? radius : -radius, vn);

#ifdef DEBUG
        Base::Vector3d oc1pf = ocurve1->pointAtParameter(ocurve1->getFirstParameter());
        Base::Vector3d oc2pf = ocurve2->pointAtParameter(ocurve2->getFirstParameter());

        Base::Console().Log("start point offset curves: (%f,%f,%f);(%f,%f,%f)\n",
                            oc1pf.x,
                            oc1pf.y,
                            oc1pf.z,
                            oc2pf.x,
                            oc2pf.y,
                            oc2pf.z);

        /*auto printoffsetcurve = [](Part::GeomOffsetCurve *c) {

            for(double param = c->getFirstParameter(); param < c->getLastParameter(); param = param
        + (c->getLastParameter()-c->getFirstParameter())/10) Base::Console().Log("\n%f:
        (%f,%f,0)\n", param, c->pointAtParameter(param).x,c->pointAtParameter(param).y);

        };

        printoffsetcurve(ocurve1);
        printoffsetcurve(ocurve2);*/
#endif

        // Next we calculate the intersection of offset curves to get the center of the fillet
        std::pair<Base::Vector3d, Base::Vector3d> filletcenterpoint;
        std::vector<std::pair<Base::Vector3d, Base::Vector3d>> offsetintersectionpoints;

        try {
            if (!ocurve1->intersect(ocurve2, offsetintersectionpoints)) {
#ifdef DEBUG
                Base::Console().Log("No intersection between offset curves\n");
#endif
                return -1;
            }
        }
        catch (Base::CADKernelError& e) {
            e.ReportException();
            THROWM(Base::CADKernelError, "Unable to find intersection between offset curves.")
        }

#ifdef DEBUG
        for (auto inter : offsetintersectionpoints) {
            Base::Console().Log("offset int(%f,%f,0)\n", inter.first.x, inter.first.y);
        }
#endif

        int res = selectintersection(offsetintersectionpoints, filletcenterpoint, refPnt1, refPnt2);

        if (res != 0)
            return res;

#ifdef DEBUG
        Base::Console().Log(
            "selected offset int(%f,%f,0)\n", filletcenterpoint.first.x, filletcenterpoint.first.y);
#endif

        double refoparam1;
        double refoparam2;

        try {
            if (!curve1->closestParameter(filletcenterpoint.first, refoparam1))
                return -1;
        }
        catch (Base::CADKernelError& e) {
            e.ReportException();
            THROWM(Base::CADKernelError, "Unable to determine the starting point of the arc.")
        }

        try {
            if (!curve2->closestParameter(filletcenterpoint.second, refoparam2))
                return -1;
        }
        catch (Base::CADKernelError& e) {
            e.ReportException();
            THROWM(Base::CADKernelError, "Unable to determine the end point of the arc.")
        }

        // Next we calculate the closest points to the fillet center, so the points where tangency
        // is to be applied
        Base::Vector3d refp1 = curve1->pointAtParameter(refoparam1);
        Base::Vector3d refp2 = curve2->pointAtParameter(refoparam2);

#ifdef DEBUG
        Base::Console().Log("refpoints: (%f,%f,%f);(%f,%f,%f)",
                            refp1.x,
                            refp1.y,
                            refp1.z,
                            refp2.x,
                            refp2.y,
                            refp2.z);
#endif
        // Now we create arc for the fillet
        double startAngle, endAngle, range;

        Base::Vector3d radDir1 = refp1 - filletcenterpoint.first;
        Base::Vector3d radDir2 = refp2 - filletcenterpoint.first;

        startAngle = atan2(radDir1.y, radDir1.x);

        range = atan2(-radDir1.y * radDir2.x + radDir1.x * radDir2.y,
                      radDir1.x * radDir2.x + radDir1.y * radDir2.y);

        endAngle = startAngle + range;

        if (endAngle < startAngle)
            std::swap(startAngle, endAngle);

        if (endAngle > 2 * M_PI)
            endAngle -= 2 * M_PI;

        if (startAngle < 0)
            endAngle += 2 * M_PI;

        // Create Arc Segment
        Part::GeomArcOfCircle* arc = new Part::GeomArcOfCircle();
        arc->setRadius(radDir1.Length());
        arc->setCenter(filletcenterpoint.first);
        arc->setRange(startAngle, endAngle, /*emulateCCWXY=*/true);

        // add arc to sketch geometry
        int filletId;
        Part::Geometry* newgeo = arc;
        filletId = addGeometry(newgeo);
        if (filletId < 0) {
            delete arc;
            return -1;
        }

        if (trim) {
            auto selectend = [](double intparam, double refparam, double startparam) {
                if ((intparam > refparam && startparam >= refparam)
                    || (intparam < refparam && startparam <= refparam)) {
                    return PointPos::start;
                }
                else {
                    return PointPos::end;
                }
            };

            // Two cases:
            // a) there as a coincidence constraint
            // b) we used the basis curve intersection


            if (curve1PosId == Sketcher::PointPos::none) {
                curve1PosId = selectend(intparam1, refoparam1, spc1);
                curve2PosId = selectend(intparam2, refoparam2, spc2);
            }


            delConstraintOnPoint(GeoId1, curve1PosId, false);
            delConstraintOnPoint(GeoId2, curve2PosId, false);


            Sketcher::Constraint* tangent1 = new Sketcher::Constraint();
            Sketcher::Constraint* tangent2 = new Sketcher::Constraint();

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

            // Base::Console().Log("dists_refpoint_to_arc_sp_ep: (%f);(%f)",dist1,dist2);

            if (dist1 < dist2) {
                tangent1->SecondPos = PointPos::start;
                tangent2->SecondPos = PointPos::end;
                movePoint(GeoId1, curve1PosId, arc->getStartPoint(true), false, true);
                movePoint(GeoId2, curve2PosId, arc->getEndPoint(true), false, true);
            }
            else {
                tangent1->SecondPos = PointPos::end;
                tangent2->SecondPos = PointPos::start;
                movePoint(GeoId1, curve1PosId, arc->getEndPoint(true), false, true);
                movePoint(GeoId2, curve2PosId, arc->getStartPoint(true), false, true);
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

        // if we do not have a recompute after the geometry creation, the sketch must be solved to
        // update the DoF of the solver
        if (noRecomputes)
            solve();

        return 0;
    }
    return -1;
}

int SketchObject::extend(int GeoId, double increment, PointPos endpoint)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const std::vector<Part::Geometry*>& geomList = getInternalGeometry();
    Part::Geometry* geom = geomList[GeoId];
    int retcode = -1;
    if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        Part::GeomLineSegment* seg = static_cast<Part::GeomLineSegment*>(geom);
        Base::Vector3d startVec = seg->getStartPoint();
        Base::Vector3d endVec = seg->getEndPoint();
        if (endpoint == PointPos::start) {
            Base::Vector3d newPoint = startVec - endVec;
            double scaleFactor = newPoint.Length() + increment;
            newPoint.Normalize();
            newPoint.Scale(scaleFactor, scaleFactor, scaleFactor);
            newPoint = newPoint + endVec;
            retcode = movePoint(GeoId, Sketcher::PointPos::start, newPoint, false, true);
        }
        else if (endpoint == PointPos::end) {
            Base::Vector3d newPoint = endVec - startVec;
            double scaleFactor = newPoint.Length() + increment;
            newPoint.Normalize();
            newPoint.Scale(scaleFactor, scaleFactor, scaleFactor);
            newPoint = newPoint + startVec;
            retcode = movePoint(GeoId, Sketcher::PointPos::end, newPoint, false, true);
        }
    }
    else if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
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

bool SketchObject::seekTrimPoints(int GeoId, const Base::Vector3d& point, int& GeoId1,
                                  Base::Vector3d& intersect1, int& GeoId2,
                                  Base::Vector3d& intersect2)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return false;

    auto geos = getCompleteGeometry();// this includes the axes too

    geos.resize(geos.size() - 2);// remove the axes to avoid intersections with the axes

    int localindex1, localindex2;

    // Not found in will be returned as -1, not as GeoUndef, Part WB is agnostic to the concept of
    // GeoUndef
    if (!Part2DObject::seekTrimPoints(
            geos, GeoId, point, localindex1, intersect1, localindex2, intersect2))
        return false;

    // invalid complete geometry indices are mapped to GeoUndef
    GeoId1 = getGeoIdFromCompleteGeometryIndex(localindex1);
    GeoId2 = getGeoIdFromCompleteGeometryIndex(localindex2);

    return true;
}

int SketchObject::trim(int GeoId, const Base::Vector3d& point)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    //******************* Basic checks rejecting the operation
    //****************************************//
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    auto geo = getGeometry(GeoId);

    if (!GeometryFacade::isInternalType(geo, InternalType::None))
        return -1;// internal alignment geometry is not trimmable

    //******************* Lambdas - common functions for different intersections
    //****************************************//

    // returns true if the point defined by (GeoId1, pos1) can be considered to be coincident with
    // point.
    auto arePointsWithinPrecision = [](Base::Vector3d point1, Base::Vector3d point2) {
        // From testing: 500x (or 0.000050) is needed in order to not falsely distinguish points
        // calculated with seekTrimPoints
        if ((point1 - point2).Length() < 500 * Precision::Confusion())
            return true;

        return false;
    };

    auto isPointAtPosition =
        [this, arePointsWithinPrecision](int GeoId1, PointPos pos1, Base::Vector3d point) {
            Base::Vector3d pp = getPoint(GeoId1, pos1);

            return arePointsWithinPrecision(point, pp);
        };

    // Helper function to remove Equal constraints from a chosen edge (e.g Line segment).
    auto delEqualConstraintsOnGeoId = [this](int GeoId) {
        std::vector<int> delete_list;
        int index = 0;
        const std::vector<Constraint*>& constraints = this->Constraints.getValues();
        for (std::vector<Constraint*>::const_iterator it = constraints.begin();
             it != constraints.end();
             ++it, ++index) {
            Constraint* constr = *(it);
            if (constr->First == GeoId && constr->Type == Sketcher::Equal) {
                delete_list.push_back(index);
            }
            if (constr->Second == GeoId && constr->Type == Sketcher::Equal) {
                delete_list.push_back(index);
            }
        }
        delConstraints(delete_list, false);
    };

    // Checks whether preexisting constraints must be converted to new constraints.
    // Preexisting point on object constraints get converted to coincidents, unless an end-to-end
    // tangency is more relevant. returns by reference:
    //      - The type of constraint that should be used to constraint GeoId1 and GeoId
    //      - The element of GeoId1 to which the constraint should be applied.
    auto transformPreexistingConstraints = [this, isPointAtPosition](int GeoId,
                                                                     int GeoId1,
                                                                     Base::Vector3d point1,
                                                                     ConstraintType& constrType,
                                                                     PointPos& secondPos) {
        const std::vector<Constraint*>& constraints = this->Constraints.getValues();
        int constrId = 0;
        std::vector<int> delete_list;
        for (std::vector<Constraint*>::const_iterator it = constraints.begin();
             it != constraints.end();
             ++it) {
            Constraint* constr = *(it);
            // There is a preexisting PointOnObject constraint, see if it must be converted to a
            // coincident
            if (constr->Type == Sketcher::PointOnObject && constr->First == GeoId1
                && constr->Second == GeoId) {
                if (isPointAtPosition(constr->First, constr->FirstPos, point1)) {
                    constrType = Sketcher::Coincident;
                    secondPos = constr->FirstPos;
                    delete_list.push_back(constrId);
                }
            }
            constrId++;
        }
        /* It is possible that the trimming entity has both a PointOnObject constraint to the
         * trimmed entity, and a simple Tangent contstrait to the trimmed entity. In this case we
         * want to change to a single end-to-end tangency, i.e we want to ensure that constrType1 is
         * set to Sketcher::Tangent, that the secondPos1 is captured from the PointOnObject, and
         * also make sure that the PointOnObject constraint is deleted. The below loop ensures this,
         * also in case the ordering of the constraints is first Tangent and then PointOnObject. */
        constrId = 0;
        for (std::vector<Constraint*>::const_iterator it = constraints.begin();
             it != constraints.end();
             ++it) {
            Constraint* constr = *(it);
            if (constr->Type == Sketcher::Tangent) {
                if (constr->First == GeoId1 && constr->Second == GeoId) {
                    constrType = Sketcher::Tangent;
                    if (secondPos == Sketcher::PointPos::none)
                        secondPos = constr->FirstPos;
                    delete_list.push_back(constrId);
                }
                else if (constr->First == GeoId && constr->Second == GeoId1) {
                    constrType = Sketcher::Tangent;
                    if (secondPos == Sketcher::PointPos::none)
                        secondPos = constr->SecondPos;
                    delete_list.push_back(constrId);
                }
            }
            if (constr->Type == Sketcher::Perpendicular) {
                if (constr->First == GeoId1 && constr->Second == GeoId) {
                    constrType = Sketcher::Perpendicular;
                    if (secondPos == Sketcher::PointPos::none)
                        secondPos = constr->FirstPos;
                    delete_list.push_back(constrId);
                }
                else if (constr->First == GeoId && constr->Second == GeoId1) {
                    constrType = Sketcher::Perpendicular;
                    if (secondPos == Sketcher::PointPos::none)
                        secondPos = constr->SecondPos;
                    delete_list.push_back(constrId);
                }
            }

            constrId++;
        }
        delConstraints(delete_list, false);
    };

    // makes an equality constraint between GeoId1 and GeoId2
    auto constrainAsEqual = [this](int GeoId1, int GeoId2) {
        auto newConstr = std::make_unique<Sketcher::Constraint>();

        // Build Constraints associated with new pair of arcs
        newConstr->Type = Sketcher::Equal;
        newConstr->First = GeoId1;
        newConstr->FirstPos = Sketcher::PointPos::none;
        newConstr->Second = GeoId2;
        newConstr->SecondPos = Sketcher::PointPos::none;
        addConstraint(std::move(newConstr));
    };

    // Removes all internal geometry of a BSplineCurve and updates the GeoId index after removal
    auto ifBSplineRemoveInternalAlignmentGeometry = [this](int& GeoId) {
        const Part::Geometry* geo = getGeometry(GeoId);
        if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            // We need to remove the internal geometry of the BSpline, as BSplines change in number
            // of poles and knots We save the tags of the relevant geometry to retrieve the new
            // GeoIds later on.
            boost::uuids::uuid GeoIdTag;

            GeoIdTag = geo->getTag();

            deleteUnusedInternalGeometry(GeoId);

            auto vals = getCompleteGeometry();

            for (size_t i = 0; i < vals.size(); i++) {
                if (vals[i]->getTag() == GeoIdTag) {
                    GeoId = getGeoIdFromCompleteGeometryIndex(i);
                    break;
                }
            }
        }
    };

    // given a geometry and tree points, returns the corresponding parameters of the geometry points
    // closest to them
    auto getIntersectionParameters = [](const Part::Geometry* geo,
                                        const Base::Vector3d point,
                                        double& pointParam,
                                        const Base::Vector3d point1,
                                        double& point1Param,
                                        const Base::Vector3d point2,
                                        double& point2Param) {
        auto curve = static_cast<const Part::GeomCurve*>(geo);

        try {
            curve->closestParameter(point, pointParam);
            curve->closestParameter(point1, point1Param);
            curve->closestParameter(point2, point2Param);
        }
        catch (Base::CADKernelError& e) {
            e.ReportException();
            return false;
        }

        return true;
    };

    //******************* Step A => Detection of intersection - Common to all Geometries
    //****************************************//
    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef;
    Base::Vector3d point1, point2;
    // Using SketchObject wrapper, as Part2DObject version returns GeoId = -1 when intersection not
    // found, which is wrong for a GeoId (axis). seekTrimPoints returns:
    // - For a parameter associated with "point" between an intersection and the end point
    // (non-periodic case) GeoId1 != GeoUndef and GeoId2 == GeoUndef
    // - For a parameter associated with "point" between the start point and an intersection
    // (non-periodic case) GeoId2 != GeoUndef and GeoId1 == GeoUndef
    // - For a parameter associated with "point" between two intersection points, GeoId1 != GeoUndef
    // and GeoId2 != GeoUndef
    //
    // FirstParam < point1param < point2param < LastParam
    if (!SketchObject::seekTrimPoints(GeoId, point, GeoId1, point1, GeoId2, point2)) {
        // If no suitable trim points are found, then trim defaults to deleting the geometry
        delGeometry(GeoId);
        return 0;
    }

    //******************* Preparation of BSplines ****************************************//
    // Trimmed B-Spline internal geometry cannot be reused
    geo = getGeometry(GeoId);

    auto isBSpline = geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId();
    auto isPeriodicBSpline =
        isBSpline && static_cast<const Part::GeomBSplineCurve*>(geo)->isPeriodic();
    auto isNonPeriodicBSpline =
        isBSpline && !static_cast<const Part::GeomBSplineCurve*>(geo)->isPeriodic();
    auto isLineSegment = geo->getTypeId() == Part::GeomLineSegment::getClassTypeId();
    auto isDerivedFromTrimmedCurve = geo->isDerivedFrom(Part::GeomTrimmedCurve::getClassTypeId());
    auto isCircle = geo->getTypeId() == Part::GeomCircle::getClassTypeId();
    auto isEllipse = geo->getTypeId() == Part::GeomEllipse::getClassTypeId();

    if (isBSpline) {

        // Two options, it is a periodic bspline and we need two intersections or
        // it is a non-periodic bspline and one intersection is enough.
        auto bspline = static_cast<const Part::GeomBSplineCurve*>(geo);

        if (bspline->isPeriodic() && (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef))
            return -1;

        ifBSplineRemoveInternalAlignmentGeometry(GeoId);// GeoId gets updated here

        // When internal alignment geometry is removed from a bspline, it moves slightly
        // this causes that small segments are detected near the endpoints.
        //
        // The alternative to this re-detection, is to remove the internal alignment geometry
        // before the detection. However, that would cause the lost of the internal alignment
        // geometry in a case where trimming does not succeed because seekTrimPoints fails to find
        // (suitable) intersection(s)
        if (!SketchObject::seekTrimPoints(GeoId, point, GeoId1, point1, GeoId2, point2)) {
            // If no suitable trim points are found, then trim defaults to deleting the geometry
            delGeometry(GeoId);
            return 0;
        }

        geo = getGeometry(GeoId);
    }

    if (GeoId1 != GeoEnum::GeoUndef && GeoId2 != GeoEnum::GeoUndef
        && arePointsWithinPrecision(point1, point2)) {
        // If both points are detected and are coincident, deletion is the only option.
        delGeometry(GeoId);

        return 0;
    }

    //******************* Step B.1 => Trimming for GeomTrimmedCurves (line segment and arcs)
    //****************************************//
    if (isDerivedFromTrimmedCurve || isNonPeriodicBSpline) {

        if (geo->isDerivedFrom(Part::GeomConic::getClassTypeId())) {
            auto* tc = static_cast<const Part::GeomConic*>(geo);
            if (tc->isReversed()) {
                // reversing does not change the curve as seen by the sketcher.
                const_cast<Part::GeomConic*>(tc)->reverse();
            }
        }

        //****** Step B.1 (1) => Determine intersection parameters ******//
        // Now LastParam > FirstParam
        double firstParam, lastParam;
        if (isDerivedFromTrimmedCurve) {
            auto aoc = static_cast<const Part::GeomTrimmedCurve*>(geo);
            aoc->getRange(firstParam, lastParam);
        }
        else if (isNonPeriodicBSpline) {
            auto bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
            firstParam = bsp->getFirstParameter();
            lastParam = bsp->getLastParameter();
        }

        double pointParam, point1Param, point2Param;
        if (!getIntersectionParameters(
                geo, point, pointParam, point1, point1Param, point2, point2Param))
            return -1;

#ifdef DEBUG
        Base::Console().Log("Trim sought: GeoId1=%d (%f), GeoId2=%d (%f)\n",
                            GeoId1,
                            point1Param,
                            GeoId2,
                            point2Param);
#endif

        // seekTrimPoints enforces that firstParam < point1Param < point2Param < lastParam
        auto paramDistance = [](double param1, double param2) {
            double distance = fabs(param1 - param2);

            if (distance < Precision::Confusion())
                return 0.;
            else
                return distance;
        };

        //****** Step B.1 (2) => Determine trimmable sections and trim operation ******//

        // Determine if there is something trimmable
        double startDistance = GeoId1 != GeoEnum::GeoUndef ? paramDistance(firstParam, point1Param)
                                                           : paramDistance(firstParam, point2Param);
        double endDistance = GeoId2 != GeoEnum::GeoUndef ? paramDistance(lastParam, point2Param)
                                                         : paramDistance(lastParam, point1Param);
        double middleDistance = (GeoId1 != GeoEnum::GeoUndef && GeoId2 != GeoEnum::GeoUndef)
            ? paramDistance(point1Param, point2Param)
            : 0.0;

        bool trimmableStart = startDistance > 0.;
        bool trimmableMiddle = middleDistance > 0.;
        bool trimmableEnd = endDistance > 0.;

        struct Operation
        {
            Operation()
                : Type(trim_none),
                  actingParam(0.),
                  intersectingGeoId(GeoEnum::GeoUndef)
            {}
            enum
            {
                trim_none,
                trim_start,
                trim_middle,
                trim_end,
                trim_delete
            } Type;

            double actingParam;
            Base::Vector3d actingPoint;
            int intersectingGeoId;
        };

        Operation op;

        if (GeoId1 != GeoEnum::GeoUndef && GeoId2 != GeoEnum::GeoUndef && pointParam > point1Param
            && pointParam < point2Param) {
            // Trim Point between intersection points

            if ((!trimmableStart && !trimmableEnd) || !trimmableMiddle) {
                // if after trimming nothing would be left or if there is nothing to trim
                op.Type = Operation::trim_delete;
            }
            else if (trimmableStart && trimmableEnd) {
                op.Type = Operation::trim_middle;// trim between point1Param and point2Param
            }
            else if (trimmableStart /*&&!trimmableEnd*/) {
                op.Type = Operation::trim_end;
                op.actingParam = point1Param;// trim from point1Param until lastParam
                op.actingPoint = point1;
                op.intersectingGeoId = GeoId1;
            }
            else {// !trimmableStart && trimmableEnd
                op.Type = Operation::trim_start;
                op.actingParam = point2Param;// trim from firstParam until point2Param
                op.actingPoint = point2;
                op.intersectingGeoId = GeoId2;
            }
        }
        else if (GeoId2 != GeoEnum::GeoUndef && pointParam < point2Param) {
            if (trimmableEnd) {
                op.Type = Operation::trim_start;
                op.actingParam = point2Param;// trim from firstParam until point2Param
                op.actingPoint = point2;
                op.intersectingGeoId = GeoId2;
            }
            else {
                op.Type = Operation::trim_delete;
            }
        }
        else if (GeoId1 != GeoEnum::GeoUndef && pointParam > point1Param) {
            if (trimmableStart) {
                op.Type = Operation::trim_end;
                op.actingParam = point1Param;// trim from point1Param until lastParam
                op.actingPoint = point1;
                op.intersectingGeoId = GeoId1;
            }
            else {
                op.Type = Operation::trim_delete;
            }
        }
        else {
            return -1;
        }

        //****** Step B.1 (3) => Execute Trimming operation ******//

        if (op.Type == Operation::trim_delete) {
            delGeometry(GeoId);

            return 0;
        }
        else if (op.Type == Operation::trim_middle) {
            // We need to create new curve, this new curve will represent the segment comprising the
            // end
            auto vals = getInternalGeometry();
            auto newVals(vals);
            newVals[GeoId] = newVals[GeoId]->clone();
            newVals.push_back(newVals[GeoId]->clone());
            int newGeoId = newVals.size() - 1;

            if (isDerivedFromTrimmedCurve) {
                static_cast<Part::GeomTrimmedCurve*>(newVals[GeoId])
                    ->setRange(firstParam, point1Param);
                static_cast<Part::GeomTrimmedCurve*>(newVals.back())
                    ->setRange(point2Param, lastParam);
            }
            else if (isNonPeriodicBSpline) {
                static_cast<Part::GeomBSplineCurve*>(newVals[GeoId])->Trim(firstParam, point1Param);
                static_cast<Part::GeomBSplineCurve*>(newVals.back())->Trim(point2Param, lastParam);
            }

            Geometry.setValues(std::move(newVals));

            // go through all constraints and replace the point (GeoId,end) with (newGeoId,end)
            transferConstraints(GeoId, PointPos::end, newGeoId, PointPos::end);

            // For a trimmed line segment, if it had an equality constraint, it must be removed as
            // the segment length is not equal For the rest of trimmed curves, the proportion shall
            // be constrain to be equal.
            if (isLineSegment) {
                delEqualConstraintsOnGeoId(GeoId);
                delEqualConstraintsOnGeoId(newGeoId);
            }

            if (!isLineSegment && !isNonPeriodicBSpline) {
                constrainAsEqual(GeoId, newGeoId);
            }

            //****** Step B.1 (4) => Constraint end points of trim sections ******//

            // constrain the trimming points on the corresponding geometries
            PointPos secondPos1 = Sketcher::PointPos::none, secondPos2 = Sketcher::PointPos::none;
            ConstraintType constrType1 = Sketcher::PointOnObject,
                           constrType2 = Sketcher::PointOnObject;

            // Segment comprising the start
            transformPreexistingConstraints(GeoId, GeoId1, point1, constrType1, secondPos1);

            addConstraint(constrType1, GeoId, Sketcher::PointPos::end, GeoId1, secondPos1);

            // Segment comprising the end
            transformPreexistingConstraints(GeoId, GeoId2, point2, constrType2, secondPos2);

            addConstraint(constrType2, newGeoId, Sketcher::PointPos::start, GeoId2, secondPos2);

            // Both segments have a coincident center
            if (!isLineSegment && !isBSpline) {
                addConstraint(Sketcher::Coincident,
                              GeoId,
                              Sketcher::PointPos::mid,
                              newGeoId,
                              Sketcher::PointPos::mid);
            }

            if (isNonPeriodicBSpline)
                exposeInternalGeometry(GeoId);

            // if we do not have a recompute, the sketch must be solved to update the DoF of the
            // solver
            if (noRecomputes)
                solve();

            return 0;
        }
        else if (op.Type == Operation::trim_start || op.Type == Operation::trim_end) {
            // drop the second/first intersection point
            geo = getGeometry(GeoId);
            if (isDerivedFromTrimmedCurve) {
                auto newGeo = std::unique_ptr<Part::GeomTrimmedCurve>(
                    static_cast<Part::GeomTrimmedCurve*>(geo->clone()));

                if (op.Type == Operation::trim_start)
                    newGeo->setRange(op.actingParam, lastParam);
                else if (op.Type == Operation::trim_end)
                    newGeo->setRange(firstParam, op.actingParam);

                Geometry.set1Value(GeoId, std::move(newGeo));
            }
            else if (isNonPeriodicBSpline) {
                auto newGeo = std::unique_ptr<Part::GeomBSplineCurve>(
                    static_cast<Part::GeomBSplineCurve*>(geo->clone()));

                if (op.Type == Operation::trim_start)
                    newGeo->Trim(op.actingParam, lastParam);
                else if (op.Type == Operation::trim_end)
                    newGeo->Trim(firstParam, op.actingParam);

                Geometry.set1Value(GeoId, std::move(newGeo));
            }

            // After trimming it, a line segment won't have the same length
            if (isLineSegment) {
                delEqualConstraintsOnGeoId(GeoId);
            }

            //****** Step B.1 (4) => Constraint end points ******//
            // So this is the fallback constraint type here.
            ConstraintType constrType = Sketcher::PointOnObject;
            PointPos secondPos = Sketcher::PointPos::none;

            transformPreexistingConstraints(
                GeoId, op.intersectingGeoId, op.actingPoint, constrType, secondPos);

            if (op.Type == Operation::trim_start) {
                delConstraintOnPoint(GeoId, PointPos::start, false);
                // constrain the trimming point on the corresponding geometry
                addConstraint(constrType, GeoId, PointPos::start, op.intersectingGeoId, secondPos);
            }
            else if (op.Type == Operation::trim_end) {
                delConstraintOnPoint(GeoId, PointPos::end, false);
                // constrain the trimming point on the corresponding geometry
                addConstraint(constrType, GeoId, PointPos::end, op.intersectingGeoId, secondPos);
            }

            if (isNonPeriodicBSpline)
                exposeInternalGeometry(GeoId);

            // if we do not have a recompute, the sketch must be solved to update the DoF of the
            // solver
            if (noRecomputes)
                solve();

            return 0;
        }
        else {
            return -1;
        }
    }
    //******************* Step B.2 => Trimming for unbounded periodic geometries
    //****************************************//
    else if (isCircle || isEllipse || isPeriodicBSpline) {
        //****** STEP A(2) => Common tests *****//
        if (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef)
            return -1;

        //****** Step B.2 (1) => Determine intersection parameters ******//
        double pointParam, point1Param, point2Param;
        if (!getIntersectionParameters(
                geo, point, pointParam, point1, point1Param, point2, point2Param))
            return -1;

#ifdef DEBUG
        Base::Console().Log("Trim sought: GeoId1=%d (%f), GeoId2=%d (%f)\n",
                            GeoId1,
                            point1Param,
                            GeoId2,
                            point2Param);
#endif

        //****** Step B.2 (3) => Execute Trimming operation ******//
        // Two intersection points detected
        std::unique_ptr<Part::Geometry> geoNew;

        if (isCircle) {
            auto circle = static_cast<const Part::GeomCircle*>(geo);
            auto aoc = std::make_unique<Part::GeomArcOfCircle>();
            aoc->setCenter(circle->getCenter());
            aoc->setRadius(circle->getRadius());
            aoc->setRange(point2Param, point1Param, /*emulateCCW=*/false);
            geoNew = std::move(aoc);
        }
        else if (isEllipse) {
            auto ellipse = static_cast<const Part::GeomEllipse*>(geo);
            auto aoe = std::make_unique<Part::GeomArcOfEllipse>();
            aoe->setCenter(ellipse->getCenter());
            aoe->setMajorRadius(ellipse->getMajorRadius());
            aoe->setMinorRadius(ellipse->getMinorRadius());
            aoe->setMajorAxisDir(ellipse->getMajorAxisDir());
            // CCW curve goes from point2 (start) to point1 (end)
            aoe->setRange(point2Param, point1Param, /*emulateCCW=*/false);
            geoNew = std::move(aoe);
        }
        else if (isPeriodicBSpline) {
            auto bspline = std::unique_ptr<Part::GeomBSplineCurve>(
                static_cast<Part::GeomBSplineCurve*>(geo->clone()));
            bspline->Trim(point2Param, point1Param);
            geoNew = std::move(bspline);
        }

        this->Geometry.set1Value(GeoId, std::move(geoNew));

        //****** Step B.2 (4) => Constraint end points ******//

        PointPos secondPos1 = Sketcher::PointPos::none, secondPos2 = Sketcher::PointPos::none;
        ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;

        // check first if start and end points are within a confusion tolerance
        if (isPointAtPosition(GeoId1, Sketcher::PointPos::start, point1)) {
            constrType1 = Sketcher::Coincident;
            secondPos1 = Sketcher::PointPos::start;
        }
        else if (isPointAtPosition(GeoId1, Sketcher::PointPos::end, point1)) {
            constrType1 = Sketcher::Coincident;
            secondPos1 = Sketcher::PointPos::end;
        }

        if (isPointAtPosition(GeoId2, Sketcher::PointPos::start, point2)) {
            constrType2 = Sketcher::Coincident;
            secondPos2 = Sketcher::PointPos::start;
        }
        else if (isPointAtPosition(GeoId2, Sketcher::PointPos::end, point2)) {
            constrType2 = Sketcher::Coincident;
            secondPos2 = Sketcher::PointPos::end;
        }

        transformPreexistingConstraints(GeoId, GeoId1, point1, constrType1, secondPos1);
        transformPreexistingConstraints(GeoId, GeoId2, point2, constrType2, secondPos2);

        if ((constrType1 == Sketcher::Coincident && secondPos1 == Sketcher::PointPos::none)
            || (constrType2 == Sketcher::Coincident && secondPos2 == Sketcher::PointPos::none))
            THROWM(
                ValueError,
                "Invalid position Sketcher::PointPos::none when creating a Coincident constraint")

        // constrain the trimming points on the corresponding geometries
        addConstraint(constrType1, GeoId, PointPos::end, GeoId1, secondPos1);

        addConstraint(constrType2, GeoId, PointPos::start, GeoId2, secondPos2);

        if (isBSpline)
            exposeInternalGeometry(GeoId);

        // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
        if (noRecomputes)
            solve();

        return 0;
    }

    return -1;
}

int SketchObject::split(int GeoId, const Base::Vector3d& point)
{
    // No need to check input data validity as this is an sketchobject managed operation

    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        return -1;
    }

    const Part::Geometry* geo = getGeometry(GeoId);
    std::vector<int> newIds;
    std::vector<Constraint*> newConstraints;

    Base::Vector3d startPoint, endPoint, splitPoint;
    double startParam, endParam, splitParam = 0.0;
    unsigned int longestPart = 0;

    auto createGeosFromPeriodic = [&](const Part::GeomCurve* curve,
                                      auto getCurveWithLimitParams,
                                      auto createAndTransferConstraints) {
        // find split point
        curve->closestParameter(point, splitParam);
        double period = curve->getLastParameter() - curve->getFirstParameter();
        startParam = splitParam;
        endParam = splitParam + period;

        // create new arc and restrict it
        auto newCurve = getCurveWithLimitParams(curve, startParam, endParam);
        int newId(GeoEnum::GeoUndef);
        newId = addGeometry(std::move(newCurve));// after here newCurve is a shell
        if (newId >= 0) {
            newIds.push_back(newId);
            setConstruction(newId, GeometryFacade::getConstruction(curve));
            exposeInternalGeometry(newId);

            // transfer any constraints
            createAndTransferConstraints(GeoId, newId);
            return true;
        }

        return false;
    };

    auto createGeosFromNonPeriodic = [&](const Part::GeomBoundedCurve* curve,
                                         auto getCurveWithLimitParams,
                                         auto createAndTransferConstraints) {
        startPoint = curve->getStartPoint();
        endPoint = curve->getEndPoint();

        // find split point
        curve->closestParameter(point, splitParam);
        startParam = curve->getFirstParameter();
        endParam = curve->getLastParameter();
        // TODO: Using parameter difference as a poor substitute of length.
        // Computing length of an arc of a generic conic would be expensive.
        if (endParam - splitParam < Precision::PConfusion()
            || splitParam - startParam < Precision::PConfusion()) {
            THROWM(ValueError, "Split point is at one of the end points of the curve.");
        }
        if (endParam - splitParam > splitParam - startParam) {
            longestPart = 1;
        }

        // create new curves
        auto newCurve = getCurveWithLimitParams(curve, startParam, splitParam);
        int newId(GeoEnum::GeoUndef);
        newId = addGeometry(std::move(newCurve));
        if (newId >= 0) {
            newIds.push_back(newId);
            setConstruction(newId, GeometryFacade::getConstruction(curve));
            exposeInternalGeometry(newId);

            // the "second" half
            newCurve = getCurveWithLimitParams(curve, splitParam, endParam);
            newId = addGeometry(std::move(newCurve));
            if (newId >= 0) {
                newIds.push_back(newId);
                setConstruction(newId, GeometryFacade::getConstruction(curve));
                exposeInternalGeometry(newId);

                // TODO: Certain transfers and new constraint can be directly made here.
                // But this may reduce readability.
                // apply appropriate constraints on the new points at split point and
                // transfer constraints from start and end of original spline
                createAndTransferConstraints(GeoId, newIds[0], newIds[1]);
                return true;
            }
        }

        return false;
    };

    bool ok = false;
    if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        ok = createGeosFromNonPeriodic(
            static_cast<const Part::GeomBoundedCurve*>(geo),
            [](const Part::GeomCurve* curve, double startParam, double endParam) {
                auto newArc = std::unique_ptr<Part::GeomLineSegment>(
                    static_cast<Part::GeomLineSegment*>(curve->copy()));
                newArc->setRange(startParam, endParam);
                return newArc;
            },
            [this, &newConstraints](int GeoId, int newId0, int newId1) {
                Constraint* joint = new Constraint();
                joint->Type = Coincident;
                joint->First = newId0;
                joint->FirstPos = PointPos::end;
                joint->Second = newId1;
                joint->SecondPos = PointPos::start;
                newConstraints.push_back(joint);

                transferConstraints(GeoId, PointPos::start, newId0, PointPos::start);
                transferConstraints(GeoId, PointPos::end, newId1, PointPos::end);
            });
    }
    else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        ok = createGeosFromPeriodic(
            static_cast<const Part::GeomCurve*>(geo),
            [](const Part::GeomCurve* curve, double startParam, double endParam) {
                auto newArc = std::make_unique<Part::GeomArcOfCircle>(
                    Handle(Geom_Circle)::DownCast(curve->handle()->Copy()));
                newArc->setRange(startParam, endParam, false);
                return newArc;
            },
            [this](int GeoId, int newId) {
                transferConstraints(GeoId, PointPos::mid, newId, PointPos::mid);
            });
    }
    else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        ok = createGeosFromPeriodic(
            static_cast<const Part::GeomCurve*>(geo),
            [](const Part::GeomCurve* curve, double startParam, double endParam) {
                auto newArc = std::make_unique<Part::GeomArcOfEllipse>(
                    Handle(Geom_Ellipse)::DownCast(curve->handle()->Copy()));
                newArc->setRange(startParam, endParam, false);
                return newArc;
            },
            [this](int GeoId, int newId) {
                transferConstraints(GeoId, PointPos::mid, newId, PointPos::mid);
            });
    }
    else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        ok = createGeosFromNonPeriodic(
            static_cast<const Part::GeomBoundedCurve*>(geo),
            [](const Part::GeomCurve* curve, double startParam, double endParam) {
                auto newArc = std::unique_ptr<Part::GeomArcOfCircle>(
                    static_cast<Part::GeomArcOfCircle*>(curve->copy()));
                newArc->setRange(startParam, endParam, false);
                return newArc;
            },
            [this, &newConstraints](int GeoId, int newId0, int newId1) {
                Constraint* joint = new Constraint();
                joint->Type = Coincident;
                joint->First = newId0;
                joint->FirstPos = PointPos::end;
                joint->Second = newId1;
                joint->SecondPos = PointPos::start;
                newConstraints.push_back(joint);

                joint = new Constraint();
                joint->Type = Coincident;
                joint->First = newId0;
                joint->FirstPos = PointPos::mid;
                joint->Second = newId1;
                joint->SecondPos = PointPos::mid;
                newConstraints.push_back(joint);

                transferConstraints(GeoId, PointPos::start, newId0, PointPos::start, true);
                transferConstraints(GeoId, PointPos::mid, newId0, PointPos::mid);
                transferConstraints(GeoId, PointPos::end, newId1, PointPos::end, true);
            });
    }
    else if (geo->isDerivedFrom(Part::GeomArcOfConic::getClassTypeId())) {
        ok = createGeosFromNonPeriodic(
            static_cast<const Part::GeomBoundedCurve*>(geo),
            [](const Part::GeomCurve* curve, double startParam, double endParam) {
                auto newArc = std::unique_ptr<Part::GeomArcOfConic>(
                    static_cast<Part::GeomArcOfConic*>(curve->copy()));
                newArc->setRange(startParam, endParam);
                return newArc;
            },
            [this, &newConstraints](int GeoId, int newId0, int newId1) {
                // apply appropriate constraints on the new points at split point
                Constraint* joint = new Constraint();
                joint->Type = Coincident;
                joint->First = newId0;
                joint->FirstPos = PointPos::end;
                joint->Second = newId1;
                joint->SecondPos = PointPos::start;
                newConstraints.push_back(joint);

                // TODO: Do we apply constraints on center etc of the conics?

                // transfer constraints from start and end of original
                transferConstraints(GeoId, PointPos::start, newId0, PointPos::start, true);
                transferConstraints(GeoId, PointPos::end, newId1, PointPos::end, true);
            });
    }
    else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
        const Part::GeomBSplineCurve* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

        // what to do for periodic b-splines?
        if (bsp->isPeriodic()) {
            ok = createGeosFromPeriodic(
                static_cast<const Part::GeomCurve*>(geo),
                [](const Part::GeomCurve* curve, double startParam, double endParam) {
                    auto newBsp = std::unique_ptr<Part::GeomBSplineCurve>(
                        static_cast<Part::GeomBSplineCurve*>(curve->copy()));
                    newBsp->Trim(startParam, endParam);
                    return newBsp;
                },
                [](int, int) {
                    // no constraints to transfer here, and we assume the split is to "break" the
                    // b-spline
                });
        }
        else {
            ok = createGeosFromNonPeriodic(
                static_cast<const Part::GeomBoundedCurve*>(geo),
                [](const Part::GeomCurve* curve, double startParam, double endParam) {
                    auto newBsp = std::unique_ptr<Part::GeomBSplineCurve>(
                        static_cast<Part::GeomBSplineCurve*>(curve->copy()));
                    newBsp->Trim(startParam, endParam);
                    return newBsp;
                },
                [this, &newConstraints](int GeoId, int newId0, int newId1) {
                    // apply appropriate constraints on the new points at split point
                    Constraint* joint = new Constraint();
                    joint->Type = Coincident;
                    joint->First = newId0;
                    joint->FirstPos = PointPos::end;
                    joint->Second = newId1;
                    joint->SecondPos = PointPos::start;
                    newConstraints.push_back(joint);

                    // transfer constraints from start and end of original spline
                    transferConstraints(GeoId, PointPos::start, newId0, PointPos::start, true);
                    transferConstraints(GeoId, PointPos::end, newId1, PointPos::end, true);
                });
        }
    }

    if (ok) {
        std::vector<int> oldConstraints;
        getConstraintIndices(GeoId, oldConstraints);

        const auto& allConstraints = this->Constraints.getValues();

        // keep constraints on internal geometries so they are deleted
        // when the old curve is deleted
        oldConstraints.erase(std::remove_if(oldConstraints.begin(),
                                            oldConstraints.end(),
                                            [=](const auto& i) {
                                                return allConstraints[i]->Type == InternalAlignment;
                                            }),
                             oldConstraints.end());

        for (unsigned int i = 0; i < oldConstraints.size(); ++i) {

            Constraint* con = allConstraints[oldConstraints[i]];
            int conId = con->First;
            PointPos conPos = con->FirstPos;
            if (conId == GeoId) {
                conId = con->Second;
                conPos = con->SecondPos;
            }

            bool transferToAll = false;
            switch (con->Type) {
                case Horizontal:
                case Vertical:
                case Parallel: {
                    transferToAll = geo->getTypeId() == Part::GeomLineSegment::getClassTypeId();
                    break;
                }
                case Tangent:
                case Perpendicular: {
                    unsigned int initial = 0;
                    unsigned int limit = newIds.size();

                    if (geo->isDerivedFrom(Part::GeomArcOfConic::getClassTypeId())) {
                        const Part::Geometry* conGeo = getGeometry(conId);

                        if (conGeo && conGeo->isDerivedFrom(Part::GeomCurve::getClassTypeId())) {
                            std::vector<std::pair<Base::Vector3d, Base::Vector3d>> intersections;
                            bool intersects[2];
                            auto* geo1 = getGeometry(newIds[0]);
                            auto* geo2 = getGeometry(newIds[1]);

                            intersects[0] = static_cast<const Part::GeomCurve*>(geo1)->intersect(
                                static_cast<const Part::GeomCurve*>(conGeo), intersections);
                            intersects[1] = static_cast<const Part::GeomCurve*>(geo2)->intersect(
                                static_cast<const Part::GeomCurve*>(conGeo), intersections);

                            initial = longestPart;
                            if (intersects[0] != intersects[1]) {
                                initial = intersects[1] ? 1 : 0;
                            }
                            limit = initial + 1;
                        }
                    }

                    for (unsigned int i = initial; i < limit; ++i) {
                        Constraint* trans = con->copy();
                        trans->substituteIndex(GeoId, newIds[i]);
                        newConstraints.push_back(trans);
                    }
                    break;
                }
                case Distance:
                case DistanceX:
                case DistanceY:
                case PointOnObject: {
                    if (con->FirstPos == PointPos::none && con->SecondPos == PointPos::none) {
                        Constraint* dist = con->copy();
                        dist->First = newIds[0];
                        dist->FirstPos = PointPos::start;
                        dist->Second = newIds[1];
                        dist->SecondPos = PointPos::end;
                        newConstraints.push_back(dist);
                    }
                    else {
                        Constraint* trans = con->copy();
                        trans->First = conId;
                        trans->FirstPos = conPos;
                        trans->SecondPos = PointPos::none;

                        Base::Vector3d conPoint(getPoint(conId, conPos));
                        int targetId = newIds[0];

                        // for non-periodic curves, see if second curve is more appropriate
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            Base::Vector3d projPoint(
                                conPoint.Perpendicular(startPoint, endPoint - startPoint));
                            Base::Vector3d splitDir = splitPoint - startPoint;
                            if ((projPoint - startPoint) * splitDir > splitDir * splitDir) {
                                targetId = newIds[1];
                            }
                        }
                        else if (geo->isDerivedFrom(Part::GeomArcOfConic::getClassTypeId())) {
                            double conParam;
                            static_cast<const Part::GeomArcOfConic*>(geo)->closestParameter(
                                conPoint, conParam);
                            if (conParam > splitParam)
                                targetId = newIds[1];
                        }
                        trans->Second = targetId;

                        newConstraints.push_back(trans);
                    }
                    break;
                }
                case Radius:
                case Diameter:
                case Equal: {
                    transferToAll = geo->getTypeId() == Part::GeomCircle::getClassTypeId()
                        || geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId();
                    break;
                }
                default:
                    // Release other constraints
                    break;
            }

            if (transferToAll) {
                for (auto& newId : newIds) {
                    Constraint* trans = con->copy();
                    trans->substituteIndex(GeoId, newId);
                    newConstraints.push_back(trans);
                }
            }
        }

        if (noRecomputes)
            solve();

        delConstraints(oldConstraints);
        addConstraints(newConstraints);
    }

    for (auto& cons : newConstraints) {
        delete cons;
    }

    if (ok) {
        delGeometry(GeoId);
        return 0;
    }

    return -1;
}

int SketchObject::join(int geoId1, Sketcher::PointPos posId1, int geoId2, Sketcher::PointPos posId2)
{
    // No need to check input data validity as this is an sketchobject managed operation

    Base::StateLocker lock(managedoperation, true);

    if (Sketcher::PointPos::start != posId1 && Sketcher::PointPos::end != posId1
        && Sketcher::PointPos::start != posId2 && Sketcher::PointPos::end != posId2) {
        THROWM(ValueError, "Invalid position(s): points must be start or end points of a curve.");
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

    // TODO: set up vectors for new poles, knots, mults
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
    newPoles.insert(newPoles.end(),
                    std::make_move_iterator(poles2.begin()),
                    std::make_move_iterator(poles2.end()));

    // TODO: Weights might need to be scaled
    weights2.erase(weights2.begin());
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
    if (bsp1->getDegree() < newMults.back())
        newMults.back() = bsp1->getDegree();
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
    else {
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

    return -1;
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
        Base::Console().Warning(
            "Probably, there is a circular reference in the document. Error: %s\n", e.what());
        return true;// prohibiting this reference won't remove the problem anyway...
    }


    // Note: Checking for the body of the support doesn't work when the support are the three base
    // planes
    // App::DocumentObject *support = this->Support.getValue();
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
    if (rsn)
        *rsn = rlAllowed;

    // Only applicable to sketches
    if (pObj->getTypeId() != Sketcher::SketchObject::getClassTypeId()) {
        if (rsn)
            *rsn = rlNotASketch;
        return false;
    }

    SketchObject* psObj = static_cast<SketchObject*>(pObj);

    // Sketches outside of the Document are NOT allowed
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
        Base::Console().Warning(
            "Probably, there is a circular reference in the document. Error: %s\n", e.what());
        return true;// prohibiting this reference won't remove the problem anyway...
    }


    // Note: Checking for the body of the support doesn't work when the support are the three base
    // planes
    // App::DocumentObject *support = this->Support.getValue();
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
                               Sketcher::PointPos refPosId /*=Sketcher::PointPos::none*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& geovals = getInternalGeometry();
    std::vector<Part::Geometry*> newgeoVals(geovals);

    const std::vector<Constraint*>& constrvals = this->Constraints.getValues();
    std::vector<Constraint*> newconstrVals(constrvals);

    newgeoVals.reserve(geovals.size() + geoIdList.size());

    int cgeoid = getHighestCurveIndex() + 1;

    std::map<int, int> geoIdMap;
    std::map<int, bool> isStartEndInverted;

    // Find out if reference is aligned with V or H axis,
    // if so we can keep Vertical and Horizontal constraints in the mirrored geometry.
    bool refIsAxisAligned = false;
    if (refGeoId == Sketcher::GeoEnum::VAxis || refGeoId == Sketcher::GeoEnum::HAxis) {
        refIsAxisAligned = true;
    }
    else {
        for (std::vector<Constraint*>::const_iterator it = constrvals.begin();
             it != constrvals.end();
             ++it) {
            Constraint* constr = *(it);
            if (constr->First == refGeoId
                && (constr->Type == Sketcher::Vertical || constr->Type == Sketcher::Horizontal))
                refIsAxisAligned = true;
        }
    }

    // reference is a line
    if (refPosId == Sketcher::PointPos::none) {
        const Part::Geometry* georef = getGeometry(refGeoId);
        if (georef->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
            Base::Console().Error("Reference for symmetric is neither a point nor a line.\n");
            return -1;
        }

        const Part::GeomLineSegment* refGeoLine = static_cast<const Part::GeomLineSegment*>(georef);
        // line
        Base::Vector3d refstart = refGeoLine->getStartPoint();
        Base::Vector3d vectline = refGeoLine->getEndPoint() - refstart;

        for (std::vector<int>::const_iterator it = geoIdList.begin(); it != geoIdList.end(); ++it) {
            const Part::Geometry* geo = getGeometry(*it);
            Part::Geometry* geosym;

            auto gf = GeometryFacade::getFacade(geo);

            if (gf->isInternalAligned()) {
                // only add this geometry if the corresponding geometry it defines is also in the
                // list.
                int definedGeo = GeoEnum::GeoUndef;

                for (auto c : Constraints.getValues()) {
                    if (c->Type == Sketcher::InternalAlignment && c->First == *it) {
                        definedGeo = c->Second;
                        break;
                    }
                }

                if (std::find(geoIdList.begin(), geoIdList.end(), definedGeo) != geoIdList.end())
                    geosym = geo->copy();
                else {
                    // we should not mirror internal alignment geometry, unless the element they
                    // define is also mirrored
                    continue;
                }
            }
            else {
                geosym = geo->copy();
            }

            // Handle Geometry
            if (geosym->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                Part::GeomLineSegment* geosymline = static_cast<Part::GeomLineSegment*>(geosym);
                Base::Vector3d sp = geosymline->getStartPoint();
                Base::Vector3d ep = geosymline->getEndPoint();

                geosymline->setPoints(
                    sp + 2.0 * (sp.Perpendicular(refGeoLine->getStartPoint(), vectline) - sp),
                    ep + 2.0 * (ep.Perpendicular(refGeoLine->getStartPoint(), vectline) - ep));
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                Part::GeomCircle* geosymcircle = static_cast<Part::GeomCircle*>(geosym);
                Base::Vector3d cp = geosymcircle->getCenter();

                geosymcircle->setCenter(
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp));
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                Part::GeomArcOfCircle* geoaoc = static_cast<Part::GeomArcOfCircle*>(geosym);
                Base::Vector3d sp = geoaoc->getStartPoint(true);
                Base::Vector3d ep = geoaoc->getEndPoint(true);
                Base::Vector3d cp = geoaoc->getCenter();

                Base::Vector3d ssp =
                    sp + 2.0 * (sp.Perpendicular(refGeoLine->getStartPoint(), vectline) - sp);
                Base::Vector3d sep =
                    ep + 2.0 * (ep.Perpendicular(refGeoLine->getStartPoint(), vectline) - ep);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                double theta1 = Base::fmod(atan2(sep.y - scp.y, sep.x - scp.x), 2.f * M_PI);
                double theta2 = Base::fmod(atan2(ssp.y - scp.y, ssp.x - scp.x), 2.f * M_PI);

                geoaoc->setCenter(scp);
                geoaoc->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(*it, true));
            }
            else if (geosym->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                Part::GeomEllipse* geosymellipse = static_cast<Part::GeomEllipse*>(geosym);
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
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                Part::GeomArcOfEllipse* geosymaoe = static_cast<Part::GeomArcOfEllipse*>(geosym);
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
                theta1 = 2.0 * M_PI - theta1;
                theta2 = 2.0 * M_PI - theta2;
                std::swap(theta1, theta2);
                if (theta1 < 0) {
                    theta1 += 2.0 * M_PI;
                    theta2 += 2.0 * M_PI;
                }

                geosymaoe->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(*it, true));
            }
            else if (geosym->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                Part::GeomArcOfHyperbola* geosymaoe =
                    static_cast<Part::GeomArcOfHyperbola*>(geosym);
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
                isStartEndInverted.insert(std::make_pair(*it, true));
            }
            else if (geosym->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                Part::GeomArcOfParabola* geosymaoe = static_cast<Part::GeomArcOfParabola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                // double df= geosymaoe->getFocal();
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
                isStartEndInverted.insert(std::make_pair(*it, true));
            }
            else if (geosym->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                Part::GeomBSplineCurve* geosymbsp = static_cast<Part::GeomBSplineCurve*>(geosym);

                std::vector<Base::Vector3d> poles = geosymbsp->getPoles();

                for (std::vector<Base::Vector3d>::iterator jt = poles.begin(); jt != poles.end();
                     ++jt) {

                    (*jt) = (*jt)
                        + 2.0
                            * ((*jt).Perpendicular(refGeoLine->getStartPoint(), vectline) - (*jt));
                }

                geosymbsp->setPoles(poles);

                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                Part::GeomPoint* geosympoint = static_cast<Part::GeomPoint*>(geosym);
                Base::Vector3d cp = geosympoint->getPoint();

                geosympoint->setPoint(
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp));
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
    else {// reference is a point
        refIsAxisAligned = true;
        Vector3d refpoint;
        const Part::Geometry* georef = getGeometry(refGeoId);

        if (georef->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            refpoint = static_cast<const Part::GeomPoint*>(georef)->getPoint();
        }
        else if (refGeoId == -1 && refPosId == Sketcher::PointPos::start) {
            refpoint = Vector3d(0, 0, 0);
        }
        else {
            switch (refPosId) {
                case Sketcher::PointPos::start:
                    if (georef->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        const Part::GeomLineSegment* geosymline =
                            static_cast<const Part::GeomLineSegment*>(georef);
                        refpoint = geosymline->getStartPoint();
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                        const Part::GeomArcOfCircle* geoaoc =
                            static_cast<const Part::GeomArcOfCircle*>(georef);
                        refpoint = geoaoc->getStartPoint(true);
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                        const Part::GeomArcOfEllipse* geosymaoe =
                            static_cast<const Part::GeomArcOfEllipse*>(georef);
                        refpoint = geosymaoe->getStartPoint(true);
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                        const Part::GeomArcOfHyperbola* geosymaoe =
                            static_cast<const Part::GeomArcOfHyperbola*>(georef);
                        refpoint = geosymaoe->getStartPoint(true);
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                        const Part::GeomArcOfParabola* geosymaoe =
                            static_cast<const Part::GeomArcOfParabola*>(georef);
                        refpoint = geosymaoe->getStartPoint(true);
                    }
                    else if (georef->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                        const Part::GeomBSplineCurve* geosymbsp =
                            static_cast<const Part::GeomBSplineCurve*>(georef);
                        refpoint = geosymbsp->getStartPoint();
                    }
                    break;
                case Sketcher::PointPos::end:
                    if (georef->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        const Part::GeomLineSegment* geosymline =
                            static_cast<const Part::GeomLineSegment*>(georef);
                        refpoint = geosymline->getEndPoint();
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                        const Part::GeomArcOfCircle* geoaoc =
                            static_cast<const Part::GeomArcOfCircle*>(georef);
                        refpoint = geoaoc->getEndPoint(true);
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                        const Part::GeomArcOfEllipse* geosymaoe =
                            static_cast<const Part::GeomArcOfEllipse*>(georef);
                        refpoint = geosymaoe->getEndPoint(true);
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                        const Part::GeomArcOfHyperbola* geosymaoe =
                            static_cast<const Part::GeomArcOfHyperbola*>(georef);
                        refpoint = geosymaoe->getEndPoint(true);
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                        const Part::GeomArcOfParabola* geosymaoe =
                            static_cast<const Part::GeomArcOfParabola*>(georef);
                        refpoint = geosymaoe->getEndPoint(true);
                    }
                    else if (georef->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                        const Part::GeomBSplineCurve* geosymbsp =
                            static_cast<const Part::GeomBSplineCurve*>(georef);
                        refpoint = geosymbsp->getEndPoint();
                    }
                    break;
                case Sketcher::PointPos::mid:
                    if (georef->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                        const Part::GeomCircle* geosymcircle =
                            static_cast<const Part::GeomCircle*>(georef);
                        refpoint = geosymcircle->getCenter();
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                        const Part::GeomArcOfCircle* geoaoc =
                            static_cast<const Part::GeomArcOfCircle*>(georef);
                        refpoint = geoaoc->getCenter();
                    }
                    else if (georef->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                        const Part::GeomEllipse* geosymellipse =
                            static_cast<const Part::GeomEllipse*>(georef);
                        refpoint = geosymellipse->getCenter();
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                        const Part::GeomArcOfEllipse* geosymaoe =
                            static_cast<const Part::GeomArcOfEllipse*>(georef);
                        refpoint = geosymaoe->getCenter();
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                        const Part::GeomArcOfHyperbola* geosymaoe =
                            static_cast<const Part::GeomArcOfHyperbola*>(georef);
                        refpoint = geosymaoe->getCenter();
                    }
                    else if (georef->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                        const Part::GeomArcOfParabola* geosymaoe =
                            static_cast<const Part::GeomArcOfParabola*>(georef);
                        refpoint = geosymaoe->getCenter();
                    }
                    break;
                default:
                    Base::Console().Error("Wrong PointPosId.\n");
                    return -1;
            }
        }

        for (std::vector<int>::const_iterator it = geoIdList.begin(); it != geoIdList.end(); ++it) {
            const Part::Geometry* geo = getGeometry(*it);

            Part::Geometry* geosym;

            auto gf = GeometryFacade::getFacade(geo);

            if (gf->isInternalAligned()) {
                // only add this geometry if the corresponding geometry it defines is also in the
                // list.
                int definedGeo = GeoEnum::GeoUndef;

                for (auto c : Constraints.getValues()) {
                    if (c->Type == Sketcher::InternalAlignment && c->First == *it) {
                        definedGeo = c->Second;
                        break;
                    }
                }

                if (std::find(geoIdList.begin(), geoIdList.end(), definedGeo) != geoIdList.end())
                    geosym = geo->copy();
                else {
                    // we should not mirror internal alignment geometry, unless the element they
                    // define is also mirrored
                    continue;
                }
            }
            else {
                geosym = geo->copy();
            }

            // Handle Geometry
            if (geosym->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                Part::GeomLineSegment* geosymline = static_cast<Part::GeomLineSegment*>(geosym);
                Base::Vector3d sp = geosymline->getStartPoint();
                Base::Vector3d ep = geosymline->getEndPoint();
                Base::Vector3d ssp = sp + 2.0 * (refpoint - sp);
                Base::Vector3d sep = ep + 2.0 * (refpoint - ep);

                geosymline->setPoints(ssp, sep);
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                Part::GeomCircle* geosymcircle = static_cast<Part::GeomCircle*>(geosym);
                Base::Vector3d cp = geosymcircle->getCenter();

                geosymcircle->setCenter(cp + 2.0 * (refpoint - cp));
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                Part::GeomArcOfCircle* geoaoc = static_cast<Part::GeomArcOfCircle*>(geosym);
                Base::Vector3d sp = geoaoc->getStartPoint(true);
                Base::Vector3d ep = geoaoc->getEndPoint(true);
                Base::Vector3d cp = geoaoc->getCenter();

                Base::Vector3d ssp = sp + 2.0 * (refpoint - sp);
                Base::Vector3d sep = ep + 2.0 * (refpoint - ep);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                double theta1 = Base::fmod(atan2(ssp.y - scp.y, ssp.x - scp.x), 2.f * M_PI);
                double theta2 = Base::fmod(atan2(sep.y - scp.y, sep.x - scp.x), 2.f * M_PI);

                geoaoc->setCenter(scp);
                geoaoc->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                Part::GeomEllipse* geosymellipse = static_cast<Part::GeomEllipse*>(geosym);
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
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                Part::GeomArcOfEllipse* geosymaoe = static_cast<Part::GeomArcOfEllipse*>(geosym);
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
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                Part::GeomArcOfHyperbola* geosymaoe =
                    static_cast<Part::GeomArcOfHyperbola*>(geosym);
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
                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                Part::GeomArcOfParabola* geosymaoe = static_cast<Part::GeomArcOfParabola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                /*double df= geosymaoe->getFocal();*/
                Base::Vector3d f1 = geosymaoe->getFocus();

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymaoe->setXAxisDir(sf1 - scp);
                geosymaoe->setCenter(scp);

                isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                Part::GeomBSplineCurve* geosymbsp = static_cast<Part::GeomBSplineCurve*>(geosym);

                std::vector<Base::Vector3d> poles = geosymbsp->getPoles();

                for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end();
                     ++it) {
                    (*it) = (*it) + 2.0 * (refpoint - (*it));
                }

                geosymbsp->setPoles(poles);

                // isStartEndInverted.insert(std::make_pair(*it, false));
            }
            else if (geosym->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                Part::GeomPoint* geosympoint = static_cast<Part::GeomPoint*>(geosym);
                Base::Vector3d cp = geosympoint->getPoint();

                geosympoint->setPoint(cp + 2.0 * (refpoint - cp));
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

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker lock(internaltransaction, true);
        Geometry.setValues(std::move(newgeoVals));

        for (std::vector<Constraint*>::const_iterator it = constrvals.begin();
             it != constrvals.end();
             ++it) {
            // we look in the map, because we might have skipped internal alignment geometry
            auto fit = geoIdMap.find((*it)->First);

            if (fit != geoIdMap.end()) {// if First of constraint is in geoIdList

                if ((*it)->Second == GeoEnum::GeoUndef /*&& (*it)->Third == GeoEnum::GeoUndef*/) {
                    if (refIsAxisAligned) {
                        // in this case we want to keep the Vertical, Horizontal constraints
                        // DistanceX ,and DistanceY constraints should also be possible to keep in
                        // this case, but keeping them causes segfault, not sure why.

                        if ((*it)->Type != Sketcher::DistanceX
                            && (*it)->Type != Sketcher::DistanceY) {
                            Constraint* constNew = (*it)->copy();
                            constNew->First = fit->second;
                            newconstrVals.push_back(constNew);
                        }
                    }
                    else if ((*it)->Type != Sketcher::DistanceX
                             && (*it)->Type != Sketcher::DistanceY
                             && (*it)->Type != Sketcher::Vertical
                             && (*it)->Type != Sketcher::Horizontal) {
                        // this includes all non-directional single GeoId constraints, as radius,
                        // diameter, weight,...

                        Constraint* constNew = (*it)->copy();
                        constNew->First = fit->second;
                        newconstrVals.push_back(constNew);
                    }
                }
                else {// other geoids intervene in this constraint

                    auto sit = geoIdMap.find((*it)->Second);

                    if (sit != geoIdMap.end()) {// Second is also in the list

                        if ((*it)->Third == GeoEnum::GeoUndef) {
                            if ((*it)->Type == Sketcher::Coincident
                                || (*it)->Type == Sketcher::Perpendicular
                                || (*it)->Type == Sketcher::Parallel
                                || (*it)->Type == Sketcher::Tangent
                                || (*it)->Type == Sketcher::Distance
                                || (*it)->Type == Sketcher::Equal || (*it)->Type == Sketcher::Angle
                                || (*it)->Type == Sketcher::PointOnObject
                                || (*it)->Type == Sketcher::InternalAlignment) {
                                Constraint* constNew = (*it)->copy();

                                constNew->First = fit->second;
                                constNew->Second = sit->second;
                                if (isStartEndInverted[(*it)->First]) {
                                    if ((*it)->FirstPos == Sketcher::PointPos::start)
                                        constNew->FirstPos = Sketcher::PointPos::end;
                                    else if ((*it)->FirstPos == Sketcher::PointPos::end)
                                        constNew->FirstPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[(*it)->Second]) {
                                    if ((*it)->SecondPos == Sketcher::PointPos::start)
                                        constNew->SecondPos = Sketcher::PointPos::end;
                                    else if ((*it)->SecondPos == Sketcher::PointPos::end)
                                        constNew->SecondPos = Sketcher::PointPos::start;
                                }

                                if (constNew->Type == Tangent || constNew->Type == Perpendicular)
                                    AutoLockTangencyAndPerpty(constNew, true);

                                if (((*it)->Type == Sketcher::Angle)
                                    && (refPosId == Sketcher::PointPos::none)) {
                                    constNew->setValue(-(*it)->getValue());
                                }

                                newconstrVals.push_back(constNew);
                            }
                        }
                        else {// three GeoIds intervene in constraint
                            auto tit = geoIdMap.find((*it)->Third);

                            if (tit != geoIdMap.end()) {// Third is also in the list
                                Constraint* constNew = (*it)->copy();
                                constNew->First = fit->second;
                                constNew->Second = sit->second;
                                constNew->Third = tit->second;
                                if (isStartEndInverted[(*it)->First]) {
                                    if ((*it)->FirstPos == Sketcher::PointPos::start)
                                        constNew->FirstPos = Sketcher::PointPos::end;
                                    else if ((*it)->FirstPos == Sketcher::PointPos::end)
                                        constNew->FirstPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[(*it)->Second]) {
                                    if ((*it)->SecondPos == Sketcher::PointPos::start)
                                        constNew->SecondPos = Sketcher::PointPos::end;
                                    else if ((*it)->SecondPos == Sketcher::PointPos::end)
                                        constNew->SecondPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[(*it)->Third]) {
                                    if ((*it)->ThirdPos == Sketcher::PointPos::start)
                                        constNew->ThirdPos = Sketcher::PointPos::end;
                                    else if ((*it)->ThirdPos == Sketcher::PointPos::end)
                                        constNew->ThirdPos = Sketcher::PointPos::start;
                                }
                                newconstrVals.push_back(constNew);
                            }
                        }
                    }
                }
            }
        }

        if (newconstrVals.size() > constrvals.size())
            Constraints.setValues(std::move(newconstrVals));
    }

    // we delayed update, so trigger it now.
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    return Geometry.getSize() - 1;
}

int SketchObject::addCopy(const std::vector<int>& geoIdList, const Base::Vector3d& displacement,
                          bool moveonly /*=false*/, bool clone /*=false*/, int csize /*=2*/,
                          int rsize /*=1*/, bool constraindisplacement /*= false*/,
                          double perpscale /*= 1.0*/)
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

                    if (std::find(newgeoIdList.begin(), newgeoIdList.end(), definedGeo)
                        == newgeoIdList.end()) {
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
                if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()
                    || geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
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

                    if (std::find(newgeoIdList.begin(), newgeoIdList.end(), definedGeo)
                        == newgeoIdList.end()) {
                        // we should not copy internal alignment geometry, unless the element they
                        // define is also mirrored
                        continue;
                    }
                }

                // We have already cloned all geometry and constraints, we only need a copy if not
                // moving
                if (!moveonly)
                    geocopy = geo->copy();
                else
                    geocopy = newgeoVals[*it];

                // Handle Geometry
                if (geocopy->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
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
                else if (geocopy->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                    Part::GeomCircle* geosymcircle = static_cast<Part::GeomCircle*>(geocopy);
                    Base::Vector3d cp = geosymcircle->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geosymcircle->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else if (geocopy->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                    Part::GeomArcOfCircle* geoaoc = static_cast<Part::GeomArcOfCircle*>(geocopy);
                    Base::Vector3d cp = geoaoc->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoc->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoc->getStartPoint(true);
                }
                else if (geocopy->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                    Part::GeomEllipse* geosymellipse = static_cast<Part::GeomEllipse*>(geocopy);
                    Base::Vector3d cp = geosymellipse->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geosymellipse->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else if (geocopy->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                    Part::GeomArcOfEllipse* geoaoe = static_cast<Part::GeomArcOfEllipse*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                    Part::GeomArcOfHyperbola* geoaoe =
                        static_cast<Part::GeomArcOfHyperbola*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                    Part::GeomArcOfParabola* geoaoe =
                        static_cast<Part::GeomArcOfParabola*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                    Part::GeomBSplineCurve* geobsp = static_cast<Part::GeomBSplineCurve*>(geocopy);

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
                else if (geocopy->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                    Part::GeomPoint* geopoint = static_cast<Part::GeomPoint*>(geocopy);
                    Base::Vector3d cp = geopoint->getPoint();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;
                    geopoint->setPoint(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else {
                    Base::Console().Error("Unsupported Geometry!! Just skipping it.\n");
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
                                    if (getGeometry((*it)->First)->getTypeId()
                                        == Part::GeomLineSegment::getClassTypeId()) {
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
        Base::StateLocker lock(internaltransaction, true);
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

int SketchObject::exposeInternalGeometry(int GeoId)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const Part::Geometry* geo = getGeometry(GeoId);
    // Only for supported types
    if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()
        || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        // First we search what has to be restored
        bool major = false;
        bool minor = false;
        bool focus1 = false;
        bool focus2 = false;

        const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {
            if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
                switch ((*it)->AlignmentType) {
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
        }

        int currentgeoid = getHighestCurveIndex();
        int incrgeo = 0;

        Base::Vector3d center;
        double majord;
        double minord;
        Base::Vector3d majdir;

        std::vector<Part::Geometry*> igeo;
        std::vector<Constraint*> icon;

        if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            const Part::GeomEllipse* ellipse = static_cast<const Part::GeomEllipse*>(geo);

            center = ellipse->getCenter();
            majord = ellipse->getMajorRadius();
            minord = ellipse->getMinorRadius();
            majdir = ellipse->getMajorAxisDir();
        }
        else {
            const Part::GeomArcOfEllipse* aoe = static_cast<const Part::GeomArcOfEllipse*>(geo);

            center = aoe->getCenter();
            majord = aoe->getMajorRadius();
            minord = aoe->getMinorRadius();
            majdir = aoe->getMajorAxisDir();
        }

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

        this->addGeometry(igeo, true);
        this->addConstraints(icon);

        for (std::vector<Part::Geometry*>::iterator it = igeo.begin(); it != igeo.end(); ++it) {
            if (*it)
                delete *it;
        }

        for (std::vector<Constraint*>::iterator it = icon.begin(); it != icon.end(); ++it) {
            if (*it)
                delete *it;
        }

        icon.clear();
        igeo.clear();

        return incrgeo;// number of added elements
    }
    else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
        // First we search what has to be restored
        bool major = false;
        bool minor = false;
        bool focus = false;

        const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {
            if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
                switch ((*it)->AlignmentType) {
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
        }

        int currentgeoid = getHighestCurveIndex();
        int incrgeo = 0;

        const Part::GeomArcOfHyperbola* aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);

        Base::Vector3d center = aoh->getCenter();
        double majord = aoh->getMajorRadius();
        double minord = aoh->getMinorRadius();
        Base::Vector3d majdir = aoh->getMajorAxisDir();

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

        this->addGeometry(igeo, true);
        this->addConstraints(icon);

        for (std::vector<Part::Geometry*>::iterator it = igeo.begin(); it != igeo.end(); ++it)
            if (*it)
                delete *it;

        for (std::vector<Constraint*>::iterator it = icon.begin(); it != icon.end(); ++it)
            if (*it)
                delete *it;

        icon.clear();
        igeo.clear();

        return incrgeo;// number of added elements
    }
    else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
        // First we search what has to be restored
        bool focus = false;
        bool focus_to_vertex = false;

        const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {
            if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
                switch ((*it)->AlignmentType) {
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
        }

        int currentgeoid = getHighestCurveIndex();
        int incrgeo = 0;

        const Part::GeomArcOfParabola* aop = static_cast<const Part::GeomArcOfParabola*>(geo);

        Base::Vector3d center = aop->getCenter();
        Base::Vector3d focusp = aop->getFocus();

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

        this->addGeometry(igeo, true);
        this->addConstraints(icon);

        for (std::vector<Part::Geometry*>::iterator it = igeo.begin(); it != igeo.end(); ++it) {
            if (*it)
                delete *it;
        }

        for (std::vector<Constraint*>::iterator it = icon.begin(); it != icon.end(); ++it) {
            if (*it)
                delete *it;
        }

        icon.clear();
        igeo.clear();

        return incrgeo;// number of added elements
    }
    else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {

        const Part::GeomBSplineCurve* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
        // First we search what has to be restored
        std::vector<bool> controlpoints(bsp->countPoles());
        std::vector<int> controlpointgeoids(bsp->countPoles());

        std::vector<bool> knotpoints(bsp->countKnots());
        std::vector<int> knotgeoids(bsp->countKnots());

        bool isfirstweightconstrained = false;

        std::vector<bool>::iterator itb;
        std::vector<int>::iterator it;

        for (it = controlpointgeoids.begin(), itb = controlpoints.begin();
             it != controlpointgeoids.end() && itb != controlpoints.end();
             ++it, ++itb) {
            (*it) = -1;
            (*itb) = false;
        }

        for (it = knotgeoids.begin(), itb = knotpoints.begin();
             it != knotgeoids.end() && itb != knotpoints.end();
             ++it, ++itb) {
            (*it) = -1;
            (*itb) = false;
        }

        const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

        // search for existing poles
        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {
            if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
                switch ((*it)->AlignmentType) {
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

        if (controlpoints[0]) {
            // search for first pole weight constraint
            for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                 it != vals.end();
                 ++it) {
                if ((*it)->Type == Sketcher::Weight && (*it)->First == controlpointgeoids[0]) {
                    isfirstweightconstrained = true;
                }
            }
        }

        int currentgeoid = getHighestCurveIndex();
        int incrgeo = 0;

        std::vector<Part::Geometry*> igeo;
        std::vector<Constraint*> icon;

        std::vector<Base::Vector3d> poles = bsp->getPoles();
        std::vector<double> weights = bsp->getWeights();
        std::vector<double> knots = bsp->getKnots();

        double distance_p0_p1 = (poles[1] - poles[0]).Length();// for visual purposes only

        int index = 0;

        for (it = controlpointgeoids.begin(), itb = controlpoints.begin();
             it != controlpointgeoids.end() && itb != controlpoints.end();
             ++it, ++itb, index++) {

            if (!(*itb))// if controlpoint not existing
            {
                Part::GeomCircle* pc = new Part::GeomCircle();
                pc->setCenter(poles[index]);
                pc->setRadius(distance_p0_p1 / 6);

                igeo.push_back(pc);

                Sketcher::Constraint* newConstr = new Sketcher::Constraint();
                newConstr->Type = Sketcher::InternalAlignment;
                newConstr->AlignmentType = Sketcher::BSplineControlPoint;
                newConstr->First = currentgeoid + incrgeo + 1;
                newConstr->FirstPos = Sketcher::PointPos::mid;
                newConstr->Second = GeoId;
                newConstr->InternalAlignmentIndex = index;

                icon.push_back(newConstr);

                if (it != controlpointgeoids.begin()) {
                    if (isfirstweightconstrained && weights[0] == weights[index]) {
                        // if pole-weight newly created AND first weight is radius-constrained,
                        // AND these weights are equal, constrain them to be equal
                        Sketcher::Constraint* newConstr2 = new Sketcher::Constraint();
                        newConstr2->Type = Sketcher::Equal;
                        newConstr2->First = currentgeoid + incrgeo + 1;
                        newConstr2->FirstPos = Sketcher::PointPos::none;
                        newConstr2->Second = controlpointgeoids[0];
                        newConstr2->SecondPos = Sketcher::PointPos::none;

                        icon.push_back(newConstr2);
                    }
                }
                else {
                    controlpointgeoids[0] = currentgeoid + incrgeo + 1;
                    if (weights[0] == 1.0) {
                        // if the first weight is 1.0 it's probably going to be non-rational
                        Sketcher::Constraint* newConstr3 = new Sketcher::Constraint();
                        newConstr3->Type = Sketcher::Weight;
                        newConstr3->First = controlpointgeoids[0];
                        newConstr3->setValue(weights[0]);

                        icon.push_back(newConstr3);

                        isfirstweightconstrained = true;
                    }
                }
                incrgeo++;
            }
        }

        index = 0;

        for (it = knotgeoids.begin(), itb = knotpoints.begin();
             it != knotgeoids.end() && itb != knotpoints.end();
             ++it, ++itb, index++) {

            if (!(*itb))// if knot point not existing
            {
                Part::GeomPoint* kp = new Part::GeomPoint();

                kp->setPoint(bsp->pointAtParameter(knots[index]));

                igeo.push_back(kp);

                Sketcher::Constraint* newConstr = new Sketcher::Constraint();
                newConstr->Type = Sketcher::InternalAlignment;
                newConstr->AlignmentType = Sketcher::BSplineKnotPoint;
                newConstr->First = currentgeoid + incrgeo + 1;
                newConstr->FirstPos = Sketcher::PointPos::start;
                newConstr->Second = GeoId;
                newConstr->InternalAlignmentIndex = index;

                icon.push_back(newConstr);

                incrgeo++;
            }
        }

        Q_UNUSED(isfirstweightconstrained);

        this->addGeometry(igeo, true);
        this->addConstraints(icon);

        for (std::vector<Part::Geometry*>::iterator it = igeo.begin(); it != igeo.end(); ++it)
            if (*it)
                delete *it;

        for (std::vector<Constraint*>::iterator it = icon.begin(); it != icon.end(); ++it)
            if (*it)
                delete *it;

        icon.clear();
        igeo.clear();

        return incrgeo;// number of added elements
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
    if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()
        || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
        || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {

        int majorelementindex = -1;
        int minorelementindex = -1;
        int focus1elementindex = -1;
        int focus2elementindex = -1;

        const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {
            if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
                switch ((*it)->AlignmentType) {
                    case Sketcher::EllipseMajorDiameter:
                    case Sketcher::HyperbolaMajor:
                        majorelementindex = (*it)->First;
                        break;
                    case Sketcher::EllipseMinorDiameter:
                    case Sketcher::HyperbolaMinor:
                        minorelementindex = (*it)->First;
                        break;
                    case Sketcher::EllipseFocus1:
                    case Sketcher::HyperbolaFocus:
                        focus1elementindex = (*it)->First;
                        break;
                    case Sketcher::EllipseFocus2:
                        focus2elementindex = (*it)->First;
                        break;
                    default:
                        return -1;
                }
            }
        }

        // Hide unused geometry here
        int majorconstraints = 0;// number of constraints associated to the geoid of the major axis
        int minorconstraints = 0;
        int focus1constraints = 0;
        int focus2constraints = 0;

        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {

            if ((*it)->Second == majorelementindex || (*it)->First == majorelementindex
                || (*it)->Third == majorelementindex)
                majorconstraints++;
            else if ((*it)->Second == minorelementindex || (*it)->First == minorelementindex
                     || (*it)->Third == minorelementindex)
                minorconstraints++;
            else if ((*it)->Second == focus1elementindex || (*it)->First == focus1elementindex
                     || (*it)->Third == focus1elementindex)
                focus1constraints++;
            else if ((*it)->Second == focus2elementindex || (*it)->First == focus2elementindex
                     || (*it)->Third == focus2elementindex)
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
        std::sort(delgeometries.begin(), delgeometries.end());

        if (!delgeometries.empty()) {
            for (std::vector<int>::reverse_iterator it = delgeometries.rbegin();
                 it != delgeometries.rend();
                 ++it) {
                delGeometry(*it, false);
            }
        }

        int ndeleted = delgeometries.size();

        delgeometries.clear();

        return ndeleted;// number of deleted elements
    }
    else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
        // if the focus-to-vertex line is constrained, then never delete the focus
        // if the line is unconstrained, then the line may be deleted,
        // in this case the focus may be deleted if unconstrained.
        int majorelementindex = -1;
        int focus1elementindex = -1;

        const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {
            if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
                switch ((*it)->AlignmentType) {
                    case Sketcher::ParabolaFocus:
                        focus1elementindex = (*it)->First;
                        break;
                    case Sketcher::ParabolaFocalAxis:
                        majorelementindex = (*it)->First;
                        break;
                    default:
                        return -1;
                }
            }
        }

        // Hide unused geometry here
        // number of constraints associated to the geoid of the major axis other than the coincident
        // ones
        int majorconstraints = 0;
        int focus1constraints = 0;

        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {
            if ((*it)->Second == majorelementindex || (*it)->First == majorelementindex
                || (*it)->Third == majorelementindex)
                majorconstraints++;
            else if ((*it)->Second == focus1elementindex || (*it)->First == focus1elementindex
                     || (*it)->Third == focus1elementindex)
                focus1constraints++;
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
        std::sort(delgeometries.begin(), delgeometries.end());

        if (!delgeometries.empty()) {
            for (std::vector<int>::reverse_iterator it = delgeometries.rbegin();
                 it != delgeometries.rend();
                 ++it) {
                delGeometry(*it, false);
            }
        }

        int ndeleted = delgeometries.size();

        delgeometries.clear();

        return ndeleted;// number of deleted elements
    }
    else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {

        const Part::GeomBSplineCurve* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

        // First we search existing IA
        std::vector<int> controlpointgeoids(bsp->countPoles());
        std::vector<int> cpassociatedconstraints(bsp->countPoles());

        std::vector<int> knotgeoids(bsp->countKnots());
        std::vector<int> kassociatedconstraints(bsp->countKnots());

        std::vector<int>::iterator it;
        std::vector<int>::iterator ita;

        for (it = controlpointgeoids.begin(), ita = cpassociatedconstraints.begin();
             it != controlpointgeoids.end() && ita != cpassociatedconstraints.end();
             ++it, ++ita) {
            (*it) = -1;
            (*ita) = 0;
        }

        for (it = knotgeoids.begin(), ita = kassociatedconstraints.begin();
             it != knotgeoids.end() && ita != kassociatedconstraints.end();
             ++it, ++ita) {
            (*it) = -1;
            (*ita) = 0;
        }

        const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

        // search for existing poles
        for (std::vector<Sketcher::Constraint*>::const_iterator jt = vals.begin(); jt != vals.end();
             ++jt) {
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

        for (it = controlpointgeoids.begin(), ita = cpassociatedconstraints.begin();
             it != controlpointgeoids.end() && ita != cpassociatedconstraints.end();
             ++it, ++ita) {
            if ((*it) != -1) {
                // look for a circle at geoid index
                for (std::vector<Sketcher::Constraint*>::const_iterator itc = vals.begin();
                     itc != vals.end();
                     ++itc) {

                    if ((*itc)->Type == Sketcher::Equal) {
                        bool f = false, s = false;
                        for (std::vector<int>::iterator its = controlpointgeoids.begin();
                             its != controlpointgeoids.end();
                             ++its) {
                            if ((*itc)->First == *its) {
                                f = true;
                            }
                            else if ((*itc)->Second == *its) {
                                s = true;
                            }

                            if (f && s) {// the equality constraint is not interpole
                                break;
                            }
                        }

                        // the equality constraint constraints a pole but it is not interpole
                        if (f != s) {
                            (*ita)++;
                        }
                    }
                    // We do not ignore weight constraints as we did with radius constraints,
                    // because the radius magnitude no longer makes sense without the B-Spline.
                }

                if ((*ita) < 2) {// IA
                    delgeometries.push_back((*it));
                }
            }
        }

        for (it = knotgeoids.begin(), ita = kassociatedconstraints.begin();
             it != knotgeoids.end() && ita != kassociatedconstraints.end();
             ++it, ++ita) {
            if ((*it) != -1) {
                // look for a point at geoid index
                for (std::vector<Sketcher::Constraint*>::const_iterator itc = vals.begin();
                     itc != vals.end();
                     ++itc) {
                    if ((*itc)->Second == (*it) || (*itc)->First == (*it)
                        || (*itc)->Third == (*it)) {
                        (*ita)++;
                    }
                }

                if ((*ita) < 2) {// IA
                    delgeometries.push_back((*it));
                }
            }
        }


        if (delgeoid)
            delgeometries.push_back(GeoId);

        int ndeleted = delGeometriesExclusiveList(delgeometries);

        return ndeleted;// number of deleted elements
    }
    else {
        return -1;// not supported type
    }
}

bool SketchObject::convertToNURBS(int GeoId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId > getHighestCurveIndex()
        || (GeoId < 0 && -GeoId > static_cast<int>(ExternalGeo.size())) || GeoId == -1
        || GeoId == -2)
        return false;

    const Part::Geometry* geo = getGeometry(GeoId);

    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId())
        return false;

    const Part::GeomCurve* geo1 = static_cast<const Part::GeomCurve*>(geo);

    Part::GeomBSplineCurve* bspline;

    try {
        bspline = geo1->toNurbs(geo1->getFirstParameter(), geo1->getLastParameter());

        if (geo1->isDerivedFrom(Part::GeomArcOfConic::getClassTypeId())) {
            const Part::GeomArcOfConic* geoaoc = static_cast<const Part::GeomArcOfConic*>(geo1);

            if (geoaoc->isReversed())
                bspline->reverse();
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        // revert to original values
        return false;
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    // Block checks and updates in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker lock(internaltransaction, true);

        if (GeoId < 0) {// external geometry
            newVals.push_back(bspline);
        }
        else {// normal geometry

            newVals[GeoId] = bspline;

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

    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return false;

    const Part::Geometry* geo = getGeometry(GeoId);

    if (geo->getTypeId() != Part::GeomBSplineCurve::getClassTypeId())
        return false;

    const Part::GeomBSplineCurve* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    const Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(bsp->handle());

    std::unique_ptr<Part::GeomBSplineCurve> bspline(new Part::GeomBSplineCurve(curve));

    try {
        int cdegree = bspline->getDegree();

        bspline->increaseDegree(cdegree + degreeincrement);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        return false;
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

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

    if (geo->getTypeId() != Part::GeomBSplineCurve::getClassTypeId())
        return false;

    const Part::GeomBSplineCurve* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    const Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(bsp->handle());

    std::unique_ptr<Part::GeomBSplineCurve> bspline(new Part::GeomBSplineCurve(curve));

    try {
        int cdegree = bspline->getDegree();

        // degree must be >= 1
        int maxdegree = cdegree - degreedecrement;
        if (maxdegree == 0)
            return false;
        bool ok = bspline->approximate(Precision::Confusion(), 20, maxdegree, 0);
        if (!ok)
            return false;
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
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

bool SketchObject::modifyBSplineKnotMultiplicity(int GeoId, int knotIndex, int multiplicityincr)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        THROWMT(Base::ValueError,
                QT_TRANSLATE_NOOP("Exceptions", "BSpline Geometry Index (GeoID) is out of bounds."))

    if (multiplicityincr == 0)// no change in multiplicity
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "You are requesting no change in knot multiplicity."))

    const Part::Geometry* geo = getGeometry(GeoId);

    if (geo->getTypeId() != Part::GeomBSplineCurve::getClassTypeId())
        THROWMT(Base::TypeError,
                QT_TRANSLATE_NOOP("Exceptions",
                                  "The Geometry Index (GeoId) provided is not a B-spline curve."))

    const Part::GeomBSplineCurve* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    int degree = bsp->getDegree();

    if (knotIndex > bsp->countKnots() || knotIndex < 1)// knotindex in OCC 1 -> countKnots
        THROWMT(Base::ValueError,
                QT_TRANSLATE_NOOP("Exceptions",
                                  "The knot index is out of bounds. Note that in accordance with "
                                  "OCC notation, the first knot has index 1 and not zero."))

    std::unique_ptr<Part::GeomBSplineCurve> bspline;

    int curmult = bsp->getMultiplicity(knotIndex);

    // zero is removing the knot, degree is just positional continuity
    if ((curmult + multiplicityincr) > degree)
        THROWMT(Base::ValueError,
                QT_TRANSLATE_NOOP(
                    "Exceptions",
                    "The multiplicity cannot be increased beyond the degree of the B-spline."))

    // zero is removing the knot, degree is just positional continuity
    if ((curmult + multiplicityincr) < 0)
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "The multiplicity cannot be decreased beyond zero."))

    try {
        bspline.reset(static_cast<Part::GeomBSplineCurve*>(bsp->clone()));

        if (multiplicityincr > 0) {// increase multiplicity
            bspline->increaseMultiplicity(knotIndex, curmult + multiplicityincr);
        }
        else {// decrease multiplicity
            bool result = bspline->removeKnot(knotIndex, curmult + multiplicityincr, 1E6);

            if (!result)
                THROWMT(
                    Base::CADKernelError,
                    QT_TRANSLATE_NOOP(
                        "Exceptions",
                        "OCC is unable to decrease the multiplicity within the maximum tolerance."))
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        return false;
    }

    // we succeeded with the multiplicity modification, so alignment geometry may be
    // invalid/inconsistent for the new bspline

    std::vector<int> delGeoId;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<Base::Vector3d> newpoles = bspline->getPoles();
    std::vector<int> prevpole(bsp->countPoles());

    for (int i = 0; i < int(poles.size()); i++)
        prevpole[i] = -1;

    int taken = 0;
    for (int j = 0; j < int(poles.size()); j++) {
        for (int i = taken; i < int(newpoles.size()); i++) {
            if (newpoles[i] == poles[j]) {
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

    for (int i = 0; i < int(knots.size()); i++)
        prevknot[i] = -1;

    taken = 0;
    for (int j = 0; j < int(knots.size()); j++) {
        for (int i = taken; i < int(newknots.size()); i++) {
            if (newknots[i] == knots[j]) {
                prevknot[j] = i;
                taken++;
                break;
            }
        }
    }

    const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

    std::vector<Constraint*> newcVals(0);

    // modify pole constraints
    for (std::vector<Sketcher::Constraint*>::const_iterator it = cvals.begin(); it != cvals.end();
         ++it) {
        if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
            if ((*it)->AlignmentType == Sketcher::BSplineControlPoint) {
                if (prevpole[(*it)->InternalAlignmentIndex] != -1) {
                    assert(prevpole[(*it)->InternalAlignmentIndex] < bspline->countPoles());
                    Constraint* newConstr = (*it)->clone();
                    newConstr->InternalAlignmentIndex = prevpole[(*it)->InternalAlignmentIndex];
                    newcVals.push_back(newConstr);
                }
                else {
                    // it is an internal alignment geometry that is no longer valid => delete it and
                    // the pole circle
                    delGeoId.push_back((*it)->First);
                }
            }
            else if ((*it)->AlignmentType == Sketcher::BSplineKnotPoint) {
                if (prevknot[(*it)->InternalAlignmentIndex] != -1) {
                    assert(prevknot[(*it)->InternalAlignmentIndex] < bspline->countKnots());
                    Constraint* newConstr = (*it)->clone();
                    newConstr->InternalAlignmentIndex = prevknot[(*it)->InternalAlignmentIndex];
                    newcVals.push_back(newConstr);
                }
                else {
                    // it is an internal alignment geometry that is no longer valid => delete it and
                    // the knot point
                    delGeoId.push_back((*it)->First);
                }
            }
            else {// it is a bspline geometry, but not a controlpoint or knot
                newcVals.push_back(*it);
            }
        }
        else {
            newcVals.push_back(*it);
        }
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    newVals[GeoId] = bspline.release();

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker lock(internaltransaction, true);
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

    // * DOCUMENTING OCC ISSUE OCC < 6.9.0
    // https://forum.freecad.org/viewtopic.php?f=10&t=9364&start=330#p162528
    //
    // A segmentation fault is generated:
    // Program received signal SIGSEGV, Segmentation fault.
    // #0 /lib/x86_64-linux-gnu/libc.so.6(+0x36cb0) [0x7f4b933bbcb0]
    // #1  0x7f4b0300ea14 in BSplCLib::BuildCache(double, double, bool, int, TColStd_Array1OfReal
    // const&, TColgp_Array1OfPnt const&, TColStd_Array1OfReal const&, TColgp_Array1OfPnt&,
    // TColStd_Array1OfReal&) from /usr/lib/x86_64-linux-gnu/libTKMath.so.10+0x484 #2 0x7f4b033f9582
    // in Geom_BSplineCurve::ValidateCache(double) from
    // /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0x202 #3  0x7f4b033f2a7e in
    // Geom_BSplineCurve::D0(double, gp_Pnt&) const from
    // /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0xde #4  0x7f4b033de1b5 in Geom_Curve::Value(double)
    // const from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0x25 #5  0x7f4b03423d73 in
    // GeomLProp_CurveTool::Value(Handle(Geom_Curve) const&, double, gp_Pnt&) from
    // /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0x13 #6  0x7f4b03427175 in
    // GeomLProp_CLProps::SetParameter(double) from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0x75 #7
    // 0x7f4b0342727d in GeomLProp_CLProps::GeomLProp_CLProps(Handle(Geom_Curve) const&, double,
    // int, double) from /usr/lib/x86_64-linux-gnu/libTKG3d.so.10+0xcd #8  0x7f4b11924b53 in
    // Part::GeomCurve::pointAtParameter(double) const from
    // /home/abdullah/github/freecad-build/Mod/Part/Part.so+0xa7


    return true;
}

bool SketchObject::insertBSplineKnot(int GeoId, double param, int multiplicity)
{
    // TODO: Check if this is still valid: no need to check input data validity as this is an
    // sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // handling unacceptable cases
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "BSpline Geometry Index (GeoID) is out of bounds."));

    if (multiplicity == 0)
        THROWMT(Base::ValueError,
                QT_TRANSLATE_NOOP("Exceptions", "Knot cannot have zero multiplicity."));

    const Part::Geometry* geo = getGeometry(GeoId);

    if (geo->getTypeId() != Part::GeomBSplineCurve::getClassTypeId())
        THROWMT(Base::TypeError,
                QT_TRANSLATE_NOOP("Exceptions",
                                  "The Geometry Index (GeoId) provided is not a B-spline curve."));

    const Part::GeomBSplineCurve* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    int degree = bsp->getDegree();
    double firstParam = bsp->getFirstParameter();
    double lastParam = bsp->getLastParameter();

    if (multiplicity > degree)
        THROWMT(Base::ValueError,
                QT_TRANSLATE_NOOP(
                    "Exceptions",
                    "Knot multiplicity cannot be higher than the degree of the BSpline."));

    if (param > lastParam || param < firstParam)
        THROWMT(Base::ValueError,
                QT_TRANSLATE_NOOP("Exceptions",
                                  "Knot cannot be inserted outside the BSpline parameter range."));

    std::unique_ptr<Part::GeomBSplineCurve> bspline;

    // run the command
    try {
        bspline.reset(static_cast<Part::GeomBSplineCurve*>(bsp->clone()));

        bspline->insertKnot(param, multiplicity);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        return false;
    }

    // once command is run update the internal geometries
    std::vector<int> delGeoId;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<Base::Vector3d> newpoles = bspline->getPoles();
    std::vector<int> prevpole(bsp->countPoles());

    for (int i = 0; i < int(poles.size()); i++)
        prevpole[i] = -1;

    int taken = 0;
    for (int j = 0; j < int(poles.size()); j++) {
        for (int i = taken; i < int(newpoles.size()); i++) {
            if (newpoles[i] == poles[j]) {
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

    for (int i = 0; i < int(knots.size()); i++)
        prevknot[i] = -1;

    taken = 0;
    for (int j = 0; j < int(knots.size()); j++) {
        for (int i = taken; i < int(newknots.size()); i++) {
            if (newknots[i] == knots[j]) {
                prevknot[j] = i;
                taken++;
                break;
            }
        }
    }

    const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

    std::vector<Constraint*> newcVals(0);

    // modify pole constraints
    for (std::vector<Sketcher::Constraint*>::const_iterator it = cvals.begin(); it != cvals.end();
         ++it) {
        if ((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId) {
            if ((*it)->AlignmentType == Sketcher::BSplineControlPoint) {
                if (prevpole[(*it)->InternalAlignmentIndex] != -1) {
                    assert(prevpole[(*it)->InternalAlignmentIndex] < bspline->countPoles());
                    Constraint* newConstr = (*it)->clone();
                    newConstr->InternalAlignmentIndex = prevpole[(*it)->InternalAlignmentIndex];
                    newcVals.push_back(newConstr);
                }
                else {
                    // it is an internal alignment geometry that is no longer valid => delete it and
                    // the pole circle
                    delGeoId.push_back((*it)->First);
                }
            }
            else if ((*it)->AlignmentType == Sketcher::BSplineKnotPoint) {
                if (prevknot[(*it)->InternalAlignmentIndex] != -1) {
                    assert(prevknot[(*it)->InternalAlignmentIndex] < bspline->countKnots());
                    Constraint* newConstr = (*it)->clone();
                    newConstr->InternalAlignmentIndex = prevknot[(*it)->InternalAlignmentIndex];
                    newcVals.push_back(newConstr);
                }
                else {
                    // it is an internal alignment geometry that is no longer valid => delete it and
                    // the knot point
                    delGeoId.push_back((*it)->First);
                }
            }
            else {
                // it is a bspline geometry, but not a controlpoint or knot
                newcVals.push_back(*it);
            }
        }
        else {
            newcVals.push_back(*it);
        }
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    newVals[GeoId] = bspline.release();

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker lock(internaltransaction, true);
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
    }
    else {
        Geometry.touch();
    }

    // handle this last return
    return true;
}

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

    if (psObj->ExternalGeometry.getSize() > 0) {
        std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
        std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

        const std::vector<DocumentObject*> originalObjects = Objects;
        const std::vector<std::string> originalSubElements = SubElements;

        std::vector<DocumentObject*> sObjects = psObj->ExternalGeometry.getValues();
        std::vector<std::string> sSubElements = psObj->ExternalGeometry.getSubValues();

        if (Objects.size() != SubElements.size() || sObjects.size() != sSubElements.size()) {
            assert(0 /*counts of objects and subelements in external geometry links do not match*/);
            Base::Console().Error("Internal error: counts of objects and subelements in external "
                                  "geometry links do not match\n");
            return -1;
        }

        int si = 0;
        for (auto& sobj : sObjects) {
            int i = 0;
            for (auto& obj : Objects) {
                if (obj == sobj && SubElements[i] == sSubElements[si]) {
                    Base::Console().Error(
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
            Base::Console().Error("%s\n", e.what());
            // revert to original values
            ExternalGeometry.setValues(originalObjects, originalSubElements);
            return -1;
        }

        solverNeedsUpdate = true;
    }

    for (std::vector<Part::Geometry*>::const_iterator it = svals.begin(); it != svals.end(); ++it) {
        Part::Geometry* geoNew = (*it)->copy();
        if (construction && geoNew->getTypeId() != Part::GeomPoint::getClassTypeId()) {
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
        Base::StateLocker lock(internaltransaction, true);
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
                // (there is a plausible alternative for a slightly different use case to copy the
                // expression of the parent if one is existing)
                /*
                 *           App::PropertyExpressionEngine::ExpressionInfo expr_info =
                 * psObj->getExpression(path);
                 *
                 *           if (expr_info.expression)*/
                // App::Expression * expr = parse(this, const std::string& buffer);
                setExpression(Constraints.createPath(nextcid), expr);
            }
        }
    }

    return svals.size();
}

int SketchObject::addExternal(App::DocumentObject* Obj, const char* SubName)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // so far only externals to the support of the sketch and datum features
    if (!isExternalAllowed(Obj->getDocument(), Obj))
        return -1;

    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string> originalSubElements = SubElements;

    if (Objects.size() != SubElements.size()) {
        assert(0 /*counts of objects and subelements in external geometry links do not match*/);
        Base::Console().Error("Internal error: counts of objects and subelements in external "
                              "geometry links do not match\n");
        return -1;
    }

    for (size_t i = 0; i < Objects.size(); ++i) {
        if (Objects[i] == Obj && std::string(SubName) == SubElements[i]) {
            Base::Console().Error("Link to %s already exists in this sketch.\n", SubName);
            return -1;
        }
    }

    // add the new ones
    Objects.push_back(Obj);
    SubElements.emplace_back(SubName);

    // set the Link list.
    ExternalGeometry.setValues(Objects, SubElements);
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects, originalSubElements);
        return -1;
    }

    acceptGeometry();// This may need to be refactored into onChanged for ExternalGeometry

    solverNeedsUpdate = true;
    return ExternalGeometry.getValues().size() - 1;
}

int SketchObject::delExternal(int ExtGeoId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

    if (ExtGeoId < 0 || ExtGeoId >= int(SubElements.size()))
        return -1;

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string> originalSubElements = SubElements;

    Objects.erase(Objects.begin() + ExtGeoId);
    SubElements.erase(SubElements.begin() + ExtGeoId);

    const std::vector<Constraint*>& constraints = Constraints.getValues();
    std::vector<Constraint*> newConstraints;
    std::vector<Constraint*> copiedConstraints;
    int GeoId = GeoEnum::RefExt - ExtGeoId;
    for (auto cstr : constraints) {
        if (cstr->First != GeoId && cstr->Second != GeoId && cstr->Third != GeoId) {
            auto copiedConstr = cstr;
            if (copiedConstr->First < GeoId && copiedConstr->First != GeoEnum::GeoUndef) {
                if (cstr == copiedConstr)
                    copiedConstr = cstr->clone();
                copiedConstr->First += 1;
            }
            if (copiedConstr->Second < GeoId && copiedConstr->Second != GeoEnum::GeoUndef) {
                if (cstr == copiedConstr)
                    copiedConstr = cstr->clone();
                copiedConstr->Second += 1;
            }
            if (copiedConstr->Third < GeoId && copiedConstr->Third != GeoEnum::GeoUndef) {
                if (cstr == copiedConstr)
                    copiedConstr = cstr->clone();
                copiedConstr->Third += 1;
            }

            newConstraints.push_back(copiedConstr);
            if (cstr != copiedConstr)
                copiedConstraints.push_back(copiedConstr);
        }
    }

    ExternalGeometry.setValues(Objects, SubElements);
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects, originalSubElements);
        for (Constraint* it : copiedConstraints)
            delete it;
        return -1;
    }

    solverNeedsUpdate = true;
    Constraints.setValues(std::move(newConstraints));
    acceptGeometry();// This may need to be refactored into OnChanged for ExternalGeometry.
    return 0;
}

int SketchObject::delAllExternal()
{
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

    for (std::vector<Constraint*>::const_iterator it = constraints.begin(); it != constraints.end();
         ++it) {
        if ((*it)->First > GeoEnum::RefExt
            && ((*it)->Second > GeoEnum::RefExt || (*it)->Second == GeoEnum::GeoUndef)
            && ((*it)->Third > GeoEnum::RefExt || (*it)->Third == GeoEnum::GeoUndef)) {
            Constraint* copiedConstr = (*it)->clone();

            newConstraints.push_back(copiedConstr);
        }
    }

    ExternalGeometry.setValues(Objects, SubElements);
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects, originalSubElements);
        for (Constraint* it : newConstraints)
            delete it;
        return -1;
    }

    solverNeedsUpdate = true;
    Constraints.setValues(std::move(newConstraints));
    acceptGeometry();// This may need to be refactored into OnChanged for ExternalGeometry
    return 0;
}

int SketchObject::delConstraintsToExternal()
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
    if (noRecomputes)
        solve();

    return 0;
}

int SketchObject::getCompleteGeometryIndex(int GeoId) const
{
    if (GeoId >= 0) {
        if (GeoId < int(Geometry.getSize()))
            return GeoId;
    }
    else if (-GeoId <= int(ExternalGeo.size()))
        return -GeoId - 1;

    return GeoEnum::GeoUndef;
}

int SketchObject::getGeoIdFromCompleteGeometryIndex(int completeGeometryIndex) const
{
    int completeGeometryCount = int(Geometry.getSize() + ExternalGeo.size());

    if (completeGeometryIndex < 0 || completeGeometryIndex >= completeGeometryCount)
        return GeoEnum::GeoUndef;

    if (completeGeometryIndex < Geometry.getSize())
        return completeGeometryIndex;
    else
        return (completeGeometryIndex - completeGeometryCount);
}

std::unique_ptr<const GeometryFacade> SketchObject::getGeometryFacade(int GeoId) const
{
    return GeometryFacade::getFacade(getGeometry(GeoId));
}

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

// Auxiliary Method: returns vector projection in XYZ space
#if 0
static gp_Vec ProjVecOnPlane_XYZ( const gp_Vec& V, const gp_Pln& Pl)
{
  return V.Dot(Pl.Position().XDirection()) * Pl.Position().XDirection() +
         V.Dot(Pl.Position().YDirection()) * Pl.Position().YDirection();
}
#endif

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

bool SketchObject::evaluateSupport()
{
    // returns false if the shape is broken, null or non-planar
    App::DocumentObject* link = Support.getValue();
    if (!link || !link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return false;
    return true;
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
        try {
            if (Obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
                const Part::Datum* datum = static_cast<const Part::Datum*>(Obj);
                refSubShape = datum->getShape();
            }
            else {
                const Part::Feature* refObj = static_cast<const Part::Feature*>(Obj);
                const Part::TopoShape& refShape = refObj->Shape.getShape();
                refSubShape = refShape.getSubShape(SubElement.c_str());
            }
        }
        catch (Standard_Failure&) {
            rebuild = true;
            Objects.erase(Objects.begin() + i);
            SubElements.erase(SubElements.begin() + i);

            const std::vector<Constraint*>& constraints = Constraints.getValues();
            std::vector<Constraint*> newConstraints(0);
            int GeoId = GeoEnum::RefExt - i;
            for (std::vector<Constraint*>::const_iterator it = constraints.begin();
                 it != constraints.end();
                 ++it) {
                if ((*it)->First != GeoId && (*it)->Second != GeoId && (*it)->Third != GeoId) {
                    Constraint* copiedConstr = (*it)->clone();
                    if (copiedConstr->First < GeoId && copiedConstr->First != GeoEnum::GeoUndef)
                        copiedConstr->First += 1;
                    if (copiedConstr->Second < GeoId && copiedConstr->Second != GeoEnum::GeoUndef)
                        copiedConstr->Second += 1;
                    if (copiedConstr->Third < GeoId && copiedConstr->Third != GeoEnum::GeoUndef)
                        copiedConstr->Third += 1;

                    newConstraints.push_back(copiedConstr);
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

void SketchObject::rebuildExternalGeometry()
{
    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

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

    for (std::vector<Part::Geometry*>::iterator it = ExternalGeo.begin(); it != ExternalGeo.end();
         ++it)
        if (*it)
            delete *it;
    ExternalGeo.clear();
    Part::GeomLineSegment* HLine = new Part::GeomLineSegment();
    Part::GeomLineSegment* VLine = new Part::GeomLineSegment();
    HLine->setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(1, 0, 0));
    VLine->setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(0, 1, 0));
    GeometryFacade::setConstruction(HLine, true);
    GeometryFacade::setConstruction(VLine, true);
    ExternalGeo.push_back(HLine);
    ExternalGeo.push_back(VLine);
    for (int i = 0; i < int(Objects.size()); i++) {
        const App::DocumentObject* Obj = Objects[i];
        const std::string SubElement = SubElements[i];

        TopoDS_Shape refSubShape;

        if (Obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
            const Part::Datum* datum = static_cast<const Part::Datum*>(Obj);
            refSubShape = datum->getShape();
        }
        else if (Obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            try {
                const Part::Feature* refObj = static_cast<const Part::Feature*>(Obj);
                const Part::TopoShape& refShape = refObj->Shape.getShape();
                refSubShape = refShape.getSubShape(SubElement.c_str());
            }
            catch (Standard_Failure& e) {
                throw Base::CADKernelError(e.GetMessageString());
            }
        }
        else if (Obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
            const App::Plane* pl = static_cast<const App::Plane*>(Obj);
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
        else {
            throw Base::TypeError(
                "Datum feature type is not yet supported as external geometry for a sketch");
        }

        switch (refSubShape.ShapeType()) {
            case TopAbs_FACE: {
                const TopoDS_Face& face = TopoDS::Face(refSubShape);
                BRepAdaptor_Surface surface(face);
                if (surface.GetType() == GeomAbs_Plane) {
                    // Check that the plane is perpendicular to the sketch plane
                    Geom_Plane plane = surface.Plane();
                    gp_Dir dnormal = plane.Axis().Direction();
                    gp_Dir snormal = sketchPlane.Axis().Direction();
                    if (fabs(dnormal.Angle(snormal) - M_PI_2) < Precision::Confusion()) {
                        // Get vector that is normal to both sketch plane normal and plane normal.
                        // This is the line's direction
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
                    }
                    else {
                        throw Base::ValueError(
                            "Selected external reference plane must be normal to sketch plane");
                    }
                }
                else {
                    throw Base::ValueError(
                        "Non-planar faces are not yet supported for external geometry of sketches");
                }
            } break;
            case TopAbs_EDGE: {
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

                        GeomAPI_ProjectPointOnSurf proj(cnt, gPlane);
                        cnt = proj.NearestPoint();
                        circle.SetLocation(cnt);
                        cnt.Transform(mov);
                        circle.Transform(mov);

                        if (beg.SquareDistance(end) < Precision::Confusion()) {
                            Part::GeomCircle* gCircle = new Part::GeomCircle();
                            gCircle->setRadius(circle.Radius());
                            gCircle->setCenter(Base::Vector3d(cnt.X(), cnt.Y(), cnt.Z()));

                            GeometryFacade::setConstruction(gCircle, true);
                            ExternalGeo.push_back(gCircle);
                        }
                        else {
                            Part::GeomArcOfCircle* gArc = new Part::GeomArcOfCircle();
                            Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
                            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(
                                hCircle, curve.FirstParameter(), curve.LastParameter());
                            gArc->setHandle(tCurve);
                            GeometryFacade::setConstruction(gArc, true);
                            ExternalGeo.push_back(gArc);
                        }
                    }
                    else {
                        // creates an ellipse or a segment

                        gp_Dir vec1 = sketchPlane.Axis().Direction();
                        gp_Dir vec2 = curve.Circle().Axis().Direction();
                        gp_Circ origCircle = curve.Circle();

                        if (vec1.IsNormal(
                                vec2, Precision::Angular())) {// circle's normal vector in plane:
                            //   projection is a line
                            //   define center by projection
                            gp_Pnt cnt = origCircle.Location();
                            GeomAPI_ProjectPointOnSurf proj(cnt, gPlane);
                            cnt = proj.NearestPoint();

                            gp_Dir dirOrientation = gp_Dir(vec1 ^ vec2);
                            gp_Dir dirLine(dirOrientation);

                            Part::GeomLineSegment* projectedSegment = new Part::GeomLineSegment();
                            Geom_Line ligne(cnt, dirLine);// helper object to compute end points
                            gp_Pnt P1, P2;                // end points of the segment, OCC style

                            ligne.D0(-origCircle.Radius(), P1);
                            ligne.D0(origCircle.Radius(), P2);

                            if (!curve.IsClosed()) {// arc of circle

                                // start point of arc of circle
                                gp_Pnt pntF = curve.Value(curve.FirstParameter());
                                // end point of arc of circle
                                gp_Pnt pntL = curve.Value(curve.LastParameter());

                                double alpha =
                                    dirOrientation.AngleWithRef(curve.Circle().XAxis().Direction(),
                                                                curve.Circle().Axis().Direction());

                                double baseAngle = curve.FirstParameter();

                                int tours = 0;
                                double startAngle = baseAngle + alpha;
                                // bring startAngle back in [-pi/2 , 3pi/2[
                                while (startAngle < -M_PI / 2.0 && tours < 10) {
                                    startAngle = baseAngle + ++tours * 2.0 * M_PI + alpha;
                                }
                                while (startAngle >= 3.0 * M_PI / 2.0 && tours > -10) {
                                    startAngle = baseAngle + --tours * 2.0 * M_PI + alpha;
                                }

                                // apply same offset to end angle
                                double endAngle = curve.LastParameter() + startAngle - baseAngle;

                                if (startAngle <= 0.0) {
                                    if (endAngle <= 0.0) {
                                        P1 = ProjPointOnPlane_XYZ(pntF, sketchPlane);
                                        P2 = ProjPointOnPlane_XYZ(pntL, sketchPlane);
                                    }
                                    else {
                                        if (endAngle <= fabs(startAngle)) {
                                            // P2 = P2 already defined
                                            P1 = ProjPointOnPlane_XYZ(pntF, sketchPlane);
                                        }
                                        else if (endAngle < M_PI) {
                                            // P2 = P2, already defined
                                            P1 = ProjPointOnPlane_XYZ(pntL, sketchPlane);
                                        }
                                        else {
                                            // P1 = P1, already defined
                                            // P2 = P2, already defined
                                        }
                                    }
                                }
                                else if (startAngle < M_PI) {
                                    if (endAngle < M_PI) {
                                        P1 = ProjPointOnPlane_XYZ(pntF, sketchPlane);
                                        P2 = ProjPointOnPlane_XYZ(pntL, sketchPlane);
                                    }
                                    else if (endAngle < 2.0 * M_PI - startAngle) {
                                        P2 = ProjPointOnPlane_XYZ(pntF, sketchPlane);
                                        // P1 = P1, already defined
                                    }
                                    else if (endAngle < 2.0 * M_PI) {
                                        P2 = ProjPointOnPlane_XYZ(pntL, sketchPlane);
                                        // P1 = P1, already defined
                                    }
                                    else {
                                        // P1 = P1, already defined
                                        // P2 = P2, already defined
                                    }
                                }
                                else {
                                    if (endAngle < 2 * M_PI) {
                                        P1 = ProjPointOnPlane_XYZ(pntF, sketchPlane);
                                        P2 = ProjPointOnPlane_XYZ(pntL, sketchPlane);
                                    }
                                    else if (endAngle < 4 * M_PI - startAngle) {
                                        P1 = ProjPointOnPlane_XYZ(pntF, sketchPlane);
                                        // P2 = P2, already defined
                                    }
                                    else if (endAngle < 3 * M_PI) {
                                        // P1 = P1, already defined
                                        P2 = ProjPointOnPlane_XYZ(pntL, sketchPlane);
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
                            ExternalGeo.push_back(projectedSegment);
                        }
                        else {// general case, full circle
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
                            Handle(Geom_Ellipse) curve =
                                new Geom_Ellipse(refFrameEllipse, origCircle.Radius(), minorRadius);
                            Part::GeomEllipse* ellipse = new Part::GeomEllipse();
                            ellipse->setHandle(curve);
                            GeometryFacade::setConstruction(ellipse, true);

                            ExternalGeo.push_back(ellipse);
                        }
                    }
                }
                else if (curve.GetType() == GeomAbs_Ellipse) {

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
                    double t_min = t_max + 0.5 * M_PI;

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
                    gp_Ax2 destCurveAx2(
                        destCenter, gp_Dir(0, 0, sens > 0.0 ? 1.0 : -1.0), gp_Dir(destAxisMajor));

                    // projection is a circle
                    if ((RDest - rDest) < (double)Precision::Confusion()) {
                        Handle(Geom_Circle) curve =
                            new Geom_Circle(destCurveAx2, 0.5 * (rDest + RDest));
                        Part::GeomCircle* circle = new Part::GeomCircle();
                        circle->setHandle(curve);
                        GeometryFacade::setConstruction(circle, true);

                        ExternalGeo.push_back(circle);
                    }
                    else {
                        if (sketchPlane.Position().Direction().IsNormal(
                                elipsOrig.Position().Direction(), Precision::Angular())) {
                            gp_Vec start = gp_Vec(destCenter.XYZ()) + destAxisMajor;
                            gp_Vec end = gp_Vec(destCenter.XYZ()) - destAxisMajor;

                            Part::GeomLineSegment* projectedSegment = new Part::GeomLineSegment();
                            projectedSegment->setPoints(
                                Base::Vector3d(start.X(), start.Y(), start.Z()),
                                Base::Vector3d(end.X(), end.Y(), end.Z()));
                            GeometryFacade::setConstruction(projectedSegment, true);
                            ExternalGeo.push_back(projectedSegment);
                        }
                        else {

                            elipsDest.SetPosition(destCurveAx2);
                            elipsDest.SetMajorRadius(destAxisMajor.Magnitude());
                            elipsDest.SetMinorRadius(destAxisMinor.Magnitude());


                            Handle(Geom_Ellipse) curve = new Geom_Ellipse(elipsDest);
                            Part::GeomEllipse* ellipse = new Part::GeomEllipse();
                            ellipse->setHandle(curve);
                            GeometryFacade::setConstruction(ellipse, true);

                            ExternalGeo.push_back(ellipse);
                        }
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
                                    Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());
                                    Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());

                                    if (Base::Distance(p1, p2) < Precision::Confusion()) {
                                        Base::Vector3d p = (p1 + p2) / 2;
                                        Part::GeomPoint* point = new Part::GeomPoint(p);
                                        GeometryFacade::setConstruction(point, true);
                                        ExternalGeo.push_back(point);
                                    }
                                    else {
                                        Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                        line->setPoints(p1, p2);
                                        GeometryFacade::setConstruction(line, true);
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
                                        circle->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));

                                        GeometryFacade::setConstruction(circle, true);
                                        ExternalGeo.push_back(circle);
                                    }
                                    else {
                                        Part::GeomArcOfCircle* arc = new Part::GeomArcOfCircle();
                                        Handle(Geom_Curve) curve = new Geom_Circle(c);
                                        Handle(Geom_TrimmedCurve) tCurve =
                                            new Geom_TrimmedCurve(curve,
                                                                  projCurve.FirstParameter(),
                                                                  projCurve.LastParameter());
                                        arc->setHandle(tCurve);
                                        GeometryFacade::setConstruction(arc, true);
                                        ExternalGeo.push_back(arc);
                                    }
                                }
                                else if (projCurve.GetType() == GeomAbs_BSplineCurve) {
                                    // Unfortunately, a normal projection of a circle can also give
                                    // a Bspline Split the spline into arcs
                                    GeomConvert_BSplineCurveKnotSplitting bSplineSplitter(
                                        projCurve.BSpline(), 2);
                                    // int s = bSplineSplitter.NbSplits();
                                    if ((curve.GetType() == GeomAbs_Circle)
                                        && (bSplineSplitter.NbSplits() == 2)) {
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
                                        circle->setCenter(
                                            Base::Vector3d(center.X(), center.Y(), center.Z()));

                                        GeometryFacade::setConstruction(circle, true);
                                        ExternalGeo.push_back(circle);
                                    }
                                    else {
                                        Part::GeomBSplineCurve* bspline =
                                            new Part::GeomBSplineCurve(projCurve.BSpline());
                                        GeometryFacade::setConstruction(bspline, true);
                                        ExternalGeo.push_back(bspline);
                                    }
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
                                        Part::GeomHyperbola* hyperbola = new Part::GeomHyperbola();
                                        hyperbola->setMajorRadius(e.MajorRadius());
                                        hyperbola->setMinorRadius(e.MinorRadius());
                                        hyperbola->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));
                                        hyperbola->setAngleXU(
                                            -xdir.AngleWithRef(xdirref.XDirection(), normal));
                                        GeometryFacade::setConstruction(hyperbola, true);
                                        ExternalGeo.push_back(hyperbola);
                                    }
                                    else {
                                        Part::GeomArcOfHyperbola* aoh =
                                            new Part::GeomArcOfHyperbola();
                                        Handle(Geom_Curve) curve = new Geom_Hyperbola(e);
                                        Handle(Geom_TrimmedCurve) tCurve =
                                            new Geom_TrimmedCurve(curve,
                                                                  projCurve.FirstParameter(),
                                                                  projCurve.LastParameter());
                                        aoh->setHandle(tCurve);
                                        GeometryFacade::setConstruction(aoh, true);
                                        ExternalGeo.push_back(aoh);
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
                                        Part::GeomParabola* parabola = new Part::GeomParabola();
                                        parabola->setFocal(e.Focal());
                                        parabola->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));
                                        parabola->setAngleXU(
                                            -xdir.AngleWithRef(xdirref.XDirection(), normal));
                                        GeometryFacade::setConstruction(parabola, true);
                                        ExternalGeo.push_back(parabola);
                                    }
                                    else {
                                        Part::GeomArcOfParabola* aop =
                                            new Part::GeomArcOfParabola();
                                        Handle(Geom_Curve) curve = new Geom_Parabola(e);
                                        Handle(Geom_TrimmedCurve) tCurve =
                                            new Geom_TrimmedCurve(curve,
                                                                  projCurve.FirstParameter(),
                                                                  projCurve.LastParameter());
                                        aop->setHandle(tCurve);
                                        GeometryFacade::setConstruction(aop, true);
                                        ExternalGeo.push_back(aop);
                                    }
                                }
                                else if (projCurve.GetType() == GeomAbs_Ellipse) {
                                    gp_Elips e = projCurve.Ellipse();
                                    gp_Pnt p = e.Location();
                                    gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
                                    gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

                                    // gp_Dir normal = e.Axis().Direction();
                                    gp_Dir normal = gp_Dir(0, 0, 1);
                                    gp_Ax2 xdirref(p, normal);

                                    if (P1.SquareDistance(P2) < Precision::Confusion()) {
                                        Part::GeomEllipse* ellipse = new Part::GeomEllipse();
                                        Handle(Geom_Ellipse) curve = new Geom_Ellipse(e);
                                        ellipse->setHandle(curve);
                                        GeometryFacade::setConstruction(ellipse, true);
                                        ExternalGeo.push_back(ellipse);
                                    }
                                    else {
                                        Part::GeomArcOfEllipse* aoe = new Part::GeomArcOfEllipse();
                                        Handle(Geom_Curve) curve = new Geom_Ellipse(e);
                                        Handle(Geom_TrimmedCurve) tCurve =
                                            new Geom_TrimmedCurve(curve,
                                                                  projCurve.FirstParameter(),
                                                                  projCurve.LastParameter());
                                        aoe->setHandle(tCurve);
                                        GeometryFacade::setConstruction(aoe, true);
                                        ExternalGeo.push_back(aoe);
                                    }
                                }
                                else {
                                    throw Base::NotImplementedError(
                                        "Not yet supported geometry for external geometry");
                                }
                            }
                        }
                    }
                    catch (Standard_Failure& e) {
                        throw Base::CADKernelError(e.GetMessageString());
                    }
                }
            } break;
            case TopAbs_VERTEX: {
                gp_Pnt P = BRep_Tool::Pnt(TopoDS::Vertex(refSubShape));
                GeomAPI_ProjectPointOnSurf proj(P, gPlane);
                P = proj.NearestPoint();
                Base::Vector3d p(P.X(), P.Y(), P.Z());
                invPlm.multVec(p, p);

                Part::GeomPoint* point = new Part::GeomPoint(p);
                GeometryFacade::setConstruction(point, true);
                ExternalGeo.push_back(point);
            } break;
            default:
                throw Base::TypeError("Unknown type of geometry");
                break;
        }
    }

    rebuildVertexIndex();
}

std::vector<Part::Geometry*> SketchObject::getCompleteGeometry() const
{
    std::vector<Part::Geometry*> vals = getInternalGeometry();
    vals.insert(vals.end(), ExternalGeo.rbegin(), ExternalGeo.rend());// in reverse order
    return vals;
}

GeoListFacade SketchObject::getGeoListFacade() const
{
    std::vector<GeometryFacadeUniquePtr> facade;
    facade.reserve(Geometry.getSize() + ExternalGeo.size());

    for (auto geo : Geometry.getValues())
        facade.push_back(GeometryFacade::getFacade(geo));

    for (auto rit = ExternalGeo.rbegin(); rit != ExternalGeo.rend(); rit++)
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
        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
        }
        else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
        }
        else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
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
    for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
         ++it) {
        if ((*it)->Type == Sketcher::Coincident) {
            int firstpresentin = -1;
            int secondpresentin = -1;

            int i = 0;

            for (std::vector<std::map<int, Sketcher::PointPos>>::const_iterator iti =
                     coincidenttree.begin();
                 iti != coincidenttree.end();
                 ++iti, i++) {
                // First
                std::map<int, Sketcher::PointPos>::const_iterator filiterator;
                filiterator = (*iti).find((*it)->First);
                if (filiterator != (*iti).end()) {
                    if ((*it)->FirstPos == (*filiterator).second)
                        firstpresentin = i;
                }
                // Second
                filiterator = (*iti).find((*it)->Second);
                if (filiterator != (*iti).end()) {
                    if ((*it)->SecondPos == (*filiterator).second)
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
                tmp.insert(std::pair<int, Sketcher::PointPos>((*it)->First, (*it)->FirstPos));
                tmp.insert(std::pair<int, Sketcher::PointPos>((*it)->Second, (*it)->SecondPos));
                coincidenttree.push_back(tmp);
            }
            else if (firstpresentin != -1) {
                // add to existing group
                coincidenttree[firstpresentin].insert(
                    std::pair<int, Sketcher::PointPos>((*it)->Second, (*it)->SecondPos));
            }
            else {// secondpresentin != -1
                // add to existing group
                coincidenttree[secondpresentin].insert(
                    std::pair<int, Sketcher::PointPos>((*it)->First, (*it)->FirstPos));
            }
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

    for (std::vector<std::map<int, Sketcher::PointPos>>::const_iterator it = coincidenttree.begin();
         it != coincidenttree.end();
         ++it) {

        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

        geoId1iterator = (*it).find(GeoId);

        if (geoId1iterator != (*it).end()) {
            // If First is in this set and the first key in this ordered element key is external
            if ((*it).begin()->first < 0) {
                if ((*geoId1iterator).second == Sketcher::PointPos::start)
                    start_external = true;
                else if ((*geoId1iterator).second == Sketcher::PointPos::mid)
                    mid_external = true;
                else if ((*geoId1iterator).second == Sketcher::PointPos::end)
                    end_external = true;
            }
        }
    }
}

const std::map<int, Sketcher::PointPos> SketchObject::getAllCoincidentPoints(int GeoId,
                                                                             PointPos PosId)
{

    const std::vector<std::map<int, Sketcher::PointPos>> coincidenttree = getCoincidenceGroups();

    for (std::vector<std::map<int, Sketcher::PointPos>>::const_iterator it = coincidenttree.begin();
         it != coincidenttree.end();
         ++it) {

        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

        geoId1iterator = (*it).find(GeoId);

        if (geoId1iterator != (*it).end()) {
            // If GeoId is in this set

            if ((*geoId1iterator).second == PosId)// and posId matches
                return (*it);
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

    for (std::vector<std::map<int, Sketcher::PointPos>>::const_iterator it = coincidenttree.begin();
         it != coincidenttree.end();
         ++it) {

        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

        geoId1iterator = (*it).find(GeoId1);

        if (geoId1iterator != (*it).end()) {
            // If First is in this set
            std::map<int, Sketcher::PointPos>::const_iterator geoId2iterator;

            geoId2iterator = (*it).find(GeoId2);

            if (geoId2iterator != (*it).end()) {
                // If Second is in this set
                if ((*geoId1iterator).second == PosId1 && (*geoId2iterator).second == PosId2)
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

    for (std::vector<Constraint*>::const_iterator it = constraints.begin(); it != constraints.end();
         ++it) {
        if ((*it)->First == GeoId || (*it)->Second == GeoId || (*it)->Third == GeoId) {
            constraintList.push_back(i);
        }
        ++i;
    }
}

void SketchObject::appendConflictMsg(const std::vector<int>& conflicting, std::string& msg)
{
    appendConstraintsMsg(conflicting,
                         "Please remove the following conflicting constraint:\n",
                         "Please remove at least one of the following conflicting constraints:\n",
                         msg);
}

void SketchObject::appendRedundantMsg(const std::vector<int>& redundant, std::string& msg)
{
    appendConstraintsMsg(redundant,
                         "Please remove the following redundant constraint:",
                         "Please remove the following redundant constraints:",
                         msg);
}

void SketchObject::appendMalformedConstraintsMsg(const std::vector<int>& malformed,
                                                 std::string& msg)
{
    appendConstraintsMsg(malformed,
                         "Please remove the following malformed constraint:",
                         "Please remove the following malformed constraints:",
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

    int geoid = 0;

    for (auto geo : geos) {
        if (geo) {
            if (geo->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {

                auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                    geo->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                if (solvext->getGeometry()
                    == Sketcher::SolverGeometryExtension::NotFullyConstraint) {
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
        }

        geoid++;
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
    if (static_cast<int>(geometry.size()) != extGeoCount + intGeoCount)
        return false;
    if (geometry.size() < 2)
        return false;

    std::vector<Sketcher::Constraint*>::const_iterator it;
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
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<Part::Geometry*> geometry = getCompleteGeometry();
    const std::vector<Sketcher::Constraint*>& constraints = Constraints.getValuesForce();

    std::vector<Sketcher::Constraint*> newConstraints;
    newConstraints.reserve(constraints.size());
    std::vector<Sketcher::Constraint*>::const_iterator it;
    for (it = constraints.begin(); it != constraints.end(); ++it) {
        bool valid = evaluateConstraint(*it);
        if (valid)
            newConstraints.push_back(*it);
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

    const Part::Geometry* p1 = this->getGeometry(GeoId1);
    const Part::Geometry* p2 = this->getGeometry(GeoId2);

    if (p1 && p2) {
        int i1 = sk.addGeometry(this->getGeometry(GeoId1));
        int i2 = sk.addGeometry(this->getGeometry(GeoId2));

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
    // save the father classes
    Part::Part2DObject::Save(writer);
}

void SketchObject::Restore(XMLReader& reader)
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

        auto doc = getDocument();

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
                            Base::Console().Error(
                                "SketchObject::onChanged(): Unmanaged change of Geometry Property "
                                "results in invalid constraint indices\n");
                        }
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
                            Base::Console().Error(
                                "SketchObject::onChanged(): Unmanaged change of Constraint "
                                "Property results in invalid constraint indices\n");
                        }
                    }
                }
            }
        }
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
        restoreFinished();
        Part::Part2DObject::onDocumentRestored();
    }
    catch (...) {
    }
}

void SketchObject::restoreFinished()
{
    try {
        migrateSketch();

        validateExternalLinks();
        rebuildExternalGeometry();
        Constraints.acceptGeometry(getCompleteGeometry());
        synchroniseGeometryState();
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

                    if (g->getTypeId() == Part::GeomPoint::getClassTypeId()
                        && !gf->isInternalAligned())
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

    auto parabolafound = std::find_if(geometries.begin(), geometries.end(), [](auto g) {
        return g->getTypeId() == Part::GeomArcOfParabola::getClassTypeId();
    });

    if (parabolafound != geometries.end()) {

        auto focalaxisfound = std::find_if(constraints.begin(), constraints.end(), [](auto c) {
            return c->Type == InternalAlignment && c->AlignmentType == ParabolaFocalAxis;
        });

        // There are parabolas and there isn't an IA axis. (1) there are no axis or (2) there is a
        // legacy construction line
        if (focalaxisfound == constraints.end()) {

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
                    auto axismajorcoincidentfound =
                        std::find_if(axisgeoid2parabolageoid.begin(),
                                     axisgeoid2parabolageoid.end(),
                                     [&](const auto& pair) {
                                         auto parabolageoid = pair.second;
                                         auto axisgeoid = pair.first;
                                         return (c->First == axisgeoid && c->Second == parabolageoid
                                                 && c->SecondPos == PointPos::mid)
                                             || (c->Second == axisgeoid && c->First == parabolageoid
                                                 && c->FirstPos == PointPos::mid);
                                     });

                    if (axismajorcoincidentfound != axisgeoid2parabolageoid.end()) {
                        // we skip this coincident, the other coincident on axis will be substituted
                        // by internal geometry constraint
                        continue;
                    }

                    auto focuscoincidentfound =
                        std::find_if(axisgeoid2parabolageoid.begin(),
                                     axisgeoid2parabolageoid.end(),
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

            Base::Console().Critical(
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
            Base::Console().Log("Constraint%i will be affected\n", i + 1);
        }
    }

    this->Constraints.setValues(std::move(newVals));

    Base::Console().Log("ChangeConstraintsLocking: affected %i of %i tangent/perp constraints\n",
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

    int cntToBeAffected = 0;//==cntSuccess+cntFail
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
                if (g->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
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
            Base::Console().Log("Constraint%i will be affected\n", ic + 1);
        };
    }

    if (!justAnalyze) {
        this->Constraints.setValues(std::move(newVals));
        Base::Console().Log("Swapped start/end of reversed external arcs in %i constraints\n",
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
    try {
        // assert ( cstr->Type == Tangent  ||  cstr->Type == Perpendicular);
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
            else {
                Base::Vector3d p = getPoint(geoIdPt, posPt);

                // this piece of code is also present in Sketch.cpp, correct for offset
                // and to do the autodecision for old sketches.
                // the difference between the datum value and the actual angle to apply.
                // (datum=angle+offset)
                double angleOffset = 0.0;
                // the desired angle value (and we are to decide if 180* should be added to it)
                double angleDesire = 0.0;
                if (cstr->Type == Tangent) {
                    angleOffset = -M_PI / 2;
                    angleDesire = 0.0;
                }
                if (cstr->Type == Perpendicular) {
                    angleOffset = 0;
                    angleDesire = M_PI / 2;
                }

                double angleErr = calculateAngleViaPoint(geoId1, geoId2, p.x, p.y) - angleDesire;

                // bring angleErr to -pi..pi
                if (angleErr > M_PI)
                    angleErr -= M_PI * 2;
                if (angleErr < -M_PI)
                    angleErr += M_PI * 2;

                // the autodetector
                if (fabs(angleErr) > M_PI / 2)
                    angleDesire += M_PI;

                // external tangency. The angle stored is offset by Pi/2 so that a value of 0.0 is
                // invalid and treated as "undecided".
                cstr->setValue(angleDesire + angleOffset);
            }
        }
    }
    catch (Base::Exception& e) {
        // failure to determine tangency type is not a big deal, so a warning.
        Base::Console().Warning("Error in AutoLockTangency. %s \n", e.what());
        return false;
    }
    return true;
}

void SketchObject::setExpression(const App::ObjectIdentifier& path,
                                 std::shared_ptr<App::Expression> expr)
{
    DocumentObject::setExpression(path, expr);

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
            e.ReportException();
            FC_ERR("Failed to recompute " << ExpressionEngine.getFullName() << ": " << e.what());
        }
        solve();
    }
}

Part::TopoShape SketchObject::getEdge(const Part::Geometry *geo, const char *name) const
{
    Part::TopoShape shape(geo->toShape());
    // shape.setElementName(Data::IndexedName::fromConst("Edge", 1),
    //                      Data::MappedName::fromRawData(name));
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
                // shape.setElementName(Data::IndexedName::fromConst("Vertex", i),
                //                      Data::MappedName::fromRawData(ss.str().c_str()));
                break;
            }
        }
    }
    return shape;
}

Data::IndexedName SketchObject::checkSubName(const char *sub) const
{
    // FIXME: trivial implementation needs to be replaced with full logic
    (void)sub;
    return Data::IndexedName();
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

std::string SketchObject::convertSubName(const Data::IndexedName & indexedName, bool postfix) const
{
    std::ostringstream ss;
    int geoId;
    PointPos posId;
    if(!geoIdFromShapeType(indexedName,geoId,posId)) {
        ss << indexedName;
        return ss.str();
    }
    if(geoId == Sketcher::GeoEnum::HAxis ||
       geoId == Sketcher::GeoEnum::VAxis ||
       geoId == Sketcher::GeoEnum::RtPnt) {
        if (postfix)
            ss << Data::ELEMENT_MAP_PREFIX;
        ss << indexedName;
        if(postfix)
           ss  << '.' << indexedName;
        return ss.str();
    }

    auto geo = getGeometry(geoId);
    if(!geo) {
        std::string res = indexedName.toString();
        return res;
    }
    if (postfix)
        ss << Data::ELEMENT_MAP_PREFIX;
    ss << (geoId>=0?'g':'e') << GeometryFacade::getFacade(geo)->getId();
    if(posId!=PointPos::none)
        ss << 'v' << static_cast<int>(posId);
    if(postfix) {
        // rename Edge to edge, and Vertex to vertex to avoid ambiguous of
        // element mapping of the public shape and internal geometry.
        if (indexedName.getIndex() <= 0)
            ss << '.' << indexedName;
        else if(boost::starts_with(indexedName.getType(),"Edge"))
            ss << ".e" << (indexedName.getType()+1) << indexedName.getIndex();
        else if(boost::starts_with(indexedName.getType(),"Vertex"))
            ss << ".v" << (indexedName.getType()+1) << indexedName.getIndex();
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
    if (analyser)
        analyser->makeMissingPointOnPointCoincident(onebyone);
}

void SketchObject::makeMissingVerticalHorizontal(bool onebyone)
{
    if (analyser)
        analyser->makeMissingVerticalHorizontal(onebyone);
}

void SketchObject::makeMissingEquality(bool onebyone)
{
    if (analyser)
        analyser->makeMissingEquality(onebyone);
}

int SketchObject::autoRemoveRedundants(bool updategeo)
{
    auto redundants = getLastRedundant();

    if (redundants.empty())
        return 0;

    // getLastRedundant is base 1, while delConstraints is base 0
    for (size_t i = 0; i < redundants.size(); i++)
        redundants[i]--;

    delConstraints(redundants, updategeo);

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
        copy->Name = name;

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

int SketchObject::setGeometryId(int GeoId, long id)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId >= int(Geometry.getValues().size()))
        return -1;

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();


    std::vector<Part::Geometry*> newVals(vals);

    // deep copy
    for (size_t i = 0; i < newVals.size(); i++) {
        newVals[i] = newVals[i]->clone();

        if ((int)i == GeoId) {
            auto gf = GeometryFacade::getFacade(newVals[i]);

            gf->setId(id);
        }
    }

    // There is not actual internal transaction going on here, however neither the geometry indices
    // nor the vertices need to be updated so this is a convenient way of preventing it.
    {
        Base::StateLocker lock(internaltransaction, true);
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
