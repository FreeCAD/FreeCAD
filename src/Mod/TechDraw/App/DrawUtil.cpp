/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
#include <cstdlib>
#include <cstring>
#include <sstream>

#include <boost_regex.hpp>

#include <QChar>
#include <QPointF>
#include <QString>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#endif

#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/Vector3D.h>

#include "DrawUtil.h"
#include "GeometryObject.h"
#include "LineGroup.h"
#include "Preferences.h"
#include "DrawViewPart.h"


using namespace TechDraw;

/*static*/ int DrawUtil::getIndexFromName(const std::string& geomName)
{
    //   Base::Console().Message("DU::getIndexFromName(%s)\n", geomName.c_str());
    boost::regex re("\\d+$");// one of more digits at end of string
    boost::match_results<std::string::const_iterator> what;
    boost::match_flag_type flags = boost::match_default;
    //   char* endChar;
    std::string::const_iterator begin = geomName.begin();
    auto pos = geomName.rfind('.');
    if (pos != std::string::npos) {
        begin += pos + 1;
    }
    std::string::const_iterator end = geomName.end();
    std::stringstream ErrorMsg;

    if (geomName.empty()) {
        throw Base::ValueError("getIndexFromName - empty geometry name");
    }


    if (boost::regex_search(begin, end, what, re, flags)) {
        return int(std::stoi(what.str()));
    } else {
        ErrorMsg << "getIndexFromName: malformed geometry name - " << geomName;
        throw Base::ValueError(ErrorMsg.str());
    }
}

std::string DrawUtil::getGeomTypeFromName(const std::string& geomName)
{
    if (geomName.empty()) {
        throw Base::ValueError("getGeomTypeFromName - empty geometry name");
    }

    boost::regex re("^[a-zA-Z]*");//one or more letters at start of string
    boost::match_results<std::string::const_iterator> what;
    boost::match_flag_type flags = boost::match_default;
    std::string::const_iterator begin = geomName.begin();
    auto pos = geomName.rfind('.');
    if (pos != std::string::npos) {
        begin += pos + 1;
    }
    std::string::const_iterator end = geomName.end();
    std::stringstream ErrorMsg;

    if (boost::regex_search(begin, end, what, re, flags)) {
        return what.str();//TODO: use std::stoi() in c++11
    } else {
        ErrorMsg << "In getGeomTypeFromName: malformed geometry name - " << geomName;
        throw Base::ValueError(ErrorMsg.str());
    }
}

std::string DrawUtil::makeGeomName(const std::string& geomType, int index)
{
    std::stringstream newName;
    newName << geomType << index;
    return newName.str();
}

//! true if v1 and v2 are the same geometric point within tolerance
bool DrawUtil::isSamePoint(TopoDS_Vertex v1, TopoDS_Vertex v2, double tolerance)
{
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);
    return p1.IsEqual(p2, tolerance);
}

bool DrawUtil::isZeroEdge(TopoDS_Edge e, double tolerance)
{
    TopoDS_Vertex vStart = TopExp::FirstVertex(e);
    TopoDS_Vertex vEnd = TopExp::LastVertex(e);
    if (!isSamePoint(vStart, vEnd, tolerance)) {
        return false;
    }

    //closed edge will have same V's but non-zero length
    BRepAdaptor_Curve adapt(e);
    double len = GCPnts_AbscissaPoint::Length(adapt, Precision::Confusion());
    if (len > tolerance) {
        return false;
    }

    return true;
}
double DrawUtil::simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2)
{
    BRepExtrema_DistShapeShape extss(s1, s2);
    if (!extss.IsDone()) {
        Base::Console().Message("DU::simpleMinDist - BRepExtrema_DistShapeShape failed");
        return -1;
    }
    int count = extss.NbSolution();
    if (count != 0) {
        return extss.Value();
    } else {
        return -1;
    }
}

//! assumes 2d on XY
//! quick angle for straight edges
double DrawUtil::angleWithX(TopoDS_Edge e, bool reverse)
{
    gp_Pnt gstart = BRep_Tool::Pnt(TopExp::FirstVertex(e));
    Base::Vector3d start(gstart.X(), gstart.Y(), gstart.Z());
    gp_Pnt gend = BRep_Tool::Pnt(TopExp::LastVertex(e));
    Base::Vector3d end(gend.X(), gend.Y(), gend.Z());
    Base::Vector3d u;
    if (reverse) {
        u = start - end;
    } else {
        u = end - start;
    }
    double result = atan2(u.y, u.x);
    if (result < 0) {
        result += 2.0 * M_PI;
    }

    return result;
}

//! find angle of edge with x-Axis at First/LastVertex
double DrawUtil::angleWithX(TopoDS_Edge e, TopoDS_Vertex v, double tolerance)
{
    double param = 0;

    BRepAdaptor_Curve adapt(e);
    if (isFirstVert(e, v, tolerance)) {
        param = adapt.FirstParameter();
    } else if (isLastVert(e, v, tolerance)) {
        param = adapt.LastParameter();
    } else {
        //TARFU
        Base::Console().Message("Error: DU::angleWithX - v is neither first nor last \n");
    }
    gp_Pnt paramPoint;
    gp_Vec derivative;
    const Handle(Geom_Curve) c = adapt.Curve().Curve();
    c->D1(param, paramPoint, derivative);
    double angle = atan2(derivative.Y(), derivative.X());
    if (angle < 0) {//map from [-PI:PI] to [0:2PI]
        angle += 2.0 * M_PI;
    }
    return angle;
}

//! find angle of incidence at First/LastVertex
double DrawUtil::incidenceAngleAtVertex(TopoDS_Edge e, TopoDS_Vertex v, double tolerance)
{
    double incidenceAngle = 0;

    BRepAdaptor_Curve adapt(e);
    double paramRange = adapt.LastParameter() - adapt.FirstParameter();
    double paramOffset = paramRange / 100.0;
    double vertexParam;
    Base::Vector3d anglePoint = DrawUtil::vertex2Vector(v);
    Base::Vector3d offsetPoint, incidenceVec;
    int noTangents = 0;
    if (isFirstVert(e, v, tolerance)) {
        vertexParam = adapt.FirstParameter();
        BRepLProp_CLProps prop(
            adapt, vertexParam + paramOffset, noTangents, Precision::Confusion());
        const gp_Pnt& gOffsetPoint = prop.Value();
        offsetPoint = Base::Vector3d(gOffsetPoint.X(), gOffsetPoint.Y(), gOffsetPoint.Z());
    } else if (isLastVert(e, v, tolerance)) {
        vertexParam = adapt.LastParameter();
        BRepLProp_CLProps prop(
            adapt, vertexParam - paramOffset, noTangents, Precision::Confusion());
        const gp_Pnt& gOffsetPoint = prop.Value();
        offsetPoint = Base::Vector3d(gOffsetPoint.X(), gOffsetPoint.Y(), gOffsetPoint.Z());
    } else {
        //TARFU
        //        Base::Console().Message("DU::incidenceAngle - v is neither first nor last \n");
    }
    incidenceVec = anglePoint - offsetPoint;
    incidenceAngle = atan2(incidenceVec.y, incidenceVec.x);

    //map to [0:2PI]
    if (incidenceAngle < 0.0) {
        incidenceAngle = M_2PI + incidenceAngle;
    }

    return incidenceAngle;
}

bool DrawUtil::isFirstVert(TopoDS_Edge e, TopoDS_Vertex v, double tolerance)
{
    TopoDS_Vertex first = TopExp::FirstVertex(e);
    return isSamePoint(first, v, tolerance);
}

bool DrawUtil::isLastVert(TopoDS_Edge e, TopoDS_Vertex v, double tolerance)
{
    TopoDS_Vertex last = TopExp::LastVertex(e);
    return isSamePoint(last, v, tolerance);
}

bool DrawUtil::fpCompare(const double& d1, const double& d2, double tolerance)
{
    return std::fabs(d1 - d2) < tolerance;
}

