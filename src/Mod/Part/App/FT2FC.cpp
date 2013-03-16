#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include FT_TYPES_H

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

#include "FT2FC.h"

// Private function prototypes
void getFTChar(char c);
std::vector<TopoDS_Wire>  getGlyphContours();
FT_Vector getKerning(char lc, char rc);
TopoDS_Wire edgesToWire(std::vector<TopoDS_Edge> Edges);

bool DEBUG=true;

struct FTDC_Ctx {               // FT Decomp Context for 1 char
  std::vector<TopoDS_Wire> TWires;
  std::vector<TopoDS_Edge> Edges;
  int ConCnt;
  int SegCnt;
  char currchar;
  int penpos;
  float scalefactor;
  FT_Vector LastVert;
  };

// Made a TopoDS_Wire from a list of TopoDS_Edges
TopoDS_Wire edgesToWire(std::vector<TopoDS_Edge> Edges) {
    TopoDS_Wire result;
    std::vector<TopoDS_Edge>::iterator iEdge;
    // if Edges.empty() ????
    BRepBuilderAPI_MakeWire mkWire;
    for (iEdge = Edges.begin(); iEdge != Edges.end(); ++iEdge){
        mkWire.Add(*iEdge);
    }
    // if(mkWire.Done())   ???
    result = mkWire.Wire();
    return(result);
    }

// FT Decompose callbacks and data defns
// move_cb called for start of new contour. pt is xy of contour start.
// p points to the context where we remember what happened previously (last point, etc)
static int move_cb(const FT_Vector* pt, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   dc->ConCnt++;
   if (!dc->Edges.empty()){                    // empty on first contour. (or messed up font)
       TopoDS_Wire newwire;
       newwire = edgesToWire(dc->Edges);
       dc->TWires.push_back(newwire);
       dc->Edges.clear();
   }
   dc->LastVert.x = pt->x + dc->penpos;                       // move along baseline
   dc->LastVert.y = pt->y;
   return 0;
   }

// line_cb called for line segment in the current contour: line(LastVert -- pt) 
static int line_cb(const FT_Vector* pt, void* p) {
   FTDC_Ctx* dc = (FTDC_Ctx*) p;
   // convert font coords to FC/OCC coords
   float v1x = dc->scalefactor * dc->LastVert.x;               // LastVert already moved along baseline
   float v1y = dc->scalefactor * dc->LastVert.y;
   float v2x = dc->scalefactor * (pt->x + dc->penpos);
   float v2y = dc->scalefactor * pt->y;
   gp_Pnt v1(v1x, v1y, 0);
   gp_Pnt v2(v2x, v2y, 0);
   BRepBuilderAPI_MakeEdge makeEdge(v1,v2);
   TopoDS_Edge edge = makeEdge.Edge();
   dc->Edges.push_back(edge);
   dc->SegCnt++;
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
   // "new" bcseg? need to free this, but don't know when! does edge need it forever? or just for creation?
   // how to delete a "handle" (!= a pointer)?
   Handle(Geom_BezierCurve) bcseg = new Geom_BezierCurve(Poles);
   BRepBuilderAPI_MakeEdge makeEdge(bcseg, v1, v2);
   TopoDS_Edge edge = makeEdge.Edge();
   dc->Edges.push_back(edge);
   dc->SegCnt++;
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
   Handle(Geom_BezierCurve) bcseg = new Geom_BezierCurve(Poles);      // new? need to free this
   BRepBuilderAPI_MakeEdge makeEdge(bcseg, v1, v2);
   TopoDS_Edge edge = makeEdge.Edge();
   dc->Edges.push_back(edge);
   dc->SegCnt++;
   dc->LastVert.x = pt2->x + dc->penpos;
   dc->LastVert.y = pt2->y;
   return 0;
   }

// FT Callbacks structure
static FT_Outline_Funcs outline_funcs = {
   (FT_Outline_MoveToFunc)move_cb,
   (FT_Outline_LineToFunc)line_cb,
   (FT_Outline_ConicToFunc)quad_cb,
   (FT_Outline_CubicToFunc)cubic_cb,
   0, 0                                      // device resolutions not needed
   };

