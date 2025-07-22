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

// File      : SMESH_Pattern.hxx
// Created   : Mon Aug  2 10:30:00 2004
// Author    : Edward AGAPOV (eap)
//
#ifndef SMESH_Pattern_HeaderFile
#define SMESH_Pattern_HeaderFile

#include "SMESH_SMESH.hxx"

#include <vector>
#include <list>
#include <map>
#include <set>
#include <iostream>

#include <TopoDS_Vertex.hxx>
#include <TopTools_IndexedMapOfOrientedShape.hxx>
#include <gp_XYZ.hxx>
#include <gp_XY.hxx>
#include <gp_Pnt.hxx>

class SMDS_MeshElement;
class SMDS_MeshFace;
class SMDS_MeshVolume;
class SMDS_MeshNode;
class SMESH_Mesh;
class SMESHDS_SubMesh;
class TopoDS_Shell;
class TopoDS_Face;
class TopoDS_Edge;

//
// Class allowing meshing by mapping of pre-defined patterns: it generates
// a 2D mesh on a geometrical face or a 3D mesh inside a geometrical block
// of 6 faces.
//

class SMESH_EXPORT SMESH_Pattern {
 public:
  
  SMESH_Pattern ();

  void Clear();
  // clear fields

  bool Load (const char* theFileContents);
  // Load a pattern from <theFileContents>

  bool Load (SMESH_Mesh*        theMesh,
             const TopoDS_Face& theFace,
             bool               theProject = false,
             TopoDS_Vertex      the1stVertex=TopoDS_Vertex());
  // Create a pattern from the mesh built on <theFace>.
  // <theProject>==true makes override nodes positions
  // on <theFace> computed by mesher

  bool Load (SMESH_Mesh*         theMesh,
             const TopoDS_Shell& theBlock);
  // Create a pattern from the mesh built on <theBlock>

  bool Save (std::ostream& theFile);
  // Save the loaded pattern into theFile

  bool Apply (const TopoDS_Face&   theFace,
              const TopoDS_Vertex& theVertexOnKeyPoint1,
              const bool           theReverse);
  // Compute nodes coordinates applying
  // the loaded pattern to <theFace>. The first key-point
  // will be mapped into <theVertexOnKeyPoint1>, which must
  // be in the outer wire of theFace

  bool Apply (const TopoDS_Shell&  theBlock,
              const TopoDS_Vertex& theVertex000,
              const TopoDS_Vertex& theVertex001);
  // Compute nodes coordinates applying
  // the loaded pattern to <theBlock>. The (0,0,0) key-point
  // will be mapped into <theVertex000>. The
  // (0,0,1) key-point will be mapped into <theVertex001>.

  bool Apply (const SMDS_MeshFace* theFace,
              const int            theNodeIndexOnKeyPoint1,
              const bool           theReverse);
  // Compute nodes coordinates applying
  // the loaded pattern to <theFace>. The first key-point
  // will be mapped into <theNodeIndexOnKeyPoint1>-th node

  bool Apply (SMESH_Mesh*          theMesh,
              const SMDS_MeshFace* theFace,
              const TopoDS_Shape&  theSurface,
              const int            theNodeIndexOnKeyPoint1,
              const bool           theReverse);
  // Compute nodes coordinates applying
  // the loaded pattern to <theFace>. The first key-point
  // will be mapped into <theNodeIndexOnKeyPoint1>-th node

  bool Apply (SMESH_Mesh*                     theMesh,
              std::set<const SMDS_MeshFace*>& theFaces,
              const int                       theNodeIndexOnKeyPoint1,
              const bool                      theReverse);
  // Compute nodes coordinates applying
  // the loaded pattern to <theFaces>. The first key-point
  // will be mapped into <theNodeIndexOnKeyPoint1>-th node

