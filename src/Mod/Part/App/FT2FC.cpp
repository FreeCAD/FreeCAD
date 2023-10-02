/***************************************************************************
 *   Copyright (c) 2013 WandererFan <wandererfan (at) gmail.com>           *
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
/***************************************************************************
 *  FreeType License (FTL) credit:                                         *
 *  Portions of this software are copyright (c) <1996-2011> The FreeType   *
 *  Project (www.freetype.org).  All rights reserved.                      *
 ***************************************************************************/

#include "PreCompiled.h"

#ifdef FCUseFreeType

#ifndef _PreComp_
# include <iostream>
# include <fstream>
# include <cstdio>
# include <cstdlib>
# include <stdexcept>
# include <vector>

# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepLib.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <Geom_Plane.hxx>
# include <Geom2d_BezierCurve.hxx>
# include <Geom2d_BSplineCurve.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <gp_Pnt.hxx>
# include <gp_Trsf.hxx>
# include <gp_Vec.hxx>
# include <Precision.hxx>
# include <ShapeConstruct_Curve.hxx>
# include <TColgp_Array1OfPnt2d.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Wire.hxx>
#endif // _PreComp

#include <Base/Console.h>
#include <Base/FileInfo.h>

#include "TopoShapeWirePy.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include FT_TYPES_H

#include "FT2FC.h"

#define CLOCKWISE 0
#define ANTICLOCKWISE 1


using namespace Part;

using UNICHAR = unsigned long;           // ul is FT2's codepoint type <=> Py_UNICODE2/4

// Private function prototypes
PyObject* getGlyphContours(FT_Face FTFace, UNICHAR currchar, double PenPos, double Scale,int charNum, double tracking);
FT_Vector getKerning(FT_Face FTFace, UNICHAR lc, UNICHAR rc);
TopoDS_Wire edgesToWire(std::vector<TopoDS_Edge> Edges);
int calcClockDir(std::vector<Base::Vector3d> points);

// for compatibility with old version - separate path & filename
PyObject* FT2FC(const Py_UNICODE *PyUString,
                const size_t length,
                const char *FontPath,
                const char *FontName,
                const double stringheight,
                const double tracking) {
   std::string FontSpec;
   std::string tmpPath = FontPath;              // can't concat const char*
   std::string tmpName = FontName;
   FontSpec = tmpPath + tmpName;
   return (FT2FC(PyUString,length,FontSpec.c_str(),stringheight,tracking));
}

