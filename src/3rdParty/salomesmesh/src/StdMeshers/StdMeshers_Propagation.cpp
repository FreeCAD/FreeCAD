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
//  File   : StdMeshers_Propagation.cxx
//  Module : SMESH
//
#include "StdMeshers_Propagation.hxx"

#include "utilities.h"

#include "SMDS_SetIterator.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_subMesh.hxx"

#include <BRepTools_WireExplorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>

#define DBGMSG(txt) \
  //  cout << txt << endl;

using namespace std;

namespace {

  // =======================================================================
  /*!
   * \brief Listener managing propagation of 1D hypotheses
   */
  // =======================================================================

  class PropagationMgr: public SMESH_subMeshEventListener
  {
  public:
    static PropagationMgr* GetListener();
    /*!
     * \brief Set listener on edge submesh
     */
    static void Set(SMESH_subMesh * submesh);
    /*!
     * \brief Return an edge from which hypotheses are propagated from
     */
    static TopoDS_Edge GetSource(SMESH_subMesh * submesh,
                                 bool&           isPropagOfDistribution);
    /*!
     * \brief Does it's main job
     */
    void ProcessEvent(const int          event,
                      const int          eventType,
                      SMESH_subMesh*     subMesh,
                      SMESH_subMeshEventListenerData* data,
                      const SMESH_Hypothesis*         hyp = 0);
  private:
    PropagationMgr();
  };
}

//=============================================================================
/*!
 * StdMeshers_Propagation Implementation
 */
//=============================================================================

StdMeshers_Propagation::StdMeshers_Propagation (int hypId, int studyId, SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = GetName();
  _param_algo_dim = -1; // 1D auxiliary
}
StdMeshers_PropagOfDistribution::StdMeshers_PropagOfDistribution (int hypId,
                                                                  int studyId,
                                                                  SMESH_Gen * gen)
  : StdMeshers_Propagation(hypId, studyId, gen)                        { _name = GetName(); }
StdMeshers_Propagation::~StdMeshers_Propagation()                      {}
string StdMeshers_Propagation::GetName ()                              { return "Propagation"; }
string StdMeshers_PropagOfDistribution::GetName ()                     { return "PropagOfDistribution"; }
ostream & StdMeshers_Propagation::SaveTo (ostream & save)              { return save; }
istream & StdMeshers_Propagation::LoadFrom (istream & load)            { return load; }
bool StdMeshers_Propagation::SetParametersByMesh(const SMESH_Mesh*,
                                                 const TopoDS_Shape& ) { return false; }
bool StdMeshers_Propagation::SetParametersByDefaults(const TDefaults&,const SMESH_Mesh*) { return false; }
void StdMeshers_Propagation::SetPropagationMgr(SMESH_subMesh* subMesh) { PropagationMgr::Set( subMesh ); }
/*!
 * \brief Return an edge from which hypotheses are propagated
 */
TopoDS_Edge StdMeshers_Propagation::GetPropagationSource(SMESH_Mesh&         theMesh,
                                                         const TopoDS_Shape& theEdge,
                                                         bool&               isPropagOfDistribution)
{
  return PropagationMgr::GetSource( theMesh.GetSubMeshContaining( theEdge ),
                                    isPropagOfDistribution);
}
const SMESH_HypoFilter& StdMeshers_Propagation::GetFilter()
{
  static SMESH_HypoFilter propagHypFilter;
  if ( propagHypFilter.IsEmpty() )
  {
    propagHypFilter.
      Init( SMESH_HypoFilter::HasName( StdMeshers_Propagation::GetName ())).
      Or  ( SMESH_HypoFilter::HasName( StdMeshers_PropagOfDistribution::GetName ()));
  }
  return propagHypFilter;
}
//=============================================================================
//=============================================================================
// PROPAGATION MANAGEMENT
//=============================================================================
//=============================================================================

namespace {

