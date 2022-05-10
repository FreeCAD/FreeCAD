// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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

// File      : SMESH_Block.hxx
// Created   : Tue Nov 30 12:42:18 2004
// Author    : Edward AGAPOV (eap)
//
#ifndef SMESH_Block_HeaderFile
#define SMESH_Block_HeaderFile

#include "SMESH_Utils.hxx"

// #include <Geom2d_Curve.hxx>
// #include <Geom_Curve.hxx>
// #include <Geom_Surface.hxx>

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfOrientedShape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_XY.hxx>
#include <gp_XYZ.hxx>
#include <math_FunctionSetWithDerivatives.hxx>

#include <ostream>
#include <vector>
#include <list>

class SMDS_MeshVolume;
class SMDS_MeshNode;
class Adaptor3d_Surface;
class Adaptor2d_Curve2d;
class Adaptor3d_Curve;
class gp_Pnt;

// =========================================================
// class calculating coordinates of 3D points by normalized
// parameters inside the block and vice versa
// =========================================================

class SMESHUtils_EXPORT SMESH_Block: public math_FunctionSetWithDerivatives
{
 public:
  enum TShapeID {
    // ----------------------------
    // Ids of the block sub-shapes
    // ----------------------------
    ID_NONE = 0,

    ID_V000 = 1, ID_V100, ID_V010, ID_V110, ID_V001, ID_V101, ID_V011, ID_V111, // 1-8

    ID_Ex00, ID_Ex10, ID_Ex01, ID_Ex11, // 9-12
    ID_E0y0, ID_E1y0, ID_E0y1, ID_E1y1, // 13-16
    ID_E00z, ID_E10z, ID_E01z, ID_E11z, // 17-20

    ID_Fxy0, ID_Fxy1, ID_Fx0z, ID_Fx1z, ID_F0yz, ID_F1yz, // 21-26

    ID_Shell, // 27

    // to use TShapeID for indexing certain type subshapes

    ID_FirstV = ID_V000, ID_FirstE = ID_Ex00, ID_FirstF = ID_Fxy0
  };


 public:
  // -------------------------------------------------
  // Block topology in terms of block sub-shapes' ids
  // -------------------------------------------------

  static int NbVertices()  { return  8; }
  static int NbEdges()     { return 12; }
  static int NbFaces()     { return  6; }
  static int NbSubShapes() { return ID_Shell; }
  // to avoid magic numbers when allocating memory for subshapes

  static inline bool IsVertexID( int theShapeID )
  { return ( theShapeID >= ID_V000 && theShapeID <= ID_V111 ); }

  static inline bool IsEdgeID( int theShapeID )
  { return ( theShapeID >= ID_Ex00 && theShapeID <= ID_E11z ); }

  static inline bool IsFaceID( int theShapeID )
  { return ( theShapeID >= ID_Fxy0 && theShapeID <= ID_F1yz ); }

  static int ShapeIndex( int theShapeID )
  {
    if ( IsVertexID( theShapeID )) return theShapeID - ID_V000;
    if ( IsEdgeID( theShapeID ))   return theShapeID - ID_Ex00;
    if ( IsFaceID( theShapeID ))   return theShapeID - ID_Fxy0;
    return 0;
  }
  // return index [0-...] for each type of sub-shapes,
  // for example :
  // ShapeIndex( ID_Ex00 ) == 0
  // ShapeIndex( ID_Ex10 ) == 1

  static void GetFaceEdgesIDs (const int faceID, std::vector< int >& edgeVec );
  // return edges IDs of a face in the order u0, u1, 0v, 1v

  static void GetEdgeVertexIDs (const int edgeID, std::vector< int >& vertexVec );
  // return vertex IDs of an edge

  static int GetCoordIndOnEdge (const int theEdgeID)
  { return (theEdgeID < ID_E0y0) ? 1 : (theEdgeID < ID_E00z) ? 2 : 3; }
  // return an index of a coordinate which varies along the edge

  static double* GetShapeCoef (const int theShapeID);
  // for theShapeID( TShapeID ), returns 3 coefficients used
  // to compute an addition of an on-theShape point to coordinates
  // of an in-shell point. If an in-shell point has parameters (Px,Py,Pz),
  // then the addition of a point P is computed as P*kx*ky*kz and ki is
  // defined by the returned coef like this:
  // ki = (coef[i] == 0) ? 1 : (coef[i] < 0) ? 1 - Pi : Pi

