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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : StdMeshers_Prism_3D.hxx
//  Module : SMESH
//
#ifndef _SMESH_Prism_3D_HXX_
#define _SMESH_Prism_3D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMDS_TypeOfPosition.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_TypeDefs.hxx"
#include "SMESH_subMesh.hxx"

#include <Adaptor2d_Curve2d.hxx>
#include <Adaptor3d_Curve.hxx>
#include <Adaptor3d_Surface.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopTools_IndexedMapOfOrientedShape.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Trsf.hxx>
#include <gp_XYZ.hxx>

#include <vector>

namespace Prism_3D
{
  struct TNode;
  struct TPrismTopo;
}
namespace StdMeshers_ProjectionUtils
{
  class TrsfFinder3D;
}
class SMESHDS_SubMesh;
class TopoDS_Edge;

typedef TopTools_IndexedMapOfOrientedShape                       TBlockShapes;
typedef std::vector<const SMDS_MeshNode* >                       TNodeColumn;
typedef std::map< double,          TNodeColumn >                 TParam2ColumnMap;
typedef std::map< double,          TNodeColumn >::const_iterator TParam2ColumnIt;
// map of bottom nodes to the column of nodes above them
// (the column includes the bottom nodes)
typedef std::map< Prism_3D::TNode, TNodeColumn >                 TNode2ColumnMap;


namespace Prism_3D
{
  // ===============================================
  /*!
   * \brief Structure containing node relative data
   */
  struct TNode
  {
    const SMDS_MeshNode* myNode;
    mutable gp_XYZ       myParams;

    gp_XYZ GetCoords() const { return gp_XYZ( myNode->X(), myNode->Y(), myNode->Z() ); }
    gp_XYZ GetParams() const { return myParams; }
    gp_XYZ& ChangeParams() const { return myParams; }
    bool HasParams() const { return myParams.X() >= 0.0; }
    SMDS_TypeOfPosition GetPositionType() const
    { return myNode ? myNode->GetPosition()->GetTypeOfPosition() : SMDS_TOP_UNSPEC; }
    bool IsNeighbor( const TNode& other ) const;

    TNode(const SMDS_MeshNode* node = 0): myNode(node), myParams(-1,-1,-1) {}
    bool operator < (const TNode& other) const { return myNode->GetID() < other.myNode->GetID(); }
  };
  // ===============================================
  /*!
   * \brief Topological data of the prism
   */
  typedef std::list< TFaceQuadStructPtr > TQuadList;

  struct TPrismTopo
  {
    TopoDS_Shape             myShape3D;
    TopoDS_Face              myBottom;
    TopoDS_Face              myTop;
    std::list< TopoDS_Edge > myBottomEdges;
    std::vector< TQuadList>  myWallQuads; // wall sides can be vertically composite
    std::vector< int >       myRightQuadIndex; // index of right neighbour wall quad
    std::list< int >         myNbEdgesInWires;

    bool                     myNotQuadOnTop;

    void Clear();
    void SetUpsideDown();
  };
}

