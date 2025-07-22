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

#include "SMDS_VtkCellIterator.hxx"
#include "utilities.h"

SMDS_VtkCellIterator::SMDS_VtkCellIterator(SMDS_Mesh* mesh, int vtkCellId, SMDSAbs_EntityType aType) :
  _mesh(mesh), _cellId(vtkCellId), _index(0), _type(aType)
{
  vtkUnstructuredGrid* grid = _mesh->getGrid();
  _vtkIdList = vtkIdList::New();
  const std::vector<int>& interlace = SMDS_MeshCell::fromVtkOrder( aType );
  if ( interlace.empty() )
  {
    grid->GetCellPoints(_cellId, _vtkIdList);
    _nbNodes = _vtkIdList->GetNumberOfIds();
  }
  else
  {
    vtkIdType npts;
    vtkIdTypePtr pts;
    grid->GetCellPoints( _cellId, npts, pts );
    _vtkIdList->SetNumberOfIds( _nbNodes = npts );
    for (int i = 0; i < _nbNodes; i++)
      _vtkIdList->SetId(i, pts[interlace[i]]);
  }
}

SMDS_VtkCellIterator::~SMDS_VtkCellIterator()
{
  _vtkIdList->Delete();
}

bool SMDS_VtkCellIterator::more()
{
  return (_index < _nbNodes);
}

const SMDS_MeshElement* SMDS_VtkCellIterator::next()
{
  vtkIdType id = _vtkIdList->GetId(_index++);
  return _mesh->FindNodeVtk(id);
}

SMDS_VtkCellIteratorToUNV::SMDS_VtkCellIteratorToUNV(SMDS_Mesh* mesh, int vtkCellId, SMDSAbs_EntityType aType) :
  SMDS_VtkCellIterator()
{
  _mesh = mesh;
  _cellId = vtkCellId;
  _index = 0;
  _type = aType;
  //MESSAGE("SMDS_VtkCellInterlacedIterator (UNV)" << _type);

  _vtkIdList = vtkIdList::New();
  vtkIdTypePtr pts;
  vtkIdType npts;
  vtkUnstructuredGrid* grid = _mesh->getGrid();
  grid->GetCellPoints((vtkIdType)_cellId, npts, pts);
  _nbNodes = static_cast<int>(npts);
  _vtkIdList->SetNumberOfIds(_nbNodes);
  const int *ids = 0;
  switch (_type)
  {
  case SMDSEntity_Quad_Edge:
  {
    static int id[] = { 0, 2, 1 };
    ids = id;
    break;
  }
  case SMDSEntity_Quad_Triangle:
  case SMDSEntity_BiQuad_Triangle:
  {
    static int id[] = { 0, 3, 1, 4, 2, 5 };
    ids = id;
    _nbNodes = 6;
    break;
  }
  case SMDSEntity_Quad_Quadrangle:
  case SMDSEntity_BiQuad_Quadrangle:
  {
    static int id[] = { 0, 4, 1, 5, 2, 6, 3, 7 };
    ids = id;
    _nbNodes = 8;
    break;
  }
  case SMDSEntity_Quad_Tetra:
  {
    static int id[] = { 0, 4, 1, 5, 2, 6, 7, 8, 9, 3 };
    ids = id;
    break;
  }
  case SMDSEntity_Quad_Pyramid:
  {
    static int id[] = { 0, 5, 1, 6, 2, 7, 3, 8, 9, 10, 11, 12, 4 };
    ids = id;
    break;
  }
  case SMDSEntity_Penta:
  {
    static int id[] = { 0, 2, 1, 3, 5, 4 };
    ids = id;
    break;
  }
  case SMDSEntity_Quad_Penta:
  {
    static int id[] = { 0, 8, 2, 7, 1, 6, 12, 14, 13, 3, 11, 5, 10, 4, 9 };
    ids = id;
    break;
  }
  case SMDSEntity_Quad_Hexa:
  case SMDSEntity_TriQuad_Hexa:
  {
    static int id[] = { 0, 8, 1, 9, 2, 10, 3, 11, 16, 17, 18, 19, 4, 12, 5, 13, 6, 14, 7, 15 };
    ids = id;
    _nbNodes = 20;
    break;
  }
  case SMDSEntity_Polygon:
#ifndef VTK_NO_QUAD_POLY
  case SMDSEntity_Quad_Polygon:
#endif
  case SMDSEntity_Polyhedra:
  case SMDSEntity_Quad_Polyhedra:
  default:
    const std::vector<int>& i = SMDS_MeshCell::interlacedSmdsOrder(aType, _nbNodes);
    if ( !i.empty() )
      ids = & i[0];
  }

  if ( ids )
    for (int i = 0; i < _nbNodes; i++)
      _vtkIdList->SetId(i, pts[ids[i]]);
  else
    for (int i = 0; i < _nbNodes; i++)
      _vtkIdList->SetId(i, pts[i]);
}

bool SMDS_VtkCellIteratorToUNV::more()
{
  return SMDS_VtkCellIterator::more();
}

const SMDS_MeshNode* SMDS_VtkCellIteratorToUNV::next()
{
  return static_cast< const SMDS_MeshNode* >( SMDS_VtkCellIterator::next() );
}

SMDS_VtkCellIteratorToUNV::~SMDS_VtkCellIteratorToUNV()
{
}

SMDS_VtkCellIteratorPolyH::SMDS_VtkCellIteratorPolyH(SMDS_Mesh* mesh, int vtkCellId, SMDSAbs_EntityType aType) :
  SMDS_VtkCellIterator()
{
  _mesh = mesh;
  _cellId = vtkCellId;
  _index = 0;
  _type = aType;
  //MESSAGE("SMDS_VtkCellIteratorPolyH " << _type);
  _vtkIdList = vtkIdList::New();
  vtkUnstructuredGrid* grid = _mesh->getGrid();
  grid->GetCellPoints(_cellId, _vtkIdList);
  _nbNodes = _vtkIdList->GetNumberOfIds();
  switch (_type)
  {
  case SMDSEntity_Polyhedra:
  {
    //MESSAGE("SMDS_VtkCellIterator Polyhedra");
    vtkIdType nFaces = 0;
    vtkIdTypePtr ptIds = 0;
    grid->GetFaceStream(_cellId, nFaces, ptIds);
    int id = 0;
    _nbNodesInFaces = 0;
    for (int i = 0; i < nFaces; i++)
    {
      int nodesInFace = ptIds[id]; // nodeIds in ptIds[id+1 .. id+nodesInFace]
      _nbNodesInFaces += nodesInFace;
      id += (nodesInFace + 1);
    }
    _vtkIdList->SetNumberOfIds(_nbNodesInFaces);
    id = 0;
    int n = 0;
    for (int i = 0; i < nFaces; i++)
    {
      int nodesInFace = ptIds[id]; // nodeIds in ptIds[id+1 .. id+nodesInFace]
      for (int k = 1; k <= nodesInFace; k++)
        _vtkIdList->SetId(n++, ptIds[id + k]);
      id += (nodesInFace + 1);
    }
    break;
  }
  default:
    assert(0);
  }
}

SMDS_VtkCellIteratorPolyH::~SMDS_VtkCellIteratorPolyH()
{
}

bool SMDS_VtkCellIteratorPolyH::more()
{
  return (_index < _nbNodesInFaces);
}
