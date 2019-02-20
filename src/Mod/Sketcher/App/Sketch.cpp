/***************************************************************************
 *   Copyright (c) Juergen Riegel         (juergen.riegel@web.de) 2010     *
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
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/TimeInfo.h>
#include <Base/Console.h>
#include <Base/VectorPy.h>

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

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>

#include "Sketch.h"
#include "Constraint.h"
#include <cmath>

#include <iostream>

using namespace Sketcher;
using namespace Base;
using namespace Part;

TYPESYSTEM_SOURCE(Sketcher::Sketch, Base::Persistence)

Sketch::Sketch()
  : SolveTime(0)
  , RecalculateInitialSolutionWhileMovingPoint(false)
  , GCSsys(), ConstraintsCounter(0)
  , isInitMove(false), isFine(true), moveStep(0)
  , defaultSolver(GCS::DogLeg)
  , defaultSolverRedundant(GCS::DogLeg)
  , debugMode(GCS::Minimal)
{
}

Sketch::~Sketch()
{
    clear();
}

void Sketch::clear(void)
{
    // clear all internal data sets
    Points.clear();
    Lines.clear();
    Arcs.clear();
    Circles.clear();
    Ellipses.clear();
    ArcsOfEllipse.clear();
    ArcsOfHyperbola.clear();
    ArcsOfParabola.clear();
    BSplines.clear();

    // deleting the doubles allocated with new
    for (std::vector<double*>::iterator it = Parameters.begin(); it != Parameters.end(); ++it)
        if (*it) delete *it;
    Parameters.clear();
    DrivenParameters.clear();
    for (std::vector<double*>::iterator it = FixParameters.begin(); it != FixParameters.end(); ++it)
        if (*it) delete *it;
    FixParameters.clear();

    // deleting the geometry copied into this sketch
    for (std::vector<GeoDef>::iterator it = Geoms.begin(); it != Geoms.end(); ++it)
        if (it->geo) delete it->geo;
    Geoms.clear();

    // deleting the non-Driving constraints copied into this sketch
    //for (std::vector<Constraint *>::iterator it = NonDrivingConstraints.begin(); it != NonDrivingConstraints.end(); ++it)
    //    if (*it) delete *it;
    Constrs.clear();

    GCSsys.clear();
    isInitMove = false;
    ConstraintsCounter = 0;
    Conflicting.clear();
}

int Sketch::setUpSketch(const std::vector<Part::Geometry *> &GeoList,
                        const std::vector<Constraint *> &ConstraintList,
                        int extGeoCount)
{
    Base::TimeInfo start_time;

    clear();

    std::vector<Part::Geometry *> intGeoList, extGeoList;
    for (int i=0; i < int(GeoList.size())-extGeoCount; i++)
        intGeoList.push_back(GeoList[i]);
    for (int i=int(GeoList.size())-extGeoCount; i < int(GeoList.size()); i++)
        extGeoList.push_back(GeoList[i]);

    std::vector<bool> blockedGeometry(intGeoList.size(),false); // these geometries are blocked, frozen and sent as fixed parameters to the solver
    std::vector<bool> unenforceableConstraints(ConstraintList.size(),false); // these constraints are unenforceable due to a Blocked constraint

    if(!intGeoList.empty())
        getBlockedGeometry(blockedGeometry, unenforceableConstraints, ConstraintList);

    addGeometry(intGeoList,blockedGeometry);
    int extStart=Geoms.size();
    addGeometry(extGeoList, true);
    int extEnd=Geoms.size()-1;
    for (int i=extStart; i <= extEnd; i++)
        Geoms[i].external = true;

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
}

void Sketch::calculateDependentParametersElements(void)
{
    for(auto geo : Geoms) {
        std::vector<double *> ownparams;
        GCS::Curve * pCurve = nullptr;

        switch(geo.type) {
            case Point:
            {
                GCS::Point & point = Points[geo.startPointId];
                for(auto param : pconstraintplistOut) {
                    if (param == point.x || param == point.y) {
                        point.hasDependentParameters = true;
                        break;
                    }
                }
            }
            break;
            case Line:
            {
                GCS::Line & line = Lines[geo.index];
                line.PushOwnParams(ownparams);
                pCurve = &line;

            }
            break;
            case Arc:
            {
                GCS::Arc & arc = Arcs[geo.index];
                arc.PushOwnParams(ownparams);
                pCurve = &arc;
            }
            break;
            case Circle:
            {
                GCS::Circle & c = Circles[geo.index];
                c.PushOwnParams(ownparams);
                pCurve = &c;
            }
            break;
            case Ellipse:
            {
                GCS::Ellipse & e = Ellipses[geo.index];
                e.PushOwnParams(ownparams);
                pCurve = &e;
            }
            break;
            case ArcOfEllipse:
            {
                GCS::ArcOfEllipse & aoe = ArcsOfEllipse[geo.index];
                aoe.PushOwnParams(ownparams);
                pCurve = &aoe;
            }
            break;
            case ArcOfHyperbola:
            {
                GCS::ArcOfHyperbola & aoh = ArcsOfHyperbola[geo.index];
                aoh.PushOwnParams(ownparams);
                pCurve = &aoh;
            }
            break;
            case ArcOfParabola:
            {
                GCS::ArcOfParabola & aop = ArcsOfParabola[geo.index];
                aop.PushOwnParams(ownparams);
                pCurve = &aop;
            }
            break;
            case BSpline:
            {
                GCS::BSpline & bsp = BSplines[geo.index];
                bsp.PushOwnParams(ownparams);
                pCurve = &bsp;
            }
            break;
            case None:
            break;
        }
        // Points (this is single point elements, not vertices of other elements) are not derived from Curve
        if(geo.type != Point && geo.type != None) {
            for(auto param : pconstraintplistOut) {
                for(auto ownparam : ownparams) {
                    if (param == ownparam) {
                        pCurve->hasDependentParameters = true;
                        break;
                    }
                }
            }
        }
    }
    // Points are the only element that defines other elements, so these points (as opposed to those points
    // above which are just GeomPoints), have to be handled separately.
    for(auto & point : Points) {
        for(auto param : pconstraintplistOut) {
            if (param == point.x || param == point.y) {
                point.hasDependentParameters = true;
                break;
            }
        }
    }
}

int Sketch::resetSolver()
{
    GCSsys.clearByTag(-1);
    GCSsys.declareUnknowns(Parameters);
    GCSsys.declareDrivenParams(DrivenParameters);
    GCSsys.initSolution(defaultSolverRedundant);
    GCSsys.getConflicting(Conflicting);
    GCSsys.getRedundant(Redundant);
    GCSsys.getDependentParams(pconstraintplistOut);

    calculateDependentParametersElements();

    return GCSsys.dofsNumber();
}

const char* nameByType(Sketch::GeoType type)
{
    switch (type) {
    case Sketch::Point:
        return "point";
    case Sketch::Line:
        return "line";
    case Sketch::Arc:
        return "arc";
    case Sketch::Circle:
        return "circle";
    case Sketch::Ellipse:
        return "ellipse";
    case Sketch::ArcOfEllipse:
        return "arcofellipse";
    case Sketch::ArcOfHyperbola:
        return "arcofhyperbola";
    case Sketch::ArcOfParabola:
        return "arcofparabola";
    case Sketch::BSpline:
        return "bspline";
    case Sketch::None:
    default:
        return "unknown";
    }
}

// Geometry adding ==========================================================

int Sketch::addGeometry(const Part::Geometry *geo, bool fixed)
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
    } else if (geo->getTypeId() == GeomCircle::getClassTypeId()) { // add a circle
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
    }
    else {
        throw Base::TypeError("Sketch::addGeometry(): Unknown or unsupported type added to a sketch");
    }
}

int Sketch::addGeometry(const std::vector<Part::Geometry *> &geo, bool fixed)
{
    int ret = -1;
    for (std::vector<Part::Geometry *>::const_iterator it=geo.begin(); it != geo.end(); ++it)
        ret = addGeometry(*it, fixed);
    return ret;
}

int Sketch::addGeometry(const std::vector<Part::Geometry *> &geo,
                        const std::vector<bool> &blockedGeometry)
{
    assert(geo.size() == blockedGeometry.size());

    int ret = -1;
    std::vector<Part::Geometry *>::const_iterator it;
    std::vector<bool>::const_iterator bit;

    for (it=geo.begin(),bit=blockedGeometry.begin(); it != geo.end() && bit !=blockedGeometry.end(); ++it,++bit)
        ret = addGeometry(*it, *bit);
    return ret;
}

int Sketch::addPoint(const Part::GeomPoint &point, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomPoint *p = static_cast<GeomPoint*>(point.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = p;
    def.type = Point;

    // set the parameter for the solver
    params.push_back(new double(p->getPoint().x));
    params.push_back(new double(p->getPoint().y));

    // set the points for later constraints
    GCS::Point p1;
    p1.x = params[params.size()-2];
    p1.y = params[params.size()-1];
    def.startPointId = Points.size();
    def.endPointId = Points.size();
    def.midPointId = Points.size();
    Points.push_back(p1);

    // store complete set
    Geoms.push_back(def);

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addLine(const Part::GeomLineSegment & /*line*/, bool /*fixed*/)
{
    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addLineSegment(const Part::GeomLineSegment &lineSegment, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomLineSegment *lineSeg = static_cast<GeomLineSegment*>(lineSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = lineSeg;
    def.type = Line;

    // get the points from the line
    Base::Vector3d start = lineSeg->getStartPoint();
    Base::Vector3d end   = lineSeg->getEndPoint();

    // the points for later constraints
    GCS::Point p1, p2;

    params.push_back(new double(start.x));
    params.push_back(new double(start.y));
    p1.x = params[params.size()-2];
    p1.y = params[params.size()-1];

    params.push_back(new double(end.x));
    params.push_back(new double(end.y));
    p2.x = params[params.size()-2];
    p2.y = params[params.size()-1];

    // add the points
    def.startPointId = Points.size();
    def.endPointId = Points.size()+1;
    Points.push_back(p1);
    Points.push_back(p2);

    // set the line for later constraints
    GCS::Line l;
    l.p1 = p1;
    l.p2 = p2;
    def.index = Lines.size();
    Lines.push_back(l);

    // store complete set
    Geoms.push_back(def);

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addArc(const Part::GeomArcOfCircle &circleSegment, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomArcOfCircle *aoc = static_cast<GeomArcOfCircle*>(circleSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = aoc;
    def.type = Arc;

    Base::Vector3d center   = aoc->getCenter();
    Base::Vector3d startPnt = aoc->getStartPoint(/*emulateCCW=*/true);
    Base::Vector3d endPnt   = aoc->getEndPoint(/*emulateCCW=*/true);
    double radius           = aoc->getRadius();
    double startAngle, endAngle;
    aoc->getRange(startAngle, endAngle, /*emulateCCW=*/true);

    GCS::Point p1, p2, p3;

    params.push_back(new double(startPnt.x));
    params.push_back(new double(startPnt.y));
    p1.x = params[params.size()-2];
    p1.y = params[params.size()-1];

    params.push_back(new double(endPnt.x));
    params.push_back(new double(endPnt.y));
    p2.x = params[params.size()-2];
    p2.y = params[params.size()-1];

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    p3.x = params[params.size()-2];
    p3.y = params[params.size()-1];

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);
    def.midPointId = Points.size();
    Points.push_back(p3);

    params.push_back(new double(radius));
    double *r = params[params.size()-1];
    params.push_back(new double(startAngle));
    double *a1 = params[params.size()-1];
    params.push_back(new double(endAngle));
    double *a2 = params[params.size()-1];

    // set the arc for later constraints
    GCS::Arc a;
    a.start      = p1;
    a.end        = p2;
    a.center     = p3;
    a.rad        = r;
    a.startAngle = a1;
    a.endAngle   = a2;
    def.index = Arcs.size();
    Arcs.push_back(a);

    // store complete set
    Geoms.push_back(def);

    // arcs require an ArcRules constraint for the end points
    if (!fixed)
        GCSsys.addConstraintArcRules(a);

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addArcOfEllipse(const Part::GeomArcOfEllipse &ellipseSegment, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomArcOfEllipse *aoe = static_cast<GeomArcOfEllipse*>(ellipseSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = aoe;
    def.type = ArcOfEllipse;

    Base::Vector3d center   = aoe->getCenter();
    Base::Vector3d startPnt = aoe->getStartPoint(/*emulateCCW=*/true);
    Base::Vector3d endPnt   = aoe->getEndPoint(/*emulateCCW=*/true);
    double radmaj         = aoe->getMajorRadius();
    double radmin         = aoe->getMinorRadius();
    Base::Vector3d radmajdir = aoe->getMajorAxisDir();

    double dist_C_F = sqrt(radmaj*radmaj-radmin*radmin);
    // solver parameters
    Base::Vector3d focus1 = center + dist_C_F*radmajdir;

    double startAngle, endAngle;
    aoe->getRange(startAngle, endAngle, /*emulateCCW=*/true);

    GCS::Point p1, p2, p3;

    params.push_back(new double(startPnt.x));
    params.push_back(new double(startPnt.y));
    p1.x = params[params.size()-2];
    p1.y = params[params.size()-1];

    params.push_back(new double(endPnt.x));
    params.push_back(new double(endPnt.y));
    p2.x = params[params.size()-2];
    p2.y = params[params.size()-1];

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    p3.x = params[params.size()-2];
    p3.y = params[params.size()-1];

    params.push_back(new double(focus1.x));
    params.push_back(new double(focus1.y));
    double *f1X = params[params.size()-2];
    double *f1Y = params[params.size()-1];

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);
    def.midPointId = Points.size();
    Points.push_back(p3);

    //Points.push_back(f1);

    // add the radius parameters
    params.push_back(new double(radmin));
    double *rmin = params[params.size()-1];
    params.push_back(new double(startAngle));
    double *a1 = params[params.size()-1];
    params.push_back(new double(endAngle));
    double *a2 = params[params.size()-1];

    // set the arc for later constraints
    GCS::ArcOfEllipse a;
    a.start      = p1;
    a.end        = p2;
    a.center     = p3;
    a.focus1.x   = f1X;
    a.focus1.y   = f1Y;
    a.radmin     = rmin;
    a.startAngle = a1;
    a.endAngle   = a2;
    def.index = ArcsOfEllipse.size();
    ArcsOfEllipse.push_back(a);

    // store complete set
    Geoms.push_back(def);

    // arcs require an ArcRules constraint for the end points
    if (!fixed)
        GCSsys.addConstraintArcOfEllipseRules(a);

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addArcOfHyperbola(const Part::GeomArcOfHyperbola &hyperbolaSegment, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomArcOfHyperbola *aoh = static_cast<GeomArcOfHyperbola*>(hyperbolaSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = aoh;
    def.type = ArcOfHyperbola;

    Base::Vector3d center   = aoh->getCenter();
    Base::Vector3d startPnt = aoh->getStartPoint();
    Base::Vector3d endPnt   = aoh->getEndPoint();
    double radmaj         = aoh->getMajorRadius();
    double radmin         = aoh->getMinorRadius();
    Base::Vector3d radmajdir = aoh->getMajorAxisDir();

    double dist_C_F = sqrt(radmaj*radmaj+radmin*radmin);
    // solver parameters
    Base::Vector3d focus1 = center+dist_C_F*radmajdir; //+x

    double startAngle, endAngle;
    aoh->getRange(startAngle, endAngle,/*emulateCCW=*/true);

    GCS::Point p1, p2, p3;

    params.push_back(new double(startPnt.x));
    params.push_back(new double(startPnt.y));
    p1.x = params[params.size()-2];
    p1.y = params[params.size()-1];

    params.push_back(new double(endPnt.x));
    params.push_back(new double(endPnt.y));
    p2.x = params[params.size()-2];
    p2.y = params[params.size()-1];

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    p3.x = params[params.size()-2];
    p3.y = params[params.size()-1];

    params.push_back(new double(focus1.x));
    params.push_back(new double(focus1.y));
    double *f1X = params[params.size()-2];
    double *f1Y = params[params.size()-1];

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);
    def.midPointId = Points.size();
    Points.push_back(p3);

    // add the radius parameters
    params.push_back(new double(radmin));
    double *rmin = params[params.size()-1];
    params.push_back(new double(startAngle));
    double *a1 = params[params.size()-1];
    params.push_back(new double(endAngle));
    double *a2 = params[params.size()-1];

    // set the arc for later constraints
    GCS::ArcOfHyperbola a;
    a.start      = p1;
    a.end        = p2;
    a.center     = p3;
    a.focus1.x    = f1X;
    a.focus1.y    = f1Y;
    a.radmin     = rmin;
    a.startAngle = a1;
    a.endAngle   = a2;
    def.index = ArcsOfHyperbola.size();
    ArcsOfHyperbola.push_back(a);

    // store complete set
    Geoms.push_back(def);

    // arcs require an ArcRules constraint for the end points
    if (!fixed)
        GCSsys.addConstraintArcOfHyperbolaRules(a);

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addArcOfParabola(const Part::GeomArcOfParabola &parabolaSegment, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomArcOfParabola *aop = static_cast<GeomArcOfParabola*>(parabolaSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = aop;
    def.type = ArcOfParabola;

    Base::Vector3d vertex   = aop->getCenter();
    Base::Vector3d startPnt = aop->getStartPoint();
    Base::Vector3d endPnt   = aop->getEndPoint();
    Base::Vector3d focus    = aop->getFocus();

    double startAngle, endAngle;
    aop->getRange(startAngle, endAngle,/*emulateCCW=*/true);

    GCS::Point p1, p2, p3, p4;

    params.push_back(new double(startPnt.x));
    params.push_back(new double(startPnt.y));
    p1.x = params[params.size()-2];
    p1.y = params[params.size()-1];

    params.push_back(new double(endPnt.x));
    params.push_back(new double(endPnt.y));
    p2.x = params[params.size()-2];
    p2.y = params[params.size()-1];

    params.push_back(new double(vertex.x));
    params.push_back(new double(vertex.y));
    p3.x = params[params.size()-2];
    p3.y = params[params.size()-1];

    params.push_back(new double(focus.x));
    params.push_back(new double(focus.y));
    p4.x = params[params.size()-2];
    p4.y = params[params.size()-1];

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);
    def.midPointId = Points.size();
    Points.push_back(p3);

    // add the radius parameters
    params.push_back(new double(startAngle));
    double *a1 = params[params.size()-1];
    params.push_back(new double(endAngle));
    double *a2 = params[params.size()-1];

    // set the arc for later constraints
    GCS::ArcOfParabola a;
    a.start      = p1;
    a.end        = p2;
    a.vertex     = p3;
    a.focus1     = p4;
    a.startAngle = a1;
    a.endAngle   = a2;
    def.index = ArcsOfParabola.size();
    ArcsOfParabola.push_back(a);

    // store complete set
    Geoms.push_back(def);

    // arcs require an ArcRules constraint for the end points
    if (!fixed)
        GCSsys.addConstraintArcOfParabolaRules(a);

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addBSpline(const Part::GeomBSplineCurve &bspline, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomBSplineCurve *bsp = static_cast<GeomBSplineCurve*>(bspline.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = bsp;
    def.type = BSpline;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<double> weights = bsp->getWeights();
    std::vector<double> knots = bsp->getKnots();
    std::vector<int> mult = bsp->getMultiplicities();
    int degree = bsp->getDegree();
    bool periodic = bsp->isPeriodic();

    // OCC hack
    // c means there is a constraint on that weight, nc no constraint
    // OCC provides normalized weights when polynomic [1 1 1] [c c c] and unnormalized weights when rational [5 1 5] [c nc c]
    // then when changing from polynomic to rational, after the first solve any not-constrained pole circle gets normalized to 1.
    // This only happens when changing from polynomic to rational, any subsequent change remains unnormalized [5 1 5] [c nc nc]
    // This creates a visual problem that one of the poles shrinks to 1 mm when deleting an equality constraint.

    int lastoneindex = -1;
    int countones = 0;
    double lastnotone = 1.0;

    for(size_t i = 0; i < weights.size(); i++) {
        if(weights[i] != 1.0) {
            lastnotone = weights[i];
        }
        else { // is 1.0
            lastoneindex = i;
            countones++;
        }
    }

    if (countones == 1)
        weights[lastoneindex] = (lastnotone * 0.99);

    // end hack

    Base::Vector3d startPnt = bsp->getStartPoint();
    Base::Vector3d endPnt   = bsp->getEndPoint();

    std::vector<GCS::Point> spoles;

    for(std::vector<Base::Vector3d>::const_iterator it = poles.begin(); it != poles.end(); ++it){
        params.push_back(new double( (*it).x ));
        params.push_back(new double( (*it).y ));

        GCS::Point p;
        p.x = params[params.size()-2];
        p.y = params[params.size()-1];

        spoles.push_back(p);
    }

    std::vector<double *> sweights;

    for(std::vector<double>::const_iterator it = weights.begin(); it != weights.end(); ++it) {
        params.push_back(new double( (*it) ));
        sweights.push_back(params[params.size()-1]);
    }

    std::vector<double *> sknots;

    for(std::vector<double>::const_iterator it = knots.begin(); it != knots.end(); ++it) {
        double * knot = new double( (*it) );
        //params.push_back(knot);
        sknots.push_back(knot);
    }

    GCS::Point p1, p2;

    double * p1x = new double(startPnt.x);
    double * p1y = new double(startPnt.y);

    // if periodic, startpoint and endpoint do not play a role in the solver, this removes unnecessary DoF of determining where in the curve
    // the start and the stop should be
    if(!periodic) {
        params.push_back(p1x);
        params.push_back(p1y);
    }

    p1.x = p1x;
    p1.y = p1y;

    double * p2x = new double(endPnt.x);
    double * p2y = new double(endPnt.y);

    // if periodic, startpoint and endpoint do not play a role in the solver, this removes unnecessary DoF of determining where in the curve
    // the start and the stop should be
    if(!periodic) {
        params.push_back(p2x);
        params.push_back(p2y);
    }
    p2.x = p2x;
    p2.y = p2y;

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);

    GCS::BSpline bs;
    bs.start        = p1;
    bs.end          = p2;
    bs.poles        = spoles;
    bs.weights      = sweights;
    bs.knots        = sknots;
    bs.mult         = mult;
    bs.degree       = degree;
    bs.periodic     = periodic;
    def.index       = BSplines.size();

    // non-solver related, just to enable initialization of knotspoints which is not a parameter of the solver
    bs.knotpointGeoids.resize(knots.size());

    for(std::vector<int>::iterator it = bs.knotpointGeoids.begin(); it != bs.knotpointGeoids.end(); ++it) {
        (*it) = Constraint::GeoUndef;
    }

    BSplines.push_back(bs);

    // store complete set
    Geoms.push_back(def);

    // WARNING: This is only valid where the multiplicity of the endpoints conforms with a BSpline
    // only then the startpoint is the first control point and the endpoint is the last control point
    // accordingly, it is never the case for a periodic BSpline.
    // NOTE: For an external B-spline (i.e. fixed=true) we must not set the coincident constraints
    // as the points are not movable anyway.
    // See #issue 0003176: Sketcher: always over-constrained when referencing external B-Spline
    if (!fixed && !bs.periodic) {
        if (bs.mult[0] > bs.degree)
            GCSsys.addConstraintP2PCoincident(*(bs.poles.begin()),bs.start);
        if (bs.mult[mult.size()-1] > bs.degree)
            GCSsys.addConstraintP2PCoincident(*(bs.poles.end()-1),bs.end);
    }

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addCircle(const Part::GeomCircle &cir, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomCircle *circ = static_cast<GeomCircle*>(cir.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = circ;
    def.type = Circle;

    Base::Vector3d center = circ->getCenter();
    double radius         = circ->getRadius();

    GCS::Point p1;

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    p1.x = params[params.size()-2];
    p1.y = params[params.size()-1];

    params.push_back(new double(radius));

    def.midPointId = Points.size();
    Points.push_back(p1);

    // add the radius parameter
    double *r = params[params.size()-1];

    // set the circle for later constraints
    GCS::Circle c;
    c.center = p1;
    c.rad    = r;
    def.index = Circles.size();
    Circles.push_back(c);

    // store complete set
    Geoms.push_back(def);

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int Sketch::addEllipse(const Part::GeomEllipse &elip, bool fixed)
{

    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomEllipse *elips = static_cast<GeomEllipse*>(elip.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo  = elips;
    def.type = Ellipse;

    Base::Vector3d center = elips->getCenter();
    double radmaj         = elips->getMajorRadius();
    double radmin         = elips->getMinorRadius();
    Base::Vector3d radmajdir = elips->getMajorAxisDir();

    double dist_C_F = sqrt(radmaj*radmaj-radmin*radmin);
    // solver parameters
    Base::Vector3d focus1 = center + dist_C_F*radmajdir; //+x
    //double *radmin;

    GCS::Point c;

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    c.x = params[params.size()-2];
    c.y = params[params.size()-1];

    def.midPointId = Points.size(); // this takes midPointId+1
    Points.push_back(c);

    params.push_back(new double(focus1.x));
    params.push_back(new double(focus1.y));
    double *f1X = params[params.size()-2];
    double *f1Y = params[params.size()-1];

    // add the radius parameters
    params.push_back(new double(radmin));
    double *rmin = params[params.size()-1];

    // set the ellipse for later constraints
    GCS::Ellipse e;
    e.focus1.x  = f1X;
    e.focus1.y  = f1Y;
    e.center    = c;
    e.radmin    = rmin;

    def.index = Ellipses.size();
    Ellipses.push_back(e);

    // store complete set
    Geoms.push_back(def);

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

std::vector<Part::Geometry *> Sketch::extractGeometry(bool withConstructionElements,
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

Py::Tuple Sketch::getPyGeometry(void) const
{
    Py::Tuple tuple(Geoms.size());
    int i=0;
    for (std::vector<GeoDef>::const_iterator it=Geoms.begin(); it != Geoms.end(); ++it, i++) {
        if (it->type == Point) {
            Base::Vector3d temp(*(Points[it->startPointId].x),*(Points[it->startPointId].y),0);
            tuple[i] = Py::asObject(new VectorPy(temp));
        } else if (it->type == Line) {
            GeomLineSegment *lineSeg = static_cast<GeomLineSegment*>(it->geo->clone());
            tuple[i] = Py::asObject(new LineSegmentPy(lineSeg));
        } else if (it->type == Arc) {
            GeomArcOfCircle *aoc = static_cast<GeomArcOfCircle*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfCirclePy(aoc));
        } else if (it->type == Circle) {
            GeomCircle *circle = static_cast<GeomCircle*>(it->geo->clone());
            tuple[i] = Py::asObject(new CirclePy(circle));
        } else if (it->type == Ellipse) {
            GeomEllipse *ellipse = static_cast<GeomEllipse*>(it->geo->clone());
            tuple[i] = Py::asObject(new EllipsePy(ellipse));
        } else if (it->type == ArcOfEllipse) {
            GeomArcOfEllipse *ellipse = static_cast<GeomArcOfEllipse*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfEllipsePy(ellipse));
        } else if (it->type == ArcOfHyperbola) {
            GeomArcOfHyperbola *aoh = static_cast<GeomArcOfHyperbola*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfHyperbolaPy(aoh));
        } else if (it->type == ArcOfParabola) {
            GeomArcOfParabola *aop = static_cast<GeomArcOfParabola*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfParabolaPy(aop));
        } else if (it->type == BSpline) {
            GeomBSplineCurve *bsp = static_cast<GeomBSplineCurve*>(it->geo->clone());
            tuple[i] = Py::asObject(new BSplineCurvePy(bsp));
        } else {
            // not implemented type in the sketch!
        }
    }
    return tuple;
}

int Sketch::checkGeoId(int geoId) const
{
    if (geoId < 0)
        geoId += Geoms.size();//convert negative external-geometry index to index into Geoms
    if(!(   geoId >= 0   &&   geoId < int(Geoms.size())   ))
        throw Base::IndexError("Sketch::checkGeoId. GeoId index out range.");
    return geoId;
}

GCS::Curve* Sketch::getGCSCurveByGeoId(int geoId)
{
    geoId = checkGeoId(geoId);
    switch (Geoms[geoId].type) {
        case Line:
            return &Lines[Geoms[geoId].index];
        break;
        case Circle:
            return &Circles[Geoms[geoId].index];
        break;
        case Arc:
            return &Arcs[Geoms[geoId].index];
        break;
        case Ellipse:
            return &Ellipses[Geoms[geoId].index];
        break;
        case ArcOfEllipse:
            return &ArcsOfEllipse[Geoms[geoId].index];
        break;
        case ArcOfHyperbola:
            return &ArcsOfHyperbola[Geoms[geoId].index];
            break;
        case ArcOfParabola:
            return &ArcsOfParabola[Geoms[geoId].index];
            break;
        case BSpline:
            return &BSplines[Geoms[geoId].index];
            break;
        default:
            return 0;
    };
}

// constraint adding ==========================================================

int Sketch::addConstraint(const Constraint *constraint)
{
    if (Geoms.empty())
        throw Base::ValueError("Sketch::addConstraint. Can't add constraint to a sketch with no geometry!");
    int rtn = -1;

    ConstrDef c;
    c.constr=const_cast<Constraint *>(constraint);
    c.driving=constraint->isDriving;

    switch (constraint->Type) {
    case DistanceX:
        if (constraint->FirstPos == none){ // horizontal length of a line
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addDistanceXConstraint(constraint->First,c.value,c.driving);
        }
        else if (constraint->Second == Constraint::GeoUndef) {// point on fixed x-coordinate
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addCoordinateXConstraint(constraint->First,constraint->FirstPos,c.value,c.driving);
        }
        else if (constraint->SecondPos != none) {// point to point horizontal distance
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addDistanceXConstraint(constraint->First,constraint->FirstPos,
                                         constraint->Second,constraint->SecondPos,c.value,c.driving);
        }
        break;
    case DistanceY:
        if (constraint->FirstPos == none){ // vertical length of a line
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addDistanceYConstraint(constraint->First,c.value,c.driving);
        }
        else if (constraint->Second == Constraint::GeoUndef){ // point on fixed y-coordinate
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addCoordinateYConstraint(constraint->First,constraint->FirstPos,c.value,c.driving);
        }
        else if (constraint->SecondPos != none){ // point to point vertical distance
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addDistanceYConstraint(constraint->First,constraint->FirstPos,
                                         constraint->Second,constraint->SecondPos,c.value,c.driving);
        }
        break;
    case Horizontal:
        if (constraint->Second == Constraint::GeoUndef) // horizontal line
            rtn = addHorizontalConstraint(constraint->First);
        else // two points on the same horizontal line
            rtn = addHorizontalConstraint(constraint->First,constraint->FirstPos,
                                          constraint->Second,constraint->SecondPos);
        break;
    case Vertical:
        if (constraint->Second == Constraint::GeoUndef) // vertical line
            rtn = addVerticalConstraint(constraint->First);
        else // two points on the same vertical line
            rtn = addVerticalConstraint(constraint->First,constraint->FirstPos,
                                        constraint->Second,constraint->SecondPos);
        break;
    case Coincident:
        rtn = addPointCoincidentConstraint(constraint->First,constraint->FirstPos,constraint->Second,constraint->SecondPos);
        break;
    case PointOnObject:
        rtn = addPointOnObjectConstraint(constraint->First,constraint->FirstPos, constraint->Second);
        break;
    case Parallel:
        rtn = addParallelConstraint(constraint->First,constraint->Second);
        break;
    case Perpendicular:
        if (constraint->FirstPos == none &&
                constraint->SecondPos == none &&
                constraint->Third == Constraint::GeoUndef){
            //simple perpendicularity
            rtn = addPerpendicularConstraint(constraint->First,constraint->Second);
        } else {
            //any other point-wise perpendicularity
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addAngleAtPointConstraint(
                        constraint->First, constraint->FirstPos,
                        constraint->Second, constraint->SecondPos,
                        constraint->Third, constraint->ThirdPos,
                        c.value, constraint->Type, c.driving);
        }
        break;
    case Tangent:
        if (constraint->FirstPos == none &&
                constraint->SecondPos == none &&
                constraint->Third == Constraint::GeoUndef){
            //simple tangency
            rtn = addTangentConstraint(constraint->First,constraint->Second);
        } else {
            //any other point-wise tangency (endpoint-to-curve, endpoint-to-endpoint, tangent-via-point)
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addAngleAtPointConstraint(
                        constraint->First, constraint->FirstPos,
                        constraint->Second, constraint->SecondPos,
                        constraint->Third, constraint->ThirdPos,
                        c.value, constraint->Type, c.driving);
        }
        break;
    case Distance:
        if (constraint->SecondPos != none){ // point to point distance
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }
            rtn = addDistanceConstraint(constraint->First,constraint->FirstPos,
                                        constraint->Second,constraint->SecondPos,
                                        c.value,c.driving);
        }
        else if (constraint->Second != Constraint::GeoUndef) {
            if (constraint->FirstPos != none) { // point to line distance
                c.value = new double(constraint->getValue());
                if(c.driving)
                    FixParameters.push_back(c.value);
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }
                rtn = addDistanceConstraint(constraint->First,constraint->FirstPos,
                                            constraint->Second,c.value,c.driving);
            }
        }
        else {// line length
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addDistanceConstraint(constraint->First,c.value,c.driving);
        }
        break;
    case Angle:
        if (constraint->Third != Constraint::GeoUndef){
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addAngleAtPointConstraint (
                        constraint->First, constraint->FirstPos,
                        constraint->Second, constraint->SecondPos,
                        constraint->Third, constraint->ThirdPos,
                        c.value, constraint->Type,c.driving);
        } else if (constraint->SecondPos != none){ // angle between two lines (with explicit start points)
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addAngleConstraint(constraint->First,constraint->FirstPos,
                                     constraint->Second,constraint->SecondPos,c.value,c.driving);
        }
        else if (constraint->Second != Constraint::GeoUndef){ // angle between two lines
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addAngleConstraint(constraint->First,constraint->Second,c.value,c.driving);
        }
        else if (constraint->First != Constraint::GeoUndef) {// orientation angle of a line
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addAngleConstraint(constraint->First,c.value,c.driving);
        }
        break;
    case Radius:
    {
        c.value = new double(constraint->getValue());
        if(c.driving)
            FixParameters.push_back(c.value);
        else {
            Parameters.push_back(c.value);
            DrivenParameters.push_back(c.value);
        }

        rtn = addRadiusConstraint(constraint->First, c.value,c.driving);
        break;
    }
    case Diameter:
    {
        c.value = new double(constraint->getValue());
        if(c.driving)
            FixParameters.push_back(c.value);
        else {
            Parameters.push_back(c.value);
            DrivenParameters.push_back(c.value);
        }

        rtn = addDiameterConstraint(constraint->First, c.value,c.driving);
        break;
    }
    case Equal:
        rtn = addEqualConstraint(constraint->First,constraint->Second);
        break;
    case Symmetric:
        if (constraint->ThirdPos != none)
            rtn = addSymmetricConstraint(constraint->First,constraint->FirstPos,
                                         constraint->Second,constraint->SecondPos,
                                         constraint->Third,constraint->ThirdPos);
        else
            rtn = addSymmetricConstraint(constraint->First,constraint->FirstPos,
                                         constraint->Second,constraint->SecondPos,constraint->Third);
        break;
    case InternalAlignment:
        switch(constraint->AlignmentType) {
            case EllipseMajorDiameter:
                rtn = addInternalAlignmentEllipseMajorDiameter(constraint->First,constraint->Second);
                break;
            case EllipseMinorDiameter:
                rtn = addInternalAlignmentEllipseMinorDiameter(constraint->First,constraint->Second);
                break;
            case EllipseFocus1:
                rtn = addInternalAlignmentEllipseFocus1(constraint->First,constraint->Second);
                break;
            case EllipseFocus2:
                rtn = addInternalAlignmentEllipseFocus2(constraint->First,constraint->Second);
                break;
            case HyperbolaMajor:
                rtn = addInternalAlignmentHyperbolaMajorDiameter(constraint->First,constraint->Second);
                break;
            case HyperbolaMinor:
                rtn = addInternalAlignmentHyperbolaMinorDiameter(constraint->First,constraint->Second);
                break;
            case HyperbolaFocus:
                rtn = addInternalAlignmentHyperbolaFocus(constraint->First,constraint->Second);
                break;
            case ParabolaFocus:
                rtn = addInternalAlignmentParabolaFocus(constraint->First,constraint->Second);
                break;
            case BSplineControlPoint:
                rtn = addInternalAlignmentBSplineControlPoint(constraint->First,constraint->Second, constraint->InternalAlignmentIndex);
                break;
            case BSplineKnotPoint:
                rtn = addInternalAlignmentKnotPoint(constraint->First,constraint->Second, constraint->InternalAlignmentIndex);
                break;
            default:
                break;
        }
        break;
    case SnellsLaw:
        {
            c.value = new double(constraint->getValue());
            c.secondvalue = new double(constraint->getValue());

            if(c.driving) {
                FixParameters.push_back(c.value);
                FixParameters.push_back(c.secondvalue);
            }
            else {
                Parameters.push_back(c.value);
                Parameters.push_back(c.secondvalue);
                DrivenParameters.push_back(c.value);
                DrivenParameters.push_back(c.secondvalue);

            }

            //assert(constraint->ThirdPos==none); //will work anyway...
            rtn = addSnellsLawConstraint(constraint->First, constraint->FirstPos,
                                         constraint->Second, constraint->SecondPos,
                                         constraint->Third,
                                         c.value, c.secondvalue,c.driving);
        }
        break;
    case Sketcher::None: // ambiguous enum value
    case Sketcher::Block: // handled separately while adding geometry
    case NumConstraintTypes:
        break;
    }

    Constrs.push_back(c);
    return rtn;
}

int Sketch::addConstraints(const std::vector<Constraint *> &ConstraintList)
{
    int rtn = -1;

    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it)
        rtn = addConstraint (*it);

    return rtn;
}

int Sketch::addConstraints(const std::vector<Constraint *> &ConstraintList,
                           const std::vector<bool> &unenforceableConstraints)
{
    int rtn = -1;

    int cid = 0;
    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it,++cid) {
        if (!unenforceableConstraints[cid] && (*it)->Type != Block) {
            rtn = addConstraint (*it);
        }
        else {
            ++ConstraintsCounter; // For correct solver redundant reporting
        }
    }

    return rtn;
}

void Sketch::getBlockedGeometry(std::vector<bool> & blockedGeometry,
                                std::vector<bool> & unenforceableConstraints,
                                const std::vector<Constraint *> &ConstraintList) const
{
    std::vector<int> internalAlignmentConstraintIndex;
    std::vector<int> internalAlignmentgeo;

    std::vector<int> geo2blockingconstraintindex(blockedGeometry.size(),-1);

    // Detect Blocked and internal constraints
    int i = 0;
    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it,++i) {
        switch((*it)->Type) {
            case Block:
            {
                int geoid = (*it)->First;

                if(geoid>=0 && geoid<int(blockedGeometry.size())) {
                    blockedGeometry[geoid]=true;
                    geo2blockingconstraintindex[geoid]=i;
                }
            }
            break;
            case InternalAlignment:
                internalAlignmentConstraintIndex.push_back(i);
            break;
            default:
            break;
        }
    }

    // if a GeoId is blocked and it is linked to Internal Alignment, then GeoIds linked via Internal Alignment are also to be blocked
    for(std::vector<int>::iterator it = internalAlignmentConstraintIndex.begin(); it != internalAlignmentConstraintIndex.end() ; it++) {
        if (blockedGeometry[ConstraintList[(*it)]->Second]) {
            blockedGeometry[ConstraintList[(*it)]->First] = true;
            // associated geometry gets the same blocking constraint index as the blocked element
            geo2blockingconstraintindex[ConstraintList[(*it)]->First]= geo2blockingconstraintindex[ConstraintList[(*it)]->Second];
            internalAlignmentgeo.push_back(ConstraintList[(*it)]->First);
            unenforceableConstraints[(*it)]= true;
        }
    }

    i = 0;
    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it,++i) {
        if((*it)->isDriving) {
            // additionally any further constraint on auxiliary elements linked via Internal Alignment are also unenforceable.
            for(std::vector<int>::iterator itg = internalAlignmentgeo.begin(); itg != internalAlignmentgeo.end() ; itg++) {
                if( (*it)->First==*itg || (*it)->Second==*itg || (*it)->Third==*itg ) {
                    unenforceableConstraints[i]= true;
                }
            }
            // IMPORTANT NOTE:
            // The rest of the ignoring of redundant/conflicting applies to constraints introduced before the blocking constraint only
            // Constraints introduced after the block will not be ignored and will lead to redundancy/conflicting status as per normal
            // solver behaviour

            // further, any constraint taking only one element, which is blocked is also unenforceable
            if((*it)->Second==Constraint::GeoUndef && (*it)->Third==Constraint::GeoUndef && (*it)->First>=0 ) {
                if (blockedGeometry[(*it)->First] && i < geo2blockingconstraintindex[(*it)->First]) {
                    unenforceableConstraints[i]= true;
                }
            }
            // further any constraint on only two elements where both elements are blocked or one is blocked and the other is an axis or external
            // provided that the constraints precede the last block constraint.
            else if((*it)->Third==Constraint::GeoUndef) {
                if ( ((*it)->First>=0 && (*it)->Second>=0 && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Second] &&
                    (i < geo2blockingconstraintindex[(*it)->First] || i < geo2blockingconstraintindex[(*it)->Second])) ||
                    ((*it)->First<0 && (*it)->Second>=0 && blockedGeometry[(*it)->Second] && i < geo2blockingconstraintindex[(*it)->Second]) ||
                    ((*it)->First>=0 && (*it)->Second<0 && blockedGeometry[(*it)->First] && i < geo2blockingconstraintindex[(*it)->First]) ){
                    unenforceableConstraints[i]= true;
                }
            }
            // further any constraint on three elements where the three of them are blocked, or two are blocked and the other is an axis or external geo
            // or any constraint on three elements where one is blocked and the other two are axis or external geo, provided that the constraints precede
            // the last block constraint.
            else {
                if( ((*it)->First>=0 && (*it)->Second>=0 && (*it)->Third>=0 &&
                    blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Second] && blockedGeometry[(*it)->Third] &&
                  (i < geo2blockingconstraintindex[(*it)->First] || i < geo2blockingconstraintindex[(*it)->Second] || i < geo2blockingconstraintindex[(*it)->Third])) ||
                  ((*it)->First<0 && (*it)->Second>=0 && (*it)->Third>=0 && blockedGeometry[(*it)->Second] && blockedGeometry[(*it)->Third] &&
                  (i < geo2blockingconstraintindex[(*it)->Second] || i < geo2blockingconstraintindex[(*it)->Third])) ||
                  ((*it)->First>=0 && (*it)->Second<0 && (*it)->Third>=0 && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Third] &&
                  (i < geo2blockingconstraintindex[(*it)->First] || i < geo2blockingconstraintindex[(*it)->Third])) ||
                  ((*it)->First>=0 && (*it)->Second>=0 && (*it)->Third<0 && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Second] &&
                  (i < geo2blockingconstraintindex[(*it)->First] || i < geo2blockingconstraintindex[(*it)->Second])) ||
                  ((*it)->First>=0 && (*it)->Second<0 && (*it)->Third<0 && blockedGeometry[(*it)->First] && i < geo2blockingconstraintindex[(*it)->First]) ||
                  ((*it)->First<0 && (*it)->Second>=0 && (*it)->Third<0 && blockedGeometry[(*it)->Second] && i < geo2blockingconstraintindex[(*it)->Second]) ||
                  ((*it)->First<0 && (*it)->Second<0 && (*it)->Third>=0 && blockedGeometry[(*it)->Third] && i < geo2blockingconstraintindex[(*it)->Third]) ) {

                    unenforceableConstraints[i]= true;
                }
            }
        }
    }
}

