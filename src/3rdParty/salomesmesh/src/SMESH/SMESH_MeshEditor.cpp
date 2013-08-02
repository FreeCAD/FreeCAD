//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMESH : idl implementation based on 'SMESH' unit's classes
// File      : SMESH_MeshEditor.cxx
// Created   : Mon Apr 12 16:10:22 2004
// Author    : Edward AGAPOV (eap)
//
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif // _MSC_VER
#include <cmath>

#include "SMESH_MeshEditor.hxx"

#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMDS_EdgePosition.hxx"
#include "SMDS_PolyhedralVolumeOfNodes.hxx"
#include "SMDS_FacePosition.hxx"
#include "SMDS_SpacePosition.hxx"
#include "SMDS_QuadraticFaceOfNodes.hxx"
#include "SMDS_MeshGroup.hxx"

#include "SMESHDS_Group.hxx"
#include "SMESHDS_Mesh.hxx"

#include "SMESH_subMesh.hxx"
#include "SMESH_ControlsDef.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_OctreeNode.hxx"
#include "SMESH_Group.hxx"

#include "utilities.h"

#include <BRep_Tool.hxx>
#include <ElCLib.hxx>
#include <Extrema_GenExtPS.hxx>
#include <Extrema_POnSurf.hxx>
#include <Geom2d_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Standard_Version.hxx>
#include <TColStd_ListOfInteger.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_XY.hxx>
#include <gp_XYZ.hxx>
#include <math.h>

#include <map>
#include <set>

#ifndef PI
#define PI M_PI
#endif

#define cast2Node(elem) static_cast<const SMDS_MeshNode*>( elem )

using namespace std;
using namespace SMESH::Controls;

typedef map<const SMDS_MeshElement*, list<const SMDS_MeshNode*> >    TElemOfNodeListMap;
typedef map<const SMDS_MeshElement*, list<const SMDS_MeshElement*> > TElemOfElemListMap;
//typedef map<const SMDS_MeshNode*, vector<const SMDS_MeshNode*> >     TNodeOfNodeVecMap;
//typedef TNodeOfNodeVecMap::iterator                                  TNodeOfNodeVecMapItr;
//typedef map<const SMDS_MeshElement*, vector<TNodeOfNodeVecMapItr> >  TElemOfVecOfMapNodesMap;

struct TNodeXYZ : public gp_XYZ {
  TNodeXYZ( const SMDS_MeshNode* n ):gp_XYZ( n->X(), n->Y(), n->Z() ) {}
};

//=======================================================================
//function : SMESH_MeshEditor
//purpose  :
//=======================================================================

SMESH_MeshEditor::SMESH_MeshEditor( SMESH_Mesh* theMesh )
  :myMesh( theMesh ) // theMesh may be NULL
{
}

//=======================================================================
/*!
 * \brief Add element
 */
//=======================================================================

SMDS_MeshElement*
SMESH_MeshEditor::AddElement(const vector<const SMDS_MeshNode*> & node,
                             const SMDSAbs_ElementType            type,
                             const bool                           isPoly,
                             const int                            ID)
{
  SMDS_MeshElement* e = 0;
  int nbnode = node.size();
  SMESHDS_Mesh* mesh = GetMeshDS();
  switch ( type ) {
  case SMDSAbs_Edge:
    if ( nbnode == 2 )
      if ( ID ) e = mesh->AddEdgeWithID(node[0], node[1], ID);
      else      e = mesh->AddEdge      (node[0], node[1] );
    else if ( nbnode == 3 )
      if ( ID ) e = mesh->AddEdgeWithID(node[0], node[1], node[2], ID);
      else      e = mesh->AddEdge      (node[0], node[1], node[2] );
    break;
  case SMDSAbs_Face:
    if ( !isPoly ) {
      if      (nbnode == 3)
        if ( ID ) e = mesh->AddFaceWithID(node[0], node[1], node[2], ID);
        else      e = mesh->AddFace      (node[0], node[1], node[2] );
      else if (nbnode == 4) 
        if ( ID ) e = mesh->AddFaceWithID(node[0], node[1], node[2], node[3], ID);
        else      e = mesh->AddFace      (node[0], node[1], node[2], node[3] );
      else if (nbnode == 6)
        if ( ID ) e = mesh->AddFaceWithID(node[0], node[1], node[2], node[3],
                                          node[4], node[5], ID);
        else      e = mesh->AddFace      (node[0], node[1], node[2], node[3],
                                          node[4], node[5] );
      else if (nbnode == 8)
        if ( ID ) e = mesh->AddFaceWithID(node[0], node[1], node[2], node[3],
                                          node[4], node[5], node[6], node[7], ID);
        else      e = mesh->AddFace      (node[0], node[1], node[2], node[3],
                                          node[4], node[5], node[6], node[7] );
    } else {
      if ( ID ) e = mesh->AddPolygonalFaceWithID(node, ID);
      else      e = mesh->AddPolygonalFace      (node    );
    }
    break;
  case SMDSAbs_Volume:
    if ( !isPoly ) {
      if      (nbnode == 4)
        if ( ID ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3], ID);
        else      e = mesh->AddVolume      (node[0], node[1], node[2], node[3] );
      else if (nbnode == 5)
        if ( ID ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                            node[4], ID);
        else      e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                            node[4] );
      else if (nbnode == 6)
        if ( ID ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                            node[4], node[5], ID);
        else      e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                            node[4], node[5] );
      else if (nbnode == 8)
        if ( ID ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7], ID);
        else      e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7] );
      else if (nbnode == 10)
        if ( ID ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7],
                                            node[8], node[9], ID);
        else      e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7],
                                            node[8], node[9] );
      else if (nbnode == 13)
        if ( ID ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7],
                                            node[8], node[9], node[10],node[11],
                                            node[12],ID);
        else      e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7],
                                            node[8], node[9], node[10],node[11],
                                            node[12] );
      else if (nbnode == 15)
        if ( ID ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7],
                                            node[8], node[9], node[10],node[11],
                                            node[12],node[13],node[14],ID);
        else      e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7],
                                            node[8], node[9], node[10],node[11],
                                            node[12],node[13],node[14] );
      else if (nbnode == 20)
        if ( ID ) e = mesh->AddVolumeWithID(node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7],
                                            node[8], node[9], node[10],node[11],
                                            node[12],node[13],node[14],node[15],
                                            node[16],node[17],node[18],node[19],ID);
        else      e = mesh->AddVolume      (node[0], node[1], node[2], node[3],
                                            node[4], node[5], node[6], node[7],
                                            node[8], node[9], node[10],node[11],
                                            node[12],node[13],node[14],node[15],
                                            node[16],node[17],node[18],node[19] );
    }
  }
  return e;
}

//=======================================================================
/*!
 * \brief Add element
 */
//=======================================================================

SMDS_MeshElement* SMESH_MeshEditor::AddElement(const vector<int> &       nodeIDs,
                                               const SMDSAbs_ElementType type,
                                               const bool                isPoly,
                                               const int                 ID)
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
  return AddElement( nodes, type, isPoly, ID );
}

//=======================================================================
//function : Remove
//purpose  : Remove a node or an element.
//           Modify a compute state of sub-meshes which become empty
//=======================================================================

bool SMESH_MeshEditor::Remove (const list< int >& theIDs,
                               const bool         isNodes )
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  SMESHDS_Mesh* aMesh = GetMeshDS();
  set< SMESH_subMesh *> smmap;

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
        if ( int aShapeID = node->GetPosition()->GetShapeId() )
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

  return true;
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

  if ( theElem->GetType() == SMDSAbs_Node ) {
    const SMDS_PositionPtr& aPosition =
      static_cast<const SMDS_MeshNode*>( theElem )->GetPosition();
    if ( aPosition.get() )
      return aPosition->GetShapeId();
    else
      return 0;
  }

  TopoDS_Shape aShape; // the shape a node is on
  SMDS_ElemIteratorPtr nodeIt = theElem->nodesIterator();
  while ( nodeIt->more() ) {
    const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
    const SMDS_PositionPtr& aPosition = node->GetPosition();
    if ( aPosition.get() ) {
      int aShapeID = aPosition->GetShapeId();
      SMESHDS_SubMesh * sm = aMesh->MeshElements( aShapeID );
      if ( sm ) {
        if ( sm->Contains( theElem ))
          return aShapeID;
        if ( aShape.IsNull() )
          aShape = aMesh->IndexToShape( aShapeID );
      }
      else {
        //MESSAGE ( "::FindShape() No SubShape for aShapeID " << aShapeID );
      }
    }
  }

  // None of nodes is on a proper shape,
  // find the shape among ancestors of aShape on which a node is
  if ( aShape.IsNull() ) {
    //MESSAGE ("::FindShape() - NONE node is on shape")
    return 0;
  }
  TopTools_ListIteratorOfListOfShape ancIt( GetMesh()->GetAncestors( aShape ));
  for ( ; ancIt.More(); ancIt.Next() ) {
    SMESHDS_SubMesh * sm = aMesh->MeshElements( ancIt.Value() );
    if ( sm && sm->Contains( theElem ))
      return aMesh->ShapeToIndex( ancIt.Value() );
  }

  //MESSAGE ("::FindShape() - SHAPE NOT FOUND")
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
//function : ShiftNodesQuadTria
//purpose  : auxilary
//           Shift nodes in the array corresponded to quadratic triangle
//           example: (0,1,2,3,4,5) -> (1,2,0,4,5,3)
//=======================================================================
static void ShiftNodesQuadTria(const SMDS_MeshNode* aNodes[])
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
//function : GetNodesFromTwoTria
//purpose  : auxilary
//           Shift nodes in the array corresponded to quadratic triangle
//           example: (0,1,2,3,4,5) -> (1,2,0,4,5,3)
//=======================================================================
static bool GetNodesFromTwoTria(const SMDS_MeshElement * theTria1,
                                const SMDS_MeshElement * theTria2,
                                const SMDS_MeshNode* N1[],
                                const SMDS_MeshNode* N2[])
{
  SMDS_ElemIteratorPtr it = theTria1->nodesIterator();
  int i=0;
  while(i<6) {
    N1[i] = static_cast<const SMDS_MeshNode*>( it->next() );
    i++;
  }
  if(it->more()) return false;
  it = theTria2->nodesIterator();
  i=0;
  while(i<6) {
    N2[i] = static_cast<const SMDS_MeshNode*>( it->next() );
    i++;
  }
  if(it->more()) return false;

  int sames[3] = {-1,-1,-1};
  int nbsames = 0;
  int j;
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
    ShiftNodesQuadTria(N1);
    if(sames[1]>-1) {
      ShiftNodesQuadTria(N1);
    }
  }
  i = sames[0] + sames[1] + sames[2];
  for(; i<2; i++) {
    ShiftNodesQuadTria(N2);
  }
  // now we receive following N1 and N2 (using numeration as above image)
  // tria1 : (1 2 4 5 9 7)  and  tria2 : (3 4 2 8 9 6)
  // i.e. first nodes from both arrays determ new diagonal
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
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if (!theTria1 || !theTria2)
    return false;

  const SMDS_FaceOfNodes* F1 = dynamic_cast<const SMDS_FaceOfNodes*>( theTria1 );
  const SMDS_FaceOfNodes* F2 = dynamic_cast<const SMDS_FaceOfNodes*>( theTria2 );
  if (F1 && F2) {

    //  1 +--+ A  theTria1: ( 1 A B ) A->2 ( 1 2 B ) 1 +--+ A
    //    | /|    theTria2: ( B A 2 ) B->1 ( 1 A 2 )   |\ |
    //    |/ |                                         | \|
    //  B +--+ 2                                     B +--+ 2

    // put nodes in array and find out indices of the same ones
    const SMDS_MeshNode* aNodes [6];
    int sameInd [] = { 0, 0, 0, 0, 0, 0 };
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
    int iA = 0, iB = 0, i1 = 0, i2 = 0;
    for ( i = 0; i < 6; i++ ) {
      if ( sameInd [ i ] == 0 )
        if ( i < 3 ) i1 = i;
        else         i2 = i;
      else if (i < 3)
        if ( iA ) iB = i;
        else      iA = i;
    }
    // nodes 1 and 2 should not be the same
    if ( aNodes[ i1 ] == aNodes[ i2 ] )
      return false;

    // theTria1: A->2
    aNodes[ iA ] = aNodes[ i2 ];
    // theTria2: B->1
    aNodes[ sameInd[ iB ]] = aNodes[ i1 ];

    //MESSAGE( theTria1 << theTria2 );

    GetMeshDS()->ChangeElementNodes( theTria1, aNodes, 3 );
    GetMeshDS()->ChangeElementNodes( theTria2, &aNodes[ 3 ], 3 );

    //MESSAGE( theTria1 << theTria2 );

    return true;

  } // end if(F1 && F2)

  // check case of quadratic faces
  const SMDS_QuadraticFaceOfNodes* QF1 =
    dynamic_cast<const SMDS_QuadraticFaceOfNodes*> (theTria1);
  if(!QF1) return false;
  const SMDS_QuadraticFaceOfNodes* QF2 =
    dynamic_cast<const SMDS_QuadraticFaceOfNodes*> (theTria2);
  if(!QF2) return false;

  //       5
  //  1 +--+--+ 2  theTria1: (1 2 4 5 9 7) or (2 4 1 9 7 5) or (4 1 2 7 5 9)
  //    |    /|    theTria2: (2 3 4 6 8 9) or (3 4 2 8 9 6) or (4 2 3 9 6 8)
  //    |   / |
  //  7 +  +  + 6
  //    | /9  |
  //    |/    |
  //  4 +--+--+ 3
  //       8

  const SMDS_MeshNode* N1 [6];
  const SMDS_MeshNode* N2 [6];
  if(!GetNodesFromTwoTria(theTria1,theTria2,N1,N2))
    return false;
  // now we receive following N1 and N2 (using numeration as above image)
  // tria1 : (1 2 4 5 9 7)  and  tria2 : (3 4 2 8 9 6)
  // i.e. first nodes from both arrays determ new diagonal

  const SMDS_MeshNode* N1new [6];
  const SMDS_MeshNode* N2new [6];
  N1new[0] = N1[0];
  N1new[1] = N2[0];
  N1new[2] = N2[1];
  N1new[3] = N1[4];
  N1new[4] = N2[3];
  N1new[5] = N1[5];
  N2new[0] = N1[0];
  N2new[1] = N1[1];
  N2new[2] = N2[0];
  N2new[3] = N1[3];
  N2new[4] = N2[5];
  N2new[5] = N1[4];
  // replaces nodes in faces
  GetMeshDS()->ChangeElementNodes( theTria1, N1new, 6 );
  GetMeshDS()->ChangeElementNodes( theTria2, N2new, 6 );

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
    if ( elem->NbNodes() == 3 )
      emap.insert( elem );
  }
  it = theNode2->GetInverseElementIterator(SMDSAbs_Face);
  while (it->more()) {
    const SMDS_MeshElement* elem = it->next();
    if ( emap.find( elem ) != emap.end() )
      if ( theTria1 ) {
        // theTria1 must be element with minimum ID
        if( theTria1->GetID() < elem->GetID() ) {
          theTria2 = elem;
        }
        else {
          theTria2 = theTria1;
          theTria1 = elem;
        }
        break;
      }
      else {
        theTria1 = elem;
      }
  }
  return ( theTria1 && theTria2 );
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

  const SMDS_FaceOfNodes* F1 = dynamic_cast<const SMDS_FaceOfNodes*>( tr1 );
  //if (!F1) return false;
  const SMDS_FaceOfNodes* F2 = dynamic_cast<const SMDS_FaceOfNodes*>( tr2 );
  //if (!F2) return false;
  if (F1 && F2) {

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

    //MESSAGE( tr1 << tr2 );

    GetMeshDS()->ChangeElementNodes( tr1, aNodes1, 3 );
    GetMeshDS()->ChangeElementNodes( tr2, aNodes2, 3 );

    //MESSAGE( tr1 << tr2 );

    return true;
  }

  // check case of quadratic faces
  const SMDS_QuadraticFaceOfNodes* QF1 =
    dynamic_cast<const SMDS_QuadraticFaceOfNodes*> (tr1);
  if(!QF1) return false;
  const SMDS_QuadraticFaceOfNodes* QF2 =
    dynamic_cast<const SMDS_QuadraticFaceOfNodes*> (tr2);
  if(!QF2) return false;
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

  const SMDS_FaceOfNodes* F1 = dynamic_cast<const SMDS_FaceOfNodes*>( tr1 );
  //if (!F1) return false;
  const SMDS_FaceOfNodes* F2 = dynamic_cast<const SMDS_FaceOfNodes*>( tr2 );
  //if (!F2) return false;
  if (F1 && F2) {

    const SMDS_MeshNode* aNodes [ 4 ];
    if ( ! getQuadrangleNodes( aNodes, theNode1, theNode2, tr1, tr2 ))
      return false;

    //MESSAGE( endl << tr1 << tr2 );

    GetMeshDS()->ChangeElementNodes( tr1, aNodes, 4 );
    myLastCreatedElems.Append(tr1);
    GetMeshDS()->RemoveElement( tr2 );

    //MESSAGE( endl << tr1 );

    return true;
  }

  // check case of quadratic faces
  const SMDS_QuadraticFaceOfNodes* QF1 =
    dynamic_cast<const SMDS_QuadraticFaceOfNodes*> (tr1);
  if(!QF1) return false;
  const SMDS_QuadraticFaceOfNodes* QF2 =
    dynamic_cast<const SMDS_QuadraticFaceOfNodes*> (tr2);
  if(!QF2) return false;

  //       5
  //  1 +--+--+ 2  tr1: (1 2 4 5 9 7) or (2 4 1 9 7 5) or (4 1 2 7 5 9)
  //    |    /|    tr2: (2 3 4 6 8 9) or (3 4 2 8 9 6) or (4 2 3 9 6 8)
  //    |   / |
  //  7 +  +  + 6
  //    | /9  |
  //    |/    |
  //  4 +--+--+ 3
  //       8

  const SMDS_MeshNode* N1 [6];
  const SMDS_MeshNode* N2 [6];
  if(!GetNodesFromTwoTria(tr1,tr2,N1,N2))
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

  GetMeshDS()->ChangeElementNodes( tr1, aNodes, 8 );
  myLastCreatedElems.Append(tr1);
  GetMeshDS()->RemoveElement( tr2 );

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
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  if (!theElem)
    return false;
  SMDS_ElemIteratorPtr it = theElem->nodesIterator();
  if ( !it || !it->more() )
    return false;

  switch ( theElem->GetType() ) {

  case SMDSAbs_Edge:
  case SMDSAbs_Face: {
    if(!theElem->IsQuadratic()) {
      int i = theElem->NbNodes();
      vector<const SMDS_MeshNode*> aNodes( i );
      while ( it->more() )
        aNodes[ --i ]= static_cast<const SMDS_MeshNode*>( it->next() );
      return GetMeshDS()->ChangeElementNodes( theElem, &aNodes[0], theElem->NbNodes() );
    }
    else {
      // quadratic elements
      if(theElem->GetType()==SMDSAbs_Edge) {
        vector<const SMDS_MeshNode*> aNodes(3);
        aNodes[1]= static_cast<const SMDS_MeshNode*>( it->next() );
        aNodes[0]= static_cast<const SMDS_MeshNode*>( it->next() );
        aNodes[2]= static_cast<const SMDS_MeshNode*>( it->next() );
        return GetMeshDS()->ChangeElementNodes( theElem, &aNodes[0], 3 );
      }
      else {
        int nbn = theElem->NbNodes();
        vector<const SMDS_MeshNode*> aNodes(nbn);
        aNodes[0]= static_cast<const SMDS_MeshNode*>( it->next() );
        int i=1;
        for(; i<nbn/2; i++) {
          aNodes[nbn/2-i]= static_cast<const SMDS_MeshNode*>( it->next() );
        }
        for(i=0; i<nbn/2; i++) {
          aNodes[nbn-i-1]= static_cast<const SMDS_MeshNode*>( it->next() );
        }
        return GetMeshDS()->ChangeElementNodes( theElem, &aNodes[0], nbn );
      }
    }
  }
  case SMDSAbs_Volume: {
    if (theElem->IsPoly()) {
      const SMDS_PolyhedralVolumeOfNodes* aPolyedre =
        static_cast<const SMDS_PolyhedralVolumeOfNodes*>( theElem );
      if (!aPolyedre) {
        MESSAGE("Warning: bad volumic element");
        return false;
      }

      int nbFaces = aPolyedre->NbFaces();
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
    else {
      SMDS_VolumeTool vTool;
      if ( !vTool.Set( theElem ))
        return false;
      vTool.Inverse();
      return GetMeshDS()->ChangeElementNodes( theElem, vTool.GetNodes(), vTool.NbNodes() );
    }
  }
  default:;
  }

  return false;
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

  MESSAGE( "::QuadToTri()" );

  if ( !theCrit.get() )
    return false;

  SMESHDS_Mesh * aMesh = GetMeshDS();

  Handle(Geom_Surface) surface;
  SMESH_MesherHelper   helper( *GetMesh() );

  TIDSortedElemSet::iterator itElem;
  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ ) {
    const SMDS_MeshElement* elem = *itElem;
    if ( !elem || elem->GetType() != SMDSAbs_Face )
      continue;
    if ( elem->NbNodes() != ( elem->IsQuadratic() ? 8 : 4 ))
      continue;

    // retrieve element nodes
    const SMDS_MeshNode* aNodes [8];
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    int i = 0;
    while ( itN->more() )
      aNodes[ i++ ] = static_cast<const SMDS_MeshNode*>( itN->next() );

    // compare two sets of possible triangles
    double aBadRate1, aBadRate2; // to what extent a set is bad
    SMDS_FaceOfNodes tr1 ( aNodes[0], aNodes[1], aNodes[2] );
    SMDS_FaceOfNodes tr2 ( aNodes[2], aNodes[3], aNodes[0] );
    aBadRate1 = getBadRate( &tr1, theCrit ) + getBadRate( &tr2, theCrit );

    SMDS_FaceOfNodes tr3 ( aNodes[1], aNodes[2], aNodes[3] );
    SMDS_FaceOfNodes tr4 ( aNodes[3], aNodes[0], aNodes[1] );
    aBadRate2 = getBadRate( &tr3, theCrit ) + getBadRate( &tr4, theCrit );

    int aShapeId = FindShape( elem );
    const SMDS_MeshElement* newElem = 0;

    if( !elem->IsQuadratic() ) {

      // split liner quadrangle

      if ( aBadRate1 <= aBadRate2 ) {
        // tr1 + tr2 is better
        aMesh->ChangeElementNodes( elem, aNodes, 3 );
        newElem = aMesh->AddFace( aNodes[2], aNodes[3], aNodes[0] );
      }
      else {
        // tr3 + tr4 is better
        aMesh->ChangeElementNodes( elem, &aNodes[1], 3 );
        newElem = aMesh->AddFace( aNodes[3], aNodes[0], aNodes[1] );
      }
    }
    else {

      // split quadratic quadrangle

      // get surface elem is on
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
      // get elem nodes
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
        TopoDS_Face face = TopoDS::Face( helper.GetSubShape() );
        gp_XY uv( 0,0 );
        for(i=0; i<4; i++)
          uv += helper.GetNodeUV( face, aNodes[i], inFaceNode );
        uv /= 4.;
        p = surface->Value( uv.X(), uv.Y() ).XYZ();
      }
      const SMDS_MeshNode* newN = aMesh->AddNode( p.X(), p.Y(), p.Z() );
      myLastCreatedNodes.Append(newN);

      // create a new element
      const SMDS_MeshNode* N[6];
      if ( aBadRate1 <= aBadRate2 ) {
        N[0] = aNodes[0];
        N[1] = aNodes[1];
        N[2] = aNodes[2];
        N[3] = aNodes[4];
        N[4] = aNodes[5];
        N[5] = newN;
        newElem = aMesh->AddFace(aNodes[2], aNodes[3], aNodes[0],
                                 aNodes[6], aNodes[7], newN );
      }
      else {
        N[0] = aNodes[1];
        N[1] = aNodes[2];
        N[2] = aNodes[3];
        N[3] = aNodes[5];
        N[4] = aNodes[6];
        N[5] = newN;
        newElem = aMesh->AddFace(aNodes[3], aNodes[0], aNodes[1],
                                 aNodes[7], aNodes[4], newN );
      }
      aMesh->ChangeElementNodes( elem, N, 6 );

    } // quadratic case

    // care of a new element

    myLastCreatedElems.Append(newElem);
    AddToSameGroups( newElem, elem, aMesh );

    // put a new triangle on the same shape
    if ( aShapeId )
      aMesh->SetMeshElementOnShape( newElem, aShapeId );
  }
  return true;
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

    if (aBadRate1 <= aBadRate2) // tr1 + tr2 is better
      return 1; // diagonal 1-3

    return 2; // diagonal 2-4
  }
  return -1;
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

