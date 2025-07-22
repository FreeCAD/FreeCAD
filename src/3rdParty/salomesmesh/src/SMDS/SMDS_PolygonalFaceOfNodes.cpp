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

#include "SMDS_PolygonalFaceOfNodes.hxx"

#include "SMDS_IteratorOfElements.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMDS_Mesh.hxx"

#include "utilities.h"

using namespace std;

//=======================================================================
//function : Constructor
//purpose  : 
//=======================================================================
SMDS_PolygonalFaceOfNodes::SMDS_PolygonalFaceOfNodes
                          (const std::vector<const SMDS_MeshNode *>& nodes)
{
  //MESSAGE("******************************************** SMDS_PolygonalFaceOfNodes");
  myNodes = nodes;
}

//=======================================================================
//function : GetType
//purpose  : 
//=======================================================================
SMDSAbs_ElementType SMDS_PolygonalFaceOfNodes::GetType() const
{
  return SMDSAbs_Face;
  //return SMDSAbs_PolygonalFace;
}

//=======================================================================
//function : ChangeNodes
//purpose  : 
//=======================================================================
bool SMDS_PolygonalFaceOfNodes::ChangeNodes (std::vector<const SMDS_MeshNode *> nodes)
{
  if (nodes.size() < 3)
    return false;

  myNodes = nodes;

  return true;
}

//=======================================================================
//function : ChangeNodes
//purpose  : to support the same interface, as SMDS_FaceOfNodes
//=======================================================================
bool SMDS_PolygonalFaceOfNodes::ChangeNodes (const SMDS_MeshNode* nodes[],
                                             const int            nbNodes)
{
  if (nbNodes < 3)
    return false;

  myNodes.resize(nbNodes);
  int i = 0;
  for (; i < nbNodes; i++) {
    myNodes[i] = nodes[i];
  }

  return true;
}

//=======================================================================
//function : NbNodes
//purpose  : 
//=======================================================================
int SMDS_PolygonalFaceOfNodes::NbNodes() const
{
  return myNodes.size();
}

//=======================================================================
//function : NbEdges
//purpose  : 
//=======================================================================
int SMDS_PolygonalFaceOfNodes::NbEdges() const
{
  return NbNodes();
}

//=======================================================================
//function : NbFaces
//purpose  : 
//=======================================================================
int SMDS_PolygonalFaceOfNodes::NbFaces() const
{
  return 1;
}

//=======================================================================
//function : Print
//purpose  : 
//=======================================================================
void SMDS_PolygonalFaceOfNodes::Print(ostream & OS) const
{
  OS << "polygonal face <" << GetID() << " > : ";
  int i, nbNodes = myNodes.size();
  for (i = 0; i < nbNodes - 1; i++)
    OS << myNodes[i] << ",";
  OS << myNodes[i] << ") " << endl;
}

//=======================================================================
//function : elementsIterator
//purpose  : 
//=======================================================================
class SMDS_PolygonalFaceOfNodes_MyIterator:public SMDS_NodeVectorElemIterator
{
 public:
  SMDS_PolygonalFaceOfNodes_MyIterator(const vector<const SMDS_MeshNode *>& s):
    SMDS_NodeVectorElemIterator( s.begin(), s.end() ) {}
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
  _MyEdgeIterator(const SMDS_MeshFace* face):myIndex(0) {
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

SMDS_ElemIteratorPtr SMDS_PolygonalFaceOfNodes::elementsIterator
                                         (SMDSAbs_ElementType type) const
{
  switch(type)
  {
  case SMDSAbs_Face:
    return SMDS_MeshElement::elementsIterator(SMDSAbs_Face);
  case SMDSAbs_Node:
    return SMDS_ElemIteratorPtr(new SMDS_PolygonalFaceOfNodes_MyIterator(myNodes));
  case SMDSAbs_Edge:
    return SMDS_ElemIteratorPtr(new _MyEdgeIterator( this ));
    break;
  default:
    return SMDS_ElemIteratorPtr
      (new SMDS_IteratorOfElements
       (this,type,SMDS_ElemIteratorPtr
        (new SMDS_PolygonalFaceOfNodes_MyIterator(myNodes))));
  }
  return SMDS_ElemIteratorPtr();
}

/*!
 * \brief Return node by its index
 * \param ind - node index
 * \retval const SMDS_MeshNode* - the node
 */
const SMDS_MeshNode* SMDS_PolygonalFaceOfNodes::GetNode(const int ind) const
{
  return myNodes[ WrappedIndex( ind )];
}
