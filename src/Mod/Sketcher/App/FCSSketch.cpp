/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <BRep_Builder.hxx>
# include <Precision.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopoDS_Compound.hxx>
# include <Standard_Version.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <cmath>
# include <iostream>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/TimeInfo.h>
#include <Base/Console.h>
#include <Base/VectorPy.h>
#include <Base/StdStlTools.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/GeometryCurvePy.h>
#include <Mod/Part/App/ArcOfCirclePy.h>
#include <Mod/Part/App/ArcOfEllipsePy.h>
#include <Mod/Part/App/CirclePy.h>
#include <Mod/Part/App/EllipsePy.h>
#include <Mod/Part/App/HyperbolaPy.h>
#include <Mod/Part/App/ArcOfHyperbolaPy.h>
#include <Mod/Part/App/ParabolaPy.h>
#include <Mod/Part/App/ArcOfParabolaPy.h>
#include <Mod/Part/App/LineSegmentPy.h>
#include <Mod/Part/App/BSplineCurvePy.h>

#include "FCSSketch.h"
#include "Constraint.h"

using namespace Sketcher;
using namespace Base;
using namespace Part;
using namespace FCS;

TYPESYSTEM_SOURCE(Sketcher::FCSSketch, Sketcher::SketchSolver)


FCSSketch::FCSSketch() : parameterStore(Py::None())
{
    parameterStore = FCS::ParameterStore::make();
}


int FCSSketch::setUpSketch(const std::vector<Part::Geometry *> &GeoList,
                        const std::vector<Constraint *> &ConstraintList,
                        int extGeoCount)
{
    
    Base::TimeInfo start_time;

    //clear();

    std::vector<Part::Geometry *> intGeoList;
    std::vector<Part::Geometry *> extGeoList;
    
    std::vector<bool> blockedGeometry; // these geometries are blocked, frozen and sent as fixed parameters to the solver
    std::vector<bool> unenforceableConstraints; // these constraints are unenforceable due to a Blocked constraint

    getSolvableGeometryContraints(GeoList, ConstraintList, extGeoCount, intGeoList, extGeoList, blockedGeometry, unenforceableConstraints);
                                  
    addGeometry(intGeoList,blockedGeometry);
    
    int extStart=Geoms.size();
    addGeometry(extGeoList, true);
    
    int extEnd=Geoms.size()-1;
    for (int i=extStart; i <= extEnd; i++)
        Geoms[i].external = true;
    
    /*
    // The Geoms list might be empty after an undo/redo
    if (!Geoms.empty()) {
        addConstraints(ConstraintList,unenforceableConstraints);
    }
    GCSsys.clearByTag(-1);
    GCSsys.declareUnknowns(Parameters);
    GCSsys.declareDrivenParams(DrivenParameters);
    GCSsys.initSolution(defaultSolverRedundant);
    GCSsys.getConflicting(Conflicting);
    GCSsys.getRedundant(Redundant);
    GCSsys.getDependentParams(pconstraintplistOut);

    calculateDependentParametersElements();

    if (debugMode==GCS::Minimal || debugMode==GCS::IterationLevel) {
        Base::TimeInfo end_time;

        Base::Console().Log("Sketcher::setUpSketch()-T:%s\n",Base::TimeInfo::diffTime(start_time,end_time).c_str());
    }

    return GCSsys.dofsNumber();
    */
    return 0;
}

int FCSSketch::addGeometry(const std::vector<Part::Geometry *> &geo,
                        const std::vector<bool> &blockedGeometry)
{
    assert(geo.size() == blockedGeometry.size());

    int ret = -1;
    std::vector<Part::Geometry *>::const_iterator it;
    std::vector<bool>::const_iterator bit;

    for ( it = geo.begin(), bit = blockedGeometry.begin(); it != geo.end() && bit !=blockedGeometry.end(); ++it, ++bit)
        ret = addGeometry(*it, *bit);
    return ret;
}

