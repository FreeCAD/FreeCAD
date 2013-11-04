/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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
#include <Mod/Part/App/CirclePy.h>
#include <Mod/Part/App/EllipsePy.h>
#include <Mod/Part/App/LinePy.h>

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>

#include "Sketch.h"
#include "Constraint.h"
#include <math.h>

#include <iostream>


using namespace Sketcher;
using namespace Base;
using namespace Part;

TYPESYSTEM_SOURCE(Sketcher::Sketch, Base::Persistence)

Sketch::Sketch()
: GCSsys(), ConstraintsCounter(0), isInitMove(false)
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

    // deleting the doubles allocated with new
    for (std::vector<double*>::iterator it = Parameters.begin(); it != Parameters.end(); ++it)
        if (*it) delete *it;
    Parameters.clear();
    for (std::vector<double*>::iterator it = FixParameters.begin(); it != FixParameters.end(); ++it)
        if (*it) delete *it;
    FixParameters.clear();

    // deleting the geometry copied into this sketch
    for (std::vector<GeoDef>::iterator it = Geoms.begin(); it != Geoms.end(); ++it)
        if (it->geo) delete it->geo;
    Geoms.clear();

    GCSsys.clear();
    isInitMove = false;
    ConstraintsCounter = 0;
    Conflicting.clear();
}

int Sketch::setUpSketch(const std::vector<Part::Geometry *> &GeoList,
                        const std::vector<Constraint *> &ConstraintList,
                        int extGeoCount)
{
    clear();

    std::vector<Part::Geometry *> intGeoList, extGeoList;
    for (int i=0; i < int(GeoList.size())-extGeoCount; i++)
        intGeoList.push_back(GeoList[i]);
    for (int i=int(GeoList.size())-extGeoCount; i < int(GeoList.size()); i++)
        extGeoList.push_back(GeoList[i]);

    addGeometry(intGeoList);
    int extStart=Geoms.size();
    addGeometry(extGeoList, true);
    int extEnd=Geoms.size()-1;
    for (int i=extStart; i <= extEnd; i++)
        Geoms[i].external = true;

    // The Geoms list might be empty after an undo/redo
    if (!Geoms.empty())
        addConstraints(ConstraintList);

    GCSsys.clearByTag(-1);
    GCSsys.declareUnknowns(Parameters);
    GCSsys.initSolution();
    GCSsys.getConflicting(Conflicting);
    GCSsys.getRedundant(Redundant);
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
    case Sketch::None:
    default:
        return "unknown";
    }
}

// Geometry adding ==========================================================

int Sketch::addGeometry(const Part::Geometry *geo, bool fixed)
{
    if (geo->getTypeId() == GeomPoint::getClassTypeId()) { // add a point
        const GeomPoint *point = dynamic_cast<const GeomPoint*>(geo);
        // create the definition struct for that geom
        return addPoint(*point, fixed);
    } else if (geo->getTypeId() == GeomLineSegment::getClassTypeId()) { // add a line
        const GeomLineSegment *lineSeg = dynamic_cast<const GeomLineSegment*>(geo);
        // create the definition struct for that geom
        return addLineSegment(*lineSeg, fixed);
    } else if (geo->getTypeId() == GeomCircle::getClassTypeId()) { // add a circle
        const GeomCircle *circle = dynamic_cast<const GeomCircle*>(geo);
        // create the definition struct for that geom
        return addCircle(*circle, fixed);
    } else if (geo->getTypeId() == GeomArcOfCircle::getClassTypeId()) { // add an arc
        const GeomArcOfCircle *aoc = dynamic_cast<const GeomArcOfCircle*>(geo);
        // create the definition struct for that geom
        return addArc(*aoc, fixed);
    } else {
        Base::Exception("Sketch::addGeometry(): Unknown or unsupported type added to a sketch");
        return 0;
    }
}

