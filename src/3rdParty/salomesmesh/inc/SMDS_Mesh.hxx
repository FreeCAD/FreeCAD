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
//  File   : SMDS_Mesh.hxx
//  Module : SMESH
//
#ifndef _SMDS_Mesh_HeaderFile
#define _SMDS_Mesh_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMDS_MeshCell.hxx"
#include "SMDS_Mesh0DElement.hxx"
#include "SMDS_MeshEdge.hxx"
#include "SMDS_MeshFace.hxx"
#include "SMDS_MeshVolume.hxx"
#include "SMDS_MeshNodeIDFactory.hxx"
#include "SMDS_MeshElementIDFactory.hxx"
#include "SMDS_MeshInfo.hxx"
#include "SMDS_ElemIterator.hxx"
#include "SMDS_VolumeOfNodes.hxx"
#include "SMDS_VtkEdge.hxx"
#include "SMDS_VtkFace.hxx"
#include "SMDS_VtkVolume.hxx"
#include "ObjectPool.hxx"
#include "SMDS_UnstructuredGrid.hxx"
#include "SMDS_BallElement.hxx"

#include <boost/shared_ptr.hpp>
#include <set>
#include <list>
#include <vector>
#include <vtkSystemIncludes.h>
#include <cassert>

#include "Utils_SALOME_Exception.hxx"

#define MYASSERT(val) if (!(val)) throw SALOME_Exception(LOCALIZED("assertion not verified"));

class SMDS_EXPORT SMDS_Mesh : public SMDS_MeshObject
{
public:
  friend class SMDS_MeshIDFactory;
  friend class SMDS_MeshNodeIDFactory;
  friend class SMDS_MeshElementIDFactory;
  friend class SMDS_MeshVolumeVtkNodes;
  friend class SMDS_MeshNode;

  SMDS_Mesh();
  
  //! to retrieve this SMDS_Mesh instance from its elements (index stored in SMDS_Elements)
  static std::vector<SMDS_Mesh*> _meshList;

  //! actual nodes coordinates, cells definition and reverse connectivity are stored in a vtkUnstructuredGrid
  inline SMDS_UnstructuredGrid* getGrid() {return myGrid; }
  inline int getMeshId() {return myMeshId; }

  virtual SMDS_NodeIteratorPtr   nodesIterator     (bool idInceasingOrder=false) const;
  virtual SMDS_EdgeIteratorPtr   edgesIterator     (bool idInceasingOrder=false) const;
  virtual SMDS_FaceIteratorPtr   facesIterator     (bool idInceasingOrder=false) const;
  virtual SMDS_VolumeIteratorPtr volumesIterator   (bool idInceasingOrder=false) const;

