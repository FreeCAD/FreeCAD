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
//  File   : StdMeshers_CompositeSegment_1D.cxx
//  Module : SMESH
//
#include "StdMeshers_CompositeSegment_1D.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_AutomaticLength.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "SMESH_Comment.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"

#include "utilities.h"

#include <BRepAdaptor_CompCurve.hxx>
#include <BRep_Builder.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Pnt.hxx>

#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>

typedef SMESH_Comment TComm;

using namespace std;


namespace {

  void careOfSubMeshes( StdMeshers_FaceSide& side );

  //================================================================================
  /*!
   * \brief Search for an edge conjunct to the given one by the vertex
   *        Return NULL if more than 2 edges share the vertex or edges
   *        continuity is less than C1
   */
  //================================================================================

  TopoDS_Edge nextC1Edge(TopoDS_Edge  edge,
                         SMESH_Mesh & aMesh,
                         const bool   forward)
  {
    if (edge.Orientation() > TopAbs_REVERSED) // INTERNAL
      edge.Orientation( TopAbs_FORWARD );
    TopoDS_Edge eNext;
    TopTools_MapOfShape edgeCounter;
    edgeCounter.Add( edge );
    TopoDS_Vertex v = forward ? TopExp::LastVertex(edge,true) : TopExp::FirstVertex(edge,true);
    TopTools_ListIteratorOfListOfShape ancestIt = aMesh.GetAncestors( v );
    for ( ; ancestIt.More(); ancestIt.Next() )
    {
      const TopoDS_Shape & ancestor = ancestIt.Value();
      if ( ancestor.ShapeType() == TopAbs_EDGE && edgeCounter.Add( ancestor ))
        eNext = TopoDS::Edge( ancestor );
    }
    if ( edgeCounter.Extent() < 3 && !eNext.IsNull() ) {
      if ( SMESH_Algo::IsContinuous( edge, eNext )) {
        // care of orientation
        if (eNext.Orientation() > TopAbs_REVERSED) // INTERNAL
          eNext.Orientation( TopAbs_FORWARD );
        TopoDS_Vertex vn =
          forward ? TopExp::FirstVertex(eNext,true) : TopExp::LastVertex(eNext,true);
        bool reverse = (!v.IsSame(vn));
        if ( reverse )
          eNext.Reverse();
        return eNext;
      }
    }
    return TopoDS_Edge();
  }

  //================================================================================
  /*!
   * \brief Class used to restore nodes on internal vertices of a complex side
   *  when StdMeshers_CompositeSegment_1D algorithm is removed
   */
  //================================================================================

  struct VertexNodesRestoringListener : public SMESH_subMeshEventListener
  {
    VertexNodesRestoringListener():
      SMESH_subMeshEventListener(1, // will be deleted by sub-mesh
                                 "StdMeshers_CompositeSegment_1D::VertexNodesRestoringListener")
    {}
    static VertexNodesRestoringListener* New() { return new VertexNodesRestoringListener(); }