void Sketch::addGeometry(const std::vector<Part::Geometry *> &geo, bool fixed)
{
    for (std::vector<Part::Geometry *>::const_iterator it=geo.begin(); it != geo.end(); ++it)
        addGeometry(*it, fixed);
}

int Sketch::addPoint(const Part::GeomPoint &point, bool fixed)
{
    std::vector<double *> &params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomPoint *p = static_cast<GeomPoint*>(point.clone());
    // points in a sketch are always construction elements
    p->Construction = true;
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

int Sketch::addLine(const Part::GeomLineSegment &line, bool fixed)
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
    Base::Vector3d startPnt = aoc->getStartPoint();
    Base::Vector3d endPnt   = aoc->getEndPoint();
    double radius           = aoc->getRadius();
    double startAngle, endAngle;
    aoc->getRange(startAngle, endAngle);

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

int Sketch::addEllipse(const Part::GeomEllipse &ellipse, bool fixed)
{
    // return the position of the newly added geometry
    return Geoms.size()-1;
}

std::vector<Part::Geometry *> Sketch::extractGeometry(bool withConstrucionElements,
                                                      bool withExternalElements) const
{
    std::vector<Part::Geometry *> temp;
    temp.reserve(Geoms.size());
    for (std::vector<GeoDef>::const_iterator it=Geoms.begin(); it != Geoms.end(); ++it)
        if ((!it->external || withExternalElements) && (!it->geo->Construction || withConstrucionElements))
            temp.push_back(it->geo->clone());

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
            GeomLineSegment *lineSeg = dynamic_cast<GeomLineSegment*>(it->geo->clone());
            tuple[i] = Py::asObject(new LinePy(lineSeg));
        } else if (it->type == Arc) {
            GeomArcOfCircle *aoc = dynamic_cast<GeomArcOfCircle*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfCirclePy(aoc));
        } else if (it->type == Circle) {
            GeomCircle *circle = dynamic_cast<GeomCircle*>(it->geo->clone());
            tuple[i] = Py::asObject(new CirclePy(circle));
        } else if (it->type == Ellipse) {
            GeomEllipse *ellipse = dynamic_cast<GeomEllipse*>(it->geo->clone());
            tuple[i] = Py::asObject(new EllipsePy(ellipse));
        } else {
            // not implemented type in the sketch!
        }
    }
    return tuple;
}

int Sketch::checkGeoId(int geoId)
{
    if (geoId < 0)
        geoId += Geoms.size();
    assert(geoId >= 0 && geoId < int(Geoms.size()));
    return geoId;
}

// constraint adding ==========================================================

