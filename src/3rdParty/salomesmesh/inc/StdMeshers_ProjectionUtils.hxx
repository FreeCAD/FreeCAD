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
//  SMESH SMESH : idl implementation based on 'SMESH' unit's calsses
// File      : StdMeshers_ProjectionUtils.hxx
// Created   : Thu Oct 26 15:37:24 2006
// Author    : Edward AGAPOV (eap)
//
#ifndef StdMeshers_ProjectionUtils_HeaderFile
#define StdMeshers_ProjectionUtils_HeaderFile

#include "SMESH_StdMeshers.hxx"

#include <TopTools_DataMapOfShapeShape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Face.hxx>

#include <list>
#include <map>

class TopoDS_Shape;
class SMDS_MeshNode;
class SMESH_Mesh;
class SMESH_Hypothesis;
class SMESH_subMesh;
class TopTools_IndexedDataMapOfShapeListOfShape;

/*!
 * \brief Class encapsulating methods common to Projection algorithms
 */
class StdMeshers_ProjectionUtils
{
 public:

  typedef TopTools_DataMapOfShapeShape                         TShapeShapeMap;
  typedef TopTools_IndexedDataMapOfShapeListOfShape            TAncestorMap;
  typedef std::map<const SMDS_MeshNode*, const SMDS_MeshNode*> TNodeNodeMap;

  /*!
   * \brief Looks for association of all subshapes of two shapes
    * \param theShape1 - shape 1
    * \param theMesh1 - mesh built on shape 1
    * \param theShape2 - shape 2
    * \param theMesh2 - mesh built on shape 2
    * \param theAssociation - association map to be filled that may
    *                         contain association of one or two pairs of vertices
    * \retval bool - true if association found
   */
  static bool FindSubShapeAssociation(const TopoDS_Shape& theShape1,
                                      SMESH_Mesh*         theMesh1,
                                      const TopoDS_Shape& theShape2,
                                      SMESH_Mesh*         theMesh2,
                                      TShapeShapeMap &    theAssociationMap);

  /*!
   * \brief Find association of edges of faces
    * \param face1 - face 1
    * \param VV1 - vertices of face 1
    * \param face2 - face 2
    * \param VV2 - vertices of face 2 associated with oned of face 1
    * \param edges1 - out list of edges of face 1
    * \param edges2 - out list of edges of face 2
    * \retval int - nb of edges in an outer wire in a success case, else zero
   */
  static int FindFaceAssociation(const TopoDS_Face&    face1,
                                 TopoDS_Vertex         VV1[2],
                                 const TopoDS_Face&    face2,
                                 TopoDS_Vertex         VV2[2],
                                 std::list< TopoDS_Edge > & edges1,
                                 std::list< TopoDS_Edge > & edges2);

  /*!
   * \brief Insert vertex association defined by a hypothesis into a map
    * \param theHyp - hypothesis
    * \param theAssociationMap - association map
    * \param theTargetShape - the shape theHyp assigned to
   */
  static void InitVertexAssociation( const SMESH_Hypothesis* theHyp,
                                     TShapeShapeMap &        theAssociationMap,
                                     const TopoDS_Shape&     theTargetShape);

  /*!
   * \brief Inserts association theShape1 <-> theShape2 to TShapeShapeMap
    * \param theShape1 - shape 1
    * \param theShape2 - shape 2
    * \param theAssociationMap - association map 
    * \param theBidirectional - if false, inserts theShape1 -> theShape2 association
    * \retval bool - true if there was no association for these shapes before
   */
  static bool InsertAssociation( const TopoDS_Shape& theShape1,
                                 const TopoDS_Shape& theShape2,
                                 TShapeShapeMap &    theAssociationMap,
                                 const bool          theBidirectional=true);

  static bool IsSubShape( const TopoDS_Shape& shape, SMESH_Mesh* aMesh );

