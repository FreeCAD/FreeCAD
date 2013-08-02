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
//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESH_Mesh.cxx
//  Author : Yves FRICAUD, OCC
//  Module : SMESH
//  $Header: 
//
#include "SMESHDS_Mesh.hxx"

#include "SMESHDS_Group.hxx"
#include "SMDS_VertexPosition.hxx"
#include "SMDS_EdgePosition.hxx"
#include "SMDS_FacePosition.hxx"
#include "SMDS_SpacePosition.hxx"
#include "SMESHDS_GroupOnGeom.hxx"

#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopoDS_Iterator.hxx>

#include "utilities.h"

using namespace std;

/*Standard_Boolean IsEqual( const TopoDS_Shape& S1, const TopoDS_Shape& S2 ) 
  {
    return S1.IsSame( S2 );
  }*/

//=======================================================================
//function : Create
//purpose  : 
//=======================================================================
SMESHDS_Mesh::SMESHDS_Mesh(int theMeshID, bool theIsEmbeddedMode):
  myMeshID(theMeshID),
  myIsEmbeddedMode(theIsEmbeddedMode),
  myCurSubID(-1)
{
  myScript = new SMESHDS_Script(theIsEmbeddedMode);
  myCurSubMesh = 0;
}

//=======================================================================
bool SMESHDS_Mesh::IsEmbeddedMode()
{
  return myIsEmbeddedMode;
}

//=======================================================================
//function : ShapeToMesh
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::ShapeToMesh(const TopoDS_Shape & S)
{
  if ( !myShape.IsNull() && S.IsNull() )
  {
    // removal of a shape to mesh, delete ...
    // - hypotheses
    myShapeToHypothesis.Clear();
    // - shape indices in SMDS_Position of nodes
    map<int,SMESHDS_SubMesh*>::iterator i_sub = myShapeIndexToSubMesh.begin();
    for ( ; i_sub != myShapeIndexToSubMesh.end(); i_sub++ ) {
      if ( !i_sub->second->IsComplexSubmesh() ) {
        SMDS_NodeIteratorPtr nIt = i_sub->second->GetNodes();
        while ( nIt->more() )
          nIt->next()->GetPosition()->SetShapeId( 0 );
      }
    }
    // - sub-meshes
    TShapeIndexToSubMesh::iterator i_sm = myShapeIndexToSubMesh.begin();
    for ( ; i_sm != myShapeIndexToSubMesh.end(); ++i_sm )
      delete i_sm->second;
    myShapeIndexToSubMesh.clear();
    myIndexToShape.Clear();
    // - groups on geometry
    set<SMESHDS_GroupBase*>::iterator gr = myGroups.begin();
    while ( gr != myGroups.end() ) {
      if ( dynamic_cast<SMESHDS_GroupOnGeom*>( *gr ))
        myGroups.erase( gr++ );
      else
        gr++;
    }
  }
  else {
    myShape = S;
    if ( !S.IsNull() )
      TopExp::MapShapes(myShape, myIndexToShape);
  }
}

//=======================================================================
//function : AddHypothesis
//purpose  : 
//=======================================================================

bool SMESHDS_Mesh::AddHypothesis(const TopoDS_Shape & SS,
                                 const SMESHDS_Hypothesis * H)
{
  if (!myShapeToHypothesis.IsBound(SS.Oriented(TopAbs_FORWARD))) {
    list<const SMESHDS_Hypothesis *> aList;
    myShapeToHypothesis.Bind(SS.Oriented(TopAbs_FORWARD), aList);
  }
  list<const SMESHDS_Hypothesis *>& alist =
    myShapeToHypothesis(SS.Oriented(TopAbs_FORWARD)); // ignore orientation of SS

  //Check if the Hypothesis is still present
  list<const SMESHDS_Hypothesis*>::iterator ith = find(alist.begin(),alist.end(), H );

  if (alist.end() != ith) return false;

  alist.push_back(H);
  return true;
}

//=======================================================================
//function : RemoveHypothesis
//purpose  : 
//=======================================================================

bool SMESHDS_Mesh::RemoveHypothesis(const TopoDS_Shape &       S,
                                    const SMESHDS_Hypothesis * H)
{
  if( myShapeToHypothesis.IsBound( S.Oriented(TopAbs_FORWARD) ) )
  {
    list<const SMESHDS_Hypothesis *>& alist=myShapeToHypothesis.ChangeFind( S.Oriented(TopAbs_FORWARD) );
    list<const SMESHDS_Hypothesis*>::iterator ith=find(alist.begin(),alist.end(), H );
    if (ith != alist.end())
    {
      alist.erase(ith);
      return true;
    }
  }
  return false;
}

//=======================================================================
//function : AddNode
//purpose  : 
//=======================================================================
SMDS_MeshNode* SMESHDS_Mesh::AddNode(double x, double y, double z){
  SMDS_MeshNode* node = SMDS_Mesh::AddNode(x, y, z);
  if(node!=NULL) myScript->AddNode(node->GetID(), x, y, z);
  return node;
}

SMDS_MeshNode* SMESHDS_Mesh::AddNodeWithID(double x, double y, double z, int ID){
  SMDS_MeshNode* node = SMDS_Mesh::AddNodeWithID(x,y,z,ID);
  if(node!=NULL) myScript->AddNode(node->GetID(), x, y, z);
  return node;
}

//=======================================================================
//function : MoveNode
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::MoveNode(const SMDS_MeshNode *n, double x, double y, double z)
{
  SMDS_MeshNode * node=const_cast<SMDS_MeshNode*>(n);
  node->setXYZ(x,y,z);
  myScript->MoveNode(n->GetID(), x, y, z);
}

//=======================================================================
//function : ChangeElementNodes
//purpose  : 
//=======================================================================

bool SMESHDS_Mesh::ChangeElementNodes(const SMDS_MeshElement * elem,
                                      const SMDS_MeshNode    * nodes[],
                                      const int                nbnodes)
{
  if ( ! SMDS_Mesh::ChangeElementNodes( elem, nodes, nbnodes ))
    return false;

  vector<int> IDs( nbnodes );
  for ( int i = 0; i < nbnodes; i++ )
    IDs [ i ] = nodes[ i ]->GetID();
  myScript->ChangeElementNodes( elem->GetID(), &IDs[0], nbnodes);

  return true;
}

//=======================================================================
//function : ChangePolygonNodes
//purpose  : 
//=======================================================================
bool SMESHDS_Mesh::ChangePolygonNodes
                   (const SMDS_MeshElement *     elem,
                    vector<const SMDS_MeshNode*> nodes)
{
  ASSERT(nodes.size() > 3);

  return ChangeElementNodes(elem, &nodes[0], nodes.size());
}

//=======================================================================
//function : ChangePolyhedronNodes
//purpose  : 
//=======================================================================
bool SMESHDS_Mesh::ChangePolyhedronNodes
                   (const SMDS_MeshElement * elem,
                    std::vector<const SMDS_MeshNode*> nodes,
                    std::vector<int>                  quantities)
{
  ASSERT(nodes.size() > 3);

  if (!SMDS_Mesh::ChangePolyhedronNodes(elem, nodes, quantities))
    return false;

  int i, len = nodes.size();
  std::vector<int> nodes_ids (len);
  for (i = 0; i < len; i++) {
    nodes_ids[i] = nodes[i]->GetID();
  }
  myScript->ChangePolyhedronNodes(elem->GetID(), nodes_ids, quantities);

  return true;
}

