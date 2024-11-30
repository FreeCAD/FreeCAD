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

//  SMESH SMDS : implementation of Salome mesh data structure
//  File   : SMDS_QuadraticEdge.hxx
//  Module : SMESH
//
#ifndef _SMDS_QuadraticEdge_HeaderFile
#define _SMDS_QuadraticEdge_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_LinearEdge.hxx"
#include <iostream>

class SMDS_EXPORT SMDS_QuadraticEdge: public SMDS_LinearEdge
{

public:
  SMDS_QuadraticEdge(const SMDS_MeshNode * node1,
                     const SMDS_MeshNode * node2,
                     const SMDS_MeshNode * node12);

  bool ChangeNodes(const SMDS_MeshNode * node1,
                   const SMDS_MeshNode * node2,
                   const SMDS_MeshNode * node12);

  void Print(std::ostream & OS) const;

  int NbNodes() const;

  virtual SMDSAbs_EntityType GetEntityType() const { return SMDSEntity_Quad_Edge; }

  virtual bool IsQuadratic() const { return true; }

  virtual bool IsMediumNode(const SMDS_MeshNode* node) const;

  SMDS_NodeIteratorPtr interlacedNodesIterator() const;

protected:
  SMDS_ElemIteratorPtr
  elementsIterator(SMDSAbs_ElementType type) const;

};
#endif