int Sketch::addCoordinateXConstraint(int geoId, PointPos pos, double * value, bool driving)
{
    geoId = checkGeoId(geoId);

    int pointId = getPointId(geoId, pos);

    if (pointId >= 0 && pointId < int(Points.size())) {

        GCS::Point &p = Points[pointId];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCoordinateX(p, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addCoordinateYConstraint(int geoId, PointPos pos, double * value, bool driving)
{
    geoId = checkGeoId(geoId);

    int pointId = getPointId(geoId, pos);

    if (pointId >= 0 && pointId < int(Points.size())) {
        GCS::Point &p = Points[pointId];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCoordinateY(p, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addDistanceXConstraint(int geoId, double * value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintDifference(l.p1.x, l.p2.x, value, tag, driving);
    return ConstraintsCounter;
}

int Sketch::addDistanceYConstraint(int geoId, double * value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintDifference(l.p1.y, l.p2.y, value, tag, driving);
    return ConstraintsCounter;
}

int Sketch::addDistanceXConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, double * value, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintDifference(p1.x, p2.x, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addDistanceYConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, double * value, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintDifference(p1.y, p2.y, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

// horizontal line constraint
int Sketch::addHorizontalConstraint(int geoId)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];
    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintHorizontal(l, tag);
    return ConstraintsCounter;
}

// two points on a horizontal line constraint
int Sketch::addHorizontalConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintHorizontal(p1, p2, tag);
        return ConstraintsCounter;
    }
    return -1;
}

// vertical line constraint
int Sketch::addVerticalConstraint(int geoId)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];
    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintVertical(l, tag);
    return ConstraintsCounter;
}

// two points on a vertical line constraint
int Sketch::addVerticalConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintVertical(p1, p2, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addPointCoincidentConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PCoincident(p1, p2, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addParallelConstraint(int geoId1, int geoId2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Line ||
        Geoms[geoId2].type != Line)
        return -1;

    GCS::Line &l1 = Lines[Geoms[geoId1].index];
    GCS::Line &l2 = Lines[Geoms[geoId2].index];
    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintParallel(l1, l2, tag);
    return ConstraintsCounter;
}

// simple perpendicularity constraint
int Sketch::addPerpendicularConstraint(int geoId1, int geoId2)
{
    // accepts the following combinations:
    // 1) Line1, Line2/Circle2/Arc2
    // 2) Circle1, Line2 (converted to case #1)
    // 3) Arc1, Line2 (converted to case #1)
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId2].type == Line) {
        if (Geoms[geoId1].type == Line) {
            GCS::Line &l1 = Lines[Geoms[geoId1].index];
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPerpendicular(l1, l2, tag);
            return ConstraintsCounter;
        }
        else
            std::swap(geoId1, geoId2);
    }

    if (Geoms[geoId1].type == Line) {
        GCS::Line &l1 = Lines[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc || Geoms[geoId2].type == Circle) {
            GCS::Point &p2 = Points[Geoms[geoId2].midPointId];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p2, l1, tag);
            return ConstraintsCounter;
        }
    }

    Base::Console().Warning("Perpendicular constraints between %s and %s are not supported.\n",
                            nameByType(Geoms[geoId1].type), nameByType(Geoms[geoId2].type));
    return -1;
}