//=======================================================================
//function : ReplaceElemInGroups
//purpose  : replace elemToRm by elemToAdd in the all groups
//=======================================================================

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
      const SMDS_MeshElement* newElem = 0;
      if ( the13Diag ) {
        aMesh->ChangeElementNodes( elem, aNodes, 3 );
        newElem = aMesh->AddFace( aNodes[2], aNodes[3], aNodes[0] );
      }
      else {
        aMesh->ChangeElementNodes( elem, &aNodes[1], 3 );
        newElem = aMesh->AddFace( aNodes[3], aNodes[0], aNodes[1] );
      }
      myLastCreatedElems.Append(newElem);
      // put a new triangle on the same shape and add to the same groups
      if ( aShapeId )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
      AddToSameGroups( newElem, elem, aMesh );
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
      const SMDS_MeshElement* newElem = 0;
      const SMDS_MeshNode* N[6];
      if ( the13Diag ) {
        N[0] = aNodes[0];
        N[1] = aNodes[1];
        N[2] = aNodes[2];
        N[3] = aNodes[4];
        N[4] = aNodes[5];
        N[5] = newN;
        newElem = aMesh->AddFace(aNodes[2], aNodes[3], aNodes[0],
                                 aNodes[6], aNodes[7], newN );
      }
      else {
        N[0] = aNodes[1];
        N[1] = aNodes[2];
        N[2] = aNodes[3];
        N[3] = aNodes[5];
        N[4] = aNodes[6];
        N[5] = newN;
        newElem = aMesh->AddFace(aNodes[3], aNodes[0], aNodes[1],
                                 aNodes[7], aNodes[4], newN );
      }
      myLastCreatedElems.Append(newElem);
      aMesh->ChangeElementNodes( elem, N, 6 );
      // put a new triangle on the same shape and add to the same groups
      if ( aShapeId )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
      AddToSameGroups( newElem, elem, aMesh );
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
  double angle = 2*PI; // bad angle

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
      if ( n == n1 || n == n2 )
        if ( iDiag < 0)
          iDiag = i;
        else {
          if ( i - iDiag == 1 )
            nFirst[ t ] = ( n == n1 ? n2 : n1 );
          else
            nFirst[ t ] = n;
          break;
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
  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ ) {
    const SMDS_MeshElement* elem = *itElem;
    if(!elem || elem->GetType() != SMDSAbs_Face ) continue;
    bool IsTria = elem->NbNodes()==3 || (elem->NbNodes()==6 && elem->IsQuadratic());
    if(!IsTria) continue;

    // retrieve element nodes
    const SMDS_MeshNode* aNodes [4];
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    int i = 0;
    while ( i<3 )
      aNodes[ i++ ] = cast2Node( itN->next() );
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
        // and remove fused elems and removed links from the maps
        mapEl_setLi.erase( tr1 );
        if ( Ok12 ) {
          mapEl_setLi.erase( tr2 );
          mapLi_listEl.erase( *link12 );
          if(tr1->NbNodes()==3) {
            if( tr1->GetID() < tr2->GetID() ) {
              aMesh->ChangeElementNodes( tr1, n12, 4 );
              myLastCreatedElems.Append(tr1);
              aMesh->RemoveElement( tr2 );
            }
            else {
              aMesh->ChangeElementNodes( tr2, n12, 4 );
              myLastCreatedElems.Append(tr2);
              aMesh->RemoveElement( tr1);
            }
          }
          else {
            const SMDS_MeshNode* N1 [6];
            const SMDS_MeshNode* N2 [6];
            GetNodesFromTwoTria(tr1,tr2,N1,N2);
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
            if( tr1->GetID() < tr2->GetID() ) {
              GetMeshDS()->ChangeElementNodes( tr1, aNodes, 8 );
              myLastCreatedElems.Append(tr1);
              GetMeshDS()->RemoveElement( tr2 );
            }
            else {
              GetMeshDS()->ChangeElementNodes( tr2, aNodes, 8 );
              myLastCreatedElems.Append(tr2);
              GetMeshDS()->RemoveElement( tr1 );
            }
            // remove middle node (9)
            GetMeshDS()->RemoveNode( N1[4] );
          }
        }
        else if ( Ok13 ) {
          mapEl_setLi.erase( tr3 );
          mapLi_listEl.erase( *link13 );
          if(tr1->NbNodes()==3) {
            if( tr1->GetID() < tr2->GetID() ) {
              aMesh->ChangeElementNodes( tr1, n13, 4 );
              myLastCreatedElems.Append(tr1);
              aMesh->RemoveElement( tr3 );
            }
            else {
              aMesh->ChangeElementNodes( tr3, n13, 4 );
              myLastCreatedElems.Append(tr3);
              aMesh->RemoveElement( tr1 );
            }
          }
          else {
            const SMDS_MeshNode* N1 [6];
            const SMDS_MeshNode* N2 [6];
            GetNodesFromTwoTria(tr1,tr3,N1,N2);
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
            if( tr1->GetID() < tr2->GetID() ) {
              GetMeshDS()->ChangeElementNodes( tr1, aNodes, 8 );
              myLastCreatedElems.Append(tr1);
              GetMeshDS()->RemoveElement( tr3 );
            }
            else {
              GetMeshDS()->ChangeElementNodes( tr3, aNodes, 8 );
              myLastCreatedElems.Append(tr3);
              GetMeshDS()->RemoveElement( tr1 );
            }
            // remove middle node (9)
            GetMeshDS()->RemoveNode( N1[4] );
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
#if OCC_VERSION_HEX >= 0x060500
      if ( projector.SquareDistance( i ) < minVal ) {
        minVal = projector.SquareDistance( i );
#else
      if ( projector.Value( i ) < minVal ) {
        minVal = projector.Value( i );
#endif
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
      theElems.insert( face );
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

  set< int >::reverse_iterator fId = faceIdSet.rbegin(); // treate 0 fId at the end
  for ( ; fId != faceIdSet.rend(); ++fId ) {
    // get face surface and submesh
    Handle(Geom_Surface) surface;
    SMESHDS_SubMesh* faceSubMesh = 0;
    TopoDS_Face face;
    double fToler2 = 0, vPeriod = 0., uPeriod = 0., f,l;
    double u1 = 0, u2 = 0, v1 = 0, v2 = 0;
    bool isUPeriodic = false, isVPeriodic = false;
    if ( *fId ) {
      face = TopoDS::Face( aMesh->IndexToShape( *fId ));
      surface = BRep_Tool::Surface( face );
      faceSubMesh = aMesh->MeshElements( *fId );
      fToler2 = BRep_Tool::Tolerance( face );
      fToler2 *= fToler2 * 10.;
      isUPeriodic = surface->IsUPeriodic();
      if ( isUPeriodic )
        vPeriod = surface->UPeriod();
      isVPeriodic = surface->IsVPeriodic();
      if ( isVPeriodic )
        uPeriod = surface->VPeriod();
      surface->Bounds( u1, u2, v1, v2 );
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
    while ( itElem != theElems.end() ) {
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
        posType = pos.get() ? pos->GetTypeOfPosition() : SMDS_TOP_3DSPACE;
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
      itN = elem->nodesIterator();
      nn = 0; nbn =  elem->NbNodes();
      if(elem->IsQuadratic())
        nbn = nbn/2;
      while ( nn++ < nbn ) {
        node = static_cast<const SMDS_MeshNode*>( itN->next() );
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
        posType = pos.get() ? pos->GetTypeOfPosition() : SMDS_TOP_3DSPACE;
        // get existing UV
        switch ( posType ) {
        case SMDS_TOP_FACE: {
          SMDS_FacePosition* fPos = ( SMDS_FacePosition* ) pos.get();
          uv.SetCoord( fPos->GetUParameter(), fPos->GetVParameter() );
          break;
        }
        case SMDS_TOP_EDGE: {
          TopoDS_Shape S = aMesh->IndexToShape( pos->GetShapeId() );
          Handle(Geom2d_Curve) pcurve;
          if ( !S.IsNull() && S.ShapeType() == TopAbs_EDGE )
            pcurve = BRep_Tool::CurveOnSurface( TopoDS::Edge( S ), face, f,l );
          if ( !pcurve.IsNull() ) {
            double u = (( SMDS_EdgePosition* ) pos.get() )->GetUParameter();
            uv = pcurve->Value( u ).XY();
          }
          break;
        }
        case SMDS_TOP_VERTEX: {
          TopoDS_Shape S = aMesh->IndexToShape( pos->GetShapeId() );
          if ( !S.IsNull() && S.ShapeType() == TopAbs_VERTEX )
            uv = BRep_Tool::Parameters( TopoDS::Vertex( S ), face ).XY();
          break;
        }
        default:;
        }
        // check existing UV
        bool project = true;
        gp_Pnt pNode ( node->X(), node->Y(), node->Z() );
        double dist1 = DBL_MAX, dist2 = 0;
        if ( posType != SMDS_TOP_3DSPACE ) {
          dist1 = pNode.SquareDistance( surface->Value( uv.X(), uv.Y() ));
          project = dist1 > fToler2;
        }
        if ( project ) { // compute new UV
          gp_XY newUV;
          if ( !getClosestUV( projector, pNode, newUV )) {
            MESSAGE("Node Projection Failed " << node);
          }
          else {
            if ( isUPeriodic )
              newUV.SetX( ElCLib::InPeriod( newUV.X(), u1, u2 ));
            if ( isVPeriodic )
              newUV.SetY( ElCLib::InPeriod( newUV.Y(), v1, v2 ));
            // check new UV
            if ( posType != SMDS_TOP_3DSPACE )
              dist2 = pNode.SquareDistance( surface->Value( newUV.X(), newUV.Y() ));
            if ( dist2 < dist1 )
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
      map< NLink, int > linkNbMap; // how many times a link encounters in elemsOnFace
      map< NLink, int >::iterator link_nb;
      // put all elements links to linkNbMap
      list< const SMDS_MeshElement* >::iterator elemIt = elemsOnFace.begin();
      for ( ; elemIt != elemsOnFace.end(); ++elemIt ) {
        const SMDS_MeshElement* elem = (*elemIt);
        int nbn =  elem->NbNodes();
        if(elem->IsQuadratic())
          nbn = nbn/2;
        // loop on elem links: insert them in linkNbMap
        const SMDS_MeshNode* curNode, *prevNode = elem->GetNodeWrap( nbn );
        for ( int iN = 0; iN < nbn; ++iN ) {
          curNode = elem->GetNode( iN );
          NLink link;
          if ( curNode < prevNode ) link = make_pair( curNode , prevNode );
          else                      link = make_pair( prevNode , curNode );
          prevNode = curNode;
          link_nb = linkNbMap.find( link );
          if ( link_nb == linkNbMap.end() )
            linkNbMap.insert( make_pair ( link, 1 ));
          else
            link_nb->second++;
        }
      }
      // remove nodes that are in links encountered only once from setMovableNodes
      for ( link_nb = linkNbMap.begin(); link_nb != linkNbMap.end(); ++link_nb ) {
        if ( link_nb->second == 1 ) {
          setMovableNodes.erase( link_nb->first.first );
          setMovableNodes.erase( link_nb->first.second );
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
        if ( uv1.Coord( iPar ) > uv2.Coord( iPar )) {
          gp_Pnt2d tmp = uv1; uv1 = uv2; uv2 = tmp;
        }
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
              gp_Pnt pMid (0.5 * ( n->X() + nSeam->X() ),
                           0.5 * ( n->Y() + nSeam->Y() ),
                           0.5 * ( n->Z() + nSeam->Z() ));
              gp_XY uv;
              getClosestUV( projector, pMid, uv );
              if ( uv.Coord( iPar ) > uvMap[ n ]->Coord( iPar ) ) {
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
          ( SMDS_PositionPtr( new SMDS_FacePosition( *fId, uv->X(), uv->Y() )));
      }
    }

    // move medium nodes of quadratic elements
    if ( isQuadratic )
    {
      SMESH_MesherHelper helper( *GetMesh() );
      if ( !face.IsNull() )
        helper.SetSubShape( face );
      list< const SMDS_MeshElement* >::iterator elemIt = elemsOnFace.begin();
      for ( ; elemIt != elemsOnFace.end(); ++elemIt ) {
        const SMDS_QuadraticFaceOfNodes* QF =
          dynamic_cast<const SMDS_QuadraticFaceOfNodes*> (*elemIt);
        if(QF) {
          vector<const SMDS_MeshNode*> Ns;
          Ns.reserve(QF->NbNodes()+1);
          SMDS_NodeIteratorPtr anIter = QF->interlacedNodesIterator();
          while ( anIter->more() )
            Ns.push_back( anIter->next() );
          Ns.push_back( Ns[0] );
          double x, y, z;
          for(int i=0; i<QF->NbNodes(); i=i+2) {
            if ( !surface.IsNull() ) {
              gp_XY uv1 = helper.GetNodeUV( face, Ns[i], Ns[i+2] );
              gp_XY uv2 = helper.GetNodeUV( face, Ns[i+2], Ns[i] );
              gp_XY uv = ( uv1 + uv2 ) / 2.;
              gp_Pnt xyz = surface->Value( uv.X(), uv.Y() );
              x = xyz.X(); y = xyz.Y(); z = xyz.Z();
            }
            else {
              x = (Ns[i]->X() + Ns[i+2]->X())/2;
              y = (Ns[i]->Y() + Ns[i+2]->Y())/2;
              z = (Ns[i]->Z() + Ns[i+2]->Z())/2;
            }
            if( fabs( Ns[i+1]->X() - x ) > disttol ||
                fabs( Ns[i+1]->Y() - y ) > disttol ||
                fabs( Ns[i+1]->Z() - z ) > disttol ) {
              // we have to move i+1 node
              aMesh->MoveNode( Ns[i+1], x, y, z );
            }
          }
        }
      }
    }

  } // loop on face ids

}

//=======================================================================
//function : isReverse
//purpose  : Return true if normal of prevNodes is not co-directied with
//           gp_Vec(prevNodes[iNotSame],nextNodes[iNotSame]).
//           iNotSame is where prevNodes and nextNodes are different
//=======================================================================

static bool isReverse(vector<const SMDS_MeshNode*> prevNodes,
                      vector<const SMDS_MeshNode*> nextNodes,
                      const int            nbNodes,
                      const int            iNotSame)
{
  int iBeforeNotSame = ( iNotSame == 0 ? nbNodes - 1 : iNotSame - 1 );
  int iAfterNotSame  = ( iNotSame + 1 == nbNodes ? 0 : iNotSame + 1 );

  const SMDS_MeshNode* nB = prevNodes[ iBeforeNotSame ];
  const SMDS_MeshNode* nA = prevNodes[ iAfterNotSame ];
  const SMDS_MeshNode* nP = prevNodes[ iNotSame ];
  const SMDS_MeshNode* nN = nextNodes[ iNotSame ];

  gp_Pnt pB ( nB->X(), nB->Y(), nB->Z() );
  gp_Pnt pA ( nA->X(), nA->Y(), nA->Z() );
  gp_Pnt pP ( nP->X(), nP->Y(), nP->Z() );
  gp_Pnt pN ( nN->X(), nN->Y(), nN->Z() );

  gp_Vec vB ( pP, pB ), vA ( pP, pA ), vN ( pP, pN );

  return (vA ^ vB) * vN < 0.0;
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
  SMESHDS_Mesh* aMesh = GetMeshDS();

  // Loop on elem nodes:
  // find new nodes and detect same nodes indices
  int nbNodes = elem->NbNodes();
  vector < list< const SMDS_MeshNode* >::const_iterator > itNN( nbNodes );
  vector<const SMDS_MeshNode*> prevNod( nbNodes );
  vector<const SMDS_MeshNode*> nextNod( nbNodes );
  vector<const SMDS_MeshNode*> midlNod( nbNodes );

  int iNode, nbSame = 0, iNotSameNode = 0, iSameNode = 0;
  vector<int> sames(nbNodes);
  vector<bool> issimple(nbNodes);

  for ( iNode = 0; iNode < nbNodes; iNode++ ) {
    TNodeOfNodeListMapItr nnIt = newNodesItVec[ iNode ];
    const SMDS_MeshNode*                 node         = nnIt->first;
    const list< const SMDS_MeshNode* > & listNewNodes = nnIt->second;
    if ( listNewNodes.empty() )
      return;

    issimple[iNode] = (listNewNodes.size()==nbSteps);

    itNN[ iNode ] = listNewNodes.begin();
    prevNod[ iNode ] = node;
    nextNod[ iNode ] = listNewNodes.front();
//cout<<"iNode="<<iNode<<endl;
//cout<<" prevNod[iNode]="<< prevNod[iNode]<<" nextNod[iNode]="<< nextNod[iNode]<<endl;
    if ( prevNod[ iNode ] != nextNod [ iNode ])
      iNotSameNode = iNode;
    else {
      iSameNode = iNode;
      //nbSame++;
      sames[nbSame++] = iNode;
    }
  }
//cout<<"1 nbSame="<<nbSame<<endl;
  if ( nbSame == nbNodes || nbSame > 2) {
    MESSAGE( " Too many same nodes of element " << elem->GetID() );
    return;
  }

//  if( elem->IsQuadratic() && nbSame>0 ) {
//    MESSAGE( "Can not rotate quadratic element " << elem->GetID() );
//    return;
//  }

  int iBeforeSame = 0, iAfterSame = 0, iOpposSame = 0;
  if ( nbSame > 0 ) {
    iBeforeSame = ( iSameNode == 0 ? nbNodes - 1 : iSameNode - 1 );
    iAfterSame  = ( iSameNode + 1 == nbNodes ? 0 : iSameNode + 1 );
    iOpposSame  = ( iSameNode - 2 < 0  ? iSameNode + 2 : iSameNode - 2 );
  }

//if(nbNodes==8)
//cout<<" prevNod[0]="<< prevNod[0]<<" prevNod[1]="<< prevNod[1]
//    <<" prevNod[2]="<< prevNod[2]<<" prevNod[3]="<< prevNod[4]
//    <<" prevNod[4]="<< prevNod[4]<<" prevNod[5]="<< prevNod[5]
//    <<" prevNod[6]="<< prevNod[6]<<" prevNod[7]="<< prevNod[7]<<endl;

  // check element orientation
  int i0 = 0, i2 = 2;
  if ( nbNodes > 2 && !isReverse( prevNod, nextNod, nbNodes, iNotSameNode )) {
    //MESSAGE("Reversed elem " << elem );
    i0 = 2;
    i2 = 0;
    if ( nbSame > 0 )
      std::swap( iBeforeSame, iAfterSame );
  }

  // make new elements
  for (int iStep = 0; iStep < nbSteps; iStep++ ) {
    // get next nodes
    for ( iNode = 0; iNode < nbNodes; iNode++ ) {
      if(issimple[iNode]) {
        nextNod[ iNode ] = *itNN[ iNode ];
        itNN[ iNode ]++;
      }
      else {
        if( elem->GetType()==SMDSAbs_Node ) {
          // we have to use two nodes
          midlNod[ iNode ] = *itNN[ iNode ];
          itNN[ iNode ]++;
          nextNod[ iNode ] = *itNN[ iNode ];
          itNN[ iNode ]++;
        }
        else if(!elem->IsQuadratic() || elem->IsMediumNode(prevNod[iNode]) ) {
          // we have to use each second node
          itNN[ iNode ]++;
          nextNod[ iNode ] = *itNN[ iNode ];
          itNN[ iNode ]++;
        }
        else {
          // we have to use two nodes
          midlNod[ iNode ] = *itNN[ iNode ];
          itNN[ iNode ]++;
          nextNod[ iNode ] = *itNN[ iNode ];
          itNN[ iNode ]++;
        }
      }
    }
    SMDS_MeshElement* aNewElem = 0;
    if(!elem->IsPoly()) {
      switch ( nbNodes ) {
      case 0:
        return;
      case 1: { // NODE
        if ( nbSame == 0 ) {
          if(issimple[0])
            aNewElem = aMesh->AddEdge( prevNod[ 0 ], nextNod[ 0 ] );
          else
            aNewElem = aMesh->AddEdge( prevNod[ 0 ], nextNod[ 0 ], midlNod[ 0 ] );
        }
        break;
      }
      case 2: { // EDGE
        if ( nbSame == 0 )
          aNewElem = aMesh->AddFace(prevNod[ 0 ], prevNod[ 1 ],
                                    nextNod[ 1 ], nextNod[ 0 ] );
        else
          aNewElem = aMesh->AddFace(prevNod[ 0 ], prevNod[ 1 ],
                                    nextNod[ iNotSameNode ] );
        break;
      }

      case 3: { // TRIANGLE or quadratic edge
        if(elem->GetType() == SMDSAbs_Face) { // TRIANGLE

          if ( nbSame == 0 )       // --- pentahedron
            aNewElem = aMesh->AddVolume (prevNod[ i0 ], prevNod[ 1 ], prevNod[ i2 ],
                                         nextNod[ i0 ], nextNod[ 1 ], nextNod[ i2 ] );

          else if ( nbSame == 1 )  // --- pyramid
            aNewElem = aMesh->AddVolume (prevNod[ iBeforeSame ],  prevNod[ iAfterSame ],
                                         nextNod[ iAfterSame ], nextNod[ iBeforeSame ],
                                         nextNod[ iSameNode ]);

          else // 2 same nodes:      --- tetrahedron
            aNewElem = aMesh->AddVolume (prevNod[ i0 ], prevNod[ 1 ], prevNod[ i2 ],
                                         nextNod[ iNotSameNode ]);
        }
        else { // quadratic edge
          if(nbSame==0) {     // quadratic quadrangle
            aNewElem = aMesh->AddFace(prevNod[0], nextNod[0], nextNod[1], prevNod[1],
                                      midlNod[0], nextNod[2], midlNod[1], prevNod[2]);
          }
          else if(nbSame==1) { // quadratic triangle
            if(sames[0]==2)
              return; // medium node on axis
            else if(sames[0]==0) {
              aNewElem = aMesh->AddFace(prevNod[0], nextNod[1], prevNod[1],
                                        nextNod[2], midlNod[1], prevNod[2]);
            }
            else { // sames[0]==1
              aNewElem = aMesh->AddFace(prevNod[0], nextNod[0], prevNod[1],
                                        midlNod[0], nextNod[2], prevNod[2]);
            }
          }
          else
            return;
        }
        break;
      }
      case 4: { // QUADRANGLE

        if ( nbSame == 0 )       // --- hexahedron
          aNewElem = aMesh->AddVolume (prevNod[ i0 ], prevNod[ 1 ], prevNod[ i2 ], prevNod[ 3 ],
                                       nextNod[ i0 ], nextNod[ 1 ], nextNod[ i2 ], nextNod[ 3 ]);

        else if ( nbSame == 1 ) { // --- pyramid + pentahedron
          aNewElem = aMesh->AddVolume (prevNod[ iBeforeSame ],  prevNod[ iAfterSame ],
                                       nextNod[ iAfterSame ], nextNod[ iBeforeSame ],
                                       nextNod[ iSameNode ]);
          newElems.push_back( aNewElem );
          aNewElem = aMesh->AddVolume (prevNod[ iAfterSame ], prevNod[ iOpposSame ],
                                       prevNod[ iBeforeSame ],  nextNod[ iAfterSame ],
                                       nextNod[ iOpposSame ],  nextNod[ iBeforeSame ] );
        }
        else if ( nbSame == 2 ) { // pentahedron
          if ( prevNod[ iBeforeSame ] == nextNod[ iBeforeSame ] )
            // iBeforeSame is same too
            aNewElem = aMesh->AddVolume (prevNod[ iBeforeSame ], prevNod[ iOpposSame ],
                                         nextNod[ iOpposSame ], prevNod[ iSameNode ],
                                         prevNod[ iAfterSame ],  nextNod[ iAfterSame ]);
          else
            // iAfterSame is same too
            aNewElem = aMesh->AddVolume (prevNod[ iSameNode ], prevNod[ iBeforeSame ],
                                         nextNod[ iBeforeSame ], prevNod[ iAfterSame ],
                                         prevNod[ iOpposSame ],  nextNod[ iOpposSame ]);
        }
        break;
      }
      case 6: { // quadratic triangle
        // create pentahedron with 15 nodes
        if(i0>0) { // reversed case
          aNewElem = aMesh->AddVolume (prevNod[0], prevNod[2], prevNod[1],
                                       nextNod[0], nextNod[2], nextNod[1],
                                       prevNod[5], prevNod[4], prevNod[3],
                                       nextNod[5], nextNod[4], nextNod[3],
                                       midlNod[0], midlNod[2], midlNod[1]);
        }
        else { // not reversed case
          aNewElem = aMesh->AddVolume (prevNod[0], prevNod[1], prevNod[2],
                                       nextNod[0], nextNod[1], nextNod[2],
                                       prevNod[3], prevNod[4], prevNod[5],
                                       nextNod[3], nextNod[4], nextNod[5],
                                       midlNod[0], midlNod[1], midlNod[2]);
        }
        break;
      }
      case 8: { // quadratic quadrangle
        // create hexahedron with 20 nodes
        if(i0>0) { // reversed case
          aNewElem = aMesh->AddVolume (prevNod[0], prevNod[3], prevNod[2], prevNod[1],
                                       nextNod[0], nextNod[3], nextNod[2], nextNod[1],
                                       prevNod[7], prevNod[6], prevNod[5], prevNod[4],
                                       nextNod[7], nextNod[6], nextNod[5], nextNod[4],
                                       midlNod[0], midlNod[3], midlNod[2], midlNod[1]);
        }
        else { // not reversed case
          aNewElem = aMesh->AddVolume (prevNod[0], prevNod[1], prevNod[2], prevNod[3],
                                       nextNod[0], nextNod[1], nextNod[2], nextNod[3],
                                       prevNod[4], prevNod[5], prevNod[6], prevNod[7],
                                       nextNod[4], nextNod[5], nextNod[6], nextNod[7],
                                       midlNod[0], midlNod[1], midlNod[2], midlNod[3]);
        }
        break;
      }
      default: {
        // realized for extrusion only
        //vector<const SMDS_MeshNode*> polyedre_nodes (nbNodes*2 + 4*nbNodes);
        //vector<int> quantities (nbNodes + 2);

        //quantities[0] = nbNodes; // bottom of prism
        //for (int inode = 0; inode < nbNodes; inode++) {
        //  polyedre_nodes[inode] = prevNod[inode];
        //}

        //quantities[1] = nbNodes; // top of prism
        //for (int inode = 0; inode < nbNodes; inode++) {
        //  polyedre_nodes[nbNodes + inode] = nextNod[inode];
        //}

        //for (int iface = 0; iface < nbNodes; iface++) {
        //  quantities[iface + 2] = 4;
        //  int inextface = (iface == nbNodes - 1) ? 0 : iface + 1;
        //  polyedre_nodes[2*nbNodes + 4*iface + 0] = prevNod[iface];
        //  polyedre_nodes[2*nbNodes + 4*iface + 1] = prevNod[inextface];
        //  polyedre_nodes[2*nbNodes + 4*iface + 2] = nextNod[inextface];
        //  polyedre_nodes[2*nbNodes + 4*iface + 3] = nextNod[iface];
        //}
        //aNewElem = aMesh->AddPolyhedralVolume (polyedre_nodes, quantities);
        break;
      }
      }
    }

    if(!aNewElem) {
      // realized for extrusion only
      vector<const SMDS_MeshNode*> polyedre_nodes (nbNodes*2 + 4*nbNodes);
      vector<int> quantities (nbNodes + 2);

      quantities[0] = nbNodes; // bottom of prism
      for (int inode = 0; inode < nbNodes; inode++) {
        polyedre_nodes[inode] = prevNod[inode];
      }

      quantities[1] = nbNodes; // top of prism
      for (int inode = 0; inode < nbNodes; inode++) {
        polyedre_nodes[nbNodes + inode] = nextNod[inode];
      }

      for (int iface = 0; iface < nbNodes; iface++) {
        quantities[iface + 2] = 4;
        int inextface = (iface == nbNodes - 1) ? 0 : iface + 1;
        polyedre_nodes[2*nbNodes + 4*iface + 0] = prevNod[iface];
        polyedre_nodes[2*nbNodes + 4*iface + 1] = prevNod[inextface];
        polyedre_nodes[2*nbNodes + 4*iface + 2] = nextNod[inextface];
        polyedre_nodes[2*nbNodes + 4*iface + 3] = nextNod[iface];
      }
      aNewElem = aMesh->AddPolyhedralVolume (polyedre_nodes, quantities);
    }

    if ( aNewElem ) {
      newElems.push_back( aNewElem );
      myLastCreatedElems.Append(aNewElem);
      srcElements.Append( elem );
    }

    // set new prev nodes
    for ( iNode = 0; iNode < nbNodes; iNode++ )
      prevNod[ iNode ] = nextNod[ iNode ];

  } // for steps
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
                                  TElemOfElemListMap &     newElemsMap,
                                  TElemOfVecOfNnlmiMap &   elemNewNodesMap,
                                  TIDSortedElemSet&        elemSet,
                                  const int                nbSteps,
                                  SMESH_SequenceOfElemPtr& srcElements)
{
  ASSERT( newElemsMap.size() == elemNewNodesMap.size() );
  SMESHDS_Mesh* aMesh = GetMeshDS();

  // Find nodes belonging to only one initial element - sweep them to get edges.

  TNodeOfNodeListMapItr nList = mapNewNodes.begin();
  for ( ; nList != mapNewNodes.end(); nList++ ) {
    const SMDS_MeshNode* node =
      static_cast<const SMDS_MeshNode*>( nList->first );
    SMDS_ElemIteratorPtr eIt = node->GetInverseElementIterator();
    int nbInitElems = 0;
    const SMDS_MeshElement* el = 0;
    SMDSAbs_ElementType highType = SMDSAbs_Edge; // count most complex elements only
    while ( eIt->more() && nbInitElems < 2 ) {
      el = eIt->next();
      SMDSAbs_ElementType type = el->GetType();
      if ( type == SMDSAbs_Volume || type < highType ) continue;
      if ( type > highType ) {
        nbInitElems = 0;
        highType = type;
      }
      if ( elemSet.find(el) != elemSet.end() )
        nbInitElems++;
    }
    if ( nbInitElems < 2 ) {
      bool NotCreateEdge = el && el->IsQuadratic() && el->IsMediumNode(node);
      if(!NotCreateEdge) {
        vector<TNodeOfNodeListMapItr> newNodesItVec( 1, nList );
        list<const SMDS_MeshElement*> newEdges;
        sweepElement( node, newNodesItVec, newEdges, nbSteps, srcElements );
      }
    }
  }

  // Make a ceiling for each element ie an equal element of last new nodes.
  // Find free links of faces - make edges and sweep them into faces.

  TElemOfElemListMap::iterator   itElem      = newElemsMap.begin();
  TElemOfVecOfNnlmiMap::iterator itElemNodes = elemNewNodesMap.begin();
  for ( ; itElem != newElemsMap.end(); itElem++, itElemNodes++ ) {
    const SMDS_MeshElement* elem = itElem->first;
    vector<TNodeOfNodeListMapItr>& vecNewNodes = itElemNodes->second;

    if ( elem->GetType() == SMDSAbs_Edge ) {
      // create a ceiling edge
      if (!elem->IsQuadratic()) {
        if ( !aMesh->FindEdge( vecNewNodes[ 0 ]->second.back(),
                               vecNewNodes[ 1 ]->second.back())) {
          myLastCreatedElems.Append(aMesh->AddEdge(vecNewNodes[ 0 ]->second.back(),
                                                   vecNewNodes[ 1 ]->second.back()));
          srcElements.Append( myLastCreatedElems.Last() );
        }
      }
      else {
        if ( !aMesh->FindEdge( vecNewNodes[ 0 ]->second.back(),
                               vecNewNodes[ 1 ]->second.back(),
                               vecNewNodes[ 2 ]->second.back())) {
          myLastCreatedElems.Append(aMesh->AddEdge(vecNewNodes[ 0 ]->second.back(),
                                                   vecNewNodes[ 1 ]->second.back(),
                                                   vecNewNodes[ 2 ]->second.back()));
          srcElements.Append( myLastCreatedElems.Last() );
        }
      }
    }
    if ( elem->GetType() != SMDSAbs_Face )
      continue;

    if(itElem->second.size()==0) continue;

    bool hasFreeLinks = false;

    TIDSortedElemSet avoidSet;
    avoidSet.insert( elem );

    set<const SMDS_MeshNode*> aFaceLastNodes;
    int iNode, nbNodes = vecNewNodes.size();
    if(!elem->IsQuadratic()) {
      // loop on the face nodes
      for ( iNode = 0; iNode < nbNodes; iNode++ ) {
        aFaceLastNodes.insert( vecNewNodes[ iNode ]->second.back() );
        // look for free links of the face
        int iNext = ( iNode + 1 == nbNodes ) ? 0 : iNode + 1;
        const SMDS_MeshNode* n1 = vecNewNodes[ iNode ]->first;
        const SMDS_MeshNode* n2 = vecNewNodes[ iNext ]->first;
        // check if a link is free
        if ( ! SMESH_MeshEditor::FindFaceInSet ( n1, n2, elemSet, avoidSet )) {
          hasFreeLinks = true;
          // make an edge and a ceiling for a new edge
          if ( !aMesh->FindEdge( n1, n2 )) {
            myLastCreatedElems.Append(aMesh->AddEdge( n1, n2 )); // free link edge
            srcElements.Append( myLastCreatedElems.Last() );
          }
          n1 = vecNewNodes[ iNode ]->second.back();
          n2 = vecNewNodes[ iNext ]->second.back();
          if ( !aMesh->FindEdge( n1, n2 )) {
            myLastCreatedElems.Append(aMesh->AddEdge( n1, n2 )); // ceiling edge
            srcElements.Append( myLastCreatedElems.Last() );
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
        // check if a link is free
        if ( ! SMESH_MeshEditor::FindFaceInSet ( n1, n2, elemSet, avoidSet )) {
          hasFreeLinks = true;
          // make an edge and a ceiling for a new edge
          // find medium node
          const SMDS_MeshNode* n3 = vecNewNodes[ iNode+nbn ]->first;
          if ( !aMesh->FindEdge( n1, n2, n3 )) {
            myLastCreatedElems.Append(aMesh->AddEdge( n1, n2, n3 )); // free link edge
            srcElements.Append( myLastCreatedElems.Last() );
          }
          n1 = vecNewNodes[ iNode ]->second.back();
          n2 = vecNewNodes[ iNext ]->second.back();
          n3 = vecNewNodes[ iNode+nbn ]->second.back();
          if ( !aMesh->FindEdge( n1, n2, n3 )) {
            myLastCreatedElems.Append(aMesh->AddEdge( n1, n2, n3 )); // ceiling edge
            srcElements.Append( myLastCreatedElems.Last() );
          }
        }
      }
      for ( iNode = nbn; iNode < 2*nbn; iNode++ ) {
        aFaceLastNodes.insert( vecNewNodes[ iNode ]->second.back() );
      }
    }

    // sweep free links into faces

    if ( hasFreeLinks )  {
      list<const SMDS_MeshElement*> & newVolumes = itElem->second;
      int iVol, volNb, nbVolumesByStep = newVolumes.size() / nbSteps;

      set<const SMDS_MeshNode*> initNodeSet, topNodeSet, faceNodeSet;
      for ( iNode = 0; iNode < nbNodes; iNode++ ) {
        initNodeSet.insert( vecNewNodes[ iNode ]->first );
        topNodeSet .insert( vecNewNodes[ iNode ]->second.back() );
      }
      for ( volNb = 0; volNb < nbVolumesByStep; volNb++ ) {
        list<const SMDS_MeshElement*>::iterator v = newVolumes.begin();
        iVol = 0;
        while ( iVol++ < volNb ) v++;
        // find indices of free faces of a volume and their source edges
        list< int > freeInd;
        list< const SMDS_MeshElement* > srcEdges; // source edges of free faces
        SMDS_VolumeTool vTool( *v );
        int iF, nbF = vTool.NbFaces();
        for ( iF = 0; iF < nbF; iF ++ ) {
          if (vTool.IsFreeFace( iF ) &&
              vTool.GetFaceNodes( iF, faceNodeSet ) &&
              initNodeSet != faceNodeSet) // except an initial face
          {
            if ( nbSteps == 1 && faceNodeSet == topNodeSet )
              continue;
            freeInd.push_back( iF );
            // find source edge of a free face iF
            vector<const SMDS_MeshNode*> commonNodes; // shared by the initial and free faces
            commonNodes.resize( initNodeSet.size(), NULL ); // avoid spoiling memory
            std::set_intersection( faceNodeSet.begin(), faceNodeSet.end(),
                                   initNodeSet.begin(), initNodeSet.end(),
                                   commonNodes.begin());
            if ( (*v)->IsQuadratic() )
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

        // create faces for all steps;
        // if such a face has been already created by sweep of edge,
        // assure that its orientation is OK
        for ( int iStep = 0; iStep < nbSteps; iStep++ )  {
          vTool.Set( *v );
          vTool.SetExternalNormal();
          list< int >::iterator ind = freeInd.begin();
          list< const SMDS_MeshElement* >::iterator srcEdge = srcEdges.begin();
          for ( ; ind != freeInd.end(); ++ind, ++srcEdge ) // loop on free faces
          {
            const SMDS_MeshNode** nodes = vTool.GetFaceNodes( *ind );
            int nbn = vTool.NbFaceNodes( *ind );
            switch ( nbn ) {
            case 3: { ///// triangle
              const SMDS_MeshFace * f = aMesh->FindFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ]);
              if ( !f )
                myLastCreatedElems.Append(aMesh->AddFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ] ));
              else if ( nodes[ 1 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ] ) + 1 ))
                aMesh->ChangeElementNodes( f, nodes, nbn );
              break;
            }
            case 4: { ///// quadrangle
              const SMDS_MeshFace * f = aMesh->FindFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ], nodes[ 3 ]);
              if ( !f )
                myLastCreatedElems.Append(aMesh->AddFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ], nodes[ 3 ] ));
              else if ( nodes[ 1 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ] ) + 1 ))
                aMesh->ChangeElementNodes( f, nodes, nbn );
              break;
            }
            default:
              if( (*v)->IsQuadratic() ) {
                if(nbn==6) { /////// quadratic triangle
                  const SMDS_MeshFace * f = aMesh->FindFace( nodes[0], nodes[2], nodes[4],
                                                             nodes[1], nodes[3], nodes[5] );
                  if ( !f )
                    myLastCreatedElems.Append(aMesh->AddFace(nodes[0], nodes[2], nodes[4],
                                                             nodes[1], nodes[3], nodes[5]));
                  else if ( nodes[ 2 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ] ) + 1 ))
                    aMesh->ChangeElementNodes( f, nodes, nbn );
                }
                else {       /////// quadratic quadrangle
                  const SMDS_MeshFace * f = aMesh->FindFace( nodes[0], nodes[2], nodes[4], nodes[6],
                                                             nodes[1], nodes[3], nodes[5], nodes[7] );
                  if ( !f )
                    myLastCreatedElems.Append(aMesh->AddFace(nodes[0], nodes[2], nodes[4], nodes[6],
                                                             nodes[1], nodes[3], nodes[5], nodes[7]));
                  else if ( nodes[ 2 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ] ) + 1 ))
                    aMesh->ChangeElementNodes( f, nodes, nbn );
                }
              }
              else { //////// polygon
                vector<const SMDS_MeshNode*> polygon_nodes ( nodes, &nodes[nbn] );
                const SMDS_MeshFace * f = aMesh->FindFace( polygon_nodes );
                if ( !f )
                  myLastCreatedElems.Append(aMesh->AddPolygonalFace(polygon_nodes));
                else if ( nodes[ 1 ] != f->GetNodeWrap( f->GetNodeIndex( nodes[ 0 ] ) + 1 ))
                  aMesh->ChangeElementNodes( f, nodes, nbn );
              }
            }
            while ( srcElements.Length() < myLastCreatedElems.Length() )
              srcElements.Append( *srcEdge );

          }  // loop on free faces

          // go to the next volume
          iVol = 0;
          while ( iVol++ < nbVolumesByStep ) v++;
        }
      }
    } // sweep free links into faces

    // Make a ceiling face with a normal external to a volume

    SMDS_VolumeTool lastVol( itElem->second.back() );

    int iF = lastVol.GetFaceIndex( aFaceLastNodes );
    if ( iF >= 0 ) {
      lastVol.SetExternalNormal();
      const SMDS_MeshNode** nodes = lastVol.GetFaceNodes( iF );
      int nbn = lastVol.NbFaceNodes( iF );
      switch ( nbn ) {
      case 3:
        if (!hasFreeLinks ||
            !aMesh->FindFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ]))
          myLastCreatedElems.Append(aMesh->AddFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ] ));
        break;
      case 4:
        if (!hasFreeLinks ||
            !aMesh->FindFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ], nodes[ 3 ]))
          myLastCreatedElems.Append(aMesh->AddFace( nodes[ 0 ], nodes[ 1 ], nodes[ 2 ], nodes[ 3 ] ));
        break;
      default:
        if(itElem->second.back()->IsQuadratic()) {
          if(nbn==6) {
            if (!hasFreeLinks ||
                !aMesh->FindFace(nodes[0], nodes[2], nodes[4],
                                 nodes[1], nodes[3], nodes[5]) ) {
              myLastCreatedElems.Append(aMesh->AddFace(nodes[0], nodes[2], nodes[4],
                                                       nodes[1], nodes[3], nodes[5]));
            }
          }
          else { // nbn==8
            if (!hasFreeLinks ||
                !aMesh->FindFace(nodes[0], nodes[2], nodes[4], nodes[6],
                                 nodes[1], nodes[3], nodes[5], nodes[7]) )
              myLastCreatedElems.Append(aMesh->AddFace(nodes[0], nodes[2], nodes[4], nodes[6],
                                                       nodes[1], nodes[3], nodes[5], nodes[7]));
          }
        }
        else {
          vector<const SMDS_MeshNode*> polygon_nodes ( nodes, &nodes[nbn] );
          if (!hasFreeLinks || !aMesh->FindFace(polygon_nodes))
            myLastCreatedElems.Append(aMesh->AddPolygonalFace(polygon_nodes));
        }
      } // switch

      while ( srcElements.Length() < myLastCreatedElems.Length() )
        srcElements.Append( myLastCreatedElems.Last() );
    }
  } // loop on swept elements
}

