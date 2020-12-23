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

// File      : SMESH_MeshEditor.cxx
// Created   : Mon Apr 12 16:10:22 2004
// Author    : Edward AGAPOV (eap)

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif

#include "SMESH_MeshEditor.hxx"

#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMDS_EdgePosition.hxx"
#include "SMDS_FacePosition.hxx"
#include "SMDS_SpacePosition.hxx"
#include "SMDS_MeshGroup.hxx"
#include "SMDS_LinearEdge.hxx"
#include "SMDS_Downward.hxx"
#include "SMDS_SetIterator.hxx"

#include "SMESHDS_Group.hxx"
#include "SMESHDS_Mesh.hxx"

#include "SMESH_Algo.hxx"
#include "SMESH_ControlsDef.hxx"
#include "SMESH_Group.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_OctreeNode.hxx"
#include "SMESH_subMesh.hxx"

#include <Basics_OCCTVersion.hxx>

#include "utilities.h"
#include "chrono.hxx"

#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRep_Tool.hxx>
#include <ElCLib.hxx>
#include <Extrema_GenExtPS.hxx>
#include <Extrema_POnCurv.hxx>
#include <Extrema_POnSurf.hxx>
#include <Geom2d_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Precision.hxx>
#include <TColStd_ListOfInteger.hxx>
#include <TopAbs_State.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_SequenceOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <gp.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_XY.hxx>
#include <gp_XYZ.hxx>

#include <cmath>

#include <map>
#include <set>
#include <numeric>
#include <limits>
#include <algorithm>
#include <sstream>

#include <boost/tuple/tuple.hpp>

#include <Standard_Failure.hxx>
#include <Standard_ErrorHandler.hxx>

#define cast2Node(elem) static_cast<const SMDS_MeshNode*>( elem )

using namespace std;
using namespace SMESH::Controls;

namespace
{
  template < class ELEM_SET >
  SMDS_ElemIteratorPtr elemSetIterator( const ELEM_SET& elements )
  {
    typedef SMDS_SetIterator
      < SMDS_pElement, typename ELEM_SET::const_iterator> TSetIterator;
    return SMDS_ElemIteratorPtr( new TSetIterator( elements.begin(), elements.end() ));
  }
}

//=======================================================================
//function : SMESH_MeshEditor
//purpose  :
//=======================================================================

SMESH_MeshEditor::SMESH_MeshEditor( SMESH_Mesh* theMesh )
  :myMesh( theMesh ) // theMesh may be NULL
{
}

//================================================================================
/*!
 * \brief Clears myLastCreatedNodes and myLastCreatedElems
 */
//================================================================================

void SMESH_MeshEditor::ClearLastCreated()
{
  myLastCreatedNodes.Clear();
  myLastCreatedElems.Clear();
}

//================================================================================
/*!
 * \brief Initializes members by an existing element
 *  \param [in] elem - the source element
 *  \param [in] basicOnly - if true, does not set additional data of Ball and Polyhedron
 */
//================================================================================

SMESH_MeshEditor::ElemFeatures&
SMESH_MeshEditor::ElemFeatures::Init( const SMDS_MeshElement* elem, bool basicOnly )
{
  if ( elem )
  {
    myType = elem->GetType();
    if ( myType == SMDSAbs_Face || myType == SMDSAbs_Volume )
    {
      myIsPoly = elem->IsPoly();
      if ( myIsPoly )
      {
        myIsQuad = elem->IsQuadratic();
        if ( myType == SMDSAbs_Volume && !basicOnly )
        {
          vector<int > quant = static_cast<const SMDS_VtkVolume* >( elem )->GetQuantities();
          myPolyhedQuantities.swap( quant );
        }
      }
    }
    else if ( myType == SMDSAbs_Ball && !basicOnly )
    {
      myBallDiameter = static_cast<const SMDS_BallElement*>(elem)->GetDiameter();
    }
  }
  return *this;
}

//=======================================================================
/*!
 * \brief Add element
 */
//=======================================================================

SMDS_MeshElement*
SMESH_MeshEditor::AddElement(const vector<const SMDS_MeshNode*> & node,
                             const ElemFeatures&                  features)
{
  SMDS_MeshElement* e = 0;
  int nbnode = node.size();
  SMESHDS_Mesh* mesh = GetMeshDS();
  const int ID = features.myID;

  switch ( features.myType ) {
  case SMDSAbs_Face:
    if ( !features.myIsPoly ) {
      if      (nbnode == 3) {
        if ( ID >= 1 ) e = mesh->AddFaceWithID(node[0], node[1], node[2], ID);
        else           e = mesh->AddFace      (node[0], node[1], node[2] );
      }
      else if (nbnode == 4) {
        if ( ID >= 1 ) e = mesh->AddFaceWithID(node[0], node[1], node[2], node[3], ID);
        else           e = mesh->AddFace      (node[0], node[1], node[2], node[3] );
      }
      else if (nbnode == 6) {
        if ( ID >= 1 ) e = mesh->AddFaceWithID(node[0], node[1], node[2], node[3],
                                               node[4], node[5], ID);
        else           e = mesh->AddFace      (node[0], node[1], node[2], node[3],
                                               node[4], node[5] );
      }
      else if (nbnode == 7) {
        if ( ID >= 1 ) e = mesh->AddFaceWithID(node[0], node[1], node[2], node[3],
                                               node[4], node[5], node[6], ID);
        else           e = mesh->AddFace      (node[0], node[1], node[2], node[3],
                                               node[4], node[5], node[6] );
      }
      else if (nbnode == 8) {
        if ( ID >= 1 ) e = mesh->AddFaceWithID(node[0], node[1], node[2], node[3],
                                               node[4], node[5], node[6], node[7], ID);
        else           e = mesh->AddFace      (node[0], node[1], node[2], node[3],
                                               node[4], node[5], node[6], node[7] );
      }
      else if (nbnode == 9) {
        if ( ID >= 1 ) e = mesh->AddFaceWithID(node[0], node[1], node[2], node[3],
                                               node[4], node[5], node[6], node[7], node[8], ID);
        else           e = mesh->AddFace      (node[0], node[1], node[2], node[3],
                                               node[4], node[5], node[6], node[7], node[8] );
      }
    }
    else if ( !features.myIsQuad )
    {
      if ( ID >= 1 ) e = mesh->AddPolygonalFaceWithID(node, ID);
      else           e = mesh->AddPolygonalFace      (node    );
    }
    else if ( nbnode % 2 == 0 ) // just a protection
    {
      if ( ID >= 1 ) e = mesh->AddQuadPolygonalFaceWithID(node, ID);
      else           e = mesh->AddQuadPolygonalFace      (node    );
    }
    break;

  case SMDSAbs_Volume:
    if ( !features.myIsPoly ) {
      if      (nbnode == 4) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3], ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3] );
      }
      else if (nbnode == 5) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4] );
      }
      else if (nbnode == 6) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], node[5], ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4], node[5] );
      }
      else if (nbnode == 8) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7], ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7] );
      }
      else if (nbnode == 10) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9] );
      }
      else if (nbnode == 12) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10], node[11], ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10], node[11] );
      }
      else if (nbnode == 13) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10],node[11],
                                                 node[12],ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10],node[11],
                                                 node[12] );
      }
      else if (nbnode == 15) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10],node[11],
                                                 node[12],node[13],node[14],ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10],node[11],
                                                 node[12],node[13],node[14] );
      }
      else if (nbnode == 20) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10],node[11],
                                                 node[12],node[13],node[14],node[15],
                                                 node[16],node[17],node[18],node[19],ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10],node[11],
                                                 node[12],node[13],node[14],node[15],
                                                 node[16],node[17],node[18],node[19] );
      }
      else if (nbnode == 27) {
        if ( ID >= 1 ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10],node[11],
                                                 node[12],node[13],node[14],node[15],
                                                 node[16],node[17],node[18],node[19],
                                                 node[20],node[21],node[22],node[23],
                                                 node[24],node[25],node[26], ID);
        else           e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                                 node[4], node[5], node[6], node[7],
                                                 node[8], node[9], node[10],node[11],
                                                 node[12],node[13],node[14],node[15],
                                                 node[16],node[17],node[18],node[19],
                                                 node[20],node[21],node[22],node[23],
                                                 node[24],node[25],node[26] );
      }
    }
    else if ( !features.myIsQuad )
    {
      if ( ID >= 1 ) e = mesh->AddPolyhedralVolumeWithID(node, features.myPolyhedQuantities, ID);
      else           e = mesh->AddPolyhedralVolume      (node, features.myPolyhedQuantities    );
    }
    else
    {
      // if ( ID >= 1 ) e = mesh->AddQuadPolyhedralVolumeWithID(node, features.myPolyhedQuantities,ID);
      // else           e = mesh->AddQuadPolyhedralVolume      (node, features.myPolyhedQuantities   );
    }
    break;

  case SMDSAbs_Edge:
    if ( nbnode == 2 ) {
      if ( ID >= 1 ) e = mesh->AddEdgeWithID(node[0], node[1], ID);
      else           e = mesh->AddEdge      (node[0], node[1] );
    }
    else if ( nbnode == 3 ) {
      if ( ID >= 1 ) e = mesh->AddEdgeWithID(node[0], node[1], node[2], ID);
      else           e = mesh->AddEdge      (node[0], node[1], node[2] );
    }
    break;

  case SMDSAbs_0DElement:
    if ( nbnode == 1 ) {
      if ( ID >= 1 ) e = mesh->Add0DElementWithID(node[0], ID);
      else           e = mesh->Add0DElement      (node[0] );
    }
    break;

  case SMDSAbs_Node:
    if ( ID >= 1 ) e = mesh->AddNodeWithID(node[0]->X(), node[0]->Y(), node[0]->Z(), ID);
    else           e = mesh->AddNode      (node[0]->X(), node[0]->Y(), node[0]->Z()    );
    break;

  case SMDSAbs_Ball:
    if ( ID >= 1 ) e = mesh->AddBallWithID(node[0], features.myBallDiameter, ID);
    else           e = mesh->AddBall      (node[0], features.myBallDiameter    );
    break;

  default:;
  }
  if ( e ) myLastCreatedElems.Append( e );
  return e;
}

//=======================================================================
/*!
 * \brief Add element
 */
//=======================================================================

SMDS_MeshElement* SMESH_MeshEditor::AddElement(const vector<int> & nodeIDs,
                                               const ElemFeatures& features)
{
  vector<const SMDS_MeshNode*> nodes;
  nodes.reserve( nodeIDs.size() );
  vector<int>::const_iterator id = nodeIDs.begin();
  while ( id != nodeIDs.end() ) {
    if ( const SMDS_MeshNode* node = GetMeshDS()->FindNode( *id++ ))
      nodes.push_back( node );
    else
      return 0;
  }
  return AddElement( nodes, features );
}

//=======================================================================
//function : Remove
//purpose  : Remove a node or an element.
//           Modify a compute state of sub-meshes which become empty
//=======================================================================

int SMESH_MeshEditor::Remove (const list< int >& theIDs,
                              const bool         isNodes )
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  SMESHDS_Mesh* aMesh = GetMeshDS();
  set< SMESH_subMesh *> smmap;

  int removed = 0;
  list<int>::const_iterator it = theIDs.begin();
  for ( ; it != theIDs.end(); it++ ) {
    const SMDS_MeshElement * elem;
    if ( isNodes )
      elem = aMesh->FindNode( *it );
    else
      elem = aMesh->FindElement( *it );
    if ( !elem )
      continue;

    // Notify VERTEX sub-meshes about modification
    if ( isNodes ) {
      const SMDS_MeshNode* node = cast2Node( elem );
      if ( node->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX )
        if ( int aShapeID = node->getshapeId() )
          if ( SMESH_subMesh * sm = GetMesh()->GetSubMeshContaining( aShapeID ) )
            smmap.insert( sm );
    }
    // Find sub-meshes to notify about modification
    //     SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
    //     while ( nodeIt->more() ) {
    //       const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
    //       const SMDS_PositionPtr& aPosition = node->GetPosition();
    //       if ( aPosition.get() ) {
    //         if ( int aShapeID = aPosition->GetShapeId() ) {
    //           if ( SMESH_subMesh * sm = GetMesh()->GetSubMeshContaining( aShapeID ) )
    //             smmap.insert( sm );
    //         }
    //       }
    //     }

    // Do remove
    if ( isNodes )
      aMesh->RemoveNode( static_cast< const SMDS_MeshNode* >( elem ));
    else
      aMesh->RemoveElement( elem );
    removed++;
  }

  // Notify sub-meshes about modification
  if ( !smmap.empty() ) {
    set< SMESH_subMesh *>::iterator smIt;
    for ( smIt = smmap.begin(); smIt != smmap.end(); smIt++ )
      (*smIt)->ComputeStateEngine( SMESH_subMesh::MESH_ENTITY_REMOVED );
  }

  //   // Check if the whole mesh becomes empty
  //   if ( SMESH_subMesh * sm = GetMesh()->GetSubMeshContaining( 1 ) )
  //     sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );

  return removed;
}

//================================================================================
/*!
 * \brief Create 0D elements on all nodes of the given object except those
 *        nodes on which a 0D element already exists.
 *  \param elements - Elements on whose nodes to create 0D elements; if empty, 
 *                    the all mesh is treated
 *  \param all0DElems - returns all 0D elements found or created on nodes of \a elements
 */
//================================================================================

void SMESH_MeshEditor::Create0DElementsOnAllNodes( const TIDSortedElemSet& elements,
                                                   TIDSortedElemSet&       all0DElems )
{
  SMDS_ElemIteratorPtr elemIt;
  vector< const SMDS_MeshElement* > allNodes;
  if ( elements.empty() )
  {
    allNodes.reserve( GetMeshDS()->NbNodes() );
    elemIt = GetMeshDS()->elementsIterator( SMDSAbs_Node );
    while ( elemIt->more() )
      allNodes.push_back( elemIt->next() );

    elemIt = elemSetIterator( allNodes );
  }
  else
  {
    elemIt = elemSetIterator( elements );
  }

  while ( elemIt->more() )
  {
    const SMDS_MeshElement* e = elemIt->next();
    SMDS_ElemIteratorPtr nodeIt = e->nodesIterator();
    while ( nodeIt->more() )
    {
      const SMDS_MeshNode* n = cast2Node( nodeIt->next() );
      SMDS_ElemIteratorPtr it0D = n->GetInverseElementIterator( SMDSAbs_0DElement );
      if ( it0D->more() )
        all0DElems.insert( it0D->next() );
      else {
        myLastCreatedElems.Append( GetMeshDS()->Add0DElement( n ));
        all0DElems.insert( myLastCreatedElems.Last() );
      }
    }
  }
}

//=======================================================================
//function : FindShape
//purpose  : Return an index of the shape theElem is on
//           or zero if a shape not found
//=======================================================================

int SMESH_MeshEditor::FindShape (const SMDS_MeshElement * theElem)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  SMESHDS_Mesh * aMesh = GetMeshDS();
  if ( aMesh->ShapeToMesh().IsNull() )
    return 0;

  int aShapeID = theElem->getshapeId();
  if ( aShapeID < 1 )
    return 0;

  if ( SMESHDS_SubMesh * sm = aMesh->MeshElements( aShapeID ))
    if ( sm->Contains( theElem ))
      return aShapeID;

  if ( theElem->GetType() == SMDSAbs_Node ) {
    MESSAGE( ":( Error: invalid myShapeId of node " << theElem->GetID() );
  }
  else {
    MESSAGE( ":( Error: invalid myShapeId of element " << theElem->GetID() );
  }

  TopoDS_Shape aShape; // the shape a node of theElem is on
  if ( theElem->GetType() != SMDSAbs_Node )
  {
    SMDS_ElemIteratorPtr nodeIt = theElem->nodesIterator();
    while ( nodeIt->more() ) {
      const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
      if ((aShapeID = node->getshapeId()) > 0) {
        if ( SMESHDS_SubMesh * sm = aMesh->MeshElements( aShapeID ) ) {
          if ( sm->Contains( theElem ))
            return aShapeID;
          if ( aShape.IsNull() )
            aShape = aMesh->IndexToShape( aShapeID );
        }
      }
    }
  }

  // None of nodes is on a proper shape,
  // find the shape among ancestors of aShape on which a node is
  if ( !aShape.IsNull() ) {
    TopTools_ListIteratorOfListOfShape ancIt( GetMesh()->GetAncestors( aShape ));
    for ( ; ancIt.More(); ancIt.Next() ) {
      SMESHDS_SubMesh * sm = aMesh->MeshElements( ancIt.Value() );
      if ( sm && sm->Contains( theElem ))
        return aMesh->ShapeToIndex( ancIt.Value() );
    }
  }
  else
  {
    SMESHDS_SubMeshIteratorPtr smIt = GetMeshDS()->SubMeshes();
    while ( const SMESHDS_SubMesh* sm = smIt->next() )
      if ( sm->Contains( theElem ))
        return sm->GetID();
  }

  return 0;
}

//=======================================================================
//function : IsMedium
//purpose  :
//=======================================================================

bool SMESH_MeshEditor::IsMedium(const SMDS_MeshNode*      node,
                                const SMDSAbs_ElementType typeToCheck)
{
  bool isMedium = false;
  SMDS_ElemIteratorPtr it = node->GetInverseElementIterator(typeToCheck);
  while (it->more() && !isMedium ) {
    const SMDS_MeshElement* elem = it->next();
    isMedium = elem->IsMediumNode(node);
  }
  return isMedium;
}

//=======================================================================
//function : shiftNodesQuadTria
//purpose  : Shift nodes in the array corresponded to quadratic triangle
//           example: (0,1,2,3,4,5) -> (1,2,0,4,5,3)
//=======================================================================

static void shiftNodesQuadTria(vector< const SMDS_MeshNode* >& aNodes)
{
  const SMDS_MeshNode* nd1 = aNodes[0];
  aNodes[0] = aNodes[1];
  aNodes[1] = aNodes[2];
  aNodes[2] = nd1;
  const SMDS_MeshNode* nd2 = aNodes[3];
  aNodes[3] = aNodes[4];
  aNodes[4] = aNodes[5];
  aNodes[5] = nd2;
}

//=======================================================================
//function : nbEdgeConnectivity
//purpose  : return number of the edges connected with the theNode.
//           if theEdges has connections with the other type of the
//           elements, return -1
//=======================================================================

static int nbEdgeConnectivity(const SMDS_MeshNode* theNode)
{
  // SMDS_ElemIteratorPtr elemIt = theNode->GetInverseElementIterator();
  // int nb=0;
  // while(elemIt->more()) {
  //   elemIt->next();
  //   nb++;
  // }
  // return nb;
  return theNode->NbInverseElements();
}

//=======================================================================
//function : getNodesFromTwoTria
//purpose  : 
//=======================================================================

static bool getNodesFromTwoTria(const SMDS_MeshElement * theTria1,
                                const SMDS_MeshElement * theTria2,
                                vector< const SMDS_MeshNode*>& N1,
                                vector< const SMDS_MeshNode*>& N2)
{
  N1.assign( theTria1->begin_nodes(), theTria1->end_nodes() );
  if ( N1.size() < 6 ) return false;
  N2.assign( theTria2->begin_nodes(), theTria2->end_nodes() );
  if ( N2.size() < 6 ) return false;

  int sames[3] = {-1,-1,-1};
  int nbsames = 0;
  int i, j;
  for(i=0; i<3; i++) {
    for(j=0; j<3; j++) {
      if(N1[i]==N2[j]) {
        sames[i] = j;
        nbsames++;
        break;
      }
    }
  }
  if(nbsames!=2) return false;
  if(sames[0]>-1) {
    shiftNodesQuadTria(N1);
    if(sames[1]>-1) {
      shiftNodesQuadTria(N1);
    }
  }
  i = sames[0] + sames[1] + sames[2];
  for(; i<2; i++) {
    shiftNodesQuadTria(N2);
  }
  // now we receive following N1 and N2 (using numeration as in the image below)
  // tria1 : (1 2 4 5 9 7)  and  tria2 : (3 4 2 8 9 6)
  // i.e. first nodes from both arrays form a new diagonal
  return true;
}

//=======================================================================
//function : InverseDiag
//purpose  : Replace two neighbour triangles with ones built on the same 4 nodes
//           but having other common link.
//           Return False if args are improper
//=======================================================================

bool SMESH_MeshEditor::InverseDiag (const SMDS_MeshElement * theTria1,
                                    const SMDS_MeshElement * theTria2 )
{
  MESSAGE("InverseDiag");
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if (!theTria1 || !theTria2)
    return false;

  const SMDS_VtkFace* F1 = dynamic_cast<const SMDS_VtkFace*>( theTria1 );
  if (!F1) return false;
  const SMDS_VtkFace* F2 = dynamic_cast<const SMDS_VtkFace*>( theTria2 );
  if (!F2) return false;
  if ((theTria1->GetEntityType() == SMDSEntity_Triangle) &&
      (theTria2->GetEntityType() == SMDSEntity_Triangle)) {

    //  1 +--+ A  theTria1: ( 1 A B ) A->2 ( 1 2 B ) 1 +--+ A
    //    | /|    theTria2: ( B A 2 ) B->1 ( 1 A 2 )   |\ |
    //    |/ |                                         | \|
    //  B +--+ 2                                     B +--+ 2

    // put nodes in array and find out indices of the same ones
    const SMDS_MeshNode* aNodes [6];
    int sameInd [] = { -1, -1, -1, -1, -1, -1 };
    int i = 0;
    SMDS_ElemIteratorPtr it = theTria1->nodesIterator();
    while ( it->more() ) {
      aNodes[ i ] = static_cast<const SMDS_MeshNode*>( it->next() );

      if ( i > 2 ) // theTria2
        // find same node of theTria1
        for ( int j = 0; j < 3; j++ )
          if ( aNodes[ i ] == aNodes[ j ]) {
            sameInd[ j ] = i;
            sameInd[ i ] = j;
            break;
          }
      // next
      i++;
      if ( i == 3 ) {
        if ( it->more() )
          return false; // theTria1 is not a triangle
        it = theTria2->nodesIterator();
      }
      if ( i == 6 && it->more() )
        return false; // theTria2 is not a triangle
    }

    // find indices of 1,2 and of A,B in theTria1
    int iA = -1, iB = 0, i1 = 0, i2 = 0;
    for ( i = 0; i < 6; i++ ) {
      if ( sameInd [ i ] == -1 ) {
        if ( i < 3 ) i1 = i;
        else         i2 = i;
      }
      else if (i < 3) {
        if ( iA >= 0) iB = i;
        else          iA = i;
      }
    }
    // nodes 1 and 2 should not be the same
    if ( aNodes[ i1 ] == aNodes[ i2 ] )
      return false;

    // theTria1: A->2
    aNodes[ iA ] = aNodes[ i2 ];
    // theTria2: B->1
    aNodes[ sameInd[ iB ]] = aNodes[ i1 ];

    GetMeshDS()->ChangeElementNodes( theTria1, aNodes, 3 );
    GetMeshDS()->ChangeElementNodes( theTria2, &aNodes[ 3 ], 3 );

    return true;

  } // end if(F1 && F2)

  // check case of quadratic faces
  if (theTria1->GetEntityType() != SMDSEntity_Quad_Triangle &&
      theTria1->GetEntityType() != SMDSEntity_BiQuad_Triangle)
    return false;
  if (theTria2->GetEntityType() != SMDSEntity_Quad_Triangle&&
      theTria2->GetEntityType() != SMDSEntity_BiQuad_Triangle)
    return false;

  //       5
  //  1 +--+--+ 2  theTria1: (1 2 4 5 9 7) or (2 4 1 9 7 5) or (4 1 2 7 5 9)
  //    |    /|    theTria2: (2 3 4 6 8 9) or (3 4 2 8 9 6) or (4 2 3 9 6 8)
  //    |   / |
  //  7 +  +  + 6
  //    | /9  |
  //    |/    |
  //  4 +--+--+ 3
  //       8

  vector< const SMDS_MeshNode* > N1;
  vector< const SMDS_MeshNode* > N2;
  if(!getNodesFromTwoTria(theTria1,theTria2,N1,N2))
    return false;
  // now we receive following N1 and N2 (using numeration as above image)
  // tria1 : (1 2 4 5 9 7)  and  tria2 : (3 4 2 8 9 6)
  // i.e. first nodes from both arrays determ new diagonal

  vector< const SMDS_MeshNode*> N1new( N1.size() );
  vector< const SMDS_MeshNode*> N2new( N2.size() );
  N1new.back() = N1.back(); // central node of biquadratic
  N2new.back() = N2.back();
  N1new[0] = N1[0];  N2new[0] = N1[0];
  N1new[1] = N2[0];  N2new[1] = N1[1];
  N1new[2] = N2[1];  N2new[2] = N2[0];
  N1new[3] = N1[4];  N2new[3] = N1[3];
  N1new[4] = N2[3];  N2new[4] = N2[5];
  N1new[5] = N1[5];  N2new[5] = N1[4];
  // change nodes in faces
  GetMeshDS()->ChangeElementNodes( theTria1, &N1new[0], N1new.size() );
  GetMeshDS()->ChangeElementNodes( theTria2, &N2new[0], N2new.size() );

  // move the central node of biquadratic triangle
  SMESH_MesherHelper helper( *GetMesh() );
  for ( int is2nd = 0; is2nd < 2; ++is2nd )
  {
    const SMDS_MeshElement*         tria = is2nd ? theTria2 : theTria1;
    vector< const SMDS_MeshNode*>& nodes = is2nd ? N2new : N1new;
    if ( nodes.size() < 7 )
      continue;
    helper.SetSubShape( tria->getshapeId() );
    const TopoDS_Face& F = TopoDS::Face( helper.GetSubShape() );
    gp_Pnt xyz;
    if ( F.IsNull() )
    {
      xyz = ( SMESH_TNodeXYZ( nodes[3] ) +
              SMESH_TNodeXYZ( nodes[4] ) +
              SMESH_TNodeXYZ( nodes[5] )) / 3.;
    }
    else
    {
      bool checkUV;
      gp_XY uv = ( helper.GetNodeUV( F, nodes[3], nodes[2], &checkUV ) +
                   helper.GetNodeUV( F, nodes[4], nodes[0], &checkUV ) +
                   helper.GetNodeUV( F, nodes[5], nodes[1], &checkUV )) / 3.;
      TopLoc_Location loc;
      Handle(Geom_Surface) S = BRep_Tool::Surface(F,loc);
      xyz = S->Value( uv.X(), uv.Y() );
      xyz.Transform( loc );
      if ( nodes[6]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE &&  // set UV
           nodes[6]->getshapeId() > 0 )
        GetMeshDS()->SetNodeOnFace( nodes[6], nodes[6]->getshapeId(), uv.X(), uv.Y() );
    }
    GetMeshDS()->MoveNode( nodes[6], xyz.X(), xyz.Y(), xyz.Z() );
  }
  return true;
}

//=======================================================================
//function : findTriangles
//purpose  : find triangles sharing theNode1-theNode2 link
//=======================================================================

static bool findTriangles(const SMDS_MeshNode *    theNode1,
                          const SMDS_MeshNode *    theNode2,
                          const SMDS_MeshElement*& theTria1,
                          const SMDS_MeshElement*& theTria2)
{
  if ( !theNode1 || !theNode2 ) return false;

  theTria1 = theTria2 = 0;

  set< const SMDS_MeshElement* > emap;
  SMDS_ElemIteratorPtr it = theNode1->GetInverseElementIterator(SMDSAbs_Face);
  while (it->more()) {
    const SMDS_MeshElement* elem = it->next();
    if ( elem->NbCornerNodes() == 3 )
      emap.insert( elem );
  }
  it = theNode2->GetInverseElementIterator(SMDSAbs_Face);
  while (it->more()) {
    const SMDS_MeshElement* elem = it->next();
    if ( emap.count( elem )) {
      if ( !theTria1 )
      {
        theTria1 = elem;
      }
      else  
      {
        theTria2 = elem;
        // theTria1 must be element with minimum ID
        if ( theTria2->GetID() < theTria1->GetID() )
          std::swap( theTria2, theTria1 );
        return true;
      }
    }
  }
  return false;
}

//=======================================================================
//function : InverseDiag
//purpose  : Replace two neighbour triangles sharing theNode1-theNode2 link
//           with ones built on the same 4 nodes but having other common link.
//           Return false if proper faces not found
//=======================================================================

bool SMESH_MeshEditor::InverseDiag (const SMDS_MeshNode * theNode1,
                                    const SMDS_MeshNode * theNode2)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  MESSAGE( "::InverseDiag()" );

  const SMDS_MeshElement *tr1, *tr2;
  if ( !findTriangles( theNode1, theNode2, tr1, tr2 ))
    return false;

  const SMDS_VtkFace* F1 = dynamic_cast<const SMDS_VtkFace*>( tr1 );
  if (!F1) return false;
  const SMDS_VtkFace* F2 = dynamic_cast<const SMDS_VtkFace*>( tr2 );
  if (!F2) return false;
  if ((tr1->GetEntityType() == SMDSEntity_Triangle) &&
      (tr2->GetEntityType() == SMDSEntity_Triangle)) {

    //  1 +--+ A  tr1: ( 1 A B ) A->2 ( 1 2 B ) 1 +--+ A
    //    | /|    tr2: ( B A 2 ) B->1 ( 1 A 2 )   |\ |
    //    |/ |                                    | \|
    //  B +--+ 2                                B +--+ 2

    // put nodes in array
    // and find indices of 1,2 and of A in tr1 and of B in tr2
    int i, iA1 = 0, i1 = 0;
    const SMDS_MeshNode* aNodes1 [3];
    SMDS_ElemIteratorPtr it;
    for (i = 0, it = tr1->nodesIterator(); it->more(); i++ ) {
      aNodes1[ i ] = static_cast<const SMDS_MeshNode*>( it->next() );
      if ( aNodes1[ i ] == theNode1 )
        iA1 = i; // node A in tr1
      else if ( aNodes1[ i ] != theNode2 )
        i1 = i;  // node 1
    }
    int iB2 = 0, i2 = 0;
    const SMDS_MeshNode* aNodes2 [3];
    for (i = 0, it = tr2->nodesIterator(); it->more(); i++ ) {
      aNodes2[ i ] = static_cast<const SMDS_MeshNode*>( it->next() );
      if ( aNodes2[ i ] == theNode2 )
        iB2 = i; // node B in tr2
      else if ( aNodes2[ i ] != theNode1 )
        i2 = i;  // node 2
    }

    // nodes 1 and 2 should not be the same
    if ( aNodes1[ i1 ] == aNodes2[ i2 ] )
      return false;

    // tr1: A->2
    aNodes1[ iA1 ] = aNodes2[ i2 ];
    // tr2: B->1
    aNodes2[ iB2 ] = aNodes1[ i1 ];

    GetMeshDS()->ChangeElementNodes( tr1, aNodes1, 3 );
    GetMeshDS()->ChangeElementNodes( tr2, aNodes2, 3 );

    return true;
  }

  // check case of quadratic faces
  return InverseDiag(tr1,tr2);
}

//=======================================================================
//function : getQuadrangleNodes
//purpose  : fill theQuadNodes - nodes of a quadrangle resulting from
//           fusion of triangles tr1 and tr2 having shared link on
//           theNode1 and theNode2
//=======================================================================

bool getQuadrangleNodes(const SMDS_MeshNode *    theQuadNodes [],
                        const SMDS_MeshNode *    theNode1,
                        const SMDS_MeshNode *    theNode2,
                        const SMDS_MeshElement * tr1,
                        const SMDS_MeshElement * tr2 )
{
  if( tr1->NbNodes() != tr2->NbNodes() )
    return false;
  // find the 4-th node to insert into tr1
  const SMDS_MeshNode* n4 = 0;
  SMDS_ElemIteratorPtr it = tr2->nodesIterator();
  int i=0;
  while ( !n4 && i<3 ) {
    const SMDS_MeshNode * n = cast2Node( it->next() );
    i++;
    bool isDiag = ( n == theNode1 || n == theNode2 );
    if ( !isDiag )
      n4 = n;
  }
  // Make an array of nodes to be in a quadrangle
  int iNode = 0, iFirstDiag = -1;
  it = tr1->nodesIterator();
  i=0;
  while ( i<3 ) {
    const SMDS_MeshNode * n = cast2Node( it->next() );
    i++;
    bool isDiag = ( n == theNode1 || n == theNode2 );
    if ( isDiag ) {
      if ( iFirstDiag < 0 )
        iFirstDiag = iNode;
      else if ( iNode - iFirstDiag == 1 )
        theQuadNodes[ iNode++ ] = n4; // insert the 4-th node between diagonal nodes
    }
    else if ( n == n4 ) {
      return false; // tr1 and tr2 should not have all the same nodes
    }
    theQuadNodes[ iNode++ ] = n;
  }
  if ( iNode == 3 ) // diagonal nodes have 0 and 2 indices
    theQuadNodes[ iNode ] = n4;

  return true;
}

//=======================================================================
//function : DeleteDiag
//purpose  : Replace two neighbour triangles sharing theNode1-theNode2 link
//           with a quadrangle built on the same 4 nodes.
//           Return false if proper faces not found
//=======================================================================

bool SMESH_MeshEditor::DeleteDiag (const SMDS_MeshNode * theNode1,
                                   const SMDS_MeshNode * theNode2)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  MESSAGE( "::DeleteDiag()" );

  const SMDS_MeshElement *tr1, *tr2;
  if ( !findTriangles( theNode1, theNode2, tr1, tr2 ))
    return false;

  const SMDS_VtkFace* F1 = dynamic_cast<const SMDS_VtkFace*>( tr1 );
  if (!F1) return false;
  const SMDS_VtkFace* F2 = dynamic_cast<const SMDS_VtkFace*>( tr2 );
  if (!F2) return false;
  SMESHDS_Mesh * aMesh = GetMeshDS();

  if ((tr1->GetEntityType() == SMDSEntity_Triangle) &&
      (tr2->GetEntityType() == SMDSEntity_Triangle)) {

    const SMDS_MeshNode* aNodes [ 4 ];
    if ( ! getQuadrangleNodes( aNodes, theNode1, theNode2, tr1, tr2 ))
      return false;

    const SMDS_MeshElement* newElem = 0;
    newElem = aMesh->AddFace( aNodes[0], aNodes[1], aNodes[2], aNodes[3] );
    myLastCreatedElems.Append(newElem);
    AddToSameGroups( newElem, tr1, aMesh );
    int aShapeId = tr1->getshapeId();
    if ( aShapeId )
      {
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
      }
    aMesh->RemoveElement( tr1 );
    aMesh->RemoveElement( tr2 );

    return true;
  }

  // check case of quadratic faces
  if (tr1->GetEntityType() != SMDSEntity_Quad_Triangle)
    return false;
  if (tr2->GetEntityType() != SMDSEntity_Quad_Triangle)
    return false;

  //       5
  //  1 +--+--+ 2  tr1: (1 2 4 5 9 7) or (2 4 1 9 7 5) or (4 1 2 7 5 9)
  //    |    /|    tr2: (2 3 4 6 8 9) or (3 4 2 8 9 6) or (4 2 3 9 6 8)
  //    |   / |
  //  7 +  +  + 6
  //    | /9  |
  //    |/    |
  //  4 +--+--+ 3
  //       8

  vector< const SMDS_MeshNode* > N1;
  vector< const SMDS_MeshNode* > N2;
  if(!getNodesFromTwoTria(tr1,tr2,N1,N2))
    return false;
  // now we receive following N1 and N2 (using numeration as above image)
  // tria1 : (1 2 4 5 9 7)  and  tria2 : (3 4 2 8 9 6)
  // i.e. first nodes from both arrays determ new diagonal

  const SMDS_MeshNode* aNodes[8];
  aNodes[0] = N1[0];
  aNodes[1] = N1[1];
  aNodes[2] = N2[0];
  aNodes[3] = N2[1];
  aNodes[4] = N1[3];
  aNodes[5] = N2[5];
  aNodes[6] = N2[3];
  aNodes[7] = N1[5];

  const SMDS_MeshElement* newElem = 0;
  newElem = aMesh->AddFace( aNodes[0], aNodes[1], aNodes[2], aNodes[3],
                            aNodes[4], aNodes[5], aNodes[6], aNodes[7]);
  myLastCreatedElems.Append(newElem);
  AddToSameGroups( newElem, tr1, aMesh );
  int aShapeId = tr1->getshapeId();
  if ( aShapeId )
    {
      aMesh->SetMeshElementOnShape( newElem, aShapeId );
    }
  aMesh->RemoveElement( tr1 );
  aMesh->RemoveElement( tr2 );

  // remove middle node (9)
  GetMeshDS()->RemoveNode( N1[4] );

  return true;
}

//=======================================================================
//function : Reorient
//purpose  : Reverse theElement orientation
//=======================================================================

bool SMESH_MeshEditor::Reorient (const SMDS_MeshElement * theElem)
{
  MESSAGE("Reorient");
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if (!theElem)
    return false;
  SMDS_ElemIteratorPtr it = theElem->nodesIterator();
  if ( !it || !it->more() )
    return false;

  const SMDSAbs_ElementType type = theElem->GetType();
  if ( type < SMDSAbs_Edge || type > SMDSAbs_Volume )
    return false;

  const SMDSAbs_EntityType geomType = theElem->GetEntityType();
  if ( geomType == SMDSEntity_Polyhedra ) // polyhedron
  {
    const SMDS_VtkVolume* aPolyedre =
      dynamic_cast<const SMDS_VtkVolume*>( theElem );
    if (!aPolyedre) {
      MESSAGE("Warning: bad volumic element");
      return false;
    }
    const int nbFaces = aPolyedre->NbFaces();
    vector<const SMDS_MeshNode *> poly_nodes;
    vector<int> quantities (nbFaces);

    // reverse each face of the polyedre
    for (int iface = 1; iface <= nbFaces; iface++) {
      int inode, nbFaceNodes = aPolyedre->NbFaceNodes(iface);
      quantities[iface - 1] = nbFaceNodes;

      for (inode = nbFaceNodes; inode >= 1; inode--) {
        const SMDS_MeshNode* curNode = aPolyedre->GetFaceNode(iface, inode);
        poly_nodes.push_back(curNode);
      }
    }
    return GetMeshDS()->ChangePolyhedronNodes( theElem, poly_nodes, quantities );
  }
  else // other elements
  {
    vector<const SMDS_MeshNode*> nodes( theElem->begin_nodes(), theElem->end_nodes() );
    const std::vector<int>& interlace = SMDS_MeshCell::reverseSmdsOrder( geomType, nodes.size() );
    if ( interlace.empty() )
    {
      std::reverse( nodes.begin(), nodes.end() ); // obsolete, just in case
    }
    else
    {
      SMDS_MeshCell::applyInterlace( interlace, nodes );
    }
    return GetMeshDS()->ChangeElementNodes( theElem, &nodes[0], nodes.size() );
  }
  return false;
}

//================================================================================
/*!
 * \brief Reorient faces.
 * \param theFaces - the faces to reorient. If empty the whole mesh is meant
 * \param theDirection - desired direction of normal of \a theFace
 * \param theFace - one of \a theFaces that sould be oriented according to
 *        \a theDirection and whose orientation defines orientation of other faces
 * \return number of reoriented faces.
 */
//================================================================================

int SMESH_MeshEditor::Reorient2D (TIDSortedElemSet &       theFaces,
                                  const gp_Dir&            theDirection,
                                  const SMDS_MeshElement * theFace)
{
  int nbReori = 0;
  if ( !theFace || theFace->GetType() != SMDSAbs_Face ) return nbReori;

  if ( theFaces.empty() )
  {
    SMDS_FaceIteratorPtr fIt = GetMeshDS()->facesIterator(/*idInceasingOrder=*/true);
    while ( fIt->more() )
      theFaces.insert( theFaces.end(), fIt->next() );
  }

  // orient theFace according to theDirection
  gp_XYZ normal;
  SMESH_MeshAlgos::FaceNormal( theFace, normal, /*normalized=*/false );
  if ( normal * theDirection.XYZ() < 0 )
    nbReori += Reorient( theFace );

  // Orient other faces

  set< const SMDS_MeshElement* > startFaces, visitedFaces;
  TIDSortedElemSet avoidSet;
  set< SMESH_TLink > checkedLinks;
  pair< set< SMESH_TLink >::iterator, bool > linkIt_isNew;

  if ( theFaces.size() > 1 )// leave 1 face to prevent finding not selected faces
    theFaces.erase( theFace );
  startFaces.insert( theFace );

  int nodeInd1, nodeInd2;
  const SMDS_MeshElement*           otherFace;
  vector< const SMDS_MeshElement* > facesNearLink;
  vector< std::pair< int, int > >   nodeIndsOfFace;

  set< const SMDS_MeshElement* >::iterator startFace = startFaces.begin();
  while ( !startFaces.empty() )
  {
    startFace = startFaces.begin();
    theFace = *startFace;
    startFaces.erase( startFace );
    if ( !visitedFaces.insert( theFace ).second )
      continue;

    avoidSet.clear();
    avoidSet.insert(theFace);

    NLink link( theFace->GetNode( 0 ), (SMDS_MeshNode *) 0 );

    const int nbNodes = theFace->NbCornerNodes();
    for ( int i = 0; i < nbNodes; ++i ) // loop on links of theFace
    {
      link.second = theFace->GetNode(( i+1 ) % nbNodes );
      linkIt_isNew = checkedLinks.insert( link );
      if ( !linkIt_isNew.second )
      {
        // link has already been checked and won't be encountered more
        // if the group (theFaces) is manifold
        //checkedLinks.erase( linkIt_isNew.first );
      }
      else
      {
        facesNearLink.clear();
        nodeIndsOfFace.clear();
        while (( otherFace = SMESH_MeshAlgos::FindFaceInSet( link.first, link.second,
                                                             theFaces, avoidSet,
                                                             &nodeInd1, &nodeInd2 )))
          if ( otherFace != theFace)
          {
            facesNearLink.push_back( otherFace );
            nodeIndsOfFace.push_back( make_pair( nodeInd1, nodeInd2 ));
            avoidSet.insert( otherFace );
          }
        if ( facesNearLink.size() > 1 )
        {
          // NON-MANIFOLD mesh shell !
          // select a face most co-directed with theFace,
          // other faces won't be visited this time
          gp_XYZ NF, NOF;
          SMESH_MeshAlgos::FaceNormal( theFace, NF, /*normalized=*/false );
          double proj, maxProj = -1;
          for ( size_t i = 0; i < facesNearLink.size(); ++i ) {
            SMESH_MeshAlgos::FaceNormal( facesNearLink[i], NOF, /*normalized=*/false );
            if (( proj = Abs( NF * NOF )) > maxProj ) {
              maxProj = proj;
              otherFace = facesNearLink[i];
              nodeInd1  = nodeIndsOfFace[i].first;
              nodeInd2  = nodeIndsOfFace[i].second;
            }
          }
          // not to visit rejected faces
          for ( size_t i = 0; i < facesNearLink.size(); ++i )
            if ( facesNearLink[i] != otherFace && theFaces.size() > 1 )
              visitedFaces.insert( facesNearLink[i] );
        }
        else if ( facesNearLink.size() == 1 )
        {
          otherFace = facesNearLink[0];
          nodeInd1  = nodeIndsOfFace.back().first;
          nodeInd2  = nodeIndsOfFace.back().second;
        }
        if ( otherFace && otherFace != theFace)
        {
          // link must be reverse in otherFace if orientation ot otherFace
          // is same as that of theFace
          if ( abs(nodeInd2-nodeInd1) == 1 ? nodeInd2 > nodeInd1 : nodeInd1 > nodeInd2 )
          {
            nbReori += Reorient( otherFace );
          }
          startFaces.insert( otherFace );
        }
      }
      std::swap( link.first, link.second ); // reverse the link
    }
  }
  return nbReori;
}

//================================================================================
/*!
 * \brief Reorient faces basing on orientation of adjacent volumes.
 * \param theFaces - faces to reorient. If empty, all mesh faces are treated.
 * \param theVolumes - reference volumes.
 * \param theOutsideNormal - to orient faces to have their normal
 *        pointing either \a outside or \a inside the adjacent volumes.
 * \return number of reoriented faces.
 */
//================================================================================

int SMESH_MeshEditor::Reorient2DBy3D (TIDSortedElemSet & theFaces,
                                      TIDSortedElemSet & theVolumes,
                                      const bool         theOutsideNormal)
{
  int nbReori = 0;

  SMDS_ElemIteratorPtr faceIt;
  if ( theFaces.empty() )
    faceIt = GetMeshDS()->elementsIterator( SMDSAbs_Face );
  else
    faceIt = elemSetIterator( theFaces );

  vector< const SMDS_MeshNode* > faceNodes;
  TIDSortedElemSet checkedVolumes;
  set< const SMDS_MeshNode* > faceNodesSet;
  SMDS_VolumeTool volumeTool;

  while ( faceIt->more() ) // loop on given faces
  {
    const SMDS_MeshElement* face = faceIt->next();
    if ( face->GetType() != SMDSAbs_Face )
      continue;

    const int nbCornersNodes = face->NbCornerNodes();
    faceNodes.assign( face->begin_nodes(), face->end_nodes() );

    checkedVolumes.clear();
    SMDS_ElemIteratorPtr vIt = faceNodes[ 0 ]->GetInverseElementIterator( SMDSAbs_Volume );
    while ( vIt->more() )
    {
      const SMDS_MeshElement* volume = vIt->next();

      if ( !checkedVolumes.insert( volume ).second )
        continue;
      if ( !theVolumes.empty() && !theVolumes.count( volume ))
        continue;

      // is volume adjacent?
      bool allNodesCommon = true;
      for ( int iN = 1; iN < nbCornersNodes && allNodesCommon; ++iN )
        allNodesCommon = ( volume->GetNodeIndex( faceNodes[ iN ]) > -1 );
      if ( !allNodesCommon )
        continue;

      // get nodes of a corresponding volume facet
      faceNodesSet.clear();
      faceNodesSet.insert( faceNodes.begin(), faceNodes.end() );
      volumeTool.Set( volume );
      int facetID = volumeTool.GetFaceIndex( faceNodesSet );
      if ( facetID < 0 ) continue;
      volumeTool.SetExternalNormal();
      const SMDS_MeshNode** facetNodes = volumeTool.GetFaceNodes( facetID );

      // compare order of faceNodes and facetNodes
      const int iQ = 1 + ( nbCornersNodes < faceNodes.size() );
      int iNN[2];
      for ( int i = 0; i < 2; ++i )
      {
        const SMDS_MeshNode* n = facetNodes[ i*iQ ];
        for ( int iN = 0; iN < nbCornersNodes; ++iN )
          if ( faceNodes[ iN ] == n )
          {
            iNN[ i ] = iN;
            break;
          }
      }
      bool isOutside = Abs( iNN[0]-iNN[1] ) == 1 ? iNN[0] < iNN[1] : iNN[0] > iNN[1];
      if ( isOutside != theOutsideNormal )
        nbReori += Reorient( face );
    }
  }  // loop on given faces

  return nbReori;
}

//=======================================================================
//function : getBadRate
//purpose  :
//=======================================================================

static double getBadRate (const SMDS_MeshElement*               theElem,
                          SMESH::Controls::NumericalFunctorPtr& theCrit)
{
  SMESH::Controls::TSequenceOfXYZ P;
  if ( !theElem || !theCrit->GetPoints( theElem, P ))
    return 1e100;
  return theCrit->GetBadRate( theCrit->GetValue( P ), theElem->NbNodes() );
  //return theCrit->GetBadRate( theCrit->GetValue( theElem->GetID() ), theElem->NbNodes() );
}

//=======================================================================
//function : QuadToTri
//purpose  : Cut quadrangles into triangles.
//           theCrit is used to select a diagonal to cut
//=======================================================================

bool SMESH_MeshEditor::QuadToTri (TIDSortedElemSet &                   theElems,
                                  SMESH::Controls::NumericalFunctorPtr theCrit)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if ( !theCrit.get() )
    return false;

  SMESHDS_Mesh * aMesh = GetMeshDS();

  Handle(Geom_Surface) surface;
  SMESH_MesherHelper   helper( *GetMesh() );

  TIDSortedElemSet::iterator itElem;
  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ )
  {
    const SMDS_MeshElement* elem = *itElem;
    if ( !elem || elem->GetType() != SMDSAbs_Face )
      continue;
    if ( elem->NbCornerNodes() != 4 )
      continue;

    // retrieve element nodes
    vector< const SMDS_MeshNode* > aNodes( elem->begin_nodes(), elem->end_nodes() );

    // compare two sets of possible triangles
    double aBadRate1, aBadRate2; // to what extent a set is bad
    SMDS_FaceOfNodes tr1 ( aNodes[0], aNodes[1], aNodes[2] );
    SMDS_FaceOfNodes tr2 ( aNodes[2], aNodes[3], aNodes[0] );
    aBadRate1 = getBadRate( &tr1, theCrit ) + getBadRate( &tr2, theCrit );

    SMDS_FaceOfNodes tr3 ( aNodes[1], aNodes[2], aNodes[3] );
    SMDS_FaceOfNodes tr4 ( aNodes[3], aNodes[0], aNodes[1] );
    aBadRate2 = getBadRate( &tr3, theCrit ) + getBadRate( &tr4, theCrit );

    const int aShapeId = FindShape( elem );
    const SMDS_MeshElement* newElem1 = 0;
    const SMDS_MeshElement* newElem2 = 0;

    if ( !elem->IsQuadratic() ) // split liner quadrangle
    {
      // for MaxElementLength2D functor we return minimum diagonal for splitting,
      // because aBadRate1=2*len(diagonal 1-3); aBadRate2=2*len(diagonal 2-4)
      if ( aBadRate1 <= aBadRate2 ) {
        // tr1 + tr2 is better
        newElem1 = aMesh->AddFace( aNodes[2], aNodes[3], aNodes[0] );
        newElem2 = aMesh->AddFace( aNodes[2], aNodes[0], aNodes[1] );
      }
      else {
        // tr3 + tr4 is better
        newElem1 = aMesh->AddFace( aNodes[3], aNodes[0], aNodes[1] );
        newElem2 = aMesh->AddFace( aNodes[3], aNodes[1], aNodes[2] );
      }
    }
    else // split quadratic quadrangle
    {
      helper.SetIsQuadratic( true );
      helper.SetIsBiQuadratic( aNodes.size() == 9 );

      helper.AddTLinks( static_cast< const SMDS_MeshFace* >( elem ));
      if ( aNodes.size() == 9 )
      {
        helper.SetIsBiQuadratic( true );
        if ( aBadRate1 <= aBadRate2 )
          helper.AddTLinkNode( aNodes[0], aNodes[2], aNodes[8] );
        else
          helper.AddTLinkNode( aNodes[1], aNodes[3], aNodes[8] );
      }
      // create a new element
      if ( aBadRate1 <= aBadRate2 ) {
        newElem1 = helper.AddFace( aNodes[2], aNodes[3], aNodes[0] );
        newElem2 = helper.AddFace( aNodes[2], aNodes[0], aNodes[1] );
      }
      else {
        newElem1 = helper.AddFace( aNodes[3], aNodes[0], aNodes[1] );
        newElem2 = helper.AddFace( aNodes[3], aNodes[1], aNodes[2] );
      }
    } // quadratic case

    // care of a new element

    myLastCreatedElems.Append(newElem1);
    myLastCreatedElems.Append(newElem2);
    AddToSameGroups( newElem1, elem, aMesh );
    AddToSameGroups( newElem2, elem, aMesh );

    // put a new triangle on the same shape
    if ( aShapeId )
      aMesh->SetMeshElementOnShape( newElem1, aShapeId );
    aMesh->SetMeshElementOnShape( newElem2, aShapeId );

    aMesh->RemoveElement( elem );
  }
  return true;
}

//=======================================================================
/*!
 * \brief Split each of given quadrangles into 4 triangles.
 * \param theElems - The faces to be splitted. If empty all faces are split.
 */
//=======================================================================

void SMESH_MeshEditor::QuadTo4Tri (TIDSortedElemSet & theElems)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  SMESH_MesherHelper helper( *GetMesh() );
  helper.SetElementsOnShape( true );

  SMDS_ElemIteratorPtr faceIt;
  if ( theElems.empty() ) faceIt = GetMeshDS()->elementsIterator(SMDSAbs_Face);
  else                    faceIt = elemSetIterator( theElems );

  bool   checkUV;
  gp_XY  uv [9]; uv[8] = gp_XY(0,0);
  gp_XYZ xyz[9];
  vector< const SMDS_MeshNode* > nodes;
  SMESHDS_SubMesh*               subMeshDS;
  TopoDS_Face                    F;
  Handle(Geom_Surface)           surface;
  TopLoc_Location                loc;

  while ( faceIt->more() )
  {
    const SMDS_MeshElement* quad = faceIt->next();
    if ( !quad || quad->NbCornerNodes() != 4 )
      continue;

    // get a surface the quad is on

    if ( quad->getshapeId() < 1 )
    {
      F.Nullify();
      helper.SetSubShape( 0 );
      subMeshDS = 0;
    }
    else if ( quad->getshapeId() != helper.GetSubShapeID() )
    {
      helper.SetSubShape( quad->getshapeId() );
      if ( !helper.GetSubShape().IsNull() &&
           helper.GetSubShape().ShapeType() == TopAbs_FACE )
      {
        F = TopoDS::Face( helper.GetSubShape() );
        surface = BRep_Tool::Surface( F, loc );
        subMeshDS = GetMeshDS()->MeshElements( quad->getshapeId() );
      }
      else
      {
        helper.SetSubShape( 0 );
        subMeshDS = 0;
      }
    }

    // create a central node

    const SMDS_MeshNode* nCentral;
    nodes.assign( quad->begin_nodes(), quad->end_nodes() );

    if ( nodes.size() == 9 )
    {
      nCentral = nodes.back();
    }
    else
    {
      size_t iN = 0;
      if ( F.IsNull() )
      {
        for ( ; iN < nodes.size(); ++iN )
          xyz[ iN ] = SMESH_TNodeXYZ( nodes[ iN ] );

        for ( ; iN < 8; ++iN ) // mid-side points of a linear qudrangle
          xyz[ iN ] = 0.5 * ( xyz[ iN - 4 ] + xyz[( iN - 3 )%4 ] );

        xyz[ 8 ] = helper.calcTFI( 0.5, 0.5,
                                   xyz[0], xyz[1], xyz[2], xyz[3],
                                   xyz[4], xyz[5], xyz[6], xyz[7] );
      }
      else
      {
        for ( ; iN < nodes.size(); ++iN )
          uv[ iN ] = helper.GetNodeUV( F, nodes[iN], nodes[(iN+2)%4], &checkUV );

        for ( ; iN < 8; ++iN ) // UV of mid-side points of a linear qudrangle
          uv[ iN ] = helper.GetMiddleUV( surface, uv[ iN - 4 ], uv[( iN - 3 )%4 ] );

        uv[ 8 ] = helper.calcTFI( 0.5, 0.5,
                                  uv[0], uv[1], uv[2], uv[3],
                                  uv[4], uv[5], uv[6], uv[7] );

        gp_Pnt p = surface->Value( uv[8].X(), uv[8].Y() ).Transformed( loc );
        xyz[ 8 ] = p.XYZ();
      }

      nCentral = helper.AddNode( xyz[8].X(), xyz[8].Y(), xyz[8].Z(), /*id=*/0,
                                 uv[8].X(), uv[8].Y() );
      myLastCreatedNodes.Append( nCentral );
    }

    // create 4 triangles

    GetMeshDS()->RemoveFreeElement( quad, subMeshDS, /*fromGroups=*/false );
    
    helper.SetIsQuadratic  ( nodes.size() > 4 );
    helper.SetIsBiQuadratic( nodes.size() == 9 );
    if ( helper.GetIsQuadratic() )
      helper.AddTLinks( static_cast< const SMDS_MeshFace*>( quad ));

    for ( int i = 0; i < 4; ++i )
    {
      SMDS_MeshElement* tria = helper.AddFace( nodes[ i ],
                                               nodes[(i+1)%4],
                                               nCentral );
      ReplaceElemInGroups( tria, quad, GetMeshDS() );
      myLastCreatedElems.Append( tria );
    }
  }
}

//=======================================================================
//function : BestSplit
//purpose  : Find better diagonal for cutting.
//=======================================================================

int SMESH_MeshEditor::BestSplit (const SMDS_MeshElement*              theQuad,
                                 SMESH::Controls::NumericalFunctorPtr theCrit)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if (!theCrit.get())
    return -1;

  if (!theQuad || theQuad->GetType() != SMDSAbs_Face )
    return -1;

  if( theQuad->NbNodes()==4 ||
      (theQuad->NbNodes()==8 && theQuad->IsQuadratic()) ) {

    // retrieve element nodes
    const SMDS_MeshNode* aNodes [4];
    SMDS_ElemIteratorPtr itN = theQuad->nodesIterator();
    int i = 0;
    //while (itN->more())
    while (i<4) {
      aNodes[ i++ ] = static_cast<const SMDS_MeshNode*>( itN->next() );
    }
    // compare two sets of possible triangles
    double aBadRate1, aBadRate2; // to what extent a set is bad
    SMDS_FaceOfNodes tr1 ( aNodes[0], aNodes[1], aNodes[2] );
    SMDS_FaceOfNodes tr2 ( aNodes[2], aNodes[3], aNodes[0] );
    aBadRate1 = getBadRate( &tr1, theCrit ) + getBadRate( &tr2, theCrit );

    SMDS_FaceOfNodes tr3 ( aNodes[1], aNodes[2], aNodes[3] );
    SMDS_FaceOfNodes tr4 ( aNodes[3], aNodes[0], aNodes[1] );
    aBadRate2 = getBadRate( &tr3, theCrit ) + getBadRate( &tr4, theCrit );
    // for MaxElementLength2D functor we return minimum diagonal for splitting,
    // because aBadRate1=2*len(diagonal 1-3); aBadRate2=2*len(diagonal 2-4)
    if (aBadRate1 <= aBadRate2) // tr1 + tr2 is better
      return 1; // diagonal 1-3

    return 2; // diagonal 2-4
  }
  return -1;
}

namespace
{
  // Methods of splitting volumes into tetra

  const int theHexTo5_1[5*4+1] =
    {
      0, 1, 2, 5,    0, 4, 5, 7,     0, 2, 3, 7,    2, 5, 6, 7,     0, 5, 2, 7,   -1
    };
  const int theHexTo5_2[5*4+1] =
    {
      1, 2, 3, 6,    1, 4, 5, 6,     0, 1, 3, 4,    3, 4, 6, 7,     1, 3, 4, 6,   -1
    };
  const int* theHexTo5[2] = { theHexTo5_1, theHexTo5_2 };

  const int theHexTo6_1[6*4+1] =
    {
      1, 5, 6, 0,    0, 1, 2, 6,     0, 4, 5, 6,    0, 4, 6, 7,     0, 2, 3, 6,   0, 3, 7, 6,  -1
    };
  const int theHexTo6_2[6*4+1] =
    {
      2, 6, 7, 1,    1, 2, 3, 7,     1, 5, 6, 7,    1, 5, 7, 4,     1, 3, 0, 7,   1, 0, 4, 7,  -1
    };
  const int theHexTo6_3[6*4+1] =
    {
      3, 7, 4, 2,    2, 3, 0, 4,     2, 6, 7, 4,    2, 6, 4, 5,     2, 0, 1, 4,   2, 1, 5, 4,  -1
    };
  const int theHexTo6_4[6*4+1] =
    {
      0, 4, 5, 3,    3, 0, 1, 5,     3, 7, 4, 5,    3, 7, 5, 6,     3, 1, 2, 5,   3, 2, 6, 5,  -1
    };
  const int* theHexTo6[4] = { theHexTo6_1, theHexTo6_2, theHexTo6_3, theHexTo6_4 };

  const int thePyraTo2_1[2*4+1] =
    {
      0, 1, 2, 4,    0, 2, 3, 4,   -1
    };
  const int thePyraTo2_2[2*4+1] =
    {
      1, 2, 3, 4,    1, 3, 0, 4,   -1
    };
  const int* thePyraTo2[2] = { thePyraTo2_1, thePyraTo2_2 };

  const int thePentaTo3_1[3*4+1] =
    {
      0, 1, 2, 3,    1, 3, 4, 2,     2, 3, 4, 5,    -1
    };
  const int thePentaTo3_2[3*4+1] =
    {
      1, 2, 0, 4,    2, 4, 5, 0,     0, 4, 5, 3,    -1
    };
  const int thePentaTo3_3[3*4+1] =
    {
      2, 0, 1, 5,    0, 5, 3, 1,     1, 5, 3, 4,    -1
    };
  const int thePentaTo3_4[3*4+1] =
    {
      0, 1, 2, 3,    1, 3, 4, 5,     2, 3, 1, 5,    -1
    };
  const int thePentaTo3_5[3*4+1] =
    {
      1, 2, 0, 4,    2, 4, 5, 3,     0, 4, 2, 3,    -1
    };
  const int thePentaTo3_6[3*4+1] =
    {
      2, 0, 1, 5,    0, 5, 3, 4,     1, 5, 0, 4,    -1
    };
  const int* thePentaTo3[6] = { thePentaTo3_1, thePentaTo3_2, thePentaTo3_3,
                                thePentaTo3_4, thePentaTo3_5, thePentaTo3_6 };

  // Methods of splitting hexahedron into prisms

  const int theHexTo4Prisms_BT[6*4+1] = // bottom-top
    {
      0, 1, 8, 4, 5, 9,    1, 2, 8, 5, 6, 9,    2, 3, 8, 6, 7, 9,   3, 0, 8, 7, 4, 9,    -1
    };
  const int theHexTo4Prisms_LR[6*4+1] = // left-right
    {
      1, 0, 8, 2, 3, 9,    0, 4, 8, 3, 7, 9,    4, 5, 8, 7, 6, 9,   5, 1, 8, 6, 2, 9,    -1
    };
  const int theHexTo4Prisms_FB[6*4+1] = // front-back
    {
      0, 3, 9, 1, 2, 8,    3, 7, 9, 2, 6, 8,    7, 4, 9, 6, 5, 8,   4, 0, 9, 5, 1, 8,    -1
    };

  const int theHexTo2Prisms_BT_1[6*2+1] =
    {
      0, 1, 3, 4, 5, 7,    1, 2, 3, 5, 6, 7,   -1
    };
  const int theHexTo2Prisms_BT_2[6*2+1] =
    {
      0, 1, 2, 4, 5, 6,    0, 2, 3, 4, 6, 7,   -1
    };
  const int* theHexTo2Prisms_BT[2] = { theHexTo2Prisms_BT_1, theHexTo2Prisms_BT_2 };

  const int theHexTo2Prisms_LR_1[6*2+1] =
    {
      1, 0, 4, 2, 3, 7,    1, 4, 5, 2, 7, 6,   -1
    };
  const int theHexTo2Prisms_LR_2[6*2+1] =
    {
      1, 0, 4, 2, 3, 7,    1, 4, 5, 2, 7, 6,   -1
    };
  const int* theHexTo2Prisms_LR[2] = { theHexTo2Prisms_LR_1, theHexTo2Prisms_LR_2 };

  const int theHexTo2Prisms_FB_1[6*2+1] =
    {
      0, 3, 4, 1, 2, 5,    3, 7, 4, 2, 6, 5,   -1
    };
  const int theHexTo2Prisms_FB_2[6*2+1] =
    {
      0, 3, 7, 1, 2, 7,    0, 7, 4, 1, 6, 5,   -1
    };
  const int* theHexTo2Prisms_FB[2] = { theHexTo2Prisms_FB_1, theHexTo2Prisms_FB_2 };


  struct TTriangleFacet //!< stores indices of three nodes of tetra facet
  {
    int _n1, _n2, _n3;
    TTriangleFacet(int n1, int n2, int n3): _n1(n1), _n2(n2), _n3(n3) {}
    bool contains(int n) const { return ( n == _n1 || n == _n2 || n == _n3 ); }
    bool hasAdjacentVol( const SMDS_MeshElement*    elem,
                         const SMDSAbs_GeometryType geom = SMDSGeom_TETRA) const;
  };
  struct TSplitMethod
  {
    int        _nbSplits;
    int        _nbCorners;
    const int* _connectivity; //!< foursomes of tetra connectivy finished by -1
    bool       _baryNode;     //!< additional node is to be created at cell barycenter
    bool       _ownConn;      //!< to delete _connectivity in destructor
    map<int, const SMDS_MeshNode*> _faceBaryNode; //!< map face index to node at BC of face

    TSplitMethod( int nbTet=0, const int* conn=0, bool addNode=false)
      : _nbSplits(nbTet), _nbCorners(4), _connectivity(conn), _baryNode(addNode), _ownConn(false) {}
    ~TSplitMethod() { if ( _ownConn ) delete [] _connectivity; _connectivity = 0; }
    bool hasFacet( const TTriangleFacet& facet ) const
    {
      if ( _nbCorners == 4 )
      {
        const int* tetConn = _connectivity;
        for ( ; tetConn[0] >= 0; tetConn += 4 )
          if (( facet.contains( tetConn[0] ) +
                facet.contains( tetConn[1] ) +
                facet.contains( tetConn[2] ) +
                facet.contains( tetConn[3] )) == 3 )
            return true;
      }
      else // prism, _nbCorners == 6
      {
        const int* prismConn = _connectivity;
        for ( ; prismConn[0] >= 0; prismConn += 6 )
        {
          if (( facet.contains( prismConn[0] ) &&
                facet.contains( prismConn[1] ) &&
                facet.contains( prismConn[2] ))
              ||
              ( facet.contains( prismConn[3] ) &&
                facet.contains( prismConn[4] ) &&
                facet.contains( prismConn[5] )))
            return true;
        }
      }
      return false;
    }
  };

  //=======================================================================
  /*!
   * \brief return TSplitMethod for the given element to split into tetrahedra
   */
  //=======================================================================

  TSplitMethod getTetraSplitMethod( SMDS_VolumeTool& vol, const int theMethodFlags)
  {
    const int iQ = vol.Element()->IsQuadratic() ? 2 : 1;

    // at HEXA_TO_24 method, each face of volume is split into triangles each based on
    // an edge and a face barycenter; tertaherdons are based on triangles and
    // a volume barycenter
    const bool is24TetMode = ( theMethodFlags == SMESH_MeshEditor::HEXA_TO_24 );

    // Find out how adjacent volumes are split

    vector < list< TTriangleFacet > > triaSplitsByFace( vol.NbFaces() ); // splits of each side
    int hasAdjacentSplits = 0, maxTetConnSize = 0;
    for ( int iF = 0; iF < vol.NbFaces(); ++iF )
    {
      int nbNodes = vol.NbFaceNodes( iF ) / iQ;
      maxTetConnSize += 4 * ( nbNodes - (is24TetMode ? 0 : 2));
      if ( nbNodes < 4 ) continue;

      list< TTriangleFacet >& triaSplits = triaSplitsByFace[ iF ];
      const int* nInd = vol.GetFaceNodesIndices( iF );
      if ( nbNodes == 4 )
      {
        TTriangleFacet t012( nInd[0*iQ], nInd[1*iQ], nInd[2*iQ] );
        TTriangleFacet t123( nInd[1*iQ], nInd[2*iQ], nInd[3*iQ] );
        if      ( t012.hasAdjacentVol( vol.Element() )) triaSplits.push_back( t012 );
        else if ( t123.hasAdjacentVol( vol.Element() )) triaSplits.push_back( t123 );
      }
      else
      {
        int iCom = 0; // common node of triangle faces to split into
        for ( int iVar = 0; iVar < nbNodes; ++iVar, ++iCom )
        {
          TTriangleFacet t012( nInd[ iQ * ( iCom             )],
                               nInd[ iQ * ( (iCom+1)%nbNodes )],
                               nInd[ iQ * ( (iCom+2)%nbNodes )]);
          TTriangleFacet t023( nInd[ iQ * ( iCom             )],
                               nInd[ iQ * ( (iCom+2)%nbNodes )],
                               nInd[ iQ * ( (iCom+3)%nbNodes )]);
          if ( t012.hasAdjacentVol( vol.Element() ) && t023.hasAdjacentVol( vol.Element() ))
          {
            triaSplits.push_back( t012 );
            triaSplits.push_back( t023 );
            break;
          }
        }
      }
      if ( !triaSplits.empty() )
        hasAdjacentSplits = true;
    }

    // Among variants of split method select one compliant with adjacent volumes

    TSplitMethod method;
    if ( !vol.Element()->IsPoly() && !is24TetMode )
    {
      int nbVariants = 2, nbTet = 0;
      const int** connVariants = 0;
      switch ( vol.Element()->GetEntityType() )
      {
      case SMDSEntity_Hexa:
      case SMDSEntity_Quad_Hexa:
      case SMDSEntity_TriQuad_Hexa:
        if ( theMethodFlags == SMESH_MeshEditor::HEXA_TO_5 )
          connVariants = theHexTo5, nbTet = 5;
        else
          connVariants = theHexTo6, nbTet = 6, nbVariants = 4;
        break;
      case SMDSEntity_Pyramid:
      case SMDSEntity_Quad_Pyramid:
        connVariants = thePyraTo2;  nbTet = 2;
        break;
      case SMDSEntity_Penta:
      case SMDSEntity_Quad_Penta:
        connVariants = thePentaTo3; nbTet = 3; nbVariants = 6;
        break;
      default:
        nbVariants = 0;
      }
      for ( int variant = 0; variant < nbVariants && method._nbSplits == 0; ++variant )
      {
        // check method compliancy with adjacent tetras,
        // all found splits must be among facets of tetras described by this method
        method = TSplitMethod( nbTet, connVariants[variant] );
        if ( hasAdjacentSplits && method._nbSplits > 0 )
        {
          bool facetCreated = true;
          for ( int iF = 0; facetCreated && iF < triaSplitsByFace.size(); ++iF )
          {
            list< TTriangleFacet >::const_iterator facet = triaSplitsByFace[iF].begin();
            for ( ; facetCreated && facet != triaSplitsByFace[iF].end(); ++facet )
              facetCreated = method.hasFacet( *facet );
          }
          if ( !facetCreated )
            method = TSplitMethod(0); // incompatible method
        }
      }
    }
    if ( method._nbSplits < 1 )
    {
      // No standard method is applicable, use a generic solution:
      // each facet of a volume is split into triangles and
      // each of triangles and a volume barycenter form a tetrahedron.

      const bool isHex27 = ( vol.Element()->GetEntityType() == SMDSEntity_TriQuad_Hexa );

      int* connectivity = new int[ maxTetConnSize + 1 ];
      method._connectivity = connectivity;
      method._ownConn = true;
      method._baryNode = !isHex27; // to create central node or not

      int connSize = 0;
      int baryCenInd = vol.NbNodes() - int( isHex27 );
      for ( int iF = 0; iF < vol.NbFaces(); ++iF )
      {
        const int nbNodes = vol.NbFaceNodes( iF ) / iQ;
        const int*   nInd = vol.GetFaceNodesIndices( iF );
        // find common node of triangle facets of tetra to create
        int iCommon = 0; // index in linear numeration
        const list< TTriangleFacet >& triaSplits = triaSplitsByFace[ iF ];
        if ( !triaSplits.empty() )
        {
          // by found facets
          const TTriangleFacet* facet = &triaSplits.front();
          for ( ; iCommon < nbNodes-1 ; ++iCommon )
            if ( facet->contains( nInd[ iQ * iCommon ]) &&
                 facet->contains( nInd[ iQ * ((iCommon+2)%nbNodes) ]))
              break;
        }
        else if ( nbNodes > 3 && !is24TetMode )
        {
          // find the best method of splitting into triangles by aspect ratio
          SMESH::Controls::NumericalFunctorPtr aspectRatio( new SMESH::Controls::AspectRatio);
          map< double, int > badness2iCommon;
          const SMDS_MeshNode** nodes = vol.GetFaceNodes( iF );
          int nbVariants = ( nbNodes == 4 ? 2 : nbNodes );
          for ( int iVar = 0; iVar < nbVariants; ++iVar, ++iCommon )
          {
            double badness = 0;
            for ( int iLast = iCommon+2; iLast < iCommon+nbNodes; ++iLast )
            {
              SMDS_FaceOfNodes tria ( nodes[ iQ*( iCommon         )],
                                      nodes[ iQ*((iLast-1)%nbNodes)],
                                      nodes[ iQ*((iLast  )%nbNodes)]);
              badness += getBadRate( &tria, aspectRatio );
            }
            badness2iCommon.insert( make_pair( badness, iCommon ));
          }
          // use iCommon with lowest badness
          iCommon = badness2iCommon.begin()->second;
        }
        if ( iCommon >= nbNodes )
          iCommon = 0; // something wrong

        // fill connectivity of tetrahedra based on a current face
        int nbTet = nbNodes - 2;
        if ( is24TetMode && nbNodes > 3 && triaSplits.empty())
        {
          int faceBaryCenInd;
          if ( isHex27 )
          {
            faceBaryCenInd = vol.GetCenterNodeIndex( iF );
            method._faceBaryNode[ iF ] = vol.GetNodes()[ faceBaryCenInd ];
          }
          else
          {
            method._faceBaryNode[ iF ] = 0;
            faceBaryCenInd = baryCenInd + method._faceBaryNode.size();
          }
          nbTet = nbNodes;
          for ( int i = 0; i < nbTet; ++i )
          {
            int i1 = i, i2 = (i+1) % nbNodes;
            if ( !vol.IsFaceExternal( iF )) swap( i1, i2 );
            connectivity[ connSize++ ] = nInd[ iQ * i1 ];
            connectivity[ connSize++ ] = nInd[ iQ * i2 ];
            connectivity[ connSize++ ] = faceBaryCenInd;
            connectivity[ connSize++ ] = baryCenInd;
          }
        }
        else
        {
          for ( int i = 0; i < nbTet; ++i )
          {
            int i1 = (iCommon+1+i) % nbNodes, i2 = (iCommon+2+i) % nbNodes;
            if ( !vol.IsFaceExternal( iF )) swap( i1, i2 );
            connectivity[ connSize++ ] = nInd[ iQ * iCommon ];
            connectivity[ connSize++ ] = nInd[ iQ * i1 ];
            connectivity[ connSize++ ] = nInd[ iQ * i2 ];
            connectivity[ connSize++ ] = baryCenInd;
          }
        }
        method._nbSplits += nbTet;

      } // loop on volume faces

      connectivity[ connSize++ ] = -1;

    } // end of generic solution

    return method;
  }
  //=======================================================================
  /*!
   * \brief return TSplitMethod to split haxhedron into prisms
   */
  //=======================================================================

  TSplitMethod getPrismSplitMethod( SMDS_VolumeTool& vol,
                                    const int        methodFlags,
                                    const int        facetToSplit)
  {
    // order of facets in HEX according to SMDS_VolumeTool::Hexa_F :
    // B, T, L, B, R, F
    const int iF = ( facetToSplit < 2 ) ? 0 : 1 + ( facetToSplit-2 ) % 2; // [0,1,2]

    if ( methodFlags == SMESH_MeshEditor::HEXA_TO_4_PRISMS )
    {
      static TSplitMethod to4methods[4]; // order BT, LR, FB
      if ( to4methods[iF]._nbSplits == 0 )
      {
        switch ( iF ) {
        case 0:
          to4methods[iF]._connectivity = theHexTo4Prisms_BT;
          to4methods[iF]._faceBaryNode[ 0 ] = 0;
          to4methods[iF]._faceBaryNode[ 1 ] = 0;
          break;
        case 1:
          to4methods[iF]._connectivity = theHexTo4Prisms_LR;
          to4methods[iF]._faceBaryNode[ 2 ] = 0;
          to4methods[iF]._faceBaryNode[ 4 ] = 0;
          break;
        case 2:
          to4methods[iF]._connectivity = theHexTo4Prisms_FB;
          to4methods[iF]._faceBaryNode[ 3 ] = 0;
          to4methods[iF]._faceBaryNode[ 5 ] = 0;
          break;
        default: return to4methods[3];
        }
        to4methods[iF]._nbSplits  = 4;
        to4methods[iF]._nbCorners = 6;
      }
      return to4methods[iF];
    }
    // else if ( methodFlags == HEXA_TO_2_PRISMS )

    TSplitMethod method;

    const int iQ = vol.Element()->IsQuadratic() ? 2 : 1;

    const int nbVariants = 2, nbSplits = 2;
    const int** connVariants = 0;
    switch ( iF ) {
    case 0: connVariants = theHexTo2Prisms_BT; break;
    case 1: connVariants = theHexTo2Prisms_LR; break;
    case 2: connVariants = theHexTo2Prisms_FB; break;
    default: return method;
    }

    // look for prisms adjacent via facetToSplit and an opposite one
    for ( int is2nd = 0; is2nd < 2; ++is2nd )
    {
      int iFacet = is2nd ? vol.GetOppFaceIndexOfHex( facetToSplit ) : facetToSplit;
      int nbNodes = vol.NbFaceNodes( iFacet ) / iQ;
      if ( nbNodes != 4 ) return method;

      const int* nInd = vol.GetFaceNodesIndices( iFacet );
      TTriangleFacet t012( nInd[0*iQ], nInd[1*iQ], nInd[2*iQ] );
      TTriangleFacet t123( nInd[1*iQ], nInd[2*iQ], nInd[3*iQ] );
      TTriangleFacet* t;
      if      ( t012.hasAdjacentVol( vol.Element(), SMDSGeom_PENTA ))
        t = &t012;
      else if ( t123.hasAdjacentVol( vol.Element(), SMDSGeom_PENTA ))
        t = &t123;
      else
        continue;

      // there are adjacent prism
      for ( int variant = 0; variant < nbVariants; ++variant )
      {
        // check method compliancy with adjacent prisms,
        // the found prism facets must be among facets of prisms described by current method
        method._nbSplits     = nbSplits;
        method._nbCorners    = 6;
        method._connectivity = connVariants[ variant ];
        if ( method.hasFacet( *t ))
          return method;
      }
    }

    // No adjacent prisms. Select a variant with a best aspect ratio.

    double badness[2] = { 0, 0 };
    static SMESH::Controls::NumericalFunctorPtr aspectRatio( new SMESH::Controls::AspectRatio);
    const SMDS_MeshNode** nodes = vol.GetNodes();
    for ( int variant = 0; variant < nbVariants; ++variant )
      for ( int is2nd = 0; is2nd < 2; ++is2nd )
      {
        int iFacet = is2nd ? vol.GetOppFaceIndexOfHex( facetToSplit ) : facetToSplit;
        const int*             nInd = vol.GetFaceNodesIndices( iFacet );

        method._connectivity = connVariants[ variant ];
        TTriangleFacet t012( nInd[0*iQ], nInd[1*iQ], nInd[2*iQ] );
        TTriangleFacet t123( nInd[1*iQ], nInd[2*iQ], nInd[3*iQ] );
        TTriangleFacet* t = ( method.hasFacet( t012 )) ? & t012 : & t123;

        SMDS_FaceOfNodes tria ( nodes[ t->_n1 ],
                                nodes[ t->_n2 ],
                                nodes[ t->_n3 ] );
        badness[ variant ] += getBadRate( &tria, aspectRatio );
      }
    const int iBetter = ( badness[1] < badness[0] && badness[0]-badness[1] > 0.1 * badness[0] );

    method._nbSplits     = nbSplits;
    method._nbCorners    = 6;
    method._connectivity = connVariants[ iBetter ];

    return method;
  }

  //================================================================================
  /*!
   * \brief Check if there is a tetraherdon adjacent to the given element via this facet
   */
  //================================================================================

  bool TTriangleFacet::hasAdjacentVol( const SMDS_MeshElement*    elem,
                                       const SMDSAbs_GeometryType geom ) const
  {
    // find the tetrahedron including the three nodes of facet
    const SMDS_MeshNode* n1 = elem->GetNode(_n1);
    const SMDS_MeshNode* n2 = elem->GetNode(_n2);
    const SMDS_MeshNode* n3 = elem->GetNode(_n3);
    SMDS_ElemIteratorPtr volIt1 = n1->GetInverseElementIterator(SMDSAbs_Volume);
    while ( volIt1->more() )
    {
      const SMDS_MeshElement* v = volIt1->next();
      if ( v->GetGeomType() != geom )
        continue;
      const int lastCornerInd = v->NbCornerNodes() - 1;
      if ( v->IsQuadratic() && v->GetNodeIndex( n1 ) > lastCornerInd )
        continue; // medium node not allowed
      const int ind2 = v->GetNodeIndex( n2 );
      if ( ind2 < 0 || lastCornerInd < ind2 )
        continue;
      const int ind3 = v->GetNodeIndex( n3 );
      if ( ind3 < 0 || lastCornerInd < ind3 )
        continue;
      return true;
    }
    return false;
  }

  //=======================================================================
  /*!
   * \brief A key of a face of volume
   */
  //=======================================================================

  struct TVolumeFaceKey: pair< pair< int, int>, pair< int, int> >
  {
    TVolumeFaceKey( SMDS_VolumeTool& vol, int iF )
    {
      TIDSortedNodeSet sortedNodes;
      const int iQ = vol.Element()->IsQuadratic() ? 2 : 1;
      int nbNodes = vol.NbFaceNodes( iF );
      const SMDS_MeshNode** fNodes = vol.GetFaceNodes( iF );
      for ( int i = 0; i < nbNodes; i += iQ )
        sortedNodes.insert( fNodes[i] );
      TIDSortedNodeSet::iterator n = sortedNodes.begin();
      first.first   = (*(n++))->GetID();
      first.second  = (*(n++))->GetID();
      second.first  = (*(n++))->GetID();
      second.second = ( sortedNodes.size() > 3 ) ? (*(n++))->GetID() : 0;
    }
  };
} // namespace

//=======================================================================
//function : SplitVolumes
//purpose  : Split volume elements into tetrahedra or prisms.
//           If facet ID < 0, element is split into tetrahedra,
//           else a hexahedron is split into prisms so that the given facet is
//           split into triangles
//=======================================================================

void SMESH_MeshEditor::SplitVolumes (const TFacetOfElem & theElems,
                                     const int            theMethodFlags)
{
  SMDS_VolumeTool    volTool;
  SMESH_MesherHelper helper( *GetMesh()), fHelper(*GetMesh());
  fHelper.ToFixNodeParameters( true );

  SMESHDS_SubMesh* subMesh = 0;//GetMeshDS()->MeshElements(1);
  SMESHDS_SubMesh* fSubMesh = 0;//subMesh;

  SMESH_SequenceOfElemPtr newNodes, newElems;

  // map face of volume to it's baricenrtic node
  map< TVolumeFaceKey, const SMDS_MeshNode* > volFace2BaryNode;
  double bc[3];
  vector<const SMDS_MeshElement* > splitVols;

  TFacetOfElem::const_iterator elem2facet = theElems.begin();
  for ( ; elem2facet != theElems.end(); ++elem2facet )
  {
    const SMDS_MeshElement* elem = elem2facet->first;
    const int       facetToSplit = elem2facet->second;
    if ( elem->GetType() != SMDSAbs_Volume )
      continue;
    const SMDSAbs_EntityType geomType = elem->GetEntityType();
    if ( geomType == SMDSEntity_Tetra || geomType == SMDSEntity_Quad_Tetra )
      continue;

    if ( !volTool.Set( elem, /*ignoreCentralNodes=*/false )) continue; // strange...

    TSplitMethod splitMethod = ( facetToSplit < 0  ?
                                 getTetraSplitMethod( volTool, theMethodFlags ) :
                                 getPrismSplitMethod( volTool, theMethodFlags, facetToSplit ));
    if ( splitMethod._nbSplits < 1 ) continue;

    // find submesh to add new tetras to
    if ( !subMesh || !subMesh->Contains( elem ))
    {
      int shapeID = FindShape( elem );
      helper.SetSubShape( shapeID ); // helper will add tetras to the found submesh
      subMesh = GetMeshDS()->MeshElements( shapeID );
    }
    int iQ;
    if ( elem->IsQuadratic() )
    {
      iQ = 2;
      // add quadratic links to the helper
      for ( int iF = 0; iF < volTool.NbFaces(); ++iF )
      {
        const SMDS_MeshNode** fNodes = volTool.GetFaceNodes( iF );
        int nbN = volTool.NbFaceNodes( iF ) - bool( volTool.GetCenterNodeIndex(iF) > 0 );
        for ( int iN = 0; iN < nbN; iN += iQ )
          helper.AddTLinkNode( fNodes[iN], fNodes[iN+2], fNodes[iN+1] );
      }
      helper.SetIsQuadratic( true );
    }
    else
    {
      iQ = 1;
      helper.SetIsQuadratic( false );
    }
    vector<const SMDS_MeshNode*> nodes( volTool.GetNodes(),
                                        volTool.GetNodes() + elem->NbNodes() );
    helper.SetElementsOnShape( true );
    if ( splitMethod._baryNode )
    {
      // make a node at barycenter
      volTool.GetBaryCenter( bc[0], bc[1], bc[2] );
      SMDS_MeshNode* gcNode = helper.AddNode( bc[0], bc[1], bc[2] );
      nodes.push_back( gcNode );
      newNodes.Append( gcNode );
    }
    if ( !splitMethod._faceBaryNode.empty() )
    {
      // make or find baricentric nodes of faces
      map<int, const SMDS_MeshNode*>::iterator iF_n = splitMethod._faceBaryNode.begin();
      for ( ; iF_n != splitMethod._faceBaryNode.end(); ++iF_n )
      {
        map< TVolumeFaceKey, const SMDS_MeshNode* >::iterator f_n =
          volFace2BaryNode.insert
          ( make_pair( TVolumeFaceKey( volTool,iF_n->first ), iF_n->second )).first;
        if ( !f_n->second )
        {
          volTool.GetFaceBaryCenter( iF_n->first, bc[0], bc[1], bc[2] );
          newNodes.Append( f_n->second = helper.AddNode( bc[0], bc[1], bc[2] ));
        }
        nodes.push_back( iF_n->second = f_n->second );
      }
    }

    // make new volumes
    splitVols.resize( splitMethod._nbSplits ); // splits of a volume
    const int* volConn = splitMethod._connectivity;
    if ( splitMethod._nbCorners == 4 ) // tetra
      for ( int i = 0; i < splitMethod._nbSplits; ++i, volConn += splitMethod._nbCorners )
        newElems.Append( splitVols[ i ] = helper.AddVolume( nodes[ volConn[0] ],
                                                            nodes[ volConn[1] ],
                                                            nodes[ volConn[2] ],
                                                            nodes[ volConn[3] ]));
    else // prisms
      for ( int i = 0; i < splitMethod._nbSplits; ++i, volConn += splitMethod._nbCorners )
        newElems.Append( splitVols[ i ] = helper.AddVolume( nodes[ volConn[0] ],
                                                            nodes[ volConn[1] ],
                                                            nodes[ volConn[2] ],
                                                            nodes[ volConn[3] ],
                                                            nodes[ volConn[4] ],
                                                            nodes[ volConn[5] ]));

    ReplaceElemInGroups( elem, splitVols, GetMeshDS() );

    // Split faces on sides of the split volume

    const SMDS_MeshNode** volNodes = volTool.GetNodes();
    for ( int iF = 0; iF < volTool.NbFaces(); ++iF )
    {
      const int nbNodes = volTool.NbFaceNodes( iF ) / iQ;
      if ( nbNodes < 4 ) continue;

      // find an existing face
      vector<const SMDS_MeshNode*> fNodes( volTool.GetFaceNodes( iF ),
                                           volTool.GetFaceNodes( iF ) + volTool.NbFaceNodes( iF ));
      while ( const SMDS_MeshElement* face = GetMeshDS()->FindElement( fNodes, SMDSAbs_Face,
                                                                       /*noMedium=*/false))
      {
        // make triangles
        helper.SetElementsOnShape( false );
        vector< const SMDS_MeshElement* > triangles;

        // find submesh to add new triangles in
        if ( !fSubMesh || !fSubMesh->Contains( face ))
        {
          int shapeID = FindShape( face );
          fSubMesh = GetMeshDS()->MeshElements( shapeID );
        }
        map<int, const SMDS_MeshNode*>::iterator iF_n = splitMethod._faceBaryNode.find(iF);
        if ( iF_n != splitMethod._faceBaryNode.end() )
        {
          const SMDS_MeshNode *baryNode = iF_n->second;
          for ( int iN = 0; iN < nbNodes*iQ; iN += iQ )
          {
            const SMDS_MeshNode* n1 = fNodes[iN];
            const SMDS_MeshNode *n2 = fNodes[(iN+iQ)%(nbNodes*iQ)];
            const SMDS_MeshNode *n3 = baryNode;
            if ( !volTool.IsFaceExternal( iF ))
              swap( n2, n3 );
            triangles.push_back( helper.AddFace( n1,n2,n3 ));
          }
          if ( fSubMesh ) // update position of the bary node on geometry
          {
            if ( subMesh )
              subMesh->RemoveNode( baryNode, false );
            GetMeshDS()->SetNodeOnFace( baryNode, fSubMesh->GetID() );
            const TopoDS_Shape& s = GetMeshDS()->IndexToShape( fSubMesh->GetID() );
            if ( !s.IsNull() && s.ShapeType() == TopAbs_FACE )
            {
              fHelper.SetSubShape( s );
              gp_XY uv( 1e100, 1e100 );
              double distXYZ[4];
              if ( !fHelper.CheckNodeUV( TopoDS::Face( s ), baryNode,
                                        uv, /*tol=*/1e-7, /*force=*/true, distXYZ ) &&
                   uv.X() < 1e100 )
              {
                // node is too far from the surface
                GetMeshDS()->MoveNode( baryNode, distXYZ[1], distXYZ[2], distXYZ[3] );
                const_cast<SMDS_MeshNode*>( baryNode )->SetPosition
                  ( SMDS_PositionPtr( new SMDS_FacePosition( uv.X(), uv.Y() )));
              }
            }
          }
        }
        else
        {
          // among possible triangles create ones discribed by split method
          const int* nInd = volTool.GetFaceNodesIndices( iF );
          int nbVariants = ( nbNodes == 4 ? 2 : nbNodes );
          int iCom = 0; // common node of triangle faces to split into
          list< TTriangleFacet > facets;
          for ( int iVar = 0; iVar < nbVariants; ++iVar, ++iCom )
          {
            TTriangleFacet t012( nInd[ iQ * ( iCom                )],
                                 nInd[ iQ * ( (iCom+1)%nbNodes )],
                                 nInd[ iQ * ( (iCom+2)%nbNodes )]);
            TTriangleFacet t023( nInd[ iQ * ( iCom                )],
                                 nInd[ iQ * ( (iCom+2)%nbNodes )],
                                 nInd[ iQ * ( (iCom+3)%nbNodes )]);
            if ( splitMethod.hasFacet( t012 ) && splitMethod.hasFacet( t023 ))
            {
              facets.push_back( t012 );
              facets.push_back( t023 );
              for ( int iLast = iCom+4; iLast < iCom+nbNodes; ++iLast )
                facets.push_back( TTriangleFacet( nInd[ iQ * ( iCom             )],
                                                  nInd[ iQ * ((iLast-1)%nbNodes )],
                                                  nInd[ iQ * ((iLast  )%nbNodes )]));
              break;
            }
          }
          list< TTriangleFacet >::iterator facet = facets.begin();
          if ( facet == facets.end() )
            break;
          for ( ; facet != facets.end(); ++facet )
          {
            if ( !volTool.IsFaceExternal( iF ))
              swap( facet->_n2, facet->_n3 );
            triangles.push_back( helper.AddFace( volNodes[ facet->_n1 ],
                                                 volNodes[ facet->_n2 ],
                                                 volNodes[ facet->_n3 ]));
          }
        }
        for ( int i = 0; i < triangles.size(); ++i )
        {
          if ( !triangles[i] ) continue;
          if ( fSubMesh )
            fSubMesh->AddElement( triangles[i]);
          newElems.Append( triangles[i] );
        }
        ReplaceElemInGroups( face, triangles, GetMeshDS() );
        GetMeshDS()->RemoveFreeElement( face, fSubMesh, /*fromGroups=*/false );

      } // while a face based on facet nodes exists
    } // loop on volume faces to split them into triangles

    GetMeshDS()->RemoveFreeElement( elem, subMesh, /*fromGroups=*/false );

    if ( geomType == SMDSEntity_TriQuad_Hexa )
    {
      // remove medium nodes that could become free
      for ( int i = 20; i < volTool.NbNodes(); ++i )
        if ( volNodes[i]->NbInverseElements() == 0 )
          GetMeshDS()->RemoveNode( volNodes[i] );
    }
  } // loop on volumes to split
  
  myLastCreatedNodes = newNodes;
  myLastCreatedElems = newElems;
}

//=======================================================================
//function : GetHexaFacetsToSplit
//purpose  : For hexahedra that will be split into prisms, finds facets to
//           split into triangles. Only hexahedra adjacent to the one closest
//           to theFacetNormal.Location() are returned.
//param [in,out] theHexas - the hexahedra
//param [in]     theFacetNormal - facet normal
//param [out]    theFacets - the hexahedra and found facet IDs
//=======================================================================

void SMESH_MeshEditor::GetHexaFacetsToSplit( TIDSortedElemSet& theHexas,
                                             const gp_Ax1&     theFacetNormal,
                                             TFacetOfElem &    theFacets)
{
  #define THIS_METHOD "SMESH_MeshEditor::GetHexaFacetsToSplit(): "

  // Find a hexa closest to the location of theFacetNormal

  const SMDS_MeshElement* startHex;
  {
    // get SMDS_ElemIteratorPtr on theHexas
    typedef const SMDS_MeshElement*                                      TValue;
    typedef TIDSortedElemSet::iterator                                   TSetIterator;
    typedef SMDS::SimpleAccessor<TValue,TSetIterator>                    TAccesor;
    typedef SMDS_MeshElement::GeomFilter                                 TFilter;
    typedef SMDS_SetIterator < TValue, TSetIterator, TAccesor, TFilter > TElemSetIter;
    SMDS_ElemIteratorPtr elemIt = SMDS_ElemIteratorPtr
      ( new TElemSetIter( theHexas.begin(),
                          theHexas.end(),
                          SMDS_MeshElement::GeomFilter( SMDSGeom_HEXA )));

    SMESH_ElementSearcher* searcher =
      SMESH_MeshAlgos::GetElementSearcher( *myMesh->GetMeshDS(), elemIt );

    startHex = searcher->FindClosestTo( theFacetNormal.Location(), SMDSAbs_Volume );

    delete searcher;

    if ( !startHex )
      throw SALOME_Exception( THIS_METHOD "startHex not found");
  }

  // Select a facet of startHex by theFacetNormal

  SMDS_VolumeTool vTool( startHex );
  double norm[3], dot, maxDot = 0;
  int facetID = -1;
  for ( int iF = 0; iF < vTool.NbFaces(); ++iF )
    if ( vTool.GetFaceNormal( iF, norm[0], norm[1], norm[2] ))
    {
      dot = Abs( theFacetNormal.Direction().Dot( gp_Dir( norm[0], norm[1], norm[2] )));
      if ( dot > maxDot )
      {
        facetID = iF;
        maxDot = dot;
      }
    }
  if ( facetID < 0 )
    throw SALOME_Exception( THIS_METHOD "facet of startHex not found");

  // Fill theFacets starting from facetID of startHex

  // facets used for seach of volumes adjacent to already treated ones
  typedef pair< TFacetOfElem::iterator, int > TElemFacets;
  typedef map< TVolumeFaceKey, TElemFacets  > TFacetMap;
  TFacetMap facetsToCheck;

  set<const SMDS_MeshNode*> facetNodes;
  const SMDS_MeshElement*   curHex;

  const bool allHex = ( theHexas.size() == myMesh->NbHexas() );

  while ( startHex )
  {
    // move in two directions from startHex via facetID
    for ( int is2nd = 0; is2nd < 2; ++is2nd )
    {
      curHex       = startHex;
      int curFacet = facetID;
      if ( is2nd ) // do not treat startHex twice
      {
        vTool.Set( curHex );
        if ( vTool.IsFreeFace( curFacet, &curHex ))
        {
          curHex = 0;
        }
        else
        {
          vTool.GetFaceNodes( curFacet, facetNodes );
          vTool.Set( curHex );
          curFacet = vTool.GetFaceIndex( facetNodes );
        }
      }
      while ( curHex )
      {
        // store a facet to split
        if ( curHex->GetGeomType() != SMDSGeom_HEXA )
        {
          theFacets.insert( make_pair( curHex, -1 ));
          break;
        }
        if ( !allHex && !theHexas.count( curHex ))
          break;

        pair< TFacetOfElem::iterator, bool > facetIt2isNew =
          theFacets.insert( make_pair( curHex, curFacet ));
        if ( !facetIt2isNew.second )
          break;

        // remember not-to-split facets in facetsToCheck
        int oppFacet = vTool.GetOppFaceIndexOfHex( curFacet );
        for ( int iF = 0; iF < vTool.NbFaces(); ++iF )
        {
          if ( iF == curFacet && iF == oppFacet )
            continue;
          TVolumeFaceKey facetKey ( vTool, iF );
          TElemFacets    elemFacet( facetIt2isNew.first, iF );
          pair< TFacetMap::iterator, bool > it2isnew =
            facetsToCheck.insert( make_pair( facetKey, elemFacet ));
          if ( !it2isnew.second )
            facetsToCheck.erase( it2isnew.first ); // adjacent hex already checked
        }
        // pass to a volume adjacent via oppFacet
        if ( vTool.IsFreeFace( oppFacet, &curHex ))
        {
          curHex = 0;
        }
        else
        {
          // get a new curFacet
          vTool.GetFaceNodes( oppFacet, facetNodes );
          vTool.Set( curHex );
          curFacet = vTool.GetFaceIndex( facetNodes, /*hint=*/curFacet );
        }
      }
    } // move in two directions from startHex via facetID

    // Find a new startHex by facetsToCheck

    startHex = 0;
    facetID  = -1;
    TFacetMap::iterator fIt = facetsToCheck.begin();
    while ( !startHex && fIt != facetsToCheck.end() )
    {
      const TElemFacets&  elemFacets = fIt->second;
      const SMDS_MeshElement*    hex = elemFacets.first->first;
      int                 splitFacet = elemFacets.first->second;
      int               lateralFacet = elemFacets.second;
      facetsToCheck.erase( fIt );
      fIt = facetsToCheck.begin();

      vTool.Set( hex );
      if ( vTool.IsFreeFace( lateralFacet, &curHex ) || 
           curHex->GetGeomType() != SMDSGeom_HEXA )
        continue;
      if ( !allHex && !theHexas.count( curHex ))
        continue;

      startHex = curHex;

      // find a facet of startHex to split

      set<const SMDS_MeshNode*> lateralNodes;
      vTool.GetFaceNodes( lateralFacet, lateralNodes );
      vTool.GetFaceNodes( splitFacet,   facetNodes );
      int oppLateralFacet = vTool.GetOppFaceIndexOfHex( lateralFacet );
      vTool.Set( startHex );
      lateralFacet = vTool.GetFaceIndex( lateralNodes, oppLateralFacet );

      // look for a facet of startHex having common nodes with facetNodes
      // but not lateralFacet
      for ( int iF = 0; iF < vTool.NbFaces(); ++iF )
      {
        if ( iF == lateralFacet )
          continue;
        int nbCommonNodes = 0;
        const SMDS_MeshNode** nn = vTool.GetFaceNodes( iF );
        for ( int iN = 0, nbN = vTool.NbFaceNodes( iF ); iN < nbN; ++iN )
          nbCommonNodes += facetNodes.count( nn[ iN ]);

        if ( nbCommonNodes >= 2 )
        {
          facetID = iF;
          break;
        }
      }
      if ( facetID < 0 )
        throw SALOME_Exception( THIS_METHOD "facet of a new startHex not found");
    }
  } //   while ( startHex )

  return;
}

namespace
{
  //================================================================================
  /*!
   * \brief Selects nodes of several elements according to a given interlace
   *  \param [in] srcNodes - nodes to select from
   *  \param [out] tgtNodesVec - array of nodes of several elements to fill in
   *  \param [in] interlace - indices of nodes for all elements
   *  \param [in] nbElems - nb of elements
   *  \param [in] nbNodes - nb of nodes in each element
   *  \param [in] mesh - the mesh
   *  \param [out] elemQueue - a list to push elements found by the selected nodes
   *  \param [in] type - type of elements to look for
   */
  //================================================================================

  void selectNodes( const vector< const SMDS_MeshNode* >& srcNodes,
                    vector< const SMDS_MeshNode* >*       tgtNodesVec,
                    const int*                            interlace,
                    const int                             nbElems,
                    const int                             nbNodes,
                    SMESHDS_Mesh*                         mesh = 0,
                    list< const SMDS_MeshElement* >*      elemQueue=0,
                    SMDSAbs_ElementType                   type=SMDSAbs_All)
  {
    for ( int iE = 0; iE < nbElems; ++iE )
    {
      vector< const SMDS_MeshNode* >& elemNodes = tgtNodesVec[iE];
      const int*                         select = & interlace[iE*nbNodes];
      elemNodes.resize( nbNodes );
      for ( int iN = 0; iN < nbNodes; ++iN )
        elemNodes[iN] = srcNodes[ select[ iN ]];
    }
    const SMDS_MeshElement* e;
    if ( elemQueue )
      for ( int iE = 0; iE < nbElems; ++iE )
        if (( e = mesh->FindElement( tgtNodesVec[iE], type, /*noMedium=*/false)))
          elemQueue->push_back( e );
  }
}

//=======================================================================
/*
 * Split bi-quadratic elements into linear ones without creation of additional nodes
 *   - bi-quadratic triangle will be split into 3 linear quadrangles;
 *   - bi-quadratic quadrangle will be split into 4 linear quadrangles;
 *   - tri-quadratic hexahedron will be split into 8 linear hexahedra;
 *   Quadratic elements of lower dimension  adjacent to the split bi-quadratic element
 *   will be split in order to keep the mesh conformal.
 *  \param elems - elements to split
 */
//=======================================================================

void SMESH_MeshEditor::SplitBiQuadraticIntoLinear(TIDSortedElemSet& theElems)
{
  vector< const SMDS_MeshNode* > elemNodes(27), subNodes[12], splitNodes[8];
  vector<const SMDS_MeshElement* > splitElems;
  list< const SMDS_MeshElement* > elemQueue;
  list< const SMDS_MeshElement* >::iterator elemIt;

  SMESHDS_Mesh * mesh = GetMeshDS();
  ElemFeatures *elemType, hexaType(SMDSAbs_Volume), quadType(SMDSAbs_Face), segType(SMDSAbs_Edge);
  int nbElems, nbNodes;

  TIDSortedElemSet::iterator elemSetIt = theElems.begin();
  for ( ; elemSetIt != theElems.end(); ++elemSetIt )
  {
    elemQueue.clear();
    elemQueue.push_back( *elemSetIt );
    for ( elemIt = elemQueue.begin(); elemIt != elemQueue.end(); ++elemIt )
    {
      const SMDS_MeshElement* elem = *elemIt;
      switch( elem->GetEntityType() )
      {
      case SMDSEntity_TriQuad_Hexa: // HEX27
      {
        elemNodes.assign( elem->begin_nodes(), elem->end_nodes() );
        nbElems  = nbNodes = 8;
        elemType = & hexaType;

        // get nodes for new elements
        static int vInd[8][8] = {{ 0,8,20,11,   16,21,26,24 },
                                 { 1,9,20,8,    17,22,26,21 },
                                 { 2,10,20,9,   18,23,26,22 },
                                 { 3,11,20,10,  19,24,26,23 },
                                 { 16,21,26,24, 4,12,25,15  },
                                 { 17,22,26,21, 5,13,25,12  },
                                 { 18,23,26,22, 6,14,25,13  },
                                 { 19,24,26,23, 7,15,25,14  }};
        selectNodes( elemNodes, & splitNodes[0], &vInd[0][0], nbElems, nbNodes );

        // add boundary faces to elemQueue
        static int fInd[6][9] = {{ 0,1,2,3, 8,9,10,11,   20 },
                                 { 4,5,6,7, 12,13,14,15, 25 },
                                 { 0,1,5,4, 8,17,12,16,  21 },
                                 { 1,2,6,5, 9,18,13,17,  22 },
                                 { 2,3,7,6, 10,19,14,18, 23 },
                                 { 3,0,4,7, 11,16,15,19, 24 }};
        selectNodes( elemNodes, & subNodes[0], &fInd[0][0], 6,9, mesh, &elemQueue, SMDSAbs_Face );

        // add boundary segments to elemQueue
        static int eInd[12][3] = {{ 0,1,8 }, { 1,2,9 }, { 2,3,10 }, { 3,0,11 },
                                  { 4,5,12}, { 5,6,13}, { 6,7,14 }, { 7,4,15 },
                                  { 0,4,16}, { 1,5,17}, { 2,6,18 }, { 3,7,19 }};
        selectNodes( elemNodes, & subNodes[0], &eInd[0][0], 12,3, mesh, &elemQueue, SMDSAbs_Edge );
        break;
      }
      case SMDSEntity_BiQuad_Triangle: // TRIA7
      {
        elemNodes.assign( elem->begin_nodes(), elem->end_nodes() );
        nbElems = 3;
        nbNodes = 4;
        elemType = & quadType;

        // get nodes for new elements
        static int fInd[3][4] = {{ 0,3,6,5 }, { 1,4,6,3 }, { 2,5,6,4 }};
        selectNodes( elemNodes, & splitNodes[0], &fInd[0][0], nbElems, nbNodes );

        // add boundary segments to elemQueue
        static int eInd[3][3] = {{ 0,1,3 }, { 1,2,4 }, { 2,0,5 }};
        selectNodes( elemNodes, & subNodes[0], &eInd[0][0], 3,3, mesh, &elemQueue, SMDSAbs_Edge );
        break;
      }
      case SMDSEntity_BiQuad_Quadrangle: // QUAD9
      {
        elemNodes.assign( elem->begin_nodes(), elem->end_nodes() );
        nbElems = 4;
        nbNodes = 4;
        elemType = & quadType;

        // get nodes for new elements
        static int fInd[4][4] = {{ 0,4,8,7 }, { 1,5,8,4 }, { 2,6,8,5 }, { 3,7,8,6 }};
        selectNodes( elemNodes, & splitNodes[0], &fInd[0][0], nbElems, nbNodes );

        // add boundary segments to elemQueue
        static int eInd[4][3] = {{ 0,1,4 }, { 1,2,5 }, { 2,3,6 }, { 3,0,7 }};
        selectNodes( elemNodes, & subNodes[0], &eInd[0][0], 4,3, mesh, &elemQueue, SMDSAbs_Edge );
        break;
      }
      case SMDSEntity_Quad_Edge:
      {
        if ( elemIt == elemQueue.begin() )
          continue; // an elem is in theElems
        elemNodes.assign( elem->begin_nodes(), elem->end_nodes() );
        nbElems = 2;
        nbNodes = 2;
        elemType = & segType;

        // get nodes for new elements
        static int eInd[2][2] = {{ 0,2 }, { 2,1 }};
        selectNodes( elemNodes, & splitNodes[0], &eInd[0][0], nbElems, nbNodes );
        break;
      }
      default: continue;
      } // switch( elem->GetEntityType() )

      // Create new elements

      SMESHDS_SubMesh* subMesh = mesh->MeshElements( elem->getshapeId() );

      splitElems.clear();

      //elemType->SetID( elem->GetID() ); // create an elem with the same ID as a removed one
      mesh->RemoveFreeElement( elem, subMesh, /*fromGroups=*/false );
      //splitElems.push_back( AddElement( splitNodes[ 0 ], *elemType ));
      //elemType->SetID( -1 );

      for ( int iE = 0; iE < nbElems; ++iE )
        splitElems.push_back( AddElement( splitNodes[ iE ], *elemType ));


      ReplaceElemInGroups( elem, splitElems, mesh );

      if ( subMesh )
        for ( size_t i = 0; i < splitElems.size(); ++i )
          subMesh->AddElement( splitElems[i] );
    }
  }
}

//=======================================================================
//function : AddToSameGroups
//purpose  : add elemToAdd to the groups the elemInGroups belongs to
//=======================================================================

void SMESH_MeshEditor::AddToSameGroups (const SMDS_MeshElement* elemToAdd,
                                        const SMDS_MeshElement* elemInGroups,
                                        SMESHDS_Mesh *          aMesh)
{
  const set<SMESHDS_GroupBase*>& groups = aMesh->GetGroups();
  if (!groups.empty()) {
    set<SMESHDS_GroupBase*>::const_iterator grIt = groups.begin();
    for ( ; grIt != groups.end(); grIt++ ) {
      SMESHDS_Group* group = dynamic_cast<SMESHDS_Group*>( *grIt );
      if ( group && group->Contains( elemInGroups ))
        group->SMDSGroup().Add( elemToAdd );
    }
  }
}


//=======================================================================
//function : RemoveElemFromGroups
//purpose  : Remove removeelem to the groups the elemInGroups belongs to
//=======================================================================
void SMESH_MeshEditor::RemoveElemFromGroups (const SMDS_MeshElement* removeelem,
                                             SMESHDS_Mesh *          aMesh)
{
  const set<SMESHDS_GroupBase*>& groups = aMesh->GetGroups();
  if (!groups.empty())
  {
    set<SMESHDS_GroupBase*>::const_iterator GrIt = groups.begin();
    for (; GrIt != groups.end(); GrIt++)
    {
      SMESHDS_Group* grp = dynamic_cast<SMESHDS_Group*>(*GrIt);
      if (!grp || grp->IsEmpty()) continue;
      grp->SMDSGroup().Remove(removeelem);
    }
  }
}

//================================================================================
/*!
 * \brief Replace elemToRm by elemToAdd in the all groups
 */
//================================================================================

void SMESH_MeshEditor::ReplaceElemInGroups (const SMDS_MeshElement* elemToRm,
                                            const SMDS_MeshElement* elemToAdd,
                                            SMESHDS_Mesh *          aMesh)
{
  const set<SMESHDS_GroupBase*>& groups = aMesh->GetGroups();
  if (!groups.empty()) {
    set<SMESHDS_GroupBase*>::const_iterator grIt = groups.begin();
    for ( ; grIt != groups.end(); grIt++ ) {
      SMESHDS_Group* group = dynamic_cast<SMESHDS_Group*>( *grIt );
      if ( group && group->SMDSGroup().Remove( elemToRm ) && elemToAdd )
        group->SMDSGroup().Add( elemToAdd );
    }
  }
}

//================================================================================
/*!
 * \brief Replace elemToRm by elemToAdd in the all groups
 */
//================================================================================

void SMESH_MeshEditor::ReplaceElemInGroups (const SMDS_MeshElement*                elemToRm,
                                            const vector<const SMDS_MeshElement*>& elemToAdd,
                                            SMESHDS_Mesh *                         aMesh)
{
  const set<SMESHDS_GroupBase*>& groups = aMesh->GetGroups();
  if (!groups.empty())
  {
    set<SMESHDS_GroupBase*>::const_iterator grIt = groups.begin();
    for ( ; grIt != groups.end(); grIt++ ) {
      SMESHDS_Group* group = dynamic_cast<SMESHDS_Group*>( *grIt );
      if ( group && group->SMDSGroup().Remove( elemToRm ) )
        for ( int i = 0; i < elemToAdd.size(); ++i )
          group->SMDSGroup().Add( elemToAdd[ i ] );
    }
  }
}

//=======================================================================
//function : QuadToTri
//purpose  : Cut quadrangles into triangles.
//           theCrit is used to select a diagonal to cut
//=======================================================================

bool SMESH_MeshEditor::QuadToTri (TIDSortedElemSet & theElems,
                                  const bool         the13Diag)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  MESSAGE( "::QuadToTri()" );

  SMESHDS_Mesh * aMesh = GetMeshDS();

  Handle(Geom_Surface) surface;
  SMESH_MesherHelper   helper( *GetMesh() );

  TIDSortedElemSet::iterator itElem;
  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ ) {
    const SMDS_MeshElement* elem = *itElem;
    if ( !elem || elem->GetType() != SMDSAbs_Face )
      continue;
    bool isquad = elem->NbNodes()==4 || elem->NbNodes()==8;
    if(!isquad) continue;

    if(elem->NbNodes()==4) {
      // retrieve element nodes
      const SMDS_MeshNode* aNodes [4];
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      int i = 0;
      while ( itN->more() )
        aNodes[ i++ ] = static_cast<const SMDS_MeshNode*>( itN->next() );

      int aShapeId = FindShape( elem );
      const SMDS_MeshElement* newElem1 = 0;
      const SMDS_MeshElement* newElem2 = 0;
      if ( the13Diag ) {
        newElem1 = aMesh->AddFace( aNodes[2], aNodes[0], aNodes[1] );
        newElem2 = aMesh->AddFace( aNodes[2], aNodes[3], aNodes[0] );
      }
      else {
        newElem1 = aMesh->AddFace( aNodes[3], aNodes[0], aNodes[1] );
        newElem2 = aMesh->AddFace( aNodes[3], aNodes[1], aNodes[2] );
      }
      myLastCreatedElems.Append(newElem1);
      myLastCreatedElems.Append(newElem2);
      // put a new triangle on the same shape and add to the same groups
      if ( aShapeId )
        {
          aMesh->SetMeshElementOnShape( newElem1, aShapeId );
          aMesh->SetMeshElementOnShape( newElem2, aShapeId );
        }
      AddToSameGroups( newElem1, elem, aMesh );
      AddToSameGroups( newElem2, elem, aMesh );
      aMesh->RemoveElement( elem );
    }

    // Quadratic quadrangle

    if( elem->NbNodes()==8 && elem->IsQuadratic() ) {

      // get surface elem is on
      int aShapeId = FindShape( elem );
      if ( aShapeId != helper.GetSubShapeID() ) {
        surface.Nullify();
        TopoDS_Shape shape;
        if ( aShapeId > 0 )
          shape = aMesh->IndexToShape( aShapeId );
        if ( !shape.IsNull() && shape.ShapeType() == TopAbs_FACE ) {
          TopoDS_Face face = TopoDS::Face( shape );
          surface = BRep_Tool::Surface( face );
          if ( !surface.IsNull() )
            helper.SetSubShape( shape );
        }
      }

      const SMDS_MeshNode* aNodes [8];
      const SMDS_MeshNode* inFaceNode = 0;
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      int i = 0;
      while ( itN->more() ) {
        aNodes[ i++ ] = static_cast<const SMDS_MeshNode*>( itN->next() );
        if ( !inFaceNode && helper.GetNodeUVneedInFaceNode() &&
             aNodes[ i-1 ]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE )
        {
          inFaceNode = aNodes[ i-1 ];
        }
      }

      // find middle point for (0,1,2,3)
      // and create a node in this point;
      gp_XYZ p( 0,0,0 );
      if ( surface.IsNull() ) {
        for(i=0; i<4; i++)
          p += gp_XYZ(aNodes[i]->X(), aNodes[i]->Y(), aNodes[i]->Z() );
        p /= 4;
      }
      else {
        TopoDS_Face geomFace = TopoDS::Face( helper.GetSubShape() );
        gp_XY uv( 0,0 );
        for(i=0; i<4; i++)
          uv += helper.GetNodeUV( geomFace, aNodes[i], inFaceNode );
        uv /= 4.;
        p = surface->Value( uv.X(), uv.Y() ).XYZ();
      }
      const SMDS_MeshNode* newN = aMesh->AddNode( p.X(), p.Y(), p.Z() );
      myLastCreatedNodes.Append(newN);

      // create a new element
      const SMDS_MeshElement* newElem1 = 0;
      const SMDS_MeshElement* newElem2 = 0;
      if ( the13Diag ) {
        newElem1 = aMesh->AddFace(aNodes[2], aNodes[3], aNodes[0],
                                  aNodes[6], aNodes[7], newN );
        newElem2 = aMesh->AddFace(aNodes[2], aNodes[0], aNodes[1],
                                  newN,      aNodes[4], aNodes[5] );
      }
      else {
        newElem1 = aMesh->AddFace(aNodes[3], aNodes[0], aNodes[1],
                                  aNodes[7], aNodes[4], newN );
        newElem2 = aMesh->AddFace(aNodes[3], aNodes[1], aNodes[2],
                                  newN,      aNodes[5], aNodes[6] );
      }
      myLastCreatedElems.Append(newElem1);
      myLastCreatedElems.Append(newElem2);
      // put a new triangle on the same shape and add to the same groups
      if ( aShapeId )
        {
          aMesh->SetMeshElementOnShape( newElem1, aShapeId );
          aMesh->SetMeshElementOnShape( newElem2, aShapeId );
        }
      AddToSameGroups( newElem1, elem, aMesh );
      AddToSameGroups( newElem2, elem, aMesh );
      aMesh->RemoveElement( elem );
    }
  }

  return true;
}

//=======================================================================
//function : getAngle
//purpose  :
//=======================================================================

double getAngle(const SMDS_MeshElement * tr1,
                const SMDS_MeshElement * tr2,
                const SMDS_MeshNode *    n1,
                const SMDS_MeshNode *    n2)
{
  double angle = 2. * M_PI; // bad angle

  // get normals
  SMESH::Controls::TSequenceOfXYZ P1, P2;
  if ( !SMESH::Controls::NumericalFunctor::GetPoints( tr1, P1 ) ||
       !SMESH::Controls::NumericalFunctor::GetPoints( tr2, P2 ))
    return angle;
  gp_Vec N1,N2;
  if(!tr1->IsQuadratic())
    N1 = gp_Vec( P1(2) - P1(1) ) ^ gp_Vec( P1(3) - P1(1) );
  else
    N1 = gp_Vec( P1(3) - P1(1) ) ^ gp_Vec( P1(5) - P1(1) );
  if ( N1.SquareMagnitude() <= gp::Resolution() )
    return angle;
  if(!tr2->IsQuadratic())
    N2 = gp_Vec( P2(2) - P2(1) ) ^ gp_Vec( P2(3) - P2(1) );
  else
    N2 = gp_Vec( P2(3) - P2(1) ) ^ gp_Vec( P2(5) - P2(1) );
  if ( N2.SquareMagnitude() <= gp::Resolution() )
    return angle;

  // find the first diagonal node n1 in the triangles:
  // take in account a diagonal link orientation
  const SMDS_MeshElement *nFirst[2], *tr[] = { tr1, tr2 };
  for ( int t = 0; t < 2; t++ ) {
    SMDS_ElemIteratorPtr it = tr[ t ]->nodesIterator();
    int i = 0, iDiag = -1;
    while ( it->more()) {
      const SMDS_MeshElement *n = it->next();
      if ( n == n1 || n == n2 ) {
        if ( iDiag < 0)
          iDiag = i;
        else {
          if ( i - iDiag == 1 )
            nFirst[ t ] = ( n == n1 ? n2 : n1 );
          else
            nFirst[ t ] = n;
          break;
        }
      }
      i++;
    }
  }
  if ( nFirst[ 0 ] == nFirst[ 1 ] )
    N2.Reverse();

  angle = N1.Angle( N2 );
  //SCRUTE( angle );
  return angle;
}

// =================================================
// class generating a unique ID for a pair of nodes
// and able to return nodes by that ID
// =================================================
class LinkID_Gen {
public:

  LinkID_Gen( const SMESHDS_Mesh* theMesh )
    :myMesh( theMesh ), myMaxID( theMesh->MaxNodeID() + 1)
  {}

  long GetLinkID (const SMDS_MeshNode * n1,
                  const SMDS_MeshNode * n2) const
  {
    return ( Min(n1->GetID(),n2->GetID()) * myMaxID + Max(n1->GetID(),n2->GetID()));
  }

  bool GetNodes (const long             theLinkID,
                 const SMDS_MeshNode* & theNode1,
                 const SMDS_MeshNode* & theNode2) const
  {
    theNode1 = myMesh->FindNode( theLinkID / myMaxID );
    if ( !theNode1 ) return false;
    theNode2 = myMesh->FindNode( theLinkID % myMaxID );
    if ( !theNode2 ) return false;
    return true;
  }

private:
  LinkID_Gen();
  const SMESHDS_Mesh* myMesh;
  long                myMaxID;
};


//=======================================================================
//function : TriToQuad
//purpose  : Fuse neighbour triangles into quadrangles.
//           theCrit is used to select a neighbour to fuse with.
//           theMaxAngle is a max angle between element normals at which
//           fusion is still performed.
//=======================================================================

bool SMESH_MeshEditor::TriToQuad (TIDSortedElemSet &                   theElems,
                                  SMESH::Controls::NumericalFunctorPtr theCrit,
                                  const double                         theMaxAngle)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  MESSAGE( "::TriToQuad()" );

  if ( !theCrit.get() )
    return false;

  SMESHDS_Mesh * aMesh = GetMeshDS();

  // Prepare data for algo: build
  // 1. map of elements with their linkIDs
  // 2. map of linkIDs with their elements

  map< SMESH_TLink, list< const SMDS_MeshElement* > > mapLi_listEl;
  map< SMESH_TLink, list< const SMDS_MeshElement* > >::iterator itLE;
  map< const SMDS_MeshElement*, set< SMESH_TLink > >  mapEl_setLi;
  map< const SMDS_MeshElement*, set< SMESH_TLink > >::iterator itEL;

  TIDSortedElemSet::iterator itElem;
  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ )
  {
    const SMDS_MeshElement* elem = *itElem;
    if(!elem || elem->GetType() != SMDSAbs_Face ) continue;
    bool IsTria = ( elem->NbCornerNodes()==3 );
    if (!IsTria) continue;

    // retrieve element nodes
    const SMDS_MeshNode* aNodes [4];
    SMDS_NodeIteratorPtr itN = elem->nodeIterator();
    int i = 0;
    while ( i < 3 )
      aNodes[ i++ ] = itN->next();
    aNodes[ 3 ] = aNodes[ 0 ];

    // fill maps
    for ( i = 0; i < 3; i++ ) {
      SMESH_TLink link( aNodes[i], aNodes[i+1] );
      // check if elements sharing a link can be fused
      itLE = mapLi_listEl.find( link );
      if ( itLE != mapLi_listEl.end() ) {
        if ((*itLE).second.size() > 1 ) // consider only 2 elems adjacent by a link
          continue;
        const SMDS_MeshElement* elem2 = (*itLE).second.front();
        //if ( FindShape( elem ) != FindShape( elem2 ))
        //  continue; // do not fuse triangles laying on different shapes
        if ( getAngle( elem, elem2, aNodes[i], aNodes[i+1] ) > theMaxAngle )
          continue; // avoid making badly shaped quads
        (*itLE).second.push_back( elem );
      }
      else {
        mapLi_listEl[ link ].push_back( elem );
      }
      mapEl_setLi [ elem ].insert( link );
    }
  }
  // Clean the maps from the links shared by a sole element, ie
  // links to which only one element is bound in mapLi_listEl

  for ( itLE = mapLi_listEl.begin(); itLE != mapLi_listEl.end(); itLE++ ) {
    int nbElems = (*itLE).second.size();
    if ( nbElems < 2  ) {
      const SMDS_MeshElement* elem = (*itLE).second.front();
      SMESH_TLink link = (*itLE).first;
      mapEl_setLi[ elem ].erase( link );
      if ( mapEl_setLi[ elem ].empty() )
        mapEl_setLi.erase( elem );
    }
  }

  // Algo: fuse triangles into quadrangles

  while ( ! mapEl_setLi.empty() ) {
    // Look for the start element:
    // the element having the least nb of shared links
    const SMDS_MeshElement* startElem = 0;
    int minNbLinks = 4;
    for ( itEL = mapEl_setLi.begin(); itEL != mapEl_setLi.end(); itEL++ ) {
      int nbLinks = (*itEL).second.size();
      if ( nbLinks < minNbLinks ) {
        startElem = (*itEL).first;
        minNbLinks = nbLinks;
        if ( minNbLinks == 1 )
          break;
      }
    }

    // search elements to fuse starting from startElem or links of elements
    // fused earlyer - startLinks
    list< SMESH_TLink > startLinks;
    while ( startElem || !startLinks.empty() ) {
      while ( !startElem && !startLinks.empty() ) {
        // Get an element to start, by a link
        SMESH_TLink linkId = startLinks.front();
        startLinks.pop_front();
        itLE = mapLi_listEl.find( linkId );
        if ( itLE != mapLi_listEl.end() ) {
          list< const SMDS_MeshElement* > & listElem = (*itLE).second;
          list< const SMDS_MeshElement* >::iterator itE = listElem.begin();
          for ( ; itE != listElem.end() ; itE++ )
            if ( mapEl_setLi.find( (*itE) ) != mapEl_setLi.end() )
              startElem = (*itE);
          mapLi_listEl.erase( itLE );
        }
      }

      if ( startElem ) {
        // Get candidates to be fused
        const SMDS_MeshElement *tr1 = startElem, *tr2 = 0, *tr3 = 0;
        const SMESH_TLink *link12, *link13;
        startElem = 0;
        ASSERT( mapEl_setLi.find( tr1 ) != mapEl_setLi.end() );
        set< SMESH_TLink >& setLi = mapEl_setLi[ tr1 ];
        ASSERT( !setLi.empty() );
        set< SMESH_TLink >::iterator itLi;
        for ( itLi = setLi.begin(); itLi != setLi.end(); itLi++ )
        {
          const SMESH_TLink & link = (*itLi);
          itLE = mapLi_listEl.find( link );
          if ( itLE == mapLi_listEl.end() )
            continue;

          const SMDS_MeshElement* elem = (*itLE).second.front();
          if ( elem == tr1 )
            elem = (*itLE).second.back();
          mapLi_listEl.erase( itLE );
          if ( mapEl_setLi.find( elem ) == mapEl_setLi.end())
            continue;
          if ( tr2 ) {
            tr3 = elem;
            link13 = &link;
          }
          else {
            tr2 = elem;
            link12 = &link;
          }

          // add other links of elem to list of links to re-start from
          set< SMESH_TLink >& links = mapEl_setLi[ elem ];
          set< SMESH_TLink >::iterator it;
          for ( it = links.begin(); it != links.end(); it++ ) {
            const SMESH_TLink& link2 = (*it);
            if ( link2 != link )
              startLinks.push_back( link2 );
          }
        }

        // Get nodes of possible quadrangles
        const SMDS_MeshNode *n12 [4], *n13 [4];
        bool Ok12 = false, Ok13 = false;
        const SMDS_MeshNode *linkNode1, *linkNode2;
        if(tr2) {
          linkNode1 = link12->first;
          linkNode2 = link12->second;
          if ( tr2 && getQuadrangleNodes( n12, linkNode1, linkNode2, tr1, tr2 ))
            Ok12 = true;
        }
        if(tr3) {
          linkNode1 = link13->first;
          linkNode2 = link13->second;
          if ( tr3 && getQuadrangleNodes( n13, linkNode1, linkNode2, tr1, tr3 ))
            Ok13 = true;
        }

        // Choose a pair to fuse
        if ( Ok12 && Ok13 ) {
          SMDS_FaceOfNodes quad12 ( n12[ 0 ], n12[ 1 ], n12[ 2 ], n12[ 3 ] );
          SMDS_FaceOfNodes quad13 ( n13[ 0 ], n13[ 1 ], n13[ 2 ], n13[ 3 ] );
          double aBadRate12 = getBadRate( &quad12, theCrit );
          double aBadRate13 = getBadRate( &quad13, theCrit );
          if (  aBadRate13 < aBadRate12 )
            Ok12 = false;
          else
            Ok13 = false;
        }

        // Make quadrangles
        // and remove fused elems and remove links from the maps
        mapEl_setLi.erase( tr1 );
        if ( Ok12 )
        {
          mapEl_setLi.erase( tr2 );
          mapLi_listEl.erase( *link12 );
          if ( tr1->NbNodes() == 3 )
          {
            const SMDS_MeshElement* newElem = 0;
            newElem = aMesh->AddFace(n12[0], n12[1], n12[2], n12[3] );
            myLastCreatedElems.Append(newElem);
            AddToSameGroups( newElem, tr1, aMesh );
            int aShapeId = tr1->getshapeId();
            if ( aShapeId )
              aMesh->SetMeshElementOnShape( newElem, aShapeId );
            aMesh->RemoveElement( tr1 );
            aMesh->RemoveElement( tr2 );
          }
          else {
            vector< const SMDS_MeshNode* > N1;
            vector< const SMDS_MeshNode* > N2;
            getNodesFromTwoTria(tr1,tr2,N1,N2);
            // now we receive following N1 and N2 (using numeration as in image in InverseDiag())
            // tria1 : (1 2 4 5 9 7)  and  tria2 : (3 4 2 8 9 6)
            // i.e. first nodes from both arrays form a new diagonal
            const SMDS_MeshNode* aNodes[8];
            aNodes[0] = N1[0];
            aNodes[1] = N1[1];
            aNodes[2] = N2[0];
            aNodes[3] = N2[1];
            aNodes[4] = N1[3];
            aNodes[5] = N2[5];
            aNodes[6] = N2[3];
            aNodes[7] = N1[5];
            const SMDS_MeshElement* newElem = 0;
            if ( N1.size() == 7 || N2.size() == 7 ) // biquadratic
              newElem = aMesh->AddFace(aNodes[0], aNodes[1], aNodes[2], aNodes[3],
                                       aNodes[4], aNodes[5], aNodes[6], aNodes[7], N1[4]);
            else
              newElem = aMesh->AddFace(aNodes[0], aNodes[1], aNodes[2], aNodes[3],
                                       aNodes[4], aNodes[5], aNodes[6], aNodes[7]);
            myLastCreatedElems.Append(newElem);
            AddToSameGroups( newElem, tr1, aMesh );
            int aShapeId = tr1->getshapeId();
            if ( aShapeId )
              aMesh->SetMeshElementOnShape( newElem, aShapeId );
            aMesh->RemoveElement( tr1 );
            aMesh->RemoveElement( tr2 );
            // remove middle node (9)
            if ( N1[4]->NbInverseElements() == 0 )
              aMesh->RemoveNode( N1[4] );
            if ( N1.size() == 7 && N1[6]->NbInverseElements() == 0 )
              aMesh->RemoveNode( N1[6] );
            if ( N2.size() == 7 && N2[6]->NbInverseElements() == 0 )
              aMesh->RemoveNode( N2[6] );
          }
        }
        else if ( Ok13 )
        {
          mapEl_setLi.erase( tr3 );
          mapLi_listEl.erase( *link13 );
          if ( tr1->NbNodes() == 3 ) {
            const SMDS_MeshElement* newElem = 0;
            newElem = aMesh->AddFace(n13[0], n13[1], n13[2], n13[3] );
            myLastCreatedElems.Append(newElem);
            AddToSameGroups( newElem, tr1, aMesh );
            int aShapeId = tr1->getshapeId();
            if ( aShapeId )
              aMesh->SetMeshElementOnShape( newElem, aShapeId );
            aMesh->RemoveElement( tr1 );
            aMesh->RemoveElement( tr3 );
          }
          else {
            vector< const SMDS_MeshNode* > N1;
            vector< const SMDS_MeshNode* > N2;
            getNodesFromTwoTria(tr1,tr3,N1,N2);
            // now we receive following N1 and N2 (using numeration as above image)
            // tria1 : (1 2 4 5 9 7)  and  tria2 : (3 4 2 8 9 6)
            // i.e. first nodes from both arrays form a new diagonal
            const SMDS_MeshNode* aNodes[8];
            aNodes[0] = N1[0];
            aNodes[1] = N1[1];
            aNodes[2] = N2[0];
            aNodes[3] = N2[1];
            aNodes[4] = N1[3];
            aNodes[5] = N2[5];
            aNodes[6] = N2[3];
            aNodes[7] = N1[5];
            const SMDS_MeshElement* newElem = 0;
            if ( N1.size() == 7 || N2.size() == 7 ) // biquadratic
              newElem = aMesh->AddFace(aNodes[0], aNodes[1], aNodes[2], aNodes[3],
                                       aNodes[4], aNodes[5], aNodes[6], aNodes[7], N1[4]);
            else
              newElem = aMesh->AddFace(aNodes[0], aNodes[1], aNodes[2], aNodes[3],
                                       aNodes[4], aNodes[5], aNodes[6], aNodes[7]);
            myLastCreatedElems.Append(newElem);
            AddToSameGroups( newElem, tr1, aMesh );
            int aShapeId = tr1->getshapeId();
            if ( aShapeId )
              aMesh->SetMeshElementOnShape( newElem, aShapeId );
            aMesh->RemoveElement( tr1 );
            aMesh->RemoveElement( tr3 );
            // remove middle node (9)
            if ( N1[4]->NbInverseElements() == 0 )
              aMesh->RemoveNode( N1[4] );
            if ( N1.size() == 7 && N1[6]->NbInverseElements() == 0 )
              aMesh->RemoveNode( N1[6] );
            if ( N2.size() == 7 && N2[6]->NbInverseElements() == 0 )
              aMesh->RemoveNode( N2[6] );
          }
        }

        // Next element to fuse: the rejected one
        if ( tr3 )
          startElem = Ok12 ? tr3 : tr2;

      } // if ( startElem )
    } // while ( startElem || !startLinks.empty() )
  } // while ( ! mapEl_setLi.empty() )

  return true;
}


/*#define DUMPSO(txt) \
//  cout << txt << endl;
//=============================================================================
//
//
//
//=============================================================================
static void swap( int i1, int i2, int idNodes[], gp_Pnt P[] )
{
if ( i1 == i2 )
return;
int tmp = idNodes[ i1 ];
idNodes[ i1 ] = idNodes[ i2 ];
idNodes[ i2 ] = tmp;
gp_Pnt Ptmp = P[ i1 ];
P[ i1 ] = P[ i2 ];
P[ i2 ] = Ptmp;
DUMPSO( i1 << "(" << idNodes[ i2 ] << ") <-> " << i2 << "(" << idNodes[ i1 ] << ")");
}

//=======================================================================
//function : SortQuadNodes
//purpose  : Set 4 nodes of a quadrangle face in a good order.
//           Swap 1<->2 or 2<->3 nodes and correspondingly return
//           1 or 2 else 0.
//=======================================================================

int SMESH_MeshEditor::SortQuadNodes (const SMDS_Mesh * theMesh,
int               idNodes[] )
{
  gp_Pnt P[4];
  int i;
  for ( i = 0; i < 4; i++ ) {
    const SMDS_MeshNode *n = theMesh->FindNode( idNodes[i] );
    if ( !n ) return 0;
    P[ i ].SetCoord( n->X(), n->Y(), n->Z() );
  }

  gp_Vec V1(P[0], P[1]);
  gp_Vec V2(P[0], P[2]);
  gp_Vec V3(P[0], P[3]);

  gp_Vec Cross1 = V1 ^ V2;
  gp_Vec Cross2 = V2 ^ V3;

  i = 0;
  if (Cross1.Dot(Cross2) < 0)
  {
    Cross1 = V2 ^ V1;
    Cross2 = V1 ^ V3;

    if (Cross1.Dot(Cross2) < 0)
      i = 2;
    else
      i = 1;
    swap ( i, i + 1, idNodes, P );

    //     for ( int ii = 0; ii < 4; ii++ ) {
    //       const SMDS_MeshNode *n = theMesh->FindNode( idNodes[ii] );
    //       DUMPSO( ii << "(" << idNodes[ii] <<") : "<<n->X()<<" "<<n->Y()<<" "<<n->Z());
    //     }
  }
  return i;
}

//=======================================================================
//function : SortHexaNodes
//purpose  : Set 8 nodes of a hexahedron in a good order.
//           Return success status
//=======================================================================

bool SMESH_MeshEditor::SortHexaNodes (const SMDS_Mesh * theMesh,
                                      int               idNodes[] )
{
  gp_Pnt P[8];
  int i;
  DUMPSO( "INPUT: ========================================");
  for ( i = 0; i < 8; i++ ) {
    const SMDS_MeshNode *n = theMesh->FindNode( idNodes[i] );
    if ( !n ) return false;
    P[ i ].SetCoord( n->X(), n->Y(), n->Z() );
    DUMPSO( i << "(" << idNodes[i] <<") : "<<n->X()<<" "<<n->Y()<<" "<<n->Z());
  }
  DUMPSO( "========================================");


  set<int> faceNodes;  // ids of bottom face nodes, to be found
  set<int> checkedId1; // ids of tried 2-nd nodes
  Standard_Real leastDist = DBL_MAX; // dist of the 4-th node from 123 plane
  const Standard_Real tol = 1.e-6;   // tolerance to find nodes in plane
  int iMin, iLoop1 = 0;

  // Loop to try the 2-nd nodes

  while ( leastDist > DBL_MIN && ++iLoop1 < 8 )
  {
    // Find not checked 2-nd node
    for ( i = 1; i < 8; i++ )
      if ( checkedId1.find( idNodes[i] ) == checkedId1.end() ) {
        int id1 = idNodes[i];
        swap ( 1, i, idNodes, P );
        checkedId1.insert ( id1 );
        break;
      }

    // Find the 3-d node so that 1-2-3 triangle to be on a hexa face,
    // ie that all but meybe one (id3 which is on the same face) nodes
    // lay on the same side from the triangle plane.

    bool manyInPlane = false; // more than 4 nodes lay in plane
    int iLoop2 = 0;
    while ( ++iLoop2 < 6 ) {

      // get 1-2-3 plane coeffs
      Standard_Real A, B, C, D;
      gp_Vec N = gp_Vec (P[0], P[1]).Crossed( gp_Vec (P[0], P[2]) );
      if ( N.SquareMagnitude() > gp::Resolution() )
      {
        gp_Pln pln ( P[0], N );
        pln.Coefficients( A, B, C, D );

        // find the node (iMin) closest to pln
        Standard_Real dist[ 8 ], minDist = DBL_MAX;
        set<int> idInPln;
        for ( i = 3; i < 8; i++ ) {
          dist[i] = A * P[i].X() + B * P[i].Y() + C * P[i].Z() + D;
          if ( fabs( dist[i] ) < minDist ) {
            minDist = fabs( dist[i] );
            iMin = i;
          }
          if ( fabs( dist[i] ) <= tol )
            idInPln.insert( idNodes[i] );
        }

        // there should not be more than 4 nodes in bottom plane
        if ( idInPln.size() > 1 )
        {
          DUMPSO( "### idInPln.size() = " << idInPln.size());
          // idInPlane does not contain the first 3 nodes
          if ( manyInPlane || idInPln.size() == 5)
            return false; // all nodes in one plane
          manyInPlane = true;

          // set the 1-st node to be not in plane
          for ( i = 3; i < 8; i++ ) {
            if ( idInPln.find( idNodes[ i ] ) == idInPln.end() ) {
              DUMPSO( "### Reset 0-th node");
              swap( 0, i, idNodes, P );
              break;
            }
          }

          // reset to re-check second nodes
          leastDist = DBL_MAX;
          faceNodes.clear();
          checkedId1.clear();
          iLoop1 = 0;
          break; // from iLoop2;
        }

        // check that the other 4 nodes are on the same side
        bool sameSide = true;
        bool isNeg = dist[ iMin == 3 ? 4 : 3 ] <= 0.;
        for ( i = 3; sameSide && i < 8; i++ ) {
          if ( i != iMin )
            sameSide = ( isNeg == dist[i] <= 0.);
        }

        // keep best solution
        if ( sameSide && minDist < leastDist ) {
          leastDist = minDist;
          faceNodes.clear();
          faceNodes.insert( idNodes[ 1 ] );
          faceNodes.insert( idNodes[ 2 ] );
          faceNodes.insert( idNodes[ iMin ] );
          DUMPSO( "loop " << iLoop2 << " id2 " << idNodes[ 1 ] << " id3 " << idNodes[ 2 ]
                  << " leastDist = " << leastDist);
          if ( leastDist <= DBL_MIN )
            break;
        }
      }

      // set next 3-d node to check
      int iNext = 2 + iLoop2;
      if ( iNext < 8 ) {
        DUMPSO( "Try 2-nd");
        swap ( 2, iNext, idNodes, P );
      }
    } // while ( iLoop2 < 6 )
  } // iLoop1

  if ( faceNodes.empty() ) return false;

  // Put the faceNodes in proper places
  for ( i = 4; i < 8; i++ ) {
    if ( faceNodes.find( idNodes[ i ] ) != faceNodes.end() ) {
      // find a place to put
      int iTo = 1;
      while ( faceNodes.find( idNodes[ iTo ] ) != faceNodes.end() )
        iTo++;
      DUMPSO( "Set faceNodes");
      swap ( iTo, i, idNodes, P );
    }
  }


  // Set nodes of the found bottom face in good order
  DUMPSO( " Found bottom face: ");
  i = SortQuadNodes( theMesh, idNodes );
  if ( i ) {
    gp_Pnt Ptmp = P[ i ];
    P[ i ] = P[ i+1 ];
    P[ i+1 ] = Ptmp;
  }
  //   else
  //     for ( int ii = 0; ii < 4; ii++ ) {
  //       const SMDS_MeshNode *n = theMesh->FindNode( idNodes[ii] );
  //       DUMPSO( ii << "(" << idNodes[ii] <<") : "<<n->X()<<" "<<n->Y()<<" "<<n->Z());
  //    }

  // Gravity center of the top and bottom faces
  gp_Pnt aGCb = ( P[0].XYZ() + P[1].XYZ() + P[2].XYZ() + P[3].XYZ() ) / 4.;
  gp_Pnt aGCt = ( P[4].XYZ() + P[5].XYZ() + P[6].XYZ() + P[7].XYZ() ) / 4.;

  // Get direction from the bottom to the top face
  gp_Vec upDir ( aGCb, aGCt );
  Standard_Real upDirSize = upDir.Magnitude();
  if ( upDirSize <= gp::Resolution() ) return false;
  upDir / upDirSize;

  // Assure that the bottom face normal points up
  gp_Vec Nb = gp_Vec (P[0], P[1]).Crossed( gp_Vec (P[0], P[2]) );
  Nb += gp_Vec (P[0], P[2]).Crossed( gp_Vec (P[0], P[3]) );
  if ( Nb.Dot( upDir ) < 0 ) {
    DUMPSO( "Reverse bottom face");
    swap( 1, 3, idNodes, P );
  }

  // Find 5-th node - the one closest to the 1-st among the last 4 nodes.
  Standard_Real minDist = DBL_MAX;
  for ( i = 4; i < 8; i++ ) {
    // projection of P[i] to the plane defined by P[0] and upDir
    gp_Pnt Pp = P[i].Translated( upDir * ( upDir.Dot( gp_Vec( P[i], P[0] ))));
    Standard_Real sqDist = P[0].SquareDistance( Pp );
    if ( sqDist < minDist ) {
      minDist = sqDist;
      iMin = i;
    }
  }
  DUMPSO( "Set 4-th");
  swap ( 4, iMin, idNodes, P );

  // Set nodes of the top face in good order
  DUMPSO( "Sort top face");
  i = SortQuadNodes( theMesh, &idNodes[4] );
  if ( i ) {
    i += 4;
    gp_Pnt Ptmp = P[ i ];
    P[ i ] = P[ i+1 ];
    P[ i+1 ] = Ptmp;
  }

  // Assure that direction of the top face normal is from the bottom face
  gp_Vec Nt = gp_Vec (P[4], P[5]).Crossed( gp_Vec (P[4], P[6]) );
  Nt += gp_Vec (P[4], P[6]).Crossed( gp_Vec (P[4], P[7]) );
  if ( Nt.Dot( upDir ) < 0 ) {
    DUMPSO( "Reverse top face");
    swap( 5, 7, idNodes, P );
  }

  //   DUMPSO( "OUTPUT: ========================================");
  //   for ( i = 0; i < 8; i++ ) {
  //     float *p = ugrid->GetPoint(idNodes[i]);
  //     DUMPSO( i << "(" << idNodes[i] << ") : " << p[0] << " " << p[1] << " " << p[2]);
  //   }

  return true;
}*/

//================================================================================
/*!
 * \brief Return nodes linked to the given one
 * \param theNode - the node
 * \param linkedNodes - the found nodes
 * \param type - the type of elements to check
 *
 * Medium nodes are ignored
 */
//================================================================================

void SMESH_MeshEditor::GetLinkedNodes( const SMDS_MeshNode* theNode,
                                       TIDSortedElemSet &   linkedNodes,
                                       SMDSAbs_ElementType  type )
{
  SMDS_ElemIteratorPtr elemIt = theNode->GetInverseElementIterator(type);
  while ( elemIt->more() )
  {
    const SMDS_MeshElement* elem = elemIt->next();
    if(elem->GetType() == SMDSAbs_0DElement)
      continue;

    SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
    if ( elem->GetType() == SMDSAbs_Volume )
    {
      SMDS_VolumeTool vol( elem );
      while ( nodeIt->more() ) {
        const SMDS_MeshNode* n = cast2Node( nodeIt->next() );
        if ( theNode != n && vol.IsLinked( theNode, n ))
          linkedNodes.insert( n );
      }
    }
    else
    {
      for ( int i = 0; nodeIt->more(); ++i ) {
        const SMDS_MeshNode* n = cast2Node( nodeIt->next() );
        if ( n == theNode ) {
          int iBefore = i - 1;
          int iAfter  = i + 1;
          if ( elem->IsQuadratic() ) {
            int nb = elem->NbNodes() / 2;
            iAfter  = SMESH_MesherHelper::WrapIndex( iAfter, nb );
            iBefore = SMESH_MesherHelper::WrapIndex( iBefore, nb );
          }
          linkedNodes.insert( elem->GetNodeWrap( iAfter ));
          linkedNodes.insert( elem->GetNodeWrap( iBefore ));
        }
      }
    }
  }
}

//=======================================================================
//function : laplacianSmooth
//purpose  : pulls theNode toward the center of surrounding nodes directly
//           connected to that node along an element edge
//=======================================================================

void laplacianSmooth(const SMDS_MeshNode*                 theNode,
                     const Handle(Geom_Surface)&          theSurface,
                     map< const SMDS_MeshNode*, gp_XY* >& theUVMap)
{
  // find surrounding nodes

  TIDSortedElemSet nodeSet;
  SMESH_MeshEditor::GetLinkedNodes( theNode, nodeSet, SMDSAbs_Face );

  // compute new coodrs

  double coord[] = { 0., 0., 0. };
  TIDSortedElemSet::iterator nodeSetIt = nodeSet.begin();
  for ( ; nodeSetIt != nodeSet.end(); nodeSetIt++ ) {
    const SMDS_MeshNode* node = cast2Node(*nodeSetIt);
    if ( theSurface.IsNull() ) { // smooth in 3D
      coord[0] += node->X();
      coord[1] += node->Y();
      coord[2] += node->Z();
    }
    else { // smooth in 2D
      ASSERT( theUVMap.find( node ) != theUVMap.end() );
      gp_XY* uv = theUVMap[ node ];
      coord[0] += uv->X();
      coord[1] += uv->Y();
    }
  }
  int nbNodes = nodeSet.size();
  if ( !nbNodes )
    return;
  coord[0] /= nbNodes;
  coord[1] /= nbNodes;

  if ( !theSurface.IsNull() ) {
    ASSERT( theUVMap.find( theNode ) != theUVMap.end() );
    theUVMap[ theNode ]->SetCoord( coord[0], coord[1] );
    gp_Pnt p3d = theSurface->Value( coord[0], coord[1] );
    coord[0] = p3d.X();
    coord[1] = p3d.Y();
    coord[2] = p3d.Z();
  }
  else
    coord[2] /= nbNodes;

  // move node

  const_cast< SMDS_MeshNode* >( theNode )->setXYZ(coord[0],coord[1],coord[2]);
}

//=======================================================================
//function : centroidalSmooth
//purpose  : pulls theNode toward the element-area-weighted centroid of the
//           surrounding elements
//=======================================================================

void centroidalSmooth(const SMDS_MeshNode*                 theNode,
                      const Handle(Geom_Surface)&          theSurface,
                      map< const SMDS_MeshNode*, gp_XY* >& theUVMap)
{
  gp_XYZ aNewXYZ(0.,0.,0.);
  SMESH::Controls::Area anAreaFunc;
  double totalArea = 0.;
  int nbElems = 0;

  // compute new XYZ

  SMDS_ElemIteratorPtr elemIt = theNode->GetInverseElementIterator(SMDSAbs_Face);
  while ( elemIt->more() )
  {
    const SMDS_MeshElement* elem = elemIt->next();
    nbElems++;

    gp_XYZ elemCenter(0.,0.,0.);
    SMESH::Controls::TSequenceOfXYZ aNodePoints;
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    int nn = elem->NbNodes();
    if(elem->IsQuadratic()) nn = nn/2;
    int i=0;
    //while ( itN->more() ) {
    while ( i<nn ) {
      const SMDS_MeshNode* aNode = static_cast<const SMDS_MeshNode*>( itN->next() );
      i++;
      gp_XYZ aP( aNode->X(), aNode->Y(), aNode->Z() );
      aNodePoints.push_back( aP );
      if ( !theSurface.IsNull() ) { // smooth in 2D
        ASSERT( theUVMap.find( aNode ) != theUVMap.end() );
        gp_XY* uv = theUVMap[ aNode ];
        aP.SetCoord( uv->X(), uv->Y(), 0. );
      }
      elemCenter += aP;
    }
    double elemArea = anAreaFunc.GetValue( aNodePoints );
    totalArea += elemArea;
    elemCenter /= nn;
    aNewXYZ += elemCenter * elemArea;
  }
  aNewXYZ /= totalArea;
  if ( !theSurface.IsNull() ) {
    theUVMap[ theNode ]->SetCoord( aNewXYZ.X(), aNewXYZ.Y() );
    aNewXYZ = theSurface->Value( aNewXYZ.X(), aNewXYZ.Y() ).XYZ();
  }

  // move node

  const_cast< SMDS_MeshNode* >( theNode )->setXYZ(aNewXYZ.X(),aNewXYZ.Y(),aNewXYZ.Z());
}

//=======================================================================
//function : getClosestUV
//purpose  : return UV of closest projection
//=======================================================================

static bool getClosestUV (Extrema_GenExtPS& projector,
                          const gp_Pnt&     point,
                          gp_XY &           result)
{
  projector.Perform( point );
  if ( projector.IsDone() ) {
    double u, v, minVal = DBL_MAX;
    for ( int i = projector.NbExt(); i > 0; i-- )
      if ( projector.SquareDistance( i ) < minVal ) {
        minVal = projector.SquareDistance( i );
        projector.Point( i ).Parameter( u, v );
      }
    result.SetCoord( u, v );
    return true;
  }
  return false;
}

//=======================================================================
//function : Smooth
//purpose  : Smooth theElements during theNbIterations or until a worst
//           element has aspect ratio <= theTgtAspectRatio.
//           Aspect Ratio varies in range [1.0, inf].
//           If theElements is empty, the whole mesh is smoothed.
//           theFixedNodes contains additionally fixed nodes. Nodes built
//           on edges and boundary nodes are always fixed.
//=======================================================================

void SMESH_MeshEditor::Smooth (TIDSortedElemSet &          theElems,
                               set<const SMDS_MeshNode*> & theFixedNodes,
                               const SmoothMethod          theSmoothMethod,
                               const int                   theNbIterations,
                               double                      theTgtAspectRatio,
                               const bool                  the2D)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  MESSAGE((theSmoothMethod==LAPLACIAN ? "LAPLACIAN" : "CENTROIDAL") << "--::Smooth()");

  if ( theTgtAspectRatio < 1.0 )
    theTgtAspectRatio = 1.0;

  const double disttol = 1.e-16;

  SMESH::Controls::AspectRatio aQualityFunc;

  SMESHDS_Mesh* aMesh = GetMeshDS();

  if ( theElems.empty() ) {
    // add all faces to theElems
    SMDS_FaceIteratorPtr fIt = aMesh->facesIterator();
    while ( fIt->more() ) {
      const SMDS_MeshElement* face = fIt->next();
      theElems.insert( theElems.end(), face );
    }
  }
  // get all face ids theElems are on
  set< int > faceIdSet;
  TIDSortedElemSet::iterator itElem;
  if ( the2D )
    for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ ) {
      int fId = FindShape( *itElem );
      // check that corresponding submesh exists and a shape is face
      if (fId &&
          faceIdSet.find( fId ) == faceIdSet.end() &&
          aMesh->MeshElements( fId )) {
        TopoDS_Shape F = aMesh->IndexToShape( fId );
        if ( !F.IsNull() && F.ShapeType() == TopAbs_FACE )
          faceIdSet.insert( fId );
      }
    }
  faceIdSet.insert( 0 ); // to smooth elements that are not on any TopoDS_Face

  // ===============================================
  // smooth elements on each TopoDS_Face separately
  // ===============================================

  SMESH_MesherHelper helper( *GetMesh() );

  set< int >::reverse_iterator fId = faceIdSet.rbegin(); // treat 0 fId at the end
  for ( ; fId != faceIdSet.rend(); ++fId )
  {
    // get face surface and submesh
    Handle(Geom_Surface) surface;
    SMESHDS_SubMesh* faceSubMesh = 0;
    TopoDS_Face face;
    double fToler2 = 0, f,l;
    double u1 = 0, u2 = 0, v1 = 0, v2 = 0;
    bool isUPeriodic = false, isVPeriodic = false;
    if ( *fId )
    {
      face = TopoDS::Face( aMesh->IndexToShape( *fId ));
      surface = BRep_Tool::Surface( face );
      faceSubMesh = aMesh->MeshElements( *fId );
      fToler2 = BRep_Tool::Tolerance( face );
      fToler2 *= fToler2 * 10.;
      isUPeriodic = surface->IsUPeriodic();
      if ( isUPeriodic )
        surface->UPeriod();
      isVPeriodic = surface->IsVPeriodic();
      if ( isVPeriodic )
        surface->VPeriod();
      surface->Bounds( u1, u2, v1, v2 );
      helper.SetSubShape( face );
    }
    // ---------------------------------------------------------
    // for elements on a face, find movable and fixed nodes and
    // compute UV for them
    // ---------------------------------------------------------
    bool checkBoundaryNodes = false;
    bool isQuadratic = false;
    set<const SMDS_MeshNode*> setMovableNodes;
    map< const SMDS_MeshNode*, gp_XY* > uvMap, uvMap2;
    list< gp_XY > listUV; // uvs the 2 uvMaps refer to
    list< const SMDS_MeshElement* > elemsOnFace;

    Extrema_GenExtPS projector;
    GeomAdaptor_Surface surfAdaptor;
    if ( !surface.IsNull() ) {
      surfAdaptor.Load( surface );
      projector.Initialize( surfAdaptor, 20,20, 1e-5,1e-5 );
    }
    int nbElemOnFace = 0;
    itElem = theElems.begin();
    // loop on not yet smoothed elements: look for elems on a face
    while ( itElem != theElems.end() )
    {
      if ( faceSubMesh && nbElemOnFace == faceSubMesh->NbElements() )
        break; // all elements found

      const SMDS_MeshElement* elem = *itElem;
      if ( !elem || elem->GetType() != SMDSAbs_Face || elem->NbNodes() < 3 ||
           ( faceSubMesh && !faceSubMesh->Contains( elem ))) {
        ++itElem;
        continue;
      }
      elemsOnFace.push_back( elem );
      theElems.erase( itElem++ );
      nbElemOnFace++;

      if ( !isQuadratic )
        isQuadratic = elem->IsQuadratic();

      // get movable nodes of elem
      const SMDS_MeshNode* node;
      SMDS_TypeOfPosition posType;
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      int nn = 0, nbn =  elem->NbNodes();
      if(elem->IsQuadratic())
        nbn = nbn/2;
      while ( nn++ < nbn ) {
        node = static_cast<const SMDS_MeshNode*>( itN->next() );
        const SMDS_PositionPtr& pos = node->GetPosition();
        posType = pos ? pos->GetTypeOfPosition() : SMDS_TOP_3DSPACE;
        if (posType != SMDS_TOP_EDGE &&
            posType != SMDS_TOP_VERTEX &&
            theFixedNodes.find( node ) == theFixedNodes.end())
        {
          // check if all faces around the node are on faceSubMesh
          // because a node on edge may be bound to face
          SMDS_ElemIteratorPtr eIt = node->GetInverseElementIterator(SMDSAbs_Face);
          bool all = true;
          if ( faceSubMesh ) {
            while ( eIt->more() && all ) {
              const SMDS_MeshElement* e = eIt->next();
              all = faceSubMesh->Contains( e );
            }
          }
          if ( all )
            setMovableNodes.insert( node );
          else
            checkBoundaryNodes = true;
        }
        if ( posType == SMDS_TOP_3DSPACE )
          checkBoundaryNodes = true;
      }

      if ( surface.IsNull() )
        continue;

      // get nodes to check UV
      list< const SMDS_MeshNode* > uvCheckNodes;
      const SMDS_MeshNode* nodeInFace = 0;
      itN = elem->nodesIterator();
      nn = 0; nbn =  elem->NbNodes();
      if(elem->IsQuadratic())
        nbn = nbn/2;
      while ( nn++ < nbn ) {
        node = static_cast<const SMDS_MeshNode*>( itN->next() );
        if ( node->GetPosition()->GetDim() == 2 )
          nodeInFace = node;
        if ( uvMap.find( node ) == uvMap.end() )
          uvCheckNodes.push_back( node );
        // add nodes of elems sharing node
        //         SMDS_ElemIteratorPtr eIt = node->GetInverseElementIterator(SMDSAbs_Face);
        //         while ( eIt->more() ) {
        //           const SMDS_MeshElement* e = eIt->next();
        //           if ( e != elem ) {
        //             SMDS_ElemIteratorPtr nIt = e->nodesIterator();
        //             while ( nIt->more() ) {
        //               const SMDS_MeshNode* n =
        //                 static_cast<const SMDS_MeshNode*>( nIt->next() );
        //               if ( uvMap.find( n ) == uvMap.end() )
        //                 uvCheckNodes.push_back( n );
        //             }
        //           }
        //         }
      }
      // check UV on face
      list< const SMDS_MeshNode* >::iterator n = uvCheckNodes.begin();
      for ( ; n != uvCheckNodes.end(); ++n ) {
        node = *n;
        gp_XY uv( 0, 0 );
        const SMDS_PositionPtr& pos = node->GetPosition();
        posType = pos ? pos->GetTypeOfPosition() : SMDS_TOP_3DSPACE;
        // get existing UV
        if ( pos )
        {
          bool toCheck = true;
          uv = helper.GetNodeUV( face, node, nodeInFace, &toCheck );
        }
        // compute not existing UV
        bool project = ( posType == SMDS_TOP_3DSPACE );
        // double dist1 = DBL_MAX, dist2 = 0;
        // if ( posType != SMDS_TOP_3DSPACE ) {
        //   dist1 = pNode.SquareDistance( surface->Value( uv.X(), uv.Y() ));
        //   project = dist1 > fToler2;
        // }
        if ( project ) { // compute new UV
          gp_XY newUV;
          gp_Pnt pNode = SMESH_TNodeXYZ( node );
          if ( !getClosestUV( projector, pNode, newUV )) {
            MESSAGE("Node Projection Failed " << node);
          }
          else {
            if ( isUPeriodic )
              newUV.SetX( ElCLib::InPeriod( newUV.X(), u1, u2 ));
            if ( isVPeriodic )
              newUV.SetY( ElCLib::InPeriod( newUV.Y(), v1, v2 ));
            // check new UV
            // if ( posType != SMDS_TOP_3DSPACE )
            //   dist2 = pNode.SquareDistance( surface->Value( newUV.X(), newUV.Y() ));
            // if ( dist2 < dist1 )
              uv = newUV;
          }
        }
        // store UV in the map
        listUV.push_back( uv );
        uvMap.insert( make_pair( node, &listUV.back() ));
      }
    } // loop on not yet smoothed elements

    if ( !faceSubMesh || nbElemOnFace != faceSubMesh->NbElements() )
      checkBoundaryNodes = true;

    // fix nodes on mesh boundary

    if ( checkBoundaryNodes ) {
      map< SMESH_TLink, int > linkNbMap; // how many times a link encounters in elemsOnFace
      map< SMESH_TLink, int >::iterator link_nb;
      // put all elements links to linkNbMap
      list< const SMDS_MeshElement* >::iterator elemIt = elemsOnFace.begin();
      for ( ; elemIt != elemsOnFace.end(); ++elemIt ) {
        const SMDS_MeshElement* elem = (*elemIt);
        int nbn =  elem->NbCornerNodes();
        // loop on elem links: insert them in linkNbMap
        for ( int iN = 0; iN < nbn; ++iN ) {
          const SMDS_MeshNode* n1 = elem->GetNode( iN );
          const SMDS_MeshNode* n2 = elem->GetNode(( iN+1 ) % nbn);
          SMESH_TLink link( n1, n2 );
          link_nb = linkNbMap.insert( make_pair( link, 0 )).first;
          link_nb->second++;
        }
      }
      // remove nodes that are in links encountered only once from setMovableNodes
      for ( link_nb = linkNbMap.begin(); link_nb != linkNbMap.end(); ++link_nb ) {
        if ( link_nb->second == 1 ) {
          setMovableNodes.erase( link_nb->first.node1() );
          setMovableNodes.erase( link_nb->first.node2() );
        }
      }
    }

    // -----------------------------------------------------
    // for nodes on seam edge, compute one more UV ( uvMap2 );
    // find movable nodes linked to nodes on seam and which
    // are to be smoothed using the second UV ( uvMap2 )
    // -----------------------------------------------------

    set<const SMDS_MeshNode*> nodesNearSeam; // to smooth using uvMap2
    if ( !surface.IsNull() ) {
      TopExp_Explorer eExp( face, TopAbs_EDGE );
      for ( ; eExp.More(); eExp.Next() ) {
        TopoDS_Edge edge = TopoDS::Edge( eExp.Current() );
        if ( !BRep_Tool::IsClosed( edge, face ))
          continue;
        SMESHDS_SubMesh* sm = aMesh->MeshElements( edge );
        if ( !sm ) continue;
        // find out which parameter varies for a node on seam
        double f,l;
        gp_Pnt2d uv1, uv2;
        Handle(Geom2d_Curve) pcurve = BRep_Tool::CurveOnSurface( edge, face, f, l );
        if ( pcurve.IsNull() ) continue;
        uv1 = pcurve->Value( f );
        edge.Reverse();
        pcurve = BRep_Tool::CurveOnSurface( edge, face, f, l );
        if ( pcurve.IsNull() ) continue;
        uv2 = pcurve->Value( f );
        int iPar = Abs( uv1.X() - uv2.X() ) > Abs( uv1.Y() - uv2.Y() ) ? 1 : 2;
        // assure uv1 < uv2
        if ( uv1.Coord( iPar ) > uv2.Coord( iPar ))
          std::swap( uv1, uv2 );
        // get nodes on seam and its vertices
        list< const SMDS_MeshNode* > seamNodes;
        SMDS_NodeIteratorPtr nSeamIt = sm->GetNodes();
        while ( nSeamIt->more() ) {
          const SMDS_MeshNode* node = nSeamIt->next();
          if ( !isQuadratic || !IsMedium( node ))
            seamNodes.push_back( node );
        }
        TopExp_Explorer vExp( edge, TopAbs_VERTEX );
        for ( ; vExp.More(); vExp.Next() ) {
          sm = aMesh->MeshElements( vExp.Current() );
          if ( sm ) {
            nSeamIt = sm->GetNodes();
            while ( nSeamIt->more() )
              seamNodes.push_back( nSeamIt->next() );
          }
        }
        // loop on nodes on seam
        list< const SMDS_MeshNode* >::iterator noSeIt = seamNodes.begin();
        for ( ; noSeIt != seamNodes.end(); ++noSeIt ) {
          const SMDS_MeshNode* nSeam = *noSeIt;
          map< const SMDS_MeshNode*, gp_XY* >::iterator n_uv = uvMap.find( nSeam );
          if ( n_uv == uvMap.end() )
            continue;
          // set the first UV
          n_uv->second->SetCoord( iPar, uv1.Coord( iPar ));
          // set the second UV
          listUV.push_back( *n_uv->second );
          listUV.back().SetCoord( iPar, uv2.Coord( iPar ));
          if ( uvMap2.empty() )
            uvMap2 = uvMap; // copy the uvMap contents
          uvMap2[ nSeam ] = &listUV.back();

          // collect movable nodes linked to ones on seam in nodesNearSeam
          SMDS_ElemIteratorPtr eIt = nSeam->GetInverseElementIterator(SMDSAbs_Face);
          while ( eIt->more() ) {
            const SMDS_MeshElement* e = eIt->next();
            int nbUseMap1 = 0, nbUseMap2 = 0;
            SMDS_ElemIteratorPtr nIt = e->nodesIterator();
            int nn = 0, nbn =  e->NbNodes();
            if(e->IsQuadratic()) nbn = nbn/2;
            while ( nn++ < nbn )
            {
              const SMDS_MeshNode* n =
                static_cast<const SMDS_MeshNode*>( nIt->next() );
              if (n == nSeam ||
                  setMovableNodes.find( n ) == setMovableNodes.end() )
                continue;
              // add only nodes being closer to uv2 than to uv1
              // gp_Pnt pMid (0.5 * ( n->X() + nSeam->X() ),
              //              0.5 * ( n->Y() + nSeam->Y() ),
              //              0.5 * ( n->Z() + nSeam->Z() ));
              // gp_XY uv;
              // getClosestUV( projector, pMid, uv );
              double x = uvMap[ n ]->Coord( iPar );
              if ( Abs( uv1.Coord( iPar ) - x ) >
                   Abs( uv2.Coord( iPar ) - x )) {
                nodesNearSeam.insert( n );
                nbUseMap2++;
              }
              else
                nbUseMap1++;
            }
            // for centroidalSmooth all element nodes must
            // be on one side of a seam
            if ( theSmoothMethod == CENTROIDAL && nbUseMap1 && nbUseMap2 ) {
              SMDS_ElemIteratorPtr nIt = e->nodesIterator();
              nn = 0;
              while ( nn++ < nbn ) {
                const SMDS_MeshNode* n =
                  static_cast<const SMDS_MeshNode*>( nIt->next() );
                setMovableNodes.erase( n );
              }
            }
          }
        } // loop on nodes on seam
      } // loop on edge of a face
    } // if ( !face.IsNull() )

    if ( setMovableNodes.empty() ) {
      MESSAGE( "Face id : " << *fId << " - NO SMOOTHING: no nodes to move!!!");
      continue; // goto next face
    }

    // -------------
    // SMOOTHING //
    // -------------

    int it = -1;
    double maxRatio = -1., maxDisplacement = -1.;
    set<const SMDS_MeshNode*>::iterator nodeToMove;
    for ( it = 0; it < theNbIterations; it++ ) {
      maxDisplacement = 0.;
      nodeToMove = setMovableNodes.begin();
      for ( ; nodeToMove != setMovableNodes.end(); nodeToMove++ ) {
        const SMDS_MeshNode* node = (*nodeToMove);
        gp_XYZ aPrevPos ( node->X(), node->Y(), node->Z() );

        // smooth
        bool map2 = ( nodesNearSeam.find( node ) != nodesNearSeam.end() );
        if ( theSmoothMethod == LAPLACIAN )
          laplacianSmooth( node, surface, map2 ? uvMap2 : uvMap );
        else
          centroidalSmooth( node, surface, map2 ? uvMap2 : uvMap );

        // node displacement
        gp_XYZ aNewPos ( node->X(), node->Y(), node->Z() );
        Standard_Real aDispl = (aPrevPos - aNewPos).SquareModulus();
        if ( aDispl > maxDisplacement )
          maxDisplacement = aDispl;
      }
      // no node movement => exit
      //if ( maxDisplacement < 1.e-16 ) {
      if ( maxDisplacement < disttol ) {
        MESSAGE("-- no node movement --");
        break;
      }

      // check elements quality
      maxRatio  = 0;
      list< const SMDS_MeshElement* >::iterator elemIt = elemsOnFace.begin();
      for ( ; elemIt != elemsOnFace.end(); ++elemIt ) {
        const SMDS_MeshElement* elem = (*elemIt);
        if ( !elem || elem->GetType() != SMDSAbs_Face )
          continue;
        SMESH::Controls::TSequenceOfXYZ aPoints;
        if ( aQualityFunc.GetPoints( elem, aPoints )) {
          double aValue = aQualityFunc.GetValue( aPoints );
          if ( aValue > maxRatio )
            maxRatio = aValue;
        }
      }
      if ( maxRatio <= theTgtAspectRatio ) {
        MESSAGE("-- quality achived --");
        break;
      }
      if (it+1 == theNbIterations) {
        MESSAGE("-- Iteration limit exceeded --");
      }
    } // smoothing iterations

    MESSAGE(" Face id: " << *fId <<
            " Nb iterstions: " << it <<
            " Displacement: " << maxDisplacement <<
            " Aspect Ratio " << maxRatio);

    // ---------------------------------------
    // new nodes positions are computed,
    // record movement in DS and set new UV
    // ---------------------------------------
    nodeToMove = setMovableNodes.begin();
    for ( ; nodeToMove != setMovableNodes.end(); nodeToMove++ ) {
      SMDS_MeshNode* node = const_cast< SMDS_MeshNode* > (*nodeToMove);
      aMesh->MoveNode( node, node->X(), node->Y(), node->Z() );
      map< const SMDS_MeshNode*, gp_XY* >::iterator node_uv = uvMap.find( node );
      if ( node_uv != uvMap.end() ) {
        gp_XY* uv = node_uv->second;
        node->SetPosition
          ( SMDS_PositionPtr( new SMDS_FacePosition( uv->X(), uv->Y() )));
      }
    }

    // move medium nodes of quadratic elements
    if ( isQuadratic )
    {
      vector<const SMDS_MeshNode*> nodes;
      bool checkUV;
      list< const SMDS_MeshElement* >::iterator elemIt = elemsOnFace.begin();
      for ( ; elemIt != elemsOnFace.end(); ++elemIt )
      {
        const SMDS_MeshElement* QF = *elemIt;
        if ( QF->IsQuadratic() )
        {
          nodes.assign( SMDS_MeshElement::iterator( QF->interlacedNodesElemIterator() ),
                        SMDS_MeshElement::iterator() );
          nodes.push_back( nodes[0] );
          gp_Pnt xyz;
          for (size_t i = 1; i < nodes.size(); i += 2 ) // i points to a medium node
          {
            if ( !surface.IsNull() )
            {
              gp_XY uv1 = helper.GetNodeUV( face, nodes[i-1], nodes[i+1], &checkUV );
              gp_XY uv2 = helper.GetNodeUV( face, nodes[i+1], nodes[i-1], &checkUV );
              gp_XY uv  = helper.GetMiddleUV( surface, uv1, uv2 );
              xyz = surface->Value( uv.X(), uv.Y() );
            }
            else {
              xyz = 0.5 * ( SMESH_TNodeXYZ( nodes[i-1] ) + SMESH_TNodeXYZ( nodes[i+1] ));
            }
            if (( SMESH_TNodeXYZ( nodes[i] ) - xyz.XYZ() ).Modulus() > disttol )
              // we have to move a medium node
              aMesh->MoveNode( nodes[i], xyz.X(), xyz.Y(), xyz.Z() );
          }
        }
      }
    }

  } // loop on face ids

}

namespace
{
  //=======================================================================
  //function : isReverse
  //purpose  : Return true if normal of prevNodes is not co-directied with
  //           gp_Vec(prevNodes[iNotSame],nextNodes[iNotSame]).
  //           iNotSame is where prevNodes and nextNodes are different.
  //           If result is true then future volume orientation is OK
  //=======================================================================

  bool isReverse(const SMDS_MeshElement*             face,
                 const vector<const SMDS_MeshNode*>& prevNodes,
                 const vector<const SMDS_MeshNode*>& nextNodes,
                 const int                           iNotSame)
  {

    SMESH_TNodeXYZ pP = prevNodes[ iNotSame ];
    SMESH_TNodeXYZ pN = nextNodes[ iNotSame ];
    gp_XYZ extrDir( pN - pP ), faceNorm;
    SMESH_MeshAlgos::FaceNormal( face, faceNorm, /*normalized=*/false );

    return faceNorm * extrDir < 0.0;
  }

  //================================================================================
  /*!
   * \brief Assure that theElemSets[0] holds elements, not nodes
   */
  //================================================================================

  void setElemsFirst( TIDSortedElemSet theElemSets[2] )
  {
    if ( !theElemSets[0].empty() &&
         (*theElemSets[0].begin())->GetType() == SMDSAbs_Node )
    {
      std::swap( theElemSets[0], theElemSets[1] );
    }
    else if ( !theElemSets[1].empty() &&
              (*theElemSets[1].begin())->GetType() != SMDSAbs_Node )
    {
      std::swap( theElemSets[0], theElemSets[1] );
    }
  }
}

//=======================================================================
/*!
 * \brief Create elements by sweeping an element
 * \param elem - element to sweep
 * \param newNodesItVec - nodes generated from each node of the element
 * \param newElems - generated elements
 * \param nbSteps - number of sweeping steps
 * \param srcElements - to append elem for each generated element
 */
//=======================================================================

void SMESH_MeshEditor::sweepElement(const SMDS_MeshElement*               elem,
                                    const vector<TNodeOfNodeListMapItr> & newNodesItVec,
                                    list<const SMDS_MeshElement*>&        newElems,
                                    const int                             nbSteps,
                                    SMESH_SequenceOfElemPtr&              srcElements)
{
  //MESSAGE("sweepElement " << nbSteps);
  SMESHDS_Mesh* aMesh = GetMeshDS();

  const int           nbNodes = elem->NbNodes();
  const int         nbCorners = elem->NbCornerNodes();
  SMDSAbs_EntityType baseType = elem->GetEntityType(); /* it can change in case of
                                                          polyhedron creation !!! */
  // Loop on elem nodes:
  // find new nodes and detect same nodes indices
  vector < list< const SMDS_MeshNode* >::const_iterator > itNN( nbNodes );
  vector<const SMDS_MeshNode*> prevNod( nbNodes );
  vector<const SMDS_MeshNode*> nextNod( nbNodes );
  vector<const SMDS_MeshNode*> midlNod( nbNodes );

  int iNode, nbSame = 0, nbDouble = 0, iNotSameNode = 0;
  vector<int> sames(nbNodes);
  vector<bool> isSingleNode(nbNodes);

  for ( iNode = 0; iNode < nbNodes; iNode++ ) {
    TNodeOfNodeListMapItr                        nnIt = newNodesItVec[ iNode ];
    const SMDS_MeshNode*                         node = nnIt->first;
    const list< const SMDS_MeshNode* > & listNewNodes = nnIt->second;
    if ( listNewNodes.empty() )
      return;

    itNN   [ iNode ] = listNewNodes.begin();
    prevNod[ iNode ] = node;
    nextNod[ iNode ] = listNewNodes.front();

    isSingleNode[iNode] = (listNewNodes.size()==nbSteps); /* medium node of quadratic or
                                                             corner node of linear */
    if ( prevNod[ iNode ] != nextNod [ iNode ])
      nbDouble += !isSingleNode[iNode];

    if( iNode < nbCorners ) { // check corners only
      if ( prevNod[ iNode ] == nextNod [ iNode ])
        sames[nbSame++] = iNode;
      else
        iNotSameNode = iNode;
    }
  }

  if ( nbSame == nbNodes || nbSame > 2) {
    MESSAGE( " Too many same nodes of element " << elem->GetID() );
    return;
  }

  if ( elem->GetType() == SMDSAbs_Face && !isReverse( elem, prevNod, nextNod, iNotSameNode ))
  {
    // fix nodes order to have bottom normal external
    if ( baseType == SMDSEntity_Polygon )
    {
      std::reverse( itNN.begin(), itNN.end() );
      std::reverse( prevNod.begin(), prevNod.end() );
      std::reverse( midlNod.begin(), midlNod.end() );
      std::reverse( nextNod.begin(), nextNod.end() );
      std::reverse( isSingleNode.begin(), isSingleNode.end() );
    }
    else
    {
      const vector<int>& ind = SMDS_MeshCell::reverseSmdsOrder( baseType, nbNodes );
      SMDS_MeshCell::applyInterlace( ind, itNN );
      SMDS_MeshCell::applyInterlace( ind, prevNod );
      SMDS_MeshCell::applyInterlace( ind, nextNod );
      SMDS_MeshCell::applyInterlace( ind, midlNod );
      SMDS_MeshCell::applyInterlace( ind, isSingleNode );
      if ( nbSame > 0 )
      {
        sames[nbSame] = iNotSameNode;
        for ( int j = 0; j <= nbSame; ++j )
          for ( size_t i = 0; i < ind.size(); ++i )
            if ( ind[i] == sames[j] )
            {
              sames[j] = i;
              break;
            }
        iNotSameNode = sames[nbSame];
      }
    }
  }
  else if ( elem->GetType() == SMDSAbs_Edge )
  {
    // orient a new face same as adjacent one
    int i1, i2;
    const SMDS_MeshElement* e;
    TIDSortedElemSet dummy;
    if (( e = SMESH_MeshAlgos::FindFaceInSet( nextNod[0], prevNod[0], dummy,dummy, &i1, &i2 )) ||
        ( e = SMESH_MeshAlgos::FindFaceInSet( prevNod[1], nextNod[1], dummy,dummy, &i1, &i2 )) ||
        ( e = SMESH_MeshAlgos::FindFaceInSet( prevNod[0], prevNod[1], dummy,dummy, &i1, &i2 )))
    {
      // there is an adjacent face, check order of nodes in it
      bool sameOrder = ( Abs( i2 - i1 ) == 1 ) ? ( i2 > i1 ) : ( i2 < i1 );
      if ( sameOrder )
      {
        std::swap( itNN[0],    itNN[1] );
        std::swap( prevNod[0], prevNod[1] );
        std::swap( nextNod[0], nextNod[1] );

        // See #0002784 for error message
        //std::swap( isSingleNode[0], isSingleNode[1] );
        bool temp = isSingleNode[0];
        isSingleNode[0] = isSingleNode[1];
        isSingleNode[1] = temp;

        if ( nbSame > 0 )
          sames[0] = 1 - sames[0];
        iNotSameNode = 1 - iNotSameNode;
      }
    }
  }

  int iSameNode = 0, iBeforeSame = 0, iAfterSame = 0, iOpposSame = 0;
  if ( nbSame > 0 ) {
    iSameNode    = sames[ nbSame-1 ];
    iBeforeSame  = ( iSameNode + nbCorners - 1 ) % nbCorners;
    iAfterSame   = ( iSameNode + 1 ) % nbCorners;
    iOpposSame   = ( iSameNode - 2 < 0  ? iSameNode + 2 : iSameNode - 2 );
  }

  if ( baseType == SMDSEntity_Polygon )
  {
    if      ( nbNodes == 3 ) baseType = SMDSEntity_Triangle;
    else if ( nbNodes == 4 ) baseType = SMDSEntity_Quadrangle;
  }
#ifndef VTK_NO_QUAD_POLY
  else if ( baseType == SMDSEntity_Quad_Polygon )
  {
    if      ( nbNodes == 6 ) baseType = SMDSEntity_Quad_Triangle;
    else if ( nbNodes == 8 ) baseType = SMDSEntity_Quad_Quadrangle;
  }
#endif

  // make new elements
  for (int iStep = 0; iStep < nbSteps; iStep++ )
  {
    // get next nodes
    for ( iNode = 0; iNode < nbNodes; iNode++ )
    {
      midlNod[ iNode ] = isSingleNode[iNode] ? 0 : *itNN[ iNode ]++;
      nextNod[ iNode ] = *itNN[ iNode ]++;
    }

    SMDS_MeshElement* aNewElem = 0;
    /*if(!elem->IsPoly())*/ {
      switch ( baseType ) {
      case SMDSEntity_0D:
      case SMDSEntity_Node: { // sweep NODE
        if ( nbSame == 0 ) {
          if ( isSingleNode[0] )
            aNewElem = aMesh->AddEdge( prevNod[ 0 ], nextNod[ 0 ] );
          else
            aNewElem = aMesh->AddEdge( prevNod[ 0 ], nextNod[ 0 ], midlNod[ 0 ] );
        }
        else
          return;
        break;
      }
      case SMDSEntity_Edge: { // sweep EDGE
        if ( nbDouble == 0 )
        {
          if ( nbSame == 0 ) // ---> quadrangle
            aNewElem = aMesh->AddFace(prevNod[ 0 ], prevNod[ 1 ],
                                      nextNod[ 1 ], nextNod[ 0 ] );
          else               // ---> triangle
            aNewElem = aMesh->AddFace(prevNod[ 0 ], prevNod[ 1 ],
                                      nextNod[ iNotSameNode ] );
        }
        else                 // ---> polygon
        {
          vector<const SMDS_MeshNode*> poly_nodes;
          poly_nodes.push_back( prevNod[0] );
          poly_nodes.push_back( prevNod[1] );
          if ( prevNod[1] != nextNod[1] )
          {
            if ( midlNod[1]) poly_nodes.push_back( midlNod[1]);
            poly_nodes.push_back( nextNod[1] );
          }
          if ( prevNod[0] != nextNod[0] )
          {
            poly_nodes.push_back( nextNod[0] );
            if ( midlNod[0]) poly_nodes.push_back( midlNod[0]);
          }
          switch ( poly_nodes.size() ) {
          case 3:
            aNewElem = aMesh->AddFace( poly_nodes[ 0 ], poly_nodes[ 1 ], poly_nodes[ 2 ]);
            break;
          case 4:
            aNewElem = aMesh->AddFace( poly_nodes[ 0 ], poly_nodes[ 1 ],
                                       poly_nodes[ 2 ], poly_nodes[ 3 ]);
            break;
          default:
            aNewElem = aMesh->AddPolygonalFace (poly_nodes);
          }
        }
        break;
      }
      case SMDSEntity_Triangle: // TRIANGLE --->
        {
          if ( nbDouble > 0 ) break;
          if ( nbSame == 0 )       // ---> pentahedron
            aNewElem = aMesh->AddVolume (prevNod[ 0 ], prevNod[ 1 ], prevNod[ 2 ],
                                         nextNod[ 0 ], nextNod[ 1 ], nextNod[ 2 ] );

          else if ( nbSame == 1 )  // ---> pyramid
            aNewElem = aMesh->AddVolume (prevNod[ iBeforeSame ], prevNod[ iAfterSame ],
                                         nextNod[ iAfterSame ],  nextNod[ iBeforeSame ],
                                         nextNod[ iSameNode ]);

          else // 2 same nodes:       ---> tetrahedron
            aNewElem = aMesh->AddVolume (prevNod[ 0 ], prevNod[ 1 ], prevNod[ 2 ],
                                         nextNod[ iNotSameNode ]);
          break;
        }
      case SMDSEntity_Quad_Edge: // sweep quadratic EDGE --->
        {
          if ( nbSame == 2 )
            return;
          if ( nbDouble+nbSame == 2 )
          {
            if(nbSame==0) {      // ---> quadratic quadrangle
              aNewElem = aMesh->AddFace(prevNod[0], prevNod[1], nextNod[1], nextNod[0],
                                        prevNod[2], midlNod[1], nextNod[2], midlNod[0]);
            }
            else { //(nbSame==1) // ---> quadratic triangle
              if(sames[0]==2) {
                return; // medium node on axis
              }
              else if(sames[0]==0)
                aNewElem = aMesh->AddFace(prevNod[0], prevNod[1], nextNod[1],
                                          prevNod[2], midlNod[1], nextNod[2] );
              else // sames[0]==1
                aNewElem = aMesh->AddFace(prevNod[0], prevNod[1], nextNod[0],
                                          prevNod[2], nextNod[2], midlNod[0]);
            }
          }
          else if ( nbDouble == 3 )
          {
            if ( nbSame == 0 ) {  // ---> bi-quadratic quadrangle
              aNewElem = aMesh->AddFace(prevNod[0], prevNod[1], nextNod[1], nextNod[0],
                                        prevNod[2], midlNod[1], nextNod[2], midlNod[0], midlNod[2]);
            }
          }
          else
            return;
          break;
        }
      case SMDSEntity_Quadrangle: { // sweep QUADRANGLE --->
        if ( nbDouble > 0 ) break;

        if ( nbSame == 0 )       // ---> hexahedron
          aNewElem = aMesh->AddVolume (prevNod[ 0 ], prevNod[ 1 ], prevNod[ 2 ], prevNod[ 3 ],
                                       nextNod[ 0 ], nextNod[ 1 ], nextNod[ 2 ], nextNod[ 3 ]);

        else if ( nbSame == 1 ) { // ---> pyramid + pentahedron
          aNewElem = aMesh->AddVolume (prevNod[ iBeforeSame ], prevNod[ iAfterSame ],
                                       nextNod[ iAfterSame ],  nextNod[ iBeforeSame ],
                                       nextNod[ iSameNode ]);
          newElems.push_back( aNewElem );
          aNewElem = aMesh->AddVolume (prevNod[ iAfterSame ],  prevNod[ iOpposSame ],
                                       prevNod[ iBeforeSame ], nextNod[ iAfterSame ],
                                       nextNod[ iOpposSame ],  nextNod[ iBeforeSame ] );
        }
        else if ( nbSame == 2 ) { // ---> pentahedron
          if ( prevNod[ iBeforeSame ] == nextNod[ iBeforeSame ] )
            // iBeforeSame is same too
            aNewElem = aMesh->AddVolume (prevNod[ iBeforeSame ], prevNod[ iOpposSame ],
                                         nextNod[ iOpposSame ],  prevNod[ iSameNode ],
                                         prevNod[ iAfterSame ],  nextNod[ iAfterSame ]);
          else
            // iAfterSame is same too
            aNewElem = aMesh->AddVolume (prevNod[ iSameNode ],   prevNod[ iBeforeSame ],
                                         nextNod[ iBeforeSame ], prevNod[ iAfterSame ],
                                         prevNod[ iOpposSame ],  nextNod[ iOpposSame ]);
        }
        break;
      }
      case SMDSEntity_Quad_Triangle:  // sweep (Bi)Quadratic TRIANGLE --->
      case SMDSEntity_BiQuad_Triangle: /* ??? */ {
        if ( nbDouble+nbSame != 3 ) break;
        if(nbSame==0) {
          // --->  pentahedron with 15 nodes
          aNewElem = aMesh->AddVolume (prevNod[0], prevNod[1], prevNod[2],
                                       nextNod[0], nextNod[1], nextNod[2],
                                       prevNod[3], prevNod[4], prevNod[5],
                                       nextNod[3], nextNod[4], nextNod[5],
                                       midlNod[0], midlNod[1], midlNod[2]);
        }
        else if(nbSame==1) {
          // --->  2d order pyramid of 13 nodes
          int apex = iSameNode;
          int i0 = ( apex + 1 ) % nbCorners;
          int i1 = ( apex - 1 + nbCorners ) % nbCorners;
          int i0a = apex + 3;
          int i1a = i1 + 3;
          int i01 = i0 + 3;
          aNewElem = aMesh->AddVolume(prevNod[i1], prevNod[i0],
                                      nextNod[i0], nextNod[i1], prevNod[apex],
                                      prevNod[i01], midlNod[i0],
                                      nextNod[i01], midlNod[i1],
                                      prevNod[i1a], prevNod[i0a],
                                      nextNod[i0a], nextNod[i1a]);
        }
        else if(nbSame==2) {
          // --->  2d order tetrahedron of 10 nodes
          int n1 = iNotSameNode;
          int n2 = ( n1 + 1             ) % nbCorners;
          int n3 = ( n1 + nbCorners - 1 ) % nbCorners;
          int n12 = n1 + 3;
          int n23 = n2 + 3;
          int n31 = n3 + 3;
          aNewElem = aMesh->AddVolume (prevNod[n1], prevNod[n2], prevNod[n3], nextNod[n1],
                                       prevNod[n12], prevNod[n23], prevNod[n31],
                                       midlNod[n1], nextNod[n12], nextNod[n31]);
        }
        break;
      }
      case SMDSEntity_Quad_Quadrangle: { // sweep Quadratic QUADRANGLE --->
        if( nbSame == 0 ) {
          if ( nbDouble != 4 ) break;
          // --->  hexahedron with 20 nodes
          aNewElem = aMesh->AddVolume (prevNod[0], prevNod[1], prevNod[2], prevNod[3],
                                       nextNod[0], nextNod[1], nextNod[2], nextNod[3],
                                       prevNod[4], prevNod[5], prevNod[6], prevNod[7],
                                       nextNod[4], nextNod[5], nextNod[6], nextNod[7],
                                       midlNod[0], midlNod[1], midlNod[2], midlNod[3]);
        }
        else if(nbSame==1) {
          // ---> pyramid + pentahedron - can not be created since it is needed
          // additional middle node at the center of face
          //INFOS( " Sweep for face " << elem->GetID() << " can not be created" );
          return;
        }
        else if( nbSame == 2 ) {
          if ( nbDouble != 2 ) break;
          // --->  2d order Pentahedron with 15 nodes
          int n1,n2,n4,n5;
          if ( prevNod[ iBeforeSame ] == nextNod[ iBeforeSame ] ) {
            // iBeforeSame is same too
            n1 = iBeforeSame;
            n2 = iOpposSame;
            n4 = iSameNode;
            n5 = iAfterSame;
          }
          else {
            // iAfterSame is same too
            n1 = iSameNode;
            n2 = iBeforeSame;
            n4 = iAfterSame;
            n5 = iOpposSame;
          }
          int n12 = n2 + 4;
          int n45 = n4 + 4;
          int n14 = n1 + 4;
          int n25 = n5 + 4;
          aNewElem = aMesh->AddVolume (prevNod[n1], prevNod[n2], nextNod[n2],
                                       prevNod[n4], prevNod[n5], nextNod[n5],
                                       prevNod[n12], midlNod[n2], nextNod[n12],
                                       prevNod[n45], midlNod[n5], nextNod[n45],
                                       prevNod[n14], prevNod[n25], nextNod[n25]);
        }
        break;
      }
      case SMDSEntity_BiQuad_Quadrangle: { // sweep BiQuadratic QUADRANGLE --->

        if( nbSame == 0 && nbDouble == 9 ) {
          // --->  tri-quadratic hexahedron with 27 nodes
          aNewElem = aMesh->AddVolume (prevNod[0], prevNod[1], prevNod[2], prevNod[3],
                                       nextNod[0], nextNod[1], nextNod[2], nextNod[3],
                                       prevNod[4], prevNod[5], prevNod[6], prevNod[7],
                                       nextNod[4], nextNod[5], nextNod[6], nextNod[7],
                                       midlNod[0], midlNod[1], midlNod[2], midlNod[3],
                                       prevNod[8], // bottom center
                                       midlNod[4], midlNod[5], midlNod[6], midlNod[7],
                                       nextNod[8], // top center
                                       midlNod[8]);// elem center
        }
        else
        {
          return;
        }
        break;
      }
      case SMDSEntity_Polygon: { // sweep POLYGON

        if ( nbNodes == 6 && nbSame == 0 && nbDouble == 0 ) {
          // --->  hexagonal prism
          aNewElem = aMesh->AddVolume (prevNod[0], prevNod[1], prevNod[2],
                                       prevNod[3], prevNod[4], prevNod[5],
                                       nextNod[0], nextNod[1], nextNod[2],
                                       nextNod[3], nextNod[4], nextNod[5]);
        }
        break;
      }
      case SMDSEntity_Ball:
        return;

      default:
        break;
      } // switch ( baseType )
    } // scope

    if ( !aNewElem && elem->GetType() == SMDSAbs_Face ) // try to create a polyherdal prism
    {
      if ( baseType != SMDSEntity_Polygon )
      {
        const std::vector<int>& ind = SMDS_MeshCell::interlacedSmdsOrder(baseType,nbNodes);
        SMDS_MeshCell::applyInterlace( ind, prevNod );
        SMDS_MeshCell::applyInterlace( ind, nextNod );
        SMDS_MeshCell::applyInterlace( ind, midlNod );
        SMDS_MeshCell::applyInterlace( ind, itNN );
        SMDS_MeshCell::applyInterlace( ind, isSingleNode );
        baseType = SMDSEntity_Polygon; // WARNING: change baseType !!!!
      }
      vector<const SMDS_MeshNode*> polyedre_nodes (nbNodes*2 + 4*nbNodes);
      vector<int> quantities (nbNodes + 2);
      polyedre_nodes.clear();
      quantities.clear();

      // bottom of prism
      for (int inode = 0; inode < nbNodes; inode++)
        polyedre_nodes.push_back( prevNod[inode] );
      quantities.push_back( nbNodes );

      // top of prism
      polyedre_nodes.push_back( nextNod[0] );
      for (int inode = nbNodes; inode-1; --inode )
        polyedre_nodes.push_back( nextNod[inode-1] );
      quantities.push_back( nbNodes );

      // side faces
      // 3--6--2
      // |     |
      // 7     5
      // |     |
      // 0--4--1
      const int iQuad = elem->IsQuadratic();
      for (int iface = 0; iface < nbNodes; iface += 1+iQuad )
      {
        const int prevNbNodes = polyedre_nodes.size(); // to detect degenerated face
        int inextface = (iface+1+iQuad) % nbNodes;
        int imid      = (iface+1) % nbNodes;
        polyedre_nodes.push_back( prevNod[inextface] );         // 0
        if ( iQuad ) polyedre_nodes.push_back( prevNod[imid] ); // 4
        polyedre_nodes.push_back( prevNod[iface] );             // 1
        if ( prevNod[iface] != nextNod[iface] ) // 1 != 2
        {
          if ( midlNod[ iface ]) polyedre_nodes.push_back( midlNod[ iface ]); // 5
          polyedre_nodes.push_back( nextNod[iface] );                         // 2
        }
        if ( iQuad ) polyedre_nodes.push_back( nextNod[imid] );               // 6
        if ( prevNod[inextface] != nextNod[inextface] ) // 0 != 3
        {
          polyedre_nodes.push_back( nextNod[inextface] );                            // 3
          if ( midlNod[ inextface ]) polyedre_nodes.push_back( midlNod[ inextface ]);// 7
        }
        const int nbFaceNodes = polyedre_nodes.size() - prevNbNodes;
        if ( nbFaceNodes > 2 )
          quantities.push_back( nbFaceNodes );
        else // degenerated face
          polyedre_nodes.resize( prevNbNodes );
      }
      aNewElem = aMesh->AddPolyhedralVolume (polyedre_nodes, quantities);

    } // try to create a polyherdal prism

    if ( aNewElem ) {
      newElems.push_back( aNewElem );
      myLastCreatedElems.Append(aNewElem);
      srcElements.Append( elem );
    }

    // set new prev nodes
    for ( iNode = 0; iNode < nbNodes; iNode++ )
      prevNod[ iNode ] = nextNod[ iNode ];

  } // loop on steps
}

//=======================================================================
/*!
 * \brief Create 1D and 2D elements around swept elements
 * \param mapNewNodes - source nodes and ones generated from them
 * \param newElemsMap - source elements and ones generated from them
 * \param elemNewNodesMap - nodes generated from each node of each element
 * \param elemSet - all swept elements
 * \param nbSteps - number of sweeping steps
 * \param srcElements - to append elem for each generated element
 */
//=======================================================================

void SMESH_MeshEditor::makeWalls (TNodeOfNodeListMap &     mapNewNodes,
                                  TTElemOfElemListMap &    newElemsMap,
                                  TElemOfVecOfNnlmiMap &   elemNewNodesMap,
                                  TIDSortedElemSet&        elemSet,
                                  const int                nbSteps,
                                  SMESH_SequenceOfElemPtr& srcElements)
{
  ASSERT( newElemsMap.size() == elemNewNodesMap.size() );
  SMESHDS_Mesh* aMesh = GetMeshDS();

  // Find nodes belonging to only one initial element - sweep them into edges.

  TNodeOfNodeListMapItr nList = mapNewNodes.begin();
  for ( ; nList != mapNewNodes.end(); nList++ )
  {
    const SMDS_MeshNode* node =
      static_cast<const SMDS_MeshNode*>( nList->first );
    if ( newElemsMap.count( node ))
      continue; // node was extruded into edge
    SMDS_ElemIteratorPtr eIt = node->GetInverseElementIterator();
    int nbInitElems = 0;
    const SMDS_MeshElement* el = 0;
    SMDSAbs_ElementType highType = SMDSAbs_Edge; // count most complex elements only
    while ( eIt->more() && nbInitElems < 2 ) {
      const SMDS_MeshElement* e = eIt->next();
      SMDSAbs_ElementType  type = e->GetType();
      if ( type == SMDSAbs_Volume ||
           type < highType ||
           !elemSet.count(e))
        continue;
      if ( type > highType ) {
        nbInitElems = 0;
        highType    = type;
      }
      el = e;
      ++nbInitElems;
    }
    if ( nbInitElems == 1 ) {
      bool NotCreateEdge = el && el->IsMediumNode(node);
      if(!NotCreateEdge) {
        vector<TNodeOfNodeListMapItr> newNodesItVec( 1, nList );
        list<const SMDS_MeshElement*> newEdges;
        sweepElement( node, newNodesItVec, newEdges, nbSteps, srcElements );
      }
    }
  }

  // Make a ceiling for each element ie an equal element of last new nodes.
  // Find free links of faces - make edges and sweep them into faces.

  ElemFeatures polyFace( SMDSAbs_Face, /*isPoly=*/true ), anyFace;

  TTElemOfElemListMap::iterator  itElem      = newElemsMap.begin();
  TElemOfVecOfNnlmiMap::iterator itElemNodes = elemNewNodesMap.begin();
  for ( ; itElem != newElemsMap.end(); itElem++, itElemNodes++ )
  {
    const SMDS_MeshElement* elem = itElem->first;
    vector<TNodeOfNodeListMapItr>& vecNewNodes = itElemNodes->second;

    if(itElem->second.size()==0) continue;

    const bool isQuadratic = elem->IsQuadratic();

    if ( elem->GetType() == SMDSAbs_Edge ) {
      // create a ceiling edge
      if ( !isQuadratic ) {
        if ( !aMesh->FindEdge( vecNewNodes[ 0 ]->second.back(),
                               vecNewNodes[ 1 ]->second.back())) {
          myLastCreatedElems.Append(aMesh->AddEdge(vecNewNodes[ 0 ]->second.back(),
                                                   vecNewNodes[ 1 ]->second.back()));
          srcElements.Append( elem );
        }
      }
      else {
        if ( !aMesh->FindEdge( vecNewNodes[ 0 ]->second.back(),
                               vecNewNodes[ 1 ]->second.back(),
                               vecNewNodes[ 2 ]->second.back())) {
          myLastCreatedElems.Append(aMesh->AddEdge(vecNewNodes[ 0 ]->second.back(),
                                                   vecNewNodes[ 1 ]->second.back(),
                                                   vecNewNodes[ 2 ]->second.back()));
          srcElements.Append( elem );
        }
      }
    }
    if ( elem->GetType() != SMDSAbs_Face )
      continue;

    bool hasFreeLinks = false;

    TIDSortedElemSet avoidSet;
    avoidSet.insert( elem );

    set<const SMDS_MeshNode*> aFaceLastNodes;
    int iNode, nbNodes = vecNewNodes.size();
    if ( !isQuadratic ) {
      // loop on the face nodes
      for ( iNode = 0; iNode < nbNodes; iNode++ ) {
        aFaceLastNodes.insert( vecNewNodes[ iNode ]->second.back() );
        // look for free links of the face
        int iNext = ( iNode + 1 == nbNodes ) ? 0 : iNode + 1;
        const SMDS_MeshNode* n1 = vecNewNodes[ iNode ]->first;
        const SMDS_MeshNode* n2 = vecNewNodes[ iNext ]->first;
        // check if a link n1-n2 is free
        if ( ! SMESH_MeshAlgos::FindFaceInSet ( n1, n2, elemSet, avoidSet )) {
          hasFreeLinks = true;
          // make a new edge and a ceiling for a new edge
          const SMDS_MeshElement* edge;
          if ( ! ( edge = aMesh->FindEdge( n1, n2 ))) {
            myLastCreatedElems.Append( edge = aMesh->AddEdge( n1, n2 )); // free link edge
            srcElements.Append( myLastCreatedElems.Last() );
          }
          n1 = vecNewNodes[ iNode ]->second.back();
          n2 = vecNewNodes[ iNext ]->second.back();
          if ( !aMesh->FindEdge( n1, n2 )) {
            myLastCreatedElems.Append(aMesh->AddEdge( n1, n2 )); // new edge ceiling
            srcElements.Append( edge );
          }
        }
      }
    }
    else { // elem is quadratic face
      int nbn = nbNodes/2;
      for ( iNode = 0; iNode < nbn; iNode++ ) {
        aFaceLastNodes.insert( vecNewNodes[ iNode ]->second.back() );
        int iNext = ( iNode + 1 == nbn ) ? 0 : iNode + 1;
        const SMDS_MeshNode* n1 = vecNewNodes[ iNode ]->first;
        const SMDS_MeshNode* n2 = vecNewNodes[ iNext ]->first;
        const SMDS_MeshNode* n3 = vecNewNodes[ iNode+nbn ]->first;
        // check if a link is free
        if ( ! SMESH_MeshAlgos::FindFaceInSet ( n1, n2, elemSet, avoidSet ) &&
             ! SMESH_MeshAlgos::FindFaceInSet ( n1, n3, elemSet, avoidSet ) &&
             ! SMESH_MeshAlgos::FindFaceInSet ( n3, n2, elemSet, avoidSet ) ) {
          hasFreeLinks = true;
          // make an edge and a ceiling for a new edge
          // find medium node
          if ( !aMesh->FindEdge( n1, n2, n3 )) {
            myLastCreatedElems.Append(aMesh->AddEdge( n1, n2, n3 )); // free link edge
            srcElements.Append( elem );
          }
          n1 = vecNewNodes[ iNode ]->second.back();
          n2 = vecNewNodes[ iNext ]->second.back();
          n3 = vecNewNodes[ iNode+nbn ]->second.back();
          if ( !aMesh->FindEdge( n1, n2, n3 )) {
            myLastCreatedElems.Append(aMesh->AddEdge( n1, n2, n3 )); // ceiling edge
            srcElements.Append( elem );
          }
        }
      }
      for ( iNode = nbn; iNode < nbNodes; iNode++ ) {
        aFaceLastNodes.insert( vecNewNodes[ iNode ]->second.back() );
      }
    }

    // sweep free links into faces

    if ( hasFreeLinks ) {
      list<const SMDS_MeshElement*> & newVolumes = itElem->second;
      int iVol, volNb, nbVolumesByStep = newVolumes.size() / nbSteps;

      set<const SMDS_MeshNode*> initNodeSet, topNodeSet, faceNodeSet;
      set<const SMDS_MeshNode*> initNodeSetNoCenter/*, topNodeSetNoCenter*/;
      for ( iNode = 0; iNode < nbNodes; iNode++ ) {
        initNodeSet.insert( vecNewNodes[ iNode ]->first );
        topNodeSet .insert( vecNewNodes[ iNode ]->second.back() );
      }
      if ( isQuadratic && nbNodes % 2 ) {  // node set for the case of a biquadratic
        initNodeSetNoCenter = initNodeSet; // swept face and a not biquadratic volume
        initNodeSetNoCenter.erase( vecNewNodes.back()->first );
      }
      for ( volNb = 0; volNb < nbVolumesByStep; volNb++ ) {
        list<const SMDS_MeshElement*>::iterator v = newVolumes.begin();
        std::advance( v, volNb );
        // find indices of free faces of a volume and their source edges
        list< int > freeInd;
        list< const SMDS_MeshElement* > srcEdges; // source edges of free faces
        SMDS_VolumeTool vTool( *v, /*ignoreCentralNodes=*/false );
        int iF, nbF = vTool.NbFaces();
        for ( iF = 0; iF < nbF; iF ++ ) {
          if (vTool.IsFreeFace( iF ) &&
              vTool.GetFaceNodes( iF, faceNodeSet ) &&
              initNodeSet != faceNodeSet) // except an initial face
          {
            if ( nbSteps == 1 && faceNodeSet == topNodeSet )
              continue;
            if ( faceNodeSet == initNodeSetNoCenter )
              continue;
            freeInd.push_back( iF );
            // find source edge of a free face iF
            vector<const SMDS_MeshNode*> commonNodes; // shared by the initial and free faces
            vector<const SMDS_MeshNode*>::iterator lastCommom;
            commonNodes.resize( nbNodes, 0 );
            lastCommom = std::set_intersection( faceNodeSet.begin(), faceNodeSet.end(),
                                                initNodeSet.begin(), initNodeSet.end(),
                                                commonNodes.begin());
            if ( std::distance( commonNodes.begin(), lastCommom ) == 3 )
              srcEdges.push_back(aMesh->FindEdge (commonNodes[0],commonNodes[1],commonNodes[2]));
            else
              srcEdges.push_back(aMesh->FindEdge (commonNodes[0],commonNodes[1]));
#ifdef _DEBUG_
            if ( !srcEdges.back() )
            {
              cout << "SMESH_MeshEditor::makeWalls(), no source edge found for a free face #"
                   << iF << " of volume #" << vTool.ID() << endl;
            }
#endif
          }
        }
        if ( freeInd.empty() )
          continue;

        // create wall faces for all steps;
        // if such a face has been already created by sweep of edge,
        // assure that its orientation is OK
        for ( int iStep = 0; iStep < nbSteps; iStep++ )
        {
          vTool.Set( *v, /*ignoreCentralNodes=*/false );
          vTool.SetExternalNormal();
          const int nextShift = vTool.IsForward() ? +1 : -1;
          list< int >::iterator ind = freeInd.begin();
          list< const SMDS_MeshElement* >::iterator srcEdge = srcEdges.begin();
          for ( ; ind != freeInd.end(); ++ind, ++srcEdge ) // loop on free faces
          {
            const SMDS_MeshNode** nodes = vTool.GetFaceNodes( *ind );
            int nbn = vTool.NbFaceNodes( *ind );
            const SMDS_MeshElement * f = 0;
            if ( nbn == 3 )              ///// triangle
            {
              f = aMesh->FindFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ]);
              if ( !f ||
                   nodes[ 1 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ]) + nextShift ))
              {
                const SMDS_MeshNode* newOrder[3] = { nodes[ 1 - nextShift ],
                                                     nodes[ 1 ],
                                                     nodes[ 1 + nextShift ] };
                if ( f )
                  aMesh->ChangeElementNodes( f, &newOrder[0], nbn );
                else
                  myLastCreatedElems.Append(aMesh->AddFace( newOrder[ 0 ], newOrder[ 1 ],
                                                            newOrder[ 2 ] ));
              }
            }
            else if ( nbn == 4 )       ///// quadrangle
            {
              f = aMesh->FindFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ], nodes[ 3 ]);
              if ( !f ||
                   nodes[ 1 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ]) + nextShift ))
              {
                const SMDS_MeshNode* newOrder[4] = { nodes[ 0 ], nodes[ 2-nextShift ],
                                                     nodes[ 2 ], nodes[ 2+nextShift ] };
                if ( f )
                  aMesh->ChangeElementNodes( f, &newOrder[0], nbn );
                else
                  myLastCreatedElems.Append(aMesh->AddFace( newOrder[ 0 ], newOrder[ 1 ],
                                                            newOrder[ 2 ], newOrder[ 3 ]));
              }
            }
            else if ( nbn == 6 && isQuadratic ) /////// quadratic triangle
            {
              f = aMesh->FindFace( nodes[0], nodes[2], nodes[4], nodes[1], nodes[3], nodes[5] );
              if ( !f ||
                   nodes[2] != f->GetNodeWrap( f->GetNodeIndex( nodes[0] ) + 2*nextShift ))
              {
                const SMDS_MeshNode* newOrder[6] = { nodes[2 - 2*nextShift],
                                                     nodes[2],
                                                     nodes[2 + 2*nextShift],
                                                     nodes[3 - 2*nextShift],
                                                     nodes[3],
                                                     nodes[3 + 2*nextShift]};
                if ( f )
                  aMesh->ChangeElementNodes( f, &newOrder[0], nbn );
                else
                  myLastCreatedElems.Append(aMesh->AddFace( newOrder[ 0 ],
                                                            newOrder[ 1 ],
                                                            newOrder[ 2 ],
                                                            newOrder[ 3 ],
                                                            newOrder[ 4 ],
                                                            newOrder[ 5 ] ));
              }
            }
            else if ( nbn == 8 && isQuadratic ) /////// quadratic quadrangle
            {
              f = aMesh->FindFace( nodes[0], nodes[2], nodes[4], nodes[6],
                                   nodes[1], nodes[3], nodes[5], nodes[7] );
              if ( !f ||
                   nodes[ 2 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ] ) + 2*nextShift ))
              {
                const SMDS_MeshNode* newOrder[8] = { nodes[0],
                                                     nodes[4 - 2*nextShift],
                                                     nodes[4],
                                                     nodes[4 + 2*nextShift],
                                                     nodes[1],
                                                     nodes[5 - 2*nextShift],
                                                     nodes[5],
                                                     nodes[5 + 2*nextShift] };
                if ( f )
                  aMesh->ChangeElementNodes( f, &newOrder[0], nbn );
                else
                  myLastCreatedElems.Append(aMesh->AddFace(newOrder[ 0 ], newOrder[ 1 ],
                                                           newOrder[ 2 ], newOrder[ 3 ],
                                                           newOrder[ 4 ], newOrder[ 5 ],
                                                           newOrder[ 6 ], newOrder[ 7 ]));
              }
            }
            else if ( nbn == 9 && isQuadratic ) /////// bi-quadratic quadrangle
            {
              f = aMesh->FindElement( vector<const SMDS_MeshNode*>( nodes, nodes+nbn ),
                                      SMDSAbs_Face, /*noMedium=*/false);
              if ( !f ||
                   nodes[ 2 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ] ) + 2*nextShift ))
              {
                const SMDS_MeshNode* newOrder[9] = { nodes[0],
                                                     nodes[4 - 2*nextShift],
                                                     nodes[4],
                                                     nodes[4 + 2*nextShift],
                                                     nodes[1],
                                                     nodes[5 - 2*nextShift],
                                                     nodes[5],
                                                     nodes[5 + 2*nextShift],
                                                     nodes[8] };
                if ( f )
                  aMesh->ChangeElementNodes( f, &newOrder[0], nbn );
                else
                  myLastCreatedElems.Append(aMesh->AddFace(newOrder[ 0 ], newOrder[ 1 ],
                                                           newOrder[ 2 ], newOrder[ 3 ],
                                                           newOrder[ 4 ], newOrder[ 5 ],
                                                           newOrder[ 6 ], newOrder[ 7 ],
                                                           newOrder[ 8 ]));
              }
            }
            else  //////// polygon
            {
              vector<const SMDS_MeshNode*> polygon_nodes ( nodes, nodes+nbn );
              const SMDS_MeshFace * f = aMesh->FindFace( polygon_nodes );
              if ( !f ||
                   nodes[ 1 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ] ) + nextShift ))
              {
                if ( !vTool.IsForward() )
                  std::reverse( polygon_nodes.begin(), polygon_nodes.end());
                if ( f )
                  aMesh->ChangeElementNodes( f, &polygon_nodes[0], nbn );
                else
                  AddElement( polygon_nodes, polyFace.SetQuad( (*v)->IsQuadratic() ));
              }
            }

            while ( srcElements.Length() < myLastCreatedElems.Length() )
              srcElements.Append( *srcEdge );

          }  // loop on free faces

          // go to the next volume
          iVol = 0;
          while ( iVol++ < nbVolumesByStep ) v++;

        } // loop on steps
      } // loop on volumes of one step
    } // sweep free links into faces

    // Make a ceiling face with a normal external to a volume

    // use SMDS_VolumeTool to get a correctly ordered nodes of a ceiling face
    SMDS_VolumeTool lastVol( itElem->second.back(), /*ignoreCentralNodes=*/false );
    int iF = lastVol.GetFaceIndex( aFaceLastNodes );

    if ( iF < 0 && isQuadratic && nbNodes % 2 ) { // remove a central node of biquadratic
      aFaceLastNodes.erase( vecNewNodes.back()->second.back() );
      iF = lastVol.GetFaceIndex( aFaceLastNodes );
    }
    if ( iF >= 0 )
    {
      lastVol.SetExternalNormal();
      const SMDS_MeshNode** nodes = lastVol.GetFaceNodes( iF );
      const               int nbn = lastVol.NbFaceNodes( iF );
      vector<const SMDS_MeshNode*> nodeVec( nodes, nodes+nbn );
      if ( !hasFreeLinks ||
           !aMesh->FindElement( nodeVec, SMDSAbs_Face, /*noMedium=*/false) )
      {
        const vector<int>& interlace =
          SMDS_MeshCell::interlacedSmdsOrder( elem->GetEntityType(), nbn );
        SMDS_MeshCell::applyInterlaceRev( interlace, nodeVec );

        AddElement( nodeVec, anyFace.Init( elem ));

        while ( srcElements.Length() < myLastCreatedElems.Length() )
          srcElements.Append( elem );
      }
    }
  } // loop on swept elements
}

//=======================================================================
//function : RotationSweep
//purpose  :
//=======================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::RotationSweep(TIDSortedElemSet   theElemSets[2],
                                const gp_Ax1&      theAxis,
                                const double       theAngle,
                                const int          theNbSteps,
                                const double       theTol,
                                const bool         theMakeGroups,
                                const bool         theMakeWalls)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  // source elements for each generated one
  SMESH_SequenceOfElemPtr srcElems, srcNodes;

  MESSAGE( "RotationSweep()");
  gp_Trsf aTrsf;
  aTrsf.SetRotation( theAxis, theAngle );
  gp_Trsf aTrsf2;
  aTrsf2.SetRotation( theAxis, theAngle/2. );

  gp_Lin aLine( theAxis );
  double aSqTol = theTol * theTol;

  SMESHDS_Mesh* aMesh = GetMeshDS();

  TNodeOfNodeListMap mapNewNodes;
  TElemOfVecOfNnlmiMap mapElemNewNodes;
  TTElemOfElemListMap newElemsMap;

  const bool isQuadraticMesh = bool( myMesh->NbEdges(ORDER_QUADRATIC) +
                                     myMesh->NbFaces(ORDER_QUADRATIC) +
                                     myMesh->NbVolumes(ORDER_QUADRATIC) );
  // loop on theElemSets
  setElemsFirst( theElemSets );
  TIDSortedElemSet::iterator itElem;
  for ( int is2ndSet = 0; is2ndSet < 2; ++is2ndSet )
  {
    TIDSortedElemSet& theElems = theElemSets[ is2ndSet ];
    for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ ) {
      const SMDS_MeshElement* elem = *itElem;
      if ( !elem || elem->GetType() == SMDSAbs_Volume )
        continue;
      vector<TNodeOfNodeListMapItr> & newNodesItVec = mapElemNewNodes[ elem ];
      newNodesItVec.reserve( elem->NbNodes() );

      // loop on elem nodes
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      while ( itN->more() )
      {
        const SMDS_MeshNode* node = cast2Node( itN->next() );

        gp_XYZ aXYZ( node->X(), node->Y(), node->Z() );
        double coord[3];
        aXYZ.Coord( coord[0], coord[1], coord[2] );
        bool isOnAxis = ( aLine.SquareDistance( aXYZ ) <= aSqTol );

        // check if a node has been already sweeped
        TNodeOfNodeListMapItr nIt =
          mapNewNodes.insert( make_pair( node, list<const SMDS_MeshNode*>() )).first;
        list<const SMDS_MeshNode*>& listNewNodes = nIt->second;
        if ( listNewNodes.empty() )
        {
          // check if we are to create medium nodes between corner ones
          bool needMediumNodes = false;
          if ( isQuadraticMesh )
          {
            SMDS_ElemIteratorPtr it = node->GetInverseElementIterator();
            while (it->more() && !needMediumNodes )
            {
              const SMDS_MeshElement* invElem = it->next();
              if ( invElem != elem && !theElems.count( invElem )) continue;
              needMediumNodes = ( invElem->IsQuadratic() && !invElem->IsMediumNode(node) );
              if ( !needMediumNodes && invElem->GetEntityType() == SMDSEntity_BiQuad_Quadrangle )
                needMediumNodes = true;
            }
          }

          // make new nodes
          const SMDS_MeshNode * newNode = node;
          for ( int i = 0; i < theNbSteps; i++ ) {
            if ( !isOnAxis ) {
              if ( needMediumNodes )  // create a medium node
              {
                aTrsf2.Transforms( coord[0], coord[1], coord[2] );
                newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
                myLastCreatedNodes.Append(newNode);
                srcNodes.Append( node );
                listNewNodes.push_back( newNode );
                aTrsf2.Transforms( coord[0], coord[1], coord[2] );
              }
              else {
                aTrsf.Transforms( coord[0], coord[1], coord[2] );
              }
              // create a corner node
              newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
              myLastCreatedNodes.Append(newNode);
              srcNodes.Append( node );
              listNewNodes.push_back( newNode );
            }
            else {
              listNewNodes.push_back( newNode );
              // if ( needMediumNodes )
              //   listNewNodes.push_back( newNode );
            }
          }
        }
        newNodesItVec.push_back( nIt );
      }
      // make new elements
      sweepElement( elem, newNodesItVec, newElemsMap[elem], theNbSteps, srcElems );
    }
  }

  if ( theMakeWalls )
    makeWalls( mapNewNodes, newElemsMap, mapElemNewNodes, theElemSets[0], theNbSteps, srcElems );

  PGroupIDs newGroupIDs;
  if ( theMakeGroups )
    newGroupIDs = generateGroups( srcNodes, srcElems, "rotated");

  return newGroupIDs;
}

//=======================================================================
//function : ExtrusParam
//purpose  : standard construction
//=======================================================================

SMESH_MeshEditor::ExtrusParam::ExtrusParam( const gp_Vec&  theStep,
                                            const int      theNbSteps,
                                            const int      theFlags,
                                            const double   theTolerance):
  myDir( theStep ),
  myFlags( theFlags ),
  myTolerance( theTolerance ),
  myElemsToUse( NULL )
{
  mySteps = new TColStd_HSequenceOfReal;
  const double stepSize = theStep.Magnitude();
  for (int i=1; i<=theNbSteps; i++ )
    mySteps->Append( stepSize );

  if (( theFlags & EXTRUSION_FLAG_SEW ) &&
      ( theTolerance > 0 ))
  {
    myMakeNodesFun = & SMESH_MeshEditor::ExtrusParam::makeNodesByDirAndSew;
  }
  else
  {
    myMakeNodesFun = & SMESH_MeshEditor::ExtrusParam::makeNodesByDir;
  }
}

//=======================================================================
//function : ExtrusParam
//purpose  : steps are given explicitly
//=======================================================================

SMESH_MeshEditor::ExtrusParam::ExtrusParam( const gp_Dir&                   theDir,
                                            Handle(TColStd_HSequenceOfReal) theSteps,
                                            const int                       theFlags,
                                            const double                    theTolerance):
  myDir( theDir ),
  mySteps( theSteps ),
  myFlags( theFlags ),
  myTolerance( theTolerance ),
  myElemsToUse( NULL )
{
  if (( theFlags & EXTRUSION_FLAG_SEW ) &&
      ( theTolerance > 0 ))
  {
    myMakeNodesFun = & SMESH_MeshEditor::ExtrusParam::makeNodesByDirAndSew;
  }
  else
  {
    myMakeNodesFun = & SMESH_MeshEditor::ExtrusParam::makeNodesByDir;
  }
}

//=======================================================================
//function : ExtrusParam
//purpose  : for extrusion by normal
//=======================================================================

SMESH_MeshEditor::ExtrusParam::ExtrusParam( const double theStepSize,
                                            const int    theNbSteps,
                                            const int    theFlags,
                                            const int    theDim ):
  myDir( 1,0,0 ),
  mySteps( new TColStd_HSequenceOfReal ),
  myFlags( theFlags ),
  myTolerance( 0 ),
  myElemsToUse( NULL )
{
  for (int i = 0; i < theNbSteps; i++ )
    mySteps->Append( theStepSize );

  if ( theDim == 1 )
  {
    myMakeNodesFun = & SMESH_MeshEditor::ExtrusParam::makeNodesByNormal1D;
  }
  else
  {
    myMakeNodesFun = & SMESH_MeshEditor::ExtrusParam::makeNodesByNormal2D;
  }
}

//=======================================================================
//function : ExtrusParam::SetElementsToUse
//purpose  : stores elements to use for extrusion by normal, depending on
//           state of EXTRUSION_FLAG_USE_INPUT_ELEMS_ONLY flag
//=======================================================================

void SMESH_MeshEditor::ExtrusParam::SetElementsToUse( const TIDSortedElemSet& elems )
{
  myElemsToUse = ToUseInpElemsOnly() ? & elems : 0;
}

//=======================================================================
//function : ExtrusParam::beginStepIter
//purpose  : prepare iteration on steps
//=======================================================================

void SMESH_MeshEditor::ExtrusParam::beginStepIter( bool withMediumNodes )
{
  myWithMediumNodes = withMediumNodes;
  myNextStep = 1;
  myCurSteps.clear();
}
//=======================================================================
//function : ExtrusParam::moreSteps
//purpose  : are there more steps?
//=======================================================================

bool SMESH_MeshEditor::ExtrusParam::moreSteps()
{
  return myNextStep <= mySteps->Length() || !myCurSteps.empty();
}
//=======================================================================
//function : ExtrusParam::nextStep
//purpose  : returns the next step
//=======================================================================

double SMESH_MeshEditor::ExtrusParam::nextStep()
{
  double res = 0;
  if ( !myCurSteps.empty() )
  {
    res = myCurSteps.back();
    myCurSteps.pop_back();
  }
  else if ( myNextStep <= mySteps->Length() )
  {
    myCurSteps.push_back( mySteps->Value( myNextStep ));
    ++myNextStep;
    if ( myWithMediumNodes )
    {
      myCurSteps.back() /= 2.;
      myCurSteps.push_back( myCurSteps.back() );
    }
    res = nextStep();
  }
  return res;
}

//=======================================================================
//function : ExtrusParam::makeNodesByDir
//purpose  : create nodes for standard extrusion
//=======================================================================

int SMESH_MeshEditor::ExtrusParam::
makeNodesByDir( SMESHDS_Mesh*                     mesh,
                const SMDS_MeshNode*              srcNode,
                std::list<const SMDS_MeshNode*> & newNodes,
                const bool                        makeMediumNodes)
{
  gp_XYZ p = SMESH_TNodeXYZ( srcNode );

  int nbNodes = 0;
  for ( beginStepIter( makeMediumNodes ); moreSteps(); ++nbNodes ) // loop on steps
  {
    p += myDir.XYZ() * nextStep();
    const SMDS_MeshNode * newNode = mesh->AddNode( p.X(), p.Y(), p.Z() );
    newNodes.push_back( newNode );
  }
  return nbNodes;
}

//=======================================================================
//function : ExtrusParam::makeNodesByDirAndSew
//purpose  : create nodes for standard extrusion with sewing
//=======================================================================

int SMESH_MeshEditor::ExtrusParam::
makeNodesByDirAndSew( SMESHDS_Mesh*                     mesh,
                      const SMDS_MeshNode*              srcNode,
                      std::list<const SMDS_MeshNode*> & newNodes,
                      const bool                        makeMediumNodes)
{
  gp_XYZ P1 = SMESH_TNodeXYZ( srcNode );

  int nbNodes = 0;
  for ( beginStepIter( makeMediumNodes ); moreSteps(); ++nbNodes ) // loop on steps
  {
    P1 += myDir.XYZ() * nextStep();

    // try to search in sequence of existing nodes
    // if myNodes.Length()>0 we 'nave to use given sequence
    // else - use all nodes of mesh
    const SMDS_MeshNode * node = 0;
    if ( myNodes.Length() > 0 ) {
      int i;
      for(i=1; i<=myNodes.Length(); i++) {
        gp_XYZ P2 = SMESH_TNodeXYZ( myNodes.Value(i) );
        if (( P1 - P2 ).SquareModulus() < myTolerance * myTolerance )
        {
          node = myNodes.Value(i);
          break;
        }
      }
    }
    else {
      SMDS_NodeIteratorPtr itn = mesh->nodesIterator();
      while(itn->more()) {
        SMESH_TNodeXYZ P2( itn->next() );
        if (( P1 - P2 ).SquareModulus() < myTolerance * myTolerance )
        {
          node = P2._node;
          break;
        }
      }
    }

    if ( !node )
      node = mesh->AddNode( P1.X(), P1.Y(), P1.Z() );

    newNodes.push_back( node );

  } // loop on steps

  return nbNodes;
}

//=======================================================================
//function : ExtrusParam::makeNodesByNormal2D
//purpose  : create nodes for extrusion using normals of faces
//=======================================================================

int SMESH_MeshEditor::ExtrusParam::
makeNodesByNormal2D( SMESHDS_Mesh*                     mesh,
                     const SMDS_MeshNode*              srcNode,
                     std::list<const SMDS_MeshNode*> & newNodes,
                     const bool                        makeMediumNodes)
{
  const bool alongAvgNorm = ( myFlags & EXTRUSION_FLAG_BY_AVG_NORMAL );

  gp_XYZ p = SMESH_TNodeXYZ( srcNode );

  // get normals to faces sharing srcNode
  vector< gp_XYZ > norms, baryCenters;
  gp_XYZ norm, avgNorm( 0,0,0 );
  SMDS_ElemIteratorPtr faceIt = srcNode->GetInverseElementIterator( SMDSAbs_Face );
  while ( faceIt->more() )
  {
    const SMDS_MeshElement* face = faceIt->next();
    if ( myElemsToUse && !myElemsToUse->count( face ))
      continue;
    if ( SMESH_MeshAlgos::FaceNormal( face, norm, /*normalized=*/true ))
    {
      norms.push_back( norm );
      avgNorm += norm;
      if ( !alongAvgNorm )
      {
        gp_XYZ bc(0,0,0);
        int nbN = 0;
        for ( SMDS_ElemIteratorPtr nIt = face->nodesIterator(); nIt->more(); ++nbN )
          bc += SMESH_TNodeXYZ( nIt->next() );
        baryCenters.push_back( bc / nbN );
      }
    }
  }

  if ( norms.empty() ) return 0;

  double normSize = avgNorm.Modulus();
  if ( normSize < std::numeric_limits<double>::min() )
    return 0;

  if ( myFlags & EXTRUSION_FLAG_BY_AVG_NORMAL ) // extrude along avgNorm
  {
    myDir = avgNorm;
    return makeNodesByDir( mesh, srcNode, newNodes, makeMediumNodes );
  }

  avgNorm /= normSize;

  int nbNodes = 0;
  for ( beginStepIter( makeMediumNodes ); moreSteps(); ++nbNodes ) // loop on steps
  {
    gp_XYZ pNew = p;
    double stepSize = nextStep();

    if ( norms.size() > 1 )
    {
      for ( size_t iF = 0; iF < norms.size(); ++iF ) // loop on faces
      {
        // translate plane of a face
        baryCenters[ iF ] += norms[ iF ] * stepSize;

        // find point of intersection of the face plane located at baryCenters[ iF ]
        // and avgNorm located at pNew
        double d    = -( norms[ iF ] * baryCenters[ iF ]); // d of plane equation ax+by+cz+d=0
        double dot  = ( norms[ iF ] * avgNorm );
        if ( dot < std::numeric_limits<double>::min() )
          dot = stepSize * 1e-3;
        double step = -( norms[ iF ] * pNew + d ) / dot;
        pNew += step * avgNorm;
      }
    }
    else
    {
      pNew += stepSize * avgNorm;
    }
    p = pNew;

    const SMDS_MeshNode * newNode = mesh->AddNode( p.X(), p.Y(), p.Z() );
    newNodes.push_back( newNode );
  }
  return nbNodes;
}

//=======================================================================
//function : ExtrusParam::makeNodesByNormal1D
//purpose  : create nodes for extrusion using normals of edges
//=======================================================================

int SMESH_MeshEditor::ExtrusParam::
makeNodesByNormal1D( SMESHDS_Mesh*                     mesh,
                     const SMDS_MeshNode*              srcNode,
                     std::list<const SMDS_MeshNode*> & newNodes,
                     const bool                        makeMediumNodes)
{
  throw SALOME_Exception("Extrusion 1D by Normal not implemented");
  return 0;
}

//=======================================================================
//function : ExtrusionSweep
//purpose  :
//=======================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::ExtrusionSweep (TIDSortedElemSet     theElems[2],
                                  const gp_Vec&        theStep,
                                  const int            theNbSteps,
                                  TTElemOfElemListMap& newElemsMap,
                                  const int            theFlags,
                                  const double         theTolerance)
{
  ExtrusParam aParams( theStep, theNbSteps, theFlags, theTolerance );
  return ExtrusionSweep( theElems, aParams, newElemsMap );
}


//=======================================================================
//function : ExtrusionSweep
//purpose  :
//=======================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::ExtrusionSweep (TIDSortedElemSet     theElemSets[2],
                                  ExtrusParam&         theParams,
                                  TTElemOfElemListMap& newElemsMap)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  // source elements for each generated one
  SMESH_SequenceOfElemPtr srcElems, srcNodes;

  SMESHDS_Mesh* aMesh = GetMeshDS();

  setElemsFirst( theElemSets );
  const int nbSteps = theParams.NbSteps();
  theParams.SetElementsToUse( theElemSets[0] );

  TNodeOfNodeListMap mapNewNodes;
  //TNodeOfNodeVecMap mapNewNodes;
  TElemOfVecOfNnlmiMap mapElemNewNodes;
  //TElemOfVecOfMapNodesMap mapElemNewNodes;

  const bool isQuadraticMesh = bool( myMesh->NbEdges(ORDER_QUADRATIC) +
                                     myMesh->NbFaces(ORDER_QUADRATIC) +
                                     myMesh->NbVolumes(ORDER_QUADRATIC) );
  // loop on theElems
  TIDSortedElemSet::iterator itElem;
  for ( int is2ndSet = 0; is2ndSet < 2; ++is2ndSet )
  {
    TIDSortedElemSet& theElems = theElemSets[ is2ndSet ];
    for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ )
    {
      // check element type
      const SMDS_MeshElement* elem = *itElem;
      if ( !elem  || elem->GetType() == SMDSAbs_Volume )
        continue;

      const size_t nbNodes = elem->NbNodes();
      vector<TNodeOfNodeListMapItr> & newNodesItVec = mapElemNewNodes[ elem ];
      newNodesItVec.reserve( nbNodes );

      // loop on elem nodes
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      while ( itN->more() )
      {
        // check if a node has been already sweeped
        const SMDS_MeshNode* node = cast2Node( itN->next() );
        TNodeOfNodeListMap::iterator nIt =
          mapNewNodes.insert( make_pair( node, list<const SMDS_MeshNode*>() )).first;
        list<const SMDS_MeshNode*>& listNewNodes = nIt->second;
        if ( listNewNodes.empty() )
        {
          // make new nodes

          // check if we are to create medium nodes between corner ones
          bool needMediumNodes = false;
          if ( isQuadraticMesh )
          {
            SMDS_ElemIteratorPtr it = node->GetInverseElementIterator();
            while (it->more() && !needMediumNodes )
            {
              const SMDS_MeshElement* invElem = it->next();
              if ( invElem != elem && !theElems.count( invElem )) continue;
              needMediumNodes = ( invElem->IsQuadratic() && !invElem->IsMediumNode(node) );
              if ( !needMediumNodes && invElem->GetEntityType() == SMDSEntity_BiQuad_Quadrangle )
                needMediumNodes = true;
            }
          }
          // create nodes for all steps
          if ( theParams.MakeNodes( GetMeshDS(), node, listNewNodes, needMediumNodes ))
          {
            list<const SMDS_MeshNode*>::iterator newNodesIt = listNewNodes.begin();
            for ( ; newNodesIt != listNewNodes.end(); ++newNodesIt )
            {
              myLastCreatedNodes.Append( *newNodesIt );
              srcNodes.Append( node );
            }
          }
          else
          {
            break; // newNodesItVec will be shorter than nbNodes
          }
        }
        newNodesItVec.push_back( nIt );
      }
      // make new elements
      if ( newNodesItVec.size() == nbNodes )
        sweepElement( elem, newNodesItVec, newElemsMap[elem], nbSteps, srcElems );
    }
  }

  if ( theParams.ToMakeBoundary() ) {
    makeWalls( mapNewNodes, newElemsMap, mapElemNewNodes, theElemSets[0], nbSteps, srcElems );
  }
  PGroupIDs newGroupIDs;
  if ( theParams.ToMakeGroups() )
    newGroupIDs = generateGroups( srcNodes, srcElems, "extruded");

  return newGroupIDs;
}

//=======================================================================
//function : ExtrusionAlongTrack
//purpose  :
//=======================================================================
SMESH_MeshEditor::Extrusion_Error
SMESH_MeshEditor::ExtrusionAlongTrack (TIDSortedElemSet     theElements[2],
                                       SMESH_subMesh*       theTrack,
                                       const SMDS_MeshNode* theN1,
                                       const bool           theHasAngles,
                                       list<double>&        theAngles,
                                       const bool           theLinearVariation,
                                       const bool           theHasRefPoint,
                                       const gp_Pnt&        theRefPoint,
                                       const bool           theMakeGroups)
{
  MESSAGE("ExtrusionAlongTrack");
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  int aNbE;
  std::list<double> aPrms;
  TIDSortedElemSet::iterator itElem;

  gp_XYZ aGC;
  TopoDS_Edge aTrackEdge;
  TopoDS_Vertex aV1, aV2;

  SMDS_ElemIteratorPtr aItE;
  SMDS_NodeIteratorPtr aItN;
  SMDSAbs_ElementType aTypeE;

  TNodeOfNodeListMap mapNewNodes;

  // 1. Check data
  aNbE = theElements[0].size() + theElements[1].size();
  // nothing to do
  if ( !aNbE )
    return EXTR_NO_ELEMENTS;

  // 1.1 Track Pattern
  ASSERT( theTrack );

  SMESHDS_SubMesh* pSubMeshDS = theTrack->GetSubMeshDS();
  if ( !pSubMeshDS )
    return ExtrusionAlongTrack( theElements, theTrack->GetFather(), theN1,
                                theHasAngles, theAngles, theLinearVariation,
                                theHasRefPoint, theRefPoint, theMakeGroups );

  aItE = pSubMeshDS->GetElements();
  while ( aItE->more() ) {
    const SMDS_MeshElement* pE = aItE->next();
    aTypeE = pE->GetType();
    // Pattern must contain links only
    if ( aTypeE != SMDSAbs_Edge )
      return EXTR_PATH_NOT_EDGE;
  }

  list<SMESH_MeshEditor_PathPoint> fullList;

  const TopoDS_Shape& aS = theTrack->GetSubShape();
  // Sub-shape for the Pattern must be an Edge or Wire
  if( aS.ShapeType() == TopAbs_EDGE ) {
    aTrackEdge = TopoDS::Edge( aS );
    // the Edge must not be degenerated
    if ( SMESH_Algo::isDegenerated( aTrackEdge ) )
      return EXTR_BAD_PATH_SHAPE;
    TopExp::Vertices( aTrackEdge, aV1, aV2 );
    aItN = theTrack->GetFather()->GetSubMesh( aV1 )->GetSubMeshDS()->GetNodes();
    const SMDS_MeshNode* aN1 = aItN->next();
    aItN = theTrack->GetFather()->GetSubMesh( aV2 )->GetSubMeshDS()->GetNodes();
    const SMDS_MeshNode* aN2 = aItN->next();
    // starting node must be aN1 or aN2
    if ( !( aN1 == theN1 || aN2 == theN1 ) )
      return EXTR_BAD_STARTING_NODE;
    aItN = pSubMeshDS->GetNodes();
    while ( aItN->more() ) {
      const SMDS_MeshNode* pNode = aItN->next();
      const SMDS_EdgePosition* pEPos =
        static_cast<const SMDS_EdgePosition*>( pNode->GetPosition() );
      double aT = pEPos->GetUParameter();
      aPrms.push_back( aT );
    }
    //Extrusion_Error err =
    MakeEdgePathPoints(aPrms, aTrackEdge, (aN1==theN1), fullList);
  } else if( aS.ShapeType() == TopAbs_WIRE ) {
    list< SMESH_subMesh* > LSM;
    TopTools_SequenceOfShape Edges;
    SMESH_subMeshIteratorPtr itSM = theTrack->getDependsOnIterator(false,true);
    while(itSM->more()) {
      SMESH_subMesh* SM = itSM->next();
      LSM.push_back(SM);
      const TopoDS_Shape& aS = SM->GetSubShape();
      Edges.Append(aS);
    }
    list< list<SMESH_MeshEditor_PathPoint> > LLPPs;
    int startNid = theN1->GetID();
    TColStd_MapOfInteger UsedNums;

    int NbEdges = Edges.Length();
    int i = 1;
    for(; i<=NbEdges; i++) {
      int k = 0;
      list< SMESH_subMesh* >::iterator itLSM = LSM.begin();
      for(; itLSM!=LSM.end(); itLSM++) {
        k++;
        if(UsedNums.Contains(k)) continue;
        aTrackEdge = TopoDS::Edge( Edges.Value(k) );
        SMESH_subMesh* locTrack = *itLSM;
        SMESHDS_SubMesh* locMeshDS = locTrack->GetSubMeshDS();
        TopExp::Vertices( aTrackEdge, aV1, aV2 );
        aItN = locTrack->GetFather()->GetSubMesh(aV1)->GetSubMeshDS()->GetNodes();
        const SMDS_MeshNode* aN1 = aItN->next();
        aItN = locTrack->GetFather()->GetSubMesh(aV2)->GetSubMeshDS()->GetNodes();
        const SMDS_MeshNode* aN2 = aItN->next();
        // starting node must be aN1 or aN2
        if ( !( aN1->GetID() == startNid || aN2->GetID() == startNid ) ) continue;
        // 2. Collect parameters on the track edge
        aPrms.clear();
        aItN = locMeshDS->GetNodes();
        while ( aItN->more() ) {
          const SMDS_MeshNode* pNode = aItN->next();
          const SMDS_EdgePosition* pEPos =
            static_cast<const SMDS_EdgePosition*>( pNode->GetPosition() );
          double aT = pEPos->GetUParameter();
          aPrms.push_back( aT );
        }
        list<SMESH_MeshEditor_PathPoint> LPP;
        //Extrusion_Error err =
        MakeEdgePathPoints(aPrms, aTrackEdge,(aN1->GetID()==startNid), LPP);
        LLPPs.push_back(LPP);
        UsedNums.Add(k);
        // update startN for search following egde
        if( aN1->GetID() == startNid ) startNid = aN2->GetID();
        else startNid = aN1->GetID();
        break;
      }
    }
    list< list<SMESH_MeshEditor_PathPoint> >::iterator itLLPP = LLPPs.begin();
    list<SMESH_MeshEditor_PathPoint> firstList = *itLLPP;
    list<SMESH_MeshEditor_PathPoint>::iterator itPP = firstList.begin();
    for(; itPP!=firstList.end(); itPP++) {
      fullList.push_back( *itPP );
    }
    SMESH_MeshEditor_PathPoint PP1 = fullList.back();
    fullList.pop_back();
    itLLPP++;
    for(; itLLPP!=LLPPs.end(); itLLPP++) {
      list<SMESH_MeshEditor_PathPoint> currList = *itLLPP;
      itPP = currList.begin();
      SMESH_MeshEditor_PathPoint PP2 = currList.front();
      gp_Dir D1 = PP1.Tangent();
      gp_Dir D2 = PP2.Tangent();
      gp_Dir Dnew( gp_Vec( (D1.X()+D2.X())/2, (D1.Y()+D2.Y())/2,
                           (D1.Z()+D2.Z())/2 ) );
      PP1.SetTangent(Dnew);
      fullList.push_back(PP1);
      itPP++;
      for(; itPP!=firstList.end(); itPP++) {
        fullList.push_back( *itPP );
      }
      PP1 = fullList.back();
      fullList.pop_back();
    }
    // if wire not closed
    fullList.push_back(PP1);
    // else ???
  }
  else {
    return EXTR_BAD_PATH_SHAPE;
  }

  return MakeExtrElements(theElements, fullList, theHasAngles, theAngles, theLinearVariation,
                          theHasRefPoint, theRefPoint, theMakeGroups);
}


//=======================================================================
//function : ExtrusionAlongTrack
//purpose  :
//=======================================================================
SMESH_MeshEditor::Extrusion_Error
SMESH_MeshEditor::ExtrusionAlongTrack (TIDSortedElemSet     theElements[2],
                                       SMESH_Mesh*          theTrack,
                                       const SMDS_MeshNode* theN1,
                                       const bool           theHasAngles,
                                       list<double>&        theAngles,
                                       const bool           theLinearVariation,
                                       const bool           theHasRefPoint,
                                       const gp_Pnt&        theRefPoint,
                                       const bool           theMakeGroups)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  int aNbE;
  std::list<double> aPrms;
  TIDSortedElemSet::iterator itElem;

  gp_XYZ aGC;
  TopoDS_Edge aTrackEdge;
  TopoDS_Vertex aV1, aV2;

  SMDS_ElemIteratorPtr aItE;
  SMDS_NodeIteratorPtr aItN;
  SMDSAbs_ElementType aTypeE;

  TNodeOfNodeListMap mapNewNodes;

  // 1. Check data
  aNbE = theElements[0].size() + theElements[1].size();
  // nothing to do
  if ( !aNbE )
    return EXTR_NO_ELEMENTS;

  // 1.1 Track Pattern
  ASSERT( theTrack );

  SMESHDS_Mesh* pMeshDS = theTrack->GetMeshDS();

  aItE = pMeshDS->elementsIterator();
  while ( aItE->more() ) {
    const SMDS_MeshElement* pE = aItE->next();
    aTypeE = pE->GetType();
    // Pattern must contain links only
    if ( aTypeE != SMDSAbs_Edge )
      return EXTR_PATH_NOT_EDGE;
  }

  list<SMESH_MeshEditor_PathPoint> fullList;

  const TopoDS_Shape& aS = theTrack->GetShapeToMesh();

  if ( !theTrack->HasShapeToMesh() ) {
    //Mesh without shape
    const SMDS_MeshNode* currentNode = NULL;
    const SMDS_MeshNode* prevNode = theN1;
    std::vector<const SMDS_MeshNode*> aNodesList;
    aNodesList.push_back(theN1);
    int nbEdges = 0, conn=0;
    const SMDS_MeshElement* prevElem = NULL;
    const SMDS_MeshElement* currentElem = NULL;
    int totalNbEdges = theTrack->NbEdges();
    SMDS_ElemIteratorPtr nIt;

    //check start node
    if( !theTrack->GetMeshDS()->Contains(theN1) ) {
      return EXTR_BAD_STARTING_NODE;
    }

    conn = nbEdgeConnectivity(theN1);
    if( conn != 1 )
      return EXTR_PATH_NOT_EDGE;

    aItE = theN1->GetInverseElementIterator();
    prevElem = aItE->next();
    currentElem = prevElem;
    //Get all nodes
    if(totalNbEdges == 1 ) {
      nIt = currentElem->nodesIterator();
      currentNode = static_cast<const SMDS_MeshNode*>(nIt->next());
      if(currentNode == prevNode)
        currentNode = static_cast<const SMDS_MeshNode*>(nIt->next());
      aNodesList.push_back(currentNode);
    } else {
      nIt = currentElem->nodesIterator();
      while( nIt->more() ) {
        currentNode = static_cast<const SMDS_MeshNode*>(nIt->next());
        if(currentNode == prevNode)
          currentNode = static_cast<const SMDS_MeshNode*>(nIt->next());
        aNodesList.push_back(currentNode);

        //case of the closed mesh
        if(currentNode == theN1) {
          nbEdges++;
          break;
        }

        conn = nbEdgeConnectivity(currentNode);
        if(conn > 2) {
          return EXTR_PATH_NOT_EDGE;
        }else if( conn == 1 && nbEdges > 0 ) {
          //End of the path
          nbEdges++;
          break;
        }else {
          prevNode = currentNode;
          aItE = currentNode->GetInverseElementIterator();
          currentElem = aItE->next();
          if( currentElem  == prevElem)
            currentElem = aItE->next();
          nIt = currentElem->nodesIterator();
          prevElem = currentElem;
          nbEdges++;
        }
      }
    }

    if(nbEdges != totalNbEdges)
      return EXTR_PATH_NOT_EDGE;

    TopTools_SequenceOfShape Edges;
    double x1,x2,y1,y2,z1,z2;
    list< list<SMESH_MeshEditor_PathPoint> > LLPPs;
    int startNid = theN1->GetID();
    for(int i = 1; i < aNodesList.size(); i++) {
      x1 = aNodesList[i-1]->X();x2 = aNodesList[i]->X();
      y1 = aNodesList[i-1]->Y();y2 = aNodesList[i]->Y();
      z1 = aNodesList[i-1]->Z();z2 = aNodesList[i]->Z();
      TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp_Pnt(x1,y1,z1),gp_Pnt(x2,y2,z2));
      list<SMESH_MeshEditor_PathPoint> LPP;
      aPrms.clear();
      MakeEdgePathPoints(aPrms, e, (aNodesList[i-1]->GetID()==startNid), LPP);
      LLPPs.push_back(LPP);
      if( aNodesList[i-1]->GetID() == startNid ) startNid = aNodesList[i]->GetID();
      else startNid = aNodesList[i-1]->GetID();

    }

    list< list<SMESH_MeshEditor_PathPoint> >::iterator itLLPP = LLPPs.begin();
    list<SMESH_MeshEditor_PathPoint> firstList = *itLLPP;
    list<SMESH_MeshEditor_PathPoint>::iterator itPP = firstList.begin();
    for(; itPP!=firstList.end(); itPP++) {
      fullList.push_back( *itPP );
    }

    SMESH_MeshEditor_PathPoint PP1 = fullList.back();
    SMESH_MeshEditor_PathPoint PP2;
    fullList.pop_back();
    itLLPP++;
    for(; itLLPP!=LLPPs.end(); itLLPP++) {
      list<SMESH_MeshEditor_PathPoint> currList = *itLLPP;
      itPP = currList.begin();
      PP2 = currList.front();
      gp_Dir D1 = PP1.Tangent();
      gp_Dir D2 = PP2.Tangent();
      gp_Dir Dnew( gp_Vec( (D1.X()+D2.X())/2, (D1.Y()+D2.Y())/2,
                           (D1.Z()+D2.Z())/2 ) );
      PP1.SetTangent(Dnew);
      fullList.push_back(PP1);
      itPP++;
      for(; itPP!=currList.end(); itPP++) {
        fullList.push_back( *itPP );
      }
      PP1 = fullList.back();
      fullList.pop_back();
    }
    fullList.push_back(PP1);

  } // Sub-shape for the Pattern must be an Edge or Wire
  else if( aS.ShapeType() == TopAbs_EDGE ) {
    aTrackEdge = TopoDS::Edge( aS );
    // the Edge must not be degenerated
    if ( SMESH_Algo::isDegenerated( aTrackEdge ) )
      return EXTR_BAD_PATH_SHAPE;
    TopExp::Vertices( aTrackEdge, aV1, aV2 );
    const SMDS_MeshNode* aN1 = SMESH_Algo::VertexNode( aV1, pMeshDS );
    const SMDS_MeshNode* aN2 = SMESH_Algo::VertexNode( aV2, pMeshDS );
    // starting node must be aN1 or aN2
    if ( !( aN1 == theN1 || aN2 == theN1 ) )
      return EXTR_BAD_STARTING_NODE;
    aItN = pMeshDS->nodesIterator();
    while ( aItN->more() ) {
      const SMDS_MeshNode* pNode = aItN->next();
      if( pNode==aN1 || pNode==aN2 ) continue;
      const SMDS_EdgePosition* pEPos =
        static_cast<const SMDS_EdgePosition*>( pNode->GetPosition() );
      double aT = pEPos->GetUParameter();
      aPrms.push_back( aT );
    }
    //Extrusion_Error err =
    MakeEdgePathPoints(aPrms, aTrackEdge, (aN1==theN1), fullList);
  }
  else if( aS.ShapeType() == TopAbs_WIRE ) {
    list< SMESH_subMesh* > LSM;
    TopTools_SequenceOfShape Edges;
    TopExp_Explorer eExp(aS, TopAbs_EDGE);
    for(; eExp.More(); eExp.Next()) {
      TopoDS_Edge E = TopoDS::Edge( eExp.Current() );
      if( SMESH_Algo::isDegenerated(E) ) continue;
      SMESH_subMesh* SM = theTrack->GetSubMesh(E);
      if(SM) {
        LSM.push_back(SM);
        Edges.Append(E);
      }
    }
    list< list<SMESH_MeshEditor_PathPoint> > LLPPs;
    TopoDS_Vertex aVprev;
    TColStd_MapOfInteger UsedNums;
    int NbEdges = Edges.Length();
    int i = 1;
    for(; i<=NbEdges; i++) {
      int k = 0;
      list< SMESH_subMesh* >::iterator itLSM = LSM.begin();
      for(; itLSM!=LSM.end(); itLSM++) {
        k++;
        if(UsedNums.Contains(k)) continue;
        aTrackEdge = TopoDS::Edge( Edges.Value(k) );
        SMESH_subMesh* locTrack = *itLSM;
        SMESHDS_SubMesh* locMeshDS = locTrack->GetSubMeshDS();
        TopExp::Vertices( aTrackEdge, aV1, aV2 );
        bool aN1isOK = false, aN2isOK = false;
        if ( aVprev.IsNull() ) {
          // if previous vertex is not yet defined, it means that we in the beginning of wire
          // and we have to find initial vertex corresponding to starting node theN1
          const SMDS_MeshNode* aN1 = SMESH_Algo::VertexNode( aV1, pMeshDS );
          const SMDS_MeshNode* aN2 = SMESH_Algo::VertexNode( aV2, pMeshDS );
          // starting node must be aN1 or aN2
          aN1isOK = ( aN1 && aN1 == theN1 );
          aN2isOK = ( aN2 && aN2 == theN1 );
        }
        else {
          // we have specified ending vertex of the previous edge on the previous iteration
          // and we have just to check that it corresponds to any vertex in current segment
          aN1isOK = aVprev.IsSame( aV1 );
          aN2isOK = aVprev.IsSame( aV2 );
        }
        if ( !aN1isOK && !aN2isOK ) continue;
        // 2. Collect parameters on the track edge
        aPrms.clear();
        aItN = locMeshDS->GetNodes();
        while ( aItN->more() ) {
          const SMDS_MeshNode*     pNode = aItN->next();
          const SMDS_EdgePosition* pEPos =
            static_cast<const SMDS_EdgePosition*>( pNode->GetPosition() );
          double aT = pEPos->GetUParameter();
          aPrms.push_back( aT );
        }
        list<SMESH_MeshEditor_PathPoint> LPP;
        //Extrusion_Error err =
        MakeEdgePathPoints(aPrms, aTrackEdge, aN1isOK, LPP);
        LLPPs.push_back(LPP);
        UsedNums.Add(k);
        // update startN for search following egde
        if ( aN1isOK ) aVprev = aV2;
        else           aVprev = aV1;
        break;
      }
    }
    list< list<SMESH_MeshEditor_PathPoint> >::iterator itLLPP = LLPPs.begin();
    list<SMESH_MeshEditor_PathPoint>& firstList = *itLLPP;
    fullList.splice( fullList.end(), firstList );

    SMESH_MeshEditor_PathPoint PP1 = fullList.back();
    fullList.pop_back();
    itLLPP++;
    for(; itLLPP!=LLPPs.end(); itLLPP++) {
      list<SMESH_MeshEditor_PathPoint>& currList = *itLLPP;
      SMESH_MeshEditor_PathPoint PP2 = currList.front();
      gp_Dir D1 = PP1.Tangent();
      gp_Dir D2 = PP2.Tangent();
      gp_Dir Dnew( D1.XYZ() + D2.XYZ() );
      PP1.SetTangent(Dnew);
      fullList.push_back(PP1);
      fullList.splice( fullList.end(), currList, ++currList.begin(), currList.end() );
      PP1 = fullList.back();
      fullList.pop_back();
    }
    // if wire not closed
    fullList.push_back(PP1);
    // else ???
  }
  else {
    return EXTR_BAD_PATH_SHAPE;
  }

  return MakeExtrElements(theElements, fullList, theHasAngles, theAngles, theLinearVariation,
                          theHasRefPoint, theRefPoint, theMakeGroups);
}


//=======================================================================
//function : MakeEdgePathPoints
//purpose  : auxilary for ExtrusionAlongTrack
//=======================================================================
SMESH_MeshEditor::Extrusion_Error
SMESH_MeshEditor::MakeEdgePathPoints(std::list<double>&                aPrms,
                                     const TopoDS_Edge&                aTrackEdge,
                                     bool                              FirstIsStart,
                                     list<SMESH_MeshEditor_PathPoint>& LPP)
{
  Standard_Real aTx1, aTx2, aL2, aTolVec, aTolVec2;
  aTolVec=1.e-7;
  aTolVec2=aTolVec*aTolVec;
  double aT1, aT2;
  TopoDS_Vertex aV1, aV2;
  TopExp::Vertices( aTrackEdge, aV1, aV2 );
  aT1=BRep_Tool::Parameter( aV1, aTrackEdge );
  aT2=BRep_Tool::Parameter( aV2, aTrackEdge );
  // 2. Collect parameters on the track edge
  aPrms.push_front( aT1 );
  aPrms.push_back( aT2 );
  // sort parameters
  aPrms.sort();
  if( FirstIsStart ) {
    if ( aT1 > aT2 ) {
      aPrms.reverse();
    }
  }
  else {
    if ( aT2 > aT1 ) {
      aPrms.reverse();
    }
  }
  // 3. Path Points
  SMESH_MeshEditor_PathPoint aPP;
  Handle(Geom_Curve) aC3D = BRep_Tool::Curve( aTrackEdge, aTx1, aTx2 );
  std::list<double>::iterator aItD = aPrms.begin();
  for(; aItD != aPrms.end(); ++aItD) {
    double aT = *aItD;
    gp_Pnt aP3D;
    gp_Vec aVec;
    aC3D->D1( aT, aP3D, aVec );
    aL2 = aVec.SquareMagnitude();
    if ( aL2 < aTolVec2 )
      return EXTR_CANT_GET_TANGENT;
    gp_Dir aTgt( FirstIsStart ? aVec : -aVec );
    aPP.SetPnt( aP3D );
    aPP.SetTangent( aTgt );
    aPP.SetParameter( aT );
    LPP.push_back(aPP);
  }
  return EXTR_OK;
}


//=======================================================================
//function : MakeExtrElements
//purpose  : auxilary for ExtrusionAlongTrack
//=======================================================================
SMESH_MeshEditor::Extrusion_Error
SMESH_MeshEditor::MakeExtrElements(TIDSortedElemSet                  theElemSets[2],
                                   list<SMESH_MeshEditor_PathPoint>& fullList,
                                   const bool                        theHasAngles,
                                   list<double>&                     theAngles,
                                   const bool                        theLinearVariation,
                                   const bool                        theHasRefPoint,
                                   const gp_Pnt&                     theRefPoint,
                                   const bool                        theMakeGroups)
{
  const int aNbTP = fullList.size();

  // Angles
  if( theHasAngles && !theAngles.empty() && theLinearVariation )
    LinearAngleVariation(aNbTP-1, theAngles);

  // fill vector of path points with angles
  vector<SMESH_MeshEditor_PathPoint> aPPs;
  list<SMESH_MeshEditor_PathPoint>::iterator itPP = fullList.begin();
  list<double>::iterator                 itAngles = theAngles.begin();
  aPPs.push_back( *itPP++ );
  for( ; itPP != fullList.end(); itPP++) {
    aPPs.push_back( *itPP );
    if ( theHasAngles && itAngles != theAngles.end() )
      aPPs.back().SetAngle( *itAngles++ );
  }

  TNodeOfNodeListMap   mapNewNodes;
  TElemOfVecOfNnlmiMap mapElemNewNodes;
  TTElemOfElemListMap  newElemsMap;
  TIDSortedElemSet::iterator itElem;
  // source elements for each generated one
  SMESH_SequenceOfElemPtr srcElems, srcNodes;

  // 3. Center of rotation aV0
  gp_Pnt aV0 = theRefPoint;
  if ( !theHasRefPoint )
  {
    gp_XYZ aGC( 0.,0.,0. );
    TIDSortedElemSet newNodes;

    for ( int is2ndSet = 0; is2ndSet < 2; ++is2ndSet )
    {
      TIDSortedElemSet& theElements = theElemSets[ is2ndSet ];
      itElem = theElements.begin();
      for ( ; itElem != theElements.end(); itElem++ )
      {
        const SMDS_MeshElement* elem = *itElem;
        SMDS_ElemIteratorPtr     itN = elem->nodesIterator();
        while ( itN->more() ) {
          const SMDS_MeshElement* node = itN->next();
          if ( newNodes.insert( node ).second )
            aGC += SMESH_TNodeXYZ( node );
        }
      }
    }
    aGC /= newNodes.size();
    aV0.SetXYZ( aGC );
  } // if (!theHasRefPoint) {

  // 4. Processing the elements
  SMESHDS_Mesh* aMesh = GetMeshDS();
  list<const SMDS_MeshNode*> emptyList;

  setElemsFirst( theElemSets );
  for ( int is2ndSet = 0; is2ndSet < 2; ++is2ndSet )
  {
    TIDSortedElemSet& theElements = theElemSets[ is2ndSet ];
    for ( itElem = theElements.begin(); itElem != theElements.end(); itElem++ )
    {
      const SMDS_MeshElement* elem = *itElem;

      vector<TNodeOfNodeListMapItr> & newNodesItVec = mapElemNewNodes[ elem ];
      newNodesItVec.reserve( elem->NbNodes() );

      // loop on elem nodes
      int nodeIndex = -1;
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      while ( itN->more() )
      {
        ++nodeIndex;
        // check if a node has been already processed
        const SMDS_MeshNode* node = cast2Node( itN->next() );
        TNodeOfNodeListMap::iterator nIt = mapNewNodes.insert( make_pair( node, emptyList )).first;
        list<const SMDS_MeshNode*>& listNewNodes = nIt->second;
        if ( listNewNodes.empty() )
        {
          // make new nodes
          Standard_Real aAngle1x, aAngleT1T0, aTolAng;
          gp_Pnt aP0x, aP1x, aPN0, aPN1, aV0x, aV1x;
          gp_Ax1 anAx1, anAxT1T0;
          gp_Dir aDT1x, aDT0x, aDT1T0;

          aTolAng=1.e-4;

          aV0x = aV0;
          aPN0 = SMESH_TNodeXYZ( node );

          const SMESH_MeshEditor_PathPoint& aPP0 = aPPs[0];
          aP0x = aPP0.Pnt();
          aDT0x= aPP0.Tangent();

          for ( int j = 1; j < aNbTP; ++j ) {
            const SMESH_MeshEditor_PathPoint& aPP1 = aPPs[j];
            aP1x     = aPP1.Pnt();
            aDT1x    = aPP1.Tangent();
            aAngle1x = aPP1.Angle();

            gp_Trsf aTrsf, aTrsfRot, aTrsfRotT1T0;
            // Translation
            gp_Vec aV01x( aP0x, aP1x );
            aTrsf.SetTranslation( aV01x );

            // traslated point
            aV1x = aV0x.Transformed( aTrsf );
            aPN1 = aPN0.Transformed( aTrsf );

            // rotation 1 [ T1,T0 ]
            aAngleT1T0=-aDT1x.Angle( aDT0x );
            if (fabs(aAngleT1T0) > aTolAng)
            {
              aDT1T0=aDT1x^aDT0x;
              anAxT1T0.SetLocation( aV1x );
              anAxT1T0.SetDirection( aDT1T0 );
              aTrsfRotT1T0.SetRotation( anAxT1T0, aAngleT1T0 );

              aPN1 = aPN1.Transformed( aTrsfRotT1T0 );
            }

            // rotation 2
            if ( theHasAngles ) {
              anAx1.SetLocation( aV1x );
              anAx1.SetDirection( aDT1x );
              aTrsfRot.SetRotation( anAx1, aAngle1x );

              aPN1 = aPN1.Transformed( aTrsfRot );
            }

            // make new node
            if ( elem->IsQuadratic() && !elem->IsMediumNode(node) )
            {
              // create additional node
              gp_XYZ midP = 0.5 * ( aPN1.XYZ() + aPN0.XYZ() );
              const SMDS_MeshNode* newNode = aMesh->AddNode( midP.X(), midP.Y(), midP.Z() );
              myLastCreatedNodes.Append(newNode);
              srcNodes.Append( node );
              listNewNodes.push_back( newNode );
            }
            const SMDS_MeshNode* newNode = aMesh->AddNode( aPN1.X(), aPN1.Y(), aPN1.Z() );
            myLastCreatedNodes.Append(newNode);
            srcNodes.Append( node );
            listNewNodes.push_back( newNode );

            aPN0 = aPN1;
            aP0x = aP1x;
            aV0x = aV1x;
            aDT0x = aDT1x;
          }
        }
        else if( elem->IsQuadratic() && !elem->IsMediumNode(node) )
        {
          // if current elem is quadratic and current node is not medium
          // we have to check - may be it is needed to insert additional nodes
          list< const SMDS_MeshNode* > & listNewNodes = nIt->second;
          if ( listNewNodes.size() == aNbTP-1 )
          {
            vector<const SMDS_MeshNode*> aNodes(2*(aNbTP-1));
            gp_XYZ P(node->X(), node->Y(), node->Z());
            list< const SMDS_MeshNode* >::iterator it = listNewNodes.begin();
            int i;
            for(i=0; i<aNbTP-1; i++) {
              const SMDS_MeshNode* N = *it;
              double x = ( N->X() + P.X() )/2.;
              double y = ( N->Y() + P.Y() )/2.;
              double z = ( N->Z() + P.Z() )/2.;
              const SMDS_MeshNode* newN = aMesh->AddNode(x,y,z);
              srcNodes.Append( node );
              myLastCreatedNodes.Append(newN);
              aNodes[2*i] = newN;
              aNodes[2*i+1] = N;
              P = gp_XYZ(N->X(),N->Y(),N->Z());
            }
            listNewNodes.clear();
            for(i=0; i<2*(aNbTP-1); i++) {
              listNewNodes.push_back(aNodes[i]);
            }
          }
        }

        newNodesItVec.push_back( nIt );
      }

      // make new elements
      sweepElement( elem, newNodesItVec, newElemsMap[elem], aNbTP-1, srcElems );
    }
  }

  makeWalls( mapNewNodes, newElemsMap, mapElemNewNodes, theElemSets[0], aNbTP-1, srcElems );

  if ( theMakeGroups )
    generateGroups( srcNodes, srcElems, "extruded");

  return EXTR_OK;
}


//=======================================================================
//function : LinearAngleVariation
//purpose  : auxilary for ExtrusionAlongTrack
//=======================================================================
void SMESH_MeshEditor::LinearAngleVariation(const int nbSteps,
                                            list<double>& Angles)
{
  int nbAngles = Angles.size();
  if( nbSteps > nbAngles ) {
    vector<double> theAngles(nbAngles);
    list<double>::iterator it = Angles.begin();
    int i = -1;
    for(; it!=Angles.end(); it++) {
      i++;
      theAngles[i] = (*it);
    }
    list<double> res;
    double rAn2St = double( nbAngles ) / double( nbSteps );
    double angPrev = 0, angle;
    for ( int iSt = 0; iSt < nbSteps; ++iSt ) {
      double angCur = rAn2St * ( iSt+1 );
      double angCurFloor  = floor( angCur );
      double angPrevFloor = floor( angPrev );
      if ( angPrevFloor == angCurFloor )
        angle = rAn2St * theAngles[ int( angCurFloor ) ];
      else {
        int iP = int( angPrevFloor );
        double angPrevCeil = ceil(angPrev);
        angle = ( angPrevCeil - angPrev ) * theAngles[ iP ];

        int iC = int( angCurFloor );
        if ( iC < nbAngles )
          angle += ( angCur - angCurFloor ) * theAngles[ iC ];

        iP = int( angPrevCeil );
        while ( iC-- > iP )
          angle += theAngles[ iC ];
      }
      res.push_back(angle);
      angPrev = angCur;
    }
    Angles.clear();
    it = res.begin();
    for(; it!=res.end(); it++)
      Angles.push_back( *it );
  }
}


//================================================================================
/*!
 * \brief Move or copy theElements applying theTrsf to their nodes
 *  \param theElems - elements to transform, if theElems is empty then apply to all mesh nodes
 *  \param theTrsf - transformation to apply
 *  \param theCopy - if true, create translated copies of theElems
 *  \param theMakeGroups - if true and theCopy, create translated groups
 *  \param theTargetMesh - mesh to copy translated elements into
 *  \return SMESH_MeshEditor::PGroupIDs - list of ids of created groups
 */
//================================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::Transform (TIDSortedElemSet & theElems,
                             const gp_Trsf&     theTrsf,
                             const bool         theCopy,
                             const bool         theMakeGroups,
                             SMESH_Mesh*        theTargetMesh)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  bool needReverse = false;
  string groupPostfix;
  switch ( theTrsf.Form() ) {
  case gp_PntMirror:
    MESSAGE("gp_PntMirror");
    needReverse = true;
    groupPostfix = "mirrored";
    break;
  case gp_Ax1Mirror:
    MESSAGE("gp_Ax1Mirror");
    groupPostfix = "mirrored";
    break;
  case gp_Ax2Mirror:
    MESSAGE("gp_Ax2Mirror");
    needReverse = true;
    groupPostfix = "mirrored";
    break;
  case gp_Rotation:
    MESSAGE("gp_Rotation");
    groupPostfix = "rotated";
    break;
  case gp_Translation:
    MESSAGE("gp_Translation");
    groupPostfix = "translated";
    break;
  case gp_Scale:
    MESSAGE("gp_Scale");
    groupPostfix = "scaled";
    break;
  case gp_CompoundTrsf: // different scale by axis
    MESSAGE("gp_CompoundTrsf");
    groupPostfix = "scaled";
    break;
  default:
    MESSAGE("default");
    needReverse = false;
    groupPostfix = "transformed";
  }

  SMESHDS_Mesh* aTgtMesh = theTargetMesh ? theTargetMesh->GetMeshDS() : 0;
  SMESHDS_Mesh* aMesh    = GetMeshDS();

  SMESH_MeshEditor targetMeshEditor( theTargetMesh );
  SMESH_MeshEditor* editor = theTargetMesh ? & targetMeshEditor : theCopy ? this : 0;
  SMESH_MeshEditor::ElemFeatures elemType;

  // map old node to new one
  TNodeNodeMap nodeMap;

  // elements sharing moved nodes; those of them which have all
  // nodes mirrored but are not in theElems are to be reversed
  TIDSortedElemSet inverseElemSet;

  // source elements for each generated one
  SMESH_SequenceOfElemPtr srcElems, srcNodes;

  // issue 021015: EDF 1578 SMESH: Free nodes are removed when translating a mesh
  TIDSortedElemSet orphanNode;

  if ( theElems.empty() ) // transform the whole mesh
  {
    // add all elements
    SMDS_ElemIteratorPtr eIt = aMesh->elementsIterator();
    while ( eIt->more() ) theElems.insert( eIt->next() );
    // add orphan nodes
    SMDS_NodeIteratorPtr nIt = aMesh->nodesIterator();
    while ( nIt->more() )
    {
      const SMDS_MeshNode* node = nIt->next();
      if ( node->NbInverseElements() == 0)
        orphanNode.insert( node );
    }
  }

  // loop on elements to transform nodes : first orphan nodes then elems
  TIDSortedElemSet::iterator itElem;
  TIDSortedElemSet *elements[] = { &orphanNode, &theElems };
  for (int i=0; i<2; i++)
    for ( itElem = elements[i]->begin(); itElem != elements[i]->end(); itElem++ )
    {
      const SMDS_MeshElement* elem = *itElem;
      if ( !elem )
        continue;

      // loop on elem nodes
      double coord[3];
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      while ( itN->more() )
      {
        const SMDS_MeshNode* node = cast2Node( itN->next() );
        // check if a node has been already transformed
        pair<TNodeNodeMap::iterator,bool> n2n_isnew =
          nodeMap.insert( make_pair ( node, node ));
        if ( !n2n_isnew.second )
          continue;

        node->GetXYZ( coord );
        theTrsf.Transforms( coord[0], coord[1], coord[2] );
        if ( theTargetMesh ) {
          const SMDS_MeshNode * newNode = aTgtMesh->AddNode( coord[0], coord[1], coord[2] );
          n2n_isnew.first->second = newNode;
          myLastCreatedNodes.Append(newNode);
          srcNodes.Append( node );
        }
        else if ( theCopy ) {
          const SMDS_MeshNode * newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
          n2n_isnew.first->second = newNode;
          myLastCreatedNodes.Append(newNode);
          srcNodes.Append( node );
        }
        else {
          aMesh->MoveNode( node, coord[0], coord[1], coord[2] );
          // node position on shape becomes invalid
          const_cast< SMDS_MeshNode* > ( node )->SetPosition
            ( SMDS_SpacePosition::originSpacePosition() );
        }

        // keep inverse elements
        if ( !theCopy && !theTargetMesh && needReverse ) {
          SMDS_ElemIteratorPtr invElemIt = node->GetInverseElementIterator();
          while ( invElemIt->more() ) {
            const SMDS_MeshElement* iel = invElemIt->next();
            inverseElemSet.insert( iel );
          }
        }
      }
    } // loop on elems in { &orphanNode, &theElems };

  // either create new elements or reverse mirrored ones
  if ( !theCopy && !needReverse && !theTargetMesh )
    return PGroupIDs();

  theElems.insert( inverseElemSet.begin(),inverseElemSet.end() );

  // Replicate or reverse elements

  std::vector<int> iForw;
  vector<const SMDS_MeshNode*> nodes;
  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ )
  {
    const SMDS_MeshElement* elem = *itElem;
    if ( !elem ) continue;

    SMDSAbs_GeometryType geomType = elem->GetGeomType();
    int                  nbNodes  = elem->NbNodes();
    if ( geomType == SMDSGeom_NONE ) continue; // node

    nodes.resize( nbNodes );

    if ( geomType == SMDSGeom_POLYHEDRA )  // ------------------ polyhedral volume
    {
      const SMDS_VtkVolume* aPolyedre = dynamic_cast<const SMDS_VtkVolume*>( elem );
      if (!aPolyedre)
        continue;
      nodes.clear();
      bool allTransformed = true;
      int nbFaces = aPolyedre->NbFaces();
      for (int iface = 1; iface <= nbFaces && allTransformed; iface++)
      {
        int nbFaceNodes = aPolyedre->NbFaceNodes(iface);
        for (int inode = 1; inode <= nbFaceNodes && allTransformed; inode++)
        {
          const SMDS_MeshNode* node = aPolyedre->GetFaceNode(iface, inode);
          TNodeNodeMap::iterator nodeMapIt = nodeMap.find(node);
          if ( nodeMapIt == nodeMap.end() )
            allTransformed = false; // not all nodes transformed
          else
            nodes.push_back((*nodeMapIt).second);
        }
        if ( needReverse && allTransformed )
          std::reverse( nodes.end() - nbFaceNodes, nodes.end() );
      }
      if ( !allTransformed )
        continue; // not all nodes transformed
    }
    else // ----------------------- the rest element types
    {
      while ( iForw.size() < nbNodes ) iForw.push_back( iForw.size() );
      const vector<int>& iRev = SMDS_MeshCell::reverseSmdsOrder( elem->GetEntityType(), nbNodes );
      const vector<int>&    i = needReverse ? iRev : iForw;

      // find transformed nodes
      int iNode = 0;
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      while ( itN->more() ) {
        const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( itN->next() );
        TNodeNodeMap::iterator nodeMapIt = nodeMap.find( node );
        if ( nodeMapIt == nodeMap.end() )
          break; // not all nodes transformed
        nodes[ i [ iNode++ ]] = (*nodeMapIt).second;
      }
      if ( iNode != nbNodes )
        continue; // not all nodes transformed
    }

    if ( editor ) {
      // copy in this or a new mesh
      if ( editor->AddElement( nodes, elemType.Init( elem, /*basicOnly=*/false )))
        srcElems.Append( elem );
    }
    else {
      // reverse element as it was reversed by transformation
      if ( nbNodes > 2 )
        aMesh->ChangeElementNodes( elem, &nodes[0], nbNodes );
    }

  } // loop on elements

  if ( editor && editor != this )
    myLastCreatedElems = editor->myLastCreatedElems;

  PGroupIDs newGroupIDs;

  if ( ( theMakeGroups && theCopy ) ||
       ( theMakeGroups && theTargetMesh ) )
    newGroupIDs = generateGroups( srcNodes, srcElems, groupPostfix, theTargetMesh, false );

  return newGroupIDs;
}

//=======================================================================
/*!
 * \brief Create groups of elements made during transformation
 *  \param nodeGens - nodes making corresponding myLastCreatedNodes
 *  \param elemGens - elements making corresponding myLastCreatedElems
 *  \param postfix - to append to names of new groups
 *  \param targetMesh - mesh to create groups in
 *  \param topPresent - is there "top" elements that are created by sweeping
 */
//=======================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::generateGroups(const SMESH_SequenceOfElemPtr& nodeGens,
                                 const SMESH_SequenceOfElemPtr& elemGens,
                                 const std::string&             postfix,
                                 SMESH_Mesh*                    targetMesh,
                                 const bool                     topPresent)
{
  PGroupIDs newGroupIDs( new list<int> );
  SMESH_Mesh* mesh = targetMesh ? targetMesh : GetMesh();

  // Sort existing groups by types and collect their names

  // containers to store an old group and generated new ones;
  // 1st new group is for result elems of different type than a source one;
  // 2nd new group is for same type result elems ("top" group at extrusion)
  using boost::tuple;
  using boost::make_tuple;
  typedef tuple< SMESHDS_GroupBase*, SMESHDS_Group*, SMESHDS_Group* > TOldNewGroup;
  vector< list< TOldNewGroup > > groupsByType( SMDSAbs_NbElementTypes );
  vector< TOldNewGroup* > orderedOldNewGroups; // in order of old groups
  // group names
  set< string > groupNames;

  SMESH_Mesh::GroupIteratorPtr groupIt = GetMesh()->GetGroups();
  if ( !groupIt->more() ) return newGroupIDs;

  int newGroupID = mesh->GetGroupIds().back()+1;
  while ( groupIt->more() )
  {
    SMESH_Group * group = groupIt->next();
    if ( !group ) continue;
    SMESHDS_GroupBase* groupDS = group->GetGroupDS();
    if ( !groupDS || groupDS->IsEmpty() ) continue;
    groupNames.insert    ( group->GetName() );
    groupDS->SetStoreName( group->GetName() );
    const SMDSAbs_ElementType type = groupDS->GetType();
    SMESHDS_Group* newGroup    = new SMESHDS_Group( newGroupID++, mesh->GetMeshDS(), type );
    SMESHDS_Group* newTopGroup = new SMESHDS_Group( newGroupID++, mesh->GetMeshDS(), type );
    groupsByType[ type ].push_back( make_tuple( groupDS, newGroup, newTopGroup ));
    orderedOldNewGroups.push_back( & groupsByType[ type ].back() );
  }

  // Loop on nodes and elements to add them in new groups

  vector< const SMDS_MeshElement* > resultElems;
  for ( int isNodes = 0; isNodes < 2; ++isNodes )
  {
    const SMESH_SequenceOfElemPtr& gens  = isNodes ? nodeGens : elemGens;
    const SMESH_SequenceOfElemPtr& elems = isNodes ? myLastCreatedNodes : myLastCreatedElems;
    if ( gens.Length() != elems.Length() )
      throw SALOME_Exception("SMESH_MeshEditor::generateGroups(): invalid args");

    // loop on created elements
    for (int iElem = 1; iElem <= elems.Length(); ++iElem )
    {
      const SMDS_MeshElement* sourceElem = gens( iElem );
      if ( !sourceElem ) {
        MESSAGE("generateGroups(): NULL source element");
        continue;
      }
      list< TOldNewGroup > & groupsOldNew = groupsByType[ sourceElem->GetType() ];
      if ( groupsOldNew.empty() ) { // no groups of this type at all
        while ( iElem < gens.Length() && gens( iElem+1 ) == sourceElem )
          ++iElem; // skip all elements made by sourceElem
        continue;
      }
      // collect all elements made by the iElem-th sourceElem
      resultElems.clear();
      if ( const SMDS_MeshElement* resElem = elems( iElem ))
        if ( resElem != sourceElem )
          resultElems.push_back( resElem );
      while ( iElem < gens.Length() && gens( iElem+1 ) == sourceElem )
        if ( const SMDS_MeshElement* resElem = elems( ++iElem ))
          if ( resElem != sourceElem )
            resultElems.push_back( resElem );

      const SMDS_MeshElement* topElem = 0;
      if ( isNodes ) // there must be a top element
      {
        topElem = resultElems.back();
        resultElems.pop_back();
      }
      else
      {
        vector< const SMDS_MeshElement* >::reverse_iterator resElemIt = resultElems.rbegin();
        for ( ; resElemIt != resultElems.rend() ; ++resElemIt )
          if ( (*resElemIt)->GetType() == sourceElem->GetType() )
          {
            topElem = *resElemIt;
            *resElemIt = 0; // erase *resElemIt
            break;
          }
      }
      // add resultElems to groups originted from ones the sourceElem belongs to
      list< TOldNewGroup >::iterator gOldNew, gLast = groupsOldNew.end();
      for ( gOldNew = groupsOldNew.begin(); gOldNew != gLast; ++gOldNew )
      {
        SMESHDS_GroupBase* oldGroup = gOldNew->get<0>();
        if ( oldGroup->Contains( sourceElem )) // sourceElem is in oldGroup
        {
          // fill in a new group
          SMDS_MeshGroup & newGroup = gOldNew->get<1>()->SMDSGroup();
          vector< const SMDS_MeshElement* >::iterator resLast = resultElems.end(), resElemIt;
          for ( resElemIt = resultElems.begin(); resElemIt != resLast; ++resElemIt )
            if ( *resElemIt )
              newGroup.Add( *resElemIt );

          // fill a "top" group
          if ( topElem )
          {
            SMDS_MeshGroup & newTopGroup = gOldNew->get<2>()->SMDSGroup();
            newTopGroup.Add( topElem );
         }
        }
      }
    } // loop on created elements
  }// loop on nodes and elements

  // Create new SMESH_Groups from SMESHDS_Groups and remove empty SMESHDS_Groups

  list<int> topGrouIds;
  for ( size_t i = 0; i < orderedOldNewGroups.size(); ++i )
  {
    SMESHDS_GroupBase* oldGroupDS =   orderedOldNewGroups[i]->get<0>();
    SMESHDS_Group*   newGroups[2] = { orderedOldNewGroups[i]->get<1>(),
                                      orderedOldNewGroups[i]->get<2>() };
    for ( int is2nd = 0; is2nd < 2; ++is2nd )
    {
      SMESHDS_Group* newGroupDS = newGroups[ is2nd ];
      if ( newGroupDS->IsEmpty() )
      {
        mesh->GetMeshDS()->RemoveGroup( newGroupDS );
      }
      else
      {
        // set group type
        newGroupDS->SetType( newGroupDS->GetElements()->next()->GetType() );

        // make a name
        const bool isTop = ( topPresent &&
                             newGroupDS->GetType() == oldGroupDS->GetType() &&
                             is2nd );

        string name = oldGroupDS->GetStoreName();
        { // remove trailing whitespaces (issue 22599)
          size_t size = name.size();
          while ( size > 1 && isspace( name[ size-1 ]))
            --size;
          if ( size != name.size() )
          {
            name.resize( size );
            oldGroupDS->SetStoreName( name.c_str() );
          }
        }
        if ( !targetMesh ) {
          string suffix = ( isTop ? "top": postfix.c_str() );
          name += "_";
          name += suffix;
          int nb = 1;
          while ( !groupNames.insert( name ).second ) // name exists
            name = SMESH_Comment( oldGroupDS->GetStoreName() ) << "_" << suffix << "_" << nb++;
        }
        else if ( isTop ) {
          name += "_top";
        }
        newGroupDS->SetStoreName( name.c_str() );

        // make a SMESH_Groups
        mesh->AddGroup( newGroupDS );
        if ( isTop )
          topGrouIds.push_back( newGroupDS->GetID() );
        else
          newGroupIDs->push_back( newGroupDS->GetID() );
      }
    }
  }
  newGroupIDs->splice( newGroupIDs->end(), topGrouIds );

  return newGroupIDs;
}

//================================================================================
/*!
 *  * \brief Return list of group of nodes close to each other within theTolerance
 *  *        Search among theNodes or in the whole mesh if theNodes is empty using
 *  *        an Octree algorithm
 *  \param [in,out] theNodes - the nodes to treat
 *  \param [in]     theTolerance - the tolerance
 *  \param [out]    theGroupsOfNodes - the result groups of coincident nodes
 *  \param [in]     theSeparateCornersAndMedium - if \c true, in quadratic mesh puts 
 *         corner and medium nodes in separate groups
 */
//================================================================================

void SMESH_MeshEditor::FindCoincidentNodes (TIDSortedNodeSet &   theNodes,
                                            const double         theTolerance,
                                            TListOfListOfNodes & theGroupsOfNodes,
                                            bool                 theSeparateCornersAndMedium)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if ( myMesh->NbEdges  ( ORDER_QUADRATIC ) +
       myMesh->NbFaces  ( ORDER_QUADRATIC ) +
       myMesh->NbVolumes( ORDER_QUADRATIC ) == 0 )
    theSeparateCornersAndMedium = false;

  TIDSortedNodeSet& corners = theNodes;
  TIDSortedNodeSet  medium;

  if ( theNodes.empty() ) // get all nodes in the mesh
  {
    TIDSortedNodeSet* nodes[2] = { &corners, &medium };
    SMDS_NodeIteratorPtr nIt = GetMeshDS()->nodesIterator(/*idInceasingOrder=*/true);
    if ( theSeparateCornersAndMedium )
      while ( nIt->more() )
      {
        const SMDS_MeshNode* n = nIt->next();
        TIDSortedNodeSet* & nodeSet = nodes[ SMESH_MesherHelper::IsMedium( n )];
        nodeSet->insert( nodeSet->end(), n );
      }
    else
      while ( nIt->more() )
        theNodes.insert( theNodes.end(),nIt->next() );
  }
  else if ( theSeparateCornersAndMedium ) // separate corners from medium nodes
  {
    TIDSortedNodeSet::iterator nIt = corners.begin();
    while ( nIt != corners.end() )
      if ( SMESH_MesherHelper::IsMedium( *nIt ))
      {
        medium.insert( medium.end(), *nIt );
        corners.erase( nIt++ );
      }
      else
      {
        ++nIt;
      }
  }

  if ( !corners.empty() )
    SMESH_OctreeNode::FindCoincidentNodes ( corners, &theGroupsOfNodes, theTolerance );
  if ( !medium.empty() )
    SMESH_OctreeNode::FindCoincidentNodes ( medium, &theGroupsOfNodes, theTolerance );
}

//=======================================================================
//function : SimplifyFace
//purpose  : split a chain of nodes into several closed chains
//=======================================================================

int SMESH_MeshEditor::SimplifyFace (const vector<const SMDS_MeshNode *>& faceNodes,
                                    vector<const SMDS_MeshNode *>&       poly_nodes,
                                    vector<int>&                         quantities) const
{
  int nbNodes = faceNodes.size();

  if (nbNodes < 3)
    return 0;

  set<const SMDS_MeshNode*> nodeSet;

  // get simple seq of nodes
  vector<const SMDS_MeshNode*> simpleNodes( nbNodes );
  int iSimple = 0;

  simpleNodes[iSimple++] = faceNodes[0];
  for (int iCur = 1; iCur < nbNodes; iCur++) {
    if (faceNodes[iCur] != simpleNodes[iSimple - 1]) {
      simpleNodes[iSimple++] = faceNodes[iCur];
      nodeSet.insert( faceNodes[iCur] );
    }
  }
  int nbUnique = nodeSet.size();
  int nbSimple = iSimple;
  if (simpleNodes[nbSimple - 1] == simpleNodes[0]) {
    nbSimple--;
    iSimple--;
  }

  if (nbUnique < 3)
    return 0;

  // separate loops
  int nbNew = 0;
  bool foundLoop = (nbSimple > nbUnique);
  while (foundLoop) {
    foundLoop = false;
    set<const SMDS_MeshNode*> loopSet;
    for (iSimple = 0; iSimple < nbSimple && !foundLoop; iSimple++) {
      const SMDS_MeshNode* n = simpleNodes[iSimple];
      if (!loopSet.insert( n ).second) {
        foundLoop = true;

        // separate loop
        int iC = 0, curLast = iSimple;
        for (; iC < curLast; iC++) {
          if (simpleNodes[iC] == n) break;
        }
        int loopLen = curLast - iC;
        if (loopLen > 2) {
          // create sub-element
          nbNew++;
          quantities.push_back(loopLen);
          for (; iC < curLast; iC++) {
            poly_nodes.push_back(simpleNodes[iC]);
          }
        }
        // shift the rest nodes (place from the first loop position)
        for (iC = curLast + 1; iC < nbSimple; iC++) {
          simpleNodes[iC - loopLen] = simpleNodes[iC];
        }
        nbSimple -= loopLen;
        iSimple -= loopLen;
      }
    } // for (iSimple = 0; iSimple < nbSimple; iSimple++)
  } // while (foundLoop)

  if (iSimple > 2) {
    nbNew++;
    quantities.push_back(iSimple);
    for (int i = 0; i < iSimple; i++)
      poly_nodes.push_back(simpleNodes[i]);
  }

  return nbNew;
}

//=======================================================================
//function : MergeNodes
//purpose  : In each group, the cdr of nodes are substituted by the first one
//           in all elements.
//=======================================================================

void SMESH_MeshEditor::MergeNodes (TListOfListOfNodes & theGroupsOfNodes)
{
  MESSAGE("MergeNodes");
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  SMESHDS_Mesh* aMesh = GetMeshDS();

  TNodeNodeMap nodeNodeMap; // node to replace - new node
  set<const SMDS_MeshElement*> elems; // all elements with changed nodes
  list< int > rmElemIds, rmNodeIds;

  // Fill nodeNodeMap and elems

  TListOfListOfNodes::iterator grIt = theGroupsOfNodes.begin();
  for ( ; grIt != theGroupsOfNodes.end(); grIt++ )
  {
    list<const SMDS_MeshNode*>& nodes = *grIt;
    list<const SMDS_MeshNode*>::iterator nIt = nodes.begin();
    const SMDS_MeshNode* nToKeep = *nIt;
    for ( ++nIt; nIt != nodes.end(); nIt++ )
    {
      const SMDS_MeshNode* nToRemove = *nIt;
      nodeNodeMap.insert( make_pair( nToRemove, nToKeep ));
      if ( nToRemove != nToKeep )
      {
        rmNodeIds.push_back( nToRemove->GetID() );
        AddToSameGroups( nToKeep, nToRemove, aMesh );
        // set _alwaysComputed to a sub-mesh of VERTEX to enable mesh computing
        // after MergeNodes() w/o creating node in place of merged ones.
        const SMDS_PositionPtr& pos = nToRemove->GetPosition();
        if ( pos && pos->GetTypeOfPosition() == SMDS_TOP_VERTEX )
          if ( SMESH_subMesh* sm = myMesh->GetSubMeshContaining( nToRemove->getshapeId() ))
            sm->SetIsAlwaysComputed( true );
      }
      SMDS_ElemIteratorPtr invElemIt = nToRemove->GetInverseElementIterator();
      while ( invElemIt->more() ) {
        const SMDS_MeshElement* elem = invElemIt->next();
        elems.insert(elem);
      }
    }
  }
  // Change element nodes or remove an element

  set<const SMDS_MeshNode*> nodeSet;
  vector< const SMDS_MeshNode*> curNodes, uniqueNodes;
  vector<int> iRepl;
  ElemFeatures elemType;

  set<const SMDS_MeshElement*>::iterator eIt = elems.begin();
  for ( ; eIt != elems.end(); eIt++ )
  {
    const SMDS_MeshElement* elem = *eIt;
    const           int  nbNodes = elem->NbNodes();
    const           int aShapeId = FindShape( elem );

    nodeSet.clear();
    curNodes.resize( nbNodes );
    uniqueNodes.resize( nbNodes );
    iRepl.resize( nbNodes );
    int iUnique = 0, iCur = 0, nbRepl = 0;

    // get new seq of nodes
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    while ( itN->more() )
    {
      const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( itN->next() );

      TNodeNodeMap::iterator nnIt = nodeNodeMap.find( n );
      if ( nnIt != nodeNodeMap.end() ) { // n sticks
        n = (*nnIt).second;
        { ////////// BUG 0020185: begin
          bool stopRecur = false;
          set<const SMDS_MeshNode*> nodesRecur;
          nodesRecur.insert(n);
          while (!stopRecur) {
            TNodeNodeMap::iterator nnIt_i = nodeNodeMap.find( n );
            if ( nnIt_i != nodeNodeMap.end() ) { // n sticks
              n = (*nnIt_i).second;
              if (!nodesRecur.insert(n).second) {
                // error: recursive dependancy
                stopRecur = true;
              }
            }
            else
              stopRecur = true;
          }
        } ////////// BUG 0020185: end
      }
      curNodes[ iCur ] = n;
      bool isUnique = nodeSet.insert( n ).second;
      if ( isUnique )
        uniqueNodes[ iUnique++ ] = n;
      else
        iRepl[ nbRepl++ ] = iCur;
      iCur++;
    }

    // Analyse element topology after replacement

    bool isOk = true;
    int nbUniqueNodes = nodeSet.size();
    if ( nbNodes != nbUniqueNodes ) // some nodes stick
    {
      if (elem->IsPoly()) // Polygons and Polyhedral volumes
      {
        if (elem->GetType() == SMDSAbs_Face) // Polygon
        {
          elemType.Init( elem );
          const bool isQuad = elemType.myIsQuad;
          if ( isQuad ) {
#ifndef VTK_NO_QUAD_POLY
            SMDS_MeshCell::applyInterlace // interlace medium and corner nodes
              ( SMDS_MeshCell::interlacedSmdsOrder( SMDSEntity_Quad_Polygon, nbNodes ), curNodes );
#else
            throw SALOME_Exception("Quadratic polygon not supported with VTK <6.2");  
#endif
          }

          // a polygon can divide into several elements
          vector<const SMDS_MeshNode *> polygons_nodes;
          vector<int> quantities;
          int nbNew = SimplifyFace( curNodes, polygons_nodes, quantities );
          if (nbNew > 0)
          {
            vector<const SMDS_MeshNode *> face_nodes;
            int inode = 0;
            for (int iface = 0; iface < nbNew; iface++)
            {
              int nbNewNodes = quantities[iface];
              face_nodes.assign( polygons_nodes.begin() + inode,
                                 polygons_nodes.begin() + inode + nbNewNodes );
              inode += nbNewNodes;
              if ( isQuad ) // check if a result elem is a valid quadratic polygon
              {
#ifndef VTK_NO_QUAD_POLY
                bool isValid = ( nbNewNodes % 2 == 0 );
                for ( int i = 0; i < nbNewNodes && isValid; ++i )
                  isValid = ( elem->IsMediumNode( face_nodes[i]) == bool( i % 2 ));
                elemType.SetQuad( isValid );
                if ( isValid ) // put medium nodes after corners
                  SMDS_MeshCell::applyInterlaceRev
                    ( SMDS_MeshCell::interlacedSmdsOrder( SMDSEntity_Quad_Polygon,
                                                          nbNewNodes ), face_nodes );
#else
                throw SALOME_Exception("Quadratic polygon not supported with VTK <6.2");  
#endif
              }
              elemType.SetPoly(( nbNewNodes / ( elemType.myIsQuad + 1 ) > 4 ));

              SMDS_MeshElement* newElem = AddElement( face_nodes, elemType );
              if ( aShapeId )
                aMesh->SetMeshElementOnShape(newElem, aShapeId);
            }
          }
          rmElemIds.push_back(elem->GetID());

        } // Polygon

        else if (elem->GetType() == SMDSAbs_Volume) // Polyhedral volume
        {
          if (nbUniqueNodes < 4) {
            rmElemIds.push_back(elem->GetID());
          }
          else {
            // each face has to be analyzed in order to check volume validity
            const SMDS_VtkVolume* aPolyedre = dynamic_cast<const SMDS_VtkVolume*>( elem );
            if (aPolyedre)
            {
              int nbFaces = aPolyedre->NbFaces();

              vector<const SMDS_MeshNode *> poly_nodes;
              vector<int> quantities;

              for (int iface = 1; iface <= nbFaces; iface++) {
                int nbFaceNodes = aPolyedre->NbFaceNodes(iface);
                vector<const SMDS_MeshNode *> faceNodes (nbFaceNodes);

                for (int inode = 1; inode <= nbFaceNodes; inode++) {
                  const SMDS_MeshNode * faceNode = aPolyedre->GetFaceNode(iface, inode);
                  TNodeNodeMap::iterator nnIt = nodeNodeMap.find(faceNode);
                  if (nnIt != nodeNodeMap.end()) { // faceNode sticks
                    faceNode = (*nnIt).second;
                  }
                  faceNodes[inode - 1] = faceNode;
                }

                SimplifyFace(faceNodes, poly_nodes, quantities);
              }

              if (quantities.size() > 3) {
                // to be done: remove coincident faces
              }

              if (quantities.size() > 3)
              {
                const SMDS_MeshElement* newElem =
                  aMesh->AddPolyhedralVolume(poly_nodes, quantities);
                myLastCreatedElems.Append(newElem);
                if ( aShapeId && newElem )
                  aMesh->SetMeshElementOnShape( newElem, aShapeId );
                rmElemIds.push_back(elem->GetID());
              }
            }
            else {
              rmElemIds.push_back(elem->GetID());
            }
          }
        }
        else {
        }

        continue;
      } // poly element

      // Regular elements
      // TODO not all the possible cases are solved. Find something more generic?
      switch ( nbNodes ) {
      case 2: ///////////////////////////////////// EDGE
        isOk = false; break;
      case 3: ///////////////////////////////////// TRIANGLE
        isOk = false; break;
      case 4:
        if ( elem->GetType() == SMDSAbs_Volume ) // TETRAHEDRON
          isOk = false;
        else { //////////////////////////////////// QUADRANGLE
          if ( nbUniqueNodes < 3 )
            isOk = false;
          else if ( nbRepl == 2 && iRepl[ 1 ] - iRepl[ 0 ] == 2 )
            isOk = false; // opposite nodes stick
          //MESSAGE("isOk " << isOk);
        }
        break;
      case 6: ///////////////////////////////////// PENTAHEDRON
        if ( nbUniqueNodes == 4 ) {
          // ---------------------------------> tetrahedron
          if (nbRepl == 3 &&
              iRepl[ 0 ] > 2 && iRepl[ 1 ] > 2 && iRepl[ 2 ] > 2 ) {
            // all top nodes stick: reverse a bottom
            uniqueNodes[ 0 ] = curNodes [ 1 ];
            uniqueNodes[ 1 ] = curNodes [ 0 ];
          }
          else if (nbRepl == 3 &&
                   iRepl[ 0 ] < 3 && iRepl[ 1 ] < 3 && iRepl[ 2 ] < 3 ) {
            // all bottom nodes stick: set a top before
            uniqueNodes[ 3 ] = uniqueNodes [ 0 ];
            uniqueNodes[ 0 ] = curNodes [ 3 ];
            uniqueNodes[ 1 ] = curNodes [ 4 ];
            uniqueNodes[ 2 ] = curNodes [ 5 ];
          }
          else if (nbRepl == 4 &&
                   iRepl[ 2 ] - iRepl [ 0 ] == 3 && iRepl[ 3 ] - iRepl [ 1 ] == 3 ) {
            // a lateral face turns into a line: reverse a bottom
            uniqueNodes[ 0 ] = curNodes [ 1 ];
            uniqueNodes[ 1 ] = curNodes [ 0 ];
          }
          else
            isOk = false;
        }
        else if ( nbUniqueNodes == 5 ) {
          // PENTAHEDRON --------------------> 2 tetrahedrons
          if ( nbRepl == 2 && iRepl[ 1 ] - iRepl [ 0 ] == 3 ) {
            // a bottom node sticks with a linked top one
            // 1.
            SMDS_MeshElement* newElem =
              aMesh->AddVolume(curNodes[ 3 ],
                               curNodes[ 4 ],
                               curNodes[ 5 ],
                               curNodes[ iRepl[ 0 ] == 2 ? 1 : 2 ]);
            myLastCreatedElems.Append(newElem);
            if ( aShapeId )
              aMesh->SetMeshElementOnShape( newElem, aShapeId );
            // 2. : reverse a bottom
            uniqueNodes[ 0 ] = curNodes [ 1 ];
            uniqueNodes[ 1 ] = curNodes [ 0 ];
            nbUniqueNodes = 4;
          }
          else
            isOk = false;
        }
        else
          isOk = false;
        break;
      case 8: {
        if(elem->IsQuadratic()) { // Quadratic quadrangle
          //   1    5    2
          //    +---+---+
          //    |       |
          //    |       |
          //   4+       +6
          //    |       |
          //    |       |
          //    +---+---+
          //   0    7    3
          isOk = false;
          if(nbRepl==2) {
            MESSAGE("nbRepl=2: " << iRepl[0] << " " << iRepl[1]);
          }
          if(nbRepl==3) {
            MESSAGE("nbRepl=3: " << iRepl[0] << " " << iRepl[1]  << " " << iRepl[2]);
            nbUniqueNodes = 6;
            if( iRepl[0]==0 && iRepl[1]==1 && iRepl[2]==4 ) {
              uniqueNodes[0] = curNodes[0];
              uniqueNodes[1] = curNodes[2];
              uniqueNodes[2] = curNodes[3];
              uniqueNodes[3] = curNodes[5];
              uniqueNodes[4] = curNodes[6];
              uniqueNodes[5] = curNodes[7];
              isOk = true;
            }
            if( iRepl[0]==0 && iRepl[1]==3 && iRepl[2]==7 ) {
              uniqueNodes[0] = curNodes[0];
              uniqueNodes[1] = curNodes[1];
              uniqueNodes[2] = curNodes[2];
              uniqueNodes[3] = curNodes[4];
              uniqueNodes[4] = curNodes[5];
              uniqueNodes[5] = curNodes[6];
              isOk = true;
            }
            if( iRepl[0]==0 && iRepl[1]==4 && iRepl[2]==7 ) {
              uniqueNodes[0] = curNodes[1];
              uniqueNodes[1] = curNodes[2];
              uniqueNodes[2] = curNodes[3];
              uniqueNodes[3] = curNodes[5];
              uniqueNodes[4] = curNodes[6];
              uniqueNodes[5] = curNodes[0];
              isOk = true;
            }
            if( iRepl[0]==1 && iRepl[1]==2 && iRepl[2]==5 ) {
              uniqueNodes[0] = curNodes[0];
              uniqueNodes[1] = curNodes[1];
              uniqueNodes[2] = curNodes[3];
              uniqueNodes[3] = curNodes[4];
              uniqueNodes[4] = curNodes[6];
              uniqueNodes[5] = curNodes[7];
              isOk = true;
            }
            if( iRepl[0]==1 && iRepl[1]==4 && iRepl[2]==5 ) {
              uniqueNodes[0] = curNodes[0];
              uniqueNodes[1] = curNodes[2];
              uniqueNodes[2] = curNodes[3];
              uniqueNodes[3] = curNodes[1];
              uniqueNodes[4] = curNodes[6];
              uniqueNodes[5] = curNodes[7];
              isOk = true;
            }
            if( iRepl[0]==2 && iRepl[1]==3 && iRepl[2]==6 ) {
              uniqueNodes[0] = curNodes[0];
              uniqueNodes[1] = curNodes[1];
              uniqueNodes[2] = curNodes[2];
              uniqueNodes[3] = curNodes[4];
              uniqueNodes[4] = curNodes[5];
              uniqueNodes[5] = curNodes[7];
              isOk = true;
            }
            if( iRepl[0]==2 && iRepl[1]==5 && iRepl[2]==6 ) {
              uniqueNodes[0] = curNodes[0];
              uniqueNodes[1] = curNodes[1];
              uniqueNodes[2] = curNodes[3];
              uniqueNodes[3] = curNodes[4];
              uniqueNodes[4] = curNodes[2];
              uniqueNodes[5] = curNodes[7];
              isOk = true;
            }
            if( iRepl[0]==3 && iRepl[1]==6 && iRepl[2]==7 ) {
              uniqueNodes[0] = curNodes[0];
              uniqueNodes[1] = curNodes[1];
              uniqueNodes[2] = curNodes[2];
              uniqueNodes[3] = curNodes[4];
              uniqueNodes[4] = curNodes[5];
              uniqueNodes[5] = curNodes[3];
              isOk = true;
            }
          }
          if(nbRepl==4) {
            MESSAGE("nbRepl=4: " << iRepl[0] << " " << iRepl[1]  << " " << iRepl[2] << " " << iRepl[3]);
          }
          if(nbRepl==5) {
            MESSAGE("nbRepl=5: " << iRepl[0] << " " << iRepl[1]  << " " << iRepl[2] << " " << iRepl[3] << " " << iRepl[4]);
          }
          break;
        }
        //////////////////////////////////// HEXAHEDRON
        isOk = false;
        SMDS_VolumeTool hexa (elem);
        hexa.SetExternalNormal();
        if ( nbUniqueNodes == 4 && nbRepl == 4 ) {
          //////////////////////// HEX ---> 1 tetrahedron
          for ( int iFace = 0; iFace < 6; iFace++ ) {
            const int *ind = hexa.GetFaceNodesIndices( iFace ); // indices of face nodes
            if (curNodes[ind[ 0 ]] == curNodes[ind[ 1 ]] &&
                curNodes[ind[ 0 ]] == curNodes[ind[ 2 ]] &&
                curNodes[ind[ 0 ]] == curNodes[ind[ 3 ]] ) {
              // one face turns into a point ...
              int iOppFace = hexa.GetOppFaceIndex( iFace );
              ind = hexa.GetFaceNodesIndices( iOppFace );
              int nbStick = 0;
              for ( iCur = 0; iCur < 4 && nbStick < 2; iCur++ ) {
                if ( curNodes[ind[ iCur ]] == curNodes[ind[ iCur + 1 ]] )
                  nbStick++;
              }
              if ( nbStick == 1 ) {
                // ... and the opposite one - into a triangle.
                // set a top node
                ind = hexa.GetFaceNodesIndices( iFace );
                uniqueNodes[ 3 ] = curNodes[ind[ 0 ]];
                isOk = true;
              }
              break;
            }
          }
        }
        else if ( nbUniqueNodes == 6 && nbRepl == 2 ) {
          //////////////////////// HEX ---> 1 prism
          int nbTria = 0, iTria[3];
          const int *ind; // indices of face nodes
          // look for triangular faces
          for ( int iFace = 0; iFace < 6 && nbTria < 3; iFace++ ) {
            ind = hexa.GetFaceNodesIndices( iFace );
            TIDSortedNodeSet faceNodes;
            for ( iCur = 0; iCur < 4; iCur++ )
              faceNodes.insert( curNodes[ind[iCur]] );
            if ( faceNodes.size() == 3 )
              iTria[ nbTria++ ] = iFace;
          }
          // check if triangles are opposite
          if ( nbTria == 2 && iTria[0] == hexa.GetOppFaceIndex( iTria[1] ))
          {
            isOk = true;
            // set nodes of the bottom triangle
            ind = hexa.GetFaceNodesIndices( iTria[ 0 ]);
            vector<int> indB;
            for ( iCur = 0; iCur < 4; iCur++ )
              if ( ind[iCur] != iRepl[0] && ind[iCur] != iRepl[1])
                indB.push_back( ind[iCur] );
            if ( !hexa.IsForward() )
              std::swap( indB[0], indB[2] );
            for ( iCur = 0; iCur < 3; iCur++ )
              uniqueNodes[ iCur ] = curNodes[indB[iCur]];
            // set nodes of the top triangle
            const int *indT = hexa.GetFaceNodesIndices( iTria[ 1 ]);
            for ( iCur = 0; iCur < 3; ++iCur )
              for ( int j = 0; j < 4; ++j )
                if ( hexa.IsLinked( indB[ iCur ], indT[ j ] ))
                {
                  uniqueNodes[ iCur + 3 ] = curNodes[ indT[ j ]];
                  break;
                }
          }
          break;
        }
        else if (nbUniqueNodes == 5 && nbRepl == 4 ) {
          //////////////////// HEXAHEDRON ---> 2 tetrahedrons
          for ( int iFace = 0; iFace < 6; iFace++ ) {
            const int *ind = hexa.GetFaceNodesIndices( iFace ); // indices of face nodes
            if (curNodes[ind[ 0 ]] == curNodes[ind[ 1 ]] &&
                curNodes[ind[ 0 ]] == curNodes[ind[ 2 ]] &&
                curNodes[ind[ 0 ]] == curNodes[ind[ 3 ]] ) {
              // one face turns into a point ...
              int iOppFace = hexa.GetOppFaceIndex( iFace );
              ind = hexa.GetFaceNodesIndices( iOppFace );
              int nbStick = 0;
              iUnique = 2;  // reverse a tetrahedron 1 bottom
              for ( iCur = 0; iCur < 4 && nbStick == 0; iCur++ ) {
                if ( curNodes[ind[ iCur ]] == curNodes[ind[ iCur + 1 ]] )
                  nbStick++;
                else if ( iUnique >= 0 )
                  uniqueNodes[ iUnique-- ] = curNodes[ind[ iCur ]];
              }
              if ( nbStick == 0 ) {
                // ... and the opposite one is a quadrangle
                // set a top node
                const int* indTop = hexa.GetFaceNodesIndices( iFace );
                uniqueNodes[ 3 ] = curNodes[indTop[ 0 ]];
                nbUniqueNodes = 4;
                // tetrahedron 2
                SMDS_MeshElement* newElem =
                  aMesh->AddVolume(curNodes[ind[ 0 ]],
                                   curNodes[ind[ 3 ]],
                                   curNodes[ind[ 2 ]],
                                   curNodes[indTop[ 0 ]]);
                myLastCreatedElems.Append(newElem);
                if ( aShapeId )
                  aMesh->SetMeshElementOnShape( newElem, aShapeId );
                isOk = true;
              }
              break;
            }
          }
        }
        else if ( nbUniqueNodes == 6 && nbRepl == 4 ) {
          ////////////////// HEXAHEDRON ---> 2 tetrahedrons or 1 prism
          // find indices of quad and tri faces
          int iQuadFace[ 6 ], iTriFace[ 6 ], nbQuad = 0, nbTri = 0, iFace;
          for ( iFace = 0; iFace < 6; iFace++ ) {
            const int *ind = hexa.GetFaceNodesIndices( iFace ); // indices of face nodes
            nodeSet.clear();
            for ( iCur = 0; iCur < 4; iCur++ )
              nodeSet.insert( curNodes[ind[ iCur ]] );
            nbUniqueNodes = nodeSet.size();
            if ( nbUniqueNodes == 3 )
              iTriFace[ nbTri++ ] = iFace;
            else if ( nbUniqueNodes == 4 )
              iQuadFace[ nbQuad++ ] = iFace;
          }
          if (nbQuad == 2 && nbTri == 4 &&
              hexa.GetOppFaceIndex( iQuadFace[ 0 ] ) == iQuadFace[ 1 ]) {
            // 2 opposite quadrangles stuck with a diagonal;
            // sample groups of merged indices: (0-4)(2-6)
            // --------------------------------------------> 2 tetrahedrons
            const int *ind1 = hexa.GetFaceNodesIndices( iQuadFace[ 0 ]); // indices of quad1 nodes
            const int *ind2 = hexa.GetFaceNodesIndices( iQuadFace[ 1 ]);
            int i0, i1d, i2, i3d, i0t, i2t; // d-daigonal, t-top
            if (curNodes[ind1[ 0 ]] == curNodes[ind2[ 0 ]] &&
                curNodes[ind1[ 2 ]] == curNodes[ind2[ 2 ]]) {
              // stuck with 0-2 diagonal
              i0  = ind1[ 3 ];
              i1d = ind1[ 0 ];
              i2  = ind1[ 1 ];
              i3d = ind1[ 2 ];
              i0t = ind2[ 1 ];
              i2t = ind2[ 3 ];
            }
            else if (curNodes[ind1[ 1 ]] == curNodes[ind2[ 3 ]] &&
                     curNodes[ind1[ 3 ]] == curNodes[ind2[ 1 ]]) {
              // stuck with 1-3 diagonal
              i0  = ind1[ 0 ];
              i1d = ind1[ 1 ];
              i2  = ind1[ 2 ];
              i3d = ind1[ 3 ];
              i0t = ind2[ 0 ];
              i2t = ind2[ 1 ];
            }
            else {
              ASSERT(0);
            }
            // tetrahedron 1
            uniqueNodes[ 0 ] = curNodes [ i0 ];
            uniqueNodes[ 1 ] = curNodes [ i1d ];
            uniqueNodes[ 2 ] = curNodes [ i3d ];
            uniqueNodes[ 3 ] = curNodes [ i0t ];
            nbUniqueNodes = 4;
            // tetrahedron 2
            SMDS_MeshElement* newElem = aMesh->AddVolume(curNodes[ i1d ],
                                                         curNodes[ i2 ],
                                                         curNodes[ i3d ],
                                                         curNodes[ i2t ]);
            myLastCreatedElems.Append(newElem);
            if ( aShapeId )
              aMesh->SetMeshElementOnShape( newElem, aShapeId );
            isOk = true;
          }
          else if (( nbTri == 2 && nbQuad == 3 ) || // merged (0-4)(1-5)
                   ( nbTri == 4 && nbQuad == 2 )) { // merged (7-4)(1-5)
            // --------------------------------------------> prism
            // find 2 opposite triangles
            nbUniqueNodes = 6;
            for ( iFace = 0; iFace + 1 < nbTri; iFace++ ) {
              if ( hexa.GetOppFaceIndex( iTriFace[ iFace ] ) == iTriFace[ iFace + 1 ]) {
                // find indices of kept and replaced nodes
                // and fill unique nodes of 2 opposite triangles
                const int *ind1 = hexa.GetFaceNodesIndices( iTriFace[ iFace ]);
                const int *ind2 = hexa.GetFaceNodesIndices( iTriFace[ iFace + 1 ]);
                const SMDS_MeshNode** hexanodes = hexa.GetNodes();
                // fill unique nodes
                iUnique = 0;
                isOk = true;
                for ( iCur = 0; iCur < 4 && isOk; iCur++ ) {
                  const SMDS_MeshNode* n     = curNodes[ind1[ iCur ]];
                  const SMDS_MeshNode* nInit = hexanodes[ind1[ iCur ]];
                  if ( n == nInit ) {
                    // iCur of a linked node of the opposite face (make normals co-directed):
                    int iCurOpp = ( iCur == 1 || iCur == 3 ) ? 4 - iCur : iCur;
                    // check that correspondent corners of triangles are linked
                    if ( !hexa.IsLinked( ind1[ iCur ], ind2[ iCurOpp ] ))
                      isOk = false;
                    else {
                      uniqueNodes[ iUnique ] = n;
                      uniqueNodes[ iUnique + 3 ] = curNodes[ind2[ iCurOpp ]];
                      iUnique++;
                    }
                  }
                }
                break;
              }
            }
          }
        } // if ( nbUniqueNodes == 6 && nbRepl == 4 )
        else
        {
          MESSAGE("MergeNodes() removes hexahedron "<< elem);
        }
        break;
      } // HEXAHEDRON

      default:
        isOk = false;
      } // switch ( nbNodes )

    } // if ( nbNodes != nbUniqueNodes ) // some nodes stick

    if ( isOk ) // the non-poly elem remains valid after sticking nodes
    {
      if ( nbNodes != nbUniqueNodes ||
           !aMesh->ChangeElementNodes( elem, & curNodes[0], nbNodes ))
      {
        elemType.Init( elem ).SetID( elem->GetID() );

        SMESHDS_SubMesh * sm = aShapeId > 0 ? aMesh->MeshElements(aShapeId) : 0;
        aMesh->RemoveFreeElement(elem, sm, /*fromGroups=*/false);

        uniqueNodes.resize(nbUniqueNodes);
        SMDS_MeshElement* newElem = this->AddElement( uniqueNodes, elemType );
        if ( sm && newElem )
          sm->AddElement( newElem );
        if ( elem != newElem )
          ReplaceElemInGroups( elem, newElem, aMesh );
      }
    }
    else {
      // Remove invalid regular element or invalid polygon
      rmElemIds.push_back( elem->GetID() );
    }

  } // loop on elements

  // Remove bad elements, then equal nodes (order important)

  Remove( rmElemIds, false );
  Remove( rmNodeIds, true );

  return;
}


// ========================================================
// class   : SortableElement
// purpose : allow sorting elements basing on their nodes
// ========================================================
class SortableElement : public set <const SMDS_MeshElement*>
{
public:

  SortableElement( const SMDS_MeshElement* theElem )
  {
    myElem = theElem;
    SMDS_ElemIteratorPtr nodeIt = theElem->nodesIterator();
    while ( nodeIt->more() )
      this->insert( nodeIt->next() );
  }

  const SMDS_MeshElement* Get() const
  { return myElem; }

private:
  mutable const SMDS_MeshElement* myElem;
};

//=======================================================================
//function : FindEqualElements
//purpose  : Return list of group of elements built on the same nodes.
//           Search among theElements or in the whole mesh if theElements is empty
//=======================================================================

void SMESH_MeshEditor::FindEqualElements(TIDSortedElemSet &        theElements,
                                         TListOfListOfElementsID & theGroupsOfElementsID)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  typedef map< SortableElement, int > TMapOfNodeSet;
  typedef list<int> TGroupOfElems;

  if ( theElements.empty() )
  { // get all elements in the mesh
    SMDS_ElemIteratorPtr eIt = GetMeshDS()->elementsIterator();
    while ( eIt->more() )
      theElements.insert( theElements.end(), eIt->next() );
  }

  vector< TGroupOfElems > arrayOfGroups;
  TGroupOfElems groupOfElems;
  TMapOfNodeSet mapOfNodeSet;

  TIDSortedElemSet::iterator elemIt = theElements.begin();
  for ( int i = 0; elemIt != theElements.end(); ++elemIt )
  {
    const SMDS_MeshElement* curElem = *elemIt;
    SortableElement SE(curElem);
    // check uniqueness
    pair< TMapOfNodeSet::iterator, bool> pp = mapOfNodeSet.insert(make_pair(SE, i));
    if ( !pp.second ) { // one more coincident elem
      TMapOfNodeSet::iterator& itSE = pp.first;
      int ind = (*itSE).second;
      arrayOfGroups[ind].push_back( curElem->GetID() );
    }
    else {
      arrayOfGroups.push_back( groupOfElems );
      arrayOfGroups.back().push_back( curElem->GetID() );
      i++;
    }
  }

  groupOfElems.clear();
  vector< TGroupOfElems >::iterator groupIt = arrayOfGroups.begin();
  for ( ; groupIt != arrayOfGroups.end(); ++groupIt )
  {
    if ( groupIt->size() > 1 ) {
      //groupOfElems.sort(); -- theElements is sorted already
      theGroupsOfElementsID.push_back( groupOfElems );
      theGroupsOfElementsID.back().splice( theGroupsOfElementsID.back().end(), *groupIt );
    }
  }
}

//=======================================================================
//function : MergeElements
//purpose  : In each given group, substitute all elements by the first one.
//=======================================================================

void SMESH_MeshEditor::MergeElements(TListOfListOfElementsID & theGroupsOfElementsID)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  typedef list<int> TListOfIDs;
  TListOfIDs rmElemIds; // IDs of elems to remove

  SMESHDS_Mesh* aMesh = GetMeshDS();

  TListOfListOfElementsID::iterator groupsIt = theGroupsOfElementsID.begin();
  while ( groupsIt != theGroupsOfElementsID.end() ) {
    TListOfIDs& aGroupOfElemID = *groupsIt;
    aGroupOfElemID.sort();
    int elemIDToKeep = aGroupOfElemID.front();
    const SMDS_MeshElement* elemToKeep = aMesh->FindElement(elemIDToKeep);
    aGroupOfElemID.pop_front();
    TListOfIDs::iterator idIt = aGroupOfElemID.begin();
    while ( idIt != aGroupOfElemID.end() ) {
      int elemIDToRemove = *idIt;
      const SMDS_MeshElement* elemToRemove = aMesh->FindElement(elemIDToRemove);
      // add the kept element in groups of removed one (PAL15188)
      AddToSameGroups( elemToKeep, elemToRemove, aMesh );
      rmElemIds.push_back( elemIDToRemove );
      ++idIt;
    }
    ++groupsIt;
  }

  Remove( rmElemIds, false );
}

//=======================================================================
//function : MergeEqualElements
//purpose  : Remove all but one of elements built on the same nodes.
//=======================================================================

void SMESH_MeshEditor::MergeEqualElements()
{
  TIDSortedElemSet aMeshElements; /* empty input ==
                                     to merge equal elements in the whole mesh */
  TListOfListOfElementsID aGroupsOfElementsID;
  FindEqualElements(aMeshElements, aGroupsOfElementsID);
  MergeElements(aGroupsOfElementsID);
}

//=======================================================================
//function : findAdjacentFace
//purpose  :
//=======================================================================

static const SMDS_MeshElement* findAdjacentFace(const SMDS_MeshNode* n1,
                                                const SMDS_MeshNode* n2,
                                                const SMDS_MeshElement* elem)
{
  TIDSortedElemSet elemSet, avoidSet;
  if ( elem )
    avoidSet.insert ( elem );
  return SMESH_MeshAlgos::FindFaceInSet( n1, n2, elemSet, avoidSet );
}

//=======================================================================
//function : findSegment
//purpose  : Return a mesh segment by two nodes one of which can be medium
//=======================================================================

static const SMDS_MeshElement* findSegment(const SMDS_MeshNode* n1,
                                           const SMDS_MeshNode* n2)
{
  SMDS_ElemIteratorPtr it = n1->GetInverseElementIterator( SMDSAbs_Edge );
  while ( it->more() )
  {
    const SMDS_MeshElement* seg = it->next();
    if ( seg->GetNodeIndex( n2 ) >= 0 )
      return seg;
  }
  return 0;
}

//=======================================================================
//function : FindFreeBorder
//purpose  :
//=======================================================================

#define ControlFreeBorder SMESH::Controls::FreeEdges::IsFreeEdge

bool SMESH_MeshEditor::FindFreeBorder (const SMDS_MeshNode*             theFirstNode,
                                       const SMDS_MeshNode*             theSecondNode,
                                       const SMDS_MeshNode*             theLastNode,
                                       list< const SMDS_MeshNode* > &   theNodes,
                                       list< const SMDS_MeshElement* >& theFaces)
{
  if ( !theFirstNode || !theSecondNode )
    return false;
  // find border face between theFirstNode and theSecondNode
  const SMDS_MeshElement* curElem = findAdjacentFace( theFirstNode, theSecondNode, 0 );
  if ( !curElem )
    return false;

  theFaces.push_back( curElem );
  theNodes.push_back( theFirstNode );
  theNodes.push_back( theSecondNode );

  const SMDS_MeshNode *nIgnore = theFirstNode, *nStart = theSecondNode;
  TIDSortedElemSet foundElems;
  bool needTheLast = ( theLastNode != 0 );

  while ( nStart != theLastNode ) {
    if ( nStart == theFirstNode )
      return !needTheLast;

    // find all free border faces sharing form nStart

    list< const SMDS_MeshElement* > curElemList;
    list< const SMDS_MeshNode* >    nStartList;
    SMDS_ElemIteratorPtr invElemIt = nStart->GetInverseElementIterator(SMDSAbs_Face);
    while ( invElemIt->more() ) {
      const SMDS_MeshElement* e = invElemIt->next();
      if ( e == curElem || foundElems.insert( e ).second ) {
        // get nodes
        int iNode = 0, nbNodes = e->NbNodes();
        vector<const SMDS_MeshNode*> nodes(nbNodes+1);

        if ( e->IsQuadratic() ) {
          const SMDS_VtkFace* F =
            dynamic_cast<const SMDS_VtkFace*>(e);
          if (!F) throw SALOME_Exception(LOCALIZED("not an SMDS_VtkFace"));
          // use special nodes iterator
          SMDS_ElemIteratorPtr anIter = F->interlacedNodesElemIterator();
          while( anIter->more() ) {
            nodes[ iNode++ ] = cast2Node(anIter->next());
          }
        }
        else {
          SMDS_ElemIteratorPtr nIt = e->nodesIterator();
          while ( nIt->more() )
            nodes[ iNode++ ] = static_cast<const SMDS_MeshNode*>( nIt->next() );
        }
        nodes[ iNode ] = nodes[ 0 ];
        // check 2 links
        for ( iNode = 0; iNode < nbNodes; iNode++ )
          if (((nodes[ iNode ] == nStart && nodes[ iNode + 1] != nIgnore ) ||
               (nodes[ iNode + 1] == nStart && nodes[ iNode ] != nIgnore )) &&
              ControlFreeBorder( &nodes[ iNode ], e->GetID() ))
          {
            nStartList.push_back( nodes[ iNode + ( nodes[ iNode ] == nStart ? 1 : 0 )]);
            curElemList.push_back( e );
          }
      }
    }
    // analyse the found

    int nbNewBorders = curElemList.size();
    if ( nbNewBorders == 0 ) {
      // no free border furthermore
      return !needTheLast;
    }
    else if ( nbNewBorders == 1 ) {
      // one more element found
      nIgnore = nStart;
      nStart = nStartList.front();
      curElem = curElemList.front();
      theFaces.push_back( curElem );
      theNodes.push_back( nStart );
    }
    else {
      // several continuations found
      list< const SMDS_MeshElement* >::iterator curElemIt;
      list< const SMDS_MeshNode* >::iterator nStartIt;
      // check if one of them reached the last node
      if ( needTheLast ) {
        for (curElemIt = curElemList.begin(), nStartIt = nStartList.begin();
             curElemIt!= curElemList.end();
             curElemIt++, nStartIt++ )
          if ( *nStartIt == theLastNode ) {
            theFaces.push_back( *curElemIt );
            theNodes.push_back( *nStartIt );
            return true;
          }
      }
      // find the best free border by the continuations
      list<const SMDS_MeshNode*>    contNodes[ 2 ], *cNL;
      list<const SMDS_MeshElement*> contFaces[ 2 ], *cFL;
      for (curElemIt = curElemList.begin(), nStartIt = nStartList.begin();
           curElemIt!= curElemList.end();
           curElemIt++, nStartIt++ )
      {
        cNL = & contNodes[ contNodes[0].empty() ? 0 : 1 ];
        cFL = & contFaces[ contFaces[0].empty() ? 0 : 1 ];
        // find one more free border
        if ( ! SMESH_MeshEditor::FindFreeBorder( nStart, *nStartIt, theLastNode, *cNL, *cFL )) {
          cNL->clear();
          cFL->clear();
        }
        else if ( !contNodes[0].empty() && !contNodes[1].empty() ) {
          // choice: clear a worse one
          int iLongest = ( contNodes[0].size() < contNodes[1].size() ? 1 : 0 );
          int iWorse = ( needTheLast ? 1 - iLongest : iLongest );
          contNodes[ iWorse ].clear();
          contFaces[ iWorse ].clear();
        }
      }
      if ( contNodes[0].empty() && contNodes[1].empty() )
        return false;

      // append the best free border
      cNL = & contNodes[ contNodes[0].empty() ? 1 : 0 ];
      cFL = & contFaces[ contFaces[0].empty() ? 1 : 0 ];
      theNodes.pop_back(); // remove nIgnore
      theNodes.pop_back(); // remove nStart
      theFaces.pop_back(); // remove curElem
      list< const SMDS_MeshNode* >::iterator nIt = cNL->begin();
      list< const SMDS_MeshElement* >::iterator fIt = cFL->begin();
      for ( ; nIt != cNL->end(); nIt++ ) theNodes.push_back( *nIt );
      for ( ; fIt != cFL->end(); fIt++ ) theFaces.push_back( *fIt );
      return true;

    } // several continuations found
  } // while ( nStart != theLastNode )

  return true;
}

//=======================================================================
//function : CheckFreeBorderNodes
//purpose  : Return true if the tree nodes are on a free border
//=======================================================================

bool SMESH_MeshEditor::CheckFreeBorderNodes(const SMDS_MeshNode* theNode1,
                                            const SMDS_MeshNode* theNode2,
                                            const SMDS_MeshNode* theNode3)
{
  list< const SMDS_MeshNode* > nodes;
  list< const SMDS_MeshElement* > faces;
  return FindFreeBorder( theNode1, theNode2, theNode3, nodes, faces);
}

//=======================================================================
//function : SewFreeBorder
//purpose  :
//warning  : for border-to-side sewing theSideSecondNode is considered as
//           the last side node and theSideThirdNode is not used
//=======================================================================

SMESH_MeshEditor::Sew_Error
SMESH_MeshEditor::SewFreeBorder (const SMDS_MeshNode* theBordFirstNode,
                                 const SMDS_MeshNode* theBordSecondNode,
                                 const SMDS_MeshNode* theBordLastNode,
                                 const SMDS_MeshNode* theSideFirstNode,
                                 const SMDS_MeshNode* theSideSecondNode,
                                 const SMDS_MeshNode* theSideThirdNode,
                                 const bool           theSideIsFreeBorder,
                                 const bool           toCreatePolygons,
                                 const bool           toCreatePolyedrs)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  MESSAGE("::SewFreeBorder()");
  Sew_Error aResult = SEW_OK;

  // ====================================
  //    find side nodes and elements
  // ====================================

  list< const SMDS_MeshNode* >    nSide[ 2 ];
  list< const SMDS_MeshElement* > eSide[ 2 ];
  list< const SMDS_MeshNode* >::iterator    nIt[ 2 ];
  list< const SMDS_MeshElement* >::iterator eIt[ 2 ];

  // Free border 1
  // --------------
  if (!FindFreeBorder(theBordFirstNode,theBordSecondNode,theBordLastNode,
                      nSide[0], eSide[0])) {
    MESSAGE(" Free Border 1 not found " );
    aResult = SEW_BORDER1_NOT_FOUND;
  }
  if (theSideIsFreeBorder) {
    // Free border 2
    // --------------
    if (!FindFreeBorder(theSideFirstNode, theSideSecondNode, theSideThirdNode,
                        nSide[1], eSide[1])) {
      MESSAGE(" Free Border 2 not found " );
      aResult = ( aResult != SEW_OK ? SEW_BOTH_BORDERS_NOT_FOUND : SEW_BORDER2_NOT_FOUND );
    }
  }
  if ( aResult != SEW_OK )
    return aResult;

  if (!theSideIsFreeBorder) {
    // Side 2
    // --------------

    // -------------------------------------------------------------------------
    // Algo:
    // 1. If nodes to merge are not coincident, move nodes of the free border
    //    from the coord sys defined by the direction from the first to last
    //    nodes of the border to the correspondent sys of the side 2
    // 2. On the side 2, find the links most co-directed with the correspondent
    //    links of the free border
    // -------------------------------------------------------------------------

    // 1. Since sewing may break if there are volumes to split on the side 2,
    //    we wont move nodes but just compute new coordinates for them
    typedef map<const SMDS_MeshNode*, gp_XYZ> TNodeXYZMap;
    TNodeXYZMap nBordXYZ;
    list< const SMDS_MeshNode* >& bordNodes = nSide[ 0 ];
    list< const SMDS_MeshNode* >::iterator nBordIt;

    gp_XYZ Pb1( theBordFirstNode->X(), theBordFirstNode->Y(), theBordFirstNode->Z() );
    gp_XYZ Pb2( theBordLastNode->X(), theBordLastNode->Y(), theBordLastNode->Z() );
    gp_XYZ Ps1( theSideFirstNode->X(), theSideFirstNode->Y(), theSideFirstNode->Z() );
    gp_XYZ Ps2( theSideSecondNode->X(), theSideSecondNode->Y(), theSideSecondNode->Z() );
    double tol2 = 1.e-8;
    gp_Vec Vbs1( Pb1 - Ps1 ),Vbs2( Pb2 - Ps2 );
    if ( Vbs1.SquareMagnitude() > tol2 || Vbs2.SquareMagnitude() > tol2 ) {
      // Need node movement.

      // find X and Z axes to create trsf
      gp_Vec Zb( Pb1 - Pb2 ), Zs( Ps1 - Ps2 );
      gp_Vec X = Zs ^ Zb;
      if ( X.SquareMagnitude() <= gp::Resolution() * gp::Resolution() )
        // Zb || Zs
        X = gp_Ax2( gp::Origin(), Zb ).XDirection();

      // coord systems
      gp_Ax3 toBordAx( Pb1, Zb, X );
      gp_Ax3 fromSideAx( Ps1, Zs, X );
      gp_Ax3 toGlobalAx( gp::Origin(), gp::DZ(), gp::DX() );
      // set trsf
      gp_Trsf toBordSys, fromSide2Sys;
      toBordSys.SetTransformation( toBordAx );
      fromSide2Sys.SetTransformation( fromSideAx, toGlobalAx );
      fromSide2Sys.SetScaleFactor( Zs.Magnitude() / Zb.Magnitude() );

      // move
      for ( nBordIt = bordNodes.begin(); nBordIt != bordNodes.end(); nBordIt++ ) {
        const SMDS_MeshNode* n = *nBordIt;
        gp_XYZ xyz( n->X(),n->Y(),n->Z() );
        toBordSys.Transforms( xyz );
        fromSide2Sys.Transforms( xyz );
        nBordXYZ.insert( TNodeXYZMap::value_type( n, xyz ));
      }
    }
    else {
      // just insert nodes XYZ in the nBordXYZ map
      for ( nBordIt = bordNodes.begin(); nBordIt != bordNodes.end(); nBordIt++ ) {
        const SMDS_MeshNode* n = *nBordIt;
        nBordXYZ.insert( TNodeXYZMap::value_type( n, gp_XYZ( n->X(),n->Y(),n->Z() )));
      }
    }

    // 2. On the side 2, find the links most co-directed with the correspondent
    //    links of the free border

    list< const SMDS_MeshElement* >& sideElems = eSide[ 1 ];
    list< const SMDS_MeshNode* >& sideNodes = nSide[ 1 ];
    sideNodes.push_back( theSideFirstNode );

    bool hasVolumes = false;
    LinkID_Gen aLinkID_Gen( GetMeshDS() );
    set<long> foundSideLinkIDs, checkedLinkIDs;
    SMDS_VolumeTool volume;
    //const SMDS_MeshNode* faceNodes[ 4 ];

    const SMDS_MeshNode*    sideNode;
    const SMDS_MeshElement* sideElem;
    const SMDS_MeshNode* prevSideNode = theSideFirstNode;
    const SMDS_MeshNode* prevBordNode = theBordFirstNode;
    nBordIt = bordNodes.begin();
    nBordIt++;
    // border node position and border link direction to compare with
    gp_XYZ bordPos = nBordXYZ[ *nBordIt ];
    gp_XYZ bordDir = bordPos - nBordXYZ[ prevBordNode ];
    // choose next side node by link direction or by closeness to
    // the current border node:
    bool searchByDir = ( *nBordIt != theBordLastNode );
    do {
      // find the next node on the Side 2
      sideNode = 0;
      double maxDot = -DBL_MAX, minDist = DBL_MAX;
      long linkID;
      checkedLinkIDs.clear();
      gp_XYZ prevXYZ( prevSideNode->X(), prevSideNode->Y(), prevSideNode->Z() );

      // loop on inverse elements of current node (prevSideNode) on the Side 2
      SMDS_ElemIteratorPtr invElemIt = prevSideNode->GetInverseElementIterator();
      while ( invElemIt->more() )
      {
        const SMDS_MeshElement* elem = invElemIt->next();
        // prepare data for a loop on links coming to prevSideNode, of a face or a volume
        int iPrevNode, iNode = 0, nbNodes = elem->NbNodes();
        vector< const SMDS_MeshNode* > faceNodes( nbNodes, (const SMDS_MeshNode*)0 );
        bool isVolume = volume.Set( elem );
        const SMDS_MeshNode** nodes = isVolume ? volume.GetNodes() : & faceNodes[0];
        if ( isVolume ) // --volume
          hasVolumes = true;
        else if ( elem->GetType()==SMDSAbs_Face ) { // --face
          // retrieve all face nodes and find iPrevNode - an index of the prevSideNode
          if(elem->IsQuadratic()) {
            const SMDS_VtkFace* F =
              dynamic_cast<const SMDS_VtkFace*>(elem);
            if (!F) throw SALOME_Exception(LOCALIZED("not an SMDS_VtkFace"));
            // use special nodes iterator
            SMDS_ElemIteratorPtr anIter = F->interlacedNodesElemIterator();
            while( anIter->more() ) {
              nodes[ iNode ] = cast2Node(anIter->next());
              if ( nodes[ iNode++ ] == prevSideNode )
                iPrevNode = iNode - 1;
            }
          }
          else {
            SMDS_ElemIteratorPtr nIt = elem->nodesIterator();
            while ( nIt->more() ) {
              nodes[ iNode ] = cast2Node( nIt->next() );
              if ( nodes[ iNode++ ] == prevSideNode )
                iPrevNode = iNode - 1;
            }
          }
          // there are 2 links to check
          nbNodes = 2;
        }
        else // --edge
          continue;
        // loop on links, to be precise, on the second node of links
        for ( iNode = 0; iNode < nbNodes; iNode++ ) {
          const SMDS_MeshNode* n = nodes[ iNode ];
          if ( isVolume ) {
            if ( !volume.IsLinked( n, prevSideNode ))
              continue;
          }
          else {
            if ( iNode ) // a node before prevSideNode
              n = nodes[ iPrevNode == 0 ? elem->NbNodes() - 1 : iPrevNode - 1 ];
            else         // a node after prevSideNode
              n = nodes[ iPrevNode + 1 == elem->NbNodes() ? 0 : iPrevNode + 1 ];
          }
          // check if this link was already used
          long iLink = aLinkID_Gen.GetLinkID( prevSideNode, n );
          bool isJustChecked = !checkedLinkIDs.insert( iLink ).second;
          if (!isJustChecked &&
              foundSideLinkIDs.find( iLink ) == foundSideLinkIDs.end() )
          {
            // test a link geometrically
            gp_XYZ nextXYZ ( n->X(), n->Y(), n->Z() );
            bool linkIsBetter = false;
            double dot = 0.0, dist = 0.0;
            if ( searchByDir ) { // choose most co-directed link
              dot = bordDir * ( nextXYZ - prevXYZ ).Normalized();
              linkIsBetter = ( dot > maxDot );
            }
            else { // choose link with the node closest to bordPos
              dist = ( nextXYZ - bordPos ).SquareModulus();
              linkIsBetter = ( dist < minDist );
            }
            if ( linkIsBetter ) {
              maxDot = dot;
              minDist = dist;
              linkID = iLink;
              sideNode = n;
              sideElem = elem;
            }
          }
        }
      } // loop on inverse elements of prevSideNode

      if ( !sideNode ) {
        MESSAGE(" Cant find path by links of the Side 2 ");
        return SEW_BAD_SIDE_NODES;
      }
      sideNodes.push_back( sideNode );
      sideElems.push_back( sideElem );
      foundSideLinkIDs.insert ( linkID );
      prevSideNode = sideNode;

      if ( *nBordIt == theBordLastNode )
        searchByDir = false;
      else {
        // find the next border link to compare with
        gp_XYZ sidePos( sideNode->X(), sideNode->Y(), sideNode->Z() );
        searchByDir = ( bordDir * ( sidePos - bordPos ) <= 0 );
        // move to next border node if sideNode is before forward border node (bordPos)
        while ( *nBordIt != theBordLastNode && !searchByDir ) {
          prevBordNode = *nBordIt;
          nBordIt++;
          bordPos = nBordXYZ[ *nBordIt ];
          bordDir = bordPos - nBordXYZ[ prevBordNode ];
          searchByDir = ( bordDir * ( sidePos - bordPos ) <= 0 );
        }
      }
    }
    while ( sideNode != theSideSecondNode );

    if ( hasVolumes && sideNodes.size () != bordNodes.size() && !toCreatePolyedrs) {
      MESSAGE("VOLUME SPLITTING IS FORBIDDEN");
      return SEW_VOLUMES_TO_SPLIT; // volume splitting is forbidden
    }
  } // end nodes search on the side 2

  // ============================
  // sew the border to the side 2
  // ============================

  int nbNodes[]  = { (int)nSide[0].size(), (int)nSide[1].size() };
  int maxNbNodes = Max( nbNodes[0], nbNodes[1] );

  bool toMergeConformal = ( nbNodes[0] == nbNodes[1] );
  if ( toMergeConformal && toCreatePolygons )
  {
    // do not merge quadrangles if polygons are OK (IPAL0052824)
    eIt[0] = eSide[0].begin();
    eIt[1] = eSide[1].begin();
    bool allQuads[2] = { true, true };
    for ( int iBord = 0; iBord < 2; iBord++ ) { // loop on 2 borders
      for ( ; allQuads[iBord] && eIt[iBord] != eSide[iBord].end(); ++eIt[iBord] )
        allQuads[iBord] = ( (*eIt[iBord])->NbCornerNodes() == 4 );
    }
    toMergeConformal = ( !allQuads[0] && !allQuads[1] );
  }

  TListOfListOfNodes nodeGroupsToMerge;
  if (( toMergeConformal ) ||
      ( theSideIsFreeBorder && !theSideThirdNode )) {

    // all nodes are to be merged

    for (nIt[0] = nSide[0].begin(), nIt[1] = nSide[1].begin();
         nIt[0] != nSide[0].end() && nIt[1] != nSide[1].end();
         nIt[0]++, nIt[1]++ )
    {
      nodeGroupsToMerge.push_back( list<const SMDS_MeshNode*>() );
      nodeGroupsToMerge.back().push_back( *nIt[1] ); // to keep
      nodeGroupsToMerge.back().push_back( *nIt[0] ); // to remove
    }
  }
  else {

    // insert new nodes into the border and the side to get equal nb of segments

    // get normalized parameters of nodes on the borders
    vector< double > param[ 2 ];
    param[0].resize( maxNbNodes );
    param[1].resize( maxNbNodes );
    int iNode, iBord;
    for ( iBord = 0; iBord < 2; iBord++ ) { // loop on 2 borders
      list< const SMDS_MeshNode* >& nodes = nSide[ iBord ];
      list< const SMDS_MeshNode* >::iterator nIt = nodes.begin();
      const SMDS_MeshNode* nPrev = *nIt;
      double bordLength = 0;
      for ( iNode = 0; nIt != nodes.end(); nIt++, iNode++ ) { // loop on border nodes
        const SMDS_MeshNode* nCur = *nIt;
        gp_XYZ segment (nCur->X() - nPrev->X(),
                        nCur->Y() - nPrev->Y(),
                        nCur->Z() - nPrev->Z());
        double segmentLen = segment.Modulus();
        bordLength += segmentLen;
        param[ iBord ][ iNode ] = bordLength;
        nPrev = nCur;
      }
      // normalize within [0,1]
      for ( iNode = 0; iNode < nbNodes[ iBord ]; iNode++ ) {
        param[ iBord ][ iNode ] /= bordLength;
      }
    }

    // loop on border segments
    const SMDS_MeshNode *nPrev[ 2 ] = { 0, 0 };
    int i[ 2 ] = { 0, 0 };
    nIt[0] = nSide[0].begin(); eIt[0] = eSide[0].begin();
    nIt[1] = nSide[1].begin(); eIt[1] = eSide[1].begin();

    TElemOfNodeListMap insertMap;
    TElemOfNodeListMap::iterator insertMapIt;
    // insertMap is
    // key:   elem to insert nodes into
    // value: 2 nodes to insert between + nodes to be inserted
    do {
      bool next[ 2 ] = { false, false };

      // find min adjacent segment length after sewing
      double nextParam = 10., prevParam = 0;
      for ( iBord = 0; iBord < 2; iBord++ ) { // loop on 2 borders
        if ( i[ iBord ] + 1 < nbNodes[ iBord ])
          nextParam = Min( nextParam, param[iBord][ i[iBord] + 1 ]);
        if ( i[ iBord ] > 0 )
          prevParam = Max( prevParam, param[iBord][ i[iBord] - 1 ]);
      }
      double  minParam = Min( param[ 0 ][ i[0] ], param[ 1 ][ i[1] ]);
      double  maxParam = Max( param[ 0 ][ i[0] ], param[ 1 ][ i[1] ]);
      double minSegLen = Min( nextParam - minParam, maxParam - prevParam );

      // choose to insert or to merge nodes
      double du = param[ 1 ][ i[1] ] - param[ 0 ][ i[0] ];
      if ( Abs( du ) <= minSegLen * 0.2 ) {
        // merge
        // ------
        nodeGroupsToMerge.push_back( list<const SMDS_MeshNode*>() );
        const SMDS_MeshNode* n0 = *nIt[0];
        const SMDS_MeshNode* n1 = *nIt[1];
        nodeGroupsToMerge.back().push_back( n1 );
        nodeGroupsToMerge.back().push_back( n0 );
        // position of node of the border changes due to merge
        param[ 0 ][ i[0] ] += du;
        // move n1 for the sake of elem shape evaluation during insertion.
        // n1 will be removed by MergeNodes() anyway
        const_cast<SMDS_MeshNode*>( n0 )->setXYZ( n1->X(), n1->Y(), n1->Z() );
        next[0] = next[1] = true;
      }
      else {
        // insert
        // ------
        int intoBord = ( du < 0 ) ? 0 : 1;
        const SMDS_MeshElement* elem = *eIt [ intoBord ];
        const SMDS_MeshNode*    n1   = nPrev[ intoBord ];
        const SMDS_MeshNode*    n2   = *nIt [ intoBord ];
        const SMDS_MeshNode*    nIns = *nIt [ 1 - intoBord ];
        if ( intoBord == 1 ) {
          // move node of the border to be on a link of elem of the side
          gp_XYZ p1 (n1->X(), n1->Y(), n1->Z());
          gp_XYZ p2 (n2->X(), n2->Y(), n2->Z());
          double ratio = du / ( param[ 1 ][ i[1] ] - param[ 1 ][ i[1]-1 ]);
          gp_XYZ p = p2 * ( 1 - ratio ) + p1 * ratio;
          GetMeshDS()->MoveNode( nIns, p.X(), p.Y(), p.Z() );
        }
        insertMapIt = insertMap.find( elem );
        bool  notFound = ( insertMapIt == insertMap.end() );
        bool otherLink = ( !notFound && (*insertMapIt).second.front() != n1 );
        if ( otherLink ) {
          // insert into another link of the same element:
          // 1. perform insertion into the other link of the elem
          list<const SMDS_MeshNode*> & nodeList = (*insertMapIt).second;
          const SMDS_MeshNode* n12 = nodeList.front(); nodeList.pop_front();
          const SMDS_MeshNode* n22 = nodeList.front(); nodeList.pop_front();
          InsertNodesIntoLink( elem, n12, n22, nodeList, toCreatePolygons );
          // 2. perform insertion into the link of adjacent faces
          while ( const SMDS_MeshElement* adjElem = findAdjacentFace( n12, n22, elem )) {
            InsertNodesIntoLink( adjElem, n12, n22, nodeList, toCreatePolygons );
          }
          while ( const SMDS_MeshElement* seg = findSegment( n12, n22 )) {
            InsertNodesIntoLink( seg, n12, n22, nodeList );
          }
          if (toCreatePolyedrs) {
            // perform insertion into the links of adjacent volumes
            UpdateVolumes(n12, n22, nodeList);
          }
          // 3. find an element appeared on n1 and n2 after the insertion
          insertMap.erase( elem );
          elem = findAdjacentFace( n1, n2, 0 );
        }
        if ( notFound || otherLink ) {
          // add element and nodes of the side into the insertMap
          insertMapIt = insertMap.insert( make_pair( elem, list<const SMDS_MeshNode*>() )).first;
          (*insertMapIt).second.push_back( n1 );
          (*insertMapIt).second.push_back( n2 );
        }
        // add node to be inserted into elem
        (*insertMapIt).second.push_back( nIns );
        next[ 1 - intoBord ] = true;
      }

      // go to the next segment
      for ( iBord = 0; iBord < 2; iBord++ ) { // loop on 2 borders
        if ( next[ iBord ] ) {
          if ( i[ iBord ] != 0 && eIt[ iBord ] != eSide[ iBord ].end())
            eIt[ iBord ]++;
          nPrev[ iBord ] = *nIt[ iBord ];
          nIt[ iBord ]++; i[ iBord ]++;
        }
      }
    }
    while ( nIt[0] != nSide[0].end() && nIt[1] != nSide[1].end());

    // perform insertion of nodes into elements

    for (insertMapIt = insertMap.begin();
         insertMapIt != insertMap.end();
         insertMapIt++ )
    {
      const SMDS_MeshElement* elem = (*insertMapIt).first;
      list<const SMDS_MeshNode*> & nodeList = (*insertMapIt).second;
      const SMDS_MeshNode* n1 = nodeList.front(); nodeList.pop_front();
      const SMDS_MeshNode* n2 = nodeList.front(); nodeList.pop_front();

      InsertNodesIntoLink( elem, n1, n2, nodeList, toCreatePolygons );

      while ( const SMDS_MeshElement* seg = findSegment( n1, n2 )) {
        InsertNodesIntoLink( seg, n1, n2, nodeList );
      }

      if ( !theSideIsFreeBorder ) {
        // look for and insert nodes into the faces adjacent to elem
        while ( const SMDS_MeshElement* adjElem = findAdjacentFace( n1, n2, elem )) {
          InsertNodesIntoLink( adjElem, n1, n2, nodeList, toCreatePolygons );
        }
      }
      if (toCreatePolyedrs) {
        // perform insertion into the links of adjacent volumes
        UpdateVolumes(n1, n2, nodeList);
      }
    }
  } // end: insert new nodes

  MergeNodes ( nodeGroupsToMerge );


  // Remove coincident segments

  // get new segments
  TIDSortedElemSet segments;
  SMESH_SequenceOfElemPtr newFaces;
  for ( int i = 1; i <= myLastCreatedElems.Length(); ++i )
  {
    if ( !myLastCreatedElems(i) ) continue;
    if ( myLastCreatedElems(i)->GetType() == SMDSAbs_Edge )
      segments.insert( segments.end(), myLastCreatedElems(i) );
    else
      newFaces.Append( myLastCreatedElems(i) );
  }
  // get segments adjacent to merged nodes
  TListOfListOfNodes::iterator groupIt = nodeGroupsToMerge.begin();
  for ( ; groupIt != nodeGroupsToMerge.end(); groupIt++ )
  {
    const list<const SMDS_MeshNode*>& nodes = *groupIt;
    SMDS_ElemIteratorPtr segIt = nodes.front()->GetInverseElementIterator( SMDSAbs_Edge );
    while ( segIt->more() )
      segments.insert( segIt->next() );
  }

  // find coincident
  TListOfListOfElementsID equalGroups;
  if ( !segments.empty() )
    FindEqualElements( segments, equalGroups );
  if ( !equalGroups.empty() )
  {
    // remove from segments those that will be removed
    TListOfListOfElementsID::iterator itGroups = equalGroups.begin();
    for ( ; itGroups != equalGroups.end(); ++itGroups )
    {
      list< int >& group = *itGroups;
      list< int >::iterator id = group.begin();
      for ( ++id; id != group.end(); ++id )
        if ( const SMDS_MeshElement* seg = GetMeshDS()->FindElement( *id ))
          segments.erase( seg );
    }
    // remove equal segments
    MergeElements( equalGroups );

    // restore myLastCreatedElems
    myLastCreatedElems = newFaces;
    TIDSortedElemSet::iterator seg = segments.begin();
    for ( ; seg != segments.end(); ++seg )
      myLastCreatedElems.Append( *seg );
  }

  return aResult;
}

//=======================================================================
//function : InsertNodesIntoLink
//purpose  : insert theNodesToInsert into theElement between theBetweenNode1
//           and theBetweenNode2 and split theElement
//=======================================================================

void SMESH_MeshEditor::InsertNodesIntoLink(const SMDS_MeshElement*     theElement,
                                           const SMDS_MeshNode*        theBetweenNode1,
                                           const SMDS_MeshNode*        theBetweenNode2,
                                           list<const SMDS_MeshNode*>& theNodesToInsert,
                                           const bool                  toCreatePoly)
{
  if ( !theElement ) return;

  SMESHDS_Mesh *aMesh = GetMeshDS();
  vector<const SMDS_MeshElement*> newElems;

  if ( theElement->GetType() == SMDSAbs_Edge )
  {
    theNodesToInsert.push_front( theBetweenNode1 );
    theNodesToInsert.push_back ( theBetweenNode2 );
    list<const SMDS_MeshNode*>::iterator n = theNodesToInsert.begin();
    const SMDS_MeshNode* n1 = *n;
    for ( ++n; n != theNodesToInsert.end(); ++n )
    {
      const SMDS_MeshNode* n2 = *n;
      if ( const SMDS_MeshElement* seg = aMesh->FindEdge( n1, n2 ))
        AddToSameGroups( seg, theElement, aMesh );
      else
        newElems.push_back( aMesh->AddEdge ( n1, n2 ));
      n1 = n2;
    }
    theNodesToInsert.pop_front();
    theNodesToInsert.pop_back();

    if ( theElement->IsQuadratic() ) // add a not split part
    {
      vector<const SMDS_MeshNode*> nodes( theElement->begin_nodes(),
                                          theElement->end_nodes() );
      int iOther = 0, nbN = nodes.size();
      for ( ; iOther < nbN; ++iOther )
        if ( nodes[iOther] != theBetweenNode1 &&
             nodes[iOther] != theBetweenNode2 )
          break;
      if      ( iOther == 0 )
      {
        if ( const SMDS_MeshElement* seg = aMesh->FindEdge( nodes[0], nodes[1] ))
          AddToSameGroups( seg, theElement, aMesh );
        else
          newElems.push_back( aMesh->AddEdge ( nodes[0], nodes[1] ));
      }
      else if ( iOther == 2 )
      {
        if ( const SMDS_MeshElement* seg = aMesh->FindEdge( nodes[1], nodes[2] ))
          AddToSameGroups( seg, theElement, aMesh );
        else
          newElems.push_back( aMesh->AddEdge ( nodes[1], nodes[2] ));
      }
    }
    // treat new elements
    for ( size_t i = 0; i < newElems.size(); ++i )
      if ( newElems[i] )
      {
        aMesh->SetMeshElementOnShape( newElems[i], theElement->getshapeId() );
        myLastCreatedElems.Append( newElems[i] );
      }
    ReplaceElemInGroups( theElement, newElems, aMesh );
    aMesh->RemoveElement( theElement );
    return;

  } // if ( theElement->GetType() == SMDSAbs_Edge )

  const SMDS_MeshElement* theFace = theElement;
  if ( theFace->GetType() != SMDSAbs_Face ) return;

  // find indices of 2 link nodes and of the rest nodes
  int iNode = 0, il1, il2, i3, i4;
  il1 = il2 = i3 = i4 = -1;
  vector<const SMDS_MeshNode*> nodes( theFace->NbNodes() );

  SMDS_NodeIteratorPtr nodeIt = theFace->interlacedNodesIterator();
  while ( nodeIt->more() ) {
    const SMDS_MeshNode* n = nodeIt->next();
    if ( n == theBetweenNode1 )
      il1 = iNode;
    else if ( n == theBetweenNode2 )
      il2 = iNode;
    else if ( i3 < 0 )
      i3 = iNode;
    else
      i4 = iNode;
    nodes[ iNode++ ] = n;
  }
  if ( il1 < 0 || il2 < 0 || i3 < 0 )
    return ;

  // arrange link nodes to go one after another regarding the face orientation
  bool reverse = ( Abs( il2 - il1 ) == 1 ? il2 < il1 : il1 < il2 );
  list<const SMDS_MeshNode *> aNodesToInsert = theNodesToInsert;
  if ( reverse ) {
    iNode = il1;
    il1 = il2;
    il2 = iNode;
    aNodesToInsert.reverse();
  }
  // check that not link nodes of a quadrangles are in good order
  int nbFaceNodes = theFace->NbNodes();
  if ( nbFaceNodes == 4 && i4 - i3 != 1 ) {
    iNode = i3;
    i3 = i4;
    i4 = iNode;
  }

  if (toCreatePoly || theFace->IsPoly()) {

    iNode = 0;
    vector<const SMDS_MeshNode *> poly_nodes (nbFaceNodes + aNodesToInsert.size());

    // add nodes of face up to first node of link
    bool isFLN = false;

    if ( theFace->IsQuadratic() ) {
      const SMDS_VtkFace* F = dynamic_cast<const SMDS_VtkFace*>(theFace);
      if (!F) throw SALOME_Exception(LOCALIZED("not an SMDS_VtkFace"));
      // use special nodes iterator
      SMDS_ElemIteratorPtr anIter = F->interlacedNodesElemIterator();
      while( anIter->more()  && !isFLN ) {
        const SMDS_MeshNode* n = cast2Node(anIter->next());
        poly_nodes[iNode++] = n;
        if (n == nodes[il1]) {
          isFLN = true;
        }
      }
      // add nodes to insert
      list<const SMDS_MeshNode*>::iterator nIt = aNodesToInsert.begin();
      for (; nIt != aNodesToInsert.end(); nIt++) {
        poly_nodes[iNode++] = *nIt;
      }
      // add nodes of face starting from last node of link
      while ( anIter->more() ) {
        poly_nodes[iNode++] = cast2Node(anIter->next());
      }
    }
    else {
      SMDS_ElemIteratorPtr nodeIt = theFace->nodesIterator();
      while ( nodeIt->more() && !isFLN ) {
        const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
        poly_nodes[iNode++] = n;
        if (n == nodes[il1]) {
          isFLN = true;
        }
      }
      // add nodes to insert
      list<const SMDS_MeshNode*>::iterator nIt = aNodesToInsert.begin();
      for (; nIt != aNodesToInsert.end(); nIt++) {
        poly_nodes[iNode++] = *nIt;
      }
      // add nodes of face starting from last node of link
      while ( nodeIt->more() ) {
        const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
        poly_nodes[iNode++] = n;
      }
    }

    // make a new face
    newElems.push_back( aMesh->AddPolygonalFace( poly_nodes ));
  }

  else if ( !theFace->IsQuadratic() )
  {
    // put aNodesToInsert between theBetweenNode1 and theBetweenNode2
    int nbLinkNodes = 2 + aNodesToInsert.size();
    //const SMDS_MeshNode* linkNodes[ nbLinkNodes ];
    vector<const SMDS_MeshNode*> linkNodes( nbLinkNodes );
    linkNodes[ 0 ] = nodes[ il1 ];
    linkNodes[ nbLinkNodes - 1 ] = nodes[ il2 ];
    list<const SMDS_MeshNode*>::iterator nIt = aNodesToInsert.begin();
    for ( iNode = 1; nIt != aNodesToInsert.end(); nIt++ ) {
      linkNodes[ iNode++ ] = *nIt;
    }
    // decide how to split a quadrangle: compare possible variants
    // and choose which of splits to be a quadrangle
    int i1, i2, iSplit, nbSplits = nbLinkNodes - 1, iBestQuad;
    if ( nbFaceNodes == 3 ) {
      iBestQuad = nbSplits;
      i4 = i3;
    }
    else if ( nbFaceNodes == 4 ) {
      SMESH::Controls::NumericalFunctorPtr aCrit( new SMESH::Controls::AspectRatio);
      double aBestRate = DBL_MAX;
      for ( int iQuad = 0; iQuad < nbSplits; iQuad++ ) {
        i1 = 0; i2 = 1;
        double aBadRate = 0;
        // evaluate elements quality
        for ( iSplit = 0; iSplit < nbSplits; iSplit++ ) {
          if ( iSplit == iQuad ) {
            SMDS_FaceOfNodes quad (linkNodes[ i1++ ],
                                   linkNodes[ i2++ ],
                                   nodes[ i3 ],
                                   nodes[ i4 ]);
            aBadRate += getBadRate( &quad, aCrit );
          }
          else {
            SMDS_FaceOfNodes tria (linkNodes[ i1++ ],
                                   linkNodes[ i2++ ],
                                   nodes[ iSplit < iQuad ? i4 : i3 ]);
            aBadRate += getBadRate( &tria, aCrit );
          }
        }
        // choice
        if ( aBadRate < aBestRate ) {
          iBestQuad = iQuad;
          aBestRate = aBadRate;
        }
      }
    }

    // create new elements
    i1 = 0; i2 = 1;
    for ( iSplit = 0; iSplit < nbSplits - 1; iSplit++ ) {
      SMDS_MeshElement* newElem = 0;
      if ( iSplit == iBestQuad )
        newElems.push_back( aMesh->AddFace (linkNodes[ i1++ ],
                                            linkNodes[ i2++ ],
                                            nodes[ i3 ],
                                            nodes[ i4 ]));
      else
        newElems.push_back( aMesh->AddFace (linkNodes[ i1++ ],
                                            linkNodes[ i2++ ],
                                            nodes[ iSplit < iBestQuad ? i4 : i3 ]));
    }

    const SMDS_MeshNode* newNodes[ 4 ];
    newNodes[ 0 ] = linkNodes[ i1 ];
    newNodes[ 1 ] = linkNodes[ i2 ];
    newNodes[ 2 ] = nodes[ iSplit >= iBestQuad ? i3 : i4 ];
    newNodes[ 3 ] = nodes[ i4 ];
    if (iSplit == iBestQuad)
      newElems.push_back( aMesh->AddFace( newNodes[0], newNodes[1], newNodes[2], newNodes[3] ));
    else
      newElems.push_back( aMesh->AddFace( newNodes[0], newNodes[1], newNodes[2] ));

  } // end if(!theFace->IsQuadratic())

  else { // theFace is quadratic
    // we have to split theFace on simple triangles and one simple quadrangle
    int tmp = il1/2;
    int nbshift = tmp*2;
    // shift nodes in nodes[] by nbshift
    int i,j;
    for(i=0; i<nbshift; i++) {
      const SMDS_MeshNode* n = nodes[0];
      for(j=0; j<nbFaceNodes-1; j++) {
        nodes[j] = nodes[j+1];
      }
      nodes[nbFaceNodes-1] = n;
    }
    il1 = il1 - nbshift;
    // now have to insert nodes between n0 and n1 or n1 and n2 (see below)
    //   n0      n1     n2    n0      n1     n2
    //     +-----+-----+        +-----+-----+
    //      \         /         |           |
    //       \       /          |           |
    //      n5+     +n3       n7+           +n3
    //         \   /            |           |
    //          \ /             |           |
    //           +              +-----+-----+
    //           n4           n6      n5     n4

    // create new elements
    int n1,n2,n3;
    if ( nbFaceNodes == 6 ) { // quadratic triangle
      newElems.push_back( aMesh->AddFace( nodes[3], nodes[4], nodes[5] ));
      if ( theFace->IsMediumNode(nodes[il1]) ) {
        // create quadrangle
        newElems.push_back( aMesh->AddFace( nodes[0], nodes[1], nodes[3], nodes[5] ));
        n1 = 1;
        n2 = 2;
        n3 = 3;
      }
      else {
        // create quadrangle
        newElems.push_back( aMesh->AddFace( nodes[1], nodes[2], nodes[3], nodes[5] ));
        n1 = 0;
        n2 = 1;
        n3 = 5;
      }
    }
    else { // nbFaceNodes==8 - quadratic quadrangle
      newElems.push_back( aMesh->AddFace( nodes[3], nodes[4], nodes[5] ));
      newElems.push_back( aMesh->AddFace( nodes[5], nodes[6], nodes[7] ));
      newElems.push_back( aMesh->AddFace( nodes[5], nodes[7], nodes[3] ));
      if ( theFace->IsMediumNode( nodes[ il1 ])) {
        // create quadrangle
        newElems.push_back( aMesh->AddFace( nodes[0], nodes[1], nodes[3], nodes[7] ));
        n1 = 1;
        n2 = 2;
        n3 = 3;
      }
      else {
        // create quadrangle
        newElems.push_back( aMesh->AddFace( nodes[1], nodes[2], nodes[3], nodes[7] ));
        n1 = 0;
        n2 = 1;
        n3 = 7;
      }
    }
    // create needed triangles using n1,n2,n3 and inserted nodes
    int nbn = 2 + aNodesToInsert.size();
    vector<const SMDS_MeshNode*> aNodes(nbn);
    aNodes[0    ] = nodes[n1];
    aNodes[nbn-1] = nodes[n2];
    list<const SMDS_MeshNode*>::iterator nIt = aNodesToInsert.begin();
    for ( iNode = 1; nIt != aNodesToInsert.end(); nIt++ ) {
      aNodes[iNode++] = *nIt;
    }
    for ( i = 1; i < nbn; i++ )
      newElems.push_back( aMesh->AddFace( aNodes[i-1], aNodes[i], nodes[n3] ));
  }

  // remove the old face
  for ( size_t i = 0; i < newElems.size(); ++i )
    if ( newElems[i] )
    {
      aMesh->SetMeshElementOnShape( newElems[i], theFace->getshapeId() );
      myLastCreatedElems.Append( newElems[i] );
    }
  ReplaceElemInGroups( theFace, newElems, aMesh );
  aMesh->RemoveElement(theFace);

} // InsertNodesIntoLink()

//=======================================================================
//function : UpdateVolumes
//purpose  :
//=======================================================================

void SMESH_MeshEditor::UpdateVolumes (const SMDS_MeshNode*        theBetweenNode1,
                                      const SMDS_MeshNode*        theBetweenNode2,
                                      list<const SMDS_MeshNode*>& theNodesToInsert)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  SMDS_ElemIteratorPtr invElemIt = theBetweenNode1->GetInverseElementIterator(SMDSAbs_Volume);
  while (invElemIt->more()) { // loop on inverse elements of theBetweenNode1
    const SMDS_MeshElement* elem = invElemIt->next();

    // check, if current volume has link theBetweenNode1 - theBetweenNode2
    SMDS_VolumeTool aVolume (elem);
    if (!aVolume.IsLinked(theBetweenNode1, theBetweenNode2))
      continue;

    // insert new nodes in all faces of the volume, sharing link theBetweenNode1 - theBetweenNode2
    int iface, nbFaces = aVolume.NbFaces();
    vector<const SMDS_MeshNode *> poly_nodes;
    vector<int> quantities (nbFaces);

    for (iface = 0; iface < nbFaces; iface++) {
      int nbFaceNodes = aVolume.NbFaceNodes(iface), nbInserted = 0;
      // faceNodes will contain (nbFaceNodes + 1) nodes, last = first
      const SMDS_MeshNode** faceNodes = aVolume.GetFaceNodes(iface);

      for (int inode = 0; inode < nbFaceNodes; inode++) {
        poly_nodes.push_back(faceNodes[inode]);

        if (nbInserted == 0) {
          if (faceNodes[inode] == theBetweenNode1) {
            if (faceNodes[inode + 1] == theBetweenNode2) {
              nbInserted = theNodesToInsert.size();

              // add nodes to insert
              list<const SMDS_MeshNode*>::iterator nIt = theNodesToInsert.begin();
              for (; nIt != theNodesToInsert.end(); nIt++) {
                poly_nodes.push_back(*nIt);
              }
            }
          }
          else if (faceNodes[inode] == theBetweenNode2) {
            if (faceNodes[inode + 1] == theBetweenNode1) {
              nbInserted = theNodesToInsert.size();

              // add nodes to insert in reversed order
              list<const SMDS_MeshNode*>::iterator nIt = theNodesToInsert.end();
              nIt--;
              for (; nIt != theNodesToInsert.begin(); nIt--) {
                poly_nodes.push_back(*nIt);
              }
              poly_nodes.push_back(*nIt);
            }
          }
          else {
          }
        }
      }
      quantities[iface] = nbFaceNodes + nbInserted;
    }

    // Replace the volume
    SMESHDS_Mesh *aMesh = GetMeshDS();

    if ( SMDS_MeshElement* newElem = aMesh->AddPolyhedralVolume( poly_nodes, quantities ))
    {
      aMesh->SetMeshElementOnShape( newElem, elem->getshapeId() );
      myLastCreatedElems.Append( newElem );
      ReplaceElemInGroups( elem, newElem, aMesh );
    }
    aMesh->RemoveElement( elem );
  }
}

namespace
{
  //================================================================================
  /*!
   * \brief Transform any volume into data of SMDSEntity_Polyhedra
   */
  //================================================================================

  void volumeToPolyhedron( const SMDS_MeshElement*         elem,
                           vector<const SMDS_MeshNode *> & nodes,
                           vector<int> &                   nbNodeInFaces )
  {
    nodes.clear();
    nbNodeInFaces.clear();
    SMDS_VolumeTool vTool ( elem );
    for ( int iF = 0; iF < vTool.NbFaces(); ++iF )
    {
      const SMDS_MeshNode** fNodes = vTool.GetFaceNodes( iF );
      nodes.insert( nodes.end(), fNodes, fNodes + vTool.NbFaceNodes( iF ));
      nbNodeInFaces.push_back( vTool.NbFaceNodes( iF ));
    }
  }
}

//=======================================================================
/*!
 * \brief Convert elements contained in a sub-mesh to quadratic
 * \return int - nb of checked elements
 */
//=======================================================================

int SMESH_MeshEditor::convertElemToQuadratic(SMESHDS_SubMesh *   theSm,
                                             SMESH_MesherHelper& theHelper,
                                             const bool          theForce3d)
{
  int nbElem = 0;
  if( !theSm ) return nbElem;

  vector<int> nbNodeInFaces;
  vector<const SMDS_MeshNode *> nodes;
  SMDS_ElemIteratorPtr ElemItr = theSm->GetElements();
  while(ElemItr->more())
  {
    nbElem++;
    const SMDS_MeshElement* elem = ElemItr->next();
    if( !elem ) continue;

    // analyse a necessity of conversion
    const SMDSAbs_ElementType aType = elem->GetType();
    if ( aType < SMDSAbs_Edge || aType > SMDSAbs_Volume )
      continue;
    const SMDSAbs_EntityType aGeomType = elem->GetEntityType();
    bool hasCentralNodes = false;
    if ( elem->IsQuadratic() )
    {
      bool alreadyOK;
      switch ( aGeomType ) {
      case SMDSEntity_Quad_Triangle:
      case SMDSEntity_Quad_Quadrangle:
      case SMDSEntity_Quad_Hexa:
        alreadyOK = !theHelper.GetIsBiQuadratic(); break;

      case SMDSEntity_BiQuad_Triangle:
      case SMDSEntity_BiQuad_Quadrangle:
      case SMDSEntity_TriQuad_Hexa:
        alreadyOK = theHelper.GetIsBiQuadratic();
        hasCentralNodes = true;
        break;
      default:
        alreadyOK = true;
      }
      // take into account already present modium nodes
      switch ( aType ) {
      case SMDSAbs_Volume:
        theHelper.AddTLinks( static_cast< const SMDS_MeshVolume* >( elem )); break;
      case SMDSAbs_Face:
        theHelper.AddTLinks( static_cast< const SMDS_MeshFace* >( elem )); break;
      case SMDSAbs_Edge:
        theHelper.AddTLinks( static_cast< const SMDS_MeshEdge* >( elem )); break;
      default:;
      }
      if ( alreadyOK )
        continue;
    }
    // get elem data needed to re-create it
    //
    const int id      = elem->GetID();
    const int nbNodes = elem->NbCornerNodes();
    nodes.assign(elem->begin_nodes(), elem->end_nodes());
    if ( aGeomType == SMDSEntity_Polyhedra )
      nbNodeInFaces = static_cast<const SMDS_VtkVolume* >( elem )->GetQuantities();
    else if ( aGeomType == SMDSEntity_Hexagonal_Prism )
      volumeToPolyhedron( elem, nodes, nbNodeInFaces );

    // remove a linear element
    GetMeshDS()->RemoveFreeElement(elem, theSm, /*fromGroups=*/false);

    // remove central nodes of biquadratic elements (biquad->quad convertion)
    if ( hasCentralNodes )
      for ( size_t i = nbNodes * 2; i < nodes.size(); ++i )
        if ( nodes[i]->NbInverseElements() == 0 )
          GetMeshDS()->RemoveFreeNode( nodes[i], theSm, /*fromGroups=*/true );

    const SMDS_MeshElement* NewElem = 0;

    switch( aType )
    {
    case SMDSAbs_Edge :
      {
        NewElem = theHelper.AddEdge(nodes[0], nodes[1], id, theForce3d);
        break;
      }
    case SMDSAbs_Face :
      {
        switch(nbNodes)
        {
        case 3:
          NewElem = theHelper.AddFace(nodes[0], nodes[1], nodes[2], id, theForce3d);
          break;
        case 4:
          NewElem = theHelper.AddFace(nodes[0], nodes[1], nodes[2], nodes[3], id, theForce3d);
          break;
        default:
          NewElem = theHelper.AddPolygonalFace(nodes, id, theForce3d);
        }
        break;
      }
    case SMDSAbs_Volume :
      {
        switch( aGeomType )
        {
        case SMDSEntity_Tetra:
          NewElem = theHelper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3], id, theForce3d);
          break;
        case SMDSEntity_Pyramid:
          NewElem = theHelper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], id, theForce3d);
          break;
        case SMDSEntity_Penta:
          NewElem = theHelper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], nodes[5], id, theForce3d);
          break;
        case SMDSEntity_Hexa:
        case SMDSEntity_Quad_Hexa:
        case SMDSEntity_TriQuad_Hexa:
          NewElem = theHelper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3],
                                        nodes[4], nodes[5], nodes[6], nodes[7], id, theForce3d);
          break;
        case SMDSEntity_Hexagonal_Prism:
        default:
          NewElem = theHelper.AddPolyhedralVolume(nodes, nbNodeInFaces, id, theForce3d);
        }
        break;
      }
    default :
      continue;
    }
    ReplaceElemInGroups( elem, NewElem, GetMeshDS());
    if( NewElem && NewElem->getshapeId() < 1 )
      theSm->AddElement( NewElem );
  }
  return nbElem;
}
//=======================================================================
//function : ConvertToQuadratic
//purpose  :
//=======================================================================

void SMESH_MeshEditor::ConvertToQuadratic(const bool theForce3d, const bool theToBiQuad)
{
  SMESHDS_Mesh* meshDS = GetMeshDS();

  SMESH_MesherHelper aHelper(*myMesh);

  aHelper.SetIsQuadratic( true );
  aHelper.SetIsBiQuadratic( theToBiQuad );
  aHelper.SetElementsOnShape(true);
  aHelper.ToFixNodeParameters( true );

  // convert elements assigned to sub-meshes
  int nbCheckedElems = 0;
  if ( myMesh->HasShapeToMesh() )
  {
    if ( SMESH_subMesh *aSubMesh = myMesh->GetSubMeshContaining(myMesh->GetShapeToMesh()))
    {
      SMESH_subMeshIteratorPtr smIt = aSubMesh->getDependsOnIterator(true,false);
      while ( smIt->more() ) {
        SMESH_subMesh* sm = smIt->next();
        if ( SMESHDS_SubMesh *smDS = sm->GetSubMeshDS() ) {
          aHelper.SetSubShape( sm->GetSubShape() );
          nbCheckedElems += convertElemToQuadratic(smDS, aHelper, theForce3d);
        }
      }
    }
  }

  // convert elements NOT assigned to sub-meshes
  int totalNbElems = meshDS->NbEdges() + meshDS->NbFaces() + meshDS->NbVolumes();
  if ( nbCheckedElems < totalNbElems ) // not all elements are in sub-meshes
  {
    aHelper.SetElementsOnShape(false);
    SMESHDS_SubMesh *smDS = 0;

    // convert edges
    SMDS_EdgeIteratorPtr aEdgeItr = meshDS->edgesIterator();
    while( aEdgeItr->more() )
    {
      const SMDS_MeshEdge* edge = aEdgeItr->next();
      if ( !edge->IsQuadratic() )
      {
        int                  id = edge->GetID();
        const SMDS_MeshNode* n1 = edge->GetNode(0);
        const SMDS_MeshNode* n2 = edge->GetNode(1);

        meshDS->RemoveFreeElement(edge, smDS, /*fromGroups=*/false);

        const SMDS_MeshEdge* NewEdge = aHelper.AddEdge(n1, n2, id, theForce3d);
        ReplaceElemInGroups( edge, NewEdge, GetMeshDS());
      }
      else
      {
        aHelper.AddTLinks( static_cast< const SMDS_MeshEdge* >( edge ));
      }
    }

    // convert faces
    SMDS_FaceIteratorPtr aFaceItr = meshDS->facesIterator();
    while( aFaceItr->more() )
    {
      const SMDS_MeshFace* face = aFaceItr->next();
      if ( !face ) continue;
      
      const SMDSAbs_EntityType type = face->GetEntityType();
      bool alreadyOK;
      switch( type )
      {
      case SMDSEntity_Quad_Triangle:
      case SMDSEntity_Quad_Quadrangle:
        alreadyOK = !theToBiQuad;
        aHelper.AddTLinks( static_cast< const SMDS_MeshFace* >( face ));
        break;
      case SMDSEntity_BiQuad_Triangle:
      case SMDSEntity_BiQuad_Quadrangle:
        alreadyOK = theToBiQuad;
        aHelper.AddTLinks( static_cast< const SMDS_MeshFace* >( face ));
        break;
      default: alreadyOK = false;
      }
      if ( alreadyOK )
        continue;

      const int id = face->GetID();
      vector<const SMDS_MeshNode *> nodes ( face->begin_nodes(), face->end_nodes());

      meshDS->RemoveFreeElement(face, smDS, /*fromGroups=*/false);

      SMDS_MeshFace * NewFace = 0;
      switch( type )
      {
      case SMDSEntity_Triangle:
      case SMDSEntity_Quad_Triangle:
      case SMDSEntity_BiQuad_Triangle:
        NewFace = aHelper.AddFace(nodes[0], nodes[1], nodes[2], id, theForce3d);
        if ( nodes.size() == 7 && nodes[6]->NbInverseElements() == 0 ) // rm a central node
          GetMeshDS()->RemoveFreeNode( nodes[6], /*sm=*/0, /*fromGroups=*/true );
        break;

      case SMDSEntity_Quadrangle:
      case SMDSEntity_Quad_Quadrangle:
      case SMDSEntity_BiQuad_Quadrangle:
        NewFace = aHelper.AddFace(nodes[0], nodes[1], nodes[2], nodes[3], id, theForce3d);
        if ( nodes.size() == 9 && nodes[8]->NbInverseElements() == 0 ) // rm a central node
          GetMeshDS()->RemoveFreeNode( nodes[8], /*sm=*/0, /*fromGroups=*/true );
        break;

      default:;
        NewFace = aHelper.AddPolygonalFace(nodes, id, theForce3d);
      }
      ReplaceElemInGroups( face, NewFace, GetMeshDS());
    }

    // convert volumes
    vector<int> nbNodeInFaces;
    SMDS_VolumeIteratorPtr aVolumeItr = meshDS->volumesIterator();
    while(aVolumeItr->more())
    {
      const SMDS_MeshVolume* volume = aVolumeItr->next();
      if ( !volume ) continue;

      const SMDSAbs_EntityType type = volume->GetEntityType();
      if ( volume->IsQuadratic() )
      {
        bool alreadyOK;
        switch ( type )
        {
        case SMDSEntity_Quad_Hexa:    alreadyOK = !theToBiQuad; break;
        case SMDSEntity_TriQuad_Hexa: alreadyOK = theToBiQuad; break;
        default:                      alreadyOK = true;
        }
        if ( alreadyOK )
        {
          aHelper.AddTLinks( static_cast< const SMDS_MeshVolume* >( volume ));
          continue;
        }
      }
      const int id = volume->GetID();
      vector<const SMDS_MeshNode *> nodes (volume->begin_nodes(), volume->end_nodes());
      if ( type == SMDSEntity_Polyhedra )
        nbNodeInFaces = static_cast<const SMDS_VtkVolume* >(volume)->GetQuantities();
      else if ( type == SMDSEntity_Hexagonal_Prism )
        volumeToPolyhedron( volume, nodes, nbNodeInFaces );

      meshDS->RemoveFreeElement(volume, smDS, /*fromGroups=*/false);

      SMDS_MeshVolume * NewVolume = 0;
      switch ( type )
      {
      case SMDSEntity_Tetra:
        NewVolume = aHelper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3], id, theForce3d );
        break;
      case SMDSEntity_Hexa:
      case SMDSEntity_Quad_Hexa:
      case SMDSEntity_TriQuad_Hexa:
        NewVolume = aHelper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3],
                                      nodes[4], nodes[5], nodes[6], nodes[7], id, theForce3d);
        for ( size_t i = 20; i < nodes.size(); ++i ) // rm central nodes
          if ( nodes[i]->NbInverseElements() == 0 )
            GetMeshDS()->RemoveFreeNode( nodes[i], /*sm=*/0, /*fromGroups=*/true );
        break;
      case SMDSEntity_Pyramid:
        NewVolume = aHelper.AddVolume(nodes[0], nodes[1], nodes[2],
                                      nodes[3], nodes[4], id, theForce3d);
        break;
      case SMDSEntity_Penta:
        NewVolume = aHelper.AddVolume(nodes[0], nodes[1], nodes[2],
                                      nodes[3], nodes[4], nodes[5], id, theForce3d);
        break;
      case SMDSEntity_Hexagonal_Prism:
      default:
        NewVolume = aHelper.AddPolyhedralVolume(nodes, nbNodeInFaces, id, theForce3d);
      }
      ReplaceElemInGroups(volume, NewVolume, meshDS);
    }
  }

  if ( !theForce3d )
  { // setenv NO_FixQuadraticElements to know if FixQuadraticElements() is guilty of bad conversion
    // aHelper.SetSubShape(0); // apply FixQuadraticElements() to the whole mesh
    // aHelper.FixQuadraticElements(myError);
    SMESH_MesherHelper( *myMesh ).FixQuadraticElements(myError);
  }
}

//================================================================================
/*!
 * \brief Makes given elements quadratic
 *  \param theForce3d - if true, the medium nodes will be placed in the middle of link
 *  \param theElements - elements to make quadratic
 */
//================================================================================

void SMESH_MeshEditor::ConvertToQuadratic(const bool        theForce3d,
                                          TIDSortedElemSet& theElements,
                                          const bool        theToBiQuad)
{
  if ( theElements.empty() ) return;

  // we believe that all theElements are of the same type
  const SMDSAbs_ElementType elemType = (*theElements.begin())->GetType();

  // get all nodes shared by theElements
  TIDSortedNodeSet allNodes;
  TIDSortedElemSet::iterator eIt = theElements.begin();
  for ( ; eIt != theElements.end(); ++eIt )
    allNodes.insert( (*eIt)->begin_nodes(), (*eIt)->end_nodes() );

  // complete theElements with elements of lower dim whose all nodes are in allNodes

  TIDSortedElemSet quadAdjacentElems    [ SMDSAbs_NbElementTypes ]; // quadratic adjacent elements
  TIDSortedElemSet checkedAdjacentElems [ SMDSAbs_NbElementTypes ];
  TIDSortedNodeSet::iterator nIt = allNodes.begin();
  for ( ; nIt != allNodes.end(); ++nIt )
  {
    const SMDS_MeshNode* n = *nIt;
    SMDS_ElemIteratorPtr invIt = n->GetInverseElementIterator();
    while ( invIt->more() )
    {
      const SMDS_MeshElement*      e = invIt->next();
      const SMDSAbs_ElementType type = e->GetType();
      if ( e->IsQuadratic() )
      {
        quadAdjacentElems[ type ].insert( e );

        bool alreadyOK;
        switch ( e->GetEntityType() ) {
        case SMDSEntity_Quad_Triangle:
        case SMDSEntity_Quad_Quadrangle:
        case SMDSEntity_Quad_Hexa:         alreadyOK = !theToBiQuad; break;
        case SMDSEntity_BiQuad_Triangle:
        case SMDSEntity_BiQuad_Quadrangle:
        case SMDSEntity_TriQuad_Hexa:      alreadyOK = theToBiQuad; break;
        default:                           alreadyOK = true;
        }
        if ( alreadyOK )
          continue;
      }
      if ( type >= elemType )
        continue; // same type or more complex linear element

      if ( !checkedAdjacentElems[ type ].insert( e ).second )
        continue; // e is already checked

      // check nodes
      bool allIn = true;
      SMDS_NodeIteratorPtr nodeIt = e->nodeIterator();
      while ( nodeIt->more() && allIn )
        allIn = allNodes.count( nodeIt->next() );
      if ( allIn )
        theElements.insert(e );
    }
  }

  SMESH_MesherHelper helper(*myMesh);
  helper.SetIsQuadratic( true );
  helper.SetIsBiQuadratic( theToBiQuad );

  // add links of quadratic adjacent elements to the helper

  if ( !quadAdjacentElems[SMDSAbs_Edge].empty() )
    for ( eIt  = quadAdjacentElems[SMDSAbs_Edge].begin();
          eIt != quadAdjacentElems[SMDSAbs_Edge].end(); ++eIt )
    {
      helper.AddTLinks( static_cast< const SMDS_MeshEdge*> (*eIt) );
    }
  if ( !quadAdjacentElems[SMDSAbs_Face].empty() )
    for ( eIt  = quadAdjacentElems[SMDSAbs_Face].begin();
          eIt != quadAdjacentElems[SMDSAbs_Face].end(); ++eIt )
    {
      helper.AddTLinks( static_cast< const SMDS_MeshFace*> (*eIt) );
    }
  if ( !quadAdjacentElems[SMDSAbs_Volume].empty() )
    for ( eIt  = quadAdjacentElems[SMDSAbs_Volume].begin();
          eIt != quadAdjacentElems[SMDSAbs_Volume].end(); ++eIt )
    {
      helper.AddTLinks( static_cast< const SMDS_MeshVolume*> (*eIt) );
    }

  // make quadratic (or bi-tri-quadratic) elements instead of linear ones

  SMESHDS_Mesh*  meshDS = GetMeshDS();
  SMESHDS_SubMesh* smDS = 0;
  for ( eIt = theElements.begin(); eIt != theElements.end(); ++eIt )
  {
    const SMDS_MeshElement* elem = *eIt;

    bool alreadyOK;
    int nbCentralNodes = 0;
    switch ( elem->GetEntityType() ) {
      // linear convertible
    case SMDSEntity_Edge:
    case SMDSEntity_Triangle:
    case SMDSEntity_Quadrangle:
    case SMDSEntity_Tetra:
    case SMDSEntity_Pyramid:
    case SMDSEntity_Hexa:
    case SMDSEntity_Penta:             alreadyOK = false;       nbCentralNodes = 0; break;
      // quadratic that can become bi-quadratic
    case SMDSEntity_Quad_Triangle:
    case SMDSEntity_Quad_Quadrangle:
    case SMDSEntity_Quad_Hexa:         alreadyOK =!theToBiQuad; nbCentralNodes = 0; break;
      // bi-quadratic
    case SMDSEntity_BiQuad_Triangle:
    case SMDSEntity_BiQuad_Quadrangle: alreadyOK = theToBiQuad; nbCentralNodes = 1; break;
    case SMDSEntity_TriQuad_Hexa:      alreadyOK = theToBiQuad; nbCentralNodes = 7; break;
      // the rest
    default:                           alreadyOK = true;
    }
    if ( alreadyOK ) continue;

    const SMDSAbs_ElementType type = elem->GetType();
    const int                   id = elem->GetID();
    const int              nbNodes = elem->NbCornerNodes();
    vector<const SMDS_MeshNode *> nodes ( elem->begin_nodes(), elem->end_nodes());

    helper.SetSubShape( elem->getshapeId() );

    if ( !smDS || !smDS->Contains( elem ))
      smDS = meshDS->MeshElements( elem->getshapeId() );
    meshDS->RemoveFreeElement(elem, smDS, /*fromGroups=*/false);

    SMDS_MeshElement * newElem = 0;
    switch( nbNodes )
    {
    case 4: // cases for most frequently used element types go first (for optimization)
      if ( type == SMDSAbs_Volume )
        newElem = helper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3], id, theForce3d);
      else
        newElem = helper.AddFace  (nodes[0], nodes[1], nodes[2], nodes[3], id, theForce3d);
      break;
    case 8:
      newElem = helper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3],
                                 nodes[4], nodes[5], nodes[6], nodes[7], id, theForce3d);
      break;
    case 3:
      newElem = helper.AddFace  (nodes[0], nodes[1], nodes[2], id, theForce3d);
      break;
    case 2:
      newElem = helper.AddEdge(nodes[0], nodes[1], id, theForce3d);
      break;
    case 5:
      newElem = helper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3],
                                 nodes[4], id, theForce3d);
      break;
    case 6:
      newElem = helper.AddVolume(nodes[0], nodes[1], nodes[2], nodes[3],
                                 nodes[4], nodes[5], id, theForce3d);
      break;
    default:;
    }
    ReplaceElemInGroups( elem, newElem, meshDS);
    if( newElem && smDS )
      smDS->AddElement( newElem );

     // remove central nodes
    for ( size_t i = nodes.size() - nbCentralNodes; i < nodes.size(); ++i )
      if ( nodes[i]->NbInverseElements() == 0 )
        meshDS->RemoveFreeNode( nodes[i], smDS, /*fromGroups=*/true );

  } // loop on theElements

  if ( !theForce3d )
  { // setenv NO_FixQuadraticElements to know if FixQuadraticElements() is guilty of bad conversion
    // helper.SetSubShape(0); // apply FixQuadraticElements() to the whole mesh
    // helper.FixQuadraticElements( myError );
    SMESH_MesherHelper( *myMesh ).FixQuadraticElements(myError);
  }
}

//=======================================================================
/*!
 * \brief Convert quadratic elements to linear ones and remove quadratic nodes
 * \return int - nb of checked elements
 */
//=======================================================================

int SMESH_MeshEditor::removeQuadElem(SMESHDS_SubMesh *    theSm,
                                     SMDS_ElemIteratorPtr theItr,
                                     const int            theShapeID)
{
  int nbElem = 0;
  SMESHDS_Mesh* meshDS = GetMeshDS();
  ElemFeatures elemType;
  vector<const SMDS_MeshNode *> nodes;

  while( theItr->more() )
  {
    const SMDS_MeshElement* elem = theItr->next();
    nbElem++;
    if( elem && elem->IsQuadratic())
    {
      // get elem data
      int nbCornerNodes = elem->NbCornerNodes();
      nodes.assign( elem->begin_nodes(), elem->end_nodes() );

      elemType.Init( elem, /*basicOnly=*/false ).SetID( elem->GetID() ).SetQuad( false );

      //remove a quadratic element
      if ( !theSm || !theSm->Contains( elem ))
        theSm = meshDS->MeshElements( elem->getshapeId() );
      meshDS->RemoveFreeElement( elem, theSm, /*fromGroups=*/false );

      // remove medium nodes
      for ( size_t i = nbCornerNodes; i < nodes.size(); ++i )
        if ( nodes[i]->NbInverseElements() == 0 )
          meshDS->RemoveFreeNode( nodes[i], theSm );

      // add a linear element
      nodes.resize( nbCornerNodes );
      SMDS_MeshElement * newElem = AddElement( nodes, elemType );
      ReplaceElemInGroups(elem, newElem, meshDS);
      if( theSm && newElem )
        theSm->AddElement( newElem );
    }
  }
  return nbElem;
}

//=======================================================================
//function : ConvertFromQuadratic
//purpose  :
//=======================================================================

bool SMESH_MeshEditor::ConvertFromQuadratic()
{
  int nbCheckedElems = 0;
  if ( myMesh->HasShapeToMesh() )
  {
    if ( SMESH_subMesh *aSubMesh = myMesh->GetSubMeshContaining(myMesh->GetShapeToMesh()))
    {
      SMESH_subMeshIteratorPtr smIt = aSubMesh->getDependsOnIterator(true,false);
      while ( smIt->more() ) {
        SMESH_subMesh* sm = smIt->next();
        if ( SMESHDS_SubMesh *smDS = sm->GetSubMeshDS() )
          nbCheckedElems += removeQuadElem( smDS, smDS->GetElements(), sm->GetId() );
      }
    }
  }

  int totalNbElems =
    GetMeshDS()->NbEdges() + GetMeshDS()->NbFaces() + GetMeshDS()->NbVolumes();
  if ( nbCheckedElems < totalNbElems ) // not all elements are in submeshes
  {
    SMESHDS_SubMesh *aSM = 0;
    removeQuadElem( aSM, GetMeshDS()->elementsIterator(), 0 );
  }

  return true;
}

namespace
{
  //================================================================================
  /*!
   * \brief Return true if all medium nodes of the element are in the node set
   */
  //================================================================================

  bool allMediumNodesIn(const SMDS_MeshElement* elem, TIDSortedNodeSet& nodeSet )
  {
    for ( int i = elem->NbCornerNodes(); i < elem->NbNodes(); ++i )
      if ( !nodeSet.count( elem->GetNode(i) ))
        return false;
    return true;
  }
}

//================================================================================
/*!
 * \brief Makes given elements linear
 */
//================================================================================

void SMESH_MeshEditor::ConvertFromQuadratic(TIDSortedElemSet& theElements)
{
  if ( theElements.empty() ) return;

  // collect IDs of medium nodes of theElements; some of these nodes will be removed
  set<int> mediumNodeIDs;
  TIDSortedElemSet::iterator eIt = theElements.begin();
  for ( ; eIt != theElements.end(); ++eIt )
  {
    const SMDS_MeshElement* e = *eIt;
    for ( int i = e->NbCornerNodes(); i < e->NbNodes(); ++i )
      mediumNodeIDs.insert( e->GetNode(i)->GetID() );
  }

  // replace given elements by linear ones
  SMDS_ElemIteratorPtr elemIt = elemSetIterator( theElements );
  removeQuadElem( /*theSm=*/0, elemIt, /*theShapeID=*/0 );

  // we need to convert remaining elements whose all medium nodes are in mediumNodeIDs
  // except those elements sharing medium nodes of quadratic element whose medium nodes
  // are not all in mediumNodeIDs

  // get remaining medium nodes
  TIDSortedNodeSet mediumNodes;
  set<int>::iterator nIdsIt = mediumNodeIDs.begin();
  for ( ; nIdsIt != mediumNodeIDs.end(); ++nIdsIt )
    if ( const SMDS_MeshNode* n = GetMeshDS()->FindNode( *nIdsIt ))
      mediumNodes.insert( mediumNodes.end(), n );

  // find more quadratic elements to convert
  TIDSortedElemSet moreElemsToConvert;
  TIDSortedNodeSet::iterator nIt = mediumNodes.begin();
  for ( ; nIt != mediumNodes.end(); ++nIt )
  {
    SMDS_ElemIteratorPtr invIt = (*nIt)->GetInverseElementIterator();
    while ( invIt->more() )
    {
      const SMDS_MeshElement* e = invIt->next();
      if ( e->IsQuadratic() && allMediumNodesIn( e, mediumNodes ))
      {
        // find a more complex element including e and
        // whose medium nodes are not in mediumNodes
        bool complexFound = false;
        for ( int type = e->GetType() + 1; type < SMDSAbs_0DElement; ++type )
        {
          SMDS_ElemIteratorPtr invIt2 =
            (*nIt)->GetInverseElementIterator( SMDSAbs_ElementType( type ));
          while ( invIt2->more() )
          {
            const SMDS_MeshElement* eComplex = invIt2->next();
            if ( eComplex->IsQuadratic() && !allMediumNodesIn( eComplex, mediumNodes))
            {
              int nbCommonNodes = SMESH_MeshAlgos::GetCommonNodes( e, eComplex ).size();
              if ( nbCommonNodes == e->NbNodes())
              {
                complexFound = true;
                type = SMDSAbs_NbElementTypes; // to quit from the outer loop
                break;
              }
            }
          }
        }
        if ( !complexFound )
          moreElemsToConvert.insert( e );
      }
    }
  }
  elemIt = elemSetIterator( moreElemsToConvert );
  removeQuadElem( /*theSm=*/0, elemIt, /*theShapeID=*/0 );
}

//=======================================================================
//function : SewSideElements
//purpose  :
//=======================================================================

SMESH_MeshEditor::Sew_Error
SMESH_MeshEditor::SewSideElements (TIDSortedElemSet&    theSide1,
                                   TIDSortedElemSet&    theSide2,
                                   const SMDS_MeshNode* theFirstNode1,
                                   const SMDS_MeshNode* theFirstNode2,
                                   const SMDS_MeshNode* theSecondNode1,
                                   const SMDS_MeshNode* theSecondNode2)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  MESSAGE ("::::SewSideElements()");
  if ( theSide1.size() != theSide2.size() )
    return SEW_DIFF_NB_OF_ELEMENTS;

  Sew_Error aResult = SEW_OK;
  // Algo:
  // 1. Build set of faces representing each side
  // 2. Find which nodes of the side 1 to merge with ones on the side 2
  // 3. Replace nodes in elements of the side 1 and remove replaced nodes

  // =======================================================================
  // 1. Build set of faces representing each side:
  // =======================================================================
  // a. build set of nodes belonging to faces
  // b. complete set of faces: find missing faces whose nodes are in set of nodes
  // c. create temporary faces representing side of volumes if correspondent
  //    face does not exist

  SMESHDS_Mesh* aMesh = GetMeshDS();
  // TODO algoritm not OK with vtkUnstructuredGrid: 2 meshes can't share nodes
  //SMDS_Mesh aTmpFacesMesh; // try to use the same mesh
  TIDSortedElemSet             faceSet1, faceSet2;
  set<const SMDS_MeshElement*> volSet1,  volSet2;
  set<const SMDS_MeshNode*>    nodeSet1, nodeSet2;
  TIDSortedElemSet             * faceSetPtr[] = { &faceSet1, &faceSet2 };
  set<const SMDS_MeshElement*> *  volSetPtr[] = { &volSet1,  &volSet2  };
  set<const SMDS_MeshNode*>    * nodeSetPtr[] = { &nodeSet1, &nodeSet2 };
  TIDSortedElemSet             * elemSetPtr[] = { &theSide1, &theSide2 };
  int iSide, iFace, iNode;

  list<const SMDS_MeshElement* > tempFaceList;
  for ( iSide = 0; iSide < 2; iSide++ ) {
    set<const SMDS_MeshNode*>    * nodeSet = nodeSetPtr[ iSide ];
    TIDSortedElemSet             * elemSet = elemSetPtr[ iSide ];
    TIDSortedElemSet             * faceSet = faceSetPtr[ iSide ];
    set<const SMDS_MeshElement*> * volSet  = volSetPtr [ iSide ];
    set<const SMDS_MeshElement*>::iterator vIt;
    TIDSortedElemSet::iterator eIt;
    set<const SMDS_MeshNode*>::iterator    nIt;

    // check that given nodes belong to given elements
    const SMDS_MeshNode* n1 = ( iSide == 0 ) ? theFirstNode1 : theFirstNode2;
    const SMDS_MeshNode* n2 = ( iSide == 0 ) ? theSecondNode1 : theSecondNode2;
    int firstIndex = -1, secondIndex = -1;
    for (eIt = elemSet->begin(); eIt != elemSet->end(); eIt++ ) {
      const SMDS_MeshElement* elem = *eIt;
      if ( firstIndex  < 0 ) firstIndex  = elem->GetNodeIndex( n1 );
      if ( secondIndex < 0 ) secondIndex = elem->GetNodeIndex( n2 );
      if ( firstIndex > -1 && secondIndex > -1 ) break;
    }
    if ( firstIndex < 0 || secondIndex < 0 ) {
      // we can simply return until temporary faces created
      return (iSide == 0 ) ? SEW_BAD_SIDE1_NODES : SEW_BAD_SIDE2_NODES;
    }

    // -----------------------------------------------------------
    // 1a. Collect nodes of existing faces
    //     and build set of face nodes in order to detect missing
    //     faces corresponding to sides of volumes
    // -----------------------------------------------------------

    set< set <const SMDS_MeshNode*> > setOfFaceNodeSet;

    // loop on the given element of a side
    for (eIt = elemSet->begin(); eIt != elemSet->end(); eIt++ ) {
      //const SMDS_MeshElement* elem = *eIt;
      const SMDS_MeshElement* elem = *eIt;
      if ( elem->GetType() == SMDSAbs_Face ) {
        faceSet->insert( elem );
        set <const SMDS_MeshNode*> faceNodeSet;
        SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
        while ( nodeIt->more() ) {
          const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
          nodeSet->insert( n );
          faceNodeSet.insert( n );
        }
        setOfFaceNodeSet.insert( faceNodeSet );
      }
      else if ( elem->GetType() == SMDSAbs_Volume )
        volSet->insert( elem );
    }
    // ------------------------------------------------------------------------------
    // 1b. Complete set of faces: find missing faces whose nodes are in set of nodes
    // ------------------------------------------------------------------------------

    for ( nIt = nodeSet->begin(); nIt != nodeSet->end(); nIt++ ) { // loop on nodes of iSide
      SMDS_ElemIteratorPtr fIt = (*nIt)->GetInverseElementIterator(SMDSAbs_Face);
      while ( fIt->more() ) { // loop on faces sharing a node
        const SMDS_MeshElement* f = fIt->next();
        if ( faceSet->find( f ) == faceSet->end() ) {
          // check if all nodes are in nodeSet and
          // complete setOfFaceNodeSet if they are
          set <const SMDS_MeshNode*> faceNodeSet;
          SMDS_ElemIteratorPtr nodeIt = f->nodesIterator();
          bool allInSet = true;
          while ( nodeIt->more() && allInSet ) { // loop on nodes of a face
            const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
            if ( nodeSet->find( n ) == nodeSet->end() )
              allInSet = false;
            else
              faceNodeSet.insert( n );
          }
          if ( allInSet ) {
            faceSet->insert( f );
            setOfFaceNodeSet.insert( faceNodeSet );
          }
        }
      }
    }

    // -------------------------------------------------------------------------
    // 1c. Create temporary faces representing sides of volumes if correspondent
    //     face does not exist
    // -------------------------------------------------------------------------

    if ( !volSet->empty() ) {
      //int nodeSetSize = nodeSet->size();

      // loop on given volumes
      for ( vIt = volSet->begin(); vIt != volSet->end(); vIt++ ) {
        SMDS_VolumeTool vol (*vIt);
        // loop on volume faces: find free faces
        // --------------------------------------
        list<const SMDS_MeshElement* > freeFaceList;
        for ( iFace = 0; iFace < vol.NbFaces(); iFace++ ) {
          if ( !vol.IsFreeFace( iFace ))
            continue;
          // check if there is already a face with same nodes in a face set
          const SMDS_MeshElement* aFreeFace = 0;
          const SMDS_MeshNode** fNodes = vol.GetFaceNodes( iFace );
          int nbNodes = vol.NbFaceNodes( iFace );
          set <const SMDS_MeshNode*> faceNodeSet;
          vol.GetFaceNodes( iFace, faceNodeSet );
          bool isNewFace = setOfFaceNodeSet.insert( faceNodeSet ).second;
          if ( isNewFace ) {
            // no such a face is given but it still can exist, check it
            vector<const SMDS_MeshNode *> nodes ( fNodes, fNodes + nbNodes);
            aFreeFace = aMesh->FindElement( nodes, SMDSAbs_Face, /*noMedium=*/false );
          }
          if ( !aFreeFace ) {
            // create a temporary face
            if ( nbNodes == 3 ) {
              //aFreeFace = aTmpFacesMesh.AddFace( fNodes[0],fNodes[1],fNodes[2] );
              aFreeFace = aMesh->AddFace( fNodes[0],fNodes[1],fNodes[2] );
            }
            else if ( nbNodes == 4 ) {
              //aFreeFace = aTmpFacesMesh.AddFace( fNodes[0],fNodes[1],fNodes[2],fNodes[3] );
              aFreeFace = aMesh->AddFace( fNodes[0],fNodes[1],fNodes[2],fNodes[3] );
            }
            else {
              vector<const SMDS_MeshNode *> poly_nodes ( fNodes, & fNodes[nbNodes]);
              //aFreeFace = aTmpFacesMesh.AddPolygonalFace(poly_nodes);
              aFreeFace = aMesh->AddPolygonalFace(poly_nodes);
            }
            if ( aFreeFace )
              tempFaceList.push_back( aFreeFace );
          }

          if ( aFreeFace )
            freeFaceList.push_back( aFreeFace );

        } // loop on faces of a volume

        // choose one of several free faces of a volume
        // --------------------------------------------
        if ( freeFaceList.size() > 1 ) {
          // choose a face having max nb of nodes shared by other elems of a side
          int maxNbNodes = -1;
          list<const SMDS_MeshElement* >::iterator fIt = freeFaceList.begin();
          while ( fIt != freeFaceList.end() ) { // loop on free faces
            int nbSharedNodes = 0;
            SMDS_ElemIteratorPtr nodeIt = (*fIt)->nodesIterator();
            while ( nodeIt->more() ) { // loop on free face nodes
              const SMDS_MeshNode* n =
                static_cast<const SMDS_MeshNode*>( nodeIt->next() );
              SMDS_ElemIteratorPtr invElemIt = n->GetInverseElementIterator();
              while ( invElemIt->more() ) {
                const SMDS_MeshElement* e = invElemIt->next();
                nbSharedNodes += faceSet->count( e );
                nbSharedNodes += elemSet->count( e );
              }
            }
            if ( nbSharedNodes > maxNbNodes ) {
              maxNbNodes = nbSharedNodes;
              freeFaceList.erase( freeFaceList.begin(), fIt++ );
            }
            else if ( nbSharedNodes == maxNbNodes ) {
              fIt++;
            }
            else {
              freeFaceList.erase( fIt++ ); // here fIt++ occurs before erase
            }
          }
          if ( freeFaceList.size() > 1 )
          {
            // could not choose one face, use another way
            // choose a face most close to the bary center of the opposite side
            gp_XYZ aBC( 0., 0., 0. );
            set <const SMDS_MeshNode*> addedNodes;
            TIDSortedElemSet * elemSet2 = elemSetPtr[ 1 - iSide ];
            eIt = elemSet2->begin();
            for ( eIt = elemSet2->begin(); eIt != elemSet2->end(); eIt++ ) {
              SMDS_ElemIteratorPtr nodeIt = (*eIt)->nodesIterator();
              while ( nodeIt->more() ) { // loop on free face nodes
                const SMDS_MeshNode* n =
                  static_cast<const SMDS_MeshNode*>( nodeIt->next() );
                if ( addedNodes.insert( n ).second )
                  aBC += gp_XYZ( n->X(),n->Y(),n->Z() );
              }
            }
            aBC /= addedNodes.size();
            double minDist = DBL_MAX;
            fIt = freeFaceList.begin();
            while ( fIt != freeFaceList.end() ) { // loop on free faces
              double dist = 0;
              SMDS_ElemIteratorPtr nodeIt = (*fIt)->nodesIterator();
              while ( nodeIt->more() ) { // loop on free face nodes
                const SMDS_MeshNode* n =
                  static_cast<const SMDS_MeshNode*>( nodeIt->next() );
                gp_XYZ p( n->X(),n->Y(),n->Z() );
                dist += ( aBC - p ).SquareModulus();
              }
              if ( dist < minDist ) {
                minDist = dist;
                freeFaceList.erase( freeFaceList.begin(), fIt++ );
              }
              else
                fIt = freeFaceList.erase( fIt++ );
            }
          }
        } // choose one of several free faces of a volume

        if ( freeFaceList.size() == 1 ) {
          const SMDS_MeshElement* aFreeFace = freeFaceList.front();
          faceSet->insert( aFreeFace );
          // complete a node set with nodes of a found free face
          //           for ( iNode = 0; iNode < ; iNode++ )
          //             nodeSet->insert( fNodes[ iNode ] );
        }

      } // loop on volumes of a side

      //       // complete a set of faces if new nodes in a nodeSet appeared
      //       // ----------------------------------------------------------
      //       if ( nodeSetSize != nodeSet->size() ) {
      //         for ( ; nIt != nodeSet->end(); nIt++ ) { // loop on nodes of iSide
      //           SMDS_ElemIteratorPtr fIt = (*nIt)->GetInverseElementIterator(SMDSAbs_Face);
      //           while ( fIt->more() ) { // loop on faces sharing a node
      //             const SMDS_MeshElement* f = fIt->next();
      //             if ( faceSet->find( f ) == faceSet->end() ) {
      //               // check if all nodes are in nodeSet and
      //               // complete setOfFaceNodeSet if they are
      //               set <const SMDS_MeshNode*> faceNodeSet;
      //               SMDS_ElemIteratorPtr nodeIt = f->nodesIterator();
      //               bool allInSet = true;
      //               while ( nodeIt->more() && allInSet ) { // loop on nodes of a face
      //                 const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
      //                 if ( nodeSet->find( n ) == nodeSet->end() )
      //                   allInSet = false;
      //                 else
      //                   faceNodeSet.insert( n );
      //               }
      //               if ( allInSet ) {
      //                 faceSet->insert( f );
      //                 setOfFaceNodeSet.insert( faceNodeSet );
      //               }
      //             }
      //           }
      //         }
      //       }
    } // Create temporary faces, if there are volumes given
  } // loop on sides

  if ( faceSet1.size() != faceSet2.size() ) {
    // delete temporary faces: they are in reverseElements of actual nodes
//    SMDS_FaceIteratorPtr tmpFaceIt = aTmpFacesMesh.facesIterator();
//    while ( tmpFaceIt->more() )
//      aTmpFacesMesh.RemoveElement( tmpFaceIt->next() );
//    list<const SMDS_MeshElement* >::iterator tmpFaceIt = tempFaceList.begin();
//    for (; tmpFaceIt !=tempFaceList.end(); ++tmpFaceIt)
//      aMesh->RemoveElement(*tmpFaceIt);
    MESSAGE("Diff nb of faces");
    return SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
  }

  // ============================================================
  // 2. Find nodes to merge:
  //              bind a node to remove to a node to put instead
  // ============================================================

  TNodeNodeMap nReplaceMap; // bind a node to remove to a node to put instead
  if ( theFirstNode1 != theFirstNode2 )
    nReplaceMap.insert( make_pair( theFirstNode1, theFirstNode2 ));
  if ( theSecondNode1 != theSecondNode2 )
    nReplaceMap.insert( make_pair( theSecondNode1, theSecondNode2 ));

  LinkID_Gen aLinkID_Gen( GetMeshDS() );
  set< long > linkIdSet; // links to process
  linkIdSet.insert( aLinkID_Gen.GetLinkID( theFirstNode1, theSecondNode1 ));

  typedef pair< const SMDS_MeshNode*, const SMDS_MeshNode* > NLink;
  list< NLink > linkList[2];
  linkList[0].push_back( NLink( theFirstNode1, theSecondNode1 ));
  linkList[1].push_back( NLink( theFirstNode2, theSecondNode2 ));
  // loop on links in linkList; find faces by links and append links
  // of the found faces to linkList
  list< NLink >::iterator linkIt[] = { linkList[0].begin(), linkList[1].begin() } ;
  for ( ; linkIt[0] != linkList[0].end(); linkIt[0]++, linkIt[1]++ )
  {
    NLink link[] = { *linkIt[0], *linkIt[1] };
    long linkID = aLinkID_Gen.GetLinkID( link[0].first, link[0].second );
    if ( !linkIdSet.count( linkID ) )
      continue;

    // by links, find faces in the face sets,
    // and find indices of link nodes in the found faces;
    // in a face set, there is only one or no face sharing a link
    // ---------------------------------------------------------------

    const SMDS_MeshElement* face[] = { 0, 0 };
    vector<const SMDS_MeshNode*> fnodes[2];
    int iLinkNode[2][2];
    TIDSortedElemSet avoidSet;
    for ( iSide = 0; iSide < 2; iSide++ ) { // loop on 2 sides
      const SMDS_MeshNode* n1 = link[iSide].first;
      const SMDS_MeshNode* n2 = link[iSide].second;
      //cout << "Side " << iSide << " ";
      //cout << "L( " << n1->GetID() << ", " << n2->GetID() << " ) " << endl;
      // find a face by two link nodes
      face[ iSide ] = SMESH_MeshAlgos::FindFaceInSet( n1, n2,
                                                      *faceSetPtr[ iSide ], avoidSet,
                                                      &iLinkNode[iSide][0],
                                                      &iLinkNode[iSide][1] );
      if ( face[ iSide ])
      {
        //cout << " F " << face[ iSide]->GetID() <<endl;
        faceSetPtr[ iSide ]->erase( face[ iSide ]);
        // put face nodes to fnodes
        if ( face[ iSide ]->IsQuadratic() )
        {
          // use interlaced nodes iterator
          const SMDS_VtkFace* F = dynamic_cast<const SMDS_VtkFace*>( face[ iSide ]);
          if (!F) throw SALOME_Exception(LOCALIZED("not an SMDS_VtkFace"));
          SMDS_ElemIteratorPtr nIter = F->interlacedNodesElemIterator();
          while ( nIter->more() )
            fnodes[ iSide ].push_back( cast2Node( nIter->next() ));
        }
        else
        {
          fnodes[ iSide ].assign( face[ iSide ]->begin_nodes(),
                                  face[ iSide ]->end_nodes() );
        }
        fnodes[ iSide ].push_back( fnodes[ iSide ].front());
      }
    }

    // check similarity of elements of the sides
    if (aResult == SEW_OK && (( face[0] && !face[1] ) || ( !face[0] && face[1] ))) {
      MESSAGE("Correspondent face not found on side " << ( face[0] ? 1 : 0 ));
      if ( nReplaceMap.size() == 2 ) { // faces on input nodes not found
        aResult = ( face[0] ? SEW_BAD_SIDE2_NODES : SEW_BAD_SIDE1_NODES );
      }
      else {
        aResult = SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
      }
      break; // do not return because it's necessary to remove tmp faces
    }

    // set nodes to merge
    // -------------------

    if ( face[0] && face[1] )  {
      const int nbNodes = face[0]->NbNodes();
      if ( nbNodes != face[1]->NbNodes() ) {
        MESSAGE("Diff nb of face nodes");
        aResult = SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
        break; // do not return because it s necessary to remove tmp faces
      }
      bool reverse[] = { false, false }; // order of nodes in the link
      for ( iSide = 0; iSide < 2; iSide++ ) { // loop on 2 sides
        // analyse link orientation in faces
        int i1 = iLinkNode[ iSide ][ 0 ];
        int i2 = iLinkNode[ iSide ][ 1 ];
        reverse[ iSide ] = Abs( i1 - i2 ) == 1 ? i1 > i2 : i2 > i1;
      }
      int di1 = reverse[0] ? -1 : +1, i1 = iLinkNode[0][1] + di1;
      int di2 = reverse[1] ? -1 : +1, i2 = iLinkNode[1][1] + di2;
      for ( int i = nbNodes - 2; i > 0; --i, i1 += di1, i2 += di2 )
      {
        nReplaceMap.insert  ( make_pair ( fnodes[0][ ( i1 + nbNodes ) % nbNodes ],
                                          fnodes[1][ ( i2 + nbNodes ) % nbNodes ]));
      }

      // add other links of the faces to linkList
      // -----------------------------------------

      for ( iNode = 0; iNode < nbNodes; iNode++ )  {
        linkID = aLinkID_Gen.GetLinkID( fnodes[0][iNode], fnodes[0][iNode+1] );
        pair< set<long>::iterator, bool > iter_isnew = linkIdSet.insert( linkID );
        if ( !iter_isnew.second ) { // already in a set: no need to process
          linkIdSet.erase( iter_isnew.first );
        }
        else // new in set == encountered for the first time: add
        {
          const SMDS_MeshNode* n1 = fnodes[0][ iNode ];
          const SMDS_MeshNode* n2 = fnodes[0][ iNode + 1];
          linkList[0].push_back ( NLink( n1, n2 ));
          linkList[1].push_back ( NLink( nReplaceMap[n1], nReplaceMap[n2] ));
        }
      }
    } // 2 faces found

    if ( faceSetPtr[0]->empty() || faceSetPtr[1]->empty() )
      break;

  } // loop on link lists

  if ( aResult == SEW_OK &&
       ( //linkIt[0] != linkList[0].end() ||
         !faceSetPtr[0]->empty() || !faceSetPtr[1]->empty() )) {
    MESSAGE( (linkIt[0] != linkList[0].end()) <<" "<< (faceSetPtr[0]->empty()) <<
             " " << (faceSetPtr[1]->empty()));
    aResult = SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
  }

  // ====================================================================
  // 3. Replace nodes in elements of the side 1 and remove replaced nodes
  // ====================================================================

  // delete temporary faces
//  SMDS_FaceIteratorPtr tmpFaceIt = aTmpFacesMesh.facesIterator();
//  while ( tmpFaceIt->more() )
//    aTmpFacesMesh.RemoveElement( tmpFaceIt->next() );
  list<const SMDS_MeshElement* >::iterator tmpFaceIt = tempFaceList.begin();
  for (; tmpFaceIt !=tempFaceList.end(); ++tmpFaceIt)
    aMesh->RemoveElement(*tmpFaceIt);

  if ( aResult != SEW_OK)
    return aResult;

  list< int > nodeIDsToRemove;
  vector< const SMDS_MeshNode*> nodes;
  ElemFeatures elemType;

  // loop on nodes replacement map
  TNodeNodeMap::iterator nReplaceMapIt = nReplaceMap.begin(), nnIt;
  for ( ; nReplaceMapIt != nReplaceMap.end(); nReplaceMapIt++ )
    if ( (*nReplaceMapIt).first != (*nReplaceMapIt).second )
    {
      const SMDS_MeshNode* nToRemove = (*nReplaceMapIt).first;
      nodeIDsToRemove.push_back( nToRemove->GetID() );
      // loop on elements sharing nToRemove
      SMDS_ElemIteratorPtr invElemIt = nToRemove->GetInverseElementIterator();
      while ( invElemIt->more() ) {
        const SMDS_MeshElement* e = invElemIt->next();
        // get a new suite of nodes: make replacement
        int nbReplaced = 0, i = 0, nbNodes = e->NbNodes();
        nodes.resize( nbNodes );
        SMDS_ElemIteratorPtr nIt = e->nodesIterator();
        while ( nIt->more() ) {
          const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nIt->next() );
          nnIt = nReplaceMap.find( n );
          if ( nnIt != nReplaceMap.end() ) {
            nbReplaced++;
            n = (*nnIt).second;
          }
          nodes[ i++ ] = n;
        }
        //       if ( nbReplaced == nbNodes && e->GetType() == SMDSAbs_Face )
        //         elemIDsToRemove.push_back( e->GetID() );
        //       else
        if ( nbReplaced )
        {
          elemType.Init( e, /*basicOnly=*/false ).SetID( e->GetID() );
          aMesh->RemoveElement( e );

          if ( SMDS_MeshElement* newElem = this->AddElement( nodes, elemType ))
          {
            AddToSameGroups( newElem, e, aMesh );
            if ( int aShapeId = e->getshapeId() )
              aMesh->SetMeshElementOnShape( newElem, aShapeId );
          }
        }
      }
    }

  Remove( nodeIDsToRemove, true );

  return aResult;
}

//================================================================================
/*!
 * \brief Find corresponding nodes in two sets of faces
 * \param theSide1 - first face set
 * \param theSide2 - second first face
 * \param theFirstNode1 - a boundary node of set 1
 * \param theFirstNode2 - a node of set 2 corresponding to theFirstNode1
 * \param theSecondNode1 - a boundary node of set 1 linked with theFirstNode1
 * \param theSecondNode2 - a node of set 2 corresponding to theSecondNode1
 * \param nReplaceMap - output map of corresponding nodes
 * \return bool  - is a success or not
 */
//================================================================================

#ifdef _DEBUG_
//#define DEBUG_MATCHING_NODES
#endif

SMESH_MeshEditor::Sew_Error
SMESH_MeshEditor::FindMatchingNodes(set<const SMDS_MeshElement*>& theSide1,
                                    set<const SMDS_MeshElement*>& theSide2,
                                    const SMDS_MeshNode*          theFirstNode1,
                                    const SMDS_MeshNode*          theFirstNode2,
                                    const SMDS_MeshNode*          theSecondNode1,
                                    const SMDS_MeshNode*          theSecondNode2,
                                    TNodeNodeMap &                nReplaceMap)
{
  set<const SMDS_MeshElement*> * faceSetPtr[] = { &theSide1, &theSide2 };

  nReplaceMap.clear();
  if ( theFirstNode1 != theFirstNode2 )
    nReplaceMap.insert( make_pair( theFirstNode1, theFirstNode2 ));
  if ( theSecondNode1 != theSecondNode2 )
    nReplaceMap.insert( make_pair( theSecondNode1, theSecondNode2 ));

  set< SMESH_TLink > linkSet; // set of nodes where order of nodes is ignored
  linkSet.insert( SMESH_TLink( theFirstNode1, theSecondNode1 ));

  list< NLink > linkList[2];
  linkList[0].push_back( NLink( theFirstNode1, theSecondNode1 ));
  linkList[1].push_back( NLink( theFirstNode2, theSecondNode2 ));

  // loop on links in linkList; find faces by links and append links
  // of the found faces to linkList
  list< NLink >::iterator linkIt[] = { linkList[0].begin(), linkList[1].begin() } ;
  for ( ; linkIt[0] != linkList[0].end(); linkIt[0]++, linkIt[1]++ ) {
    NLink link[] = { *linkIt[0], *linkIt[1] };
    if ( linkSet.find( link[0] ) == linkSet.end() )
      continue;

    // by links, find faces in the face sets,
    // and find indices of link nodes in the found faces;
    // in a face set, there is only one or no face sharing a link
    // ---------------------------------------------------------------

    const SMDS_MeshElement* face[] = { 0, 0 };
    list<const SMDS_MeshNode*> notLinkNodes[2];
    //bool reverse[] = { false, false }; // order of notLinkNodes
    int nbNodes[2];
    for ( int iSide = 0; iSide < 2; iSide++ ) // loop on 2 sides
    {
      const SMDS_MeshNode* n1 = link[iSide].first;
      const SMDS_MeshNode* n2 = link[iSide].second;
      set<const SMDS_MeshElement*> * faceSet = faceSetPtr[ iSide ];
      set< const SMDS_MeshElement* > facesOfNode1;
      for ( int iNode = 0; iNode < 2; iNode++ ) // loop on 2 nodes of a link
      {
        // during a loop of the first node, we find all faces around n1,
        // during a loop of the second node, we find one face sharing both n1 and n2
        const SMDS_MeshNode* n = iNode ? n1 : n2; // a node of a link
        SMDS_ElemIteratorPtr fIt = n->GetInverseElementIterator(SMDSAbs_Face);
        while ( fIt->more() ) { // loop on faces sharing a node
          const SMDS_MeshElement* f = fIt->next();
          if (faceSet->find( f ) != faceSet->end() && // f is in face set
              ! facesOfNode1.insert( f ).second ) // f encounters twice
          {
            if ( face[ iSide ] ) {
              MESSAGE( "2 faces per link " );
              return ( iSide ? SEW_BAD_SIDE2_NODES : SEW_BAD_SIDE1_NODES );
            }
            face[ iSide ] = f;
            faceSet->erase( f );

            // get not link nodes
            int nbN = f->NbNodes();
            if ( f->IsQuadratic() )
              nbN /= 2;
            nbNodes[ iSide ] = nbN;
            list< const SMDS_MeshNode* > & nodes = notLinkNodes[ iSide ];
            int i1 = f->GetNodeIndex( n1 );
            int i2 = f->GetNodeIndex( n2 );
            int iEnd = nbN, iBeg = -1, iDelta = 1;
            bool reverse = ( Abs( i1 - i2 ) == 1 ? i1 > i2 : i2 > i1 );
            if ( reverse ) {
              std::swap( iEnd, iBeg ); iDelta = -1;
            }
            int i = i2;
            while ( true ) {
              i += iDelta;
              if ( i == iEnd ) i = iBeg + iDelta;
              if ( i == i1 ) break;
              nodes.push_back ( f->GetNode( i ) );
            }
          }
        }
      }
    }
    // check similarity of elements of the sides
    if (( face[0] && !face[1] ) || ( !face[0] && face[1] )) {
      MESSAGE("Correspondent face not found on side " << ( face[0] ? 1 : 0 ));
      if ( nReplaceMap.size() == 2 ) { // faces on input nodes not found
        return ( face[0] ? SEW_BAD_SIDE2_NODES : SEW_BAD_SIDE1_NODES );
      }
      else {
        return SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
      }
    }

    // set nodes to merge
    // -------------------

    if ( face[0] && face[1] )  {
      if ( nbNodes[0] != nbNodes[1] ) {
        MESSAGE("Diff nb of face nodes");
        return SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
      }
#ifdef DEBUG_MATCHING_NODES
      MESSAGE ( " Link 1: " << link[0].first->GetID() <<" "<< link[0].second->GetID()
                << " F 1: " << face[0] << "| Link 2: " << link[1].first->GetID() <<" "
                << link[1].second->GetID() << " F 2: " << face[1] << " | Bind: " ) ;
#endif
      int nbN = nbNodes[0];
      {
        list<const SMDS_MeshNode*>::iterator n1 = notLinkNodes[0].begin();
        list<const SMDS_MeshNode*>::iterator n2 = notLinkNodes[1].begin();
        for ( int i = 0 ; i < nbN - 2; ++i ) {
#ifdef DEBUG_MATCHING_NODES
          MESSAGE ( (*n1)->GetID() << " to " << (*n2)->GetID() );
#endif
          nReplaceMap.insert( make_pair( *(n1++), *(n2++) ));
        }
      }

      // add other links of the face 1 to linkList
      // -----------------------------------------

      const SMDS_MeshElement* f0 = face[0];
      const SMDS_MeshNode* n1 = f0->GetNode( nbN - 1 );
      for ( int i = 0; i < nbN; i++ )
      {
        const SMDS_MeshNode* n2 = f0->GetNode( i );
        pair< set< SMESH_TLink >::iterator, bool > iter_isnew =
          linkSet.insert( SMESH_TLink( n1, n2 ));
        if ( !iter_isnew.second ) { // already in a set: no need to process
          linkSet.erase( iter_isnew.first );
        }
        else // new in set == encountered for the first time: add
        {
#ifdef DEBUG_MATCHING_NODES
          MESSAGE ( "Add link 1: " << n1->GetID() << " " << n2->GetID() << " "
                    << " | link 2: " << nReplaceMap[n1]->GetID() << " " << nReplaceMap[n2]->GetID() << " " );
#endif
          linkList[0].push_back ( NLink( n1, n2 ));
          linkList[1].push_back ( NLink( nReplaceMap[n1], nReplaceMap[n2] ));
        }
        n1 = n2;
      }
    } // 2 faces found
  } // loop on link lists

  return SEW_OK;
}

//================================================================================
/*!
 * \brief Create elements equal (on same nodes) to given ones
 *  \param [in] theElements - a set of elems to duplicate. If it is empty, all
 *              elements of the uppest dimension are duplicated.
 */
//================================================================================

void SMESH_MeshEditor::DoubleElements( const TIDSortedElemSet& theElements )
{
  ClearLastCreated();
  SMESHDS_Mesh* mesh = GetMeshDS();

  // get an element type and an iterator over elements

  SMDSAbs_ElementType type;
  SMDS_ElemIteratorPtr elemIt;
  vector< const SMDS_MeshElement* > allElems;
  if ( theElements.empty() )
  {
    if ( mesh->NbNodes() == 0 )
      return;
    // get most complex type
    SMDSAbs_ElementType types[SMDSAbs_NbElementTypes] = {
      SMDSAbs_Volume, SMDSAbs_Face, SMDSAbs_Edge,
      SMDSAbs_0DElement, SMDSAbs_Ball, SMDSAbs_Node
    };
    for ( int i = 0; i < SMDSAbs_NbElementTypes; ++i )
      if ( mesh->GetMeshInfo().NbElements( types[i] ))
      {
        type = types[i];
        break;
      }
    // put all elements in the vector <allElems>
    allElems.reserve( mesh->GetMeshInfo().NbElements( type ));
    elemIt = mesh->elementsIterator( type );
    while ( elemIt->more() )
      allElems.push_back( elemIt->next());
    elemIt = elemSetIterator( allElems );
  }
  else
  {
    type = (*theElements.begin())->GetType();
    elemIt = elemSetIterator( theElements );
  }

  // duplicate elements

  ElemFeatures elemType;

  vector< const SMDS_MeshNode* > nodes;
  while ( elemIt->more() )
  {
    const SMDS_MeshElement* elem = elemIt->next();
    if ( elem->GetType() != type )
      continue;

    elemType.Init( elem, /*basicOnly=*/false );
    nodes.assign( elem->begin_nodes(), elem->end_nodes() );

    AddElement( nodes, elemType );
  }
}

//================================================================================
/*!
  \brief Creates a hole in a mesh by doubling the nodes of some particular elements
  \param theElems - the list of elements (edges or faces) to be replicated
  The nodes for duplication could be found from these elements
  \param theNodesNot - list of nodes to NOT replicate
  \param theAffectedElems - the list of elements (cells and edges) to which the
  replicated nodes should be associated to.
  \return TRUE if operation has been completed successfully, FALSE otherwise
*/
//================================================================================

bool SMESH_MeshEditor::DoubleNodes( const TIDSortedElemSet& theElems,
                                    const TIDSortedElemSet& theNodesNot,
                                    const TIDSortedElemSet& theAffectedElems )
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if ( theElems.size() == 0 )
    return false;

  SMESHDS_Mesh* aMeshDS = GetMeshDS();
  if ( !aMeshDS )
    return false;

  bool res = false;
  TNodeNodeMap anOldNodeToNewNode;
  // duplicate elements and nodes
  res = doubleNodes( aMeshDS, theElems, theNodesNot, anOldNodeToNewNode, true );
  // replce nodes by duplications
  res = doubleNodes( aMeshDS, theAffectedElems, theNodesNot, anOldNodeToNewNode, false );
  return res;
}

//================================================================================
/*!
  \brief Creates a hole in a mesh by doubling the nodes of some particular elements
  \param theMeshDS - mesh instance
  \param theElems - the elements replicated or modified (nodes should be changed)
  \param theNodesNot - nodes to NOT replicate
  \param theNodeNodeMap - relation of old node to new created node
  \param theIsDoubleElem - flag os to replicate element or modify
  \return TRUE if operation has been completed successfully, FALSE otherwise
*/
//================================================================================

bool SMESH_MeshEditor::doubleNodes(SMESHDS_Mesh*           theMeshDS,
                                   const TIDSortedElemSet& theElems,
                                   const TIDSortedElemSet& theNodesNot,
                                   TNodeNodeMap&           theNodeNodeMap,
                                   const bool              theIsDoubleElem )
{
  MESSAGE("doubleNodes");
  // iterate through element and duplicate them (by nodes duplication)
  bool res = false;
  std::vector<const SMDS_MeshNode*> newNodes;
  ElemFeatures elemType;

  TIDSortedElemSet::const_iterator elemItr = theElems.begin();
  for ( ;  elemItr != theElems.end(); ++elemItr )
  {
    const SMDS_MeshElement* anElem = *elemItr;
    if (!anElem)
      continue;

    // duplicate nodes to duplicate element
    bool isDuplicate = false;
    newNodes.resize( anElem->NbNodes() );
    SMDS_ElemIteratorPtr anIter = anElem->nodesIterator();
    int ind = 0;
    while ( anIter->more() )
    {
      const SMDS_MeshNode* aCurrNode = static_cast<const SMDS_MeshNode*>( anIter->next() );
      const SMDS_MeshNode*  aNewNode = aCurrNode;
      TNodeNodeMap::iterator     n2n = theNodeNodeMap.find( aCurrNode );
      if ( n2n != theNodeNodeMap.end() )
      {
        aNewNode = n2n->second;
      }
      else if ( theIsDoubleElem && !theNodesNot.count( aCurrNode ))
      {
        // duplicate node
        aNewNode = theMeshDS->AddNode( aCurrNode->X(), aCurrNode->Y(), aCurrNode->Z() );
        copyPosition( aCurrNode, aNewNode );
        theNodeNodeMap[ aCurrNode ] = aNewNode;
        myLastCreatedNodes.Append( aNewNode );
      }
      isDuplicate |= (aCurrNode != aNewNode);
      newNodes[ ind++ ] = aNewNode;
    }
    if ( !isDuplicate )
      continue;

    if ( theIsDoubleElem )
      AddElement( newNodes, elemType.Init( anElem, /*basicOnly=*/false ));
    else
      theMeshDS->ChangeElementNodes( anElem, &newNodes[ 0 ], newNodes.size() );

    res = true;
  }
  return res;
}

//================================================================================
/*!
  \brief Creates a hole in a mesh by doubling the nodes of some particular elements
  \param theNodes - identifiers of nodes to be doubled
  \param theModifiedElems - identifiers of elements to be updated by the new (doubled)
  nodes. If list of element identifiers is empty then nodes are doubled but
  they not assigned to elements
  \return TRUE if operation has been completed successfully, FALSE otherwise
*/
//================================================================================

bool SMESH_MeshEditor::DoubleNodes( const std::list< int >& theListOfNodes,
                                    const std::list< int >& theListOfModifiedElems )
{
  MESSAGE("DoubleNodes");
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if ( theListOfNodes.size() == 0 )
    return false;

  SMESHDS_Mesh* aMeshDS = GetMeshDS();
  if ( !aMeshDS )
    return false;

  // iterate through nodes and duplicate them

  std::map< const SMDS_MeshNode*, const SMDS_MeshNode* > anOldNodeToNewNode;

  std::list< int >::const_iterator aNodeIter;
  for ( aNodeIter = theListOfNodes.begin(); aNodeIter != theListOfNodes.end(); ++aNodeIter )
  {
    int aCurr = *aNodeIter;
    SMDS_MeshNode* aNode = (SMDS_MeshNode*)aMeshDS->FindNode( aCurr );
    if ( !aNode )
      continue;

    // duplicate node

    const SMDS_MeshNode* aNewNode = aMeshDS->AddNode( aNode->X(), aNode->Y(), aNode->Z() );
    if ( aNewNode )
    {
      copyPosition( aNode, aNewNode );
      anOldNodeToNewNode[ aNode ] = aNewNode;
      myLastCreatedNodes.Append( aNewNode );
    }
  }

  // Create map of new nodes for modified elements

  std::map< SMDS_MeshElement*, vector<const SMDS_MeshNode*> > anElemToNodes;

  std::list< int >::const_iterator anElemIter;
  for ( anElemIter = theListOfModifiedElems.begin();
        anElemIter != theListOfModifiedElems.end(); ++anElemIter )
  {
    int aCurr = *anElemIter;
    SMDS_MeshElement* anElem = (SMDS_MeshElement*)aMeshDS->FindElement( aCurr );
    if ( !anElem )
      continue;

    vector<const SMDS_MeshNode*> aNodeArr( anElem->NbNodes() );

    SMDS_ElemIteratorPtr anIter = anElem->nodesIterator();
    int ind = 0;
    while ( anIter->more() )
    {
      SMDS_MeshNode* aCurrNode = (SMDS_MeshNode*)anIter->next();
      if ( aCurr && anOldNodeToNewNode.find( aCurrNode ) != anOldNodeToNewNode.end() )
      {
        const SMDS_MeshNode* aNewNode = anOldNodeToNewNode[ aCurrNode ];
        aNodeArr[ ind++ ] = aNewNode;
      }
      else
        aNodeArr[ ind++ ] = aCurrNode;
    }
    anElemToNodes[ anElem ] = aNodeArr;
  }

  // Change nodes of elements

  std::map< SMDS_MeshElement*, vector<const SMDS_MeshNode*> >::iterator
    anElemToNodesIter = anElemToNodes.begin();
  for ( ; anElemToNodesIter != anElemToNodes.end(); ++anElemToNodesIter )
  {
    const SMDS_MeshElement* anElem = anElemToNodesIter->first;
    vector<const SMDS_MeshNode*> aNodeArr = anElemToNodesIter->second;
    if ( anElem )
      {
      MESSAGE("ChangeElementNodes");
      aMeshDS->ChangeElementNodes( anElem, &aNodeArr[ 0 ], anElem->NbNodes() );
      }
  }

  return true;
}

namespace {

  //================================================================================
  /*!
  \brief Check if element located inside shape
  \return TRUE if IN or ON shape, FALSE otherwise
  */
  //================================================================================

  template<class Classifier>
  bool isInside(const SMDS_MeshElement* theElem,
                Classifier&             theClassifier,
                const double            theTol)
  {
    gp_XYZ centerXYZ (0, 0, 0);
    SMDS_ElemIteratorPtr aNodeItr = theElem->nodesIterator();
    while (aNodeItr->more())
      centerXYZ += SMESH_TNodeXYZ(cast2Node( aNodeItr->next()));

    gp_Pnt aPnt = centerXYZ / theElem->NbNodes();
    theClassifier.Perform(aPnt, theTol);
    TopAbs_State aState = theClassifier.State();
    return (aState == TopAbs_IN || aState == TopAbs_ON );
  }

  //================================================================================
  /*!
   * \brief Classifier of the 3D point on the TopoDS_Face
   *        with interaface suitable for isInside()
   */
  //================================================================================

  struct _FaceClassifier
  {
    Extrema_ExtPS       _extremum;
    BRepAdaptor_Surface _surface;
    TopAbs_State        _state;

    _FaceClassifier(const TopoDS_Face& face):_extremum(),_surface(face),_state(TopAbs_OUT)
    {
      _extremum.Initialize( _surface,
                            _surface.FirstUParameter(), _surface.LastUParameter(),
                            _surface.FirstVParameter(), _surface.LastVParameter(),
                            _surface.Tolerance(), _surface.Tolerance() );
    }
    void Perform(const gp_Pnt& aPnt, double theTol)
    {
      theTol *= theTol;
      _state = TopAbs_OUT;
      _extremum.Perform(aPnt);
      if ( _extremum.IsDone() )
        for ( int iSol = 1; iSol <= _extremum.NbExt() && _state == TopAbs_OUT; ++iSol)
          _state = ( _extremum.SquareDistance(iSol) <= theTol ? TopAbs_IN : TopAbs_OUT );
    }
    TopAbs_State State() const
    {
      return _state;
    }
  };
}

//================================================================================
/*!
  \brief Identify the elements that will be affected by node duplication (actual duplication is not performed).
  This method is the first step of DoubleNodeElemGroupsInRegion.
  \param theElems - list of groups of elements (edges or faces) to be replicated
  \param theNodesNot - list of groups of nodes not to replicated
  \param theShape - shape to detect affected elements (element which geometric center
         located on or inside shape). If the shape is null, detection is done on faces orientations
         (select elements with a gravity center on the side given by faces normals).
         This mode (null shape) is faster, but works only when theElems are faces, with coherents orientations.
         The replicated nodes should be associated to affected elements.
  \return groups of affected elements
  \sa DoubleNodeElemGroupsInRegion()
 */
//================================================================================

bool SMESH_MeshEditor::AffectedElemGroupsInRegion( const TIDSortedElemSet& theElems,
                                                   const TIDSortedElemSet& theNodesNot,
                                                   const TopoDS_Shape&     theShape,
                                                   TIDSortedElemSet&       theAffectedElems)
{
  if ( theShape.IsNull() )
  {
    std::set<const SMDS_MeshNode*> alreadyCheckedNodes;
    std::set<const SMDS_MeshElement*> alreadyCheckedElems;
    std::set<const SMDS_MeshElement*> edgesToCheck;
    alreadyCheckedNodes.clear();
    alreadyCheckedElems.clear();
    edgesToCheck.clear();

    // --- iterates on elements to be replicated and get elements by back references from their nodes

    TIDSortedElemSet::const_iterator elemItr = theElems.begin();
    int ielem;
    for ( ielem=1;  elemItr != theElems.end(); ++elemItr )
    {
      SMDS_MeshElement* anElem = (SMDS_MeshElement*)*elemItr;
      if (!anElem || (anElem->GetType() != SMDSAbs_Face))
        continue;
      gp_XYZ normal;
      SMESH_MeshAlgos::FaceNormal( anElem, normal, /*normalized=*/true );
      MESSAGE("element " << ielem++ <<  " normal " << normal.X() << " " << normal.Y() << " " << normal.Z());
      std::set<const SMDS_MeshNode*> nodesElem;
      nodesElem.clear();
      SMDS_ElemIteratorPtr nodeItr = anElem->nodesIterator();
      while ( nodeItr->more() )
      {
        const SMDS_MeshNode* aNode = cast2Node(nodeItr->next());
        nodesElem.insert(aNode);
      }
      std::set<const SMDS_MeshNode*>::iterator nodit = nodesElem.begin();
      for (; nodit != nodesElem.end(); nodit++)
      {
        MESSAGE("  noeud ");
        const SMDS_MeshNode* aNode = *nodit;
        if ( !aNode || theNodesNot.find(aNode) != theNodesNot.end() )
          continue;
        if (alreadyCheckedNodes.find(aNode) != alreadyCheckedNodes.end())
          continue;
        alreadyCheckedNodes.insert(aNode);
        SMDS_ElemIteratorPtr backElemItr = aNode->GetInverseElementIterator();
        while ( backElemItr->more() )
        {
          MESSAGE("    backelem ");
          const SMDS_MeshElement* curElem = backElemItr->next();
          if (alreadyCheckedElems.find(curElem) != alreadyCheckedElems.end())
            continue;
          if (theElems.find(curElem) != theElems.end())
            continue;
          alreadyCheckedElems.insert(curElem);
          double x=0, y=0, z=0;
          int nb = 0;
          SMDS_ElemIteratorPtr nodeItr2 = curElem->nodesIterator();
          while ( nodeItr2->more() )
          {
            const SMDS_MeshNode* anotherNode = cast2Node(nodeItr2->next());
            x += anotherNode->X();
            y += anotherNode->Y();
            z += anotherNode->Z();
            nb++;
          }
          gp_XYZ p;
          p.SetCoord( x/nb -aNode->X(),
                      y/nb -aNode->Y(),
                      z/nb -aNode->Z() );
          MESSAGE("      check " << p.X() << " " << p.Y() << " " << p.Z());
          if (normal*p > 0)
          {
            MESSAGE("    --- inserted")
            theAffectedElems.insert( curElem );
          }
          else if (curElem->GetType() == SMDSAbs_Edge)
            edgesToCheck.insert(curElem);
        }
      }
    }
    // --- add also edges lying on the set of faces (all nodes in alreadyCheckedNodes)
    std::set<const SMDS_MeshElement*>::iterator eit = edgesToCheck.begin();
    for( ; eit != edgesToCheck.end(); eit++)
    {
      bool onside = true;
      const SMDS_MeshElement* anEdge = *eit;
      SMDS_ElemIteratorPtr nodeItr = anEdge->nodesIterator();
      while ( nodeItr->more() )
      {
        const SMDS_MeshNode* aNode = cast2Node(nodeItr->next());
        if (alreadyCheckedNodes.find(aNode) == alreadyCheckedNodes.end())
        {
          onside = false;
          break;
        }
      }
      if (onside)
      {
        MESSAGE("    --- edge onside inserted")
        theAffectedElems.insert(anEdge);
      }
    }
  }
  else
  {
    const double aTol = Precision::Confusion();
    unique_ptr< BRepClass3d_SolidClassifier> bsc3d;
    unique_ptr<_FaceClassifier>              aFaceClassifier;
    if ( theShape.ShapeType() == TopAbs_SOLID )
    {
      bsc3d.reset( new BRepClass3d_SolidClassifier(theShape));;
      bsc3d->PerformInfinitePoint(aTol);
    }
    else if (theShape.ShapeType() == TopAbs_FACE )
    {
      aFaceClassifier.reset( new _FaceClassifier(TopoDS::Face(theShape)));
    }

    // iterates on indicated elements and get elements by back references from their nodes
    TIDSortedElemSet::const_iterator elemItr = theElems.begin();
    int ielem;
    for ( ielem = 1;  elemItr != theElems.end(); ++elemItr )
    {
      MESSAGE("element " << ielem++);
      SMDS_MeshElement* anElem = (SMDS_MeshElement*)*elemItr;
      if (!anElem)
        continue;
      SMDS_ElemIteratorPtr nodeItr = anElem->nodesIterator();
      while ( nodeItr->more() )
      {
        MESSAGE("  noeud ");
        const SMDS_MeshNode* aNode = cast2Node(nodeItr->next());
        if ( !aNode || theNodesNot.find(aNode) != theNodesNot.end() )
          continue;
        SMDS_ElemIteratorPtr backElemItr = aNode->GetInverseElementIterator();
        while ( backElemItr->more() )
        {
          MESSAGE("    backelem ");
          const SMDS_MeshElement* curElem = backElemItr->next();
          if ( curElem && theElems.find(curElem) == theElems.end() &&
              ( bsc3d.get() ?
                isInside( curElem, *bsc3d, aTol ) :
                isInside( curElem, *aFaceClassifier, aTol )))
            theAffectedElems.insert( curElem );
        }
      }
    }
  }
  return true;
}

//================================================================================
/*!
  \brief Creates a hole in a mesh by doubling the nodes of some particular elements
  \param theElems - group of of elements (edges or faces) to be replicated
  \param theNodesNot - group of nodes not to replicate
  \param theShape - shape to detect affected elements (element which geometric center
  located on or inside shape).
  The replicated nodes should be associated to affected elements.
  \return TRUE if operation has been completed successfully, FALSE otherwise
*/
//================================================================================

bool SMESH_MeshEditor::DoubleNodesInRegion( const TIDSortedElemSet& theElems,
                                            const TIDSortedElemSet& theNodesNot,
                                            const TopoDS_Shape&     theShape )
{
  if ( theShape.IsNull() )
    return false;

  const double aTol = Precision::Confusion();
  unique_ptr< BRepClass3d_SolidClassifier> bsc3d;
  unique_ptr<_FaceClassifier>              aFaceClassifier;
  if ( theShape.ShapeType() == TopAbs_SOLID )
  {
    bsc3d.reset( new BRepClass3d_SolidClassifier(theShape));;
    bsc3d->PerformInfinitePoint(aTol);
  }
  else if (theShape.ShapeType() == TopAbs_FACE )
  {
    aFaceClassifier.reset( new _FaceClassifier(TopoDS::Face(theShape)));
  }

  // iterates on indicated elements and get elements by back references from their nodes
  TIDSortedElemSet anAffected;
  TIDSortedElemSet::const_iterator elemItr = theElems.begin();
  for ( ;  elemItr != theElems.end(); ++elemItr )
  {
    SMDS_MeshElement* anElem = (SMDS_MeshElement*)*elemItr;
    if (!anElem)
      continue;

    SMDS_ElemIteratorPtr nodeItr = anElem->nodesIterator();
    while ( nodeItr->more() )
    {
      const SMDS_MeshNode* aNode = cast2Node(nodeItr->next());
      if ( !aNode || theNodesNot.find(aNode) != theNodesNot.end() )
        continue;
      SMDS_ElemIteratorPtr backElemItr = aNode->GetInverseElementIterator();
      while ( backElemItr->more() )
      {
        const SMDS_MeshElement* curElem = backElemItr->next();
        if ( curElem && theElems.find(curElem) == theElems.end() &&
             ( bsc3d.get() ?
               isInside( curElem, *bsc3d, aTol ) :
               isInside( curElem, *aFaceClassifier, aTol )))
          anAffected.insert( curElem );
      }
    }
  }
  return DoubleNodes( theElems, theNodesNot, anAffected );
}

/*!
 *  \brief compute an oriented angle between two planes defined by four points.
 *  The vector (p0,p1) defines the intersection of the 2 planes (p0,p1,g1) and (p0,p1,g2)
 *  @param p0 base of the rotation axe
 *  @param p1 extremity of the rotation axe
 *  @param g1 belongs to the first plane
 *  @param g2 belongs to the second plane
 */
double SMESH_MeshEditor::OrientedAngle(const gp_Pnt& p0, const gp_Pnt& p1, const gp_Pnt& g1, const gp_Pnt& g2)
{
//  MESSAGE("    p0: " << p0.X() << " " << p0.Y() << " " << p0.Z());
//  MESSAGE("    p1: " << p1.X() << " " << p1.Y() << " " << p1.Z());
//  MESSAGE("    g1: " << g1.X() << " " << g1.Y() << " " << g1.Z());
//  MESSAGE("    g2: " << g2.X() << " " << g2.Y() << " " << g2.Z());
  gp_Vec vref(p0, p1);
  gp_Vec v1(p0, g1);
  gp_Vec v2(p0, g2);
  gp_Vec n1 = vref.Crossed(v1);
  gp_Vec n2 = vref.Crossed(v2);
  try {
    return n2.AngleWithRef(n1, vref);
  }
  catch ( Standard_Failure &) {
  }
  return Max( v1.Magnitude(), v2.Magnitude() );
}

/*!
 * \brief Double nodes on shared faces between groups of volumes and create flat elements on demand.
 *  The list of groups must contain at least two groups. The groups have to be disjoint: no common element into two different groups.
 * The nodes of the internal faces at the boundaries of the groups are doubled. Optionally, the internal faces are replaced by flat elements.
 * Triangles are transformed into prisms, and quadrangles into hexahedrons.
 * The flat elements are stored in groups of volumes. These groups are named according to the position of the group in the list:
 * the group j_n_p is the group of the flat elements that are built between the group #n and the group #p in the list.
 * If there is no shared faces between the group #n and the group #p in the list, the group j_n_p is not created.
 * All the flat elements are gathered into the group named "joints3D" (or "joints2D" in 2D situation).
 * The flat element of the multiple junctions between the simple junction are stored in a group named "jointsMultiples".
 * \param theElems - list of groups of volumes, where a group of volume is a set of
 *        SMDS_MeshElements sorted by Id.
 * \param createJointElems - if TRUE, create the elements
 * \param onAllBoundaries - if TRUE, the nodes and elements are also created on
 *        the boundary between \a theDomains and the rest mesh
 * \return TRUE if operation has been completed successfully, FALSE otherwise
 */
bool SMESH_MeshEditor::DoubleNodesOnGroupBoundaries( const std::vector<TIDSortedElemSet>& theElems,
                                                     bool                                 createJointElems,
                                                     bool                                 onAllBoundaries)
{
  MESSAGE("----------------------------------------------");
  MESSAGE("SMESH_MeshEditor::doubleNodesOnGroupBoundaries");
  MESSAGE("----------------------------------------------");

  SMESHDS_Mesh *meshDS = this->myMesh->GetMeshDS();
  meshDS->BuildDownWardConnectivity(true);
  CHRONO(50);
  SMDS_UnstructuredGrid *grid = meshDS->getGrid();

  // --- build the list of faces shared by 2 domains (group of elements), with their domain and volume indexes
  //     build the list of cells with only a node or an edge on the border, with their domain and volume indexes
  //     build the list of nodes shared by 2 or more domains, with their domain indexes

  std::map<DownIdType, std::map<int,int>, DownIdCompare> faceDomains; // face --> (id domain --> id volume)
  std::map<int,int>celldom; // cell vtkId --> domain
  std::map<DownIdType, std::map<int,int>, DownIdCompare> cellDomains;  // oldNode --> (id domain --> id cell)
  std::map<int, std::map<int,int> > nodeDomains; // oldId -->  (domainId --> newId)
  faceDomains.clear();
  celldom.clear();
  cellDomains.clear();
  nodeDomains.clear();
  std::map<int,int> emptyMap;
  std::set<int> emptySet;
  emptyMap.clear();

  MESSAGE(".. Number of domains :"<<theElems.size());

  TIDSortedElemSet theRestDomElems;
  const int iRestDom  = -1;
  const int idom0     = onAllBoundaries ? iRestDom : 0;
  const int nbDomains = theElems.size();

  // Check if the domains do not share an element
  for (int idom = 0; idom < nbDomains-1; idom++)
    {
//       MESSAGE("... Check of domain #" << idom);
      const TIDSortedElemSet& domain = theElems[idom];
      TIDSortedElemSet::const_iterator elemItr = domain.begin();
      for (; elemItr != domain.end(); ++elemItr)
        {
          const SMDS_MeshElement* anElem = *elemItr;
          int idombisdeb = idom + 1 ;
          for (int idombis = idombisdeb; idombis < theElems.size(); idombis++) // check if the element belongs to a domain further in the list
          {
            const TIDSortedElemSet& domainbis = theElems[idombis];
            if ( domainbis.count(anElem) )
            {
              MESSAGE(".... Domain #" << idom);
              MESSAGE(".... Domain #" << idombis);
              throw SALOME_Exception("The domains are not disjoint.");
              return false ;
            }
          }
        }
    }

  for (int idom = 0; idom < nbDomains; idom++)
    {

      // --- build a map (face to duplicate --> volume to modify)
      //     with all the faces shared by 2 domains (group of elements)
      //     and corresponding volume of this domain, for each shared face.
      //     a volume has a face shared by 2 domains if it has a neighbor which is not in his domain.

      MESSAGE("... Neighbors of domain #" << idom);
      const TIDSortedElemSet& domain = theElems[idom];
      TIDSortedElemSet::const_iterator elemItr = domain.begin();
      for (; elemItr != domain.end(); ++elemItr)
        {
          const SMDS_MeshElement* anElem = *elemItr;
          if (!anElem)
            continue;
          int vtkId = anElem->getVtkId();
          //MESSAGE("  vtkId " << vtkId << " smdsId " << anElem->GetID());
          int neighborsVtkIds[NBMAXNEIGHBORS];
          int downIds[NBMAXNEIGHBORS];
          unsigned char downTypes[NBMAXNEIGHBORS];
          int nbNeighbors = grid->GetNeighbors(neighborsVtkIds, downIds, downTypes, vtkId);
          for (int n = 0; n < nbNeighbors; n++)
            {
              int smdsId = meshDS->fromVtkToSmds(neighborsVtkIds[n]);
              const SMDS_MeshElement* elem = meshDS->FindElement(smdsId);
              if (elem && ! domain.count(elem)) // neighbor is in another domain : face is shared
                {
                  bool ok = false ;
                  for (int idombis = 0; idombis < theElems.size() && !ok; idombis++) // check if the neighbor belongs to another domain of the list
                  {
                    // MESSAGE("Domain " << idombis);
                    const TIDSortedElemSet& domainbis = theElems[idombis];
                    if ( domainbis.count(elem)) ok = true ; // neighbor is in a correct domain : face is kept
                  }
                  if ( ok || onAllBoundaries ) // the characteristics of the face is stored
                  {
                    DownIdType face(downIds[n], downTypes[n]);
                    if (!faceDomains[face].count(idom))
                      {
                        faceDomains[face][idom] = vtkId; // volume associated to face in this domain
                        celldom[vtkId] = idom;
                        //MESSAGE("       cell with a border " << vtkId << " domain " << idom);
                      }
                    if ( !ok )
                    {
                      theRestDomElems.insert( elem );
                      faceDomains[face][iRestDom] = neighborsVtkIds[n];
                      celldom[neighborsVtkIds[n]] = iRestDom;
                    }
                  }
                }
            }
        }
    }

  //MESSAGE("Number of shared faces " << faceDomains.size());
  std::map<DownIdType, std::map<int, int>, DownIdCompare>::iterator itface;

  // --- explore the shared faces domain by domain,
  //     explore the nodes of the face and see if they belong to a cell in the domain,
  //     which has only a node or an edge on the border (not a shared face)

  for (int idomain = idom0; idomain < nbDomains; idomain++)
    {
      //MESSAGE("Domain " << idomain);
      const TIDSortedElemSet& domain = (idomain == iRestDom) ? theRestDomElems : theElems[idomain];
      itface = faceDomains.begin();
      for (; itface != faceDomains.end(); ++itface)
        {
          const std::map<int, int>& domvol = itface->second;
          if (!domvol.count(idomain))
            continue;
          DownIdType face = itface->first;
          //MESSAGE(" --- face " << face.cellId);
          std::set<int> oldNodes;
          oldNodes.clear();
          grid->GetNodeIds(oldNodes, face.cellId, face.cellType);
          std::set<int>::iterator itn = oldNodes.begin();
          for (; itn != oldNodes.end(); ++itn)
            {
              int oldId = *itn;
              //MESSAGE("     node " << oldId);
              vtkCellLinks::Link l = grid->GetCellLinks()->GetLink(oldId);
              for (int i=0; i<l.ncells; i++)
                {
                  int vtkId = l.cells[i];
                  const SMDS_MeshElement* anElem = GetMeshDS()->FindElement(GetMeshDS()->fromVtkToSmds(vtkId));
                  if (!domain.count(anElem))
                    continue;
                  int vtkType = grid->GetCellType(vtkId);
                  int downId = grid->CellIdToDownId(vtkId);
                  if (downId < 0)
                    {
                      MESSAGE("doubleNodesOnGroupBoundaries: internal algorithm problem");
                      continue; // not OK at this stage of the algorithm:
                                //no cells created after BuildDownWardConnectivity
                    }
                  DownIdType aCell(downId, vtkType);
                  cellDomains[aCell][idomain] = vtkId;
                  celldom[vtkId] = idomain;
                  //MESSAGE("       cell " << vtkId << " domain " << idomain);
                }
            }
        }
    }

  // --- explore the shared faces domain by domain, to duplicate the nodes in a coherent way
  //     for each shared face, get the nodes
  //     for each node, for each domain of the face, create a clone of the node

  // --- edges at the intersection of 3 or 4 domains, with the order of domains to build
  //     junction elements of type prism or hexa. the key is the pair of nodesId (lower first)
  //     the value is the ordered domain ids. (more than 4 domains not taken into account)

  std::map<std::vector<int>, std::vector<int> > edgesMultiDomains; // nodes of edge --> ordered domains
  std::map<int, std::vector<int> > mutipleNodes; // nodes multi domains with domain order
  std::map<int, std::vector<int> > mutipleNodesToFace; // nodes multi domains with domain order to transform in Face (junction between 3 or more 2D domains)

  MESSAGE(".. Duplication of the nodes");
  for (int idomain = idom0; idomain < nbDomains; idomain++)
    {
      itface = faceDomains.begin();
      for (; itface != faceDomains.end(); ++itface)
        {
          const std::map<int, int>& domvol = itface->second;
          if (!domvol.count(idomain))
            continue;
          DownIdType face = itface->first;
          //MESSAGE(" --- face " << face.cellId);
          std::set<int> oldNodes;
          oldNodes.clear();
          grid->GetNodeIds(oldNodes, face.cellId, face.cellType);
          std::set<int>::iterator itn = oldNodes.begin();
          for (; itn != oldNodes.end(); ++itn)
            {
              int oldId = *itn;
              if (nodeDomains[oldId].empty())
                {
                  nodeDomains[oldId][idomain] = oldId; // keep the old node in the first domain
                  //MESSAGE("-+-+-b     oldNode " << oldId << " domain " << idomain);
                }
              std::map<int, int>::const_iterator itdom = domvol.begin();
              for (; itdom != domvol.end(); ++itdom)
                {
                  int idom = itdom->first;
                  //MESSAGE("         domain " << idom);
                  if (!nodeDomains[oldId].count(idom)) // --- node to clone
                    {
                      if (nodeDomains[oldId].size() >= 2) // a multiple node
                        {
                          vector<int> orderedDoms;
                          //MESSAGE("multiple node " << oldId);
                          if (mutipleNodes.count(oldId))
                            orderedDoms = mutipleNodes[oldId];
                          else
                            {
                              map<int,int>::iterator it = nodeDomains[oldId].begin();
                              for (; it != nodeDomains[oldId].end(); ++it)
                                orderedDoms.push_back(it->first);
                            }
                          orderedDoms.push_back(idom); // TODO order ==> push_front or back
                          //stringstream txt;
                          //for (int i=0; i<orderedDoms.size(); i++)
                          //  txt << orderedDoms[i] << " ";
                          //MESSAGE("orderedDoms " << txt.str());
                          mutipleNodes[oldId] = orderedDoms;
                        }
                      double *coords = grid->GetPoint(oldId);
                      SMDS_MeshNode *newNode = meshDS->AddNode(coords[0], coords[1], coords[2]);
                      copyPosition( meshDS->FindNodeVtk( oldId ), newNode );
                      int newId = newNode->getVtkId();
                      nodeDomains[oldId][idom] = newId; // cloned node for other domains
                      //MESSAGE("-+-+-c     oldNode " << oldId << " domain " << idomain << " newNode " << newId << " domain " << idom << " size=" <<nodeDomains[oldId].size());
                    }
                }
            }
        }
    }

  MESSAGE(".. Creation of elements");
  for (int idomain = idom0; idomain < nbDomains; idomain++)
    {
      itface = faceDomains.begin();
      for (; itface != faceDomains.end(); ++itface)
        {
          std::map<int, int> domvol = itface->second;
          if (!domvol.count(idomain))
            continue;
          DownIdType face = itface->first;
          //MESSAGE(" --- face " << face.cellId);
          std::set<int> oldNodes;
          oldNodes.clear();
          grid->GetNodeIds(oldNodes, face.cellId, face.cellType);
          int nbMultipleNodes = 0;
          std::set<int>::iterator itn = oldNodes.begin();
          for (; itn != oldNodes.end(); ++itn)
            {
              int oldId = *itn;
              if (mutipleNodes.count(oldId))
                nbMultipleNodes++;
            }
          if (nbMultipleNodes > 1) // check if an edge of the face is shared between 3 or more domains
            {
              //MESSAGE("multiple Nodes detected on a shared face");
              int downId = itface->first.cellId;
              unsigned char cellType = itface->first.cellType;
              // --- shared edge or shared face ?
              if ((cellType == VTK_LINE) || (cellType == VTK_QUADRATIC_EDGE)) // shared edge (between two faces)
                {
                  int nodes[3];
                  int nbNodes = grid->getDownArray(cellType)->getNodes(downId, nodes);
                  for (int i=0; i< nbNodes; i=i+nbNodes-1) // i=0 , i=nbNodes-1
                    if (mutipleNodes.count(nodes[i]))
                      if (!mutipleNodesToFace.count(nodes[i]))
                        mutipleNodesToFace[nodes[i]] = mutipleNodes[nodes[i]];
                }
              else // shared face (between two volumes)
                {
                  int nbEdges = grid->getDownArray(cellType)->getNumberOfDownCells(downId);
                  const int* downEdgeIds = grid->getDownArray(cellType)->getDownCells(downId);
                  const unsigned char* edgeType = grid->getDownArray(cellType)->getDownTypes(downId);
                  for (int ie =0; ie < nbEdges; ie++)
                    {
                      int nodes[3];
                      int nbNodes = grid->getDownArray(edgeType[ie])->getNodes(downEdgeIds[ie], nodes);
                      if (mutipleNodes.count(nodes[0]) && mutipleNodes.count(nodes[nbNodes-1]))
                        {
                          vector<int> vn0 = mutipleNodes[nodes[0]];
                          vector<int> vn1 = mutipleNodes[nodes[nbNodes - 1]];
                          vector<int> doms;
                          for (int i0 = 0; i0 < vn0.size(); i0++)
                            for (int i1 = 0; i1 < vn1.size(); i1++)
                              if (vn0[i0] == vn1[i1])
                                doms.push_back(vn0[i0]);
                          if (doms.size() >2)
                            {
                              //MESSAGE(" detect edgesMultiDomains " << nodes[0] << " " << nodes[nbNodes - 1]);
                              double *coords = grid->GetPoint(nodes[0]);
                              gp_Pnt p0(coords[0], coords[1], coords[2]);
                              coords = grid->GetPoint(nodes[nbNodes - 1]);
                              gp_Pnt p1(coords[0], coords[1], coords[2]);
                              gp_Pnt gref;
                              int vtkVolIds[1000];  // an edge can belong to a lot of volumes
                              map<int, SMDS_VtkVolume*> domvol; // domain --> a volume with the edge
                              map<int, double> angleDom; // oriented angles between planes defined by edge and volume centers
                              int nbvol = grid->GetParentVolumes(vtkVolIds, downEdgeIds[ie], edgeType[ie]);
                              for (int id=0; id < doms.size(); id++)
                                {
                                  int idom = doms[id];
                                  const TIDSortedElemSet& domain = (idom == iRestDom) ? theRestDomElems : theElems[idom];
                                  for (int ivol=0; ivol<nbvol; ivol++)
                                    {
                                      int smdsId = meshDS->fromVtkToSmds(vtkVolIds[ivol]);
                                      SMDS_MeshElement* elem = (SMDS_MeshElement*)meshDS->FindElement(smdsId);
                                      if (domain.count(elem))
                                        {
                                          SMDS_VtkVolume* svol = dynamic_cast<SMDS_VtkVolume*>(elem);
                                          domvol[idom] = svol;
                                          //MESSAGE("  domain " << idom << " volume " << elem->GetID());
                                          double values[3];
                                          vtkIdType npts = 0;
                                          vtkIdType* pts = 0;
                                          grid->GetCellPoints(vtkVolIds[ivol], npts, pts);
                                          SMDS_VtkVolume::gravityCenter(grid, pts, npts, values);
                                          if (id ==0)
                                            {
                                              gref.SetXYZ(gp_XYZ(values[0], values[1], values[2]));
                                              angleDom[idom] = 0;
                                            }
                                          else
                                            {
                                              gp_Pnt g(values[0], values[1], values[2]);
                                              angleDom[idom] = OrientedAngle(p0, p1, gref, g); // -pi<angle<+pi
                                              //MESSAGE("  angle=" << angleDom[idom]);
                                            }
                                          break;
                                        }
                                    }
                                }
                              map<double, int> sortedDom; // sort domains by angle
                              for (map<int, double>::iterator ia = angleDom.begin(); ia != angleDom.end(); ++ia)
                                sortedDom[ia->second] = ia->first;
                              vector<int> vnodes;
                              vector<int> vdom;
                              for (map<double, int>::iterator ib = sortedDom.begin(); ib != sortedDom.end(); ++ib)
                                {
                                  vdom.push_back(ib->second);
                                  //MESSAGE("  ordered domain " << ib->second << "  angle " << ib->first);
                                }
                              for (int ino = 0; ino < nbNodes; ino++)
                                vnodes.push_back(nodes[ino]);
                              edgesMultiDomains[vnodes] = vdom; // nodes vector --> ordered domains
                            }
                        }
                    }
                }
            }
        }
    }

  // --- iterate on shared faces (volumes to modify, face to extrude)
  //     get node id's of the face (id SMDS = id VTK)
  //     create flat element with old and new nodes if requested

  // --- new quad nodes on flat quad elements: oldId --> ((domain1 X domain2) --> newId)
  //     (domain1 X domain2) = domain1 + MAXINT*domain2

  std::map<int, std::map<long,int> > nodeQuadDomains;
  std::map<std::string, SMESH_Group*> mapOfJunctionGroups;

  MESSAGE(".. Creation of elements: simple junction");
  if (createJointElems)
    {
      int idg;
      string joints2DName = "joints2D";
      mapOfJunctionGroups[joints2DName] = this->myMesh->AddGroup(SMDSAbs_Face, joints2DName.c_str(), idg);
      SMESHDS_Group *joints2DGrp = dynamic_cast<SMESHDS_Group*>(mapOfJunctionGroups[joints2DName]->GetGroupDS());
      string joints3DName = "joints3D";
      mapOfJunctionGroups[joints3DName] = this->myMesh->AddGroup(SMDSAbs_Volume, joints3DName.c_str(), idg);
      SMESHDS_Group *joints3DGrp = dynamic_cast<SMESHDS_Group*>(mapOfJunctionGroups[joints3DName]->GetGroupDS());

      itface = faceDomains.begin();
      for (; itface != faceDomains.end(); ++itface)
        {
          DownIdType face = itface->first;
          std::set<int> oldNodes;
          std::set<int>::iterator itn;
          oldNodes.clear();
          grid->GetNodeIds(oldNodes, face.cellId, face.cellType);

          std::map<int, int> domvol = itface->second;
          std::map<int, int>::iterator itdom = domvol.begin();
          int dom1 = itdom->first;
          int vtkVolId = itdom->second;
          itdom++;
          int dom2 = itdom->first;
          SMDS_MeshCell *vol = grid->extrudeVolumeFromFace(vtkVolId, dom1, dom2, oldNodes, nodeDomains,
                                                             nodeQuadDomains);
          stringstream grpname;
          grpname << "j_";
          if (dom1 < dom2)
            grpname << dom1 << "_" << dom2;
          else
            grpname << dom2 << "_" << dom1;
          string namegrp = grpname.str();
          if (!mapOfJunctionGroups.count(namegrp))
            mapOfJunctionGroups[namegrp] = this->myMesh->AddGroup(vol->GetType(), namegrp.c_str(), idg);
          SMESHDS_Group *sgrp = dynamic_cast<SMESHDS_Group*>(mapOfJunctionGroups[namegrp]->GetGroupDS());
          if (sgrp)
            sgrp->Add(vol->GetID());
          if (vol->GetType() == SMDSAbs_Volume)
            joints3DGrp->Add(vol->GetID());
          else if (vol->GetType() == SMDSAbs_Face)
            joints2DGrp->Add(vol->GetID());
        }
    }

  // --- create volumes on multiple domain intersection if requested
  //     iterate on mutipleNodesToFace
  //     iterate on edgesMultiDomains

  MESSAGE(".. Creation of elements: multiple junction");
  if (createJointElems)
    {
      // --- iterate on mutipleNodesToFace

      std::map<int, std::vector<int> >::iterator itn =  mutipleNodesToFace.begin();
      for (; itn != mutipleNodesToFace.end(); ++itn)
        {
          int node = itn->first;
          vector<int> orderDom = itn->second;
          vector<vtkIdType> orderedNodes;
          for (int idom = 0; idom <orderDom.size(); idom++)
              orderedNodes.push_back( nodeDomains[node][orderDom[idom]] );
          SMDS_MeshFace* face = this->GetMeshDS()->AddFaceFromVtkIds(orderedNodes);

          stringstream grpname;
          grpname << "m2j_";
          grpname << 0 << "_" << 0;
          int idg;
          string namegrp = grpname.str();
          if (!mapOfJunctionGroups.count(namegrp))
              mapOfJunctionGroups[namegrp] = this->myMesh->AddGroup(SMDSAbs_Face, namegrp.c_str(), idg);
          SMESHDS_Group *sgrp = dynamic_cast<SMESHDS_Group*>(mapOfJunctionGroups[namegrp]->GetGroupDS());
          if (sgrp)
              sgrp->Add(face->GetID());
        }

      // --- iterate on edgesMultiDomains

      std::map<std::vector<int>, std::vector<int> >::iterator ite = edgesMultiDomains.begin();
      for (; ite != edgesMultiDomains.end(); ++ite)
        {
          vector<int> nodes = ite->first;
          vector<int> orderDom = ite->second;
          vector<vtkIdType> orderedNodes;
          if (nodes.size() == 2)
            {
              //MESSAGE(" use edgesMultiDomains " << nodes[0] << " " << nodes[1]);
              for (int ino=0; ino < nodes.size(); ino++)
                if (orderDom.size() == 3)
                  for (int idom = 0; idom <orderDom.size(); idom++)
                    orderedNodes.push_back( nodeDomains[nodes[ino]][orderDom[idom]] );
                else
                  for (int idom = orderDom.size()-1; idom >=0; idom--)
                    orderedNodes.push_back( nodeDomains[nodes[ino]][orderDom[idom]] );
              SMDS_MeshVolume* vol = this->GetMeshDS()->AddVolumeFromVtkIds(orderedNodes);

              int idg;
              string namegrp = "jointsMultiples";
              if (!mapOfJunctionGroups.count(namegrp))
                mapOfJunctionGroups[namegrp] = this->myMesh->AddGroup(SMDSAbs_Volume, namegrp.c_str(), idg);
              SMESHDS_Group *sgrp = dynamic_cast<SMESHDS_Group*>(mapOfJunctionGroups[namegrp]->GetGroupDS());
              if (sgrp)
                sgrp->Add(vol->GetID());
            }
          else
            {
              //INFOS("Quadratic multiple joints not implemented");
              // TODO quadratic nodes
            }
        }
    }

  // --- list the explicit faces and edges of the mesh that need to be modified,
  //     i.e. faces and edges built with one or more duplicated nodes.
  //     associate these faces or edges to their corresponding domain.
  //     only the first domain found is kept when a face or edge is shared

  std::map<DownIdType, std::map<int,int>, DownIdCompare> faceOrEdgeDom; // cellToModify --> (id domain --> id cell)
  std::map<int,int> feDom; // vtk id of cell to modify --> id domain
  faceOrEdgeDom.clear();
  feDom.clear();

  MESSAGE(".. Modification of elements");
  for (int idomain = idom0; idomain < nbDomains; idomain++)
    {
      std::map<int, std::map<int, int> >::const_iterator itnod = nodeDomains.begin();
      for (; itnod != nodeDomains.end(); ++itnod)
        {
          int oldId = itnod->first;
          //MESSAGE("     node " << oldId);
          vtkCellLinks::Link l = grid->GetCellLinks()->GetLink(oldId);
          for (int i = 0; i < l.ncells; i++)
            {
              int vtkId = l.cells[i];
              int vtkType = grid->GetCellType(vtkId);
              int downId = grid->CellIdToDownId(vtkId);
              if (downId < 0)
                continue; // new cells: not to be modified
              DownIdType aCell(downId, vtkType);
              int volParents[1000];
              int nbvol = grid->GetParentVolumes(volParents, vtkId);
              for (int j = 0; j < nbvol; j++)
                if (celldom.count(volParents[j]) && (celldom[volParents[j]] == idomain))
                  if (!feDom.count(vtkId))
                    {
                      feDom[vtkId] = idomain;
                      faceOrEdgeDom[aCell] = emptyMap;
                      faceOrEdgeDom[aCell][idomain] = vtkId; // affect face or edge to the first domain only
                      //MESSAGE("affect cell " << this->GetMeshDS()->fromVtkToSmds(vtkId) << " domain " << idomain
                      //        << " type " << vtkType << " downId " << downId);
                    }
            }
        }
    }

  // --- iterate on shared faces (volumes to modify, face to extrude)
  //     get node id's of the face
  //     replace old nodes by new nodes in volumes, and update inverse connectivity

  std::map<DownIdType, std::map<int,int>, DownIdCompare>* maps[3] = {&faceDomains, &cellDomains, &faceOrEdgeDom};
  for (int m=0; m<3; m++)
    {
      std::map<DownIdType, std::map<int,int>, DownIdCompare>* amap = maps[m];
      itface = (*amap).begin();
      for (; itface != (*amap).end(); ++itface)
        {
          DownIdType face = itface->first;
          std::set<int> oldNodes;
          std::set<int>::iterator itn;
          oldNodes.clear();
          grid->GetNodeIds(oldNodes, face.cellId, face.cellType);
          //MESSAGE("examine cell, downId " << face.cellId << " type " << int(face.cellType));
          std::map<int, int> localClonedNodeIds;

          std::map<int, int> domvol = itface->second;
          std::map<int, int>::iterator itdom = domvol.begin();
          for (; itdom != domvol.end(); ++itdom)
            {
              int idom = itdom->first;
              int vtkVolId = itdom->second;
              //MESSAGE("modify nodes of cell " << this->GetMeshDS()->fromVtkToSmds(vtkVolId) << " domain " << idom);
              localClonedNodeIds.clear();
              for (itn = oldNodes.begin(); itn != oldNodes.end(); ++itn)
                {
                  int oldId = *itn;
                  if (nodeDomains[oldId].count(idom))
                    {
                      localClonedNodeIds[oldId] = nodeDomains[oldId][idom];
                      //MESSAGE("     node " << oldId << " --> " << localClonedNodeIds[oldId]);
                    }
                }
              meshDS->ModifyCellNodes(vtkVolId, localClonedNodeIds);
            }
        }
    }

  // Remove empty groups (issue 0022812)
  std::map<std::string, SMESH_Group*>::iterator name_group = mapOfJunctionGroups.begin();
  for ( ; name_group != mapOfJunctionGroups.end(); ++name_group )
  {
    if ( name_group->second && name_group->second->GetGroupDS()->IsEmpty() )
      myMesh->RemoveGroup( name_group->second->GetGroupDS()->GetID() );
  }

  meshDS->CleanDownWardConnectivity(); // Mesh has been modified, downward connectivity is no more usable, free memory
  grid->BuildLinks();

  CHRONOSTOP(50);
  counters::stats();
  return true;
}

/*!
 * \brief Double nodes on some external faces and create flat elements.
 * Flat elements are mainly used by some types of mechanic calculations.
 *
 * Each group of the list must be constituted of faces.
 * Triangles are transformed in prisms, and quadrangles in hexahedrons.
 * @param theElems - list of groups of faces, where a group of faces is a set of
 * SMDS_MeshElements sorted by Id.
 * @return TRUE if operation has been completed successfully, FALSE otherwise
 */
bool SMESH_MeshEditor::CreateFlatElementsOnFacesGroups(const std::vector<TIDSortedElemSet>& theElems)
{
  MESSAGE("-------------------------------------------------");
  MESSAGE("SMESH_MeshEditor::CreateFlatElementsOnFacesGroups");
  MESSAGE("-------------------------------------------------");

  SMESHDS_Mesh *meshDS = this->myMesh->GetMeshDS();

  // --- For each group of faces
  //     duplicate the nodes, create a flat element based on the face
  //     replace the nodes of the faces by their clones

  std::map<const SMDS_MeshNode*, const SMDS_MeshNode*> clonedNodes;
  std::map<const SMDS_MeshNode*, const SMDS_MeshNode*> intermediateNodes;
  clonedNodes.clear();
  intermediateNodes.clear();
  std::map<std::string, SMESH_Group*> mapOfJunctionGroups;
  mapOfJunctionGroups.clear();

  for (int idom = 0; idom < theElems.size(); idom++)
    {
      const TIDSortedElemSet& domain = theElems[idom];
      TIDSortedElemSet::const_iterator elemItr = domain.begin();
      for (; elemItr != domain.end(); ++elemItr)
        {
          SMDS_MeshElement* anElem = (SMDS_MeshElement*) *elemItr;
          SMDS_MeshFace* aFace = dynamic_cast<SMDS_MeshFace*> (anElem);
          if (!aFace)
            continue;
          // MESSAGE("aFace=" << aFace->GetID());
          bool isQuad = aFace->IsQuadratic();
          vector<const SMDS_MeshNode*> ln0, ln1, ln2, ln3, ln4;

          // --- clone the nodes, create intermediate nodes for non medium nodes of a quad face

          SMDS_ElemIteratorPtr nodeIt = aFace->nodesIterator();
          while (nodeIt->more())
            {
              const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*> (nodeIt->next());
              bool isMedium = isQuad && (aFace->IsMediumNode(node));
              if (isMedium)
                ln2.push_back(node);
              else
                ln0.push_back(node);

              const SMDS_MeshNode* clone = 0;
              if (!clonedNodes.count(node))
                {
                  clone = meshDS->AddNode(node->X(), node->Y(), node->Z());
                  copyPosition( node, clone );
                  clonedNodes[node] = clone;
                }
              else
                clone = clonedNodes[node];

              if (isMedium)
                ln3.push_back(clone);
              else
                ln1.push_back(clone);

              const SMDS_MeshNode* inter = 0;
              if (isQuad && (!isMedium))
                {
                  if (!intermediateNodes.count(node))
                    {
                      inter = meshDS->AddNode(node->X(), node->Y(), node->Z());
                      copyPosition( node, inter );
                      intermediateNodes[node] = inter;
                    }
                  else
                    inter = intermediateNodes[node];
                  ln4.push_back(inter);
                }
            }

          // --- extrude the face

          vector<const SMDS_MeshNode*> ln;
          SMDS_MeshVolume* vol = 0;
          vtkIdType aType = aFace->GetVtkType();
          switch (aType)
          {
            case VTK_TRIANGLE:
              vol = meshDS->AddVolume(ln0[2], ln0[1], ln0[0], ln1[2], ln1[1], ln1[0]);
              // MESSAGE("vol prism " << vol->GetID());
              ln.push_back(ln1[0]);
              ln.push_back(ln1[1]);
              ln.push_back(ln1[2]);
              break;
            case VTK_QUAD:
              vol = meshDS->AddVolume(ln0[3], ln0[2], ln0[1], ln0[0], ln1[3], ln1[2], ln1[1], ln1[0]);
              // MESSAGE("vol hexa " << vol->GetID());
              ln.push_back(ln1[0]);
              ln.push_back(ln1[1]);
              ln.push_back(ln1[2]);
              ln.push_back(ln1[3]);
              break;
            case VTK_QUADRATIC_TRIANGLE:
              vol = meshDS->AddVolume(ln1[0], ln1[1], ln1[2], ln0[0], ln0[1], ln0[2], ln3[0], ln3[1], ln3[2],
                                      ln2[0], ln2[1], ln2[2], ln4[0], ln4[1], ln4[2]);
              // MESSAGE("vol quad prism " << vol->GetID());
              ln.push_back(ln1[0]);
              ln.push_back(ln1[1]);
              ln.push_back(ln1[2]);
              ln.push_back(ln3[0]);
              ln.push_back(ln3[1]);
              ln.push_back(ln3[2]);
              break;
            case VTK_QUADRATIC_QUAD:
//              vol = meshDS->AddVolume(ln0[0], ln0[1], ln0[2], ln0[3], ln1[0], ln1[1], ln1[2], ln1[3],
//                                      ln2[0], ln2[1], ln2[2], ln2[3], ln3[0], ln3[1], ln3[2], ln3[3],
//                                      ln4[0], ln4[1], ln4[2], ln4[3]);
              vol = meshDS->AddVolume(ln1[0], ln1[1], ln1[2], ln1[3], ln0[0], ln0[1], ln0[2], ln0[3],
                                      ln3[0], ln3[1], ln3[2], ln3[3], ln2[0], ln2[1], ln2[2], ln2[3],
                                      ln4[0], ln4[1], ln4[2], ln4[3]);
              // MESSAGE("vol quad hexa " << vol->GetID());
              ln.push_back(ln1[0]);
              ln.push_back(ln1[1]);
              ln.push_back(ln1[2]);
              ln.push_back(ln1[3]);
              ln.push_back(ln3[0]);
              ln.push_back(ln3[1]);
              ln.push_back(ln3[2]);
              ln.push_back(ln3[3]);
              break;
            case VTK_POLYGON:
              break;
            default:
              break;
          }

          if (vol)
            {
              stringstream grpname;
              grpname << "jf_";
              grpname << idom;
              int idg;
              string namegrp = grpname.str();
              if (!mapOfJunctionGroups.count(namegrp))
                mapOfJunctionGroups[namegrp] = this->myMesh->AddGroup(SMDSAbs_Volume, namegrp.c_str(), idg);
              SMESHDS_Group *sgrp = dynamic_cast<SMESHDS_Group*>(mapOfJunctionGroups[namegrp]->GetGroupDS());
              if (sgrp)
                sgrp->Add(vol->GetID());
            }

          // --- modify the face

          aFace->ChangeNodes(&ln[0], ln.size());
        }
    }
  return true;
}

/*!
 *  \brief identify all the elements around a geom shape, get the faces delimiting the hole
 *  Build groups of volume to remove, groups of faces to replace on the skin of the object,
 *  groups of faces to remove inside the object, (idem edges).
 *  Build ordered list of nodes at the border of each group of faces to replace (to be used to build a geom subshape)
 */
void SMESH_MeshEditor::CreateHoleSkin(double radius,
                                      const TopoDS_Shape& theShape,
                                      SMESH_NodeSearcher* theNodeSearcher,
                                      const char* groupName,
                                      std::vector<double>&   nodesCoords,
                                      std::vector<std::vector<int> >& listOfListOfNodes)
{
  MESSAGE("--------------------------------");
  MESSAGE("SMESH_MeshEditor::CreateHoleSkin");
  MESSAGE("--------------------------------");

  // --- zone of volumes to remove is given :
  //     1 either by a geom shape (one or more vertices) and a radius,
  //     2 either by a group of nodes (representative of the shape)to use with the radius,
  //     3 either by a group of nodes where all the elements build on one of this nodes are to remove,
  //     In the case 2, the group of nodes is an external group of nodes from another mesh,
  //     In the case 3, the group of nodes is an internal group of the mesh (obtained for instance by a filter),
  //     defined by it's name.

  SMESHDS_GroupBase* groupDS = 0;
  SMESH_Mesh::GroupIteratorPtr groupIt = this->myMesh->GetGroups();
  while ( groupIt->more() )
    {
      groupDS = 0;
      SMESH_Group * group = groupIt->next();
      if ( !group ) continue;
      groupDS = group->GetGroupDS();
      if ( !groupDS || groupDS->IsEmpty() ) continue;
      std::string grpName = group->GetName();
      //MESSAGE("grpName=" << grpName);
      if (grpName == groupName)
        break;
      else
        groupDS = 0;
    }

  bool isNodeGroup = false;
  bool isNodeCoords = false;
  if (groupDS)
    {
      if (groupDS->GetType() != SMDSAbs_Node)
        return;
      isNodeGroup = true;     // a group of nodes exists and it is in this mesh
    }

  if (nodesCoords.size() > 0)
    isNodeCoords = true; // a list o nodes given by their coordinates
  //MESSAGE("---" << isNodeGroup << " " << isNodeCoords);

  // --- define groups to build

  int idg; // --- group of SMDS volumes
  string grpvName = groupName;
  grpvName += "_vol";
  SMESH_Group *grp = this->myMesh->AddGroup(SMDSAbs_Volume, grpvName.c_str(), idg);
  if (!grp)
    {
      MESSAGE("group not created " << grpvName);
      return;
    }
  SMESHDS_Group *sgrp = dynamic_cast<SMESHDS_Group*>(grp->GetGroupDS());

  int idgs; // --- group of SMDS faces on the skin
  string grpsName = groupName;
  grpsName += "_skin";
  SMESH_Group *grps = this->myMesh->AddGroup(SMDSAbs_Face, grpsName.c_str(), idgs);
  if (!grps)
    {
      MESSAGE("group not created " << grpsName);
      return;
    }
  SMESHDS_Group *sgrps = dynamic_cast<SMESHDS_Group*>(grps->GetGroupDS());

  int idgi; // --- group of SMDS faces internal (several shapes)
  string grpiName = groupName;
  grpiName += "_internalFaces";
  SMESH_Group *grpi = this->myMesh->AddGroup(SMDSAbs_Face, grpiName.c_str(), idgi);
  if (!grpi)
    {
      MESSAGE("group not created " << grpiName);
      return;
    }
  SMESHDS_Group *sgrpi = dynamic_cast<SMESHDS_Group*>(grpi->GetGroupDS());

  int idgei; // --- group of SMDS faces internal (several shapes)
  string grpeiName = groupName;
  grpeiName += "_internalEdges";
  SMESH_Group *grpei = this->myMesh->AddGroup(SMDSAbs_Edge, grpeiName.c_str(), idgei);
  if (!grpei)
    {
      MESSAGE("group not created " << grpeiName);
      return;
    }
  SMESHDS_Group *sgrpei = dynamic_cast<SMESHDS_Group*>(grpei->GetGroupDS());

  // --- build downward connectivity

  SMESHDS_Mesh *meshDS = this->myMesh->GetMeshDS();
  meshDS->BuildDownWardConnectivity(true);
  SMDS_UnstructuredGrid* grid = meshDS->getGrid();

  // --- set of volumes detected inside

  std::set<int> setOfInsideVol;
  std::set<int> setOfVolToCheck;

  std::vector<gp_Pnt> gpnts;
  gpnts.clear();

  if (isNodeGroup) // --- a group of nodes is provided : find all the volumes using one or more of this nodes
    {
      MESSAGE("group of nodes provided");
      SMDS_ElemIteratorPtr elemIt = groupDS->GetElements();
      while ( elemIt->more() )
        {
          const SMDS_MeshElement* elem = elemIt->next();
          if (!elem)
            continue;
          const SMDS_MeshNode* node = dynamic_cast<const SMDS_MeshNode*>(elem);
          if (!node)
            continue;
          SMDS_MeshElement* vol = 0;
          SMDS_ElemIteratorPtr volItr = node->GetInverseElementIterator(SMDSAbs_Volume);
          while (volItr->more())
            {
              vol = (SMDS_MeshElement*)volItr->next();
              setOfInsideVol.insert(vol->getVtkId());
              sgrp->Add(vol->GetID());
            }
        }
    }
  else if (isNodeCoords)
    {
      MESSAGE("list of nodes coordinates provided");
      int i = 0;
      int k = 0;
      while (i < nodesCoords.size()-2)
        {
          double x = nodesCoords[i++];
          double y = nodesCoords[i++];
          double z = nodesCoords[i++];
          gp_Pnt p = gp_Pnt(x, y ,z);
          gpnts.push_back(p);
          MESSAGE("TopoDS_Vertex " << k << " " << p.X() << " " << p.Y() << " " << p.Z());
          k++;
        }
    }
  else // --- no group, no coordinates : use the vertices of the geom shape provided, and radius
    {
      MESSAGE("no group of nodes provided, using vertices from geom shape, and radius");
      TopTools_IndexedMapOfShape vertexMap;
      TopExp::MapShapes( theShape, TopAbs_VERTEX, vertexMap );
      gp_Pnt p = gp_Pnt(0,0,0);
      if (vertexMap.Extent() < 1)
        return;

      for ( int i = 1; i <= vertexMap.Extent(); ++i )
        {
          const TopoDS_Vertex& vertex = TopoDS::Vertex( vertexMap( i ));
          p = BRep_Tool::Pnt(vertex);
          gpnts.push_back(p);
          MESSAGE("TopoDS_Vertex " << i << " " << p.X() << " " << p.Y() << " " << p.Z());
        }
    }

  if (gpnts.size() > 0)
    {
      int nodeId = 0;
      const SMDS_MeshNode* startNode = theNodeSearcher->FindClosestTo(gpnts[0]);
      if (startNode)
        nodeId = startNode->GetID();
      MESSAGE("nodeId " << nodeId);

      double radius2 = radius*radius;
      MESSAGE("radius2 " << radius2);

      // --- volumes on start node

      setOfVolToCheck.clear();
      SMDS_MeshElement* startVol = 0;
      SMDS_ElemIteratorPtr volItr = startNode->GetInverseElementIterator(SMDSAbs_Volume);
      while (volItr->more())
        {
          startVol = (SMDS_MeshElement*)volItr->next();
          setOfVolToCheck.insert(startVol->getVtkId());
        }
      if (setOfVolToCheck.empty())
        {
          MESSAGE("No volumes found");
          return;
        }

      // --- starting with central volumes then their neighbors, check if they are inside
      //     or outside the domain, until no more new neighbor volume is inside.
      //     Fill the group of inside volumes

      std::map<int, double> mapOfNodeDistance2;
      mapOfNodeDistance2.clear();
      std::set<int> setOfOutsideVol;
      while (!setOfVolToCheck.empty())
        {
          std::set<int>::iterator it = setOfVolToCheck.begin();
          int vtkId = *it;
          MESSAGE("volume to check,  vtkId " << vtkId << " smdsId " << meshDS->fromVtkToSmds(vtkId));
          bool volInside = false;
          vtkIdType npts = 0;
          vtkIdType* pts = 0;
          grid->GetCellPoints(vtkId, npts, pts);
          for (int i=0; i<npts; i++)
            {
              double distance2 = 0;
              if (mapOfNodeDistance2.count(pts[i]))
                {
                  distance2 = mapOfNodeDistance2[pts[i]];
                  MESSAGE("point " << pts[i] << " distance2 " << distance2);
                }
              else
                {
                  double *coords = grid->GetPoint(pts[i]);
                  gp_Pnt aPoint = gp_Pnt(coords[0], coords[1], coords[2]);
                  distance2 = 1.E40;
                  for (int j=0; j<gpnts.size(); j++)
                    {
                      double d2 = aPoint.SquareDistance(gpnts[j]);
                      if (d2 < distance2)
                        {
                          distance2 = d2;
                          if (distance2 < radius2)
                            break;
                        }
                    }
                  mapOfNodeDistance2[pts[i]] = distance2;
                  MESSAGE("  point "  << pts[i]  << " distance2 " << distance2 << " coords " << coords[0] << " " << coords[1] << " " <<  coords[2]);
                }
              if (distance2 < radius2)
                {
                  volInside = true; // one or more nodes inside the domain
                  sgrp->Add(meshDS->fromVtkToSmds(vtkId));
                  break;
                }
            }
          if (volInside)
            {
              setOfInsideVol.insert(vtkId);
              MESSAGE("  volume inside,  vtkId " << vtkId << " smdsId " << meshDS->fromVtkToSmds(vtkId));
              int neighborsVtkIds[NBMAXNEIGHBORS];
              int downIds[NBMAXNEIGHBORS];
              unsigned char downTypes[NBMAXNEIGHBORS];
              int nbNeighbors = grid->GetNeighbors(neighborsVtkIds, downIds, downTypes, vtkId);
              for (int n = 0; n < nbNeighbors; n++)
                if (!setOfInsideVol.count(neighborsVtkIds[n]) ||setOfOutsideVol.count(neighborsVtkIds[n]))
                  setOfVolToCheck.insert(neighborsVtkIds[n]);
            }
          else
            {
              setOfOutsideVol.insert(vtkId);
              MESSAGE("  volume outside, vtkId " << vtkId << " smdsId " << meshDS->fromVtkToSmds(vtkId));
            }
          setOfVolToCheck.erase(vtkId);
        }
    }

  // --- for outside hexahedrons, check if they have more than one neighbor volume inside
  //     If yes, add the volume to the inside set

  bool addedInside = true;
  std::set<int> setOfVolToReCheck;
  while (addedInside)
    {
      MESSAGE(" --------------------------- re check");
      addedInside = false;
      std::set<int>::iterator itv = setOfInsideVol.begin();
      for (; itv != setOfInsideVol.end(); ++itv)
        {
          int vtkId = *itv;
          int neighborsVtkIds[NBMAXNEIGHBORS];
          int downIds[NBMAXNEIGHBORS];
          unsigned char downTypes[NBMAXNEIGHBORS];
          int nbNeighbors = grid->GetNeighbors(neighborsVtkIds, downIds, downTypes, vtkId);
          for (int n = 0; n < nbNeighbors; n++)
            if (!setOfInsideVol.count(neighborsVtkIds[n]))
              setOfVolToReCheck.insert(neighborsVtkIds[n]);
        }
      setOfVolToCheck = setOfVolToReCheck;
      setOfVolToReCheck.clear();
      while  (!setOfVolToCheck.empty())
        {
          std::set<int>::iterator it = setOfVolToCheck.begin();
          int vtkId = *it;
          if (grid->GetCellType(vtkId) == VTK_HEXAHEDRON)
            {
              MESSAGE("volume to recheck,  vtkId " << vtkId << " smdsId " << meshDS->fromVtkToSmds(vtkId));
              int countInside = 0;
              int neighborsVtkIds[NBMAXNEIGHBORS];
              int downIds[NBMAXNEIGHBORS];
              unsigned char downTypes[NBMAXNEIGHBORS];
              int nbNeighbors = grid->GetNeighbors(neighborsVtkIds, downIds, downTypes, vtkId);
              for (int n = 0; n < nbNeighbors; n++)
                if (setOfInsideVol.count(neighborsVtkIds[n]))
                  countInside++;
              MESSAGE("countInside " << countInside);
              if (countInside > 1)
                {
                  MESSAGE("  volume inside,  vtkId " << vtkId << " smdsId " << meshDS->fromVtkToSmds(vtkId));
                  setOfInsideVol.insert(vtkId);
                  sgrp->Add(meshDS->fromVtkToSmds(vtkId));
                  addedInside = true;
                }
              else
                setOfVolToReCheck.insert(vtkId);
            }
          setOfVolToCheck.erase(vtkId);
        }
    }

  // --- map of Downward faces at the boundary, inside the global volume
  //     map of Downward faces on the skin of the global volume (equivalent to SMDS faces on the skin)
  //     fill group of SMDS faces inside the volume (when several volume shapes)
  //     fill group of SMDS faces on the skin of the global volume (if skin)

  std::map<DownIdType, int, DownIdCompare> boundaryFaces; // boundary faces inside the volume --> corresponding cell
  std::map<DownIdType, int, DownIdCompare> skinFaces;     // faces on the skin of the global volume --> corresponding cell
  std::set<int>::iterator it = setOfInsideVol.begin();
  for (; it != setOfInsideVol.end(); ++it)
    {
      int vtkId = *it;
      //MESSAGE("  vtkId " << vtkId  << " smdsId " << meshDS->fromVtkToSmds(vtkId));
      int neighborsVtkIds[NBMAXNEIGHBORS];
      int downIds[NBMAXNEIGHBORS];
      unsigned char downTypes[NBMAXNEIGHBORS];
      int nbNeighbors = grid->GetNeighbors(neighborsVtkIds, downIds, downTypes, vtkId, true);
      for (int n = 0; n < nbNeighbors; n++)
        {
          int neighborDim = SMDS_Downward::getCellDimension(grid->GetCellType(neighborsVtkIds[n]));
          if (neighborDim == 3)
            {
              if (! setOfInsideVol.count(neighborsVtkIds[n])) // neighbor volume is not inside : face is boundary
                {
                  DownIdType face(downIds[n], downTypes[n]);
                  boundaryFaces[face] = vtkId;
                }
              // if the face between to volumes is in the mesh, get it (internal face between shapes)
              int vtkFaceId = grid->getDownArray(downTypes[n])->getVtkCellId(downIds[n]);
              if (vtkFaceId >= 0)
                {
                  sgrpi->Add(meshDS->fromVtkToSmds(vtkFaceId));
                  // find also the smds edges on this face
                  int nbEdges = grid->getDownArray(downTypes[n])->getNumberOfDownCells(downIds[n]);
                  const int* dEdges = grid->getDownArray(downTypes[n])->getDownCells(downIds[n]);
                  const unsigned char* dTypes = grid->getDownArray(downTypes[n])->getDownTypes(downIds[n]);
                  for (int i = 0; i < nbEdges; i++)
                    {
                      int vtkEdgeId = grid->getDownArray(dTypes[i])->getVtkCellId(dEdges[i]);
                      if (vtkEdgeId >= 0)
                        sgrpei->Add(meshDS->fromVtkToSmds(vtkEdgeId));
                    }
                }
            }
          else if (neighborDim == 2) // skin of the volume
            {
              DownIdType face(downIds[n], downTypes[n]);
              skinFaces[face] = vtkId;
              int vtkFaceId = grid->getDownArray(downTypes[n])->getVtkCellId(downIds[n]);
              if (vtkFaceId >= 0)
                sgrps->Add(meshDS->fromVtkToSmds(vtkFaceId));
            }
        }
    }

  // --- identify the edges constituting the wire of each subshape on the skin
  //     define polylines with the nodes of edges, equivalent to wires
  //     project polylines on subshapes, and partition, to get geom faces

  std::map<int, std::set<int> > shapeIdToVtkIdSet; // shapeId --> set of vtkId on skin
  std::set<int> emptySet;
  emptySet.clear();
  std::set<int> shapeIds;

  SMDS_ElemIteratorPtr itelem = sgrps->GetElements();
  while (itelem->more())
    {
      const SMDS_MeshElement *elem = itelem->next();
      int shapeId = elem->getshapeId();
      int vtkId = elem->getVtkId();
      if (!shapeIdToVtkIdSet.count(shapeId))
        {
          shapeIdToVtkIdSet[shapeId] = emptySet;
          shapeIds.insert(shapeId);
        }
      shapeIdToVtkIdSet[shapeId].insert(vtkId);
    }

  std::map<int, std::set<DownIdType, DownIdCompare> > shapeIdToEdges; // shapeId --> set of downward edges
  std::set<DownIdType, DownIdCompare> emptyEdges;
  emptyEdges.clear();

  std::map<int, std::set<int> >::iterator itShape =  shapeIdToVtkIdSet.begin();
  for (; itShape != shapeIdToVtkIdSet.end(); ++itShape)
    {
      int shapeId = itShape->first;
      MESSAGE(" --- Shape ID --- "<< shapeId);
      shapeIdToEdges[shapeId] = emptyEdges;

      std::vector<int> nodesEdges;

      std::set<int>::iterator its = itShape->second.begin();
      for (; its != itShape->second.end(); ++its)
        {
          int vtkId = *its;
          MESSAGE("     " << vtkId);
          int neighborsVtkIds[NBMAXNEIGHBORS];
          int downIds[NBMAXNEIGHBORS];
          unsigned char downTypes[NBMAXNEIGHBORS];
          int nbNeighbors = grid->GetNeighbors(neighborsVtkIds, downIds, downTypes, vtkId);
          for (int n = 0; n < nbNeighbors; n++)
            {
              if (neighborsVtkIds[n]<0) // only smds faces are considered as neighbors here
                continue;
              int smdsId = meshDS->fromVtkToSmds(neighborsVtkIds[n]);
              const SMDS_MeshElement* elem = meshDS->FindElement(smdsId);
              if ( shapeIds.count(elem->getshapeId()) && !sgrps->Contains(elem)) // edge : neighbor in the set of shape, not in the group
                {
                  DownIdType edge(downIds[n], downTypes[n]);
                  if (!shapeIdToEdges[shapeId].count(edge))
                    {
                      shapeIdToEdges[shapeId].insert(edge);
                      int vtkNodeId[3];
                      int nbNodes = grid->getDownArray(downTypes[n])->getNodes(downIds[n],vtkNodeId);
                      nodesEdges.push_back(vtkNodeId[0]);
                      nodesEdges.push_back(vtkNodeId[nbNodes-1]);
                      MESSAGE("       --- nodes " << vtkNodeId[0]+1 << " " << vtkNodeId[nbNodes-1]+1);
                    }
                }
            }
        }

      std::list<int> order;
      order.clear();
      if (nodesEdges.size() > 0)
        {
          order.push_back(nodesEdges[0]); MESSAGE("       --- back " << order.back()+1); // SMDS id = VTK id + 1;
          nodesEdges[0] = -1;
          order.push_back(nodesEdges[1]); MESSAGE("       --- back " << order.back()+1);
          nodesEdges[1] = -1; // do not reuse this edge
          bool found = true;
          while (found)
            {
              int nodeTofind = order.back(); // try first to push back
              int i = 0;
              for (i = 0; i<nodesEdges.size(); i++)
                if (nodesEdges[i] == nodeTofind)
                  break;
              if (i == nodesEdges.size())
                found = false; // no follower found on back
              else
                {
                  if (i%2) // odd ==> use the previous one
                    if (nodesEdges[i-1] < 0)
                      found = false;
                    else
                      {
                        order.push_back(nodesEdges[i-1]); MESSAGE("       --- back " << order.back()+1);
                        nodesEdges[i-1] = -1;
                      }
                  else // even ==> use the next one
                    if (nodesEdges[i+1] < 0)
                      found = false;
                    else
                      {
                        order.push_back(nodesEdges[i+1]); MESSAGE("       --- back " << order.back()+1);
                        nodesEdges[i+1] = -1;
                      }
                }
              if (found)
                continue;
              // try to push front
              found = true;
              nodeTofind = order.front(); // try to push front
              for (i = 0; i<nodesEdges.size(); i++)
                if (nodesEdges[i] == nodeTofind)
                  break;
              if (i == nodesEdges.size())
                {
                  found = false; // no predecessor found on front
                  continue;
                }
              if (i%2) // odd ==> use the previous one
                if (nodesEdges[i-1] < 0)
                  found = false;
                else
                  {
                    order.push_front(nodesEdges[i-1]); MESSAGE("       --- front " << order.front()+1);
                    nodesEdges[i-1] = -1;
                  }
              else // even ==> use the next one
                if (nodesEdges[i+1] < 0)
                  found = false;
                else
                  {
                    order.push_front(nodesEdges[i+1]); MESSAGE("       --- front " << order.front()+1);
                    nodesEdges[i+1] = -1;
                  }
            }
        }


      std::vector<int> nodes;
      nodes.push_back(shapeId);
      std::list<int>::iterator itl = order.begin();
      for (; itl != order.end(); itl++)
        {
          nodes.push_back((*itl) + 1); // SMDS id = VTK id + 1;
          MESSAGE("              ordered node " << nodes[nodes.size()-1]);
        }
      listOfListOfNodes.push_back(nodes);
    }

  //     partition geom faces with blocFissure
  //     mesh blocFissure and geom faces of the skin (external wires given, triangle algo to choose)
  //     mesh volume around blocFissure (skin triangles and quadrangle given, tetra algo to choose)

  return;
}


//================================================================================
/*!
 * \brief Generates skin mesh (containing 2D cells) from 3D mesh
 * The created 2D mesh elements based on nodes of free faces of boundary volumes
 * \return TRUE if operation has been completed successfully, FALSE otherwise
 */
//================================================================================

bool SMESH_MeshEditor::Make2DMeshFrom3D()
{
  // iterates on volume elements and detect all free faces on them
  SMESHDS_Mesh* aMesh = GetMeshDS();
  if (!aMesh)
    return false;

  ElemFeatures faceType( SMDSAbs_Face );
  int nbFree = 0, nbExisted = 0, nbCreated = 0;
  SMDS_VolumeIteratorPtr vIt = aMesh->volumesIterator();
  while(vIt->more())
  {
    const SMDS_MeshVolume* volume = vIt->next();
    SMDS_VolumeTool vTool( volume, /*ignoreCentralNodes=*/false );
    vTool.SetExternalNormal();
    const int iQuad = volume->IsQuadratic();
    faceType.SetQuad( iQuad );
    for ( int iface = 0, n = vTool.NbFaces(); iface < n; iface++ )
    {
      if (!vTool.IsFreeFace(iface))
        continue;
      nbFree++;
      vector<const SMDS_MeshNode *> nodes;
      int nbFaceNodes = vTool.NbFaceNodes(iface);
      const SMDS_MeshNode** faceNodes = vTool.GetFaceNodes(iface);
      int inode = 0;
      for ( ; inode < nbFaceNodes; inode += iQuad+1)
        nodes.push_back(faceNodes[inode]);

      if (iQuad) // add medium nodes
      {
        for ( inode = 1; inode < nbFaceNodes; inode += 2)
          nodes.push_back(faceNodes[inode]);
        if ( nbFaceNodes == 9 ) // bi-quadratic quad
          nodes.push_back(faceNodes[8]);
      }
      // add new face based on volume nodes
      if (aMesh->FindElement( nodes, SMDSAbs_Face, /*noMedium=*/false) )
      {
        nbExisted++; // face already exsist
      }
      else
      {
        AddElement( nodes, faceType.SetPoly( nbFaceNodes/(iQuad+1) > 4 ));
        nbCreated++;
      }
    }
  }
  return ( nbFree == ( nbExisted + nbCreated ));
}

namespace
{
  inline const SMDS_MeshNode* getNodeWithSameID(SMESHDS_Mesh* mesh, const SMDS_MeshNode* node)
  {
    if ( const SMDS_MeshNode* n = mesh->FindNode( node->GetID() ))
      return n;
    return mesh->AddNodeWithID( node->X(),node->Y(),node->Z(), node->GetID() );
  }
}
//================================================================================
/*!
 * \brief Creates missing boundary elements
 *  \param elements - elements whose boundary is to be checked
 *  \param dimension - defines type of boundary elements to create
 *  \param group - a group to store created boundary elements in
 *  \param targetMesh - a mesh to store created boundary elements in
 *  \param toCopyElements - if true, the checked elements will be copied into the targetMesh
 *  \param toCopyExistingBoundary - if true, not only new but also pre-existing
 *                                boundary elements will be copied into the targetMesh
 *  \param toAddExistingBondary - if true, not only new but also pre-existing
 *                                boundary elements will be added into the new group
 *  \param aroundElements - if true, elements will be created on boundary of given
 *                          elements else, on boundary of the whole mesh.
 * \return nb of added boundary elements
 */
//================================================================================

int SMESH_MeshEditor::MakeBoundaryMesh(const TIDSortedElemSet& elements,
                                       Bnd_Dimension           dimension,
                                       SMESH_Group*            group/*=0*/,
                                       SMESH_Mesh*             targetMesh/*=0*/,
                                       bool                    toCopyElements/*=false*/,
                                       bool                    toCopyExistingBoundary/*=false*/,
                                       bool                    toAddExistingBondary/*= false*/,
                                       bool                    aroundElements/*= false*/)
{
  SMDSAbs_ElementType missType = (dimension == BND_2DFROM3D) ? SMDSAbs_Face : SMDSAbs_Edge;
  SMDSAbs_ElementType elemType = (dimension == BND_1DFROM2D) ? SMDSAbs_Face : SMDSAbs_Volume;
  // hope that all elements are of the same type, do not check them all
  if ( !elements.empty() && (*elements.begin())->GetType() != elemType )
    throw SALOME_Exception(LOCALIZED("wrong element type"));

  if ( !targetMesh )
    toCopyElements = toCopyExistingBoundary = false;

  SMESH_MeshEditor tgtEditor( targetMesh ? targetMesh : myMesh );
  SMESHDS_Mesh* aMesh = GetMeshDS(), *tgtMeshDS = tgtEditor.GetMeshDS();
  int nbAddedBnd = 0;

  // editor adding present bnd elements and optionally holding elements to add to the group
  SMESH_MeshEditor* presentEditor;
  SMESH_MeshEditor tgtEditor2( tgtEditor.GetMesh() );
  presentEditor = toAddExistingBondary ? &tgtEditor : &tgtEditor2;

  SMESH_MesherHelper helper( *myMesh );
  const TopAbs_ShapeEnum missShapeType = ( missType==SMDSAbs_Face ? TopAbs_FACE : TopAbs_EDGE );
  SMDS_VolumeTool vTool;
  TIDSortedElemSet avoidSet;
  const TIDSortedElemSet emptySet, *elemSet = aroundElements ? &elements : &emptySet;
  size_t inode;

  typedef vector<const SMDS_MeshNode*> TConnectivity;
  TConnectivity tgtNodes;
  ElemFeatures elemKind( missType ), elemToCopy;

  vector<const SMDS_MeshElement*> presentBndElems;
  vector<TConnectivity>           missingBndElems;
  vector<int>                     freeFacets;
  TConnectivity nodes, elemNodes;

  SMDS_ElemIteratorPtr eIt;
  if (elements.empty()) eIt = aMesh->elementsIterator(elemType);
  else                  eIt = elemSetIterator( elements );

  while (eIt->more())
  {
    const SMDS_MeshElement* elem = eIt->next();
    const int              iQuad = elem->IsQuadratic();
    elemKind.SetQuad( iQuad );

    // ------------------------------------------------------------------------------------
    // 1. For an elem, get present bnd elements and connectivities of missing bnd elements
    // ------------------------------------------------------------------------------------
    presentBndElems.clear();
    missingBndElems.clear();
    freeFacets.clear(); nodes.clear(); elemNodes.clear();
    if ( vTool.Set(elem, /*ignoreCentralNodes=*/true) ) // elem is a volume --------------
    {
      const SMDS_MeshElement* otherVol = 0;
      for ( int iface = 0, n = vTool.NbFaces(); iface < n; iface++ )
      {
        if ( !vTool.IsFreeFace(iface, &otherVol) &&
             ( !aroundElements || elements.count( otherVol )))
          continue;
        freeFacets.push_back( iface );
      }
      if ( missType == SMDSAbs_Face )
        vTool.SetExternalNormal();
      for ( size_t i = 0; i < freeFacets.size(); ++i )
      {
        int                iface = freeFacets[i];
        const SMDS_MeshNode** nn = vTool.GetFaceNodes(iface);
        const size_t nbFaceNodes = vTool.NbFaceNodes (iface);
        if ( missType == SMDSAbs_Edge ) // boundary edges
        {
          nodes.resize( 2+iQuad );
          for ( int i = 0; i < nbFaceNodes; i += 1+iQuad)
          {
            for ( int j = 0; j < nodes.size(); ++j )
              nodes[j] = nn[ i+j ];
            if ( const SMDS_MeshElement* edge =
                 aMesh->FindElement( nodes, SMDSAbs_Edge, /*noMedium=*/false ))
              presentBndElems.push_back( edge );
            else
              missingBndElems.push_back( nodes );
          }
        }
        else // boundary face
        {
          nodes.clear();
          for ( inode = 0; inode < nbFaceNodes; inode += 1+iQuad)
            nodes.push_back( nn[inode] ); // add corner nodes
          if (iQuad)
            for ( inode = 1; inode < nbFaceNodes; inode += 2)
              nodes.push_back( nn[inode] ); // add medium nodes
          int iCenter = vTool.GetCenterNodeIndex(iface); // for HEX27
          if ( iCenter > 0 )
            nodes.push_back( vTool.GetNodes()[ iCenter ] );

          if (const SMDS_MeshElement * f = aMesh->FindElement( nodes,
                                                               SMDSAbs_Face, /*noMedium=*/false ))
            presentBndElems.push_back( f );
          else
            missingBndElems.push_back( nodes );

          if ( targetMesh != myMesh )
          {
            // add 1D elements on face boundary to be added to a new mesh
            const SMDS_MeshElement* edge;
            for ( inode = 0; inode < nbFaceNodes; inode += 1+iQuad)
            {
              if ( iQuad )
                edge = aMesh->FindEdge( nn[inode], nn[inode+1], nn[inode+2]);
              else
                edge = aMesh->FindEdge( nn[inode], nn[inode+1]);
              if ( edge && avoidSet.insert( edge ).second )
                presentBndElems.push_back( edge );
            }
          }
        }
      }
    }
    else if ( elem->GetType() == SMDSAbs_Face ) // elem is a face ------------------------
    {
      avoidSet.clear(), avoidSet.insert( elem );
      elemNodes.assign( SMDS_MeshElement::iterator( elem->interlacedNodesElemIterator() ),
                        SMDS_MeshElement::iterator() );
      elemNodes.push_back( elemNodes[0] );
      nodes.resize( 2 + iQuad );
      const int nbLinks = elem->NbCornerNodes();
      for ( int i = 0, iN = 0; i < nbLinks; i++, iN += 1+iQuad )
      {
        nodes[0] = elemNodes[iN];
        nodes[1] = elemNodes[iN+1+iQuad];
        if ( SMESH_MeshAlgos::FindFaceInSet( nodes[0], nodes[1], *elemSet, avoidSet))
          continue; // not free link

        if ( iQuad ) nodes[2] = elemNodes[iN+1];
        if ( const SMDS_MeshElement* edge =
             aMesh->FindElement(nodes,SMDSAbs_Edge,/*noMedium=*/false))
          presentBndElems.push_back( edge );
        else
          missingBndElems.push_back( nodes );
      }
    }

    // ---------------------------------
    // 2. Add missing boundary elements
    // ---------------------------------
    if ( targetMesh != myMesh )
      // instead of making a map of nodes in this mesh and targetMesh,
      // we create nodes with same IDs.
      for ( size_t i = 0; i < missingBndElems.size(); ++i )
      {
        TConnectivity& srcNodes = missingBndElems[i];
        tgtNodes.resize( srcNodes.size() );
        for ( inode = 0; inode < srcNodes.size(); ++inode )
          tgtNodes[inode] = getNodeWithSameID( tgtMeshDS, srcNodes[inode] );
        if ( aroundElements && tgtEditor.GetMeshDS()->FindElement( tgtNodes,
                                                                   missType,
                                                                   /*noMedium=*/false))
          continue;
        tgtEditor.AddElement( tgtNodes, elemKind.SetPoly( tgtNodes.size()/(iQuad+1) > 4 ));
        ++nbAddedBnd;
      }
    else
      for ( int i = 0; i < missingBndElems.size(); ++i )
      {
        TConnectivity& nodes = missingBndElems[i];
        if ( aroundElements && tgtEditor.GetMeshDS()->FindElement( nodes,
                                                                   missType,
                                                                   /*noMedium=*/false))
          continue;
        SMDS_MeshElement* newElem = 
          tgtEditor.AddElement( nodes, elemKind.SetPoly( nodes.size()/(iQuad+1) > 4 ));
        nbAddedBnd += bool( newElem );

        // try to set a new element to a shape
        if ( myMesh->HasShapeToMesh() )
        {
          bool ok = true;
          set< pair<TopAbs_ShapeEnum, int > > mediumShapes;
          const size_t nbN = nodes.size() / (iQuad+1 );
          for ( inode = 0; inode < nbN && ok; ++inode )
          {
            pair<int, TopAbs_ShapeEnum> i_stype =
              helper.GetMediumPos( nodes[inode], nodes[(inode+1)%nbN]);
            if (( ok = ( i_stype.first > 0 && i_stype.second >= TopAbs_FACE )))
              mediumShapes.insert( make_pair ( i_stype.second, i_stype.first ));
          }
          if ( ok && mediumShapes.size() > 1 )
          {
            set< pair<TopAbs_ShapeEnum, int > >::iterator stype_i = mediumShapes.begin();
            pair<TopAbs_ShapeEnum, int> stype_i_0 = *stype_i;
            for ( ++stype_i; stype_i != mediumShapes.end() && ok; ++stype_i )
            {
              if (( ok = ( stype_i->first != stype_i_0.first )))
                ok = helper.IsSubShape( aMesh->IndexToShape( stype_i->second ),
                                        aMesh->IndexToShape( stype_i_0.second ));
            }
          }
          if ( ok && mediumShapes.begin()->first == missShapeType )
            aMesh->SetMeshElementOnShape( newElem, mediumShapes.begin()->second );
        }
      }

    // ----------------------------------
    // 3. Copy present boundary elements
    // ----------------------------------
    if ( toCopyExistingBoundary )
      for ( int i = 0 ; i < presentBndElems.size(); ++i )
      {
        const SMDS_MeshElement* e = presentBndElems[i];
        tgtNodes.resize( e->NbNodes() );
        for ( inode = 0; inode < nodes.size(); ++inode )
          tgtNodes[inode] = getNodeWithSameID( tgtMeshDS, e->GetNode(inode) );
        presentEditor->AddElement( tgtNodes, elemToCopy.Init( e ));
      }
    else // store present elements to add them to a group
      for ( int i = 0 ; i < presentBndElems.size(); ++i )
      {
        presentEditor->myLastCreatedElems.Append( presentBndElems[i] );
      }

  } // loop on given elements

  // ---------------------------------------------
  // 4. Fill group with boundary elements
  // ---------------------------------------------
  if ( group )
  {
    if ( SMESHDS_Group* g = dynamic_cast<SMESHDS_Group*>( group->GetGroupDS() ))
      for ( int i = 0; i < tgtEditor.myLastCreatedElems.Size(); ++i )
        g->SMDSGroup().Add( tgtEditor.myLastCreatedElems( i+1 ));
  }
  tgtEditor.myLastCreatedElems.Clear();
  tgtEditor2.myLastCreatedElems.Clear();

  // -----------------------
  // 5. Copy given elements
  // -----------------------
  if ( toCopyElements && targetMesh != myMesh )
  {
    if (elements.empty()) eIt = aMesh->elementsIterator(elemType);
    else                  eIt = elemSetIterator( elements );
    while (eIt->more())
    {
      const SMDS_MeshElement* elem = eIt->next();
      tgtNodes.resize( elem->NbNodes() );
      for ( inode = 0; inode < tgtNodes.size(); ++inode )
        tgtNodes[inode] = getNodeWithSameID( tgtMeshDS, elem->GetNode(inode) );
      tgtEditor.AddElement( tgtNodes, elemToCopy.Init( elem ));

      tgtEditor.myLastCreatedElems.Clear();
    }
  }
  return nbAddedBnd;
}

//================================================================================
/*!
 * \brief Copy node position and set \a to node on the same geometry
 */
//================================================================================

void SMESH_MeshEditor::copyPosition( const SMDS_MeshNode* from,
                                     const SMDS_MeshNode* to )
{
  if ( !from || !to ) return;

  SMDS_PositionPtr pos = from->GetPosition();
  if ( !pos || from->getshapeId() < 1 ) return;

  switch ( pos->GetTypeOfPosition() )
  {
  case SMDS_TOP_3DSPACE: break;

  case SMDS_TOP_FACE:
  {
    const SMDS_FacePosition* fPos = static_cast< const SMDS_FacePosition* >( pos );
    GetMeshDS()->SetNodeOnFace( to, from->getshapeId(),
                                fPos->GetUParameter(), fPos->GetVParameter() );
    break;
  }
  case SMDS_TOP_EDGE:
  {
    // WARNING: it is dangerous to set equal nodes on one EDGE!!!!!!!!
    const SMDS_EdgePosition* ePos = static_cast< const SMDS_EdgePosition* >( pos );
    GetMeshDS()->SetNodeOnEdge( to, from->getshapeId(), ePos->GetUParameter() );
    break;
  }
  case SMDS_TOP_VERTEX:
  {
    GetMeshDS()->SetNodeOnVertex( to, from->getshapeId() );
    break;
  }
  case SMDS_TOP_UNSPEC:
  default:;
  }
}

#if defined(__clang__)
# pragma clang diagnostic pop
#endif