// simple tangency constraint
int Sketch::addTangentConstraint(int geoId1, int geoId2)
{
    // accepts the following combinations:
    // 1) Line1, Line2/Circle2/Arc2
    // 2) Circle1, Line2 (converted to case #1)
    //    Circle1, Circle2/Arc2
    // 3) Arc1, Line2 (converted to case #1)
    //    Arc1, Circle2/Arc2
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId2].type == Line) {
        if (Geoms[geoId1].type == Line) {
            GCS::Line &l1 = Lines[Geoms[geoId1].index];
            GCS::Point &l2p1 = Points[Geoms[geoId2].startPointId];
            GCS::Point &l2p2 = Points[Geoms[geoId2].endPointId];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(l2p1, l1, tag);
            GCSsys.addConstraintPointOnLine(l2p2, l1, tag);
            return ConstraintsCounter;
        }
        else
            std::swap(geoId1, geoId2);
    }

    if (Geoms[geoId1].type == Line) {
        GCS::Line &l = Lines[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(l, a, tag);
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(l, c, tag);
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == Ellipse) {
            GCS::Ellipse &e = Ellipses[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(l, e, tag);
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == ArcOfEllipse) {
            GCS::ArcOfEllipse &a = ArcsOfEllipse[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(l, a, tag);
            return ConstraintsCounter;
        }
    } else if (Geoms[geoId1].type == Circle) {
        GCS::Circle &c = Circles[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c2 = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, c2, tag);
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == Ellipse) {
            Base::Console().Error("Direct tangency constraint between circle and ellipse is not supported. Use tangent-via-point instead.");
            return -1;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, a, tag);
            return ConstraintsCounter;
        }
    } else if (Geoms[geoId1].type == Ellipse) {
        if (Geoms[geoId2].type == Circle) {
            Base::Console().Error("Direct tangency constraint between circle and ellipse is not supported. Use tangent-via-point instead.");
            return -1;
        } else if (Geoms[geoId2].type == Arc) {
            Base::Console().Error("Direct tangency constraint between arc and ellipse is not supported. Use tangent-via-point instead.");
            return -1;
        }
    } else if (Geoms[geoId1].type == Arc) {
        GCS::Arc &a = Arcs[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, a, tag);
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == Ellipse) {
            Base::Console().Error("Direct tangency constraint between arc and ellipse is not supported. Use tangent-via-point instead.");
            return -1;
        } else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(a, a2, tag);
            return ConstraintsCounter;
        }
    }

    return -1;
}

