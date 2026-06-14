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
// File      : StdMeshers_Projection_1D2D.cxx
// Module    : SMESH
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_Projection_1D2D.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_ProjectionSource2D.hxx"
#include "StdMeshers_ProjectionUtils.hxx"

#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>

using namespace std;

namespace
{
  // --------------------------------------------------------------------------------
  /*!
   * \brief an event listener updating submehses of EDGEs according to
   *        events on the target FACE submesh
   */
  struct EventProparatorToEdges : public SMESH_subMeshEventListener
  {
    EventProparatorToEdges(): SMESH_subMeshEventListener(/*isDeletable=*/false,
                                                         "Projection_1D2D::EventProparatorToEdges")
    {}
    static EventProparatorToEdges* Instance() { static EventProparatorToEdges E; return &E; }

    static void Set(SMESH_subMesh* faceSubMesh)
    {
      SMESH_subMeshEventListenerData* edgeSubMeshes =
        new SMESH_subMeshEventListenerData(/*isDeletable=*/true);
      SMESH_Mesh* mesh = faceSubMesh->GetFather();
      TopExp_Explorer eExp( faceSubMesh->GetSubShape(), TopAbs_EDGE );
      for ( ; eExp.More(); eExp.Next() )
        edgeSubMeshes->mySubMeshes.push_back( mesh->GetSubMesh( eExp.Current() ));

      // set a listener
      faceSubMesh->SetEventListener( Instance(), edgeSubMeshes, faceSubMesh );
    }
  };
  // --------------------------------------------------------------------------------
  /*!
   * \brief Structure used to temporary remove EventProparatorToEdges from faceSubMesh
   *  in order to prevent propagation of CLEAN event from FACE to EDGEs during 
   *  StdMeshers_Projection_1D2D::Compute(). The CLEAN event is emmited by Pattern mapper
   *  and causes removal of faces generated on adjacent FACEs.
   */
  struct UnsetterOfEventProparatorToEdges
  {
    SMESH_subMesh* _faceSubMesh;
    UnsetterOfEventProparatorToEdges( SMESH_subMesh* faceSubMesh ):_faceSubMesh(faceSubMesh)
    {
      faceSubMesh->DeleteEventListener( EventProparatorToEdges::Instance() );
    }
    ~UnsetterOfEventProparatorToEdges()
    {
      EventProparatorToEdges::Set(_faceSubMesh);
    }
  };
}

//=======================================================================
//function : StdMeshers_Projection_1D2D
//purpose  : 
//=======================================================================