//=======================================================================
//function : Renumber
//purpose  : 
//=======================================================================

void SMESHDS_Mesh::Renumber (const bool isNodes, const int startID, const int deltaID)
{
  SMDS_Mesh::Renumber( isNodes, startID, deltaID );
  myScript->Renumber( isNodes, startID, deltaID );
}

//=======================================================================
//function :AddEdgeWithID
//purpose  : 
//=======================================================================
SMDS_MeshEdge* SMESHDS_Mesh::AddEdgeWithID(int n1, int n2, int ID)
{
  SMDS_MeshEdge* anElem = SMDS_Mesh::AddEdgeWithID(n1,n2,ID);
  if(anElem) myScript->AddEdge(ID,n1,n2);
  return anElem;
}

SMDS_MeshEdge* SMESHDS_Mesh::AddEdgeWithID(const SMDS_MeshNode * n1,
					   const SMDS_MeshNode * n2, 
					   int ID)
{
  return AddEdgeWithID(n1->GetID(),
		       n2->GetID(),
		       ID);
}

SMDS_MeshEdge* SMESHDS_Mesh::AddEdge(const SMDS_MeshNode * n1,
				     const SMDS_MeshNode * n2)
{
  SMDS_MeshEdge* anElem = SMDS_Mesh::AddEdge(n1,n2);
  if(anElem) myScript->AddEdge(anElem->GetID(), 
			       n1->GetID(), 
			       n2->GetID());
  return anElem;
}

//=======================================================================
//function :AddFace
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddFaceWithID(int n1, int n2, int n3, int ID)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddFaceWithID(n1, n2, n3, ID);
  if(anElem) myScript->AddFace(ID,n1,n2,n3);
  return anElem;
}

SMDS_MeshFace* SMESHDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
					   const SMDS_MeshNode * n2,
					   const SMDS_MeshNode * n3, 
					   int ID)
{
  return AddFaceWithID(n1->GetID(),
		       n2->GetID(),
		       n3->GetID(),
		       ID);
}

SMDS_MeshFace* SMESHDS_Mesh::AddFace( const SMDS_MeshNode * n1,
				      const SMDS_MeshNode * n2,
				      const SMDS_MeshNode * n3)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddFace(n1, n2, n3);
  if(anElem) myScript->AddFace(anElem->GetID(), 
			       n1->GetID(), 
			       n2->GetID(),
			       n3->GetID());
  return anElem;
}

//=======================================================================
//function :AddFace
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddFaceWithID(int n1, int n2, int n3, int n4, int ID)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddFaceWithID(n1, n2, n3, n4, ID);
  if(anElem) myScript->AddFace(ID, n1, n2, n3, n4);
  return anElem;
}

SMDS_MeshFace* SMESHDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
					   const SMDS_MeshNode * n2,
					   const SMDS_MeshNode * n3,
					   const SMDS_MeshNode * n4, 
					   int ID)
{
  return AddFaceWithID(n1->GetID(),
		       n2->GetID(),
		       n3->GetID(),
		       n4->GetID(),
		       ID);
}

SMDS_MeshFace* SMESHDS_Mesh::AddFace(const SMDS_MeshNode * n1,
				     const SMDS_MeshNode * n2,
				     const SMDS_MeshNode * n3,
				     const SMDS_MeshNode * n4)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddFace(n1, n2, n3, n4);
  if(anElem) myScript->AddFace(anElem->GetID(), 
			       n1->GetID(), 
			       n2->GetID(), 
			       n3->GetID(),
			       n4->GetID());
  return anElem;
}

//=======================================================================
//function :AddVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4, int ID)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, ID);
  if(anElem) myScript->AddVolume(ID, n1, n2, n3, n4);
  return anElem;
}

SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
					       const SMDS_MeshNode * n2,
					       const SMDS_MeshNode * n3,
					       const SMDS_MeshNode * n4, 
					       int ID)
{
  return AddVolumeWithID(n1->GetID(), 
			 n2->GetID(), 
			 n3->GetID(),
			 n4->GetID(),
			 ID);
}

SMDS_MeshVolume* SMESHDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
					 const SMDS_MeshNode * n2,
					 const SMDS_MeshNode * n3,
					 const SMDS_MeshNode * n4)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolume(n1, n2, n3, n4);
  if(anElem) myScript->AddVolume(anElem->GetID(), 
				 n1->GetID(), 
				 n2->GetID(), 
				 n3->GetID(),
				 n4->GetID());
  return anElem;
}

//=======================================================================
//function :AddVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4, int n5, int ID)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, ID);
  if(anElem) myScript->AddVolume(ID, n1, n2, n3, n4, n5);
  return anElem;
}

SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
					       const SMDS_MeshNode * n2,
					       const SMDS_MeshNode * n3,
					       const SMDS_MeshNode * n4,
					       const SMDS_MeshNode * n5, 
					       int ID)
{
  return AddVolumeWithID(n1->GetID(), 
			 n2->GetID(), 
			 n3->GetID(),
			 n4->GetID(), 
			 n5->GetID(),
			 ID);
}

SMDS_MeshVolume* SMESHDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
					 const SMDS_MeshNode * n2,
					 const SMDS_MeshNode * n3,
					 const SMDS_MeshNode * n4,
					 const SMDS_MeshNode * n5)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolume(n1, n2, n3, n4, n5);
  if(anElem) myScript->AddVolume(anElem->GetID(), 
				 n1->GetID(), 
				 n2->GetID(), 
				 n3->GetID(),
				 n4->GetID(), 
				 n5->GetID());
  return anElem;
}

//=======================================================================
//function :AddVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4, int n5, int n6, int ID)
{
  SMDS_MeshVolume *anElem= SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n6, ID);
  if(anElem) myScript->AddVolume(ID, n1, n2, n3, n4, n5, n6);
  return anElem;
}

SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
					       const SMDS_MeshNode * n2,
					       const SMDS_MeshNode * n3,
					       const SMDS_MeshNode * n4,
					       const SMDS_MeshNode * n5,
					       const SMDS_MeshNode * n6, 
					       int ID)
{
  return AddVolumeWithID(n1->GetID(), 
			 n2->GetID(), 
			 n3->GetID(),
			 n4->GetID(), 
			 n5->GetID(), 
			 n6->GetID(),
			 ID);
}

SMDS_MeshVolume* SMESHDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
					 const SMDS_MeshNode * n2,
					 const SMDS_MeshNode * n3,
					 const SMDS_MeshNode * n4,
					 const SMDS_MeshNode * n5,
					 const SMDS_MeshNode * n6)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolume(n1, n2, n3, n4, n5, n6);
  if(anElem) myScript->AddVolume(anElem->GetID(), 
				 n1->GetID(), 
				 n2->GetID(), 
				 n3->GetID(),
				 n4->GetID(), 
				 n5->GetID(), 
				 n6->GetID());
  return anElem;
}