int Sketch::addConstraint(const Constraint *constraint)
{
    // constraints on nothing makes no sense
    assert(int(Geoms.size()) > 0);
    int rtn = -1;
    switch (constraint->Type) {
    case DistanceX:
        if (constraint->FirstPos == none) // horizontal length of a line
            rtn = addDistanceXConstraint(constraint->First,constraint->Value);
        else if (constraint->Second == Constraint::GeoUndef) // point on fixed x-coordinate
            rtn = addCoordinateXConstraint(constraint->First,constraint->FirstPos,constraint->Value);
        else if (constraint->SecondPos != none) // point to point horizontal distance
            rtn = addDistanceXConstraint(constraint->First,constraint->FirstPos,
                                         constraint->Second,constraint->SecondPos,constraint->Value);
        break;
    case DistanceY:
        if (constraint->FirstPos == none) // vertical length of a line
            rtn = addDistanceYConstraint(constraint->First,constraint->Value);
        else if (constraint->Second == Constraint::GeoUndef) // point on fixed y-coordinate
            rtn = addCoordinateYConstraint(constraint->First,constraint->FirstPos,constraint->Value);
        else if (constraint->SecondPos != none) // point to point vertical distance
            rtn = addDistanceYConstraint(constraint->First,constraint->FirstPos,
                                         constraint->Second,constraint->SecondPos,constraint->Value);
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
        if (constraint->SecondPos != none) // perpendicularity at common point
            rtn = addPerpendicularConstraint(constraint->First,constraint->FirstPos,
                                             constraint->Second,constraint->SecondPos);
        else if (constraint->Second != Constraint::GeoUndef) {
            if (constraint->FirstPos != none) // "First" is a connecting point
                rtn = addPerpendicularConstraint(constraint->First,constraint->FirstPos,
                                                 constraint->Second);
            else // simple perpendicularity
                rtn = addPerpendicularConstraint(constraint->First,constraint->Second);
        }
        break;
    case Tangent:
        if (constraint->SecondPos != none) // tangency at common point
            rtn = addTangentConstraint(constraint->First,constraint->FirstPos,
                                       constraint->Second,constraint->SecondPos);
        else if (constraint->Second != Constraint::GeoUndef) {
            if (constraint->FirstPos != none) // "First" is a tangency point
                rtn = addTangentConstraint(constraint->First,constraint->FirstPos,
                                           constraint->Second);
            else // simple tangency
                rtn = addTangentConstraint(constraint->First,constraint->Second);
        }
        break;
    case Distance:
        if (constraint->SecondPos != none) // point to point distance
            rtn = addDistanceConstraint(constraint->First,constraint->FirstPos,
                                        constraint->Second,constraint->SecondPos,
                                        constraint->Value);
        else if (constraint->Second != Constraint::GeoUndef) {
            if (constraint->FirstPos != none) // point to line distance
                rtn = addDistanceConstraint(constraint->First,constraint->FirstPos,
                                            constraint->Second,constraint->Value);
            else // line to line distance (not implemented yet)
                rtn = addDistanceConstraint(constraint->First,constraint->Second,constraint->Value);
        }
        else // line length
            rtn = addDistanceConstraint(constraint->First,constraint->Value);
        break;
    case Angle:
        if (constraint->SecondPos != none) // angle between two lines (with explicit start points)
            rtn = addAngleConstraint(constraint->First,constraint->FirstPos,
                                     constraint->Second,constraint->SecondPos,constraint->Value);
        else if (constraint->Second != Constraint::GeoUndef) // angle between two lines
            rtn = addAngleConstraint(constraint->First,constraint->Second,constraint->Value);
        else if (constraint->First != Constraint::GeoUndef) // orientation angle of a line
            rtn = addAngleConstraint(constraint->First,constraint->Value);
        break;
    case Radius:
        rtn = addRadiusConstraint(constraint->First, constraint->Value);
        break;
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
    case None:
        break;
    }
    return rtn;
}

int Sketch::addConstraints(const std::vector<Constraint *> &ConstraintList)
{
    // constraints on nothing makes no sense
    assert(!Geoms.empty() || ConstraintList.empty());

    int rtn = -1;
    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it)
        rtn = addConstraint (*it);

    return rtn;
}

