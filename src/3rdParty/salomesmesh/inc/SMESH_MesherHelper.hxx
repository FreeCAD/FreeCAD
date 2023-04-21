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

#include <Geom_Surface.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt2d.hxx>

#include <map>
#include <vector>

class GeomAPI_ProjectPointOnSurf;
class GeomAPI_ProjectPointOnCurve;
class SMESH_ProxyMesh;

typedef std::map<SMESH_TLink, const SMDS_MeshNode*>           TLinkNodeMap;
typedef std::map<SMESH_TLink, const SMDS_MeshNode*>::iterator ItTLinkNode;

typedef SMDS_Iterator<const TopoDS_Shape*>  PShapeIterator;
typedef boost::shared_ptr< PShapeIterator > PShapeIteratorPtr;
  
typedef std::vector<const SMDS_MeshNode* > TNodeColumn;
typedef std::map< double, TNodeColumn >    TParam2ColumnMap;

typedef gp_XY (*xyFunPtr)(const gp_XY& uv1, const gp_XY& uv2);

//=======================================================================
/*!
 * \brief It helps meshers to add elements and provides other utilities
 *
 * - It allows meshers not to care about creation of medium nodes
 * when filling a quadratic mesh. Helper does it itself.
 * It defines order of elements to create when IsQuadraticSubMesh()
 * is called.
 * - It provides information on a shape it is initialized with:
 * periodicity, presence of singularities etc.
 * - ...
 */
//=======================================================================

class SMESH_EXPORT SMESH_MesherHelper
{
 public:
  // ---------- PUBLIC UTILITIES ----------
  
  /*!
   * \brief Returns true if all elements of a sub-mesh are of same shape
    * \param smDS - sub-mesh to check elements of
    * \param shape - expected shape of elements
    * \param nullSubMeshRes - result value for the case of smDS == NULL
    * \retval bool - check result
   */
  static bool IsSameElemGeometry(const SMESHDS_SubMesh* smDS,
                                 SMDSAbs_GeometryType   shape,
                                 const bool             nullSubMeshRes = true);

  /*!
   * \brief Load nodes bound to face into a map of node columns
    * \param theParam2ColumnMap - map of node columns to fill
    * \param theFace - the face on which nodes are searched for
    * \param theBaseSide - the edges holding nodes on which columns' bases
    * \param theMesh - the mesh containing nodes
    * \retval bool - false if something is wrong
   * 
   * The key of the map is a normalized parameter of each
   * base node on theBaseSide. Edges in theBaseSide must be sequenced.
   * This method works in supposition that nodes on the face
   * forms a structured grid and elements can be quardrangles or triangles
   */
  static bool LoadNodeColumns(TParam2ColumnMap &            theParam2ColumnMap,
                              const TopoDS_Face&            theFace,
                              const std::list<TopoDS_Edge>& theBaseSide,
                              SMESHDS_Mesh*                 theMesh,
                              SMESH_ProxyMesh*              theProxyMesh=0);
  /*!
   * \brief Variant of LoadNodeColumns() above with theBaseSide given by one edge
   */
  static bool LoadNodeColumns(TParam2ColumnMap & theParam2ColumnMap,
                              const TopoDS_Face& theFace,
                              const TopoDS_Edge& theBaseEdge,
                              SMESHDS_Mesh*      theMesh,
                              SMESH_ProxyMesh*   theProxyMesh=0);
  /*!
   * \brief Return true if 2D mesh on FACE is structured
   */
  static bool IsStructured( SMESH_subMesh* faceSM );

  /*!
   * \brief Return true if 2D mesh on FACE is distored
   */
  static bool IsDistorted2D( SMESH_subMesh* faceSM, bool checkUV=false );

