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
//  File   : StdMeshers_Prism_3D.hxx
//  Module : SMESH
//
#ifndef _SMESH_Prism_3D_HXX_
#define _SMESH_Prism_3D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_3D_Algo.hxx"
#include "SMDS_TypeOfPosition.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_Comment.hxx"

#include <vector>

#include <Adaptor3d_Curve.hxx>
#include <Adaptor3d_Surface.hxx>
#include <Adaptor2d_Curve2d.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopTools_IndexedMapOfOrientedShape.hxx>
#include <gp_XYZ.hxx>


class SMESHDS_SubMesh;
class TopoDS_Edge;
class TopoDS_Faces;
struct TNode;

//typedef std::map<const SMDS_MeshNode*, const SMDS_MeshNode*> TNodeNodeMap;
typedef std::vector<const SMDS_MeshNode* > TNodeColumn;

// map of bottom nodes to the column of nodes above them
// (the column includes the bottom nodes)
typedef std::map< TNode, TNodeColumn > TNode2ColumnMap;
typedef std::map< double, TNodeColumn > TParam2ColumnMap;
typedef std::map< double, TNodeColumn >::const_iterator TParam2ColumnIt;

typedef TopTools_IndexedMapOfOrientedShape TBlockShapes;

// ===============================================
/*!
 * \brief Structure containing node relative data
 */
// ===============================================

struct TNode
{
  const SMDS_MeshNode* myNode;
  gp_XYZ               myParams;

  gp_XYZ GetCoords() const { return gp_XYZ( myNode->X(), myNode->Y(), myNode->Z() ); }
  gp_XYZ GetParams() const { return myParams; }
  gp_XYZ& ChangeParams() { return myParams; }
  bool HasParams() const { return myParams.X() >= 0.0; }
  SMDS_TypeOfPosition GetPositionType() const
  { return myNode ? myNode->GetPosition()->GetTypeOfPosition() : SMDS_TOP_UNSPEC; }
  bool IsNeighbor( const TNode& other ) const;

  TNode(const SMDS_MeshNode* node = 0): myNode(node), myParams(-1,-1,-1) {}
  bool operator < (const TNode& other) const { return myNode->GetID() < other.myNode->GetID(); }
};

// ===============================================================
/*!
 * \brief Tool analyzing and giving access to a prism geometry 
 *  treating it like a block, i.e. the four side faces are
 *  emulated by division/uniting of missing/excess faces.
 *  It also manage associations between block subshapes and a mesh.
 */
// ===============================================================

class STDMESHERS_EXPORT StdMeshers_PrismAsBlock: public SMESH_Block
{
public:
  /*!
   * \brief Constructor. Initialization is needed
   */
  StdMeshers_PrismAsBlock();

  ~StdMeshers_PrismAsBlock();

  /*!
   * \brief Initialization.
    * \param helper - helper loaded with mesh and 3D shape
    * \param shape3D - a closed shell or solid
    * \retval bool - false if a mesh or a shape are KO
    *
    * Analyse shape geometry and mesh.
    * If there are triangles on one of faces, it becomes 'bottom'
   */
  bool Init(SMESH_MesherHelper* helper, const TopoDS_Shape& shape3D);

  /*!
   * \brief Return problem description
   */
  SMESH_ComputeErrorPtr GetError() const { return myError; }

  /*!
   * \brief Return number of nodes on every vertical edge
    * \retval int - number of nodes including end nodes
   */
  int VerticalSize() const { return myParam2ColumnMaps[0].begin()->second.size(); }

  bool HasNotQuadElemOnTop() const { return myNotQuadOnTop; }

  /*!
   * \brief Return pointer to column of nodes
    * \param node - bottom node from which the returned column goes up
    * \retval const TNodeColumn* - the found column
   */
  const TNodeColumn* GetNodeColumn(const SMDS_MeshNode* node) const;

