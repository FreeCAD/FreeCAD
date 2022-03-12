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

// File      : SMESH_MeshEditor.hxx
// Created   : Mon Apr 12 14:56:19 2004
// Author    : Edward AGAPOV (eap)
// Module    : SMESH
//
#ifndef SMESH_MeshEditor_HeaderFile
#define SMESH_MeshEditor_HeaderFile

#include "SMESH_SMESH.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMESH_Controls.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_TypeDefs.hxx"
#include "SMESH_ComputeError.hxx"

#include <utilities.h>

#include <TColStd_HSequenceOfReal.hxx>
#include <gp_Dir.hxx>

#include <list>
#include <map>
#include <set>

class SMDS_MeshFace;
class SMDS_MeshNode;
class gp_Ax1;
class gp_Vec;
class gp_Pnt;
class SMESH_MesherHelper;
struct SMESH_NodeSearcher;

// ============================================================
/*!
 * \brief Editor of a mesh
 */
// ============================================================

class SMESH_EXPORT SMESH_MeshEditor
{
public:

  SMESH_MeshEditor( SMESH_Mesh* theMesh );

  SMESH_Mesh   *                 GetMesh()   { return myMesh; }
  SMESHDS_Mesh *                 GetMeshDS() { return myMesh->GetMeshDS(); }

  const SMESH_SequenceOfElemPtr& GetLastCreatedNodes() const { return myLastCreatedNodes; }
  const SMESH_SequenceOfElemPtr& GetLastCreatedElems() const { return myLastCreatedElems; }
  void                           ClearLastCreated();
  SMESH_ComputeErrorPtr &        GetError() { return myError; }

  // --------------------------------------------------------------------------------
  struct ElemFeatures //!< Features of element to create
  {
    SMDSAbs_ElementType myType;
    bool                myIsPoly, myIsQuad;
    int                 myID;
    double              myBallDiameter;
    std::vector<int>    myPolyhedQuantities;

    SMESH_EXPORT ElemFeatures( SMDSAbs_ElementType type=SMDSAbs_All, bool isPoly=false, bool isQuad=false )
      :myType( type ), myIsPoly(isPoly), myIsQuad(isQuad), myID(-1), myBallDiameter(0) {}

    SMESH_EXPORT ElemFeatures& Init( SMDSAbs_ElementType type, bool isPoly=false, bool isQuad=false )
    { myType = type; myIsPoly = isPoly; myIsQuad = isQuad; return *this; }

    SMESH_EXPORT ElemFeatures& Init( const SMDS_MeshElement* elem, bool basicOnly=true );

    SMESH_EXPORT ElemFeatures& Init( double diameter )
    { myType = SMDSAbs_Ball; myBallDiameter = diameter; return *this; }

    SMESH_EXPORT ElemFeatures& Init( std::vector<int>& quanities, bool isQuad=false )
    { myType = SMDSAbs_Volume; myIsPoly = 1; myIsQuad = isQuad;
      myPolyhedQuantities.swap( quanities ); return *this; }

    SMESH_EXPORT ElemFeatures& Init( const std::vector<int>& quanities, bool isQuad=false )
    { myType = SMDSAbs_Volume; myIsPoly = 1; myIsQuad = isQuad;
      myPolyhedQuantities = quanities; return *this; }

    SMESH_EXPORT ElemFeatures& SetPoly(bool isPoly) { myIsPoly = isPoly; return *this; }
    SMESH_EXPORT ElemFeatures& SetQuad(bool isQuad) { myIsQuad = isQuad; return *this; }
    SMESH_EXPORT ElemFeatures& SetID  (int ID)      { myID = ID; return *this; }
  };

  /*!
   * \brief Add element
   */
  SMDS_MeshElement* AddElement(const std::vector<const SMDS_MeshNode*> & nodes,
                               const ElemFeatures&                       features);
  /*!
   * \brief Add element
   */
  SMDS_MeshElement* AddElement(const std::vector<int> & nodeIDs,
                               const ElemFeatures&      features);

  int Remove (const std::list< int >& theElemIDs, const bool isNodes);
  // Remove a node or an element.
  // Modify a compute state of sub-meshes which become empty

  void Create0DElementsOnAllNodes( const TIDSortedElemSet& elements,
                                   TIDSortedElemSet&       all0DElems);
  // Create 0D elements on all nodes of the given object except those
  // nodes on which a 0D element already exists. \a all0DElems returns
  // all 0D elements found or created on nodes of \a elements

