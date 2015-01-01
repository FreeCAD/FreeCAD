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
# include <TopoDS.hxx>
# include <TopExp_Explorer.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax3.hxx>
# include <gp_Circ.hxx>
# include <gp_Elips.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRep_Tool.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Circle.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <BRepOffsetAPI_NormalProjection.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <Standard_Version.hxx>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Console.h>

#include <Mod/Part/App/Geometry.h>

#include <vector>

#include "SketchObject.h"
#include "SketchObjectPy.h"
#include "Sketch.h"

using namespace Sketcher;
using namespace Base;


PROPERTY_SOURCE(Sketcher::SketchObject, Part::Part2DObject)


SketchObject::SketchObject()
{
    ADD_PROPERTY_TYPE(Geometry,        (0)  ,"Sketch",(App::PropertyType)(App::Prop_None),"Sketch geometry");
    ADD_PROPERTY_TYPE(Constraints,     (0)  ,"Sketch",(App::PropertyType)(App::Prop_None),"Sketch constraints");
    ADD_PROPERTY_TYPE(ExternalGeometry,(0,0),"Sketch",(App::PropertyType)(App::Prop_None),"Sketch external geometry");

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
}

SketchObject::~SketchObject()
{
    for (std::vector<Part::Geometry *>::iterator it=ExternalGeo.begin(); it != ExternalGeo.end(); ++it)
        if (*it) delete *it;
    ExternalGeo.clear();
}

App::DocumentObjectExecReturn *SketchObject::execute(void)
{
    try {
        this->positionBySupport();
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

    Sketch sketch;
    
    int dofs = sketch.setUpSketch(getCompleteGeometry(), Constraints.getValues(),
                                  getExternalGeometryCount());
    if (dofs < 0) { // over-constrained sketch
        std::string msg="Over-constrained sketch\n";
        appendConflictMsg(sketch.getConflicting(), msg);
        return new App::DocumentObjectExecReturn(msg.c_str(),this);
    }
    if (sketch.hasConflicts()) { // conflicting constraints
        std::string msg="Sketch with conflicting constraints\n";
        appendConflictMsg(sketch.getConflicting(), msg);
        return new App::DocumentObjectExecReturn(msg.c_str(),this);
    }
    if (sketch.hasRedundancies()) { // redundant constraints
        std::string msg="Sketch with redundant constraints\n";
        appendRedundantMsg(sketch.getRedundant(), msg);
        return new App::DocumentObjectExecReturn(msg.c_str(),this);
    }

    // solve the sketch
    if (sketch.solve() != 0)
        return new App::DocumentObjectExecReturn("Solving the sketch failed",this);

    std::vector<Part::Geometry *> geomlist = sketch.extractGeometry();
    Geometry.setValues(geomlist);
    for (std::vector<Part::Geometry *>::iterator it=geomlist.begin(); it != geomlist.end(); ++it)
        if (*it) delete *it;

    Shape.setValue(sketch.toShape());

    return App::DocumentObject::StdReturn;
}

int SketchObject::hasConflicts(void) const
{
    // set up a sketch (including dofs counting and diagnosing of conflicts)
    Sketch sketch;
    int dofs = sketch.setUpSketch(getCompleteGeometry(), Constraints.getValues(),
                                  getExternalGeometryCount());
    if (dofs < 0) // over-constrained sketch
        return -2;
    if (sketch.hasConflicts()) // conflicting constraints
        return -1;

    return 0;
}

int SketchObject::solve()
{
    // set up a sketch (including dofs counting and diagnosing of conflicts)
    Sketch sketch;
    int dofs = sketch.setUpSketch(getCompleteGeometry(), Constraints.getValues(),
                                  getExternalGeometryCount());
    int err=0;
    if (dofs < 0) // over-constrained sketch
        err = -3;
    else if (sketch.hasConflicts()) // conflicting constraints
        err = -3;
    else if (sketch.solve() != 0) // solving
        err = -2;

    if (err == 0) {
        // set the newly solved geometry
        std::vector<Part::Geometry *> geomlist = sketch.extractGeometry();
        Geometry.setValues(geomlist);
        for (std::vector<Part::Geometry *>::iterator it = geomlist.begin(); it != geomlist.end(); ++it)
            if (*it) delete *it;
    }

    return err;
}

int SketchObject::setDatum(int ConstrId, double Datum)
{
    // set the changed value for the constraint
    const std::vector<Constraint *> &vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= int(vals.size()))
        return -1;
    ConstraintType type = vals[ConstrId]->Type;
    if (type != Distance &&
        type != DistanceX &&
        type != DistanceY &&
        type != Radius &&
        type != Angle &&
        type != Tangent && //for tangent, value==0 is autodecide, value==Pi/2 is external and value==-Pi/2 is internal
        type != Perpendicular &&
        type != SnellsLaw)
        return -1;

    if ((type == Distance || type == Radius) && Datum <= 0)
        return (Datum == 0) ? -5 : -4;

    // copy the list
    std::vector<Constraint *> newVals(vals);
    // clone the changed Constraint
    Constraint *constNew = vals[ConstrId]->clone();
    constNew->Value = Datum;
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(newVals);
    delete constNew;

    int err = solve();
    if (err)
        this->Constraints.setValues(vals);

    return err;
}