  /*!
   * \brief Returns true if given node is medium
    * \param n - node to check
    * \param typeToCheck - type of elements containing the node to ask about node status
    * \retval bool - check result
   */
  static bool IsMedium(const SMDS_MeshNode*      node,
                       const SMDSAbs_ElementType typeToCheck = SMDSAbs_All);
  /*!
   * \brief Return support shape of a node
   * \param node - the node
   * \param meshDS - mesh DS
   * \retval TopoDS_Shape - found support shape
   * \sa SMESH_Algo::VertexNode( const TopoDS_Vertex&, SMESHDS_Mesh* )
   */
  static TopoDS_Shape GetSubShapeByNode(const SMDS_MeshNode* node,
                                        const SMESHDS_Mesh*  meshDS);

  /*!
   * \brief Return a valid node index, fixing the given one if necessary
    * \param ind - node index
    * \param nbNodes - total nb of nodes
    * \retval int - valid node index
   */
  static inline int WrapIndex(int ind, const int nbNodes) {
    return (( ind %= nbNodes ) < 0 ) ? ind + nbNodes : ind;
  }

  /*!
   * \brief Return UV of a point inside a quadrilateral FACE by it's
   *        normalized parameters within a unit quadrangle and the
   *        corresponding projections on sub-shapes of the real-world FACE.
   *        The used calculation method is called Trans-Finite Interpolation (TFI).
   *  \param x,y - normalized parameters that should be in range [0,1]
   *  \param a0,a1,a2,a3 - UV of VERTEXes of the FACE == projections on VERTEXes
   *  \param p0,p1,p2,p3 - UV of the point projections on EDGEs of the FACE
   *  \return gp_XY - UV of the point on the FACE
   *
   *  Y ^              Order of those UV in the FACE is as follows.
   *    |
   *   a3   p2    a2
   *    o---x-----o
   *    |   :     |
   *    |   :UV   |
   * p3 x...O.....x p1
   *    |   :     |
   *    o---x-----o    ----> X
   *   a0   p0    a1
   */
  inline static gp_XY calcTFI(double x, double y,
                              const gp_XY& a0,const gp_XY& a1,const gp_XY& a2,const gp_XY& a3,
                              const gp_XY& p0,const gp_XY& p1,const gp_XY& p2,const gp_XY& p3);

  /*!
   * \brief Same as "gp_XY calcTFI(...)" but in 3D
   */
  inline static gp_XYZ calcTFI(double x, double y,
                               const gp_XYZ& a0,const gp_XYZ& a1,const gp_XYZ& a2,const gp_XYZ& a3,
                               const gp_XYZ& p0,const gp_XYZ& p1,const gp_XYZ& p2,const gp_XYZ& p3);
  /*!
   * \brief Count nb of sub-shapes
    * \param shape - the shape
    * \param type - the type of sub-shapes to count
    * \param ignoreSame - if true, use map not to count same shapes, else use explorer
    * \retval int - the calculated number
   */
  static int Count(const TopoDS_Shape&    shape,
                   const TopAbs_ShapeEnum type,
                   const bool             ignoreSame);

  /*!
   * \brief Return number of unique ancestors of the shape
   */
  static int NbAncestors(const TopoDS_Shape& shape,
                         const SMESH_Mesh&   mesh,
                         TopAbs_ShapeEnum    ancestorType=TopAbs_SHAPE);
  /*!
   * \brief Return iterator on ancestors of the given type
   */
  static PShapeIteratorPtr GetAncestors(const TopoDS_Shape& shape,
                                        const SMESH_Mesh&   mesh,
                                        TopAbs_ShapeEnum    ancestorType);
  /*!
   * \brief Find a common ancestor, of the given type, of two shapes
   */
  static TopoDS_Shape GetCommonAncestor(const TopoDS_Shape& shape1,
                                        const TopoDS_Shape& shape2,
                                        const SMESH_Mesh&   mesh,
                                        TopAbs_ShapeEnum    ancestorType);
  /*!
   * \brief Return orientation of sub-shape in the main shape
   */
  static TopAbs_Orientation GetSubShapeOri(const TopoDS_Shape& shape,
                                           const TopoDS_Shape& subShape);

  static bool IsSubShape( const TopoDS_Shape& shape, const TopoDS_Shape& mainShape );

