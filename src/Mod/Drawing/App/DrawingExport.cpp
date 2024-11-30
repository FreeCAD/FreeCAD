/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <sstream>

#include <Approx_Curve3d.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRep_Tool.hxx>
#include <GeomConvert_BSplineCurveKnotSplitting.hxx>
#include <GeomConvert_BSplineCurveToBezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Poly_Polygon3D.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#if OCC_VERSION_HEX < 0x070600
#include <BRepAdaptor_HCurve.hxx>
#endif
#endif

#include <Base/Tools.h>
#include <Base/Vector3D.h>

#include "DrawingExport.h"


#if OCC_VERSION_HEX >= 0x070600
using BRepAdaptor_HCurve = BRepAdaptor_Curve;
#endif

using namespace Drawing;
using namespace std;

TopoDS_Edge DrawingOutput::asCircle(const BRepAdaptor_Curve& c) const
{
    double curv = 0;
    gp_Pnt pnt, center;

    try {
        // approximate the circle center from three positions
        BRepLProp_CLProps prop(c, c.FirstParameter(), 2, Precision::Confusion());
        curv += prop.Curvature();
        prop.CentreOfCurvature(pnt);
        center.ChangeCoord().Add(pnt.Coord());

        prop.SetParameter(0.5 * (c.FirstParameter() + c.LastParameter()));
        curv += prop.Curvature();
        prop.CentreOfCurvature(pnt);
        center.ChangeCoord().Add(pnt.Coord());

        prop.SetParameter(c.LastParameter());
        curv += prop.Curvature();
        prop.CentreOfCurvature(pnt);
        center.ChangeCoord().Add(pnt.Coord());

        center.ChangeCoord().Divide(3);
        curv /= 3;
    }
    catch (Standard_Failure&) {
        // if getting center of curvature fails, e.g.
        // for straight lines it raises LProp_NotDefined
        return TopoDS_Edge();
    }

    // get circle from curvature information
    double radius = 1 / curv;

    TopLoc_Location location;
    Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(c.Edge(), location);
    if (!polygon.IsNull()) {
        const TColgp_Array1OfPnt& nodes = polygon->Nodes();
        for (int i = nodes.Lower(); i <= nodes.Upper(); i++) {
            gp_Pnt p = nodes(i);
            double dist = p.Distance(center);
            if (std::abs(dist - radius) > 0.001) {
                return TopoDS_Edge();
            }
        }

        gp_Circ circ;
        circ.SetLocation(center);
        circ.SetRadius(radius);
        gp_Pnt p1 = nodes(nodes.Lower());
        gp_Pnt p2 = nodes(nodes.Upper());
        double dist = p1.Distance(p2);

        if (dist < Precision::Confusion()) {
            BRepBuilderAPI_MakeEdge mkEdge(circ);
            return mkEdge.Edge();
        }
        else {
            gp_Vec dir1(center, p1);
            dir1.Normalize();
            gp_Vec dir2(center, p2);
            dir2.Normalize();
            p1 = gp_Pnt(center.XYZ() + radius * dir1.XYZ());
            p2 = gp_Pnt(center.XYZ() + radius * dir2.XYZ());
            BRepBuilderAPI_MakeEdge mkEdge(circ, p1, p2);
            return mkEdge.Edge();
        }
    }

    return TopoDS_Edge();
}

TopoDS_Edge DrawingOutput::asBSpline(const BRepAdaptor_Curve& c, int maxDegree) const
{
    Standard_Real tol3D = 0.001;
    Standard_Integer maxSegment = 50;
    Handle(BRepAdaptor_HCurve) hCurve = new BRepAdaptor_HCurve(c);
    // approximate the curve using a tolerance
    Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C0, maxSegment, maxDegree);
    if (approx.IsDone() && approx.HasResult()) {
        // have the result
        Handle(Geom_BSplineCurve) spline = approx.Curve();
        BRepBuilderAPI_MakeEdge mkEdge(spline, spline->FirstParameter(), spline->LastParameter());
        return mkEdge.Edge();
    }

    return TopoDS_Edge();
}

SVGOutput::SVGOutput()
{}