// load glyph outline into FTFont. return char's h-adv metric.
void getFTChar(FT_Face FTFont, char c) {
   FT_Error error;   
   FT_UInt flags = FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP;
   std::stringstream ErrorMsg;  

   error = FT_Load_Char(FTFont,
                        c,
                        flags);
   if(error) {
      ErrorMsg << "FT_Load_Char failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
      }
   return;
   }

// get kerning values for this char pair
//TODO: should check FT_HASKERNING flag
FT_Vector getKerning(FT_Face FTFont, char lc, char rc) {
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

// get glyph outline for current char
std::vector<TopoDS_Wire> getGlyphContours(FT_Face FTFont, char currchar, int PenPos, float Scale) {
   FT_Error error = 0;
   std::stringstream ErrorMsg;
   FT_Outline*     FTOPointer;
   FTDC_Ctx ctx;
   FTOPointer = &FTFont->glyph->outline;

   ctx.ConCnt = 0;
   ctx.SegCnt = 0;
   ctx.currchar = currchar;
   ctx.penpos = PenPos;
   ctx.scalefactor = Scale;
   ctx.Edges.clear();
   ctx.TWires.clear();

   error = FT_Outline_Decompose(FTOPointer, &outline_funcs, &ctx);
   if(error) {
      ErrorMsg << "FT_Decompose failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
   }
   if (DEBUG)
      std::cout << "getGlyphContours processed char: " << currchar << " with " << ctx.ConCnt <<
         " contours containing " << ctx.SegCnt << " segments." << std::endl;

   if (!ctx.Edges.empty()){                    // make the last twire
       TopoDS_Wire newwire;
       newwire = edgesToWire(ctx.Edges);
       ctx.TWires.push_back(newwire);
   }
   return(ctx.TWires);
   }

// get string's wires (contours) in FC/OCC coords
std::vector <std::vector <TopoDS_Wire> > FT2FC(const std::string shapestring,
                          const std::string FontPath, 
                          const std::string FontName,
                          const float stringheight,                 // in fc coords
                          const int tracking) {                     // in fc coords
   FT_Library  FTLib;                
   FT_Face     FTFont; 
   FT_Error    error;        
   FT_Long     FaceIndex; 
   FT_Vector   kern;
   std::string FontSpec;
   std::stringstream ErrorMsg;

   float scalefactor;
   char prevchar,currchar;
   int  cadv,PenPos;
   size_t i;
   std::vector<TopoDS_Wire> CharWires;
   std::vector<std::vector <TopoDS_Wire> >  RetString;
   
   std::cout << "FT2FC started: "<< FontPath << std::endl;


   error = FT_Init_FreeType(&FTLib); 
   if(error) {
      ErrorMsg << "FT_Init_FreeType failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
      }
   FontSpec = FontPath + FontName;
   FaceIndex = 0;                               // some fonts have multiple faces
   // NOTE: FT blows up if font file not found.  It does not return an error!!!
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
//TODO: check that FTFont is scalable. 

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

   prevchar = 0;
   PenPos = 0;
   scalefactor = float(stringheight/FTFont->height);
   for (i=0;i<shapestring.length();i++) {
       currchar = shapestring[i];
       getFTChar(FTFont,currchar);
       cadv = FTFont->glyph->advance.x;
       kern = getKerning(FTFont,prevchar,currchar);
       PenPos += kern.x;
       CharWires = getGlyphContours(FTFont,currchar,PenPos, scalefactor);
       if (CharWires.empty())                                       // whitespace char
           std::cout << "Char " << i << " = " << currchar << " has no wires! " << std::endl;
       else
           RetString.push_back(CharWires);
       // not entirely happy with tracking solution.  It's specified in FC units,
       // so we have to convert back to font units to use it here.  We could 
       // lose some accuracy.  Hard to put it into FT callbacks since tracking
       // only affects position of chars 2 - n.
       PenPos += cadv + int(tracking/scalefactor);
       prevchar = currchar;
       }

   error = FT_Done_FreeType(FTLib);
   if(error) {
      ErrorMsg << "FT_Done_FreeType failed: " << error;
      throw std::runtime_error(ErrorMsg.str());
      }

   return(RetString);
   }