//=======================================================================
//function :AddVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int ID)
{
  SMDS_MeshVolume *anElem= SMDS_Mesh::AddVolumeWithID(n1, n2, n3, n4, n5, n6, n7, n8, ID);
  if(anElem) myScript->AddVolume(ID, n1, n2, n3, n4, n5, n6, n7, n8);
  return anElem;
}

SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
					       const SMDS_MeshNode * n2,
					       const SMDS_MeshNode * n3,
					       const SMDS_MeshNode * n4,
					       const SMDS_MeshNode * n5,
					       const SMDS_MeshNode * n6,
					       const SMDS_MeshNode * n7,
					       const SMDS_MeshNode * n8, 
					       int ID)
{
  return AddVolumeWithID(n1->GetID(), 
			 n2->GetID(), 
			 n3->GetID(),
			 n4->GetID(), 
			 n5->GetID(), 
			 n6->GetID(), 
			 n7->GetID(), 
			 n8->GetID(),
			 ID);
}

SMDS_MeshVolume* SMESHDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
					 const SMDS_MeshNode * n2,
					 const SMDS_MeshNode * n3,
					 const SMDS_MeshNode * n4,
					 const SMDS_MeshNode * n5,
					 const SMDS_MeshNode * n6,
					 const SMDS_MeshNode * n7,
					 const SMDS_MeshNode * n8)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolume(n1, n2, n3, n4, n5, n6, n7, n8);
  if(anElem) myScript->AddVolume(anElem->GetID(), 
				 n1->GetID(), 
				 n2->GetID(), 
				 n3->GetID(),
				 n4->GetID(), 
				 n5->GetID(), 
				 n6->GetID(), 
				 n7->GetID(), 
				 n8->GetID());
  return anElem;
}

//=======================================================================
//function : AddPolygonalFace
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddPolygonalFaceWithID (std::vector<int> nodes_ids,
                                                     const int        ID)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddPolygonalFaceWithID(nodes_ids, ID);
  if (anElem) {
    myScript->AddPolygonalFace(ID, nodes_ids);
  }
  return anElem;
}

SMDS_MeshFace* SMESHDS_Mesh::AddPolygonalFaceWithID
                             (std::vector<const SMDS_MeshNode*> nodes,
                              const int                         ID)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddPolygonalFaceWithID(nodes, ID);
  if (anElem) {
    int i, len = nodes.size();
    std::vector<int> nodes_ids (len);
    for (i = 0; i < len; i++) {
      nodes_ids[i] = nodes[i]->GetID();
    }
    myScript->AddPolygonalFace(ID, nodes_ids);
  }
  return anElem;
}

SMDS_MeshFace* SMESHDS_Mesh::AddPolygonalFace
                             (std::vector<const SMDS_MeshNode*> nodes)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddPolygonalFace(nodes);
  if (anElem) {
    int i, len = nodes.size();
    std::vector<int> nodes_ids (len);
    for (i = 0; i < len; i++) {
      nodes_ids[i] = nodes[i]->GetID();
    }
    myScript->AddPolygonalFace(anElem->GetID(), nodes_ids);
  }
  return anElem;
}

//=======================================================================
//function : AddPolyhedralVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddPolyhedralVolumeWithID (std::vector<int> nodes_ids,
                                                          std::vector<int> quantities,
                                                          const int        ID)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddPolyhedralVolumeWithID(nodes_ids, quantities, ID);
  if (anElem) {
    myScript->AddPolyhedralVolume(ID, nodes_ids, quantities);
  }
  return anElem;
}

SMDS_MeshVolume* SMESHDS_Mesh::AddPolyhedralVolumeWithID
                               (std::vector<const SMDS_MeshNode*> nodes,
                                std::vector<int>                  quantities,
                                const int                         ID)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddPolyhedralVolumeWithID(nodes, quantities, ID);
  if (anElem) {
    int i, len = nodes.size();
    std::vector<int> nodes_ids (len);
    for (i = 0; i < len; i++) {
      nodes_ids[i] = nodes[i]->GetID();
    }
    myScript->AddPolyhedralVolume(ID, nodes_ids, quantities);
  }
  return anElem;
}

SMDS_MeshVolume* SMESHDS_Mesh::AddPolyhedralVolume
                               (std::vector<const SMDS_MeshNode*> nodes,
                                std::vector<int>                  quantities)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddPolyhedralVolume(nodes, quantities);
  if (anElem) {
    int i, len = nodes.size();
    std::vector<int> nodes_ids (len);
    for (i = 0; i < len; i++) {
      nodes_ids[i] = nodes[i]->GetID();
    }
    myScript->AddPolyhedralVolume(anElem->GetID(), nodes_ids, quantities);
  }
  return anElem;
}

//=======================================================================
//function : removeFromContainers
//purpose  : 
//=======================================================================

static void removeFromContainers (map<int,SMESHDS_SubMesh*>&     theSubMeshes,
                                  set<SMESHDS_GroupBase*>&       theGroups,
                                  list<const SMDS_MeshElement*>& theElems,
                                  const bool                     isNode)
{
  if ( theElems.empty() )
    return;

  // Rm from group
  // Element can belong to several groups
  if ( !theGroups.empty() )
  {
    set<SMESHDS_GroupBase*>::iterator GrIt = theGroups.begin();
    for ( ; GrIt != theGroups.end(); GrIt++ )
    {
      SMESHDS_Group* group = dynamic_cast<SMESHDS_Group*>( *GrIt );
      if ( !group || group->IsEmpty() ) continue;

      list<const SMDS_MeshElement *>::iterator elIt = theElems.begin();
      for ( ; elIt != theElems.end(); elIt++ )
      {
        group->SMDSGroup().Remove( *elIt );
        if ( group->IsEmpty() ) break;
      }
    }
  }

  const bool deleted=true;

  // Rm from sub-meshes
  // Element should belong to only one sub-mesh
  map<int,SMESHDS_SubMesh*>::iterator SubIt = theSubMeshes.begin();
  for ( ; SubIt != theSubMeshes.end(); SubIt++ )
  {
    int size = isNode ? (*SubIt).second->NbNodes() : (*SubIt).second->NbElements();
    if ( size == 0 ) continue;

    list<const SMDS_MeshElement *>::iterator elIt = theElems.begin();
    while ( elIt != theElems.end() )
    {
      bool removed = false;
      if ( isNode )
        removed = (*SubIt).second->RemoveNode( static_cast<const SMDS_MeshNode*> (*elIt), deleted );
      else
        removed = (*SubIt).second->RemoveElement( *elIt, deleted );

      if (removed)
      {
        elIt = theElems.erase( elIt );
        if ( theElems.empty() )
          return; // all elements are found and removed
      }
      else
      {
        elIt++ ;
      }
    }
  }
}
  
