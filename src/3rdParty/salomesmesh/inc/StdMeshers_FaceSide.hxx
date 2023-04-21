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

// File      : StdMeshers_FaceSide.hxx
// Created   : Wed Jan 31 18:41:25 2007
// Author    : Edward AGAPOV (eap)
// Module    : SMESH
//
#ifndef StdMeshers_FaceSide_HeaderFile
#define StdMeshers_FaceSide_HeaderFile

#include "SMESH_StdMeshers.hxx"

#include "SMESH_ProxyMesh.hxx"

#include <Geom2d_Curve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt2d.hxx>

#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>

class SMDS_MeshNode;
class SMESH_Mesh;
class Adaptor2d_Curve2d;
class Adaptor3d_Curve;
class BRepAdaptor_CompCurve;
struct SMESH_ComputeError;
class StdMeshers_FaceSide;

typedef boost::shared_ptr< SMESH_ComputeError >  TError;
typedef boost::shared_ptr< StdMeshers_FaceSide > StdMeshers_FaceSidePtr;
typedef std::vector< StdMeshers_FaceSidePtr >    TSideVector;

//================================================================================
/*!
 * \brief Represents a side of a quasi quadrilateral face.
 * It can be composed of several edges. Gives access to geometry and 1D mesh of a side.
 */
//================================================================================

class STDMESHERS_EXPORT StdMeshers_FaceSide
{
public:
  /*!
   * \brief Wrap one edge
   */
  StdMeshers_FaceSide(const TopoDS_Face&   theFace,
                      const TopoDS_Edge&   theEdge,
                      SMESH_Mesh*          theMesh,
                      const bool           theIsForward,
                      const bool           theIgnoreMediumNodes,
                      SMESH_ProxyMesh::Ptr theProxyMesh = SMESH_ProxyMesh::Ptr());
  /*!
   * \brief Wrap several edges. Edges must be properly ordered and oriented.
   */
  StdMeshers_FaceSide(const TopoDS_Face&      theFace,
                      std::list<TopoDS_Edge>& theEdges,
                      SMESH_Mesh*             theMesh,
                      const bool              theIsForward,
                      const bool              theIgnoreMediumNodes,
                      SMESH_ProxyMesh::Ptr    theProxyMesh = SMESH_ProxyMesh::Ptr());
  /*!
   * \brief Simulate a side from a vertex using data from other FaceSide
   */
  StdMeshers_FaceSide(const StdMeshers_FaceSide*  theSide,
                      const SMDS_MeshNode*        theNode,
                      const gp_Pnt2d*             thePnt2d1,
                      const gp_Pnt2d*             thePnt2d2=NULL,
                      const Handle(Geom2d_Curve)& theC2d=NULL,
                      const double                theUFirst=0.,
                      const double                theULast=1.);
  /*!
   * \brief Create a side from an UVPtStructVec
   */
  StdMeshers_FaceSide(UVPtStructVec&     theSideNodes,
                      const TopoDS_Face& theFace = TopoDS_Face());

  // static "constructors"
  static StdMeshers_FaceSidePtr New(const TopoDS_Face&   Face,
                                    const TopoDS_Edge&   Edge,
                                    SMESH_Mesh*          Mesh,
                                    const bool           IsForward,
                                    const bool           IgnoreMediumNodes,
                                    SMESH_ProxyMesh::Ptr ProxyMesh = SMESH_ProxyMesh::Ptr())
  { return StdMeshers_FaceSidePtr
      ( new StdMeshers_FaceSide( Face,Edge,Mesh,IsForward,IgnoreMediumNodes,ProxyMesh ));
  }
  static StdMeshers_FaceSidePtr New (const TopoDS_Face&      Face,
                                     std::list<TopoDS_Edge>& Edges,
                                     SMESH_Mesh*             Mesh,
                                     const bool              IsForward,
                                     const bool              IgnoreMediumNodes,
                                     SMESH_ProxyMesh::Ptr    ProxyMesh = SMESH_ProxyMesh::Ptr())
  { return StdMeshers_FaceSidePtr
      ( new StdMeshers_FaceSide( Face,Edges,Mesh,IsForward,IgnoreMediumNodes,ProxyMesh ));
  }
  static StdMeshers_FaceSidePtr New (const StdMeshers_FaceSide*  Side,
                                     const SMDS_MeshNode*        Node,
                                     const gp_Pnt2d*             Pnt2d1,
                                     const gp_Pnt2d*             Pnt2d2=NULL,
                                     const Handle(Geom2d_Curve)& C2d=NULL,
                                     const double                UFirst=0.,
                                     const double                ULast=1.)
  { return StdMeshers_FaceSidePtr
      ( new StdMeshers_FaceSide( Side,Node,Pnt2d1,Pnt2d2,C2d,UFirst,ULast ));
  }
  static StdMeshers_FaceSidePtr New (UVPtStructVec&     theSideNodes,
                                     const TopoDS_Face& theFace = TopoDS_Face())
  {
    return StdMeshers_FaceSidePtr( new StdMeshers_FaceSide( theSideNodes, theFace ));
  }