int SketchObject::movePoint(int GeoId, PointPos PosId, const Base::Vector3d& toPoint, bool relative)
{
    Sketch sketch;
    int dofs = sketch.setUpSketch(getCompleteGeometry(), Constraints.getValues(),
                                  getExternalGeometryCount());
    if (dofs < 0) // over-constrained sketch
        return -1;
    if (sketch.hasConflicts()) // conflicting constraints
        return -1;

    // move the point and solve
    int ret = sketch.movePoint(GeoId, PosId, toPoint, relative);
    if (ret == 0) {
        std::vector<Part::Geometry *> geomlist = sketch.extractGeometry();
        Geometry.setValues(geomlist);
        //Constraints.acceptGeometry(getCompleteGeometry());
        for (std::vector<Part::Geometry *>::iterator it=geomlist.begin(); it != geomlist.end(); ++it) {
            if (*it) delete *it;
        }
    }

    return ret;
}

Base::Vector3d SketchObject::getPoint(int GeoId, PointPos PosId) const
{
    assert(GeoId == H_Axis || GeoId == V_Axis ||
           (GeoId <= getHighestCurveIndex() && GeoId >= -getExternalGeometryCount()) );
    const Part::Geometry *geo = getGeometry(GeoId);
    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
        const Part::GeomPoint *p = dynamic_cast<const Part::GeomPoint*>(geo);
        if (PosId == start || PosId == mid || PosId == end)
            return p->getPoint();
    } else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geo);
        if (PosId == start)
            return lineSeg->getStartPoint();
        else if (PosId == end)
            return lineSeg->getEndPoint();
    } else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle*>(geo);
        if (PosId == mid)
            return circle->getCenter();
    } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        const Part::GeomEllipse *ellipse = dynamic_cast<const Part::GeomEllipse*>(geo);
        if (PosId == mid)
            return ellipse->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle *aoc = dynamic_cast<const Part::GeomArcOfCircle*>(geo);
        if (PosId == start)
            return aoc->getStartPoint();
        else if (PosId == end)
            return aoc->getEndPoint();
        else if (PosId == mid)
            return aoc->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        const Part::GeomArcOfEllipse *aoc = dynamic_cast<const Part::GeomArcOfEllipse*>(geo);
        if (PosId == start)
            return aoc->getStartPoint();
        else if (PosId == end)
            return aoc->getEndPoint();
        else if (PosId == mid)
            return aoc->getCenter();
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
                Part::GeomLineSegment *lineSeg = dynamic_cast<Part::GeomLineSegment*>(*geo);
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

int SketchObject::addGeometry(const std::vector<Part::Geometry *> &geoList)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);
    for (std::vector<Part::Geometry *>::const_iterator it = geoList.begin(); it != geoList.end(); ++it) {
        newVals.push_back(*it);
    }
    Geometry.setValues(newVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    return Geometry.getSize()-1;
}

int SketchObject::addGeometry(const Part::Geometry *geo)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);
    Part::Geometry *geoNew = geo->clone();
    newVals.push_back(geoNew);
    Geometry.setValues(newVals);
    Constraints.acceptGeometry(getCompleteGeometry());
    delete geoNew;
    rebuildVertexIndex();
    return Geometry.getSize()-1;
}

int SketchObject::delGeometry(int GeoId)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    this->DeleteUnusedInternalGeometry(GeoId);
    
    std::vector< Part::Geometry * > newVals(vals);
    newVals.erase(newVals.begin()+GeoId);

    // Find coincident points to replace the points of the deleted geometry
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    for (PointPos PosId = start; PosId != mid; ) {
        getCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
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
    this->Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    return 0;
}

int SketchObject::toggleConstruction(int GeoId)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    std::vector< Part::Geometry * > newVals(vals);

    Part::Geometry *geoNew = newVals[GeoId]->clone();
    geoNew->Construction = !geoNew->Construction;
    newVals[GeoId]=geoNew;

    this->Geometry.setValues(newVals);
    this->Constraints.acceptGeometry(getCompleteGeometry());
    return 0;
}