//=======================================================================
//function : RemoveNode
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::RemoveNode(const SMDS_MeshNode * n)
{
  if ( n->NbInverseElements() == 0 && !(hasConstructionEdges() || hasConstructionFaces()))
  {
    SMESHDS_SubMesh* subMesh=0;
    map<int,SMESHDS_SubMesh*>::iterator SubIt =
      myShapeIndexToSubMesh.find( n->GetPosition()->GetShapeId() );
    if ( SubIt != myShapeIndexToSubMesh.end() )
      subMesh = SubIt->second;
    else
      SubIt = myShapeIndexToSubMesh.begin();
    for ( ; !subMesh && SubIt != myShapeIndexToSubMesh.end(); SubIt++ )
      if (!SubIt->second->IsComplexSubmesh() && SubIt->second->Contains( n ))
        subMesh = SubIt->second;

    RemoveFreeNode( n, subMesh, true);
    return;
  }
    
  myScript->RemoveNode(n->GetID());
  
  list<const SMDS_MeshElement *> removedElems;
  list<const SMDS_MeshElement *> removedNodes;

  SMDS_Mesh::RemoveElement( n, removedElems, removedNodes, true );

  removeFromContainers( myShapeIndexToSubMesh, myGroups, removedElems, false );
  removeFromContainers( myShapeIndexToSubMesh, myGroups, removedNodes, true );
}

//=======================================================================
//function : RemoveFreeNode
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::RemoveFreeNode(const SMDS_MeshNode * n,
                                  SMESHDS_SubMesh *     subMesh,
                                  bool                  fromGroups)
{
  myScript->RemoveNode(n->GetID());

  // Rm from group
  // Node can belong to several groups
  if (fromGroups && !myGroups.empty()) {
    set<SMESHDS_GroupBase*>::iterator GrIt = myGroups.begin();
    for (; GrIt != myGroups.end(); GrIt++) {
      SMESHDS_Group* group = dynamic_cast<SMESHDS_Group*>(*GrIt);
      if (!group || group->IsEmpty()) continue;
      group->SMDSGroup().Remove(n);
    }
  }

  // Rm from sub-mesh
  // Node should belong to only one sub-mesh
  if( subMesh )
    subMesh->RemoveNode(n,/*deleted=*/false);

  SMDS_Mesh::RemoveFreeElement(n);
}

//=======================================================================
//function : RemoveElement
//purpose  : 
//========================================================================
void SMESHDS_Mesh::RemoveElement(const SMDS_MeshElement * elt)
{
  if (elt->GetType() == SMDSAbs_Node)
  {
    RemoveNode( static_cast<const SMDS_MeshNode*>( elt ));
    return;
  }
  if (!hasConstructionEdges() && !hasConstructionFaces())
  {
    SMESHDS_SubMesh* subMesh=0;
    map<int,SMESHDS_SubMesh*>::iterator SubIt = myShapeIndexToSubMesh.begin();
    for ( ; !subMesh && SubIt != myShapeIndexToSubMesh.end(); SubIt++ )
      if (!SubIt->second->IsComplexSubmesh() && SubIt->second->Contains( elt ))
        subMesh = SubIt->second;

    RemoveFreeElement( elt, subMesh, true);
    return;
  }
 
  myScript->RemoveElement(elt->GetID());

  list<const SMDS_MeshElement *> removedElems;
  list<const SMDS_MeshElement *> removedNodes;

  SMDS_Mesh::RemoveElement(elt, removedElems, removedNodes, false);
  
  removeFromContainers( myShapeIndexToSubMesh, myGroups, removedElems, false );
}

//=======================================================================
//function : RemoveFreeElement
//purpose  : 
//========================================================================
void SMESHDS_Mesh::RemoveFreeElement(const SMDS_MeshElement * elt,
                                     SMESHDS_SubMesh *        subMesh,
                                     bool                     fromGroups)
{
  if (elt->GetType() == SMDSAbs_Node) {
    RemoveFreeNode( static_cast<const SMDS_MeshNode*>(elt), subMesh);
    return;
  }

  if (hasConstructionEdges() || hasConstructionFaces())
    // this methods is only for meshes without descendants
    return;

  myScript->RemoveElement(elt->GetID());

  // Rm from group
  // Node can belong to several groups
  if ( fromGroups && !myGroups.empty() ) {
    set<SMESHDS_GroupBase*>::iterator GrIt = myGroups.begin();
    for (; GrIt != myGroups.end(); GrIt++) {
      SMESHDS_Group* group = dynamic_cast<SMESHDS_Group*>(*GrIt);
      if (group && !group->IsEmpty())
        group->SMDSGroup().Remove(elt);
    }
  }

  // Rm from sub-mesh
  // Element should belong to only one sub-mesh
  if( subMesh )
    subMesh->RemoveElement(elt, /*deleted=*/false);

  SMDS_Mesh::RemoveFreeElement(elt);
}

//================================================================================
/*!
 * \brief Remove all data from the mesh
 */
//================================================================================

void SMESHDS_Mesh::ClearMesh()
{
  myScript->ClearMesh();
  SMDS_Mesh::Clear();

  // clear submeshes
  map<int,SMESHDS_SubMesh*>::iterator sub, subEnd = myShapeIndexToSubMesh.end();
  for ( sub = myShapeIndexToSubMesh.begin(); sub != subEnd; ++sub )
    sub->second->Clear();

  // clear groups
  TGroups::iterator group, groupEnd = myGroups.end();
  for ( group = myGroups.begin(); group != groupEnd; ++group ) {
    if ( SMESHDS_Group* g = dynamic_cast<SMESHDS_Group*>(*group)) {
      SMDSAbs_ElementType groupType = g->GetType();
      g->Clear();
      g->SetType( groupType );
    }
  }
}

//================================================================================
/*!
 * \brief return submesh by shape
  * \param shape - the subshape
  * \retval SMESHDS_SubMesh* - the found submesh
  *
 * search of submeshes is optimized
 */
//================================================================================

SMESHDS_SubMesh* SMESHDS_Mesh::getSubmesh( const TopoDS_Shape & shape )
{
  if ( shape.IsNull() )
    return 0;

  if ( !myCurSubShape.IsNull() && shape.IsSame( myCurSubShape ))
    return myCurSubMesh;

  getSubmesh( ShapeToIndex( shape ));
  myCurSubShape = shape;
  return myCurSubMesh;
}

//================================================================================
/*!
 * \brief return submesh by subshape index
  * \param Index - the subshape index
  * \retval SMESHDS_SubMesh* - the found submesh
 * search of submeshes is optimized
 */
//================================================================================

SMESHDS_SubMesh* SMESHDS_Mesh::getSubmesh( const int Index )
{
  //Update or build submesh
  if ( Index != myCurSubID ) {
    map<int,SMESHDS_SubMesh*>::iterator it = myShapeIndexToSubMesh.find( Index );
    if ( it == myShapeIndexToSubMesh.end() )
      it = myShapeIndexToSubMesh.insert( make_pair(Index, new SMESHDS_SubMesh() )).first;
    myCurSubMesh = it->second;
    myCurSubID = Index;
    myCurSubShape.Nullify(); // myCurSubShape no more corresponds to submesh
  }
  return myCurSubMesh;
}

//================================================================================
/*!
 * \brief Add element or node to submesh
  * \param elem - element to add
  * \param subMesh - submesh to be filled in
 */