  static bool IsSubShape( const TopoDS_Shape& shape, SMESH_Mesh* aMesh );

  static bool IsBlock( const TopoDS_Shape& shape );

  static double MaxTolerance( const TopoDS_Shape& shape );

  static double GetAngle( const TopoDS_Edge & E1, const TopoDS_Edge & E2,
                          const TopoDS_Face & F,  const TopoDS_Vertex & V,
                          gp_Vec* faceNormal=0);

  static bool IsClosedEdge( const TopoDS_Edge& anEdge );

  static TopoDS_Vertex IthVertex( const bool is2nd, TopoDS_Edge anEdge, const bool CumOri=true );

  static TopAbs_ShapeEnum GetGroupType(const TopoDS_Shape& group,
                                       const bool          avoidCompound=false);

  static TopoDS_Shape GetShapeOfHypothesis( const SMESHDS_Hypothesis * hyp,
                                            const TopoDS_Shape&        shape,
                                            SMESH_Mesh*                mesh);


public:
  // ---------- PUBLIC INSTANCE METHODS ----------

  // constructor
  SMESH_MesherHelper(SMESH_Mesh& theMesh);

  SMESH_Gen*    GetGen() const { return GetMesh()->GetGen(); }
    
  SMESH_Mesh*   GetMesh() const { return myMesh; }
    
  SMESHDS_Mesh* GetMeshDS() const { return GetMesh()->GetMeshDS(); }
    
  /*!
   * Check submesh for given shape: if all elements on this shape are quadratic,
   * quadratic elements will be created. Also fill myTLinkNodeMap
   */
  bool IsQuadraticSubMesh(const TopoDS_Shape& theShape);

  /*!
   * \brief Set order of elements to create without calling IsQuadraticSubMesh()
   */
  void SetIsQuadratic(const bool theBuildQuadratic)
  { myCreateQuadratic = theBuildQuadratic; }

  /*!
   * \brief Set myCreateBiQuadratic flag
   */
  void SetIsBiQuadratic(const bool theBuildBiQuadratic)
  { myCreateBiQuadratic = theBuildBiQuadratic; }
  
  /*!
   * \brief Return myCreateQuadratic flag
   */
  bool GetIsQuadratic() const { return myCreateQuadratic; }

  /*
   * \brief Find out elements orientation on a geometrical face
   */
  bool IsReversedSubMesh (const TopoDS_Face& theFace);

  /*!
   * \brief Return myCreateBiQuadratic flag
   */
  bool GetIsBiQuadratic() const { return myCreateBiQuadratic; }

  /*!
   * \brief Move medium nodes of faces and volumes to fix distorted elements
   * \param error - container of fixed distorted elements
   * \param volumeOnly - fix nodes on geom faces or not if the shape is solid
   */
  void FixQuadraticElements(SMESH_ComputeErrorPtr& error, bool volumeOnly=true);

  /*!
   * \brief To set created elements on the shape set by IsQuadraticSubMesh()
   *        or the next methods. By default elements are set on the shape if
   *        a mesh has no shape to be meshed
   */
  bool SetElementsOnShape(bool toSet)
  { bool res = mySetElemOnShape; mySetElemOnShape = toSet; return res; }

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
  const TopoDS_Shape& GetSubShape() const  { return myShape; }