std::string SVGOutput::exportEdges(const TopoDS_Shape& input)
{
    std::stringstream result;

    TopExp_Explorer edges(input, TopAbs_EDGE);
    for (int i = 1; edges.More(); edges.Next(), i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() == GeomAbs_Circle) {
            printCircle(adapt, result);
        }
        else if (adapt.GetType() == GeomAbs_Ellipse) {
            printEllipse(adapt, i, result);
        }
        else if (adapt.GetType() == GeomAbs_BSplineCurve) {
            //            TopoDS_Edge circle = asCircle(adapt);
            //            if (circle.IsNull()) {
            printBSpline(adapt, i, result);
            //            }
            //            else {
            //                BRepAdaptor_Curve adapt_circle(circle);
            //                printCircle(adapt_circle, result);
            //            }
        }
        else if (adapt.GetType() == GeomAbs_BezierCurve) {
            printBezier(adapt, i, result);
        }
        // fallback
        else {
            printGeneric(adapt, i, result);
        }
    }

    return result.str();
}

void SVGOutput::printCircle(const BRepAdaptor_Curve& c, std::ostream& out)
{
    gp_Circ circ = c.Circle();
    const gp_Pnt& p = circ.Location();
    double r = circ.Radius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l + f) / 2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m, s);
    gp_Vec v2(m, e);
    gp_Vec v3(0, 0, 1);
    double a = v3.DotCross(v1, v2);

    // a full circle
    if (fabs(l - f) > 1.0 && s.SquareDistance(e) < 0.001) {
        out << "<circle cx =\"" << p.X() << "\" cy =\"" << p.Y() << "\" r =\"" << r << "\" />";
    }
    // arc of circle
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        char xar = '0';                         // x-axis-rotation
        char las = (l - f > D_PI) ? '1' : '0';  // large-arc-flag
        char swp = (a < 0) ? '1' : '0';  // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() << " " << s.Y() << " A" << r << " " << r << " " << xar << " "
            << las << " " << swp << " " << e.X() << " " << e.Y() << "\" />";
    }
}

void SVGOutput::printEllipse(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    gp_Elips ellp = c.Ellipse();
    const gp_Pnt& p = ellp.Location();
    double r1 = ellp.MajorRadius();
    double r2 = ellp.MinorRadius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l + f) / 2.0);
    gp_Pnt e = c.Value(l);

    // If the minor radius is very small compared to the major radius
    // the geometry actually degenerates to a line
    double ratio = std::min(r1, r2) / std::max(r1, r2);
    if (ratio < 0.001) {
        printGeneric(c, id, out);
        return;
    }

    gp_Vec v1(m, s);
    gp_Vec v2(m, e);
    gp_Vec v3(0, 0, 1);
    double a = v3.DotCross(v1, v2);

    // a full ellipse
    // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
    gp_Dir xaxis = ellp.XAxis().Direction();
    Standard_Real angle = xaxis.AngleWithRef(gp_Dir(1, 0, 0), gp_Dir(0, 0, -1));
    angle = Base::toDegrees<double>(angle);
    if (fabs(l - f) > 1.0 && s.SquareDistance(e) < 0.001) {
        out << "<g transform = \"rotate(" << angle << "," << p.X() << "," << p.Y() << ")\">"
            << std::endl;
        out << "<ellipse cx =\"" << p.X() << "\" cy =\"" << p.Y() << "\" rx =\"" << r1
            << "\"  ry =\"" << r2 << "\"/>" << std::endl;
        out << "</g>" << std::endl;
    }
    // arc of ellipse
    else {
        char las = (l - f > D_PI) ? '1' : '0';  // large-arc-flag
        char swp = (a < 0) ? '1' : '0';  // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() << " " << s.Y() << " A" << r1 << " " << r2 << " " << angle
            << " " << las << " " << swp << " " << e.X() << " " << e.Y() << "\" />" << std::endl;
    }
}