int Sketch::addCoordinateXConstraint(int geoId, PointPos pos, double value)
{
    geoId = checkGeoId(geoId);

    int pointId = getPointId(geoId, pos);

    if (pointId >= 0 && pointId < int(Points.size())) {
        double *val = new double(value);
        FixParameters.push_back(val);
        GCS::Point &p = Points[pointId];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCoordinateX(p, val, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addCoordinateYConstraint(int geoId, PointPos pos, double value)
{
    geoId = checkGeoId(geoId);

    int pointId = getPointId(geoId, pos);

    if (pointId >= 0 && pointId < int(Points.size())) {
        double *val = new double(value);
        FixParameters.push_back(val);
        GCS::Point &p = Points[pointId];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCoordinateY(p, val, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addDistanceXConstraint(int geoId, double value)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];

    FixParameters.push_back(new double(value));
    double *diff = FixParameters[FixParameters.size()-1];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintDifference(l.p1.x, l.p2.x, diff, tag);
    return ConstraintsCounter;
}

int Sketch::addDistanceYConstraint(int geoId, double value)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];

    FixParameters.push_back(new double(value));
    double *diff = FixParameters[FixParameters.size()-1];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintDifference(l.p1.y, l.p2.y, diff, tag);
    return ConstraintsCounter;
}

int Sketch::addDistanceXConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, double value)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        FixParameters.push_back(new double(value));
        double *difference = FixParameters[FixParameters.size()-1];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintDifference(p1.x, p2.x, difference, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addDistanceYConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, double value)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        FixParameters.push_back(new double(value));
        double *difference = FixParameters[FixParameters.size()-1];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintDifference(p1.y, p2.y, difference, tag);
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

// perpendicularity at specific point constraint
int Sketch::addPerpendicularConstraint(int geoId1, PointPos pos1, int geoId2)
{
    // accepts the following combinations:
    // 1) Line1, start/end, Line2/Circle2/Arc2
    // 2) Arc1, start/end, Line2/Circle2/Arc2
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);

    if (pointId1 < 0 || pointId1 >= int(Points.size()))
        return addPerpendicularConstraint(geoId1, geoId2);

    GCS::Point &p1 = Points[pointId1];
    if (Geoms[geoId1].type == Line) {
        GCS::Line &l1 = Lines[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Line) {
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p1, l2, tag);
            GCSsys.addConstraintPerpendicular(l1, l2, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            GCS::Point &p2 = Points[Geoms[geoId2].midPointId];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnArc(p1, a2, tag);
            GCSsys.addConstraintPointOnLine(p2, l1, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c2 = Circles[Geoms[geoId2].index];
            GCS::Point &p2 = Points[Geoms[geoId2].midPointId];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnCircle(p1, c2, tag);
            GCSsys.addConstraintPointOnLine(p2, l1, tag);
            return ConstraintsCounter;
        }
    }
    else if (Geoms[geoId1].type == Arc) {
        GCS::Arc &a1 = Arcs[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Line) {
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p1, l2, tag);
            GCSsys.addConstraintPointOnLine(a1.center, l2, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Arc || Geoms[geoId2].type == Circle) {
            int tag = ++ConstraintsCounter;
            GCS::Point &center = Points[Geoms[geoId2].midPointId];
            double *radius;
            if (Geoms[geoId2].type == Arc) {
                GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
                radius = a2.rad;
            }
            else {
                GCS::Circle &c2 = Circles[Geoms[geoId2].index];
                radius = c2.rad;
            }
            if (pos1 == start)
                GCSsys.addConstraintPerpendicularCircle2Arc(center, radius, a1, tag);
            else if (pos1 == end)
                GCSsys.addConstraintPerpendicularArc2Circle(a1, center, radius, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}

// perpendicularity at common point constraint
int Sketch::addPerpendicularConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    // accepts the following combinations:
    // 1) Line1, start/end, Line2/Arc2, start/end
    // 2) Arc1, start/end, Line2, start/end (converted to case #1)
    // 3) Arc1, start/end, Arc2, start/end
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 < 0 || pointId1 >= int(Points.size()) ||
        pointId2 < 0 || pointId2 >= int(Points.size()))
        return -1;

    GCS::Point &p1 = Points[pointId1];
    GCS::Point &p2 = Points[pointId2];
    if (Geoms[geoId2].type == Line) {
        if (Geoms[geoId1].type == Line) {
            GCS::Line &l1 = Lines[Geoms[geoId1].index];
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintP2PCoincident(p1, p2, tag);
            GCSsys.addConstraintPerpendicular(l1, l2, tag);
            return ConstraintsCounter;
        }
        else {
            std::swap(geoId1, geoId2);
            std::swap(pos1, pos2);
            std::swap(pointId1, pointId2);
            p1 = Points[pointId1];
            p2 = Points[pointId2];
        }
    }

    if (Geoms[geoId1].type == Line) {
        GCS::Line &l1 = Lines[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            if (pos2 == start) {
                if (pos1 == start) {
                    int tag = ++ConstraintsCounter;
                    GCSsys.addConstraintPerpendicularLine2Arc(l1.p2, l1.p1, a2, tag);
                    return ConstraintsCounter;
                }
                else if (pos1 == end) {
                    int tag = ++ConstraintsCounter;
                    GCSsys.addConstraintPerpendicularLine2Arc(l1.p1, l1.p2, a2, tag);
                    return ConstraintsCounter;
                }
            }
            else if (pos2 == end) {
                if (pos1 == start) {
                    int tag = ++ConstraintsCounter;
                    GCSsys.addConstraintPerpendicularArc2Line(a2, l1.p1, l1.p2, tag);
                    return ConstraintsCounter;
                }
                else if (pos1 == end) {
                    int tag = ++ConstraintsCounter;
                    GCSsys.addConstraintPerpendicularArc2Line(a2, l1.p2, l1.p1, tag);
                    return ConstraintsCounter;
                }
            }
            else
                return -1;
        }
    }
    else if (Geoms[geoId1].type == Arc) {
        GCS::Arc &a1 = Arcs[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            if (pos1 == start && (pos2 == start || pos2 == end)) {
                int tag = ++ConstraintsCounter;
                if (pos2 == start)
                    GCSsys.addConstraintPerpendicularArc2Arc(a1, true, a2, false, tag);
                else // if (pos2 == end)
                    GCSsys.addConstraintPerpendicularArc2Arc(a1, true, a2, true, tag);
                    // GCSsys.addConstraintTangentArc2Arc(a2, false, a1, false, tag);
                return ConstraintsCounter;
            }
            else if (pos1 == end && (pos2 == start || pos2 == end)) {
                int tag = ++ConstraintsCounter;
                if (pos2 == start)
                    GCSsys.addConstraintPerpendicularArc2Arc(a1, false, a2, false, tag);
                else // if (pos2 == end)
                    GCSsys.addConstraintPerpendicularArc2Arc(a1, false, a2, true, tag);
                return ConstraintsCounter;
            }
        }
    }
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
        }
    } else if (Geoms[geoId1].type == Circle) {
        GCS::Circle &c = Circles[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c2 = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, c2, tag);
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, a, tag);
            return ConstraintsCounter;
        }
    } else if (Geoms[geoId1].type == Arc) {
        GCS::Arc &a = Arcs[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, a, tag);
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(a, a2, tag);
            return ConstraintsCounter;
        }
    }

    return -1;
}

