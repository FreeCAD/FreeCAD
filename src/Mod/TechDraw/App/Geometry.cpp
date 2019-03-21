/***************************************************************************
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <Approx_Curve3d.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_HCurve.hxx>
#include <BRepLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Precision.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <gce_MakeCirc.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax2.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Geometry.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomConvert_BSplineCurveToBezierCurve.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <GeomLProp_CLProps.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Poly_Polygon3D.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TColgp_Array1OfPnt.hxx>

#include <cmath>
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools2D.h>

#include <Mod/Part/App/Geometry.h>

#include "Geometry.h"
#include "DrawUtil.h"

using namespace TechDrawGeometry;
using namespace TechDraw;

// Collection of Geometric Features
Wire::Wire()
{
}

Wire::Wire(const TopoDS_Wire &w)
{
    TopExp_Explorer edges(w, TopAbs_EDGE);
    for (; edges.More(); edges.Next()) {
        const auto edge( TopoDS::Edge(edges.Current()) );
        geoms.push_back( BaseGeom::baseFactory(edge) );
    }
}

Wire::~Wire()
{
    for(auto it : geoms) {
        delete it;
    }
    geoms.clear();
}

Face::~Face()
{
    for(auto it : wires) {
        delete it;
    }
    wires.clear();
}

BaseGeom::BaseGeom() :
    geomType(NOTDEF),
    extractType(Plain),             //obs
    classOfEdge(ecNONE),
    visible(true),
    reversed(false),
    ref3D(-1)                      //obs?
{
}


std::vector<Base::Vector2d> BaseGeom::findEndPoints()
{
    std::vector<Base::Vector2d> result;

    gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(occEdge));
    result.push_back(Base::Vector2d(p.X(),p.Y()));
    p = BRep_Tool::Pnt(TopExp::LastVertex(occEdge));
    result.push_back(Base::Vector2d(p.X(),p.Y()));

    return result;
}


Base::Vector2d BaseGeom::getStartPoint()
{
    std::vector<Base::Vector2d> verts = findEndPoints();
    return verts[0];
}


Base::Vector2d BaseGeom::getEndPoint()
{
    std::vector<Base::Vector2d> verts = findEndPoints();
    return verts[1];
}


double BaseGeom::minDist(Base::Vector2d p)
{
    double minDist = -1.0;
    gp_Pnt pnt(p.x,p.y,0.0);
    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
    minDist = TechDraw::DrawUtil::simpleMinDist(occEdge,v);
    return minDist;
}

//!find point on me nearest to p
Base::Vector2d BaseGeom::nearPoint(const BaseGeom* p)
{
    Base::Vector2d result(0.0,0.0);
    TopoDS_Edge pEdge = p->occEdge;
    BRepExtrema_DistShapeShape extss(occEdge, pEdge);
    if (extss.IsDone()) {
        int count = extss.NbSolution();
        if (count != 0) {
            gp_Pnt p1;
            p1 = extss.PointOnShape1(1);
            result =  Base::Vector2d(p1.X(),p1.Y());
        }
    }
    return result;
}

Base::Vector2d BaseGeom::nearPoint(Base::Vector2d p)
{
    gp_Pnt pnt(p.x,p.y,0.0);
    Base::Vector2d result(0.0,0.0);
    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
    BRepExtrema_DistShapeShape extss(occEdge, v);
    if (extss.IsDone()) {
        int count = extss.NbSolution();
        if (count != 0) {
            gp_Pnt p1;
            p1 = extss.PointOnShape1(1);
            result =  Base::Vector2d(p1.X(),p1.Y());
        }
    }
    return result;
}

std::string BaseGeom::dump()
{
    Base::Vector2d start = getStartPoint();
    Base::Vector2d end   = getEndPoint();
    std::stringstream ss;
    ss << "BaseGeom: s:(" << start.x << "," << start.y << ") e:(" << end.x << "," << end.y << ") ";
    ss << "type: " << geomType << " class: " << classOfEdge << " viz: " << visible << " rev: " << reversed;
    return ss.str();
}


//! Convert 1 OCC edge into 1 BaseGeom (static factory method)
BaseGeom* BaseGeom::baseFactory(TopoDS_Edge edge)
{
    BaseGeom* result = NULL;
    BRepAdaptor_Curve adapt(edge);
    switch(adapt.GetType()) {
      case GeomAbs_Circle: {
        double f = adapt.FirstParameter();
        double l = adapt.LastParameter();
        gp_Pnt s = adapt.Value(f);
        gp_Pnt e = adapt.Value(l);

        if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
              Circle *circle = new Circle(edge);
              result = circle;
        } else {
              AOC *aoc = new AOC(edge);
              result = aoc;
        }
      } break;
      case GeomAbs_Ellipse: {
        double f = adapt.FirstParameter();
        double l = adapt.LastParameter();
        gp_Pnt s = adapt.Value(f);
        gp_Pnt e = adapt.Value(l);
        if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
              Ellipse *ellipse = new Ellipse(edge);
              result = ellipse;
        } else {
              AOE *aoe = new AOE(edge);
              result = aoe;
        }
      } break;
      case GeomAbs_BezierCurve: {
          Handle(Geom_BezierCurve) bez = adapt.Bezier();
          //if (bez->Degree() < 4) {
          result = new BezierSegment(edge);
          //}
          //    OCC is quite happy with Degree > 3 but QtGui handles only 2,3
      } break;
      case GeomAbs_BSplineCurve: {
        BSpline *bspline = 0;
        Generic* gen = NULL;
        Circle* circ = nullptr;
        AOC*    aoc  = nullptr;
        bool isArc = false;
        try {
            bspline = new BSpline(edge);
            if (bspline->isLine()) {
                gen = new Generic(edge);
                result = gen;
                delete bspline;
                bspline = nullptr;
            } else {
                TopoDS_Edge circEdge = bspline->isCircle2(isArc);
                if (!circEdge.IsNull()) {
                    if (isArc) {
                        aoc = new AOC(circEdge);
                        result = aoc;
                        delete bspline;
                        bspline = nullptr;
                    } else {
                        circ = new Circle(circEdge);
                        result = circ;
                        delete bspline;
                        bspline = nullptr;
                    }
                }else {
                    result = bspline;
                }
            }
            break;
        }
        catch (Standard_Failure&) {
            if (bspline != nullptr) {
                delete bspline;
                bspline = nullptr;
            }
            if (gen != nullptr) {
                delete gen;
                gen = nullptr;
            }
        }
      }
      default: {
        Generic *primitive = new Generic(edge);
        result = primitive;
      }  break;
    }
    return result;
}


Ellipse::Ellipse(const TopoDS_Edge &e)
{
    geomType = ELLIPSE;
    BRepAdaptor_Curve c(e);
    occEdge = e;
    gp_Elips ellp = c.Ellipse();
    const gp_Pnt &p = ellp.Location();

    center = Base::Vector2d(p.X(), p.Y());

    major = ellp.MajorRadius();
    minor = ellp.MinorRadius();

    gp_Dir xaxis = ellp.XAxis().Direction();
    angle = xaxis.AngleWithRef(gp_Dir(1, 0, 0), gp_Dir(0, 0, -1));
}


AOE::AOE(const TopoDS_Edge &e) : Ellipse(e)
{
    geomType = ARCOFELLIPSE;

    BRepAdaptor_Curve c(e);
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt ePt = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,ePt);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    startAngle = fmod(f,2.0*M_PI);
    endAngle = fmod(l,2.0*M_PI);
    cw = (a < 0) ? true: false;
    largeArc = (l-f > M_PI) ? true : false;

    startPnt = Base::Vector2d(s.X(), s.Y());
    endPnt = Base::Vector2d(ePt.X(), ePt.Y());
    midPnt = Base::Vector2d(m.X(), m.Y());
}


Circle::Circle(const TopoDS_Edge &e)
{
    geomType = CIRCLE;             //center, radius
    BRepAdaptor_Curve c(e);
    occEdge = e;

    gp_Circ circ = c.Circle();
    const gp_Pnt& p = circ.Location();

    radius = circ.Radius();
    center = Base::Vector2d(p.X(), p.Y());
}


AOC::AOC(const TopoDS_Edge &e) : Circle(e)
{
    geomType = ARCOFCIRCLE;
    BRepAdaptor_Curve c(e);

    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt ePt = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,ePt);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    startAngle = fmod(f,2.0*M_PI);
    endAngle = fmod(l,2.0*M_PI);
    cw = (a < 0) ? true: false;
    largeArc = (l-f > M_PI) ? true : false;

    startPnt = Base::Vector2d(s.X(), s.Y());
    endPnt = Base::Vector2d(ePt.X(), ePt.Y());
    midPnt = Base::Vector2d(m.X(), m.Y());
}

bool AOC::isOnArc(Base::Vector3d p)
{
    bool result = false;
    double minDist = -1.0;
    gp_Pnt pnt(p.x,p.y,p.z);
    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
    BRepExtrema_DistShapeShape extss(occEdge, v);
    if (extss.IsDone()) {
        int count = extss.NbSolution();
        if (count != 0) {
            minDist = extss.Value();
            if (minDist < Precision::Confusion()) {
                result = true;
            }
        }
    }
    return result;
}

double AOC::distToArc(Base::Vector3d p)
{
    Base::Vector2d p2(p.x,p.y);
    double result = minDist(p2);
    return result;
}


bool AOC::intersectsArc(Base::Vector3d p1,Base::Vector3d p2)
{
    bool result = false;
    double minDist = -1.0;
    gp_Pnt pnt1(p1.x,p1.y,p1.z);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(pnt1);
    gp_Pnt pnt2(p2.x,p2.y,p2.z);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(pnt2);
    BRepBuilderAPI_MakeEdge mkEdge(v1,v2);
    TopoDS_Edge line = mkEdge.Edge();
    BRepExtrema_DistShapeShape extss(occEdge, line);
    if (extss.IsDone()) {
        int count = extss.NbSolution();
        if (count != 0) {
            minDist = extss.Value();
            if (minDist < Precision::Confusion()) {
                result = true;
            }
        }
    }
    return result;
}


//! Generic is a multiline
Generic::Generic(const TopoDS_Edge &e)
{
    geomType = GENERIC;
    occEdge = e;
    BRepLib::BuildCurve3d(occEdge);

    TopLoc_Location location;
    Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(occEdge, location);

    if (!polygon.IsNull()) {
        const TColgp_Array1OfPnt &nodes = polygon->Nodes();
        for (int i = nodes.Lower(); i <= nodes.Upper(); i++){
            points.push_back(Base::Vector2d(nodes(i).X(), nodes(i).Y()));
        }
    } else {
        //no polygon representation? approximate with line
        gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(occEdge));
        points.push_back(Base::Vector2d(p.X(), p.Y()));
        p = BRep_Tool::Pnt(TopExp::LastVertex(occEdge));
        points.push_back(Base::Vector2d(p.X(), p.Y()));
    }
}


Generic::Generic()
{
    geomType = GENERIC;
}

Base::Vector2d Generic::asVector(void)
{
    Base::Vector2d result = getEndPoint() - getStartPoint();
    return result;
}

double Generic::slope(void)
{
    double slope;
    Base::Vector2d v = asVector();
    if (v.x == 0.0) {
        slope = DOUBLE_MAX;
    } else {
        slope = v.y/v.x;
    }
    return slope;
}

Base::Vector2d Generic::apparentInter(Generic* g)
{
    Base::Vector3d dir0 = DrawUtil::vector23(asVector());
    Base::Vector3d dir1 = DrawUtil::vector23(g->asVector());

    // Line Intersetion (taken from ViewProviderSketch.cpp)
    double det = dir0.x*dir1.y - dir0.y*dir1.x;
    if ((det > 0 ? det : -det) < 1e-10)
        throw Base::ValueError("Invalid selection - Det = 0");

    double c0 = dir0.y*points.at(0).x - dir0.x*points.at(0).y;
    double c1 = dir1.y*g->points.at(1).x - dir1.x*g->points.at(1).y;
    double x = (dir0.x*c1 - dir1.x*c0)/det;
    double y = (dir0.y*c1 - dir1.y*c0)/det;

    // Apparent Intersection point
    Base::Vector2d result = Base::Vector2d(x,y);
    return result;

}

BSpline::BSpline(const TopoDS_Edge &e)
{
    geomType = BSPLINE;
    BRepAdaptor_Curve c(e);
    isArc = !c.IsClosed();
    Handle(Geom_BSplineCurve) cSpline = c.BSpline();
    occEdge = e;
    Handle(Geom_BSplineCurve) spline;

    double f,l;
    f = c.FirstParameter();
    l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt ePt = c.Value(l);
    startPnt = Base::Vector2d(s.X(), s.Y());
    endPnt = Base::Vector2d(ePt.X(), ePt.Y());
    midPnt = Base::Vector2d(m.X(), m.Y());
    gp_Vec v1(m,s);
    gp_Vec v2(m,ePt);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);
    cw = (a < 0) ? true: false;

    startAngle = atan2(startPnt.y,startPnt.x);
    if (startAngle < 0) {
         startAngle += 2.0 * M_PI;
    }
    endAngle = atan2(endPnt.y,endPnt.x);
    if (endAngle < 0) {
         endAngle += 2.0 * M_PI;
    }

    Standard_Real tol3D = 0.001;                                   //1/1000 of a mm? screen can't resolve this
    Standard_Integer maxDegree = 3, maxSegment = 200;
    Handle(BRepAdaptor_HCurve) hCurve = new BRepAdaptor_HCurve(c);
    // approximate the curve using a tolerance
    //Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C2, maxSegment, maxDegree);   //gives degree == 5  ==> too many poles ==> buffer overrun
    Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C0, maxSegment, maxDegree);
    if (approx.IsDone() && approx.HasResult()) {
        spline = approx.Curve();
    } else {
        if (approx.HasResult()) {                   //result, but not within tolerance
            spline = approx.Curve();
            Base::Console().Log("Geometry::BSpline - result not within tolerance\n");
        } else {
            f = c.FirstParameter();
            l = c.LastParameter();
            s = c.Value(f);
            ePt = c.Value(l);
            Base::Console().Log("Error - Geometry::BSpline - no result- from:(%.3f,%.3f) to:(%.3f,%.3f) poles: %d\n",
                                 s.X(),s.Y(),ePt.X(),ePt.Y(),spline->NbPoles());
            TColgp_Array1OfPnt controlPoints(0,1);
            controlPoints.SetValue(0,s);
            controlPoints.SetValue(1,ePt);
            spline = GeomAPI_PointsToBSpline(controlPoints,1).Curve();
        }
    }

    GeomConvert_BSplineCurveToBezierCurve crt(spline);

    gp_Pnt controlPoint;
    for (Standard_Integer i = 1; i <= crt.NbArcs(); ++i) {
        BezierSegment tempSegment;
        Handle(Geom_BezierCurve) bezier = crt.Arc(i);
        if (bezier->Degree() > 3) {
            Base::Console().Log("Geometry::BSpline - converted curve degree > 3\n");
        }
//        Base::Console().Message("TRACE - Geo::BSpline - bezier degree: %d bezier poles: %d\n",
//                            bezier->Degree(),bezier->NbPoles());
        tempSegment.poles = bezier->NbPoles();
        tempSegment.degree = bezier->Degree();
        for (int pole = 1; pole <= tempSegment.poles; ++pole) {
            controlPoint = bezier->Pole(pole);
            tempSegment.pnts.push_back(Base::Vector2d(controlPoint.X(), controlPoint.Y()));
        }
        segments.push_back(tempSegment);
    }
}


//! Can this BSpline be represented by a straight line?
// if len(first-last) == sum(len(pi - pi+1)) then it is a line
bool BSpline::isLine()
{
    bool result = false;
    BRepAdaptor_Curve c(occEdge);

    Handle(Geom_BSplineCurve) spline = c.BSpline();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt e = c.Value(l);

    bool samePnt = s.IsEqual(e,FLT_EPSILON);
    if (!samePnt) {
        Base::Vector3d vs = DrawUtil::gpPnt2V3(s);
        Base::Vector3d ve = DrawUtil::gpPnt2V3(e);
        double endLength = (vs - ve).Length();
        int low = 0;
        int high = spline->NbPoles() - 1;
        TColgp_Array1OfPnt poles(low,high);
        spline->Poles(poles);
        double lenTotal = 0.0;
        for (int i = 0; i < high; i++) {
            gp_Pnt p1 = poles(i);
            Base::Vector3d v1 = DrawUtil::gpPnt2V3(p1);
            gp_Pnt p2 = poles(i+1);
            Base::Vector3d v2 = DrawUtil::gpPnt2V3(p2);
            lenTotal += (v2-v1).Length();
        }

        if (DrawUtil::fpCompare(lenTotal,endLength)) {
            result = true;
        }
    }
    return result;
}

//obs? used by DVDim for approximate dims
bool BSpline::isCircle()
{
    bool result = false;
    double radius;
    Base::Vector3d center;
    bool isArc = false;
    getCircleParms(result,radius,center,isArc);
//    Base::Console().Message("TRACE - GEO::BS::isCircle - result: %d radius: %.3f center: %s isArc %d\n",
//                            result,radius,DrawUtil::formatVector(center).c_str(),isArc);
    
    return result;
}

//obs? used by DVDim for approximate dims
void BSpline::getCircleParms(bool& isCircle, double& radius, Base::Vector3d& center, bool& isArc)
{
//    bool result = false;
    double curveLimit = 0.0001;
    BRepAdaptor_Curve c(occEdge);
    Handle(Geom_BSplineCurve) spline = c.BSpline();
    double f,l;
    f = c.FirstParameter();
    l = c.LastParameter();
    double parmRange = fabs(l - f);
    int testCount = 6;
    double parmStep = parmRange/testCount;
    std::vector<double> curvatures;
    std::vector<gp_Pnt> centers;
    gp_Pnt curveCenter;
    double sumCurvature = 0;
    Base::Vector3d sumCenter, valueAt;
    try {
        GeomLProp_CLProps prop(spline,f,3,Precision::Confusion());
        curvatures.push_back(prop.Curvature());
        sumCurvature += prop.Curvature();
        prop.CentreOfCurvature(curveCenter);
        centers.push_back(curveCenter);
        sumCenter += Base::Vector3d(curveCenter.X(),curveCenter.Y(),curveCenter.Z());

        for (int i = 1; i < (testCount - 1); i++) {
            prop.SetParameter(parmStep * i);
            curvatures.push_back(prop.Curvature());
            sumCurvature += prop.Curvature();
            prop.CentreOfCurvature(curveCenter);
            centers.push_back(curveCenter);
            sumCenter += Base::Vector3d(curveCenter.X(),curveCenter.Y(),curveCenter.Z());
        }
        prop.SetParameter(l);
        curvatures.push_back(prop.Curvature());
        sumCurvature += prop.Curvature();
        prop.CentreOfCurvature(curveCenter);
        centers.push_back(curveCenter);
        sumCenter += Base::Vector3d(curveCenter.X(),curveCenter.Y(),curveCenter.Z());
    }
    catch (Standard_Failure&) {
        Base::Console().Log("TechDraw - GEO::BSpline::getCircleParms - CLProps failed\n");
        isCircle = false;
        return;
//        throw Base::RuntimeError(e.GetMessageString());
    }
    Base::Vector3d avgCenter = sumCenter/testCount;
    double errorCenter = 0;
    for (auto& c: centers) {
        errorCenter += (avgCenter - Base::Vector3d(c.X(), c.Y(), c.Z())).Length();
    }
    errorCenter = errorCenter/testCount;

    double avgCurve = sumCurvature/testCount;
    double errorCurve  = 0;
    for (auto& cv: curvatures) {
        errorCurve += fabs(avgCurve - cv);    //fabs???
    }
    errorCurve  = errorCurve/testCount;

    isArc = !c.IsClosed();
    isCircle = false;
    if ( errorCurve < curveLimit ) {
        isCircle = true;
        radius = 1.0/avgCurve;
        center = avgCenter;
    }
}

// can this BSpline be a circle?
TopoDS_Edge BSpline::isCircle2(bool& arc)
{
    TopoDS_Edge result;
    BRepAdaptor_Curve c(occEdge);

    // find the two ends
    Handle(Geom_Curve) curve = c.Curve().Curve();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt e = c.Value(l);

    arc  = !c.IsClosed();
    Handle(Geom_BSplineCurve) spline = c.BSpline();

    if (spline->NbPoles() < 5) {    //need 5 poles (s-p1-pm-p2-e) for algo
        return result;              //how to do with fewer poles?
    }

    // get three points on curve (non extreme poles)
    int nb_poles = spline->NbPoles();
    gp_Pnt p1 = spline->Pole(2);          //OCC numbering starts at 1!!
    gp_Pnt p2 = spline->Pole(nb_poles-1);
    gp_Pnt pm;
    if (nb_poles == 5) {
        pm = spline->Pole(3);   //5 poles => 2.5 => 2 
    } else {
        pm = spline->Pole(nb_poles / 2);
    }

    // project three poles onto the curve
    GeomAPI_ProjectPointOnCurve proj1;
    GeomAPI_ProjectPointOnCurve proj2;
    GeomAPI_ProjectPointOnCurve projm;
    try {
        proj1.Init(p1, curve, f, l);
        proj1.Perform(p1);
        proj2.Init(p2, curve, f, l);
        proj2.Perform(p2);
        projm.Init(pm, curve, f, l);
        projm.Perform(pm);
    }
    catch(const StdFail_NotDone &e) {
        Base::Console().Log("Geometry::isCircle2 - init: %s\n",e.GetMessageString());
        return result;
    }
    if ( (proj1.NbPoints() == 0) ||
         (proj2.NbPoints() == 0) ||
         (projm.NbPoints() == 0) ) {
        return result;
    }
    gp_Pnt pc1, pc2, pcm;

    // get projected points
    try {
        pc1 = proj1.NearestPoint();
        pc2 = proj2.NearestPoint();
        pcm = projm.NearestPoint();
    }
    catch(const StdFail_NotDone &e) {
        Base::Console().Log("Geometry::isCircle2 - nearPoint: %s\n",e.GetMessageString());
        return result;
    }

    // make 2 circles and find their radii
    gce_MakeCirc gce_circ1 = gce_MakeCirc(s,pc1,pcm);   //3 point circle
    if (gce_circ1.Status() != gce_Done) {
        return result;
    }
    gp_Circ circle1 = gce_circ1.Value();
    double radius1 = circle1.Radius();
    gp_Pnt center1 = circle1.Location(); 
    Base::Vector3d vc1 = DrawUtil::gpPnt2V3(center1);

    gce_MakeCirc gce_circ2 = gce_MakeCirc(pcm,pc2,e);
    if (gce_circ2.Status() != gce_Done) {
        return result;
    }
    gp_Circ circle2 = gce_circ2.Value();
    double radius2 = circle2.Radius();
    gp_Pnt center2 = circle2.Location();
    Base::Vector3d vc2 = DrawUtil::gpPnt2V3(center2);

    // compare radii & centers
    double allowError = 0.001;           //mm^-3 good enough for printing
    double radius;
    Base::Vector3d center;
    if ( (DrawUtil::fpCompare(radius2,radius1, allowError)) &&
         (vc1.IsEqual(vc2,allowError)) ) {
        if (arc) {
            GC_MakeArcOfCircle makeArc(s,pcm,e);
            Handle(Geom_TrimmedCurve) tCurve = makeArc.Value();
            BRepBuilderAPI_MakeEdge newEdge(tCurve);
            result = newEdge;
        } else { 
            radius = (radius1 + radius2) / 2.0;
            center = (vc1 + vc2) / 2.0;
            gp_Pnt gCenter(center.x,center.y,center.z);
            gp_Ax2 stdZ(gCenter,gp_Dir(0,0,1));
            gp_Circ newCirc(stdZ,radius);
            BRepBuilderAPI_MakeEdge newEdge(newCirc);
            result = newEdge;
        }
    }
    return result;
}


bool BSpline::intersectsArc(Base::Vector3d p1,Base::Vector3d p2)
{
    bool result = false;
    double minDist = -1.0;
    gp_Pnt pnt1(p1.x,p1.y,p1.z);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(pnt1);
    gp_Pnt pnt2(p2.x,p2.y,p2.z);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(pnt2);
    BRepBuilderAPI_MakeEdge mkEdge(v1,v2);
    TopoDS_Edge line = mkEdge.Edge();
    BRepExtrema_DistShapeShape extss(occEdge, line);
    if (extss.IsDone()) {
        int count = extss.NbSolution();
        if (count != 0) {
            minDist = extss.Value();
            if (minDist < Precision::Confusion()) {
                result = true;
            }
        }
    }
    return result;
}


BezierSegment::BezierSegment(const TopoDS_Edge &e)
{
    geomType = BEZIER;
    occEdge = e;
    BRepAdaptor_Curve c(e);
    Handle(Geom_BezierCurve) bez = c.Bezier();
    poles = bez->NbPoles();
    degree = bez->Degree();
    if (poles > 4)  {
        Base::Console().Log("Warning - BezierSegment has degree > 3: %d\n",degree);
    }
    for (int i = 1; i <= poles; ++i) {
        gp_Pnt controlPoint = bez->Pole(i);
        pnts.push_back(Base::Vector2d(controlPoint.X(), controlPoint.Y()));
    }
}


//**** Vertex
Vertex::Vertex(double x, double y)
{
    pnt = Base::Vector2d(x, y);
    extractType = ExtractionType::Plain;       //obs?
    visible = false;
    ref3D = -1;                        //obs. never used.
    isCenter = false;
    BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(x,y,0.0));
    occVertex = mkVert.Vertex();
}

bool Vertex::isEqual(Vertex* v, double tol)
{
    bool result = false;
    double dist = (pnt - (v->pnt)).Length();
    if (dist <= tol) {
        result = true;
    }
    return result;
}


/*static*/
BaseGeomPtrVector GeometryUtils::chainGeoms(BaseGeomPtrVector geoms)
{
    BaseGeomPtrVector result;
    std::vector<bool> used(geoms.size(),false);

    if (geoms.empty()) {
        return result;
    }

    if (geoms.size() == 1) {
        //don't bother for single geom (circles, ellipses,etc)
        result.push_back(geoms[0]);
    } else {
        //start with first edge
        result.push_back(geoms[0]);
        Base::Vector2d atPoint = (geoms[0])->getEndPoint();
        used[0] = true;
        for (unsigned int i = 1; i < geoms.size(); i++) { //do size-1 more edges
            auto next( nextGeom(atPoint, geoms, used, Precision::Confusion()) );
            if (next.index) { //found an unused edge with vertex == atPoint
                BaseGeom* nextEdge = geoms.at(next.index);
                used[next.index] = true;
                nextEdge->reversed = next.reversed;
                result.push_back(nextEdge);
                if (next.reversed) {
                    atPoint = nextEdge->getStartPoint();
                } else {
                    atPoint = nextEdge->getEndPoint();
                }
            } else {
                Base::Console().Log("Error - Geometry::chainGeoms - couldn't find next edge\n");
                //TARFU
            }
        }
    }
    return result;
}


/*static*/ GeometryUtils::ReturnType GeometryUtils::nextGeom(
        Base::Vector2d atPoint,
        BaseGeomPtrVector geoms,
        std::vector<bool> used,
        double tolerance )
{
    ReturnType result(0, false);
    auto index(0);
    for (auto itGeom : geoms) {
        if (used[index]) {
            ++index;
            continue;
        }
        if ((atPoint - itGeom->getStartPoint()).Length() < tolerance) {
            result.index = index;
            result.reversed = false;
            break;
        } else if ((atPoint - itGeom->getEndPoint()).Length() < tolerance) {
            result.index = index;
            result.reversed = true;
            break;
        }
        ++index;
    }
    return result;
}
