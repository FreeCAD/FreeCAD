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

//  SMESH SMDS : implementaion of Salome mesh data structure
//  File   : SMDS_MeshElementIDFactory.hxx
//  Module : SMESH
//
#ifndef _SMDS_MeshElementIDFactory_HeaderFile
#define _SMDS_MeshElementIDFactory_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshNodeIDFactory.hxx"

#include <vector>

class SMDS_MeshElement;
class SMDS_Mesh;

class SMDS_EXPORT SMDS_MeshElementIDFactory:public SMDS_MeshNodeIDFactory
{
public:
  friend class SMDS_Mesh;
  
  SMDS_MeshElementIDFactory();
  bool BindID(int ID, SMDS_MeshElement * elem);
  int SetInVtkGrid(SMDS_MeshElement * elem);
  SMDS_MeshElement * MeshElement(int ID);
  virtual int GetFreeID();
  virtual void ReleaseID(int ID, int vtkId = -1);
  SMDS_ElemIteratorPtr elementsIterator() const;
  virtual void Clear();

protected:
  void updateMinMax() const;
  void updateMinMax(int id) const
  {
    if (id > myMax) myMax = id;
    if (id < myMin) myMin = id;
  }

};

#endif