// tangency at specific point constraint
int Sketch::addTangentConstraint(int geoId1, PointPos pos1, int geoId2)
{
    // accepts the following combinations:
    // 1) Line1, start/end, Line2/Circle2/Arc2
    // 2) Arc1, start/end, Line2/Circle2/Arc2
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);

    if (pointId1 < 0 || pointId1 >= int(Points.size()))
        return addTangentConstraint(geoId1, geoId2);

    GCS::Point &p1 = Points[pointId1];
    if (Geoms[geoId1].type == Line) {
        GCS::Line &l1 = Lines[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Line) {
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p1, l2, tag);
            GCSsys.addConstraintParallel(l1, l2, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnArc(p1, a2, tag);
            GCSsys.addConstraintTangent(l1, a2, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c2 = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnCircle(p1, c2, tag);
            GCSsys.addConstraintTangent(l1, c2, tag);
            return ConstraintsCounter;
        }
    }
    else if (Geoms[geoId1].type == Arc) {
        GCS::Arc &a1 = Arcs[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Line) {
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p1, l2, tag);
            GCSsys.addConstraintTangent(l2, a1, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnArc(p1, a2, tag);
            GCSsys.addConstraintTangent(a1, a2, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c2 = Circles[Geoms[geoId2].index];
            if (pos1 == start) {
                int tag = ++ConstraintsCounter;
                GCSsys.addConstraintTangentCircle2Arc(c2, a1, tag);
                return ConstraintsCounter;
            }
            else if (pos1 == end) {
                int tag = ++ConstraintsCounter;
                GCSsys.addConstraintTangentArc2Circle(a1, c2, tag);
                return ConstraintsCounter;
            }
        }
    }
    return -1;
}

// tangency at common point constraint
int Sketch::addTangentConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    // accepts the following combinations:
    // 1) Line1, start/end, Line2/Arc2, start/end
    // 2) Arc1, start/end, Line2, start/end (converted to case #1)
    // 3) Arc1, start/end, Arc2, start/end
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 < 0 || pointId1 >= int(Points.size()) ||
        pointId2 < 0 || pointId2 >= int(Points.size()))
        return -1;

    GCS::Point &p1 = Points[pointId1];
    GCS::Point &p2 = Points[pointId2];
    if (Geoms[geoId2].type == Line) {
        if (Geoms[geoId1].type == Line) {
            GCS::Line &l1 = Lines[Geoms[geoId1].index];
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintP2PCoincident(p1, p2, tag);
            GCSsys.addConstraintParallel(l1, l2, tag);
            return ConstraintsCounter;
        }
        else {
            std::swap(geoId1, geoId2);
            std::swap(pos1, pos2);
            std::swap(pointId1, pointId2);
            p1 = Points[pointId1];
            p2 = Points[pointId2];
        }
    }

    if (Geoms[geoId1].type == Line) {
        GCS::Line &l1 = Lines[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            if (pos2 == start) {
                if (pos1 == start) {
                    int tag = ++ConstraintsCounter;
                    GCSsys.addConstraintTangentLine2Arc(l1.p2, l1.p1, a2, tag);
                    return ConstraintsCounter;
                }
                else if (pos1 == end) {
                    int tag = ++ConstraintsCounter;
                    GCSsys.addConstraintTangentLine2Arc(l1.p1, l1.p2, a2, tag);
                    return ConstraintsCounter;
                }
            }
            else if (pos2 == end) {
                if (pos1 == start) {
                    int tag = ++ConstraintsCounter;
                    GCSsys.addConstraintTangentArc2Line(a2, l1.p1, l1.p2, tag);
                    return ConstraintsCounter;
                }
                else if (pos1 == end) {
                    int tag = ++ConstraintsCounter;
                    GCSsys.addConstraintTangentArc2Line(a2, l1.p2, l1.p1, tag);
                    return ConstraintsCounter;
                }
            }
            else
                return -1;
        }
    }
    else if (Geoms[geoId1].type == Arc) {
        GCS::Arc &a1 = Arcs[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a2 = Arcs[Geoms[geoId2].index];
            if (pos1 == start && (pos2 == start || pos2 == end)) {
                int tag = ++ConstraintsCounter;
                if (pos2 == start)
                    GCSsys.addConstraintTangentArc2Arc(a1, true, a2, false, tag);
                else // if (pos2 == end)
                    GCSsys.addConstraintTangentArc2Arc(a1, true, a2, true, tag);
                    // GCSsys.addConstraintTangentArc2Arc(a2, false, a1, false, tag);
                return ConstraintsCounter;
            }
            else if (pos1 == end && (pos2 == start || pos2 == end)) {
                int tag = ++ConstraintsCounter;
                if (pos2 == start)
                    GCSsys.addConstraintTangentArc2Arc(a1, false, a2, false, tag);
                else // if (pos2 == end)
                    GCSsys.addConstraintTangentArc2Arc(a1, false, a2, true, tag);
                return ConstraintsCounter;
            }
        }
    }
    return -1;
}