//=======================================================================
//function : RotationSweep
//purpose  :
//=======================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::RotationSweep(TIDSortedElemSet & theElems,
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
  TElemOfElemListMap newElemsMap;

  // loop on theElems
  TIDSortedElemSet::iterator itElem;
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
      // check if a node has been already sweeped
      const SMDS_MeshNode* node = cast2Node( itN->next() );
      TNodeOfNodeListMapItr nIt = mapNewNodes.find( node );
      if ( nIt == mapNewNodes.end() ) {
        nIt = mapNewNodes.insert( make_pair( node, list<const SMDS_MeshNode*>() )).first;
        list<const SMDS_MeshNode*>& listNewNodes = nIt->second;

        // make new nodes
        gp_XYZ aXYZ( node->X(), node->Y(), node->Z() );
        double coord[3];
        aXYZ.Coord( coord[0], coord[1], coord[2] );
        bool isOnAxis = ( aLine.SquareDistance( aXYZ ) <= aSqTol );
        const SMDS_MeshNode * newNode = node;
        for ( int i = 0; i < theNbSteps; i++ ) {
          if ( !isOnAxis ) {
            if( elem->IsQuadratic() && !elem->IsMediumNode(node) ) {
              // create two nodes
              aTrsf2.Transforms( coord[0], coord[1], coord[2] );
              //aTrsf.Transforms( coord[0], coord[1], coord[2] );
              newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
              myLastCreatedNodes.Append(newNode);
              srcNodes.Append( node );
              listNewNodes.push_back( newNode );
              aTrsf2.Transforms( coord[0], coord[1], coord[2] );
              //aTrsf.Transforms( coord[0], coord[1], coord[2] );
            }
            else {
              aTrsf.Transforms( coord[0], coord[1], coord[2] );
            }
            newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
            myLastCreatedNodes.Append(newNode);
            srcNodes.Append( node );
          }
          listNewNodes.push_back( newNode );
        }
      }
      else {
        // if current elem is quadratic and current node is not medium
        // we have to check - may be it is needed to insert additional nodes
        if( elem->IsQuadratic() && !elem->IsMediumNode(node) ) {
          list< const SMDS_MeshNode* > & listNewNodes = nIt->second;
          if(listNewNodes.size()==theNbSteps) {
            listNewNodes.clear();
            // make new nodes
            gp_XYZ aXYZ( node->X(), node->Y(), node->Z() );
            double coord[3];
            aXYZ.Coord( coord[0], coord[1], coord[2] );
            const SMDS_MeshNode * newNode = node;
            for(int i = 0; i<theNbSteps; i++) {
              aTrsf2.Transforms( coord[0], coord[1], coord[2] );
              newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
              myLastCreatedNodes.Append(newNode);
              listNewNodes.push_back( newNode );
              srcNodes.Append( node );
              aTrsf2.Transforms( coord[0], coord[1], coord[2] );
              newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
              myLastCreatedNodes.Append(newNode);
              srcNodes.Append( node );
              listNewNodes.push_back( newNode );
            }
          }
        }
      }
      newNodesItVec.push_back( nIt );
    }
    // make new elements
    sweepElement( elem, newNodesItVec, newElemsMap[elem], theNbSteps, srcElems );
  }

  if ( theMakeWalls )
    makeWalls( mapNewNodes, newElemsMap, mapElemNewNodes, theElems, theNbSteps, srcElems );
  
  PGroupIDs newGroupIDs;
  if ( theMakeGroups )
    newGroupIDs = generateGroups( srcNodes, srcElems, "rotated");

  return newGroupIDs;
}