  bool InverseDiag (const SMDS_MeshElement * theTria1,
                    const SMDS_MeshElement * theTria2 );
  // Replace two neighbour triangles with ones built on the same 4 nodes
  // but having other common link.
  // Return False if args are improper

  bool InverseDiag (const SMDS_MeshNode * theNode1,
                    const SMDS_MeshNode * theNode2 );
  // Replace two neighbour triangles sharing theNode1-theNode2 link
  // with ones built on the same 4 nodes but having other common link.
  // Return false if proper faces not found

  bool DeleteDiag (const SMDS_MeshNode * theNode1,
                   const SMDS_MeshNode * theNode2 );
  // Replace two neighbour triangles sharing theNode1-theNode2 link
  // with a quadrangle built on the same 4 nodes.
  // Return false if proper faces not found

  bool Reorient (const SMDS_MeshElement * theElement);
  // Reverse theElement orientation

  int Reorient2D (TIDSortedElemSet &       theFaces,
                  const gp_Dir&            theDirection,
                  const SMDS_MeshElement * theFace);
  // Reverse theFaces whose orientation to be same as that of theFace
  // oriented according to theDirection. Return nb of reoriented faces

  int Reorient2DBy3D (TIDSortedElemSet & theFaces,
                      TIDSortedElemSet & theVolumes,
                      const bool         theOutsideNormal);
  // Reorient faces basing on orientation of adjacent volumes.
  // Return nb of reoriented faces

  /*!
   * \brief Fuse neighbour triangles into quadrangles.
   * \param theElems     - The triangles to be fused.
   * \param theCriterion - Is used to choose a neighbour to fuse with.
   * \param theMaxAngle  - Is a max angle between element normals at which fusion
   *                       is still performed; theMaxAngle is measured in radians.
   * \return bool - Success or not.
   */
  bool TriToQuad (TIDSortedElemSet &                   theElems,
                  SMESH::Controls::NumericalFunctorPtr theCriterion,
                  const double                         theMaxAngle);
  /*!
   * \brief Split quadrangles into triangles.
   * \param theElems     - The faces to be split.
   * \param theCriterion - Is used to choose a diagonal for splitting.
   * \return bool - Success or not.
   */
  bool QuadToTri (TIDSortedElemSet &                   theElems,
                  SMESH::Controls::NumericalFunctorPtr theCriterion);
  /*!
   * \brief Split quadrangles into triangles.
   * \param theElems  - The faces to be split.
   * \param the13Diag - Is used to choose a diagonal for splitting.
   * \return bool - Success or not.
   */
  bool QuadToTri (TIDSortedElemSet & theElems,
                  const bool         the13Diag);
  /*!
   * \brief Split each of given quadrangles into 4 triangles.
   * \param theElems - The faces to be split. If empty all faces are split.
   */
  void QuadTo4Tri (TIDSortedElemSet & theElems);

  /*!
   * \brief Find better diagonal for splitting.
   * \param theQuad      - The face to find better splitting of.
   * \param theCriterion - Is used to choose a diagonal for splitting.
   * \return int - 1 for 1-3 diagonal, 2 for 2-4, -1 - for errors.
   */
  int BestSplit (const SMDS_MeshElement*              theQuad,
                 SMESH::Controls::NumericalFunctorPtr theCriterion);


  typedef std::map < const SMDS_MeshElement*, int, TIDCompare > TFacetOfElem;

    //!<2nd arg of SplitVolumes()
  enum SplitVolumToTetraFlags { HEXA_TO_5 = 1, // split into tetrahedra
                                HEXA_TO_6,
                                HEXA_TO_24,
                                HEXA_TO_2_PRISMS, // split into prisms
                                HEXA_TO_4_PRISMS };
  /*!
   * \brief Split volumic elements into tetrahedra or prisms.
   *        If facet ID < 0, element is split into tetrahedra,
   *        else a hexahedron is split into prisms so that the given facet is
   *        split into triangles
   */
  void SplitVolumes (const TFacetOfElem & theElems, const int theMethodFlags);

  /*!
   * \brief For hexahedra that will be split into prisms, finds facets to
   *        split into triangles
   *  \param [in,out] theHexas - the hexahedra
   *  \param [in]     theFacetNormal - facet normal
   *  \param [out]    theFacets - the hexahedra and found facet IDs
   */
  void GetHexaFacetsToSplit( TIDSortedElemSet& theHexas,
                             const gp_Ax1&     theFacetNormal,
                             TFacetOfElem &    theFacets);