//This function handles any type of tangent, perpendicular and angle
// constraint that involves a point.
// i.e. endpoint-to-curve, endpoint-to-endpoint and tangent-via-point
//geoid1, geoid2 and geoid3 as in in the constraint object.
//For perp-ty and tangency, angle is used to lock the direction.
//angle==0 - autodetect direction. +pi/2, -pi/2 - specific direction.
int Sketch::addAngleAtPointConstraint(
        int geoId1, PointPos pos1,
        int geoId2, PointPos pos2,
        int geoId3, PointPos pos3,
        double * value,
        ConstraintType cTyp, bool driving)
{

    if(!(cTyp == Angle || cTyp == Tangent || cTyp == Perpendicular)) {
        //assert(0);//none of the three types. Why are we here??
        return -1;
    }

    bool avp = geoId3!=Constraint::GeoUndef; //is angle-via-point?
    bool e2c = pos2 == none  &&  pos1 != none;//is endpoint-to-curve?
    bool e2e = pos2 != none  &&  pos1 != none;//is endpoint-to-endpoint?

    if (!( avp || e2c || e2e )) {
        //assert(0);//none of the three types. Why are we here??
        return -1;
    }

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);
    if(avp)
        geoId3 = checkGeoId(geoId3);

    if (Geoms[geoId1].type == Point ||
        Geoms[geoId2].type == Point){
        Base::Console().Error("addAngleAtPointConstraint: one of the curves is a point!\n");
        return -1;
    }

    GCS::Curve* crv1 =getGCSCurveByGeoId(geoId1);
    GCS::Curve* crv2 =getGCSCurveByGeoId(geoId2);
    if (!crv1 || !crv2) {
        Base::Console().Error("addAngleAtPointConstraint: getGCSCurveByGeoId returned NULL!\n");
        return -1;
    }

    int pointId = -1;
    if(avp)
        pointId = getPointId(geoId3, pos3);
    else if (e2e || e2c)
        pointId = getPointId(geoId1, pos1);

    if (pointId < 0 || pointId >= int(Points.size())){
        Base::Console().Error("addAngleAtPointConstraint: point index out of range.\n");
        return -1;
    }
    GCS::Point &p = Points[pointId];
    GCS::Point* p2 = 0;
    if(e2e){//we need second point
        int pointId = getPointId(geoId2, pos2);
        if (pointId < 0 || pointId >= int(Points.size())){
            Base::Console().Error("addAngleAtPointConstraint: point index out of range.\n");
            return -1;
        }
        p2 = &(Points[pointId]);
    }

    double *angle = value;

    //For tangency/perpendicularity, we don't just copy the angle.
    //The angle stored for tangency/perpendicularity is offset, so that the options
    // are -Pi/2 and Pi/2. If value is 0 - this is an indicator of an old sketch.
    // Use autodetect then.
    //The same functionality is implemented in SketchObject.cpp, where
    // it is used to permanently lock down the autodecision.
    if (cTyp != Angle)
    {
        //The same functionality is implemented in SketchObject.cpp, where
        // it is used to permanently lock down the autodecision.
        double angleOffset = 0.0;//the difference between the datum value and the actual angle to apply. (datum=angle+offset)
        double angleDesire = 0.0;//the desired angle value (and we are to decide if 180* should be added to it)
        if (cTyp == Tangent) {angleOffset = -M_PI/2; angleDesire = 0.0;}
        if (cTyp == Perpendicular) {angleOffset = 0; angleDesire = M_PI/2;}

        if (*value==0.0) {//autodetect tangency internal/external (and same for perpendicularity)
            double angleErr = GCSsys.calculateAngleViaPoint(*crv1, *crv2, p) - angleDesire;
            //bring angleErr to -pi..pi
            if (angleErr > M_PI) angleErr -= M_PI*2;
            if (angleErr < -M_PI) angleErr += M_PI*2;

            //the autodetector
            if(fabs(angleErr) > M_PI/2 )
                angleDesire += M_PI;

            *angle = angleDesire;
        } else
            *angle = *value-angleOffset;
    }

    int tag = -1;
    if(e2c)
        tag = Sketch::addPointOnObjectConstraint(geoId1, pos1, geoId2, driving);//increases ConstraintsCounter
    if (e2e){
        tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PCoincident(p, *p2, tag, driving);
    }
    if(avp)
        tag = ++ConstraintsCounter;

    GCSsys.addConstraintAngleViaPoint(*crv1, *crv2, p, angle, tag, driving);
    return ConstraintsCounter;
}

