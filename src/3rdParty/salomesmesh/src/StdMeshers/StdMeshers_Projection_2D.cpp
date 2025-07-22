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
// File      : StdMeshers_Projection_2D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_Projection_2D.hxx"

#include "StdMeshers_ProjectionSource2D.hxx"
#include "StdMeshers_ProjectionUtils.hxx"
#include "StdMeshers_FaceSide.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMDS_FacePosition.hxx"
#include "SMESHDS_Hypothesis.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_Pattern.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"

#include "utilities.h"

#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_B2d.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomLib_IsPlanarSurface.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_GTrsf.hxx>


using namespace std;

#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }

namespace TAssocTool = StdMeshers_ProjectionUtils;
//typedef StdMeshers_ProjectionUtils TAssocTool;

//=======================================================================
//function : StdMeshers_Projection_2D
//purpose  : 
//=======================================================================

StdMeshers_Projection_2D::StdMeshers_Projection_2D(int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_2D_Algo(hypId, studyId, gen)
{
  _name = "Projection_2D";
  _compatibleHypothesis.push_back("ProjectionSource2D");
  _sourceHypo = 0;
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_Projection_2D::~StdMeshers_Projection_2D()
{}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_Projection_2D::CheckHypothesis(SMESH_Mesh&                          theMesh,
                                               const TopoDS_Shape&                  theShape,
                                               SMESH_Hypothesis::Hypothesis_Status& theStatus)
{
  list <const SMESHDS_Hypothesis * >::const_iterator itl;

  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(theMesh, theShape);
  if ( hyps.size() == 0 )
  {
    theStatus = HYP_MISSING;
    return false;  // can't work with no hypothesis
  }

  if ( hyps.size() > 1 )
  {
    theStatus = HYP_ALREADY_EXIST;
    return false;
  }

  const SMESHDS_Hypothesis *theHyp = hyps.front();

  string hypName = theHyp->GetName();

  theStatus = HYP_OK;

  if (hypName == "ProjectionSource2D")
  {
    _sourceHypo = static_cast<const StdMeshers_ProjectionSource2D *>(theHyp);

    // Check hypo parameters

    SMESH_Mesh* srcMesh = _sourceHypo->GetSourceMesh();
    SMESH_Mesh* tgtMesh = & theMesh;
    if ( !srcMesh )
      srcMesh = tgtMesh;

    // check vertices
    if ( _sourceHypo->HasVertexAssociation() )
    {
      // source vertices
      TopoDS_Shape edge = TAssocTool::GetEdgeByVertices
        ( srcMesh, _sourceHypo->GetSourceVertex(1), _sourceHypo->GetSourceVertex(2) );
      if ( edge.IsNull() ||
           !SMESH_MesherHelper::IsSubShape( edge, srcMesh ) ||
           !SMESH_MesherHelper::IsSubShape( edge, _sourceHypo->GetSourceFace() ))
      {
        theStatus = HYP_BAD_PARAMETER;
        error("Invalid source vertices");
        SCRUTE((edge.IsNull()));
        SCRUTE((SMESH_MesherHelper::IsSubShape( edge, srcMesh )));
        SCRUTE((SMESH_MesherHelper::IsSubShape( edge, _sourceHypo->GetSourceFace() )));
      }
      else
      {
        // target vertices
        edge = TAssocTool::GetEdgeByVertices
          ( tgtMesh, _sourceHypo->GetTargetVertex(1), _sourceHypo->GetTargetVertex(2) );
        if ( edge.IsNull() || !SMESH_MesherHelper::IsSubShape( edge, tgtMesh ))
        {
          theStatus = HYP_BAD_PARAMETER;
          error("Invalid target vertices");
          SCRUTE((edge.IsNull()));
          SCRUTE((SMESH_MesherHelper::IsSubShape( edge, tgtMesh )));
        }
        // PAL16203
        else if ( !_sourceHypo->IsCompoundSource() &&
                  !SMESH_MesherHelper::IsSubShape( edge, theShape ))
        {
          theStatus = HYP_BAD_PARAMETER;
          error("Invalid target vertices");
          SCRUTE((SMESH_MesherHelper::IsSubShape( edge, theShape )));
        }
      }
    }
    // check a source face
    if ( !SMESH_MesherHelper::IsSubShape( _sourceHypo->GetSourceFace(), srcMesh ) ||
         ( srcMesh == tgtMesh && theShape == _sourceHypo->GetSourceFace() ))
    {
      theStatus = HYP_BAD_PARAMETER;
      error("Invalid source face");
      SCRUTE((SMESH_MesherHelper::IsSubShape( _sourceHypo->GetSourceFace(), srcMesh )));
      SCRUTE((srcMesh == tgtMesh));
      SCRUTE(( theShape == _sourceHypo->GetSourceFace() ));
    }
  }
  else
  {
    theStatus = HYP_INCOMPATIBLE;
  }
  return ( theStatus == HYP_OK );
}

namespace {

  //================================================================================
  /*!
   * \brief define if a node is new or old
   * \param node - node to check
   * \retval bool - true if the node existed before Compute() is called
   */
  //================================================================================

  bool isOldNode( const SMDS_MeshNode* node )
  {
    // old nodes are shared by edges and new ones are shared
    // only by faces created by mapper
    //if ( is1DComputed )
    {
      bool isOld = node->NbInverseElements(SMDSAbs_Edge) > 0;
      return isOld;
    }
    // else
    // {
    //   SMDS_ElemIteratorPtr invFace = node->GetInverseElementIterator(SMDSAbs_Face);
    //   bool isNew = invFace->more();
    //   return !isNew;
    // }
  }

  //================================================================================
  /*!
   * \brief Class to remove mesh built by pattern mapper on edges
   * and vertices in the case of failure of projection algo.
   * It does it's job at destruction
   */
  //================================================================================

  class MeshCleaner {
    SMESH_subMesh* sm;
  public:
    MeshCleaner( SMESH_subMesh* faceSubMesh ): sm(faceSubMesh) {}
    ~MeshCleaner() { Clean(sm); }
    void Release() { sm = 0; } // mesh will not be removed
    static void Clean( SMESH_subMesh* sm, bool withSub=true )
    {
      if ( !sm || !sm->GetSubMeshDS() ) return;
      // PAL16567, 18920. Remove face nodes as well
//       switch ( sm->GetSubShape().ShapeType() ) {
//       case TopAbs_VERTEX:
//       case TopAbs_EDGE: {
        SMDS_NodeIteratorPtr nIt = sm->GetSubMeshDS()->GetNodes();
        SMESHDS_Mesh* mesh = sm->GetFather()->GetMeshDS();
        while ( nIt->more() ) {
          const SMDS_MeshNode* node = nIt->next();
          if ( !isOldNode( node ) )
            mesh->RemoveNode( node );
        }
        // do not break but iterate over DependsOn()
//       }
//       default:
        if ( !withSub ) return;
        SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(false,false);
        while ( smIt->more() )
          Clean( smIt->next(), false );
//       }
    }
  };

  //================================================================================
  /*!
   * \brief find new nodes belonging to one free border of mesh on face
    * \param sm - submesh on edge or vertex containg nodes to choose from
    * \param face - the face bound by the submesh
    * \param u2nodes - map to fill with nodes
    * \param seamNodes - set of found nodes
    * \retval bool - is a success
   */
  //================================================================================

  bool getBoundaryNodes ( SMESH_subMesh*                        sm,
                          const TopoDS_Face&                    face,
                          map< double, const SMDS_MeshNode* > & u2nodes,
                          set< const SMDS_MeshNode* > &         seamNodes)
  {
    u2nodes.clear();
    seamNodes.clear();
    if ( !sm || !sm->GetSubMeshDS() )
      RETURN_BAD_RESULT("Null submesh");

    SMDS_NodeIteratorPtr nIt = sm->GetSubMeshDS()->GetNodes();
    switch ( sm->GetSubShape().ShapeType() ) {

    case TopAbs_VERTEX: {
      while ( nIt->more() ) {
        const SMDS_MeshNode* node = nIt->next();
        if ( isOldNode( node ) ) continue;
        u2nodes.insert( make_pair( 0., node ));
        seamNodes.insert( node );
        return true;
      }
      break;
    }
    case TopAbs_EDGE: {
      
      // Get submeshes of sub-vertices
      const map< int, SMESH_subMesh * >& subSM = sm->DependsOn();
      if ( subSM.size() != 2 )
        RETURN_BAD_RESULT("there must be 2 submeshes of sub-vertices"
                          " but we have " << subSM.size());
      SMESH_subMesh* smV1 = subSM.begin()->second;
      SMESH_subMesh* smV2 = subSM.rbegin()->second;
      if ( !smV1->IsMeshComputed() || !smV2->IsMeshComputed() )
        RETURN_BAD_RESULT("Empty vertex submeshes");

      const SMDS_MeshNode* nV1 = 0;
      const SMDS_MeshNode* nE = 0;

      // Look for nV1 - a new node on V1
      nIt = smV1->GetSubMeshDS()->GetNodes();
      while ( nIt->more() && !nE ) {
        const SMDS_MeshNode* node = nIt->next();
        if ( isOldNode( node ) ) continue;
        nV1 = node;

        // Find nE - a new node connected to nV1 and belonging to edge submesh;
        SMESHDS_SubMesh* smDS = sm->GetSubMeshDS();
        SMDS_ElemIteratorPtr vElems = nV1->GetInverseElementIterator(SMDSAbs_Face);
        while ( vElems->more() && !nE ) {
          const SMDS_MeshElement* elem = vElems->next();
          int nbNodes = elem->NbNodes();
          if ( elem->IsQuadratic() )
            nbNodes /= 2;
          int iV1 = elem->GetNodeIndex( nV1 );
          // try next after nV1
          int iE = SMESH_MesherHelper::WrapIndex( iV1 + 1, nbNodes );
          if ( smDS->Contains( elem->GetNode( iE ) ))
            nE = elem->GetNode( iE );
          if ( !nE ) {
            // try node before nV1
            iE = SMESH_MesherHelper::WrapIndex( iV1 - 1, nbNodes );
            if ( smDS->Contains( elem->GetNode( iE )))
              nE = elem->GetNode( iE );
          }
          if ( nE && elem->IsQuadratic() ) { // find medium node between nV1 and nE
            if ( Abs( iV1 - iE ) == 1 )
              nE = elem->GetNode( Min ( iV1, iE ) + nbNodes );
            else
              nE = elem->GetNode( elem->NbNodes() - 1 );
          }
        }
      }
      if ( !nV1 )
        RETURN_BAD_RESULT("No new node found on V1");
      if ( !nE )
        RETURN_BAD_RESULT("new node on edge not found");

      // Get the whole free border of a face
      list< const SMDS_MeshNode* > bordNodes;
      list< const SMDS_MeshElement* > bordFaces;
      if ( !SMESH_MeshEditor::FindFreeBorder (nV1, nE, nV1, bordNodes, bordFaces ))
        RETURN_BAD_RESULT("free border of a face not found by nodes " <<
                          nV1->GetID() << " " << nE->GetID() );

      // Insert nodes of the free border to the map until node on V2 encountered
      SMESHDS_SubMesh* v2smDS = smV2->GetSubMeshDS();
      list< const SMDS_MeshNode* >::iterator bordIt = bordNodes.begin();
      bordIt++; // skip nV1
      for ( ; bordIt != bordNodes.end(); ++bordIt ) {
        const SMDS_MeshNode* node = *bordIt;
        if ( v2smDS->Contains( node ))
          break;
        if ( node->GetPosition()->GetTypeOfPosition() != SMDS_TOP_EDGE )
          RETURN_BAD_RESULT("Bad node position type: node " << node->GetID() <<
                            " pos type " << node->GetPosition()->GetTypeOfPosition());
        const SMDS_EdgePosition* pos =
          static_cast<const SMDS_EdgePosition*>(node->GetPosition());
        u2nodes.insert( make_pair( pos->GetUParameter(), node ));
        seamNodes.insert( node );
      }
      if ( u2nodes.size() != seamNodes.size() )
        RETURN_BAD_RESULT("Bad node params on edge " << sm->GetId() <<
                          ", " << u2nodes.size() << " != " << seamNodes.size() );
      return true;
    }
    default:;
    }
    RETURN_BAD_RESULT ("Unexpected submesh type");

  } // bool getBoundaryNodes()

  //================================================================================
  /*!
   * \brief Check if two consecutive EDGEs are connected in 2D
   *  \param [in] E1 - a well oriented non-seam EDGE
   *  \param [in] E2 - a possibly well oriented seam EDGE
   *  \param [in] F - a FACE
   *  \return bool - result
   */
  //================================================================================

  bool are2dConnected( const TopoDS_Edge & E1,
                       const TopoDS_Edge & E2,
                       const TopoDS_Face & F )
  {
    double f,l;
    Handle(Geom2d_Curve) c1 = BRep_Tool::CurveOnSurface( E1, F, f, l );
    gp_Pnt2d uvFirst1 = c1->Value( f );
    gp_Pnt2d uvLast1  = c1->Value( l );

    Handle(Geom2d_Curve) c2 = BRep_Tool::CurveOnSurface( E2, F, f, l );
    gp_Pnt2d uvFirst2 = c2->Value( E2.Orientation() == TopAbs_REVERSED ? l : f );
    double tol2 = Max( Precision::PConfusion() * Precision::PConfusion(),
                       1e-5 * uvLast1.SquareDistance( uvFirst1 ));

    return (( uvFirst2.SquareDistance( uvFirst1 ) < tol2 ) ||
            ( uvFirst2.SquareDistance( uvLast1  ) < tol2 ));
  }

  //================================================================================
  /*!
   * \brief Compose TSideVector for both FACEs keeping matching order of EDGEs
   *        and fill src2tgtNodes map
   */
  //================================================================================

  TError getWires(const TopoDS_Face&                 tgtFace,
                  const TopoDS_Face&                 srcFace,
                  SMESH_Mesh *                       tgtMesh,
                  SMESH_Mesh *                       srcMesh,
                  const TAssocTool::TShapeShapeMap&  shape2ShapeMap,
                  TSideVector&                       srcWires,
                  TSideVector&                       tgtWires,
                  TAssocTool::TNodeNodeMap&          src2tgtNodes,
                  bool&                              is1DComputed)
  {
    SMESHDS_Mesh* tgtMeshDS = tgtMesh->GetMeshDS();
    SMESHDS_Mesh* srcMeshDS = srcMesh->GetMeshDS();

    src2tgtNodes.clear();

    // get ordered src EDGEs
    TError err;
    srcWires = StdMeshers_FaceSide::GetFaceWires( srcFace, *srcMesh,/*skipMediumNodes=*/0, err);
    if ( ( err && !err->IsOK() ) || srcWires.empty() )
      return err;

    SMESH_MesherHelper srcHelper( *srcMesh );
    srcHelper.SetSubShape( srcFace );

    // make corresponding sequence of tgt EDGEs
    tgtWires.resize( srcWires.size() );
    for ( size_t iW = 0; iW < srcWires.size(); ++iW )
    {
      StdMeshers_FaceSidePtr srcWire = srcWires[iW];

      list< TopoDS_Edge > tgtEdges;
      TopTools_IndexedMapOfShape edgeMap; // to detect seam edges
      for ( int iE = 0; iE < srcWire->NbEdges(); ++iE )
      {
        TopoDS_Edge     srcE = srcWire->Edge( iE );
        TopoDS_Edge     tgtE = TopoDS::Edge( shape2ShapeMap( srcE, /*isSrc=*/true));
        TopoDS_Shape srcEbis = shape2ShapeMap( tgtE, /*isSrc=*/false );
        if ( srcE.Orientation() != srcEbis.Orientation() )
          tgtE.Reverse();
        // reverse a seam edge encountered for the second time
        const int index = edgeMap.Add( tgtE );
        if ( index < edgeMap.Extent() ) // E is a seam
        {
          // check which of edges to reverse, E or one already being in tgtEdges
          if ( are2dConnected( tgtEdges.back(), tgtE, tgtFace ))
          {
            list< TopoDS_Edge >::iterator eIt = tgtEdges.begin();
            std::advance( eIt, index-1 );
            if ( are2dConnected( tgtEdges.back(), *eIt, tgtFace ))
              eIt->Reverse();
          }
          else
          {
            tgtE.Reverse();
          }
        }
        if ( srcWire->NbEdges() == 1 && tgtMesh == srcMesh ) // circle
        {
          // try to verify ori by propagation
          pair<int,TopoDS_Edge> nE =
            StdMeshers_ProjectionUtils::GetPropagationEdge( srcMesh, tgtE, srcE );
          if ( !nE.second.IsNull() )
            tgtE = nE.second;
        }
        tgtEdges.push_back( tgtE );
      }

      tgtWires[ iW ].reset( new StdMeshers_FaceSide( tgtFace, tgtEdges, tgtMesh,
                                                     /*theIsForward = */ true,
                                                     /*theIgnoreMediumNodes = */false));
      StdMeshers_FaceSidePtr tgtWire = tgtWires[ iW ];

      // Fill map of src to tgt nodes with nodes on edges

      for ( int iE = 0; iE < srcWire->NbEdges(); ++iE )
      {
        if ( srcMesh->GetSubMesh( srcWire->Edge(iE) )->IsEmpty() ||
             tgtMesh->GetSubMesh( tgtWire->Edge(iE) )->IsEmpty() )
        {
          // add nodes on VERTEXes for a case of not meshes EDGEs
          const SMDS_MeshNode* srcN = srcWire->VertexNode( iE );
          const SMDS_MeshNode* tgtN = tgtWire->VertexNode( iE );
          if ( srcN && tgtN )
            src2tgtNodes.insert( make_pair( srcN, tgtN ));
        }
        else
        {
          const bool skipMedium = true, isFwd = true;
          StdMeshers_FaceSide srcEdge( srcFace, srcWire->Edge(iE), srcMesh, isFwd, skipMedium);
          StdMeshers_FaceSide tgtEdge( tgtFace, tgtWire->Edge(iE), tgtMesh, isFwd, skipMedium);
          
          vector< const SMDS_MeshNode* > srcNodes = srcEdge.GetOrderedNodes();
          vector< const SMDS_MeshNode* > tgtNodes = tgtEdge.GetOrderedNodes();

          if (( srcNodes.size() != tgtNodes.size() ) && tgtNodes.size() > 0 )
            return SMESH_ComputeError::New( COMPERR_BAD_INPUT_MESH,
                                            "Different number of nodes on edges");
          if ( !tgtNodes.empty() )
          {
            vector< const SMDS_MeshNode* >::iterator tn = tgtNodes.begin();
            //if ( srcWire->Edge(iE).Orientation() == tgtWire->Edge(iE).Orientation() )
            {
              vector< const SMDS_MeshNode* >::iterator sn = srcNodes.begin();
              for ( ; tn != tgtNodes.end(); ++tn, ++sn)
                src2tgtNodes.insert( make_pair( *sn, *tn ));
            }
            // else
            // {
            //   vector< const SMDS_MeshNode* >::reverse_iterator sn = srcNodes.rbegin();
            //   for ( ; tn != tgtNodes.end(); ++tn, ++sn)
            //     src2tgtNodes.insert( make_pair( *sn, *tn ));
            // }
            is1DComputed = true;
          }
        }
      } // loop on EDGEs of a WIRE

    } // loop on WIREs

    return TError();
  }

  //================================================================================
  /*!
   * \brief Preform projection in case if tgtFace.IsPartner( srcFace ) and in case
   * if projection by 3D transformation is possible
   */
  //================================================================================

  bool projectPartner(const TopoDS_Face&                 tgtFace,
                      const TopoDS_Face&                 srcFace,
                      const TSideVector&                 tgtWires,
                      const TSideVector&                 srcWires,
                      const TAssocTool::TShapeShapeMap&  shape2ShapeMap,
                      TAssocTool::TNodeNodeMap&          src2tgtNodes,
                      const bool                         is1DComputed)
  {
    SMESH_Mesh *    tgtMesh = tgtWires[0]->GetMesh();
    SMESH_Mesh *    srcMesh = srcWires[0]->GetMesh();
    SMESHDS_Mesh* tgtMeshDS = tgtMesh->GetMeshDS();
    SMESHDS_Mesh* srcMeshDS = srcMesh->GetMeshDS();
    SMESH_MesherHelper helper( *tgtMesh );

    const double tol = 1.e-7 * srcMeshDS->getMaxDim();

    // transformation to get location of target nodes from source ones
    StdMeshers_ProjectionUtils::TrsfFinder3D trsf;
    bool trsfIsOK = false;
    if ( tgtFace.IsPartner( srcFace ))
    {
      gp_GTrsf srcTrsf = srcFace.Location().Transformation();
      gp_GTrsf tgtTrsf = tgtFace.Location().Transformation();
      gp_GTrsf t = srcTrsf.Inverted().Multiplied( tgtTrsf );
      trsf.Set( t );
      // check
      gp_Pnt srcP = BRep_Tool::Pnt( srcWires[0]->FirstVertex() );
      gp_Pnt tgtP = BRep_Tool::Pnt( tgtWires[0]->FirstVertex() );
      trsfIsOK = ( tgtP.Distance( trsf.Transform( srcP )) < tol );
      if ( !trsfIsOK )
      {
        trsf.Set( tgtTrsf.Inverted().Multiplied( srcTrsf ));
        trsfIsOK = ( tgtP.Distance( trsf.Transform( srcP )) < tol );
      }
    }
    if ( !trsfIsOK )
    {
      // Try to find the 3D transformation

      const int totNbSeg = 50;
      vector< gp_XYZ > srcPnts, tgtPnts;
      srcPnts.reserve( totNbSeg );
      tgtPnts.reserve( totNbSeg );
      gp_XYZ srcBC( 0,0,0 ), tgtBC( 0,0,0 );
      for ( size_t iW = 0; iW < srcWires.size(); ++iW )
      {
        const double minSegLen = srcWires[iW]->Length() / totNbSeg;
        for ( int iE = 0; iE < srcWires[iW]->NbEdges(); ++iE )
        {
          int nbSeg    = Max( 1, int( srcWires[iW]->EdgeLength( iE ) / minSegLen ));
          double srcU  = srcWires[iW]->FirstParameter( iE );
          double tgtU  = tgtWires[iW]->FirstParameter( iE );
          double srcDu = ( srcWires[iW]->LastParameter( iE )- srcU ) / nbSeg;
          double tgtDu = ( tgtWires[iW]->LastParameter( iE )- tgtU ) / nbSeg;
          for ( size_t i = 0; i < nbSeg; ++i  )
          {
            srcPnts.push_back( srcWires[iW]->Value3d( srcU ).XYZ() );
            tgtPnts.push_back( tgtWires[iW]->Value3d( tgtU ).XYZ() );
            srcU += srcDu;
            tgtU += tgtDu;
            srcBC += srcPnts.back();
            tgtBC += tgtPnts.back();
          }
        }
      }
      if ( !trsf.Solve( srcPnts, tgtPnts ))
        return false;

      // check trsf

      const int nbTestPnt = 20;
      const size_t  iStep = Max( 1, int( srcPnts.size() / nbTestPnt ));
      // check boundary
      gp_Pnt trsfTgt = trsf.Transform( srcBC / srcPnts.size() );
      trsfIsOK = ( trsfTgt.SquareDistance( tgtBC / tgtPnts.size() ) < tol*tol );
      for ( size_t i = 0; ( i < srcPnts.size() && trsfIsOK ); i += iStep )
      {
        gp_Pnt trsfTgt = trsf.Transform( srcPnts[i] );
        trsfIsOK = ( trsfTgt.SquareDistance( tgtPnts[i] ) < tol*tol );
      }
      // check an in-FACE point
      if ( trsfIsOK )
      {
        BRepAdaptor_Surface srcSurf( srcFace );
        gp_Pnt srcP =
          srcSurf.Value( 0.321 * ( srcSurf.FirstUParameter() + srcSurf.LastUParameter() ),
                         0.123 * ( srcSurf.FirstVParameter() + srcSurf.LastVParameter() ));
        gp_Pnt tgtTrsfP = trsf.Transform( srcP );
        TopLoc_Location loc;
        GeomAPI_ProjectPointOnSurf& proj = helper.GetProjector( tgtFace, loc, 0.1*tol );
        if ( !loc.IsIdentity() )
          tgtTrsfP.Transform( loc.Transformation().Inverted() );
        proj.Perform( tgtTrsfP );
        trsfIsOK = ( proj.IsDone() &&
                     proj.NbPoints() > 0 &&
                     proj.LowerDistance() < tol );
      }
      if ( !trsfIsOK )
        return false;
    }

    // Make new faces

    // prepare the helper to adding quadratic elements if necessary
    //helper.SetSubShape( tgtFace );
    helper.IsQuadraticSubMesh( tgtFace );

    SMESHDS_SubMesh* srcSubDS = srcMeshDS->MeshElements( srcFace );
    if ( !is1DComputed && srcSubDS->NbElements() )
      helper.SetIsQuadratic( srcSubDS->GetElements()->next()->IsQuadratic() );

    SMESH_MesherHelper srcHelper( *srcMesh );
    srcHelper.SetSubShape( srcFace );

    const SMDS_MeshNode* nullNode = 0;
    TAssocTool::TNodeNodeMap::iterator srcN_tgtN;

    // indices of nodes to create properly oriented faces
    bool isReverse = ( !trsf.IsIdentity() );
    int tri1 = 1, tri2 = 2, quad1 = 1, quad3 = 3;
    if ( isReverse )
      std::swap( tri1, tri2 ), std::swap( quad1, quad3 );

    SMDS_ElemIteratorPtr elemIt = srcSubDS->GetElements();
    vector< const SMDS_MeshNode* > tgtNodes;
    while ( elemIt->more() ) // loop on all mesh faces on srcFace
    {
      const SMDS_MeshElement* elem = elemIt->next();
      const int nbN = elem->NbCornerNodes(); 
      tgtNodes.resize( nbN );
      helper.SetElementsOnShape( false );
      for ( int i = 0; i < nbN; ++i ) // loop on nodes of the source element
      {
        const SMDS_MeshNode* srcNode = elem->GetNode(i);
        srcN_tgtN = src2tgtNodes.insert( make_pair( srcNode, nullNode )).first;
        if ( srcN_tgtN->second == nullNode )
        {
          // create a new node
          gp_Pnt tgtP = trsf.Transform( SMESH_TNodeXYZ( srcNode ));
          SMDS_MeshNode* n = helper.AddNode( tgtP.X(), tgtP.Y(), tgtP.Z() );
          srcN_tgtN->second = n;
          switch ( srcNode->GetPosition()->GetTypeOfPosition() )
          {
          case SMDS_TOP_FACE:
          {
            gp_Pnt2d srcUV = srcHelper.GetNodeUV( srcFace, srcNode );
            tgtMeshDS->SetNodeOnFace( n, helper.GetSubShapeID(), srcUV.X(), srcUV.Y() );
            break;
          }
          case SMDS_TOP_EDGE:
          {
            const TopoDS_Shape & srcE = srcMeshDS->IndexToShape( srcNode->getshapeId() );
            const TopoDS_Shape & tgtE = shape2ShapeMap( srcE, /*isSrc=*/true );
            double srcU = srcHelper.GetNodeU( TopoDS::Edge( srcE ), srcNode );
            tgtMeshDS->SetNodeOnEdge( n, TopoDS::Edge( tgtE ), srcU );
            break;
          }
          case SMDS_TOP_VERTEX:
          {
            const TopoDS_Shape & srcV = srcMeshDS->IndexToShape( srcNode->getshapeId() );
            const TopoDS_Shape & tgtV = shape2ShapeMap( srcV, /*isSrc=*/true );
            tgtMeshDS->SetNodeOnVertex( n, TopoDS::Vertex( tgtV ));
            break;
          }
          default:;
          }
        }
        tgtNodes[i] = srcN_tgtN->second;
      }
      // create a new face
      helper.SetElementsOnShape( true );
      switch ( nbN )
      {
      case 3: helper.AddFace(tgtNodes[0], tgtNodes[tri1], tgtNodes[tri2]); break;
      case 4: helper.AddFace(tgtNodes[0], tgtNodes[quad1], tgtNodes[2], tgtNodes[quad3]); break;
      default:
        if ( isReverse ) std::reverse( tgtNodes.begin(), tgtNodes.end() );
        helper.AddPolygonalFace( tgtNodes );
      }
    }

    // check node positions

    if ( !tgtFace.IsPartner( srcFace ) )
    {
      SMESH_MesherHelper edgeHelper( *tgtMesh );
      edgeHelper.ToFixNodeParameters( true );
      helper.ToFixNodeParameters( true );

      int nbOkPos = 0;
      bool toCheck = true;
      const double tol2d = 1e-12;
      srcN_tgtN = src2tgtNodes.begin();
      for ( ; srcN_tgtN != src2tgtNodes.end(); ++srcN_tgtN )
      {
        const SMDS_MeshNode* n = srcN_tgtN->second;
        switch ( n->GetPosition()->GetTypeOfPosition() )
        {
        case SMDS_TOP_FACE:
        {
          if ( nbOkPos > 10 ) break;
          gp_XY uv = helper.GetNodeUV( tgtFace, n ), uvBis = uv;
          if (( helper.CheckNodeUV( tgtFace, n, uv, tol )) &&
              (( uv - uvBis ).SquareModulus() < tol2d ))
            ++nbOkPos;
          else
            nbOkPos = -((int) src2tgtNodes.size() );
          break;
        }
        case SMDS_TOP_EDGE:
        {
          const TopoDS_Edge & tgtE = TopoDS::Edge( tgtMeshDS->IndexToShape( n->getshapeId() ));
          edgeHelper.SetSubShape( tgtE );
          edgeHelper.GetNodeU( tgtE, n, 0, &toCheck );
          break;
        }
        default:;
        }
      }
    }

    return true;

  } //   bool projectPartner()

  //================================================================================
  /*!
   * \brief Preform projection in case if the faces are similar in 2D space
   */
  //================================================================================

  bool projectBy2DSimilarity(const TopoDS_Face&                 tgtFace,
                             const TopoDS_Face&                 srcFace,
                             const TSideVector&                 tgtWires,
                             const TSideVector&                 srcWires,
                             const TAssocTool::TShapeShapeMap&  shape2ShapeMap,
                             TAssocTool::TNodeNodeMap&          src2tgtNodes,
                             const bool                         is1DComputed)
  {
    SMESH_Mesh * tgtMesh = tgtWires[0]->GetMesh();
    SMESH_Mesh * srcMesh = srcWires[0]->GetMesh();

    // WARNING: we can have problems if the FACE is symmetrical in 2D,
    // then the projection can be mirrored relating to what is expected

    // 1) Find 2D transformation

    StdMeshers_ProjectionUtils::TrsfFinder2D trsf;
    {
      // get 2 pairs of corresponding UVs
      gp_Pnt2d srcP0 = srcWires[0]->Value2d(0.0);
      gp_Pnt2d srcP1 = srcWires[0]->Value2d(0.333);
      gp_Pnt2d tgtP0 = tgtWires[0]->Value2d(0.0);
      gp_Pnt2d tgtP1 = tgtWires[0]->Value2d(0.333);

      // make transformation
      gp_Trsf2d fromTgtCS, toSrcCS; // from/to global CS
      gp_Ax2d srcCS( srcP0, gp_Vec2d( srcP0, srcP1 ));
      gp_Ax2d tgtCS( tgtP0, gp_Vec2d( tgtP0, tgtP1 ));
      toSrcCS  .SetTransformation( srcCS );
      fromTgtCS.SetTransformation( tgtCS );
      fromTgtCS.Invert();
      trsf.Set( fromTgtCS * toSrcCS );

      // check transformation
      bool trsfIsOK = true;
      const double tol = 1e-5 * gp_Vec2d( srcP0, srcP1 ).Magnitude();
      for ( double u = 0.12; ( u < 1. && trsfIsOK ); u += 0.1 )
      {
        gp_Pnt2d srcUV  = srcWires[0]->Value2d( u );
        gp_Pnt2d tgtUV  = tgtWires[0]->Value2d( u );
        gp_Pnt2d tgtUV2 = trsf.Transform( srcUV );
        trsfIsOK = ( tgtUV.Distance( tgtUV2 ) < tol );
      }

      // Find trsf using a least-square approximation
      if ( !trsfIsOK )
      {
        // find trsf
        const int totNbSeg = 50;
        vector< gp_XY > srcPnts, tgtPnts;
        srcPnts.resize( totNbSeg );
        tgtPnts.resize( totNbSeg );
        for ( size_t iW = 0; iW < srcWires.size(); ++iW )
        {
          const double minSegLen = srcWires[iW]->Length() / totNbSeg;
          for ( int iE = 0; iE < srcWires[iW]->NbEdges(); ++iE )
          {
            int nbSeg    = Max( 1, int( srcWires[iW]->EdgeLength( iE ) / minSegLen ));
            double srcU  = srcWires[iW]->FirstParameter( iE );
            double tgtU  = tgtWires[iW]->FirstParameter( iE );
            double srcDu = ( srcWires[iW]->LastParameter( iE )- srcU ) / nbSeg;
            double tgtDu = ( tgtWires[iW]->LastParameter( iE )- tgtU ) / nbSeg;
            for ( size_t i = 0; i < nbSeg; ++i, srcU += srcDu, tgtU += tgtDu  )
            {
              srcPnts.push_back( srcWires[iW]->Value2d( srcU ).XY() );
              tgtPnts.push_back( tgtWires[iW]->Value2d( tgtU ).XY() );
            }
          }
        }
        if ( !trsf.Solve( srcPnts, tgtPnts ))
          return false;

        // check trsf

        trsfIsOK = true;
        const int nbTestPnt = 10;
        const size_t  iStep = Max( 1, int( srcPnts.size() / nbTestPnt ));
        for ( size_t i = 0; ( i < srcPnts.size() && trsfIsOK ); i += iStep )
        {
          gp_Pnt2d trsfTgt = trsf.Transform( srcPnts[i] );
          trsfIsOK = ( trsfTgt.Distance( tgtPnts[i] ) < tol );
        }
        if ( !trsfIsOK )
          return false;
      }
    } // "Find transformation" block

    // 2) Projection

    SMESHDS_SubMesh* srcSubDS = srcMesh->GetMeshDS()->MeshElements( srcFace );

    SMESH_MesherHelper helper( *tgtMesh );
    helper.SetSubShape( tgtFace );
    if ( is1DComputed )
      helper.IsQuadraticSubMesh( tgtFace );
    else
      helper.SetIsQuadratic( srcSubDS->GetElements()->next()->IsQuadratic() );
    helper.SetElementsOnShape( true );
    Handle(Geom_Surface) tgtSurface = BRep_Tool::Surface( tgtFace );
    SMESHDS_Mesh* tgtMeshDS = tgtMesh->GetMeshDS();

    SMESH_MesherHelper srcHelper( *srcMesh );
    srcHelper.SetSubShape( srcFace );

    const SMDS_MeshNode* nullNode = 0;
    TAssocTool::TNodeNodeMap::iterator srcN_tgtN;

    SMDS_ElemIteratorPtr elemIt = srcSubDS->GetElements();
    vector< const SMDS_MeshNode* > tgtNodes;
    bool uvOK;
    while ( elemIt->more() ) // loop on all mesh faces on srcFace
    {
      const SMDS_MeshElement* elem = elemIt->next();
      const int nbN = elem->NbCornerNodes(); 
      tgtNodes.resize( nbN );
      for ( int i = 0; i < nbN; ++i ) // loop on nodes of the source element
      {
        const SMDS_MeshNode* srcNode = elem->GetNode(i);
        srcN_tgtN = src2tgtNodes.insert( make_pair( srcNode, nullNode )).first;
        if ( srcN_tgtN->second == nullNode )
        {
          // create a new node
          gp_Pnt2d srcUV = srcHelper.GetNodeUV( srcFace, srcNode,
                                                elem->GetNode( helper.WrapIndex(i+1,nbN)), &uvOK);
          gp_Pnt2d   tgtUV = trsf.Transform( srcUV );
          gp_Pnt      tgtP = tgtSurface->Value( tgtUV.X(), tgtUV.Y() );
          SMDS_MeshNode* n = tgtMeshDS->AddNode( tgtP.X(), tgtP.Y(), tgtP.Z() );
          switch ( srcNode->GetPosition()->GetTypeOfPosition() )
          {
          case SMDS_TOP_FACE: {
            tgtMeshDS->SetNodeOnFace( n, helper.GetSubShapeID(), tgtUV.X(), tgtUV.Y() );
            break;
          }
          case SMDS_TOP_EDGE: {
            TopoDS_Shape srcEdge = srcHelper.GetSubShapeByNode( srcNode, srcHelper.GetMeshDS() );
            TopoDS_Edge  tgtEdge = TopoDS::Edge( shape2ShapeMap( srcEdge, /*isSrc=*/true ));
            double U = Precision::Infinite();
            helper.CheckNodeU( tgtEdge, n, U, Precision::PConfusion());
            tgtMeshDS->SetNodeOnEdge( n, TopoDS::Edge( tgtEdge ), U );
            break;
          }
          case SMDS_TOP_VERTEX: {
            TopoDS_Shape srcV = srcHelper.GetSubShapeByNode( srcNode, srcHelper.GetMeshDS() );
            TopoDS_Shape tgtV = shape2ShapeMap( srcV, /*isSrc=*/true );
            tgtMeshDS->SetNodeOnVertex( n, TopoDS::Vertex( tgtV ));
            break;
          }
          }
          srcN_tgtN->second = n;
        }
        tgtNodes[i] = srcN_tgtN->second;
      }
      // create a new face (with reversed orientation)
      switch ( nbN )
      {
      case 3: helper.AddFace(tgtNodes[0], tgtNodes[2], tgtNodes[1]); break;
      case 4: helper.AddFace(tgtNodes[0], tgtNodes[3], tgtNodes[2], tgtNodes[1]); break;
      }
    }  // loop on all mesh faces on srcFace

    return true;
  }

  //================================================================================
  /*!
   * \brief Preform projection in case of quadrilateral faces
   */
  //================================================================================

  inline bool projectQuads(const TopoDS_Face&                 tgtFace,
                    const TopoDS_Face&                 srcFace,
                    const TSideVector&                 tgtWires,
                    const TSideVector&                 srcWires,
                    const TAssocTool::TShapeShapeMap&  shape2ShapeMap,
                    TAssocTool::TNodeNodeMap&          src2tgtNodes,
                    const bool                         is1DComputed)
  {
    SMESH_Mesh * tgtMesh = tgtWires[0]->GetMesh();
    SMESH_Mesh * srcMesh = srcWires[0]->GetMesh();
    SMESHDS_Mesh * tgtMeshDS = tgtMesh->GetMeshDS();
    SMESHDS_Mesh * srcMeshDS = srcMesh->GetMeshDS();

    if ( srcWires[0]->NbEdges() != 4 )
      return false;
    if ( !is1DComputed )
      return false;
    for ( int iE = 0; iE < 4; ++iE )
    {
      SMESHDS_SubMesh* sm = srcMeshDS->MeshElements( srcWires[0]->Edge( iE ));
      if ( !sm ) return false;
      if ( sm->NbNodes() + sm->NbElements() == 0 ) return false;
    }
    if ( BRepAdaptor_Surface( tgtFace ).GetType() != GeomAbs_Plane )
      return false;
    // if ( BRepAdaptor_Surface( tgtFace ).GetType() == GeomAbs_Plane &&
    //      BRepAdaptor_Surface( srcFace ).GetType() == GeomAbs_Plane )
    //   return false; // too easy

    // load EDGEs to SMESH_Block

    SMESH_Block block;
    TopTools_IndexedMapOfOrientedShape blockSubShapes;
    {
      const TopoDS_Solid& box = srcMesh->PseudoShape();
      TopoDS_Shell shell = TopoDS::Shell( TopExp_Explorer( box, TopAbs_SHELL ).Current() );
      TopoDS_Vertex v;
      block.LoadBlockShapes( shell, v, v, blockSubShapes ); // fill all since operator[] is missing
    }
    const SMESH_Block::TShapeID srcFaceBID = SMESH_Block::ID_Fxy0;
    const SMESH_Block::TShapeID tgtFaceBID = SMESH_Block::ID_Fxy1;
    vector< int > edgeBID;
    block.GetFaceEdgesIDs( srcFaceBID, edgeBID ); // u0, u1, 0v, 1v
    blockSubShapes.Substitute( edgeBID[0], srcWires[0]->Edge(0) );
    blockSubShapes.Substitute( edgeBID[1], srcWires[0]->Edge(2) );
    blockSubShapes.Substitute( edgeBID[2], srcWires[0]->Edge(3) );
    blockSubShapes.Substitute( edgeBID[3], srcWires[0]->Edge(1) );
    block.GetFaceEdgesIDs( tgtFaceBID, edgeBID ); // u0, u1, 0v, 1v
    blockSubShapes.Substitute( edgeBID[0], tgtWires[0]->Edge(0) );
    blockSubShapes.Substitute( edgeBID[1], tgtWires[0]->Edge(2) );
    blockSubShapes.Substitute( edgeBID[2], tgtWires[0]->Edge(3) );
    blockSubShapes.Substitute( edgeBID[3], tgtWires[0]->Edge(1) );
    block.LoadFace( srcFace, srcFaceBID, blockSubShapes );
    block.LoadFace( tgtFace, tgtFaceBID, blockSubShapes );

    // remember connectivity of new faces in terms of ( node-or-XY )

    typedef std::pair< const SMDS_MeshNode*, gp_XYZ > TNodeOrXY; // node-or-XY
    typedef std::vector< TNodeOrXY* >                 TFaceConn; // face connectivity
    std::vector< TFaceConn >                    newFacesVec;     // connectivity of all faces
    std::map< const SMDS_MeshNode*, TNodeOrXY > srcNode2tgtNXY;  // src node -> node-or-XY

    TAssocTool::TNodeNodeMap::iterator                                       srcN_tgtN;
    std::map< const SMDS_MeshNode*, TNodeOrXY >::iterator                    srcN_tgtNXY;
    std::pair< std::map< const SMDS_MeshNode*, TNodeOrXY >::iterator, bool > n2n_isNew;
    TNodeOrXY nullNXY( (SMDS_MeshNode*)NULL, gp_XYZ(0,0,0) );

    SMESHDS_SubMesh* srcSubDS = srcMeshDS->MeshElements( srcFace );
    newFacesVec.resize( srcSubDS->NbElements() );
    int iFaceSrc = 0;

    SMDS_ElemIteratorPtr elemIt = srcSubDS->GetElements();
    while ( elemIt->more() ) // loop on all mesh faces on srcFace
    {
      const SMDS_MeshElement* elem = elemIt->next();
      TFaceConn& tgtNodes = newFacesVec[ iFaceSrc++ ];

      const int nbN = elem->NbCornerNodes(); 
      tgtNodes.resize( nbN );
      for ( int i = 0; i < nbN; ++i ) // loop on nodes of the source element
      {
        const SMDS_MeshNode* srcNode = elem->GetNode(i);
        n2n_isNew = srcNode2tgtNXY.insert( make_pair( srcNode, nullNXY ));
        TNodeOrXY & tgtNodeOrXY = n2n_isNew.first->second;
        if ( n2n_isNew.second ) // new src node encounters
        {
          srcN_tgtN = src2tgtNodes.find( srcNode );
          if ( srcN_tgtN != src2tgtNodes.end() )
          {
            tgtNodeOrXY.first = srcN_tgtN->second; // tgt node exists
          }
          else 
          {
            // find XY of src node withing the quadrilateral srcFace
            if ( !block.ComputeParameters( SMESH_TNodeXYZ( srcNode ),
                                           tgtNodeOrXY.second, srcFaceBID ))
              return false;
          }
        }
        tgtNodes[ i ] = & tgtNodeOrXY;
      }
    }

    // as all XY are computed, create tgt nodes and faces

    SMESH_MesherHelper helper( *tgtMesh );
    helper.SetSubShape( tgtFace );
    if ( is1DComputed )
      helper.IsQuadraticSubMesh( tgtFace );
    else
      helper.SetIsQuadratic( srcSubDS->GetElements()->next()->IsQuadratic() );
    helper.SetElementsOnShape( true );
    Handle(Geom_Surface) tgtSurface = BRep_Tool::Surface( tgtFace );

    SMESH_MesherHelper srcHelper( *srcMesh );
    srcHelper.SetSubShape( srcFace );

    vector< const SMDS_MeshNode* > tgtNodes;
    gp_XY uv;

    for ( size_t iFaceTgt = 0; iFaceTgt < newFacesVec.size(); ++iFaceTgt )
    {
      TFaceConn& tgtConn = newFacesVec[ iFaceTgt ];
      tgtNodes.resize( tgtConn.size() );
      for ( size_t iN = 0; iN < tgtConn.size(); ++iN )
      {
        const SMDS_MeshNode* & tgtN = tgtConn[ iN ]->first;
        if ( !tgtN ) // create a node
        {
          if ( !block.FaceUV( tgtFaceBID, tgtConn[iN]->second, uv ))
            return false;
          gp_Pnt p = tgtSurface->Value( uv.X(), uv.Y() );
          tgtN = helper.AddNode( p.X(), p.Y(), p.Z(), uv.X(), uv.Y() );
        }
        tgtNodes[ tgtNodes.size() - iN - 1] = tgtN; // reversed orientation
      }
      switch ( tgtNodes.size() )
      {
      case 3: helper.AddFace(tgtNodes[0], tgtNodes[1], tgtNodes[2]); break;
      case 4: helper.AddFace(tgtNodes[0], tgtNodes[1], tgtNodes[2], tgtNodes[3]); break;
      default:
        if ( tgtNodes.size() > 4 )
          helper.AddPolygonalFace( tgtNodes );
      }
    }
    return true;

  } // bool projectQuads(...)

  //================================================================================
  /*!
   * \brief Fix bad faces by smoothing
   */
  //================================================================================

  bool fixDistortedFaces( SMESH_MesherHelper& helper,
                          TSideVector&        tgtWires )
  {
    SMESH_subMesh* faceSM = helper.GetMesh()->GetSubMesh( helper.GetSubShape() );

    if ( helper.IsDistorted2D( faceSM, /*checkUV=*/false ))
    {
      SMESH_MeshEditor editor( helper.GetMesh() );
      SMESHDS_SubMesh* smDS = faceSM->GetSubMeshDS();
      const TopoDS_Face&  F = TopoDS::Face( faceSM->GetSubShape() );

      TIDSortedElemSet faces;
      SMDS_ElemIteratorPtr faceIt = smDS->GetElements();
      for ( faceIt = smDS->GetElements(); faceIt->more(); )
        faces.insert( faces.end(), faceIt->next() );

      // choose smoothing algo
      //SMESH_MeshEditor:: SmoothMethod algo = SMESH_MeshEditor::CENTROIDAL;
      bool isConcaveBoundary = false;
      for ( size_t iW = 0; iW < tgtWires.size() && !isConcaveBoundary; ++iW )
      {
        TopoDS_Edge prevEdge = tgtWires[iW]->Edge( tgtWires[iW]->NbEdges() - 1 );
        for ( int iE = 0; iE < tgtWires[iW]->NbEdges() && !isConcaveBoundary; ++iE )
        {
          double angle = helper.GetAngle( prevEdge, tgtWires[iW]->Edge( iE ),
                                          F,        tgtWires[iW]->FirstVertex( iE ));
          isConcaveBoundary = ( angle < -5. * M_PI / 180. );

          prevEdge = tgtWires[iW]->Edge( iE );
        }
      }
      SMESH_MeshEditor:: SmoothMethod algo =
        isConcaveBoundary ? SMESH_MeshEditor::CENTROIDAL : SMESH_MeshEditor::LAPLACIAN;

      // smooth in 2D or 3D?
      TopLoc_Location loc;
      Handle(Geom_Surface) surface = BRep_Tool::Surface( F, loc );
      bool isPlanar = GeomLib_IsPlanarSurface( surface ).IsPlanar();

      // smoothing
      set<const SMDS_MeshNode*> fixedNodes;
      editor.Smooth( faces, fixedNodes, algo, /*nbIterations=*/ 10,
                     /*theTgtAspectRatio=*/1.0, /*the2D=*/!isPlanar);

      helper.ToFixNodeParameters( true );

      return !helper.IsDistorted2D( faceSM, /*checkUV=*/true );
    }
    return true;
  }

} // namespace


//=======================================================================
//function : Compute
//purpose  :
//=======================================================================

bool StdMeshers_Projection_2D::Compute(SMESH_Mesh& theMesh, const TopoDS_Shape& theShape)
{
  _src2tgtNodes.clear();

  MESSAGE("Projection_2D Compute");
  if ( !_sourceHypo )
    return false;

  SMESH_Mesh * srcMesh = _sourceHypo->GetSourceMesh();
  SMESH_Mesh * tgtMesh = & theMesh;
  if ( !srcMesh )
    srcMesh = tgtMesh;

  SMESHDS_Mesh * meshDS = theMesh.GetMeshDS();
  SMESH_MesherHelper helper( theMesh );

  // ---------------------------
  // Make sub-shapes association
  // ---------------------------

  TopoDS_Face   tgtFace = TopoDS::Face( theShape.Oriented(TopAbs_FORWARD));
  TopoDS_Shape srcShape = _sourceHypo->GetSourceFace().Oriented(TopAbs_FORWARD);

  TAssocTool::TShapeShapeMap shape2ShapeMap;
  TAssocTool::InitVertexAssociation( _sourceHypo, shape2ShapeMap );
  if ( !TAssocTool::FindSubShapeAssociation( tgtFace, tgtMesh, srcShape, srcMesh,
                                             shape2ShapeMap)  ||
       !shape2ShapeMap.IsBound( tgtFace ))
  {
    if ( srcShape.ShapeType() == TopAbs_FACE )
    {
      int nbE1 = helper.Count( tgtFace, TopAbs_EDGE, /*ignoreSame=*/true );
      int nbE2 = helper.Count( srcShape, TopAbs_EDGE, /*ignoreSame=*/true );
      if ( nbE1 != nbE2 )
        return error(COMPERR_BAD_SHAPE,
                     SMESH_Comment("Different number of edges in source and target faces: ")
                     << nbE2 << " and " << nbE1 );
    }
    return error(COMPERR_BAD_SHAPE,"Topology of source and target faces seems different" );
  }
  TopoDS_Face srcFace = TopoDS::Face( shape2ShapeMap( tgtFace ).Oriented(TopAbs_FORWARD));

  // orient faces
  // if ( srcMesh == tgtMesh )
  // {
  //   TopoDS_Shape solid =
  //     helper.GetCommonAncestor( srcFace, tgtFace, *tgtMesh, TopAbs_SOLID );
  //   if ( !solid.IsNull() )
  //   {
  //     srcFace.Orientation( helper.GetSubShapeOri( solid, srcFace ));
  //     tgtFace.Orientation( helper.GetSubShapeOri( solid, tgtFace ));
  //   }
  //   else if ( helper.NbAncestors( srcFace, *tgtMesh, TopAbs_SOLID ) == 1 &&
  //             helper.NbAncestors( tgtFace, *tgtMesh, TopAbs_SOLID ) == 1 )
  //   {
  //     srcFace.Orientation( helper.GetSubShapeOri( tgtMesh->GetShapeToMesh(), srcFace ));
  //     tgtFace.Orientation( helper.GetSubShapeOri( tgtMesh->GetShapeToMesh(), tgtFace ));
  //   }
  // }
  // ----------------------------------------------
  // Assure that mesh on a source Face is computed
  // ----------------------------------------------

  SMESH_subMesh* srcSubMesh = srcMesh->GetSubMesh( srcFace );
  SMESH_subMesh* tgtSubMesh = tgtMesh->GetSubMesh( tgtFace );

  string srcMeshError;
  if ( tgtMesh == srcMesh ) {
    if ( !TAssocTool::MakeComputed( srcSubMesh ))
      srcMeshError = TAssocTool::SourceNotComputedError( srcSubMesh, this );
  }
  else {
    if ( !srcSubMesh->IsMeshComputed() )
      srcMeshError = TAssocTool::SourceNotComputedError();
  }
  if ( !srcMeshError.empty() )
    return error(COMPERR_BAD_INPUT_MESH, srcMeshError );

  // ===========
  // Projection
  // ===========

  // get ordered src and tgt EDGEs
  TSideVector srcWires, tgtWires;
  bool is1DComputed = false; // if any tgt EDGE is meshed
  TError err = getWires( tgtFace, srcFace, tgtMesh, srcMesh,
                         shape2ShapeMap, srcWires, tgtWires, _src2tgtNodes, is1DComputed );
  if ( err && !err->IsOK() )
    return error( err );

  bool projDone = false;

  if ( !projDone )
  {
    // try to project from the same face with different location
    projDone = projectPartner( tgtFace, srcFace, tgtWires, srcWires,
                               shape2ShapeMap, _src2tgtNodes, is1DComputed );
  }
  if ( !projDone )
  {
    // projection in case if the faces are similar in 2D space
    projDone = projectBy2DSimilarity( tgtFace, srcFace, tgtWires, srcWires,
                                      shape2ShapeMap, _src2tgtNodes, is1DComputed);
  }
  if ( !projDone )
  {
    // projection in case of quadrilateral faces
    // projDone = projectQuads( tgtFace, srcFace, tgtWires, srcWires,
    //                          shape2ShapeMap, _src2tgtNodes, is1DComputed);
  }

  helper.SetSubShape( tgtFace );

  // it will remove mesh built on edges and vertices in failure case
  MeshCleaner cleaner( tgtSubMesh );

  if ( !projDone )
  {
    _src2tgtNodes.clear();
    // --------------------
    // Prepare to mapping 
    // --------------------

    // Check if node projection to a face is needed
    Bnd_B2d uvBox;
    SMDS_ElemIteratorPtr faceIt = srcSubMesh->GetSubMeshDS()->GetElements();
    set< const SMDS_MeshNode* > faceNodes;
    for ( ; faceNodes.size() < 3 && faceIt->more();  ) {
      const SMDS_MeshElement* face = faceIt->next();
      SMDS_ElemIteratorPtr nodeIt = face->nodesIterator();
      while ( nodeIt->more() ) {
        const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
        if ( node->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE &&
             faceNodes.insert( node ).second )
          uvBox.Add( helper.GetNodeUV( srcFace, node ));
      }
    }
    bool toProjectNodes = false;
    if ( faceNodes.size() == 1 )
      toProjectNodes = ( uvBox.IsVoid() || uvBox.CornerMin().IsEqual( gp_XY(0,0), 1e-12 ));
    else if ( faceNodes.size() > 1 )
      toProjectNodes = ( uvBox.IsVoid() || uvBox.SquareExtent() < DBL_MIN );

    // Find the corresponding source and target vertex
    // and <theReverse> flag needed to call mapper.Apply()

    TopoDS_Vertex srcV1, tgtV1;
    bool reverse = false;

    TopExp_Explorer vSrcExp( srcFace, TopAbs_VERTEX );
    srcV1 = TopoDS::Vertex( vSrcExp.Current() );
    tgtV1 = TopoDS::Vertex( shape2ShapeMap( srcV1, /*isSrc=*/true ));

    list< TopoDS_Edge > tgtEdges, srcEdges;
    list< int > nbEdgesInWires;
    SMESH_Block::GetOrderedEdges( tgtFace, tgtEdges, nbEdgesInWires, tgtV1 );
    SMESH_Block::GetOrderedEdges( srcFace, srcEdges, nbEdgesInWires, srcV1 );

    if ( nbEdgesInWires.front() > 1 ) // possible to find out orientation
    {
      TopoDS_Edge srcE1 = srcEdges.front(), tgtE1 = tgtEdges.front();
      TopoDS_Shape srcE1bis = shape2ShapeMap( tgtE1 );
      reverse = ( ! srcE1.IsSame( srcE1bis ));
      if ( reverse &&
           //_sourceHypo->HasVertexAssociation() &&
           nbEdgesInWires.front() > 2 &&
           helper.IsRealSeam( tgtEdges.front() ))
      {
        // projection to a face with seam EDGE; pb is that GetOrderedEdges()
        // always puts a seam EDGE first (if possible) and as a result
        // we can't use only theReverse flag to correctly associate source
        // and target faces in the mapper. Thus we select srcV1 so that
        // GetOrderedEdges() to return EDGEs in a needed order
        TopoDS_Face tgtFaceBis = tgtFace;
        TopTools_MapOfShape checkedVMap( tgtEdges.size() );
        checkedVMap.Add ( srcV1 );
        for ( vSrcExp.Next(); vSrcExp.More(); )
        {
          tgtFaceBis.Reverse();
          tgtEdges.clear();
          SMESH_Block::GetOrderedEdges( tgtFaceBis, tgtEdges, nbEdgesInWires, tgtV1 );
          bool ok = true;
          list< TopoDS_Edge >::iterator edgeS = srcEdges.begin(), edgeT = tgtEdges.begin();
          for ( ; edgeS != srcEdges.end() && ok ; ++edgeS, ++edgeT )
            ok = edgeT->IsSame( shape2ShapeMap( *edgeS, /*isSrc=*/true ));
          if ( ok )
            break; // FOUND!

          reverse = !reverse;
          if ( reverse )
          {
            vSrcExp.Next();
            while ( vSrcExp.More() && !checkedVMap.Add( vSrcExp.Current() ))
              vSrcExp.Next();
          }
          else
          {
            srcV1 = TopoDS::Vertex( vSrcExp.Current() );
            tgtV1 = TopoDS::Vertex( shape2ShapeMap( srcV1, /*isSrc=*/true ));
            srcEdges.clear();
            SMESH_Block::GetOrderedEdges( srcFace, srcEdges, nbEdgesInWires, srcV1 );
          }
        }
      }
      // for the case: project to a closed face from a non-closed face w/o vertex assoc;
      // avoid projecting to a seam from two EDGEs with different nb nodes on them
      // ( test mesh_Projection_2D_01/B1 )
      if ( !_sourceHypo->HasVertexAssociation() &&
           nbEdgesInWires.front() > 2 &&
           helper.IsRealSeam( tgtEdges.front() ))
      {
        TopoDS_Shape srcEdge1 = shape2ShapeMap( tgtEdges.front() );
        list< TopoDS_Edge >::iterator srcEdge2 =
          std::find( srcEdges.begin(), srcEdges.end(), srcEdge1);
        list< TopoDS_Edge >::iterator srcEdge3 =
          std::find( srcEdges.begin(), srcEdges.end(), srcEdge1.Reversed());
        if ( srcEdge2 == srcEdges.end() || srcEdge3 == srcEdges.end() ) // srcEdge1 is not a seam
        {
          // find srcEdge2 which also will be projected to tgtEdges.front()
          for ( srcEdge2 = srcEdges.begin(); srcEdge2 != srcEdges.end(); ++srcEdge2 )
            if ( !srcEdge1.IsSame( *srcEdge2 ) &&
                 tgtEdges.front().IsSame( shape2ShapeMap( *srcEdge2, /*isSrc=*/true )))
              break;
          // compare nb nodes on srcEdge1 and srcEdge2
          if ( srcEdge2 != srcEdges.end() )
          {
            int nbN1 = 0, nbN2 = 0;
            if ( SMESHDS_SubMesh* sm = srcMesh->GetMeshDS()->MeshElements( srcEdge1 ))
              nbN1 = sm->NbNodes();
            if ( SMESHDS_SubMesh* sm = srcMesh->GetMeshDS()->MeshElements( *srcEdge2 ))
              nbN2 = sm->NbNodes();
            if ( nbN1 != nbN2 )
              srcV1 = helper.IthVertex( 1, srcEdges.front() );
          }
        }
      }
    }
    else if ( nbEdgesInWires.front() == 1 )
    {
      // TODO::Compare orientation of curves in a sole edge
      //RETURN_BAD_RESULT("Not implemented case");
    }
    else
    {
      RETURN_BAD_RESULT("Bad result from SMESH_Block::GetOrderedEdges()");
    }

    // Load pattern from the source face
    SMESH_Pattern mapper;
    mapper.Load( srcMesh, srcFace, toProjectNodes, srcV1 );
    if ( mapper.GetErrorCode() != SMESH_Pattern::ERR_OK )
      return error(COMPERR_BAD_INPUT_MESH,"Can't load mesh pattern from the source face");

    // --------------------
    // Perform 2D mapping
    // --------------------

    // Compute mesh on a target face

    mapper.Apply( tgtFace, tgtV1, reverse );
    if ( mapper.GetErrorCode() != SMESH_Pattern::ERR_OK ) {
      // std::ofstream file("/tmp/Pattern.smp" );
      // mapper.Save( file );
      return error("Can't apply source mesh pattern to the face");
    }

    // Create the mesh

    const bool toCreatePolygons = false, toCreatePolyedrs = false;
    mapper.MakeMesh( tgtMesh, toCreatePolygons, toCreatePolyedrs );
    if ( mapper.GetErrorCode() != SMESH_Pattern::ERR_OK )
      return error("Can't make mesh by source mesh pattern");

    // -------------------------------------------------------------------------
    // mapper doesn't take care of nodes already existing on edges and vertices,
    // so we must merge nodes created by it with existing ones
    // -------------------------------------------------------------------------

    SMESH_MeshEditor::TListOfListOfNodes groupsOfNodes;

    // Make groups of nodes to merge

    // loop on EDGE and VERTEX sub-meshes of a target FACE
    SMESH_subMeshIteratorPtr smIt = tgtSubMesh->getDependsOnIterator(/*includeSelf=*/false,
                                                                     /*complexShapeFirst=*/false);
    while ( smIt->more() )
    {
      SMESH_subMesh*     sm = smIt->next();
      SMESHDS_SubMesh* smDS = sm->GetSubMeshDS();
      if ( !smDS || smDS->NbNodes() == 0 )
        continue;
      //if ( !is1DComputed && sm->GetSubShape().ShapeType() == TopAbs_EDGE )
      //  break;

      if ( helper.IsDegenShape( sm->GetId() ) ) // to merge all nodes on degenerated
      {
        if ( sm->GetSubShape().ShapeType() == TopAbs_EDGE )
        {
          groupsOfNodes.push_back( list< const SMDS_MeshNode* >() );
          SMESH_subMeshIteratorPtr smDegenIt
            = sm->getDependsOnIterator(/*includeSelf=*/true,/*complexShapeFirst=*/false);
          while ( smDegenIt->more() )
            if (( smDS = smDegenIt->next()->GetSubMeshDS() ))
            {
              SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
              while ( nIt->more() )
                groupsOfNodes.back().push_back( nIt->next() );
            }
        }
        continue; // do not treat sm of degen VERTEX
      }

      // Sort new and old nodes of a submesh separately

      bool isSeam = helper.IsRealSeam( sm->GetId() );

      enum { NEW_NODES = 0, OLD_NODES };
      map< double, const SMDS_MeshNode* > u2nodesMaps[2], u2nodesOnSeam;
      map< double, const SMDS_MeshNode* >::iterator u_oldNode, u_newNode, u_newOnSeam, newEnd;
      set< const SMDS_MeshNode* > seamNodes;

      // mapper changed, no more "mapper puts on a seam edge nodes from 2 edges"
      if ( isSeam && ! getBoundaryNodes ( sm, tgtFace, u2nodesOnSeam, seamNodes ))
      {
         //RETURN_BAD_RESULT("getBoundaryNodes() failed");
      }

      SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
      while ( nIt->more() )
      {
        const SMDS_MeshNode* node = nIt->next();
        bool isOld = isOldNode( node );

        if ( !isOld && isSeam ) { // new node on a seam edge
          if ( seamNodes.count( node ) )
            continue; // node is already in the map
        }

        // sort nodes on edges by their position
        map< double, const SMDS_MeshNode* > & pos2nodes = u2nodesMaps[isOld ? OLD_NODES : NEW_NODES];
        switch ( node->GetPosition()->GetTypeOfPosition() )
        {
        case  SMDS_TOP_VERTEX: {
          if ( !is1DComputed && !pos2nodes.empty() )
            u2nodesMaps[isOld ? NEW_NODES : OLD_NODES].insert( make_pair( 0, node ));
          else
            pos2nodes.insert( make_pair( 0, node ));
          break;
        }
        case  SMDS_TOP_EDGE:   {
          const SMDS_EdgePosition* pos =
            static_cast<const SMDS_EdgePosition*>(node->GetPosition());
          pos2nodes.insert( make_pair( pos->GetUParameter(), node ));
          break;
        }
        default:
          RETURN_BAD_RESULT("Wrong node position type: "<<
                            node->GetPosition()->GetTypeOfPosition());
        }
      }
      const bool mergeNewToOld =
        ( u2nodesMaps[ NEW_NODES ].size() == u2nodesMaps[ OLD_NODES ].size() );
      const bool mergeSeamToNew =
        ( u2nodesMaps[ NEW_NODES ].size() == u2nodesOnSeam.size() );

      if ( !mergeNewToOld )
        if ( u2nodesMaps[ NEW_NODES ].size() > 0 &&
             u2nodesMaps[ OLD_NODES ].size() > 0 )
        {
          u_oldNode = u2nodesMaps[ OLD_NODES ].begin(); 
          newEnd    = u2nodesMaps[ OLD_NODES ].end();
          for ( ; u_oldNode != newEnd; ++u_oldNode )
            SMESH_Algo::addBadInputElement( u_oldNode->second );
          return error( COMPERR_BAD_INPUT_MESH,
                        SMESH_Comment( "Existing mesh mismatches the projected 2D mesh on " )
                        << ( sm->GetSubShape().ShapeType() == TopAbs_EDGE ? "edge" : "vertex" )
                        << " #" << sm->GetId() );
        }
      if ( isSeam && !mergeSeamToNew ) {
        const TopoDS_Shape& seam = sm->GetSubShape();
        if ( u2nodesMaps[ NEW_NODES ].size() > 0 &&
             u2nodesOnSeam.size()            > 0 &&
             seam.ShapeType() == TopAbs_EDGE )
        {
          int nbE1 = helper.Count( tgtFace, TopAbs_EDGE, /*ignoreSame=*/true );
          int nbE2 = helper.Count( srcFace, TopAbs_EDGE, /*ignoreSame=*/true );
          if ( nbE1 != nbE2 ) // 2 EDGEs are mapped to a seam EDGE
          {
            // find the 2 EDGEs of srcFace
            TopTools_DataMapIteratorOfDataMapOfShapeShape src2tgtIt( shape2ShapeMap._map2to1 );
            for ( ; src2tgtIt.More(); src2tgtIt.Next() )
              if ( seam.IsSame( src2tgtIt.Value() ))
                SMESH_Algo::addBadInputElements
                  ( srcMesh->GetMeshDS()->MeshElements( src2tgtIt.Key() ));
            return error( COMPERR_BAD_INPUT_MESH,
                          "Different number of nodes on two edges projected to a seam edge" );
          }
        }
      }

      // Make groups of nodes to merge

      u_oldNode = u2nodesMaps[ OLD_NODES ].begin(); 
      u_newNode = u2nodesMaps[ NEW_NODES ].begin();
      newEnd    = u2nodesMaps[ NEW_NODES ].end();
      u_newOnSeam = u2nodesOnSeam.begin();
      if ( mergeNewToOld )
        for ( ; u_newNode != newEnd; ++u_newNode, ++u_oldNode )
        {
          groupsOfNodes.push_back( list< const SMDS_MeshNode* >() );
          groupsOfNodes.back().push_back( u_oldNode->second );
          groupsOfNodes.back().push_back( u_newNode->second );
          if ( mergeSeamToNew )
            groupsOfNodes.back().push_back( (u_newOnSeam++)->second );
        }
      else if ( mergeSeamToNew )
        for ( ; u_newNode != newEnd; ++u_newNode, ++u_newOnSeam )
        {
          groupsOfNodes.push_back( list< const SMDS_MeshNode* >() );
          groupsOfNodes.back().push_back( u_newNode->second );
          groupsOfNodes.back().push_back( u_newOnSeam->second );
        }

    } // loop on EDGE and VERTEX submeshes of a target FACE

    // Merge

    SMESH_MeshEditor editor( tgtMesh );
    int nbFaceBeforeMerge = tgtSubMesh->GetSubMeshDS()->NbElements();
    editor.MergeNodes( groupsOfNodes );
    int nbFaceAtferMerge = tgtSubMesh->GetSubMeshDS()->NbElements();
    if ( nbFaceBeforeMerge != nbFaceAtferMerge && !helper.HasDegeneratedEdges() )
      return error(COMPERR_BAD_INPUT_MESH, "Probably invalid node parameters on geom faces");

    // ----------------------------------------------------------------
    // The mapper can't create quadratic elements, so convert if needed
    // ----------------------------------------------------------------

    faceIt         = srcSubMesh->GetSubMeshDS()->GetElements();
    bool srcIsQuad = faceIt->next()->IsQuadratic();
    faceIt         = tgtSubMesh->GetSubMeshDS()->GetElements();
    bool tgtIsQuad = faceIt->next()->IsQuadratic();
    if ( srcIsQuad && !tgtIsQuad )
    {
      TIDSortedElemSet tgtFaces;
      faceIt = tgtSubMesh->GetSubMeshDS()->GetElements();
      while ( faceIt->more() )
        tgtFaces.insert( tgtFaces.end(), faceIt->next() );

      editor.ConvertToQuadratic(/*theForce3d=*/false, tgtFaces, false);
    }

  } // end of projection using Pattern mapping


  if ( !projDone || is1DComputed )
    // ----------------------------------------------------------------
    // The mapper can create distorted faces by placing nodes out of the FACE
    // boundary, also bad face can be created if EDGEs already discretized
    // --> fix bad faces by smoothing
    // ----------------------------------------------------------------
    if ( !fixDistortedFaces( helper, tgtWires ))
      return error("Invalid mesh generated");

  // ---------------------------
  // Check elements orientation
  // ---------------------------

  TopoDS_Face face = TopoDS::Face( theShape );
  if ( !theMesh.IsMainShape( tgtFace ))
  {
    // find the main shape
    TopoDS_Shape mainShape = meshDS->ShapeToMesh();
    switch ( mainShape.ShapeType() ) {
    case TopAbs_SHELL:
    case TopAbs_SOLID: break;
    default:
      TopTools_ListIteratorOfListOfShape ancestIt = theMesh.GetAncestors( face );
      for ( ; ancestIt.More(); ancestIt.Next() ) {
        TopAbs_ShapeEnum type = ancestIt.Value().ShapeType();
        if ( type == TopAbs_SOLID ) {
          mainShape = ancestIt.Value();
          break;
        } else if ( type == TopAbs_SHELL ) {
          mainShape = ancestIt.Value();
        }
      }
    }
    // find tgtFace in the main solid or shell to know it's true orientation.
    TopExp_Explorer exp( mainShape, TopAbs_FACE );
    for ( ; exp.More(); exp.Next() ) {
      if ( tgtFace.IsSame( exp.Current() )) {
        face = TopoDS::Face( exp.Current() );
        break;
      }
    }
  }
  // Fix orientation
  if ( helper.IsReversedSubMesh( face ))
  {
    SMESH_MeshEditor editor( tgtMesh );
    SMDS_ElemIteratorPtr eIt = meshDS->MeshElements( face )->GetElements();
    while ( eIt->more() ) {
      const SMDS_MeshElement* e = eIt->next();
      if ( e->GetType() == SMDSAbs_Face && !editor.Reorient( e ))
        RETURN_BAD_RESULT("Pb of SMESH_MeshEditor::Reorient()");
    }
  }

  cleaner.Release(); // not to remove mesh

  return true;
}


//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================

bool StdMeshers_Projection_2D::Evaluate(SMESH_Mesh&         theMesh,
                                        const TopoDS_Shape& theShape,
                                        MapShapeNbElems&    aResMap)
{
  if ( !_sourceHypo )
    return false;

  SMESH_Mesh * srcMesh = _sourceHypo->GetSourceMesh();
  SMESH_Mesh * tgtMesh = & theMesh;
  if ( !srcMesh )
    srcMesh = tgtMesh;

  // ---------------------------
  // Make sub-shapes association
  // ---------------------------

  TopoDS_Face tgtFace = TopoDS::Face( theShape.Oriented(TopAbs_FORWARD));
  TopoDS_Shape srcShape = _sourceHypo->GetSourceFace().Oriented(TopAbs_FORWARD);

  TAssocTool::TShapeShapeMap shape2ShapeMap;
  TAssocTool::InitVertexAssociation( _sourceHypo, shape2ShapeMap );
  if ( !TAssocTool::FindSubShapeAssociation( tgtFace, tgtMesh, srcShape, srcMesh,
                                             shape2ShapeMap)  ||
       !shape2ShapeMap.IsBound( tgtFace ))
    return error(COMPERR_BAD_SHAPE,"Topology of source and target faces seems different" );

  TopoDS_Face srcFace = TopoDS::Face( shape2ShapeMap( tgtFace ).Oriented(TopAbs_FORWARD));

  // -------------------------------------------------------
  // Assure that mesh on a source Face is computed/evaluated
  // -------------------------------------------------------

  std::vector<int> aVec;

  SMESH_subMesh* srcSubMesh = srcMesh->GetSubMesh( srcFace );
  if ( srcSubMesh->IsMeshComputed() )
  {
    aVec.resize( SMDSEntity_Last, 0 );
    aVec[SMDSEntity_Node] = srcSubMesh->GetSubMeshDS()->NbNodes();

    SMDS_ElemIteratorPtr elemIt = srcSubMesh->GetSubMeshDS()->GetElements();
    while ( elemIt->more() )
      aVec[ elemIt->next()->GetEntityType() ]++;
  }
  else
  {
    MapShapeNbElems  tmpResMap;
    MapShapeNbElems& srcResMap = (srcMesh == tgtMesh) ? aResMap : tmpResMap;
    if ( !_gen->Evaluate( *srcMesh, srcShape, srcResMap ))
      return error(COMPERR_BAD_INPUT_MESH,"Source mesh not evaluatable");
    aVec = srcResMap[ srcSubMesh ];
    if ( aVec.empty() )
      return error(COMPERR_BAD_INPUT_MESH,"Source mesh is wrongly evaluated");
  }

  SMESH_subMesh * sm = theMesh.GetSubMesh(theShape);
  aResMap.insert(std::make_pair(sm,aVec));

  return true;
}


//=============================================================================
/*!
 * \brief Sets a default event listener to submesh of the source face
  * \param subMesh - submesh where algo is set
 *
 * This method is called when a submesh gets HYP_OK algo_state.
 * After being set, event listener is notified on each event of a submesh.
 * Arranges that CLEAN event is translated from source submesh to
 * the submesh
 */
//=============================================================================

void StdMeshers_Projection_2D::SetEventListener(SMESH_subMesh* subMesh)
{
  TAssocTool::SetEventListener( subMesh,
                                _sourceHypo->GetSourceFace(),
                                _sourceHypo->GetSourceMesh() );
}
