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
// File:      SMESH_MesherHelper.hxx
// Created:   15.02.06 14:48:09
// Author:    Sergey KUUL
//
#ifndef SMESH_MesherHelper_HeaderFile
#define SMESH_MesherHelper_HeaderFile

#include "SMESH_SMESH.hxx"

#include "SMESH_MeshEditor.hxx" // needed for many meshers
#include <SMDS_MeshNode.hxx>
#include <SMDS_QuadraticEdge.hxx>

#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt2d.hxx>

#include <map>

typedef std::pair<const SMDS_MeshNode*, const SMDS_MeshNode*> NLink;
typedef std::map<NLink, const SMDS_MeshNode*>                 NLinkNodeMap;
typedef std::map<NLink, const SMDS_MeshNode*>::iterator       ItNLinkNode;

/*!
 * \brief It helps meshers to add elements
 *
 * It allow meshers not to care about creation of medium nodes
 * when filling a quadratic mesh. Helper does it itself.
 * It defines degree of elements to create when IsQuadraticSubMesh()
 * is called.
 */

typedef std::vector<const SMDS_MeshNode* > TNodeColumn;
typedef std::map< double, TNodeColumn > TParam2ColumnMap;

class SMESH_EXPORT SMESH_MesherHelper
{
public:
  // ---------- PUBLIC UTILITIES ----------
  
  /*!
   * \brief Returns true if given node is medium
    * \param n - node to check
    * \param typeToCheck - type of elements containing the node to ask about node status
    * \retval bool - check result
   */
  static bool IsMedium(const SMDS_MeshNode*      node,
                       const SMDSAbs_ElementType typeToCheck = SMDSAbs_All);

  /*!
   * \brief Load nodes bound to face into a map of node columns
    * \param theParam2ColumnMap - map of node columns to fill
    * \param theFace - the face on which nodes are searched for
    * \param theBaseEdge - the edge nodes of which are columns' bases
    * \param theMesh - the mesh containing nodes
    * \retval bool - false if something is wrong
   * 
   * The key of the map is a normalized parameter of each
   * base node on theBaseEdge.
   * This method works in supposition that nodes on the face
   * forms a rectangular grid and elements can be quardrangles or triangles
   */
  static bool LoadNodeColumns(TParam2ColumnMap & theParam2ColumnMap,
                              const TopoDS_Face& theFace,
                              const TopoDS_Edge& theBaseEdge,
                              SMESHDS_Mesh*      theMesh);
  /*!
   * \brief Return support shape of a node
   * \param node - the node
   * \param meshDS - mesh DS
   * \retval TopoDS_Shape - found support shape
   */
  static const TopoDS_Shape& GetSubShapeByNode(const SMDS_MeshNode* node,
                                               SMESHDS_Mesh*        meshDS)
  { return meshDS->IndexToShape( node->GetPosition()->GetShapeId() ); }

  /*!
   * \brief Return a valid node index, fixing the given one if necessary
    * \param ind - node index
    * \param nbNodes - total nb of nodes
    * \retval int - valid node index
   */
  static int WrapIndex(const int ind, const int nbNodes) {
    if ( ind < 0 ) return nbNodes + ind % nbNodes;
    if ( ind >= nbNodes ) return ind % nbNodes;
    return ind;
  }

  /*!
   * \brief Return number of unique ancestors of the shape
   */
  static int NbAncestors(const TopoDS_Shape& shape,
                         const SMESH_Mesh&   mesh,
                         TopAbs_ShapeEnum    ancestorType=TopAbs_SHAPE);

public:
  // ---------- PUBLIC INSTANCE METHODS ----------

  // constructor
  SMESH_MesherHelper(SMESH_Mesh& theMesh);

  SMESH_Mesh* GetMesh() const { return myMesh; }
    
  SMESHDS_Mesh* GetMeshDS() const { return GetMesh()->GetMeshDS(); }
    
  /*!
   * Check submesh for given shape: if all elements on this shape are quadratic,
   * quadratic elements will be created. Also fill myNLinkNodeMap
   */
  bool IsQuadraticSubMesh(const TopoDS_Shape& theShape);
  /*!
   * \brief Set order of elements to create without calling IsQuadraticSubMesh()
   */
  void SetIsQuadratic(const bool theBuildQuadratic)
  { myCreateQuadratic = theBuildQuadratic; }
  /*!
   * \brief Return myCreateQuadratic flag
   */
  bool GetIsQuadratic() const { return myCreateQuadratic; }

