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
//  File   : SMDS_BallElement.hxx
//  Module : SMESH
//
#ifndef _SMDS_BallElement_HeaderFile
#define _SMDS_BallElement_HeaderFile

#include "SMESH_SMDS.hxx"
#include "SMDS_MeshCell.hxx"

#include <iostream>

class SMDS_EXPORT SMDS_BallElement: public SMDS_MeshCell
{
 public:
  SMDS_BallElement();
  SMDS_BallElement (const SMDS_MeshNode * node, double diameter);
  SMDS_BallElement(vtkIdType nodeId, double diameter, SMDS_Mesh* mesh);
  void init(vtkIdType nodeId, double diameter, SMDS_Mesh* mesh);
  double GetDiameter() const;
  void SetDiameter(double diameter);
  bool ChangeNode (const SMDS_MeshNode * node);

  virtual bool ChangeNodes(const SMDS_MeshNode* nodes[],
                           const int      /*nbNodes*/) { return ChangeNode( nodes[0] ); }
  virtual void Print (std::ostream & OS) const;

  virtual SMDSAbs_ElementType  GetType()       const { return SMDSAbs_Ball; }
  virtual vtkIdType            GetVtkType()    const { return VTK_POLY_VERTEX; }
  virtual SMDSAbs_EntityType   GetEntityType() const { return SMDSEntity_Ball; }
  virtual SMDSAbs_GeometryType GetGeomType()   const { return SMDSGeom_BALL; }
  virtual int NbNodes() const { return 1; }
  virtual int NbEdges() const { return 0; }
  virtual int NbFaces() const { return 0; }
  virtual const SMDS_MeshNode* GetNode (const int ind) const;

 protected:
  SMDS_ElemIteratorPtr elementsIterator (SMDSAbs_ElementType type) const;
};

#endif
