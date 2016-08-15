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
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax2.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <GeomConvert_BSplineCurveToBezierCurve.hxx>
#include <Poly_Polygon3D.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <cmath>
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools2D.h>
//#include <Base/Vector3D.h>
#include "Geometry.h"

using namespace TechDrawGeometry;

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
    extractType(Plain),
    classOfEdge(ecNONE),
    visible(true),
    reversed(false),
    ref3D(-1)
{
}


std::vector<Base::Vector2D> BaseGeom::findEndPoints()
{
    std::vector<Base::Vector2D> result;

    gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(occEdge));
    result.push_back(Base::Vector2D(p.X(),p.Y()));
    p = BRep_Tool::Pnt(TopExp::LastVertex(occEdge));
    result.push_back(Base::Vector2D(p.X(),p.Y()));

    return result;
}


Base::Vector2D BaseGeom::getStartPoint()
{
    std::vector<Base::Vector2D> verts = findEndPoints();
    return verts[0];
}


Base::Vector2D BaseGeom::getEndPoint()
{
    std::vector<Base::Vector2D> verts = findEndPoints();
    return verts[1];
}


double BaseGeom::minDist(Base::Vector2D p)
{
    double minDist = -1.0;
    gp_Pnt pnt(p.fX,p.fY,0.0);
    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
    BRepExtrema_DistShapeShape extss(occEdge, v);
    if (extss.IsDone()) {
        int count = extss.NbSolution();
        if (count != 0) {
            minDist = extss.Value();
        }
    }
    return minDist;
}

//!find point on me nearest to p
Base::Vector2D BaseGeom::nearPoint(Base::Vector2D p)
{
    gp_Pnt pnt(p.fX,p.fY,0.0);
    Base::Vector2D result(0.0,0.0);
    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
    BRepExtrema_DistShapeShape extss(occEdge, v);
    if (extss.IsDone()) {
        int count = extss.NbSolution();
        if (count != 0) {
            gp_Pnt p1;
            p1 = extss.PointOnShape1(1);
            result =  Base::Vector2D(p1.X(),p1.Y());
        }
    }
    return result;
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
              //circle->extractType = extractionType;
              result = circle;
        } else {
              AOC *aoc = new AOC(edge);
              //aoc->extractType = extractionType;
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
              //ellipse->extractType = extractionType;
              result = ellipse;
        } else {
              AOE *aoe = new AOE(edge);
              //aoe->extractType = extractionType;
              result = aoe;
        }
      } break;
      case GeomAbs_BSplineCurve: {
        BSpline *bspline = 0;
        Generic* gen = NULL;
        try {
            bspline = new BSpline(edge);
            //bspline->extractType = extractionType;
            if (bspline->isLine()) {
                gen = new Generic(edge);
                //gen->extractType = extractionType;
                result = gen;
                delete bspline;
            } else {
                result = bspline;
            }
            break;
        }
        catch (Standard_Failure) {
            delete bspline;
            delete gen;
            bspline = 0;
            // Move onto generating a primitive
        }
      }
      default: {
        Generic *primitive = new Generic(edge);
        //primitive->extractType = extractionType;
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

    center = Base::Vector2D(p.X(), p.Y());

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

    startPnt = Base::Vector2D(s.X(), s.Y());
    endPnt = Base::Vector2D(ePt.X(), ePt.Y());
    midPnt = Base::Vector2D(m.X(), m.Y());
}