  /*!
   * \brief To set created elements on the shape set by IsQuadraticSubMesh()
   *        or the next methods. By defaul elements are set on the shape if
   *        a mesh has no shape to be meshed
   */
  void SetElementsOnShape(bool toSet) { mySetElemOnShape = toSet; }

  /*!
   * \brief Set shape to make elements on without calling IsQuadraticSubMesh()
   */
  void SetSubShape(const int           subShapeID);//!==SMESHDS_Mesh::ShapeToIndex(shape)
  void SetSubShape(const TopoDS_Shape& subShape);
  /*!
   * \brief Return ID of the shape set by IsQuadraticSubMesh() or SetSubShape() 
    * \retval int - shape index in SMESHDS
   */
  int GetSubShapeID() const { return myShapeID; }
  /*!
   * \brief Return the shape set by IsQuadraticSubMesh() or SetSubShape() 
   */
  TopoDS_Shape GetSubShape() const  { return myShape; }

  /*!
   * Creates a node
   */
  SMDS_MeshNode* AddNode(double x, double y, double z, int ID = 0);
  /*!
   * Creates quadratic or linear edge
   */
  SMDS_MeshEdge* AddEdge(const SMDS_MeshNode* n1,
                         const SMDS_MeshNode* n2,
                         const int id = 0, 
                         const bool force3d = true);
  /*!
   * Creates quadratic or linear triangle
   */
  SMDS_MeshFace* AddFace(const SMDS_MeshNode* n1,
                         const SMDS_MeshNode* n2,
                         const SMDS_MeshNode* n3,
                         const int id=0, 
			 const bool force3d = false);
  /*!
   * Creates quadratic or linear quadrangle
   */
  SMDS_MeshFace* AddFace(const SMDS_MeshNode* n1,
                         const SMDS_MeshNode* n2,
                         const SMDS_MeshNode* n3,
                         const SMDS_MeshNode* n4,
                         const int id = 0,
			 const bool force3d = false);
  /*!
   * Creates quadratic or linear tetraahedron
   */
  SMDS_MeshVolume* AddVolume(const SMDS_MeshNode* n1,
                             const SMDS_MeshNode* n2,
                             const SMDS_MeshNode* n3,
                             const SMDS_MeshNode* n4,
                             const int id = 0,
			     const bool force3d = true);
  /*!
   * Creates quadratic or linear pyramid
   */
  SMDS_MeshVolume* AddVolume(const SMDS_MeshNode* n1,
                             const SMDS_MeshNode* n2,
                             const SMDS_MeshNode* n3,
                             const SMDS_MeshNode* n4,
                             const SMDS_MeshNode* n5,
                             const int id = 0,
			     const bool force3d = true);
  /*!
   * Creates quadratic or linear pentahedron
   */
  SMDS_MeshVolume* AddVolume(const SMDS_MeshNode* n1,
                             const SMDS_MeshNode* n2,
                             const SMDS_MeshNode* n3,
                             const SMDS_MeshNode* n4,
                             const SMDS_MeshNode* n5,
                             const SMDS_MeshNode* n6,
                             const int id = 0, 
			     const bool force3d = true);
  /*!
   * Creates quadratic or linear hexahedron
   */
  SMDS_MeshVolume* AddVolume(const SMDS_MeshNode* n1,
                             const SMDS_MeshNode* n2,
                             const SMDS_MeshNode* n3,
                             const SMDS_MeshNode* n4,
                             const SMDS_MeshNode* n5,
                             const SMDS_MeshNode* n6,
                             const SMDS_MeshNode* n7,
                             const SMDS_MeshNode* n8,
                             const int id = 0, 
			     bool force3d = true);
  /*!
   * \brief Return U of the given node on the edge
   */
  double GetNodeU(const TopoDS_Edge&   theEdge,
                  const SMDS_MeshNode* theNode);
  /*!
   * \brief Return node UV on face
    * \param inFaceNode - a node of element being created located inside a face
   */
  gp_XY GetNodeUV(const TopoDS_Face&   F,
                  const SMDS_MeshNode* n,
                  const SMDS_MeshNode* inFaceNode=0) const;
  /*!
   * \brief Check if inFaceNode argument is necessary for call GetNodeUV(F,..)
    * \retval bool - return true if the face is periodic
    *
    * if F is Null, answer about subshape set through IsQuadraticSubMesh() or
    * SetSubShape()
   */
  bool GetNodeUVneedInFaceNode(const TopoDS_Face& F = TopoDS_Face()) const;

