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
//  File   : StdMeshers_Import_1D2D.cxx
//  Module : SMESH
//
#include "StdMeshers_Import_1D2D.hxx"

#include "StdMeshers_Import_1D.hxx"
#include "StdMeshers_ImportSource.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESHDS_Group.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Group.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_OctreeNode.hxx"
#include "SMESH_subMesh.hxx"

#include "Utils_SALOME_Exception.hxx"
#include "utilities.h"

#include <BRepBndLib.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_B2d.hxx>
#include <Bnd_Box.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include <numeric>

using namespace std;

namespace
{
  double getMinElemSize2( const SMESHDS_GroupBase* srcGroup )
  {
    double minSize2 = 1e100;
    SMDS_ElemIteratorPtr srcElems = srcGroup->GetElements();
    while ( srcElems->more() ) // loop on group contents
    {
      const SMDS_MeshElement* face = srcElems->next();
      int nbN = face->NbCornerNodes();

      SMESH_TNodeXYZ prevN( face->GetNode( nbN-1 ));
      for ( int i = 0; i < nbN; ++i )
      {
        SMESH_TNodeXYZ n( face->GetNode( i ) );
        double size2 = ( n - prevN ).SquareModulus();
        minSize2 = std::min( minSize2, size2 );
        prevN = n;
      }
    }
    return minSize2;
  }
}

//=============================================================================
/*!
 * Creates StdMeshers_Import_1D2D
 */
//=============================================================================