// line length constraint
int Sketch::addDistanceConstraint(int geoId, double value)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];

    // add the parameter for the length
    FixParameters.push_back(new double(value));
    double *distance = FixParameters[FixParameters.size()-1];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintP2PDistance(l.p1, l.p2, distance, tag);
    return ConstraintsCounter;
}

// line to line distance constraint
int Sketch::addDistanceConstraint(int geoId1, int geoId2, double value)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    //assert(Geoms[geoId1].type == Line);
    //assert(Geoms[geoId2].type == Line);

    Base::Console().Warning("Line to line distance constraints are not implemented yet.\n");

    return -1;
}

// point to line distance constraint
int Sketch::addDistanceConstraint(int geoId1, PointPos pos1, int geoId2, double value)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);

    if (Geoms[geoId2].type != Line)
        return -1;

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Line &l2 = Lines[Geoms[geoId2].index];

        // add the parameter for the distance
        FixParameters.push_back(new double(value));
        double *distance = FixParameters[FixParameters.size()-1];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2LDistance(p1, l2, distance, tag);
        return ConstraintsCounter;
    }
    return -1;
}

// point to point distance constraint
int Sketch::addDistanceConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, double value)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) &&
        pointId2 >= 0 && pointId2 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];
        GCS::Point &p2 = Points[pointId2];

        // add the parameter for the distance
        FixParameters.push_back(new double(value));
        double *distance = FixParameters[FixParameters.size()-1];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PDistance(p1, p2, distance, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addRadiusConstraint(int geoId, double value)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type == Circle) {
        GCS::Circle &c = Circles[Geoms[geoId].index];
        // add the parameter for the radius
        FixParameters.push_back(new double(value));
        double *radius = FixParameters[FixParameters.size()-1];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCircleRadius(c, radius, tag);
        return ConstraintsCounter;
    }
    else if (Geoms[geoId].type == Arc) {
        GCS::Arc &a = Arcs[Geoms[geoId].index];
        // add the parameter for the radius
        FixParameters.push_back(new double(value));
        double *radius = FixParameters[FixParameters.size()-1];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintArcRadius(a, radius, tag);
        return ConstraintsCounter;
    }
    return -1;
}