//brute force intersection points of line(point, dir) with box(xRange, yRange)
std::pair<Base::Vector3d, Base::Vector3d>
DrawUtil::boxIntersect2d(Base::Vector3d point, Base::Vector3d dirIn, double xRange, double yRange)
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d p1, p2;
    Base::Vector3d dir = dirIn;
    dir.Normalize();
    // y = mx + b
    // m = (y1 - y0) / (x1 - x0)
    if (DrawUtil::fpCompare(dir.x, 0.0)) {//vertical case
        p1 = Base::Vector3d(point.x, point.y - (yRange / 2.0), 0.0);
        p2 = Base::Vector3d(point.x, point.y + (yRange / 2.0), 0.0);
    } else {
        double slope = dir.y / dir.x;
        double left = -xRange / 2.0;
        double right = xRange / 2.0;
        if (DrawUtil::fpCompare(slope, 0.0)) {//horizontal case
            p1 = Base::Vector3d(point.x - (xRange / 2.0), point.y);
            p2 = Base::Vector3d(point.x + (xRange / 2.0), point.y);
        } else {//normal case
            double top = yRange / 2.0;
            double bottom = -yRange / 2.0;
            double yLeft = point.y - slope * (point.x - left);
            double yRight = point.y - slope * (point.x - right);
            double xTop = point.x - ((point.y - top) / slope);
            double xBottom = point.x - ((point.y - bottom) / slope);

            if ((bottom < yLeft) && (top > yLeft)) {
                p1 = Base::Vector3d(left, yLeft);
            } else if (yLeft <= bottom) {
                p1 = Base::Vector3d(xBottom, bottom);
            } else if (yLeft >= top) {
                p1 = Base::Vector3d(xTop, top);
            }

            if ((bottom < yRight) && (top > yRight)) {
                p2 = Base::Vector3d(right, yRight);
            } else if (yRight <= bottom) {
                p2 = Base::Vector3d(xBottom, bottom);
            } else if (yRight >= top) {
                p2 = Base::Vector3d(xTop, top);
            }
        }
    }
    result.first = p1;
    result.second = p2;
    Base::Vector3d dirCheck = p2 - p1;
    dirCheck.Normalize();
    if (!dir.IsEqual(dirCheck, 0.00001)) {
        result.first = p2;
        result.second = p1;
    }

    return result;
}

//find the apparent intersection of 2 3d curves. We are only interested in curves that are lines, so we will have either 0 or 1
//apparent intersection.  The intersection is "apparent" because the curve's progenator is a trimmed curve (line segment)
//NOTE: these curves do not have a location, so the intersection point does not respect the
//placement of the edges which spawned the curves.  Use the apparentIntersection(edge, edge) method instead.
bool DrawUtil::apparentIntersection(const Handle(Geom_Curve) curve1,
                                    const Handle(Geom_Curve) curve2, Base::Vector3d& result)
{
    GeomAPI_ExtremaCurveCurve intersector(curve1, curve2);
    if (intersector.NbExtrema() == 0 || intersector.LowerDistance() > EWTOLERANCE) {
        //no intersection
        return false;
    }
    //for our purposes, only one intersection point is required.
    gp_Pnt p1, p2;
    intersector.Points(1, p1, p2);
    result = toVector3d(p1);
    return true;
}

bool DrawUtil::apparentIntersection(TopoDS_Edge& edge0, TopoDS_Edge& edge1, gp_Pnt& intersect)
{
    gp_Pnt gStart0 = BRep_Tool::Pnt(TopExp::FirstVertex(edge0));
    gp_Pnt gEnd0 = BRep_Tool::Pnt(TopExp::LastVertex(edge0));
    gp_Pnt gStart1 = BRep_Tool::Pnt(TopExp::FirstVertex(edge1));
    gp_Pnt gEnd1 = BRep_Tool::Pnt(TopExp::LastVertex(edge1));

    //intersection of 2 3d lines in point&direction form
    //https://math.stackexchange.com/questions/270767/find-intersection-of-two-3d-lines
    gp_Vec C(gStart0.XYZ());
    gp_Vec D(gStart1.XYZ());
    gp_Vec e(gEnd0.XYZ() - gStart0.XYZ());//direction of line0
    gp_Vec f(gEnd1.XYZ() - gStart1.XYZ());//direction of line1
    Base::Console().Message(
        "DU::apparentInter - e: %s  f: %s\n", formatVector(e).c_str(), formatVector(f).c_str());

    //check for cases the algorithm doesn't handle well
    gp_Vec C1(gEnd0.XYZ());
    gp_Vec D1(gEnd1.XYZ());
    if (C.IsEqual(D, EWTOLERANCE, EWTOLERANCE) || C.IsEqual(D1, EWTOLERANCE, EWTOLERANCE)) {
        intersect = gp_Pnt(C.XYZ());
        return true;
    }
    if (C1.IsEqual(D, EWTOLERANCE, EWTOLERANCE) || C1.IsEqual(D1, EWTOLERANCE, EWTOLERANCE)) {
        intersect = gp_Pnt(C1.XYZ());
        return true;
    }

    gp_Vec g(D - C);//between a point on each line
    Base::Console().Message("DU::apparentInter - C: %s  D: %s  g: %s\n",
                            formatVector(C).c_str(),
                            formatVector(D).c_str(),
                            formatVector(g).c_str());

    gp_Vec fxg = f.Crossed(g);
    double h = fxg.Magnitude();
    gp_Vec fxe = f.Crossed(e);
    double k = fxe.Magnitude();
    Base::Console().Message("DU::apparentInter - h: %.3f k: %.3f\n", h, k);
    if (fpCompare(k, 0.0)) {
        //no intersection
        return false;
    }
    gp_Vec el = e * (h / k);
    double pm = 1.0;
    if (fpCompare(fxg.Dot(fxe), -1.0)) {
        //opposite directions
        pm = -1.0;
    }
    intersect = gp_Pnt(C.XYZ() + el.XYZ() * pm);
    return true;
}

//find the intersection of 2 lines (point0, dir0) and (point1, dir1)
//existence of an intersection is not checked
bool DrawUtil::intersect2Lines3d(Base::Vector3d point0, Base::Vector3d dir0, Base::Vector3d point1,
                                 Base::Vector3d dir1, Base::Vector3d& intersect)
{
    Base::Vector3d g = point1 - point0;
    Base::Vector3d fxg = dir1.Cross(g);
    Base::Vector3d fxgn = fxg;
    fxgn.Normalize();
    Base::Vector3d fxe = dir1.Cross(dir0);
    Base::Vector3d fxen = fxe;
    fxen.Normalize();
    Base::Vector3d dir0n = dir0;
    dir0n.Normalize();
    Base::Vector3d dir1n = dir1;
    dir1n.Normalize();
    if (fabs(dir0n.Dot(dir1n)) == 1.0) {
        //parallel lines, no intersection
        Base::Console().Message("DU::intersect2 - parallel lines, no intersection\n");
        return false;
    }

    double scaler = fxg.Length() / fxe.Length();
    double direction = -1.0;
    if (fxgn == fxen) {
        direction = 1.0;
    }

    intersect = point0 + dir0 * scaler * direction;
    return true;
}

Base::Vector3d DrawUtil::vertex2Vector(const TopoDS_Vertex& v)
{
    gp_Pnt gp = BRep_Tool::Pnt(v);
    return Base::Vector3d(gp.X(), gp.Y(), gp.Z());
}

//TODO: make formatVector using toVector3d
std::string DrawUtil::formatVector(const Base::Vector3d& v)
{
    std::stringstream builder;
    builder << std::fixed << std::setprecision(Base::UnitsApi::getDecimals());
    builder << " (" << v.x << ", " << v.y << ", " << v.z << ") ";
    return builder.str();
}

std::string DrawUtil::formatVector(const gp_Dir& v)
{
    std::stringstream builder;
    builder << std::fixed << std::setprecision(Base::UnitsApi::getDecimals());
    builder << " (" << v.X() << ", " << v.Y() << ", " << v.Z() << ") ";
    return builder.str();
}

std::string DrawUtil::formatVector(const gp_Dir2d& v)
{
    std::stringstream builder;
    builder << std::fixed << std::setprecision(Base::UnitsApi::getDecimals());
    builder << " (" << v.X() << ", " << v.Y() << ") ";
    return builder.str();
}
std::string DrawUtil::formatVector(const gp_Vec& v)
{
    std::stringstream builder;
    builder << std::fixed << std::setprecision(Base::UnitsApi::getDecimals());
    builder << " (" << v.X() << ", " << v.Y() << ", " << v.Z() << ") ";
    return builder.str();
}