  enum SubMeshState { WAIT_PROPAG_HYP, // propagation hyp or local 1D hyp is missing
                      HAS_PROPAG_HYP,  // propag hyp on this submesh
                      IN_CHAIN,        // submesh is in propagation chain
                      LAST_IN_CHAIN,   // submesh with local 1D hyp breaking a chain
                      MEANINGLESS_LAST };          // meaningless

  struct PropagationMgrData : public EventListenerData
  {
    bool myForward; //!< true if a curve of edge in chain is codirected with one of source edge
    bool myIsPropagOfDistribution; //!< type of Propagation hyp
    PropagationMgrData( SubMeshState state=WAIT_PROPAG_HYP ): EventListenerData(true) {
      myType = state; myForward = true; myIsPropagOfDistribution = false;
    }
    void Init() {
      myType = WAIT_PROPAG_HYP;  mySubMeshes.clear(); myForward = true;
      myIsPropagOfDistribution = false;
    }
    SubMeshState State() const {
      return (SubMeshState) myType;
    }
    void SetState(SubMeshState state) {
      myType = state;
    }
    void SetSource(SMESH_subMesh* sm ) {
      mySubMeshes.clear(); if ( sm ) mySubMeshes.push_back( sm );
    }
    void AddSource(SMESH_subMesh* sm ) {
      if ( sm ) mySubMeshes.push_back( sm );
    }
    void SetChain(list< SMESH_subMesh* >& chain ) {
      mySubMeshes.clear(); mySubMeshes.splice( mySubMeshes.end(), chain );
    }
    SMESH_subMeshIteratorPtr GetChain() const;
    SMESH_subMesh* GetSource() const;
  };