//=======================================================================
//function : CreateNode
//purpose  :
//=======================================================================
const SMDS_MeshNode* SMESH_MeshEditor::CreateNode(const double x,
                                                  const double y,
                                                  const double z,
                                                  const double tolnode,
                                                  SMESH_SequenceOfNode& aNodes)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  gp_Pnt P1(x,y,z);
  SMESHDS_Mesh * aMesh = myMesh->GetMeshDS();

  // try to search in sequence of existing nodes
  // if aNodes.Length()>0 we 'nave to use given sequence
  // else - use all nodes of mesh
  if(aNodes.Length()>0) {
    int i;
    for(i=1; i<=aNodes.Length(); i++) {
      gp_Pnt P2(aNodes.Value(i)->X(),aNodes.Value(i)->Y(),aNodes.Value(i)->Z());
      if(P1.Distance(P2)<tolnode)
        return aNodes.Value(i);
    }
  }
  else {
    SMDS_NodeIteratorPtr itn = aMesh->nodesIterator();
    while(itn->more()) {
      const SMDS_MeshNode* aN = static_cast<const SMDS_MeshNode*> (itn->next());
      gp_Pnt P2(aN->X(),aN->Y(),aN->Z());
      if(P1.Distance(P2)<tolnode)
        return aN;
    }
  }

  // create new node and return it
  const SMDS_MeshNode* NewNode = aMesh->AddNode(x,y,z);
  myLastCreatedNodes.Append(NewNode);
  return NewNode;
}


//=======================================================================
//function : ExtrusionSweep
//purpose  :
//=======================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::ExtrusionSweep (TIDSortedElemSet &  theElems,
                                  const gp_Vec&       theStep,
                                  const int           theNbSteps,
                                  TElemOfElemListMap& newElemsMap,
                                  const bool          theMakeGroups,
                                  const int           theFlags,
                                  const double        theTolerance)
{
  ExtrusParam aParams;
  aParams.myDir = gp_Dir(theStep);
  aParams.myNodes.Clear();
  aParams.mySteps = new TColStd_HSequenceOfReal;
  int i;
  for(i=1; i<=theNbSteps; i++)
    aParams.mySteps->Append(theStep.Magnitude());

  return
    ExtrusionSweep(theElems,aParams,newElemsMap,theMakeGroups,theFlags,theTolerance);
}


//=======================================================================
//function : ExtrusionSweep
//purpose  :
//=======================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::ExtrusionSweep (TIDSortedElemSet &  theElems,
                                  ExtrusParam&        theParams,
                                  TElemOfElemListMap& newElemsMap,
                                  const bool          theMakeGroups,
                                  const int           theFlags,
                                  const double        theTolerance)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  // source elements for each generated one
  SMESH_SequenceOfElemPtr srcElems, srcNodes;

  SMESHDS_Mesh* aMesh = GetMeshDS();

  int nbsteps = theParams.mySteps->Length();

  TNodeOfNodeListMap mapNewNodes;
  //TNodeOfNodeVecMap mapNewNodes;
  TElemOfVecOfNnlmiMap mapElemNewNodes;
  //TElemOfVecOfMapNodesMap mapElemNewNodes;

  // loop on theElems
  TIDSortedElemSet::iterator itElem;
  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ ) {
    // check element type
    const SMDS_MeshElement* elem = *itElem;
    if ( !elem  || elem->GetType() == SMDSAbs_Volume )
      continue;

    vector<TNodeOfNodeListMapItr> & newNodesItVec = mapElemNewNodes[ elem ];
    //vector<TNodeOfNodeVecMapItr> & newNodesItVec = mapElemNewNodes[ elem ];
    newNodesItVec.reserve( elem->NbNodes() );

    // loop on elem nodes
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    while ( itN->more() )
    {
      // check if a node has been already sweeped
      const SMDS_MeshNode* node = cast2Node( itN->next() );
      TNodeOfNodeListMap::iterator nIt = mapNewNodes.find( node );
      //TNodeOfNodeVecMap::iterator nIt = mapNewNodes.find( node );
      if ( nIt == mapNewNodes.end() ) {
        nIt = mapNewNodes.insert( make_pair( node, list<const SMDS_MeshNode*>() )).first;
        //nIt = mapNewNodes.insert( make_pair( node, vector<const SMDS_MeshNode*>() )).first;
        list<const SMDS_MeshNode*>& listNewNodes = nIt->second;
        //vector<const SMDS_MeshNode*>& vecNewNodes = nIt->second;
        //vecNewNodes.reserve(nbsteps);

        // make new nodes
        double coord[] = { node->X(), node->Y(), node->Z() };
        //int nbsteps = theParams.mySteps->Length();
        for ( int i = 0; i < nbsteps; i++ ) {
          if( elem->IsQuadratic() && !elem->IsMediumNode(node) ) {
            // create additional node
            double x = coord[0] + theParams.myDir.X()*theParams.mySteps->Value(i+1)/2.;
            double y = coord[1] + theParams.myDir.Y()*theParams.mySteps->Value(i+1)/2.;
            double z = coord[2] + theParams.myDir.Z()*theParams.mySteps->Value(i+1)/2.;
            if( theFlags & EXTRUSION_FLAG_SEW ) {
              const SMDS_MeshNode * newNode = CreateNode(x, y, z,
                                                         theTolerance, theParams.myNodes);
              listNewNodes.push_back( newNode );
            }
            else {
              const SMDS_MeshNode * newNode = aMesh->AddNode(x, y, z);
              myLastCreatedNodes.Append(newNode);
              srcNodes.Append( node );
              listNewNodes.push_back( newNode );
            }
          }
          //aTrsf.Transforms( coord[0], coord[1], coord[2] );
          coord[0] = coord[0] + theParams.myDir.X()*theParams.mySteps->Value(i+1);
          coord[1] = coord[1] + theParams.myDir.Y()*theParams.mySteps->Value(i+1);
          coord[2] = coord[2] + theParams.myDir.Z()*theParams.mySteps->Value(i+1);
          if( theFlags & EXTRUSION_FLAG_SEW ) {
            const SMDS_MeshNode * newNode = CreateNode(coord[0], coord[1], coord[2],
                                                       theTolerance, theParams.myNodes);
            listNewNodes.push_back( newNode );
            //vecNewNodes[i]=newNode;
          }
          else {
            const SMDS_MeshNode * newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
            myLastCreatedNodes.Append(newNode);
            srcNodes.Append( node );
            listNewNodes.push_back( newNode );
            //vecNewNodes[i]=newNode;
          }
        }
      }
      else {
        // if current elem is quadratic and current node is not medium
        // we have to check - may be it is needed to insert additional nodes
        if( elem->IsQuadratic() && !elem->IsMediumNode(node) ) {
          list< const SMDS_MeshNode* > & listNewNodes = nIt->second;
          if(listNewNodes.size()==nbsteps) {
            listNewNodes.clear();
            double coord[] = { node->X(), node->Y(), node->Z() };
            for ( int i = 0; i < nbsteps; i++ ) {
              double x = coord[0] + theParams.myDir.X()*theParams.mySteps->Value(i+1);
              double y = coord[1] + theParams.myDir.Y()*theParams.mySteps->Value(i+1);
              double z = coord[2] + theParams.myDir.Z()*theParams.mySteps->Value(i+1);
              if( theFlags & EXTRUSION_FLAG_SEW ) {
                const SMDS_MeshNode * newNode = CreateNode(x, y, z,
                                                           theTolerance, theParams.myNodes);
                listNewNodes.push_back( newNode );
              }
              else {
                const SMDS_MeshNode * newNode = aMesh->AddNode(x, y, z);
                myLastCreatedNodes.Append(newNode);
                srcNodes.Append( node );
                listNewNodes.push_back( newNode );
              }
              coord[0] = coord[0] + theParams.myDir.X()*theParams.mySteps->Value(i+1);
              coord[1] = coord[1] + theParams.myDir.Y()*theParams.mySteps->Value(i+1);
              coord[2] = coord[2] + theParams.myDir.Z()*theParams.mySteps->Value(i+1);
              if( theFlags & EXTRUSION_FLAG_SEW ) {
                const SMDS_MeshNode * newNode = CreateNode(coord[0], coord[1], coord[2],
                                                           theTolerance, theParams.myNodes);
                listNewNodes.push_back( newNode );
              }
              else {
                const SMDS_MeshNode * newNode = aMesh->AddNode( coord[0], coord[1], coord[2] );
                myLastCreatedNodes.Append(newNode);
                srcNodes.Append( node );
                listNewNodes.push_back( newNode );
              }
            }
          }
        }
      }
      newNodesItVec.push_back( nIt );
    }
    // make new elements
    sweepElement( elem, newNodesItVec, newElemsMap[elem], nbsteps, srcElems );
  }

  if( theFlags & EXTRUSION_FLAG_BOUNDARY ) {
    makeWalls( mapNewNodes, newElemsMap, mapElemNewNodes, theElems, nbsteps, srcElems );
  }
  PGroupIDs newGroupIDs;
  if ( theMakeGroups )
    newGroupIDs = generateGroups( srcNodes, srcElems, "extruded");

  return newGroupIDs;
}


//=======================================================================
//class    : SMESH_MeshEditor_PathPoint
//purpose  : auxiliary class
//=======================================================================
class SMESH_MeshEditor_PathPoint {
public:
  SMESH_MeshEditor_PathPoint() {
    myPnt.SetCoord(99., 99., 99.);
    myTgt.SetCoord(1.,0.,0.);
    myAngle=0.;
    myPrm=0.;
  }
  void SetPnt(const gp_Pnt& aP3D){
    myPnt=aP3D;
  }
  void SetTangent(const gp_Dir& aTgt){
    myTgt=aTgt;
  }
  void SetAngle(const double& aBeta){
    myAngle=aBeta;
  }
  void SetParameter(const double& aPrm){
    myPrm=aPrm;
  }
  const gp_Pnt& Pnt()const{
    return myPnt;
  }
  const gp_Dir& Tangent()const{
    return myTgt;
  }
  double Angle()const{
    return myAngle;
  }
  double Parameter()const{
    return myPrm;
  }

protected:
  gp_Pnt myPnt;
  gp_Dir myTgt;
  double myAngle;
  double myPrm;
};

