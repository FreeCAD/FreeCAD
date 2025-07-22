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
//  File   : SMDS_PolyhedralVolumeOfNodes.hxx
//  Module : SMESH
//
#ifndef _SMDS_PolyhedralVolumeOfNodes_HeaderFile
#define _SMDS_PolyhedralVolumeOfNodes_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_VolumeOfNodes.hxx"

class SMDS_EXPORT SMDS_PolyhedralVolumeOfNodes:public SMDS_VolumeOfNodes
{
 public:
  SMDS_PolyhedralVolumeOfNodes (std::vector<const SMDS_MeshNode *> nodes,
                                std::vector<int>                   quantities);

  //virtual ~SMDS_PolyhedralVolumeOfNodes();

  virtual SMDSAbs_ElementType GetType() const;  
  virtual SMDSAbs_EntityType   GetEntityType() const { return SMDSEntity_Polyhedra; }
  virtual bool IsPoly() const { return true; };

  bool ChangeNodes (const std::vector<const SMDS_MeshNode *> & nodes,
                    const std::vector<int> &                   quantities);

  virtual int NbNodes() const;
  virtual int NbEdges() const;
  virtual int NbFaces() const;

  int NbFaceNodes (const int face_ind) const;
  // 1 <= face_ind <= NbFaces()

  const SMDS_MeshNode* GetFaceNode (const int face_ind, const int node_ind) const;
  // 1 <= face_ind <= NbFaces()
  // 1 <= node_ind <= NbFaceNodes()

  const std::vector<int> & GetQuanities() const { return myQuantities; }

  virtual void Print (std::ostream & OS) const;

  /*!
   * \brief Return node by its index
   */
  virtual const SMDS_MeshNode* GetNode(const int ind) const;

  /*!
   * \brief Return iterator on unique nodes
   */
  SMDS_ElemIteratorPtr uniqueNodesIterator() const;
  /*!
   * \brief Return nb of unique nodes
   */
  int NbUniqueNodes() const { return myNbNodes; }

protected:
  SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType type) const;

 private:
  // usage disabled
  bool ChangeNodes (const SMDS_MeshNode* nodes[],
                    const int            nbNodes);

 private:
  std::vector<const SMDS_MeshNode *> myNodesByFaces;
  std::vector<int> myQuantities;
};

#endif
