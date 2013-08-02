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


#ifdef FCUseFreeType

#include "PreCompiled.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdio> 
#include <cstdlib> 
#include <stdexcept>
#include <vector>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <Geom_BezierCurve.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColgp_Array1OfPnt.hxx>

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
PyObject* getGlyphContours(FT_Face FTFont, UNICHAR currchar, int PenPos, float Scale);
FT_Vector getKerning(FT_Face FTFont, UNICHAR lc, UNICHAR rc);
TopoShapeWirePy* edgesToWire(std::vector<TopoDS_Edge> Edges);

// get string's wires (contours) in FC/OCC coords
PyObject* FT2FC(const Py_UNICODE *PyUString,
                const size_t length,
                const char *FontPath,
                const char *FontName,
                const float stringheight,                 // fc coords
                const int tracking) {                     // fc coords
   FT_Library  FTLib;                
   FT_Face     FTFont; 
   FT_Error    error;        
   FT_Long     FaceIndex = 0;                   // some fonts have multiple faces
   FT_Vector   kern;
   FT_UInt     FTLoadFlags = FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP;

   std::string FontSpec;
   std::stringstream ErrorMsg;
   float scalefactor;
   UNICHAR prevchar = 0, currchar = 0;
   int  cadv, PenPos = 0, PyErr;
   size_t i;
   
   PyObject *WireList, *CharList;
   
   error = FT_Init_FreeType(&FTLib); 
   if(error) {
      ErrorMsg << "FT_Init_FreeType failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
      }
      
   std::string tmpPath = FontPath;              // can't concat const char*
   std::string tmpName = FontName;
   FontSpec = tmpPath + tmpName;

   // FT does not return an error if font file not found? 
   std::ifstream is;
   is.open (FontSpec.c_str());
   if (!is) {
      ErrorMsg << "Font file not found: " << FontSpec;
      throw std::runtime_error(ErrorMsg.str());
      }
   // maybe boost::filesystem::exists for x-platform??   

   error = FT_New_Face(FTLib,FontSpec.c_str(),FaceIndex, &FTFont); 
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
 
   scalefactor = float(stringheight/FTFont->height);
   CharList = PyList_New(0);
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
       WireList = getGlyphContours(FTFont,currchar,PenPos, scalefactor);
       if (!PyList_Size(WireList))                                  // empty ==> whitespace
           std::cout << "char " << i << " = " << hex << std::showbase << currchar << " has no wires! " << std::endl;
       else
           PyErr = PyList_Append(CharList, WireList);    //add error check
       // not entirely happy with tracking solution.  It's specified in FC units,
       // so we have to convert back to font units to use it here.  We could 
       // lose some accuracy.  Hard to put it into FT callbacks since tracking
       // only affects position of chars 2 - n.  Don't want to loop 2x through wires.
       PenPos += cadv + int(tracking/scalefactor);
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
//  std::vector<TopoShapeWirePy*> TWires;
  PyObject* WireList;
  std::vector<TopoDS_Edge> Edges;
  UNICHAR currchar;
  int penpos;
  float scalefactor;
  FT_Vector LastVert;
  };

// move_cb called for start of new contour. pt is xy of contour start.
// p points to the context where we remember what happened previously (last point, etc)
static int move_cb(const FT_Vector* pt, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   int PyErr;
   if (!dc->Edges.empty()){                    
       TopoShapeWirePy* newwire;
       newwire = edgesToWire(dc->Edges);
       PyErr = PyList_Append(dc->WireList, newwire);   // add error check
       dc->Edges.clear();
   }
   dc->LastVert.x = pt->x + dc->penpos;                       
   dc->LastVert.y = pt->y;
   return 0;
   }

// line_cb called for line segment in the current contour: line(LastVert -- pt) 
static int line_cb(const FT_Vector* pt, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   // convert font coords to FC/OCC coords
   float v1x = dc->scalefactor * dc->LastVert.x;               
   float v1y = dc->scalefactor * dc->LastVert.y;
   float v2x = dc->scalefactor * (pt->x + dc->penpos);
   float v2y = dc->scalefactor * pt->y;
   gp_Pnt v1(v1x, v1y, 0);
   gp_Pnt v2(v2x, v2y, 0);
   BRepBuilderAPI_MakeEdge makeEdge(v1,v2);
   TopoDS_Edge edge = makeEdge.Edge();
   dc->Edges.push_back(edge);
   dc->LastVert.x = pt->x + dc->penpos;
   dc->LastVert.y = pt->y;
   return 0;
   }
   
