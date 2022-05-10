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
//  File   : SMDS_VtkEdge.hxx
//  Module : SMESH

#ifndef _SMDS_VTKEDGE_HXX_
#define _SMDS_VTKEDGE_HXX_

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshEdge.hxx"
#include <vtkUnstructuredGrid.h>
#include <vector>

class SMDS_EXPORT SMDS_VtkEdge: public SMDS_MeshEdge
{

public:
  SMDS_VtkEdge();
  SMDS_VtkEdge(std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh);
  ~SMDS_VtkEdge();
  void init(std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh);
  bool ChangeNodes(const SMDS_MeshNode * node1, const SMDS_MeshNode * node2);
  virtual bool ChangeNodes(const SMDS_MeshNode* nodes[], const int nbNodes);
  virtual bool IsMediumNode(const SMDS_MeshNode* node) const;

  virtual void Print(std::ostream & OS) const;
  virtual int NbNodes() const;
  virtual int NbEdges() const;

  virtual vtkIdType GetVtkType() const;
  virtual SMDSAbs_EntityType GetEntityType() const;
  virtual const SMDS_MeshNode* GetNode(const int ind) const;
  virtual bool IsQuadratic() const;

  virtual SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType type) const;
  virtual SMDS_NodeIteratorPtr nodesIteratorToUNV() const;
  virtual SMDS_NodeIteratorPtr interlacedNodesIterator() const;
protected:
};
#endif
