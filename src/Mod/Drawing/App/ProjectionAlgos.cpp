/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <sstream>
# include <BRepAdaptor_Curve.hxx>
# include <Geom_Circle.hxx>
# include <gp_Circ.hxx>
# include <gp_Elips.hxx>
#endif

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
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


#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include <Mod/Part/App/PartFeature.h>

#include "ProjectionAlgos.h"

using namespace Drawing;
using namespace std;


//===========================================================================
// ProjectionAlgos
//===========================================================================



ProjectionAlgos::ProjectionAlgos(const TopoDS_Shape &Input, const Base::Vector3f &Dir) 
  : Input(Input), Direction(Dir)
{
    execute();
}

ProjectionAlgos::~ProjectionAlgos()
{
}

TopoDS_Shape ProjectionAlgos::invertY(const TopoDS_Shape& shape)
{
    // make sure to have the y coordinates inverted
    gp_Trsf mat;
    Bnd_Box bounds;
    BRepBndLib::Add(shape, bounds);
    bounds.SetGap(0.0);
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    mat.SetMirror(gp_Ax2(gp_Pnt((xMin+xMax)/2,(yMin+yMax)/2,(zMin+zMax)/2), gp_Dir(0,1,0)));
    BRepBuilderAPI_Transform mkTrf(shape, mat);
    return mkTrf.Shape();
}

void ProjectionAlgos::execute(void)
{
    Handle( HLRBRep_Algo ) brep_hlr = new HLRBRep_Algo;
    brep_hlr->Add(Input);

    gp_Ax2 transform(gp_Pnt(0,0,0),gp_Dir(Direction.x,Direction.y,Direction.z));
    HLRAlgo_Projector projector( transform );
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();

    // extracting the result sets:
    HLRBRep_HLRToShape shapes( brep_hlr );

    V  = shapes.VCompound       ();// hard edge visibly
    V1 = shapes.Rg1LineVCompound();// Smoth edges visibly
    VN = shapes.RgNLineVCompound();// contour edges visibly
    VO = shapes.OutLineVCompound();// contours apparents visibly
    VI = shapes.IsoLineVCompound();// isoparamtriques   visibly
    H  = shapes.HCompound       ();// hard edge       invisibly
    H1 = shapes.Rg1LineHCompound();// Smoth edges  invisibly
    HN = shapes.RgNLineHCompound();// contour edges invisibly
    HO = shapes.OutLineHCompound();// contours apparents invisibly
    HI = shapes.IsoLineHCompound();// isoparamtriques   invisibly

}

std::string ProjectionAlgos::getSVG(SvgExtractionType type, float scale)
{
    std::stringstream result;
    if (!H.IsNull() && (type & WithHidden)) {
        float width = 0.15f/scale;
        BRepMesh::Mesh(H,0.1);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"5 3\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << Edges2SVG(H)
                << "</g>" << endl;
    }
    if (!HO.IsNull() && (type & WithHidden)) {
        float width = 0.15f/scale;
        BRepMesh::Mesh(HO,0.1);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"5 3\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << Edges2SVG(HO)
                << "</g>" << endl;
    }
    if (!VO.IsNull()) {
        float width = 0.35f/scale;
        BRepMesh::Mesh(VO,0.1);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << Edges2SVG(VO)
                << "</g>" << endl;
    }
    if (!V.IsNull()) {
        float width = 0.35f/scale;
        BRepMesh::Mesh(V,0.1);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << Edges2SVG(V)
                << "</g>" << endl;
    }
    if (!V1.IsNull() && (type & WithSmooth)) {
        float width = 0.35f/scale;
        BRepMesh::Mesh(V1,0.1);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << Edges2SVG(V1)
                << "</g>" << endl;
    }
    if (!H1.IsNull() && (type & WithSmooth) && (type & WithHidden)) {
        float width = 0.15f/scale;
        BRepMesh::Mesh(H1,0.1);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"5 3\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << Edges2SVG(H1)
                << "</g>" << endl;
    }

    return result.str();
}