  /*!
   * \brief Split bi-quadratic elements into linear ones without creation of additional nodes
   *   - bi-quadratic triangle will be split into 3 linear quadrangles;
   *   - bi-quadratic quadrangle will be split into 4 linear quadrangles;
   *   - tri-quadratic hexahedron will be split into 8 linear hexahedra;
   *   Quadratic elements of lower dimension  adjacent to the split bi-quadratic element
   *   will be split in order to keep the mesh conformal.
   *  \param elems - elements to split
   */
  void SplitBiQuadraticIntoLinear(TIDSortedElemSet& theElems);

  enum SmoothMethod { LAPLACIAN = 0, CENTROIDAL };

  void Smooth (TIDSortedElemSet &               theElements,
               std::set<const SMDS_MeshNode*> & theFixedNodes,
               const SmoothMethod               theSmoothMethod,
               const int                        theNbIterations,
               double                           theTgtAspectRatio = 1.0,
               const bool                       the2D = true);
  // Smooth theElements using theSmoothMethod during theNbIterations
  // or until a worst element has aspect ratio <= theTgtAspectRatio.
  // Aspect Ratio varies in range [1.0, inf].
  // If theElements is empty, the whole mesh is smoothed.
  // theFixedNodes contains additionally fixed nodes. Nodes built
  // on edges and boundary nodes are always fixed.
  // If the2D, smoothing is performed using UV parameters of nodes
  // on geometrical faces

  typedef TIDTypeCompare TElemSort;
  typedef std::map < const SMDS_MeshElement*,
    std::list<const SMDS_MeshElement*>, TElemSort >                        TTElemOfElemListMap;
  typedef std::map<const SMDS_MeshNode*, std::list<const SMDS_MeshNode*> > TNodeOfNodeListMap;
  typedef TNodeOfNodeListMap::iterator                                     TNodeOfNodeListMapItr;
  typedef std::vector<TNodeOfNodeListMapItr>                               TVecOfNnlmiMap;
  typedef std::map<const SMDS_MeshElement*, TVecOfNnlmiMap, TElemSort >    TElemOfVecOfNnlmiMap;
  typedef std::unique_ptr< std::list<int> > PGroupIDs;

  PGroupIDs RotationSweep (TIDSortedElemSet   theElements[2],
                           const gp_Ax1&      theAxis,
                           const double       theAngle,
                           const int          theNbSteps,
                           const double       theToler,
                           const bool         theMakeGroups,
                           const bool         theMakeWalls=true);
  // Generate new elements by rotation of theElements around theAxis
  // by theAngle by theNbSteps

  /*!
   * Flags of extrusion.
   * BOUNDARY: create or not boundary for result of extrusion
   * SEW:      try to use existing nodes or create new nodes in any case
   * GROUPS:   to create groups
   * BY_AVG_NORMAL: step size is measured along average normal to elements,
   *                else step size is measured along average normal of any element
   * USE_INPUT_ELEMS_ONLY: to use only input elements to compute extrusion direction
   *                       for ExtrusionByNormal()
   */
  enum ExtrusionFlags {
    EXTRUSION_FLAG_BOUNDARY = 0x01,
    EXTRUSION_FLAG_SEW = 0x02,
    EXTRUSION_FLAG_GROUPS = 0x04,
    EXTRUSION_FLAG_BY_AVG_NORMAL = 0x08,
    EXTRUSION_FLAG_USE_INPUT_ELEMS_ONLY = 0x10
  };

  /*!
   * Generator of nodes for extrusion functionality
   */
  class SMESH_EXPORT ExtrusParam {
    gp_Dir                          myDir;   // direction of extrusion
    Handle(TColStd_HSequenceOfReal) mySteps; // magnitudes for each step
    SMESH_SequenceOfNode            myNodes; // nodes for using in sewing
    int                             myFlags; // see ExtrusionFlags
    double                          myTolerance; // tolerance for sewing nodes
    const TIDSortedElemSet*         myElemsToUse; // elements to use for extrusion by normal

    int (ExtrusParam::*myMakeNodesFun)(SMESHDS_Mesh*                     mesh,
                                       const SMDS_MeshNode*              srcNode,
                                       std::list<const SMDS_MeshNode*> & newNodes,
                                       const bool                        makeMediumNodes);

  public:
    ExtrusParam( const gp_Vec&  theStep,
                 const int      theNbSteps,
                 const int      theFlags = 0,
                 const double   theTolerance = 1e-6);
    ExtrusParam( const gp_Dir&                   theDir,
                 Handle(TColStd_HSequenceOfReal) theSteps,
                 const int                       theFlags = 0,
                 const double                    theTolerance = 1e-6);
    ExtrusParam( const double theStep,
                 const int    theNbSteps,
                 const int    theFlags,
                 const int    theDim); // for extrusion by normal