std::string DrawUtil::formatVector(const gp_Pnt& v)
{
    std::stringstream builder;
    builder << std::fixed << std::setprecision(Base::UnitsApi::getDecimals());
    builder << " (" << v.X() << ", " << v.Y() << ", " << v.Z() << ") ";
    return builder.str();
}

std::string DrawUtil::formatVector(const gp_Pnt2d& v)
{
    std::stringstream builder;
    builder << std::fixed << std::setprecision(Base::UnitsApi::getDecimals());
    builder << " (" << v.X() << ", " << v.Y() << ") ";
    return builder.str();
}

std::string DrawUtil::formatVector(const QPointF& v)
{
    std::stringstream builder;
    builder << std::fixed << std::setprecision(Base::UnitsApi::getDecimals());
    builder << " (" << v.x() << ", " << v.y() << ") ";
    return builder.str();
}

//! compare 2 vectors for sorting - true if v1 < v2
//! precision::Confusion() is too strict for vertex - vertex comparisons
bool DrawUtil::vectorLess(const Base::Vector3d& v1, const Base::Vector3d& v2)
{
    if ((v1 - v2).Length() > EWTOLERANCE) {//ie v1 != v2
        if (!DrawUtil::fpCompare(v1.x, v2.x, 2.0 * EWTOLERANCE)) {
            return (v1.x < v2.x);
        } else if (!DrawUtil::fpCompare(v1.y, v2.y, 2.0 * EWTOLERANCE)) {
            return (v1.y < v2.y);
        } else {
            return (v1.z < v2.z);
        }
    }
    return false;
}

//! test for equality of two vertexes using the vectorLess comparator as used
//! in sorts and containers
bool DrawUtil::vertexEqual(TopoDS_Vertex& v1, TopoDS_Vertex& v2)
{
    gp_Pnt gv1 = BRep_Tool::Pnt(v1);
    gp_Pnt gv2 = BRep_Tool::Pnt(v2);
    Base::Vector3d vv1(gv1.X(), gv1.Y(), gv1.Z());
    Base::Vector3d vv2(gv2.X(), gv2.Y(), gv2.Z());
    return vectorEqual(vv1, vv2);
}

//! test for equality of two vectors using the vectorLess comparator as used
//! in sorts and containers
bool DrawUtil::vectorEqual(Base::Vector3d& v1, Base::Vector3d& v2)
{
    bool less1 = vectorLess(v1, v2);
    bool less2 = vectorLess(v2, v1);
    return !less1 && !less2;
}

//TODO: the next 2 could be templated
//construct a compound shape from a list of edges
TopoDS_Shape DrawUtil::vectorToCompound(std::vector<TopoDS_Edge> vecIn, bool invert)
{
    BRep_Builder builder;
    TopoDS_Compound compOut;
    builder.MakeCompound(compOut);
    for (auto& v : vecIn) {
        builder.Add(compOut, v);
    }
    if (invert) {
        return ShapeUtils::mirrorShape(compOut);
    }
    return compOut;
}

//construct a compound shape from a list of wires
TopoDS_Shape DrawUtil::vectorToCompound(std::vector<TopoDS_Wire> vecIn, bool invert)
{
    BRep_Builder builder;
    TopoDS_Compound compOut;
    builder.MakeCompound(compOut);
    for (auto& v : vecIn) {
        builder.Add(compOut, v);
    }
    if (invert) {
        return ShapeUtils::mirrorShape(compOut);
    }
    return compOut;
}

// construct a compound shape from a list of shapes
// this version needs a different name since edges/wires are shapes
TopoDS_Shape DrawUtil::shapeVectorToCompound(std::vector<TopoDS_Shape> vecIn, bool invert)
{
    BRep_Builder builder;
    TopoDS_Compound compOut;
    builder.MakeCompound(compOut);
    for (auto& v : vecIn) {
        if (!v.IsNull()) {
            builder.Add(compOut, v);
        }
    }
    if (invert) {
        return ShapeUtils::mirrorShape(compOut);
    }
    return compOut;
}

//constructs a list of edges from a shape
std::vector<TopoDS_Edge> DrawUtil::shapeToVector(TopoDS_Shape shapeIn)
{
    std::vector<TopoDS_Edge> vectorOut;
    TopExp_Explorer expl(shapeIn, TopAbs_EDGE);
    for (; expl.More(); expl.Next()) {
        vectorOut.push_back(TopoDS::Edge(expl.Current()));
    }
    return vectorOut;
}

//!convert fromPoint in coordinate system fromSystem to reference coordinate system
Base::Vector3d DrawUtil::toR3(const gp_Ax2& fromSystem, const Base::Vector3d& fromPoint)
{
    gp_Pnt gFromPoint(fromPoint.x, fromPoint.y, fromPoint.z);
    gp_Trsf T;
    gp_Ax3 gRef;
    gp_Ax3 gFrom(fromSystem);
    T.SetTransformation(gFrom, gRef);
    gp_Pnt gToPoint = gFromPoint.Transformed(T);
    Base::Vector3d toPoint(gToPoint.X(), gToPoint.Y(), gToPoint.Z());
    return toPoint;
}

//! check if two vectors are parallel. Vectors don't have to be unit vectors
bool DrawUtil::checkParallel(const Base::Vector3d v1, Base::Vector3d v2, double tolerance)
{
    double dot = fabs(v1.Dot(v2));
    double mag = v1.Length() * v2.Length();
    return DrawUtil::fpCompare(dot, mag, tolerance);
}

//! rotate vector by angle radians around axis through org
Base::Vector3d DrawUtil::vecRotate(Base::Vector3d vec, double angle, Base::Vector3d axis,
                                   Base::Vector3d org)
{
    Base::Matrix4D xForm;
    xForm.rotLine(org, axis, angle);
    return Base::Vector3d(xForm * (vec));
}

gp_Vec DrawUtil::closestBasis(gp_Vec inVec)
{
    return gp_Vec(togp_Dir(closestBasis(toVector3d(inVec))));
}

Base::Vector3d DrawUtil::closestBasis(Base::Vector3d v)
{
    Base::Vector3d result(0.0, -1, 0);
    Base::Vector3d stdX(1.0, 0.0, 0.0);
    Base::Vector3d stdY(0.0, 1.0, 0.0);
    Base::Vector3d stdZ(0.0, 0.0, 1.0);
    Base::Vector3d stdXr(-1.0, 0.0, 0.0);
    Base::Vector3d stdYr(0.0, -1.0, 0.0);
    Base::Vector3d stdZr(0.0, 0.0, -1.0);

    //first check if already a basis
    if (v.Dot(stdX) == 1.0 || v.Dot(stdY) == 1.0 || v.Dot(stdZ) == 1.0) {
        return v;
    }
    if (v.Dot(stdX) == -1.0 || v.Dot(stdY) == -1.0 || v.Dot(stdZ) == -1.0) {
        return -v;
    }

    //not a basis. find smallest angle with a basis.
    double angleX, angleY, angleZ, angleXr, angleYr, angleZr, angleMin;
    angleX = stdX.GetAngle(v);
    angleY = stdY.GetAngle(v);
    angleZ = stdZ.GetAngle(v);
    angleXr = stdXr.GetAngle(v);
    angleYr = stdYr.GetAngle(v);
    angleZr = stdZr.GetAngle(v);

    angleMin = std::min({angleX, angleY, angleZ, angleXr, angleYr, angleZr});
    if (angleX == angleMin) {
        return Base::Vector3d(1.0, 0.0, 0.0);
    }

    if (angleY == angleMin) {
        return Base::Vector3d(0.0, 1.0, 0.0);
    }

    if (angleZ == angleMin) {
        return Base::Vector3d(0.0, 0.0, 1.0);
    }

    if (angleXr == angleMin) {
        return Base::Vector3d(1.0, 0.0, 0.0);
    }

    if (angleYr == angleMin) {
        return Base::Vector3d(0.0, 1.0, 0.0);
    }

    if (angleZr == angleMin) {
        return Base::Vector3d(0.0, 0.0, 1.0);
    }

    //should not get to here
    return Base::Vector3d(1.0, 0.0, 0.0);
}

