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
//  File   : SMDS_LinearEdge.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_LinearEdge.hxx"
#include "SMDS_IteratorOfElements.hxx"
#include "SMDS_MeshNode.hxx"
#include "utilities.h"

using namespace std;

//=======================================================================
//function : SMDS_LinearEdge
//purpose  : 
//=======================================================================

SMDS_LinearEdge::SMDS_LinearEdge(const SMDS_MeshNode * node1,
                                 const SMDS_MeshNode * node2)
{
  //MESSAGE("SMDS_LinearEdge " << GetID());
  myNodes[0] = node1;
  myNodes[1] = node2;
}

//=======================================================================
//function : Print
//purpose  : 
//=======================================================================

void SMDS_LinearEdge::Print(ostream & OS) const
{
  OS << "edge <" << GetID() << "> : (" << myNodes[0] << " , " << myNodes[1]
      << ") " << endl;
}

int SMDS_LinearEdge::NbNodes() const
{
  return 2;
}

int SMDS_LinearEdge::NbEdges() const
{
  return 1;
}

class SMDS_LinearEdge_MyNodeIterator: public SMDS_ElemIterator
{
  const SMDS_MeshNode * const * myNodes;
  int myIndex;
public:
  SMDS_LinearEdge_MyNodeIterator(const SMDS_MeshNode * const * nodes) :
    myNodes(nodes), myIndex(0)
  {
  }

  bool more()
  {
    return myIndex < 2;
  }

  const SMDS_MeshElement* next()
  {
    myIndex++;
    return myNodes[myIndex - 1];
  }
};

SMDS_ElemIteratorPtr SMDS_LinearEdge::elementsIterator(SMDSAbs_ElementType type) const
{
  switch (type)
  {
    case SMDSAbs_Edge:
      return SMDS_MeshElement::elementsIterator(SMDSAbs_Edge);
    case SMDSAbs_Node:
      return SMDS_ElemIteratorPtr(new SMDS_LinearEdge_MyNodeIterator(myNodes));
    default:
      return SMDS_ElemIteratorPtr
        (new SMDS_IteratorOfElements(this,
                                     type,
                                     SMDS_ElemIteratorPtr
                                     (new SMDS_LinearEdge_MyNodeIterator(myNodes))));
  }
}

bool operator<(const SMDS_LinearEdge & e1, const SMDS_LinearEdge & e2)
{
  int id11 = e1.myNodes[0]->getVtkId();
  int id21 = e2.myNodes[0]->getVtkId();
  int id12 = e1.myNodes[1]->getVtkId();
  int id22 = e2.myNodes[1]->getVtkId();
  int tmp;

  if (id11 >= id12)
    {
      tmp = id11;
      id11 = id12;
      id12 = tmp;
    }
  if (id21 >= id22)
    {
      tmp = id21;
      id21 = id22;
      id22 = tmp;
    }

  if (id11 < id21)
    return true;
  else if (id11 == id21)
    return (id21 < id22);
  else
    return false;
}

/*!
 * \brief Return node by its index
 * \param ind - node index
 * \retval const SMDS_MeshNode* - the node
 */
const SMDS_MeshNode* SMDS_LinearEdge::GetNode(const int ind) const
{
  return myNodes[ind];
}

//=======================================================================
//function : ChangeNodes
//purpose  : 
//=======================================================================

bool SMDS_LinearEdge::ChangeNodes(const SMDS_MeshNode * node1,
                                  const SMDS_MeshNode * node2)
{
  myNodes[0] = node1;
  myNodes[1] = node2;
  return true;
}