//=======================================================================
//function : ExtrusionAlongTrack
//purpose  :
//=======================================================================
SMESH_MeshEditor::Extrusion_Error
  SMESH_MeshEditor::ExtrusionAlongTrack (TIDSortedElemSet &   theElements,
					 SMESH_subMesh*       theTrack,
					 const SMDS_MeshNode* theN1,
					 const bool           theHasAngles,
					 list<double>&        theAngles,
					 const bool           theHasRefPoint,
					 const gp_Pnt&        theRefPoint,
                                         const bool           theMakeGroups)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  // source elements for each generated one
  SMESH_SequenceOfElemPtr srcElems, srcNodes;

  int j, aNbTP, aNbE, aNb;
  double aT1, aT2, aT, aAngle, aX, aY, aZ;
  std::list<double> aPrms;
  std::list<double>::iterator aItD;
  TIDSortedElemSet::iterator itElem;

  Standard_Real aTx1, aTx2, aL2, aTolVec, aTolVec2;
  gp_Pnt aP3D, aV0;
  gp_Vec aVec;
  gp_XYZ aGC;
  Handle(Geom_Curve) aC3D;
  TopoDS_Edge aTrackEdge;
  TopoDS_Vertex aV1, aV2;

  SMDS_ElemIteratorPtr aItE;
  SMDS_NodeIteratorPtr aItN;
  SMDSAbs_ElementType aTypeE;

  TNodeOfNodeListMap mapNewNodes;
  TElemOfVecOfNnlmiMap mapElemNewNodes;
  TElemOfElemListMap newElemsMap;

  aTolVec=1.e-7;
  aTolVec2=aTolVec*aTolVec;

  // 1. Check data
  aNbE = theElements.size();
  // nothing to do
  if ( !aNbE )
    return EXTR_NO_ELEMENTS;

  // 1.1 Track Pattern
  ASSERT( theTrack );

  SMESHDS_SubMesh* pSubMeshDS=theTrack->GetSubMeshDS();

  aItE = pSubMeshDS->GetElements();
  while ( aItE->more() ) {
    const SMDS_MeshElement* pE = aItE->next();
    aTypeE = pE->GetType();
    // Pattern must contain links only
    if ( aTypeE != SMDSAbs_Edge )
      return EXTR_PATH_NOT_EDGE;
  }

  const TopoDS_Shape& aS = theTrack->GetSubShape();
  // Sub shape for the Pattern must be an Edge
  if ( aS.ShapeType() != TopAbs_EDGE )
    return EXTR_BAD_PATH_SHAPE;

  aTrackEdge = TopoDS::Edge( aS );
  // the Edge must not be degenerated
  if ( BRep_Tool::Degenerated( aTrackEdge ) )
    return EXTR_BAD_PATH_SHAPE;

  TopExp::Vertices( aTrackEdge, aV1, aV2 );
  aT1=BRep_Tool::Parameter( aV1, aTrackEdge );
  aT2=BRep_Tool::Parameter( aV2, aTrackEdge );

  aItN = theTrack->GetFather()->GetSubMesh( aV1 )->GetSubMeshDS()->GetNodes();
  const SMDS_MeshNode* aN1 = aItN->next();

  aItN = theTrack->GetFather()->GetSubMesh( aV2 )->GetSubMeshDS()->GetNodes();
  const SMDS_MeshNode* aN2 = aItN->next();

  // starting node must be aN1 or aN2
  if ( !( aN1 == theN1 || aN2 == theN1 ) )
    return EXTR_BAD_STARTING_NODE;

  aNbTP = pSubMeshDS->NbNodes() + 2;

  // 1.2. Angles
  vector<double> aAngles( aNbTP );

  for ( j=0; j < aNbTP; ++j ) {
    aAngles[j] = 0.;
  }

  if ( theHasAngles ) {
    aItD = theAngles.begin();
    for ( j=1; (aItD != theAngles.end()) && (j<aNbTP); ++aItD, ++j ) {
      aAngle = *aItD;
      aAngles[j] = aAngle;
    }
  }

  // 2. Collect parameters on the track edge
  aPrms.push_back( aT1 );
  aPrms.push_back( aT2 );

  aItN = pSubMeshDS->GetNodes();
  while ( aItN->more() ) {
    const SMDS_MeshNode* pNode = aItN->next();
    const SMDS_EdgePosition* pEPos =
      static_cast<const SMDS_EdgePosition*>( pNode->GetPosition().get() );
    aT = pEPos->GetUParameter();
    aPrms.push_back( aT );
  }

  // sort parameters
  aPrms.sort();
  if ( aN1 == theN1 ) {
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
  vector<SMESH_MeshEditor_PathPoint> aPPs( aNbTP );
  //
  aC3D = BRep_Tool::Curve( aTrackEdge, aTx1, aTx2 );
  //
  aItD = aPrms.begin();
  for ( j=0; aItD != aPrms.end(); ++aItD, ++j ) {
    aT = *aItD;
    aC3D->D1( aT, aP3D, aVec );
    aL2 = aVec.SquareMagnitude();
    if ( aL2 < aTolVec2 )
      return EXTR_CANT_GET_TANGENT;

    gp_Dir aTgt( aVec );
    aAngle = aAngles[j];

    aPP.SetPnt( aP3D );
    aPP.SetTangent( aTgt );
    aPP.SetAngle( aAngle );
    aPP.SetParameter( aT );
    aPPs[j]=aPP;
  }

  // 3. Center of rotation aV0
  aV0 = theRefPoint;
  if ( !theHasRefPoint ) {
    aNb = 0;
    aGC.SetCoord( 0.,0.,0. );

    itElem = theElements.begin();
    for ( ; itElem != theElements.end(); itElem++ ) {
      const SMDS_MeshElement* elem = *itElem;

      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      while ( itN->more() ) {
	const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( itN->next() );
	aX = node->X();
	aY = node->Y();
	aZ = node->Z();

	if ( mapNewNodes.find( node ) == mapNewNodes.end() ) {
	  list<const SMDS_MeshNode*> aLNx;
	  mapNewNodes[node] = aLNx;
	  //
	  gp_XYZ aXYZ( aX, aY, aZ );
	  aGC += aXYZ;
	  ++aNb;
	}
      }
    }
    aGC /= aNb;
    aV0.SetXYZ( aGC );
  } // if (!theHasRefPoint) {
  mapNewNodes.clear();

  // 4. Processing the elements
  SMESHDS_Mesh* aMesh = GetMeshDS();

  for ( itElem = theElements.begin(); itElem != theElements.end(); itElem++ ) {
    // check element type
    const SMDS_MeshElement* elem = *itElem;
    aTypeE = elem->GetType();
    if ( !elem || ( aTypeE != SMDSAbs_Face && aTypeE != SMDSAbs_Edge ) )
      continue;

    vector<TNodeOfNodeListMapItr> & newNodesItVec = mapElemNewNodes[ elem ];
    newNodesItVec.reserve( elem->NbNodes() );

    // loop on elem nodes
    int nodeIndex = -1;
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    while ( itN->more() )
    {
      ++nodeIndex;
      // check if a node has been already processed
      const SMDS_MeshNode* node =
	static_cast<const SMDS_MeshNode*>( itN->next() );
      TNodeOfNodeListMap::iterator nIt = mapNewNodes.find( node );
      if ( nIt == mapNewNodes.end() ) {
        nIt = mapNewNodes.insert( make_pair( node, list<const SMDS_MeshNode*>() )).first;
        list<const SMDS_MeshNode*>& listNewNodes = nIt->second;

	// make new nodes
	aX = node->X();  aY = node->Y(); aZ = node->Z();

	Standard_Real aAngle1x, aAngleT1T0, aTolAng;
	gp_Pnt aP0x, aP1x, aPN0, aPN1, aV0x, aV1x;
	gp_Ax1 anAx1, anAxT1T0;
	gp_Dir aDT1x, aDT0x, aDT1T0;

	aTolAng=1.e-4;

	aV0x = aV0;
	aPN0.SetCoord(aX, aY, aZ);

	const SMESH_MeshEditor_PathPoint& aPP0 = aPPs[0];
	aP0x = aPP0.Pnt();
	aDT0x= aPP0.Tangent();

	for ( j = 1; j < aNbTP; ++j ) {
	  const SMESH_MeshEditor_PathPoint& aPP1 = aPPs[j];
	  aP1x = aPP1.Pnt();
	  aDT1x = aPP1.Tangent();
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
	  if (fabs(aAngleT1T0) > aTolAng) {
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
          if( elem->IsQuadratic() && !elem->IsMediumNode(node) ) {
            // create additional node
            double x = ( aPN1.X() + aPN0.X() )/2.;
            double y = ( aPN1.Y() + aPN0.Y() )/2.;
            double z = ( aPN1.Z() + aPN0.Z() )/2.;
            const SMDS_MeshNode* newNode = aMesh->AddNode(x,y,z);
            myLastCreatedNodes.Append(newNode);
            srcNodes.Append( node );
            listNewNodes.push_back( newNode );
          }
	  aX = aPN1.X();
	  aY = aPN1.Y();
	  aZ = aPN1.Z();
	  const SMDS_MeshNode* newNode = aMesh->AddNode( aX, aY, aZ );
          myLastCreatedNodes.Append(newNode);
          srcNodes.Append( node );
	  listNewNodes.push_back( newNode );

	  aPN0 = aPN1;
	  aP0x = aP1x;
	  aV0x = aV1x;
	  aDT0x = aDT1x;
	}
      }

      else {
        // if current elem is quadratic and current node is not medium
        // we have to check - may be it is needed to insert additional nodes
        if( elem->IsQuadratic() && !elem->IsMediumNode(node) ) {
          list< const SMDS_MeshNode* > & listNewNodes = nIt->second;
          if(listNewNodes.size()==aNbTP-1) {
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
      }

      newNodesItVec.push_back( nIt );
    }
    // make new elements
    //sweepElement( aMesh, elem, newNodesItVec, newElemsMap[elem],
    //              newNodesItVec[0]->second.size(), myLastCreatedElems );
    sweepElement( elem, newNodesItVec, newElemsMap[elem], aNbTP-1, srcElems );
  }

  makeWalls( mapNewNodes, newElemsMap, mapElemNewNodes, theElements, aNbTP-1, srcElems );

  if ( theMakeGroups )
    generateGroups( srcNodes, srcElems, "extruded");

  return EXTR_OK;
}

//=======================================================================
//function : Transform
//purpose  :
//=======================================================================

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
  case gp_Ax1Mirror:
  case gp_Ax2Mirror:
    needReverse = true;
    groupPostfix = "mirrored";
    break;
  case gp_Rotation:
    groupPostfix = "rotated";
    break;
  case gp_Translation:
    groupPostfix = "translated";
    break;
  case gp_Scale:
    groupPostfix = "scaled";
    break;
  default:
    needReverse = false;
    groupPostfix = "transformed";
  }

  SMESH_MeshEditor targetMeshEditor( theTargetMesh );
  SMESHDS_Mesh* aTgtMesh = theTargetMesh ? theTargetMesh->GetMeshDS() : 0;
  SMESHDS_Mesh* aMesh    = GetMeshDS();
  

  // map old node to new one
  TNodeNodeMap nodeMap;

  // elements sharing moved nodes; those of them which have all
  // nodes mirrored but are not in theElems are to be reversed
  TIDSortedElemSet inverseElemSet;

  // source elements for each generated one
  SMESH_SequenceOfElemPtr srcElems, srcNodes;

  // loop on theElems
  TIDSortedElemSet::iterator itElem;
  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ ) {
    const SMDS_MeshElement* elem = *itElem;
    if ( !elem )
      continue;

    // loop on elem nodes
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    while ( itN->more() ) {

      // check if a node has been already transformed
      const SMDS_MeshNode* node = cast2Node( itN->next() );
      pair<TNodeNodeMap::iterator,bool> n2n_isnew =
        nodeMap.insert( make_pair ( node, node ));
      if ( !n2n_isnew.second )
        continue;

      double coord[3];
      coord[0] = node->X();
      coord[1] = node->Y();
      coord[2] = node->Z();
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
  }

  // either create new elements or reverse mirrored ones
  if ( !theCopy && !needReverse && !theTargetMesh )
    return PGroupIDs();

  TIDSortedElemSet::iterator invElemIt = inverseElemSet.begin();
  for ( ; invElemIt != inverseElemSet.end(); invElemIt++ )
    theElems.insert( *invElemIt );

  // replicate or reverse elements

  enum {
    REV_TETRA   = 0,  //  = nbNodes - 4
    REV_PYRAMID = 1,  //  = nbNodes - 4
    REV_PENTA   = 2,  //  = nbNodes - 4
    REV_FACE    = 3,
    REV_HEXA    = 4,  //  = nbNodes - 4
    FORWARD     = 5
    };
  int index[][8] = {
    { 2, 1, 0, 3, 4, 0, 0, 0 },  // REV_TETRA
    { 2, 1, 0, 3, 4, 0, 0, 0 },  // REV_PYRAMID
    { 2, 1, 0, 5, 4, 3, 0, 0 },  // REV_PENTA
    { 2, 1, 0, 3, 0, 0, 0, 0 },  // REV_FACE
    { 2, 1, 0, 3, 6, 5, 4, 7 },  // REV_HEXA
    { 0, 1, 2, 3, 4, 5, 6, 7 }   // FORWARD
  };

  for ( itElem = theElems.begin(); itElem != theElems.end(); itElem++ )
  {
    const SMDS_MeshElement* elem = *itElem;
    if ( !elem || elem->GetType() == SMDSAbs_Node )
      continue;

    int nbNodes = elem->NbNodes();
    int elemType = elem->GetType();

    if (elem->IsPoly()) {
      // Polygon or Polyhedral Volume
      switch ( elemType ) {
      case SMDSAbs_Face:
        {
          vector<const SMDS_MeshNode*> poly_nodes (nbNodes);
          int iNode = 0;
          SMDS_ElemIteratorPtr itN = elem->nodesIterator();
          while (itN->more()) {
            const SMDS_MeshNode* node =
              static_cast<const SMDS_MeshNode*>(itN->next());
            TNodeNodeMap::iterator nodeMapIt = nodeMap.find(node);
            if (nodeMapIt == nodeMap.end())
              break; // not all nodes transformed
            if (needReverse) {
              // reverse mirrored faces and volumes
              poly_nodes[nbNodes - iNode - 1] = (*nodeMapIt).second;
            } else {
              poly_nodes[iNode] = (*nodeMapIt).second;
            }
            iNode++;
          }
          if ( iNode != nbNodes )
            continue; // not all nodes transformed

          if ( theTargetMesh ) {
            myLastCreatedElems.Append(aTgtMesh->AddPolygonalFace(poly_nodes));
            srcElems.Append( elem );
          }
          else if ( theCopy ) {
            myLastCreatedElems.Append(aMesh->AddPolygonalFace(poly_nodes));
            srcElems.Append( elem );
          }
          else {
            aMesh->ChangePolygonNodes(elem, poly_nodes);
          }
        }
        break;
      case SMDSAbs_Volume:
        {
          // ATTENTION: Reversing is not yet done!!!
          const SMDS_PolyhedralVolumeOfNodes* aPolyedre =
            dynamic_cast<const SMDS_PolyhedralVolumeOfNodes*>( elem );
          if (!aPolyedre) {
            MESSAGE("Warning: bad volumic element");
            continue;
          }

          vector<const SMDS_MeshNode*> poly_nodes;
          vector<int> quantities;

          bool allTransformed = true;
          int nbFaces = aPolyedre->NbFaces();
          for (int iface = 1; iface <= nbFaces && allTransformed; iface++) {
            int nbFaceNodes = aPolyedre->NbFaceNodes(iface);
            for (int inode = 1; inode <= nbFaceNodes && allTransformed; inode++) {
              const SMDS_MeshNode* node = aPolyedre->GetFaceNode(iface, inode);
              TNodeNodeMap::iterator nodeMapIt = nodeMap.find(node);
              if (nodeMapIt == nodeMap.end()) {
                allTransformed = false; // not all nodes transformed
              } else {
                poly_nodes.push_back((*nodeMapIt).second);
              }
            }
            quantities.push_back(nbFaceNodes);
          }
          if ( !allTransformed )
            continue; // not all nodes transformed

          if ( theTargetMesh ) {
            myLastCreatedElems.Append(aTgtMesh->AddPolyhedralVolume(poly_nodes, quantities));
            srcElems.Append( elem );
          }
          else if ( theCopy ) {
            myLastCreatedElems.Append(aMesh->AddPolyhedralVolume(poly_nodes, quantities));
            srcElems.Append( elem );
          }
          else {
            aMesh->ChangePolyhedronNodes(elem, poly_nodes, quantities);
          }
        }
        break;
    default:;
    }
    continue;
  }

  // Regular elements
  int* i = index[ FORWARD ];
  if ( needReverse && nbNodes > 2) // reverse mirrored faces and volumes
    if ( elemType == SMDSAbs_Face )
      i = index[ REV_FACE ];
    else
      i = index[ nbNodes - 4 ];

    if(elem->IsQuadratic()) {
      static int anIds[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
      i = anIds;
      if(needReverse) {
        if(nbNodes==3) { // quadratic edge
          static int anIds[] = {1,0,2};
          i = anIds;
        }
        else if(nbNodes==6) { // quadratic triangle
          static int anIds[] = {0,2,1,5,4,3};
          i = anIds;
        }
        else if(nbNodes==8) { // quadratic quadrangle
          static int anIds[] = {0,3,2,1,7,6,5,4};
          i = anIds;
        }
        else if(nbNodes==10) { // quadratic tetrahedron of 10 nodes
          static int anIds[] = {0,2,1,3,6,5,4,7,9,8};
          i = anIds;
        }
        else if(nbNodes==13) { // quadratic pyramid of 13 nodes
          static int anIds[] = {0,3,2,1,4,8,7,6,5,9,12,11,10};
          i = anIds;
        }
        else if(nbNodes==15) { // quadratic pentahedron with 15 nodes
          static int anIds[] = {0,2,1,3,5,4,8,7,6,11,10,9,12,14,13};
          i = anIds;
        }
        else { // nbNodes==20 - quadratic hexahedron with 20 nodes
          static int anIds[] = {0,3,2,1,4,7,6,5,11,10,9,8,15,14,13,12,16,19,18,17};
          i = anIds;
        }
      }
    }

    // find transformed nodes
    vector<const SMDS_MeshNode*> nodes(nbNodes);
    int iNode = 0;
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    while ( itN->more() ) {
      const SMDS_MeshNode* node =
        static_cast<const SMDS_MeshNode*>( itN->next() );
      TNodeNodeMap::iterator nodeMapIt = nodeMap.find( node );
      if ( nodeMapIt == nodeMap.end() )
        break; // not all nodes transformed
      nodes[ i [ iNode++ ]] = (*nodeMapIt).second;
    }
    if ( iNode != nbNodes )
      continue; // not all nodes transformed

    if ( theTargetMesh ) {
      if ( SMDS_MeshElement* copy =
           targetMeshEditor.AddElement( nodes, elem->GetType(), elem->IsPoly() )) {
        myLastCreatedElems.Append( copy );
        srcElems.Append( elem );
      }
    }
    else if ( theCopy ) {
      if ( SMDS_MeshElement* copy = AddElement( nodes, elem->GetType(), elem->IsPoly() )) {
        myLastCreatedElems.Append( copy );
        srcElems.Append( elem );
      }
    }
    else {
      // reverse element as it was reversed by transformation
      if ( nbNodes > 2 )
        aMesh->ChangeElementNodes( elem, &nodes[0], nbNodes );
    }
  }

  PGroupIDs newGroupIDs;

  if ( theMakeGroups && theCopy ||
       theMakeGroups && theTargetMesh )
    newGroupIDs = generateGroups( srcNodes, srcElems, groupPostfix, theTargetMesh );

  return newGroupIDs;
}

//=======================================================================
/*!
 * \brief Create groups of elements made during transformation
 * \param nodeGens - nodes making corresponding myLastCreatedNodes
 * \param elemGens - elements making corresponding myLastCreatedElems
 * \param postfix - to append to names of new groups
 */
//=======================================================================

SMESH_MeshEditor::PGroupIDs
SMESH_MeshEditor::generateGroups(const SMESH_SequenceOfElemPtr& nodeGens,
                                 const SMESH_SequenceOfElemPtr& elemGens,
                                 const std::string&             postfix,
                                 SMESH_Mesh*                    targetMesh)
{
  PGroupIDs newGroupIDs( new list<int> );
  SMESH_Mesh* mesh = targetMesh ? targetMesh : GetMesh();

  // Sort existing groups by types and collect their names

  // to store an old group and a generated new one
  typedef pair< SMESHDS_GroupBase*, SMDS_MeshGroup* > TOldNewGroup;
  vector< list< TOldNewGroup > > groupsByType( SMDSAbs_NbElementTypes );
  // group names
  set< string > groupNames;
  //
  SMDS_MeshGroup* nullNewGroup = (SMDS_MeshGroup*) 0;
  SMESH_Mesh::GroupIteratorPtr groupIt = GetMesh()->GetGroups();
  while ( groupIt->more() ) {
    SMESH_Group * group = groupIt->next();
    if ( !group ) continue;
    SMESHDS_GroupBase* groupDS = group->GetGroupDS();
    if ( !groupDS || groupDS->IsEmpty() ) continue;
    groupNames.insert( group->GetName() );
    groupDS->SetStoreName( group->GetName() );
    groupsByType[ groupDS->GetType() ].push_back( make_pair( groupDS, nullNewGroup ));
  }

  // Groups creation

  // loop on nodes and elements
  for ( int isNodes = 0; isNodes < 2; ++isNodes )
  {
    const SMESH_SequenceOfElemPtr& gens  = isNodes ? nodeGens : elemGens;
    const SMESH_SequenceOfElemPtr& elems = isNodes ? myLastCreatedNodes : myLastCreatedElems;
    if ( gens.Length() != elems.Length() )
      throw SMESH_Exception(LOCALIZED("invalid args"));

    // loop on created elements
    for (int iElem = 1; iElem <= elems.Length(); ++iElem )
    {
      const SMDS_MeshElement* sourceElem = gens( iElem );
      if ( !sourceElem ) {
        MESSAGE("generateGroups(): NULL source element");
        continue;
      }
      list< TOldNewGroup > & groupsOldNew = groupsByType[ sourceElem->GetType() ];
      if ( groupsOldNew.empty() ) {
        while ( iElem < gens.Length() && gens( iElem+1 ) == sourceElem )
          ++iElem; // skip all elements made by sourceElem
        continue;
      }
      // collect all elements made by sourceElem
      list< const SMDS_MeshElement* > resultElems;
      if ( const SMDS_MeshElement* resElem = elems( iElem ))
        if ( resElem != sourceElem )
          resultElems.push_back( resElem );
      while ( iElem < gens.Length() && gens( iElem+1 ) == sourceElem )
        if ( const SMDS_MeshElement* resElem = elems( ++iElem ))
          if ( resElem != sourceElem )
            resultElems.push_back( resElem );
      // do not generate element groups from node ones
      if ( sourceElem->GetType() == SMDSAbs_Node &&
           elems( iElem )->GetType() != SMDSAbs_Node )
        continue;

      // add resultElems to groups made by ones the sourceElem belongs to
      list< TOldNewGroup >::iterator gOldNew, gLast = groupsOldNew.end();
      for ( gOldNew = groupsOldNew.begin(); gOldNew != gLast; ++gOldNew )
      {
        SMESHDS_GroupBase* oldGroup = gOldNew->first;
        if ( oldGroup->Contains( sourceElem )) // sourceElem in oldGroup
        {
          SMDS_MeshGroup* & newGroup = gOldNew->second;
          if ( !newGroup )// create a new group
          {
            // make a name
            string name = oldGroup->GetStoreName();
            if ( !targetMesh ) {
              name += "_";
              name += postfix;
              int nb = 0;
              while ( !groupNames.insert( name ).second ) // name exists
              {
                if ( nb == 0 ) {
                  name += "_1";
                }
                else {
                  TCollection_AsciiString nbStr(nb+1);
                  name.resize( name.rfind('_')+1 );
                  name += nbStr.ToCString();
                }
                ++nb;
              }
            }
            // make a group
            int id;
            SMESH_Group* group = mesh->AddGroup( resultElems.back()->GetType(),
                                                 name.c_str(), id );
            SMESHDS_Group* groupDS = static_cast<SMESHDS_Group*>(group->GetGroupDS());
            newGroup = & groupDS->SMDSGroup();
            newGroupIDs->push_back( id );
          }

          // fill in a new group
          list< const SMDS_MeshElement* >::iterator resLast = resultElems.end(), resElemIt;
          for ( resElemIt = resultElems.begin(); resElemIt != resLast; ++resElemIt )
            newGroup->Add( *resElemIt );
        }
      }
    } // loop on created elements
  }// loop on nodes and elements

  return newGroupIDs;
}

//=======================================================================
//function : FindCoincidentNodes
//purpose  : Return list of group of nodes close to each other within theTolerance
//           Search among theNodes or in the whole mesh if theNodes is empty using
//           an Octree algorithm
//=======================================================================

void SMESH_MeshEditor::FindCoincidentNodes (set<const SMDS_MeshNode*> & theNodes,
                                            const double                theTolerance,
                                            TListOfListOfNodes &        theGroupsOfNodes)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  set<const SMDS_MeshNode*> nodes;
  if ( theNodes.empty() )
  { // get all nodes in the mesh
    SMDS_NodeIteratorPtr nIt = GetMeshDS()->nodesIterator();
    while ( nIt->more() )
      nodes.insert( nodes.end(),nIt->next());
  }
  else
    nodes=theNodes;
  SMESH_OctreeNode::FindCoincidentNodes ( nodes, &theGroupsOfNodes, theTolerance);

}

//=======================================================================
/*!
 * \brief Implementation of search for the node closest to point
 */
//=======================================================================

struct SMESH_NodeSearcherImpl: public SMESH_NodeSearcher
{
  /*!
   * \brief Constructor
   */
  SMESH_NodeSearcherImpl( const SMESHDS_Mesh* theMesh )
  {
    set<const SMDS_MeshNode*> nodes;
    if ( theMesh ) {
      SMDS_NodeIteratorPtr nIt = theMesh->nodesIterator();
      while ( nIt->more() )
        nodes.insert( nodes.end(), nIt->next() );
    }
    myOctreeNode = new SMESH_OctreeNode(nodes) ;
  }
  /*!
   * \brief Do it's job
   */
  const SMDS_MeshNode* FindClosestTo( const gp_Pnt& thePnt )
  {
    SMDS_MeshNode tgtNode( thePnt.X(), thePnt.Y(), thePnt.Z() );
    list<const SMDS_MeshNode*> nodes;
    const double precision = 1e-6;
    //myOctreeNode->NodesAround( &tgtNode, &nodes, precision );

    double minSqDist = DBL_MAX;
    Bnd_B3d box;
    if ( nodes.empty() )  // get all nodes of OctreeNode's closest to thePnt
    {
      // sort leafs by their distance from thePnt
      typedef map< double, SMESH_OctreeNode* > TDistTreeMap;
      TDistTreeMap treeMap;
      list< SMESH_OctreeNode* > treeList;
      list< SMESH_OctreeNode* >::iterator trIt;
      treeList.push_back( myOctreeNode );
      for ( trIt = treeList.begin(); trIt != treeList.end(); ++trIt)
      {
        SMESH_OctreeNode* tree = *trIt;
        if ( !tree->isLeaf() ) { // put children to the queue
          SMESH_OctreeNodeIteratorPtr cIt = tree->GetChildrenIterator();
          while ( cIt->more() )
            treeList.push_back( cIt->next() );
        }
        else if ( tree->NbNodes() ) { // put tree to treeMap
          tree->getBox( box );
          double sqDist = thePnt.SquareDistance( 0.5 * ( box.CornerMin() + box.CornerMax() ));
          pair<TDistTreeMap::iterator,bool> it_in = treeMap.insert( make_pair( sqDist, tree ));
          if ( !it_in.second ) // not unique distance to box center
            treeMap.insert( it_in.first, make_pair( sqDist - 1e-13*treeMap.size(), tree ));
        }
      }
      // find distance after which there is no sense to check tree's
      double sqLimit = DBL_MAX;
      TDistTreeMap::iterator sqDist_tree = treeMap.begin();
      if ( treeMap.size() > 5 ) {
        SMESH_OctreeNode* closestTree = sqDist_tree->second;
        closestTree->getBox( box );
        double limit = sqrt( sqDist_tree->first ) + sqrt ( box.SquareExtent() );
        sqLimit = limit * limit;
      }
      // get all nodes from trees
      for ( ; sqDist_tree != treeMap.end(); ++sqDist_tree) {
        if ( sqDist_tree->first > sqLimit )
          break;
        SMESH_OctreeNode* tree = sqDist_tree->second;
        tree->NodesAround( tree->GetNodeIterator()->next(), &nodes );
      }
    }
    // find closest among nodes
    minSqDist = DBL_MAX;
    const SMDS_MeshNode* closestNode = 0;
    list<const SMDS_MeshNode*>::iterator nIt = nodes.begin();
    for ( ; nIt != nodes.end(); ++nIt ) {
      double sqDist = thePnt.SquareDistance( TNodeXYZ( *nIt ) );
      if ( minSqDist > sqDist ) {
        closestNode = *nIt;
        minSqDist = sqDist;
      }
    }
    return closestNode;
  }
  /*!
   * \brief Destructor
   */
  ~SMESH_NodeSearcherImpl() { delete myOctreeNode; }
private:
  SMESH_OctreeNode* myOctreeNode;
};

//=======================================================================
/*!
 * \brief Return SMESH_NodeSearcher
 */
//=======================================================================

SMESH_NodeSearcher* SMESH_MeshEditor::GetNodeSearcher() 
{
  return new SMESH_NodeSearcherImpl( GetMeshDS() );
}

//=======================================================================
//function : SimplifyFace
//purpose  :
//=======================================================================
int SMESH_MeshEditor::SimplifyFace (const vector<const SMDS_MeshNode *> faceNodes,
                                    vector<const SMDS_MeshNode *>&      poly_nodes,
                                    vector<int>&                        quantities) const
{
  int nbNodes = faceNodes.size();

  if (nbNodes < 3)
    return 0;

  set<const SMDS_MeshNode*> nodeSet;

  // get simple seq of nodes
  //const SMDS_MeshNode* simpleNodes[ nbNodes ];
  vector<const SMDS_MeshNode*> simpleNodes( nbNodes );
  int iSimple = 0, nbUnique = 0;

  simpleNodes[iSimple++] = faceNodes[0];
  nbUnique++;
  for (int iCur = 1; iCur < nbNodes; iCur++) {
    if (faceNodes[iCur] != simpleNodes[iSimple - 1]) {
      simpleNodes[iSimple++] = faceNodes[iCur];
      if (nodeSet.insert( faceNodes[iCur] ).second)
        nbUnique++;
    }
  }
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
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  SMESHDS_Mesh* aMesh = GetMeshDS();

  TNodeNodeMap nodeNodeMap; // node to replace - new node
  set<const SMDS_MeshElement*> elems; // all elements with changed nodes
  list< int > rmElemIds, rmNodeIds;

  // Fill nodeNodeMap and elems

  TListOfListOfNodes::iterator grIt = theGroupsOfNodes.begin();
  for ( ; grIt != theGroupsOfNodes.end(); grIt++ ) {
    list<const SMDS_MeshNode*>& nodes = *grIt;
    list<const SMDS_MeshNode*>::iterator nIt = nodes.begin();
    const SMDS_MeshNode* nToKeep = *nIt;
    for ( ++nIt; nIt != nodes.end(); nIt++ ) {
      const SMDS_MeshNode* nToRemove = *nIt;
      nodeNodeMap.insert( TNodeNodeMap::value_type( nToRemove, nToKeep ));
      if ( nToRemove != nToKeep ) {
        rmNodeIds.push_back( nToRemove->GetID() );
        AddToSameGroups( nToKeep, nToRemove, aMesh );
      }

      SMDS_ElemIteratorPtr invElemIt = nToRemove->GetInverseElementIterator();
      while ( invElemIt->more() ) {
        const SMDS_MeshElement* elem = invElemIt->next();
          elems.insert(elem);
      }
    }
  }
  // Change element nodes or remove an element

  set<const SMDS_MeshElement*>::iterator eIt = elems.begin();
  for ( ; eIt != elems.end(); eIt++ ) {
    const SMDS_MeshElement* elem = *eIt;
    int nbNodes = elem->NbNodes();
    int aShapeId = FindShape( elem );

    set<const SMDS_MeshNode*> nodeSet;
    vector< const SMDS_MeshNode*> curNodes( nbNodes ), uniqueNodes( nbNodes );
    int iUnique = 0, iCur = 0, nbRepl = 0;
    vector<int> iRepl( nbNodes );

    // get new seq of nodes
    SMDS_ElemIteratorPtr itN = elem->nodesIterator();
    while ( itN->more() ) {
      const SMDS_MeshNode* n =
        static_cast<const SMDS_MeshNode*>( itN->next() );

      TNodeNodeMap::iterator nnIt = nodeNodeMap.find( n );
      if ( nnIt != nodeNodeMap.end() ) { // n sticks
        n = (*nnIt).second;
        // BUG 0020185: begin
        {
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
        }
        // BUG 0020185: end
        iRepl[ nbRepl++ ] = iCur;
      }
      curNodes[ iCur ] = n;
      bool isUnique = nodeSet.insert( n ).second;
      if ( isUnique )
        uniqueNodes[ iUnique++ ] = n;
      iCur++;
    }

    // Analyse element topology after replacement

    bool isOk = true;
    int nbUniqueNodes = nodeSet.size();
    if ( nbNodes != nbUniqueNodes ) { // some nodes stick
      // Polygons and Polyhedral volumes
      if (elem->IsPoly()) {

        if (elem->GetType() == SMDSAbs_Face) {
          // Polygon
          vector<const SMDS_MeshNode *> face_nodes (nbNodes);
          int inode = 0;
          for (; inode < nbNodes; inode++) {
            face_nodes[inode] = curNodes[inode];
          }

          vector<const SMDS_MeshNode *> polygons_nodes;
          vector<int> quantities;
          int nbNew = SimplifyFace(face_nodes, polygons_nodes, quantities);

          if (nbNew > 0) {
            inode = 0;
            for (int iface = 0; iface < nbNew - 1; iface++) {
              int nbNodes = quantities[iface];
              vector<const SMDS_MeshNode *> poly_nodes (nbNodes);
              for (int ii = 0; ii < nbNodes; ii++, inode++) {
                poly_nodes[ii] = polygons_nodes[inode];
              }
              SMDS_MeshElement* newElem = aMesh->AddPolygonalFace(poly_nodes);
              myLastCreatedElems.Append(newElem);
              if (aShapeId)
                aMesh->SetMeshElementOnShape(newElem, aShapeId);
            }
            aMesh->ChangeElementNodes(elem, &polygons_nodes[inode], quantities[nbNew - 1]);
          }
          else {
            rmElemIds.push_back(elem->GetID());
          }

        }
        else if (elem->GetType() == SMDSAbs_Volume) {
          // Polyhedral volume
          if (nbUniqueNodes < 4) {
            rmElemIds.push_back(elem->GetID());
          }
          else {
            // each face has to be analized in order to check volume validity
            const SMDS_PolyhedralVolumeOfNodes* aPolyedre =
              static_cast<const SMDS_PolyhedralVolumeOfNodes*>( elem );
            if (aPolyedre) {
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
                aMesh->ChangePolyhedronNodes(elem, poly_nodes, quantities);
              else
                rmElemIds.push_back(elem->GetID());

            }
            else {
              rmElemIds.push_back(elem->GetID());
            }
          }
        }
        else {
        }

        continue;
      }

      // Regular elements
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
          if(nbRepl==3) {
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
          break;
        }
        //////////////////////////////////// HEXAHEDRON
        isOk = false;
        SMDS_VolumeTool hexa (elem);
        hexa.SetExternalNormal();
        if ( nbUniqueNodes == 4 && nbRepl == 6 ) {
          //////////////////////// ---> tetrahedron
          for ( int iFace = 0; iFace < 6; iFace++ ) {
            const int *ind = hexa.GetFaceNodesIndices( iFace ); // indices of face nodes
            if (curNodes[ind[ 0 ]] == curNodes[ind[ 1 ]] &&
                curNodes[ind[ 0 ]] == curNodes[ind[ 2 ]] &&
                curNodes[ind[ 0 ]] == curNodes[ind[ 3 ]] ) {
              // one face turns into a point ...
              int iOppFace = hexa.GetOppFaceIndex( iFace );
              ind = hexa.GetFaceNodesIndices( iOppFace );
              int nbStick = 0;
              iUnique = 2; // reverse a tetrahedron bottom
              for ( iCur = 0; iCur < 4 && nbStick < 2; iCur++ ) {
                if ( curNodes[ind[ iCur ]] == curNodes[ind[ iCur + 1 ]] )
                  nbStick++;
                else if ( iUnique >= 0 )
                  uniqueNodes[ iUnique-- ] = curNodes[ind[ iCur ]];
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
        break;
      } // HEXAHEDRON

      default:
        isOk = false;
      } // switch ( nbNodes )

    } // if ( nbNodes != nbUniqueNodes ) // some nodes stick

    if ( isOk ) {
      if (elem->IsPoly() && elem->GetType() == SMDSAbs_Volume) {
        // Change nodes of polyedre
        const SMDS_PolyhedralVolumeOfNodes* aPolyedre =
          static_cast<const SMDS_PolyhedralVolumeOfNodes*>( elem );
        if (aPolyedre) {
          int nbFaces = aPolyedre->NbFaces();

          vector<const SMDS_MeshNode *> poly_nodes;
          vector<int> quantities (nbFaces);

          for (int iface = 1; iface <= nbFaces; iface++) {
            int inode, nbFaceNodes = aPolyedre->NbFaceNodes(iface);
            quantities[iface - 1] = nbFaceNodes;

            for (inode = 1; inode <= nbFaceNodes; inode++) {
              const SMDS_MeshNode* curNode = aPolyedre->GetFaceNode(iface, inode);

              TNodeNodeMap::iterator nnIt = nodeNodeMap.find( curNode );
              if (nnIt != nodeNodeMap.end()) { // curNode sticks
                curNode = (*nnIt).second;
              }
              poly_nodes.push_back(curNode);
            }
          }
          aMesh->ChangePolyhedronNodes( elem, poly_nodes, quantities );
        }
      }
      else {
        // Change regular element or polygon
        aMesh->ChangeElementNodes( elem, & uniqueNodes[0], nbUniqueNodes );
      }
    }
    else {
      // Remove invalid regular element or invalid polygon
      rmElemIds.push_back( elem->GetID() );
    }

  } // loop on elements

  // Remove equal nodes and bad elements

  Remove( rmNodeIds, true );
  Remove( rmElemIds, false );

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

  void Set(const SMDS_MeshElement* e) const
    { myElem = e; }


 private:
  mutable const SMDS_MeshElement* myElem;
};

//=======================================================================
//function : FindEqualElements
//purpose  : Return list of group of elements built on the same nodes.
//           Search among theElements or in the whole mesh if theElements is empty
//=======================================================================
void SMESH_MeshEditor::FindEqualElements(set<const SMDS_MeshElement*> & theElements,
					 TListOfListOfElementsID &      theGroupsOfElementsID)
{
  myLastCreatedElems.Clear();
  myLastCreatedNodes.Clear();

  typedef set<const SMDS_MeshElement*> TElemsSet;
  typedef map< SortableElement, int > TMapOfNodeSet;
  typedef list<int> TGroupOfElems;

  TElemsSet elems;
  if ( theElements.empty() )
  { // get all elements in the mesh
    SMDS_ElemIteratorPtr eIt = GetMeshDS()->elementsIterator();
    while ( eIt->more() )
      elems.insert( elems.end(), eIt->next());
  }
  else
    elems = theElements;

  vector< TGroupOfElems > arrayOfGroups;
  TGroupOfElems groupOfElems;
  TMapOfNodeSet mapOfNodeSet;

  TElemsSet::iterator elemIt = elems.begin();
  for ( int i = 0, j=0; elemIt != elems.end(); ++elemIt, ++j ) {
    const SMDS_MeshElement* curElem = *elemIt;
    SortableElement SE(curElem);
    int ind = -1;
    // check uniqueness
    pair< TMapOfNodeSet::iterator, bool> pp = mapOfNodeSet.insert(make_pair(SE, i));
    if( !(pp.second) ) {
      TMapOfNodeSet::iterator& itSE = pp.first;
      ind = (*itSE).second;
      arrayOfGroups[ind].push_back(curElem->GetID());
    }
    else {
      groupOfElems.clear();
      groupOfElems.push_back(curElem->GetID());
      arrayOfGroups.push_back(groupOfElems);
      i++;
    }
  }

  vector< TGroupOfElems >::iterator groupIt = arrayOfGroups.begin();
  for ( ; groupIt != arrayOfGroups.end(); ++groupIt ) {
    groupOfElems = *groupIt;
    if ( groupOfElems.size() > 1 ) {
      groupOfElems.sort();
      theGroupsOfElementsID.push_back(groupOfElems);
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
  set<const SMDS_MeshElement*> aMeshElements; /* empty input -
						 to merge equal elements in the whole mesh */
  TListOfListOfElementsID aGroupsOfElementsID;
  FindEqualElements(aMeshElements, aGroupsOfElementsID);
  MergeElements(aGroupsOfElementsID);
}

//=======================================================================
//function : FindFaceInSet
//purpose  : Return a face having linked nodes n1 and n2 and which is
//           - not in avoidSet,
//           - in elemSet provided that !elemSet.empty()
//=======================================================================

const SMDS_MeshElement*
  SMESH_MeshEditor::FindFaceInSet(const SMDS_MeshNode*    n1,
                                  const SMDS_MeshNode*    n2,
                                  const TIDSortedElemSet& elemSet,
                                  const TIDSortedElemSet& avoidSet)

{
  SMDS_ElemIteratorPtr invElemIt = n1->GetInverseElementIterator(SMDSAbs_Face);
  while ( invElemIt->more() ) { // loop on inverse elements of n1
    const SMDS_MeshElement* elem = invElemIt->next();
    if (avoidSet.find( elem ) != avoidSet.end() )
      continue;
    if ( !elemSet.empty() && elemSet.find( elem ) == elemSet.end())
      continue;
    // get face nodes and find index of n1
    int i1, nbN = elem->NbNodes(), iNode = 0;
    //const SMDS_MeshNode* faceNodes[ nbN ], *n;
    vector<const SMDS_MeshNode*> faceNodes( nbN );
    const SMDS_MeshNode* n;
    SMDS_ElemIteratorPtr nIt = elem->nodesIterator();
    while ( nIt->more() ) {
      faceNodes[ iNode ] = static_cast<const SMDS_MeshNode*>( nIt->next() );
      if ( faceNodes[ iNode++ ] == n1 )
        i1 = iNode - 1;
    }
    // find a n2 linked to n1
    if(!elem->IsQuadratic()) {
      for ( iNode = 0; iNode < 2; iNode++ ) {
        if ( iNode ) // node before n1
          n = faceNodes[ i1 == 0 ? nbN - 1 : i1 - 1 ];
        else         // node after n1
          n = faceNodes[ i1 + 1 == nbN ? 0 : i1 + 1 ];
        if ( n == n2 )
          return elem;
      }
    }
    else { // analysis for quadratic elements
      bool IsFind = false;
      // check using only corner nodes
      for ( iNode = 0; iNode < 2; iNode++ ) {
        if ( iNode ) // node before n1
          n = faceNodes[ i1 == 0 ? nbN/2 - 1 : i1 - 1 ];
        else         // node after n1
          n = faceNodes[ i1 + 1 == nbN/2 ? 0 : i1 + 1 ];
        if ( n == n2 )
          IsFind = true;
      }
      if(IsFind) {
        return elem;
      }
      else {
        // check using all nodes
        const SMDS_QuadraticFaceOfNodes* F =
          static_cast<const SMDS_QuadraticFaceOfNodes*>(elem);
        // use special nodes iterator
        iNode = 0;
        SMDS_NodeIteratorPtr anIter = F->interlacedNodesIterator();
        while ( anIter->more() ) {
          faceNodes[iNode] = static_cast<const SMDS_MeshNode*>(anIter->next());
          if ( faceNodes[ iNode++ ] == n1 )
            i1 = iNode - 1;
        }
        for ( iNode = 0; iNode < 2; iNode++ ) {
          if ( iNode ) // node before n1
            n = faceNodes[ i1 == 0 ? nbN - 1 : i1 - 1 ];
          else         // node after n1
            n = faceNodes[ i1 + 1 == nbN ? 0 : i1 + 1 ];
          if ( n == n2 ) {
            return elem;
          }
        }
      }
    } // end analysis for quadratic elements
  }
  return 0;
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
  return SMESH_MeshEditor::FindFaceInSet( n1, n2, elemSet, avoidSet );
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

  //vector<const SMDS_MeshNode*> nodes;
  const SMDS_MeshNode *nIgnore = theFirstNode, *nStart = theSecondNode;
  set < const SMDS_MeshElement* > foundElems;
  bool needTheLast = ( theLastNode != 0 );

  while ( nStart != theLastNode ) {
    if ( nStart == theFirstNode )
      return !needTheLast;

    // find all free border faces sharing form nStart

    list< const SMDS_MeshElement* > curElemList;
    list< const SMDS_MeshNode* > nStartList;
    SMDS_ElemIteratorPtr invElemIt = nStart->GetInverseElementIterator(SMDSAbs_Face);
    while ( invElemIt->more() ) {
      const SMDS_MeshElement* e = invElemIt->next();
      if ( e == curElem || foundElems.insert( e ).second ) {
        // get nodes
        int iNode = 0, nbNodes = e->NbNodes();
        //const SMDS_MeshNode* nodes[nbNodes+1];
        vector<const SMDS_MeshNode*> nodes(nbNodes+1);
        
        if(e->IsQuadratic()) {
          const SMDS_QuadraticFaceOfNodes* F =
            static_cast<const SMDS_QuadraticFaceOfNodes*>(e);
          // use special nodes iterator
          SMDS_NodeIteratorPtr anIter = F->interlacedNodesIterator();
          while( anIter->more() ) {
            nodes[ iNode++ ] = anIter->next();
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
        if ( ! FindFreeBorder( nStart, *nStartIt, theLastNode, *cNL, *cFL )) {
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

  list< const SMDS_MeshNode* > nSide[ 2 ];
  list< const SMDS_MeshElement* > eSide[ 2 ];
  list< const SMDS_MeshNode* >::iterator nIt[ 2 ];
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

    // 1. Since sewing may brake if there are volumes to split on the side 2,
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
            const SMDS_QuadraticFaceOfNodes* F =
              static_cast<const SMDS_QuadraticFaceOfNodes*>(elem);
            // use special nodes iterator
            SMDS_NodeIteratorPtr anIter = F->interlacedNodesIterator();
            while( anIter->more() ) {
              nodes[ iNode ] = anIter->next();
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

  int nbNodes[]  = { nSide[0].size(), nSide[1].size() };
  int maxNbNodes = Max( nbNodes[0], nbNodes[1] );

  TListOfListOfNodes nodeGroupsToMerge;
  if ( nbNodes[0] == nbNodes[1] ||
      ( theSideIsFreeBorder && !theSideThirdNode)) {

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
    //double param[ 2 ][ maxNbNodes ];
    double* param[ 2 ];
    param[0] = new double [ maxNbNodes ];
    param[1] = new double [ maxNbNodes ];
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
      double minParam = Min( param[ 0 ][ i[0] ], param[ 1 ][ i[1] ]);
      double maxParam = Max( param[ 0 ][ i[0] ], param[ 1 ][ i[1] ]);
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
        const SMDS_MeshElement* elem = *eIt[ intoBord ];
        const SMDS_MeshNode*    n1   = nPrev[ intoBord ];
        const SMDS_MeshNode*    n2   = *nIt[ intoBord ];
        const SMDS_MeshNode*    nIns = *nIt[ 1 - intoBord ];
        if ( intoBord == 1 ) {
          // move node of the border to be on a link of elem of the side
          gp_XYZ p1 (n1->X(), n1->Y(), n1->Z());
          gp_XYZ p2 (n2->X(), n2->Y(), n2->Z());
          double ratio = du / ( param[ 1 ][ i[1] ] - param[ 1 ][ i[1]-1 ]);
          gp_XYZ p = p2 * ( 1 - ratio ) + p1 * ratio;
          GetMeshDS()->MoveNode( nIns, p.X(), p.Y(), p.Z() );
        }
        insertMapIt = insertMap.find( elem );
        bool notFound = ( insertMapIt == insertMap.end() );
        bool otherLink = ( !notFound && (*insertMapIt).second.front() != n1 );
        if ( otherLink ) {
          // insert into another link of the same element:
          // 1. perform insertion into the other link of the elem
          list<const SMDS_MeshNode*> & nodeList = (*insertMapIt).second;
          const SMDS_MeshNode* n12 = nodeList.front(); nodeList.pop_front();
          const SMDS_MeshNode* n22 = nodeList.front(); nodeList.pop_front();
          InsertNodesIntoLink( elem, n12, n22, nodeList, toCreatePolygons );
          // 2. perform insertion into the link of adjacent faces
          while (true) {
            const SMDS_MeshElement* adjElem = findAdjacentFace( n12, n22, elem );
            if ( adjElem )
              InsertNodesIntoLink( adjElem, n12, n22, nodeList, toCreatePolygons );
            else
              break;
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
          insertMapIt = insertMap.insert
            ( TElemOfNodeListMap::value_type( elem, list<const SMDS_MeshNode*>() )).first;
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

      if ( !theSideIsFreeBorder ) {
        // look for and insert nodes into the faces adjacent to elem
        while (true) {
          const SMDS_MeshElement* adjElem = findAdjacentFace( n1, n2, elem );
          if ( adjElem )
            InsertNodesIntoLink( adjElem, n1, n2, nodeList, toCreatePolygons );
          else
            break;
        }
      }
      if (toCreatePolyedrs) {
        // perform insertion into the links of adjacent volumes
        UpdateVolumes(n1, n2, nodeList);
      }
    }

    delete param[0];
    delete param[1];
  } // end: insert new nodes

  MergeNodes ( nodeGroupsToMerge );

  return aResult;
}

//=======================================================================
//function : InsertNodesIntoLink
//purpose  : insert theNodesToInsert into theFace between theBetweenNode1
//           and theBetweenNode2 and split theElement
//=======================================================================

void SMESH_MeshEditor::InsertNodesIntoLink(const SMDS_MeshElement*     theFace,
                                           const SMDS_MeshNode*        theBetweenNode1,
                                           const SMDS_MeshNode*        theBetweenNode2,
                                           list<const SMDS_MeshNode*>& theNodesToInsert,
                                           const bool                  toCreatePoly)
{
  if ( theFace->GetType() != SMDSAbs_Face ) return;

  // find indices of 2 link nodes and of the rest nodes
  int iNode = 0, il1, il2, i3, i4;
  il1 = il2 = i3 = i4 = -1;
  //const SMDS_MeshNode* nodes[ theFace->NbNodes() ];
  vector<const SMDS_MeshNode*> nodes( theFace->NbNodes() );

  if(theFace->IsQuadratic()) {
    const SMDS_QuadraticFaceOfNodes* F =
      static_cast<const SMDS_QuadraticFaceOfNodes*>(theFace);
    // use special nodes iterator
    SMDS_NodeIteratorPtr anIter = F->interlacedNodesIterator();
    while( anIter->more() ) {
      const SMDS_MeshNode* n = anIter->next();
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
  }
  else {
    SMDS_ElemIteratorPtr nodeIt = theFace->nodesIterator();
    while ( nodeIt->more() ) {
      const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
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

    if(theFace->IsQuadratic()) {
      const SMDS_QuadraticFaceOfNodes* F =
        static_cast<const SMDS_QuadraticFaceOfNodes*>(theFace);
      // use special nodes iterator
      SMDS_NodeIteratorPtr anIter = F->interlacedNodesIterator();
      while( anIter->more()  && !isFLN ) {
        const SMDS_MeshNode* n = anIter->next();
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
        poly_nodes[iNode++] = anIter->next();
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

    // edit or replace the face
    SMESHDS_Mesh *aMesh = GetMeshDS();

    if (theFace->IsPoly()) {
      aMesh->ChangePolygonNodes(theFace, poly_nodes);
    }
    else {
      int aShapeId = FindShape( theFace );

      SMDS_MeshElement* newElem = aMesh->AddPolygonalFace(poly_nodes);
      myLastCreatedElems.Append(newElem);
      if ( aShapeId && newElem )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );

      aMesh->RemoveElement(theFace);
    }
    return;
  }

  if( !theFace->IsQuadratic() ) {

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
    SMESHDS_Mesh *aMesh = GetMeshDS();
    int aShapeId = FindShape( theFace );

    i1 = 0; i2 = 1;
    for ( iSplit = 0; iSplit < nbSplits - 1; iSplit++ ) {
      SMDS_MeshElement* newElem = 0;
      if ( iSplit == iBestQuad )
        newElem = aMesh->AddFace (linkNodes[ i1++ ],
                                  linkNodes[ i2++ ],
                                  nodes[ i3 ],
                                  nodes[ i4 ]);
      else
        newElem = aMesh->AddFace (linkNodes[ i1++ ],
                                  linkNodes[ i2++ ],
                                  nodes[ iSplit < iBestQuad ? i4 : i3 ]);
      myLastCreatedElems.Append(newElem);
      if ( aShapeId && newElem )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
    }

    // change nodes of theFace
    const SMDS_MeshNode* newNodes[ 4 ];
    newNodes[ 0 ] = linkNodes[ i1 ];
    newNodes[ 1 ] = linkNodes[ i2 ];
    newNodes[ 2 ] = nodes[ iSplit >= iBestQuad ? i3 : i4 ];
    newNodes[ 3 ] = nodes[ i4 ];
    aMesh->ChangeElementNodes( theFace, newNodes, iSplit == iBestQuad ? 4 : 3 );
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
    SMESHDS_Mesh *aMesh = GetMeshDS();
    int aShapeId = FindShape( theFace );

    int n1,n2,n3;
    if(nbFaceNodes==6) { // quadratic triangle
      SMDS_MeshElement* newElem =
        aMesh->AddFace(nodes[3],nodes[4],nodes[5]);
      myLastCreatedElems.Append(newElem);
      if ( aShapeId && newElem )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
      if(theFace->IsMediumNode(nodes[il1])) {
        // create quadrangle
        newElem = aMesh->AddFace(nodes[0],nodes[1],nodes[3],nodes[5]);
        myLastCreatedElems.Append(newElem);
        if ( aShapeId && newElem )
          aMesh->SetMeshElementOnShape( newElem, aShapeId );
        n1 = 1;
        n2 = 2;
        n3 = 3;
      }
      else {
        // create quadrangle
        newElem = aMesh->AddFace(nodes[1],nodes[2],nodes[3],nodes[5]);
        myLastCreatedElems.Append(newElem);
        if ( aShapeId && newElem )
          aMesh->SetMeshElementOnShape( newElem, aShapeId );
        n1 = 0;
        n2 = 1;
        n3 = 5;
      }
    }
    else { // nbFaceNodes==8 - quadratic quadrangle
      SMDS_MeshElement* newElem =
        aMesh->AddFace(nodes[3],nodes[4],nodes[5]);
      myLastCreatedElems.Append(newElem);
      if ( aShapeId && newElem )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
      newElem = aMesh->AddFace(nodes[5],nodes[6],nodes[7]);
      myLastCreatedElems.Append(newElem);
      if ( aShapeId && newElem )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
      newElem = aMesh->AddFace(nodes[5],nodes[7],nodes[3]);
      myLastCreatedElems.Append(newElem);
      if ( aShapeId && newElem )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
      if(theFace->IsMediumNode(nodes[il1])) {
        // create quadrangle
        newElem = aMesh->AddFace(nodes[0],nodes[1],nodes[3],nodes[7]);
        myLastCreatedElems.Append(newElem);
        if ( aShapeId && newElem )
          aMesh->SetMeshElementOnShape( newElem, aShapeId );
        n1 = 1;
        n2 = 2;
        n3 = 3;
      }
      else {
        // create quadrangle
        newElem = aMesh->AddFace(nodes[1],nodes[2],nodes[3],nodes[7]);
        myLastCreatedElems.Append(newElem);
        if ( aShapeId && newElem )
          aMesh->SetMeshElementOnShape( newElem, aShapeId );
        n1 = 0;
        n2 = 1;
        n3 = 7;
      }
    }
    // create needed triangles using n1,n2,n3 and inserted nodes
    int nbn = 2 + aNodesToInsert.size();
    //const SMDS_MeshNode* aNodes[nbn];
    vector<const SMDS_MeshNode*> aNodes(nbn);
    aNodes[0] = nodes[n1];
    aNodes[nbn-1] = nodes[n2];
    list<const SMDS_MeshNode*>::iterator nIt = aNodesToInsert.begin();
    for ( iNode = 1; nIt != aNodesToInsert.end(); nIt++ ) {
      aNodes[iNode++] = *nIt;
    }
    for(i=1; i<nbn; i++) {
      SMDS_MeshElement* newElem =
        aMesh->AddFace(aNodes[i-1],aNodes[i],nodes[n3]);
      myLastCreatedElems.Append(newElem);
      if ( aShapeId && newElem )
        aMesh->SetMeshElementOnShape( newElem, aShapeId );
    }
    // remove old quadratic face
    aMesh->RemoveElement(theFace);
  }
}

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

    // Replace or update the volume
    SMESHDS_Mesh *aMesh = GetMeshDS();

    if (elem->IsPoly()) {
      aMesh->ChangePolyhedronNodes(elem, poly_nodes, quantities);

    }
    else {
      int aShapeId = FindShape( elem );

      SMDS_MeshElement* newElem =
        aMesh->AddPolyhedralVolume(poly_nodes, quantities);
      myLastCreatedElems.Append(newElem);
      if (aShapeId && newElem)
        aMesh->SetMeshElementOnShape(newElem, aShapeId);

      aMesh->RemoveElement(elem);
    }
  }
}

//=======================================================================
/*!
 * \brief Convert elements contained in a submesh to quadratic
 * \retval int - nb of checked elements
 */
//=======================================================================

int SMESH_MeshEditor::convertElemToQuadratic(SMESHDS_SubMesh *   theSm,
                                             SMESH_MesherHelper& theHelper,
                                             const bool          theForce3d)
{
  int nbElem = 0;
  if( !theSm ) return nbElem;

  const bool notFromGroups = false;
  SMDS_ElemIteratorPtr ElemItr = theSm->GetElements();
  while(ElemItr->more())
  {
    nbElem++;
    const SMDS_MeshElement* elem = ElemItr->next();
    if( !elem || elem->IsQuadratic() ) continue;

    int id = elem->GetID();
    int nbNodes = elem->NbNodes();
    vector<const SMDS_MeshNode *> aNds (nbNodes);

    for(int i = 0; i < nbNodes; i++)
    {
      aNds[i] = elem->GetNode(i);
    }
    SMDSAbs_ElementType aType = elem->GetType();

    GetMeshDS()->RemoveFreeElement(elem, theSm, notFromGroups);

    const SMDS_MeshElement* NewElem = 0;

    switch( aType )
    {
    case SMDSAbs_Edge :
    {
      NewElem = theHelper.AddEdge(aNds[0], aNds[1], id, theForce3d);
      break;
    }
    case SMDSAbs_Face :
    {
      switch(nbNodes)
      {
      case 3:
	NewElem = theHelper.AddFace(aNds[0], aNds[1], aNds[2], id, theForce3d);
	break;
      case 4:
	NewElem = theHelper.AddFace(aNds[0], aNds[1], aNds[2], aNds[3], id, theForce3d);
	break;
      default:
	continue;
      }
      break;
    }
    case SMDSAbs_Volume :
    {
      switch(nbNodes)
      {
      case 4:
	NewElem = theHelper.AddVolume(aNds[0], aNds[1], aNds[2], aNds[3], id, theForce3d);
	break;
      case 6:
	NewElem = theHelper.AddVolume(aNds[0], aNds[1], aNds[2], aNds[3], aNds[4], aNds[5], id, theForce3d);
	break;
      case 8:
	NewElem = theHelper.AddVolume(aNds[0], aNds[1], aNds[2], aNds[3],
                                      aNds[4], aNds[5], aNds[6], aNds[7], id, theForce3d);
	break;
      default:
	continue;
      }
      break;
    }
    default :
      continue;
    }
    ReplaceElemInGroups( elem, NewElem, GetMeshDS());
    if( NewElem )
      theSm->AddElement( NewElem );
  }
  return nbElem;
}

//=======================================================================
//function : ConvertToQuadratic
//purpose  :
//=======================================================================
void SMESH_MeshEditor::ConvertToQuadratic(const bool theForce3d)
{
  SMESHDS_Mesh* meshDS = GetMeshDS();

  SMESH_MesherHelper aHelper(*myMesh);
  aHelper.SetIsQuadratic( true );
  const bool notFromGroups = false;

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
  int totalNbElems = meshDS->NbEdges() + meshDS->NbFaces() + meshDS->NbVolumes();
  if ( nbCheckedElems < totalNbElems ) // not all elements are in submeshes
  {
    SMESHDS_SubMesh *smDS = 0;
    SMDS_EdgeIteratorPtr aEdgeItr = meshDS->edgesIterator();
    while(aEdgeItr->more())
    {
      const SMDS_MeshEdge* edge = aEdgeItr->next();
      if(edge && !edge->IsQuadratic())
      {
	int id = edge->GetID();
	const SMDS_MeshNode* n1 = edge->GetNode(0);
	const SMDS_MeshNode* n2 = edge->GetNode(1);

	meshDS->RemoveFreeElement(edge, smDS, notFromGroups);

        const SMDS_MeshEdge* NewEdge = aHelper.AddEdge(n1, n2, id, theForce3d);
        ReplaceElemInGroups( edge, NewEdge, GetMeshDS());
      }
    }
    SMDS_FaceIteratorPtr aFaceItr = meshDS->facesIterator();
    while(aFaceItr->more())
    {
      const SMDS_MeshFace* face = aFaceItr->next();
      if(!face || face->IsQuadratic() ) continue;

      int id = face->GetID();
      int nbNodes = face->NbNodes();
      vector<const SMDS_MeshNode *> aNds (nbNodes);

      for(int i = 0; i < nbNodes; i++)
      {
	aNds[i] = face->GetNode(i);
      }

      meshDS->RemoveFreeElement(face, smDS, notFromGroups);

      SMDS_MeshFace * NewFace = 0;
      switch(nbNodes)
      {
      case 3:
	NewFace = aHelper.AddFace(aNds[0], aNds[1], aNds[2], id, theForce3d);
	break;
      case 4:
	NewFace = aHelper.AddFace(aNds[0], aNds[1], aNds[2], aNds[3], id, theForce3d);
	break;
      default:
	continue;
      }
      ReplaceElemInGroups( face, NewFace, GetMeshDS());
    }
    SMDS_VolumeIteratorPtr aVolumeItr = meshDS->volumesIterator();
    while(aVolumeItr->more())
    {
      const SMDS_MeshVolume* volume = aVolumeItr->next();
      if(!volume || volume->IsQuadratic() ) continue;

      int id = volume->GetID();
      int nbNodes = volume->NbNodes();
      vector<const SMDS_MeshNode *> aNds (nbNodes);

      for(int i = 0; i < nbNodes; i++)
      {
	aNds[i] = volume->GetNode(i);
      }

      meshDS->RemoveFreeElement(volume, smDS, notFromGroups);

      SMDS_MeshVolume * NewVolume = 0;
      switch(nbNodes)
      {
      case 4:
	NewVolume = aHelper.AddVolume(aNds[0], aNds[1], aNds[2],
                                      aNds[3], id, theForce3d );
	break;
      case 6:
	NewVolume = aHelper.AddVolume(aNds[0], aNds[1], aNds[2],
                                      aNds[3], aNds[4], aNds[5], id, theForce3d);
	break;
      case 8:
	NewVolume = aHelper.AddVolume(aNds[0], aNds[1], aNds[2], aNds[3],
                                      aNds[4], aNds[5], aNds[6], aNds[7], id, theForce3d);
	break;
      default:
	continue;
      }
      ReplaceElemInGroups(volume, NewVolume, meshDS);
    }
  }
}

//=======================================================================
/*!
 * \brief Convert quadratic elements to linear ones and remove quadratic nodes
 * \retval int - nb of checked elements
 */
//=======================================================================

int SMESH_MeshEditor::removeQuadElem(SMESHDS_SubMesh *    theSm,
                                     SMDS_ElemIteratorPtr theItr,
                                     const int            theShapeID)
{
  int nbElem = 0;
  SMESHDS_Mesh* meshDS = GetMeshDS();
  const bool notFromGroups = false;

  while( theItr->more() )
  {
    const SMDS_MeshElement* elem = theItr->next();
    nbElem++;
    if( elem && elem->IsQuadratic())
    {
      int id = elem->GetID();
      int nbNodes = elem->NbNodes();
      vector<const SMDS_MeshNode *> aNds, mediumNodes;
      aNds.reserve( nbNodes );
      mediumNodes.reserve( nbNodes );

      for(int i = 0; i < nbNodes; i++)
      {
	const SMDS_MeshNode* n = elem->GetNode(i);

	if( elem->IsMediumNode( n ) )
          mediumNodes.push_back( n );
	else
	  aNds.push_back( n );
      }
      if( aNds.empty() ) continue;
      SMDSAbs_ElementType aType = elem->GetType();

      //remove old quadratic element
      meshDS->RemoveFreeElement( elem, theSm, notFromGroups );

      SMDS_MeshElement * NewElem = AddElement( aNds, aType, false, id );
      ReplaceElemInGroups(elem, NewElem, meshDS);
      if( theSm && NewElem )
	theSm->AddElement( NewElem );

      // remove medium nodes
      vector<const SMDS_MeshNode*>::iterator nIt = mediumNodes.begin();
      for ( ; nIt != mediumNodes.end(); ++nIt ) {
        const SMDS_MeshNode* n = *nIt;
        if ( n->NbInverseElements() == 0 ) {
          if ( n->GetPosition()->GetShapeId() != theShapeID )
            meshDS->RemoveFreeNode( n, meshDS->MeshElements
                                    ( n->GetPosition()->GetShapeId() ));
          else
            meshDS->RemoveFreeNode( n, theSm );
	}
      }
    }
  }
  return nbElem;
}

//=======================================================================
//function : ConvertFromQuadratic
//purpose  :
//=======================================================================
bool  SMESH_MeshEditor::ConvertFromQuadratic()
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
  // b. complete set of faces: find missing fices whose nodes are in set of nodes
  // c. create temporary faces representing side of volumes if correspondent
  //    face does not exist

  SMESHDS_Mesh* aMesh = GetMeshDS();
  SMDS_Mesh aTmpFacesMesh;
  set<const SMDS_MeshElement*> faceSet1, faceSet2;
  set<const SMDS_MeshElement*> volSet1,  volSet2;
  set<const SMDS_MeshNode*>    nodeSet1, nodeSet2;
  set<const SMDS_MeshElement*> * faceSetPtr[] = { &faceSet1, &faceSet2 };
  set<const SMDS_MeshElement*>  * volSetPtr[] = { &volSet1,  &volSet2  };
  set<const SMDS_MeshNode*>    * nodeSetPtr[] = { &nodeSet1, &nodeSet2 };
  TIDSortedElemSet * elemSetPtr[] = { &theSide1, &theSide2 };
  int iSide, iFace, iNode;

  for ( iSide = 0; iSide < 2; iSide++ ) {
    set<const SMDS_MeshNode*>    * nodeSet = nodeSetPtr[ iSide ];
    TIDSortedElemSet * elemSet = elemSetPtr[ iSide ];
    set<const SMDS_MeshElement*> * faceSet = faceSetPtr[ iSide ];
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
    //     faces corresponing to sides of volumes
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
    // 1b. Complete set of faces: find missing fices whose nodes are in set of nodes
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
            if ( nbNodes == 3 ) {
              aFreeFace = aMesh->FindFace( fNodes[0],fNodes[1],fNodes[2] );
            }
            else if ( nbNodes == 4 ) {
              aFreeFace = aMesh->FindFace( fNodes[0],fNodes[1],fNodes[2],fNodes[3] );
            }
            else {
              vector<const SMDS_MeshNode *> poly_nodes ( fNodes, & fNodes[nbNodes]);
              aFreeFace = aMesh->FindFace(poly_nodes);
            }
          }
          if ( !aFreeFace ) {
            // create a temporary face
            if ( nbNodes == 3 ) {
              aFreeFace = aTmpFacesMesh.AddFace( fNodes[0],fNodes[1],fNodes[2] );
            }
            else if ( nbNodes == 4 ) {
              aFreeFace = aTmpFacesMesh.AddFace( fNodes[0],fNodes[1],fNodes[2],fNodes[3] );
            }
            else {
              vector<const SMDS_MeshNode *> poly_nodes ( fNodes, & fNodes[nbNodes]);
              aFreeFace = aTmpFacesMesh.AddPolygonalFace(poly_nodes);
            }
          }
          if ( aFreeFace )
            freeFaceList.push_back( aFreeFace );

        } // loop on faces of a volume

        // choose one of several free faces
        // --------------------------------------
        if ( freeFaceList.size() > 1 ) {
          // choose a face having max nb of nodes shared by other elems of a side
          int maxNbNodes = -1/*, nbExcludedFaces = 0*/;
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
                if ( faceSet->find( e ) != faceSet->end() )
                  nbSharedNodes++;
                if ( elemSet->find( e ) != elemSet->end() )
                  nbSharedNodes++;
              }
            }
            if ( nbSharedNodes >= maxNbNodes ) {
              maxNbNodes = nbSharedNodes;
              fIt++;
            }
            else
              freeFaceList.erase( fIt++ ); // here fIt++ occures before erase
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
    SMDS_FaceIteratorPtr tmpFaceIt = aTmpFacesMesh.facesIterator();
    while ( tmpFaceIt->more() )
      aTmpFacesMesh.RemoveElement( tmpFaceIt->next() );
    MESSAGE("Diff nb of faces");
    return SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
  }

  // ============================================================
  // 2. Find nodes to merge:
  //              bind a node to remove to a node to put instead
  // ============================================================

  TNodeNodeMap nReplaceMap; // bind a node to remove to a node to put instead
  if ( theFirstNode1 != theFirstNode2 )
    nReplaceMap.insert( TNodeNodeMap::value_type( theFirstNode1, theFirstNode2 ));
  if ( theSecondNode1 != theSecondNode2 )
    nReplaceMap.insert( TNodeNodeMap::value_type( theSecondNode1, theSecondNode2 ));

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
  for ( ; linkIt[0] != linkList[0].end(); linkIt[0]++, linkIt[1]++ ) {
    NLink link[] = { *linkIt[0], *linkIt[1] };
    long linkID = aLinkID_Gen.GetLinkID( link[0].first, link[0].second );
    if ( linkIdSet.find( linkID ) == linkIdSet.end() )
      continue;

    // by links, find faces in the face sets,
    // and find indices of link nodes in the found faces;
    // in a face set, there is only one or no face sharing a link
    // ---------------------------------------------------------------

    const SMDS_MeshElement* face[] = { 0, 0 };
    //const SMDS_MeshNode* faceNodes[ 2 ][ 5 ];
    vector<const SMDS_MeshNode*> fnodes1(9);
    vector<const SMDS_MeshNode*> fnodes2(9);
    //const SMDS_MeshNode* notLinkNodes[ 2 ][ 2 ] = {{ 0, 0 },{ 0, 0 }} ;
    vector<const SMDS_MeshNode*> notLinkNodes1(6);
    vector<const SMDS_MeshNode*> notLinkNodes2(6);
    int iLinkNode[2][2];
    for ( iSide = 0; iSide < 2; iSide++ ) { // loop on 2 sides
      const SMDS_MeshNode* n1 = link[iSide].first;
      const SMDS_MeshNode* n2 = link[iSide].second;
      set<const SMDS_MeshElement*> * faceSet = faceSetPtr[ iSide ];
      set< const SMDS_MeshElement* > fMap;
      for ( int i = 0; i < 2; i++ ) { // loop on 2 nodes of a link
        const SMDS_MeshNode* n = i ? n1 : n2; // a node of a link
        SMDS_ElemIteratorPtr fIt = n->GetInverseElementIterator(SMDSAbs_Face);
        while ( fIt->more() ) { // loop on faces sharing a node
          const SMDS_MeshElement* f = fIt->next();
          if (faceSet->find( f ) != faceSet->end() && // f is in face set
              ! fMap.insert( f ).second ) // f encounters twice
          {
            if ( face[ iSide ] ) {
              MESSAGE( "2 faces per link " );
              aResult = iSide ? SEW_BAD_SIDE2_NODES : SEW_BAD_SIDE1_NODES;
              break;
            }
            face[ iSide ] = f;
            faceSet->erase( f );
            // get face nodes and find ones of a link
            iNode = 0;
            int nbl = -1;
            if(f->IsPoly()) {
              if(iSide==0) {
                fnodes1.resize(f->NbNodes()+1);
                notLinkNodes1.resize(f->NbNodes()-2);
              }
              else {
                fnodes2.resize(f->NbNodes()+1);
                notLinkNodes2.resize(f->NbNodes()-2);
              }
            }
            if(!f->IsQuadratic()) {
              SMDS_ElemIteratorPtr nIt = f->nodesIterator();
              while ( nIt->more() ) {
                const SMDS_MeshNode* n =
                  static_cast<const SMDS_MeshNode*>( nIt->next() );
                if ( n == n1 ) {
                  iLinkNode[ iSide ][ 0 ] = iNode;
                }
                else if ( n == n2 ) {
                  iLinkNode[ iSide ][ 1 ] = iNode;
                }
                //else if ( notLinkNodes[ iSide ][ 0 ] )
                //  notLinkNodes[ iSide ][ 1 ] = n;
                //else
                //  notLinkNodes[ iSide ][ 0 ] = n;
                else {
                  nbl++;
                  if(iSide==0)
                    notLinkNodes1[nbl] = n;
                    //notLinkNodes1.push_back(n);
                  else
                    notLinkNodes2[nbl] = n;
                    //notLinkNodes2.push_back(n);
                }
                //faceNodes[ iSide ][ iNode++ ] = n;
                if(iSide==0) {
                  fnodes1[iNode++] = n;
                }
                else {
                  fnodes2[iNode++] = n;
                }
              }
            }
            else { // f->IsQuadratic()
              const SMDS_QuadraticFaceOfNodes* F =
                static_cast<const SMDS_QuadraticFaceOfNodes*>(f);
              // use special nodes iterator
              SMDS_NodeIteratorPtr anIter = F->interlacedNodesIterator();
              while ( anIter->more() ) {
                const SMDS_MeshNode* n =
                  static_cast<const SMDS_MeshNode*>( anIter->next() );
                if ( n == n1 ) {
                  iLinkNode[ iSide ][ 0 ] = iNode;
                }
                else if ( n == n2 ) {
                  iLinkNode[ iSide ][ 1 ] = iNode;
                }
                else {
                  nbl++;
                  if(iSide==0) {
                    notLinkNodes1[nbl] = n;
                  }
                  else {
                    notLinkNodes2[nbl] = n;
                  }
                }
                if(iSide==0) {
                  fnodes1[iNode++] = n;
                }
                else {
                  fnodes2[iNode++] = n;
                }
              }
            }
            //faceNodes[ iSide ][ iNode ] = faceNodes[ iSide ][ 0 ];
            if(iSide==0) {
              fnodes1[iNode] = fnodes1[0];
            }
            else {
              fnodes2[iNode] = fnodes1[0];
            }
          }
        }
      }
    }

    // check similarity of elements of the sides
    if (aResult == SEW_OK && ( face[0] && !face[1] ) || ( !face[0] && face[1] )) {
      MESSAGE("Correspondent face not found on side " << ( face[0] ? 1 : 0 ));
      if ( nReplaceMap.size() == 2 ) { // faces on input nodes not found
        aResult = ( face[0] ? SEW_BAD_SIDE2_NODES : SEW_BAD_SIDE1_NODES );
      }
      else {
        aResult = SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
      }
      break; // do not return because it s necessary to remove tmp faces
    }

    // set nodes to merge
    // -------------------

    if ( face[0] && face[1] )  {
      int nbNodes = face[0]->NbNodes();
      if ( nbNodes != face[1]->NbNodes() ) {
        MESSAGE("Diff nb of face nodes");
        aResult = SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
        break; // do not return because it s necessary to remove tmp faces
      }
      bool reverse[] = { false, false }; // order of notLinkNodes of quadrangle
      if ( nbNodes == 3 ) {
        //nReplaceMap.insert( TNodeNodeMap::value_type
        //                   ( notLinkNodes[0][0], notLinkNodes[1][0] ));
        nReplaceMap.insert( TNodeNodeMap::value_type
                           ( notLinkNodes1[0], notLinkNodes2[0] ));
      }
      else {
        for ( iSide = 0; iSide < 2; iSide++ ) { // loop on 2 sides
          // analyse link orientation in faces
          int i1 = iLinkNode[ iSide ][ 0 ];
          int i2 = iLinkNode[ iSide ][ 1 ];
          reverse[ iSide ] = Abs( i1 - i2 ) == 1 ? i1 > i2 : i2 > i1;
          // if notLinkNodes are the first and the last ones, then
          // their order does not correspond to the link orientation
          if (( i1 == 1 && i2 == 2 ) ||
              ( i1 == 2 && i2 == 1 ))
            reverse[ iSide ] = !reverse[ iSide ];
        }
        if ( reverse[0] == reverse[1] ) {
          //nReplaceMap.insert( TNodeNodeMap::value_type
          //                   ( notLinkNodes[0][0], notLinkNodes[1][0] ));
          //nReplaceMap.insert( TNodeNodeMap::value_type
          //                   ( notLinkNodes[0][1], notLinkNodes[1][1] ));
          for(int nn=0; nn<nbNodes-2; nn++) {
            nReplaceMap.insert( TNodeNodeMap::value_type
                             ( notLinkNodes1[nn], notLinkNodes2[nn] ));
          }
        }
        else {
          //nReplaceMap.insert( TNodeNodeMap::value_type
          //                   ( notLinkNodes[0][0], notLinkNodes[1][1] ));
          //nReplaceMap.insert( TNodeNodeMap::value_type
          //                   ( notLinkNodes[0][1], notLinkNodes[1][0] ));
          for(int nn=0; nn<nbNodes-2; nn++) {
            nReplaceMap.insert( TNodeNodeMap::value_type
                             ( notLinkNodes1[nn], notLinkNodes2[nbNodes-3-nn] ));
          }
        }
      }

      // add other links of the faces to linkList
      // -----------------------------------------

      //const SMDS_MeshNode** nodes = faceNodes[ 0 ];
      for ( iNode = 0; iNode < nbNodes; iNode++ )  {
        //linkID = aLinkID_Gen.GetLinkID( nodes[iNode], nodes[iNode+1] );
        linkID = aLinkID_Gen.GetLinkID( fnodes1[iNode], fnodes1[iNode+1] );
        pair< set<long>::iterator, bool > iter_isnew = linkIdSet.insert( linkID );
        if ( !iter_isnew.second ) { // already in a set: no need to process
          linkIdSet.erase( iter_isnew.first );
        }
        else // new in set == encountered for the first time: add
        {
          //const SMDS_MeshNode* n1 = nodes[ iNode ];
          //const SMDS_MeshNode* n2 = nodes[ iNode + 1];
          const SMDS_MeshNode* n1 = fnodes1[ iNode ];
          const SMDS_MeshNode* n2 = fnodes1[ iNode + 1];
          linkList[0].push_back ( NLink( n1, n2 ));
          linkList[1].push_back ( NLink( nReplaceMap[n1], nReplaceMap[n2] ));
        }
      }
    } // 2 faces found
  } // loop on link lists

  if ( aResult == SEW_OK &&
      ( linkIt[0] != linkList[0].end() ||
       !faceSetPtr[0]->empty() || !faceSetPtr[1]->empty() )) {
    MESSAGE( (linkIt[0] != linkList[0].end()) <<" "<< (faceSetPtr[0]->empty()) <<
            " " << (faceSetPtr[1]->empty()));
    aResult = SEW_TOPO_DIFF_SETS_OF_ELEMENTS;
  }

  // ====================================================================
  // 3. Replace nodes in elements of the side 1 and remove replaced nodes
  // ====================================================================

  // delete temporary faces: they are in reverseElements of actual nodes
  SMDS_FaceIteratorPtr tmpFaceIt = aTmpFacesMesh.facesIterator();
  while ( tmpFaceIt->more() )
    aTmpFacesMesh.RemoveElement( tmpFaceIt->next() );

  if ( aResult != SEW_OK)
    return aResult;

  list< int > nodeIDsToRemove/*, elemIDsToRemove*/;
  // loop on nodes replacement map
  TNodeNodeMap::iterator nReplaceMapIt = nReplaceMap.begin(), nnIt;
  for ( ; nReplaceMapIt != nReplaceMap.end(); nReplaceMapIt++ )
    if ( (*nReplaceMapIt).first != (*nReplaceMapIt).second ) {
      const SMDS_MeshNode* nToRemove = (*nReplaceMapIt).first;
      nodeIDsToRemove.push_back( nToRemove->GetID() );
      // loop on elements sharing nToRemove
      SMDS_ElemIteratorPtr invElemIt = nToRemove->GetInverseElementIterator();
      while ( invElemIt->more() ) {
        const SMDS_MeshElement* e = invElemIt->next();
        // get a new suite of nodes: make replacement
        int nbReplaced = 0, i = 0, nbNodes = e->NbNodes();
        vector< const SMDS_MeshNode*> nodes( nbNodes );
        SMDS_ElemIteratorPtr nIt = e->nodesIterator();
        while ( nIt->more() ) {
          const SMDS_MeshNode* n =
            static_cast<const SMDS_MeshNode*>( nIt->next() );
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
          aMesh->ChangeElementNodes( e, & nodes[0], nbNodes );
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
    * \retval bool  - is a success or not
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

/*!
  \brief Creates a hole in a mesh by doubling the nodes of some particular elements
  \param theNodes - identifiers of nodes to be doubled
  \param theModifiedElems - identifiers of elements to be updated by the new (doubled) 
         nodes. If list of element identifiers is empty then nodes are doubled but 
         they not assigned to elements
  \return TRUE if operation has been completed successfully, FALSE otherwise
*/
bool SMESH_MeshEditor::DoubleNodes( const std::list< int >& theListOfNodes, 
                                    const std::list< int >& theListOfModifiedElems )
{
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
      aMeshDS->ChangeElementNodes( anElem, &aNodeArr[ 0 ], anElem->NbNodes() );
  }

  return true;
}