// ===============================================================
/*!
 * \brief Tool analyzing and giving access to a prism geometry 
 *  treating it like a block, i.e. the four side faces are
 *  emulated by division/uniting of missing/excess faces.
 *  It also manage associations between block sub-shapes and a mesh.
 */
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
   * \param prism - prism topology
   * \retval bool - false if a mesh or a shape are KO
   */
  bool Init(SMESH_MesherHelper* helper, const Prism_3D::TPrismTopo& prism);

  /*!
   * \brief Return problem description
   */
  SMESH_ComputeErrorPtr GetError() const { return myError; }

  /*!
   * \brief Free allocated memory
   */
  void Clear();

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
    * \retval const TParam2ColumnMap* - map
   */
  const TParam2ColumnMap* GetParam2ColumnMap(const int baseEdgeID,
                                             bool &    isReverse) const
  {
    std::map< int, std::pair< TParam2ColumnMap*, bool > >::const_iterator i_mo =
      myShapeIndex2ColumnMap.find( baseEdgeID );
    if ( i_mo == myShapeIndex2ColumnMap.end() ) return 0;

    const std::pair< TParam2ColumnMap*, bool >& col_frw = i_mo->second;
    isReverse = !col_frw.second;
    return col_frw.first;
  }

  /*!
   * \brief Return transformations to get coordinates of nodes of each internal layer
   *        by nodes of the bottom. Layer is a set of nodes at a certain step
   *        from bottom to top.
   */
  bool GetLayersTransformation(std::vector<gp_Trsf> &      trsf,
                               const Prism_3D::TPrismTopo& prism) const;
  
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
    * \param shape - block sub-shape
    * \retval int - ID or zero if the shape has no ID
   */
  int ShapeID(const TopoDS_Shape& shape) const
  { return myShapeIDMap.FindIndex( shape ); }

  /*!
   * \brief Check curve orientation of a bottom edge
   *  \param meshDS - mesh DS
   *  \param columnsMap - node columns map of side face
   *  \param bottomEdge - the bottom edge
   *  \param sideFaceID - side face in-block ID
   *  \retval bool - true if orientation coincides with in-block forward orientation
   */
  static bool IsForwardEdge(SMESHDS_Mesh*           meshDS,
                            const TParam2ColumnMap& columnsMap,
                            const TopoDS_Edge &     bottomEdge,
                            const int               sideFaceID);

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
    typedef boost::shared_ptr<BRepAdaptor_Surface> PSurface;

    int                             myID; //!< in-block ID
    // map used to find out real UV by it's normalized UV
    TParam2ColumnMap*               myParamToColumnMap;
    PSurface                        mySurface;
    TopoDS_Edge                     myBaseEdge;
    map< int, PSurface >            myShapeID2Surf;
    // first and last normalized params and orientation for each component or it-self
    std::vector< std::pair< double, double> > myParams; // select my columns in myParamToColumnMap
    bool                            myIsForward;
    std::vector< TSideFace* >       myComponents;
    SMESH_MesherHelper              myHelper;
  public:
    TSideFace( SMESH_Mesh&                mesh,
               const int                  faceID,
               const Prism_3D::TQuadList& quadList,
               const TopoDS_Edge&         baseEdge,
               TParam2ColumnMap*          columnsMap,
               const double               first = 0.0,
               const double               last  = 1.0);
    TSideFace( SMESH_Mesh&                                       mesh,
               const std::vector< TSideFace* >&                  components,
               const std::vector< std::pair< double, double> > & params);
    TSideFace( const TSideFace& other );
    ~TSideFace();
    bool IsComplex() const
    { return ( NbComponents() > 0 || myParams[0].first != 0. || myParams[0].second != 1. ); }
    int FaceID() const { return myID; }
    SMESH_Mesh* GetMesh() const { return myHelper.GetMesh(); }
    TParam2ColumnMap* GetColumns() const { return myParamToColumnMap; }
    gp_XY GetNodeUV(const TopoDS_Face& F, const SMDS_MeshNode* n, const SMDS_MeshNode* n2=0) const
    { return ((SMESH_MesherHelper&) myHelper).SetSubShape(F), myHelper.GetNodeUV( F, n, n2 ); }
    const TopoDS_Edge & BaseEdge() const { return myBaseEdge; }
    int ColumnHeight() const {
      if ( NbComponents() ) return GetComponent(0)->GetColumns()->begin()->second.size();
      else                  return GetColumns()->begin()->second.size(); }
    double GetColumns(const double U, TParam2ColumnIt & col1, TParam2ColumnIt& col2 ) const;
    void GetNodesAtZ(const int Z, std::map<double, const SMDS_MeshNode* >& nodes ) const;
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
    // debug
    void dumpNodes(int nbNodes) const;
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
    // debug
    void dumpNodes(int nbNodes) const;
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
    // debug
    void dumpNodes(int nbNodes) const;
  };

  // --------------------------------------------------------------------
  /*!
   * \brief Class emulating pcurve on a hirizontal face
   */
  // --------------------------------------------------------------------
  class STDMESHERS_EXPORT TPCurveOnHorFaceAdaptor: public Adaptor2d_Curve2d
  {
    std::map< double, gp_XY > myUVmap; // normalized parameter to UV on a horizontal face
  public:
    TPCurveOnHorFaceAdaptor( const TSideFace*   sideFace,
                             const bool         isTop,
                             const TopoDS_Face& horFace);
    gp_Pnt2d Value(const Standard_Real U) const;
    Standard_Real FirstParameter() const { return 0; }
    Standard_Real LastParameter() const { return 1; }
  };

  bool                  myNotQuadOnTop;
  SMESH_MesherHelper*   myHelper;
  TBlockShapes          myShapeIDMap;
  SMESH_ComputeErrorPtr myError;

  // container of 4 side faces
  TSideFace*            mySide; 
  // node columns for each base edge
  std::vector< TParam2ColumnMap >                       myParam2ColumnMaps;
  // to find a column for a node by edge SMESHDS Index
  std::map< int, std::pair< TParam2ColumnMap*, bool > > myShapeIndex2ColumnMap;

  /*!
   * \brief store error and comment and then return ( error == COMPERR_OK )
   */
  bool error(int error, const SMESH_Comment& comment = "") {
    myError = SMESH_ComputeError::New(error,comment);
    return myError->IsOK();
  }
  /*!
   * \brief Prints a script creating a normal grid on the prism side
   */
  void faceGridToPythonDump(const SMESH_Block::TShapeID face,
                            const int                   nb=10);

}; // class StdMeshers_PrismAsBlock

// ===============================================
/*!
 * \brief Tool building internal nodes in a prism
 */
struct StdMeshers_Sweeper
{
  std::vector< TNodeColumn* > myBndColumns; // boundary nodes
  std::vector< TNodeColumn* > myIntColumns; // internal nodes

