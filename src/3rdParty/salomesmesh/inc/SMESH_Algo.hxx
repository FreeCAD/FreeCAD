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
//  File   : SMESH_Algo.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#ifndef _SMESH_ALGO_HXX_
#define _SMESH_ALGO_HXX_

#include "SMESH_SMESH.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMDSAbs_ElementType.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_ComputeError.hxx"
#include "SMESH_Hypothesis.hxx"

#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <GeomAbs_Shape.hxx>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

class SMESHDS_Mesh;
class SMESHDS_SubMesh;
class SMESH_Gen;
class SMESH_HypoFilter;
class SMESH_Mesh;
class SMESH_MesherHelper;
class SMESH_ProxyMesh;
class SMESH_subMesh;
class TopoDS_Face;
class TopoDS_Shape;
class TopoDS_Vertex;
class TopoDS_Wire;
class gp_XYZ;

typedef std::map< SMESH_subMesh*, std::vector<int> >           MapShapeNbElems;
typedef std::map< SMESH_subMesh*, std::vector<int> >::iterator MapShapeNbElemsItr;

// ==================================================================================
/*!
 * \brief Root of all algorithms
 *
 *  Methods of the class are grouped into several parts:
 *  - main lifecycle methods, like Compute()
 *  - methods describing features of the algorithm, like NeedShape()
 *  - methods related to dependencies between sub-meshes imposed by the algorith
 *  - static utilities, like EdgeLength()
 */
// ==================================================================================

class SMESH_EXPORT SMESH_Algo : public SMESH_Hypothesis
{
 public:
  //==================================================================================
  /*!
   * \brief Structure describing algorithm features
   */
  // --------------------------------------------------------------------------------
  struct Features
  {
    int                            _dim;
    std::set<SMDSAbs_GeometryType> _inElemTypes;  // acceptable types of input mesh element
    std::set<SMDSAbs_GeometryType> _outElemTypes; // produced types of mesh elements
    std::string                    _label;        // GUI type name

    bool IsCompatible( const Features& algo2 ) const;
  };
  /*!
   * \brief Returns a structure describing algorithm features
   */
  static const Features& GetFeatures( const std::string& algoType );
  const Features&        GetFeatures() const { return GetFeatures( _name ); }

 public:
  //==================================================================================
  /*!
   * \brief Creates algorithm
    * \param hypId - algorithm ID
    * \param studyId - study ID
    * \param gen - SMESH_Gen
   */
  SMESH_Algo(int hypId, int studyId, SMESH_Gen * gen);

  /*!
   * \brief Destructor
   */
  virtual ~ SMESH_Algo();

  /*!
   * \brief Saves nothing in a stream
    * \param save - the stream
    * \retval std::ostream & - the stream
   */
  virtual std::ostream & SaveTo(std::ostream & save);

  /*!
   * \brief Loads nothing from a stream
    * \param load - the stream
    * \retval std::ostream & - the stream
   */
  virtual std::istream & LoadFrom(std::istream & load);

  /*!
   * \brief Returns all types of compatible hypotheses
   */
  const std::vector < std::string > & GetCompatibleHypothesis();

