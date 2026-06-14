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
//  File   : SMDS_Mesh0DElement.hxx
//  Module : SMESH
//
#ifndef _SMDS_Mesh0DElement_HeaderFile
#define _SMDS_Mesh0DElement_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshCell.hxx"

#include <iostream>

class SMDS_EXPORT SMDS_Mesh0DElement: public SMDS_MeshCell
{
 public:
  SMDS_Mesh0DElement (const SMDS_MeshNode * node);
  virtual bool ChangeNodes(const SMDS_MeshNode* nodes[], const int nbNodes);
  virtual void Print (std::ostream & OS) const;

  virtual SMDSAbs_ElementType  GetType() const;
  virtual vtkIdType            GetVtkType() const;
  virtual SMDSAbs_EntityType   GetEntityType() const {return SMDSEntity_0D;}
  virtual SMDSAbs_GeometryType GetGeomType() const { return SMDSGeom_POINT; }
  virtual const SMDS_MeshNode* GetNode (const int ind) const;
  virtual int NbNodes() const;
  virtual int NbEdges() const;

 protected:
  virtual SMDS_ElemIteratorPtr elementsIterator (SMDSAbs_ElementType type) const;

 protected:
  const SMDS_MeshNode* myNode;
};

#endif
