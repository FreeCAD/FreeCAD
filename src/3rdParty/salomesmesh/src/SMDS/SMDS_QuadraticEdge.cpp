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
// File:      SMDS_QuadraticEdge.cxx
// Created:   16.01.06 16:25:42
// Author:    Sergey KUUL
//
#include "SMDS_QuadraticEdge.hxx"

#include "SMDS_SetIterator.hxx"
#include "SMDS_IteratorOfElements.hxx"
#include "SMDS_MeshNode.hxx"
#include "utilities.h"

using namespace std;

//=======================================================================
//function : SMDS_QuadraticEdge
//purpose  : 
//=======================================================================

SMDS_QuadraticEdge::SMDS_QuadraticEdge(const SMDS_MeshNode * node1,
                                       const SMDS_MeshNode * node2,
                                       const SMDS_MeshNode * node12)
     :SMDS_LinearEdge(node1,node2)
{
  //MESSAGE("******************************************************* SMDS_QuadraticEdge");
  myNodes[2]=node12;
}


//=======================================================================
//function : Print
//purpose  : 
//=======================================================================

void SMDS_QuadraticEdge::Print(ostream & OS) const
{
  OS << "quadratic edge <" << GetID() << "> : ( first-" << myNodes[0]
     << " , last-" << myNodes[1] << " , medium-" << myNodes[2] << ") " << endl;
}


//=======================================================================
//function : NbNodes
//purpose  : 
//=======================================================================

int SMDS_QuadraticEdge::NbNodes() const
{
  return 3;
}

//=======================================================================
//function : ChangeNodes
//purpose  : 
//=======================================================================

bool SMDS_QuadraticEdge::ChangeNodes(const SMDS_MeshNode * node1,
                                     const SMDS_MeshNode * node2,
                                     const SMDS_MeshNode * node12)
{
  myNodes[0]=node1;
  myNodes[1]=node2;
  myNodes[2]=node12;
  return true;
}

//=======================================================================
//function : IsMediumNode
//purpose  : 
//=======================================================================

bool SMDS_QuadraticEdge::IsMediumNode(const SMDS_MeshNode * node) const
{
  return (myNodes[2]==node);
}

namespace
{
  //=======================================================================
  //class : _MyInterlacedNodeIterator
  //purpose  : 
  //=======================================================================

  class _MyInterlacedNodeIterator: public SMDS_NodeArrayIterator
  {
    const SMDS_MeshNode * myNodes[3];
  public:
    _MyInterlacedNodeIterator(const SMDS_MeshNode * const * nodes):
      SMDS_NodeArrayIterator( myNodes, & myNodes[3] )
    {
      myNodes[0] = nodes[0];
      myNodes[1] = nodes[2];
      myNodes[2] = nodes[1];
    }
  };

  //=======================================================================
  //class : _MyNodeIterator
  //purpose  : 
  //=======================================================================

  class _MyNodeIterator:public SMDS_NodeArrayElemIterator
  {
  public:
    _MyNodeIterator(const SMDS_MeshNode * const * nodes):
      SMDS_NodeArrayElemIterator( nodes, & nodes[3] ) {}
  };
}

//=======================================================================
//function : interlacedNodesIterator
//purpose  : 
//=======================================================================

SMDS_NodeIteratorPtr SMDS_QuadraticEdge::interlacedNodesIterator() const
{
  return SMDS_NodeIteratorPtr (new _MyInterlacedNodeIterator (myNodes));
}

//=======================================================================
//function : elementsIterator
//purpose  : 
//=======================================================================

SMDS_ElemIteratorPtr SMDS_QuadraticEdge::elementsIterator(SMDSAbs_ElementType type) const
{
  switch(type)
  {
  case SMDSAbs_Edge:
    return SMDS_MeshElement::elementsIterator(SMDSAbs_Edge); 
  case SMDSAbs_Node:
    return SMDS_ElemIteratorPtr(new _MyNodeIterator(myNodes));
  default:
    return SMDS_ElemIteratorPtr
      (new SMDS_IteratorOfElements
       (this,type, SMDS_ElemIteratorPtr(new _MyNodeIterator(myNodes))));
  }
}