  bool Apply (const SMDS_MeshVolume* theVolume,
              const int              theNode000Index,
              const int              theNode001Index);
  // Compute nodes coordinates applying
  // the loaded pattern to <theVolume>. The (0,0,0) key-point
  // will be mapped into <theNode000Index>-th node. The
  // (0,0,1) key-point will be mapped into <theNode000Index>-th
  // node.

  bool Apply (std::set<const SMDS_MeshVolume*>& theVolumes,
              const int                         theNode000Index,
              const int                         theNode001Index);
  // Compute nodes coordinates applying
  // the loaded pattern to <theVolumes>. The (0,0,0) key-point
  // will be mapped into <theNode000Index>-th node. The
  // (0,0,1) key-point will be mapped into <theNode000Index>-th
  // node.

  bool GetMappedPoints ( std::list<const gp_XYZ *> & thePoints ) const;
  // Return nodes coordinates computed by Apply() method

  bool MakeMesh(SMESH_Mesh* theMesh,
                const bool toCreatePolygons = false,
                const bool toCreatePolyedrs = false);
  // Create nodes and elements in <theMesh> using nodes
  // coordinates computed by either of Apply...() methods

  // ----------
  // Inquiries
  // ----------

  enum ErrorCode {
    ERR_OK,
    // Load(file)
    ERR_READ_NB_POINTS, // couldn't read nb of points
    ERR_READ_POINT_COORDS, // invalid nb of point coordinates
    ERR_READ_TOO_FEW_POINTS,  // too few points in a pattern
    ERR_READ_3D_COORD,  // coordinate of 3D point out of [0,1] range
    ERR_READ_NO_KEYPOINT, // no key-points in 2D pattern
    ERR_READ_BAD_INDEX, // invalid point index
    ERR_READ_ELEM_POINTS, // invalid nb of points in element
    ERR_READ_NO_ELEMS, // no elements in a pattern
    ERR_READ_BAD_KEY_POINT, // a key-point not on a boundary
    // Save(file)
    ERR_SAVE_NOT_LOADED, // pattern was not loaded
    // Load(shape)
    ERR_LOAD_EMPTY_SUBMESH, // no elements to load
    // Load(face)
    ERR_LOADF_NARROW_FACE, // too narrow face
    ERR_LOADF_CLOSED_FACE, // closed face
    ERR_LOADF_CANT_PROJECT, // impossible to project nodes
    // Load(volume)
    ERR_LOADV_BAD_SHAPE, // volume is not a brick of 6 faces
    ERR_LOADV_COMPUTE_PARAMS, // can't compute point parameters
    // Apply(shape)
    ERR_APPL_NOT_COMPUTED, // mapping failed
    ERR_APPL_NOT_LOADED, // pattern was not loaded
    ERR_APPL_BAD_DIMENTION, // wrong shape dimension
    ERR_APPL_BAD_NB_VERTICES, // keypoints - vertices mismatch
    // Apply(face)
    ERR_APPLF_BAD_TOPOLOGY, // bad pattern topology
    ERR_APPLF_BAD_VERTEX, // first vertex not on an outer face boundary
    ERR_APPLF_INTERNAL_EEROR, // program error
    // Apply(volume)
    ERR_APPLV_BAD_SHAPE, // volume is not a brick of 6 faces
    // Apply(mesh_face)
    ERR_APPLF_BAD_FACE_GEOM, // bad face geometry
    // MakeMesh
    ERR_MAKEM_NOT_COMPUTED, // mapping failed
    //Unexpected error 
    ERR_UNEXPECTED // Unexpected of the pattern mapping algorithm
  };

  ErrorCode GetErrorCode() const { return myErrorCode; }
  // return ErrorCode of the last operation

  bool IsLoaded() const { return !myPoints.empty() && !myElemPointIDs.empty(); }
  // Return true if a pattern was successfully loaded

  bool Is2D() const { return myIs2D; }
  // Return true if the loaded pattern is a 2D one

  bool GetPoints ( std::list<const gp_XYZ *> & thePoints ) const;
  // Return nodes coordinates of the pattern