    SMESH_SequenceOfNode& ChangeNodes() { return myNodes; }
    int& Flags()                   { return myFlags; }
    bool ToMakeBoundary()    const { return myFlags & EXTRUSION_FLAG_BOUNDARY; }
    bool ToMakeGroups()      const { return myFlags & EXTRUSION_FLAG_GROUPS; }
    bool ToUseInpElemsOnly() const { return myFlags & EXTRUSION_FLAG_USE_INPUT_ELEMS_ONLY; }
    int  NbSteps()           const { return mySteps->Length(); }

    // stores elements to use for extrusion by normal, depending on
    // state of EXTRUSION_FLAG_USE_INPUT_ELEMS_ONLY flag
    void SetElementsToUse( const TIDSortedElemSet& elems );

    // creates nodes and returns number of nodes added in \a newNodes
    int MakeNodes( SMESHDS_Mesh*                     mesh,
                   const SMDS_MeshNode*              srcNode,
                   std::list<const SMDS_MeshNode*> & newNodes,
                   const bool                        makeMediumNodes)
    {
      return (this->*myMakeNodesFun)( mesh, srcNode, newNodes, makeMediumNodes );
    }
  private:

    int makeNodesByDir( SMESHDS_Mesh*                     mesh,
                        const SMDS_MeshNode*              srcNode,
                        std::list<const SMDS_MeshNode*> & newNodes,
                        const bool                        makeMediumNodes);
    int makeNodesByDirAndSew( SMESHDS_Mesh*                     mesh,
                              const SMDS_MeshNode*              srcNode,
                              std::list<const SMDS_MeshNode*> & newNodes,
                              const bool                        makeMediumNodes);
    int makeNodesByNormal2D( SMESHDS_Mesh*                     mesh,
                             const SMDS_MeshNode*              srcNode,
                             std::list<const SMDS_MeshNode*> & newNodes,
                             const bool                        makeMediumNodes);
    int makeNodesByNormal1D( SMESHDS_Mesh*                     mesh,
                             const SMDS_MeshNode*              srcNode,
                             std::list<const SMDS_MeshNode*> & newNodes,
                             const bool                        makeMediumNodes);
    // step iteration
    void   beginStepIter( bool withMediumNodes );
    bool   moreSteps();
    double nextStep();
    std::vector< double > myCurSteps;
    bool                  myWithMediumNodes;
    int                   myNextStep;
  };

  /*!
   * Generate new elements by extrusion of theElements
   * It is a method used in .idl file. All functionality
   * is implemented in the next method (see below) which
   * is used in the current method.
   * @param theElems - list of elements for extrusion
   * @param newElemsMap returns history of extrusion
   * @param theFlags set flags for performing extrusion (see description
   *   of enum ExtrusionFlags for additional information)
   * @param theTolerance - uses for comparing locations of nodes if flag
   *   EXTRUSION_FLAG_SEW is set
   */
  PGroupIDs ExtrusionSweep (TIDSortedElemSet     theElems[2],
                            const gp_Vec&        theStep,
                            const int            theNbSteps,
                            TTElemOfElemListMap& newElemsMap,
                            const int            theFlags,
                            const double         theTolerance = 1.e-6);
  
  /*!
   * Generate new elements by extrusion of theElements
   * @param theElems - list of elements for extrusion
   * @param newElemsMap returns history of extrusion
   * @param theFlags set flags for performing extrusion (see description
   *   of enum ExtrusionFlags for additional information)
   * @param theTolerance - uses for comparing locations of nodes if flag
   *   EXTRUSION_FLAG_SEW is set
   * @param theParams - special structure for manage of extrusion
   */
  PGroupIDs ExtrusionSweep (TIDSortedElemSet     theElems[2],
                            ExtrusParam&         theParams,
                            TTElemOfElemListMap& newElemsMap);


  // Generate new elements by extrusion of theElements 
  // by theStep by theNbSteps

  enum Extrusion_Error {
    EXTR_OK,
    EXTR_NO_ELEMENTS, 
    EXTR_PATH_NOT_EDGE,
    EXTR_BAD_PATH_SHAPE,
    EXTR_BAD_STARTING_NODE,
    EXTR_BAD_ANGLES_NUMBER,
    EXTR_CANT_GET_TANGENT
    };
  