Base::Vector3d DrawUtil::closestBasis(Base::Vector3d vDir, gp_Ax2 coordSys)
{
    gp_Dir gDir(vDir.x, vDir.y, vDir.z);
    return closestBasis(gDir, coordSys);
}

Base::Vector3d DrawUtil::closestBasis(gp_Dir gDir, gp_Ax2 coordSys)
{
    gp_Dir xCS = coordSys.XDirection();
    gp_Dir yCS = coordSys.YDirection();
    gp_Dir zCS = coordSys.Direction();

    //first check if already a basis
    if (gDir.Dot(xCS) == 1.0 || gDir.Dot(yCS) == 1.0 || gDir.Dot(zCS) == 1.0) {
        //gDir is parallel with a basis
        return Base::Vector3d(gDir.X(), gDir.Y(), gDir.Z());
    }

    if (gDir.Dot(xCS.Reversed()) == 1.0 || gDir.Dot(yCS.Reversed()) == 1.0
        || gDir.Dot(zCS.Reversed()) == 1.0) {
        //gDir is anti-parallel with a basis
        return Base::Vector3d(-gDir.X(), -gDir.Y(), -gDir.Z());
    }

    //not a basis. find smallest angle with a basis.
    double angleX, angleY, angleZ, angleXr, angleYr, angleZr, angleMin;
    angleX = gDir.Angle(xCS);
    angleY = gDir.Angle(yCS);
    angleZ = gDir.Angle(zCS);
    angleXr = gDir.Angle(xCS.Reversed());
    angleYr = gDir.Angle(yCS.Reversed());
    angleZr = gDir.Angle(zCS.Reversed());

    angleMin = std::min({angleX, angleY, angleZ, angleXr, angleYr, angleZr});
    if (angleX == angleMin) {
        return Base::Vector3d(xCS.X(), xCS.Y(), xCS.Z());
    }

    if (angleY == angleMin) {
        return Base::Vector3d(yCS.X(), yCS.Y(), yCS.Z());
    }

    if (angleZ == angleMin) {
        return Base::Vector3d(zCS.X(), zCS.Y(), zCS.Z());
    }

    if (angleXr == angleMin) {
        return Base::Vector3d(-xCS.X(), -xCS.Y(), -xCS.Z());
    }

    if (angleYr == angleMin) {
        return Base::Vector3d(-yCS.X(), -yCS.Y(), -yCS.Z());
    }

    if (angleZr == angleMin) {
        return Base::Vector3d(-zCS.X(), -zCS.Y(), -zCS.Z());
    }

    //should not get to here
    return Base::Vector3d(xCS.X(), xCS.Y(), xCS.Z());
}

double DrawUtil::getWidthInDirection(gp_Dir direction, TopoDS_Shape& shape)
{
    Base::Vector3d stdX(1.0, 0.0, 0.0);
    Base::Vector3d stdY(0.0, 1.0, 0.0);
    Base::Vector3d stdZ(0.0, 0.0, 1.0);
    Base::Vector3d stdXr(-1.0, 0.0, 0.0);
    Base::Vector3d stdYr(0.0, -1.0, 0.0);
    Base::Vector3d stdZr(0.0, 0.0, -1.0);
    Base::Vector3d vClosestBasis = closestBasis(toVector3d(direction));

    Bnd_Box shapeBox;
    shapeBox.SetGap(0.0);
    BRepBndLib::AddOptimal(shape, shapeBox);
    double xMin = 0, xMax = 0, yMin = 0, yMax = 0, zMin = 0, zMax = 0;
    if (shapeBox.IsVoid()) {
        //this really shouldn't happen here as null shapes should have been caught
        //long before this
        Base::Console().Error("DU::getWidthInDirection - shapeBox is void\n");
        return 0.0;
    }
    shapeBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    if (vClosestBasis.IsEqual(stdX, EWTOLERANCE) || vClosestBasis.IsEqual(stdXr, EWTOLERANCE)) {
        return xMax - xMin;
    }

    if (vClosestBasis.IsEqual(stdY, EWTOLERANCE) || vClosestBasis.IsEqual(stdYr, EWTOLERANCE)) {
        return yMax - yMin;
    }

    if (vClosestBasis.IsEqual(stdZ, EWTOLERANCE) || vClosestBasis.IsEqual(stdZr, EWTOLERANCE)) {
        return zMax - zMin;
    }

    return 0.0;
}

//based on Function provided by Joe Dowsett, 2014
double DrawUtil::sensibleScale(double working_scale)
{
    if (!(working_scale > 0.0)) {
        return 1.0;
    }
    //which gives the largest scale for which the min_space requirements can be met, but we want a 'sensible' scale, rather than 0.28457239...
    //eg if working_scale = 0.115, then we want to use 0.1, similarly 7.65 -> 5, and 76.5 -> 50

    float exponent = std::floor(std::log10(working_scale));//if working_scale = a * 10^b, what is b?
    working_scale *= std::pow(10, -exponent);              //now find what 'a' is.

    //int choices = 10;
    float valid_scales[2][10] = {{1.0,
                                  1.25,
                                  2.0,
                                  2.5,
                                  3.75,
                                  5.0,
                                  7.5,
                                  10.0,
                                  50.0,
                                  100.0},//equate to 1:10, 1:8, 1:5, 1:4, 3:8, 1:2, 3:4, 1:1
                                         //          .1   .125            .375      .75
                                 {1.0,
                                  1.5,
                                  2.0,
                                  3.0,
                                  4.0,
                                  5.0,
                                  8.0,
                                  10.0,
                                  50.0,
                                  100.0}};//equate to 1:1, 3:2, 2:1, 3:1, 4:1, 5:1, 8:1, 10:1
                                          //              1.5:1
    //int i = choices - 1;
    int i = 9;
    while (valid_scales[(exponent >= 0)][i]
           > working_scale) {//choose closest value smaller than 'a' from list.
        i -= 1;              //choosing top list if exponent -ve, bottom list for +ve exponent
    }

    //now have the appropriate scale, reapply the *10^b
    return valid_scales[(exponent >= 0)][i] * pow(10, exponent);
}

double DrawUtil::getDefaultLineWeight(std::string lineType)
{
    return TechDraw::LineGroup::getDefaultWidth(lineType);
}

bool DrawUtil::isBetween(const Base::Vector3d pt, const Base::Vector3d end1,
                         const Base::Vector3d end2)
{
    double segLength = (end2 - end1).Length();
    double l1 = (pt - end1).Length();
    double l2 = (pt - end2).Length();
    if (fpCompare(segLength, l1 + l2)) {
        return true;
    }
    return false;
}

Base::Vector3d DrawUtil::Intersect2d(Base::Vector3d p1, Base::Vector3d d1, Base::Vector3d p2,
                                     Base::Vector3d d2)
{
    Base::Vector3d p12(p1.x + d1.x, p1.y + d1.y, 0.0);
    double A1 = d1.y;
    double B1 = -d1.x;
    double C1 = A1 * p1.x + B1 * p1.y;

    Base::Vector3d p22(p2.x + d2.x, p2.y + d2.y, 0.0);
    double A2 = d2.y;
    double B2 = -d2.x;
    double C2 = A2 * p2.x + B2 * p2.y;

    double det = A1 * B2 - A2 * B1;
    if (fpCompare(det, 0.0, Precision::Confusion())) {
        Base::Console().Message("Lines are parallel\n");
        return Base::Vector3d(0.0, 0.0, 0.0);
    }

    double x = (B2 * C1 - B1 * C2) / det;
    double y = (A1 * C2 - A2 * C1) / det;
    return Base::Vector3d(x, y, 0.0);
}

Base::Vector2d DrawUtil::Intersect2d(Base::Vector2d p1, Base::Vector2d d1, Base::Vector2d p2,
                                     Base::Vector2d d2)
{
    Base::Vector2d p12(p1.x + d1.x, p1.y + d1.y);
    double A1 = d1.y;
    double B1 = -d1.x;
    double C1 = A1 * p1.x + B1 * p1.y;

    Base::Vector2d p22(p2.x + d2.x, p2.y + d2.y);
    double A2 = d2.y;
    double B2 = -d2.x;
    double C2 = A2 * p2.x + B2 * p2.y;

    double det = A1 * B2 - A2 * B1;
    if (fpCompare(det, 0.0, Precision::Confusion())) {
        Base::Console().Message("Lines are parallel\n");
        return Base::Vector2d(0.0, 0.0);
    }

    double x = (B2 * C1 - B1 * C2) / det;
    double y = (A1 * C2 - A2 * C1) / det;
    return Base::Vector2d(x, y);
}