  /*!
   * \brief Check hypothesis definition to mesh a shape
    * \param aMesh - the mesh
    * \param aShape - the shape
    * \param aStatus - check result
    * \retval bool - true if hypothesis is well defined
    *
    * Textual description of a problem can be stored in _comment field.
   */
  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus) = 0;
  /*!
   * \brief Computes mesh on a shape
    * \param aMesh - the mesh
    * \param aShape - the shape
    * \retval bool - is a success
    *
    * Algorithms that !NeedDiscreteBoundary() || !OnlyUnaryInput() are
    * to set SMESH_ComputeError returned by SMESH_submesh::GetComputeError()
    * to report problematic sub-shapes
   */
  virtual bool Compute(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape) = 0;

  /*!
   * \brief Computes mesh without geometry
    * \param aMesh - the mesh
    * \param aHelper - helper that must be used for adding elements to \aaMesh
    * \retval bool - is a success
    *
    * The method is called if ( !aMesh->HasShapeToMesh() )
   */
  virtual bool Compute(SMESH_Mesh & aMesh, SMESH_MesherHelper* aHelper);

  /*!
   * \brief Sets _computeCanceled to true. It's usage depends on
   *        implementation of a particular mesher.
   */
  virtual void CancelCompute();

  /*!
   * \brief If possible, returns progress of computation [0.,1.]
   */
  virtual double GetProgress() const;

  /*!
   * \brief evaluates size of prospective mesh on a shape
   *  \param aMesh - the mesh
   *  \param aShape - the shape
   *  \param aResMap - prospective number of elements by SMDSAbs_ElementType by a sub-mesh
   *  \retval bool - is a success
   */
  virtual bool Evaluate(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape,
                        MapShapeNbElems& aResMap) = 0;

  /*!
   * \brief Returns a list of compatible hypotheses used to mesh a shape
    * \param aMesh - the mesh 
    * \param aShape - the shape
    * \param ignoreAuxiliary - do not include auxiliary hypotheses in the list
    * \retval const std::list <const SMESHDS_Hypothesis*> - hypotheses list
   * 
   *  List the hypothesis used by the algorithm associated to the shape.
   *  Hypothesis associated to father shape -are- taken into account (see
   *  GetAppliedHypothesis). Relevant hypothesis have a name (type) listed in
   *  the algorithm. This method could be surcharged by specific algorithms, in 
   *  case of several hypothesis simultaneously applicable.
   */
  virtual const std::list <const SMESHDS_Hypothesis *> &
  GetUsedHypothesis(SMESH_Mesh &         aMesh,
                    const TopoDS_Shape & aShape,
                    const bool           ignoreAuxiliary=true) const;
  /*!
   * \brief Returns a list of compatible hypotheses assigned to a shape in a mesh
    * \param aMesh - the mesh 
    * \param aShape - the shape
    * \param ignoreAuxiliary - do not include auxiliary hypotheses in the list
    * \retval const std::list <const SMESHDS_Hypothesis*> - hypotheses list
   * 
   *  List the relevant hypothesis associated to the shape. Relevant hypothesis
   *  have a name (type) listed in the algorithm. Hypothesis associated to
   *  father shape -are not- taken into account (see GetUsedHypothesis)
   */
  const std::list <const SMESHDS_Hypothesis *> &
  GetAppliedHypothesis(SMESH_Mesh &         aMesh,
                       const TopoDS_Shape & aShape,
                       const bool           ignoreAuxiliary=true) const;
  /*!
   * \brief Returns the filter recognizing only compatible hypotheses
   *  \param ignoreAuxiliary - make filter ignore compatible auxiliary hypotheses
   *  \retval SMESH_HypoFilter* - the filter that can be NULL
   */
  const SMESH_HypoFilter* GetCompatibleHypoFilter(const bool ignoreAuxiliary) const;

  /*!
   * \brief Just return false as the algorithm does not hold parameters values
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

  /*!
   * \brief return compute error
   */
  SMESH_ComputeErrorPtr GetComputeError() const;
  /*!
   * \brief initialize compute error etc. before call of Compute()
   */
  void InitComputeError();
  /*!
   * \brief Return compute progress by nb of calls of this method
   */
  double GetProgressByTic() const;
  /*!
   * Return a vector of sub-meshes to Compute()
   */
  std::vector<SMESH_subMesh*>& SubMeshesToCompute() { return _smToCompute; }

public:
  // ==================================================================
  // Algo features influencing how Compute() is called:
  // in what turn and with what input shape
  // ==================================================================

  // SMESH_Hypothesis::GetDim();
  // 1 - dimension of target mesh

  bool OnlyUnaryInput() const { return _onlyUnaryInput; }
  // 2 - is collection of tesselatable shapes inacceptable as input;
  // "collection" means a shape containing shapes of dim equal
  // to GetDim().
  // Algo which can process a collection shape should expect
  // an input temporary shape that is neither MainShape nor
  // its child.

  bool NeedDiscreteBoundary() const { return _requireDiscreteBoundary; }
  // 3 - is a Dim-1 mesh prerequisite

  bool NeedShape() const { return _requireShape; }
  // 4 - is shape existence required

  bool SupportSubmeshes() const { return _supportSubmeshes; }
  // 5 - whether supports submeshes if !NeedDiscreteBoundary()

  bool NeedLowerHyps(int dim) const { return _neededLowerHyps[ dim ]; }
  // 6 - if algo !NeedDiscreteBoundary() but requires presence of
  // hypotheses of dimension <dim> to generate all-dimensional mesh.
  // This info is used not to issue warnings on hiding of lower global algos.