// line length constraint
int Sketch::addDistanceConstraint(int geoId, double * value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintP2PDistance(l.p1, l.p2, value, tag, driving);
    return ConstraintsCounter;
}

// point to line distance constraint
int Sketch::addDistanceConstraint(int geoId1, PointPos pos1, int geoId2, double * value, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);

    if (Geoms[geoId2].type != Line)
        return -1;

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Line &l2 = Lines[Geoms[geoId2].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2LDistance(p1, l2, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

// point to point distance constraint
int Sketch::addDistanceConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, double * value, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PDistance(p1, p2, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addRadiusConstraint(int geoId, double * value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type == Circle) {
        GCS::Circle &c = Circles[Geoms[geoId].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCircleRadius(c, value, tag, driving);
        return ConstraintsCounter;
    }
    else if (Geoms[geoId].type == Arc) {
        GCS::Arc &a = Arcs[Geoms[geoId].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintArcRadius(a, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addDiameterConstraint(int geoId, double * value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type == Circle) {
        GCS::Circle &c = Circles[Geoms[geoId].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCircleDiameter(c, value, tag, driving);
        return ConstraintsCounter;
    }
    else if (Geoms[geoId].type == Arc) {
        GCS::Arc &a = Arcs[Geoms[geoId].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintArcDiameter(a, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

// line orientation angle constraint
int Sketch::addAngleConstraint(int geoId, double * value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type == Line) {
        GCS::Line &l = Lines[Geoms[geoId].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PAngle(l.p1, l.p2, value, tag, driving);
        return ConstraintsCounter;
    }
    else if (Geoms[geoId].type == Arc) {
        GCS::Arc &a = Arcs[Geoms[geoId].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintL2LAngle(a.center, a.start, a.center, a.end, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

// line to line angle constraint
int Sketch::addAngleConstraint(int geoId1, int geoId2, double * value, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Line ||
        Geoms[geoId2].type != Line)
        return -1;

    GCS::Line &l1 = Lines[Geoms[geoId1].index];
    GCS::Line &l2 = Lines[Geoms[geoId2].index];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintL2LAngle(l1, l2, value, tag, driving);
    return ConstraintsCounter;
}

// line to line angle constraint (with explicitly given start points)
int Sketch::addAngleConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, double * value, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Line ||
        Geoms[geoId2].type != Line)
        return -1;

    GCS::Point *l1p1=0, *l1p2=0;
    if (pos1 == start) {
        l1p1 = &Points[Geoms[geoId1].startPointId];
        l1p2 = &Points[Geoms[geoId1].endPointId];
    } else if (pos1 == end) {
        l1p1 = &Points[Geoms[geoId1].endPointId];
        l1p2 = &Points[Geoms[geoId1].startPointId];
    }

    GCS::Point *l2p1=0, *l2p2=0;
    if (pos2 == start) {
        l2p1 = &Points[Geoms[geoId2].startPointId];
        l2p2 = &Points[Geoms[geoId2].endPointId];
    } else if (pos2 == end) {
        l2p1 = &Points[Geoms[geoId2].endPointId];
        l2p2 = &Points[Geoms[geoId2].startPointId];
    }

    if (l1p1 == 0 || l2p1 == 0)
        return -1;

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintL2LAngle(*l1p1, *l1p2, *l2p1, *l2p2, value, tag, driving);
    return ConstraintsCounter;
}


int Sketch::addEqualConstraint(int geoId1, int geoId2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type == Line &&
        Geoms[geoId2].type == Line) {
        GCS::Line &l1 = Lines[Geoms[geoId1].index];
        GCS::Line &l2 = Lines[Geoms[geoId2].index];
        double dx1 = (*l1.p2.x - *l1.p1.x);
        double dy1 = (*l1.p2.y - *l1.p1.y);
        double dx2 = (*l2.p2.x - *l2.p1.x);
        double dy2 = (*l2.p2.y - *l2.p1.y);
        double value = (sqrt(dx1*dx1+dy1*dy1)+sqrt(dx2*dx2+dy2*dy2))/2;
        // add the parameter for the common length (this is added to Parameters, not FixParameters)
        Parameters.push_back(new double(value));
        double *length = Parameters[Parameters.size()-1];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintEqualLength(l1, l2, length, tag);
        return ConstraintsCounter;
    }

    if (Geoms[geoId2].type == Circle) {
        if (Geoms[geoId1].type == Circle) {
            GCS::Circle &c1 = Circles[Geoms[geoId1].index];
            GCS::Circle &c2 = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadius(c1, c2, tag);
            return ConstraintsCounter;
        }
        else
            std::swap(geoId1, geoId2);
    }

    if (Geoms[geoId2].type == Ellipse) {
        if (Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse &e1 = Ellipses[Geoms[geoId1].index];
            GCS::Ellipse &e2 = Ellipses[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadii(e1, e2, tag);
            return ConstraintsCounter;
        }
        else
            std::swap(geoId1, geoId2);
    }

    if (Geoms[geoId1].type == Circle) {
        GCS::Circle &c1 = Circles[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadius(c1, a2, tag);
            return ConstraintsCounter;
        }
    }

    if (Geoms[geoId1].type == Arc &&
        Geoms[geoId2].type == Arc) {
        GCS::Arc &a1 = Arcs[Geoms[geoId1].index];
        GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintEqualRadius(a1, a2, tag);
        return ConstraintsCounter;
    }

    if (Geoms[geoId2].type == ArcOfEllipse) {
        if (Geoms[geoId1].type == ArcOfEllipse) {
            GCS::ArcOfEllipse &a1 = ArcsOfEllipse[Geoms[geoId1].index];
            GCS::ArcOfEllipse &a2 = ArcsOfEllipse[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadii(a1, a2, tag);
            return ConstraintsCounter;
        }
    }

    if (Geoms[geoId2].type == ArcOfHyperbola) {
        if (Geoms[geoId1].type == ArcOfHyperbola) {
            GCS::ArcOfHyperbola &a1 = ArcsOfHyperbola[Geoms[geoId1].index];
            GCS::ArcOfHyperbola &a2 = ArcsOfHyperbola[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadii(a1, a2, tag);
            return ConstraintsCounter;
        }
    }

    if (Geoms[geoId2].type == ArcOfParabola) {
        if (Geoms[geoId1].type == ArcOfParabola) {
            GCS::ArcOfParabola &a1 = ArcsOfParabola[Geoms[geoId1].index];
            GCS::ArcOfParabola &a2 = ArcsOfParabola[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualFocus(a1, a2, tag);
            return ConstraintsCounter;
        }
    }

    if (Geoms[geoId1].type == Ellipse) {
        GCS::Ellipse &e1 = Ellipses[Geoms[geoId1].index];
        if (Geoms[geoId2].type == ArcOfEllipse) {
            GCS::ArcOfEllipse &a2 = ArcsOfEllipse[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadii(a2, e1, tag);
            return ConstraintsCounter;
        }
    }

    Base::Console().Warning("Equality constraints between %s and %s are not supported.\n",
                            nameByType(Geoms[geoId1].type), nameByType(Geoms[geoId2].type));
    return -1;
}

// point on object constraint
int Sketch::addPointOnObjectConstraint(int geoId1, PointPos pos1, int geoId2, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];

        if (Geoms[geoId2].type == Line) {
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p1, l2, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnArc(p1, a, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnCircle(p1, c, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Ellipse) {
            GCS::Ellipse &e = Ellipses[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnEllipse(p1, e, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == ArcOfEllipse) {
            GCS::ArcOfEllipse &a = ArcsOfEllipse[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnEllipse(p1, a, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == ArcOfHyperbola) {
            GCS::ArcOfHyperbola &a = ArcsOfHyperbola[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnHyperbolicArc(p1, a, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == ArcOfParabola) {
            GCS::ArcOfParabola &a = ArcsOfParabola[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnParabolicArc(p1, a, tag, driving);
            return ConstraintsCounter;
        }
    }
    return -1;
}

// symmetric points constraint
int Sketch::addSymmetricConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, int geoId3)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);
    geoId3 = checkGeoId(geoId3);

    if (Geoms[geoId3].type != Line)
        return -1;

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];
        GCS::Line &l = Lines[Geoms[geoId3].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PSymmetric(p1, p2, l, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addSymmetricConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2,
                                   int geoId3, PointPos pos3)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);
    geoId3 = checkGeoId(geoId3);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);
    int pointId3 = getPointId(geoId3, pos3);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size()) &&
        pointId3 >= 0 && pointId3 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];
        GCS::Point &p = Points[pointId3];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PSymmetric(p1, p2, p, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addSnellsLawConstraint(int geoIdRay1, PointPos posRay1,
                                   int geoIdRay2, PointPos posRay2,
                                   int geoIdBnd,
                                   double * value,
                                   double * secondvalue,
                                   bool driving
                                  )
{

    geoIdRay1 = checkGeoId(geoIdRay1);
    geoIdRay2 = checkGeoId(geoIdRay2);
    geoIdBnd = checkGeoId(geoIdBnd);

    if (Geoms[geoIdRay1].type == Point ||
        Geoms[geoIdRay2].type == Point){
        Base::Console().Error("addSnellsLawConstraint: point is not a curve. Not applicable!\n");
        return -1;
    }

    GCS::Curve* ray1 =getGCSCurveByGeoId(geoIdRay1);
    GCS::Curve* ray2 =getGCSCurveByGeoId(geoIdRay2);
    GCS::Curve* boundary =getGCSCurveByGeoId(geoIdBnd);
    if (!ray1 || !ray2 || !boundary) {
        Base::Console().Error("addSnellsLawConstraint: getGCSCurveByGeoId returned NULL!\n");
        return -1;
    }

    int pointId1 = getPointId(geoIdRay1, posRay1);
    int pointId2 = getPointId(geoIdRay2, posRay2);
    if ( pointId1 < 0 || pointId1 >= int(Points.size()) ||
         pointId2 < 0 || pointId2 >= int(Points.size()) ){
        Base::Console().Error("addSnellsLawConstraint: point index out of range.\n");
        return -1;
    }
    GCS::Point &p1 = Points[pointId1];

    // add the parameters (refractive indexes)
    // n1 uses the place hold by n2divn1, so that is retrivable in updateNonDrivingConstraints
    double *n1 = value;
    double *n2 = secondvalue;

    double n2divn1=*value;

    if ( fabs(n2divn1) >= 1.0 ){
        *n2 = n2divn1;
        *n1 = 1.0;
    } else {
        *n2 = 1.0;
        *n1 = 1/n2divn1;
    }

    int tag = -1;
    //tag = Sketch::addPointOnObjectConstraint(geoIdRay1, posRay1, geoIdBnd);//increases ConstraintsCounter
    tag = ++ConstraintsCounter;
    //GCSsys.addConstraintP2PCoincident(p1, p2, tag);
    GCSsys.addConstraintSnellsLaw(*ray1, *ray2,
                                  *boundary, p1,
                                  n1, n2,
                                  posRay1==start, posRay2 == end,
                                  tag, driving);
    return ConstraintsCounter;
}


int Sketch::addInternalAlignmentEllipseMajorDiameter(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Ellipse && Geoms[geoId1].type != ArcOfEllipse)
        return -1;
    if (Geoms[geoId2].type != Line)
        return -1;

    int pointId1 = getPointId(geoId2, start);
    int pointId2 = getPointId(geoId2, end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        if(Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse &e1 = Ellipses[Geoms[geoId1].index];

            // constraints
            // 1. start point with ellipse -a
            // 2. end point with ellipse +a
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseMajorDiameter(e1, p1, p2, tag);
            return ConstraintsCounter;

        }
        else {
            GCS::ArcOfEllipse &a1 = ArcsOfEllipse[Geoms[geoId1].index];

            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseMajorDiameter(a1, p1, p2, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}

int Sketch::addInternalAlignmentEllipseMinorDiameter(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Ellipse && Geoms[geoId1].type != ArcOfEllipse)
        return -1;
    if (Geoms[geoId2].type != Line)
        return -1;

    int pointId1 = getPointId(geoId2, start);
    int pointId2 = getPointId(geoId2, end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        if(Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse &e1 = Ellipses[Geoms[geoId1].index];

            // constraints
            // 1. start point with ellipse -a
            // 2. end point with ellipse +a
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseMinorDiameter(e1, p1, p2, tag);
            return ConstraintsCounter;
        }
        else {
            GCS::ArcOfEllipse &a1 = ArcsOfEllipse[Geoms[geoId1].index];

            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseMinorDiameter(a1, p1, p2, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}

int Sketch::addInternalAlignmentEllipseFocus1(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Ellipse && Geoms[geoId1].type != ArcOfEllipse)
        return -1;
    if (Geoms[geoId2].type != Point)
        return -1;

    int pointId1 = getPointId(geoId2, start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];

        if(Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse &e1 = Ellipses[Geoms[geoId1].index];

            // constraints
            // 1. start point with ellipse -a
            // 2. end point with ellipse +a
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseFocus1(e1, p1, tag);
            return ConstraintsCounter;
        }
        else {
            GCS::ArcOfEllipse &a1 = ArcsOfEllipse[Geoms[geoId1].index];

            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseFocus1(a1, p1, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}


int Sketch::addInternalAlignmentEllipseFocus2(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Ellipse && Geoms[geoId1].type != ArcOfEllipse)
        return -1;
    if (Geoms[geoId2].type != Point)
        return -1;

    int pointId1 = getPointId(geoId2, start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];

        if(Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse &e1 = Ellipses[Geoms[geoId1].index];

            // constraints
            // 1. start point with ellipse -a
            // 2. end point with ellipse +a
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseFocus2(e1, p1, tag);
            return ConstraintsCounter;
        }
        else {
            GCS::ArcOfEllipse &a1 = ArcsOfEllipse[Geoms[geoId1].index];

            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseFocus2(a1, p1, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}


int Sketch::addInternalAlignmentHyperbolaMajorDiameter(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfHyperbola)
        return -1;
    if (Geoms[geoId2].type != Line)
        return -1;

    int pointId1 = getPointId(geoId2, start);
    int pointId2 = getPointId(geoId2, end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {

        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        GCS::ArcOfHyperbola &a1 = ArcsOfHyperbola[Geoms[geoId1].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentHyperbolaMajorDiameter(a1, p1, p2, tag);
        return ConstraintsCounter;
    }

    return -1;
}

int Sketch::addInternalAlignmentHyperbolaMinorDiameter(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfHyperbola)
        return -1;
    if (Geoms[geoId2].type != Line)
        return -1;

    int pointId1 = getPointId(geoId2, start);
    int pointId2 = getPointId(geoId2, end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {

        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        GCS::ArcOfHyperbola &a1 = ArcsOfHyperbola[Geoms[geoId1].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentHyperbolaMinorDiameter(a1, p1, p2, tag);
        return ConstraintsCounter;
    }

    return -1;
}

int Sketch::addInternalAlignmentHyperbolaFocus(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfHyperbola)
        return -1;
    if (Geoms[geoId2].type != Point)
        return -1;

    int pointId1 = getPointId(geoId2, start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];

        GCS::ArcOfHyperbola &a1 = ArcsOfHyperbola[Geoms[geoId1].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentHyperbolaFocus(a1, p1, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addInternalAlignmentParabolaFocus(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfParabola)
        return -1;
    if (Geoms[geoId2].type != Point)
        return -1;

    int pointId1 = getPointId(geoId2, start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];

        GCS::ArcOfParabola &a1 = ArcsOfParabola[Geoms[geoId1].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentParabolaFocus(a1, p1, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addInternalAlignmentBSplineControlPoint(int geoId1, int geoId2, int poleindex)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != BSpline)
        return -1;
    if (Geoms[geoId2].type != Circle)
        return -1;

    int pointId1 = getPointId(geoId2, mid);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Circle &c = Circles[Geoms[geoId2].index];

        GCS::BSpline &b = BSplines[Geoms[geoId1].index];

        assert(poleindex < static_cast<int>(b.poles.size()) && poleindex >= 0);

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentBSplineControlPoint(b, c, poleindex, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addInternalAlignmentKnotPoint(int geoId1, int geoId2, int knotindex)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != BSpline)
        return -1;
    if (Geoms[geoId2].type != Point)
        return -1;

    int pointId1 = getPointId(geoId2, start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
       // GCS::Point &p = Points[pointId1];

        GCS::BSpline &b = BSplines[Geoms[geoId1].index];

        // no constraint is actually added, as knots are fixed geometry in this implementation
        // indexing is added here.

        b.knotpointGeoids[knotindex] = geoId2;

        return ConstraintsCounter;
    }
    return -1;
}

double Sketch::calculateAngleViaPoint(int geoId1, int geoId2, double px, double py)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    GCS::Point p;
    p.x = &px;
    p.y = &py;

    //check pointers
    GCS::Curve* crv1 =getGCSCurveByGeoId(geoId1);
    GCS::Curve* crv2 =getGCSCurveByGeoId(geoId2);
    if (!crv1 || !crv2) {
        throw Base::ValueError("calculateAngleViaPoint: getGCSCurveByGeoId returned NULL!");
    }

    return GCSsys.calculateAngleViaPoint(*crv1, *crv2, p);
}

Base::Vector3d Sketch::calculateNormalAtPoint(int geoIdCurve, double px, double py)
{
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
}

bool Sketch::updateGeometry()
{
    int i=0;
    for (std::vector<GeoDef>::const_iterator it=Geoms.begin(); it != Geoms.end(); ++it, i++) {
        try {
            if (it->type == Point) {
                GeomPoint *point = static_cast<GeomPoint*>(it->geo);

                if(!point->Construction) {
                    point->setPoint(Vector3d(*Points[it->startPointId].x,
                                         *Points[it->startPointId].y,
                                         0.0)
                               );
                }
            } else if (it->type == Line) {
                GeomLineSegment *lineSeg = static_cast<GeomLineSegment*>(it->geo);
                lineSeg->setPoints(Vector3d(*Lines[it->index].p1.x,
                                            *Lines[it->index].p1.y,
                                            0.0),
                                   Vector3d(*Lines[it->index].p2.x,
                                            *Lines[it->index].p2.y,
                                            0.0)
                                  );
            } else if (it->type == Arc) {
                GCS::Arc &myArc = Arcs[it->index];
                // the following 4 lines are redundant since these equations are already included in the arc constraints
//                *myArc.start.x = *myArc.center.x + *myArc.rad * cos(*myArc.startAngle);
//                *myArc.start.y = *myArc.center.y + *myArc.rad * sin(*myArc.startAngle);
//                *myArc.end.x = *myArc.center.x + *myArc.rad * cos(*myArc.endAngle);
//                *myArc.end.y = *myArc.center.y + *myArc.rad * sin(*myArc.endAngle);
                GeomArcOfCircle *aoc = static_cast<GeomArcOfCircle*>(it->geo);
                aoc->setCenter(Vector3d(*Points[it->midPointId].x,
                                        *Points[it->midPointId].y,
                                        0.0)
                              );
                aoc->setRadius(*myArc.rad);
                aoc->setRange(*myArc.startAngle, *myArc.endAngle, /*emulateCCW=*/true);
            } else if (it->type == ArcOfEllipse) {
                GCS::ArcOfEllipse &myArc = ArcsOfEllipse[it->index];

                GeomArcOfEllipse *aoe = static_cast<GeomArcOfEllipse*>(it->geo);

                Base::Vector3d center = Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 = Vector3d(*myArc.focus1.x, *myArc.focus1.y, 0.0);
                double radmin = *myArc.radmin;

                Base::Vector3d fd=f1-center;
                double radmaj = sqrt(fd*fd+radmin*radmin);

                aoe->setCenter(center);
                if ( radmaj >= aoe->getMinorRadius() ){//ensure that ellipse's major radius is always larger than minor raduis... may still cause problems with degenerates.
                    aoe->setMajorRadius(radmaj);
                    aoe->setMinorRadius(radmin);
                }  else {
                    aoe->setMinorRadius(radmin);
                    aoe->setMajorRadius(radmaj);
                }
                aoe->setMajorAxisDir(fd);
                aoe->setRange(*myArc.startAngle, *myArc.endAngle, /*emulateCCW=*/true);
            } else if (it->type == Circle) {
                GeomCircle *circ = static_cast<GeomCircle*>(it->geo);
                circ->setCenter(Vector3d(*Points[it->midPointId].x,
                                         *Points[it->midPointId].y,
                                         0.0)
                               );
                circ->setRadius(*Circles[it->index].rad);
            } else if (it->type == Ellipse) {

                GeomEllipse *ellipse = static_cast<GeomEllipse*>(it->geo);

                Base::Vector3d center = Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 = Vector3d(*Ellipses[it->index].focus1.x, *Ellipses[it->index].focus1.y, 0.0);
                double radmin = *Ellipses[it->index].radmin;

                Base::Vector3d fd=f1-center;
                double radmaj = sqrt(fd*fd+radmin*radmin);

                ellipse->setCenter(center);
                if ( radmaj >= ellipse->getMinorRadius() ){//ensure that ellipse's major radius is always larger than minor raduis... may still cause problems with degenerates.
                    ellipse->setMajorRadius(radmaj);
                    ellipse->setMinorRadius(radmin);
                }  else {
                    ellipse->setMinorRadius(radmin);
                    ellipse->setMajorRadius(radmaj);
                }
                ellipse->setMajorAxisDir(fd);
            } else if (it->type == ArcOfHyperbola) {
                GCS::ArcOfHyperbola &myArc = ArcsOfHyperbola[it->index];

                GeomArcOfHyperbola *aoh = static_cast<GeomArcOfHyperbola*>(it->geo);

                Base::Vector3d center = Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 = Vector3d(*myArc.focus1.x, *myArc.focus1.y, 0.0);
                double radmin = *myArc.radmin;

                Base::Vector3d fd=f1-center;
                double radmaj = sqrt(fd*fd-radmin*radmin);

                aoh->setCenter(center);
                if ( radmaj >= aoh->getMinorRadius() ){
                    aoh->setMajorRadius(radmaj);
                    aoh->setMinorRadius(radmin);
                }  else {
                    aoh->setMinorRadius(radmin);
                    aoh->setMajorRadius(radmaj);
                }
                aoh->setMajorAxisDir(fd);
                aoh->setRange(*myArc.startAngle, *myArc.endAngle, /*emulateCCW=*/true);
            } else if (it->type == ArcOfParabola) {
                GCS::ArcOfParabola &myArc = ArcsOfParabola[it->index];

                GeomArcOfParabola *aop = static_cast<GeomArcOfParabola*>(it->geo);

                Base::Vector3d vertex = Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 = Vector3d(*myArc.focus1.x, *myArc.focus1.y, 0.0);

                Base::Vector3d fd=f1-vertex;

                aop->setXAxisDir(fd);
                aop->setCenter(vertex);
                aop->setFocal(fd.Length());
                aop->setRange(*myArc.startAngle, *myArc.endAngle, /*emulateCCW=*/true);
            } else if (it->type == BSpline) {
                GCS::BSpline &mybsp = BSplines[it->index];

                GeomBSplineCurve *bsp = static_cast<GeomBSplineCurve*>(it->geo);

                std::vector<Base::Vector3d> poles;
                std::vector<double> weights;

                std::vector<GCS::Point>::const_iterator it1;
                std::vector<double *>::const_iterator it2;

                for( it1 = mybsp.poles.begin(), it2 = mybsp.weights.begin(); it1 != mybsp.poles.end() && it2 != mybsp.weights.end(); ++it1, ++it2) {
                    poles.push_back(Vector3d( *(*it1).x , *(*it1).y , 0.0));
                    weights.push_back(*(*it2));
                }

                bsp->setPoles(poles, weights);

                std::vector<double> knots;
                std::vector<int> mult;

                std::vector<double *>::const_iterator it3;
                std::vector<int>::const_iterator it4;

                for( it3 = mybsp.knots.begin(), it4 = mybsp.mult.begin(); it3 != mybsp.knots.end() && it4 != mybsp.mult.end(); ++it3, ++it4) {
                    knots.push_back(*(*it3));
                    mult.push_back((*it4));
                }

                bsp->setKnots(knots,mult);

                #if OCC_VERSION_HEX >= 0x060900
                int index = 0;
                for(std::vector<int>::const_iterator it5 = mybsp.knotpointGeoids.begin(); it5 != mybsp.knotpointGeoids.end(); ++it5, index++) {
                    if( *it5 != Constraint::GeoUndef) {
                        if (Geoms[*it5].type == Point) {
                            GeomPoint *point = static_cast<GeomPoint*>(Geoms[*it5].geo);

                            if(point->Construction) {
                                point->setPoint(bsp->pointAtParameter(knots[index]));
                            }
                        }
                    }
                }
                #endif

            }
        } catch (Base::Exception &e) {
            Base::Console().Error("Updating geometry: Error build geometry(%d): %s\n",
                                  i,e.what());
            return false;
        }
    }
    return true;
}

bool Sketch::updateNonDrivingConstraints()
{
     for (std::vector<ConstrDef>::iterator it = Constrs.begin();it!=Constrs.end();++it){
        if(!(*it).driving) {
            if((*it).constr->Type==SnellsLaw) {
                double n1 = *((*it).value);
                double n2 = *((*it).secondvalue);

                (*it).constr->setValue(n2/n1);
            }
            else if((*it).constr->Type==Angle) {

                (*it).constr->setValue(std::remainder(*((*it).value), 2.0*M_PI));
            }
            else if((*it).constr->Type==Diameter && (*it).constr->First>=0 ) {

                (*it).constr->setValue(2.0**((*it).value));
            }
            else {
                (*it).constr->setValue(*((*it).value));
            }
        }
     }
    return true;
}

// solving ==========================================================

int Sketch::solve(void)
{
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
}

int Sketch::initMove(int geoId, PointPos pos, bool fine)
{
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
    return 0;
}

void Sketch::resetInitMove()
{
    isInitMove = false;
}

int Sketch::movePoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative)
{
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
}

int Sketch::setDatum(int /*constrId*/, double /*value*/)
{
    return -1;
}

int Sketch::getPointId(int geoId, PointPos pos) const
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

Base::Vector3d Sketch::getPoint(int geoId, PointPos pos) const
{
    geoId = checkGeoId(geoId);
    int pointId = getPointId(geoId, pos);
    if (pointId != -1)
        return Base::Vector3d(*Points[pointId].x, *Points[pointId].y, 0);

    return Base::Vector3d();
}

bool Sketch::hasDependentParameters(int geoId, PointPos pos) const
{
    try {
        geoId = checkGeoId(geoId);
    }
    catch (Base::Exception&) {
        return false;
    }

    if(Geoms[geoId].external)
        return true;

    switch(Geoms[geoId].type) {
        case Point:
        {
            switch(pos) { // NOTE: points are added to all the cases, see addition.
                case none: return Points[Geoms[geoId].index].hasDependentParameters;break;
                case start: return Points[Geoms[geoId].startPointId].hasDependentParameters;break;
                case end: return Points[Geoms[geoId].endPointId].hasDependentParameters;break;
                case mid: return Points[Geoms[geoId].midPointId].hasDependentParameters;break;
            }
        }
        case Line:
        {
            switch(pos) {
                case none: return Lines[Geoms[geoId].index].hasDependentParameters;break;
                case start: return Points[Geoms[geoId].startPointId].hasDependentParameters;break;
                case end: return Points[Geoms[geoId].endPointId].hasDependentParameters;break;
                case mid: return false;break;
            }
        }
        case Arc:
        {
            switch(pos) {
                case none: return Arcs[Geoms[geoId].index].hasDependentParameters;break;
                case start: return Points[Geoms[geoId].startPointId].hasDependentParameters;break;
                case end: return Points[Geoms[geoId].endPointId].hasDependentParameters;break;
                case mid: return Points[Geoms[geoId].midPointId].hasDependentParameters;break;
            }
        }
        case Circle:
        {
            switch(pos) { // NOTE: points are added to all the cases, see addition.
                case none: return Circles[Geoms[geoId].index].hasDependentParameters;break;
                case start: return false;break;
                case end: return false;break;
                case mid: return Points[Geoms[geoId].midPointId].hasDependentParameters;break;
            }
        }

        case Ellipse:
        {
            switch(pos) { // NOTE: points are added to all the cases, see addition.
                case none: return Ellipses[Geoms[geoId].index].hasDependentParameters;break;
                case start: return false;break;
                case end: return false;break;
                case mid: return Points[Geoms[geoId].midPointId].hasDependentParameters;break;
            }
        }
        case ArcOfEllipse:
        {
            switch(pos) {
                case none: return ArcsOfEllipse[Geoms[geoId].index].hasDependentParameters;break;
                case start: return Points[Geoms[geoId].startPointId].hasDependentParameters;break;
                case end: return Points[Geoms[geoId].endPointId].hasDependentParameters;break;
                case mid: return Points[Geoms[geoId].midPointId].hasDependentParameters;break;
            }
        }
        case ArcOfHyperbola:
        {
            switch(pos) {
                case none: return ArcsOfHyperbola[Geoms[geoId].index].hasDependentParameters;break;
                case start: return Points[Geoms[geoId].startPointId].hasDependentParameters;break;
                case end: return Points[Geoms[geoId].endPointId].hasDependentParameters;break;
                case mid: return Points[Geoms[geoId].midPointId].hasDependentParameters;break;
            }
        }
        case ArcOfParabola:
        {
            switch(pos) {
                case none: return ArcsOfParabola[Geoms[geoId].index].hasDependentParameters;break;
                case start: return Points[Geoms[geoId].startPointId].hasDependentParameters;break;
                case end: return Points[Geoms[geoId].endPointId].hasDependentParameters;break;
                case mid: return Points[Geoms[geoId].midPointId].hasDependentParameters;break;
            }
        }
        case BSpline:
        {
            switch(pos) {
                case none: return BSplines[Geoms[geoId].index].hasDependentParameters;break;
                case start: return Points[Geoms[geoId].startPointId].hasDependentParameters;break;
                case end: return Points[Geoms[geoId].endPointId].hasDependentParameters;break;
                case mid: return false;break;
            }
        }
        case None:
            return false; break;
    }

    return false;
}

TopoShape Sketch::toShape(void) const
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
        if (!it->external && !it->geo->Construction && (it->type != Point)) {
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
    // FIXME: if free edges are left over its probably better to
    // create a compound with the closed structures and let the
    // features decide what to do with it...
    if (edge_list.size() > 0)
        Base::Console().Warning("Left over edges in Sketch. Only closed structures will be propagated at the moment!\n");

#endif

    return result;
}

// Persistence implementer -------------------------------------------------

unsigned int Sketch::getMemSize(void) const
{
    return 0;
}

void Sketch::Save(Writer &) const
{

}

void Sketch::Restore(XMLReader &)
{

}