  const std::list< int > & GetKeyPointIDs () const { return myKeyPointIDs; }
  // Return indices of key-points within the sequences returned by
  // GetPoints() and GetMappedPoints()
  
  const std::list< std::list< int > >& GetElementPointIDs (bool applied) const
  { return myElemXYZIDs.empty() || !applied ? myElemPointIDs : myElemXYZIDs; }
  // Return nodal connectivity of the elements of the pattern

  void DumpPoints() const;
  // Debug

  // -----------------------------
  // Utilities for advanced usage
  // -----------------------------

  TopoDS_Shape GetSubShape( const int i ) const {
    if ( i < 1 || i > myShapeIDMap.Extent() ) return TopoDS_Shape();
    return myShapeIDMap( i );
  }
  // Return a shape from myShapeIDMap where shapes are indexed so that first go
  // ordered vertices, then ordered edge, then faces and maybe a shell

private:
  // private methods

  struct TPoint {
    gp_XYZ myInitXYZ; // loaded position
    gp_XY  myInitUV;
    double myInitU; // [0,1]
    gp_Pnt myXYZ; // position to compute
    gp_XY  myUV;
    double myU;
    TPoint();
  };
  friend std::ostream & operator <<(std::ostream & OS, const TPoint& p);

  bool setErrorCode( const ErrorCode theErrorCode );
  // set ErrorCode and return true if it is Ok

  bool setShapeToMesh(const TopoDS_Shape& theShape);
  // Set a shape to be meshed. Return True if meshing is possible

  std::list< TPoint* > & getShapePoints(const TopoDS_Shape& theShape);
  // Return list of points located on theShape.
  // A list of edge-points include vertex-points (for 2D pattern only).
  // A list of face-points doesn't include edge-points.
  // A list of volume-points doesn't include face-points.

  std::list< TPoint* > & getShapePoints(const int theShapeID);
  // Return list of points located on the shape

  bool findBoundaryPoints();
  // If loaded from file, find points to map on edges and faces and
  // compute their parameters

  void arrangeBoundaries (std::list< std::list< TPoint* > >& boundaryPoints);
  // if there are several wires, arrange boundaryPoints so that
  // the outer wire goes first and fix inner wires orientation;
  // update myKeyPointIDs to correspond to the order of key-points
  // in boundaries; sort internal boundaries by the nb of key-points

  void computeUVOnEdge( const TopoDS_Edge& theEdge, const std::list< TPoint* > & ePoints );
  // compute coordinates of points on theEdge

  bool compUVByIsoIntersection (const std::list< std::list< TPoint* > >& boundaryPoints,
                                const gp_XY&                   theInitUV,
                                gp_XY&                         theUV,
                                bool &                         theIsDeformed);
  // compute UV by intersection of iso-lines found by points on edges

  bool compUVByElasticIsolines(const std::list< std::list< TPoint* > >& boundaryPoints,
                               const std::list< TPoint* >&         pointsToCompute);
  // compute UV as nodes of iso-poly-lines consisting of
  // segments keeping relative size as in the pattern

  double setFirstEdge (std::list< TopoDS_Edge > & theWire, int theFirstEdgeID);
  // choose the best first edge of theWire; return the summary distance
  // between point UV computed by isolines intersection and
  // eventual UV got from edge p-curves

  typedef std::list< std::list< TopoDS_Edge > > TListOfEdgesList;

  bool sortSameSizeWires (TListOfEdgesList &                theWireList,
                          const TListOfEdgesList::iterator& theFromWire,
                          const TListOfEdgesList::iterator& theToWire,
                          const int                         theFirstEdgeID,
                          std::list< std::list< TPoint* > >&          theEdgesPointsList );
  // sort wires in theWireList from theFromWire until theToWire,
  // the wires are set in the order to correspond to the order
  // of boundaries; after sorting, edges in the wires are put
  // in a good order, point UVs on edges are computed and points
  // are appended to theEdgesPointsList