//================================================================================

bool SMESHDS_Mesh::add(const SMDS_MeshElement* elem, SMESHDS_SubMesh* subMesh )
{
  if ( elem && subMesh ) {
    if ( elem->GetType() == SMDSAbs_Node )
      subMesh->AddNode( static_cast<const SMDS_MeshNode* >( elem ));
    else
      subMesh->AddElement( elem );
    return true;
  }
  return false;
}

namespace {

  //================================================================================
  /*!
   * \brief Creates a node position in volume
   */
  //================================================================================

  inline SMDS_PositionPtr volumePosition(int volId)
  {
    SMDS_SpacePosition* pos = new SMDS_SpacePosition();
    pos->SetShapeId( volId );
    return SMDS_PositionPtr(pos);
  }
}

//=======================================================================
//function : SetNodeOnVolume
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeInVolume(SMDS_MeshNode *      aNode,
                                   const TopoDS_Shell & S)
{
  if ( add( aNode, getSubmesh(S) ))
    aNode->SetPosition ( volumePosition( myCurSubID ));
}
//=======================================================================
//function : SetNodeOnVolume
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeInVolume(SMDS_MeshNode *      aNode,
                                   const TopoDS_Solid & S)
{
  if ( add( aNode, getSubmesh(S) ))
    aNode->SetPosition ( volumePosition( myCurSubID ));
}

//=======================================================================
//function : SetNodeOnFace
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeOnFace(SMDS_MeshNode *     aNode,
                                 const TopoDS_Face & S,
                                 double              u,
                                 double              v)
{
  if ( add( aNode, getSubmesh(S) ))
    aNode->SetPosition(SMDS_PositionPtr(new SMDS_FacePosition(myCurSubID, u, v)));
}

//=======================================================================
//function : SetNodeOnEdge
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeOnEdge(SMDS_MeshNode *     aNode,
                                 const TopoDS_Edge & S,
                                 double              u)
{
  if ( add( aNode, getSubmesh(S) ))
    aNode->SetPosition(SMDS_PositionPtr(new SMDS_EdgePosition(myCurSubID, u)));
}

//=======================================================================
//function : SetNodeOnVertex
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeOnVertex(SMDS_MeshNode *       aNode,
                                   const TopoDS_Vertex & S)
{
  if ( add( aNode, getSubmesh(S) ))
    aNode->SetPosition(SMDS_PositionPtr(new SMDS_VertexPosition(myCurSubID)));
}

//=======================================================================
//function : UnSetNodeOnShape
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::UnSetNodeOnShape(const SMDS_MeshNode* aNode)
{
  if ( (aNode!=NULL) && (aNode->GetPosition()!=NULL) ) {
    map<int,SMESHDS_SubMesh*>::iterator it =
      myShapeIndexToSubMesh.find( aNode->GetPosition()->GetShapeId() );
    if ( it != myShapeIndexToSubMesh.end() )
      it->second->RemoveNode( aNode, /*deleted=*/false );
  }
}

//=======================================================================
//function : SetMeshElementOnShape
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetMeshElementOnShape(const SMDS_MeshElement * anElement,
                                         const TopoDS_Shape &     S)
{
  add( anElement, getSubmesh(S) );
}

//=======================================================================
//function : UnSetMeshElementOnShape
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::UnSetMeshElementOnShape(const SMDS_MeshElement * elem,
                                           const TopoDS_Shape &     S)
{
  int Index = myIndexToShape.FindIndex(S);

  map<int,SMESHDS_SubMesh*>::iterator it = myShapeIndexToSubMesh.find( Index );
  if ( it != myShapeIndexToSubMesh.end() )
    if ( elem->GetType() == SMDSAbs_Node )
      it->second->RemoveNode( static_cast<const SMDS_MeshNode* >( elem ), /*deleted=*/false );
    else
      it->second->RemoveElement( elem, /*deleted=*/false );
}

//=======================================================================
//function : ShapeToMesh
//purpose  : 
//=======================================================================
TopoDS_Shape SMESHDS_Mesh::ShapeToMesh() const
{
	return myShape;
}

//=======================================================================
//function : IsGroupOfSubShapes
//purpose  : return true if at least one subshape of theShape is a subshape
//           of myShape or theShape == myShape
//=======================================================================

bool SMESHDS_Mesh::IsGroupOfSubShapes (const TopoDS_Shape& theShape) const
{
  if ( myShape.IsSame( theShape ))
    return true;

  for ( TopoDS_Iterator it( theShape ); it.More(); it.Next() ) {
    if (myIndexToShape.Contains( it.Value() ) ||
        IsGroupOfSubShapes( it.Value() ))
      return true;
  }
  
  return false;
}