std::string DrawUtil::shapeToString(TopoDS_Shape s)
{
    std::ostringstream buffer;
    BRepTools::Write(s, buffer);
    return buffer.str();
}

TopoDS_Shape DrawUtil::shapeFromString(std::string s)
{
    TopoDS_Shape result;
    BRep_Builder builder;
    std::istringstream buffer(s);
    BRepTools::Read(result, buffer, builder);
    return result;
}

Base::Vector3d DrawUtil::invertY(Base::Vector3d v)
{
    return Base::Vector3d(v.x, -v.y, v.z);
}

QPointF DrawUtil::invertY(QPointF v)
{
    return QPointF(v.x(), -v.y());
}


//obs? was used in CSV prototype of Cosmetics
std::vector<std::string> DrawUtil::split(std::string csvLine)
{
    //    Base::Console().Message("DU::split - csvLine: %s\n", csvLine.c_str());
    std::vector<std::string> result;
    std::stringstream lineStream(csvLine);
    std::string cell;

    while (std::getline(lineStream, cell, ',')) {
        result.push_back(cell);
    }
    return result;
}

//obs? was used in CSV prototype of Cosmetics
std::vector<std::string> DrawUtil::tokenize(std::string csvLine, std::string delimiter)
{
    //    Base::Console().Message("DU::tokenize - csvLine: %s delimit: %s\n", csvLine.c_str(), delimiter.c_str());
    std::string s(csvLine);
    size_t pos = 0;
    std::vector<std::string> tokens;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        tokens.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    if (!s.empty()) {
        tokens.push_back(s);
    }
    return tokens;
}

App::Color DrawUtil::pyTupleToColor(PyObject* pColor)
{
    //    Base::Console().Message("DU::pyTupleToColor()\n");
    double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
    if (!PyTuple_Check(pColor)) {
        return App::Color(red, green, blue, alpha);
    }

    int tSize = (int)PyTuple_Size(pColor);
    if (tSize > 2) {
        PyObject* pRed = PyTuple_GetItem(pColor, 0);
        red = PyFloat_AsDouble(pRed);
        PyObject* pGreen = PyTuple_GetItem(pColor, 1);
        green = PyFloat_AsDouble(pGreen);
        PyObject* pBlue = PyTuple_GetItem(pColor, 2);
        blue = PyFloat_AsDouble(pBlue);
    }
    if (tSize > 3) {
        PyObject* pAlpha = PyTuple_GetItem(pColor, 3);
        alpha = PyFloat_AsDouble(pAlpha);
    }
    return App::Color(red, green, blue, alpha);
}

PyObject* DrawUtil::colorToPyTuple(App::Color color)
{
    //    Base::Console().Message("DU::pyTupleToColor()\n");
    PyObject* pTuple = PyTuple_New(4);
    PyObject* pRed = PyFloat_FromDouble(color.r);
    PyObject* pGreen = PyFloat_FromDouble(color.g);
    PyObject* pBlue = PyFloat_FromDouble(color.b);
    PyObject* pAlpha = PyFloat_FromDouble(color.a);

    PyTuple_SET_ITEM(pTuple, 0, pRed);
    PyTuple_SET_ITEM(pTuple, 1, pGreen);
    PyTuple_SET_ITEM(pTuple, 2, pBlue);
    PyTuple_SET_ITEM(pTuple, 3, pAlpha);

    return pTuple;
}

//check for crazy edge.  This is probably a geometry error of some sort.
bool DrawUtil::isCrazy(TopoDS_Edge e)
{

    if (e.IsNull()) {
        return true;
    }

    bool crazyOK = Preferences::getPreferenceGroup("debug")->GetBool("allowCrazyEdge", false);
    if (crazyOK) {
        return false;
    }

    BRepAdaptor_Curve adapt(e);

    double edgeLength = GCPnts_AbscissaPoint::Length(adapt, Precision::Confusion());
    if (edgeLength < 0.00001) {//edge is scaled.  this is 0.00001 mm on paper
        return true;
    }
    if (edgeLength > 9999.9) {//edge is scaled. this is 10 m on paper.  can't be right?
        return true;
    }

    double start = BRepLProp_CurveTool::FirstParameter(adapt);
    double end = BRepLProp_CurveTool::LastParameter(adapt);
    BRepLProp_CLProps propStart(adapt, start, 0, Precision::Confusion());
    const gp_Pnt& vStart = propStart.Value();
    BRepLProp_CLProps propEnd(adapt, end, 0, Precision::Confusion());
    const gp_Pnt& vEnd = propEnd.Value();
    double distance = vStart.Distance(vEnd);
    double ratio = edgeLength / distance;
    if (adapt.GetType() == GeomAbs_BSplineCurve && distance > 0.001 &&// not a closed loop
        ratio > 9999.9) {                                             // 10, 000x
        return true;                                                  //this is crazy edge
    } else if (adapt.GetType() == GeomAbs_Ellipse) {
        gp_Elips ellp = adapt.Ellipse();
        double major = ellp.MajorRadius();
        double minor = ellp.MinorRadius();
        if (minor < 0.001) {//too narrow
            return true;
        } else if (major > 9999.9) {//too big
            return true;
        }
    }

    //    Base::Console().Message("DU::isCrazy - returns: %d ratio: %.3f\n", false, ratio);
    return false;
}

//get 3d position of a face's center
Base::Vector3d DrawUtil::getFaceCenter(TopoDS_Face f)
{
    BRepAdaptor_Surface adapt(f);
    double u1 = adapt.FirstUParameter();
    double u2 = adapt.LastUParameter();
    double mu = (u1 + u2) / 2.0;
    double v1 = adapt.FirstVParameter();
    double v2 = adapt.LastVParameter();
    double mv = (v1 + v2) / 2.0;
    BRepLProp_SLProps prop(adapt, mu, mv, 0, Precision::Confusion());
    const gp_Pnt gv = prop.Value();
    return Base::Vector3d(gv.X(), gv.Y(), gv.Z());
}

// test the circulation of the triangle A-B-C
bool DrawUtil::circulation(Base::Vector3d A, Base::Vector3d B, Base::Vector3d C)
{
    if (A.x * B.y + A.y * C.x + B.x * C.y - C.x * B.y - C.y * A.x - B.x * A.y > 0.0) {
        return true;
    } else {
        return false;
    }
}

Base::Vector3d DrawUtil::getTrianglePoint(Base::Vector3d p1, Base::Vector3d dir, Base::Vector3d p2)
{
    // get third point of a perpendicular triangle
    // p1, p2 ...vertexes of hypothenusis, dir ...direction of one kathete, p3 ...3rd vertex
    float a = -dir.y;
    float b = dir.x;
    float c1 = p1.x * a + p1.y * b;
    float c2 = -p2.x * b + p2.y * a;
    float ab = a * a + b * b;
    float x = (c1 * a - c2 * b) / ab;
    float y = (c2 * a + c1 * b) / ab;
    Base::Vector3d p3(x,y,0.0);
    return p3;
}

int DrawUtil::countSubShapes(TopoDS_Shape shape, TopAbs_ShapeEnum subShape)
{
    int count = 0;
    TopExp_Explorer Ex(shape, subShape);
    while (Ex.More()) {
        count++;
        Ex.Next();
    }
    return count;
}

//https://stackoverflow.com/questions/5665231/most-efficient-way-to-escape-xml-html-in-c-string
//for every character in inoutText, check if it is on the list of characters to be substituted and
//replace it with the encoding string
void DrawUtil::encodeXmlSpecialChars(std::string& inoutText)
{
    std::string buffer;
    buffer.reserve(inoutText.size());
    for (size_t cursor = 0; cursor != inoutText.size(); ++cursor) {
        switch (inoutText.at(cursor)) {
            case '&':
                buffer.append("&amp;");
                break;
            case '\"':
                buffer.append("&quot;");
                break;
            case '\'':
                buffer.append("&apos;");
                break;
            case '<':
                buffer.append("&lt;");
                break;
            case '>':
                buffer.append("&gt;");
                break;
            default:
                buffer.append(&inoutText.at(cursor), 1);//not a special character
                break;
        }
    }
    inoutText.swap(buffer);
}