public:
  // ==================================================================
  // Methods to track non hierarchical dependencies between submeshes 
  // ==================================================================

  /*!
   * \brief Sets event listener to submeshes if necessary
    * \param subMesh - submesh where algo is set
   *
   * This method is called when a submesh gets HYP_OK algo_state.
   * After being set, event listener is notified on each event of a submesh.
   * By default none listener is set
   */
  virtual void SetEventListener(SMESH_subMesh* subMesh);
  
  /*!
   * \brief Allow algo to do something after persistent restoration
    * \param subMesh - restored submesh
   *
   * This method is called only if a submesh has HYP_OK algo_state.
   */
  virtual void SubmeshRestored(SMESH_subMesh* subMesh);
  
public:
  // ==================================================================
  // Common algo utilities
  // ==================================================================
  /*!
   * \brief Fill vector of node parameters on geometrical edge, including vertex nodes
   * \param theMesh - The mesh containing nodes
   * \param theEdge - The geometrical edge of interest
   * \param theParams - The resulting vector of sorted node parameters
   * \retval bool - false if not all parameters are OK
   * \warning Nodes moved to other geometry by MergeNodes() are NOT returned.
   */
  static bool GetNodeParamOnEdge(const SMESHDS_Mesh*     theMesh,
                                 const TopoDS_Edge&      theEdge,
                                 std::vector< double > & theParams);
  /*!
   * \brief Fill map of node parameter on geometrical edge to node it-self
   * \param theMesh - The mesh containing nodes
   * \param theEdge - The geometrical edge of interest
   * \param theNodes - The resulting map
   * \param ignoreMediumNodes - to store medium nodes of quadratic elements or not
   * \param typeToCheck - type of elements to check for medium nodes
   * \retval bool - false if not all parameters are OK
   * \warning Nodes moved to other geometry by MergeNodes() are NOT returned.
   */
  static bool GetSortedNodesOnEdge(const SMESHDS_Mesh*                        theMesh,
                                   const TopoDS_Edge&                         theEdge,
                                   const bool                                 ignoreMediumNodes,
                                   std::map< double, const SMDS_MeshNode* > & theNodes,
                                   const SMDSAbs_ElementType                  typeToCheck = SMDSAbs_All);

  /*!
   * \brief Compute length of an edge
    * \param E - the edge
    * \retval double - the length
   */
  static double EdgeLength(const TopoDS_Edge & E);

  int NumberOfPoints(SMESH_Mesh& aMesh,const TopoDS_Wire& W);

  /*!
   * \brief Return continuity of two edges
    * \param E1 - the 1st edge
    * \param E2 - the 2nd edge
    * \retval GeomAbs_Shape - regularity at the junction between E1 and E2
   */
  static GeomAbs_Shape Continuity(TopoDS_Edge E1, TopoDS_Edge E2);

  /*!
   * \brief Return true if an edge can be considered as a continuation of another
   */
  static bool IsContinuous(const TopoDS_Edge & E1, const TopoDS_Edge & E2) {
    return ( Continuity( E1, E2 ) >= GeomAbs_G1 );
  }
  /*!
   * \brief Return true if an edge can be considered straight
   */
  static bool IsStraight( const TopoDS_Edge & E, const bool degenResult=false );
  /*!
   * \brief Return true if an edge has no 3D curve
   */
  static bool isDegenerated( const TopoDS_Edge & E );

  /*!
   * \brief Return the node built on a vertex
    * \param V - the vertex
    * \param meshDS - mesh data structure
    * \retval const SMDS_MeshNode* - found node or NULL
   */
  static const SMDS_MeshNode* VertexNode(const TopoDS_Vertex& V, const SMESHDS_Mesh* meshDS);

  /*!
   * \brief Return the node built on a vertex.
   *        A node moved to other geometry by MergeNodes() is also returned.
    * \param V - the vertex
    * \param mesh - mesh
    * \retval const SMDS_MeshNode* - found node or NULL
   */
  static const SMDS_MeshNode* VertexNode(const TopoDS_Vertex& V, const SMESH_Mesh* mesh);

  /*!
   * \brief Return the node built on a vertex.
   *        A node moved to other geometry by MergeNodes() is also returned.
    * \param V - the vertex
    * \param edgeSM - sub-mesh of a meshed EDGE sharing the vertex
    * \param mesh - the mesh
    * \param checkV - if \c true, presence of a node on the vertex is checked
    * \retval const SMDS_MeshNode* - found node or NULL
   */
  static const SMDS_MeshNode* VertexNode(const TopoDS_Vertex&   V,
                                         const SMESHDS_SubMesh* edgeSM,
                                         const SMESH_Mesh*      mesh,
                                         const bool             checkV=true);

  enum EMeshError { MEr_OK = 0, MEr_HOLES, MEr_BAD_ORI, MEr_EMPTY };

  /*!
   * \brief Finds topological errors of a sub-mesh 
   */
  static EMeshError GetMeshError(SMESH_subMesh* subMesh);

 protected:

  /*!
   * \brief store error and comment and then return ( error == COMPERR_OK )
   */
  bool error(int error, const SMESH_Comment& comment = "");
  /*!
   * \brief store COMPERR_ALGO_FAILED error and comment and then return false
   */
  bool error(const SMESH_Comment& comment = "")
  { return error(COMPERR_ALGO_FAILED, comment); }
  /*!
   * \brief store error and return error->IsOK()
   */
  bool error(SMESH_ComputeErrorPtr error);
  /*!
   * \brief store a bad input element preventing computation,
   *        which may be a temporary one i.e. not residing the mesh,
   *        then it will be deleted by InitComputeError()
   */
  void addBadInputElement(const SMDS_MeshElement* elem);

  void addBadInputElements(const SMESHDS_SubMesh* sm,
                           const bool             addNodes=false);