    /*!
     * \brief Restore nodes on internal vertices of a complex side
     * \param event - algo_event or compute_event itself (of SMESH_subMesh)
     * \param eventType - ALGO_EVENT or COMPUTE_EVENT (of SMESH_subMesh)
     * \param subMesh - the submesh where the event occurs
     * \param data - listener data stored in the subMesh
     * \param hyp - hypothesis, if eventType is algo_event
     */
    void ProcessEvent(const int          event,
                      const int          eventType,
                      SMESH_subMesh*     subMesh,
                      EventListenerData* data,
                      const SMESH_Hypothesis*  /*hyp*/)
    {
      if ( data && eventType == SMESH_subMesh::ALGO_EVENT )
      {
        bool hypRemoved;
        if ( subMesh->GetAlgoState() != SMESH_subMesh::HYP_OK )
          hypRemoved = true;
        else {
          SMESH_Algo* algo = subMesh->GetAlgo();
          hypRemoved = ( string( algo->GetName() ) != StdMeshers_CompositeSegment_1D::AlgoName());
        }
        if ( hypRemoved )
        {
          list<SMESH_subMesh*>::iterator smIt = data->mySubMeshes.begin();
          for ( ; smIt != data->mySubMeshes.end(); ++smIt )
            if ( SMESH_subMesh* sm = *smIt ) {
              sm->SetIsAlwaysComputed( false );
              sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
            }
        }
      }
      // at study restoration:
      // check if edge submesh must have _alwaysComputed flag
      else if ( event     == SMESH_subMesh::SUBMESH_RESTORED &&
                eventType == SMESH_subMesh::COMPUTE_EVENT )
      {
        if ( !subMesh->GetEventListenerData( this )) { // not yet checked
          SMESHDS_Mesh * meshDS = subMesh->GetFather()->GetMeshDS();
          if ( meshDS->NbNodes() > 0 ) {
            // check if there are nodes on all vertices
            bool hasNodesOnVerext = true;
            SMESH_subMeshIteratorPtr smIt = subMesh->getDependsOnIterator(false,false);
            while ( hasNodesOnVerext && smIt->more() ) {
              SMESH_subMesh* sm = smIt->next();
              hasNodesOnVerext = ( sm->GetSubMeshDS() && sm->GetSubMeshDS()->NbNodes() );
            }
            if ( !hasNodesOnVerext ) {
              // check if an edge is a part of a complex side
              TopoDS_Face face;
              TopoDS_Edge edge = TopoDS::Edge( subMesh->GetSubShape() );
              unique_ptr< StdMeshers_FaceSide > side
                ( StdMeshers_CompositeSegment_1D::GetFaceSide(*subMesh->GetFather(),
                                                              edge, face, false ));
              if ( side->NbEdges() > 1 && side->NbSegments() )
                careOfSubMeshes( *side );
            }
          }
        }
      }
      // clean all EDGEs of a complex side if one EDGE is cleaned
      else if ( event     == SMESH_subMesh::CLEAN &&
                eventType == SMESH_subMesh::COMPUTE_EVENT )
      {
        SMESH_subMeshIteratorPtr smIt = subMesh->getDependsOnIterator(/*includeSelf=*/false);
        while ( smIt->more() ) // loop on VERTEX sub-meshes
        {
          SMESH_subMesh* sm = smIt->next();
          if ( sm->IsAlwaysComputed() ) // it's an internal node sub-mesh
            sm->ComputeStateEngine( SMESH_subMesh::CLEAN );
        }
      }
    }
  }; // struct VertexNodesRestoringListener

  //================================================================================
  /*!
   * \brief Update submeshes state for all edges and internal vertices,
   * make them look computed even if none edge or node is set on them
   */
  //================================================================================