StdMeshers_Projection_1D2D::StdMeshers_Projection_1D2D(int hypId, int studyId, SMESH_Gen* gen)
  :StdMeshers_Projection_2D(hypId, studyId, gen)
{
  _name = "Projection_1D2D";
  _requireDiscreteBoundary = false;
  _supportSubmeshes = true;
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_Projection_1D2D::Compute(SMESH_Mesh& theMesh, const TopoDS_Shape& theShape)
{
  UnsetterOfEventProparatorToEdges eventBarrier( theMesh.GetSubMesh( theShape ));

  // 1) Project faces

  if ( !StdMeshers_Projection_2D::Compute(theMesh, theShape))
    return false;

  // 2) Create segments

  SMESHDS_Mesh * meshDS = theMesh.GetMeshDS();

  SMESHDS_SubMesh * faceSubMesh = meshDS->MeshElements( theShape );
  if ( !faceSubMesh || faceSubMesh->NbElements() == 0 ) return false;
  _quadraticMesh = faceSubMesh->GetElements()->next()->IsQuadratic();

  SMESH_MesherHelper helper( theMesh );
  helper.SetSubShape( theShape );

  if ( _quadraticMesh )
  {
    // 2a) Move some medium nodes from FACE to EDGES. They are on FACE because
    // EDGEs are discreteized later than FACE, in this case.

    SMESH_MesherHelper posFixer( theMesh );
    posFixer.ToFixNodeParameters( true );
    SMDS_ElemIteratorPtr fIt = faceSubMesh->GetElements();
    vector< const SMDS_MeshNode* > nodes;
    double dummyU, tol = 1e-7;
    while ( fIt->more() ) // loop on mesh faces created by StdMeshers_Projection_2D
    {
      const SMDS_MeshElement* f = fIt->next();
      //if ( !f->IsQuadratic() ) continue;
      nodes.assign( SMDS_MeshElement::iterator( f->interlacedNodesElemIterator() ),
                    SMDS_MeshElement::iterator() );
      nodes.push_back( nodes[0] );
      for ( size_t i = 2; i < nodes.size(); i += 2 )
      {
        pair<int, TopAbs_ShapeEnum> idType = helper.GetMediumPos( nodes[i], nodes[i-2] );
        if ( idType.second == TopAbs_EDGE &&
             idType.first  != nodes[i-1]->getshapeId() )
        {
          faceSubMesh->RemoveNode( nodes[i-1], /*isDeleted=*/false );
          meshDS->SetNodeOnEdge( (SMDS_MeshNode*) nodes[i-1], idType.first );
          posFixer.SetSubShape( idType.first );
          posFixer.CheckNodeU( TopoDS::Edge( posFixer.GetSubShape() ),
                               nodes[i-1], dummyU=0., tol, /*force=*/true );
        }
      }
    }
  }
  TopoDS_Face F = TopoDS::Face( theShape );
  TError err;
  TSideVector wires = StdMeshers_FaceSide::GetFaceWires( F, theMesh,
                                                         /*ignoreMediumNodes=*/false, err);
  if ( err && !err->IsOK() )
    return error( err );

  for ( size_t iWire = 0; iWire < wires.size(); ++iWire )
  {
    vector<const SMDS_MeshNode*> nodes = wires[ iWire ]->GetOrderedNodes();
    if ( nodes.empty() )
      return error("Wrong nodes on a wire");

    // check that all nodes are shared by faces generated on F
    for ( size_t i = 0; i < nodes.size(); ++i )
    {
      SMDS_ElemIteratorPtr fIt = nodes[i]->GetInverseElementIterator(SMDSAbs_Face);
      bool faceFound = false;
      while ( !faceFound && fIt->more() )
        faceFound = ( helper.GetSubShapeID() == fIt->next()->getshapeId() );
      if ( !faceFound )
        return error("The existing 1D mesh mismatches the generated 2D mesh");
    }

    const bool checkExisting = ( wires[ iWire ]->NbSegments() || helper.HasSeam() );

    if ( _quadraticMesh )
    {
      for ( size_t i = 2; i < nodes.size(); i += 2 )
      {
        if ( checkExisting && meshDS->FindEdge( nodes[i-2], nodes[i], nodes[i-1]))
          continue;
        SMDS_MeshElement* e = meshDS->AddEdge( nodes[i-2], nodes[i], nodes[i-1] );
        meshDS->SetMeshElementOnShape( e, nodes[i-1]->getshapeId() );
      }
    }
    else
    {
      int edgeID = meshDS->ShapeToIndex( wires[ iWire ]->Edge(0) );
      for ( size_t i = 1; i < nodes.size(); ++i )
      {
        if ( checkExisting && meshDS->FindEdge( nodes[i-1], nodes[i]))
          continue;
        SMDS_MeshElement* e = meshDS->AddEdge( nodes[i-1], nodes[i] );
        if ( nodes[i-1]->getshapeId() != edgeID &&
             nodes[i  ]->getshapeId() != edgeID )
        {
          edgeID = helper.GetMediumPos( nodes[i-1], nodes[i] ).first;
          if ( edgeID < 1 ) edgeID = helper.GetSubShapeID();
        }
        meshDS->SetMeshElementOnShape( e, edgeID );
      }
    }
  }

  return true;
}

//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================

bool StdMeshers_Projection_1D2D::Evaluate(SMESH_Mesh&         theMesh,
                                          const TopoDS_Shape& theShape,
                                          MapShapeNbElems&    aResMap)
{
  if ( !StdMeshers_Projection_2D::Evaluate(theMesh,theShape,aResMap))
    return false;

  TopoDS_Shape srcFace = _sourceHypo->GetSourceFace();
  SMESH_Mesh * srcMesh = _sourceHypo->GetSourceMesh();
  if ( !srcMesh ) srcMesh = & theMesh;
  SMESH_subMesh* srcFaceSM = srcMesh->GetSubMesh( srcFace );

  namespace SPU = StdMeshers_ProjectionUtils;
  SPU::TShapeShapeMap shape2ShapeMap;
  SPU::InitVertexAssociation( _sourceHypo, shape2ShapeMap );
  if ( !SPU::FindSubShapeAssociation( theShape, &theMesh, srcFace, srcMesh, shape2ShapeMap))
    return error(COMPERR_BAD_SHAPE,"Topology of source and target faces seems different" );

  MapShapeNbElems srcResMap;
  if ( !srcFaceSM->IsMeshComputed() )
    _gen->Evaluate( *srcMesh, srcFace, srcResMap);

  SMESH_subMeshIteratorPtr smIt = srcFaceSM->getDependsOnIterator(/*includeSelf=*/false,
                                                                  /*complexShapeFirst=*/true);
  while ( smIt->more() )
  {
    SMESH_subMesh* srcSM = smIt->next();
    TopAbs_ShapeEnum shapeType = srcSM->GetSubShape().ShapeType();
    if ( shapeType == TopAbs_EDGE )
    {
      std::vector<int> aVec;
      SMESHDS_SubMesh* srcSubMeshDS = srcSM->GetSubMeshDS();
      if ( srcSubMeshDS && srcSubMeshDS->NbElements() )
      {
        aVec.resize(SMDSEntity_Last, 0);
        SMDS_ElemIteratorPtr eIt = srcSubMeshDS->GetElements();
        _quadraticMesh = ( eIt->more() && eIt->next()->IsQuadratic() );

        aVec[SMDSEntity_Node] = srcSubMeshDS->NbNodes();
        aVec[_quadraticMesh ? SMDSEntity_Quad_Edge : SMDSEntity_Edge] = srcSubMeshDS->NbElements();
      }
      else
      {
        if ( srcResMap.empty() )
          if ( !_gen->Evaluate( *srcMesh, srcSM->GetSubShape(), srcResMap ))
            return error(COMPERR_BAD_INPUT_MESH,"Source mesh not evaluatable");
        aVec = srcResMap[ srcSM ];
        if ( aVec.empty() )
          return error(COMPERR_BAD_INPUT_MESH,"Source mesh is wrongly evaluated");
      }
      TopoDS_Shape tgtEdge = shape2ShapeMap( srcSM->GetSubShape(), /*isSrc=*/true  );
      SMESH_subMesh* tgtSM = theMesh.GetSubMesh( tgtEdge );
      aResMap.insert(std::make_pair(tgtSM,aVec));
    }
    if ( shapeType == TopAbs_VERTEX ) break;
  }

  return true;
}

//=======================================================================
//function : SetEventListener
//purpose  : Sets a default event listener to submesh of the source face.
//           faceSubMesh - submesh where algo is set
// After being set, event listener is notified on each event of a submesh.
// This method is called when a submesh gets HYP_OK algo_state.
// Arranges that CLEAN event is translated from source submesh to
// the faceSubMesh submesh.
//=======================================================================

void StdMeshers_Projection_1D2D::SetEventListener(SMESH_subMesh* faceSubMesh)
{
  // set a listener of events on a source submesh
  StdMeshers_Projection_2D::SetEventListener(faceSubMesh);

  // set a listener to the target FACE submesh in order to update submehses
  // of EDGEs according to events on the target FACE submesh
  EventProparatorToEdges::Set( faceSubMesh );
}