Circle::Circle(const TopoDS_Edge &e)
{
    geomType = CIRCLE;
    BRepAdaptor_Curve c(e);
    occEdge = e;

    gp_Circ circ = c.Circle();
    const gp_Pnt& p = circ.Location();
    //const gp_Ax2& p1 = circ.Position();
    //const gp_Pnt& l = p1.Location();

    radius = circ.Radius();
    center = Base::Vector2D(p.X(), p.Y());
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

    startPnt = Base::Vector2D(s.X(), s.Y());
    endPnt = Base::Vector2D(ePt.X(), ePt.Y());
    midPnt = Base::Vector2D(m.X(), m.Y());
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
    Base::Vector2D p2(p.x,p.y);
    double result = minDist(p2);
    return result;
//    double minDist = -1.0;
//    gp_Pnt pnt(p.x,p.y,p.z);
//    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
//    BRepExtrema_DistShapeShape extss(occEdge, v);
//    if (extss.IsDone()) {
//        int count = extss.NbSolution();
//        if (count != 0) {
//            minDist = extss.Value();
//        }
//    }
//    return minDist;
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
    Handle_Poly_Polygon3D polygon = BRep_Tool::Polygon3D(occEdge, location);

    if (!polygon.IsNull()) {
        const TColgp_Array1OfPnt &nodes = polygon->Nodes();
        for (int i = nodes.Lower(); i <= nodes.Upper(); i++){
            points.push_back(Base::Vector2D(nodes(i).X(), nodes(i).Y()));
        }
    } else {
        //no polygon representation? approximate with line?
        Base::Console().Log("INFO - Generic::Generic(edge) - polygon is NULL\n");
        gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(occEdge));
        points.push_back(Base::Vector2D(p.X(), p.Y()));
        p = BRep_Tool::Pnt(TopExp::LastVertex(occEdge));
        points.push_back(Base::Vector2D(p.X(), p.Y()));
    }
}


Generic::Generic()
{
    geomType = GENERIC;
}


BSpline::BSpline(const TopoDS_Edge &e)
{
    geomType = BSPLINE;
    BRepAdaptor_Curve c(e);
    occEdge = e;
    Handle_Geom_BSplineCurve spline = c.BSpline();

    if (spline->Degree() > 3) {                                        //if spline is too complex, approximate it
        Standard_Real tol3D = 0.001;
        Standard_Integer maxDegree = 3, maxSegment = 10;
        Handle_BRepAdaptor_HCurve hCurve = new BRepAdaptor_HCurve(c);
        // approximate the curve using a tolerance
        //Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C2, maxSegment, maxDegree);   //gives degree == 5  ==> too many poles ==> buffer overrun
        Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C0, maxSegment, maxDegree);
        if (approx.IsDone() && approx.HasResult()) {
            spline = approx.Curve();
        } else {
            throw Base::Exception("Geometry::BSpline - could not approximate curve");
        }
    }

    GeomConvert_BSplineCurveToBezierCurve crt(spline);

    BezierSegment tempSegment;
    gp_Pnt controlPoint;

    for (Standard_Integer i = 1; i <= crt.NbArcs(); ++i) {
        Handle_Geom_BezierCurve bezier = crt.Arc(i);
        if (bezier->Degree() > 3) {
            throw Base::Exception("Geometry::BSpline - converted curve degree > 3");
        }
        tempSegment.poles = bezier->NbPoles();
        // Note: We really only need to keep the pnts[0] for the first Bezier segment,
        // assuming this only gets used as in QGIViewPart::drawPainterPath
        // ...it also gets used in GeometryObject::calcBoundingBox(), similar note applies
        for (int pole = 1; pole <= tempSegment.poles; ++pole) {
            controlPoint = bezier->Pole(pole);
            tempSegment.pnts[pole - 1] = Base::Vector2D(controlPoint.X(), controlPoint.Y());
        }
        segments.push_back(tempSegment);
    }
}


//! Can this BSpline be represented by a straight line?
bool BSpline::isLine()
{
    bool result = false;
    BRepAdaptor_Curve c(occEdge);
    Handle_Geom_BSplineCurve spline = c.BSpline();
    if (spline->Degree() == 1) {
        result = true;
    }
    return result;
}


//**** Vertex
Vertex::Vertex(double x, double y)
{
    pnt = Base::Vector2D(x, y);
    extractType = ExtractionType::Plain;       //obs?
    visible = false;
    ref3D = -1;                        //obs. never used.
    isCenter = false;
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
        Base::Vector2D atPoint = (geoms[0])->getEndPoint();
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
        Base::Vector2D atPoint,
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
