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
#include <sstream>

#include <BRepLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#endif

#include <Base/Exception.h>

#include "DrawingExport.h"
#include "ProjectionAlgos.h"


using namespace Drawing;
using namespace std;

//===========================================================================
// ProjectionAlgos
//===========================================================================


ProjectionAlgos::ProjectionAlgos(const TopoDS_Shape& Input, const Base::Vector3d& Dir)
    : Input(Input)
    , Direction(Dir)
{
    execute();
}

ProjectionAlgos::~ProjectionAlgos()
{}

// added by tanderson. aka blobfish.
// projection algorithms build a 2d curve(pcurve) but no 3d curve.
// this causes problems with meshing algorithms after save and load.
static const TopoDS_Shape& build3dCurves(const TopoDS_Shape& shape)
{
    TopExp_Explorer it;
    for (it.Init(shape, TopAbs_EDGE); it.More(); it.Next()) {
        BRepLib::BuildCurve3d(TopoDS::Edge(it.Current()));
    }
    return shape;
}

void ProjectionAlgos::execute(void)
{
    Handle(HLRBRep_Algo) brep_hlr = new HLRBRep_Algo;
    brep_hlr->Add(Input);

    gp_Ax2 transform(gp_Pnt(0, 0, 0), gp_Dir(Direction.x, Direction.y, Direction.z));
    HLRAlgo_Projector projector(transform);
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();

    // extracting the result sets:
    HLRBRep_HLRToShape shapes(brep_hlr);

    V = build3dCurves(shapes.VCompound());          // hard edge visibly
    V1 = build3dCurves(shapes.Rg1LineVCompound());  // Smoth edges visibly
    VN = build3dCurves(shapes.RgNLineVCompound());  // contour edges visibly
    VO = build3dCurves(shapes.OutLineVCompound());  // contours apparents visibly
    VI = build3dCurves(shapes.IsoLineVCompound());  // isoparamtriques   visibly
    H = build3dCurves(shapes.HCompound());          // hard edge       invisibly
    H1 = build3dCurves(shapes.Rg1LineHCompound());  // Smoth edges  invisibly
    HN = build3dCurves(shapes.RgNLineHCompound());  // contour edges invisibly
    HO = build3dCurves(shapes.OutLineHCompound());  // contours apparents invisibly
    HI = build3dCurves(shapes.IsoLineHCompound());  // isoparamtriques   invisibly
}