  Extrusion_Error ExtrusionAlongTrack (TIDSortedElemSet     theElements[2],
                                       SMESH_subMesh*       theTrackPattern,
                                       const SMDS_MeshNode* theNodeStart,
                                       const bool           theHasAngles,
                                       std::list<double>&   theAngles,
                                       const bool           theLinearVariation,
                                       const bool           theHasRefPoint,
                                       const gp_Pnt&        theRefPoint,
                                       const bool           theMakeGroups);
  Extrusion_Error ExtrusionAlongTrack (TIDSortedElemSet     theElements[2],
                                       SMESH_Mesh*          theTrackPattern,
                                       const SMDS_MeshNode* theNodeStart,
                                       const bool           theHasAngles,
                                       std::list<double>&   theAngles,
                                       const bool           theLinearVariation,
                                       const bool           theHasRefPoint,
                                       const gp_Pnt&        theRefPoint,
                                       const bool           theMakeGroups);
  // Generate new elements by extrusion of theElements along path given by theTrackPattern,
  // theHasAngles are the rotation angles, base point can be given by theRefPoint

  PGroupIDs Transform (TIDSortedElemSet & theElements,
                       const gp_Trsf&     theTrsf,
                       const bool         theCopy,
                       const bool         theMakeGroups,
                       SMESH_Mesh*        theTargetMesh=0);
  // Move or copy theElements applying theTrsf to their nodes

  typedef std::list< std::list< const SMDS_MeshNode* > > TListOfListOfNodes;

  void FindCoincidentNodes (TIDSortedNodeSet &   theNodes,
                            const double         theTolerance,
                            TListOfListOfNodes & theGroupsOfNodes,
                            bool                 theSeparateCornersAndMedium);
  // Return list of group of nodes close to each other within theTolerance.
  // Search among theNodes or in the whole mesh if theNodes is empty.

  void MergeNodes (TListOfListOfNodes & theNodeGroups);
  // In each group, the cdr of nodes are substituted by the first one
  // in all elements.

  typedef std::list< std::list< int > > TListOfListOfElementsID;

  void FindEqualElements(TIDSortedElemSet &        theElements,
                         TListOfListOfElementsID & theGroupsOfElementsID);
  // Return list of group of elements build on the same nodes.
  // Search among theElements or in the whole mesh if theElements is empty.

  void MergeElements(TListOfListOfElementsID & theGroupsOfElementsID);
  // In each group remove all but first of elements.

  void MergeEqualElements();
  // Remove all but one of elements built on the same nodes.
  // Return nb of successfully merged groups.

  int SimplifyFace (const std::vector<const SMDS_MeshNode *>& faceNodes,
                    std::vector<const SMDS_MeshNode *>&       poly_nodes,
                    std::vector<int>&                         quantities) const;
  // Split face, defined by <faceNodes>, into several faces by repeating nodes.
  // Is used by MergeNodes()

  static bool CheckFreeBorderNodes(const SMDS_MeshNode* theNode1,
                                   const SMDS_MeshNode* theNode2,
                                   const SMDS_MeshNode* theNode3 = 0);
  // Return true if the three nodes are on a free border

  static bool FindFreeBorder (const SMDS_MeshNode*                  theFirstNode,
                              const SMDS_MeshNode*                  theSecondNode,
                              const SMDS_MeshNode*                  theLastNode,
                              std::list< const SMDS_MeshNode* > &   theNodes,
                              std::list< const SMDS_MeshElement* >& theFaces);
  // Return nodes and faces of a free border if found 

  enum Sew_Error {
    SEW_OK,
    // for SewFreeBorder()
    SEW_BORDER1_NOT_FOUND,
    SEW_BORDER2_NOT_FOUND,
    SEW_BOTH_BORDERS_NOT_FOUND,
    SEW_BAD_SIDE_NODES,
    SEW_VOLUMES_TO_SPLIT,
    // for SewSideElements()
    SEW_DIFF_NB_OF_ELEMENTS,
    SEW_TOPO_DIFF_SETS_OF_ELEMENTS,
    SEW_BAD_SIDE1_NODES,
    SEW_BAD_SIDE2_NODES,
    SEW_INTERNAL_ERROR
    };
    