//Sort edges into nose to tail order. From Part/App/AppPartPy.cpp.  gives back a sequence
//of nose to tail edges and a shrunken input sequence of edges (the unconnected left overs)
//struct EdgePoints {
//    gp_Pnt v1, v2;
//    std::list<TopoDS_Edge>::iterator it;
//    TopoDS_Edge edge;
//};
std::list<TopoDS_Edge> DrawUtil::sort_Edges(double tol3d, std::list<TopoDS_Edge>& edges)
{
    tol3d = tol3d * tol3d;
    std::list<EdgePoints> edge_points;
    TopExp_Explorer xp;
    for (std::list<TopoDS_Edge>::iterator it = edges.begin(); it != edges.end(); ++it) {
        EdgePoints ep;
        xp.Init(*it, TopAbs_VERTEX);
        ep.v1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        xp.Next();
        ep.v2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        ep.it = it;
        ep.edge = *it;
        edge_points.push_back(ep);
    }

    if (edge_points.empty()) {
        return std::list<TopoDS_Edge>();
    }

    std::list<TopoDS_Edge> sorted;
    gp_Pnt gpChainFirst, gpChainLast;
    gpChainFirst = edge_points.front().v1;
    gpChainLast = edge_points.front().v2;

    sorted.push_back(edge_points.front().edge);
    edges.erase(edge_points.front().it);
    edge_points.erase(edge_points.begin());

    while (!edge_points.empty()) {
        // search for adjacent edge
        std::list<EdgePoints>::iterator itEdgePoint;
        for (itEdgePoint = edge_points.begin(); itEdgePoint != edge_points.end(); ++itEdgePoint) {
            if (itEdgePoint->v1.SquareDistance(gpChainLast) <= tol3d) {
                //found a connection from end of chain to start of edge
                gpChainLast = itEdgePoint->v2;
                sorted.push_back(itEdgePoint->edge);
                edges.erase(itEdgePoint->it);
                edge_points.erase(itEdgePoint);
                itEdgePoint = edge_points.begin();
                break;
            } else if (itEdgePoint->v2.SquareDistance(gpChainFirst) <= tol3d) {
                //found a connection from start of chain to end of edge
                gpChainFirst = itEdgePoint->v1;
                sorted.push_front(itEdgePoint->edge);
                edges.erase(itEdgePoint->it);
                edge_points.erase(itEdgePoint);
                itEdgePoint = edge_points.begin();
                break;
            } else if (itEdgePoint->v2.SquareDistance(gpChainLast) <= tol3d) {
                //found a connection from end of chain to end of edge
                gpChainLast = itEdgePoint->v1;
                Standard_Real firstParam, lastParam;
                const Handle(Geom_Curve)& curve =
                    BRep_Tool::Curve(itEdgePoint->edge, firstParam, lastParam);
                firstParam = curve->ReversedParameter(firstParam);
                lastParam = curve->ReversedParameter(lastParam);
                TopoDS_Edge edgeReversed =
                    BRepBuilderAPI_MakeEdge(curve->Reversed(), firstParam, lastParam);
                sorted.push_back(edgeReversed);
                edges.erase(itEdgePoint->it);
                edge_points.erase(itEdgePoint);
                itEdgePoint = edge_points.begin();
                break;
            } else if (itEdgePoint->v1.SquareDistance(gpChainFirst) <= tol3d) {
                //found a connection from start of chain to start of edge
                gpChainFirst = itEdgePoint->v2;
                Standard_Real firstParam, lastParam;
                const Handle(Geom_Curve)& curve =
                    BRep_Tool::Curve(itEdgePoint->edge, firstParam, lastParam);
                firstParam = curve->ReversedParameter(firstParam);
                lastParam = curve->ReversedParameter(lastParam);
                TopoDS_Edge edgeReversed =
                    BRepBuilderAPI_MakeEdge(curve->Reversed(), firstParam, lastParam);
                sorted.push_front(edgeReversed);
                edges.erase(itEdgePoint->it);
                edge_points.erase(itEdgePoint);
                itEdgePoint = edge_points.begin();
                break;
            }
        }

        if (itEdgePoint == edge_points.end()
            || gpChainLast.SquareDistance(gpChainFirst) <= tol3d) {
            // no adjacent edge found or polyline is closed
            return sorted;
        }
    }

    return sorted;
}

// Supplementary mathematical functions
// ====================================

int DrawUtil::sgn(double x)
{
    return (x > +Precision::Confusion()) - (x < -Precision::Confusion());
}

double DrawUtil::sqr(double x)
{
    return x * x;
}

void DrawUtil::angleNormalize(double& fi)
{
    while (fi <= -M_PI) {
        fi += M_2PI;
    }
    while (fi > M_PI) {
        fi -= M_2PI;
    }
}

double DrawUtil::angleComposition(double fi, double delta)
{
    fi += delta;

    angleNormalize(fi);
    return fi;
}

double DrawUtil::angleDifference(double fi1, double fi2, bool reflex)
{
    angleNormalize(fi1);
    angleNormalize(fi2);

    fi1 -= fi2;

    if ((fi1 > +M_PI || fi1 <= -M_PI) != reflex) {
        fi1 += fi1 > 0.0 ? -M_2PI : +M_2PI;
    }

    return fi1;
}

// Interval marking functions
// ==========================

unsigned int DrawUtil::intervalMerge(std::vector<std::pair<double, bool>>& marking, double boundary,
                                     bool wraps)
{
    // We will be returning the placement index instead of an iterator, because indices
    // are still valid after we insert on higher positions, while iterators may be invalidated
    // due to the insertion triggered reallocation
    unsigned int i = 0;
    bool last = false;

    if (wraps && !marking.empty()) {
        last = marking.back().second;
    }

    while (i < marking.size()) {
        if (marking[i].first == boundary) {
            return i;
        }
        if (marking[i].first > boundary) {
            break;
        }

        last = marking[i].second;
        ++i;
    }

    if (!wraps && i >= marking.size()) {
        last = false;
    }

    marking.insert(marking.begin() + i, std::pair<double, bool>(boundary, last));
    return i;
}

void DrawUtil::intervalMarkLinear(std::vector<std::pair<double, bool>>& marking, double start,
                                  double length, bool value)
{
    if (length == 0.0) {
        return;
    }
    if (length < 0.0) {
        length = -length;
        start -= length;
    }

    unsigned int startIndex = intervalMerge(marking, start, false);
    unsigned int endIndex = intervalMerge(marking, start + length, false);

    while (startIndex < endIndex) {
        marking[startIndex].second = value;
        ++startIndex;
    }
}

void DrawUtil::intervalMarkCircular(std::vector<std::pair<double, bool>>& marking, double start,
                                    double length, bool value)
{
    if (length == 0.0) {
        return;
    }
    if (length < 0.0) {
        length = -length;
        start -= length;
    }
    if (length > M_2PI) {
        length = M_2PI;
    }

    angleNormalize(start);

    double end = start + length;
    if (end > M_PI) {
        end -= M_2PI;
    }

    // Just make sure the point is stored, its index is read last
    intervalMerge(marking, end, true);
    unsigned int startIndex = intervalMerge(marking, start, true);
    unsigned int endIndex = intervalMerge(marking, end, true);

    do {
        marking[startIndex].second = value;
        ++startIndex;
        startIndex %= marking.size();
    } while (startIndex != endIndex);
}

// Supplementary 2D analytic geometry functions
//=============================================