int SketchObject::setConstruction(int GeoId, bool on)
{
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();
    if (GeoId < 0 || GeoId >= int(vals.size()))
        return -1;

    std::vector< Part::Geometry * > newVals(vals);

    Part::Geometry *geoNew = newVals[GeoId]->clone();
    geoNew->Construction = on;
    newVals[GeoId]=geoNew;

    this->Geometry.setValues(newVals);
    this->Constraints.acceptGeometry(getCompleteGeometry());
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
    for(int i = newVals.size()-ConstraintList.size(); i<newVals.size(); i++){
        if( newVals[i]->Type == Tangent || newVals[i]->Type == Perpendicular ){
            Constraint *constNew = newVals[i]->clone();
            AutoLockTangencyAndPerpty(constNew);
            tbd.push_back(constNew);
            newVals[i] = constNew;
        }
    }

    this->Constraints.setValues(newVals);

    //clean up - delete temporary copies of constraints that were made to affect the constraints
    for(int i=0; i<tbd.size(); i++){
        delete (tbd[i]);
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
    return 0;
}

int SketchObject::delConstraintOnPoint(int VertexId, bool onlyCoincident)
{
    int GeoId;
    PointPos PosId;
    if (VertexId == -1) { // RootPoint
        GeoId = -1;
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
            else if ((*it)->Type == Sketcher::Tangent) {
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
    for (int i=0; i < int(newVals.size()); i++) {
        if (vals[i]->First == fromGeoId && vals[i]->FirstPos == fromPosId &&
            !(vals[i]->Second == toGeoId && vals[i]->SecondPos == toPosId)) {
            Constraint *constNew = newVals[i]->clone();
            constNew->First = toGeoId;
            constNew->FirstPos = toPosId;
            newVals[i] = constNew;
        } else if (vals[i]->Second == fromGeoId && vals[i]->SecondPos == fromPosId &&
                   !(vals[i]->First == toGeoId && vals[i]->FirstPos == toPosId)) {
            Constraint *constNew = newVals[i]->clone();
            constNew->Second = toGeoId;
            constNew->SecondPos = toPosId;
            newVals[i] = constNew;
        }
    }
    this->Constraints.setValues(newVals);
    return 0;
}

int SketchObject::fillet(int GeoId, PointPos PosId, double radius, bool trim)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    // Find the other geometry Id associated with the coincident point
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    getCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);

    // only coincident points between two (non-external) edges can be filleted
    if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
        const Part::Geometry *geo1 = getGeometry(GeoIdList[0]);
        const Part::Geometry *geo2 = getGeometry(GeoIdList[1]);
        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
            geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {
            const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment*>(geo1);
            const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment*>(geo2);

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
        const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment*>(geo1);
        const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment*>(geo2);

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
            dist1.ProjToLine(arc->getStartPoint()-intersection, dir1);
            dist2.ProjToLine(arc->getStartPoint()-intersection, dir2);
            Part::Geometry *newgeo = dynamic_cast<Part::Geometry* >(arc);
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
                movePoint(GeoId1, PosId1, arc->getStartPoint());
                movePoint(GeoId2, PosId2, arc->getEndPoint());
            }
            else {
                tangent1->SecondPos = end;
                tangent2->SecondPos = start;
                movePoint(GeoId1, PosId1, arc->getEndPoint());
                movePoint(GeoId2, PosId2, arc->getStartPoint());
            }

            addConstraint(tangent1);
            addConstraint(tangent2);
            delete tangent1;
            delete tangent2;
        }
        delete arc;
        return 0;
    }
    return -1;
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

    Part::Geometry *geo = geomlist[GeoId];
    if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geo);
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

                    movePoint(GeoId, end, point1);
                    movePoint(newGeoId, start, point2);

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
                    movePoint(GeoId, start, point1);

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
                else if (x1 < x0) { // trim line end
                    delConstraintOnPoint(GeoId, end, false);
                    movePoint(GeoId, end, point1);
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
    } else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle*>(geo);
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
            geoNew->setRange(theta1, theta2);

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
                    secondPos1 = constr->FirstPos;
                } else if(secondPos2 == Sketcher::none && (constr->First == GeoId2  && constr->Second == GeoId)) {
                    constrType2 = Sketcher::Coincident;
                    secondPos2 = constr->FirstPos;
                }
            }

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

            return 0;
        }
    } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
        const Part::GeomEllipse *ellipse = dynamic_cast<const Part::GeomEllipse*>(geo);
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
            geoNew->setAngleXU(ellipse->getAngleXU());
            geoNew->setRange(theta1, theta2);

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
                    secondPos1 = constr->FirstPos;
                } else if(secondPos2 == Sketcher::none && (constr->First == GeoId2  && constr->Second == GeoId)) {
                    constrType2 = Sketcher::Coincident;
                    secondPos2 = constr->FirstPos;
                }
            }

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

            return 0;
        }
    } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle *aoc = dynamic_cast<const Part::GeomArcOfCircle*>(geo);
        Base::Vector3d center = aoc->getCenter();
        double startAngle, endAngle;
        aoc->getRange(startAngle, endAngle);
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

                    Part::GeomArcOfCircle *aoc1 = dynamic_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    Part::GeomArcOfCircle *aoc2 = dynamic_cast<Part::GeomArcOfCircle*>(geomlist[newGeoId]);
                    aoc1->setRange(startAngle, startAngle + theta1);
                    aoc2->setRange(startAngle + theta2, endAngle);

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
                    Part::GeomArcOfCircle *aoc1 = dynamic_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    aoc1->setRange(startAngle + theta1, endAngle);
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
                    Part::GeomArcOfCircle *aoc1 = dynamic_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    aoc1->setRange(startAngle, startAngle + theta1);
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
    } else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        const Part::GeomArcOfEllipse *aoe = dynamic_cast<const Part::GeomArcOfEllipse*>(geo);
        Base::Vector3d center = aoe->getCenter();
        double startAngle, endAngle;
        aoe->getRange(startAngle, endAngle);
        double dir = (startAngle < endAngle) ? 1 : -1; // this is always == 1
        double arcLength = (endAngle - startAngle)*dir;
        double theta0 = Base::fmod(
                atan2(-aoe->getMajorRadius()*((point.x-center.x)*sin(aoe->getAngleXU())-(point.y-center.y)*cos(aoe->getAngleXU())),
                            aoe->getMinorRadius()*((point.x-center.x)*cos(aoe->getAngleXU())+(point.y-center.y)*sin(aoe->getAngleXU()))
                )- startAngle, 2.f*M_PI); // x0
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double theta1 = Base::fmod(
                atan2(-aoe->getMajorRadius()*((point1.x-center.x)*sin(aoe->getAngleXU())-(point1.y-center.y)*cos(aoe->getAngleXU())),
                            aoe->getMinorRadius()*((point1.x-center.x)*cos(aoe->getAngleXU())+(point1.y-center.y)*sin(aoe->getAngleXU()))
                )- startAngle, 2.f*M_PI) * dir; // x1
            double theta2 = Base::fmod(
                atan2(-aoe->getMajorRadius()*((point2.x-center.x)*sin(aoe->getAngleXU())-(point2.y-center.y)*cos(aoe->getAngleXU())),
                            aoe->getMinorRadius()*((point2.x-center.x)*cos(aoe->getAngleXU())+(point2.y-center.y)*sin(aoe->getAngleXU()))
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

                    Part::GeomArcOfEllipse *aoe1 = dynamic_cast<Part::GeomArcOfEllipse*>(geomlist[GeoId]);
                    Part::GeomArcOfEllipse *aoe2 = dynamic_cast<Part::GeomArcOfEllipse*>(geomlist[newGeoId]);
                    aoe1->setRange(startAngle, startAngle + theta1);
                    aoe2->setRange(startAngle + theta2, endAngle);

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
                        atan2(-aoe->getMajorRadius()*((point1.x-center.x)*sin(aoe->getAngleXU())-(point1.y-center.y)*cos(aoe->getAngleXU())),
                              aoe->getMinorRadius()*((point1.x-center.x)*cos(aoe->getAngleXU())+(point1.y-center.y)*sin(aoe->getAngleXU()))
                             )- startAngle, 2.f*M_PI) * dir; // x1
                
            if (theta1 >= 0.001*arcLength && theta1 <= 0.999*arcLength) {
                if (theta1 > theta0) { // trim arc start
                    delConstraintOnPoint(GeoId, start, false);
                    Part::GeomArcOfEllipse *aoe1 = dynamic_cast<Part::GeomArcOfEllipse*>(geomlist[GeoId]);
                    aoe1->setRange(startAngle + theta1, endAngle);
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
                    Part::GeomArcOfEllipse *aoe1 = dynamic_cast<Part::GeomArcOfEllipse*>(geomlist[GeoId]);
                    aoe1->setRange(startAngle, startAngle + theta1);
 
                    return 0;
                }
            }
        }
    }

    return -1;
}