  typedef std::set<const SMDS_MeshNode*> TNodeSet;

  void mergePoints (const bool uniteGroups);
  // Merge XYZ on edges and/or faces.

  void makePolyElements(const std::vector< const SMDS_MeshNode* >& theNodes,
                        const bool                                 toCreatePolygons,
                        const bool                                 toCreatePolyedrs);
  // prepare intermediate data to create Polygons and Polyhedrons

  void createElements(SMESH_Mesh*                                 theMesh,
                      const std::vector<const SMDS_MeshNode* >&   theNodesVector,
                      const std::list< std::list< int > > &       theElemNodeIDs,
                      const std::vector<const SMDS_MeshElement*>& theElements);
  // add elements to the mesh

  bool getFacesDefinition(const SMDS_MeshNode**                      theBndNodes,
                          const int                                  theNbBndNodes,
                          const std::vector< const SMDS_MeshNode* >& theNodes,
                          std::list< int >&                          theFaceDefs,
                          std::vector<int>&                          theQuantity);
  // fill faces definition for a volume face defined by theBndNodes
  // return true if a face definition changes
  

  bool isReversed(const SMDS_MeshNode*    theFirstNode,
                  const std::list< int >& theIdsList) const;
  // check xyz ids order in theIdsList taking into account
  // theFirstNode on a link

  void clearMesh(SMESH_Mesh* theMesh) const;
  // clear mesh elements existing on myShape in theMesh

  bool findExistingNodes( SMESH_Mesh*                           mesh,
                          const TopoDS_Shape&                   S,
                          const std::list< TPoint* > &          points,
                          std::vector< const SMDS_MeshNode* > & nodes);
  // fills nodes vector with nodes existing on a given shape

  static SMESHDS_SubMesh * getSubmeshWithElements(SMESH_Mesh*         theMesh,
                                                  const TopoDS_Shape& theShape);
  // return submesh containing elements bound to theShape in theMesh

 private:
  // fields

  typedef std::list< int > TElemDef; // element definition is its nodes ids

  bool                                 myIs2D;
  std::vector< TPoint >                myPoints;
  std::list< int >                     myKeyPointIDs;
  std::list< TElemDef >                myElemPointIDs;

  ErrorCode                            myErrorCode;
  bool                                 myIsComputed;
  bool                                 myIsBoundaryPointsFound;

  TopoDS_Shape                         myShape;
  // all functions assure that shapes are indexed so that first go
  // ordered vertices, then ordered edge, then faces and maybe a shell
  TopTools_IndexedMapOfOrientedShape   myShapeIDMap;
  std::map< int, std::list< TPoint*> > myShapeIDToPointsMap;

  // for the 2d case:
  // nb of key-points in each of pattern boundaries
  std::list< int >                     myNbKeyPntInBoundary;

  
  // to compute while applying to mesh elements, not to shapes

  std::vector<gp_XYZ>                  myXYZ;            // XYZ of nodes to create
  std::list< TElemDef >                myElemXYZIDs;     // new elements definitions
  std::map< int, const SMDS_MeshNode*> myXYZIdToNodeMap; // map XYZ id to node of a refined element
  std::vector<const SMDS_MeshElement*> myElements;       // refined elements
  std::vector<const SMDS_MeshNode*>    myOrderedNodes;

   // elements to replace with polygon or polyhedron
  std::vector<const SMDS_MeshElement*> myPolyElems;
  // definitions of new poly elements
  std::list< TElemDef >                myPolyElemXYZIDs;
  std::list< std::vector<int> >        myPolyhedronQuantities;

  // map a boundary to XYZs on it;
  // a boundary (edge or face) is defined as a set of its nodes,
  // XYZs on a boundary are indices of myXYZ s
  std::map<TNodeSet,std::list<std::list<int> > >  myIdsOnBoundary;
  // map XYZ id to element it is in
  std::map< int, std::list< TElemDef* > >         myReverseConnectivity;
};


#endif