StdMeshers_Import_1D2D::StdMeshers_Import_1D2D(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_2D_Algo(hypId, studyId, gen), _sourceHyp(0)
{
  MESSAGE("StdMeshers_Import_1D2D::StdMeshers_Import_1D2D");
  _name = "Import_1D2D";
  _shapeType = (1 << TopAbs_FACE);

  _compatibleHypothesis.push_back("ImportSource2D");
  _requireDiscreteBoundary = false;
  _supportSubmeshes = true;
}

//=============================================================================
/*!
 * Check presence of a hypothesis
 */
//=============================================================================

bool StdMeshers_Import_1D2D::CheckHypothesis
                         (SMESH_Mesh&                          aMesh,
                          const TopoDS_Shape&                  aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  _sourceHyp = 0;

  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(aMesh, aShape);
  if ( hyps.size() == 0 )
  {
    aStatus = SMESH_Hypothesis::HYP_MISSING;
    return false;  // can't work with no hypothesis
  }

  if ( hyps.size() > 1 )
  {
    aStatus = SMESH_Hypothesis::HYP_ALREADY_EXIST;
    return false;
  }

  const SMESHDS_Hypothesis *theHyp = hyps.front();

  string hypName = theHyp->GetName();

  if (hypName == _compatibleHypothesis.front())
  {
    _sourceHyp = (StdMeshers_ImportSource1D *)theHyp;
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;
  }

  aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;
  return true;
}

namespace
{
  /*!
   * \brief OrientedLink additionally storing a medium node
   */
  struct TLink : public SMESH_OrientedLink
  {
    const SMDS_MeshNode* _medium;
    TLink( const SMDS_MeshNode* n1,
           const SMDS_MeshNode* n2,
           const SMDS_MeshNode* medium=0)
      : SMESH_OrientedLink( n1,n2 ), _medium( medium ) {}
  };
}

//=============================================================================
/*!
 * Import elements from the other mesh 
 */
//=============================================================================

bool StdMeshers_Import_1D2D::Compute(SMESH_Mesh & theMesh, const TopoDS_Shape & theShape)
{
  if ( !_sourceHyp ) return false;

  const vector<SMESH_Group*>& srcGroups = _sourceHyp->GetGroups(/*loaded=*/true);
  if ( srcGroups.empty() )
    return error("Invalid source groups");

  bool allGroupsEmpty = true;
  for ( size_t iG = 0; iG < srcGroups.size() && allGroupsEmpty; ++iG )
    allGroupsEmpty = srcGroups[iG]->GetGroupDS()->IsEmpty();
  if ( allGroupsEmpty )
    return error("No faces in source groups");

  SMESH_MesherHelper helper(theMesh);
  helper.SetSubShape(theShape);
  SMESHDS_Mesh* tgtMesh = theMesh.GetMeshDS();

  const TopoDS_Face& geomFace = TopoDS::Face( theShape );
  const double faceTol = helper.MaxTolerance( geomFace );
  const int shapeID = tgtMesh->ShapeToIndex( geomFace );
  const bool toCheckOri = (helper.NbAncestors( geomFace, theMesh, TopAbs_SOLID ) == 1 );


  Handle(Geom_Surface) surface = BRep_Tool::Surface( geomFace );
  const bool reverse =
    ( helper.GetSubShapeOri( tgtMesh->ShapeToMesh(), geomFace ) == TopAbs_REVERSED );
  gp_Pnt p; gp_Vec du, dv;

  // BRepClass_FaceClassifier is most time consuming, so minimize its usage
  BRepClass_FaceClassifier classifier;
  Bnd_B2d bndBox2d;
  Bnd_Box bndBox3d;
  {
    Standard_Real umin,umax,vmin,vmax;
    BRepTools::UVBounds(geomFace,umin,umax,vmin,vmax);
    gp_XY pmin( umin,vmin ), pmax( umax,vmax );
    bndBox2d.Add( pmin );
    bndBox2d.Add( pmax );
    if ( helper.HasSeam() )
    {
      const int i = helper.GetPeriodicIndex();
      pmin.SetCoord( i, helper.GetOtherParam( pmin.Coord( i )));
      pmax.SetCoord( i, helper.GetOtherParam( pmax.Coord( i )));
      bndBox2d.Add( pmin );
      bndBox2d.Add( pmax );
    }
    bndBox2d.Enlarge( 1e-2 * Sqrt( bndBox2d.SquareExtent() ));

    BRepBndLib::Add( geomFace, bndBox3d );
    bndBox3d.Enlarge( 1e-5 * sqrt( bndBox3d.SquareExtent() ));
  }

  set<int> subShapeIDs;
  subShapeIDs.insert( shapeID );

  // nodes already existing on sub-shapes of the FACE
  TIDSortedNodeSet existingNodes;

  // get/make nodes on vertices and add them to existingNodes
  TopExp_Explorer exp( theShape, TopAbs_VERTEX );
  for ( ; exp.More(); exp.Next() )
  {
    const TopoDS_Vertex& v = TopoDS::Vertex( exp.Current() );
    if ( !subShapeIDs.insert( tgtMesh->ShapeToIndex( v )).second )
      continue;
    const SMDS_MeshNode* n = SMESH_Algo::VertexNode( v, tgtMesh );
    if ( !n )
    {
      _gen->Compute(theMesh,v,/*anUpward=*/true);
      n = SMESH_Algo::VertexNode( v, tgtMesh );
      if ( !n ) return false; // very strange
    }
    existingNodes.insert( n );
  }

  // get EDGEs and their ids and get existing nodes on EDGEs
  vector< TopoDS_Edge > edges;
  for ( exp.Init( theShape, TopAbs_EDGE ); exp.More(); exp.Next() )
  {
    const TopoDS_Edge & edge = TopoDS::Edge( exp.Current() );
    if ( !SMESH_Algo::isDegenerated( edge ))
      if ( subShapeIDs.insert( tgtMesh->ShapeToIndex( edge )).second )
      {
        edges.push_back( edge );
        if ( SMESHDS_SubMesh* eSM = tgtMesh->MeshElements( edge ))
        {
          typedef SMDS_StdIterator< const SMDS_MeshNode*, SMDS_NodeIteratorPtr > iterator;
          existingNodes.insert( iterator( eSM->GetNodes() ), iterator() );
        }
      }
  }
  // octree to find existing nodes
  SMESH_OctreeNode existingNodeOcTr( existingNodes );
  std::map<double, const SMDS_MeshNode*> dist2foundNodes;

  // to count now many times a link between nodes encounters
  map<TLink, int> linkCount;
  map<TLink, int>::iterator link2Nb;
  double minGroupTol = Precision::Infinite();

  // =========================
  // Import faces from groups
  // =========================

  StdMeshers_Import_1D::TNodeNodeMap* n2n;
  StdMeshers_Import_1D::TElemElemMap* e2e;
  vector<TopAbs_State>         nodeState;
  vector<const SMDS_MeshNode*> newNodes; // of a face
  set   <const SMDS_MeshNode*> bndNodes; // nodes classified ON
  vector<bool>                 isNodeIn; // nodes classified IN, by node ID

  for ( size_t iG = 0; iG < srcGroups.size(); ++iG )
  {
    const SMESHDS_GroupBase* srcGroup = srcGroups[iG]->GetGroupDS();

    const int meshID = srcGroup->GetMesh()->GetPersistentId();
    const SMESH_Mesh* srcMesh = GetMeshByPersistentID( meshID );
    if ( !srcMesh ) continue;
    StdMeshers_Import_1D::getMaps( srcMesh, &theMesh, n2n, e2e );

    const double groupTol = 0.5 * sqrt( getMinElemSize2( srcGroup ));
    minGroupTol = std::min( groupTol, minGroupTol );

    //GeomAdaptor_Surface S( surface );
    // const double clsfTol = Min( S.UResolution( 0.1 * groupTol ), -- issue 0023092
    //                             S.VResolution( 0.1 * groupTol ));
    const double clsfTol = BRep_Tool::Tolerance( geomFace );

    StdMeshers_Import_1D::TNodeNodeMap::iterator n2nIt;
    pair< StdMeshers_Import_1D::TNodeNodeMap::iterator, bool > it_isnew;

    SMDS_ElemIteratorPtr srcElems = srcGroup->GetElements();
    while ( srcElems->more() ) // loop on group contents
    {
      const SMDS_MeshElement* face = srcElems->next();

      SMDS_MeshElement::iterator node = face->begin_nodes();
      if ( bndBox3d.IsOut( SMESH_TNodeXYZ( *node )))
        continue;

      // find or create nodes of a new face
      nodeState.resize( face->NbNodes() );
      newNodes.resize( nodeState.size() );
      newNodes.back() = 0;
      int nbCreatedNodes = 0;
      bool isOut = false, isIn = false; // if at least one node isIn - do not classify other nodes
      for ( size_t i = 0; i < newNodes.size(); ++i, ++node )
      {
        SMESH_TNodeXYZ nXYZ = *node;
        nodeState[ i ] = TopAbs_UNKNOWN;
        newNodes [ i ] = 0;

        it_isnew = n2n->insert( make_pair( *node, (SMDS_MeshNode*)0 ));
        n2nIt    = it_isnew.first;

        const SMDS_MeshNode* & newNode = n2nIt->second;
        if ( !it_isnew.second && !newNode )
          break; // a node is mapped to NULL - it is OUT of the FACE

        if ( newNode )
        {
          if ( !subShapeIDs.count( newNode->getshapeId() ))
            break; // node is Imported onto other FACE
          if ( newNode->GetID() < (int) isNodeIn.size() &&
               isNodeIn[ newNode->GetID() ])
            isIn = true;
          if ( !isIn && bndNodes.count( *node ))
            nodeState[ i ] = TopAbs_ON;
        }
        else
        {
          // find a node pre-existing on EDGE or VERTEX
          dist2foundNodes.clear();
          existingNodeOcTr.NodesAround( nXYZ, dist2foundNodes, groupTol );
          if ( !dist2foundNodes.empty() )
          {
            newNode = dist2foundNodes.begin()->second;
            nodeState[ i ] = TopAbs_ON;
          }
        }

        if ( !newNode )
        {
          // find out if node lies on the surface of theShape
          gp_XY uv( Precision::Infinite(), 0 );
          isOut = ( !helper.CheckNodeUV( geomFace, *node, uv, groupTol, /*force=*/true ) ||
                    bndBox2d.IsOut( uv ));
          if ( !isOut && !isIn ) // classify
          {
            classifier.Perform( geomFace, uv, clsfTol );
            nodeState[i] = classifier.State();
            isOut = ( nodeState[i] == TopAbs_OUT );
          }
          if ( !isOut ) // create a new node
          {
            newNode = tgtMesh->AddNode( nXYZ.X(), nXYZ.Y(), nXYZ.Z());
            tgtMesh->SetNodeOnFace( newNode, shapeID, uv.X(), uv.Y() );
            nbCreatedNodes++;
            if ( newNode->GetID() >= (int) isNodeIn.size() )
            {
              isNodeIn.push_back( false ); // allow allocate more than newNode->GetID()
              isNodeIn.resize( newNode->GetID() + 1, false );
            }
            if ( nodeState[i] == TopAbs_ON )
              bndNodes.insert( *node );
            else
              isNodeIn[ newNode->GetID() ] = isIn = true;
          }
        }
        if ( !(newNodes[i] = newNode ) || isOut )
          break;
      }

      if ( !newNodes.back() )
        continue; // not all nodes of the face lie on theShape

      if ( !isIn ) // if all nodes are on FACE boundary, a mesh face can be OUT
      {
        // check state of nodes created for other faces
        for ( size_t i = 0; i < nodeState.size() && !isIn; ++i )
        {
          if ( nodeState[i] != TopAbs_UNKNOWN ) continue;
          gp_XY uv = helper.GetNodeUV( geomFace, newNodes[i] );
          classifier.Perform( geomFace, uv, clsfTol );
          nodeState[i] = classifier.State();
          isIn = ( nodeState[i] == TopAbs_IN );
        }
        if ( !isIn ) // classify face center
        {
          gp_XYZ gc( 0., 0., 0 );
          for ( size_t i = 0; i < newNodes.size(); ++i )
            gc += SMESH_TNodeXYZ( newNodes[i] );
          gc /= newNodes.size();

          TopLoc_Location loc;
          GeomAPI_ProjectPointOnSurf& proj = helper.GetProjector( geomFace,
                                                                  loc,
                                                                  helper.MaxTolerance( geomFace ));
          if ( !loc.IsIdentity() ) loc.Transformation().Inverted().Transforms( gc );
          proj.Perform( gc );
          if ( !proj.IsDone() || proj.NbPoints() < 1 )
            continue;
          Standard_Real U,V;
          proj.LowerDistanceParameters(U,V);
          gp_XY uv( U,V );
          classifier.Perform( geomFace, uv, clsfTol );
          if ( classifier.State() != TopAbs_IN )
            continue;
        }
      }

      // try to find already created face
      SMDS_MeshElement * newFace = 0;
      if ( nbCreatedNodes == 0 &&
           tgtMesh->FindElement(newNodes, SMDSAbs_Face, /*noMedium=*/false))
        continue; // repeated face in source groups already created

      // check future face orientation
      const int nbCorners = face->NbCornerNodes();
      const bool isQuad   = ( nbCorners != (int) newNodes.size() );
      if ( toCheckOri )
      {
        int iNode = -1;
        gp_Vec geomNorm;
        do
        {
          gp_XY uv = helper.GetNodeUV( geomFace, newNodes[++iNode] );
          surface->D1( uv.X(),uv.Y(), p, du,dv );
          geomNorm = reverse ? dv^du : du^dv;
        }
        while ( geomNorm.SquareMagnitude() < 1e-6 && iNode+1 < nbCorners );

        int iNext = helper.WrapIndex( iNode+1, nbCorners );
        int iPrev = helper.WrapIndex( iNode-1, nbCorners );

        SMESH_TNodeXYZ prevNode( newNodes[iPrev] );
        SMESH_TNodeXYZ curNode ( newNodes[iNode] );
        SMESH_TNodeXYZ nextNode( newNodes[iNext] );
        gp_Vec n1n0( prevNode - curNode);
        gp_Vec n1n2( nextNode - curNode );
        gp_Vec meshNorm = n1n2 ^ n1n0;

        if ( geomNorm * meshNorm < 0 )
          SMDS_MeshCell::applyInterlace
            ( SMDS_MeshCell::reverseSmdsOrder( face->GetEntityType(), newNodes.size() ), newNodes );
      }

      // make a new face
      if ( face->IsPoly() )
        newFace = tgtMesh->AddPolygonalFace( newNodes );
      else
        switch ( newNodes.size() )
        {
        case 3:
          newFace = tgtMesh->AddFace( newNodes[0], newNodes[1], newNodes[2] );
          break;
        case 4:
          newFace = tgtMesh->AddFace( newNodes[0], newNodes[1], newNodes[2], newNodes[3] );
          break;
        case 6:
          newFace = tgtMesh->AddFace( newNodes[0], newNodes[1], newNodes[2],
                                      newNodes[3], newNodes[4], newNodes[5]);
          break;
        case 8:
          newFace = tgtMesh->AddFace( newNodes[0], newNodes[1], newNodes[2], newNodes[3],
                                      newNodes[4], newNodes[5], newNodes[6], newNodes[7]);
          break;
        default: continue;
        }
      tgtMesh->SetMeshElementOnShape( newFace, shapeID );
      e2e->insert( make_pair( face, newFace ));

      // collect links
      const SMDS_MeshNode* medium = 0;
      for ( int i = 0; i < nbCorners; ++i )
      {
        const SMDS_MeshNode* n1 = newNodes[i];
        const SMDS_MeshNode* n2 = newNodes[ (i+1)%nbCorners ];
        if ( isQuad ) // quadratic face
          medium = newNodes[i+nbCorners];
        link2Nb = linkCount.insert( make_pair( TLink( n1, n2, medium ), 0)).first;
        ++link2Nb->second;
        // if ( link2Nb->second == 1 )
        // {
        //   // measure link length
        //   double len2 = SMESH_TNodeXYZ( n1 ).SquareDistance( n2 );
        //   if ( len2 < minGroupTol )
        //     minGroupTol = len2;
        // }
      }
    }
    // Remove OUT nodes from n2n map
    for ( n2nIt = n2n->begin(); n2nIt != n2n->end(); )
      if ( !n2nIt->second )
        n2n->erase( n2nIt++ );
      else
        ++n2nIt;
  }


  // ==========================================================
  // Put nodes on geom edges and create edges on them;
  // check if the whole geom face is covered by imported faces
  // ==========================================================

  // use large tolerance for projection of nodes to edges because of
  // BLSURF mesher specifics (issue 0020918, Study2.hdf)
  const double projTol = minGroupTol;

  bool isFaceMeshed = false;
  SMESHDS_SubMesh* tgtFaceSM = tgtMesh->MeshElements( theShape );
  if ( tgtFaceSM )
  {
    // the imported mesh is valid if all external links (encountered once)
    // lie on geom edges
    subShapeIDs.erase( shapeID ); // to contain edges and vertices only
    double u, f, l;
    for ( link2Nb = linkCount.begin(); link2Nb != linkCount.end(); ++link2Nb)
    {
      const TLink& link = (*link2Nb).first;
      int nbFaces = link2Nb->second;
      if ( nbFaces == 1 )
      {
        // check if a not shared link lies on face boundary
        bool nodesOnBoundary = true;
        list< TopoDS_Shape > bndShapes;
        for ( int is1stN = 0; is1stN < 2 && nodesOnBoundary; ++is1stN )
        {
          const SMDS_MeshNode* n = is1stN ? link.node1() : link.node2();
          if ( !subShapeIDs.count( n->getshapeId() )) // n is assigned to FACE
          {
            for ( size_t iE = 0; iE < edges.size(); ++iE )
              if ( helper.CheckNodeU( edges[iE], n, u=0, projTol, /*force=*/true ))
              {
                BRep_Tool::Range(edges[iE],f,l);
                if ( Abs(u-f) < 2 * faceTol || Abs(u-l) < 2 * faceTol )
                  // duplicated node on vertex
                  return error("Source elements overlap one another");
                tgtFaceSM->RemoveNode( n, /*isNodeDeleted=*/false );
                tgtMesh->SetNodeOnEdge( n, edges[iE], u );
                break;
              }
            nodesOnBoundary = subShapeIDs.count( n->getshapeId());
          }
          if ( nodesOnBoundary )
          {
            TopoDS_Shape s = helper.GetSubShapeByNode( n, tgtMesh );
            if ( s.ShapeType() == TopAbs_VERTEX )
              bndShapes.push_front( s ); // vertex first
            else
              bndShapes.push_back( s ); // edges last
          }
        }
        if ( !nodesOnBoundary )
        {
          error("free internal link"); // just for an easier debug
          break;
        }
        if ( bndShapes.front().ShapeType() == TopAbs_EDGE && // all link nodes are on EDGEs
             bndShapes.front() != bndShapes.back() )
          // link nodes on different geom edges
          return error(COMPERR_BAD_INPUT_MESH, "Source nodes mismatch target vertices");

        // find geom edge the link is on
        if ( bndShapes.back().ShapeType() != TopAbs_EDGE ) // all link nodes are on VERTEXes
        {
          // find geom edge by two vertices
          TopoDS_Shape geomEdge = helper.GetCommonAncestor( bndShapes.back(),
                                                            bndShapes.front(),
                                                            theMesh, TopAbs_EDGE );
          if ( geomEdge.IsNull() )
          {
            error("free internal link");
            break; // vertices belong to different edges
          }
          bndShapes.push_back( geomEdge );
        }

        // create an edge if not yet exists
        newNodes.resize(2);
        newNodes[0] = link.node1(), newNodes[1] = link.node2();
        const SMDS_MeshElement* edge = tgtMesh->FindElement( newNodes, SMDSAbs_Edge );
        if ( edge ) continue;

        if ( link._reversed ) std::swap( newNodes[0], newNodes[1] );
        if ( link._medium )
        {
          edge = tgtMesh->AddEdge( newNodes[0], newNodes[1], link._medium );

          TopoDS_Edge geomEdge = TopoDS::Edge(bndShapes.back());
          helper.CheckNodeU( geomEdge, link._medium, u, projTol, /*force=*/true );
          tgtFaceSM->RemoveNode( link._medium, /*isNodeDeleted=*/false );
          tgtMesh->SetNodeOnEdge( (SMDS_MeshNode*)link._medium, geomEdge, u );
        }
        else
        {
          edge = tgtMesh->AddEdge( newNodes[0], newNodes[1]);
        }
        if ( !edge )
          return false;

        tgtMesh->SetMeshElementOnShape( edge, bndShapes.back() );
      }
      else if ( nbFaces > 2 )
      {
        return error( COMPERR_BAD_INPUT_MESH, "Non-manifold source mesh");
      }
    }
    isFaceMeshed = ( link2Nb == linkCount.end() && !linkCount.empty());
    if ( isFaceMeshed )
    {
      // check that source faces do not overlap:
      // there must be only two edges sharing each vertex and bound to sub-edges of theShape
      SMESH_MeshEditor editor( &theMesh );
      set<int>::iterator subID = subShapeIDs.begin();
      for ( ; subID != subShapeIDs.end(); ++subID )
      {
        const TopoDS_Shape& s = tgtMesh->IndexToShape( *subID );
        if ( s.ShapeType() != TopAbs_VERTEX ) continue;
        const SMDS_MeshNode* n = SMESH_Algo::VertexNode( TopoDS::Vertex(s), tgtMesh );
        SMDS_ElemIteratorPtr eIt = n->GetInverseElementIterator(SMDSAbs_Edge);
        int nbEdges = 0;
        while ( eIt->more() )
        {
          const SMDS_MeshElement* edge = eIt->next();
          int sId = editor.FindShape( edge );
          nbEdges += subShapeIDs.count( sId );
        }
        if ( nbEdges < 2 && !helper.IsRealSeam( s ))
          return false; // weird
        if ( nbEdges > 2 )
          return error( COMPERR_BAD_INPUT_MESH, "Source elements overlap one another");
      }
    }
  }
  if ( !isFaceMeshed )
    return error( COMPERR_BAD_INPUT_MESH,
                  "Source elements don't cover totally the geometrical face" );

  if ( helper.HasSeam() )
  {
    // links on seam edges are shared by two faces, so no edges were created on them
    // by the previous detection of 2D mesh boundary
    for ( size_t iE = 0; iE < edges.size(); ++iE )
    {
      if ( !helper.IsRealSeam( edges[iE] )) continue;
      const TopoDS_Edge& seamEdge = edges[iE];
      // to find nodes lying on the seamEdge we check nodes of mesh faces sharing a node on one
      // of its vertices; after finding another node on seamEdge we continue the same way
      // until finding all nodes.
      TopoDS_Vertex      seamVertex = helper.IthVertex( 0, seamEdge );
      const SMDS_MeshNode* vertNode = SMESH_Algo::VertexNode( seamVertex, tgtMesh );
      set< const SMDS_MeshNode* > checkedNodes; checkedNodes.insert( vertNode );
      set< const SMDS_MeshElement* > checkedFaces;
      // as a face can have more than one node on the seamEdge, there is a difficulty in selecting
      // one of those nodes to treat next; so we simply find all nodes on the seamEdge and
      // then sort them by U on edge
      typedef list< pair< double, const SMDS_MeshNode* > > TUNodeList;
      TUNodeList nodesOnSeam;
      double u = helper.GetNodeU( seamEdge, vertNode );
      nodesOnSeam.push_back( make_pair( u, vertNode ));
      TUNodeList::iterator u2nIt = nodesOnSeam.begin();
      for ( ; u2nIt != nodesOnSeam.end(); ++u2nIt )
      {
        const SMDS_MeshNode* startNode = (*u2nIt).second;
        SMDS_ElemIteratorPtr faceIt = startNode->GetInverseElementIterator( SMDSAbs_Face );
        while ( faceIt->more() )
        {
          const SMDS_MeshElement* face = faceIt->next();
          if ( !checkedFaces.insert( face ).second ) continue;
          for ( int i = 0, nbNodes = face->NbCornerNodes(); i < nbNodes; ++i )
          {
            const SMDS_MeshNode* n = face->GetNode( i );
            if ( n == startNode || !checkedNodes.insert( n ).second ) continue;
            if ( helper.CheckNodeU( seamEdge, n, u=0, projTol, /*force=*/true ))
              nodesOnSeam.push_back( make_pair( u, n ));
          }
        }
      }
      // sort the found nodes by U on the seamEdge; most probably they are in a good order,
      // so we can use the hint to spead-up map filling
      map< double, const SMDS_MeshNode* > u2nodeMap;
      for ( u2nIt = nodesOnSeam.begin(); u2nIt != nodesOnSeam.end(); ++u2nIt )
        u2nodeMap.insert( u2nodeMap.end(), *u2nIt );

      // create edges
      {
        SMESH_MesherHelper seamHelper( theMesh );
        seamHelper.SetSubShape( edges[ iE ]);
        seamHelper.SetElementsOnShape( true );

        if ( !checkedFaces.empty() && (*checkedFaces.begin())->IsQuadratic() )
          for ( set< const SMDS_MeshElement* >::iterator fIt = checkedFaces.begin();
                fIt != checkedFaces.end(); ++fIt )
            seamHelper.AddTLinks( static_cast<const SMDS_MeshFace*>( *fIt ));

        map< double, const SMDS_MeshNode* >::iterator n1, n2, u2nEnd = u2nodeMap.end();
        for ( n2 = u2nodeMap.begin(), n1 = n2++; n2 != u2nEnd; ++n1, ++n2 )
        {
          const SMDS_MeshNode* node1 = n1->second;
          const SMDS_MeshNode* node2 = n2->second;
          seamHelper.AddEdge( node1, node2 );
          if ( node2->getshapeId() == helper.GetSubShapeID() )
          {
            tgtFaceSM->RemoveNode( node2, /*isNodeDeleted=*/false );
            tgtMesh->SetNodeOnEdge( const_cast<SMDS_MeshNode*>( node2 ), seamEdge, n2->first );
          }
        }
      }
    } // loop on edges to find seam ones
  } // if ( helper.HasSeam() )

  // notify sub-meshes of edges on computation
  for ( size_t iE = 0; iE < edges.size(); ++iE )
  {
    SMESH_subMesh * sm = theMesh.GetSubMesh( edges[iE] );
    // if ( SMESH_Algo::isDegenerated( edges[iE] ))
    //   sm->SetIsAlwaysComputed( true );
    sm->ComputeStateEngine(SMESH_subMesh::CHECK_COMPUTE_STATE);
    if ( sm->GetComputeState() != SMESH_subMesh::COMPUTE_OK )
      return error(SMESH_Comment("Failed to create segments on the edge #") << sm->GetId());
  }

  // ============
  // Copy meshes
  // ============

  vector<SMESH_Mesh*> srcMeshes = _sourceHyp->GetSourceMeshes();
  for ( size_t i = 0; i < srcMeshes.size(); ++i )
    StdMeshers_Import_1D::importMesh( srcMeshes[i], theMesh, _sourceHyp, theShape );

  return true;
}

//=============================================================================
/*!
 * \brief Set needed event listeners and create a submesh for a copied mesh
 *
 * This method is called only if a submesh has HYP_OK algo_state.
 */
//=============================================================================

void StdMeshers_Import_1D2D::SetEventListener(SMESH_subMesh* subMesh)
{
  if ( !_sourceHyp )
  {
    const TopoDS_Shape& tgtShape = subMesh->GetSubShape();
    SMESH_Mesh*         tgtMesh  = subMesh->GetFather();
    Hypothesis_Status aStatus;
    CheckHypothesis( *tgtMesh, tgtShape, aStatus );
  }
  StdMeshers_Import_1D::setEventListener( subMesh, _sourceHyp );
}
void StdMeshers_Import_1D2D::SubmeshRestored(SMESH_subMesh* subMesh)
{
  SetEventListener(subMesh);
}

//=============================================================================
/*!
 * Predict nb of mesh entities created by Compute()
 */
//=============================================================================

bool StdMeshers_Import_1D2D::Evaluate(SMESH_Mesh &         theMesh,
                                      const TopoDS_Shape & theShape,
                                      MapShapeNbElems&     aResMap)
{
  if ( !_sourceHyp ) return false;

  const vector<SMESH_Group*>& srcGroups = _sourceHyp->GetGroups();
  if ( srcGroups.empty() )
    return error("Invalid source groups");

  vector<int> aVec(SMDSEntity_Last,0);

  bool toCopyMesh, toCopyGroups;
  _sourceHyp->GetCopySourceMesh(toCopyMesh, toCopyGroups);
  if ( toCopyMesh ) // the whole mesh is copied
  {
    vector<SMESH_Mesh*> srcMeshes = _sourceHyp->GetSourceMeshes();
    for ( unsigned i = 0; i < srcMeshes.size(); ++i )
    {
      SMESH_subMesh* sm = StdMeshers_Import_1D::getSubMeshOfCopiedMesh( theMesh, *srcMeshes[i]);
      if ( !sm || aResMap.count( sm )) continue; // already counted
      const SMDS_MeshInfo& aMeshInfo = srcMeshes[i]->GetMeshDS()->GetMeshInfo();
      for (int i = 0; i < SMDSEntity_Last; i++)
        aVec[i] = aMeshInfo.NbEntities((SMDSAbs_EntityType)i);
    }
  }
  else
  {
    // std-like iterator used to get coordinates of nodes of mesh element
    typedef SMDS_StdIterator< SMESH_TNodeXYZ, SMDS_ElemIteratorPtr > TXyzIterator;

    SMESH_MesherHelper helper(theMesh);
    helper.SetSubShape(theShape);

    const TopoDS_Face& geomFace = TopoDS::Face( theShape );

    // take into account nodes on vertices
    TopExp_Explorer exp( theShape, TopAbs_VERTEX );
    for ( ; exp.More(); exp.Next() )
      theMesh.GetSubMesh( exp.Current())->Evaluate( aResMap );

    // to count now many times a link between nodes encounters,
    // negative nb additionally means that a link is quadratic
    map<SMESH_TLink, int> linkCount;
    map<SMESH_TLink, int>::iterator link2Nb;

    // count faces and nodes imported from groups
    set<const SMDS_MeshNode* > allNodes;
    gp_XY uv;
    double minGroupTol = 1e100;
    for ( int iG = 0; iG < srcGroups.size(); ++iG )
    {
      const SMESHDS_GroupBase* srcGroup = srcGroups[iG]->GetGroupDS();
      const double groupTol = 0.5 * sqrt( getMinElemSize2( srcGroup ));
      minGroupTol = std::min( groupTol, minGroupTol );
      SMDS_ElemIteratorPtr srcElems = srcGroup->GetElements();
      SMDS_MeshNode *tmpNode =helper.AddNode(0,0,0);
      while ( srcElems->more() ) // loop on group contents
      {
        const SMDS_MeshElement* face = srcElems->next();
        // find out if face is located on geomEdge by projecting
        // a gravity center of face to geomFace
        gp_XYZ gc(0,0,0);
        gc = accumulate( TXyzIterator(face->nodesIterator()), TXyzIterator(), gc)/face->NbNodes();
        tmpNode->setXYZ( gc.X(), gc.Y(), gc.Z());
        if ( helper.CheckNodeUV( geomFace, tmpNode, uv, groupTol, /*force=*/true ))
        {
          ++aVec[ face->GetEntityType() ];

          // collect links
          int nbConers = face->NbCornerNodes();
          for ( int i = 0; i < face->NbNodes(); ++i )
          {
            const SMDS_MeshNode* n1 = face->GetNode(i);
            allNodes.insert( n1 );
            if ( i < nbConers )
            {
              const SMDS_MeshNode* n2 = face->GetNode( (i+1)%nbConers );
              link2Nb = linkCount.insert( make_pair( SMESH_TLink( n1, n2 ), 0)).first;
              if ( (*link2Nb).second )
                link2Nb->second += (link2Nb->second < 0 ) ? -1 : 1;
              else
                link2Nb->second += ( face->IsQuadratic() ) ? -1 : 1;
            }
          }
        }
      }
      helper.GetMeshDS()->RemoveNode(tmpNode);
    }

    int nbNodes = allNodes.size();
    allNodes.clear();

    // count nodes and edges on geom edges

    double u;
    for ( exp.Init(theShape, TopAbs_EDGE); exp.More(); exp.Next() )
    {
      TopoDS_Edge geomEdge = TopoDS::Edge( exp.Current() );
      SMESH_subMesh* sm = theMesh.GetSubMesh( geomEdge );
      vector<int>& edgeVec = aResMap[sm];
      if ( edgeVec.empty() )
      {
        edgeVec.resize(SMDSEntity_Last,0);
        for ( link2Nb = linkCount.begin(); link2Nb != linkCount.end(); )
        {
          const SMESH_TLink& link = (*link2Nb).first;
          int nbFacesOfLink = Abs( link2Nb->second );
          bool eraseLink = ( nbFacesOfLink != 1 );
          if ( nbFacesOfLink == 1 )
          {
            if ( helper.CheckNodeU( geomEdge, link.node1(), u, minGroupTol, /*force=*/true )&&
                 helper.CheckNodeU( geomEdge, link.node2(), u, minGroupTol, /*force=*/true ))
            {
              bool isQuadratic = ( link2Nb->second < 0 );
              ++edgeVec[ isQuadratic ? SMDSEntity_Quad_Edge : SMDSEntity_Edge ];
              ++edgeVec[ SMDSEntity_Node ];
              --nbNodes;
              eraseLink = true;
            }
          }
          if ( eraseLink )
            linkCount.erase(link2Nb++);
          else
            link2Nb++;
        }
        if ( edgeVec[ SMDSEntity_Node] > 0 )
          --edgeVec[ SMDSEntity_Node ]; // for one node on vertex
      }
      else if ( !helper.IsSeamShape( geomEdge ) ||
                geomEdge.Orientation() == TopAbs_FORWARD )
      {
        nbNodes -= 1+edgeVec[ SMDSEntity_Node ];
      }
    }

    aVec[SMDSEntity_Node] = nbNodes;
  }

  SMESH_subMesh * sm = theMesh.GetSubMesh(theShape);
  aResMap.insert(make_pair(sm,aVec));

  return true;
}