string ProjectionAlgos::getSVG(ExtractionType type,
                               double tolerance,
                               XmlAttributes V_style,
                               XmlAttributes V0_style,
                               XmlAttributes V1_style,
                               XmlAttributes H_style,
                               XmlAttributes H0_style,
                               XmlAttributes H1_style)
{
    stringstream result;
    SVGOutput output;

    if (!H.IsNull() && (type & WithHidden)) {
        H_style.insert({"stroke", "rgb(0, 0, 0)"});
        H_style.insert({"stroke-width", "0.15"});
        H_style.insert({"stroke-linecap", "butt"});
        H_style.insert({"stroke-linejoin", "miter"});
        H_style.insert({"stroke-dasharray", "0.2,0.1)"});
        H_style.insert({"fill", "none"});
        H_style.insert({"transform", "scale(1,-1)"});
        BRepMesh_IncrementalMesh(H, tolerance);
        result << "<g";
        for (const auto& attribute : H_style) {
            result << "   " << attribute.first << "=\"" << attribute.second << "\"\n";
        }
        result << "  >" << endl << output.exportEdges(H) << "</g>" << endl;
    }
    if (!HO.IsNull() && (type & WithHidden)) {
        H0_style.insert({"stroke", "rgb(0, 0, 0)"});
        H0_style.insert({"stroke-width", "0.15"});
        H0_style.insert({"stroke-linecap", "butt"});
        H0_style.insert({"stroke-linejoin", "miter"});
        H0_style.insert({"stroke-dasharray", "0.02,0.1)"});
        H0_style.insert({"fill", "none"});
        H0_style.insert({"transform", "scale(1,-1)"});
        BRepMesh_IncrementalMesh(HO, tolerance);
        result << "<g";
        for (const auto& attribute : H0_style) {
            result << "   " << attribute.first << "=\"" << attribute.second << "\"\n";
        }
        result << "  >" << endl << output.exportEdges(HO) << "</g>" << endl;
    }
    if (!VO.IsNull()) {
        V0_style.insert({"stroke", "rgb(0, 0, 0)"});
        V0_style.insert({"stroke-width", "1.0"});
        V0_style.insert({"stroke-linecap", "butt"});
        V0_style.insert({"stroke-linejoin", "miter"});
        V0_style.insert({"fill", "none"});
        V0_style.insert({"transform", "scale(1,-1)"});
        BRepMesh_IncrementalMesh(VO, tolerance);
        result << "<g";
        for (const auto& attribute : V0_style) {
            result << "   " << attribute.first << "=\"" << attribute.second << "\"\n";
        }
        result << "  >" << endl << output.exportEdges(VO) << "</g>" << endl;
    }
    if (!V.IsNull()) {
        V_style.insert({"stroke", "rgb(0, 0, 0)"});
        V_style.insert({"stroke-width", "1.0"});
        V_style.insert({"stroke-linecap", "butt"});
        V_style.insert({"stroke-linejoin", "miter"});
        V_style.insert({"fill", "none"});
        V_style.insert({"transform", "scale(1,-1)"});
        BRepMesh_IncrementalMesh(V, tolerance);
        result << "<g";
        for (const auto& attribute : V_style) {
            result << "   " << attribute.first << "=\"" << attribute.second << "\"\n";
        }
        result << "  >" << endl << output.exportEdges(V) << "</g>" << endl;
    }
    if (!V1.IsNull() && (type & WithSmooth)) {
        V1_style.insert({"stroke", "rgb(0, 0, 0)"});
        V1_style.insert({"stroke-width", "1.0"});
        V1_style.insert({"stroke-linecap", "butt"});
        V1_style.insert({"stroke-linejoin", "miter"});
        V1_style.insert({"fill", "none"});
        V1_style.insert({"transform", "scale(1,-1)"});
        BRepMesh_IncrementalMesh(V1, tolerance);
        result << "<g";
        for (const auto& attribute : V1_style) {
            result << "   " << attribute.first << "=\"" << attribute.second << "\"\n";
        }
        result << "  >" << endl << output.exportEdges(V1) << "</g>" << endl;
    }
    if (!H1.IsNull() && (type & WithSmooth) && (type & WithHidden)) {
        H1_style.insert({"stroke", "rgb(0, 0, 0)"});
        H1_style.insert({"stroke-width", "0.15"});
        H1_style.insert({"stroke-linecap", "butt"});
        H1_style.insert({"stroke-linejoin", "miter"});
        H1_style.insert({"stroke-dasharray", "0.09,0.05)"});
        H1_style.insert({"fill", "none"});
        H1_style.insert({"transform", "scale(1,-1)"});
        BRepMesh_IncrementalMesh(H1, tolerance);
        result << "<g";
        for (const auto& attribute : H1_style) {
            result << "   " << attribute.first << "=\"" << attribute.second << "\"\n";
        }
        result << "  >" << endl << output.exportEdges(H1) << "</g>" << endl;
    }
    return result.str();
}

/* dxf output section - Dan Falck 2011/09/25  */

string ProjectionAlgos::getDXF(ExtractionType type, double /*scale*/, double tolerance)
{
    stringstream result;
    DXFOutput output;

    if (!H.IsNull() && (type & WithHidden)) {
        // float width = 0.15f/scale;
        BRepMesh_IncrementalMesh(H, tolerance);
        result << output.exportEdges(H);
    }
    if (!HO.IsNull() && (type & WithHidden)) {
        // float width = 0.15f/scale;
        BRepMesh_IncrementalMesh(HO, tolerance);
        result << output.exportEdges(HO);
    }
    if (!VO.IsNull()) {
        // float width = 0.35f/scale;
        BRepMesh_IncrementalMesh(VO, tolerance);
        result << output.exportEdges(VO);
    }
    if (!V.IsNull()) {
        // float width = 0.35f/scale;
        BRepMesh_IncrementalMesh(V, tolerance);
        result << output.exportEdges(V);
    }
    if (!V1.IsNull() && (type & WithSmooth)) {
        // float width = 0.35f/scale;
        BRepMesh_IncrementalMesh(V1, tolerance);
        result << output.exportEdges(V1);
    }
    if (!H1.IsNull() && (type & WithSmooth) && (type & WithHidden)) {
        // float width = 0.15f/scale;
        BRepMesh_IncrementalMesh(H1, tolerance);
        result << output.exportEdges(H1);
    }

    return result.str();
}