// quad_cb called for quadratic (conic) BCurve segment in the current contour 
// (ie V-C-V in TTF fonts). BCurve(LastVert -- pt0 -- pt1)
static int quad_cb(const FT_Vector* pt0, const FT_Vector* pt1, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   TColgp_Array1OfPnt Poles(1,3);
   // convert font coords to FC/OCC coords
   float v1x = dc->scalefactor * dc->LastVert.x;
   float v1y = dc->scalefactor * dc->LastVert.y;
   float c1x = dc->scalefactor * (pt0->x + dc->penpos);
   float c1y = dc->scalefactor * pt0->y;
   float v2x = dc->scalefactor * (pt1->x + dc->penpos);
   float v2y = dc->scalefactor * pt1->y;
   gp_Pnt v1(v1x, v1y, 0);
   gp_Pnt c1(c1x, c1y, 0);
   gp_Pnt v2(v2x, v2y, 0);
   Poles.SetValue(1, v1);
   Poles.SetValue(2, c1);
   Poles.SetValue(3, v2);
   // "new" bcseg? need to free this? don't know when. does makeedge need it forever? or just for creation?
   // how to delete a "handle"? memory leak?
   Handle(Geom_BezierCurve) bcseg = new Geom_BezierCurve(Poles);
   BRepBuilderAPI_MakeEdge makeEdge(bcseg, v1, v2);
   TopoDS_Edge edge = makeEdge.Edge();
   dc->Edges.push_back(edge);
   dc->LastVert.x = pt1->x + dc->penpos;
   dc->LastVert.y = pt1->y;
   return 0;
   }

// cubic_cb called for cubic BCurve segment in the current contour (ie V-C-C-V in
// Type 1 fonts). BCurve(LastVert -- pt0 -- pt1 -- pt2)
static int cubic_cb(const FT_Vector* pt0, const FT_Vector* pt1, const FT_Vector* pt2, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   TColgp_Array1OfPnt Poles(1,4);
   // convert font coords to FC/OCC coords
   float v1x = dc->scalefactor * dc->LastVert.x;
   float v1y = dc->scalefactor * dc->LastVert.y;
   float c1x = dc->scalefactor * (pt0->x + dc->penpos);
   float c1y = dc->scalefactor * pt0->y;
   float c2x = dc->scalefactor * (pt1->x + dc->penpos);
   float c2y = dc->scalefactor * pt1->y;
   float v2x = dc->scalefactor * (pt2->x + dc->penpos);
   float v2y = dc->scalefactor * pt2->y;
   gp_Pnt v1(v1x, v1y, 0);
   gp_Pnt c1(c1x, c1y, 0);
   gp_Pnt c2(c2x, c2y, 0);
   gp_Pnt v2(v2x, v2y, 0);
   Poles.SetValue(1, v1);
   Poles.SetValue(2, c1);
   Poles.SetValue(3, c2);
   Poles.SetValue(4, v2);
   Handle(Geom_BezierCurve) bcseg = new Geom_BezierCurve(Poles);      // new? need to free this?
   BRepBuilderAPI_MakeEdge makeEdge(bcseg, v1, v2);
   TopoDS_Edge edge = makeEdge.Edge();
   dc->Edges.push_back(edge);
   dc->LastVert.x = pt2->x + dc->penpos;
   dc->LastVert.y = pt2->y;
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
//std::vector<TopoShapeWirePy*> getGlyphContours(FT_Face FTFont, UNICHAR currchar, int PenPos, float Scale) {
PyObject* getGlyphContours(FT_Face FTFont, UNICHAR currchar, int PenPos, float Scale) {
   FT_Error error = 0;
   std::stringstream ErrorMsg;
   FTDC_Ctx ctx;
   int PyErr;

   ctx.currchar = currchar;
   ctx.penpos = PenPos;
   ctx.scalefactor = Scale;
   ctx.WireList = PyList_New(0);

   error = FT_Outline_Decompose(&FTFont->glyph->outline, 
                                &FTcbFuncs, 
                                &ctx);
   if(error) {
      ErrorMsg << "FT_Decompose failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
   }

// make the last twire
   if (!ctx.Edges.empty()){                    
//       ctx.TWires.push_back(edgesToWire(ctx.Edges));
       PyErr = PyList_Append(ctx.WireList, edgesToWire(ctx.Edges));    // add error check
   }
   
//   return(ctx.TWires);
   return(ctx.WireList);
}

// get kerning values for this char pair
//TODO: should check FT_HASKERNING flag?
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
TopoShapeWirePy* edgesToWire(std::vector<TopoDS_Edge> Edges) {
    TopoDS_Wire occwire;
    std::vector<TopoDS_Edge>::iterator iEdge;
    // if Edges.empty() ????
    BRepBuilderAPI_MakeWire mkWire;
    for (iEdge = Edges.begin(); iEdge != Edges.end(); ++iEdge){
        mkWire.Add(*iEdge);
    }
    // if(mkWire.Done())   ???
    occwire = mkWire.Wire();
    TopoShapeWirePy* newwire = new TopoShapeWirePy(new TopoShape (occwire)); 

    return(newwire);
}


#endif //#ifdef FCUseFreeType