void SVGOutput::printBezier(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    try {
        std::stringstream str;
        str << "<path d=\"M";

        Handle(Geom_BezierCurve) bezier = c.Bezier();
        Standard_Integer poles = bezier->NbPoles();

        // if it's a bezier with degree higher than 3 convert it into a B-spline
        if (bezier->Degree() > 3 || bezier->IsRational()) {
            TopoDS_Edge edge = asBSpline(c, 3);
            if (!edge.IsNull()) {
                BRepAdaptor_Curve spline(edge);
                printBSpline(spline, id, out);
            }
            else {
                Standard_Failure::Raise("do it the generic way");
            }

            return;
        }


        gp_Pnt p1 = bezier->Pole(1);
        str << p1.X() << "," << p1.Y();
        if (bezier->Degree() == 3) {
            if (poles != 4) {
                Standard_Failure::Raise("do it the generic way");
            }
            gp_Pnt p2 = bezier->Pole(2);
            gp_Pnt p3 = bezier->Pole(3);
            gp_Pnt p4 = bezier->Pole(4);
            str << " C" << p2.X() << "," << p2.Y() << " " << p3.X() << "," << p3.Y() << " "
                << p4.X() << "," << p4.Y() << " ";
        }
        else if (bezier->Degree() == 2) {
            if (poles != 3) {
                Standard_Failure::Raise("do it the generic way");
            }
            gp_Pnt p2 = bezier->Pole(2);
            gp_Pnt p3 = bezier->Pole(3);
            str << " Q" << p2.X() << "," << p2.Y() << " " << p3.X() << "," << p3.Y() << " ";
        }
        else if (bezier->Degree() == 1) {
            if (poles != 2) {
                Standard_Failure::Raise("do it the generic way");
            }
            gp_Pnt p2 = bezier->Pole(2);
            str << " L" << p2.X() << "," << p2.Y() << " ";
        }
        else {
            Standard_Failure::Raise("do it the generic way");
        }

        str << "\" />";
        out << str.str();
    }
    catch (Standard_Failure&) {
        printGeneric(c, id, out);
    }
}

void SVGOutput::printBSpline(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    try {
        std::stringstream str;
        Handle(Geom_BSplineCurve) spline;
        Standard_Real tol3D = 0.001;
        Standard_Integer maxDegree = 3, maxSegment = 100;
        Handle(BRepAdaptor_HCurve) hCurve = new BRepAdaptor_HCurve(c);
        // approximate the curve using a tolerance
        Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C0, maxSegment, maxDegree);
        if (approx.IsDone() && approx.HasResult()) {
            // have the result
            spline = approx.Curve();
        }
        else {
            printGeneric(c, id, out);
            return;
        }

        GeomConvert_BSplineCurveToBezierCurve crt(spline);
        Standard_Integer arcs = crt.NbArcs();
        str << "<path d=\"M";
        for (Standard_Integer i = 1; i <= arcs; i++) {
            Handle(Geom_BezierCurve) bezier = crt.Arc(i);
            Standard_Integer poles = bezier->NbPoles();
            if (i == 1) {
                gp_Pnt p1 = bezier->Pole(1);
                str << p1.X() << "," << p1.Y();
            }
            if (bezier->Degree() == 3) {
                if (poles != 4) {
                    Standard_Failure::Raise("do it the generic way");
                }
                gp_Pnt p2 = bezier->Pole(2);
                gp_Pnt p3 = bezier->Pole(3);
                gp_Pnt p4 = bezier->Pole(4);
                str << " C" << p2.X() << "," << p2.Y() << " " << p3.X() << "," << p3.Y() << " "
                    << p4.X() << "," << p4.Y() << " ";
            }
            else if (bezier->Degree() == 2) {
                if (poles != 3) {
                    Standard_Failure::Raise("do it the generic way");
                }
                gp_Pnt p2 = bezier->Pole(2);
                gp_Pnt p3 = bezier->Pole(3);
                str << " Q" << p2.X() << "," << p2.Y() << " " << p3.X() << "," << p3.Y() << " ";
            }
            else if (bezier->Degree() == 1) {
                if (poles != 2) {
                    Standard_Failure::Raise("do it the generic way");
                }
                gp_Pnt p2 = bezier->Pole(2);
                str << " L" << p2.X() << "," << p2.Y() << " ";
            }
            else {
                Standard_Failure::Raise("do it the generic way");
            }
        }

        str << "\" />";
        out << str.str();
    }
    catch (Standard_Failure&) {
        printGeneric(c, id, out);
    }
}