  bool ComputeNodes( SMESH_MesherHelper& helper,
                     const double        tol,
                     const bool          allowHighBndError );

private:

  gp_XYZ bndPoint( int iP, int z ) const
  { return SMESH_TNodeXYZ( (*myBndColumns[ iP ])[ z ]); }

  gp_XYZ intPoint( int iP, int z ) const
  { return SMESH_TNodeXYZ( (*myIntColumns[ iP ])[ z ]); }

  static bool projectIntPoints(const std::vector< gp_XYZ >& fromBndPoints,
                               const std::vector< gp_XYZ >& toBndPoints,
                               const std::vector< gp_XYZ >& fromIntPoints,
                               std::vector< gp_XYZ >&       toIntPoints,
                               StdMeshers_ProjectionUtils::TrsfFinder3D& trsf,
                               std::vector< gp_XYZ > *      bndError);

  static void applyBoundaryError(const std::vector< gp_XYZ >& bndPoints,
                                 const std::vector< gp_XYZ >& bndError1,
                                 const std::vector< gp_XYZ >& bndError2,
                                 const double                 r,
                                 std::vector< gp_XYZ >&       toIntPoints,
                                 std::vector< double >&       int2BndDist);
};

// ===============================================
/*!
 * \brief Algo building prisms on a prism shape
 */
class STDMESHERS_EXPORT StdMeshers_Prism_3D: public SMESH_3D_Algo
{
public:
  StdMeshers_Prism_3D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_Prism_3D();

  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);

  virtual bool Evaluate(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape,
                        MapShapeNbElems& aResMap);

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
                         SMESH_MesherHelper*               helper);

  static bool IsApplicable(const TopoDS_Shape & aShape, bool toCheckAll);

 private:

  /*!
   * \brief Analyse shape geometry and mesh.
    * If there are triangles on one of faces, it becomes 'bottom'
   */
  bool initPrism(Prism_3D::TPrismTopo& thePrism,
                 const TopoDS_Shape&   theSolid,
                 const bool            selectBottom = true);

  /*!
   * \brief Fill thePrism.myWallQuads and thePrism.myTopEdges
   */
  bool getWallFaces( Prism_3D::TPrismTopo& thePrism,
                     const int             totalNbFaces);

  /*!
   * \brief Compute mesh on a SOLID
   */
  bool compute(const Prism_3D::TPrismTopo& thePrism);

  /*!
   * \brief Compute 2D mesh on walls FACEs of a prism
   */
  bool computeWalls(const Prism_3D::TPrismTopo& thePrism);

  /*!
   * \brief Returns a source EDGE of propagation to a given EDGE
   */
  TopoDS_Edge findPropagationSource( const TopoDS_Edge& E );

  /*!
   * \brief Find correspondence between bottom and top nodes.
   *  If elements on the bottom and top faces are topologically different,
   *  and projection is possible and allowed, perform the projection
    * \retval bool - is a success or not
   */
  bool assocOrProjBottom2Top( const gp_Trsf & bottomToTopTrsf,
                              const Prism_3D::TPrismTopo& thePrism);

  /*!
   * \brief Remove quadrangles from the top face and
   *        create triangles there by projection from the bottom
    * \retval bool - a success or not
   */
  bool projectBottomToTop( const gp_Trsf & bottomToTopTrsf,
                           const Prism_3D::TPrismTopo& thePrism );

  /*!
   * \brief Compute tolerance to pass to StdMeshers_Sweeper
   */
  double getSweepTolerance( const Prism_3D::TPrismTopo& thePrism );

  /*!
   * \brief Defines if it's safe to use the block approach
   */
  bool isSimpleBottom( const Prism_3D::TPrismTopo& thePrism );

  /*!
   * \brief Project mesh faces from a source FACE of one prism to
   *        a source FACE of another prism
   *  \retval bool - a success or not
   */
  bool project2dMesh(const TopoDS_Face& source, const TopoDS_Face& target);

  /*!
   * \brief Set projection coordinates of a node to a face and it's sub-shapes
    * \param faceID - the face given by in-block ID
    * \param params - node normalized parameters
    * \retval bool - is a success
   */
  bool setFaceAndEdgesXYZ( const int faceID, const gp_XYZ& params, int z );

  /*!
   * \brief If (!isOK), sets the error to a sub-mesh of a current SOLID
   */
  bool toSM( bool isOK );

  /*!
   * \brief Return index of a shape
   */
  int shapeID( const TopoDS_Shape& S );

private:

  bool myProjectTriangles;
  bool mySetErrorToSM;
  bool myUseBlock;

  StdMeshers_PrismAsBlock myBlock;
  SMESH_MesherHelper*     myHelper;

  std::vector<gp_XYZ>     myShapeXYZ; // point on each sub-shape of the block

  // map of bottom nodes to the column of nodes above them
  // (the column includes the bottom node)
  TNode2ColumnMap         myBotToColumnMap;

  TopTools_IndexedMapOfShape* myPropagChains;

}; // class StdMeshers_Prism_3D

#endif
