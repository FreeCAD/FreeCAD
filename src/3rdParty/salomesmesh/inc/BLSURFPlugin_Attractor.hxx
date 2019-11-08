// Copyright (C) 2007-2016  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

// ---
// File    : BLSURFPlugin_Attractor.hxx
// Authors : Renaud Nédélec (OCC)
// ---
// 
// The idea of the algorithm used to calculate the distance on a 
// non-euclidian parametric surface has been found in the ref. below:
//
// Ref:"Accurate Anisotropic Fast Marching for Diffusion-Based Geodesic Tractography"
// S. Jbabdi, P. Bellec, R. Toro, Daunizeau, M. Pélégrini-Issac, and H. Benali1
//

#ifndef _BLSURFPlugin_Attractor_HXX_
#define _BLSURFPlugin_Attractor_HXX_

#include <BLSURFPlugin_Defs.hxx>

#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <limits>
#include <utilities.h>

// OPENCASCADE includes
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <NCollection_Map.hxx>

#include <Geom_Surface.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom_Curve.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>

#include <gp_Pnt2d.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Shape.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>

#include <TopTools_DataMapOfShapeInteger.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

#ifndef WIN32
#include <fenv.h>
#endif

#include <Standard_ErrorHandler.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <gp_XY.hxx>
#include <gp_XYZ.hxx>
#include <TopTools_MapOfShape.hxx>

#define TYPE_EXP 0
#define TYPE_LIN 1

class BLSURFPLUGIN_EXPORT BLSURFPlugin_Attractor {
  
  public:
    
    BLSURFPlugin_Attractor ();
    BLSURFPlugin_Attractor (const TopoDS_Face& Face, const TopoDS_Shape& Attractor, const std::string& attEntry); 
    
    bool init();                                                // Calculates the discrete points correponding to attractor 
                                                                // and intialises the map of distances
    void edgeInit(Handle(Geom_Surface) aSurf, const TopoDS_Edge& anEdge);
    
    double              GetSize (double u, double v);
    TopoDS_Face         GetFace()           const { return _face; }
    TopoDS_Shape        GetAttractorShape() const { return _attractorShape; }
    std::string         GetAttractorEntry() const { return _attEntry; }
    std::vector<double> GetParameters()     const 
    { 
      double tab_params[] = {_startSize, _endSize, _actionRadius, _constantRadius}; 
      std::vector<double> params (tab_params, tab_params + sizeof(tab_params) / sizeof(double) );
      return params;
    }
    
    void SetParameters(double Start_Size, double End_Size, double Action_Radius, double Constant_Radius);
    void SetType(int type){ _type = type; }
    
    void BuildMap();                                            // Builds the map of distances between source point and any point P(u,v)
    bool IsMapBuilt() const { return _isMapBuilt; }             // Controls if the map has been built
    bool Empty()      const { return _isEmpty; }
  
    typedef std::vector<double> TDiscreteParam;
    typedef std::vector< std::vector<double> > TDistMap;
    typedef std::vector< std::vector<bool> > TPointSet;
    typedef std::set< std::vector<double> > TTrialSet;
    typedef std::vector<double> Trial_Pnt;
    typedef std::vector<int> IJ_Pnt;
          
  private:
    
    TopoDS_Face       _face;
    TopoDS_Shape      _attractorShape;
    std::string       _attEntry;
    TDiscreteParam    _vectU;
    TDiscreteParam    _vectV;
    TDistMap          _DMap;
    TPointSet         _known;
    TTrialSet         _trial;
    int               _type;                                    // Type of function used to calculate the size from the distance (unused for now)
    int               _gridU;                                   // Number of grid points in U direction
    int               _gridV;                                   // Number of grid points in V direction
    double            _u1, _u2, _v1, _v2;                       // Bounds of the parametric space of the face 
    double            _startSize, _endSize;                     // User parameters
    double            _actionRadius, _constantRadius;           //
    
    bool              _isMapBuilt;
    bool              _isEmpty;

  // data of a specific case: a point attractor on a plane
  Handle(Geom_Surface) _plane;
  gp_Pnt               _attractorPnt;
    
  double            (BLSURFPlugin_Attractor::*_distance)(double u, double v);            // Retrieve the value of the distance map at point (u,v) of the parametric space of _face
  double            _distanceFromMap(double u, double v);
  double            _distanceFromPoint(double u, double v);
};    

#endif