  Sew_Error SewFreeBorder (const SMDS_MeshNode* theBorderFirstNode,
                           const SMDS_MeshNode* theBorderSecondNode,
                           const SMDS_MeshNode* theBorderLastNode,
                           const SMDS_MeshNode* theSide2FirstNode,
                           const SMDS_MeshNode* theSide2SecondNode,
                           const SMDS_MeshNode* theSide2ThirdNode = 0,
                           const bool           theSide2IsFreeBorder = true,
                           const bool           toCreatePolygons = false,
                           const bool           toCreatePolyedrs = false);
  // Sew the free border to the side2 by replacing nodes in
  // elements on the free border with nodes of the elements
  // of the side 2. If nb of links in the free border and
  // between theSide2FirstNode and theSide2LastNode are different,
  // additional nodes are inserted on a link provided that no
  // volume elements share the split link.
  // The side 2 is a free border if theSide2IsFreeBorder == true.
  // Sewing is performed between the given first, second and last
  // nodes on the sides.
  // theBorderFirstNode is merged with theSide2FirstNode.
  // if (!theSide2IsFreeBorder) then theSide2SecondNode gives
  // the last node on the side 2, which will be merged with
  // theBorderLastNode.
  // if (theSide2IsFreeBorder) then theSide2SecondNode will
  // be merged with theBorderSecondNode.
  // if (theSide2IsFreeBorder && theSide2ThirdNode == 0) then
  // the 2 free borders are sewn link by link and no additional
  // nodes are inserted.
  // Return false, if sewing failed.

  Sew_Error SewSideElements (TIDSortedElemSet&    theSide1,
                             TIDSortedElemSet&    theSide2,
                             const SMDS_MeshNode* theFirstNode1ToMerge,
                             const SMDS_MeshNode* theFirstNode2ToMerge,
                             const SMDS_MeshNode* theSecondNode1ToMerge,
                             const SMDS_MeshNode* theSecondNode2ToMerge);
  // Sew two sides of a mesh. Nodes belonging to theSide1 are
  // merged with nodes of elements of theSide2.
  // Number of elements in theSide1 and in theSide2 must be
  // equal and they should have similar node connectivity.
  // The nodes to merge should belong to side s borders and
  // the first node should be linked to the second.

  void InsertNodesIntoLink(const SMDS_MeshElement*          theFace,
                           const SMDS_MeshNode*             theBetweenNode1,
                           const SMDS_MeshNode*             theBetweenNode2,
                           std::list<const SMDS_MeshNode*>& theNodesToInsert,
                           const bool                       toCreatePoly = false);
  // insert theNodesToInsert into theFace between theBetweenNode1 and theBetweenNode2.
  // If toCreatePoly is true, replace theFace by polygon, else split theFace.

  void UpdateVolumes (const SMDS_MeshNode*             theBetweenNode1,
                      const SMDS_MeshNode*             theBetweenNode2,
                      std::list<const SMDS_MeshNode*>& theNodesToInsert);
  // insert theNodesToInsert into all volumes, containing link
  // theBetweenNode1 - theBetweenNode2, between theBetweenNode1 and theBetweenNode2.

  void ConvertToQuadratic(const bool theForce3d, const bool theToBiQuad);
  void ConvertToQuadratic(const bool theForce3d,
                          TIDSortedElemSet& theElements, const bool theToBiQuad);
  // Converts all mesh to quadratic or bi-quadratic one, deletes old elements, 
  // replacing them with quadratic or bi-quadratic ones with the same id.
  // If theForce3d = 1; this results in the medium node lying at the 
  // middle of the line segments connecting start and end node of a mesh element.
  // If theForce3d = 0; this results in the medium node lying at the 
  // geometrical edge from which the mesh element is built.

  bool ConvertFromQuadratic();
  void ConvertFromQuadratic(TIDSortedElemSet& theElements);
  // Converts all mesh from quadratic to ordinary ones, deletes old quadratic elements, replacing 
  // them with ordinary mesh elements with the same id.
  // Returns true in case of success, false otherwise.

  static void AddToSameGroups (const SMDS_MeshElement* elemToAdd,
                               const SMDS_MeshElement* elemInGroups,
                               SMESHDS_Mesh *          aMesh);
  // Add elemToAdd to the all groups the elemInGroups belongs to

  static void RemoveElemFromGroups (const SMDS_MeshElement* element,
                                    SMESHDS_Mesh *          aMesh);
  // remove element from the all groups

  static void ReplaceElemInGroups (const SMDS_MeshElement* elemToRm,
                                   const SMDS_MeshElement* elemToAdd,
                                   SMESHDS_Mesh *          aMesh);
  // replace elemToRm by elemToAdd in the all groups

  static void ReplaceElemInGroups (const SMDS_MeshElement*                     elemToRm,
                                   const std::vector<const SMDS_MeshElement*>& elemToAdd,
                                   SMESHDS_Mesh *                              aMesh);
  // replace elemToRm by elemToAdd in the all groups

