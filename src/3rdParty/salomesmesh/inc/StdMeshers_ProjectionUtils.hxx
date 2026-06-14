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

//  SMESH SMESH : idl implementation based on 'SMESH' unit's classes
// File      : StdMeshers_ProjectionUtils.hxx
// Created   : Thu Oct 26 15:37:24 2006
// Author    : Edward AGAPOV (eap)
//
#ifndef StdMeshers_ProjectionUtils_HeaderFile
#define StdMeshers_ProjectionUtils_HeaderFile

#include "SMESH_StdMeshers.hxx"

#include "SMDS_MeshNode.hxx"

#include <TopTools_DataMapOfShapeShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_GTrsf.hxx>
#include <gp_GTrsf2d.hxx>

#include <list>
#include <map>

class SMDS_MeshNode;
class SMESH_Algo;
class SMESH_Hypothesis;
class SMESH_Mesh;
class SMESH_subMesh;
class TopoDS_Shape;

/*!
 * \brief Struct used instead of a sole TopTools_DataMapOfShapeShape to avoid
 *        problems with bidirectional bindings
 */
struct StdMeshers_ShapeShapeBiDirectionMap
{
  TopTools_DataMapOfShapeShape _map1to2, _map2to1;

  enum EAssocType {
    UNDEF, INIT_VERTEX, PROPAGATION, PARTNER, CLOSE_VERTEX, COMMON_VERTEX, FEW_EF };
  EAssocType _assocType;

  // convention: s1 - target, s2 - source
  bool Bind( const TopoDS_Shape& s1, const TopoDS_Shape& s2 )
  { _map1to2.Bind( s1, s2 ); return _map2to1.Bind( s2, s1 ); }
  bool IsBound( const TopoDS_Shape& s, const bool isShape2=false ) const 
  { return (isShape2 ? _map2to1 : _map1to2).IsBound( s ); }
  bool IsEmpty() const { return _map1to2.IsEmpty(); }
  int  Extent()  const { return _map1to2.Extent(); }
  void Clear() { _map1to2.Clear(); _map2to1.Clear(); }
  const TopoDS_Shape& operator()( const TopoDS_Shape& s, const bool isShape2=false ) const
  { // if we get a Standard_NoSuchObject here, it means that the calling code
    // passes incorrect isShape2
    return (isShape2 ? _map2to1 : _map1to2)( s );
  }
  StdMeshers_ShapeShapeBiDirectionMap() : _assocType( UNDEF ) {}
  void SetAssocType( EAssocType type ) { if ( _assocType == UNDEF ) _assocType = type; }
};

/*!
 * \brief Methods common to Projection algorithms
 */
namespace StdMeshers_ProjectionUtils
{
  typedef StdMeshers_ShapeShapeBiDirectionMap                  TShapeShapeMap;
  typedef TopTools_IndexedDataMapOfShapeListOfShape            TAncestorMap;
  typedef std::map<const SMDS_MeshNode*, const SMDS_MeshNode*,
                   TIDCompare>                                 TNodeNodeMap;


  /*!
   * \brief Finds transformation between two sets of 2D points using
   *        a least square approximation
   */
  class TrsfFinder2D
  {
    gp_GTrsf2d _trsf;
    gp_XY      _srcOrig;
  public:
    TrsfFinder2D(): _srcOrig(0,0) {}

    void Set( const gp_GTrsf2d& t ) { _trsf = t; } // it's an alternative to Solve()

    bool Solve( const std::vector< gp_XY >& srcPnts,
                const std::vector< gp_XY >& tgtPnts );

    gp_XY Transform( const gp_Pnt2d& srcUV ) const;

    bool IsIdentity() const { return ( _trsf.Form() == gp_Identity ); }
  };
  /*!
   * \brief Finds transformation between two sets of 3D points using
   *        a least square approximation
   */
  class TrsfFinder3D
  {
    gp_GTrsf _trsf;
    gp_XYZ   _srcOrig;
  public:
    TrsfFinder3D(): _srcOrig(0,0,0) {}

    void Set( const gp_GTrsf& t ) { _trsf = t; } // it's an alternative to Solve()

    bool Solve( const std::vector< gp_XYZ > & srcPnts,
                const std::vector< gp_XYZ > & tgtPnts );

    gp_XYZ Transform( const gp_Pnt& srcP ) const;

    gp_XYZ TransformVec( const gp_Vec& v ) const;

    bool IsIdentity() const { return ( _trsf.Form() == gp_Identity ); }

    bool Invert();
  };

  /*!
   * \brief Looks for association of all sub-shapes of two shapes
   * \param theShape1 - shape 1
   * \param theMesh1 - mesh built on shape 1
   * \param theShape2 - shape 2
   * \param theMesh2 - mesh built on shape 2
   * \param theAssociation - association map to be filled that may
   *                         contain association of one or two pairs of vertices
   * \retval bool - true if association found
   */
  bool FindSubShapeAssociation(const TopoDS_Shape& theShape1,
                               SMESH_Mesh*         theMesh1,
                               const TopoDS_Shape& theShape2,
                               SMESH_Mesh*         theMesh2,
                               TShapeShapeMap &    theAssociationMap);