  //=============================================================================
  /*!
   * \brief return static PropagationMgr
   */
  PropagationMgr* PropagationMgr::GetListener()
  {
    static PropagationMgr theListener;
    return &theListener;
  }
  PropagationMgr* getListener()
  {
    return PropagationMgr::GetListener();
  }
  //=============================================================================
  /*!
   * \brief return PropagationMgrData found on a submesh
   */
  PropagationMgrData* findData(SMESH_subMesh* sm)
  {
    if ( sm )
      return static_cast< PropagationMgrData* >( sm->GetEventListenerData( getListener() ));
    return 0;
  }
  //=============================================================================
  /*!
   * \brief return PropagationMgrData found on theEdge submesh
   */
  inline PropagationMgrData* findData(SMESH_Mesh& theMesh, const TopoDS_Shape& theEdge)
  {
    if ( theEdge.ShapeType() == TopAbs_EDGE )
      return findData( theMesh.GetSubMeshContaining( theEdge ) );
    return 0;
  }
  //=============================================================================
  /*!
   * \brief return existing or a new PropagationMgrData
   */
  PropagationMgrData* getData(SMESH_subMesh* sm)
  {
    PropagationMgrData* data = findData( sm );
    if ( !data && sm ) {
      data = new PropagationMgrData();
      sm->SetEventListener( getListener(), data, sm );
    }
    return data;
  }
  //=============================================================================
  /*!
   * \brief Returns a local 1D hypothesis used for theEdge
   */
  const SMESH_Hypothesis* getLocal1DHyp (SMESH_subMesh*      theSubMesh,
                                         //const TopoDS_Shape& theEdge,
                                         TopoDS_Shape*       theSssignedTo=0)
  {
    static SMESH_HypoFilter hypo;
    hypo.Init( hypo.HasDim( 1 )).
      AndNot ( hypo.IsAlgo() ).
      AndNot ( hypo.HasName( StdMeshers_Propagation::GetName() )).
      AndNot ( hypo.HasName( StdMeshers_PropagOfDistribution::GetName() )).
      AndNot ( hypo.IsAssignedTo( theSubMesh->GetFather()->GetShapeToMesh() ));

    return theSubMesh->GetFather()->GetHypothesis( theSubMesh, hypo, true, theSssignedTo );
  }
  //=============================================================================
  /*!
   * \brief Returns a propagation hypothesis assigned to theEdge
   */
  const SMESH_Hypothesis* getProagationHyp (SMESH_subMesh* theSubMesh)
  {
    return theSubMesh->GetFather()->GetHypothesis
      ( theSubMesh, StdMeshers_Propagation::GetFilter(), true );
  }
  //================================================================================
  /*!
   * \brief Return an iterator on a list of submeshes
   */
  SMESH_subMeshIteratorPtr iterate( list<SMESH_subMesh*>::const_iterator from,
                                    list<SMESH_subMesh*>::const_iterator to)
  {
    typedef SMESH_subMesh* TsubMesh;
    typedef SMDS_SetIterator< TsubMesh, list< TsubMesh >::const_iterator > TIterator;
    return SMESH_subMeshIteratorPtr ( new TIterator( from, to ));
  }
  //================================================================================
  /*!
   * \brief Build propagation chain
    * \param theMainSubMesh - the submesh with Propagation hypothesis
   */
  bool buildPropagationChain ( SMESH_subMesh* theMainSubMesh )
  {
    DBGMSG( "buildPropagationChain from " << theMainSubMesh->GetId() );
    const TopoDS_Shape& theMainEdge = theMainSubMesh->GetSubShape();
    if (theMainEdge.ShapeType() != TopAbs_EDGE) return true;

    SMESH_Mesh* mesh = theMainSubMesh->GetFather();

    TopoDS_Shape shapeOfHyp1D; // shape to which an hyp being propagated is assigned
    const SMESH_Hypothesis* hyp1D = getLocal1DHyp( theMainSubMesh, &shapeOfHyp1D );
    SMESH_HypoFilter moreLocalCheck( SMESH_HypoFilter::IsMoreLocalThan( shapeOfHyp1D, *mesh ));

    PropagationMgrData* chainData = getData( theMainSubMesh );
    chainData->SetState( HAS_PROPAG_HYP );

    if ( const SMESH_Hypothesis * propagHyp = getProagationHyp( theMainSubMesh ))
      chainData->myIsPropagOfDistribution =
        ( StdMeshers_PropagOfDistribution::GetName() == propagHyp->GetName() );

    // Edge submeshes, to which the 1D hypothesis will be propagated from theMainEdge
    list<SMESH_subMesh*> & chain = chainData->mySubMeshes;
    chain.clear();
    chain.push_back( theMainSubMesh );

    TopTools_MapOfShape checkedShapes;
    checkedShapes.Add( theMainEdge );

    vector<TopoDS_Edge> edges;

    list<SMESH_subMesh*>::iterator smIt = chain.begin();
    for ( ; smIt != chain.end(); ++smIt )
    {
      const TopoDS_Edge& anE = TopoDS::Edge( (*smIt)->GetSubShape() );
      PropagationMgrData* data = findData( *smIt );
      if ( !data ) continue;

      // Iterate on faces, having edge <anE>
      TopTools_ListIteratorOfListOfShape itA (mesh->GetAncestors(anE));
      for (; itA.More(); itA.Next())
      {
        // there are objects of different type among the ancestors of edge
        if ( itA.Value().ShapeType() != TopAbs_WIRE /*|| !checkedShapes.Add( itA.Value() )*/)
          continue;

        // Get ordered edges and find index of anE in a sequence
        edges.clear();
        BRepTools_WireExplorer aWE (TopoDS::Wire(itA.Value()));
        int edgeIndex = 0;
        for (; aWE.More(); aWE.Next()) {
          TopoDS_Edge edge = aWE.Current();
          edge.Orientation( aWE.Orientation() );
          if ( edge.IsSame( anE ))
            edgeIndex = edges.size();
          edges.push_back( edge );
        }

        // Find an edge opposite to anE
        TopoDS_Edge anOppE;
        if ( edges.size() < 4 ) {
          continue; // too few edges
        }
        else if ( edges.size() == 4 ) {
          int oppIndex = ( edgeIndex + 2 ) % 4;
          anOppE = edges[ oppIndex ];
        }
        else {
          // count nb sides
          TopoDS_Edge prevEdge = anE;
          int nbSide = 0, eIndex = edgeIndex + 1;
          for ( int i = 0; i < edges.size(); ++i, ++eIndex )
          {
            if ( eIndex == edges.size() )
              eIndex = 0;
            if ( !SMESH_Algo::IsContinuous( prevEdge, edges[ eIndex ])) {
              nbSide++;
            }
            else {
              // check that anE is not a part of a composite side
              if ( anE.IsSame( prevEdge ) || anE.IsSame( edges[ eIndex ])) {
                anOppE.Nullify(); break;
              }
            }
            if ( nbSide == 2 ) { // opposite side
              if ( !anOppE.IsNull() ) {
                // composite opposite side -> stop propagation
                anOppE.Nullify(); break;
              }
              anOppE = edges[ eIndex ];
            }
            if ( nbSide == 5 ) {
              anOppE.Nullify(); break; // too many sides
            }
            prevEdge = edges[ eIndex ];
          }
          if ( anOppE.IsNull() )
            continue;
          if ( nbSide != 4 ) {
            DBGMSG( nbSide << " sides in wire #" << mesh->GetMeshDS()->ShapeToIndex( itA.Value() ) << " - SKIP" );
            continue;
          }
        }
        if ( anOppE.IsNull() || !checkedShapes.Add( anOppE ))
          continue;
        SMESH_subMesh* oppSM = mesh->GetSubMesh( anOppE );
        PropagationMgrData* oppData = getData( oppSM );

        // Add anOppE to aChain if ...
        if ( oppData->State() == WAIT_PROPAG_HYP ) // ... anOppE is not in any chain
        {
          oppData->SetSource( theMainSubMesh );
          if ( ! (hyp1D = getLocal1DHyp( oppSM, &shapeOfHyp1D )) || //...no 1d hyp on anOppE
               ! (moreLocalCheck.IsOk( hyp1D, shapeOfHyp1D ))) // ... or hyp1D is "more global"
          {
            oppData->myForward = data->myForward;
            if ( edges[ edgeIndex ].Orientation() == anOppE.Orientation() )
              oppData->myForward = !oppData->myForward;
            chain.push_back( oppSM );
            oppSM->ComputeStateEngine( SMESH_subMesh::CLEAN );
            oppData->SetState( IN_CHAIN );
            DBGMSG( "set IN_CHAIN on " << oppSM->GetId() );
            if ( oppSM->GetAlgoState() != SMESH_subMesh::HYP_OK )
              // make oppSM check algo state
              if ( SMESH_Algo* algo = oppSM->GetAlgo() )
                oppSM->AlgoStateEngine(SMESH_subMesh::ADD_FATHER_ALGO, algo);
          }
          else {
            oppData->SetState( LAST_IN_CHAIN );
            DBGMSG( "set LAST_IN_CHAIN on " << oppSM->GetId() );
          }
        }
        else if ( oppData->State() == LAST_IN_CHAIN ) // anOppE breaks other chain
        {
          DBGMSG( "encounters LAST_IN_CHAIN on " << oppSM->GetId() );
          oppData->AddSource( theMainSubMesh );
        }
      } // loop on face ancestors
    } // loop on the chain

    // theMainSubMesh must not be in a chain
    chain.pop_front();

    return true;
  }
  //================================================================================
  /*!
   * \brief Clear propagation chain
   */
  bool clearPropagationChain( SMESH_subMesh* subMesh )
  {
    DBGMSG( "clearPropagationChain from " << subMesh->GetId() );
    if ( PropagationMgrData* data = findData( subMesh ))
    {
      switch ( data->State() ) {
      case IN_CHAIN:
        return clearPropagationChain( data->GetSource() );

      case HAS_PROPAG_HYP: {
        SMESH_subMeshIteratorPtr smIt = data->GetChain();
        while ( smIt->more() ) {
          SMESH_subMesh* sm = smIt->next();
          getData( sm )->Init();
          sm->ComputeStateEngine( SMESH_subMesh::CLEAN );
        }
        data->Init();
        break;
      }
      case LAST_IN_CHAIN: {
        SMESH_subMeshIteratorPtr smIt = iterate( data->mySubMeshes.begin(),
                                                 data->mySubMeshes.end());
        while ( smIt->more() )
          clearPropagationChain( smIt->next() );
        data->Init();
        break;
      }
      default:;
      }
      return true;
    }
    return false;
  }


