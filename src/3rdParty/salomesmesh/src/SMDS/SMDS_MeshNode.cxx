// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SMESH SMDS : implementaion of Salome mesh data structure
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_MeshNode.hxx"
#include "SMDS_SpacePosition.hxx"
#include "SMDS_IteratorOfElements.hxx"
#include "SMDS_Mesh.hxx"
#include <vtkUnstructuredGrid.h>

#include "utilities.h"
#include "Utils_SALOME_Exception.hxx"
#include <cassert>

using namespace std;

int SMDS_MeshNode::nbNodes =0;

//=======================================================================
//function : SMDS_MeshNode
//purpose  :
//=======================================================================
SMDS_MeshNode::SMDS_MeshNode() :
  SMDS_MeshElement(-1, -1, 0),
  myPosition(SMDS_SpacePosition::originSpacePosition())
{
  nbNodes++;
}

SMDS_MeshNode::SMDS_MeshNode(int id, int meshId, int shapeId, double x, double y, double z):
  SMDS_MeshElement(id, meshId, shapeId),
  myPosition(SMDS_SpacePosition::originSpacePosition())
{
  nbNodes++;
  init(id, meshId, shapeId, x, y ,z);
}

void SMDS_MeshNode::init(int id, int meshId, int shapeId, double x, double y, double z)
{
  SMDS_MeshElement::init(id, meshId, shapeId);
  myVtkID = id - 1;
  assert(myVtkID >= 0);
  SMDS_UnstructuredGrid * grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  vtkPoints *points = grid->GetPoints();
  points->InsertPoint(myVtkID, x, y, z);
  if ( grid->HasLinks() )
    grid->GetLinks()->ResizeForPoint( myVtkID );
}

SMDS_MeshNode::~SMDS_MeshNode()
{
  nbNodes--;
  if ( myPosition && myPosition != SMDS_SpacePosition::originSpacePosition() )
    delete myPosition, myPosition = 0;
}

//=======================================================================
//function : RemoveInverseElement
//purpose  :
//=======================================================================

void SMDS_MeshNode::RemoveInverseElement(const SMDS_MeshElement * elem)
{
  if ( SMDS_Mesh::_meshList[myMeshId]->getGrid()->HasLinks() )
    SMDS_Mesh::_meshList[myMeshId]->getGrid()->RemoveReferenceToCell(myVtkID, elem->getVtkId());
}

//=======================================================================
//function : Print
//purpose  :
//=======================================================================

void SMDS_MeshNode::Print(ostream & OS) const
{
  OS << "Node <" << myID << "> : X = " << X() << " Y = "
     << Y() << " Z = " << Z() << endl;
}

//=======================================================================
//function : SetPosition
//purpose  :
//=======================================================================

void SMDS_MeshNode::SetPosition(const SMDS_PositionPtr& aPos)
{
  if ( myPosition &&
       myPosition != SMDS_SpacePosition::originSpacePosition() &&
       myPosition != aPos )
    delete myPosition;
  myPosition = aPos;
}

//=======================================================================
//function : GetPosition
//purpose  :
//=======================================================================

const SMDS_PositionPtr& SMDS_MeshNode::GetPosition() const
{
  return myPosition;
}

//=======================================================================
/*!
 * \brief Iterator on list of elements
 */
//=======================================================================

class SMDS_MeshNode_MyInvIterator: public SMDS_ElemIterator
{
private:
  SMDS_Mesh* myMesh;
  vtkIdType* myCells;
  int myNcells;
  SMDSAbs_ElementType myType;
  int iter;
  vector<vtkIdType> cellList;

public:
  SMDS_MeshNode_MyInvIterator(SMDS_Mesh *mesh, vtkIdType* cells, int ncells, SMDSAbs_ElementType type) :
    myMesh(mesh), myCells(cells), myNcells(ncells), myType(type), iter(0)
  {
    if ( ncells )
    {
      cellList.reserve( ncells );
      if (type == SMDSAbs_All)
      {
        cellList.assign( cells, cells + ncells );
      }
      else
      {
        for (int i = 0; i < ncells; i++)
        {
          int vtkId = cells[i];
          int smdsId = myMesh->fromVtkToSmds(vtkId);
          const SMDS_MeshElement* elem = myMesh->FindElement(smdsId);
          if (elem->GetType() == type)
          {
            cellList.push_back(vtkId);
          }
        }
      }
      myCells = cellList.empty() ? 0 : &cellList[0];
      myNcells = cellList.size();
    }
  }

  bool more()
  {
    return (iter < myNcells);
  }