  /*!
   * Creates a node (!Note ID before u=0.,v0.)
   */
  SMDS_MeshNode* AddNode(double x, double y, double z, int ID = 0, double u=0., double v=0.);
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
   * Creates bi-quadratic, quadratic or linear quadrangle
   */
  SMDS_MeshFace* AddFace(const SMDS_MeshNode* n1,
                         const SMDS_MeshNode* n2,
                         const SMDS_MeshNode* n3,
                         const SMDS_MeshNode* n4,
                         const int id = 0,
                         const bool force3d = false);
  /*!
   * Creates polygon, with additional nodes in quadratic mesh
   */
  SMDS_MeshFace* AddPolygonalFace (const std::vector<const SMDS_MeshNode*>& nodes,
                                   const int id = 0,
                                   const bool force3d = false);
  /*!
   * Creates quadratic or linear tetrahedron
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
   * Creates bi-quadratic, quadratic or linear hexahedron
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
   * Creates LINEAR!!!!!!!!! octahedron
   */
  SMDS_MeshVolume* AddVolume(const SMDS_MeshNode* n1,
                             const SMDS_MeshNode* n2,
                             const SMDS_MeshNode* n3,
                             const SMDS_MeshNode* n4,
                             const SMDS_MeshNode* n5,
                             const SMDS_MeshNode* n6,
                             const SMDS_MeshNode* n7,
                             const SMDS_MeshNode* n8,
                             const SMDS_MeshNode* n9,
                             const SMDS_MeshNode* n10,
                             const SMDS_MeshNode* n11,
                             const SMDS_MeshNode* n12,
                             const int id = 0, 
                             bool force3d = true);

  /*!
   * Creates polyhedron. In quadratic mesh, adds medium nodes
   */
  SMDS_MeshVolume* AddPolyhedralVolume (const std::vector<const SMDS_MeshNode*>& nodes,
                                        const std::vector<int>&                  quantities,
                                        const int                                ID=0,
                                        const bool                               force3d = true);
  /*!
   * \brief Enables fixing node parameters on EDGEs and FACEs by
   * GetNodeU(...,check=true), GetNodeUV(...,check=true), CheckNodeUV() and
   * CheckNodeU() in case if a node lies on a shape set via SetSubShape().
   * Default is False
   */
  void ToFixNodeParameters(bool toFix);

  /*!
   * \brief Return U of the given node on the edge
   */
  double GetNodeU(const TopoDS_Edge&   theEdge,
                  const SMDS_MeshNode* theNode,
                  const SMDS_MeshNode* inEdgeNode=0,
                  bool*                check=0) const;
  /*!
   * \brief Return node UV on face
   *  \param inFaceNode - a node of element being created located inside a face
   *  \param check - if provided, returns result of UV check that it enforces
   */
  gp_XY GetNodeUV(const TopoDS_Face&   F,
                  const SMDS_MeshNode* n,
                  const SMDS_MeshNode* inFaceNode=0,
                  bool*                check=0) const;
  /*!
   * \brief Check and fix node UV on a face
   *  \param force - check even if checks of other nodes on this face passed OK
   *  \param distXYZ - returns result distance and point coordinates
   *  \retval bool - false if UV is bad and could not be fixed
   */
  bool CheckNodeUV(const TopoDS_Face&   F,
                   const SMDS_MeshNode* n,
                   gp_XY&               uv,
                   const double         tol,
                   const bool           force=false,
                   double               distXYZ[4]=0) const;
  /*!
   * \brief Check and fix node U on an edge
   *  \param force - check even if checks of other nodes on this edge passed OK
   *  \param distXYZ - returns result distance and point coordinates
   *  \retval bool - false if U is bad and could not be fixed
   */
  bool CheckNodeU(const TopoDS_Edge&   E,
                  const SMDS_MeshNode* n,
                  double&              u,
                  const double         tol,
                  const bool           force=false,
                  double               distXYZ[4]=0) const;
  /*!
   * \brief Return middle UV taking in account surface period
   */
  static gp_XY GetMiddleUV(const Handle(Geom_Surface)& surface,
                           const gp_XY&                uv1,
                           const gp_XY&                uv2);
  /*!
   * \brief Return UV for the central node of a biquadratic triangle
   */
  static gp_XY GetCenterUV(const gp_XY& uv1,
                           const gp_XY& uv2, 
                           const gp_XY& uv3, 
                           const gp_XY& uv12,
                           const gp_XY& uv23,
                           const gp_XY& uv31,
                           bool *       isBadTria=0);
  /*!
   * \brief Define a pointer to wrapper over a function of gp_XY class,
   *       suitable to pass as xyFunPtr to ApplyIn2D().
   *       For example gp_XY_FunPtr(Added) defines pointer gp_XY_Added to function
   *       calling gp_XY::Added(gp_XY), which is to be used like following
   *       ApplyIn2D(surf, uv1, uv2, gp_XY_Added)
   */
#define gp_XY_FunPtr(meth) \
  static gp_XY __gpXY_##meth (const gp_XY& uv1, const gp_XY& uv2) { return uv1.meth( uv2 ); } \
  static xyFunPtr gp_XY_##meth = & __gpXY_##meth