  /*!
   * \brief Check if shape is a degenerated edge or it's vertex
    * \param subShape - edge or vertex index in SMESHDS
    * \retval bool - true if subShape is a degenerated shape
    *
    * It works only if IsQuadraticSubMesh() or SetSubShape() has been called
   */
  bool IsDegenShape(const int subShape) const
  { return myDegenShapeIds.find( subShape ) != myDegenShapeIds.end(); }
  /*!
   * \brief Check if shape is a seam edge or it's vertex
    * \param subShape - edge or vertex index in SMESHDS
    * \retval bool - true if subShape is a seam shape
    *
    * It works only if IsQuadraticSubMesh() or SetSubShape() has been called.
    * Seam shape has two 2D alternative represenations on the face
   */
  bool IsSeamShape(const int subShape) const
  { return mySeamShapeIds.find( subShape ) != mySeamShapeIds.end(); }
  /*!
   * \brief Check if shape is a seam edge or it's vertex
    * \param subShape - edge or vertex
    * \retval bool - true if subShape is a seam shape
    *
    * It works only if IsQuadraticSubMesh() or SetSubShape() has been called.
    * Seam shape has two 2D alternative represenations on the face
   */
  bool IsSeamShape(const TopoDS_Shape& subShape) const
  { return IsSeamShape( GetMeshDS()->ShapeToIndex( subShape )); }
  /*!
   * \brief Return true if an edge or a vertex encounters twice in face wire
   *  \param subShape - Id of edge or vertex
   */
  bool IsRealSeam(const int subShape) const
  { return mySeamShapeIds.find( -subShape ) != mySeamShapeIds.end(); }
  /*!
   * \brief Return true if an edge or a vertex encounters twice in face wire
   *  \param subShape - edge or vertex
   */
  bool IsRealSeam(const TopoDS_Shape& subShape) const
  { return IsRealSeam( GetMeshDS()->ShapeToIndex( subShape)); }
  /*!
   * \brief Check if the shape set through IsQuadraticSubMesh() or SetSubShape()
   *        has a seam edge
    * \retval bool - true if it has
   */
  bool HasSeam() const { return !mySeamShapeIds.empty(); }
  /*!
   * \brief Return index of periodic parametric direction of a closed face
    * \retval int - 1 for U, 2 for V direction
   */
  int GetPeriodicIndex() const { return myParIndex; }
  /*!
   * \brief Return an alternative parameter for a node on seam
   */
  double GetOtherParam(const double param) const;

  /**
   * Special function for search or creation medium node
   */
  const SMDS_MeshNode* GetMediumNode(const SMDS_MeshNode* n1,
                                     const SMDS_MeshNode* n2,
                                     const bool force3d);
  /*!
   * Auxilary function for filling myNLinkNodeMap
   */
  void AddNLinkNode(const SMDS_MeshNode* n1,
                    const SMDS_MeshNode* n2,
                    const SMDS_MeshNode* n12);
  /**
   * Auxilary function for filling myNLinkNodeMap
   */
  void AddNLinkNodeMap(const NLinkNodeMap& aMap)
    { myNLinkNodeMap.insert(aMap.begin(), aMap.end()); }

  /**
   * Returns myNLinkNodeMap
   */
  const NLinkNodeMap& GetNLinkNodeMap() const { return myNLinkNodeMap; }

  /**
   * Check mesh without geometry for: if all elements on this shape are quadratic,
   * quadratic elements will be created.
   * Used then generated 3D mesh without geometry.
   */
  enum MType{ LINEAR, QUADRATIC, COMP };
  MType IsQuadraticMesh();
  
protected:

  /*!
   * \brief Select UV on either of 2 pcurves of a seam edge, closest to the given UV
    * \param uv1 - UV on the seam
    * \param uv2 - UV within a face
    * \retval gp_Pnt2d - selected UV
   */
  gp_Pnt2d GetUVOnSeam( const gp_Pnt2d& uv1, const gp_Pnt2d& uv2 ) const;

 private:

  // Forbiden copy constructor
  SMESH_MesherHelper (const SMESH_MesherHelper& theOther) {};

  // special map for using during creation of quadratic elements
  NLinkNodeMap    myNLinkNodeMap;

  std::set< int > myDegenShapeIds;
  std::set< int > mySeamShapeIds;
  double          myPar1, myPar2; // bounds of a closed periodic surface
  int             myParIndex;     // bounds' index (1-U, 2-V)

  TopoDS_Shape    myShape;
  SMESH_Mesh*     myMesh;
  int             myShapeID;

  // to create quadratic elements
  bool            myCreateQuadratic;
  bool            mySetElemOnShape;

};


#endif