  const SMDS_MeshElement* next()
  {
    int vtkId = myCells[iter];
    int smdsId = myMesh->fromVtkToSmds(vtkId);
    const SMDS_MeshElement* elem = myMesh->FindElement(smdsId);
    if (!elem)
    {
      MESSAGE("SMDS_MeshNode_MyInvIterator problem Null element");
      throw SALOME_Exception("SMDS_MeshNode_MyInvIterator problem Null element");
    }
    iter++;
    return elem;
  }
};

SMDS_ElemIteratorPtr SMDS_MeshNode::GetInverseElementIterator(SMDSAbs_ElementType type) const
{
  if ( SMDS_Mesh::_meshList[myMeshId]->NbElements() > 0 ) // avoid building links
  {
    vtkCellLinks::Link& l = SMDS_Mesh::_meshList[myMeshId]->getGrid()->GetLinks()->GetLink(myVtkID);
    return SMDS_ElemIteratorPtr(new SMDS_MeshNode_MyInvIterator(SMDS_Mesh::_meshList[myMeshId], l.cells, l.ncells, type));
  }
  else
  {
    return SMDS_ElemIteratorPtr(new SMDS_MeshNode_MyInvIterator(SMDS_Mesh::_meshList[myMeshId], 0, 0, type));
  }
}

SMDS_ElemIteratorPtr SMDS_MeshNode::elementsIterator(SMDSAbs_ElementType type) const
{
  if ( type == SMDSAbs_Node )
    return SMDS_MeshElement::elementsIterator( SMDSAbs_Node );
  else
    return GetInverseElementIterator( type );
}

int SMDS_MeshNode::NbNodes() const
{
  return 1;
}

double* SMDS_MeshNode::getCoord() const
{
  return SMDS_Mesh::_meshList[myMeshId]->getGrid()->GetPoint(myVtkID);
}

double SMDS_MeshNode::X() const
{
  double *coord = getCoord();
  return coord[0];
}

double SMDS_MeshNode::Y() const
{
  double *coord = getCoord();
  return coord[1];
}

double SMDS_MeshNode::Z() const
{
  double *coord = getCoord();
  return coord[2];
}

//================================================================================
/*!
 * \brief thread safe getting coords
 */
//================================================================================

void SMDS_MeshNode::GetXYZ(double xyz[3]) const
{
  return SMDS_Mesh::_meshList[myMeshId]->getGrid()->GetPoint(myVtkID,xyz);
}

//================================================================================
void SMDS_MeshNode::setXYZ(double x, double y, double z)
{
  SMDS_Mesh *mesh = SMDS_Mesh::_meshList[myMeshId];
  vtkPoints *points = mesh->getGrid()->GetPoints();
  points->InsertPoint(myVtkID, x, y, z);
  mesh->adjustBoundingBox(x, y, z);
  mesh->setMyModified();
}

SMDSAbs_ElementType SMDS_MeshNode::GetType() const
{
  return SMDSAbs_Node;
}

vtkIdType SMDS_MeshNode::GetVtkType() const
{
  return VTK_VERTEX;
}

//=======================================================================
//function : AddInverseElement
//purpose  :
//=======================================================================
void SMDS_MeshNode::AddInverseElement(const SMDS_MeshElement* ME)
{
  SMDS_UnstructuredGrid* grid = SMDS_Mesh::_meshList[myMeshId]->getGrid();
  if ( grid->HasLinks() )
  {
    vtkCellLinks *Links = grid->GetLinks();
    Links->ResizeCellList(myVtkID, 1);
    Links->AddCellReference(ME->getVtkId(), myVtkID);
  }
}

//=======================================================================
//function : ClearInverseElements
//purpose  :
//=======================================================================
void SMDS_MeshNode::ClearInverseElements()
{
  SMDS_Mesh::_meshList[myMeshId]->getGrid()->ResizeCellList(myVtkID, 0);
}

//================================================================================
/*!
 * \brief Count inverse elements of given type
 */
//================================================================================

int SMDS_MeshNode::NbInverseElements(SMDSAbs_ElementType type) const
{
  int nb = 0;
  if ( SMDS_Mesh::_meshList[myMeshId]->NbElements() > 0 ) // avoid building links
  {
    vtkCellLinks::Link& l = SMDS_Mesh::_meshList[myMeshId]->getGrid()->GetLinks()->GetLink(myVtkID);

    if ( type == SMDSAbs_All )
      return l.ncells;

    SMDS_Mesh *mesh = SMDS_Mesh::_meshList[myMeshId];
    for ( int i = 0; i < l.ncells; i++ )
    {
      const SMDS_MeshElement* elem = mesh->FindElement( mesh->fromVtkToSmds( l.cells[i] ));
      nb += ( elem->GetType() == type );
    }
  }
  return nb;
}