  //================================================================================
  /*!
   * \brief Return an iterator on chain submeshes
   */
  //================================================================================

  SMESH_subMeshIteratorPtr PropagationMgrData::GetChain() const
  {
    switch ( State() ) {
    case HAS_PROPAG_HYP:
      return iterate( mySubMeshes.begin(), mySubMeshes.end() );
    case IN_CHAIN:
      if ( mySubMeshes.empty() ) break;
      return getData( mySubMeshes.front() )->GetChain();
    default:;
    }
    return iterate( mySubMeshes.end(), mySubMeshes.end() );
  }
  //================================================================================
  /*!
   * \brief Return a propagation source submesh
   */
  //================================================================================

  SMESH_subMesh* PropagationMgrData::GetSource() const
  {
    if ( myType == IN_CHAIN )
      if ( !mySubMeshes.empty() ) 
        return mySubMeshes.front();
    return 0;
  }


  //================================================================================
  /*!
   * \brief Constructor
   */
  //================================================================================

  PropagationMgr::PropagationMgr()
    : SMESH_subMeshEventListener( false, // won't be deleted by submesh
                                  "StdMeshers_Propagation::PropagationMgr")
  {}
  //================================================================================
  /*!
   * \brief Set PropagationMgr on a submesh
   */
  //================================================================================