int SketchObject::ExposeInternalGeometry(int GeoId)
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
        
        int majorelementindex=-1;
        int minorelementindex=-1;
        int focus1elementindex=-1;
        int focus2elementindex=-1;
        
        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();
        
        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::EllipseMajorDiameter:
                        major=true;
                        majorelementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseMinorDiameter:
                        minor=true;
                        minorelementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseFocus1: 
                        focus1=true;
                        focus1elementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseFocus2: 
                        focus2=true;
                        focus2elementindex=(*it)->First;
                        break;
                }
            }
        }
        
        int currentgeoid= getHighestCurveIndex();
        int incrgeo= 0;
        
        Base::Vector3d center;
        double majord;
        double minord;
        double phi;
        
        if(geo->getTypeId() == Part::GeomEllipse::getClassTypeId()){
            const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo);
            
            center=ellipse->getCenter();
            majord=ellipse->getMajorRadius();
            minord=ellipse->getMinorRadius();
            phi=ellipse->getAngleXU();
        }
        else {
            const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo);
            
            center=aoe->getCenter();
            majord=aoe->getMajorRadius();
            minord=aoe->getMinorRadius();
            phi=aoe->getAngleXU();                    
        }
  
        Base::Vector3d majorpositiveend = center + majord * Base::Vector3d(cos(phi),sin(phi),0);
        Base::Vector3d majornegativeend = center - majord * Base::Vector3d(cos(phi),sin(phi),0);  
        Base::Vector3d minorpositiveend = center + minord * Base::Vector3d(-sin(phi),cos(phi),0);
        Base::Vector3d minornegativeend = center - minord * Base::Vector3d(-sin(phi),cos(phi),0);
        
        double df= sqrt(majord*majord-minord*minord);
        
        Base::Vector3d focus1P = center + df * Base::Vector3d(cos(phi),sin(phi),0);
        Base::Vector3d focus2P = center - df * Base::Vector3d(cos(phi),sin(phi),0);
        
        if(!major)
        {
            Part::GeomLineSegment *lmajor = new Part::GeomLineSegment();
            lmajor->setPoints(majorpositiveend,majornegativeend);
            
            this->addGeometry(lmajor); // create line for major axis                
            this->setConstruction(currentgeoid+incrgeo+1,true);
            delete lmajor;
            
            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = EllipseMajorDiameter;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->Second = GeoId;

            addConstraint(newConstr);
            delete newConstr;
            incrgeo++;
        }
        if(!minor)
        {       
            Part::GeomLineSegment *lminor = new Part::GeomLineSegment();
            lminor->setPoints(minorpositiveend,minornegativeend);
            
            this->addGeometry(lminor); // create line for major axis                
            this->setConstruction(currentgeoid+incrgeo+1,true);
            delete lminor;
            
            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = EllipseMinorDiameter;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->Second = GeoId;

            addConstraint(newConstr);
            delete newConstr;
            incrgeo++;
        }
        if(!focus1)
        {
            Part::GeomPoint *pf1 = new Part::GeomPoint();
            pf1->setPoint(focus1P);
            this->addGeometry(pf1); 
            delete pf1;
            
            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = EllipseFocus1;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->FirstPos = Sketcher::start;
            newConstr->Second = GeoId;

            addConstraint(newConstr);
            delete newConstr;
            incrgeo++;
        }
        if(!focus2)
        {
            Part::GeomPoint *pf2 = new Part::GeomPoint();
            pf2->setPoint(focus2P);
            this->addGeometry(pf2);                 
            delete pf2;
            
            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = Sketcher::InternalAlignment;
            newConstr->AlignmentType = EllipseFocus2;
            newConstr->First = currentgeoid+incrgeo+1;
            newConstr->FirstPos = Sketcher::start;
            newConstr->Second = GeoId;

            addConstraint(newConstr);
            delete newConstr;  
        }
        
        return incrgeo; //number of added elements
    }
    else
        return -1; // not supported type
}

