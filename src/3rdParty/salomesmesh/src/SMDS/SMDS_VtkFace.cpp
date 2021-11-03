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

#include "SMDS_VtkFace.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDS_VtkCellIterator.hxx"

#include "utilities.h"

#include <vector>

using namespace std;

SMDS_VtkFace::SMDS_VtkFace()
{
}

SMDS_VtkFace::SMDS_VtkFace(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh)
{
  init(nodeIds, mesh);
}

SMDS_VtkFace::~SMDS_VtkFace()
{
}

void SMDS_VtkFace::init(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh)
{
  SMDS_MeshFace::init();
  vtkUnstructuredGrid* grid = mesh->getGrid();
  myMeshId = mesh->getMeshId();
  vtkIdType aType = VTK_TRIANGLE;
  switch (nodeIds.size())
  {
    case 3:
      aType = VTK_TRIANGLE;
      break;
    case 4:
      aType = VTK_QUAD;
      break;
    case 6:
      aType = VTK_QUADRATIC_TRIANGLE;
      break;
    case 8:
      aType = VTK_QUADRATIC_QUAD;
      break;
    case 9:
      aType = VTK_BIQUADRATIC_QUAD;
      break;
    case 7:
      aType = VTK_BIQUADRATIC_TRIANGLE;
      break;
    default:
      aType = VTK_POLYGON;
      break;
  }
  myVtkID = grid->InsertNextLinkedCell(aType, nodeIds.size(), (vtkIdType*) &nodeIds[0]);
  mesh->setMyModified();
  //MESSAGE("SMDS_VtkFace::init myVtkID " << myVtkID);
}

void SMDS_VtkFace::initPoly(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh)
{
  SMDS_MeshFace::init();
  vtkUnstructuredGrid* grid = mesh->getGrid();
  myMeshId = mesh->getMeshId();
  myVtkID = grid->InsertNextLinkedCell(VTK_POLYGON, nodeIds.size(), (vtkIdType*) &nodeIds[0]);
  mesh->setMyModified();
}

void SMDS_VtkFace::initQuadPoly(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh)
{
#ifndef VTK_NO_QUAD_POLY    
  SMDS_MeshFace::init();
  vtkUnstructuredGrid* grid = mesh->getGrid();
  myMeshId = mesh->getMeshId();
  myVtkID = grid->InsertNextLinkedCell(VTK_QUADRATIC_POLYGON, nodeIds.size(), (vtkIdType*) &nodeIds[0]);
  mesh->setMyModified();
#else
  throw SALOME_Exception("Quadratic polygon not supported with VTK <6.2");
#endif
}

bool SMDS_VtkFace::ChangeNodes(const SMDS_MeshNode* nodes[], const int nbNodes)
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

void SMDS_VtkFace::Print(std::ostream & OS) const
{
  OS << "face <" << GetID() << "> : ";
}

int SMDS_VtkFace::NbEdges() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType aVtkType = grid->GetCellType(this->myVtkID);
  int nbEdges = 3;
  switch (aVtkType)
  {
  case VTK_TRIANGLE:
  case VTK_QUADRATIC_TRIANGLE:
  case VTK_BIQUADRATIC_TRIANGLE:
    nbEdges = 3;
    break;
  case VTK_QUAD:
  case VTK_QUADRATIC_QUAD:
  case VTK_BIQUADRATIC_QUAD:
    nbEdges = 4;
    break;
#ifndef VTK_NO_QUAD_POLY
  case VTK_QUADRATIC_POLYGON:
    nbEdges = grid->GetCell(myVtkID)->GetNumberOfPoints() / 2;
    break;
#endif
  case VTK_POLYGON:
  default:
    nbEdges = grid->GetCell(myVtkID)->GetNumberOfPoints();
    break;
  }
  return nbEdges;
}

int SMDS_VtkFace::NbFaces() const
{
  return 1;
}

int SMDS_VtkFace::NbNodes() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  int nbPoints = grid->GetCell(myVtkID)->GetNumberOfPoints();
  return nbPoints;
}

/*!
 * \brief Return node by its index
 * \param ind - node index
 * \retval const SMDS_MeshNode* - the node
 */
const SMDS_MeshNode*
SMDS_VtkFace::GetNode(const int ind) const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType npts;
  vtkIdTypePtr pts;
  grid->GetCellPoints( this->myVtkID, npts, pts );
  return SMDS_Mesh::_meshList[myMeshId]->FindNodeVtk( pts[ ind ]);
}

/*!
 * \brief Check if a node belongs to the element
 * \param node - the node to check
 * \retval int - node index within the element, -1 if not found
 */
int SMDS_VtkFace::GetNodeIndex( const SMDS_MeshNode* node ) const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType npts;
  vtkIdTypePtr pts;
  grid->GetCellPoints( this->myVtkID, npts, pts );
  for ( vtkIdType i = 0; i < npts; ++i )
    if ( pts[i] == node->getVtkId() )
      return i;
  return -1;
}

bool SMDS_VtkFace::IsQuadratic() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType aVtkType = grid->GetCellType(this->myVtkID);
  // TODO quadratic polygons ?
  switch (aVtkType)
  {
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUADRATIC_QUAD:
#ifndef VTK_NO_QUAD_POLY
    case VTK_QUADRATIC_POLYGON:
#endif
    case VTK_BIQUADRATIC_QUAD:
    case VTK_BIQUADRATIC_TRIANGLE:
      return true;
      break;
    default:
      return false;
  }
}

bool SMDS_VtkFace::IsPoly() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType aVtkType = grid->GetCellType(this->myVtkID);
  bool isPoly = aVtkType == VTK_POLYGON;
