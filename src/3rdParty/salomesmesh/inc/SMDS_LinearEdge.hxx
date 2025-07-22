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
//  File   : SMDS_LinearEdge.hxx
//  Module : SMESH
//
#ifndef _SMDS_LinearEdge_HeaderFile
#define _SMDS_LinearEdge_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshEdge.hxx"
#include <iostream>

class SMDS_EXPORT SMDS_LinearEdge: public SMDS_MeshEdge
{

public:
  SMDS_LinearEdge(const SMDS_MeshNode * node1, const SMDS_MeshNode * node2);
  bool ChangeNodes(const SMDS_MeshNode * node1, const SMDS_MeshNode * node2);
  void Print(std::ostream & OS) const;

  virtual SMDSAbs_EntityType GetEntityType() const
  {
    return SMDSEntity_Edge;
  }
  virtual bool ChangeNodes(const SMDS_MeshNode* /*nodes*/[], const int /*nbNodes*/)
  {
    return false;
  }
  int NbNodes() const;
  int NbEdges() const;
  friend bool operator<(const SMDS_LinearEdge& e1, const SMDS_LinearEdge& e2);

  /*!
   * \brief Return node by its index
   * \param ind - node index
   * \retval const SMDS_MeshNode* - the node
   */
  virtual const SMDS_MeshNode* GetNode(const int ind) const;

protected:
  SMDS_ElemIteratorPtr
  elementsIterator(SMDSAbs_ElementType type) const;

protected:
  const SMDS_MeshNode* myNodes[3];

};
#endif
