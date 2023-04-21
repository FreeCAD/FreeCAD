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

//  SMESH SMDS : implementation of Salome mesh data structure
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_PolyhedralVolumeOfNodes.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMDS_VolumeTool.hxx"
#include "utilities.h"

#include <set>

using namespace std;

//=======================================================================
//function : Constructor
//purpose  : Create a volume of many faces
//=======================================================================
SMDS_PolyhedralVolumeOfNodes::SMDS_PolyhedralVolumeOfNodes
                                (vector<const SMDS_MeshNode *> nodes,
                                 vector<int>                   quantities)
: SMDS_VolumeOfNodes(NULL, NULL, NULL, NULL)
{
  //MESSAGE("****************************************** SMDS_PolyhedralVolumeOfNodes");
  ChangeNodes(nodes, quantities);
}

//=======================================================================
//function : GetType
//purpose  : 
//=======================================================================
SMDSAbs_ElementType SMDS_PolyhedralVolumeOfNodes::GetType() const
{
//  return SMDSAbs_PolyhedralVolume;
  return SMDSAbs_Volume;
}

//=======================================================================
//function : ChangeNodes
//purpose  : 
//=======================================================================
bool SMDS_PolyhedralVolumeOfNodes::ChangeNodes (const vector<const SMDS_MeshNode *>& nodes,
                                                const vector<int>&                   quantities)
{
  myNodesByFaces = nodes;
  myQuantities = quantities;

  // Init fields of parent class, it allows to get only unique nodes(?)

  set<const SMDS_MeshNode *> aSet;
  aSet.insert( nodes.begin(), nodes.end());
  //SMDS_VolumeOfNodes::ChangeNodes(aNodes, aNbNodes);
  delete [] myNodes;
  myNbNodes = aSet.size();
  myNodes = new const SMDS_MeshNode* [myNbNodes];
  set<const SMDS_MeshNode *>::iterator anIter = aSet.begin();
  for (int k=0; anIter != aSet.end(); anIter++, k++)
    myNodes[k] = *anIter;

  return true;
}

//=======================================================================
//function : NbEdges
//purpose  : 
//=======================================================================
int SMDS_PolyhedralVolumeOfNodes::NbNodes() const
{
  return myNodesByFaces.size();
}

//=======================================================================
//function : NbEdges
//purpose  : 
//=======================================================================
int SMDS_PolyhedralVolumeOfNodes::NbEdges() const
{
  int nbEdges = 0;

  for (int ifa = 0; ifa < myQuantities.size(); ifa++) {
    nbEdges += myQuantities[ifa];
  }
  nbEdges /= 2;

  return nbEdges;
}

//=======================================================================
//function : NbFaces
//purpose  : 
//=======================================================================
int SMDS_PolyhedralVolumeOfNodes::NbFaces() const
{
  return myQuantities.size();
}

//=======================================================================
//function : NbFaceNodes
//purpose  : 
//=======================================================================
int SMDS_PolyhedralVolumeOfNodes::NbFaceNodes (const int face_ind) const
{
  if (face_ind < 1 || myQuantities.size() < face_ind)
    return 0;
  return myQuantities[face_ind - 1];
}

//=======================================================================
//function : GetFaceNode
//purpose  : 
//=======================================================================
const SMDS_MeshNode* SMDS_PolyhedralVolumeOfNodes::GetFaceNode (const int face_ind,
                                                                const int node_ind) const
{
  if (node_ind < 1 || NbFaceNodes(face_ind) < node_ind)
    return NULL;

  int i, first_node = 0;
  for (i = 0; i < face_ind - 1; i++) {
    first_node += myQuantities[i];
  }

  return myNodesByFaces[first_node + node_ind - 1];
}

//=======================================================================
//function : Print
//purpose  : 
//=======================================================================
void SMDS_PolyhedralVolumeOfNodes::Print (ostream & OS) const
{
  OS << "polyhedral volume <" << GetID() << "> : ";

  int faces_len = myQuantities.size();
  //int nodes_len = myNodesByFaces.size();
  int cur_first_node = 0;

  int i, j;
  for (i = 0; i < faces_len; i++) {
    OS << "face_" << i << " (";
    for (j = 0; j < myQuantities[i] - 1; j++) {
      OS << myNodesByFaces[cur_first_node + j] << ",";
    }
    OS << myNodesByFaces[cur_first_node + j] << ") ";
    cur_first_node += myQuantities[i];
  }
}

//=======================================================================
//function : ChangeNodes
//purpose  : usage disabled
//=======================================================================
bool SMDS_PolyhedralVolumeOfNodes::ChangeNodes (const SMDS_MeshNode* nodes[],
                                                const int            nbNodes)
{
  return false;
}

/// ===================================================================
/*!
 * \brief Iterator on node of volume
 */
/// ===================================================================

struct _MyIterator:public SMDS_NodeVectorElemIterator
{
  _MyIterator(const vector<const SMDS_MeshNode *>& nodes):
    SMDS_NodeVectorElemIterator( nodes.begin(), nodes.end()) {}
};

/// ===================================================================
/*!
 * \brief Iterator on faces or edges of volume
 */
/// ===================================================================

class _MySubIterator : public SMDS_ElemIterator
{
  vector< const SMDS_MeshElement* > myElems;
  int myIndex;
public:
  _MySubIterator(const SMDS_MeshVolume* vol, SMDSAbs_ElementType type):myIndex(0) {
    SMDS_VolumeTool vTool(vol);
    if (type == SMDSAbs_Face)
      vTool.GetAllExistingFaces( myElems );
    else
      vTool.GetAllExistingEdges( myElems );
  }
  /// Return true if and only if there are other object in this iterator
  virtual bool more() { return myIndex < myElems.size(); }

  /// Return the current object and step to the next one
  virtual const SMDS_MeshElement* next() { return myElems[ myIndex++ ]; }
};

//================================================================================
/*!
 * \brief Return Iterator of sub elements
 */
//================================================================================

SMDS_ElemIteratorPtr SMDS_PolyhedralVolumeOfNodes::elementsIterator(SMDSAbs_ElementType type) const
{
  switch(type)
  {
  case SMDSAbs_Volume:
    return SMDS_MeshElement::elementsIterator(SMDSAbs_Volume);
  case SMDSAbs_Node:
    return SMDS_ElemIteratorPtr(new _MyIterator(myNodesByFaces));
  case SMDSAbs_Face:
    return SMDS_ElemIteratorPtr(new _MySubIterator(this,SMDSAbs_Face));
  case SMDSAbs_Edge:
    return SMDS_ElemIteratorPtr(new _MySubIterator(this,SMDSAbs_Edge));
  default:
    MESSAGE("ERROR : Iterator not implemented");
    return SMDS_ElemIteratorPtr((SMDS_ElemIterator*)NULL);
  }
}

//================================================================================
/*!
 * \brief Return iterator on unique nodes
 */
//================================================================================

SMDS_ElemIteratorPtr SMDS_PolyhedralVolumeOfNodes::uniqueNodesIterator() const
{
  return SMDS_ElemIteratorPtr
    (new SMDS_NodeArrayElemIterator( myNodes, & myNodes[ myNbNodes ]));
}

//================================================================================
/*!
 * \brief Return node by its index
 */
//================================================================================

const SMDS_MeshNode* SMDS_PolyhedralVolumeOfNodes::GetNode(const int ind) const
{
  return myNodesByFaces[ ind ];
}
