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

#ifndef _SMDS_VTKFACE_HXX_
#define _SMDS_VTKFACE_HXX_

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshFace.hxx"
#include <vtkUnstructuredGrid.h>
#include <vector>

class SMDS_EXPORT SMDS_VtkFace: public SMDS_MeshFace
{
public:
  SMDS_VtkFace();
  SMDS_VtkFace(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh);
  ~SMDS_VtkFace();
  void init(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh);
  void initPoly(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh);
  void initQuadPoly(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh);

  bool ChangeNodes(const SMDS_MeshNode* nodes[], const int nbNodes);
  void ChangeApex(SMDS_MeshNode* node); // to use only for tmp triangles

  virtual void Print(std::ostream & OS) const;
  virtual int NbEdges() const;
  virtual int NbFaces() const;
  virtual int NbNodes() const;

  virtual vtkIdType            GetVtkType() const;
  virtual SMDSAbs_EntityType   GetEntityType() const;
  virtual SMDSAbs_GeometryType GetGeomType() const;
  virtual const SMDS_MeshNode* GetNode(const int ind) const;
  virtual int GetNodeIndex( const SMDS_MeshNode* node ) const;

  virtual bool IsQuadratic() const;
  virtual bool IsPoly() const;
  virtual bool IsMediumNode(const SMDS_MeshNode* node) const;
  virtual int  NbCornerNodes() const;

  virtual SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType type) const;
  virtual SMDS_NodeIteratorPtr nodesIteratorToUNV() const;
  virtual SMDS_NodeIteratorPtr interlacedNodesIterator() const;
protected:
};

#endif
