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
// File      : StdMeshers_Projection_1D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_Projection_1D.hxx"

#include "StdMeshers_ProjectionSource1D.hxx"
#include "StdMeshers_ProjectionUtils.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Comment.hxx"

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>

#include "utilities.h"


using namespace std;

#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }

//typedef StdMeshers_ProjectionUtils TAssocTool;
namespace TAssocTool = StdMeshers_ProjectionUtils;

//=======================================================================
//function : StdMeshers_Projection_1D
//purpose  : 
//=======================================================================

StdMeshers_Projection_1D::StdMeshers_Projection_1D(int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_1D_Algo(hypId, studyId, gen)
{
  _name = "Projection_1D";
  _shapeType = (1 << TopAbs_EDGE);      // 1 bit per shape type

  _compatibleHypothesis.push_back("ProjectionSource1D");
  _sourceHypo = 0;
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_Projection_1D::~StdMeshers_Projection_1D()
{}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_Projection_1D::CheckHypothesis(SMESH_Mesh&                          aMesh,
                                               const TopoDS_Shape&                  aShape,
                                               SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  _sourceHypo = 0;
  list <const SMESHDS_Hypothesis * >::const_iterator itl;

  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(aMesh, aShape);
  if ( hyps.size() == 0 )
  {
    aStatus = HYP_MISSING;
    return false;  // can't work with no hypothesis
  }

  if ( hyps.size() > 1 )
  {
    aStatus = HYP_ALREADY_EXIST;
    return false;
  }

  const SMESHDS_Hypothesis *theHyp = hyps.front();

  string hypName = theHyp->GetName();

  aStatus = HYP_OK;

  if (hypName == "ProjectionSource1D")
  {
    _sourceHypo = static_cast<const StdMeshers_ProjectionSource1D *>(theHyp);

    // Check hypo parameters

    SMESH_Mesh* srcMesh = _sourceHypo->GetSourceMesh();
    SMESH_Mesh* tgtMesh = & aMesh;
    if ( !srcMesh )
      srcMesh = tgtMesh;

    // check vertices
    if ( _sourceHypo->HasVertexAssociation() )
    {
      // source and target vertices
      if ( !SMESH_MesherHelper::IsSubShape( _sourceHypo->GetSourceVertex(), srcMesh ) ||
           !SMESH_MesherHelper::IsSubShape( _sourceHypo->GetTargetVertex(), tgtMesh ) ||
           !SMESH_MesherHelper::IsSubShape( _sourceHypo->GetSourceVertex(),
                                            _sourceHypo->GetSourceEdge() ))
      {
        aStatus = HYP_BAD_PARAMETER;
        SCRUTE((SMESH_MesherHelper::IsSubShape( _sourceHypo->GetSourceVertex(), srcMesh )));
        SCRUTE((SMESH_MesherHelper::IsSubShape( _sourceHypo->GetTargetVertex(), tgtMesh )));
        SCRUTE((SMESH_MesherHelper::IsSubShape( _sourceHypo->GetSourceVertex(),
                                                _sourceHypo->GetSourceEdge() )));
      }
      // PAL16202
      else
      {
        bool isSub = SMESH_MesherHelper::IsSubShape( _sourceHypo->GetTargetVertex(), aShape );
        if ( !_sourceHypo->IsCompoundSource() ) {
          if ( !isSub ) {
            aStatus = HYP_BAD_PARAMETER;
            SCRUTE((SMESH_MesherHelper::IsSubShape( _sourceHypo->GetTargetVertex(), aShape)));
          }
        }
        else if ( isSub ) {
          // is Ok provided that source vertex is shared only by one edge
          // of the source group
          TopoDS_Shape sharingEdge;
          TopTools_ListIteratorOfListOfShape ancestIt
            ( aMesh.GetAncestors( _sourceHypo->GetSourceVertex() ));
          for ( ; ancestIt.More(); ancestIt.Next() )
          {
            const TopoDS_Shape& ancestor = ancestIt.Value();
            if ( ancestor.ShapeType() == TopAbs_EDGE &&
                 SMESH_MesherHelper::IsSubShape( ancestor, _sourceHypo->GetSourceEdge() ))
            {
              if ( sharingEdge.IsNull() || ancestor.IsSame( sharingEdge ))
                sharingEdge = ancestor;
              else {
                // the second encountered
                aStatus = HYP_BAD_PARAMETER;
                MESSAGE("Source vertex is shared by several edges of a group");
                break;
              }
            }
          }
        }
      }
    }
    // check source edge
    if ( !SMESH_MesherHelper::IsSubShape( _sourceHypo->GetSourceEdge(), srcMesh ) ||
         ( srcMesh == tgtMesh && aShape == _sourceHypo->GetSourceEdge() ))
    {
      aStatus = HYP_BAD_PARAMETER;
      SCRUTE((SMESH_MesherHelper::IsSubShape( _sourceHypo->GetSourceEdge(), srcMesh )));
      SCRUTE((srcMesh == tgtMesh));
      SCRUTE(( aShape == _sourceHypo->GetSourceEdge() ));
    }
  }
  else
  {
    aStatus = HYP_INCOMPATIBLE;
  }
  return ( aStatus == HYP_OK );
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_Projection_1D::Compute(SMESH_Mesh& theMesh, const TopoDS_Shape& theShape)
{
  if ( !_sourceHypo )
    return false;

  SMESH_Mesh * srcMesh = _sourceHypo->GetSourceMesh(); 
  SMESH_Mesh * tgtMesh = & theMesh;
  if ( !srcMesh )
    srcMesh = tgtMesh;

  SMESHDS_Mesh * meshDS = theMesh.GetMeshDS();

  // ---------------------------
  // Make sub-shapes association
  // ---------------------------

  TopoDS_Edge srcEdge, tgtEdge = TopoDS::Edge( theShape.Oriented(TopAbs_FORWARD));
  TopoDS_Shape srcShape = _sourceHypo->GetSourceEdge().Oriented(TopAbs_FORWARD);

  TAssocTool::TShapeShapeMap shape2ShapeMap;
  TAssocTool::InitVertexAssociation( _sourceHypo, shape2ShapeMap );
  if ( !TAssocTool::FindSubShapeAssociation( tgtEdge, tgtMesh, srcShape, srcMesh,
                                             shape2ShapeMap) ||
       !shape2ShapeMap.IsBound( tgtEdge ))
    return error("Vertices association failed" );

  srcEdge = TopoDS::Edge( shape2ShapeMap( tgtEdge ).Oriented(TopAbs_FORWARD));
//   cout << " srcEdge #" << srcMesh->GetMeshDS()->ShapeToIndex( srcEdge )
//        << " tgtEdge #" << tgtMesh->GetMeshDS()->ShapeToIndex( tgtEdge ) << endl;

  TopoDS_Vertex tgtV[2], srcV[2];
  TopExp::Vertices( tgtEdge, tgtV[0], tgtV[1] );
  TopExp::Vertices( srcEdge, srcV[0], srcV[1] );

  // ----------------------------------------------
  // Assure that mesh on a source edge is computed
  // ----------------------------------------------

  SMESH_subMesh* srcSubMesh = srcMesh->GetSubMesh( srcEdge );
  //SMESH_subMesh* tgtSubMesh = tgtMesh->GetSubMesh( tgtEdge );

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

  // -----------------------------------------------
  // Find out nodes distribution on the source edge
  // -----------------------------------------------

  double srcLength = EdgeLength( srcEdge );
  double tgtLength = EdgeLength( tgtEdge );
  
  vector< double > params; // sorted parameters of nodes on the source edge
  if ( !SMESH_Algo::GetNodeParamOnEdge( srcMesh->GetMeshDS(), srcEdge, params ))
    return error(COMPERR_BAD_INPUT_MESH,"Bad node parameters on the source edge");

  int i, nbNodes = params.size();

  vector< double > lengths( nbNodes - 1 ); // lengths of segments of the source edge
  if ( srcLength > 0 )
  {
    BRepAdaptor_Curve curveAdaptor( srcEdge );
    for ( i = 1; i < nbNodes; ++i )
      lengths[ i-1 ] = GCPnts_AbscissaPoint::Length( curveAdaptor, params[i-1], params[i]);
  }
  else // degenerated source edge
  {
    for ( i = 1; i < nbNodes; ++i )
      lengths[ i-1 ] = params[i] - params[i-1];
    srcLength = params.back() - params[0];
  }

  bool reverse = ( srcV[0].IsSame( shape2ShapeMap( tgtV[1] )));
  if ( tgtV[0].IsSame( tgtV[1] )) // case of closed edge
    reverse = ( shape2ShapeMap( tgtEdge ).Orientation() == TopAbs_REVERSED );
  if ( reverse ) // reverse lengths of segments
    std::reverse( lengths.begin(), lengths.end() );

  // ----------
  // Make mesh
  // ----------

  // vector of target nodes
  vector< const SMDS_MeshNode* > nodes ( nbNodes );

  // Get the first and last nodes
  nodes.front() = VertexNode( tgtV[0], meshDS );
  nodes.back()  = VertexNode( tgtV[1], meshDS );
  if ( !nodes.front() || !nodes.back() )
    return error(COMPERR_BAD_INPUT_MESH,"No node on vertex");

  // Compute parameters on the target edge and make internal nodes
  // --------------------------------------------------------------

  vector< double > tgtParams( nbNodes );

  BRep_Tool::Range( tgtEdge, tgtParams.front(), tgtParams.back() );
  if ( tgtLength <= 0 )
    tgtLength = tgtParams.back() - tgtParams.front();
  double dl = tgtLength / srcLength;

  if ( tgtLength > 0 )
  {
    BRepAdaptor_Curve curveAdaptor( tgtEdge );

    // compute params on internal nodes
    for ( i = 1; i < nbNodes - 1; ++i )
    {
      // computes a point on a <curveAdaptor> at the given distance
      // from the point at given parameter.
      GCPnts_AbscissaPoint Discret( curveAdaptor, dl * lengths[ i-1 ], tgtParams[ i-1 ] );
      if ( !Discret.IsDone() )
        return error("GCPnts_AbscissaPoint failed");
      tgtParams[ i ] = Discret.Parameter();
    }
    // make internal nodes 
    for ( i = 1; i < nbNodes - 1; ++i )
    {
      gp_Pnt P = curveAdaptor.Value( tgtParams[ i ]);
      SMDS_MeshNode* node = meshDS->AddNode(P.X(), P.Y(), P.Z());
      meshDS->SetNodeOnEdge( node, tgtEdge, tgtParams[ i ]);
      nodes[ i ] = node;
    }
  }
  else // degenerated target edge
  {
    // compute params and make internal nodes
    gp_Pnt P = BRep_Tool::Pnt( tgtV[0] );

    for ( i = 1; i < nbNodes - 1; ++i )
    {
      SMDS_MeshNode* node = meshDS->AddNode(P.X(), P.Y(), P.Z());
      tgtParams[ i ] = tgtParams[ i-1 ] + dl * lengths[ i-1 ];
      meshDS->SetNodeOnEdge( node, tgtEdge, tgtParams[ i ]);
      nodes[ i ] = node;
    }
  }

  // Quadratic mesh?
  // ----------------

  bool quadratic = false;
  SMDS_ElemIteratorPtr elemIt = srcSubMesh->GetSubMeshDS()->GetElements();
  if ( elemIt->more() )
    quadratic = elemIt->next()->IsQuadratic();
  else {
    SMDS_NodeIteratorPtr nodeIt = srcSubMesh->GetSubMeshDS()->GetNodes();
    while ( nodeIt->more() && !quadratic )
      quadratic = SMESH_MesherHelper::IsMedium( nodeIt->next() );
  }
  // enough nodes to make all edges quadratic?
  if ( quadratic && ( nbNodes < 3 || ( nbNodes % 2 != 1 )))
    return error(COMPERR_BAD_INPUT_MESH,
                 SMESH_Comment("Wrong number of nodes to make quadratic mesh: ")<<nbNodes);

  // Create edges
  // -------------

  SMDS_MeshElement* edge = 0;
  int di = quadratic ? 2 : 1;
  for ( i = di; i < nbNodes; i += di)
  {
    if ( quadratic )
      edge = meshDS->AddEdge( nodes[i-2], nodes[i], nodes[i-1] );
    else
      edge = meshDS->AddEdge( nodes[i-1], nodes[i] );
    meshDS->SetMeshElementOnShape(edge, tgtEdge );
  }

  return true;
}


//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================

bool StdMeshers_Projection_1D::Evaluate(SMESH_Mesh& theMesh,
                                        const TopoDS_Shape& theShape,
                                        MapShapeNbElems& aResMap)
{
  if ( !_sourceHypo )
    return false;

  SMESH_Mesh * srcMesh = _sourceHypo->GetSourceMesh(); 
  SMESH_Mesh * tgtMesh = & theMesh;
  if ( !srcMesh )
    srcMesh = tgtMesh;

  //SMESHDS_Mesh * meshDS = theMesh.GetMeshDS();

  // ---------------------------
  // Make sub-shapes association
  // ---------------------------

  TopoDS_Edge srcEdge, tgtEdge = TopoDS::Edge( theShape.Oriented(TopAbs_FORWARD));
  TopoDS_Shape srcShape = _sourceHypo->GetSourceEdge().Oriented(TopAbs_FORWARD);

  TAssocTool::TShapeShapeMap shape2ShapeMap;
  TAssocTool::InitVertexAssociation( _sourceHypo, shape2ShapeMap );
  if ( !TAssocTool::FindSubShapeAssociation( tgtEdge, tgtMesh, srcShape, srcMesh,
                                             shape2ShapeMap) ||
       !shape2ShapeMap.IsBound( tgtEdge ))
    return error("Vertices association failed" );

  srcEdge = TopoDS::Edge( shape2ShapeMap( tgtEdge ).Oriented(TopAbs_FORWARD));
//   cout << " srcEdge #" << srcMesh->GetMeshDS()->ShapeToIndex( srcEdge )
//        << " tgtEdge #" << tgtMesh->GetMeshDS()->ShapeToIndex( tgtEdge ) << endl;

  TopoDS_Vertex tgtV[2], srcV[2];
  TopExp::Vertices( tgtEdge, tgtV[0], tgtV[1] );
  TopExp::Vertices( srcEdge, srcV[0], srcV[1] );

  // ----------------------------------------------
  // Assure that mesh on a source edge is computed
  // ----------------------------------------------

  SMESH_subMesh* srcSubMesh = srcMesh->GetSubMesh( srcEdge );
  //SMESH_subMesh* tgtSubMesh = tgtMesh->GetSubMesh( tgtEdge );

  if ( tgtMesh == srcMesh ) {
    if ( !TAssocTool::MakeComputed( srcSubMesh ))
      return error(COMPERR_BAD_INPUT_MESH,"Source mesh not computed");
  }
  else {
    if ( !srcSubMesh->IsMeshComputed() )
      return error(COMPERR_BAD_INPUT_MESH,"Source mesh not computed");
  }
  // -----------------------------------------------
  // Find out nodes distribution on the source edge
  // -----------------------------------------------

  //double srcLength = EdgeLength( srcEdge );
  //double tgtLength = EdgeLength( tgtEdge );
  
  vector< double > params; // sorted parameters of nodes on the source edge
  if ( !SMESH_Algo::GetNodeParamOnEdge( srcMesh->GetMeshDS(), srcEdge, params ))
    return error(COMPERR_BAD_INPUT_MESH,"Bad node parameters on the source edge");

  int nbNodes = params.size();

  std::vector<int> aVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i] = 0;

  aVec[SMDSEntity_Node] = nbNodes;

  bool quadratic = false;
  SMDS_ElemIteratorPtr elemIt = srcSubMesh->GetSubMeshDS()->GetElements();
  if ( elemIt->more() )
    quadratic = elemIt->next()->IsQuadratic();
  if(quadratic)
    aVec[SMDSEntity_Quad_Edge] = (nbNodes-1)/2;
  else
    aVec[SMDSEntity_Edge] = nbNodes - 1;

  SMESH_subMesh * sm = theMesh.GetSubMesh(theShape);
  aResMap.insert(std::make_pair(sm,aVec));

  return true;
}


//=============================================================================
/*!
 * \brief Sets a default event listener to submesh of the source edge
  * \param subMesh - submesh where algo is set
 *
 * This method is called when a submesh gets HYP_OK algo_state.
 * After being set, event listener is notified on each event of a submesh.
 * Arranges that CLEAN event is translated from source submesh to
 * the submesh
 */
//=============================================================================

void StdMeshers_Projection_1D::SetEventListener(SMESH_subMesh* subMesh)
{
  TAssocTool::SetEventListener( subMesh,
                                _sourceHypo->GetSourceEdge(),
                                _sourceHypo->GetSourceMesh() );
}