  /*!
   * \brief Return TParam2ColumnMap for a base edge
    * \param baseEdgeID - base edge SMESHDS Index
    * \param isReverse - columns in-block orientation
    * \retval const TParam2ColumnMap& - map
   */
  const TParam2ColumnMap& GetParam2ColumnMap(const int baseEdgeID,
                                             bool &    isReverse)
  {
    std::pair< TParam2ColumnMap*, bool > & col_frw =
      myShapeIndex2ColumnMap[ baseEdgeID ];
    isReverse = !col_frw.second;
    return * col_frw.first;
  }
  
  /*!
   * \brief Return pointer to mesh
    * \retval SMESH_Mesh - mesh
   */
  SMESH_Mesh* Mesh() const { return myHelper->GetMesh(); }

  /*!
   * \brief Return pointer to mesh DS
    * \retval SMESHDS_Mesh - mesh DS
   */
  SMESHDS_Mesh* MeshDS() const { return Mesh()->GetMeshDS(); }

  /*!
   * \brief Return submesh of a shape
    * \param shapeID - shape given by in-block index
    * \retval SMESH_subMesh* - found submesh
   */
  SMESH_subMesh* SubMesh(const int shapeID) const
  { return Mesh()->GetSubMesh( Shape( shapeID )); }

  /*!
   * \brief Return submesh DS of a shape
    * \param shapeID - shape given by in-block index
    * \retval SMESHDS_SubMesh* - found submesh DS
   */
  SMESHDS_SubMesh* SubMeshDS(const int shapeID) const
  { return SubMesh(shapeID)->GetSubMeshDS(); }

  /*!
   * \brief Return a in-block shape
    * \param shapeID - shape given by in-block index
    * \retval SMESHDS_SubMesh* - found submesh
   */
  const TopoDS_Shape& Shape(const int shapeID) const
  { return myShapeIDMap( shapeID ); }

  /*!
   * \brief Return in-block ID of a shape
    * \param shape - block subshape
    * \retval int - ID or zero if the shape has no ID
   */
  int ShapeID(const TopoDS_Shape& shape) const
  { return myShapeIDMap.FindIndex( shape ); }

  /*!
   * \brief Check curve orientation of a bootom edge
   *  \param meshDS - mesh DS
   *  \param columnsMap - node columns map of side face
   *  \param bottomEdge - the bootom edge
   *  \param sideFaceID - side face in-block ID
   *  \retval bool - true if orienation coinside with in-block froward orienation
   */
  static bool IsForwardEdge(SMESHDS_Mesh*           meshDS,
                            const TParam2ColumnMap& columnsMap,
                            const TopoDS_Edge &     bottomEdge,
                            const int               sideFaceID);
  /*!
   * \brief Find wall faces by bottom edges
    * \param mesh - the mesh
    * \param mainShape - the prism
    * \param bottomFace - the bottom face
    * \param bottomEdges - edges bounding the bottom face
    * \param wallFaces - faces list to fill in
   */
  static bool GetWallFaces( SMESH_Mesh*                     mesh,
                            const TopoDS_Shape &            mainShape,
                            const TopoDS_Shape &            bottomFace,
                            const std::list< TopoDS_Edge >& bottomEdges,
                            std::list< TopoDS_Face >&       wallFaces);

private:

