/***************************************************************************
 *   Copyright (c) wandererfan       <wandererfan (at) gmail.com> 2013     *
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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdio> 
#include <cstdlib> 
#include <stdexcept>
#include <vector>

#include <BRepLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom_Plane.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>

#include <Base/Console.h>
#include "TopoShape.h"
#include "TopoShapePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeWirePy.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include FT_TYPES_H

#include "FT2FC.h"

using namespace Part;

typedef unsigned long UNICHAR;           // ul is FT2's codepoint type <=> Py_UNICODE2/4

// Private function prototypes
PyObject* getGlyphContours(FT_Face FTFont, UNICHAR currchar, double PenPos, double Scale,int charNum, double tracking);
FT_Vector getKerning(FT_Face FTFont, UNICHAR lc, UNICHAR rc);
TopoDS_Wire edgesToWire(std::vector<TopoDS_Edge> Edges);

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
                const double tracking) {                     // fc coords
   FT_Library  FTLib;                
   FT_Face     FTFont; 
   FT_Error    error;        
   FT_Long     FaceIndex = 0;                   // some fonts have multiple faces
   FT_Vector   kern;
   FT_UInt     FTLoadFlags = FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP;

   std::stringstream ErrorMsg;
   double PenPos = 0, scalefactor;
   UNICHAR prevchar = 0, currchar = 0;
   int  cadv;
   size_t i;
   PyObject *WireList, *CharList;
   
   error = FT_Init_FreeType(&FTLib); 
   if(error) {
      ErrorMsg << "FT_Init_FreeType failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
      }
      
   // FT does not return an error if font file not found? 
   std::ifstream is;
   is.open (FontSpec);
   if (!is) {
      ErrorMsg << "Font file not found: " << FontSpec;
      throw std::runtime_error(ErrorMsg.str());
      }

   error = FT_New_Face(FTLib,FontSpec,FaceIndex, &FTFont); 
   if(error) {
      ErrorMsg << "FT_New_Face failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
      }
      
//TODO: check that FTFont is scalable?  only relevant for hinting etc?
 
//  FT2 blows up if char size is not set to some non-zero value. 
//  This sets size to 48 point. Magic. 
   error = FT_Set_Char_Size(FTFont,         
                            0,             /* char_width in 1/64th of points */ 
                            48*64,         /* char_height in 1/64th of points */ 
                            0,             /* horizontal device resolution */ 
                            0 );           /* vertical device resolution */ 
   if(error) {
      ErrorMsg << "FT_Set_Char_Size failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
      }
 
   CharList = PyList_New(0);
   scalefactor = stringheight/float(FTFont->height);
   for (i=0; i<length; i++) {
       currchar = PyUString[i];
       error = FT_Load_Char(FTFont,
                            currchar,
                            FTLoadFlags);
       if(error) {
           ErrorMsg << "FT_Load_Char failed: " << error;
           throw std::runtime_error(ErrorMsg.str());
       }

       cadv = FTFont->glyph->advance.x;
       kern = getKerning(FTFont,prevchar,currchar);
       PenPos += kern.x;
       WireList = getGlyphContours(FTFont,currchar,PenPos, scalefactor,i,tracking);
       if (!PyList_Size(WireList))                                  // empty ==> whitespace
           Base::Console().Log("FT2FC char '0x%04x'/'%d' has no Wires!\n", currchar, currchar);
       else
           PyList_Append(CharList, WireList);
       PenPos += cadv;
       prevchar = currchar;
   }

   error = FT_Done_FreeType(FTLib);
   if(error) {
      ErrorMsg << "FT_Done_FreeType failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
      }

   return(CharList);
   }
   
//********** FT Decompose callbacks and data defns
// FT Decomp Context for 1 char
struct FTDC_Ctx {               
  std::vector<TopoDS_Wire> Wires;
  std::vector<TopoDS_Edge> Edges;
  UNICHAR currchar;
  FT_Vector LastVert;
  Handle(Geom_Surface) surf;
};

// move_cb called for start of new contour. pt is xy of contour start.
// p points to the context where we remember what happened previously (last point, etc)
static int move_cb(const FT_Vector* pt, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   if (!dc->Edges.empty()){                    
       TopoDS_Wire newwire = edgesToWire(dc->Edges);
       dc->Wires.push_back(newwire);
       dc->Edges.clear();
   }
   dc->LastVert = *pt;
   return 0;
}