void SVGOutput::printGeneric(const BRepAdaptor_Curve& bac, int id, std::ostream& out)
{
    TopLoc_Location location;
    Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(bac.Edge(), location);
    if (!polygon.IsNull()) {
        const TColgp_Array1OfPnt& nodes = polygon->Nodes();
        char c = 'M';
        out << "<path id= \"" /*<< ViewName*/ << id << "\" d=\" ";
        for (int i = nodes.Lower(); i <= nodes.Upper(); i++) {
            out << c << " " << nodes(i).X() << " " << nodes(i).Y() << " ";
            c = 'L';
        }
        out << "\" />" << endl;
    }
    else if (bac.GetType() == GeomAbs_Line) {
        // BRep_Tool::Polygon3D assumes the edge has polygon representation - ie already been
        // "tessellated" this is not true for all edges, especially "floating edges"
        double f = bac.FirstParameter();
        double l = bac.LastParameter();
        gp_Pnt s = bac.Value(f);
        gp_Pnt e = bac.Value(l);
        char c = 'M';
        out << "<path id= \"" /*<< ViewName*/ << id << "\" d=\" ";
        out << c << " " << s.X() << " " << s.Y() << " ";
        c = 'L';
        out << c << " " << e.X() << " " << e.Y() << " ";
        out << "\" />" << endl;
    }
}

// ----------------------------------------------------------------------------

DXFOutput::DXFOutput()
{}

std::string DXFOutput::exportEdges(const TopoDS_Shape& input)
{
    std::stringstream result;

    TopExp_Explorer edges(input, TopAbs_EDGE);
    for (int i = 1; edges.More(); edges.Next(), i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() == GeomAbs_Circle) {
            printCircle(adapt, result);
        }
        else if (adapt.GetType() == GeomAbs_Ellipse) {
            printEllipse(adapt, i, result);
        }
        else if (adapt.GetType() == GeomAbs_BSplineCurve) {
            printBSpline(adapt, i, result);
        }
        // fallback
        else {
            printGeneric(adapt, i, result);
        }
    }

    return result.str();
}

void DXFOutput::printHeader(std::ostream& out)
{
    out << 0 << endl;
    out << "SECTION" << endl;
    out << 2 << endl;
    out << "ENTITIES" << endl;
}

void DXFOutput::printCircle(const BRepAdaptor_Curve& c, std::ostream& out)
{
    gp_Circ circ = c.Circle();
    // const gp_Ax1& axis = c->Axis();
    const gp_Pnt& p = circ.Location();
    double r = circ.Radius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l + f) / 2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m, s);
    gp_Vec v2(m, e);
    gp_Vec v3(0, 0, 1);
    double a = v3.DotCross(v1, v2);

    // a full circle
    if (s.SquareDistance(e) < 0.001) {
        // out << "<circle cx =\"" << p.X() << "\" cy =\""
        //<< p.Y() << "\" r =\"" << r << "\" />";
        out << 0 << endl;
        out << "CIRCLE" << endl;
        out << 8 << endl;              // Group code for layer name
        out << "sheet_layer" << endl;  // Layer number
        out << "100" << endl;
        out << "AcDbEntity" << endl;
        out << "100" << endl;
        out << "AcDbCircle" << endl;
        out << 10 << endl;     // Centre X
        out << p.X() << endl;  // X in WCS coordinates
        out << 20 << endl;
        out << p.Y() << endl;  // Y in WCS coordinates
        out << 30 << endl;
        out << 0 << endl;   // Z in WCS coordinates-leaving flat
        out << 40 << endl;  //
        out << r << endl;   // Radius
    }


    // arc of circle
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        /*char xar = '0'; // x-axis-rotation
        char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
        char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() <<  " " << s.Y()
            << " A" << r << " " << r << " "
            << xar << " " << las << " " << swp << " "
            << e.X() << " " << e.Y() << "\" />";*/
        double ax = s.X() - p.X();
        double ay = s.Y() - p.Y();
        double bx = e.X() - p.X();
        double by = e.Y() - p.Y();

        double start_angle = atan2(ay, ax) * 180 / D_PI;
        double end_angle = atan2(by, bx) * 180 / D_PI;


        if (a > 0) {
            double temp = start_angle;
            start_angle = end_angle;
            end_angle = temp;
        }
        out << 0 << endl;
        out << "ARC" << endl;
        out << 8 << endl;              // Group code for layer name
        out << "sheet_layer" << endl;  // Layer number
        out << "100" << endl;
        out << "AcDbEntity" << endl;
        out << "100" << endl;
        out << "AcDbCircle" << endl;
        out << 10 << endl;     // Centre X
        out << p.X() << endl;  // X in WCS coordinates
        out << 20 << endl;
        out << p.Y() << endl;  // Y in WCS coordinates
        out << 30 << endl;
        out << 0 << endl;   // Z in WCS coordinates
        out << 40 << endl;  //
        out << r << endl;   // Radius
        out << "100" << endl;
        out << "AcDbArc" << endl;
        out << 50 << endl;
        out << start_angle << endl;  // Start angle
        out << 51 << endl;
        out << end_angle << endl;  // End angle
    }
}