  static bool IsSubShape( const TopoDS_Shape& shape, const TopoDS_Shape& mainShape );

  /*!
   * \brief Finds an edge by its vertices in a main shape of the mesh
   */
  static TopoDS_Edge GetEdgeByVertices( SMESH_Mesh*          aMesh,
                                        const TopoDS_Vertex& V1,
                                        const TopoDS_Vertex& V2);
                                        
  /*!
   * \brief Return another face sharing an edge
   * \param edgeToFaces - data map of descendants to ancestors
   */
  static TopoDS_Face GetNextFace( const TAncestorMap& edgeToFaces,
                                  const TopoDS_Edge&  edge,
                                  const TopoDS_Face&  face);
  /*!
   * \brief Return other vertex of an edge
   */
  static TopoDS_Vertex GetNextVertex(const TopoDS_Edge&   edge,
                                     const TopoDS_Vertex& vertex);

  /*!
   * \brief Return an oriented propagation edge
    * \param aMesh - mesh
    * \param fromEdge - start edge for propagation
    * \retval pair<int,TopoDS_Edge> - propagation step and found edge
   */
  static std::pair<int,TopoDS_Edge> GetPropagationEdge( SMESH_Mesh*        aMesh,
                                                        const TopoDS_Edge& anEdge,
                                                        const TopoDS_Edge& fromEdge);

  /*!
   * \brief Find corresponding nodes on two faces
    * \param face1 - the first face
    * \param mesh1 - mesh containing elements on the first face
    * \param face2 - the second face
    * \param mesh2 - mesh containing elements on the second face
    * \param assocMap - map associating subshapes of the faces
    * \param nodeIn2OutMap - map containing found matching nodes
    * \retval bool - is a success
   */
  static bool FindMatchingNodesOnFaces( const TopoDS_Face&     face1,
                                        SMESH_Mesh*            mesh1,
                                        const TopoDS_Face&     face2,
                                        SMESH_Mesh*            mesh2,
                                        const TShapeShapeMap & assocMap,
                                        TNodeNodeMap &         nodeIn2OutMap);
  /*!
   * \brief Check if the first and last vertices of an edge are the same
    * \param anEdge - the edge to check
    * \retval bool - true if same
   */
  static bool IsClosedEdge( const TopoDS_Edge& anEdge );

  /*!
   * \brief Return any subshape of a face belonging to the outer wire
    * \param face - the face
    * \param type - type of subshape to return
    * \retval TopoDS_Shape - the found subshape
   */
  static TopoDS_Shape OuterShape( const TopoDS_Face& face,
                                  TopAbs_ShapeEnum   type);

  /*!
   * \brief Check that submeshis is computed and try to compute it if is not
    * \param sm - submesh to compute
    * \param iterationNb - int used to stop infinite recursive call
    * \retval bool - true if computed
   */
  static bool MakeComputed(SMESH_subMesh * sm, const int iterationNb = 0);

  /*!
   * \brief Count nb of subshapes
    * \param shape - the shape
    * \param type - the type of subshapes to count
    * \param ignoreSame - if true, use map not to count same shapes, esle use explorer
    * \retval int - the calculated number
   */
  static int Count(const TopoDS_Shape&    shape,
                   const TopAbs_ShapeEnum type,
                   const bool             ignoreSame);

  /*!
   * \brief Set event listeners to submesh with projection algo
    * \param subMesh - submesh with projection algo
    * \param srcShape - source shape
    * \param srcMesh - source mesh
   */
  static void SetEventListener(SMESH_subMesh* subMesh,
                               TopoDS_Shape   srcShape,
                               SMESH_Mesh*    srcMesh);

  /*!
   * \brief Return true if edge is a boundary of edgeContainer
   */
  static bool IsBoundaryEdge(const TopoDS_Edge&  edge,
                             const TopoDS_Shape& edgeContainer,
                             SMESH_Mesh&         mesh);
};

#endif
