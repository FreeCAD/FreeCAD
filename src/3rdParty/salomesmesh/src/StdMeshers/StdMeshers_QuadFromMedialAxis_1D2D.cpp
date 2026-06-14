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
// File      : StdMeshers_QuadFromMedialAxis_1D2D.cxx
// Created   : Wed Jun  3 17:33:45 2015
// Author    : Edward AGAPOV (eap)

#include "StdMeshers_QuadFromMedialAxis_1D2D.hxx"

#include "SMESH_Block.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_MAT2d.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MeshEditor.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_ProxyMesh.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_LayerDistribution.hxx"
#include "StdMeshers_NumberOfLayers.hxx"
#include "StdMeshers_Regular_1D.hxx"
#include "StdMeshers_ViscousLayers2D.hxx"

#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <Geom_Surface.hxx>
#include <Precision.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <list>
#include <vector>

//================================================================================
/*!
 * \brief 1D algo
 */
class StdMeshers_QuadFromMedialAxis_1D2D::Algo1D : public StdMeshers_Regular_1D
{
public:
  Algo1D(int studyId, SMESH_Gen* gen):
    StdMeshers_Regular_1D( gen->GetANewId(), studyId, gen )
  {
  }
  void SetSegmentLength( double len )
  {
    SMESH_Algo::_usedHypList.clear();
    _value[ BEG_LENGTH_IND ] = len;
    _value[ PRECISION_IND  ] = 1e-7;
    _hypType = LOCAL_LENGTH;
  }
  void SetRadialDistribution( const SMESHDS_Hypothesis* hyp )
  {
    SMESH_Algo::_usedHypList.clear();
    if ( !hyp )
      return;

    if ( const StdMeshers_NumberOfLayers* nl =
         dynamic_cast< const StdMeshers_NumberOfLayers* >( hyp ))
    {
      _ivalue[ NB_SEGMENTS_IND  ] = nl->GetNumberOfLayers();
      _ivalue[ DISTR_TYPE_IND ]   = 0;
      _hypType = NB_SEGMENTS;
    }
    if ( const StdMeshers_LayerDistribution* ld =
         dynamic_cast< const StdMeshers_LayerDistribution* >( hyp ))
    {
      if ( SMESH_Hypothesis* h = ld->GetLayerDistribution() )
      {
        SMESH_Algo::_usedHypList.clear();
        SMESH_Algo::_usedHypList.push_back( h );
      }
    }
  }
  void ComputeDistribution(SMESH_MesherHelper& theHelper,
                           const gp_Pnt&       thePnt1,
                           const gp_Pnt&       thePnt2,
                           list< double >&     theParams)
  {
    SMESH_Mesh& mesh = *theHelper.GetMesh();
    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge( thePnt1, thePnt2 );

    SMESH_Hypothesis::Hypothesis_Status aStatus;
    CheckHypothesis( mesh, edge, aStatus );

    theParams.clear();
    BRepAdaptor_Curve C3D(edge);
    double f = C3D.FirstParameter(), l = C3D.LastParameter(), len = thePnt1.Distance( thePnt2 );
    if ( !StdMeshers_Regular_1D::computeInternalParameters( mesh, C3D, len, f, l, theParams, false))
    {
      for ( size_t i = 1; i < 15; ++i )
        theParams.push_back( i/15 );
    }
    else
    {
      for (list<double>::iterator itU = theParams.begin(); itU != theParams.end(); ++itU )
        *itU /= len;
    }
  }
  virtual const list <const SMESHDS_Hypothesis *> &
  GetUsedHypothesis(SMESH_Mesh &, const TopoDS_Shape &, const bool)
  {
    return SMESH_Algo::_usedHypList;
  }
  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus)
  {
    if ( !SMESH_Algo::_usedHypList.empty() )
      return StdMeshers_Regular_1D::CheckHypothesis( aMesh, aShape, aStatus );
    return true;
  }
};
 
//================================================================================
/*!
 * \brief Constructor sets algo features
 */
//================================================================================