int DrawUtil::findRootForValue(double Ax2, double Bxy, double Cy2, double Dx, double Ey, double F,
                               double value, bool findX, double roots[])
{
    double qA = 0.0;
    double qB = 0.0;
    double qC = 0.0;

    if (findX) {
        qA = Ax2;
        qB = Bxy * value + Dx;
        qC = Cy2 * value * value + Ey * value + F;
    } else {
        qA = Cy2;
        qB = Bxy * value + Ey;
        qC = Ax2 * value * value + Dx * value + F;
    }

    if (fabs(qA) < Precision::Confusion()) {
        // No quadratic coefficient - the equation is linear
        if (fabs(qB) < Precision::Confusion()) {
            // Not even linear coefficient - test for zero
            if (fabs(qC) > Precision::Confusion()) {
                // This equation has no solution
                return 0;
            } else {
                // Signal infinite number of solutions by returning 2, but do not touch root variables
                return 2;
            }
        } else {
            roots[0] = -qC / qB;
            return 1;
        }
    } else {
        double qD = sqr(qB) - 4.0 * qA * qC;
        if (qD < -Precision::Confusion()) {
            // Negative discriminant => no real roots
            return 0;
        } else if (qD > +Precision::Confusion()) {
            // Two distinctive roots
            roots[0] = (-qB + sqrt(qD)) * 0.5 / qA;
            roots[1] = (-qB - sqrt(qD)) * 0.5 / qA;
            return 2;
        } else {
            // Double root
            roots[0] = -qB * 0.5 / qA;
            return 1;
        }
    }
}

bool DrawUtil::mergeBoundedPoint(const Base::Vector2d& point, const Base::BoundBox2d& boundary,
                                 std::vector<Base::Vector2d>& storage)
{
    if (!boundary.Contains(point, Precision::Confusion())) {
        return false;
    }

    for (unsigned int i = 0; i < storage.size(); ++i) {
        if (point.IsEqual(storage[i], Precision::Confusion())) {
            return false;
        }
    }

    storage.push_back(point);
    return true;
}

void DrawUtil::findConicRectangleIntersections(double conicAx2, double conicBxy, double conicCy2,
                                               double conicDx, double conicEy, double conicF,
                                               const Base::BoundBox2d& rectangle,
                                               std::vector<Base::Vector2d>& intersections)
{
    double roots[2];
    int rootCount;

    // Find intersections with rectangle left side line
    roots[0] = rectangle.MinY;
    roots[1] = rectangle.MaxY;
    rootCount = findRootForValue(
        conicAx2, conicBxy, conicCy2, conicDx, conicEy, conicF, rectangle.MinX, false, roots);
    if (rootCount > 0) {
        mergeBoundedPoint(Base::Vector2d(rectangle.MinX, roots[0]), rectangle, intersections);
    }
    if (rootCount > 1) {
        mergeBoundedPoint(Base::Vector2d(rectangle.MinX, roots[1]), rectangle, intersections);
    }

    // Find intersections with rectangle right side line
    roots[0] = rectangle.MinY;
    roots[1] = rectangle.MaxY;
    rootCount = findRootForValue(
        conicAx2, conicBxy, conicCy2, conicDx, conicEy, conicF, rectangle.MaxX, false, roots);
    if (rootCount > 0) {
        mergeBoundedPoint(Base::Vector2d(rectangle.MaxX, roots[0]), rectangle, intersections);
    }
    if (rootCount > 1) {
        mergeBoundedPoint(Base::Vector2d(rectangle.MaxX, roots[1]), rectangle, intersections);
    }

    // Find intersections with rectangle top side line
    roots[0] = rectangle.MinX;
    roots[1] = rectangle.MaxX;
    rootCount = findRootForValue(
        conicAx2, conicBxy, conicCy2, conicDx, conicEy, conicF, rectangle.MinY, true, roots);
    if (rootCount > 0) {
        mergeBoundedPoint(Base::Vector2d(roots[0], rectangle.MinY), rectangle, intersections);
    }
    if (rootCount > 1) {
        mergeBoundedPoint(Base::Vector2d(roots[1], rectangle.MinY), rectangle, intersections);
    }

    // Find intersections with rectangle top side line
    roots[0] = rectangle.MinX;
    roots[1] = rectangle.MaxX;
    rootCount = findRootForValue(
        conicAx2, conicBxy, conicCy2, conicDx, conicEy, conicF, rectangle.MaxY, true, roots);
    if (rootCount > 0) {
        mergeBoundedPoint(Base::Vector2d(roots[0], rectangle.MaxY), rectangle, intersections);
    }
    if (rootCount > 1) {
        mergeBoundedPoint(Base::Vector2d(roots[1], rectangle.MaxY), rectangle, intersections);
    }
}

void DrawUtil::findLineRectangleIntersections(const Base::Vector2d& linePoint, double lineAngle,
                                              const Base::BoundBox2d& rectangle,
                                              std::vector<Base::Vector2d>& intersections)
{
    Base::Vector2d lineDirection(Base::Vector2d::FromPolar(1.0, lineAngle));
    findConicRectangleIntersections(0.0,
                                    0.0,
                                    0.0,
                                    +lineDirection.y,
                                    -lineDirection.x,
                                    lineDirection.x * linePoint.y - lineDirection.y * linePoint.x,
                                    rectangle,
                                    intersections);
}

void DrawUtil::findCircleRectangleIntersections(const Base::Vector2d& circleCenter,
                                                double circleRadius,
                                                const Base::BoundBox2d& rectangle,
                                                std::vector<Base::Vector2d>& intersections)
{
    findConicRectangleIntersections(1.0,
                                    0.0,
                                    1.0,
                                    -2.0 * circleCenter.x,
                                    -2.0 * circleCenter.y,
                                    sqr(circleCenter.x) + sqr(circleCenter.y) - sqr(circleRadius),
                                    rectangle,
                                    intersections);
}

void DrawUtil::findLineSegmentRectangleIntersections(const Base::Vector2d& linePoint,
                                                     double lineAngle, double segmentBasePosition,
                                                     double segmentLength,
                                                     const Base::BoundBox2d& rectangle,
                                                     std::vector<Base::Vector2d>& intersections)
{
    findLineRectangleIntersections(linePoint, lineAngle, rectangle, intersections);

    if (segmentLength < 0.0) {
        segmentLength = -segmentLength;
        segmentBasePosition -= segmentLength;
    }

    // Dispose the points on rectangle but not within the line segment boundaries
    Base::Vector2d segmentDirection(Base::Vector2d::FromPolar(1.0, lineAngle));
    for (unsigned int i = 0; i < intersections.size();) {
        double pointPosition = segmentDirection * (intersections[i] - linePoint);

        if (pointPosition < segmentBasePosition - Precision::Confusion()
            || pointPosition > segmentBasePosition + segmentLength + Precision::Confusion()) {
            intersections.erase(intersections.begin() + i);
        } else {
            ++i;
        }
    }

    // Try to add the line segment end points
    mergeBoundedPoint(linePoint + segmentBasePosition * segmentDirection, rectangle, intersections);
    mergeBoundedPoint(linePoint + (segmentBasePosition + segmentLength) * segmentDirection,
                      rectangle,
                      intersections);
}

void DrawUtil::findCircularArcRectangleIntersections(const Base::Vector2d& circleCenter,
                                                     double circleRadius, double arcBaseAngle,
                                                     double arcRotation,
                                                     const Base::BoundBox2d& rectangle,
                                                     std::vector<Base::Vector2d>& intersections)
{
    findCircleRectangleIntersections(circleCenter, circleRadius, rectangle, intersections);

    if (arcRotation < 0.0) {
        arcRotation = -arcRotation;
        arcBaseAngle -= arcRotation;
        if (arcBaseAngle <= -M_PI) {
            arcBaseAngle += M_2PI;
        }
    }

    // Dispose the points on rectangle but not within the circular arc boundaries
    for (unsigned int i = 0; i < intersections.size();) {
        double pointAngle = (intersections[i] - circleCenter).Angle();
        if (pointAngle < arcBaseAngle - Precision::Confusion()) {
            pointAngle += M_2PI;
        }

        if (pointAngle > arcBaseAngle + arcRotation + Precision::Confusion()) {
            intersections.erase(intersections.begin() + i);
        } else {
            ++i;
        }
    }

    // Try to add the circular arc end points
    mergeBoundedPoint(circleCenter + Base::Vector2d::FromPolar(circleRadius, arcBaseAngle),
                      rectangle,
                      intersections);
    mergeBoundedPoint(circleCenter
                          + Base::Vector2d::FromPolar(circleRadius, arcBaseAngle + arcRotation),
                      rectangle,
                      intersections);
}