// line orientation angle constraint
int Sketch::addAngleConstraint(int geoId, double value)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line)
        return -1;

    GCS::Line &l = Lines[Geoms[geoId].index];

    // add the parameter for the angle
    FixParameters.push_back(new double(value));
    double *angle = FixParameters[FixParameters.size()-1];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintP2PAngle(l.p1, l.p2, angle, tag);
    return ConstraintsCounter;
}

// line to line angle constraint
int Sketch::addAngleConstraint(int geoId1, int geoId2, double value)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Line ||
        Geoms[geoId2].type != Line)
        return -1;

    GCS::Line &l1 = Lines[Geoms[geoId1].index];
    GCS::Line &l2 = Lines[Geoms[geoId2].index];

    // add the parameter for the angle
    FixParameters.push_back(new double(value));
    double *angle = FixParameters[FixParameters.size()-1];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintL2LAngle(l1, l2, angle, tag);
    return ConstraintsCounter;
}

// line to line angle constraint (with explicitly given start points)
int Sketch::addAngleConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, double value)
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

    // add the parameter for the angle
    FixParameters.push_back(new double(value));
    double *angle = FixParameters[FixParameters.size()-1];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintL2LAngle(*l1p1, *l1p2, *l2p1, *l2p2, angle, tag);
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

    Base::Console().Warning("Equality constraints between %s and %s are not supported.\n",
                            nameByType(Geoms[geoId1].type), nameByType(Geoms[geoId2].type));
    return -1;
}

