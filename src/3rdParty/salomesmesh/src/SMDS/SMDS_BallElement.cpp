// Copyright (C) 2010-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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
//  Module     : SMESH
//  File       : SMDS_BallElement.cxx
//  Author     : Edward AGAPOV (eap)

#include "SMDS_BallElement.hxx"

#include "SMDS_ElemIterator.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_VtkCellIterator.hxx"

SMDS_BallElement::SMDS_BallElement()
{
  SMDS_MeshCell::init();
}

SMDS_BallElement::SMDS_BallElement (const SMDS_MeshNode * node, double diameter)
{
  init( node->getVtkId(), diameter, SMDS_Mesh::_meshList[ node->getMeshId() ] );
}

SMDS_BallElement::SMDS_BallElement(vtkIdType nodeId, double diameter, SMDS_Mesh* mesh)
{
  init( nodeId, diameter, mesh );
}

void SMDS_BallElement::init(vtkIdType nodeId, double diameter, SMDS_Mesh* mesh)
{
  SMDS_MeshCell::init();
  SMDS_UnstructuredGrid* grid = mesh->getGrid();
  myVtkID = grid->InsertNextLinkedCell( GetVtkType(), 1, &nodeId );
  myMeshId = mesh->getMeshId();
  grid->SetBallDiameter( myVtkID, diameter );
  mesh->setMyModified();
}

double SMDS_BallElement::GetDiameter() const
{
  return SMDS_Mesh::_meshList[myMeshId]->getGrid()->GetBallDiameter( myVtkID );
}

void SMDS_BallElement::SetDiameter(double diameter)
{
  SMDS_Mesh::_meshList[myMeshId]->getGrid()->SetBallDiameter( myVtkID, diameter );
}

bool SMDS_BallElement::ChangeNode (const SMDS_MeshNode * node)
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
#ifdef VTK_CELL_ARRAY_V2
  vtkNew<vtkIdList> cellPoints;
  grid->GetCellPoints(myVtkID, cellPoints.GetPointer());
  cellPoints->SetId(0, node->getVtkId());
#else
  vtkIdType npts = 0;
  vtkIdType* pts = 0;
  grid->GetCellPoints(myVtkID, npts, pts);
  pts[0] = node->getVtkId();
#endif
  SMDS_Mesh::_meshList[myMeshId]->setMyModified();
  return true;
}

void SMDS_BallElement::Print (std::ostream & OS) const
{
  OS << "ball<" << GetID() << "> : ";
}

const SMDS_MeshNode* SMDS_BallElement::GetNode (const int ind) const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType npts;
  vtkIdTypePtr pts;
  grid->GetCellPoints( myVtkID, npts, pts );
  return SMDS_Mesh::_meshList[myMeshId]->FindNodeVtk( pts[ 0 ]);
}

SMDS_ElemIteratorPtr SMDS_BallElement::elementsIterator (SMDSAbs_ElementType type) const
{
  switch (type)
  {
    case SMDSAbs_Node:
      return SMDS_ElemIteratorPtr(new SMDS_VtkCellIterator(SMDS_Mesh::_meshList[myMeshId], myVtkID, GetEntityType()));
    default:
      ;
      return SMDS_ElemIteratorPtr((SMDS_ElemIterator*) NULL);
  }
}