protected:

  const SMESH_HypoFilter *              _compatibleAllHypFilter;
  const SMESH_HypoFilter *              _compatibleNoAuxHypFilter;
  std::vector<std::string>              _compatibleHypothesis;
  std::list<const SMESHDS_Hypothesis *> _appliedHypList;
  std::list<const SMESHDS_Hypothesis *> _usedHypList;
  

  // Algo features influencing which Compute() and how is called:
  // in what turn and with what input shape.
  // These fields must be redefined if necessary by each descendant at constructor.
  bool _onlyUnaryInput;         // mesh one shape of GetDim() at once. Default TRUE
  bool _requireDiscreteBoundary;// GetDim()-1 mesh must be present. Default TRUE
  bool _requireShape;           // work with GetDim()-1 mesh bound to geom only. Default TRUE
  bool _supportSubmeshes;       // if !_requireDiscreteBoundary. Default FALSE
  bool _neededLowerHyps[4];     // hyp dims needed by algo that !_requireDiscreteBoundary. Df. FALSE

  // indicates if quadratic mesh creation is required,
  // is usually set like this: _quadraticMesh = SMESH_MesherHelper::IsQuadraticSubMesh(shape)
  bool _quadraticMesh;

  int         _error;    //!< SMESH_ComputeErrorName or anything algo specific
  std::string _comment;  //!< any text explaining what is wrong in Compute()
  std::list<const SMDS_MeshElement*> _badInputElements; //!< to explain COMPERR_BAD_INPUT_MESH

  volatile bool _computeCanceled; //!< is set to True while computing to stop it

  double        _progress;        /* progress of Compute() [0.,1.],
                                     to be set by an algo really tracking the progress */
  int           _progressTic;     // counter of calls from SMESH_Mesh::GetComputeProgress()
  std::vector<SMESH_subMesh*> _smToCompute; // sub-meshes to Compute()
};


class SMESH_EXPORT SMESH_0D_Algo: public SMESH_Algo
{
public:
  SMESH_0D_Algo(int hypId, int studyId,  SMESH_Gen* gen);
};

class SMESH_EXPORT SMESH_1D_Algo: public SMESH_Algo
{
public:
  SMESH_1D_Algo(int hypId, int studyId,  SMESH_Gen* gen);
};

class SMESH_EXPORT SMESH_2D_Algo: public SMESH_Algo
{
public:
  SMESH_2D_Algo(int hypId, int studyId, SMESH_Gen* gen);
  /*!
   * \brief Method in which an algorithm generating a structured mesh
   *        fixes positions of in-face nodes after there movement
   *        due to insertion of viscous layers.
   */
  virtual bool FixInternalNodes(const SMESH_ProxyMesh& mesh,
                                const TopoDS_Face&     face);
};

class SMESH_EXPORT SMESH_3D_Algo: public SMESH_Algo
{
public:
  SMESH_3D_Algo(int hypId, int studyId, SMESH_Gen* gen);
};

#endif