  /*!
   * \brief Find association of edges of faces
   *  \param face1 - face 1
   *  \param VV1 - vertices of face 1
   *  \param face2 - face 2
   *  \param VV2 - vertices of face 2 associated with oned of face 1
   *  \param edges1 - out list of edges of face 1
   *  \param edges2 - out list of edges of face 2
   *  \param isClosenessAssoc - is association starting by VERTEX closeness
   *  \retval int - nb of edges in an outer wire in a success case, else zero
   */
  int FindFaceAssociation(const TopoDS_Face&         face1,
                          TopoDS_Vertex              VV1[2],
                          const TopoDS_Face&         face2,
                          TopoDS_Vertex              VV2[2],
                          std::list< TopoDS_Edge > & edges1,
                          std::list< TopoDS_Edge > & edges2,
                          const bool                 isClosenessAssoc=false);

  /*!
   * \brief Insert vertex association defined by a hypothesis into a map
   * \param theHyp - hypothesis
   * \param theAssociationMap - association map
   * \param theTargetShape - the shape theHyp assigned to
   */
  void InitVertexAssociation( const SMESH_Hypothesis* theHyp,
                              TShapeShapeMap &        theAssociationMap);

  /*!
   * \brief Inserts association theShape1 <-> theShape2 to TShapeShapeMap
   * \param theShape1 - target shape
   * \param theShape2 - source shape
   * \param theAssociationMap - association map 
   * \param theBidirectional - if false, inserts theShape1 -> theShape2 association
   * \retval bool - true if there was no association for these shapes before
   */
  bool InsertAssociation( const TopoDS_Shape& theShape1, // target
                          const TopoDS_Shape& theShape2, // source
                          TShapeShapeMap &    theAssociationMap);

  /*!
   * \brief Finds an edge by its vertices in a main shape of the mesh
   */
  TopoDS_Edge GetEdgeByVertices( SMESH_Mesh*          aMesh,
                                 const TopoDS_Vertex& V1,
                                 const TopoDS_Vertex& V2);

  /*!
   * \brief Return another face sharing an edge
   * \param edgeToFaces - data map of descendants to ancestors
   */
  TopoDS_Face GetNextFace( const TAncestorMap& edgeToFaces,
                           const TopoDS_Edge&  edge,
                           const TopoDS_Face&  face);
  /*!
   * \brief Return other vertex of an edge
   */
  TopoDS_Vertex GetNextVertex(const TopoDS_Edge&   edge,
                              const TopoDS_Vertex& vertex);

  /*!
   * \brief Return an oriented propagation edge
   * \param aMesh - mesh
   * \param fromEdge - start edge for propagation
   * \param chain - return, if provided, a propagation chain passed till
   *        anEdge; if anEdge.IsNull() then a full propagation chain is returned
   * \retval pair<int,TopoDS_Edge> - propagation step and found edge
   */
  std::pair<int,TopoDS_Edge> GetPropagationEdge( SMESH_Mesh*                 aMesh,
                                                 const TopoDS_Edge&          anEdge,
                                                 const TopoDS_Edge&          fromEdge,
                                                 TopTools_IndexedMapOfShape* chain=0);

  /*!
   * \brief Find corresponding nodes on two faces
   * \param face1 - the first face
   * \param mesh1 - mesh containing elements on the first face
   * \param face2 - the second face
   * \param mesh2 - mesh containing elements on the second face
   * \param assocMap - map associating sub-shapes of the faces
   * \param nodeIn2OutMap - map containing found matching nodes
   * \retval bool - is a success
   */
  bool FindMatchingNodesOnFaces( const TopoDS_Face&     face1,
                                 SMESH_Mesh*            mesh1,
                                 const TopoDS_Face&     face2,
                                 SMESH_Mesh*            mesh2,
                                 const TShapeShapeMap & assocMap,
                                 TNodeNodeMap &         nodeIn2OutMap);
  /*!
   * \brief Return any sub-shape of a face belonging to the outer wire
   * \param face - the face
   * \param type - type of sub-shape to return
   * \retval TopoDS_Shape - the found sub-shape
   */
  TopoDS_Shape OuterShape( const TopoDS_Face& face,
                           TopAbs_ShapeEnum   type);

  /*!
   * \brief Check that submeshis is computed and try to compute it if is not
   * \param sm - submesh to compute
   * \param iterationNb - int used to stop infinite recursive call
   * \retval bool - true if computed
   */
  bool MakeComputed(SMESH_subMesh * sm, const int iterationNb = 0);

  /*!
   * \brief Returns an error message to show in case if MakeComputed( sm ) fails.
   */
  std::string SourceNotComputedError( SMESH_subMesh * sm = 0,
                                      SMESH_Algo*     projAlgo=0);

  /*!
   * \brief Set event listeners to submesh with projection algo
   * \param subMesh - submesh with projection algo
   * \param srcShape - source shape
   * \param srcMesh - source mesh
   */
  void SetEventListener(SMESH_subMesh* subMesh,
                        TopoDS_Shape   srcShape,
                        SMESH_Mesh*    srcMesh);

  /*!
   * \brief Return a boundary EDGE (or all boundary EDGEs) of edgeContainer
   */
  TopoDS_Edge GetBoundaryEdge(const TopoDS_Shape&       edgeContainer,
                              const SMESH_Mesh&         mesh,
                              std::list< TopoDS_Edge >* allBndEdges = 0 );
};

#endif