  void careOfSubMeshes( StdMeshers_FaceSide& side )
  {
    if ( side.NbEdges() < 2)
      return;
    for ( int iE = 0; iE < side.NbEdges(); ++iE )
    {
      // set listener and its data
      EventListenerData * listenerData = new EventListenerData(true);
      const TopoDS_Edge& edge = side.Edge( iE );
      SMESH_subMesh * sm = side.GetMesh()->GetSubMesh( edge );
      sm->SetEventListener( new VertexNodesRestoringListener(), listenerData, sm );
      // add edge submesh to the data
      sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
      if ( sm->GetComputeState() != SMESH_subMesh::COMPUTE_OK ) {
        sm->SetIsAlwaysComputed( true );
        listenerData->mySubMeshes.push_back( sm );
      }
      // add internal vertex submesh to the data
      if ( iE )
      {
        TopoDS_Vertex V = side.FirstVertex( iE );
        sm = side.GetMesh()->GetSubMesh( V );
        sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
        if ( sm->GetComputeState() != SMESH_subMesh::COMPUTE_OK )
          sm->SetIsAlwaysComputed( true );
        listenerData->mySubMeshes.push_back( sm );
      }
    }
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_CompositeSegment_1D::StdMeshers_CompositeSegment_1D(int         hypId,
                                                               int         studyId,
                                                               SMESH_Gen * gen)
  :StdMeshers_Regular_1D(hypId, studyId, gen)
{
  MESSAGE("StdMeshers_CompositeSegment_1D::StdMeshers_CompositeSegment_1D");
  _name = AlgoName();
}

//=======================================================================
//function : AlgoName
//purpose  : Returns algo type name
//=======================================================================

std::string StdMeshers_CompositeSegment_1D::AlgoName()
{
  return "CompositeSegment_1D";
}

//=============================================================================
/*!
 * \brief Sets event listener to submeshes if necessary
 * \param subMesh - submesh where algo is set
 *
 * This method is called when a submesh gets HYP_OK algo_state.
 * After being set, event listener is notified on each event of a submesh.
 */
//=============================================================================

void StdMeshers_CompositeSegment_1D::SetEventListener(SMESH_subMesh* subMesh)
{
  // issue 0020279. Set "_alwaysComputed" flag to the submeshes of internal
  // vertices of composite edge in order to avoid creation of vertices on
  // them for the sake of stability.

  // check if "_alwaysComputed" is not yet set
  bool isAlwaysComputed = false;
  SMESH_subMeshIteratorPtr smIt = subMesh->getDependsOnIterator(false,false);
  while ( !isAlwaysComputed && smIt->more() )
    isAlwaysComputed = smIt->next()->IsAlwaysComputed();

  if ( !isAlwaysComputed )
  {
    // check if an edge is a part of a complex side
    TopoDS_Face face;
    TopoDS_Edge edge = TopoDS::Edge( subMesh->GetSubShape() );
    unique_ptr< StdMeshers_FaceSide > side
      ( StdMeshers_CompositeSegment_1D::GetFaceSide(*subMesh->GetFather(),edge, face, false ));
    if ( side->NbEdges() > 1 ) { // complex

      // set _alwaysComputed to vertices
      for ( int iE = 1; iE < side->NbEdges(); ++iE )
      {
        TopoDS_Vertex V = side->FirstVertex( iE );
        SMESH_subMesh* sm = side->GetMesh()->GetSubMesh( V );
        sm->SetIsAlwaysComputed( true );
      }
    }
  }
  // set listener that will remove _alwaysComputed from submeshes at algorithm change
  subMesh->SetEventListener( new VertexNodesRestoringListener(), 0, subMesh);
  StdMeshers_Regular_1D::SetEventListener( subMesh );
}

//=============================================================================
/*!
 * \brief Return a face side the edge belongs to
 */
//=============================================================================

StdMeshers_FaceSide *
StdMeshers_CompositeSegment_1D::GetFaceSide(SMESH_Mesh&        aMesh,
                                            const TopoDS_Edge& anEdge,
                                            const TopoDS_Face& aFace,
                                            const bool         ignoreMeshed)
{
  list< TopoDS_Edge > edges;
  if ( anEdge.Orientation() <= TopAbs_REVERSED )
    edges.push_back( anEdge );
  else
    edges.push_back( TopoDS::Edge( anEdge.Oriented( TopAbs_FORWARD ))); // PAL21718

  list <const SMESHDS_Hypothesis *> hypList;
  SMESH_Algo* theAlgo = aMesh.GetGen()->GetAlgo( aMesh, anEdge );
  if ( theAlgo ) hypList = theAlgo->GetUsedHypothesis(aMesh, anEdge, false);
  for ( int forward = 0; forward < 2; ++forward )
  {
    TopoDS_Edge eNext = nextC1Edge( edges.back(), aMesh, forward );
    while ( !eNext.IsNull() ) {
      if ( ignoreMeshed ) {
        // eNext must not have computed mesh
        if ( SMESHDS_SubMesh* sm = aMesh.GetMeshDS()->MeshElements(eNext) )
          if ( sm->NbNodes() || sm->NbElements() )
            break;
      }
      // eNext must have same hypotheses
      SMESH_Algo* algo = aMesh.GetGen()->GetAlgo( aMesh, eNext );
      if ( !algo ||
           string(theAlgo->GetName()) != algo->GetName() ||
           hypList != algo->GetUsedHypothesis(aMesh, eNext, false))
        break;
      if ( std::find( edges.begin(), edges.end(), eNext ) != edges.end() )
        break;
      if ( forward )
        edges.push_back( eNext );
      else
        edges.push_front( eNext );
      eNext = nextC1Edge( eNext, aMesh, forward );
    }
  }
  return new StdMeshers_FaceSide( aFace, edges, &aMesh, true, false );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_CompositeSegment_1D::Compute(SMESH_Mesh &         aMesh,
                                             const TopoDS_Shape & aShape)
{
  TopoDS_Edge edge = TopoDS::Edge( aShape );
  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();

  // Get edges to be discretized as a whole
  TopoDS_Face nullFace;
  unique_ptr< StdMeshers_FaceSide > side( GetFaceSide(aMesh, edge, nullFace, true ));
  //side->dump("IN COMPOSITE SEG");

  if ( side->NbEdges() < 2 )
    return StdMeshers_Regular_1D::Compute( aMesh, aShape );

  // update segment lenght computed by StdMeshers_AutomaticLength
  const list <const SMESHDS_Hypothesis * > & hyps = GetUsedHypothesis(aMesh, aShape);
  if ( !hyps.empty() ) {
    StdMeshers_AutomaticLength * autoLenHyp = const_cast<StdMeshers_AutomaticLength *>
      (dynamic_cast <const StdMeshers_AutomaticLength * >(hyps.front()));
    if ( autoLenHyp )
      _value[ BEG_LENGTH_IND ]= autoLenHyp->GetLength( &aMesh, side->Length() );
  }

  // Compute node parameters
  unique_ptr< BRepAdaptor_CompCurve > C3d ( side->GetCurve3d() );
  double f = C3d->FirstParameter(), l = C3d->LastParameter();
  list< double > params;
  if ( !computeInternalParameters ( aMesh, *C3d, side->Length(), f, l, params, false ))
    return false;

  // Redistribute parameters near ends
  TopoDS_Vertex VFirst = side->FirstVertex();
  TopoDS_Vertex VLast  = side->LastVertex();
  redistributeNearVertices( aMesh, *C3d, side->Length(), params, VFirst, VLast );

  params.push_front(f);
  params.push_back(l);
  int nbNodes = params.size();

  // Create mesh

  // compute and get nodes on extremity VERTEX'es
  SMESH_subMesh* smVFirst = aMesh.GetSubMesh( VFirst );
  smVFirst->SetIsAlwaysComputed( false );
  smVFirst->ComputeStateEngine( SMESH_subMesh::COMPUTE );
  //
  SMESH_subMesh* smVLast = aMesh.GetSubMesh( VLast );
  smVLast->SetIsAlwaysComputed( false );
  smVLast->ComputeStateEngine( SMESH_subMesh::COMPUTE );
  //
  const SMDS_MeshNode * nFirst = SMESH_Algo::VertexNode( VFirst, meshDS );
  const SMDS_MeshNode * nLast  = SMESH_Algo::VertexNode( VLast, meshDS );
  if (!nFirst)
    return error(COMPERR_BAD_INPUT_MESH, TComm("No node on vertex ")
                 <<meshDS->ShapeToIndex(VFirst));
  if (!nLast)
    return error(COMPERR_BAD_INPUT_MESH, TComm("No node on vertex ")
                 <<meshDS->ShapeToIndex(VLast));

  vector<const SMDS_MeshNode*> nodes( nbNodes, (const SMDS_MeshNode*)0 );
  nodes.front() = nFirst;
  nodes.back()  = nLast;

  // create internal nodes
  list< double >::iterator parIt = params.begin();
  double prevPar = *parIt;
  Standard_Real u;
  for ( int iN = 0; parIt != params.end(); ++iN, ++parIt)
  {
    if ( !nodes[ iN ] ) {
      gp_Pnt p = C3d->Value( *parIt );
      SMDS_MeshNode* n = meshDS->AddNode( p.X(), p.Y(), p.Z());
      C3d->Edge( *parIt, edge, u );
      meshDS->SetNodeOnEdge( n, edge, u );
//       cout << "new NODE: par="<<*parIt<<" ePar="<<u<<" e="<<edge.TShape().operator->()
//            << " " << n << endl;
      nodes[ iN ] = n;
    }
    // create edges
    if ( iN ) {
      double mPar = ( prevPar + *parIt )/2;
      if ( _quadraticMesh ) {
        // create medium node
        double segLen = GCPnts_AbscissaPoint::Length(*C3d, prevPar, *parIt);
        GCPnts_AbscissaPoint ruler( *C3d, segLen/2., prevPar );
        if ( ruler.IsDone() )
          mPar = ruler.Parameter();
        gp_Pnt p = C3d->Value( mPar );
        SMDS_MeshNode* n = meshDS->AddNode( p.X(), p.Y(), p.Z());
        //cout << "new NODE "<< n << endl;
        meshDS->SetNodeOnEdge( n, edge, u );
        SMDS_MeshEdge * seg = meshDS->AddEdge(nodes[ iN-1 ], nodes[ iN ], n);
        meshDS->SetMeshElementOnShape(seg, edge);
      }
      else {
        C3d->Edge( mPar, edge, u );
        SMDS_MeshEdge * seg = meshDS->AddEdge(nodes[ iN-1 ], nodes[ iN ]);
        meshDS->SetMeshElementOnShape(seg, edge);
      }
    }
    prevPar = *parIt;
  }

  // remove nodes on internal vertices
  for ( int iE = 1; iE < side->NbEdges(); ++iE )
  {
    TopoDS_Vertex V = side->FirstVertex( iE );
    while ( const SMDS_MeshNode * n = SMESH_Algo::VertexNode( V, meshDS ))
      meshDS->RemoveNode( n );
  }

  // Update submeshes state for all edges and internal vertices,
  // make them look computed even if none edge or node is set on them
  careOfSubMeshes( *side );

  return true;
}