  void PropagationMgr::Set(SMESH_subMesh * submesh)
  {
    if ( findData( submesh )) return;
    DBGMSG( "PropagationMgr::Set() on  " << submesh->GetId() );
    PropagationMgrData* data = new PropagationMgrData();
    submesh->SetEventListener( getListener(), data, submesh );

    const SMESH_Hypothesis * propagHyp =
      getProagationHyp( submesh );
    if ( propagHyp )
    {
      data->myIsPropagOfDistribution =
        ( StdMeshers_PropagOfDistribution::GetName() == propagHyp->GetName() );
      getListener()->ProcessEvent( SMESH_subMesh::ADD_HYP,
                                   SMESH_subMesh::ALGO_EVENT,
                                   submesh,
                                   data,
                                   propagHyp);
    }
  }
  //================================================================================
  /*!
   * \brief Return an edge from which hypotheses are propagated
   */
  //================================================================================

  TopoDS_Edge PropagationMgr::GetSource(SMESH_subMesh * submesh,
                                        bool&           isPropagOfDistribution)
  {
    if ( PropagationMgrData* data = findData( submesh )) {
      if ( data->State() == IN_CHAIN ) {
        if ( SMESH_subMesh* sm = data->GetSource() )
        {
          TopoDS_Shape edge = sm->GetSubShape();
          edge = edge.Oriented( data->myForward ? TopAbs_FORWARD : TopAbs_REVERSED );
          DBGMSG( " GetSource() = edge " << sm->GetId() << " REV = " << (!data->myForward));
          isPropagOfDistribution = false;
          if ( PropagationMgrData* data = findData( sm ))
            isPropagOfDistribution = data->myIsPropagOfDistribution;
          if ( edge.ShapeType() == TopAbs_EDGE )
            return TopoDS::Edge( edge );
        }
      }
    }
    return TopoDS_Edge();
  }
  //================================================================================
  /*!
   * \brief React on events on 1D submeshes
   */
  //================================================================================