// line_cb called for line segment in the current contour: line(LastVert -- pt) 
static int line_cb(const FT_Vector* pt, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   gp_Pnt2d v1(dc->LastVert.x, dc->LastVert.y);
   gp_Pnt2d v2(pt->x, pt->y);
   if (!v1.IsEqual(v2, Precision::Confusion())) {
       Handle(Geom2d_TrimmedCurve) lseg = GCE2d_MakeSegment(v1,v2);
       TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(lseg , dc->surf);
       dc->Edges.push_back(edge);
       dc->LastVert = *pt;
   }
   return 0;
}
   
// quad_cb called for quadratic (conic) BCurve segment in the current contour 
// (ie V-C-V in TTF fonts). BCurve(LastVert -- pt0 -- pt1)
static int quad_cb(const FT_Vector* pt0, const FT_Vector* pt1, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   TColgp_Array1OfPnt2d Poles(1,3);
   gp_Pnt2d v1(dc->LastVert.x, dc->LastVert.y);
   gp_Pnt2d c1(pt0->x, pt0->y);
   gp_Pnt2d v2(pt1->x, pt1->y);
   Poles.SetValue(1, v1);
   Poles.SetValue(2, c1);
   Poles.SetValue(3, v2);
   Handle(Geom2d_BezierCurve) bcseg = new Geom2d_BezierCurve(Poles);
   TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(bcseg , dc->surf);
   dc->Edges.push_back(edge);
   dc->LastVert = *pt1;
   return 0;
}

// cubic_cb called for cubic BCurve segment in the current contour (ie V-C-C-V in
// Type 1 fonts). BCurve(LastVert -- pt0 -- pt1 -- pt2)
static int cubic_cb(const FT_Vector* pt0, const FT_Vector* pt1, const FT_Vector* pt2, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
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
   TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(bcseg , dc->surf);
   dc->Edges.push_back(edge);
   dc->LastVert = *pt2;
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
PyObject* getGlyphContours(FT_Face FTFont, UNICHAR currchar, double PenPos, double Scale, int charNum, double tracking) {
   FT_Error error = 0;
   std::stringstream ErrorMsg;
   gp_Pnt origin = gp_Pnt(0.0,0.0,0.0);
   FTDC_Ctx ctx;

   ctx.currchar = currchar;
   ctx.surf = new Geom_Plane(origin,gp::DZ());

   error = FT_Outline_Decompose(&FTFont->glyph->outline, 
                                &FTcbFuncs, 
                                &ctx);
   if(error) {
      ErrorMsg << "FT_Decompose failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
   }

// make the last TopoDS_Wire
   if (!ctx.Edges.empty()){                    
       ctx.Wires.push_back(edgesToWire(ctx.Edges));
   }
   /*FT_Orientation fontClass =*/ FT_Outline_Get_Orientation(&FTFont->glyph->outline);
   PyObject* ret = PyList_New(0);

   gp_Vec pointer = gp_Vec(PenPos * Scale + charNum*tracking,0.0,0.0);
   gp_Trsf xForm;
   xForm.SetScale(origin,Scale);
   xForm.SetTranslationPart(pointer);
   BRepBuilderAPI_Transform BRepScale(xForm);
   bool bCopy = true;                                                           // no effect?

   for(std::vector<TopoDS_Wire>::iterator iWire=ctx.Wires.begin();iWire != ctx.Wires.end();++iWire) {
       BRepScale.Perform(*iWire,bCopy);
       if (!BRepScale.IsDone())  {
          ErrorMsg << "FT2FC OCC BRepScale failed \n";
          throw std::runtime_error(ErrorMsg.str());
       }
       PyList_Append(ret,new TopoShapeWirePy(new TopoShape(TopoDS::Wire(BRepScale.Shape()))));
   } 
   return(ret);
}

// get kerning values for this char pair
//TODO: should check FT_HASKERNING flag? returns (0,0) if no kerning?
FT_Vector getKerning(FT_Face FTFont, UNICHAR lc, UNICHAR rc) {
   FT_Vector retXY;
   FT_Error error;        
   std::stringstream ErrorMsg;  
   FT_Vector ftKern;
   FT_UInt lcx = FT_Get_Char_Index(FTFont, lc);    
   FT_UInt rcx = FT_Get_Char_Index(FTFont, rc);
   error = FT_Get_Kerning(FTFont,lcx,rcx,FT_KERNING_DEFAULT,&ftKern);
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


#endif //#ifdef FCUseFreeType