std::string ProjectionAlgos::Edges2SVG(const TopoDS_Shape &Input)
{
    std::stringstream result;

    TopExp_Explorer edges( Input, TopAbs_EDGE );
    for (int i = 1 ; edges.More(); edges.Next(),i++ ) {
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

void ProjectionAlgos::printGeneric(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    TopLoc_Location location;
    Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(c.Edge(), location);
    if (!polygon.IsNull()) {
        const TColgp_Array1OfPnt& nodes = polygon->Nodes();
        char c = 'M';
        out << "<path id= \"" /*<< ViewName*/ << id << "\" d=\" "; 
        for (int i = nodes.Lower(); i <= nodes.Upper(); i++){
            out << c << " " << nodes(i).X() << " " << nodes(i).Y()<< " " ; 
            c = 'L';
        }
        out << "\" />" << endl;
    }
}

void ProjectionAlgos::printCircle(const BRepAdaptor_Curve& c, std::ostream& out)
{
    gp_Circ circ = c.Circle();
    const gp_Pnt& p= circ.Location();
    double r = circ.Radius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    // a full circle
    if (s.SquareDistance(e) < 0.001) {
        out << "<circle cx =\"" << p.X() << "\" cy =\"" 
            << p.Y() << "\" r =\"" << r << "\" />";
    }
    // arc of circle
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        char xar = '0'; // x-axis-rotation
        char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
        char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() <<  " " << s.Y()
            << " A" << r << " " << r << " "
            << xar << " " << las << " " << swp << " "
            << e.X() << " " << e.Y() << "\" />";
    }
}

void ProjectionAlgos::printEllipse(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    gp_Elips ellp = c.Ellipse();
    const gp_Pnt& p= ellp.Location();
    double r1 = ellp.MajorRadius();
    double r2 = ellp.MinorRadius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    // a full ellipse
    if (s.SquareDistance(e) < 0.001) {
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
    }
}

void ProjectionAlgos::printBSpline(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    try {
        std::stringstream str;
        Handle_Geom_BSplineCurve spline = c.BSpline();
        if (spline->Degree() > 3) {
            Standard_Real tol3D = 0.001;
            Standard_Integer maxDegree = 3, maxSegment = 10;
            Handle_BRepAdaptor_HCurve hCurve = new BRepAdaptor_HCurve(c);
            // approximate the curve using a tolerance
            Approx_Curve3d approx(hCurve,tol3D,GeomAbs_C2,maxSegment,maxDegree);
            if (approx.IsDone() && approx.HasResult()) {
                // have the result
                spline = approx.Curve();
            }
        }

        GeomConvert_BSplineCurveToBezierCurve crt(spline);
        Standard_Integer arcs = crt.NbArcs();
        str << "<path d=\"M";
        for (Standard_Integer i=1; i<=arcs; i++) {
            Handle_Geom_BezierCurve bezier = crt.Arc(i);
            Standard_Integer poles = bezier->NbPoles();
            if (bezier->Degree() == 3) {
                if (poles != 4)
                    Standard_Failure::Raise("do it the generic way");
                gp_Pnt p1 = bezier->Pole(1);
                gp_Pnt p2 = bezier->Pole(2);
                gp_Pnt p3 = bezier->Pole(3);
                gp_Pnt p4 = bezier->Pole(4);
                if (i == 1) {
                    str << p1.X() << "," << p1.Y() << " C"
                        << p2.X() << "," << p2.Y() << " "
                        << p3.X() << "," << p3.Y() << " "
                        << p4.X() << "," << p4.Y() << " ";
                }
                else {
                    str << "S"
                        << p3.X() << "," << p3.Y() << " "
                        << p4.X() << "," << p4.Y() << " ";
                }
            }
            else if (bezier->Degree() == 2) {
                if (poles != 3)
                    Standard_Failure::Raise("do it the generic way");
                gp_Pnt p1 = bezier->Pole(1);
                gp_Pnt p2 = bezier->Pole(2);
                gp_Pnt p3 = bezier->Pole(3);
                if (i == 1) {
                    str << p1.X() << "," << p1.Y() << " Q"
                        << p2.X() << "," << p2.Y() << " "
                        << p3.X() << "," << p3.Y() << " ";
                }
                else {
                    str << "T"
                        << p3.X() << "," << p3.Y() << " ";
                }
            }
            else {
                Standard_Failure::Raise("do it the generic way");
            }
        }

        str << "\" />";
        out << str.str();
    }
    catch (Standard_Failure) {
        printGeneric(c, id, out);
    }
}