void DXFOutput::printEllipse(const BRepAdaptor_Curve& c, int /*id*/, std::ostream& out)
{
    gp_Elips ellp = c.Ellipse();
    const gp_Pnt& p = ellp.Location();
    double r1 = ellp.MajorRadius();
    double r2 = ellp.MinorRadius();
    double dp = ellp.Axis().Direction().Dot(gp_Vec(0, 0, 1));

    // a full ellipse
    /* if (s.SquareDistance(e) < 0.001) {
         out << "<ellipse cx =\"" << p.X() << "\" cy =\""
             << p.Y() << "\" rx =\"" << r1 << "\"  ry =\"" << r2 << "\"/>";
     }
     // arc of ellipse
     else {
         // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
         gp_Dir xaxis = ellp.XAxis().Direction();
         Standard_Real angle = xaxis.Angle(gp_Dir(1,0,0));
         angle = Base::toDegrees<double>(angle);
         char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
         char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
         out << "<path d=\"M" << s.X() <<  " " << s.Y()
             << " A" << r1 << " " << r2 << " "
             << angle << " " << las << " " << swp << " "
             << e.X() << " " << e.Y() << "\" />";
     }*/
    gp_Dir xaxis = ellp.XAxis().Direction();
    double angle = xaxis.AngleWithRef(gp_Dir(1, 0, 0), gp_Dir(0, 0, -1));
    // double rotation = Base::toDegrees<double>(angle);

    double start_angle = c.FirstParameter();
    double end_angle = c.LastParameter();

    double major_x;
    double major_y;

    major_x = r1 * cos(angle);
    major_y = r1 * sin(angle);

    double ratio = r2 / r1;

    if (dp < 0) {
        double temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }
    out << 0 << endl;
    out << "ELLIPSE" << endl;
    out << 8 << endl;              // Group code for layer name
    out << "sheet_layer" << endl;  // Layer number
    out << "100" << endl;
    out << "AcDbEntity" << endl;
    out << "100" << endl;
    out << "AcDbEllipse" << endl;
    out << 10 << endl;     // Centre X
    out << p.X() << endl;  // X in WCS coordinates
    out << 20 << endl;
    out << p.Y() << endl;  // Y in WCS coordinates
    out << 30 << endl;
    out << 0 << endl;        // Z in WCS coordinates
    out << 11 << endl;       //
    out << major_x << endl;  // Major X
    out << 21 << endl;
    out << major_y << endl;  // Major Y
    out << 31 << endl;
    out << 0 << endl;      // Major Z
    out << 40 << endl;     //
    out << ratio << endl;  // Ratio
    out << 41 << endl;
    out << start_angle << endl;  // Start angle
    out << 42 << endl;
    out << end_angle << endl;  // End angle
}