  /*!
   * \brief Return nodes linked to the given one in elements of the type
   */
  static void GetLinkedNodes( const SMDS_MeshNode* node,
                              TIDSortedElemSet &   linkedNodes,
                              SMDSAbs_ElementType  type = SMDSAbs_All );

  /*!
   * \brief Find corresponding nodes in two sets of faces 
    * \param theSide1 - first face set
    * \param theSide2 - second first face
    * \param theFirstNode1 - a boundary node of set 1
    * \param theFirstNode2 - a node of set 2 corresponding to theFirstNode1
    * \param theSecondNode1 - a boundary node of set 1 linked with theFirstNode1
    * \param theSecondNode2 - a node of set 2 corresponding to theSecondNode1
    * \param nReplaceMap - output map of corresponding nodes
    * \return Sew_Error  - is a success or not
   */
  static Sew_Error FindMatchingNodes(std::set<const SMDS_MeshElement*>& theSide1,
                                     std::set<const SMDS_MeshElement*>& theSide2,
                                     const SMDS_MeshNode*               theFirstNode1,
                                     const SMDS_MeshNode*               theFirstNode2,
                                     const SMDS_MeshNode*               theSecondNode1,
                                     const SMDS_MeshNode*               theSecondNode2,
                                     TNodeNodeMap &                     theNodeReplaceMap);

  /*!
   * \brief Returns true if given node is medium
    * \param n - node to check
    * \param typeToCheck - type of elements containing the node to ask about node status
    * \return bool - check result
   */
  static bool IsMedium(const SMDS_MeshNode*      node,
                       const SMDSAbs_ElementType typeToCheck = SMDSAbs_All);

  int FindShape (const SMDS_MeshElement * theElem);
  // Return an index of the shape theElem is on
  // or zero if a shape not found

  void DoubleElements( const TIDSortedElemSet& theElements );

  bool DoubleNodes( const std::list< int >& theListOfNodes, 
                    const std::list< int >& theListOfModifiedElems );
  
  bool DoubleNodes( const TIDSortedElemSet& theElems, 
                    const TIDSortedElemSet& theNodesNot,
                    const TIDSortedElemSet& theAffectedElems );

  bool AffectedElemGroupsInRegion( const TIDSortedElemSet& theElems,
                                   const TIDSortedElemSet& theNodesNot,
                                   const TopoDS_Shape&     theShape,
                                   TIDSortedElemSet& theAffectedElems);

  bool DoubleNodesInRegion( const TIDSortedElemSet& theElems, 
                            const TIDSortedElemSet& theNodesNot,
                            const TopoDS_Shape&     theShape );
  
  double OrientedAngle(const gp_Pnt& p0, const gp_Pnt& p1, const gp_Pnt& g1, const gp_Pnt& g2);

  bool DoubleNodesOnGroupBoundaries( const std::vector<TIDSortedElemSet>& theElems,
                                     bool                                 createJointElems,
                                     bool                                 onAllBoundaries);

  bool CreateFlatElementsOnFacesGroups( const std::vector<TIDSortedElemSet>& theElems );

  void CreateHoleSkin(double radius,
                      const TopoDS_Shape& theShape,
                      SMESH_NodeSearcher* theNodeSearcher,
                      const char* groupName,
                      std::vector<double>&   nodesCoords,
                      std::vector<std::vector<int> >& listOfListOfNodes);

  /*!
   * \brief Generated skin mesh (containing 2D cells) from 3D mesh
   * The created 2D mesh elements based on nodes of free faces of boundary volumes
   * \return TRUE if operation has been completed successfully, FALSE otherwise
   */
  bool Make2DMeshFrom3D();

  enum Bnd_Dimension { BND_2DFROM3D, BND_1DFROM3D, BND_1DFROM2D };

  int MakeBoundaryMesh(const TIDSortedElemSet& elements,
                       Bnd_Dimension           dimension,
                       SMESH_Group*            group = 0,
                       SMESH_Mesh*             targetMesh = 0,
                       bool                    toCopyElements = false,
                       bool                    toCopyExistingBondary = false,
                       bool                    toAddExistingBondary = false,
                       bool                    aroundElements = false);

 private:

  /*!
   * \brief Convert elements contained in a submesh to quadratic
   * \return int - nb of checked elements
   */
  int convertElemToQuadratic(SMESHDS_SubMesh *   theSm,
                             SMESH_MesherHelper& theHelper,
                             const bool          theForce3d);