// get string's wires (contours) in FC/OCC coords
PyObject* FT2FC(const Py_UNICODE *PyUString,
                const size_t length,
                const char *FontSpec,
                const double stringheight,                 // fc coords
                const double tracking)                     // fc coords
{
    FT_Library  FTLib;
    FT_Face     FTFace;
    FT_Error    error;
    FT_Long     FaceIndex = 0;                   // some fonts have multiple faces
    FT_Vector   kern;
    FT_UInt     FTLoadFlags = FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP;

    std::stringstream ErrorMsg;
    double PenPos = 0, scalefactor;
    UNICHAR prevchar = 0, currchar = 0;
    int  cadv;
    size_t i;
    Py::List CharList;

    error = FT_Init_FreeType(&FTLib);
    if (error) {
        ErrorMsg << "FT_Init_FreeType failed: " << error;
        throw std::runtime_error(ErrorMsg.str());
    }


    std::ifstream fontfile;
#ifdef FC_OS_WIN32
    Base::FileInfo winFI(FontSpec);
    fontfile.open(winFI.toStdWString().c_str(), std::ios::binary | std::ios::in);
#else
    fontfile.open(FontSpec, std::ios::binary | std::ios::in);
#endif
    if (!fontfile.is_open()) {
        //get indignant
        ErrorMsg << "Can not open font file: " << FontSpec;
        throw std::runtime_error(ErrorMsg.str());
    }
    fontfile.seekg (0, fontfile.end);
    int bytesNeeded = fontfile.tellg();
    fontfile.clear();
    fontfile.seekg (0, fontfile.beg);
    std::unique_ptr <char[]> buffer (new char[bytesNeeded]);
    fontfile.read(buffer.get(), bytesNeeded);
    if (!fontfile) {
        //get indignant
        ErrorMsg << "Can not read font file: " << FontSpec;
        throw std::runtime_error(ErrorMsg.str());
    }
    fontfile.close();

    const FT_Byte* ftBuffer = reinterpret_cast<FT_Byte*>(buffer.get());
    error = FT_New_Memory_Face(FTLib, ftBuffer, bytesNeeded, FaceIndex, &FTFace);
    if (error) {
        ErrorMsg << "FT_New_Face failed: " << error;
        throw std::runtime_error(ErrorMsg.str());
    }

    //TODO: check that FTFace is scalable?  only relevant for hinting etc?

    //  FT2 blows up if char size is not set to some non-zero value.
    //  This sets size to 48 point. Magic.
    error = FT_Set_Char_Size(FTFace,
                             0,             /* char_width in 1/64th of points */
                             48*64*10,      /* char_height in 1/64th of points */ // increased 10X to preserve very small details
                             0,             /* horizontal device resolution */
                             0 );           /* vertical device resolution */
    if (error) {
        ErrorMsg << "FT_Set_Char_Size failed: " << error;
        throw std::runtime_error(ErrorMsg.str());
    }

    scalefactor = (stringheight/float(FTFace->height))/10;  // divide scale by 10 to offset the 10X increased scale in FT_Set_Char_Size above
    for (i=0; i<length; i++) {
        currchar = PyUString[i];
        error = FT_Load_Char(FTFace,
                             currchar,
                             FTLoadFlags);
        if (error) {
            ErrorMsg << "FT_Load_Char failed: " << error;
            throw std::runtime_error(ErrorMsg.str());
        }

        cadv = FTFace->glyph->advance.x;
        kern = getKerning(FTFace,prevchar,currchar);
        PenPos += kern.x;
        try {
            Py::List WireList(getGlyphContours(FTFace, currchar, PenPos, scalefactor, i, tracking), true);
            CharList.append(WireList);
        }
        catch (Py::Exception& e) {
            e.clear();
            Base::Console().Log("FT2FC char '0x%04x'/'%d' has no Wires!\n", currchar, currchar);
        }

        PenPos += cadv;
        prevchar = currchar;
    }

    error = FT_Done_FreeType(FTLib);
    if (error) {
        ErrorMsg << "FT_Done_FreeType failed: " << error;
        throw std::runtime_error(ErrorMsg.str());
    }

    return Py::new_reference_to(CharList);
}

//********** FT Decompose callbacks and data defns
// FT Decomp Context for 1 char
struct FTDC_Ctx {
  std::vector<TopoDS_Wire> Wires;
  std::vector<int> wDir;
  std::vector<TopoDS_Edge> Edges;
  std::vector<Base::Vector3d> polyPoints;
  UNICHAR currchar;
  FT_Vector LastVert;
  Handle(Geom_Surface) surf;
};

// move_cb called for start of new contour. pt is xy of contour start.
// p points to the context where we remember what happened previously (last point, etc)
static int move_cb(const FT_Vector* pt, void* p) {
   FTDC_Ctx* dc = static_cast<FTDC_Ctx*>(p);
   if (!dc->Edges.empty()){
       TopoDS_Wire newwire = edgesToWire(dc->Edges);
       dc->Wires.push_back(newwire);
       dc->Edges.clear();
       dc->wDir.push_back(calcClockDir(dc->polyPoints));
       dc->polyPoints.clear();
   }
   dc->LastVert = *pt;
   if (dc->polyPoints.empty()) {
        dc->polyPoints.emplace_back(pt->x, pt->y, 0.0);
   }

   return 0;
}

// line_cb called for line segment in the current contour: line(LastVert -- pt)
static int line_cb(const FT_Vector* pt, void* p) {
   FTDC_Ctx* dc = static_cast<FTDC_Ctx*>(p);
   gp_Pnt2d v1(dc->LastVert.x, dc->LastVert.y);
   gp_Pnt2d v2(pt->x, pt->y);
   if (!v1.IsEqual(v2, Precision::Confusion())) {
       Handle(Geom2d_TrimmedCurve) lseg = GCE2d_MakeSegment(v1,v2);
       TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(lseg , dc->surf);
       dc->Edges.push_back(edge);
       dc->LastVert = *pt;
       dc->polyPoints.emplace_back(pt->x, pt->y, 0.0);
   }
   return 0;
}

