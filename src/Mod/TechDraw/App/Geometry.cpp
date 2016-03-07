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
# include <BRepAdaptor_Curve.hxx>
# include <Geom_Circle.hxx>
# include <gp_Circ.hxx>
# include <gp_Elips.hxx>
#endif

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <HLRBRep_Algo.hxx>
#include <TopoDS_Shape.hxx>
#include <HLRTopoBRep_OutLiner.hxx>
//#include <BRepAPI_MakeOutLine.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh.hxx>

#include <BRepAdaptor_CompCurve.hxx>
#include <Handle_BRepAdaptor_HCompCurve.hxx>
#include <Approx_Curve3d.hxx>
#include <BRepAdaptor_HCurve.hxx>
#include <Handle_BRepAdaptor_HCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Handle_Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <GeomConvert_BSplineCurveToBezierCurve.hxx>
#include <GeomConvert_BSplineCurveKnotSplitting.hxx>
#include <Geom2d_BSplineCurve.hxx>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools2D.h>
#include <Base/Vector3D.h>
#include "Geometry.h"

using namespace TechDrawGeometry;

// Collection of Geometric Features
Wire::Wire()
{

}

Wire::~Wire()
{
    for(std::vector<BaseGeom *>::iterator it = geoms.begin(); it != geoms.end(); ++it) {
        delete (*it);
        *it = 0;
    }
    geoms.clear();
}

Face::Face()
{

}

Face::~Face()
{
    for(std::vector<Wire *>::iterator it = wires.begin(); it != wires.end(); ++it) {
        delete (*it);
        *it = 0;
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
    //if (occEdge) {
        gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(occEdge));
        result.push_back(Base::Vector2D(p.X(),p.Y()));
        p = BRep_Tool::Pnt(TopExp::LastVertex(occEdge));
        result.push_back(Base::Vector2D(p.X(),p.Y()));
    //}
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

    startAngle = f;
    endAngle = l;
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

    startAngle = f;
    endAngle = l;
    cw = (a < 0) ? true: false;
    largeArc = (l-f > M_PI) ? true : false;

    startPnt = Base::Vector2D(s.X(), s.Y());
    endPnt = Base::Vector2D(ePt.X(), ePt.Y());
    midPnt = Base::Vector2D(m.X(), m.Y());
}


//!Generic is a multiline
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

//! can this BSpline be represented by a straight line?
bool BSpline::isLine()
{
    bool result = false;
    BRepAdaptor_Curve c(occEdge);
    Handle_Geom_BSplineCurve spline = c.BSpline();
    if (spline->Degree() == 1) {
        result = true;
    }
    return result;
#if 0
    bool result = true;
    std::vector<BezierSegment>::iterator iSeg = segments.begin();
    double slope;
    if ((*iSeg).poles == 2) {
        slope = ((*iSeg).pnts[1].fY - (*iSeg).pnts[0].fY) /
                ((*iSeg).pnts[1].fX - (*iSeg).pnts[0].fX);  //always at least 2 points?
    }
    for (; iSeg != segments.end(); iSeg++) {
        if ((*iSeg).poles != 2) {
            result = false;
            break;
        }
        double newSlope = ((*iSeg).pnts[1].fY - (*iSeg).pnts[0].fY) / ((*iSeg).pnts[1].fX - (*iSeg).pnts[0].fX);
        if (fabs(newSlope - slope) > Precision::Confusion()) {
            result = false;
            break;
        }
    }
    return result;
#endif
}

//**** Vertex
bool Vertex::isEqual(Vertex* v, double tol)
{
    bool result = false;
    double dist = (pnt - (v->pnt)).Length();
    if (dist <= tol) {
        result = true;
    }
    return result;
}

//****  TechDrawGeometry utility funtions

extern "C" {
//! return a vector of BaseGeom*'s in tail to nose order
std::vector<TechDrawGeometry::BaseGeom*> TechDrawExport chainGeoms(std::vector<TechDrawGeometry::BaseGeom*> geoms)
{
    std::vector<TechDrawGeometry::BaseGeom*> result;
    std::vector<bool> used(geoms.size(),false);
    double tolerance = 0.0;

    if (geoms.empty()) {
        return result;
    }

    if (geoms.size() == 1) {                                              //don't bother for single geom (circles, ellipses,etc)
        result.push_back(geoms[0]);
    } else {
        result.push_back(geoms[0]);                                    //start with first edge
        Base::Vector2D atPoint = (geoms[0])->getEndPoint();
        used[0] = true;
        for (unsigned int i = 1; i < geoms.size(); i++) {              //do size-1 more edges
            getNextReturnVal next = nextGeom(atPoint,geoms,used,tolerance);
            if (next.index) {                                          //found an unused edge with vertex == atPoint
                TechDrawGeometry::BaseGeom* nextEdge = geoms.at(next.index);
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

//! find an unused geom starts or ends at atPoint. returns index[1:geoms.size()),reversed [true,false]
getNextReturnVal TechDrawExport nextGeom(Base::Vector2D atPoint,
                       std::vector<TechDrawGeometry::BaseGeom*> geoms,
                       std::vector<bool> used,
                       double tolerance)
{
    getNextReturnVal result(0,false);
    std::vector<TechDrawGeometry::BaseGeom*>::iterator itGeom = geoms.begin();
    for (; itGeom != geoms.end(); itGeom++) {
        unsigned int index = itGeom - geoms.begin();
        if (used[index]) {
            continue;
        }
        if (atPoint == (*itGeom)->getStartPoint()) {
            result.index = index;
            result.reversed = false;
            break;
        } else if (atPoint == (*itGeom)->getEndPoint()) {
            result.index = index;
            result.reversed = true;
            break;
        }
    }
    return result;
}

} //end extern C