StdMeshers_QuadFromMedialAxis_1D2D::StdMeshers_QuadFromMedialAxis_1D2D(int        hypId,
                                                                       int        studyId,
                                                                       SMESH_Gen* gen)
  : StdMeshers_Quadrangle_2D(hypId, studyId, gen),
    _regular1D( 0 )
{
  _name = "QuadFromMedialAxis_1D2D";
  _shapeType = (1 << TopAbs_FACE);
  _onlyUnaryInput          = true;  // FACE by FACE so far
  _requireDiscreteBoundary = false; // make 1D by myself
  _supportSubmeshes        = true; // make 1D by myself
  _neededLowerHyps[ 1 ]    = true;  // suppress warning on hiding a global 1D algo
  _neededLowerHyps[ 2 ]    = true;  // suppress warning on hiding a global 2D algo
  _compatibleHypothesis.clear();
  _compatibleHypothesis.push_back("ViscousLayers2D");
  _compatibleHypothesis.push_back("LayerDistribution2D");
  _compatibleHypothesis.push_back("NumberOfLayers2D");
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_QuadFromMedialAxis_1D2D::~StdMeshers_QuadFromMedialAxis_1D2D()
{
  delete _regular1D;
  _regular1D = 0;
}

//================================================================================
/*!
 * \brief Check if needed hypotheses are present
 */
//================================================================================

bool StdMeshers_QuadFromMedialAxis_1D2D::CheckHypothesis(SMESH_Mesh&         aMesh,
                                                         const TopoDS_Shape& aShape,
                                                         Hypothesis_Status&  aStatus)
{
  aStatus = HYP_OK;

  // get one main optional hypothesis
  const list <const SMESHDS_Hypothesis * >& hyps = GetUsedHypothesis(aMesh, aShape);
  _hyp2D  = hyps.empty() ? 0 : hyps.front();

  return true; // does not require hypothesis
}

namespace
{
  typedef map< const SMDS_MeshNode*, list< const SMDS_MeshNode* > > TMergeMap;
  
  //================================================================================
  /*!
   * \brief Sinuous face
   */
  struct SinuousFace
  {
    FaceQuadStruct::Ptr          _quad;
    vector< TopoDS_Edge >        _edges;
    vector< TopoDS_Edge >        _sinuSide[2], _shortSide[2];
    vector< TopoDS_Edge >        _sinuEdges;
    vector< Handle(Geom_Curve) > _sinuCurves;
    int                          _nbWires;
    list< int >                  _nbEdgesInWire;
    TMergeMap                    _nodesToMerge;

    SinuousFace( const TopoDS_Face& f ): _quad( new FaceQuadStruct )
    {
      list< TopoDS_Edge > edges;
      _nbWires = SMESH_Block::GetOrderedEdges (f, edges, _nbEdgesInWire);
      _edges.assign( edges.begin(), edges.end() );

      _quad->side.resize( 4 );
      _quad->face = f;
    }
    const TopoDS_Face& Face() const { return _quad->face; }
    bool IsRing() const { return _shortSide[0].empty() && !_sinuSide[0].empty(); }
  };

  //================================================================================
  /*!
   * \brief Temporary mesh
   */
  struct TmpMesh : public SMESH_Mesh
  {
    TmpMesh()
    {
      _myMeshDS = new SMESHDS_Mesh(/*id=*/0, /*isEmbeddedMode=*/true);
    }
  };

  //================================================================================
  /*!
   * \brief Event listener which removes mesh from EDGEs when 2D hyps change
   */
  struct EdgeCleaner : public SMESH_subMeshEventListener
  {
    int _prevAlgoEvent;
    EdgeCleaner():
      SMESH_subMeshEventListener( /*isDeletable=*/true,
                                  "StdMeshers_QuadFromMedialAxis_1D2D::EdgeCleaner")
    {
      _prevAlgoEvent = -1;
    }
    virtual void ProcessEvent(const int                       event,
                              const int                       eventType,
                              SMESH_subMesh*                  faceSubMesh,
                              SMESH_subMeshEventListenerData* data,
                              const SMESH_Hypothesis*         hyp)
    {
      if ( eventType == SMESH_subMesh::ALGO_EVENT )
      {
        _prevAlgoEvent = event;
        return;
      }
      // SMESH_subMesh::COMPUTE_EVENT
      if ( _prevAlgoEvent == SMESH_subMesh::REMOVE_HYP ||
           _prevAlgoEvent == SMESH_subMesh::REMOVE_ALGO ||
           _prevAlgoEvent == SMESH_subMesh::MODIF_HYP )
      {
        SMESH_subMeshIteratorPtr smIt = faceSubMesh->getDependsOnIterator(/*includeSelf=*/false);
        while ( smIt->more() )
          smIt->next()->ComputeStateEngine( SMESH_subMesh::CLEAN );
      }
      _prevAlgoEvent = -1;
    }
  };

  //================================================================================
  /*!
   * \brief Return a member of a std::pair
   */
  //================================================================================

  template< typename T >
  T& get( std::pair< T, T >& thePair, bool is2nd )
  {
    return is2nd ? thePair.second : thePair.first;
  }

  //================================================================================
  /*!
   * \brief Select two EDGEs from a map, either mapped to least values or to max values
   */
  //================================================================================

  // template< class TVal2EdgesMap >
  // void getTwo( bool                 least,
  //              TVal2EdgesMap&       map,
  //              vector<TopoDS_Edge>& twoEdges,
  //              vector<TopoDS_Edge>& otherEdges)
  // {
  //   twoEdges.clear();
  //   otherEdges.clear();
  //   if ( least )
  //   {
  //     TVal2EdgesMap::iterator i = map.begin();
  //     twoEdges.push_back( i->second );
  //     twoEdges.push_back( ++i->second );
  //     for ( ; i != map.end(); ++i )
  //       otherEdges.push_back( i->second );
  //   }
  //   else
  //   {
  //     TVal2EdgesMap::reverse_iterator i = map.rbegin();
  //     twoEdges.push_back( i->second );
  //     twoEdges.push_back( ++i->second );
  //     for ( ; i != map.rend(); ++i )
  //       otherEdges.push_back( i->second );
  //   }
  //   TopoDS_Vertex v;
  //   if ( TopExp::CommonVertex( twoEdges[0], twoEdges[1], v ))
  //   {
  //     twoEdges.clear(); // two EDGEs must not be connected
  //     otherEdges.clear();
  //   }
  // }

  //================================================================================
  /*!
   * \brief Finds out a minimal segment length given EDGEs will be divided into.
   *        This length is further used to discretize the Medial Axis
   */
  //================================================================================

  double getMinSegLen(SMESH_MesherHelper&        theHelper,
                      const vector<TopoDS_Edge>& theEdges)
  {
    TmpMesh tmpMesh;
    SMESH_Mesh* mesh = theHelper.GetMesh();

    vector< SMESH_Algo* > algos( theEdges.size() );
    for ( size_t i = 0; i < theEdges.size(); ++i )
    {
      SMESH_subMesh* sm = mesh->GetSubMesh( theEdges[i] );
      algos[i] = sm->GetAlgo();
    }

    int nbSegDflt = mesh->GetGen() ? mesh->GetGen()->GetDefaultNbSegments() : 15;
    double minSegLen = Precision::Infinite();

    for ( size_t i = 0; i < theEdges.size(); ++i )
    {
      SMESH_subMesh* sm = mesh->GetSubMesh( theEdges[i] );
      if ( SMESH_Algo::IsStraight( theEdges[i], /*degenResult=*/true ))
        continue;
      // get algo
      size_t iOpp = ( theEdges.size() == 4 ? (i+2)%4 : i );
      SMESH_Algo*  algo = sm->GetAlgo();
      if ( !algo ) algo = algos[ iOpp ];
      // get hypo
      SMESH_Hypothesis::Hypothesis_Status status = SMESH_Hypothesis::HYP_MISSING;
      if ( algo )
      {
        if ( !algo->CheckHypothesis( *mesh, theEdges[i], status ))
          algo->CheckHypothesis( *mesh, theEdges[iOpp], status );
      }
      // compute
      if ( status != SMESH_Hypothesis::HYP_OK )
      {
        minSegLen = Min( minSegLen, SMESH_Algo::EdgeLength( theEdges[i] ) / nbSegDflt );
      }
      else
      {
        tmpMesh.Clear();
        tmpMesh.ShapeToMesh( TopoDS_Shape());
        tmpMesh.ShapeToMesh( theEdges[i] );
        try {
          if ( !mesh->GetGen() ) continue; // tmp mesh
          mesh->GetGen()->Compute( tmpMesh, theEdges[i], true, true ); // make nodes on VERTEXes
          if ( !algo->Compute( tmpMesh, theEdges[i] ))
            continue;
        }
        catch (...) {
          continue;
        }
        SMDS_EdgeIteratorPtr segIt = tmpMesh.GetMeshDS()->edgesIterator();
        while ( segIt->more() )
        {
          const SMDS_MeshElement* seg = segIt->next();
          double len = SMESH_TNodeXYZ( seg->GetNode(0) ).Distance( seg->GetNode(1) );
          minSegLen = Min( minSegLen, len );
        }
      }
    }
    if ( Precision::IsInfinite( minSegLen ))
      minSegLen = mesh->GetShapeDiagonalSize() / nbSegDflt;

    return minSegLen;
  }

  //================================================================================
  /*!
   * \brief Returns EDGEs located between two VERTEXes at which given MA branches end
   *  \param [in] br1 - one MA branch
   *  \param [in] br2 - one more MA branch
   *  \param [in] allEdges - all EDGEs of a FACE
   *  \param [out] shortEdges - the found EDGEs
   *  \return bool - is OK or not
   */
  //================================================================================

  bool getConnectedEdges( const SMESH_MAT2d::Branch* br1,
                          const SMESH_MAT2d::Branch* br2,
                          const vector<TopoDS_Edge>& allEdges,
                          vector<TopoDS_Edge>&       shortEdges)
  {
    vector< size_t > edgeIDs[4];
    br1->getGeomEdges( edgeIDs[0], edgeIDs[1] );
    br2->getGeomEdges( edgeIDs[2], edgeIDs[3] );

    // EDGEs returned by a Branch form a connected chain with a VERTEX where
    // the Branch ends at the chain middle. One of end EDGEs of the chain is common
    // with either end EDGE of the chain of the other Branch, or the chains are connected
    // at a common VERTEX;

    // Get indices of end EDGEs of the branches
    bool vAtStart1 = ( br1->getEnd(0)->_type == SMESH_MAT2d::BE_ON_VERTEX );
    bool vAtStart2 = ( br2->getEnd(0)->_type == SMESH_MAT2d::BE_ON_VERTEX );
    size_t iEnd[4] = {
      vAtStart1 ? edgeIDs[0].back() : edgeIDs[0][0],
      vAtStart1 ? edgeIDs[1].back() : edgeIDs[1][0],
      vAtStart2 ? edgeIDs[2].back() : edgeIDs[2][0],
      vAtStart2 ? edgeIDs[3].back() : edgeIDs[3][0]
    };

    set< size_t > connectedIDs;
    TopoDS_Vertex vCommon;
    // look for the same EDGEs
    for ( int i = 0; i < 2; ++i )
      for ( int j = 2; j < 4; ++j )
        if ( iEnd[i] == iEnd[j] )
        {
          connectedIDs.insert( edgeIDs[i].begin(), edgeIDs[i].end() );
          connectedIDs.insert( edgeIDs[j].begin(), edgeIDs[j].end() );
          i = j = 4;
        }
    if ( connectedIDs.empty() )
      // look for connected EDGEs
      for ( int i = 0; i < 2; ++i )
        for ( int j = 2; j < 4; ++j )
          if ( TopExp::CommonVertex( allEdges[ iEnd[i]], allEdges[ iEnd[j]], vCommon ))
          {
            connectedIDs.insert( edgeIDs[i].begin(), edgeIDs[i].end() );
            connectedIDs.insert( edgeIDs[j].begin(), edgeIDs[j].end() );
            i = j = 4;
          }
    if ( connectedIDs.empty() ||                     // nothing
         allEdges.size() - connectedIDs.size() < 2 ) // too many
      return false;

    // set shortEdges in the order as in allEdges
    if ( connectedIDs.count( 0 ) &&
         connectedIDs.count( allEdges.size()-1 ))
    {
      size_t iE = allEdges.size()-1;
      while ( connectedIDs.count( iE-1 ))
        --iE;
      for ( size_t i = 0; i < connectedIDs.size(); ++i )
      {
        shortEdges.push_back( allEdges[ iE ]);
        iE = ( iE + 1 ) % allEdges.size();
      }
    }
    else
    {
      set< size_t >::iterator i = connectedIDs.begin();
      for ( ; i != connectedIDs.end(); ++i )
        shortEdges.push_back( allEdges[ *i ]);
    }
    return true;
  }

  //================================================================================
  /*!
   * \brief Find EDGEs to discretize using projection from MA
   *  \param [in,out] theSinuFace - the FACE to be meshed
   *  \return bool - OK or not
   *
   * It separates all EDGEs into four sides of a quadrangle connected in the order:
   * theSinuEdges[0], theShortEdges[0], theSinuEdges[1], theShortEdges[1]
   */
  //================================================================================

  bool getSinuousEdges( SMESH_MesherHelper& theHelper,
                        SinuousFace&        theSinuFace)
  {
    vector<TopoDS_Edge> * theSinuEdges  = & theSinuFace._sinuSide [0];
    vector<TopoDS_Edge> * theShortEdges = & theSinuFace._shortSide[0];
    theSinuEdges[0].clear();
    theSinuEdges[1].clear();
    theShortEdges[0].clear();
    theShortEdges[1].clear();
   
    vector<TopoDS_Edge> & allEdges = theSinuFace._edges;
    const size_t nbEdges = allEdges.size();
    if ( nbEdges < 4 && theSinuFace._nbWires == 1 )
      return false;

    if ( theSinuFace._nbWires == 2 ) // ring
    {
      size_t nbOutEdges = theSinuFace._nbEdgesInWire.front();
      theSinuEdges[0].assign ( allEdges.begin(), allEdges.begin() + nbOutEdges );
      theSinuEdges[1].assign ( allEdges.begin() + nbOutEdges, allEdges.end() );
      theSinuFace._sinuEdges = allEdges;
      return true;
    }
    if ( theSinuFace._nbWires > 2 )
      return false;

    // create MedialAxis to find short edges by analyzing MA branches
    double minSegLen = getMinSegLen( theHelper, allEdges );
    SMESH_MAT2d::MedialAxis ma( theSinuFace.Face(), allEdges, minSegLen * 3 );

    // in an initial request case, theFace represents a part of a river with almost parallel banks
    // so there should be two branch points
    using SMESH_MAT2d::BranchEnd;
    using SMESH_MAT2d::Branch;
    const vector< const BranchEnd* >& braPoints = ma.getBranchPoints();
    if ( braPoints.size() < 2 )
      return false;
    TopTools_MapOfShape shortMap;
    size_t nbBranchPoints = 0;
    for ( size_t i = 0; i < braPoints.size(); ++i )
    {
      vector< const Branch* > vertBranches; // branches with an end on VERTEX
      for ( size_t ib = 0; ib < braPoints[i]->_branches.size(); ++ib )
      {
        const Branch* branch = braPoints[i]->_branches[ ib ];
        if ( branch->hasEndOfType( SMESH_MAT2d::BE_ON_VERTEX ))
          vertBranches.push_back( branch );
      }
      if ( vertBranches.size() != 2 || braPoints[i]->_branches.size() != 3)
        continue;

      // get common EDGEs of two branches
      if ( !getConnectedEdges( vertBranches[0], vertBranches[1],
                               allEdges, theShortEdges[ nbBranchPoints > 0 ] ))
        return false;

      for ( size_t iS = 0; iS < theShortEdges[ nbBranchPoints ].size(); ++iS )
        shortMap.Add( theShortEdges[ nbBranchPoints ][ iS ]);

      ++nbBranchPoints;
    }

    if ( nbBranchPoints != 2 )
      return false;

    // add to theSinuEdges all edges that are not theShortEdges
    vector< vector<TopoDS_Edge> > sinuEdges(1);
    TopoDS_Vertex vCommon;
    for ( size_t i = 0; i < allEdges.size(); ++i )
    {
      if ( !shortMap.Contains( allEdges[i] ))
      {
        if ( !sinuEdges.back().empty() )
          if ( !TopExp::CommonVertex( sinuEdges.back().back(), allEdges[ i ], vCommon ))
            sinuEdges.resize( sinuEdges.size() + 1 );

        sinuEdges.back().push_back( allEdges[i] );
      }
    }
    if ( sinuEdges.size() == 3 )
    {
      if ( !TopExp::CommonVertex( sinuEdges.back().back(), sinuEdges[0][0], vCommon ))
        return false;
      vector<TopoDS_Edge>& last = sinuEdges.back();
      last.insert( last.end(), sinuEdges[0].begin(), sinuEdges[0].end() );
      sinuEdges[0].swap( last );
      sinuEdges.resize( 2 );
    }
    if ( sinuEdges.size() != 2 )
      return false;

    theSinuEdges[0].swap( sinuEdges[0] );
    theSinuEdges[1].swap( sinuEdges[1] );

    if ( !TopExp::CommonVertex( theSinuEdges[0].back(), theShortEdges[0][0], vCommon ) ||
         !vCommon.IsSame( theHelper.IthVertex( 1, theSinuEdges[0].back() )))
      theShortEdges[0].swap( theShortEdges[1] );

    theSinuFace._sinuEdges = theSinuEdges[0];
    theSinuFace._sinuEdges.insert( theSinuFace._sinuEdges.end(),
                                   theSinuEdges[1].begin(), theSinuEdges[1].end() );

    return ( theShortEdges[0].size() > 0 && theShortEdges[1].size() > 0 &&
             theSinuEdges [0].size() > 0 && theSinuEdges [1].size() > 0 );

    // the sinuous EDGEs can be composite and C0 continuous,
    // therefore we use a complex criterion to find TWO short non-sinuous EDGEs
    // and the rest EDGEs will be treated as sinuous.
    // A short edge should have the following features:
    // a) straight
    // b) short
    // c) with convex corners at ends
    // d) far from the other short EDGE

    // vector< double > isStraightEdge( nbEdges, 0 ); // criterion value

    // // a0) evaluate continuity
    // const double contiWgt = 0.5; // weight of continuity in the criterion
    // multimap< int, TopoDS_Edge > continuity;
    // for ( size_t i = 0; i < nbEdges; ++I )
    // {
    //   BRepAdaptor_Curve curve( allEdges[i] );
    //   GeomAbs_Shape C = GeomAbs_CN;
    //   try:
    //     C = curve.Continuity(); // C0, G1, C1, G2, C2, C3, CN
    //   catch ( Standard_Failure ) {}
    //   continuity.insert( make_pair( C, allEdges[i] ));
    //   isStraight[i] += double( C ) / double( CN ) * contiWgt;
    // }

    // // try to choose by continuity
    // int mostStraight = (int) continuity.rbegin()->first;
    // int lessStraight = (int) continuity.begin()->first;
    // if ( mostStraight != lessStraight )
    // {
    //   int nbStraight = continuity.count( mostStraight );
    //   if ( nbStraight == 2 )
    //   {
    //     getTwo( /*least=*/false, continuity, theShortEdges, theSinuEdges );
    //   }
    //   else if ( nbStraight == 3 && nbEdges == 4 )
    //   {
    //     theSinuEdges.push_back( continuity.begin()->second );
    //     vector<TopoDS_Edge>::iterator it =
    //       std::find( allEdges.begin(), allEdges.end(), theSinuEdges[0] );
    //     int i = std::distance( allEdges.begin(), it );
    //     theSinuEdges .push_back( allEdges[( i+2 )%4 ]);
    //     theShortEdges.push_back( allEdges[( i+1 )%4 ]);
    //     theShortEdges.push_back( allEdges[( i+3 )%4 ]);
    //   }
    //   if ( theShortEdges.size() == 2 )
    //     return true;
    // }

    // // a) curvature; evaluate aspect ratio
    // {
    //   const double curvWgt = 0.5;
    //   for ( size_t i = 0; i < nbEdges; ++I )
    //   {
    //     BRepAdaptor_Curve curve( allEdges[i] );
    //     double curvature = 1;
    //     if ( !curve.IsClosed() )
    //     {
    //       const double f = curve.FirstParameter(), l = curve.LastParameter();
    //       gp_Pnt pf = curve.Value( f ), pl = curve.Value( l );
    //       gp_Lin line( pf, pl.XYZ() - pf.XYZ() );
    //       double distMax = 0;
    //       for ( double u = f; u < l; u += (l-f)/30. )
    //         distMax = Max( distMax, line.SquareDistance( curve.Value( u )));
    //       curvature = Sqrt( distMax ) / ( pf.Distance( pl ));
    //     }
    //     isStraight[i] += curvWgt / (              curvature + 1e-20 );
    //   }
    // }
    // // b) length
    // {
    //   const double lenWgt = 0.5;
    //   for ( size_t i = 0; i < nbEdges; ++I )
    //   {
    //     double length = SMESH_Algo::Length( allEdges[i] );
    //     if ( length > 0 )
    //       isStraight[i] += lenWgt / length;
    //   }
    // }
    // // c) with convex corners at ends
    // {
    //   const double cornerWgt = 0.25;
    //   for ( size_t i = 0; i < nbEdges; ++I )
    //   {
    //     double convex = 0;
    //     int iPrev = SMESH_MesherHelper::WrapIndex( int(i)-1, nbEdges );
    //     int iNext = SMESH_MesherHelper::WrapIndex( int(i)+1, nbEdges );
    //     TopoDS_Vertex v = helper.IthVertex( 0, allEdges[i] );
    //     double angle = SMESH_MesherHelper::GetAngle( allEdges[iPrev], allEdges[i], theFace, v );
    //     if ( angle < M_PI ) // [-PI; PI]
    //       convex += ( angle + M_PI ) / M_PI / M_PI;
    //     v = helper.IthVertex( 1, allEdges[i] );
    //     angle = SMESH_MesherHelper::GetAngle( allEdges[iNext], allEdges[i], theFace, v );
    //     if ( angle < M_PI ) // [-PI; PI]
    //       convex += ( angle + M_PI ) / M_PI / M_PI;
    //     isStraight[i] += cornerWgt * convex;
    //   }
    // }
  }

  //================================================================================
  /*!
   * \brief Creates an EDGE from a sole branch of MA
   */
  //================================================================================

  TopoDS_Edge makeEdgeFromMA( SMESH_MesherHelper&            theHelper,
                              const SMESH_MAT2d::MedialAxis& theMA,
                              const double                   theMinSegLen)
  {
    if ( theMA.nbBranches() != 1 )
      return TopoDS_Edge();

    vector< gp_XY > uv;
    theMA.getPoints( theMA.getBranch(0), uv );
    if ( uv.size() < 2 )
      return TopoDS_Edge();

    TopoDS_Face face = TopoDS::Face( theHelper.GetSubShape() );
    Handle(Geom_Surface) surface = BRep_Tool::Surface( face );

    vector< gp_Pnt > pnt;
    pnt.reserve( uv.size() * 2 );
    pnt.push_back( surface->Value( uv[0].X(), uv[0].Y() ));
    for ( size_t i = 1; i < uv.size(); ++i )
    {
      gp_Pnt p = surface->Value( uv[i].X(), uv[i].Y() );
      int nbDiv = int( p.Distance( pnt.back() ) / theMinSegLen );
      for ( int iD = 1; iD < nbDiv; ++iD )
      {
        double  R = iD / double( nbDiv );
        gp_XY uvR = uv[i-1] * (1 - R) + uv[i] * R;
        pnt.push_back( surface->Value( uvR.X(), uvR.Y() ));
      }
      pnt.push_back( p );
    }

    // cout << "from salome.geom import geomBuilder" << endl;
    // cout << "geompy = geomBuilder.New(salome.myStudy)" << endl;
    Handle(TColgp_HArray1OfPnt) points = new TColgp_HArray1OfPnt(1, pnt.size());
    for ( size_t i = 0; i < pnt.size(); ++i )
    {
      gp_Pnt& p = pnt[i];
      points->SetValue( i+1, p );
      // cout << "geompy.MakeVertex( "<< p.X()<<", " << p.Y()<<", " << p.Z()
      //      <<" theName = 'p_" << i << "')" << endl;
    }

    GeomAPI_Interpolate interpol( points, /*isClosed=*/false, gp::Resolution());
    interpol.Perform();
    if ( !interpol.IsDone())
      return TopoDS_Edge();

    TopoDS_Edge branchEdge = BRepBuilderAPI_MakeEdge(interpol.Curve());
    return branchEdge;
  }

  //================================================================================
  /*!
   * \brief Returns a type of shape, to which a hypothesis used to mesh a given edge is assigned
   */
  //================================================================================

  TopAbs_ShapeEnum getHypShape( SMESH_Mesh* mesh, const TopoDS_Shape& edge )
  {
    TopAbs_ShapeEnum shapeType = TopAbs_SHAPE;

    SMESH_subMesh* sm = mesh->GetSubMesh( edge );
    SMESH_Algo*  algo = sm->GetAlgo();
    if ( !algo ) return shapeType;

    const list <const SMESHDS_Hypothesis *> & hyps =
      algo->GetUsedHypothesis( *mesh, edge, /*ignoreAuxiliary=*/true );
    if ( hyps.empty() ) return shapeType;

    TopoDS_Shape shapeOfHyp =
      SMESH_MesherHelper::GetShapeOfHypothesis( hyps.front(), edge, mesh);

    return SMESH_MesherHelper::GetGroupType( shapeOfHyp, /*woCompound=*/true);
  }

  //================================================================================
  /*!
   * \brief Discretize a sole branch of MA an returns parameters of divisions on MA
   */
  //================================================================================

  bool divideMA( SMESH_MesherHelper&            theHelper,
                 const SMESH_MAT2d::MedialAxis& theMA,
                 const SinuousFace&             theSinuFace,
                 SMESH_Algo*                    the1dAlgo,
                 const double                   theMinSegLen,
                 vector<double>&                theMAParams )
  {
    // Check if all EDGEs of one size are meshed, then MA discretization is not needed
    SMESH_Mesh* mesh = theHelper.GetMesh();
    size_t nbComputedEdges[2] = { 0, 0 };
    for ( size_t iS = 0; iS < 2; ++iS )
      for ( size_t i = 0; i < theSinuFace._sinuSide[iS].size(); ++i )
      {
        const TopoDS_Edge& sinuEdge = theSinuFace._sinuSide[iS][i];
        SMESH_subMesh*           sm = mesh->GetSubMesh( sinuEdge );
        bool             isComputed = ( !sm->IsEmpty() );
        if ( isComputed )
        {
          TopAbs_ShapeEnum shape = getHypShape( mesh, sinuEdge );
          if ( shape == TopAbs_SHAPE || shape <= TopAbs_FACE )
          {
            // EDGE computed using global hypothesis -> clear it
            bool hasComputedFace = false;
            PShapeIteratorPtr faceIt = theHelper.GetAncestors( sinuEdge, *mesh, TopAbs_FACE );
            while ( const TopoDS_Shape* face = faceIt->next() )
              if (( !face->IsSame( theSinuFace.Face() )) &&
                  ( hasComputedFace = !mesh->GetSubMesh( *face )->IsEmpty() ))
                break;
            if ( !hasComputedFace )
            {
              sm->ComputeStateEngine( SMESH_subMesh::CLEAN );
              isComputed = false;
            }
          }
        }
        nbComputedEdges[ iS ] += isComputed;
      }
    if ( nbComputedEdges[0] == theSinuFace._sinuSide[0].size() ||
         nbComputedEdges[1] == theSinuFace._sinuSide[1].size() )
      return true; // discretization is not needed

    // Make MA EDGE
    TopoDS_Edge branchEdge = makeEdgeFromMA( theHelper, theMA, theMinSegLen );
    if ( branchEdge.IsNull() )
      return false;

    // const char* file = "/misc/dn25/salome/eap/salome/misc/tmp/MAedge.brep";
    // BRepTools::Write( branchEdge, file);
    // cout << "Write " << file << endl;


    // Find 1D algo to mesh branchEdge
  
    // look for a most local 1D hyp assigned to the FACE
    int mostSimpleShape = -1, maxShape = TopAbs_EDGE;
    TopoDS_Edge edge;
    for ( size_t i = 0; i < theSinuFace._sinuEdges.size(); ++i )
    {
      TopAbs_ShapeEnum shapeType = getHypShape( mesh, theSinuFace._sinuEdges[i] );
      if ( mostSimpleShape < shapeType && shapeType < maxShape )
      {
        edge = theSinuFace._sinuEdges[i];
        mostSimpleShape = shapeType;
      }
    }

    SMESH_Algo* algo = the1dAlgo;
    if ( mostSimpleShape > -1 )
    {
      algo = mesh->GetSubMesh( edge )->GetAlgo();
      SMESH_Hypothesis::Hypothesis_Status status;
      if ( !algo->CheckHypothesis( *mesh, edge, status ))
        algo = the1dAlgo;
    }

    TmpMesh tmpMesh;
    tmpMesh.ShapeToMesh( branchEdge );
    try {
      mesh->GetGen()->Compute( tmpMesh, branchEdge, true, true ); // make nodes on VERTEXes
      if ( !algo->Compute( tmpMesh, branchEdge ))
        return false;
    }
    catch (...) {
      return false;
    }
    return SMESH_Algo::GetNodeParamOnEdge( tmpMesh.GetMeshDS(), branchEdge, theMAParams );
  }

  //================================================================================
  /*!
   * \brief Select division parameters on MA and make them coincide at ends with
   *        projections of VERTEXes to MA for a given pair of opposite EDGEs
   *  \param [in] theEdgePairInd - index of the EDGE pair
   *  \param [in] theDivPoints - the BranchPoint's dividing MA into parts each
   *         corresponding to a unique pair of opposite EDGEs
   *  \param [in] theMAParams - the MA division parameters
   *  \param [out] theSelectedMAParams - the selected MA parameters
   *  \return bool - is OK
   */
  //================================================================================

  bool getParamsForEdgePair( const size_t                              theEdgePairInd,
                             const vector< SMESH_MAT2d::BranchPoint >& theDivPoints,
                             const vector<double>&                     theMAParams,
                             vector<double>&                           theSelectedMAParams)
  {
    if ( theDivPoints.empty() )
    {
      theSelectedMAParams = theMAParams;
      return true;
    }
    if ( theEdgePairInd > theDivPoints.size() || theMAParams.empty() )
      return false;

    // find a range of params to copy

    double par1 = 0;
    size_t iPar1 = 0;
    if ( theEdgePairInd > 0 )
    {
      const SMESH_MAT2d::BranchPoint& bp = theDivPoints[ theEdgePairInd-1 ];
      bp._branch->getParameter( bp, par1 );
      while ( theMAParams[ iPar1 ] < par1 ) ++iPar1;
      if ( par1 - theMAParams[ iPar1-1 ] < theMAParams[ iPar1 ] - par1 )
        --iPar1;
    }

    double par2 = 1;
    size_t iPar2 = theMAParams.size() - 1;
    if ( theEdgePairInd < theDivPoints.size() )
    {
      const SMESH_MAT2d::BranchPoint& bp = theDivPoints[ theEdgePairInd ];
      bp._branch->getParameter( bp, par2 );
      iPar2 = iPar1;
      while ( theMAParams[ iPar2 ] < par2 ) ++iPar2;
      if ( par2 - theMAParams[ iPar2-1 ] < theMAParams[ iPar2 ] - par2 )
        --iPar2;
    }

    theSelectedMAParams.assign( theMAParams.begin() + iPar1,
                                theMAParams.begin() + iPar2 + 1 );

    // adjust theSelectedMAParams to fit between par1 and par2

    double d = par1 - theSelectedMAParams[0];
    double f = ( par2 - par1 ) / ( theSelectedMAParams.back() - theSelectedMAParams[0] );

    for ( size_t i = 0; i < theSelectedMAParams.size(); ++i )
    {
      theSelectedMAParams[i] += d;
      theSelectedMAParams[i] = par1 + ( theSelectedMAParams[i] - par1 ) * f;
    }

    return true;
  }

  //--------------------------------------------------------------------------------
  // node or node parameter on EDGE
  struct NodePoint
  {
    const SMDS_MeshNode* _node;
    double               _u;
    int                  _edgeInd; // index in theSinuEdges vector

    NodePoint(): _node(0), _u(0), _edgeInd(-1) {}
    NodePoint(const SMDS_MeshNode* n, double u, size_t iEdge ): _node(n), _u(u), _edgeInd(iEdge) {}
    NodePoint(double u, size_t iEdge) : _node(0), _u(u), _edgeInd(iEdge) {}
    NodePoint(const SMESH_MAT2d::BoundaryPoint& p) : _node(0), _u(p._param), _edgeInd(p._edgeIndex) {}
    gp_Pnt Point(const vector< Handle(Geom_Curve) >& curves) const
    {
      return _node ? SMESH_TNodeXYZ(_node) : curves[ _edgeInd ]->Value( _u );
    }
  };
  typedef multimap< double, pair< NodePoint, NodePoint > > TMAPar2NPoints;

  //================================================================================
  /*!
   * \brief Finds a VERTEX corresponding to a point on EDGE, which is also filled
   *        with a node on the VERTEX, present or created
   *  \param [in,out] theNodePnt - the node position on the EDGE
   *  \param [in] theSinuEdges - the sinuous EDGEs
   *  \param [in] theMeshDS - the mesh
   *  \return bool - true if the \a theBndPnt is on VERTEX
   */
  //================================================================================

  bool findVertexAndNode( NodePoint&                  theNodePnt,
                          const vector<TopoDS_Edge>&  theSinuEdges,
                          SMESHDS_Mesh*               theMeshDS = 0,
                          size_t                      theEdgeIndPrev = 0,
                          size_t                      theEdgeIndNext = 0)
  {
    if ( theNodePnt._edgeInd >= theSinuEdges.size() )
      return false;

    double f,l;
    BRep_Tool::Range( theSinuEdges[ theNodePnt._edgeInd ], f,l );
    const double tol = 1e-3 * ( l - f );

    TopoDS_Vertex V;
    if      ( Abs( f - theNodePnt._u ) < tol )
      V = SMESH_MesherHelper::IthVertex( 0, theSinuEdges[ theNodePnt._edgeInd ], /*CumOri=*/false);
    else if ( Abs( l - theNodePnt._u ) < tol )
      V = SMESH_MesherHelper::IthVertex( 1, theSinuEdges[ theNodePnt._edgeInd ], /*CumOri=*/false);
    else if ( theEdgeIndPrev != theEdgeIndNext )
      TopExp::CommonVertex( theSinuEdges[theEdgeIndPrev], theSinuEdges[theEdgeIndNext], V );

    if ( !V.IsNull() && theMeshDS )
    {
      theNodePnt._node = SMESH_Algo::VertexNode( V, theMeshDS );
      if ( !theNodePnt._node )
      {
        gp_Pnt p = BRep_Tool::Pnt( V );
        theNodePnt._node = theMeshDS->AddNode( p.X(), p.Y(), p.Z() );
        theMeshDS->SetNodeOnVertex( theNodePnt._node, V );
      }
    }
    return !V.IsNull();
  }

  //================================================================================
  /*!
   * \brief Add to the map of NodePoint's those on VERTEXes
   *  \param [in,out] theHelper - the helper
   *  \param [in] theMA - Medial Axis
   *  \param [in] theMinSegLen - minimal segment length
   *  \param [in] theDivPoints - projections of VERTEXes to MA
   *  \param [in] theSinuEdges - the sinuous EDGEs
   *  \param [in] theSideEdgeIDs - indices of sinuous EDGEs per side
   *  \param [in] theIsEdgeComputed - is sinuous EGDE is meshed
   *  \param [in,out] thePointsOnE - the map to fill
   *  \param [out] theNodes2Merge - the map of nodes to merge
   */
  //================================================================================

  bool projectVertices( SMESH_MesherHelper&                 theHelper,
                        const SMESH_MAT2d::MedialAxis&      theMA,
                        vector< SMESH_MAT2d::BranchPoint >& theDivPoints,
                        const vector< std::size_t > &       theEdgeIDs1,
                        const vector< std::size_t > &       theEdgeIDs2,
                        const vector< bool >&               theIsEdgeComputed,
                        TMAPar2NPoints &                    thePointsOnE,
                        SinuousFace&                        theSinuFace)
  {
    if ( theDivPoints.empty() )
      return true;

    SMESHDS_Mesh* meshDS = theHelper.GetMeshDS();
    const vector< TopoDS_Edge >&     theSinuEdges = theSinuFace._sinuEdges;
    const vector< Handle(Geom_Curve) >& theCurves = theSinuFace._sinuCurves;

    double uMA;
    SMESH_MAT2d::BoundaryPoint bp[2]; // 2 sinuous sides
    const SMESH_MAT2d::Branch& branch = *theMA.getBranch(0);
    {
      // add to thePointsOnE NodePoint's of ends of theSinuEdges
      if ( !branch.getBoundaryPoints( 0., bp[0], bp[1] ) ||
           !theMA.getBoundary().moveToClosestEdgeEnd( bp[0] )) return false;
      if ( !theSinuFace.IsRing() &&
           !theMA.getBoundary().moveToClosestEdgeEnd( bp[1] )) return false;
      NodePoint np0( bp[0] ), np1( bp[1] );
      findVertexAndNode( np0, theSinuEdges, meshDS );
      findVertexAndNode( np1, theSinuEdges, meshDS );
      thePointsOnE.insert( make_pair( -0.1, make_pair( np0, np1 )));
    }
    if ( !theSinuFace.IsRing() )
    {
      if ( !branch.getBoundaryPoints( 1., bp[0], bp[1] ) ||
           !theMA.getBoundary().moveToClosestEdgeEnd( bp[0] ) ||
           !theMA.getBoundary().moveToClosestEdgeEnd( bp[1] )) return false;
      NodePoint np0( bp[0] ), np1( bp[1] );
      findVertexAndNode( np0, theSinuEdges, meshDS );
      findVertexAndNode( np1, theSinuEdges, meshDS );
      thePointsOnE.insert( make_pair( 1.1, make_pair( np0, np1)));
    }
    else
    {
      // project a VERTEX of outer sinuous side corresponding to branch(0.)
      // which is not included into theDivPoints
      if ( ! ( theDivPoints[0]._iEdge     == 0 &&
               theDivPoints[0]._edgeParam == 0. )) // recursive call
      {
        SMESH_MAT2d::BranchPoint brp( &branch, 0, 0 );
        vector< SMESH_MAT2d::BranchPoint > divPoint( 1, brp );
        vector< std::size_t > edgeIDs1(2), edgeIDs2(2);
        edgeIDs1[0] = theEdgeIDs1.back();
        edgeIDs1[1] = theEdgeIDs1[0];
        edgeIDs2[0] = theEdgeIDs2.back();
        edgeIDs2[1] = theEdgeIDs2[0];
        projectVertices( theHelper, theMA, divPoint, edgeIDs1, edgeIDs2,
                         theIsEdgeComputed, thePointsOnE, theSinuFace );
      }
    }

    // project theDivPoints

    TMAPar2NPoints::iterator u2NP;
    for ( size_t i = 0; i < theDivPoints.size(); ++i )
    {
      if ( !branch.getParameter( theDivPoints[i], uMA ))
        return false;
      if ( !branch.getBoundaryPoints( theDivPoints[i], bp[0], bp[1] ))
        return false;

      NodePoint  np[2] = {
        NodePoint( bp[0] ),
        NodePoint( bp[1] )
      };
      bool isVertex[2] = {
        findVertexAndNode( np[0], theSinuEdges, meshDS, theEdgeIDs1[i], theEdgeIDs1[i+1] ),
        findVertexAndNode( np[1], theSinuEdges, meshDS, theEdgeIDs2[i], theEdgeIDs2[i+1] )
      };
      const size_t iVert = isVertex[0] ? 0 : 1; // side with a VERTEX
      const size_t iNode = 1 - iVert;           // opposite (meshed?) side

      if ( isVertex[0] != isVertex[1] ) // try to find an opposite VERTEX
      {
        theMA.getBoundary().moveToClosestEdgeEnd( bp[iNode] ); // EDGE -> VERTEX
        SMESH_MAT2d::BranchPoint brp;
        theMA.getBoundary().getBranchPoint( bp[iNode], brp );  // WIRE -> MA
        SMESH_MAT2d::BoundaryPoint bp2[2];
        branch.getBoundaryPoints( brp, bp2[0], bp2[1] );       // MA -> WIRE
        NodePoint np2[2] = { NodePoint( bp2[0]), NodePoint( bp2[1]) };
        findVertexAndNode( np2[0], theSinuEdges, meshDS );
        findVertexAndNode( np2[1], theSinuEdges, meshDS );
        if ( np2[ iVert ]._node == np[ iVert ]._node &&
             np2[ iNode ]._node)
        {
          np[ iNode ] = np2[ iNode ];
          isVertex[ iNode ] = true;
        }
      }

      u2NP = thePointsOnE.insert( make_pair( uMA, make_pair( np[0], np[1])));

      if ( !isVertex[0] && !isVertex[1] ) return false; // error
      if ( isVertex[0] && isVertex[1] )
        continue;

      bool isOppComputed = theIsEdgeComputed[ np[ iNode ]._edgeInd ];
      if ( !isOppComputed )
        continue;

      // a VERTEX is projected on a meshed EDGE; there are two options:
      // 1) a projected point is joined with a closet node if a strip between this and neighbor
      // projection is WIDE enough; joining is done by creating a node coincident with the
      //  existing node which will be merged together after all;
      // 2) a neighbor projection is merged with this one if it is TOO CLOSE; a node of deleted
      // projection is set to the BoundaryPoint of this projection

      // evaluate distance to neighbor projections
      const double rShort = 0.33;
      bool isShortPrev[2], isShortNext[2], isPrevCloser[2];
      TMAPar2NPoints::iterator u2NPPrev = u2NP, u2NPNext = u2NP;
      --u2NPPrev; ++u2NPNext;
      // bool hasPrev = ( u2NP     != thePointsOnE.begin() );
      // bool hasNext = ( u2NPNext != thePointsOnE.end() );
      // if ( !hasPrev ) u2NPPrev = u2NP0;
      // if ( !hasNext ) u2NPNext = u2NP1;
      for ( int iS = 0; iS < 2; ++iS ) // side with Vertex and side with Nodes
      {
        NodePoint np     = get( u2NP->second,     iS );
        NodePoint npPrev = get( u2NPPrev->second, iS );
        NodePoint npNext = get( u2NPNext->second, iS );
        gp_Pnt     p     = np    .Point( theCurves );
        gp_Pnt     pPrev = npPrev.Point( theCurves );
        gp_Pnt     pNext = npNext.Point( theCurves );
        double  distPrev = p.Distance( pPrev );
        double  distNext = p.Distance( pNext );
        double         r = distPrev / ( distPrev + distNext );
        isShortPrev [iS] = ( r < rShort );
        isShortNext [iS] = (( 1 - r ) > ( 1 - rShort ));
        isPrevCloser[iS] = (( r < 0.5 ) && ( u2NPPrev->first > 0 ));
      }
      // if ( !hasPrev ) isShortPrev[0] = isShortPrev[1] = false;
      // if ( !hasNext ) isShortNext[0] = isShortNext[1] = false;

      TMAPar2NPoints::iterator u2NPClose;

      if (( isShortPrev[0] && isShortPrev[1] ) || // option 2) -> remove a too close projection
          ( isShortNext[0] && isShortNext[1] ))
      {
        u2NPClose = isPrevCloser[0] ? u2NPPrev : u2NPNext;
        NodePoint& npProj  = get( u2NP->second,      iNode ); // NP of VERTEX projection
        NodePoint npCloseN = get( u2NPClose->second, iNode ); // NP close to npProj
        NodePoint npCloseV = get( u2NPClose->second, iVert ); // NP close to VERTEX
        if ( !npCloseV._node )
        {
          npProj = npCloseN;
          thePointsOnE.erase( isPrevCloser[0] ? u2NPPrev : u2NPNext );
          continue;
        }
        else
        {
          // can't remove the neighbor projection as it is also from VERTEX, -> option 1)
        }
      }
      // else: option 1) - wide enough -> "duplicate" existing node
      {
        u2NPClose = isPrevCloser[ iNode ] ? u2NPPrev : u2NPNext;
        NodePoint& npProj   = get( u2NP->second,      iNode ); // NP of VERTEX projection
        NodePoint& npCloseN = get( u2NPClose->second, iNode ); // NP close to npProj
        npProj = npCloseN;
        npProj._node = 0;
        //npProj._edgeInd = npCloseN._edgeInd;
        // npProj._u       = npCloseN._u + 1e-3 * Abs( get( u2NPPrev->second, iNode )._u -
        //                                             get( u2NPNext->second, iNode )._u );
        // gp_Pnt        p = npProj.Point( theCurves );
        // npProj._node    = meshDS->AddNode( p.X(), p.Y(), p.Z() );
        // meshDS->SetNodeOnEdge( npProj._node, theSinuEdges[ npProj._edgeInd ], npProj._u  );

        //theNodes2Merge[ npCloseN._node ].push_back( npProj._node );
      }
    }

    // remove auxiliary NodePoint's of ends of theSinuEdges
    for ( u2NP = thePointsOnE.begin(); u2NP->first < 0; )
      thePointsOnE.erase( u2NP++ );
    thePointsOnE.erase( 1.1 );

    return true;
  }

  double getUOnEdgeByPoint( const size_t     iEdge,
                            const NodePoint* point,
                            SinuousFace&     sinuFace )
  {
    if ( point->_edgeInd == iEdge )
      return point->_u;

    TopoDS_Vertex V0 = TopExp::FirstVertex( sinuFace._sinuEdges[ iEdge ]);
    TopoDS_Vertex V1 = TopExp::LastVertex ( sinuFace._sinuEdges[ iEdge ]);
    gp_Pnt p0 = BRep_Tool::Pnt( V0 );
    gp_Pnt p1 = BRep_Tool::Pnt( V1 );
    gp_Pnt  p = point->Point( sinuFace._sinuCurves );

    double f,l;
    BRep_Tool::Range( sinuFace._sinuEdges[ iEdge ], f,l );
    return p.SquareDistance( p0 ) < p.SquareDistance( p1 ) ? f : l;
  }

  //================================================================================
  /*!
   * \brief Move coincident nodes to make node params on EDGE unique
   *  \param [in] theHelper - the helper
   *  \param [in] thePointsOnE - nodes on two opposite river sides
   *  \param [in] theSinuFace - the sinuous FACE
   *  \param [out] theNodes2Merge - the map of nodes to merge
   */
  //================================================================================

  void separateNodes( SMESH_MesherHelper&            theHelper,
                      const SMESH_MAT2d::MedialAxis& theMA,
                      TMAPar2NPoints &               thePointsOnE,
                      SinuousFace&                   theSinuFace,
                      const vector< bool >&          theIsComputedEdge)
  {
    if ( thePointsOnE.size() < 2 )
      return;

    SMESHDS_Mesh* meshDS = theHelper.GetMeshDS();
    const vector<TopoDS_Edge>&    theSinuEdges = theSinuFace._sinuEdges;
    const vector< Handle(Geom_Curve) >& curves = theSinuFace._sinuCurves;

    SMESH_MAT2d::BoundaryPoint bp[2];
    const SMESH_MAT2d::Branch& branch = *theMA.getBranch(0);

    typedef TMAPar2NPoints::iterator TIterator;

    for ( int iSide = 0; iSide < 2; ++iSide ) // loop on two sinuous sides
    {
      // get a tolerance to compare points
      double tol = Precision::Confusion();
      for ( size_t i = 0; i < theSinuFace._sinuSide[ iSide ].size(); ++i )
        tol = Max( tol , BRep_Tool::Tolerance( theSinuFace._sinuSide[ iSide ][ i ]));

      // find coincident points
      TIterator u2NP = thePointsOnE.begin();
      vector< TIterator > sameU2NP( 1, u2NP++ );
      while ( u2NP != thePointsOnE.end() )
      {
        for ( ; u2NP != thePointsOnE.end(); ++u2NP )
        {
          NodePoint& np1 = get( sameU2NP.back()->second, iSide );
          NodePoint& np2 = get( u2NP           ->second, iSide );

          if (( !np1._node || !np2._node ) &&
              ( np1.Point( curves ).SquareDistance( np2.Point( curves )) < tol*tol ))
          {
            sameU2NP.push_back( u2NP );
          }
          else if ( sameU2NP.size() == 1 )
          {
            sameU2NP[ 0 ] = u2NP;
          }
          else
          {
            break;
          }
        }

        if ( sameU2NP.size() > 1 )
        {
          // find an existing node on VERTEX among sameU2NP and get underlying EDGEs
          const SMDS_MeshNode* existingNode = 0;
          set< int > edgeInds;
          NodePoint* np;
          for ( size_t i = 0; i < sameU2NP.size(); ++i )
          {
            np = &get( sameU2NP[i]->second, iSide );
            if ( np->_node )
              if ( !existingNode || np->_node->GetPosition()->GetDim() == 0 )
                existingNode = np->_node;
            edgeInds.insert( np->_edgeInd );
          }
          list< const SMDS_MeshNode* >& mergeNodes = theSinuFace._nodesToMerge[ existingNode ];

          TIterator u2NPprev = sameU2NP.front();
          TIterator u2NPnext = sameU2NP.back() ;
          if ( u2NPprev->first < 0. ) ++u2NPprev;
          if ( u2NPnext->first > 1. ) --u2NPnext;

          set< int >::iterator edgeID = edgeInds.begin();
          for ( ; edgeID != edgeInds.end(); ++edgeID )
          {
            // get U range on iEdge within which the equal points will be distributed
            double u0, u1;
            np = &get( u2NPprev->second, iSide );
            u0 = getUOnEdgeByPoint( *edgeID, np, theSinuFace );

            np = &get( u2NPnext->second, iSide );
            u1 = getUOnEdgeByPoint( *edgeID, np, theSinuFace );

            if ( u0 == u1 )
            {
              if ( u2NPprev != thePointsOnE.begin() ) --u2NPprev;
              if ( u2NPnext != --thePointsOnE.end() ) ++u2NPnext;
              np = &get( u2NPprev->second, iSide );
              u0 = getUOnEdgeByPoint( *edgeID, np, theSinuFace );
              np = &get( u2NPnext->second, iSide );
              u1 = getUOnEdgeByPoint( *edgeID, np, theSinuFace );
            }

            // distribute points and create nodes
            double du = ( u1 - u0 ) / ( sameU2NP.size() + 1 /*!existingNode*/ );
            double u  = u0 + du;
            for ( size_t i = 0; i < sameU2NP.size(); ++i )
            {
              np = &get( sameU2NP[i]->second, iSide );
              if ( !np->_node && *edgeID == np->_edgeInd )
              {
                np->_u = u;
                u += du;
                gp_Pnt p = np->Point( curves );
                np->_node = meshDS->AddNode( p.X(), p.Y(), p.Z() );
                meshDS->SetNodeOnEdge( np->_node, theSinuEdges[ *edgeID ], np->_u  );

                if ( theIsComputedEdge[ *edgeID ])
                  mergeNodes.push_back( np->_node );
              }
            }
          }

          sameU2NP.resize( 1 );
          u2NP = ++sameU2NP.back();
          sameU2NP[ 0 ] = u2NP;

        } // if ( sameU2NP.size() > 1 )
      } // while ( u2NP != thePointsOnE.end() )
    } // for ( int iSide = 0; iSide < 2; ++iSide )

    return;
  } // separateNodes()

  //================================================================================
  /*!
   * \brief Setup sides of SinuousFace::_quad
   *  \param [in] theHelper - helper
   *  \param [in] thePointsOnEdges - NodePoint's on sinuous sides
   *  \param [in,out] theSinuFace - the FACE
   *  \param [in] the1dAlgo - algorithm to use for radial discretization of a ring FACE
   *  \return bool - is OK
   */
  //================================================================================

  bool setQuadSides(SMESH_MesherHelper&   theHelper,
                    const TMAPar2NPoints& thePointsOnEdges,
                    SinuousFace&          theFace,
                    SMESH_Algo*           the1dAlgo)
  {
    SMESH_Mesh*               mesh = theHelper.GetMesh();
    const TopoDS_Face&        face = theFace._quad->face;
    SMESH_ProxyMesh::Ptr proxyMesh = StdMeshers_ViscousLayers2D::Compute( *mesh, face );
    if ( !proxyMesh )
      return false;

    list< TopoDS_Edge > side[4];
    side[0].insert( side[0].end(), theFace._shortSide[0].begin(), theFace._shortSide[0].end() );
    side[1].insert( side[1].end(), theFace._sinuSide[1].begin(),  theFace._sinuSide[1].end() );
    side[2].insert( side[2].end(), theFace._shortSide[1].begin(), theFace._shortSide[1].end() );
    side[3].insert( side[3].end(), theFace._sinuSide[0].begin(),  theFace._sinuSide[0].end() );

    for ( int i = 0; i < 4; ++i )
    {
      theFace._quad->side[i] = StdMeshers_FaceSide::New( face, side[i], mesh, i < QUAD_TOP_SIDE,
                                                         /*skipMediumNodes=*/true, proxyMesh );
    }

    if ( theFace.IsRing() )
    {
      // --------------------------------------
      // Discretize a ring in radial direction
      // --------------------------------------

      if ( thePointsOnEdges.size() < 4 )
        return false;

      // find most distant opposite nodes
      double maxDist = 0, dist;
      TMAPar2NPoints::const_iterator u2NPdist, u2NP = thePointsOnEdges.begin();
      for ( ; u2NP != thePointsOnEdges.end(); ++u2NP )
      {
        SMESH_TNodeXYZ        xyz( u2NP->second.first._node ); // node out
        dist = xyz.SquareDistance( u2NP->second.second._node );// node in
        if ( dist > maxDist )
        {
          u2NPdist = u2NP;
          maxDist = dist;
        }
      }
      // compute distribution of radial nodes
      list< double > params; // normalized params
      static_cast< StdMeshers_QuadFromMedialAxis_1D2D::Algo1D* >
        ( the1dAlgo )->ComputeDistribution( theHelper,
                                            SMESH_TNodeXYZ( u2NPdist->second.first._node ),
                                            SMESH_TNodeXYZ( u2NPdist->second.second._node ),
                                            params );

      // add a radial quad side
      u2NP = thePointsOnEdges.begin();
      const SMDS_MeshNode* nOut = u2NP->second.first._node;
      const SMDS_MeshNode*  nIn = u2NP->second.second._node;
      nOut = proxyMesh->GetProxyNode( nOut );
      nIn  = proxyMesh->GetProxyNode( nIn );
      gp_XY uvOut = theHelper.GetNodeUV( face, nOut );
      gp_XY uvIn  = theHelper.GetNodeUV( face, nIn );
      Handle(Geom_Surface) surface = BRep_Tool::Surface( face );
      UVPtStructVec uvsNew; UVPtStruct uvPt;
      uvPt.node = nOut;
      uvPt.u    = uvOut.X();
      uvPt.v    = uvOut.Y();
      uvsNew.push_back( uvPt );
      for (list<double>::iterator itU = params.begin(); itU != params.end(); ++itU )
      {
        gp_XY uv  = ( 1 - *itU ) * uvOut + *itU * uvIn;
        gp_Pnt p  = surface->Value( uv.X(), uv.Y() );
        uvPt.node = theHelper.AddNode( p.X(), p.Y(), p.Z(), /*id=*/0, uv.X(), uv.Y() );
        uvPt.u    = uv.X();
        uvPt.v    = uv.Y();
        uvsNew.push_back( uvPt );
      }
      uvPt.node = nIn;
      uvPt.u    = uvIn.X();
      uvPt.v    = uvIn.Y();
      uvsNew.push_back( uvPt );

      theFace._quad->side[ 0 ] = StdMeshers_FaceSide::New( uvsNew );
      theFace._quad->side[ 2 ] = theFace._quad->side[ 0 ];

      if ( theFace._quad->side[ 1 ].GetUVPtStruct().empty() ||
           theFace._quad->side[ 3 ].GetUVPtStruct().empty() )
        return false;

      // assure that the outer sinuous side starts at nOut
      if ( theFace._sinuSide[0].size() > 1 )
      {
        const UVPtStructVec& uvsOut = theFace._quad->side[ 3 ].GetUVPtStruct(); // _sinuSide[0]
        size_t i; // find UVPtStruct holding nOut
        for ( i = 0; i < uvsOut.size(); ++i )
          if ( nOut == uvsOut[i].node )
            break;
        if ( i == uvsOut.size() )
          return false;

        if ( i != 0  &&  i != uvsOut.size()-1 )
        {
          // create a new OUT quad side
          uvsNew.clear();
          uvsNew.reserve( uvsOut.size() );
          uvsNew.insert( uvsNew.end(), uvsOut.begin() + i, uvsOut.end() );
          uvsNew.insert( uvsNew.end(), uvsOut.begin() + 1, uvsOut.begin() + i + 1);
          theFace._quad->side[ 3 ] = StdMeshers_FaceSide::New( uvsNew );
        }
      }

      // rotate the IN side if opposite nodes of IN and OUT sides don't match
      const SMDS_MeshNode * nIn0 = theFace._quad->side[ 1 ].First().node;
      if ( nIn0 != nIn )
      {
        nIn  = proxyMesh->GetProxyNode( nIn );
        const UVPtStructVec& uvsIn = theFace._quad->side[ 1 ].GetUVPtStruct(); // _sinuSide[1]
        size_t i; // find UVPtStruct holding nIn
        for ( i = 0; i < uvsIn.size(); ++i )
          if ( nIn == uvsIn[i].node )
            break;
        if ( i == uvsIn.size() )
          return false;

        // create a new IN quad side
        uvsNew.clear();
        uvsNew.reserve( uvsIn.size() );
        uvsNew.insert( uvsNew.end(), uvsIn.begin() + i, uvsIn.end() );
        uvsNew.insert( uvsNew.end(), uvsIn.begin() + 1, uvsIn.begin() + i + 1);
        theFace._quad->side[ 1 ] = StdMeshers_FaceSide::New( uvsNew );
      }

      if ( theFace._quad->side[ 1 ].GetUVPtStruct().empty() ||
           theFace._quad->side[ 3 ].GetUVPtStruct().empty() )
        return false;

    } // if ( theFace.IsRing() )

    return true;

  } // setQuadSides()

  //================================================================================
  /*!
   * \brief Divide the sinuous EDGEs by projecting the division point of Medial
   *        Axis to the EGDEs
   *  \param [in] theHelper - the helper
   *  \param [in] theMinSegLen - minimal segment length
   *  \param [in] theMA - the Medial Axis
   *  \param [in] theMAParams - parameters of division points of \a theMA
   *  \param [in] theSinuEdges - the EDGEs to make nodes on
   *  \param [in] theSinuSide0Size - the number of EDGEs in the 1st sinuous side
   *  \param [in] the1dAlgo - algorithm to use for radial discretization of a ring FACE
   *  \return bool - is OK or not
   */
  //================================================================================

  bool computeSinuEdges( SMESH_MesherHelper&        theHelper,
                         double                     /*theMinSegLen*/,
                         SMESH_MAT2d::MedialAxis&   theMA,
                         vector<double>&            theMAParams,
                         SinuousFace&               theSinuFace,
                         SMESH_Algo*                the1dAlgo)
  {
    if ( theMA.nbBranches() != 1 )
      return false;

    // normalize theMAParams
    for ( size_t i = 0; i < theMAParams.size(); ++i )
      theMAParams[i] /= theMAParams.back();


    SMESH_Mesh*     mesh = theHelper.GetMesh();
    SMESHDS_Mesh* meshDS = theHelper.GetMeshDS();
    double f,l;

    // get data of sinuous EDGEs and remove unnecessary nodes
    const vector< TopoDS_Edge >& theSinuEdges = theSinuFace._sinuEdges;
    vector< Handle(Geom_Curve) >& curves      = theSinuFace._sinuCurves;
    vector< int >                edgeIDs   ( theSinuEdges.size() ); // IDs in the main shape
    vector< bool >               isComputed( theSinuEdges.size() );
    curves.resize( theSinuEdges.size(), 0 );
    bool                         allComputed = true;
    for ( size_t i = 0; i < theSinuEdges.size(); ++i )
    {
      curves[i] = BRep_Tool::Curve( theSinuEdges[i], f,l );
      if ( !curves[i] )
        return false;
      SMESH_subMesh* sm = mesh->GetSubMesh( theSinuEdges[i] );
      edgeIDs   [i] = sm->GetId();
      isComputed[i] = ( !sm->IsEmpty() );
      if ( !isComputed[i] )
        allComputed = false;
    }

    const SMESH_MAT2d::Branch& branch = *theMA.getBranch(0);
    SMESH_MAT2d::BoundaryPoint bp[2];

    vector< std::size_t > edgeIDs1, edgeIDs2; // indices in theSinuEdges
    vector< SMESH_MAT2d::BranchPoint > divPoints;
    if ( !allComputed )
      branch.getOppositeGeomEdges( edgeIDs1, edgeIDs2, divPoints );

    for ( size_t i = 0; i < edgeIDs1.size(); ++i )
      if ( isComputed[ edgeIDs1[i]] &&
           isComputed[ edgeIDs2[i]] )
      {
        int nbNodes1 = meshDS->MeshElements(edgeIDs[ edgeIDs1[i]] )->NbNodes();
        int nbNodes2 = meshDS->MeshElements(edgeIDs[ edgeIDs2[i]] )->NbNodes();
        if ( nbNodes1 != nbNodes2 )
          return false;
        if (( i >= 1 ) &&
            ( edgeIDs1[i-1] == edgeIDs1[i] ||
              edgeIDs2[i-1] == edgeIDs2[i] ))
          return false;
        if (( i+1 < edgeIDs1.size() ) &&
            ( edgeIDs1[i+1] == edgeIDs1[i] ||
              edgeIDs2[i+1] == edgeIDs2[i] ))
          return false;
      }

    // map (param on MA) to (parameters of nodes on a pair of theSinuEdges)
    TMAPar2NPoints pointsOnE;
    vector<double> maParams;
    set<int>       projectedEdges; // treated EDGEs which 'isComputed'

    // compute params of nodes on EDGEs by projecting division points from MA

    for ( size_t iEdgePair = 0; iEdgePair < edgeIDs1.size(); ++iEdgePair )
      // loop on pairs of opposite EDGEs
    {
      if ( projectedEdges.count( edgeIDs1[ iEdgePair ]) ||
           projectedEdges.count( edgeIDs2[ iEdgePair ]) )
        continue;

      // --------------------------------------------------------------------------------
      if ( isComputed[ edgeIDs1[ iEdgePair ]] !=                    // one EDGE is meshed
           isComputed[ edgeIDs2[ iEdgePair ]])
      {
        // "projection" from one side to the other

        size_t iEdgeComputed = edgeIDs1[iEdgePair], iSideComputed = 0;
        if ( !isComputed[ iEdgeComputed ])
          ++iSideComputed, iEdgeComputed = edgeIDs2[iEdgePair];

        map< double, const SMDS_MeshNode* > nodeParams; // params of existing nodes
        if ( !SMESH_Algo::GetSortedNodesOnEdge( meshDS, theSinuEdges[ iEdgeComputed ], /*skipMedium=*/true, nodeParams ))
          return false;

        projectedEdges.insert( iEdgeComputed );

        SMESH_MAT2d::BoundaryPoint& bndPnt = bp[ 1-iSideComputed ];
        SMESH_MAT2d::BranchPoint brp;
        NodePoint npN, npB; // NodePoint's initialized by node and BoundaryPoint
        NodePoint& np0 = iSideComputed ? npB : npN;
        NodePoint& np1 = iSideComputed ? npN : npB;

        double maParam1st, maParamLast, maParam;
        if ( !theMA.getBoundary().getBranchPoint( iEdgeComputed, nodeParams.begin()->first, brp ))
          return false;
        branch.getParameter( brp, maParam1st );
        if ( !theMA.getBoundary().getBranchPoint( iEdgeComputed, nodeParams.rbegin()->first, brp ))
          return false;
        branch.getParameter( brp, maParamLast );

        map< double, const SMDS_MeshNode* >::iterator u2n = nodeParams.begin(), u2nEnd = nodeParams.end();
        TMAPar2NPoints::iterator end = pointsOnE.end(), pos = end;
        TMAPar2NPoints::iterator & hint = (maParamLast > maParam1st) ? end : pos;
        for ( ++u2n, --u2nEnd; u2n != u2nEnd; ++u2n )
        {
          // point on EDGE (u2n) --> MA point (brp)
          if ( !theMA.getBoundary().getBranchPoint( iEdgeComputed, u2n->first, brp ))
            return false;
          // MA point --> points on 2 EDGEs (bp)
          if ( !branch.getBoundaryPoints( brp, bp[0], bp[1] ) ||
               !branch.getParameter( brp, maParam ))
            return false;

          npN = NodePoint( u2n->second, u2n->first, iEdgeComputed );
          npB = NodePoint( bndPnt );
          pos = pointsOnE.insert( hint, make_pair( maParam, make_pair( np0, np1 )));
        }
      }
      // --------------------------------------------------------------------------------
      else if ( !isComputed[ edgeIDs1[ iEdgePair ]] &&         // none of EDGEs is meshed
                !isComputed[ edgeIDs2[ iEdgePair ]])
      {
        // "projection" from MA
        maParams.clear();
        if ( !getParamsForEdgePair( iEdgePair, divPoints, theMAParams, maParams ))
          return false;

        for ( size_t i = 1; i < maParams.size()-1; ++i )
        {
          if ( !branch.getBoundaryPoints( maParams[i], bp[0], bp[1] ))
            return false;

          pointsOnE.insert( pointsOnE.end(), make_pair( maParams[i], make_pair( NodePoint(bp[0]),
                                                                                NodePoint(bp[1]))));
        }
      }
      // --------------------------------------------------------------------------------
      else if ( isComputed[ edgeIDs1[ iEdgePair ]] &&             // equally meshed EDGES
                isComputed[ edgeIDs2[ iEdgePair ]])
      {
        // add existing nodes

        size_t iE0 = edgeIDs1[ iEdgePair ];
        size_t iE1 = edgeIDs2[ iEdgePair ];
        map< double, const SMDS_MeshNode* > nodeParams[2]; // params of existing nodes
        if ( !SMESH_Algo::GetSortedNodesOnEdge( meshDS, theSinuEdges[ iE0 ],
                                                /*skipMedium=*/false, nodeParams[0] ) ||
             !SMESH_Algo::GetSortedNodesOnEdge( meshDS, theSinuEdges[ iE1 ],
                                                /*skipMedium=*/false, nodeParams[1] ) ||
             nodeParams[0].size() != nodeParams[1].size() )
          return false;

        if ( nodeParams[0].size() <= 2 )
          continue; // nodes on VERTEXes only

        bool reverse = ( theSinuEdges[0].Orientation() == theSinuEdges[1].Orientation() );
        double maParam;
        SMESH_MAT2d::BranchPoint brp;
        std::pair< NodePoint, NodePoint > npPair;

        map< double, const SMDS_MeshNode* >::iterator
          u2n0F = ++nodeParams[0].begin(),
          u2n1F = ++nodeParams[1].begin();
        map< double, const SMDS_MeshNode* >::reverse_iterator
          u2n1R = ++nodeParams[1].rbegin();
        for ( ; u2n0F != nodeParams[0].end(); ++u2n0F )
        {
          if ( !theMA.getBoundary().getBranchPoint( iE0, u2n0F->first, brp ) ||
               !branch.getParameter( brp, maParam ))
            return false;

          npPair.first = NodePoint( u2n0F->second, u2n0F->first, iE0 );
          if ( reverse )
          {
            npPair.second = NodePoint( u2n1R->second, u2n1R->first, iE1 );
            ++u2n1R;
          }
          else
          {
            npPair.second = NodePoint( u2n1F->second, u2n1F->first, iE1 );
            ++u2n1F;
          }
          pointsOnE.insert( make_pair( maParam, npPair ));
        }
      }
    }  // loop on pairs of opposite EDGEs

    if ( !projectVertices( theHelper, theMA, divPoints, edgeIDs1, edgeIDs2,
                           isComputed, pointsOnE, theSinuFace ))
      return false;

    separateNodes( theHelper, theMA, pointsOnE, theSinuFace, isComputed );

    // create nodes
    TMAPar2NPoints::iterator u2np = pointsOnE.begin();
    for ( ; u2np != pointsOnE.end(); ++u2np )
    {
      NodePoint* np[2] = { & u2np->second.first, & u2np->second.second };
      for ( int iSide = 0; iSide < 2; ++iSide )
      {
        if ( np[ iSide ]->_node ) continue;
        size_t       iEdge = np[ iSide ]->_edgeInd;
        double           u = np[ iSide ]->_u;
        gp_Pnt           p = curves[ iEdge ]->Value( u );
        np[ iSide ]->_node = meshDS->AddNode( p.X(), p.Y(), p.Z() );
        meshDS->SetNodeOnEdge( np[ iSide ]->_node, edgeIDs[ iEdge ], u );
      }
    }

    // create mesh segments on EDGEs
    theHelper.SetElementsOnShape( false );
    TopoDS_Face face = TopoDS::Face( theHelper.GetSubShape() );
    for ( size_t i = 0; i < theSinuEdges.size(); ++i )
    {
      SMESH_subMesh* sm = mesh->GetSubMesh( theSinuEdges[i] );
      if ( sm->GetSubMeshDS() && sm->GetSubMeshDS()->NbElements() > 0 )
        continue;

      StdMeshers_FaceSide side( face, theSinuEdges[i], mesh,
                                /*isFwd=*/true, /*skipMediumNodes=*/true );
      vector<const SMDS_MeshNode*> nodes = side.GetOrderedNodes();
      for ( size_t in = 1; in < nodes.size(); ++in )
      {
        const SMDS_MeshElement* seg = theHelper.AddEdge( nodes[in-1], nodes[in], 0, false );
        meshDS->SetMeshElementOnShape( seg, edgeIDs[ i ] );
      }
    }

    // update sub-meshes on VERTEXes
    for ( size_t i = 0; i < theSinuEdges.size(); ++i )
    {
      mesh->GetSubMesh( theHelper.IthVertex( 0, theSinuEdges[i] ))
        ->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
      mesh->GetSubMesh( theHelper.IthVertex( 1, theSinuEdges[i] ))
        ->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
    }

    // Setup sides of a quadrangle
    if ( !setQuadSides( theHelper, pointsOnE, theSinuFace, the1dAlgo ))
      return false;

    return true;
  }

  //================================================================================
  /*!
   * \brief Mesh short EDGEs
   */
  //================================================================================

  bool computeShortEdges( SMESH_MesherHelper&        theHelper,
                          const vector<TopoDS_Edge>& theShortEdges,
                          SMESH_Algo*                the1dAlgo,
                          const bool                 theHasRadialHyp,
                          const bool                 theIs2nd)
  {
    SMESH_Hypothesis::Hypothesis_Status aStatus;
    for ( size_t i = 0; i < theShortEdges.size(); ++i )
    {
      if ( !theHasRadialHyp )
        // use global hyps
        theHelper.GetGen()->Compute( *theHelper.GetMesh(), theShortEdges[i], true, true );

      SMESH_subMesh* sm = theHelper.GetMesh()->GetSubMesh(theShortEdges[i] );
      if ( sm->IsEmpty() )
      {
        // use 2D hyp or minSegLen
        try {
          // compute VERTEXes
          SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(/*includeSelf=*/false);
          while ( smIt->more() )
            smIt->next()->ComputeStateEngine( SMESH_subMesh::COMPUTE );

          // compute EDGE
          the1dAlgo->CheckHypothesis( *theHelper.GetMesh(), theShortEdges[i], aStatus );
          if ( !the1dAlgo->Compute( *theHelper.GetMesh(), theShortEdges[i] ))
            return false;
        }
        catch (...) {
          return false;
        }
        sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
        if ( sm->IsEmpty() )
          return false;
      }
    }
    return true;
  }

  inline double area( const UVPtStruct& p1, const UVPtStruct& p2, const UVPtStruct& p3 )
  {
    gp_XY v1 = p2.UV() - p1.UV();
    gp_XY v2 = p3.UV() - p1.UV();
    return v2 ^ v1;
  }

  bool ellipticSmooth( FaceQuadStruct::Ptr quad, int nbLoops )
  {
    //nbLoops = 10;
    if ( quad->uv_grid.empty() )
      return true;

    int nbhoriz  = quad->iSize;
    int nbvertic = quad->jSize;

    const double dksi = 0.5, deta = 0.5;
    const double  dksi2 = dksi*dksi, deta2 = deta*deta;
    double err = 0., g11, g22, g12;
    int nbErr = 0;

    FaceQuadStruct& q = *quad;
    UVPtStruct pNew;

    double refArea = area( q.UVPt(0,0), q.UVPt(1,0), q.UVPt(1,1) );

    for ( int iLoop = 0; iLoop < nbLoops; ++iLoop )
    {
      err = 0;
      for ( int i = 1; i < nbhoriz - 1; i++ )
        for ( int j = 1; j < nbvertic - 1; j++ )
        {
          g11 = ( (q.U(i,j+1) - q.U(i,j-1))*(q.U(i,j+1) - q.U(i,j-1))/dksi2 +
                  (q.V(i,j+1) - q.V(i,j-1))*(q.V(i,j+1) - q.V(i,j-1))/deta2 )/4;

          g22 = ( (q.U(i+1,j) - q.U(i-1,j))*(q.U(i+1,j) - q.U(i-1,j))/dksi2 +
                  (q.V(i+1,j) - q.V(i-1,j))*(q.V(i+1,j) - q.V(i-1,j))/deta2 )/4;

          g12 = ( (q.U(i+1,j) - q.U(i-1,j))*(q.U(i,j+1) - q.U(i,j-1))/dksi2 +
                  (q.V(i+1,j) - q.V(i-1,j))*(q.V(i,j+1) - q.V(i,j-1))/deta2 )/(4*dksi*deta);

          pNew.u = dksi2/(2*(g11+g22)) * (g11*(q.U(i+1,j) + q.U(i-1,j))/dksi2 +
                                          g22*(q.U(i,j+1) + q.U(i,j-1))/dksi2
                                          - 0.5*g12*q.U(i+1,j+1) + 0.5*g12*q.U(i-1,j+1) +
                                          - 0.5*g12*q.U(i-1,j-1) + 0.5*g12*q.U(i+1,j-1));

          pNew.v = deta2/(2*(g11+g22)) * (g11*(q.V(i+1,j) + q.V(i-1,j))/deta2 +
                                          g22*(q.V(i,j+1) + q.V(i,j-1))/deta2
                                          - 0.5*g12*q.V(i+1,j+1) + 0.5*g12*q.V(i-1,j+1) +
                                          - 0.5*g12*q.V(i-1,j-1) + 0.5*g12*q.V(i+1,j-1));

          // if (( refArea * area( q.UVPt(i-1,j-1), q.UVPt(i,j-1), pNew ) > 0 ) &&
          //     ( refArea * area( q.UVPt(i+1,j-1), q.UVPt(i+1,j), pNew ) > 0 ) &&
          //     ( refArea * area( q.UVPt(i+1,j+1), q.UVPt(i,j+1), pNew ) > 0 ) &&
          //     ( refArea * area( q.UVPt(i-1,j), q.UVPt(i-1,j-1), pNew ) > 0 ))
          {
            err += sqrt(( q.U(i,j) - pNew.u ) * ( q.U(i,j) - pNew.u ) +
                        ( q.V(i,j) - pNew.v ) * ( q.V(i,j) - pNew.v ));
            q.U(i,j) = pNew.u;
            q.V(i,j) = pNew.v;
          }
          // else if ( ++nbErr < 10 )
          // {
          //   cout << i << ", " << j << endl;
          //   cout << "x = ["
          //        << "[ " << q.U(i-1,j-1) << ", " <<q.U(i,j-1) << ", " << q.U(i+1,j-1) << " ],"
          //        << "[ " << q.U(i-1,j-0) << ", " <<q.U(i,j-0) << ", " << q.U(i+1,j-0) << " ],"
          //        << "[ " << q.U(i-1,j+1) << ", " <<q.U(i,j+1) << ", " << q.U(i+1,j+1) << " ]]" << endl;
          //   cout << "y = ["
          //        << "[ " << q.V(i-1,j-1) << ", " <<q.V(i,j-1) << ", " << q.V(i+1,j-1) << " ],"
          //        << "[ " << q.V(i-1,j-0) << ", " <<q.V(i,j-0) << ", " << q.V(i+1,j-0) << " ],"
          //        << "[ " << q.V(i-1,j+1) << ", " <<q.V(i,j+1) << ", " << q.V(i+1,j+1) << " ]]" << endl<<endl;
          // }
        }

      if ( err / ( nbhoriz - 2 ) / ( nbvertic - 2 ) < 1e-6 )
        break;
    }
    //cout << " ERR " << err / ( nbhoriz - 2 ) / ( nbvertic - 2 ) << endl;

    return true;
  }

  //================================================================================
  /*!
   * \brief Remove temporary node
   */
  //================================================================================

  void mergeNodes( SMESH_MesherHelper& theHelper,
                   SinuousFace&        theSinuFace )
  {
    SMESH_MeshEditor editor( theHelper.GetMesh() );
    SMESH_MeshEditor::TListOfListOfNodes nodesGroups;

    TMergeMap::iterator n2nn = theSinuFace._nodesToMerge.begin();
    for ( ; n2nn != theSinuFace._nodesToMerge.end(); ++n2nn )
    {
      if ( !n2nn->first ) continue;
      nodesGroups.push_back( list< const SMDS_MeshNode* >() );
      list< const SMDS_MeshNode* > & group = nodesGroups.back();

      group.push_back( n2nn->first );
      group.splice( group.end(), n2nn->second );
    }
    editor.MergeNodes( nodesGroups );
  }

} // namespace

//================================================================================
/*!
 * \brief Sets event listener which removes mesh from EDGEs when 2D hyps change
 */
//================================================================================

void StdMeshers_QuadFromMedialAxis_1D2D::SetEventListener(SMESH_subMesh* faceSubMesh)
{
  faceSubMesh->SetEventListener( new EdgeCleaner, 0, faceSubMesh );
}

//================================================================================
/*!
 * \brief Create quadrangle elements
 *  \param [in] theHelper - the helper
 *  \param [in] theFace - the face to mesh
 *  \param [in] theSinuEdges - the sinuous EDGEs
 *  \param [in] theShortEdges - the short EDGEs
 *  \return bool - is OK or not
 */
//================================================================================

bool StdMeshers_QuadFromMedialAxis_1D2D::computeQuads( SMESH_MesherHelper& theHelper,
                                                       FaceQuadStruct::Ptr theQuad)
{
  StdMeshers_Quadrangle_2D::myHelper     = &theHelper;
  StdMeshers_Quadrangle_2D::myNeedSmooth = false;
  StdMeshers_Quadrangle_2D::myCheckOri   = false;
  StdMeshers_Quadrangle_2D::myQuadList.clear();

  int nbNodesShort0 = theQuad->side[0].NbPoints();
  int nbNodesShort1 = theQuad->side[2].NbPoints();

  // compute UV of internal points
  myQuadList.push_back( theQuad );
  if ( !StdMeshers_Quadrangle_2D::setNormalizedGrid( theQuad ))
    return false;

  // elliptic smooth of internal points to get boundary cell normal to the boundary
  bool isRing = theQuad->side[0].grid->Edge(0).IsNull();
  if ( !isRing )
    ellipticSmooth( theQuad, 1 );

  // create quadrangles
  bool ok;
  theHelper.SetElementsOnShape( true );
  if ( nbNodesShort0 == nbNodesShort1 )
    ok = StdMeshers_Quadrangle_2D::computeQuadDominant( *theHelper.GetMesh(),
                                                        theQuad->face, theQuad );
  else
    ok = StdMeshers_Quadrangle_2D::computeTriangles( *theHelper.GetMesh(),
                                                     theQuad->face, theQuad );

  StdMeshers_Quadrangle_2D::myHelper = 0;

  return ok;
}

//================================================================================
/*!
 * \brief Generate quadrangle mesh
 */
//================================================================================

bool StdMeshers_QuadFromMedialAxis_1D2D::Compute(SMESH_Mesh&         theMesh,
                                                 const TopoDS_Shape& theShape)
{
  SMESH_MesherHelper helper( theMesh );
  helper.SetSubShape( theShape );

  TopoDS_Face F = TopoDS::Face( theShape );
  if ( F.Orientation() >= TopAbs_INTERNAL ) F.Orientation( TopAbs_FORWARD );

  SinuousFace sinuFace( F );

  _progress = 0.01;

  if ( getSinuousEdges( helper, sinuFace ))
  {
    _progress = 0.4;

    double minSegLen = getMinSegLen( helper, sinuFace._sinuEdges );
    SMESH_MAT2d::MedialAxis ma( F, sinuFace._sinuEdges, minSegLen, /*ignoreCorners=*/true );

    if ( !_regular1D )
      _regular1D = new Algo1D( _studyId, _gen );
    _regular1D->SetSegmentLength( minSegLen );

    vector<double> maParams;
    if ( ! divideMA( helper, ma, sinuFace, _regular1D, minSegLen, maParams ))
      return error(COMPERR_BAD_SHAPE);

    _progress = 0.8;
    if ( _hyp2D )
      _regular1D->SetRadialDistribution( _hyp2D );

    if ( !computeShortEdges( helper, sinuFace._shortSide[0], _regular1D, _hyp2D, 0 ) ||
         !computeShortEdges( helper, sinuFace._shortSide[1], _regular1D, _hyp2D, 1 ))
      return error("Failed to mesh short edges");

    _progress = 0.85;

    if ( !computeSinuEdges( helper, minSegLen, ma, maParams, sinuFace, _regular1D ))
      return error("Failed to mesh sinuous edges");

    _progress = 0.9;

    bool ok = computeQuads( helper, sinuFace._quad );

    if ( ok )
      mergeNodes( helper, sinuFace );

    _progress = 1.;

    return ok;
  }

  return error(COMPERR_BAD_SHAPE, "Not implemented so far");
}

//================================================================================
/*!
 * \brief Predict nb of elements
 */
//================================================================================

bool StdMeshers_QuadFromMedialAxis_1D2D::Evaluate(SMESH_Mesh &         theMesh,
                                                  const TopoDS_Shape & theShape,
                                                  MapShapeNbElems&     theResMap)
{
  return StdMeshers_Quadrangle_2D::Evaluate(theMesh,theShape,theResMap);
}

//================================================================================
/*!
 * \brief Return true if the algorithm can mesh this shape
 *  \param [in] aShape - shape to check
 *  \param [in] toCheckAll - if true, this check returns OK if all shapes are OK,
 *              else, returns OK if at least one shape is OK
 */
//================================================================================

bool StdMeshers_QuadFromMedialAxis_1D2D::IsApplicable( const TopoDS_Shape & aShape,
                                                       bool                 toCheckAll )
{
  TmpMesh tmpMesh;
  SMESH_MesherHelper helper( tmpMesh );

  int nbFoundFaces = 0;
  for (TopExp_Explorer exp( aShape, TopAbs_FACE ); exp.More(); exp.Next(), ++nbFoundFaces )
  {
    const TopoDS_Face& face = TopoDS::Face( exp.Current() );
    SinuousFace sinuFace( face );
    bool isApplicable = getSinuousEdges( helper, sinuFace );

    if ( toCheckAll  && !isApplicable ) return false;
    if ( !toCheckAll &&  isApplicable ) return true;
  }
  return ( toCheckAll && nbFoundFaces != 0 );
}