  /*!
   * \brief Perform given operation on two 2d points in parameric space of given surface.
   *        It takes into account period of the surface. Use gp_XY_FunPtr macro
   *        to easily define pointer to function of gp_XY class.
   */
  static gp_XY ApplyIn2D(Handle(Geom_Surface) surface,
                         const gp_XY&         uv1,
                         const gp_XY&         uv2,
                         xyFunPtr             fun,
                         const bool           resultInPeriod=true);

  /*!
   * \brief Move node positions on a FACE within surface period
   *  \param [in] face - the FACE
   *  \param [inout] uv - node positions to adjust
   *  \param [in] nbUV - nb of \a uv
   */
  void AdjustByPeriod( const TopoDS_Face& face, gp_XY uv[], const int nbUV );

  /*!
   * \brief Check if inFaceNode argument is necessary for call GetNodeUV(F,..)
   *  \retval bool - return true if the face is periodic
   *
   * If F is Null, answer about subshape set through IsQuadraticSubMesh() or
   * SetSubShape()
   */
  bool GetNodeUVneedInFaceNode(const TopoDS_Face& F = TopoDS_Face()) const;

  /*!
   * \brief Return projector initialized by given face without location, which is returned
   */
  GeomAPI_ProjectPointOnSurf& GetProjector(const TopoDS_Face& F,
                                           TopLoc_Location&   loc,
                                           double             tol=0 ) const; 
  /*!
   * \brief Return a cached ShapeAnalysis_Surface of a FACE
   */
  Handle(ShapeAnalysis_Surface) GetSurface(const TopoDS_Face& F ) const;

  /*!
   * \brief Check if shape is a degenerated edge or it's vertex
   *  \param subShape - edge or vertex index in SMESHDS
   *  \retval bool - true if subShape is a degenerated shape
   *
   * It works only if IsQuadraticSubMesh() or SetSubShape() has been called
   */
  bool IsDegenShape(const int subShape) const
  { return myDegenShapeIds.find( subShape ) != myDegenShapeIds.end(); }
  /*!
   * \brief Check if the shape set through IsQuadraticSubMesh() or SetSubShape()
   *        has a degenerated edges
    * \retval bool - true if it has
   */
  bool HasDegeneratedEdges() const { return !myDegenShapeIds.empty(); }

  /*!
   * \brief Check if shape is a seam edge or it's vertex
    * \param subShape - edge or vertex index in SMESHDS
    * \retval bool - true if subShape is a seam shape
    *
    * It works only if IsQuadraticSubMesh() or SetSubShape() has been called.
    * Seam shape has two 2D alternative representations on the face
   */
  bool IsSeamShape(const int subShape) const
  { return mySeamShapeIds.find( subShape ) != mySeamShapeIds.end(); }
  /*!
   * \brief Check if shape is a seam edge or it's vertex
    * \param subShape - edge or vertex
    * \retval bool - true if subShape is a seam shape
    *
    * It works only if IsQuadraticSubMesh() or SetSubShape() has been called.
    * Seam shape has two 2D alternative representations on the face
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
   *        has a seam edge, i.e. an edge that has two parametric representations
   *        on a surface
   *  \retval bool - true if it has
   */
  bool HasSeam() const { return !mySeamShapeIds.empty(); }
  /*!
   * \brief Check if the shape set through IsQuadraticSubMesh() or SetSubShape()
   *        has a seam edge that encounters twice in a wire
   *  \retval bool - true if it has
   */
  bool HasRealSeam() const { return HasSeam() && ( *mySeamShapeIds.begin() < 0 ); }
  /*!
   * \brief Return index of periodic parametric direction of a closed face
   *  \retval int - 1 for U, 2 for V direction
   */
  int GetPeriodicIndex() const { return myParIndex; }
  /*!
   * \brief Return an alternative parameter for a node on seam
   */
  double GetOtherParam(const double param) const;