  // --------------------------------------------------------------------
  /*!
   * \brief Class representing a part of a geom face or
   * a union of seleral faces. Or just an ordinary geom face
   *
   * It's parametrization is within [0,1] range.
   * It redefines Adaptor3d_Surface::Value(U,V) where U and V are within [0,1]
   */
  // --------------------------------------------------------------------
  class TSideFace: public Adaptor3d_Surface
  {
    int                             myID; //!< in-block ID
    // map used to find out real UV by it's normalized UV
    TParam2ColumnMap*               myParamToColumnMap;
    BRepAdaptor_Surface             mySurface;
    TopoDS_Edge                     myBaseEdge;
    // first and last normalized params and orientaion for each component or it-self
    std::vector< std::pair< double, double> > myParams;
    bool                            myIsForward;
    std::vector< TSideFace* >       myComponents;
    SMESH_MesherHelper *            myHelper;
  public:
    TSideFace( SMESH_MesherHelper* helper,
               const int           faceID,
               const TopoDS_Face&  face,
               const TopoDS_Edge&  baseEdge,
               TParam2ColumnMap*   columnsMap,
               const double        first = 0.0,
               const double        last = 1.0);
    TSideFace( const std::vector< TSideFace* >&             components,
               const std::vector< std::pair< double, double> > & params);
    TSideFace( const TSideFace& other );
    ~TSideFace();
    bool IsComplex() const
    { return ( NbComponents() > 0 || myParams[0].first != 0. || myParams[0].second != 1. ); }
    int FaceID() const { return myID; }
    TParam2ColumnMap* GetColumns() const { return myParamToColumnMap; }
    gp_XY GetNodeUV(const TopoDS_Face& F, const SMDS_MeshNode* n) const
    { return myHelper->GetNodeUV( F, n ); }
    const TopoDS_Edge & BaseEdge() const { return myBaseEdge; }
    int ColumnHeight() const {
      if ( NbComponents() ) return GetComponent(0)->GetColumns()->begin()->second.size();
      else                  return GetColumns()->begin()->second.size(); }
    double GetColumns(const double U, TParam2ColumnIt & col1, TParam2ColumnIt& col2 ) const;
    int NbComponents() const { return myComponents.size(); }
    TSideFace* GetComponent(const int i) const { return myComponents.at( i ); }
    void SetComponent(const int i, TSideFace* c)
    { if ( myComponents[i] ) delete myComponents[i]; myComponents[i]=c; }
    TSideFace* GetComponent(const double U, double& localU) const;
    bool IsForward() const { return myIsForward; }
    // boundary geometry for a face
    Adaptor3d_Surface* Surface() const { return new TSideFace( *this ); }
    bool GetPCurves(Adaptor2d_Curve2d* pcurv[4]) const;
    Adaptor2d_Curve2d* HorizPCurve(const bool isTop, const TopoDS_Face& horFace) const;
    Adaptor3d_Curve* HorizCurve(const bool isTop) const;
    Adaptor3d_Curve* VertiCurve(const bool isMax) const;
    TopoDS_Edge GetEdge( const int edge ) const;
    int InsertSubShapes( TBlockShapes& shapeMap ) const;
    // redefine Adaptor methods
    gp_Pnt Value(const Standard_Real U,const Standard_Real V) const;
  };

  // --------------------------------------------------------------------
  /*!
   * \brief Class emulating geometry of a vertical edge
   */
  // --------------------------------------------------------------------
  class STDMESHERS_EXPORT TVerticalEdgeAdaptor: public Adaptor3d_Curve
  {
    const TNodeColumn* myNodeColumn;
  public:
    TVerticalEdgeAdaptor( const TParam2ColumnMap* columnsMap, const double parameter );
    gp_Pnt Value(const Standard_Real U) const;
    Standard_Real FirstParameter() const { return 0; }
    Standard_Real LastParameter() const { return 1; }
  };

  // --------------------------------------------------------------------
  /*!
   * \brief Class emulating geometry of a hirizontal edge
   */
  // --------------------------------------------------------------------
  class STDMESHERS_EXPORT THorizontalEdgeAdaptor: public Adaptor3d_Curve
  {
    const TSideFace* mySide;
    double           myV;
  public:
    THorizontalEdgeAdaptor( const TSideFace* sideFace, const bool isTop)
      :mySide(sideFace), myV( isTop ? 1.0 : 0.0 ) {}
    gp_Pnt Value(const Standard_Real U) const;
    Standard_Real FirstParameter() const { return 0; }
    Standard_Real LastParameter() const { return 1; }
  };