// quad_cb called for quadratic (conic) BCurve segment in the current contour
// (ie V-C-V in TTF fonts). BCurve(LastVert -- pt0 -- pt1)
static int quad_cb(const FT_Vector* pt0, const FT_Vector* pt1, void* p) {
   FTDC_Ctx* dc = static_cast<FTDC_Ctx*>(p);
   TColgp_Array1OfPnt2d Poles(1,3);
   gp_Pnt2d v1(dc->LastVert.x, dc->LastVert.y);
   gp_Pnt2d c1(pt0->x, pt0->y);
   gp_Pnt2d v2(pt1->x, pt1->y);
   Poles.SetValue(1, v1);
   Poles.SetValue(2, c1);
   Poles.SetValue(3, v2);
   Handle(Geom2d_BezierCurve) bcseg = new Geom2d_BezierCurve(Poles);
   double u,v;
   u = bcseg->FirstParameter();
   v = bcseg->LastParameter();
   ShapeConstruct_Curve scc;
   Handle(Geom2d_BSplineCurve) spline = scc.ConvertToBSpline(bcseg, u, v, Precision::Confusion());
   if (spline.IsNull()) {
       Base::Console().Message("Conversion to B-spline failed");
   }
   TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(spline , dc->surf);
   dc->Edges.push_back(edge);
   dc->LastVert = *pt1;
   dc->polyPoints.emplace_back(pt1->x, pt1->y, 0.0);
   return 0;
}

// cubic_cb called for cubic BCurve segment in the current contour (ie V-C-C-V in
// Type 1 fonts). BCurve(LastVert -- pt0 -- pt1 -- pt2)
static int cubic_cb(const FT_Vector* pt0, const FT_Vector* pt1, const FT_Vector* pt2, void* p) {
   FTDC_Ctx* dc = static_cast<FTDC_Ctx*>(p);
   TColgp_Array1OfPnt2d Poles(1,4);
   gp_Pnt2d v1(dc->LastVert.x, dc->LastVert.y);
   gp_Pnt2d c1(pt0->x, pt0->y);
   gp_Pnt2d c2(pt1->x, pt1->y);
   gp_Pnt2d v2(pt2->x, pt2->y);
   Poles.SetValue(1, v1);
   Poles.SetValue(2, c1);
   Poles.SetValue(3, c2);
   Poles.SetValue(4, v2);
   Handle(Geom2d_BezierCurve) bcseg = new Geom2d_BezierCurve(Poles);
   double u,v;
   u = bcseg->FirstParameter();
   v = bcseg->LastParameter();
   ShapeConstruct_Curve scc;
   Handle(Geom2d_BSplineCurve) spline = scc.ConvertToBSpline(bcseg, u, v, Precision::Confusion());
   if (spline.IsNull()) {
       Base::Console().Message("Conversion to B-spline failed");
   }
   TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(spline , dc->surf);
   dc->Edges.push_back(edge);
   dc->LastVert = *pt2;
   dc->polyPoints.emplace_back(pt2->x, pt2->y, 0.0);
   return 0;
}

// FT Callbacks structure
static FT_Outline_Funcs FTcbFuncs = {
   (FT_Outline_MoveToFunc)move_cb,
   (FT_Outline_LineToFunc)line_cb,
   (FT_Outline_ConicToFunc)quad_cb,
   (FT_Outline_CubicToFunc)cubic_cb,
   0, 0 // not needed for FC
};