int SketchObject::DeleteUnusedInternalGeometry(int GeoId)
{
   if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;
    
    const Part::Geometry *geo = getGeometry(GeoId);            
    // Only for supported types
    if(geo->getTypeId() == Part::GeomEllipse::getClassTypeId() || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
        // First we search what has to be deleted
        bool major=false;
        bool minor=false;
        bool focus1=false;
        bool focus2=false;
        
        int majorelementindex=-1;
        int minorelementindex=-1;
        int focus1elementindex=-1;
        int focus2elementindex=-1;
        
        const std::vector< Sketcher::Constraint * > &vals = Constraints.getValues();
        
        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::EllipseMajorDiameter:
                        major=true;
                        majorelementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseMinorDiameter:
                        minor=true;
                        minorelementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseFocus1: 
                        focus1=true;
                        focus1elementindex=(*it)->First;
                        break;
                    case Sketcher::EllipseFocus2: 
                        focus2=true;
                        focus2elementindex=(*it)->First;
                        break;
                }
            }
        }
        
        // Hide unused geometry here
        int majorconstraints=0; // number of constraints associated to the geoid of the major axis
        int minorconstraints=0;
        int focus1constraints=0;
        int focus2constraints=0;
        
        int decrgeo=0;
        
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
        if(focus2constraints<2)
            delgeometries.push_back(focus2elementindex);
        
        if(focus1constraints<2) 
            delgeometries.push_back(focus1elementindex);
        
        if(minorconstraints<2) 
            delgeometries.push_back(minorelementindex);
              
        if(majorconstraints<2) 
            delgeometries.push_back(majorelementindex);
        
        std::sort(delgeometries.begin(), delgeometries.end()); // indices over an erased element get automatically updated!!
        
        if(delgeometries.size()>0)
        {
            for (std::vector<int>::reverse_iterator it=delgeometries.rbegin(); it!=delgeometries.rend(); ++it) {
                delGeometry(*it);
            }
        }

        return delgeometries.size(); //number of deleted elements
    }
    else
        return -1; // not supported type
}

