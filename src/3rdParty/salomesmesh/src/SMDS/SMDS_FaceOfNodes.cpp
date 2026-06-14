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

#include "SMDS_SetIterator.hxx"
#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_IteratorOfElements.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_Mesh.hxx"

#include "utilities.h"

using namespace std;

//=======================================================================
//function : NbEdges
//purpose  : 
//=======================================================================

int SMDS_FaceOfNodes::NbEdges() const
{
        return NbNodes();
}

int SMDS_FaceOfNodes::NbFaces() const
{
        return 1;
}

int SMDS_FaceOfNodes::NbNodes() const
{
        return myNbNodes;
}

//=======================================================================
//function : Print
//purpose  : 
//=======================================================================

void SMDS_FaceOfNodes::Print(ostream & OS) const
{
        OS << "face <" << GetID() << " > : ";
        int i;
        for (i = 0; i < NbNodes() - 1; i++) OS << myNodes[i] << ",";
        OS << myNodes[i] << ") " << endl;
}

//=======================================================================
//function : elementsIterator
//purpose  : 
//=======================================================================

class SMDS_FaceOfNodes_MyIterator:public SMDS_NodeArrayElemIterator
{
 public:
  SMDS_FaceOfNodes_MyIterator(const SMDS_MeshNode* const *s, int l):
    SMDS_NodeArrayElemIterator( s, & s[ l ] ) {}
};

/// ===================================================================
/*!
 * \brief Iterator on edges of face
 */
/// ===================================================================

class _MyEdgeIterator : public SMDS_ElemIterator
{
  vector< const SMDS_MeshElement* > myElems;
  int myIndex;
public:
  _MyEdgeIterator(const SMDS_FaceOfNodes* face):myIndex(0) {
    myElems.reserve( face->NbNodes() );
    for ( int i = 0; i < face->NbNodes(); ++i ) {
      const SMDS_MeshElement* edge =
        SMDS_Mesh::FindEdge( face->GetNode( i ), face->GetNodeWrap( i + 1 ));
      if ( edge )
        myElems.push_back( edge );
    }
  }
  /// Return true if and only if there are other object in this iterator
  virtual bool more() { return myIndex < myElems.size(); }

  /// Return the current object and step to the next one
  virtual const SMDS_MeshElement* next() { return myElems[ myIndex++ ]; }
};

SMDS_ElemIteratorPtr SMDS_FaceOfNodes::elementsIterator
                         (SMDSAbs_ElementType type) const
{
  switch(type)
  {
  case SMDSAbs_Face:
    return SMDS_MeshElement::elementsIterator(SMDSAbs_Face);
  case SMDSAbs_Node:
    return SMDS_ElemIteratorPtr(new SMDS_FaceOfNodes_MyIterator(myNodes,myNbNodes));
  case SMDSAbs_Edge:
    return SMDS_ElemIteratorPtr(new _MyEdgeIterator( this ));
    break;
  default:
    return SMDS_ElemIteratorPtr
      (new SMDS_IteratorOfElements
       (this,type,SMDS_ElemIteratorPtr
        (new SMDS_FaceOfNodes_MyIterator(myNodes,myNbNodes))));
  }
  return SMDS_ElemIteratorPtr();
}

SMDS_FaceOfNodes::SMDS_FaceOfNodes(const SMDS_MeshNode* node1,
                                   const SMDS_MeshNode* node2,
                                   const SMDS_MeshNode* node3)
{
  //MESSAGE("******************************************************* SMDS_FaceOfNodes");
        myNbNodes = 3;
        myNodes[0]=node1;
        myNodes[1]=node2;
        myNodes[2]=node3;
        myNodes[3]=0;
}

SMDS_FaceOfNodes::SMDS_FaceOfNodes(const SMDS_MeshNode* node1,
                                   const SMDS_MeshNode* node2,
                                   const SMDS_MeshNode* node3,
                                   const SMDS_MeshNode* node4)
{
  //MESSAGE("******************************************************* SMDS_FaceOfNodes");
        myNbNodes = 4;
        myNodes[0]=node1;
        myNodes[1]=node2;
        myNodes[2]=node3;
        myNodes[3]=node4;       
}
bool SMDS_FaceOfNodes::ChangeNodes(const SMDS_MeshNode* nodes[],
                                   const int            nbNodes)
{
  myNbNodes = nbNodes;
  myNodes[0]=nodes[0];
  myNodes[1]=nodes[1];
  myNodes[2]=nodes[2];
  if (nbNodes == 4)
    myNodes[3]=nodes[3];
  else if (nbNodes != 3)
    return false;

  return true;
}

/*!
 * \brief Return node by its index
 * \param ind - node index
 * \retval const SMDS_MeshNode* - the node
 */
const SMDS_MeshNode* SMDS_FaceOfNodes::GetNode(const int ind) const
{
  return myNodes[ ind ];
}

SMDSAbs_EntityType SMDS_FaceOfNodes::GetEntityType() const
{
  return NbNodes() == 3 ? SMDSEntity_Triangle : SMDSEntity_Quadrangle;
}
SMDSAbs_GeometryType SMDS_FaceOfNodes::GetGeomType() const
{
  return NbNodes() == 3 ? SMDSGeom_TRIANGLE : SMDSGeom_QUADRANGLE;
}