  void PropagationMgr::ProcessEvent(const int                       event,
                                    const int                       eventType,
                                    SMESH_subMesh*                  subMesh,
                                    SMESH_subMeshEventListenerData* listenerData,
                                    const SMESH_Hypothesis*         hyp)
  {
    if ( !listenerData )
      return;
    if ( !hyp || hyp->GetType() != SMESHDS_Hypothesis::PARAM_ALGO || hyp->GetDim() != 1 )
      return;
    if ( eventType != SMESH_subMesh::ALGO_EVENT )
      return;
    DBGMSG( "PropagationMgr::ProcessEvent() on  " << subMesh->GetId() );

    bool isPropagHyp = ( StdMeshers_Propagation::GetName()          == hyp->GetName() ||
                         StdMeshers_PropagOfDistribution::GetName() == hyp->GetName() );

    PropagationMgrData* data = static_cast<PropagationMgrData*>( listenerData );
    switch ( data->State() ) {

    case WAIT_PROPAG_HYP: { // propagation hyp or local 1D hyp is missing
      // --------------------------------------------------------
      bool hasPropagHyp = ( isPropagHyp || getProagationHyp( subMesh ));
      if ( !hasPropagHyp )
        return;
      bool hasLocal1DHyp =  getLocal1DHyp( subMesh );
      if ( !hasLocal1DHyp )
        return;
      if ( event == SMESH_subMesh::ADD_HYP ||
           event == SMESH_subMesh::ADD_FATHER_HYP ) // add local or propagation hyp
      {
        DBGMSG( "ADD_HYP propagation to WAIT_PROPAG_HYP " << subMesh->GetId() );
        // build propagation chain
        buildPropagationChain( subMesh );
      }
      return;
    }
    case HAS_PROPAG_HYP: {  // propag hyp on this submesh
      // --------------------------------------------------------
      switch ( event ) {
      case SMESH_subMesh::REMOVE_HYP:
      case SMESH_subMesh::REMOVE_FATHER_HYP: // remove propagation hyp
        if ( isPropagHyp && !getProagationHyp( subMesh ))
        {
          DBGMSG( "REMOVE_HYP propagation from HAS_PROPAG_HYP " << subMesh->GetId() );
          // clear propagation chain
          clearPropagationChain( subMesh );
        }
        // return; -- hyp is modified any way
        /* FALLTHRU */
      default:
        //case SMESH_subMesh::MODIF_HYP: // hyp modif
        // clear mesh in a chain
        DBGMSG( "MODIF_HYP on HAS_PROPAG_HYP " << subMesh->GetId() );
        SMESH_subMeshIteratorPtr smIt = data->GetChain();
        while ( smIt->more() ) {
          SMESH_subMesh* smInChain = smIt->next();
          smInChain->AlgoStateEngine( SMESH_subMesh::MODIF_HYP,
                                      (SMESH_Hypothesis*) hyp );
        }
        return;
      }
      return;
    }
    case IN_CHAIN: {       // submesh is in propagation chain
      // --------------------------------------------------------
      if ( event == SMESH_subMesh::ADD_HYP ) { // add local hypothesis
        if ( isPropagHyp ) { // propagation hyp added
          DBGMSG( "ADD_HYP propagation on IN_CHAIN " << subMesh->GetId() );
          // collision - do nothing
        }
        else { // 1D hyp added
          // rebuild propagation chain
          DBGMSG( "ADD_HYP 1D on IN_CHAIN " << subMesh->GetId() );
          SMESH_subMesh* sourceSM = data->GetSource();
          clearPropagationChain( sourceSM );
          buildPropagationChain( sourceSM );
        }
      }
      return;
    }
    case LAST_IN_CHAIN: { // submesh with local 1D hyp, breaking a chain
      // --------------------------------------------------------
      if ( event == SMESH_subMesh::REMOVE_HYP ) { // remove local hyp
        // rebuild propagation chain
        DBGMSG( "REMOVE_HYP 1D from LAST_IN_CHAIN " << subMesh->GetId() );
        list<SMESH_subMesh*> sourceSM = data->mySubMeshes;
        clearPropagationChain( subMesh );
        SMESH_subMeshIteratorPtr smIt = iterate( sourceSM.begin(), sourceSM.end());
        while ( smIt->more() )
          buildPropagationChain( smIt->next() );
      }
      return;
    }
    } // switch by SubMeshState
  }

} // namespace
