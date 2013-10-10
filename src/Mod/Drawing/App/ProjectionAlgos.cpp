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
#include <GeomConvert_BSplineCurveKnotSplitting.hxx>
#include <Geom2d_BSplineCurve.hxx>

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include <Mod/Part/App/PartFeature.h>

#include "ProjectionAlgos.h"
#include "DrawingExport.h"

using namespace Drawing;
using namespace std;

//===========================================================================
// ProjectionAlgos
//===========================================================================



ProjectionAlgos::ProjectionAlgos(const TopoDS_Shape &Input, const Base::Vector3d &Dir) 
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

    try {
        gp_Ax2 transform(gp_Pnt(0,0,0),gp_Dir(Direction.x,Direction.y,Direction.z));
        HLRAlgo_Projector projector( transform );
        brep_hlr->Projector(projector);
        brep_hlr->Update();
        brep_hlr->Hide();
    }
    catch (...) {
        Standard_Failure::Raise("Fatal error occurred while projecting shape");
    }

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

std::string ProjectionAlgos::getSVG(ExtractionType type, double scale, double tolerance)
{
    std::stringstream result;
    SVGOutput output;
    float hfactor = 0.5f; // hidden line size factor, was 0.15f / 0.35f;

    if (!H.IsNull() && (type & WithHidden)) {
        double width = hfactor * scale;
        BRepMesh::Mesh(H,tolerance);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"0.2,0.1\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << output.exportEdges(H)
                << "</g>" << endl;
    }
    if (!HO.IsNull() && (type & WithHidden)) {
        double width = hfactor * scale;
        BRepMesh::Mesh(HO,tolerance);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"0.02,0.1\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << output.exportEdges(HO)
                << "</g>" << endl;
    }
    if (!VO.IsNull()) {
        double width = scale;
        BRepMesh::Mesh(VO,tolerance);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << output.exportEdges(VO)
                << "</g>" << endl;
    }
    if (!V.IsNull()) {
        double width = scale;
        BRepMesh::Mesh(V,tolerance);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << output.exportEdges(V)
                << "</g>" << endl;
    }
    if (!V1.IsNull() && (type & WithSmooth)) {
        double width = scale;
        BRepMesh::Mesh(V1,tolerance);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << output.exportEdges(V1)
                << "</g>" << endl;
    }
    if (!H1.IsNull() && (type & WithSmooth) && (type & WithHidden)) {
        double width = hfactor * scale;
        BRepMesh::Mesh(H1,tolerance);
        result  << "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"0.09,0.05\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl
                << output.exportEdges(H1)
                << "</g>" << endl;
    }
        /*result << "0"          << endl
        << "SECTION"  << endl
        << "2"          << endl
        << "ENTITIES" << endl;*/
    return result.str();
}

/* dxf output section - Dan Falck 2011/09/25  */

std::string ProjectionAlgos::getDXF(ExtractionType type, double scale, double tolerance)
{
    std::stringstream result;
    DXFOutput output;
    
    result << "0"          << endl
        << "SECTION"  << endl

        << "2"          << endl
        << "ENTITIES" << endl;

    if (!H.IsNull() && (type & WithHidden)) {
        //float width = 0.15f/scale;
        BRepMesh::Mesh(H,tolerance);
        result  //<< "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                /*<< "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"5 3\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl*/
                << output.exportEdges(H);
                //<< "</g>" << endl;
    }
    if (!HO.IsNull() && (type & WithHidden)) {
        //float width = 0.15f/scale;
        BRepMesh::Mesh(HO,tolerance);
        result  //<< "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                /*<< "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"5 3\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl*/
                << output.exportEdges(HO);
                //<< "</g>" << endl;
    }
    if (!VO.IsNull()) {
        //float width = 0.35f/scale;
        BRepMesh::Mesh(VO,tolerance);
        result  //<< "<g" 
                //<< " id=\"" << ViewName << "\"" << endl

                /*<< "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl*/

                << output.exportEdges(VO);
                //<< "</g>" << endl;
    }
    if (!V.IsNull()) {
        //float width = 0.35f/scale;
        BRepMesh::Mesh(V,tolerance);
        result  //<< "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                /*<< "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl

                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl*/
                << output.exportEdges(V);
                //<< "</g>" << endl;

    }
    if (!V1.IsNull() && (type & WithSmooth)) {
        //float width = 0.35f/scale;
        BRepMesh::Mesh(V1,tolerance);
        result  //<< "<g" 

                //<< " id=\"" << ViewName << "\"" << endl
               /* << "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl
                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   fill=\"none\"" << endl

                << "  >" << endl*/
                << output.exportEdges(V1);
                //<< "</g>" << endl;
    }
    if (!H1.IsNull() && (type & WithSmooth) && (type & WithHidden)) {
        //float width = 0.15f/scale;
        BRepMesh::Mesh(H1,tolerance);
        result  //<< "<g" 
                //<< " id=\"" << ViewName << "\"" << endl
                /*<< "   stroke=\"rgb(0, 0, 0)\"" << endl 
                << "   stroke-width=\"" << width << "\"" << endl

                << "   stroke-linecap=\"butt\"" << endl
                << "   stroke-linejoin=\"miter\"" << endl
                << "   stroke-dasharray=\"5 3\"" << endl
                << "   fill=\"none\"" << endl
                << "  >" << endl*/

                << output.exportEdges(H1);
                //<< "</g>" << endl;
    }


    result      << 0          << endl
                << "ENDSEC"   << endl
                << 0          << endl
                << "EOF";

    return result.str();
}