  static int GetShapeIDByParams ( const gp_XYZ& theParams );
  // define an id of the block sub-shape by point parameters

  static std::ostream& DumpShapeID (const int theBlockShapeID, std::ostream& stream);
  // DEBUG: dump an id of a block sub-shape


 public:
  // ---------------
  // Initialization
  // ---------------

  SMESH_Block();

  bool LoadBlockShapes(const TopoDS_Shell&         theShell,
                       const TopoDS_Vertex&        theVertex000,
                       const TopoDS_Vertex&        theVertex001,
                       TopTools_IndexedMapOfOrientedShape& theShapeIDMap );
  // Initialize block geometry with theShell,
  // add sub-shapes of theBlock to theShapeIDMap so that they get
  // IDs according to enum TShapeID

  bool LoadBlockShapes(const TopTools_IndexedMapOfOrientedShape& theShapeIDMap);
  // Initialize block geometry with shapes from theShapeIDMap

  bool LoadMeshBlock(const SMDS_MeshVolume*        theVolume,
                     const int                     theNode000Index,
                     const int                     theNode001Index,
                     std::vector<const SMDS_MeshNode*>& theOrderedNodes);
  // prepare to work with theVolume and
  // return nodes in theVolume corners in the order of TShapeID enum

  bool LoadFace(const TopoDS_Face& theFace,
                const int          theFaceID,
                const TopTools_IndexedMapOfOrientedShape& theShapeIDMap);
  // Load face geometry.
  // It is enough to compute params or coordinates on the face.
  // Face subshapes must be loaded into theShapeIDMap before

  static bool Insert(const TopoDS_Shape& theShape,
                     const int           theShapeID,
                     TopTools_IndexedMapOfOrientedShape& theShapeIDMap);
  // Insert theShape into theShapeIDMap with theShapeID,
  // Not yet set shapes preceding theShapeID are filled with compounds
  // Return true if theShape was successfully bound to theShapeID

  static bool FindBlockShapes(const TopoDS_Shell&         theShell,
                              const TopoDS_Vertex&        theVertex000,
                              const TopoDS_Vertex&        theVertex001,
                              TopTools_IndexedMapOfOrientedShape& theShapeIDMap );
  // add sub-shapes of theBlock to theShapeIDMap so that they get
  // IDs according to enum TShapeID

public:
  // ---------------------------------
  // Define coordinates by parameters
  // ---------------------------------

  bool VertexPoint( const int theVertexID, gp_XYZ& thePoint ) const {
    if ( !IsVertexID( theVertexID ))           return false;
    thePoint = myPnt[ theVertexID - ID_FirstV ]; return true;
  }
  // return vertex coordinates, parameters are defined by theVertexID

  bool EdgePoint( const int theEdgeID, const gp_XYZ& theParams, gp_XYZ& thePoint ) const {
    if ( !IsEdgeID( theEdgeID ))                                 return false;
    thePoint = myEdge[ theEdgeID - ID_FirstE ].Point( theParams ); return true;
  }
  // return coordinates of a point on edge

  bool EdgeU( const int theEdgeID, const gp_XYZ& theParams, double& theU ) const {
    if ( !IsEdgeID( theEdgeID ))                              return false;
    theU = myEdge[ theEdgeID - ID_FirstE ].GetU( theParams ); return true;
  }
  // return parameter on edge by in-block parameters

  bool FacePoint( const int theFaceID, const gp_XYZ& theParams, gp_XYZ& thePoint ) const {
    if ( !IsFaceID ( theFaceID ))                                return false;
    thePoint = myFace[ theFaceID - ID_FirstF ].Point( theParams ); return true;
  }
  // return coordinates of a point on face

  bool FaceUV( const int theFaceID, const gp_XYZ& theParams, gp_XY& theUV ) const {
    if ( !IsFaceID ( theFaceID ))                               return false;
    theUV = myFace[ theFaceID - ID_FirstF ].GetUV( theParams ); return true;
  }
  // return UV coordinates on a face by in-block parameters