  /*!
   * \brief Return wires of a face as StdMeshers_FaceSide's
   */
  static TSideVector GetFaceWires(const TopoDS_Face&   theFace,
                                  SMESH_Mesh &         theMesh,
                                  const bool           theIgnoreMediumNodes,
                                  TError &             theError,
                                  SMESH_ProxyMesh::Ptr theProxyMesh = SMESH_ProxyMesh::Ptr(),
                                  const bool           theCheckVertexNodes=true);
  /*!
   * \brief Change orientation of side geometry
   */
  void Reverse();
  /*!
   * \brief Make ignore medium nodes
   */
  void SetIgnoreMediumNodes(bool toIgnore);

  /*!
   * \brief Return nb nodes on edges and vertices (+1 to be == GetUVPtStruct().size() ).
   *        Call it with update == true if mesh of this side can be recomputed
   *        since creation of this side
   */
  int NbPoints(const bool update = false) const;
  /*!
   * \brief Return nb edges
   *        Call it with update == true if mesh of this side can be recomputed
   *        since creation of this side
   */
  int NbSegments(const bool update = false) const;
  /*!
   * \brief Return mesh
   */
  SMESH_Mesh* GetMesh() const { return myProxyMesh->GetMesh(); }
  /*!
   * \brief Return true if there are vertices without nodes
   */
  bool MissVertexNode() const { return myMissingVertexNodes; }

  /*!
   * \brief Return detailed data on nodes
    * \param isXConst - true if normalized parameter X is constant
    * \param constValue - constant parameter value
    *
    * Missing nodes are allowed only on internal vertices.
    * For a closed side, the 1st point repeats at end
   */
  const UVPtStructVec& GetUVPtStruct(bool isXConst =0, double constValue =0) const;
  /*!
   * \brief Simulates detailed data on nodes
    * \param isXConst - true if normalized parameter X is constant
    * \param constValue - constant parameter value
   */
  const UVPtStructVec& SimulateUVPtStruct(int    nbSeg,
                                          bool   isXConst   = 0,
                                          double constValue = 0) const;
  /*!
   * \brief Return nodes in the order they encounter while walking along
   *  the while side or a specified EDGE.
    * For a closed side, the 1st point repeats at end
   */
  std::vector<const SMDS_MeshNode*> GetOrderedNodes(int iE=-1) const;

  /*!
   * \brief Return nodes of the i-th EDGE.
   *        Nodes moved to other geometry by MergeNodes() are also returned.
   * \retval bool - is OK
   */
  bool GetEdgeNodes(const size_t                       i,
                    std::vector<const SMDS_MeshNode*>& nodes,
                    bool                               inlude1stVertex=true,
                    bool                               inludeLastVertex=true) const;

  /*!
   * \brief Return a node from the i-th VERTEX (count starts from zero)
   *        Nodes moved to other geometry by MergeNodes() are also returned.
   */
  const SMDS_MeshNode* VertexNode(std::size_t i, bool* isMoved = 0) const;

  /*!
   * \brief Return edge and parameter on edge by normalized parameter
   */
  inline double Parameter(double U, TopoDS_Edge & edge) const;
  /*!
   * \brief Return UV by normalized parameter
   */
  gp_Pnt2d Value2d(double U) const;
  /*!
   * \brief Return XYZ by normalized parameter
   */
  gp_Pnt   Value3d(double U) const;
  /*!
   * \brief Creates a Adaptor2d_Curve2d to be used in SMESH_Block
   */
  Adaptor2d_Curve2d* GetCurve2d() const;
  /*!
   * \brief Creates a fully functional Adaptor_Curve
   */
  BRepAdaptor_CompCurve* GetCurve3d() const;
  /*!
   * \brief Return nb of wrapped edges
   */
  int NbEdges() const { return myEdge.size(); }
  /*!
   * \brief Return i-th edge (count starts from zero)
   */
  const TopoDS_Edge& Edge(int i) const { return myEdge[i]; }
  /*!
   * \brief Return all edges
   */
  const std::vector<TopoDS_Edge>& Edges() const { return myEdge; }
  /*!
   * \brief Return 1st vertex of the i-th edge (count starts from zero)
   */
  TopoDS_Vertex FirstVertex(int i=0) const;
  /*!
   * \brief Return last vertex of the i-th edge (count starts from zero)
   */
  TopoDS_Vertex LastVertex(int i=-1) const;
  /*!
   * \brief Return \c true if the chain of EDGEs is closed
   */
  bool IsClosed() const;
  /*!
   * \brief Return side length
   */
  double Length() const { return myLength; }
  /*!
   * \brief Return edge index corresponding to normalized parameter
   */
  inline int EdgeIndex( double U ) const;

  //virtual gp_Pnt Value(double U) const;
  
  void dump(const char* msg=0) const;
  