///////////////////////////////////////////////////////////////////////////////
/// Return the sub mesh linked to the a given TopoDS_Shape or NULL if the given
/// TopoDS_Shape is unknown
///////////////////////////////////////////////////////////////////////////////
SMESHDS_SubMesh * SMESHDS_Mesh::MeshElements(const TopoDS_Shape & S) const
{
  int Index = ShapeToIndex(S);
  TShapeIndexToSubMesh::const_iterator anIter = myShapeIndexToSubMesh.find(Index);
  if (anIter != myShapeIndexToSubMesh.end())
    return anIter->second;
  else
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/// Return the sub mesh by Id of shape it is linked to
///////////////////////////////////////////////////////////////////////////////
SMESHDS_SubMesh * SMESHDS_Mesh::MeshElements(const int Index)
{
  TShapeIndexToSubMesh::const_iterator anIter = myShapeIndexToSubMesh.find(Index);
  if (anIter != myShapeIndexToSubMesh.end())
    return anIter->second;
  else
    return NULL;
}

//=======================================================================
//function : SubMeshIndices
//purpose  : 
//=======================================================================
list<int> SMESHDS_Mesh::SubMeshIndices()
{
  list<int> anIndices;
  std::map<int,SMESHDS_SubMesh*>::iterator anIter = myShapeIndexToSubMesh.begin();
  for (; anIter != myShapeIndexToSubMesh.end(); anIter++) {
    anIndices.push_back((*anIter).first);
  }
  return anIndices;
}

//=======================================================================
//function : GetHypothesis
//purpose  : 
//=======================================================================

const list<const SMESHDS_Hypothesis*>&
SMESHDS_Mesh::GetHypothesis(const TopoDS_Shape & S) const
{
  if ( myShapeToHypothesis.IsBound( S.Oriented(TopAbs_FORWARD) ) ) // ignore orientation of S
     return myShapeToHypothesis.Find( S.Oriented(TopAbs_FORWARD) );

  static list<const SMESHDS_Hypothesis*> empty;
  return empty;
}

//=======================================================================
//function : GetScript
//purpose  : 
//=======================================================================
SMESHDS_Script* SMESHDS_Mesh::GetScript()
{
	return myScript;
}

//=======================================================================
//function : ClearScript
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::ClearScript()
{
	myScript->Clear();
}

//=======================================================================
//function : HasMeshElements
//purpose  : 
//=======================================================================
bool SMESHDS_Mesh::HasMeshElements(const TopoDS_Shape & S)
{
	if (myShape.IsNull()) MESSAGE("myShape is NULL");
	int Index = myIndexToShape.FindIndex(S);
	return myShapeIndexToSubMesh.find(Index)!=myShapeIndexToSubMesh.end();
}

//=======================================================================
//function : HasHypothesis
//purpose  : 
//=======================================================================
bool SMESHDS_Mesh::HasHypothesis(const TopoDS_Shape & S)
{
  return myShapeToHypothesis.IsBound(S.Oriented(TopAbs_FORWARD));
}

//=======================================================================
//function : NewSubMesh 
//purpose  : 
//=======================================================================
SMESHDS_SubMesh * SMESHDS_Mesh::NewSubMesh(int Index)
{
  SMESHDS_SubMesh* SM = 0;
  TShapeIndexToSubMesh::iterator anIter = myShapeIndexToSubMesh.find(Index);
  if (anIter == myShapeIndexToSubMesh.end())
  {
    SM = new SMESHDS_SubMesh();
    myShapeIndexToSubMesh[Index]=SM;
  }
  else
    SM = anIter->second;
  return SM;
}

//=======================================================================
//function : AddCompoundSubmesh
//purpose  : 
//=======================================================================

int SMESHDS_Mesh::AddCompoundSubmesh(const TopoDS_Shape& S,
                                     TopAbs_ShapeEnum    type)
{
  int aMainIndex = 0;
  if ( IsGroupOfSubShapes( S ) || (S.ShapeType() == TopAbs_VERTEX && myIndexToShape.Contains(S)) )
  {
    aMainIndex = myIndexToShape.Add( S );
    bool all = ( type == TopAbs_SHAPE );
    if ( all ) // corresponding simple submesh may exist
      aMainIndex = -aMainIndex;
    //MESSAGE("AddCompoundSubmesh index = " << aMainIndex );
    SMESHDS_SubMesh * aNewSub = NewSubMesh( aMainIndex );
    if ( !aNewSub->IsComplexSubmesh() ) // is empty
    {
      int shapeType = Max( TopAbs_SOLID, all ? myShape.ShapeType() : type );
      int typeLimit = all ? TopAbs_VERTEX : type;
      for ( ; shapeType <= typeLimit; shapeType++ )
      {
        TopExp_Explorer exp( S, TopAbs_ShapeEnum( shapeType ));
        for ( ; exp.More(); exp.Next() )
        {
          int index = myIndexToShape.FindIndex( exp.Current() );
          if ( index )
            aNewSub->AddSubMesh( NewSubMesh( index ));
        }
      }
    }
  }
  return aMainIndex;
}

//=======================================================================
//function : IndexToShape
//purpose  : 
//=======================================================================
const TopoDS_Shape& SMESHDS_Mesh::IndexToShape(int ShapeIndex) const
{
	return myIndexToShape.FindKey(ShapeIndex);
}

//=======================================================================
//function : ShapeToIndex
//purpose  : 
//=======================================================================
int SMESHDS_Mesh::ShapeToIndex(const TopoDS_Shape & S) const
{
  if (myShape.IsNull())
    MESSAGE("myShape is NULL");

  int index = myIndexToShape.FindIndex(S);
  
  return index;
}

//=======================================================================
//function : SetNodeOnVolume
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeInVolume(const SMDS_MeshNode* aNode, int Index)
{
  if ( add( aNode, getSubmesh( Index )))
    ((SMDS_MeshNode*) aNode)->SetPosition( volumePosition( Index ));
}

//=======================================================================
//function : SetNodeOnFace
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeOnFace(SMDS_MeshNode* aNode, int Index, double u, double v)
{
  //Set Position on Node
  if ( add( aNode, getSubmesh( Index )))
    aNode->SetPosition(SMDS_PositionPtr(new SMDS_FacePosition(Index, u, v)));
}

//=======================================================================
//function : SetNodeOnEdge
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeOnEdge(SMDS_MeshNode* aNode,
                                 int            Index,
                                 double         u)
{
  //Set Position on Node
  if ( add( aNode, getSubmesh( Index )))
    aNode->SetPosition(SMDS_PositionPtr(new SMDS_EdgePosition(Index, u)));
}

//=======================================================================
//function : SetNodeOnVertex
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetNodeOnVertex(SMDS_MeshNode* aNode, int Index)
{
  //Set Position on Node
  if ( add( aNode, getSubmesh( Index )))
    aNode->SetPosition(SMDS_PositionPtr(new SMDS_VertexPosition(Index)));
}

//=======================================================================
//function : SetMeshElementOnShape
//purpose  : 
//=======================================================================
void SMESHDS_Mesh::SetMeshElementOnShape(const SMDS_MeshElement* anElement,
                                         int                     Index)
{
  add( anElement, getSubmesh( Index ));
}

//=======================================================================
//function : ~SMESHDS_Mesh
//purpose  : 
//=======================================================================
SMESHDS_Mesh::~SMESHDS_Mesh()
{
  // myScript
  delete myScript;
  // submeshes
  TShapeIndexToSubMesh::iterator i_sm = myShapeIndexToSubMesh.begin();
  for ( ; i_sm != myShapeIndexToSubMesh.end(); ++i_sm )
    delete i_sm->second;
}


//********************************************************************
//********************************************************************
//********                                                   *********
//*****       Methods for addition of quadratic elements        ******
//********                                                   *********
//********************************************************************
//********************************************************************

//=======================================================================
//function : AddEdgeWithID
//purpose  : 
//=======================================================================
SMDS_MeshEdge* SMESHDS_Mesh::AddEdgeWithID(int n1, int n2, int n12, int ID) 
{
  SMDS_MeshEdge* anElem = SMDS_Mesh::AddEdgeWithID(n1,n2,n12,ID);
  if(anElem) myScript->AddEdge(ID,n1,n2,n12);
  return anElem;
}

//=======================================================================
//function : AddEdge
//purpose  : 
//=======================================================================
SMDS_MeshEdge* SMESHDS_Mesh::AddEdge(const SMDS_MeshNode* n1,
                                     const SMDS_MeshNode* n2,
                                     const SMDS_MeshNode* n12)
{
  SMDS_MeshEdge* anElem = SMDS_Mesh::AddEdge(n1,n2,n12);
  if(anElem) myScript->AddEdge(anElem->GetID(), 
			       n1->GetID(), 
			       n2->GetID(),
                               n12->GetID());
  return anElem;
}

//=======================================================================
//function : AddEdgeWithID
//purpose  : 
//=======================================================================
SMDS_MeshEdge* SMESHDS_Mesh::AddEdgeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2, 
                                           const SMDS_MeshNode * n12, 
                                           int ID)
{
  return AddEdgeWithID(n1->GetID(),
		       n2->GetID(),
                       n12->GetID(),
		       ID);
}


//=======================================================================
//function : AddFace
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddFace(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n12,
                                     const SMDS_MeshNode * n23,
                                     const SMDS_MeshNode * n31)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddFace(n1,n2,n3,n12,n23,n31);
  if(anElem) myScript->AddFace(anElem->GetID(), 
			       n1->GetID(), n2->GetID(), n3->GetID(),
                               n12->GetID(), n23->GetID(), n31->GetID());
  return anElem;
}