  virtual SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType type=SMDSAbs_All) const;
  virtual SMDS_ElemIteratorPtr elementGeomIterator(SMDSAbs_GeometryType type) const;
  virtual SMDS_ElemIteratorPtr elementEntityIterator(SMDSAbs_EntityType type) const;

  SMDSAbs_ElementType GetElementType( const int id, const bool iselem ) const;

  SMDS_Mesh *AddSubMesh();

  virtual SMDS_MeshNode* AddNodeWithID(double x, double y, double z, int ID);
  virtual SMDS_MeshNode* AddNode      (double x, double y, double z);

  virtual SMDS_Mesh0DElement* Add0DElementWithID(int n,                   int ID);
  virtual SMDS_Mesh0DElement* Add0DElementWithID(const SMDS_MeshNode * n, int ID);
  virtual SMDS_Mesh0DElement* Add0DElement      (const SMDS_MeshNode * n);

  virtual SMDS_BallElement* AddBallWithID(int n,                   double diameter, int ID);
  virtual SMDS_BallElement* AddBallWithID(const SMDS_MeshNode * n, double diameter, int ID);
  virtual SMDS_BallElement* AddBall      (const SMDS_MeshNode * n, double diameter);

  virtual SMDS_MeshEdge* AddEdgeWithID(int n1, int n2, int ID);
  virtual SMDS_MeshEdge* AddEdgeWithID(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       int ID);
  virtual SMDS_MeshEdge* AddEdge(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2);

  // 2d order edge with 3 nodes: n12 - node between n1 and n2
  virtual SMDS_MeshEdge* AddEdgeWithID(int n1, int n2, int n12, int ID);
  virtual SMDS_MeshEdge* AddEdgeWithID(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       const SMDS_MeshNode * n12,
                                       int ID);
  virtual SMDS_MeshEdge* AddEdge(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2,
                                 const SMDS_MeshNode * n12);

  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       const SMDS_MeshNode * n3,
                                       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2,
                                 const SMDS_MeshNode * n3);

  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3, int n4, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       const SMDS_MeshNode * n3,
                                       const SMDS_MeshNode * n4,
                                       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2,
                                 const SMDS_MeshNode * n3,
                                 const SMDS_MeshNode * n4);

  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshEdge * e1,
                                       const SMDS_MeshEdge * e2,
                                       const SMDS_MeshEdge * e3, int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshEdge * e1,
                                 const SMDS_MeshEdge * e2,
                                 const SMDS_MeshEdge * e3);

  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshEdge * e1,
                                       const SMDS_MeshEdge * e2,
                                       const SMDS_MeshEdge * e3,
                                       const SMDS_MeshEdge * e4, int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshEdge * e1,
                                 const SMDS_MeshEdge * e2,
                                 const SMDS_MeshEdge * e3,
                                 const SMDS_MeshEdge * e4);

  // 2d order triangle of 6 nodes
  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3,
                                       int n12,int n23,int n31, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       const SMDS_MeshNode * n3,
                                       const SMDS_MeshNode * n12,
                                       const SMDS_MeshNode * n23,
                                       const SMDS_MeshNode * n31,
                                       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2,
                                 const SMDS_MeshNode * n3,
                                 const SMDS_MeshNode * n12,
                                 const SMDS_MeshNode * n23,
                                 const SMDS_MeshNode * n31);

  // 2d order triangle of 7 nodes
  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3,
                                       int n12,int n23,int n31, int nCenter, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       const SMDS_MeshNode * n3,
                                       const SMDS_MeshNode * n12,
                                       const SMDS_MeshNode * n23,
                                       const SMDS_MeshNode * n31,
                                       const SMDS_MeshNode * nCenter,
                                       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2,
                                 const SMDS_MeshNode * n3,
                                 const SMDS_MeshNode * n12,
                                 const SMDS_MeshNode * n23,
                                 const SMDS_MeshNode * n31,
                                 const SMDS_MeshNode * nCenter);

  // 2d order quadrangle
  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3, int n4,
                                       int n12,int n23,int n34,int n41, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       const SMDS_MeshNode * n3,
                                       const SMDS_MeshNode * n4,
                                       const SMDS_MeshNode * n12,
                                       const SMDS_MeshNode * n23,
                                       const SMDS_MeshNode * n34,
                                       const SMDS_MeshNode * n41,
                                       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2,
                                 const SMDS_MeshNode * n3,
                                 const SMDS_MeshNode * n4,
                                 const SMDS_MeshNode * n12,
                                 const SMDS_MeshNode * n23,
                                 const SMDS_MeshNode * n34,
                                 const SMDS_MeshNode * n41);

  virtual SMDS_MeshFace* AddFaceWithID(int n1, int n2, int n3, int n4,
                                       int n12,int n23,int n34,int n41, int nCenter, int ID);
  virtual SMDS_MeshFace* AddFaceWithID(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       const SMDS_MeshNode * n3,
                                       const SMDS_MeshNode * n4,
                                       const SMDS_MeshNode * n12,
                                       const SMDS_MeshNode * n23,
                                       const SMDS_MeshNode * n34,
                                       const SMDS_MeshNode * n41,
                                       const SMDS_MeshNode * nCenter,
                                       int ID);
  virtual SMDS_MeshFace* AddFace(const SMDS_MeshNode * n1,
                                 const SMDS_MeshNode * n2,
                                 const SMDS_MeshNode * n3,
                                 const SMDS_MeshNode * n4,
                                 const SMDS_MeshNode * n12,
                                 const SMDS_MeshNode * n23,
                                 const SMDS_MeshNode * n34,
                                 const SMDS_MeshNode * n41,
                                 const SMDS_MeshNode * nCenter);

  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4);

  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4,
                                           int n5, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n5,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n5);

  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4,
                                           int n5, int n6, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n5,
                                           const SMDS_MeshNode * n6,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n5,
                                     const SMDS_MeshNode * n6);

  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4,
                                           int n5, int n6, int n7, int n8, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n5,
                                           const SMDS_MeshNode * n6,
                                           const SMDS_MeshNode * n7,
                                           const SMDS_MeshNode * n8,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n5,
                                     const SMDS_MeshNode * n6,
                                     const SMDS_MeshNode * n7,
                                     const SMDS_MeshNode * n8);

  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshFace * f1,
                                           const SMDS_MeshFace * f2,
                                           const SMDS_MeshFace * f3,
                                           const SMDS_MeshFace * f4, int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshFace * f1,
                                     const SMDS_MeshFace * f2,
                                     const SMDS_MeshFace * f3,
                                     const SMDS_MeshFace * f4);

  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshFace * f1,
                                           const SMDS_MeshFace * f2,
                                           const SMDS_MeshFace * f3,
                                           const SMDS_MeshFace * f4,
                                           const SMDS_MeshFace * f5, int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshFace * f1,
                                     const SMDS_MeshFace * f2,
                                     const SMDS_MeshFace * f3,
                                     const SMDS_MeshFace * f4,
                                     const SMDS_MeshFace * f5);

  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshFace * f1,
                                           const SMDS_MeshFace * f2,
                                           const SMDS_MeshFace * f3,
                                           const SMDS_MeshFace * f4,
                                           const SMDS_MeshFace * f5,
                                           const SMDS_MeshFace * f6, int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshFace * f1,
                                     const SMDS_MeshFace * f2,
                                     const SMDS_MeshFace * f3,
                                     const SMDS_MeshFace * f4,
                                     const SMDS_MeshFace * f5,
                                     const SMDS_MeshFace * f6);

  // hexagonal prism
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4, int n5, int n6,
                                           int n7, int n8, int n9, int n10, int n11, int n12,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n5,
                                           const SMDS_MeshNode * n6,
                                           const SMDS_MeshNode * n7,
                                           const SMDS_MeshNode * n8,
                                           const SMDS_MeshNode * n9,
                                           const SMDS_MeshNode * n10,
                                           const SMDS_MeshNode * n11,
                                           const SMDS_MeshNode * n12,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n5,
                                     const SMDS_MeshNode * n6,
                                     const SMDS_MeshNode * n7,
                                     const SMDS_MeshNode * n8,
                                     const SMDS_MeshNode * n9,
                                     const SMDS_MeshNode * n10,
                                     const SMDS_MeshNode * n11,
                                     const SMDS_MeshNode * n12);

  // 2d order tetrahedron of 10 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4,
                                           int n12,int n23,int n31,
                                           int n14,int n24,int n34, int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n12,
                                           const SMDS_MeshNode * n23,
                                           const SMDS_MeshNode * n31,
                                           const SMDS_MeshNode * n14,
                                           const SMDS_MeshNode * n24,
                                           const SMDS_MeshNode * n34,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n12,
                                     const SMDS_MeshNode * n23,
                                     const SMDS_MeshNode * n31,
                                     const SMDS_MeshNode * n14,
                                     const SMDS_MeshNode * n24,
                                     const SMDS_MeshNode * n34);

  // 2d order pyramid of 13 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4, int n5,
                                           int n12,int n23,int n34,int n41,
                                           int n15,int n25,int n35,int n45,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n5,
                                           const SMDS_MeshNode * n12,
                                           const SMDS_MeshNode * n23,
                                           const SMDS_MeshNode * n34,
                                           const SMDS_MeshNode * n41,
                                           const SMDS_MeshNode * n15,
                                           const SMDS_MeshNode * n25,
                                           const SMDS_MeshNode * n35,
                                           const SMDS_MeshNode * n45,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n5,
                                     const SMDS_MeshNode * n12,
                                     const SMDS_MeshNode * n23,
                                     const SMDS_MeshNode * n34,
                                     const SMDS_MeshNode * n41,
                                     const SMDS_MeshNode * n15,
                                     const SMDS_MeshNode * n25,
                                     const SMDS_MeshNode * n35,
                                     const SMDS_MeshNode * n45);

  // 2d order Pentahedron with 15 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3,
                                           int n4, int n5, int n6,
                                           int n12,int n23,int n31,
                                           int n45,int n56,int n64,
                                           int n14,int n25,int n36,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n5,
                                           const SMDS_MeshNode * n6,
                                           const SMDS_MeshNode * n12,
                                           const SMDS_MeshNode * n23,
                                           const SMDS_MeshNode * n31,
                                           const SMDS_MeshNode * n45,
                                           const SMDS_MeshNode * n56,
                                           const SMDS_MeshNode * n64,
                                           const SMDS_MeshNode * n14,
                                           const SMDS_MeshNode * n25,
                                           const SMDS_MeshNode * n36,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n5,
                                     const SMDS_MeshNode * n6,
                                     const SMDS_MeshNode * n12,
                                     const SMDS_MeshNode * n23,
                                     const SMDS_MeshNode * n31,
                                     const SMDS_MeshNode * n45,
                                     const SMDS_MeshNode * n56,
                                     const SMDS_MeshNode * n64,
                                     const SMDS_MeshNode * n14,
                                     const SMDS_MeshNode * n25,
                                     const SMDS_MeshNode * n36);

  // 2d oreder Hexahedrons with 20 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4,
                                           int n5, int n6, int n7, int n8,
                                           int n12,int n23,int n34,int n41,
                                           int n56,int n67,int n78,int n85,
                                           int n15,int n26,int n37,int n48,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n5,
                                           const SMDS_MeshNode * n6,
                                           const SMDS_MeshNode * n7,
                                           const SMDS_MeshNode * n8,
                                           const SMDS_MeshNode * n12,
                                           const SMDS_MeshNode * n23,
                                           const SMDS_MeshNode * n34,
                                           const SMDS_MeshNode * n41,
                                           const SMDS_MeshNode * n56,
                                           const SMDS_MeshNode * n67,
                                           const SMDS_MeshNode * n78,
                                           const SMDS_MeshNode * n85,
                                           const SMDS_MeshNode * n15,
                                           const SMDS_MeshNode * n26,
                                           const SMDS_MeshNode * n37,
                                           const SMDS_MeshNode * n48,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n5,
                                     const SMDS_MeshNode * n6,
                                     const SMDS_MeshNode * n7,
                                     const SMDS_MeshNode * n8,
                                     const SMDS_MeshNode * n12,
                                     const SMDS_MeshNode * n23,
                                     const SMDS_MeshNode * n34,
                                     const SMDS_MeshNode * n41,
                                     const SMDS_MeshNode * n56,
                                     const SMDS_MeshNode * n67,
                                     const SMDS_MeshNode * n78,
                                     const SMDS_MeshNode * n85,
                                     const SMDS_MeshNode * n15,
                                     const SMDS_MeshNode * n26,
                                     const SMDS_MeshNode * n37,
                                     const SMDS_MeshNode * n48);

  // 2d oreder Hexahedrons with 27 nodes
  virtual SMDS_MeshVolume* AddVolumeWithID(int n1, int n2, int n3, int n4,
                                           int n5, int n6, int n7, int n8,
                                           int n12,int n23,int n34,int n41,
                                           int n56,int n67,int n78,int n85,
                                           int n15,int n26,int n37,int n48,
                                           int n1234,int n1256,int n2367,int n3478,
                                           int n1458,int n5678,int nCenter,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolumeWithID(const SMDS_MeshNode * n1,
                                           const SMDS_MeshNode * n2,
                                           const SMDS_MeshNode * n3,
                                           const SMDS_MeshNode * n4,
                                           const SMDS_MeshNode * n5,
                                           const SMDS_MeshNode * n6,
                                           const SMDS_MeshNode * n7,
                                           const SMDS_MeshNode * n8,
                                           const SMDS_MeshNode * n12,
                                           const SMDS_MeshNode * n23,
                                           const SMDS_MeshNode * n34,
                                           const SMDS_MeshNode * n41,
                                           const SMDS_MeshNode * n56,
                                           const SMDS_MeshNode * n67,
                                           const SMDS_MeshNode * n78,
                                           const SMDS_MeshNode * n85,
                                           const SMDS_MeshNode * n15,
                                           const SMDS_MeshNode * n26,
                                           const SMDS_MeshNode * n37,
                                           const SMDS_MeshNode * n48,
                                           const SMDS_MeshNode * n1234,
                                           const SMDS_MeshNode * n1256,
                                           const SMDS_MeshNode * n2367,
                                           const SMDS_MeshNode * n3478,
                                           const SMDS_MeshNode * n1458,
                                           const SMDS_MeshNode * n5678,
                                           const SMDS_MeshNode * nCenter,
                                           int ID);
  virtual SMDS_MeshVolume* AddVolume(const SMDS_MeshNode * n1,
                                     const SMDS_MeshNode * n2,
                                     const SMDS_MeshNode * n3,
                                     const SMDS_MeshNode * n4,
                                     const SMDS_MeshNode * n5,
                                     const SMDS_MeshNode * n6,
                                     const SMDS_MeshNode * n7,
                                     const SMDS_MeshNode * n8,
                                     const SMDS_MeshNode * n12,
                                     const SMDS_MeshNode * n23,
                                     const SMDS_MeshNode * n34,
                                     const SMDS_MeshNode * n41,
                                     const SMDS_MeshNode * n56,
                                     const SMDS_MeshNode * n67,
                                     const SMDS_MeshNode * n78,
                                     const SMDS_MeshNode * n85,
                                     const SMDS_MeshNode * n15,
                                     const SMDS_MeshNode * n26,
                                     const SMDS_MeshNode * n37,
                                     const SMDS_MeshNode * n48,
                                     const SMDS_MeshNode * n1234,
                                     const SMDS_MeshNode * n1256,
                                     const SMDS_MeshNode * n2367,
                                     const SMDS_MeshNode * n3478,
                                     const SMDS_MeshNode * n1458,
                                     const SMDS_MeshNode * n5678,
                                     const SMDS_MeshNode * nCenter);

  virtual SMDS_MeshFace* AddPolygonalFaceWithID (const std::vector<int> & nodes_ids,
                                                 const int                ID);

  virtual SMDS_MeshFace* AddPolygonalFaceWithID (const std::vector<const SMDS_MeshNode*> & nodes,
                                                 const int                                 ID);

  virtual SMDS_MeshFace* AddPolygonalFace (const std::vector<const SMDS_MeshNode*> & nodes);

  virtual SMDS_MeshFace* AddQuadPolygonalFaceWithID(const std::vector<int> & nodes_ids,
                                                    const int                ID);

  virtual SMDS_MeshFace* AddQuadPolygonalFaceWithID(const std::vector<const SMDS_MeshNode*> & nodes,
                                                    const int                                 ID);

  virtual SMDS_MeshFace* AddQuadPolygonalFace(const std::vector<const SMDS_MeshNode*> & nodes);

  virtual SMDS_MeshVolume* AddPolyhedralVolumeWithID
    (const std::vector<int> & nodes_ids,
     const std::vector<int> & quantities,
     const int                ID);

  virtual SMDS_MeshVolume* AddPolyhedralVolumeWithID
    (const std::vector<const SMDS_MeshNode*> & nodes,
     const std::vector<int>                  & quantities,
                            const int                                 ID);

  virtual SMDS_MeshVolume* AddPolyhedralVolume
                           (const std::vector<const SMDS_MeshNode*> & nodes,
                            const std::vector<int>                  & quantities);

  virtual SMDS_MeshVolume* AddVolumeFromVtkIds(const std::vector<vtkIdType>& vtkNodeIds);

  virtual SMDS_MeshVolume* AddVolumeFromVtkIdsWithID(const std::vector<vtkIdType>& vtkNodeIds,
                                                     const int ID);

  virtual SMDS_MeshFace* AddFaceFromVtkIds(const std::vector<vtkIdType>& vtkNodeIds);

  virtual SMDS_MeshFace* AddFaceFromVtkIdsWithID(const std::vector<vtkIdType>& vtkNodeIds,
                                                     const int ID);
  virtual void MoveNode(const SMDS_MeshNode *n, double x, double y, double z);

  virtual void RemoveElement(const SMDS_MeshElement *        elem,
                             std::list<const SMDS_MeshElement *>& removedElems,
                             std::list<const SMDS_MeshElement *>& removedNodes,
                             const bool                      removenodes = false);
  virtual void RemoveElement(const SMDS_MeshElement * elem, bool removenodes = false);
  virtual void RemoveNode(const SMDS_MeshNode * node);
  virtual void Remove0DElement(const SMDS_Mesh0DElement * elem0d);
  virtual void RemoveEdge(const SMDS_MeshEdge * edge);
  virtual void RemoveFace(const SMDS_MeshFace * face);
  virtual void RemoveVolume(const SMDS_MeshVolume * volume);

  /*! Remove only the given element and only if it is free.
   *  Method does not work for meshes with descendants.
   *  Implemented for fast cleaning of meshes.
   */
  virtual void RemoveFreeElement(const SMDS_MeshElement * elem);

  virtual void Clear();

  virtual bool RemoveFromParent();
  virtual bool RemoveSubMesh(const SMDS_Mesh * aMesh);

  bool ChangeElementNodes(const SMDS_MeshElement * elem,
                          const SMDS_MeshNode    * nodes[],
                          const int                nbnodes);
  bool ChangePolyhedronNodes(const SMDS_MeshElement *                 elem,
                             const std::vector<const SMDS_MeshNode*>& nodes,
                             const std::vector<int> &                 quantities);

  virtual void Renumber (const bool isNodes, const int startID = 1, const int deltaID = 1);
  // Renumber all nodes or elements.
  virtual void compactMesh();

  const SMDS_MeshNode *FindNode(int idnode) const;
  const SMDS_MeshNode *FindNodeVtk(int idnode) const;
  const SMDS_Mesh0DElement* Find0DElement(int idnode) const;
  const SMDS_BallElement* FindBall(int idnode) const;
  const SMDS_MeshEdge *FindEdge(int idnode1, int idnode2) const;
  const SMDS_MeshEdge *FindEdge(int idnode1, int idnode2, int idnode3) const;
  const SMDS_MeshFace *FindFace(int idnode1, int idnode2, int idnode3) const;
  const SMDS_MeshFace *FindFace(int idnode1, int idnode2, int idnode3, int idnode4) const;
  const SMDS_MeshFace *FindFace(int idnode1, int idnode2, int idnode3,
                                int idnode4, int idnode5, int idnode6) const;
  const SMDS_MeshFace *FindFace(int idnode1, int idnode2, int idnode3, int idnode4,
                                int idnode5, int idnode6, int idnode7, int idnode8) const;
  const SMDS_MeshElement *FindElement(int IDelem) const;
  static const SMDS_Mesh0DElement* Find0DElement(const SMDS_MeshNode * n);
  static const SMDS_BallElement* FindBall(const SMDS_MeshNode * n);
  static const SMDS_MeshEdge* FindEdge(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2);
  static const SMDS_MeshEdge* FindEdge(const SMDS_MeshNode * n1,
                                       const SMDS_MeshNode * n2,
                                       const SMDS_MeshNode * n3);
  static const SMDS_MeshFace* FindFace(const SMDS_MeshNode *n1,
                                       const SMDS_MeshNode *n2,
                                       const SMDS_MeshNode *n3);
  static const SMDS_MeshFace* FindFace(const SMDS_MeshNode *n1,
                                       const SMDS_MeshNode *n2,
                                       const SMDS_MeshNode *n3,
                                       const SMDS_MeshNode *n4);
  static const SMDS_MeshFace* FindFace(const SMDS_MeshNode *n1,
                                       const SMDS_MeshNode *n2,
                                       const SMDS_MeshNode *n3,
                                       const SMDS_MeshNode *n4,
                                       const SMDS_MeshNode *n5,
                                       const SMDS_MeshNode *n6);
  static const SMDS_MeshFace* FindFace(const SMDS_MeshNode *n1,
                                       const SMDS_MeshNode *n2,
                                       const SMDS_MeshNode *n3,
                                       const SMDS_MeshNode *n4,
                                       const SMDS_MeshNode *n5,
                                       const SMDS_MeshNode *n6,
                                       const SMDS_MeshNode *n7,
                                       const SMDS_MeshNode *n8);

  const SMDS_MeshFace *FindFace(const std::vector<int>& nodes_ids) const;
  static const SMDS_MeshFace* FindFace(const std::vector<const SMDS_MeshNode *>& nodes);
  static const SMDS_MeshElement* FindElement(const std::vector<const SMDS_MeshNode *>& nodes,
                                             const SMDSAbs_ElementType                 type=SMDSAbs_All,
                                             const bool                                noMedium=true);

  /*!
   * \brief Raise an exception if free memory (ram+swap) too low
    * \param doNotRaise - if true, suppress exception, just return free memory size
    * \retval int - amount of available memory in MB or negative number in failure case
   */
  static int CheckMemory(const bool doNotRaise=false);

  int MaxNodeID() const;
  int MinNodeID() const;
  int MaxElementID() const;
  int MinElementID() const;

  const SMDS_MeshInfo& GetMeshInfo() const { return myInfo; }

  virtual int NbNodes() const;
  virtual int Nb0DElements() const;
  virtual int NbBalls() const;
  virtual int NbEdges() const;
  virtual int NbFaces() const;
  virtual int NbVolumes() const;
  virtual int NbSubMesh() const;

  void DumpNodes() const;
  void Dump0DElements() const;
  void DumpEdges() const;
  void DumpFaces() const;
  void DumpVolumes() const;
  void DebugStats() const;

  virtual ~SMDS_Mesh();

  bool hasConstructionEdges();
  bool hasConstructionFaces();
  bool hasInverseElements();
  void setConstructionEdges(bool);
  void setConstructionFaces(bool);
  void setInverseElements(bool);

  /*!
   * Checks if the element is present in mesh.
   * Useful to determine dead pointers.
   * Use this function for debug purpose only! Do not check in the code
   * using it even in _DEBUG_ mode
   */
  bool Contains (const SMDS_MeshElement* elem) const;

  typedef std::vector<SMDS_MeshNode *> SetOfNodes;
  typedef std::vector<SMDS_MeshCell *> SetOfCells;

  void updateNodeMinMax();
  void updateBoundingBox();
  double getMaxDim();
  int fromVtkToSmds(int vtkid);

  void incrementNodesCapacity(int nbNodes);
  void incrementCellsCapacity(int nbCells);
  void adjustStructure();
  void dumpGrid(string ficdump="dumpGrid");
  static int chunkSize;

  //! low level modification: add, change or remove node or element
  inline void setMyModified() { this->myModified = true; }

  void Modified();
  VTK_MTIME_TYPE GetMTime() const;
  bool isCompacted();

protected:
  SMDS_Mesh(SMDS_Mesh * parent);

  SMDS_MeshFace * createTriangle(const SMDS_MeshNode * node1,
                                 const SMDS_MeshNode * node2,
                                 const SMDS_MeshNode * node3,
                                 int ID);
  SMDS_MeshFace * createQuadrangle(const SMDS_MeshNode * node1,
                                   const SMDS_MeshNode * node2,
                                   const SMDS_MeshNode * node3,
                                   const SMDS_MeshNode * node4,
                                   int ID);
  SMDS_MeshEdge* FindEdgeOrCreate(const SMDS_MeshNode * n1,
                                  const SMDS_MeshNode * n2);
  SMDS_MeshFace* FindFaceOrCreate(const SMDS_MeshNode *n1,
                                  const SMDS_MeshNode *n2,
                                  const SMDS_MeshNode *n3);
  SMDS_MeshFace* FindFaceOrCreate(const SMDS_MeshNode *n1,
                                  const SMDS_MeshNode *n2,
                                  const SMDS_MeshNode *n3,
                                  const SMDS_MeshNode *n4);

  bool registerElement(int ID, SMDS_MeshElement * element);

  void addChildrenWithNodes(std::set<const SMDS_MeshElement*>& setOfChildren,
                            const SMDS_MeshElement *           element,
                            std::set<const SMDS_MeshElement*>& nodes);

  inline void adjustmyCellsCapacity(int ID)
  {
    assert(ID >= 0);
    myElementIDFactory->adjustMaxId(ID);
    if (ID >= static_cast<int>(myCells.size()))
      myCells.resize(ID+SMDS_Mesh::chunkSize,0);
  }

  inline void adjustBoundingBox(double x, double y, double z)
  {
    if (x > xmax) xmax = x;
    else if (x < xmin) xmin = x;
    if (y > ymax) ymax = y;
    else if (y < ymin) ymin = y;
    if (z > zmax) zmax = z;
    else if (z < zmin) zmin = z;
  }

  // Fields PRIVATE

  //! index of this SMDS_mesh in the static vector<SMDS_Mesh*> _meshList
  int myMeshId;

  //! actual nodes coordinates, cells definition and reverse connectivity are stored in a vtkUnstructuredGrid
  SMDS_UnstructuredGrid*      myGrid;

  //! Small objects like SMDS_MeshNode are allocated by chunks to limit memory costs of new
  ObjectPool<SMDS_MeshNode>* myNodePool;

  //! Small objects like SMDS_VtkVolume are allocated by chunks to limit memory costs of new
  ObjectPool<SMDS_VtkVolume>*   myVolumePool;
  ObjectPool<SMDS_VtkFace>*     myFacePool;
  ObjectPool<SMDS_VtkEdge>*     myEdgePool;
  ObjectPool<SMDS_BallElement>* myBallPool;

  //! SMDS_MeshNodes refer to vtk nodes (vtk id = index in myNodes),store reference to this mesh, and sub-shape
  SetOfNodes             myNodes;

  //! SMDS_MeshCells refer to vtk cells (vtk id != index in myCells),store reference to this mesh, and sub-shape
  SetOfCells             myCells;

  //! a buffer to speed up elements addition by excluding some memory allocation
  std::vector<vtkIdType> myNodeIds;

  //! for cells only: index = ID in vtkUnstructuredGrid, value = ID for SMDS users
  std::vector<int>       myCellIdVtkToSmds;

  SMDS_Mesh *            myParent;
  std::list<SMDS_Mesh *> myChildren;
  SMDS_MeshNodeIDFactory *myNodeIDFactory;
  SMDS_MeshElementIDFactory *myElementIDFactory;
  SMDS_MeshInfo          myInfo;

  //! use a counter to keep track of modifications
  unsigned long myModifTime, myCompactTime;

  int myNodeMin;
  int myNodeMax;

  bool myHasConstructionEdges;
  bool myHasConstructionFaces;
  bool myHasInverseElements;

  //! any add, remove or change of node or cell
  bool myModified;

  double xmin;
  double xmax;
  double ymin;
  double ymax;
  double zmin;
  double zmax;
};


#endif
