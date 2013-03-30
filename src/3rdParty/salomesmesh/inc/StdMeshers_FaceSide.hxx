//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMESH : implementaion of SMESH idl descriptions
// File      : StdMeshers_FaceSide.hxx
// Created   : Wed Jan 31 18:41:25 2007
// Author    : Edward AGAPOV (eap)
// Module    : SMESH
//
#ifndef StdMeshers_FaceSide_HeaderFile
#define StdMeshers_FaceSide_HeaderFile

#include <gp_Pnt2d.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <Geom2d_Curve.hxx>
#include <TopExp.hxx>

#include "SMESH_StdMeshers.hxx"
#include "SMESH_Algo.hxx"

#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>

class SMDS_MeshNode;
class SMESH_Mesh;
class Adaptor2d_Curve2d;
class Adaptor3d_Curve;
class BRepAdaptor_CompCurve;
class TopoDS_Face;
struct SMESH_ComputeError;

typedef struct uvPtStruct
{
  double param;
  //int    curvIndex;
  double normParam;
  double u; // original 2d parameter
  double v;
  double x; // 2d parameter, normalized [0,1]
  double y;
  const SMDS_MeshNode * node;
} UVPtStruct;

class StdMeshers_FaceSide;
typedef boost::shared_ptr< StdMeshers_FaceSide > StdMeshers_FaceSidePtr;
typedef boost::shared_ptr< uvPtStruct > UVPtStructPtr;
typedef std::vector< StdMeshers_FaceSidePtr > TSideVector;
typedef boost::shared_ptr< SMESH_ComputeError > TError;

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
  StdMeshers_FaceSide(const TopoDS_Face& theFace,
                      const TopoDS_Edge& theEdge,
                      SMESH_Mesh*        theMesh,
                      const bool         theIsForward,
                      const bool         theIgnoreMediumNodes);
  /*!
   * \brief Wrap several edges. Edges must be properly ordered and oriented.
   */
  StdMeshers_FaceSide(const TopoDS_Face& theFace,
                      std::list<TopoDS_Edge>& theEdges,
                      SMESH_Mesh*        theMesh,
                      const bool         theIsForward,
                      const bool         theIgnoreMediumNodes);

  /*!
   * \brief Return wires of a face as StdMeshers_FaceSide's
   */
  static TSideVector GetFaceWires(const TopoDS_Face& theFace,
                                  SMESH_Mesh &       theMesh,
                                  const bool         theIgnoreMediumNodes,
                                  TError &           theError);  

  /*!
   * \brief Change orientation of side geometry
   */
  void Reverse();
  /*!
   * \brief Return nb nodes on edges and vertices (+1 to be == GetUVPtStruct().size() )
   */
  int NbPoints() const { return myNbPonits; }
  /*!
   * \brief Return nb edges
   */
  int NbSegments() const { return myNbSegments; }
  /*!
   * \brief Return mesh
   */
  SMESH_Mesh* GetMesh() const { return myMesh; }
  /*!
   * \brief Return true if there vertices without nodes
   */
  bool MissVertexNode() const { return myMissingVertexNodes; }
  /*!
   * \brief Return detailed data on nodes
    * \param isXConst - true if normalized parameter X is constant
    * \param constValue - constant parameter value
    *
    * Missing nodes are allowed only on internal vertices
   */
  const std::vector<UVPtStruct>& GetUVPtStruct(bool isXConst =0, double constValue =0) const;
  /*!
   * \brief Simulates detailed data on nodes
    * \param isXConst - true if normalized parameter X is constant
    * \param constValue - constant parameter value
   */
  const std::vector<UVPtStruct>& SimulateUVPtStruct(int    nbSeg,
                                               bool   isXConst   = 0,
                                               double constValue = 0) const;
  /*!
   * \brief Return edge and parameter on edge by normalized parameter
   */
  inline double Parameter(double U, TopoDS_Edge & edge) const;
  /*!
   * \brief Return UV by normalized parameter
   */
  gp_Pnt2d Value2d(double U) const;
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
   * \brief Return i-th wrapped edge (count starts from zero)
   */
  const TopoDS_Edge& Edge(int i) const { return myEdge[i]; }
  /*!
   * \brief Return 1st vertex of the i-the edge (count starts from zero)
   */
  inline TopoDS_Vertex FirstVertex(int i=0) const;
  /*!
   * \brief Return last vertex of the i-the edge (count starts from zero)
   */
  inline TopoDS_Vertex LastVertex(int i=-1) const;
  /*!
   * \brief Return first normalized parameter of the i-the edge (count starts from zero)
   */
  inline double FirstParameter(int i) const;
  /*!
   * \brief Return ast normalized parameter of the i-the edge (count starts from zero)
   */
  inline double LastParameter(int i) const;
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
  

protected:
  std::vector<uvPtStruct>           myPoints, myFalsePoints;
  std::vector<TopoDS_Edge>          myEdge;
  std::vector<Handle(Geom2d_Curve)> myC2d;
  std::vector<double>               myFirst, myLast;
  std::vector<double>               myNormPar;
  double                            myLength;
  int                               myNbPonits, myNbSegments;
  SMESH_Mesh*                       myMesh;
  bool                              myMissingVertexNodes, myIgnoreMediumNodes;
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
 * \brief Return 1st vertex of the i-the edge
 */
//================================================================================

inline TopoDS_Vertex StdMeshers_FaceSide::FirstVertex(int i) const
{
  return i < myEdge.size() ? TopExp::FirstVertex( myEdge[i], 1 ) : TopoDS_Vertex();
}

//================================================================================
/*!
 * \brief Return last vertex of the i-the edge
 */
//================================================================================

inline TopoDS_Vertex StdMeshers_FaceSide::LastVertex(int i) const
{
  return i<0 ? TopExp::LastVertex( myEdge.back(), 1) : i<myEdge.size() ? TopExp::LastVertex( myEdge[i], 1 ) : TopoDS_Vertex();
}

//================================================================================
/*!
 * \brief Return first normalized parameter of the i-the edge
 */
//================================================================================

inline double StdMeshers_FaceSide::FirstParameter(int i) const
{
  return i==0 ? 0. : i<myNormPar.size() ? myNormPar[i-1] : 1.;
}

//================================================================================
/*!
 * \brief Return ast normalized parameter of the i-the edge
 */
//================================================================================

inline double StdMeshers_FaceSide::LastParameter(int i) const
{
  return i<myNormPar.size() ? myNormPar[i] : 1;
}

#endif