//copy whole text file from inSpec to outSpec
//create empty outSpec file if inSpec
void DrawUtil::copyFile(std::string inSpec, std::string outSpec)
{
    //    Base::Console().Message("DU::copyFile(%s, %s)\n", inSpec.c_str(), outSpec.c_str());
    if (inSpec.empty()) {
        // create an empty file
        Base::FileInfo fi(outSpec);
        Base::ofstream output(fi);
        return;
    }
    Base::FileInfo fi(inSpec);
    if (!fi.isReadable()) {
        return;
    }
    bool rc = fi.copyTo(outSpec.c_str());
    if (!rc) {
        Base::Console().Message(
            "DU::copyFile - failed - in: %s out:%s\n", inSpec.c_str(), outSpec.c_str());
    }
}

//! static method that provides a translated std::string for objects that are not derived from DrawView
std::string DrawUtil::translateArbitrary(std::string context, std::string baseName, std::string uniqueName)
{
    std::string suffix("");
    if (uniqueName.length() > baseName.length()) {
        suffix = uniqueName.substr(baseName.length(), uniqueName.length() - baseName.length());
    }
    QString qTranslated = qApp->translate(context.c_str(), baseName.c_str());
    std::string ssTranslated = Base::Tools::toStdString(qTranslated);
    return ssTranslated + suffix;
}

// true if owner->element is a cosmetic vertex
bool DrawUtil::isCosmeticVertex(App::DocumentObject* owner, std::string element)
{
    auto ownerView = static_cast<TechDraw::DrawViewPart*>(owner);
    auto vertex = ownerView->getVertex(element);
    if (vertex) {
        return vertex->getCosmetic();
    }
    return false;
}

// true if owner->element is a cosmetic edge
bool DrawUtil::isCosmeticEdge(App::DocumentObject* owner, std::string element)
{
    auto ownerView = static_cast<TechDraw::DrawViewPart*>(owner);
    auto edge = ownerView->getEdge(element);
    if (edge && edge->source() == 1 && edge->getCosmetic()) {
        return true;
    }
    return false;
}

// true if owner->element is a center line
bool DrawUtil::isCenterLine(App::DocumentObject* owner, std::string element)
{
    auto ownerView = static_cast<TechDraw::DrawViewPart*>(owner);
    auto edge = ownerView->getEdge(element);
    if (edge && edge->source() == 2 && edge->getCosmetic()) {
        return true;
    }
    return false;
}

//============================
// various debugging routines.
void DrawUtil::dumpVertexes(const char* text, const TopoDS_Shape& s)
{
    Base::Console().Message("DUMP - %s\n", text);
    TopExp_Explorer expl(s, TopAbs_VERTEX);
    for (int i = 1; expl.More(); expl.Next(), i++) {
        const TopoDS_Vertex& v = TopoDS::Vertex(expl.Current());
        gp_Pnt pnt = BRep_Tool::Pnt(v);
        Base::Console().Message("v%d: (%.3f, %.3f, %.3f)\n", i, pnt.X(), pnt.Y(), pnt.Z());
    }
}

void DrawUtil::countFaces(const char* text, const TopoDS_Shape& s)
{
    TopTools_IndexedMapOfShape mapOfFaces;
    TopExp::MapShapes(s, TopAbs_FACE, mapOfFaces);
    int num = mapOfFaces.Extent();
    Base::Console().Message("COUNT - %s has %d Faces\n", text, num);
}

//count # of unique Wires in shape.
void DrawUtil::countWires(const char* text, const TopoDS_Shape& s)
{
    TopTools_IndexedMapOfShape mapOfWires;
    TopExp::MapShapes(s, TopAbs_WIRE, mapOfWires);
    int num = mapOfWires.Extent();
    Base::Console().Message("COUNT - %s has %d wires\n", text, num);
}

void DrawUtil::countEdges(const char* text, const TopoDS_Shape& s)
{
    TopTools_IndexedMapOfShape mapOfEdges;
    TopExp::MapShapes(s, TopAbs_EDGE, mapOfEdges);
    int num = mapOfEdges.Extent();
    Base::Console().Message("COUNT - %s has %d edges\n", text, num);
}

void DrawUtil::dumpEdges(const char* text, const TopoDS_Shape& s)
{
    Base::Console().Message("DUMP - %s\n", text);
    TopExp_Explorer expl(s, TopAbs_EDGE);
    for (int i = 1; expl.More(); expl.Next(), i++) {
        const TopoDS_Edge& e = TopoDS::Edge(expl.Current());
        dumpEdge("dumpEdges", i, e);
    }
}

void DrawUtil::dump1Vertex(const char* text, const TopoDS_Vertex& v)
{
    //    Base::Console().Message("DUMP - dump1Vertex - %s\n",text);
    gp_Pnt pnt = BRep_Tool::Pnt(v);
    Base::Console().Message("%s: (%.3f, %.3f, %.3f)\n", text, pnt.X(), pnt.Y(), pnt.Z());
}

void DrawUtil::dumpEdge(const char* label, int i, TopoDS_Edge e)
{
    BRepAdaptor_Curve adapt(e);
    double start = BRepLProp_CurveTool::FirstParameter(adapt);
    double end = BRepLProp_CurveTool::LastParameter(adapt);
    BRepLProp_CLProps propStart(adapt, start, 0, Precision::Confusion());
    const gp_Pnt& vStart = propStart.Value();
    BRepLProp_CLProps propEnd(adapt, end, 0, Precision::Confusion());
    const gp_Pnt& vEnd = propEnd.Value();
    //Base::Console().Message("%s edge:%d start:(%.3f, %.3f, %.3f)/%0.3f end:(%.2f, %.3f, %.3f)/%.3f\n", label, i,
    //                        vStart.X(), vStart.Y(), vStart.Z(), start, vEnd.X(), vEnd.Y(), vEnd.Z(), end);
    Base::Console().Message(
        "%s edge:%d start:(%.3f, %.3f, %.3f)  end:(%.2f, %.3f, %.3f) Orient: %d\n",
        label,
        i,
        vStart.X(),
        vStart.Y(),
        vStart.Z(),
        vEnd.X(),
        vEnd.Y(),
        vEnd.Z(),
        static_cast<int>(e.Orientation()));
    double edgeLength = GCPnts_AbscissaPoint::Length(adapt, Precision::Confusion());
    Base::Console().Message(">>>>>>> length: %.3f  distance: %.3f ration: %.3f type: %d\n",
                            edgeLength,
                            vStart.Distance(vEnd),
                            edgeLength / vStart.Distance(vEnd),
                            static_cast<int>(adapt.GetType()));
}

const char* DrawUtil::printBool(bool b)
{
    return (b ? "True" : "False");
}

QString DrawUtil::qbaToDebug(const QByteArray& line)
{
    QString s;
    uchar c;

    for (int i = 0; i < line.size(); i++) {
        c = line[i];
        if ((c >= 0x20) && (c <= 126)) {
            s.append(QChar::fromLatin1(c));
        } else {
            s.append(QString::fromUtf8("<%1>").arg(c, 2, 16, QChar::fromLatin1('0')));
        }
    }
    return s;
}

void DrawUtil::dumpCS(const char* text, const gp_Ax2& CS)
{
    gp_Dir baseAxis = CS.Direction();
    gp_Dir baseX = CS.XDirection();
    gp_Dir baseY = CS.YDirection();
    gp_Pnt baseOrg = CS.Location();
    Base::Console().Message("DU::dumpCS - %s Loc: %s Axis: %s X: %s Y: %s\n",
                            text,
                            DrawUtil::formatVector(baseOrg).c_str(),
                            DrawUtil::formatVector(baseAxis).c_str(),
                            DrawUtil::formatVector(baseX).c_str(),
                            DrawUtil::formatVector(baseY).c_str());
}

void DrawUtil::dumpCS3(const char* text, const gp_Ax3& CS)
{
    gp_Dir baseAxis = CS.Direction();
    gp_Dir baseX = CS.XDirection();
    gp_Dir baseY = CS.YDirection();
    gp_Pnt baseOrg = CS.Location();
    Base::Console().Message("DU::dumpCS3 - %s Loc: %s Axis: %s X: %s Y: %s\n",
                            text,
                            DrawUtil::formatVector(baseOrg).c_str(),
                            DrawUtil::formatVector(baseAxis).c_str(),
                            DrawUtil::formatVector(baseX).c_str(),
                            DrawUtil::formatVector(baseY).c_str());
}