//=======================================================================
//function : AddFaceWithID
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddFaceWithID(int n1, int n2, int n3,
                                           int n12,int n23,int n31, int ID)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddFaceWithID(n1,n2,n3,n12,n23,n31,ID);
  if(anElem) myScript->AddFace(ID,n1,n2,n3,n12,n23,n31);
  return anElem;
}

//=======================================================================
//function : AddFaceWithID
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n12,
                                           const SMDS_MeshNode * n23,
                                           const SMDS_MeshNode * n31, 
                                           int ID)
{
  return AddFaceWithID(n1->GetID(), n2->GetID(), n3->GetID(),
                       n12->GetID(), n23->GetID(), n31->GetID(),
		       ID);
}


//=======================================================================
//function : AddFace
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddFace(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n12,
                                     const SMDS_MeshNode * n23,
                                     const SMDS_MeshNode * n34,
                                     const SMDS_MeshNode * n41)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddFace(n1,n2,n3,n4,n12,n23,n34,n41);
  if(anElem) myScript->AddFace(anElem->GetID(), 
			       n1->GetID(), n2->GetID(), n3->GetID(), n4->GetID(),
                               n12->GetID(), n23->GetID(), n34->GetID(), n41->GetID());
  return anElem;
}

//=======================================================================
//function : AddFaceWithID
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddFaceWithID(int n1, int n2, int n3, int n4,
                                           int n12,int n23,int n34,int n41, int ID)
{
  SMDS_MeshFace *anElem = SMDS_Mesh::AddFaceWithID(n1,n2,n3,n4,n12,n23,n34,n41,ID);
  if(anElem) myScript->AddFace(ID,n1,n2,n3,n4,n12,n23,n34,n41);
  return anElem;
}

//=======================================================================
//function : AddFaceWithID
//purpose  : 
//=======================================================================
SMDS_MeshFace* SMESHDS_Mesh::AddFaceWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n12,
                                           const SMDS_MeshNode * n23,
                                           const SMDS_MeshNode * n34, 
                                           const SMDS_MeshNode * n41, 
                                           int ID)
{
  return AddFaceWithID(n1->GetID(), n2->GetID(), n3->GetID(), n4->GetID(),
                       n12->GetID(), n23->GetID(), n34->GetID(), n41->GetID(),
		       ID);
}


//=======================================================================
//function : AddVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                         const SMDS_MeshNode * n2, 
                                         const SMDS_MeshNode * n3,
                                         const SMDS_MeshNode * n4,
                                         const SMDS_MeshNode * n12,
                                         const SMDS_MeshNode * n23,
                                         const SMDS_MeshNode * n31,
                                         const SMDS_MeshNode * n14, 
                                         const SMDS_MeshNode * n24,
                                         const SMDS_MeshNode * n34)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolume(n1,n2,n3,n4,n12,n23,n31,n14,n24,n34);
  if(anElem) myScript->AddVolume(anElem->GetID(), 
				 n1->GetID(), n2->GetID(), n3->GetID(), n4->GetID(),
				 n12->GetID(), n23->GetID(), n31->GetID(),
                                 n14->GetID(), n24->GetID(), n34->GetID());
  return anElem;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4,
                                               int n12,int n23,int n31,
                                               int n14,int n24,int n34, int ID)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolumeWithID(n1,n2,n3,n4,n12,n23,
                                                       n31,n14,n24,n34,ID);
  if(anElem) myScript->AddVolume(ID,n1,n2,n3,n4,n12,n23,n31,n14,n24,n34);
  return anElem;
}
	
//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order tetrahedron of 10 nodes
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                               const SMDS_MeshNode * n2,
                                               const SMDS_MeshNode * n3,
                                               const SMDS_MeshNode * n4,
                                               const SMDS_MeshNode * n12,
                                               const SMDS_MeshNode * n23,
                                               const SMDS_MeshNode * n31,
                                               const SMDS_MeshNode * n14, 
                                               const SMDS_MeshNode * n24,
                                               const SMDS_MeshNode * n34,
                                               int ID)
{
  return AddVolumeWithID(n1->GetID(), n2->GetID(), n3->GetID(), n4->GetID(),
                         n12->GetID(), n23->GetID(), n31->GetID(),
                         n14->GetID(), n24->GetID(), n34->GetID(), ID);
}


//=======================================================================
//function : AddVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                         const SMDS_MeshNode * n2, 
                                         const SMDS_MeshNode * n3,
                                         const SMDS_MeshNode * n4,
                                         const SMDS_MeshNode * n5, 
                                         const SMDS_MeshNode * n12,
                                         const SMDS_MeshNode * n23,
                                         const SMDS_MeshNode * n34,
                                         const SMDS_MeshNode * n41,
                                         const SMDS_MeshNode * n15, 
                                         const SMDS_MeshNode * n25,
                                         const SMDS_MeshNode * n35,
                                         const SMDS_MeshNode * n45)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolume(n1,n2,n3,n4,n5,n12,n23,n34,n41,
                                                 n15,n25,n35,n45);
  if(anElem)
    myScript->AddVolume(anElem->GetID(), n1->GetID(), n2->GetID(),
                        n3->GetID(), n4->GetID(), n5->GetID(),
                        n12->GetID(), n23->GetID(), n34->GetID(), n41->GetID(),
                        n15->GetID(), n25->GetID(), n35->GetID(), n45->GetID());
  return anElem;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4, int n5,
                                               int n12,int n23,int n34,int n41,
                                               int n15,int n25,int n35,int n45, int ID)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolumeWithID(n1,n2,n3,n4,n5,
                                                       n12,n23,n34,n41,
                                                       n15,n25,n35,n45,ID);
  if(anElem) myScript->AddVolume(ID,n1,n2,n3,n4,n5,n12,n23,n34,n41,
                                 n15,n25,n35,n45);
  return anElem;
}
	
//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order pyramid of 13 nodes
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                               const SMDS_MeshNode * n2,
                                               const SMDS_MeshNode * n3,
                                               const SMDS_MeshNode * n4,
                                               const SMDS_MeshNode * n5, 
                                               const SMDS_MeshNode * n12,
                                               const SMDS_MeshNode * n23,
                                               const SMDS_MeshNode * n34,
                                               const SMDS_MeshNode * n41,
                                               const SMDS_MeshNode * n15, 
                                               const SMDS_MeshNode * n25,
                                               const SMDS_MeshNode * n35,
                                               const SMDS_MeshNode * n45,
                                               int ID)
{
  return AddVolumeWithID(n1->GetID(), n2->GetID(), n3->GetID(),
                         n4->GetID(), n5->GetID(),
                         n12->GetID(), n23->GetID(), n34->GetID(), n41->GetID(),
                         n15->GetID(), n25->GetID(), n35->GetID(), n45->GetID(),
                         ID);
}