int FCSSketch::addGeometry(const std::vector<Part::Geometry *> &geo, bool fixed)
{
    int ret = -1;
    for (std::vector<Part::Geometry *>::const_iterator it=geo.begin(); it != geo.end(); ++it)
        ret = addGeometry(*it, fixed);
    return ret;
}

int FCSSketch::addGeometry(const Part::Geometry *geo, bool fixed)
{
    
    if (geo->getTypeId() == GeomPoint::getClassTypeId()) { // add a point
        const GeomPoint *point = static_cast<const GeomPoint*>(geo);
        // create the definition struct for that geom
        if( point->Construction == false ) {
            return addPoint(*point, fixed);
        }
        else {
            return addPoint(*point, true);
        }
    } else if (geo->getTypeId() == GeomLineSegment::getClassTypeId()) { // add a line
        const GeomLineSegment *lineSeg = static_cast<const GeomLineSegment*>(geo);
        // create the definition struct for that geom
        return addLineSegment(*lineSeg, fixed);
    }/*else if (geo->getTypeId() == GeomCircle::getClassTypeId()) { // add a circle
        const GeomCircle *circle = static_cast<const GeomCircle*>(geo);
        // create the definition struct for that geom
        return addCircle(*circle, fixed);
    } else if (geo->getTypeId() == GeomEllipse::getClassTypeId()) { // add a ellipse
        const GeomEllipse *ellipse = static_cast<const GeomEllipse*>(geo);
        // create the definition struct for that geom
        return addEllipse(*ellipse, fixed);
    } else if (geo->getTypeId() == GeomArcOfCircle::getClassTypeId()) { // add an arc
        const GeomArcOfCircle *aoc = static_cast<const GeomArcOfCircle*>(geo);
        // create the definition struct for that geom
        return addArc(*aoc, fixed);
    } else if (geo->getTypeId() == GeomArcOfEllipse::getClassTypeId()) { // add an arc
        const GeomArcOfEllipse *aoe = static_cast<const GeomArcOfEllipse*>(geo);
        // create the definition struct for that geom
        return addArcOfEllipse(*aoe, fixed);
    } else if (geo->getTypeId() == GeomArcOfHyperbola::getClassTypeId()) { // add an arc of hyperbola
        const GeomArcOfHyperbola *aoh = static_cast<const GeomArcOfHyperbola*>(geo);
        // create the definition struct for that geom
        return addArcOfHyperbola(*aoh, fixed);
    } else if (geo->getTypeId() == GeomArcOfParabola::getClassTypeId()) { // add an arc of parabola
        const GeomArcOfParabola *aop = static_cast<const GeomArcOfParabola*>(geo);
        // create the definition struct for that geom
        return addArcOfParabola(*aop, fixed);
    } else if (geo->getTypeId() == GeomBSplineCurve::getClassTypeId()) { // add a bspline
        const GeomBSplineCurve *bsp = static_cast<const GeomBSplineCurve*>(geo);
        // create the definition struct for that geom
        return addBSpline(*bsp, fixed);
    }*/
    else {
        throw Base::TypeError("FCSSketch::addGeometry(): Unknown or unsupported type added to a sketch");
    }
}

