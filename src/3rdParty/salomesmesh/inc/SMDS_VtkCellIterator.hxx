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

#ifndef _SMDS_VTKCELLITERATOR_HXX_
#define _SMDS_VTKCELLITERATOR_HXX_

#include "SMDS_ElemIterator.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDSAbs_ElementType.hxx"

#include <vtkCell.h>
#include <vtkIdList.h>

class SMDS_VtkCellIterator: public SMDS_ElemIterator
{
public:
  SMDS_VtkCellIterator(SMDS_Mesh* mesh, int vtkCellId, SMDSAbs_EntityType aType);
  virtual ~SMDS_VtkCellIterator();
  virtual bool more();
  virtual const SMDS_MeshElement* next();
  inline void exchange(vtkIdType a, vtkIdType b)
  {
    vtkIdType t = _vtkIdList->GetId(a);
    _vtkIdList->SetId(a, _vtkIdList->GetId(b));
    _vtkIdList->SetId(b, t);
  }

protected:
  SMDS_VtkCellIterator() {};

  SMDS_Mesh* _mesh;
  int _cellId;
  int _index;
  int _nbNodes;
  SMDSAbs_EntityType _type;
  vtkIdList* _vtkIdList;
};

class SMDS_VtkCellIteratorToUNV: public SMDS_NodeIterator, protected SMDS_VtkCellIterator
{
public:
  SMDS_VtkCellIteratorToUNV(SMDS_Mesh* mesh, int vtkCellId, SMDSAbs_EntityType aType);
  virtual const SMDS_MeshNode* next();
  virtual bool more();
  virtual ~SMDS_VtkCellIteratorToUNV();
};

class SMDS_VtkCellIteratorPolyH: public SMDS_VtkCellIterator
{
public:
  SMDS_VtkCellIteratorPolyH(SMDS_Mesh* mesh, int vtkCellId, SMDSAbs_EntityType aType);
  virtual ~SMDS_VtkCellIteratorPolyH();
  virtual bool more();
protected:
  int _nbNodesInFaces;
};

#endif