int SketchObject::addExternal(App::DocumentObject *Obj, const char* SubName)
{
    // so far only externals to the support of the sketch
    if (Support.getValue() != Obj)
        return -1;

    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string>     originalSubElements = SubElements;

    std::vector<std::string>::iterator it;
    it = std::find(SubElements.begin(), SubElements.end(), SubName);

    // avoid duplicates
    if (it != SubElements.end())
        return -1;

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
    int GeoId = -3 - ExtGeoId;
    for (std::vector<Constraint *>::const_iterator it = constraints.begin();
         it != constraints.end(); ++it) {
        if ((*it)->First != GeoId && (*it)->Second != GeoId) {
            Constraint *copiedConstr = (*it)->clone();
            if (copiedConstr->First < GeoId &&
                copiedConstr->First != Constraint::GeoUndef)
                copiedConstr->First += 1;
            if (copiedConstr->Second < GeoId &&
                copiedConstr->Second != Constraint::GeoUndef)
                copiedConstr->Second += 1;
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
        return -1;
    }

    Constraints.setValues(newConstraints);
    Constraints.acceptGeometry(getCompleteGeometry());
    rebuildVertexIndex();
    return 0;
}

int SketchObject::delConstraintsToExternal()
{
    const std::vector< Constraint * > &constraints = Constraints.getValues();
    std::vector< Constraint * > newConstraints(0);
    int GeoId = -3, NullId = -2000;
    for (std::vector<Constraint *>::const_iterator it = constraints.begin();
         it != constraints.end(); ++it) {
        if ((*it)->First > GeoId && ((*it)->Second > GeoId || (*it)->Second == NullId) && ((*it)->Third > GeoId || (*it)->Third == NullId)) {
            newConstraints.push_back(*it);
        }
    }

    Constraints.setValues(newConstraints);
    Constraints.acceptGeometry(getCompleteGeometry());

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

        const Part::Feature *refObj=static_cast<const Part::Feature*>(Obj);
        const Part::TopoShape& refShape=refObj->Shape.getShape();

        TopoDS_Shape refSubShape;
        try {
            refSubShape = refShape.getSubShape(SubElement.c_str());
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            throw Base::Exception(e->GetMessageString());
        }

        switch (refSubShape.ShapeType())
        {
        case TopAbs_FACE:
            {
                const TopoDS_Face& face = TopoDS::Face(refSubShape);
                BRepAdaptor_Surface surface(face);
                if (surface.GetType() == GeomAbs_Plane) {
                }
                throw Base::Exception("Faces are not yet supported for external geometry of sketches");
            }
            break;
        case TopAbs_EDGE:
            {
                const TopoDS_Edge& edge = TopoDS::Edge(refSubShape);
                BRepAdaptor_Curve curve(edge);
                if (curve.GetType() == GeomAbs_Line) {
                    gp_Pnt P1 = curve.Value(curve.FirstParameter());
                    gp_Pnt P2 = curve.Value(curve.LastParameter());

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
                        ExternalGeo.push_back(point);
                    }
                    else {
                        Part::GeomLineSegment* line = new Part::GeomLineSegment();
                        line->setPoints(p1,p2);
                        line->Construction = true;
                        ExternalGeo.push_back(line);
                    }
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
                            Handle_Geom_Curve hCircle = new Geom_Circle(circle);
                            Handle_Geom_TrimmedCurve tCurve = new Geom_TrimmedCurve(hCircle, curve.FirstParameter(),
                                                                                    curve.LastParameter());
                            gArc->setHandle(tCurve);
                            gArc->Construction = true;
                            ExternalGeo.push_back(gArc);
                        }
                    }
                    else {
                        // creates an ellipse
                        throw Base::Exception("Not yet supported geometry for external geometry");
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
                                        Handle_Geom_Curve curve = new Geom_Circle(c);
                                        Handle_Geom_TrimmedCurve tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                                projCurve.LastParameter());
                                        arc->setHandle(tCurve);
                                        arc->Construction = true;
                                        ExternalGeo.push_back(arc);
                                    }
                                }
                                else if (projCurve.GetType() == GeomAbs_Ellipse) {
                                    gp_Elips e = projCurve.Ellipse();
                                    gp_Pnt p = e.Location();
                                    gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
                                    gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());
                                    
                                    gp_Dir normal = e.Axis().Direction();
                                    gp_Dir xdir = e.XAxis().Direction();
                                    gp_Ax2 xdirref(p, normal);

                                    if (P1.SquareDistance(P2) < Precision::Confusion()) {
                                        Part::GeomEllipse* ellipse = new Part::GeomEllipse();
                                        ellipse->setMajorRadius(e.MajorRadius());
                                        ellipse->setMinorRadius(e.MinorRadius());
                                        ellipse->setCenter(Base::Vector3d(p.X(),p.Y(),p.Z()));
                                        ellipse->setAngleXU(-xdir.AngleWithRef(xdirref.XDirection(),normal));
                                        ellipse->Construction = true;
                                        ExternalGeo.push_back(ellipse);
                                    }
                                    else {
                                        Part::GeomArcOfEllipse* aoe = new Part::GeomArcOfEllipse();
                                        Handle_Geom_Curve curve = new Geom_Ellipse(e);
                                        Handle_Geom_TrimmedCurve tCurve = new Geom_TrimmedCurve(curve, projCurve.FirstParameter(),
                                                                                                projCurve.LastParameter());
                                        aoe->setHandle(tCurve);
                                        aoe->Construction = true;
                                        ExternalGeo.push_back(aoe);
                                    }
                                }                                
                                else {
                                    throw Base::Exception("Not yet supported geometry for external geometry");
                                }
                            }
                        }
                    }
                    catch (Standard_Failure) {
                        Handle_Standard_Failure e = Standard_Failure::Caught();
                        throw Base::Exception(e->GetMessageString());
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
            throw Base::Exception("Unknown type of geometry");
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
        }
    }
}