void DXFOutput::printBSpline(const BRepAdaptor_Curve& c,
                             int id,
                             std::ostream& out)  // Not even close yet- DF
{
    try {
        std::stringstream str;
        Handle(Geom_BSplineCurve) spline;
        Standard_Real tol3D = 0.001;
        Standard_Integer maxDegree = 3, maxSegment = 50;
        Handle(BRepAdaptor_HCurve) hCurve = new BRepAdaptor_HCurve(c);
        // approximate the curve using a tolerance
        Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C0, maxSegment, maxDegree);
        if (approx.IsDone() && approx.HasResult()) {
            // have the result
            spline = approx.Curve();
        }
        else {
            printGeneric(c, id, out);
            return;
        }

        // GeomConvert_BSplineCurveToBezierCurve crt(spline);
        // GeomConvert_BSplineCurveKnotSplitting crt(spline,0);
        // Standard_Integer arcs = crt.NbArcs();
        // Standard_Integer arcs = crt.NbSplits()-1;
        Standard_Integer m = 0;
        if (spline->IsPeriodic()) {
            m = spline->NbPoles() + 2 * spline->Degree() - spline->Multiplicity(1) + 2;
        }
        else {
            for (int i = 1; i <= spline->NbKnots(); i++) {
                m += spline->Multiplicity(i);
            }
        }
        TColStd_Array1OfReal knotsequence(1, m);
        spline->KnotSequence(knotsequence);
        TColgp_Array1OfPnt poles(1, spline->NbPoles());
        spline->Poles(poles);


        str << 0 << endl
            << "SPLINE" << endl
            << 8 << endl              // Group code for layer name
            << "sheet_layer" << endl  // Layer name
            << "100" << endl
            << "AcDbEntity" << endl
            << "100" << endl
            << "AcDbSpline" << endl
            << 70 << endl
            << spline->IsRational() * 4 << endl  // flags
            << 71 << endl
            << spline->Degree() << endl
            << 72 << endl
            << knotsequence.Length() << endl
            << 73 << endl
            << poles.Length() << endl
            << 74 << endl
            << 0 << endl;  // fitpoints

        for (int i = knotsequence.Lower(); i <= knotsequence.Upper(); i++) {
            str << 40 << endl << knotsequence(i) << endl;
        }
        for (int i = poles.Lower(); i <= poles.Upper(); i++) {
            gp_Pnt pole = poles(i);
            str << 10 << endl
                << pole.X() << endl
                << 20 << endl
                << pole.Y() << endl
                << 30 << endl
                << pole.Z() << endl;
            if (spline->IsRational()) {
                str << 41 << endl << spline->Weight(i) << endl;
            }
        }

        // str << "\" />";
        out << str.str();
    }
    catch (Standard_Failure&) {
        printGeneric(c, id, out);
    }
}

void DXFOutput::printGeneric(const BRepAdaptor_Curve& c, int /*id*/, std::ostream& out)
{
    double uStart = c.FirstParameter();
    gp_Pnt PS;
    gp_Vec VS;
    c.D1(uStart, PS, VS);

    double uEnd = c.LastParameter();
    gp_Pnt PE;
    gp_Vec VE;
    c.D1(uEnd, PE, VE);

    out << "0" << endl;
    out << "LINE" << endl;
    out << "8" << endl;            // Group code for layer name
    out << "sheet_layer" << endl;  // Layer name
    out << "100" << endl;
    out << "AcDbEntity" << endl;
    out << "100" << endl;
    out << "AcDbLine" << endl;
    out << "10" << endl;    // Start point of line
    out << PS.X() << endl;  // X in WCS coordinates
    out << "20" << endl;
    out << PS.Y() << endl;  // Y in WCS coordinates
    out << "30" << endl;
    out << "0" << endl;     // Z in WCS coordinates
    out << "11" << endl;    // End point of line
    out << PE.X() << endl;  // X in WCS coordinates
    out << "21" << endl;
    out << PE.Y() << endl;  // Y in WCS coordinates
    out << "31" << endl;
    out << "0" << endl;  // Z in WCS coordinates
}