  bool ShellPoint( const gp_XYZ& theParams, gp_XYZ& thePoint ) const;
  // return coordinates of a point in shell

  static bool ShellPoint(const gp_XYZ&         theParams,
                         const std::vector<gp_XYZ>& thePointOnShape,
                         gp_XYZ&               thePoint );
  // computes coordinates of a point in shell by points on sub-shapes
  // and point parameters.
  // thePointOnShape[ subShapeID ] must be a point on a subShape;
  // thePointOnShape.size() == ID_Shell, thePointOnShape[0] not used


 public:
  // ---------------------------------
  // Define parameters by coordinates
  // ---------------------------------

  bool ComputeParameters (const gp_Pnt& thePoint,
                          gp_XYZ&       theParams,
                          const int     theShapeID    = ID_Shell,
                          const gp_XYZ& theParamsHint = gp_XYZ(-1,-1,-1));
  // compute point parameters in the block.
  // Note: for edges, it is better to use EdgeParameters()
  // Return false only in case of "hard" failure, use IsToleranceReached() etc
  // to evaluate quality of the found solution

  bool VertexParameters(const int theVertexID, gp_XYZ& theParams);
  // return parameters of a vertex given by TShapeID

  bool EdgeParameters(const int theEdgeID, const double theU, gp_XYZ& theParams);
  // return parameters of a point given by theU on edge

  void SetTolerance(const double tol);
  // set tolerance for ComputeParameters()

  double GetTolerance() const { return myTolerance; }
  // return current tolerance of ComputeParameters()

  bool IsToleranceReached() const;
  // return true if solution found by ComputeParameters() is within the tolerance

  double DistanceReached() const { return distance(); }
  // return distance between solution found by ComputeParameters() and thePoint

 public:
  // ---------
  // Services
  // ---------

  static bool IsForwardEdge (const TopoDS_Edge &                       theEdge,
                             const TopTools_IndexedMapOfOrientedShape& theShapeIDMap) {
    int v1ID = theShapeIDMap.FindIndex( TopExp::FirstVertex( theEdge ).Oriented( TopAbs_FORWARD ));
    int v2ID = theShapeIDMap.FindIndex( TopExp::LastVertex( theEdge ).Oriented( TopAbs_FORWARD ));
    return ( v1ID < v2ID );
  }
  // Return true if an in-block parameter increases along theEdge curve

  static int GetOrderedEdges (const TopoDS_Face&        theFace,
                              std::list< TopoDS_Edge >& theEdges,
                              std::list< int >  &       theNbEdgesInWires,
                              TopoDS_Vertex             theFirstVertex=TopoDS_Vertex(),
                              const bool                theShapeAnalysisAlgo=false);
  // Return nb wires and a list of ordered edges.
  // It is used to assign indices to subshapes.
  // theFirstVertex may be NULL.
  // Always try to set a seam edge first
  // if (theShapeAnalysisAlgo) then ShapeAnalysis::OuterWire() is used to find the outer
  // wire else BRepTools::OuterWire() is used

 public:
  // -----------------------------------------------------------
  // Methods of math_FunctionSetWithDerivatives used internally
  // to define parameters by coordinates
  // -----------------------------------------------------------
  Standard_Integer NbVariables() const;
  Standard_Integer NbEquations() const;
  Standard_Boolean Value(const math_Vector& X,math_Vector& F) ;
  Standard_Boolean Derivatives(const math_Vector& X,math_Matrix& D) ;
  Standard_Boolean Values(const math_Vector& X,math_Vector& F,math_Matrix& D) ;
  Standard_Integer GetStateNumber ();

 protected:

  /*!
   * \brief Call it after geometry initialisation
   */
  void init();

  // Note: to compute params of a point on a face, it is enough to set
  // TFace, TEdge's and points for that face only

  // Note 2: curve adaptors need to have only Value(double), FirstParameter() and
  // LastParameter() defined to be used by Block algorithms