//=======================================================================
//function : AddVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                         const SMDS_MeshNode * n2, 
                                         const SMDS_MeshNode * n3,
                                         const SMDS_MeshNode * n4,
                                         const SMDS_MeshNode * n5, 
                                         const SMDS_MeshNode * n6, 
                                         const SMDS_MeshNode * n12,
                                         const SMDS_MeshNode * n23,
                                         const SMDS_MeshNode * n31, 
                                         const SMDS_MeshNode * n45,
                                         const SMDS_MeshNode * n56,
                                         const SMDS_MeshNode * n64, 
                                         const SMDS_MeshNode * n14,
                                         const SMDS_MeshNode * n25,
                                         const SMDS_MeshNode * n36)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolume(n1,n2,n3,n4,n5,n6,n12,n23,n31,
                                                 n45,n56,n64,n14,n25,n36);
  if(anElem)
    myScript->AddVolume(anElem->GetID(), n1->GetID(), n2->GetID(),
                        n3->GetID(), n4->GetID(), n5->GetID(), n6->GetID(),
                        n12->GetID(), n23->GetID(), n31->GetID(),
                        n45->GetID(), n56->GetID(), n64->GetID(),
                        n14->GetID(), n25->GetID(), n36->GetID());
  return anElem;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(int n1, int n2, int n3,
                                               int n4, int n5, int n6,
                                               int n12,int n23,int n31,
                                               int n45,int n56,int n64,
                                               int n14,int n25,int n36, int ID)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolumeWithID(n1,n2,n3,n4,n5,n6,
                                                       n12,n23,n31,
                                                       n45,n56,n64,
                                                       n14,n25,n36,ID);
  if(anElem) myScript->AddVolume(ID,n1,n2,n3,n4,n5,n6,n12,n23,n31,
                                 n45,n56,n64,n14,n25,n36);
  return anElem;
}
	
//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order Pentahedron with 15 nodes
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                               const SMDS_MeshNode * n2,
                                               const SMDS_MeshNode * n3,
                                               const SMDS_MeshNode * n4,
                                               const SMDS_MeshNode * n5, 
                                               const SMDS_MeshNode * n6, 
                                               const SMDS_MeshNode * n12,
                                               const SMDS_MeshNode * n23,
                                               const SMDS_MeshNode * n31, 
                                               const SMDS_MeshNode * n45,
                                               const SMDS_MeshNode * n56,
                                               const SMDS_MeshNode * n64, 
                                               const SMDS_MeshNode * n14,
                                               const SMDS_MeshNode * n25,
                                               const SMDS_MeshNode * n36,
                                               int ID)
{
  return AddVolumeWithID(n1->GetID(), n2->GetID(), n3->GetID(),
                         n4->GetID(), n5->GetID(), n6->GetID(),
                         n12->GetID(), n23->GetID(), n31->GetID(),
                         n45->GetID(), n56->GetID(), n64->GetID(),
                         n14->GetID(), n25->GetID(), n36->GetID(),
                         ID);
}


//=======================================================================
//function : AddVolume
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolume(const SMDS_MeshNode * n1,
                                         const SMDS_MeshNode * n2, 
                                         const SMDS_MeshNode * n3,
                                         const SMDS_MeshNode * n4,
                                         const SMDS_MeshNode * n5, 
                                         const SMDS_MeshNode * n6, 
                                         const SMDS_MeshNode * n7,
                                         const SMDS_MeshNode * n8, 
                                         const SMDS_MeshNode * n12,
                                         const SMDS_MeshNode * n23,
                                         const SMDS_MeshNode * n34,
                                         const SMDS_MeshNode * n41, 
                                         const SMDS_MeshNode * n56,
                                         const SMDS_MeshNode * n67,
                                         const SMDS_MeshNode * n78,
                                         const SMDS_MeshNode * n85, 
                                         const SMDS_MeshNode * n15,
                                         const SMDS_MeshNode * n26,
                                         const SMDS_MeshNode * n37,
                                         const SMDS_MeshNode * n48)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolume(n1,n2,n3,n4,n5,n6,n7,n8,
                                                 n12,n23,n34,n41,
                                                 n56,n67,n78,n85,
                                                 n15,n26,n37,n48);
  if(anElem)
    myScript->AddVolume(anElem->GetID(), n1->GetID(), n2->GetID(),
                        n3->GetID(), n4->GetID(), n5->GetID(),
                        n6->GetID(), n7->GetID(), n8->GetID(),
                        n12->GetID(), n23->GetID(), n34->GetID(), n41->GetID(),
                        n56->GetID(), n67->GetID(), n78->GetID(), n85->GetID(),
                        n15->GetID(), n26->GetID(), n37->GetID(), n48->GetID());
  return anElem;
}

//=======================================================================
//function : AddVolumeWithID
//purpose  : 
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(int n1, int n2, int n3, int n4,
                                               int n5, int n6, int n7, int n8,
                                               int n12,int n23,int n34,int n41,
                                               int n56,int n67,int n78,int n85,
                                               int n15,int n26,int n37,int n48, int ID)
{
  SMDS_MeshVolume *anElem = SMDS_Mesh::AddVolumeWithID(n1,n2,n3,n4,n5,n6,n7,n8,
                                                       n12,n23,n34,n41,
                                                       n56,n67,n78,n85,
                                                       n15,n26,n37,n48,ID);
  if(anElem) myScript->AddVolume(ID,n1,n2,n3,n4,n5,n6,n7,n8,n12,n23,n34,n41,
                                 n56,n67,n78,n85,n15,n26,n37,n48);
  return anElem;
}
	
//=======================================================================
//function : AddVolumeWithID
//purpose  : 2d order Hexahedrons with 20 nodes
//=======================================================================
SMDS_MeshVolume* SMESHDS_Mesh::AddVolumeWithID(const SMDS_MeshNode * n1,
                                               const SMDS_MeshNode * n2,
                                               const SMDS_MeshNode * n3,
                                               const SMDS_MeshNode * n4,
                                               const SMDS_MeshNode * n5, 
                                               const SMDS_MeshNode * n6, 
                                               const SMDS_MeshNode * n7,
                                               const SMDS_MeshNode * n8, 
                                               const SMDS_MeshNode * n12,
                                               const SMDS_MeshNode * n23,
                                               const SMDS_MeshNode * n34,
                                               const SMDS_MeshNode * n41, 
                                               const SMDS_MeshNode * n56,
                                               const SMDS_MeshNode * n67,
                                               const SMDS_MeshNode * n78,
                                               const SMDS_MeshNode * n85, 
                                               const SMDS_MeshNode * n15,
                                               const SMDS_MeshNode * n26,
                                               const SMDS_MeshNode * n37,
                                               const SMDS_MeshNode * n48,
                                               int ID)
{
  return AddVolumeWithID(n1->GetID(), n2->GetID(), n3->GetID(), n4->GetID(),
                         n5->GetID(), n6->GetID(), n7->GetID(), n8->GetID(),
                         n12->GetID(), n23->GetID(), n34->GetID(), n41->GetID(),
                         n56->GetID(), n67->GetID(), n78->GetID(), n85->GetID(),
                         n15->GetID(), n26->GetID(), n37->GetID(), n48->GetID(),
                         ID);
}