  /*!
   * \brief Return existing or create new medium nodes between given ones
   *  \param force3d - true means node creation at the middle between the
   *                   two given nodes, else node position is found on its
   *                   supporting geometrical shape, if any.
   *  \param expectedSupport - shape type corresponding to element being created
   *                           , e.g TopAbs_EDGE if SMDSAbs_Edge is created
   *                           basing on \a n1 and \a n2
   */
  const SMDS_MeshNode* GetMediumNode(const SMDS_MeshNode* n1,
                                     const SMDS_MeshNode* n2,
                                     const bool           force3d,
                                     TopAbs_ShapeEnum     expectedSupport=TopAbs_SHAPE);
  /*!
   * \brief Return existing or create a new central node for a quardilateral
   *       quadratic face given its 8 nodes.
   *  \param force3d - true means node creation in between the given nodes,
   *                   else node position is found on a geometrical face if any.
   */
  const SMDS_MeshNode* GetCentralNode(const SMDS_MeshNode* n1,
                                      const SMDS_MeshNode* n2,
                                      const SMDS_MeshNode* n3,
                                      const SMDS_MeshNode* n4,
                                      const SMDS_MeshNode* n12,
                                      const SMDS_MeshNode* n23,
                                      const SMDS_MeshNode* n34,
                                      const SMDS_MeshNode* n41,
                                      bool                 force3d);
  /*!
   * \brief Return existing or create a new central node for a 
   *       quadratic triangle given its 6 nodes.
   *  \param force3d - true means node creation in between the given nodes,
   *                   else node position is found on a geometrical face if any.
   */
  const SMDS_MeshNode* GetCentralNode(const SMDS_MeshNode* n1,
                                      const SMDS_MeshNode* n2,
                                      const SMDS_MeshNode* n3,
                                      const SMDS_MeshNode* n12,
                                      const SMDS_MeshNode* n23,
                                      const SMDS_MeshNode* n31,
                                      bool                 force3d);
  /*!
   * \brief Return index and type of the shape (EDGE or FACE only) to set a medium node on
   */
  std::pair<int, TopAbs_ShapeEnum> GetMediumPos(const SMDS_MeshNode* n1,
                                                const SMDS_MeshNode* n2,
                                                const bool           useCurSubShape=false,
                                                TopAbs_ShapeEnum     expectedSupport=TopAbs_SHAPE);
  /*!
   * \brief Add a link in my data structure
   */
  void AddTLinkNode(const SMDS_MeshNode* n1,
                    const SMDS_MeshNode* n2,
                    const SMDS_MeshNode* n12);
  /*!
   * \brief Add many links in my data structure
   */
  void AddTLinkNodeMap(const TLinkNodeMap& aMap)
    { myTLinkNodeMap.insert(aMap.begin(), aMap.end()); }

  bool AddTLinks(const SMDS_MeshEdge*   edge);
  bool AddTLinks(const SMDS_MeshFace*   face);
  bool AddTLinks(const SMDS_MeshVolume* vol);

  /**
   * Returns myTLinkNodeMap
   */
  const TLinkNodeMap& GetTLinkNodeMap() const { return myTLinkNodeMap; }

  /**
   * Check mesh without geometry for: if all elements on this shape are quadratic,
   * quadratic elements will be created.
   * Used then generated 3D mesh without geometry.
   */
  enum MType{ LINEAR, QUADRATIC, COMP };
  MType IsQuadraticMesh();
  
  virtual ~SMESH_MesherHelper();

 protected:

  /*!
   * \brief Select UV on either of 2 pcurves of a seam edge, closest to the given UV
   *  \param uv1 - UV on the seam
   *  \param uv2 - UV within a face
   *  \retval gp_Pnt2d - selected UV
   */
  gp_Pnt2d getUVOnSeam( const gp_Pnt2d& uv1, const gp_Pnt2d& uv2 ) const;

  const SMDS_MeshNode* getMediumNodeOnComposedWire(const SMDS_MeshNode* n1,
                                                   const SMDS_MeshNode* n2,
                                                   bool                 force3d);

  double getFaceMaxTol( const TopoDS_Shape& face ) const;

 private:

  // Forbidden copy constructor
  SMESH_MesherHelper (const SMESH_MesherHelper& theOther);

  // key of a map of bi-quadratic face to it's central node
  struct TBiQuad: public std::pair<int, std::pair<int, int> >
  {
    TBiQuad(const SMDS_MeshNode* n1,
            const SMDS_MeshNode* n2, 
            const SMDS_MeshNode* n3,
            const SMDS_MeshNode* n4=0)
    {
      TIDSortedNodeSet s;
      s.insert(n1);
      s.insert(n2);
      s.insert(n3);
      if ( n4 ) s.insert(n4);
      TIDSortedNodeSet::iterator n = s.begin();
      first = (*n++)->GetID();
      second.first = (*n++)->GetID();
      second.second = (*n++)->GetID();
    }
  };

  // maps used during creation of quadratic elements
  TLinkNodeMap                              myTLinkNodeMap;       // medium nodes on links
  std::map< TBiQuad, const SMDS_MeshNode* > myMapWithCentralNode; // central nodes of faces

  std::set< int > myDegenShapeIds;
  std::set< int > mySeamShapeIds;
  double          myPar1[2], myPar2[2]; // U and V bounds of a closed periodic surface
  int             myParIndex;     // bounds' index (1-U, 2-V, 3-both)

  std::map< int, double > myFaceMaxTol;

  typedef std::map< int, Handle(ShapeAnalysis_Surface)> TID2Surface;
  typedef std::map< int, GeomAPI_ProjectPointOnSurf* >  TID2ProjectorOnSurf;
  typedef std::map< int, GeomAPI_ProjectPointOnCurve* > TID2ProjectorOnCurve;
  mutable TID2Surface  myFace2Surface;
  TID2ProjectorOnSurf  myFace2Projector;
  TID2ProjectorOnCurve myEdge2Projector;

  TopoDS_Shape    myShape;
  SMESH_Mesh*     myMesh;
  int             myShapeID;

  bool            myCreateQuadratic;
  bool            myCreateBiQuadratic;
  bool            mySetElemOnShape;
  bool            myFixNodeParameters;

  std::map< int,bool > myNodePosShapesValidity;
  bool toCheckPosOnShape(int shapeID ) const;
  void setPosOnShapeValidity(int shapeID, bool ok ) const;
};

//=======================================================================
inline gp_XY
SMESH_MesherHelper::calcTFI(double x, double y,
                            const gp_XY& a0,const gp_XY& a1,const gp_XY& a2,const gp_XY& a3,
                            const gp_XY& p0,const gp_XY& p1,const gp_XY& p2,const gp_XY& p3)
{
  return
    ((1 - y) * p0 + x * p1 + y * p2 + (1 - x) * p3 ) -
    ((1 - x) * (1 - y) * a0 + x * (1 - y) * a1 + x * y * a2 + (1 - x) * y * a3);
}
//=======================================================================
inline gp_XYZ
SMESH_MesherHelper::calcTFI(double x, double y,
                            const gp_XYZ& a0,const gp_XYZ& a1,const gp_XYZ& a2,const gp_XYZ& a3,
                            const gp_XYZ& p0,const gp_XYZ& p1,const gp_XYZ& p2,const gp_XYZ& p3)
{
  return
    ((1 - y) * p0 + x * p1 + y * p2 + (1 - x) * p3 ) -
    ((1 - x) * (1 - y) * a0 + x * (1 - y) * a1 + x * y * a2 + (1 - x) * y * a3);
}
//=======================================================================

#endif