  class SMESHUtils_EXPORT TEdge {
    int                myCoordInd;
    double             myFirst;
    double             myLast;
    Adaptor3d_Curve*   myC3d;
    // if mesh volume
    gp_XYZ             myNodes[2];
  public:
    void Set( const int edgeID, Adaptor3d_Curve* curve, const bool isForward );
    void Set( const int edgeID, const gp_XYZ& node1, const gp_XYZ& node2 );
    Adaptor3d_Curve* GetCurve() const { return myC3d; }
    double EndParam(int i) const { return i ? myLast : myFirst; }
    int CoordInd() const { return myCoordInd; }
    const gp_XYZ& NodeXYZ(int i) const { return i ? myNodes[1] : myNodes[0]; }
    gp_XYZ Point( const gp_XYZ& theParams ) const; // Return coord by params
    double GetU( const gp_XYZ& theParams ) const;  // Return U by params
    TEdge(): myC3d(0) {}
    ~TEdge();
  };

  class SMESHUtils_EXPORT TFace {
    // 4 edges in the order u0, u1, 0v, 1v
    int                  myCoordInd[ 4 ];
    double               myFirst   [ 4 ];
    double               myLast    [ 4 ];
    Adaptor2d_Curve2d*   myC2d     [ 4 ];
    // 4 corner points in the order 00, 10, 11, 01
    gp_XY                myCorner  [ 4 ];
    // surface
    Adaptor3d_Surface*   myS;
    // if mesh volume
    gp_XYZ               myNodes[4];
  public:
    void Set( const int faceID, Adaptor3d_Surface* S, // must be in GetFaceEdgesIDs() order:
              Adaptor2d_Curve2d* c2d[4], const bool isForward[4] );
    void Set( const int faceID, const TEdge& edgeU0, const TEdge& edgeU1 );
    gp_XY  GetUV( const gp_XYZ& theParams ) const;
    gp_XYZ Point( const gp_XYZ& theParams ) const;
    int GetUInd() const { return myCoordInd[ 0 ]; }
    int GetVInd() const { return myCoordInd[ 2 ]; }
    void GetCoefs( int i, const gp_XYZ& theParams, double& eCoef, double& vCoef ) const;
    const Adaptor3d_Surface* Surface() const { return myS; }
    bool IsUVInQuad( const gp_XY& uv,
                     const gp_XYZ& param0, const gp_XYZ& param1,
                     const gp_XYZ& param2, const gp_XYZ& param3 ) const;
    gp_XY GetUVRange() const;
    TFace(): myS(0) { myC2d[0]=myC2d[1]=myC2d[2]=myC2d[3]=0; }
    ~TFace();
  };

  // geometry in the order as in TShapeID:
  // 8 vertices
  gp_XYZ myPnt[ 8 ];
  // 12 edges
  TEdge  myEdge[ 12 ];
  // 6 faces
  TFace  myFace[ 6 ];

  // for param computation

  enum { SQUARE_DIST = 0, DRV_1, DRV_2, DRV_3 };
  double distance () const { return sqrt( myValues[ SQUARE_DIST ]); }
  double funcValue(double sqDist) const { return mySquareFunc ? sqDist : sqrt(sqDist); }
  bool computeParameters(const gp_Pnt& thePoint, gp_XYZ& theParams, const gp_XYZ& theParamsHint, int);
  void refineParametersOnFace( const gp_Pnt& thePoint, gp_XYZ& theParams, int theFaceID );
  bool findUVByHalfDivision( const gp_Pnt& thePoint, const gp_XY& theUV,
                             const TFace& tface, gp_XYZ& theParams);
  bool findUVAround( const gp_Pnt& thePoint, const gp_XY& theUV,
                     const TFace& tface, gp_XYZ& theParams, int nbGetWorstLimit );
  bool saveBetterSolution( const gp_XYZ& theNewParams, gp_XYZ& theParams, double sqDistance );

  int      myFaceIndex;
  double   myFaceParam;
  int      myNbIterations;
  double   mySumDist;
  double   myTolerance;
  bool     mySquareFunc;

  gp_XYZ   myPoint; // the given point
  gp_XYZ   myParam; // the best parameters guess
  double   myValues[ 4 ]; // values computed at myParam: square distance and 3 derivatives

  typedef std::pair<gp_XYZ,gp_XYZ> TxyzPair;
  TxyzPair my3x3x3GridNodes[ 1000 ]; // to compute the first param guess
  bool     myGridComputed;
};


#endif