  /*!
   * \brief Convert quadratic elements to linear ones and remove quadratic nodes
   * \return nb of checked elements
   */
  int removeQuadElem( SMESHDS_SubMesh *    theSm,
                      SMDS_ElemIteratorPtr theItr,
                      const int            theShapeID);
  /*!
   * \brief Create groups of elements made during transformation
   * \param nodeGens - nodes making corresponding myLastCreatedNodes
   * \param elemGens - elements making corresponding myLastCreatedElems
   * \param postfix - to append to names of new groups
   * \param targetMesh - mesh to create groups in
   * \param topPresent - is there "top" elements that are created by sweeping
   */
  PGroupIDs generateGroups(const SMESH_SequenceOfElemPtr& nodeGens,
                           const SMESH_SequenceOfElemPtr& elemGens,
                           const std::string&             postfix,
                           SMESH_Mesh*                    targetMesh=0,
                           const bool                     topPresent=true);
  /*!
   * \brief Create elements by sweeping an element
   * \param elem - element to sweep
   * \param newNodesItVec - nodes generated from each node of the element
   * \param newElems - generated elements
   * \param nbSteps - number of sweeping steps
   * \param srcElements - to append elem for each generated element
   */
  void sweepElement(const SMDS_MeshElement*                    elem,
                    const std::vector<TNodeOfNodeListMapItr> & newNodesItVec,
                    std::list<const SMDS_MeshElement*>&        newElems,
                    const int                                  nbSteps,
                    SMESH_SequenceOfElemPtr&                   srcElements);

  /*!
   * \brief Create 1D and 2D elements around swept elements
   * \param mapNewNodes - source nodes and ones generated from them
   * \param newElemsMap - source elements and ones generated from them
   * \param elemNewNodesMap - nodes generated from each node of each element
   * \param elemSet - all swept elements
   * \param nbSteps - number of sweeping steps
   * \param srcElements - to append elem for each generated element
   */
  void makeWalls (TNodeOfNodeListMap &     mapNewNodes,
                  TTElemOfElemListMap &    newElemsMap,
                  TElemOfVecOfNnlmiMap &   elemNewNodesMap,
                  TIDSortedElemSet&        elemSet,
                  const int                nbSteps,
                  SMESH_SequenceOfElemPtr& srcElements);

  struct SMESH_MeshEditor_PathPoint
  {
    gp_Pnt myPnt;
    gp_Dir myTgt;
    double myAngle, myPrm;

    SMESH_MeshEditor_PathPoint(): myPnt(99., 99., 99.), myTgt(1.,0.,0.), myAngle(0), myPrm(0) {}
    void          SetPnt      (const gp_Pnt& aP3D)  { myPnt  =aP3D; }
    void          SetTangent  (const gp_Dir& aTgt)  { myTgt  =aTgt; }
    void          SetAngle    (const double& aBeta) { myAngle=aBeta; }
    void          SetParameter(const double& aPrm)  { myPrm  =aPrm; }
    const gp_Pnt& Pnt         ()const               { return myPnt; }
    const gp_Dir& Tangent     ()const               { return myTgt; }
    double        Angle       ()const               { return myAngle; }
    double        Parameter   ()const               { return myPrm; }
  };
  Extrusion_Error MakeEdgePathPoints(std::list<double>&                     aPrms,
                                     const TopoDS_Edge&                     aTrackEdge,
                                     bool                                   aFirstIsStart,
                                     std::list<SMESH_MeshEditor_PathPoint>& aLPP);
  Extrusion_Error MakeExtrElements(TIDSortedElemSet                       theElements[2],
                                   std::list<SMESH_MeshEditor_PathPoint>& theFullList,
                                   const bool                             theHasAngles,
                                   std::list<double>&                     theAngles,
                                   const bool                             theLinearVariation,
                                   const bool                             theHasRefPoint,
                                   const gp_Pnt&                          theRefPoint,
                                   const bool                             theMakeGroups);
  void LinearAngleVariation(const int     NbSteps,
                            std::list<double>& theAngles);

  bool doubleNodes( SMESHDS_Mesh*           theMeshDS,
                    const TIDSortedElemSet& theElems,
                    const TIDSortedElemSet& theNodesNot,
                    TNodeNodeMap&           theNodeNodeMap,
                    const bool              theIsDoubleElem );

  void copyPosition( const SMDS_MeshNode* from,
                     const SMDS_MeshNode* to );

private:

  SMESH_Mesh *            myMesh;

  // Nodes and elements created during last operation
  SMESH_SequenceOfElemPtr myLastCreatedNodes, myLastCreatedElems;

  // Description of error/warning occurred during last operation
  SMESH_ComputeErrorPtr   myError;
};

#endif
