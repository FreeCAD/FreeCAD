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
//  File   : SMDS_QuadraticVolumeOfNodes.hxx
//  Module : SMESH
//
#ifndef _SMDS_QuadraticFaceOfNodes_HeaderFile
#define _SMDS_QuadraticFaceOfNodes_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshFace.hxx"

class SMDS_EXPORT SMDS_QuadraticFaceOfNodes:public SMDS_MeshFace
{
public:
  SMDS_QuadraticFaceOfNodes (const SMDS_MeshNode * n1,
                             const SMDS_MeshNode * n2,
                             const SMDS_MeshNode * n3,
                             const SMDS_MeshNode * n12,
                             const SMDS_MeshNode * n23,
                             const SMDS_MeshNode * n31);

  SMDS_QuadraticFaceOfNodes(const SMDS_MeshNode * n1,
                            const SMDS_MeshNode * n2,
                            const SMDS_MeshNode * n3,
                            const SMDS_MeshNode * n4,
                            const SMDS_MeshNode * n12,
                            const SMDS_MeshNode * n23,
                            const SMDS_MeshNode * n34,
                            const SMDS_MeshNode * n41);

  virtual SMDSAbs_EntityType GetEntityType() const;
  virtual bool IsQuadratic() const { return true; }

  virtual bool IsMediumNode(const SMDS_MeshNode* node) const;

  bool ChangeNodes(const SMDS_MeshNode* nodes[],
                   const int            nbNodes);

  virtual int NbNodes() const;
  virtual int NbEdges() const;
  virtual int NbFaces() const;

  virtual void Print (std::ostream & OS) const;

  SMDS_NodeIteratorPtr interlacedNodesIterator() const;

  /*!
   * \brief Return node by its index
    * \param ind - node index
    * \retval const SMDS_MeshNode* - the node
   */
  virtual const SMDS_MeshNode* GetNode(const int ind) const;

protected:
  virtual SMDS_ElemIteratorPtr elementsIterator (SMDSAbs_ElementType type) const;

 private:
  std::vector<const SMDS_MeshNode *> myNodes;
};

#endif
