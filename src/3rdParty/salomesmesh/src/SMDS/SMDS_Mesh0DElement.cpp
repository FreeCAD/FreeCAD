// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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
//  File   : SMDS_Mesh0DElement.cxx
//  Module : SMESH
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_Mesh0DElement.hxx"
#include "SMDS_IteratorOfElements.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_Mesh.hxx"

#include "utilities.h"

using namespace std;

//=======================================================================
//function : SMDS_Mesh0DElement
//purpose  :
//=======================================================================
SMDS_Mesh0DElement::SMDS_Mesh0DElement (const SMDS_MeshNode * node)
{
  myNode = node;
}

//=======================================================================
//function : Print
//purpose  :
//=======================================================================
void SMDS_Mesh0DElement::Print (ostream & OS) const
{
  OS << "0D Element <" << GetID() << "> : (" << myNode << ") " << endl;
}

//=======================================================================
//function : NbNodes
//purpose  :
//=======================================================================
int SMDS_Mesh0DElement::NbNodes() const
{
  return 1;
}

//=======================================================================
//function : NbEdges
//purpose  :
//=======================================================================
int SMDS_Mesh0DElement::NbEdges() const
{
  return 0;
}

//=======================================================================
//function : GetType
//purpose  :
//=======================================================================
SMDSAbs_ElementType SMDS_Mesh0DElement::GetType() const
{
  return SMDSAbs_0DElement;
}

vtkIdType SMDS_Mesh0DElement::GetVtkType() const
{
  return VTK_VERTEX;
}

//=======================================================================
//function : elementsIterator
//purpose  :
//=======================================================================
class SMDS_Mesh0DElement_MyNodeIterator: public SMDS_ElemIterator
{
  const SMDS_MeshNode * myNode;
  int myIndex;
 public:
  SMDS_Mesh0DElement_MyNodeIterator(const SMDS_MeshNode * node):
    myNode(node),myIndex(0) {}

  bool more()
  {
    return myIndex < 1;
  }

  const SMDS_MeshElement* next()
  {
    myIndex++;
    if (myIndex == 1)
      return myNode;
    return NULL;
  }
};

SMDS_ElemIteratorPtr SMDS_Mesh0DElement::elementsIterator (SMDSAbs_ElementType type) const
{
  switch(type)
  {
  case SMDSAbs_0DElement:
    return SMDS_MeshElement::elementsIterator(SMDSAbs_0DElement);
  case SMDSAbs_Node:
    return SMDS_ElemIteratorPtr(new SMDS_Mesh0DElement_MyNodeIterator(myNode));
  default:
    return SMDS_ElemIteratorPtr
      (new SMDS_IteratorOfElements
       (this,type, SMDS_ElemIteratorPtr(new SMDS_Mesh0DElement_MyNodeIterator(myNode))));
  }
}

/*!
 * \brief Return node by its index
 * \param ind - node index
 * \retval const SMDS_MeshNode* - the node
 */
const SMDS_MeshNode* SMDS_Mesh0DElement::GetNode(const int ind) const
{
  if (ind == 0)
    return myNode;
  return NULL;
}

//=======================================================================
//function : ChangeNode
//purpose  :
//=======================================================================
bool SMDS_Mesh0DElement::ChangeNodes(const SMDS_MeshNode* nodes[], const int nbNodes)
{
  if ( nbNodes == 1 )
  {
    vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
#ifdef VTK_CELL_ARRAY_V2
    vtkNew<vtkIdList> cellPoints;
    grid->GetCellPoints(myVtkID, cellPoints.GetPointer());
    if (nbNodes != cellPoints->GetNumberOfIds())
    {
      MESSAGE("ChangeNodes problem: not the same number of nodes " << cellPoints->GetNumberOfIds() << " -> " << nbNodes);
      return false;
    }
    myNode = nodes[0];
    cellPoints->SetId(0, myNode->getVtkId());
#else
    vtkIdType npts = 0;
    vtkIdType* pts = 0;
    grid->GetCellPoints(myVtkID, npts, pts);
    if (nbNodes != npts)
    {
      MESSAGE("ChangeNodes problem: not the same number of nodes " << npts << " -> " << nbNodes);
      return false;
    }
    myNode = nodes[0];
    pts[0] = myNode->getVtkId();
#endif

    SMDS_Mesh::_meshList[myMeshId]->setMyModified();
    return true;
  }
  return false;
}