#ifndef VTK_NO_QUAD_POLY
  isPoly = isPoly || aVtkType == VTK_QUADRATIC_POLYGON;
#endif
  return isPoly;
}

bool SMDS_VtkFace::IsMediumNode(const SMDS_MeshNode* node) const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType aVtkType = grid->GetCellType(this->myVtkID);
  int rankFirstMedium = 0;
  switch (aVtkType)
  {
  case VTK_QUADRATIC_TRIANGLE:
  case VTK_BIQUADRATIC_TRIANGLE:
    rankFirstMedium = 3; // medium nodes are of rank 3,4,5
    break;
  case VTK_QUADRATIC_QUAD:
  case VTK_BIQUADRATIC_QUAD:
    rankFirstMedium = 4; // medium nodes are of rank 4,5,6,7
    break;
#ifndef VTK_NO_QUAD_POLY
  case VTK_QUADRATIC_POLYGON:
    rankFirstMedium = grid->GetCell(myVtkID)->GetNumberOfPoints() / 2;
    break;
#endif
  default:
    //MESSAGE("wrong element type " << aVtkType);
    return false;
  }
  vtkIdType npts = 0;
  vtkIdTypePtr pts = 0;
  grid->GetCellPoints(myVtkID, npts, pts);
  vtkIdType nodeId = node->getVtkId();
  for (int rank = 0; rank < npts; rank++)
  {
    if (pts[rank] == nodeId)
    {
      //MESSAGE("rank " << rank << " is medium node " << (rank < rankFirstMedium));
      if (rank < rankFirstMedium)
        return false;
      else
        return true;
    }
  }
  //throw SALOME_Exception(LOCALIZED("node does not belong to this element"));
  MESSAGE("======================================================");
  MESSAGE("= IsMediumNode: node does not belong to this element =");
  MESSAGE("======================================================");
  return false;
}

int SMDS_VtkFace::NbCornerNodes() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  int       nbPoints = grid->GetCell(myVtkID)->GetNumberOfPoints();
  vtkIdType aVtkType = grid->GetCellType(myVtkID);
  switch ( aVtkType )
  {
  case VTK_POLYGON:
    break;
#ifndef VTK_NO_QUAD_POLY
  case VTK_QUADRATIC_POLYGON:
    nbPoints /= 2;
    break;
#endif
  default:
    if ( nbPoints > 4 )
      nbPoints /= 2;
  }
  return nbPoints;
}

SMDSAbs_EntityType SMDS_VtkFace::GetEntityType() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType aVtkType = grid->GetCellType(this->myVtkID);
  return SMDS_MeshCell::toSmdsType( VTKCellType( aVtkType ));
}

SMDSAbs_GeometryType SMDS_VtkFace::GetGeomType() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType aVtkType = grid->GetCellType(this->myVtkID);
  switch ( aVtkType ) {
  case VTK_TRIANGLE:
  case VTK_QUADRATIC_TRIANGLE:
  case VTK_BIQUADRATIC_TRIANGLE: return SMDSGeom_TRIANGLE;

  case VTK_QUAD:
  case VTK_QUADRATIC_QUAD:
  case VTK_BIQUADRATIC_QUAD: return SMDSGeom_QUADRANGLE;
 
#ifndef VTK_NO_QUAD_POLY
  case VTK_QUADRATIC_POLYGON:
#endif
  case VTK_POLYGON: return SMDSGeom_POLYGON;
  default:;
  }
  return SMDSGeom_NONE;
}

vtkIdType SMDS_VtkFace::GetVtkType() const
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkIdType aVtkType = grid->GetCellType(this->myVtkID);
  return aVtkType;
}

SMDS_ElemIteratorPtr SMDS_VtkFace::elementsIterator(SMDSAbs_ElementType type) const
{
  switch (type)
  {
    case SMDSAbs_Node:
      return SMDS_ElemIteratorPtr(new SMDS_VtkCellIterator(SMDS_Mesh::_meshList[myMeshId], myVtkID, GetEntityType()));
    default:
      MESSAGE("ERROR : Iterator not implemented")
      ;
      return SMDS_ElemIteratorPtr((SMDS_ElemIterator*) NULL);
  }
}

SMDS_NodeIteratorPtr SMDS_VtkFace::nodesIteratorToUNV() const
{
  return SMDS_NodeIteratorPtr(new SMDS_VtkCellIteratorToUNV(SMDS_Mesh::_meshList[myMeshId], myVtkID, GetEntityType()));
}

SMDS_NodeIteratorPtr SMDS_VtkFace::interlacedNodesIterator() const
{
  return nodesIteratorToUNV();
}

//! change only the first node, used for temporary triangles in quadrangle to triangle adaptor
void SMDS_VtkFace::ChangeApex(SMDS_MeshNode* node)
{
  vtkUnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
#ifdef VTK_CELL_ARRAY_V2
  vtkNew<vtkIdList> cellPoints;
  grid->GetCellPoints(myVtkID, cellPoints.GetPointer());
  grid->RemoveReferenceToCell(cellPoints->GetId(0), myVtkID);
  cellPoints->SetId(0, node->getVtkId());
#else
  vtkIdType npts = 0;
  vtkIdType* pts = 0;
  grid->GetCellPoints(myVtkID, npts, pts);
  grid->RemoveReferenceToCell(pts[0], myVtkID);
  pts[0] = node->getVtkId();
#endif
  node->AddInverseElement(this),
  SMDS_Mesh::_meshList[myMeshId]->setMyModified();
}