  /*!
   * \brief Return ID of i-th wrapped edge (count starts from zero)
   */
  inline int EdgeID(int i) const;
  /*!
   * \brief Return p-curve of i-th wrapped edge (count starts from zero)
   */
  inline Handle(Geom2d_Curve) Curve2d(int i) const;
  /*!
   * \brief Return first normalized parameter of the i-th edge (count starts from zero)
   */
  inline double FirstParameter(int i) const;
  /*!
   * \brief Return last normalized parameter of the i-th edge (count starts from zero)
   */
  inline double LastParameter(int i) const;
  /*!
   * \brief Return first parameter of the i-th edge (count starts from zero).
   *        EDGE orientation is taken into account
   */
  inline double FirstU(int i) const;
  /*!
   * \brief Return last parameter of the i-th edge (count starts from zero).
   *        EDGE orientation is taken into account
   */
  inline double LastU(int i) const;
  /*!
   * \brief Return length of i-th wrapped edge (count starts from zero)
   */
  inline double EdgeLength(int i) const;
  /*!
   * \brief Return orientation of i-th wrapped edge (count starts from zero)
   */
  inline bool IsReversed(int i) const;

protected:

  void reverseProxySubmesh( const TopoDS_Edge& E );

  // DON't FORGET to update Reverse() when adding one more vector!
  TopoDS_Face                       myFace;
  std::vector<uvPtStruct>           myPoints, myFalsePoints;
  std::vector<TopoDS_Edge>          myEdge;
  std::vector<int>                  myEdgeID;
  std::vector<Handle(Geom2d_Curve)> myC2d;
  std::vector<GeomAdaptor_Curve>    myC3dAdaptor;
  std::vector<double>               myFirst, myLast;
  std::vector<double>               myNormPar;
  std::vector<double>               myEdgeLength;
  std::vector<int>                  myIsUniform;
  double                            myLength;
  int                               myNbPonits, myNbSegments;
  SMESH_ProxyMesh::Ptr              myProxyMesh;
  bool                              myMissingVertexNodes, myIgnoreMediumNodes;
  gp_Pnt2d                          myDefaultPnt2d;
};


//================================================================================
/*!
 * \brief Return edge index corresponding to normalized parameter
  * \param U - the parameter
  * \retval int - index
 */
//================================================================================

inline int StdMeshers_FaceSide::EdgeIndex( double U ) const
{
  int i = myNormPar.size() - 1;
  while ( i > 0 && U < myNormPar[ i-1 ] ) --i;
  return i;
}

//================================================================================
/*!
 * \brief Return edge and parameter on edge by normalized parameter
  * \param U - the parameter
  * \retval double - pameter on a curve
 */
//================================================================================

inline double StdMeshers_FaceSide::Parameter(double U, TopoDS_Edge & edge) const
{
  int i = EdgeIndex( U );
  edge = myEdge[ i ];
  double prevU = i ? myNormPar[ i-1 ] : 0;
  double r = ( U - prevU )/ ( myNormPar[ i ] - prevU );
  return myFirst[i] * ( 1 - r ) + myLast[i] * r;
}

//================================================================================
/*!
 * \brief Return first normalized parameter of the i-th edge
 */
//================================================================================

inline double StdMeshers_FaceSide::FirstParameter(int i) const
{
  return i==0 ? 0. : i<(int)myNormPar.size() ? myNormPar[i-1] : 1.;
}

//================================================================================
/*!
 * \brief Return ast normalized parameter of the i-th edge
 */
//================================================================================

inline double StdMeshers_FaceSide::LastParameter(int i) const
{
  return i < (int)myNormPar.size() ? myNormPar[i] : 1;
}

//================================================================================
/*!
 * \brief Return first parameter of the i-th edge
 */
//================================================================================

inline double StdMeshers_FaceSide::FirstU(int i) const
{
  return myFirst[ i % myFirst.size() ];
}

//================================================================================
/*!
 * \brief Return last parameter of the i-th edge
 */
//================================================================================

inline double StdMeshers_FaceSide::LastU(int i) const
{
  return myLast[ i % myLast.size() ];
}

//================================================================================
  /*!
   * \brief Return ID of i-th wrapped edge (count starts from zero)
   */
//================================================================================

inline int StdMeshers_FaceSide::EdgeID(int i) const
{
  return myEdgeID[ i % myEdgeID.size() ];
}

//================================================================================
/*!
   * \brief Return p-curve of i-th wrapped edge (count starts from zero)
   */
//================================================================================

inline Handle(Geom2d_Curve) StdMeshers_FaceSide::Curve2d(int i) const
{
  return myC2d[ i % myC2d.size() ];
}

//================================================================================
/*!
 * \brief Return length of i-th wrapped edge (count starts from zero)
 */
 //================================================================================

inline double StdMeshers_FaceSide::EdgeLength(int i) const
{
  return myEdgeLength[ i % myEdgeLength.size() ];
}

//================================================================================
/*!
 * \brief Return orientation of i-th wrapped edge (count starts from zero)
 */
 //================================================================================

inline bool StdMeshers_FaceSide::IsReversed(int i) const
{
  return myFirst[i] > myLast[i];
}

#endif
