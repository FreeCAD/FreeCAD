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

#ifndef _SMDS_VTKVOLUME_HXX_
#define _SMDS_VTKVOLUME_HXX_

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshVolume.hxx"
#include "SMDS_UnstructuredGrid.hxx"
#include <vector>

class SMDS_EXPORT SMDS_VtkVolume: public SMDS_MeshVolume
{
public:
  SMDS_VtkVolume();
  SMDS_VtkVolume(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh);
  ~SMDS_VtkVolume();
  void init(const std::vector<vtkIdType>& nodeIds, SMDS_Mesh* mesh);
//#ifdef VTK_HAVE_POLYHEDRON
  void initPoly(const std::vector<vtkIdType>& nodeIds,
                const std::vector<int>& nbNodesPerFace, SMDS_Mesh* mesh);
//#endif
  virtual bool ChangeNodes(const SMDS_MeshNode* nodes[], const int nbNodes);
  virtual bool vtkOrder(const SMDS_MeshNode* nodes[], const int nbNodes);

  virtual void Print(std::ostream & OS) const;
  virtual int NbFaces() const;
  virtual int NbNodes() const;
  virtual int NbEdges() const;

  // 1 <= face_ind <= NbFaces()
  int NbFaceNodes (const int face_ind) const;
  // 1 <= face_ind <= NbFaces()
  // 1 <= node_ind <= NbFaceNodes()
  const SMDS_MeshNode* GetFaceNode (const int face_ind, const int node_ind) const;

  virtual SMDSAbs_ElementType  GetType() const;
  virtual vtkIdType            GetVtkType() const;
  virtual SMDSAbs_EntityType   GetEntityType() const;
  virtual SMDSAbs_GeometryType GetGeomType() const;
  virtual const SMDS_MeshNode* GetNode(const int ind) const;
  virtual int GetNodeIndex( const SMDS_MeshNode* node ) const;
  virtual bool IsQuadratic() const;
  virtual bool IsPoly() const;
  virtual bool IsMediumNode(const SMDS_MeshNode* node) const;
  virtual int  NbCornerNodes() const;
  static void gravityCenter(SMDS_UnstructuredGrid* grid,
                            const vtkIdType *nodeIds,
                            int nbNodes,
                            double* result);
  static bool isForward(double* a,double* b,double* c,double* d);
  int NbUniqueNodes() const;
  SMDS_ElemIteratorPtr uniqueNodesIterator() const;
  std::vector<int> GetQuantities() const;

  virtual SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType type) const;
  virtual SMDS_NodeIteratorPtr nodesIteratorToUNV() const;
  virtual SMDS_NodeIteratorPtr interlacedNodesIterator() const;

protected:
};

#endif
