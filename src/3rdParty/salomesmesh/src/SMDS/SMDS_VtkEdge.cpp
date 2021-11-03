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

#include "SMDS_VtkEdge.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDS_VtkCellIterator.hxx"

#include "utilities.h"

#include <vector>
#include <cassert>

using namespace std;

SMDS_VtkEdge::SMDS_VtkEdge()
{
}

SMDS_VtkEdge::SMDS_VtkEdge(std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh)
{
  init(nodeIds, mesh);
}

SMDS_VtkEdge::~SMDS_VtkEdge()
{
}

void SMDS_VtkEdge::init(std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh)
{
  SMDS_MeshEdge::init();
  vtkUnstructuredGrid* grid = mesh->getGrid();
  myMeshId = mesh->getMeshId();
  vtkIdType aType = VTK_LINE;
  if (nodeIds.size() == 3)
    aType = VTK_QUADRATIC_EDGE;

  myVtkID = grid->InsertNextLinkedCell(aType, nodeIds.size(), &nodeIds[0]);

  mesh->setMyModified();
  //MESSAGE("SMDS_VtkEdge::init myVtkID " << myVtkID);
}

bool SMDS_VtkEdge::ChangeNodes(const SMDS_MeshNode * node1, const SMDS_MeshNode * node2)
{
  const SMDS_MeshNode* nodes[] = { node1, node2 };
  SMDS_Mesh::_meshList[myMeshId]->setMyModified();
  return ChangeNodes(nodes, 2);
}

bool SMDS_VtkEdge::ChangeNodes(const SMDS_MeshNode* nodes[], const int nbNodes)
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
  for (int i = 0; i < nbNodes; i++)
    {
      cellPoints->SetId(i, nodes[i]->getVtkId());
    }
#else
  vtkIdType npts = 0;
  vtkIdType* pts = 0;
  grid->GetCellPoints(myVtkID, npts, pts);
  if (nbNodes != npts)
    {
      MESSAGE("ChangeNodes problem: not the same number of nodes " << npts << " -> " << nbNodes);
      return false;
    }
  for (int i = 0; i < nbNodes; i++)
    {
      pts[i] = nodes[i]->getVtkId();
    }
#endif
  SMDS_Mesh::_meshList[myMeshId]->setMyModified();
  return true;
}

bool SMDS_VtkEdge::IsMediumNode(const SMDS_MeshNode* node) const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType npts = 0;
  vtkIdTypePtr pts = 0;
  grid->GetCellPoints(myVtkID, npts, pts);
  //MESSAGE("IsMediumNode " << npts  << " " << (node->getVtkId() == pts[npts-1]));
  return ((npts == 3) && (node->getVtkId() == pts[2]));
}

void SMDS_VtkEdge::Print(std::ostream & OS) const
{
  OS << "edge <" << GetID() << "> : ";
}

int SMDS_VtkEdge::NbNodes() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  int nbPoints = grid->GetCell(myVtkID)->GetNumberOfPoints();
  assert(nbPoints >= 2);
  return nbPoints;
}

int SMDS_VtkEdge::NbEdges() const
{
  return 1;
}

SMDSAbs_EntityType SMDS_VtkEdge::GetEntityType() const
{
  if (NbNodes() == 2)
    return SMDSEntity_Edge;
  else
    return SMDSEntity_Quad_Edge;
}

vtkIdType SMDS_VtkEdge::GetVtkType() const
{
  if (NbNodes() == 2)
    return VTK_LINE;
  else
    return VTK_QUADRATIC_EDGE;

}

/*!
 * \brief Return node by its index
 * \param ind - node index
 * \retval const SMDS_MeshNode* - the node
 */
const SMDS_MeshNode*
SMDS_VtkEdge::GetNode(const int ind) const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType npts;
  vtkIdTypePtr pts;
  grid->GetCellPoints( this->myVtkID, npts, pts );
  return SMDS_Mesh::_meshList[myMeshId]->FindNodeVtk( pts[ ind ]);
}

bool SMDS_VtkEdge::IsQuadratic() const
{
  if (this->NbNodes() > 2)
    return true;
  else
    return false;
}

SMDS_ElemIteratorPtr SMDS_VtkEdge::elementsIterator(SMDSAbs_ElementType type) const
{
  switch (type)
  {
    case SMDSAbs_Node:
      return SMDS_ElemIteratorPtr(new SMDS_VtkCellIterator(SMDS_Mesh::_meshList[myMeshId], myVtkID, GetEntityType()));
    default:
      MESSAGE("ERROR : Iterator not implemented");
      return SMDS_ElemIteratorPtr((SMDS_ElemIterator*) NULL);
  }
}

SMDS_NodeIteratorPtr SMDS_VtkEdge::nodesIteratorToUNV() const
{
  return SMDS_NodeIteratorPtr(new SMDS_VtkCellIteratorToUNV(SMDS_Mesh::_meshList[myMeshId], myVtkID, GetEntityType()));
}

SMDS_NodeIteratorPtr SMDS_VtkEdge::interlacedNodesIterator() const
{
  return nodesIteratorToUNV();
}