// point on object constraint
int Sketch::addPointOnObjectConstraint(int geoId1, PointPos pos1, int geoId2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point &p1 = Points[pointId1];

        if (Geoms[geoId2].type == Line) {
            GCS::Line &l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p1, l2, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc &a = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnArc(p1, a, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Circle) {
            GCS::Circle &c = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnCircle(p1, c, tag);
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

bool Sketch::updateGeometry()
{
    int i=0;
    for (std::vector<GeoDef>::const_iterator it=Geoms.begin(); it != Geoms.end(); ++it, i++) {
        try {
            if (it->type == Point) {
                GeomPoint *point = dynamic_cast<GeomPoint*>(it->geo);
                point->setPoint(Vector3d(*Points[it->startPointId].x,
                                         *Points[it->startPointId].y,
                                         0.0)
                               );
            } else if (it->type == Line) {
                GeomLineSegment *lineSeg = dynamic_cast<GeomLineSegment*>(it->geo);
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
                GeomArcOfCircle *aoc = dynamic_cast<GeomArcOfCircle*>(it->geo);
                aoc->setCenter(Vector3d(*Points[it->midPointId].x,
                                        *Points[it->midPointId].y,
                                        0.0)
                              );
                aoc->setRadius(*myArc.rad);
                aoc->setRange(*myArc.startAngle, *myArc.endAngle);
            } else if (it->type == Circle) {
                GeomCircle *circ = dynamic_cast<GeomCircle*>(it->geo);
                circ->setCenter(Vector3d(*Points[it->midPointId].x,
                                         *Points[it->midPointId].y,
                                         0.0)
                               );
                circ->setRadius(*Circles[it->index].rad);
            }
        } catch (Base::Exception e) {
            Base::Console().Error("Updating geometry: Error build geometry(%d): %s\n",
                                  i,e.what());
            return false;
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

    int ret;
    bool valid_solution;
    for (int soltype=0; soltype < (isInitMove ? 1 : 4); soltype++) {
        std::string solvername;
        switch (soltype) {
        case 0: // solving with the default DogLeg solver
                // (or with SQP if we are in moving mode)
            solvername = isInitMove ? "SQP" : "DogLeg";
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
            }
        } else {
            valid_solution = false;
            //Base::Console().Log("NotSolved ");
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

    Base::TimeInfo end_time;
    //Base::Console().Log("T:%s\n",Base::TimeInfo::diffTime(start_time,end_time).c_str());
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

int Sketch::movePoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative)
{
    geoId = checkGeoId(geoId);

    // don't try to move sketches that contain conflicting constraints
    if (hasConflicts())
        return -1;

    if (!isInitMove)
        initMove(geoId, pos);

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
    }

    return solve();
}

int Sketch::setDatum(int constrId, double value)
{
    return -1;
}

int Sketch::getPointId(int geoId, PointPos pos) const
{
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

Base::Vector3d Sketch::getPoint(int geoId, PointPos pos)
{
    geoId = checkGeoId(geoId);
    int pointId = getPointId(geoId, pos);
    if (pointId != -1)
        return Base::Vector3d(*Points[pointId].x, *Points[pointId].y, 0);

    return Base::Vector3d();
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
                result._Shape = sh;
            } else {
                result._Shape = result.fuse(sh);
            }
        }
    }
    return result;
#else
    std::list<TopoDS_Edge> edge_list;
    std::list<TopoDS_Wire> wires;

    // collecting all (non constructive and non external) edges out of the sketch
    for (;it!=Geoms.end();++it) {
        if (!it->external && !it->geo->Construction) {
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

        // try to connect each edge to the wire, the wire is complete if no more egdes are connectible
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
        // generate a face with holes (inner wires have to be taged REVERSE or INNER).
        // thats the only way to transport a somwhat more complex sketch...
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
        result._Shape = comp;
    }
    // FIXME: if free edges are left over its probably better to
    // create a compound with the closed structures and let the
    // features decide what to do with it...
    if (edge_list.size() > 0)
        Base::Console().Warning("Left over edges in Sketch. Only closed structures will be propagated at the moment!\n");

#endif

    return result;
}

// Persistance implementer -------------------------------------------------

unsigned int Sketch::getMemSize(void) const
{
    return 0;
}

void Sketch::Save(Writer &writer) const
{

}

void Sketch::Restore(XMLReader &reader)
{

}