int FCSSketch::addPoint(const Part::GeomPoint &point, bool fixed)
{
    // create our own copy
    GeomPoint *p = static_cast<GeomPoint*>(point.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = std::unique_ptr<Geometry>(static_cast<Geometry *>(p));
    def.type = GeoType::Point;

    FCS::G2D::HParaPoint hp = new FCS::G2D::ParaPoint();
    
    hp->makeParameters(parameterStore);
    
    hp->x.savedValue() = p->getPoint().x;
    hp->y.savedValue() = p->getPoint().y;
    
    if(fixed) {
        hp->x.fix();
        hp->y.fix();
    }
    
    def.startPointId = Points.size();
    def.endPointId = Points.size();
    def.midPointId = Points.size();
    
    Points.push_back(hp);

    // store complete set
    Geoms.push_back(def);

    // return the position of the newly added geometry
    return Geoms.size()-1;

}

int FCSSketch::addLineSegment(const Part::GeomLineSegment &lineSegment, bool fixed)
{
    // create our own copy
    GeomLineSegment *lineSeg = static_cast<GeomLineSegment*>(lineSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = std::unique_ptr<Geometry>(static_cast<Geometry *>(lineSeg));
    def.type = GeoType::Line;

    // get the points from the line
    Base::Vector3d start = lineSeg->getStartPoint();
    Base::Vector3d end   = lineSeg->getEndPoint();

    FCS::G2D::HParaLine hl = new FCS::G2D::ParaLine();
    
    hl->makeParameters(parameterStore);
    
    hl->p0->x.savedValue() = start.x;
    hl->p0->y.savedValue() = start.y;
    hl->p1->x.savedValue() = end.x;
    hl->p1->y.savedValue() = end.y;
    
    if(fixed) {
       hl->p0->x.fix();
       hl->p0->y.fix();
       hl->p1->x.fix();
       hl->p1->y.fix();
    }

    // add the points
    def.startPointId = Points.size();
    def.endPointId = Points.size()+1;
    Points.push_back(hl->p0);
    Points.push_back(hl->p1);

    // set the line for later constraints
    LineSegments.push_back(hl);

    // store complete set
    Geoms.push_back(std::move(def));

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

std::vector<Part::Geometry *> FCSSketch::extractGeometry(bool withConstructionElements,
                                                      bool withExternalElements) const
{
    std::vector<Part::Geometry *> temp;
    temp.reserve(Geoms.size());
    
    for (std::vector<GeoDef>::const_iterator it=Geoms.begin(); it != Geoms.end(); ++it) {
        if ((!it->external || withExternalElements) && (!it->geo->Construction || withConstructionElements))
            temp.push_back(it->geo->clone());
    }

    return temp;
}


Base::Vector3d FCSSketch::calculateNormalAtPoint(int geoIdCurve, double px, double py)
{
    /*
    geoIdCurve = checkGeoId(geoIdCurve);

    GCS::Point p;
    p.x = &px;
    p.y = &py;

    //check pointers
    GCS::Curve* crv = getGCSCurveByGeoId(geoIdCurve);
    if (!crv) {
        throw Base::ValueError("calculateNormalAtPoint: getGCSCurveByGeoId returned NULL!\n");
    }

    double tx = 0.0, ty = 0.0;
    GCSsys.calculateNormalAtPoint(*crv, p, tx, ty);
    return Base::Vector3d(tx,ty,0.0);
    */
    return Base::Vector3d();
}

int FCSSketch::solve(void)
{
    /*
    Base::TimeInfo start_time;
    if (!isInitMove) { // make sure we are in single subsystem mode
        GCSsys.clearByTag(-1);
        isFine = true;
    }

    int ret = -1;
    bool valid_solution;
    std::string solvername;
    int defaultsoltype = -1;

    if(isInitMove){
        solvername = "DogLeg"; // DogLeg is used for dragging (same as before)
        ret = GCSsys.solve(isFine, GCS::DogLeg);
    }
    else{
        switch (defaultSolver) {
            case 0:
                solvername = "BFGS";
                ret = GCSsys.solve(isFine, GCS::BFGS);
                defaultsoltype=2;
                break;
            case 1: // solving with the LevenbergMarquardt solver
                solvername = "LevenbergMarquardt";
                ret = GCSsys.solve(isFine, GCS::LevenbergMarquardt);
                defaultsoltype=1;
                break;
            case 2: // solving with the BFGS solver
                solvername = "DogLeg";
                ret = GCSsys.solve(isFine, GCS::DogLeg);
                defaultsoltype=0;
                break;
        }
    }

    // if successfully solved try to write the parameters back
    if (ret == GCS::Success) {
        GCSsys.applySolution();
        valid_solution = updateGeometry();
        if (!valid_solution) {
            GCSsys.undoSolution();
            updateGeometry();
            Base::Console().Warning("Invalid solution from %s solver.\n", solvername.c_str());
        }
        else {
            updateNonDrivingConstraints();
        }
    }
    else {
        valid_solution = false;
        if(debugMode==GCS::Minimal || debugMode==GCS::IterationLevel){

            Base::Console().Log("Sketcher::Solve()-%s- Failed!! Falling back...\n",solvername.c_str());
        }
    }

    if(!valid_solution && !isInitMove) { // Fall back to other solvers
        for (int soltype=0; soltype < 4; soltype++) {

            if(soltype==defaultsoltype){
                    continue; // skip default solver
            }

            switch (soltype) {
            case 0:
                solvername = "DogLeg";
                ret = GCSsys.solve(isFine, GCS::DogLeg);
                break;
            case 1: // solving with the LevenbergMarquardt solver
                solvername = "LevenbergMarquardt";
                ret = GCSsys.solve(isFine, GCS::LevenbergMarquardt);
                break;
            case 2: // solving with the BFGS solver
                solvername = "BFGS";
                ret = GCSsys.solve(isFine, GCS::BFGS);
                break;
            case 3: // last resort: augment the system with a second subsystem and use the SQP solver
                solvername = "SQP(augmented system)";
                InitParameters.resize(Parameters.size());
                int i=0;
                for (std::vector<double*>::iterator it = Parameters.begin(); it != Parameters.end(); ++it, i++) {
                    InitParameters[i] = **it;
                    GCSsys.addConstraintEqual(*it, &InitParameters[i], -1);
                }
                GCSsys.initSolution();
                ret = GCSsys.solve(isFine);
                break;
            }

            // if successfully solved try to write the parameters back
            if (ret == GCS::Success) {
                GCSsys.applySolution();
                valid_solution = updateGeometry();
                if (!valid_solution) {
                    GCSsys.undoSolution();
                    updateGeometry();
                    Base::Console().Warning("Invalid solution from %s solver.\n", solvername.c_str());
                    ret = GCS::SuccessfulSolutionInvalid;
                }else
                {
                    updateNonDrivingConstraints();
                }
            } else {
                valid_solution = false;
                if(debugMode==GCS::Minimal || debugMode==GCS::IterationLevel){

                    Base::Console().Log("Sketcher::Solve()-%s- Failed!! Falling back...\n",solvername.c_str());
                }
            }

            if (soltype == 3) // cleanup temporary constraints of the augmented system
                GCSsys.clearByTag(-1);

            if (valid_solution) {
                if (soltype == 1)
                    Base::Console().Log("Important: the LevenbergMarquardt solver succeeded where the DogLeg solver had failed.\n");
                else if (soltype == 2)
                    Base::Console().Log("Important: the BFGS solver succeeded where the DogLeg and LevenbergMarquardt solvers have failed.\n");
                else if (soltype == 3)
                    Base::Console().Log("Important: the SQP solver succeeded where all single subsystem solvers have failed.\n");

                if (soltype > 0) {
                    Base::Console().Log("If you see this message please report a way of reproducing this result at\n");
                    Base::Console().Log("http://www.freecadweb.org/tracker/main_page.php\n");
                }

                break;
            }
        } // soltype
    }

    Base::TimeInfo end_time;

    if(debugMode==GCS::Minimal || debugMode==GCS::IterationLevel){

        Base::Console().Log("Sketcher::Solve()-%s-T:%s\n",solvername.c_str(),Base::TimeInfo::diffTime(start_time,end_time).c_str());
    }

    SolveTime = Base::TimeInfo::diffTimeF(start_time,end_time);
    return ret;
    */
    return 0;
}

int FCSSketch::initMove(int geoId, PointPos pos, bool fine)
{
    /*
    isFine = fine;

    geoId = checkGeoId(geoId);

    GCSsys.clearByTag(-1);

    // don't try to move sketches that contain conflicting constraints
    if (hasConflicts()) {
        isInitMove = false;
        return -1;
    }

    if (Geoms[geoId].type == Point) {
        if (pos == start) {
            GCS::Point &point = Points[Geoms[geoId].startPointId];
            GCS::Point p0;
            MoveParameters.resize(2); // px,py
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *point.x;
            *p0.y = *point.y;
            GCSsys.addConstraintP2PCoincident(p0,point,-1);
        }
    } else if (Geoms[geoId].type == Line) {
        if (pos == start || pos == end) {
            MoveParameters.resize(2); // x,y
            GCS::Point p0;
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            if (pos == start) {
                GCS::Point &p = Points[Geoms[geoId].startPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            } else if (pos == end) {
                GCS::Point &p = Points[Geoms[geoId].endPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }
        } else if (pos == none || pos == mid) {
            MoveParameters.resize(4); // x1,y1,x2,y2
            GCS::Point p1, p2;
            p1.x = &MoveParameters[0];
            p1.y = &MoveParameters[1];
            p2.x = &MoveParameters[2];
            p2.y = &MoveParameters[3];
            GCS::Line &l = Lines[Geoms[geoId].index];
            *p1.x = *l.p1.x;
            *p1.y = *l.p1.y;
            *p2.x = *l.p2.x;
            *p2.y = *l.p2.y;
            GCSsys.addConstraintP2PCoincident(p1,l.p1,-1);
            GCSsys.addConstraintP2PCoincident(p2,l.p2,-1);
        }
    } else if (Geoms[geoId].type == Circle) {
        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        } else if (pos == none) {
            MoveParameters.resize(4); // x,y,cx,cy
            GCS::Circle &c = Circles[Geoms[geoId].index];
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y + *c.rad;
            GCSsys.addConstraintPointOnCircle(p0,c,-1);
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;
            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    } else if (Geoms[geoId].type == Ellipse) {

        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid || pos == none) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        }
    } else if (Geoms[geoId].type == ArcOfEllipse) {

        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid || pos == none) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0,center,-1);

        } else if (pos == start || pos == end) {

            MoveParameters.resize(4); // x,y,cx,cy
            if (pos == start || pos == end) {
                GCS::Point &p = (pos == start) ? Points[Geoms[geoId].startPointId]
                                            : Points[Geoms[geoId].endPointId];;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }

            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    } else if (Geoms[geoId].type == ArcOfHyperbola) {

        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid || pos == none) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        } else if (pos == start || pos == end) {

            MoveParameters.resize(4); // x,y,cx,cy
            if (pos == start || pos == end) {
                GCS::Point &p = (pos == start) ? Points[Geoms[geoId].startPointId]
                                            : Points[Geoms[geoId].endPointId];;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);

        }
    } else if (Geoms[geoId].type == ArcOfParabola) {

        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid || pos == none) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        } else if (pos == start || pos == end) {

            MoveParameters.resize(4); // x,y,cx,cy
            if (pos == start || pos == end) {
                GCS::Point &p = (pos == start) ? Points[Geoms[geoId].startPointId]
                                            : Points[Geoms[geoId].endPointId];;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);

        }
    } else if (Geoms[geoId].type == BSpline) {
        if (pos == start || pos == end) {
            MoveParameters.resize(2); // x,y
            GCS::Point p0;
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            if (pos == start) {
                GCS::Point &p = Points[Geoms[geoId].startPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            } else if (pos == end) {
                GCS::Point &p = Points[Geoms[geoId].endPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }
        } else if (pos == none || pos == mid) {
            GCS::BSpline &bsp = BSplines[Geoms[geoId].index];
            MoveParameters.resize(bsp.poles.size()*2); // x0,y0,x1,y1,....xp,yp

            int mvindex = 0;
            for(std::vector<GCS::Point>::iterator it = bsp.poles.begin(); it != bsp.poles.end() ; it++, mvindex++) {
                GCS::Point p1;
                p1.x = &MoveParameters[mvindex];
                mvindex++;
                p1.y = &MoveParameters[mvindex];

                *p1.x = *(*it).x;
                *p1.y = *(*it).y;

                GCSsys.addConstraintP2PCoincident(p1,(*it),-1);
            }

        }
    } else if (Geoms[geoId].type == Arc) {
        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        } else if (pos == start || pos == end || pos == none) {
            MoveParameters.resize(4); // x,y,cx,cy
            if (pos == start || pos == end) {
                GCS::Point &p = (pos == start) ? Points[Geoms[geoId].startPointId]
                                               : Points[Geoms[geoId].endPointId];;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            } else if (pos == none) {
                GCS::Arc &a = Arcs[Geoms[geoId].index];
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *center.x;
                *p0.y = *center.y + *a.rad;
                GCSsys.addConstraintPointOnArc(p0,a,-1);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;
            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    }
    InitParameters = MoveParameters;

    GCSsys.initSolution();
    isInitMove = true;
    */
    return 0;
}

void FCSSketch::resetInitMove()
{
    //isInitMove = false;
}

int FCSSketch::movePoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative)
{
    /*
    geoId = checkGeoId(geoId);

    // don't try to move sketches that contain conflicting constraints
    if (hasConflicts())
        return -1;

    if (!isInitMove) {
        initMove(geoId, pos);
        initToPoint = toPoint;
        moveStep = 0;
    }
    else {
        if(!relative && RecalculateInitialSolutionWhileMovingPoint) {
            if (moveStep == 0) {
                moveStep = (toPoint-initToPoint).Length();
            }
            else {
                if( (toPoint-initToPoint).Length() > 20*moveStep) { // I am getting too far away from the original solution so reinit the solution
                    initMove(geoId, pos);
                    initToPoint = toPoint;
                }
            }
        }
    }

    if (relative) {
        for (int i=0; i < int(MoveParameters.size()-1); i+=2) {
            MoveParameters[i] = InitParameters[i] + toPoint.x;
            MoveParameters[i+1] = InitParameters[i+1] + toPoint.y;
        }
    } else if (Geoms[geoId].type == Point) {
        if (pos == start) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == Line) {
        if (pos == start || pos == end) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        } else if (pos == none || pos == mid) {
            double dx = (InitParameters[2]-InitParameters[0])/2;
            double dy = (InitParameters[3]-InitParameters[1])/2;
            MoveParameters[0] = toPoint.x - dx;
            MoveParameters[1] = toPoint.y - dy;
            MoveParameters[2] = toPoint.x + dx;
            MoveParameters[3] = toPoint.y + dy;
        }
    } else if (Geoms[geoId].type == Circle) {
        if (pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == Arc) {
        if (pos == start || pos == end || pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == Ellipse) {
        if (pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == ArcOfEllipse) {
        if (pos == start || pos == end || pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == ArcOfHyperbola) {
        if (pos == start || pos == end || pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == ArcOfParabola) {
        if (pos == start || pos == end || pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == BSpline) {
        if (pos == start || pos == end) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        } else if (pos == none || pos == mid) {
            GCS::BSpline &bsp = BSplines[Geoms[geoId].index];

            double cx = 0, cy = 0; // geometric center
            for (int i=0; i < int(InitParameters.size()-1); i+=2) {
                cx += InitParameters[i];
                cy += InitParameters[i+1];
            }

            cx /= bsp.poles.size();
            cy /= bsp.poles.size();

            for (int i=0; i < int(MoveParameters.size()-1); i+=2) {

                MoveParameters[i]       = toPoint.x + InitParameters[i] - cx;
                MoveParameters[i+1]     = toPoint.y + InitParameters[i+1] - cy;
            }

        }
    }

    return solve();
    */
    return 0;
}

int FCSSketch::getPointId(int geoId, PointPos pos) const
{

    // do a range check first
    if (geoId < 0 || geoId >= (int)Geoms.size())
        return -1;
    switch (pos) {
    case start:
        return Geoms[geoId].startPointId;
    case end:
        return Geoms[geoId].endPointId;
    case mid:
        return Geoms[geoId].midPointId;
    case none:
        break;
    }
    return -1;
}

int FCSSketch::checkGeoId(int geoId) const
{
    if (geoId < 0)
        geoId += Geoms.size();//convert negative external-geometry index to index into Geoms
    if(!(   geoId >= 0   &&   geoId < int(Geoms.size())   ))
        throw Base::IndexError("FCSSketch::checkGeoId. GeoId index out range.");
    return geoId;
}

Base::Vector3d FCSSketch::getPoint(int geoId, PointPos pos) const
{
    geoId = checkGeoId(geoId);
    int pointId = getPointId(geoId, pos);
    if (pointId != -1)
        return Base::Vector3d(Points[pointId]->x.savedValue(), Points[pointId]->y.savedValue(), 0);

    return Base::Vector3d();
}

TopoShape FCSSketch::toShape(void) const
{
    
    TopoShape result;
    std::vector<GeoDef>::const_iterator it=Geoms.begin();

#if 0

    bool first = true;
    for (;it!=Geoms.end();++it) {
        if (!it->geo->Construction) {
            TopoDS_Shape sh = it->geo->toShape();
            if (first) {
                first = false;
                result.setShape(sh);
            } else {
                result.setShape(result.fuse(sh));
            }
        }
    }
    return result;
#else
    std::list<TopoDS_Edge> edge_list;
    std::list<TopoDS_Wire> wires;

    // collecting all (non constructive and non external) edges out of the sketch
    for (;it!=Geoms.end();++it) {
        if (!it->external && !it->geo->Construction && (it->type != GeoType::Point)) {
            edge_list.push_back(TopoDS::Edge(it->geo->toShape()));
        }
    }

    // FIXME: Use ShapeAnalysis_FreeBounds::ConnectEdgesToWires() as an alternative
    //
    // sort them together to wires
    while (edge_list.size() > 0) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        mkWire.Add(edge_list.front());
        edge_list.pop_front();

        TopoDS_Wire new_wire = mkWire.Wire(); // current new wire

        // try to connect each edge to the wire, the wire is complete if no more edges are connectible
        bool found = false;
        do {
            found = false;
            for (std::list<TopoDS_Edge>::iterator pE = edge_list.begin(); pE != edge_list.end(); ++pE) {
                mkWire.Add(*pE);
                if (mkWire.Error() != BRepBuilderAPI_DisconnectedWire) {
                    // edge added ==> remove it from list
                    found = true;
                    edge_list.erase(pE);
                    new_wire = mkWire.Wire();
                    break;
                }
            }
        }
        while (found);

        // Fix any topological issues of the wire
        ShapeFix_Wire aFix;
        aFix.SetPrecision(Precision::Confusion());
        aFix.Load(new_wire);
        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        wires.push_back(aFix.Wire());
    }

    if (wires.size() == 1)
        result = *wires.begin();
    else if (wires.size() > 1) {
        // FIXME: The right way here would be to determine the outer and inner wires and
        // generate a face with holes (inner wires have to be tagged REVERSE or INNER).
        // that's the only way to transport a somewhat more complex sketch...
        //result = *wires.begin();

        // I think a compound can be used as container because it is just a collection of
        // shapes and doesn't need too much information about the topology.
        // The actual knowledge how to create a prism from several wires should go to the Pad
        // feature (Werner).
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        for (std::list<TopoDS_Wire>::iterator wt = wires.begin(); wt != wires.end(); ++wt)
            builder.Add(comp, *wt);
        result.setShape(comp);
    }
#endif

    return result;
}

int FCSSketch::getGeometrySize(void) const
{
    return 0;
}

float FCSSketch::getSolveTime()
{
    return 0.0f;
}

void FCSSketch::setRecalculateInitialSolutionWhileMovingPoint(bool on)
{
    
}

// Persistence implementer -------------------------------------------------

unsigned int FCSSketch::getMemSize(void) const
{
    return 0;
}

void FCSSketch::Save(Writer &) const
{

}

void FCSSketch::Restore(XMLReader &)
{

}