  // --------------------------------------------------------------------
  /*!
   * \brief Class emulating pcurve on a hirizontal face
   */
  // --------------------------------------------------------------------
  class STDMESHERS_EXPORT TPCurveOnHorFaceAdaptor: public Adaptor2d_Curve2d
  {
    const TSideFace*  mySide;
    int               myZ;
    TopoDS_Face       myFace;
  public:
    TPCurveOnHorFaceAdaptor( const TSideFace*   sideFace,
                             const bool         isTop,
                             const TopoDS_Face& horFace)
      : mySide(sideFace), myFace(horFace), myZ(isTop ? mySide->ColumnHeight() - 1 : 0 ) {}
    gp_Pnt2d Value(const Standard_Real U) const;
    Standard_Real FirstParameter() const { return 0; }
    Standard_Real LastParameter() const { return 1; }
  };
  // --------------------------------------------------------------------

  bool myNotQuadOnTop;
  SMESH_MesherHelper* myHelper;
  TBlockShapes myShapeIDMap;

  // container of 4 side faces
  TSideFace*                 mySide; 
  // node columns for each base edge
  std::vector< TParam2ColumnMap > myParam2ColumnMaps;
  // to find a column for a node by edge SMESHDS Index
  std::map< int, std::pair< TParam2ColumnMap*, bool > > myShapeIndex2ColumnMap;

  SMESH_ComputeErrorPtr myError;
  /*!
   * \brief store error and comment and then return ( error == COMPERR_OK )
   */
  bool error(int error, const SMESH_Comment& comment = "") {
    myError = SMESH_ComputeError::New(error,comment);
    return myError->IsOK();
  }
  //std::vector< SMESH_subMesh* >           mySubMeshesVec; // submesh by in-block id
};

// =============================================
/*!
 * \brief Algo building prisms on a prism shape
 */
// =============================================

class STDMESHERS_EXPORT StdMeshers_Prism_3D: public SMESH_3D_Algo
{
public:
  StdMeshers_Prism_3D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_Prism_3D();

  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);

  /*!
   * \brief Enable removal of quadrangles from the bottom face and
   * triangles creation there by projection from the top
   * (sole face meshed with triangles is considered to be a bottom one).
   * If there are two faces with triangles, triangles must
   * be of the same topology, else the algo fails.
   * The method must be called before Compute()
   */
  void ProjectTriangles() { myProjectTriangles = true; }

  /*!
   * \brief Create prisms
    * \param nodeColumns - columns of nodes generated from nodes of a mesh face
    * \param helper - helper initialized by mesh and shape to add prisms to
   */
  static void AddPrisms( std::vector<const TNodeColumn*> & nodeColumns,
                         SMESH_MesherHelper*          helper);

private:

  /*!
   * \brief Find correspondence between bottom and top nodes.
   *  If elements on the bottom and top faces are topologically different,
   *  and projection is possible and allowed, perform the projection
    * \retval bool - is a success or not
   */
  bool assocOrProjBottom2Top();

  /*!
   * \brief Remove quadrangles from the top face and
   * create triangles there by projection from the bottom
    * \retval bool - a success or not
   */
  bool projectBottomToTop();

  /*!
   * \brief Set projection coordinates of a node to a face and it's subshapes
    * \param faceID - the face given by in-block ID
    * \param params - node normalized parameters
    * \retval bool - is a success
   */
  bool setFaceAndEdgesXYZ( const int faceID, const gp_XYZ& params, int z );

private:

  bool myProjectTriangles;

  StdMeshers_PrismAsBlock myBlock;
  SMESH_MesherHelper*     myHelper;

  std::vector<gp_XYZ>     myShapeXYZ; // point on each sub-shape

  // map of bottom nodes to the column of nodes above them
  // (the column includes the bottom node)
  TNode2ColumnMap         myBotToColumnMap;
};

#endif