//********** FT2FC Helpers
// get glyph outline in wires
PyObject* getGlyphContours(FT_Face FTFace, UNICHAR currchar, double PenPos, double Scale, int charNum, double tracking) {
   FT_Error error = 0;
   std::stringstream ErrorMsg;
   gp_Pnt origin = gp_Pnt(0.0,0.0,0.0);
   FTDC_Ctx ctx;

   ctx.currchar = currchar;
   ctx.surf = new Geom_Plane(origin,gp::DZ());

   error = FT_Outline_Decompose(&FTFace->glyph->outline,
                                &FTcbFuncs,
                                &ctx);
   if(error) {
      ErrorMsg << "FT_Decompose failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
   }

// make the last TopoDS_Wire
   if (!ctx.Edges.empty()){
       ctx.Wires.push_back(edgesToWire(ctx.Edges));
       ctx.wDir.push_back(calcClockDir(ctx.polyPoints));
   }

//a ttf outer contour is clockwise with material on the right.
//an occ outer contour has material on the left, so it must be reversed?


   FT_Orientation ftOrient = FT_Outline_Get_Orientation(&FTFace->glyph->outline);
   bool isTTF = false;
   if (ftOrient == FT_ORIENTATION_TRUETYPE) {
        isTTF = true;
   }

   Py::List list;

   gp_Vec pointer = gp_Vec(PenPos * Scale + charNum*tracking,0.0,0.0);
   gp_Trsf xForm;
   xForm.SetScale(origin,Scale);
   xForm.SetTranslationPart(pointer);
   BRepBuilderAPI_Transform BRepScale(xForm);
   bool bCopy = true;                                                           // no effect?


   int wCount = 0;
   for(std::vector<TopoDS_Wire>::iterator iWire=ctx.Wires.begin();iWire != ctx.Wires.end(); ++iWire, wCount++) {
       if ((ctx.wDir[wCount] == CLOCKWISE) && isTTF) {         //ttf outer wire. fill inside / right
           (*iWire).Orientation(TopAbs_REVERSED);
       } else if ((ctx.wDir[wCount] == CLOCKWISE) && !isTTF) { //ps inner wire. fill outside / right
           (*iWire).Orientation(TopAbs_REVERSED);
       } else if ((ctx.wDir[wCount] == ANTICLOCKWISE) && isTTF) {  //ttf inner wire. fill outside / left
           (*iWire).Orientation(TopAbs_REVERSED);
       } else if ((ctx.wDir[wCount] == ANTICLOCKWISE) && !isTTF) {  //ps outer wire. fill inside / left
           (*iWire).Orientation(TopAbs_REVERSED);
       } else {
            //this is likely a poorly constructed font (ex a ttf with outer wires ACW )
            Base::Console().Message("FT2FC::getGlyphContours - indeterminate wire direction\n");
       }

       BRepScale.Perform(*iWire,bCopy);
       if (!BRepScale.IsDone())  {
          ErrorMsg << "FT2FC OCC BRepScale failed \n";
          throw std::runtime_error(ErrorMsg.str());
       }

       PyObject* wire = new TopoShapeWirePy(new TopoShape(TopoDS::Wire(BRepScale.Shape())));
       list.append(Py::asObject(wire));
   }
   return Py::new_reference_to(list);
}

// get kerning values for this char pair
//TODO: should check FT_HASKERNING flag? returns (0,0) if no kerning?
FT_Vector getKerning(FT_Face FTFace, UNICHAR lc, UNICHAR rc) {
   FT_Vector retXY;
   FT_Error error;
   std::stringstream ErrorMsg;
   FT_Vector ftKern;
   FT_UInt lcx = FT_Get_Char_Index(FTFace, lc);
   FT_UInt rcx = FT_Get_Char_Index(FTFace, rc);
   error = FT_Get_Kerning(FTFace,lcx,rcx,FT_KERNING_DEFAULT,&ftKern);
   if(error) {
      ErrorMsg << "FT_Get_Kerning failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
   }
   retXY.x = ftKern.x;
   retXY.y = ftKern.y;
   return(retXY);
}

// Make a TopoDS_Wire from a list of TopoDS_Edges
TopoDS_Wire edgesToWire(std::vector<TopoDS_Edge> Edges) {
    TopoDS_Wire occwire;
    std::vector<TopoDS_Edge>::iterator iEdge;
    BRepBuilderAPI_MakeWire mkWire;
    for (iEdge = Edges.begin(); iEdge != Edges.end(); ++iEdge){
        mkWire.Add(*iEdge);
        if (!mkWire.IsDone()) {
            Base::Console().Message("FT2FC Trace edgesToWire failed to add wire\n");
        }
    }
    occwire = mkWire.Wire();
    BRepLib::BuildCurves3d(occwire);
    return(occwire);
}

//is polygon formed by points clockwise (0) or anticlockwise(1)
int calcClockDir(std::vector<Base::Vector3d> points)
{
    int result = CLOCKWISE;
    int stop = points.size() - 1;
    int iPoint = 0;
    double bigArea = 0;
    for ( ; iPoint < stop; iPoint++) {
        double area = points[iPoint].x * points[iPoint + 1].y -
                      points[iPoint + 1].x * points[iPoint].y;
        bigArea += area;
    }
    double area = points.back().x * points.front().y -
                  points.front().x * points.back().y;
    bigArea += area;
    if (bigArea < 0) {
        result = ANTICLOCKWISE;
    }
    return result;
}

#endif //#ifdef FCUseFreeType