void SketchObject::getCoincidentPoints(int GeoId, PointPos PosId, std::vector<int> &GeoIdList,
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

void SketchObject::getCoincidentPoints(int VertexId, std::vector<int> &GeoIdList,
                                       std::vector<PointPos> &PosIdList)
{
    int GeoId;
    PointPos PosId;
    getGeoVertexIndex(VertexId, GeoId, PosId);
    getCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
}

bool SketchObject::arePointsCoincident(int GeoId1, PointPos PosId1,
                                       int GeoId2, PointPos PosId2)
{
    if (GeoId1 == GeoId2 && PosId1 == PosId2)
        return true;

    const std::vector<Constraint *> &constraints = this->Constraints.getValues();
    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
         it != constraints.end(); ++it) {
        if ((*it)->Type == Sketcher::Coincident)
            if (((*it)->First == GeoId1 && (*it)->FirstPos == PosId1 &&
                 (*it)->Second == GeoId2 && (*it)->SecondPos == PosId2) ||
                ((*it)->First == GeoId2 && (*it)->FirstPos == PosId2 &&
                 (*it)->Second == GeoId1 && (*it)->SecondPos == PosId1))
                return true;
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
    const std::vector<Sketcher::Constraint *>& constraints = Constraints.getValues();
    if (static_cast<int>(geometry.size()) != extGeoCount + intGeoCount)
        return false;
    if (geometry.size() < 2)
        return false;

    std::vector<Sketcher::Constraint *>::const_iterator it;
    for (it = constraints.begin(); it != constraints.end(); ++it) {
        if (!evaluateConstraint(*it))
            return false;
    }

    return true;
}

void SketchObject::validateConstraints()
{
    int intGeoCount = getHighestCurveIndex() + 1;
    int extGeoCount = getExternalGeometryCount();

    std::vector<Part::Geometry *> geometry = getCompleteGeometry();
    const std::vector<Sketcher::Constraint *>& constraints = Constraints.getValues();

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
}

//This function is necessary for precalculation of an angle when adding
// an angle constraint. It is also used here, in SketchObject, to
// lock down the type of tangency/perpendicularity.
double SketchObject::calculateAngleViaPoint(int GeoId1, int GeoId2, double px, double py)
{
    // Temporary sketch based calculation. Slow, but guaranteed consistency with constraints.
    Sketcher::Sketch sk;
    int i1 = sk.addGeometry(this->getGeometry(GeoId1));
    int i2 = sk.addGeometry(this->getGeometry(GeoId2));

    return sk.calculateAngleViaPoint(i1,i2,px,py);


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
    if (! g1.closestParameterToBasicCurve(p, u1) ) throw Base::Exception("SketchObject::calculateAngleViaPoint: closestParameter(curve1) failed!");
    if (! g2.closestParameterToBasicCurve(p, u2) ) throw Base::Exception("SketchObject::calculateAngleViaPoint: closestParameter(curve2) failed!");

    gp_Dir tan1, tan2;
    if (! g1.tangent(u1,tan1) ) throw Base::Exception("SketchObject::calculateAngleViaPoint: tangent1 failed!");
    if (! g2.tangent(u2,tan2) ) throw Base::Exception("SketchObject::calculateAngleViaPoint: tangent2 failed!");

    assert(abs(tan1.Z())<0.0001);
    assert(abs(tan2.Z())<0.0001);

    double ang = atan2(-tan2.X()*tan1.Y()+tan2.Y()*tan1.X(), tan2.X()*tan1.X() + tan2.Y()*tan1.Y());
    return ang;
*/
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
        for(int i=0; i<GeoIdList.size(); i++){
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
    Part::Part2DObject::onChanged(prop);
}

void SketchObject::onDocumentRestored()
{
    try {
        rebuildExternalGeometry();
        Constraints.acceptGeometry(getCompleteGeometry());
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
    for(int i=0;i<VertexId2GeoId.size();i++) {
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

    for(int i = 0; i<newVals.size(); i++){
        if( newVals[i]->Type == Tangent || newVals[i]->Type == Perpendicular ){
            //create a constraint copy, affect it, replace the pointer
            cntToBeAffected++;
            Constraint *constNew = newVals[i]->clone();
            bool ret = AutoLockTangencyAndPerpty(constNew, /*bForce=*/true, bLock);
            if (ret) cntSuccess++;
            tbd.push_back(constNew);
            newVals[i] = constNew;
        }
    }

    this->Constraints.setValues(newVals);

    //clean up - delete temporary copies of constraints that were made to affect the constraints
    for(int i=0; i<tbd.size(); i++){
        delete (tbd[i]);
    }

    Base::Console().Log("ChangeConstraintsLocking: affected %i of %i tangent/perp constraints\n",
                        cntSuccess, cntToBeAffected);

    return cntSuccess;
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
/// bForce - specifies whether to ignore tha already locked constraint or not.
/// bLock - specufies whether to lock the constraint or not (if bForce is
///  true, the constraint gets unlocked, otherwise nothing is done at all).
///
///Return values:
/// true - success.
/// false - fail (this indicates an error, or that a constraint locking isn't supported).
bool SketchObject::AutoLockTangencyAndPerpty(Constraint *cstr, bool bForce, bool bLock)
{
    try{
        assert ( cstr->Type == Tangent  ||  cstr->Type == Perpendicular);
        if(cstr->Value != 0.0 && ! bForce) /*tangency type already set. If not bForce - don't touch.*/
            return true;
        if(!bLock){
            cstr->Value=0.0;//reset
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
                if(abs(angleErr) > M_PI/2 )
                    angleDesire += M_PI;

                cstr->Value = angleDesire + angleOffset; //external tangency. The angle stored is offset by Pi/2 so that a value of 0.0 is invalid and threated as "undecided".
            }
        }
    } catch (Base::Exception& e){
        //failure to determine tangency type is not a big deal, so a warning.
        assert(0);//but it shouldn't happen (failure to determine tangency type)!
        Base::Console().Warning("Error in AutoLockTangency. %s \n", e.what());
        return false;
    }
    return true;
}

// Python Sketcher feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Sketcher::SketchObjectPython, Sketcher::SketchObject)
template<> const char* Sketcher::SketchObjectPython::getViewProviderName(void) const {
    return "SketcherGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class SketcherExport FeaturePythonT<Sketcher::SketchObject>;
}
