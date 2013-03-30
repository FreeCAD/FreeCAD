//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMDS : implementaion of Salome mesh data structure
//  File   : SMDS_Mesh.hxx
//  Module : SMESH
//
#ifndef _SMDS_Mesh_HeaderFile
#define _SMDS_Mesh_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMDS_MeshEdge.hxx"
#include "SMDS_MeshFace.hxx"
#include "SMDS_MeshVolume.hxx"
#include "SMDS_MeshElementIDFactory.hxx"
#include "SMDS_MeshInfo.hxx"
#include "SMDS_ElemIterator.hxx"
#include <NCollection_Map.hxx>

#include <boost/shared_ptr.hpp>
#include <set>
#include <list>

class SMDS_EXPORT SMDS_Mesh:public SMDS_MeshObject{
public:
  
  SMDS_Mesh();
  
  SMDS_NodeIteratorPtr nodesIterator() const;
  SMDS_EdgeIteratorPtr edgesIterator() const;
  SMDS_FaceIteratorPtr facesIterator() const;
  SMDS_VolumeIteratorPtr volumesIterator() const;
  SMDS_ElemIteratorPtr elementsIterator() const;  
  
  SMDSAbs_ElementType GetElementType( const int id, const bool iselem ) const;

  SMDS_Mesh *AddSubMesh();
  
  virtual SMDS_MeshNode* AddNodeWithID(double x, double y, double z, int ID);
  virtual SMDS_MeshNode* AddNode(double x, double y, double z);
  
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

  virtual SMDS_MeshFace* AddPolygonalFaceWithID (std::vector<int> nodes_ids,
                                                 const int        ID);

  virtual SMDS_MeshFace* AddPolygonalFaceWithID (std::vector<const SMDS_MeshNode*> nodes,
                                                 const int                         ID);

  virtual SMDS_MeshFace* AddPolygonalFace (std::vector<const SMDS_MeshNode*> nodes);

  virtual SMDS_MeshVolume* AddPolyhedralVolumeWithID
                           (std::vector<int> nodes_ids,
                            std::vector<int> quantities,
                            const int        ID);

  virtual SMDS_MeshVolume* AddPolyhedralVolumeWithID
                           (std::vector<const SMDS_MeshNode*> nodes,
                            std::vector<int>                  quantities,
                            const int                         ID);

  virtual SMDS_MeshVolume* AddPolyhedralVolume
                           (std::vector<const SMDS_MeshNode*> nodes,
                            std::vector<int>                  quantities);

  virtual void RemoveElement(const SMDS_MeshElement *        elem,
                             std::list<const SMDS_MeshElement *>& removedElems,
                             std::list<const SMDS_MeshElement *>& removedNodes,
							 bool                      removenodes = false);
  virtual void RemoveElement(const SMDS_MeshElement * elem, bool removenodes = false);
  virtual void RemoveNode(const SMDS_MeshNode * node);
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

  const SMDS_MeshNode *FindNode(int idnode) const;
  const SMDS_MeshEdge *FindEdge(int idnode1, int idnode2) const;
  const SMDS_MeshEdge *FindEdge(int idnode1, int idnode2, int idnode3) const;
  const SMDS_MeshFace *FindFace(int idnode1, int idnode2, int idnode3) const;
  const SMDS_MeshFace *FindFace(int idnode1, int idnode2, int idnode3, int idnode4) const;
  const SMDS_MeshFace *FindFace(int idnode1, int idnode2, int idnode3,
                                int idnode4, int idnode5, int idnode6) const;
  const SMDS_MeshFace *FindFace(int idnode1, int idnode2, int idnode3, int idnode4,
                                int idnode5, int idnode6, int idnode7, int idnode8) const;
  const SMDS_MeshElement *FindElement(int IDelem) const;
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

  const SMDS_MeshFace *FindFace(std::vector<int> nodes_ids) const;
  static const SMDS_MeshFace* FindFace(std::vector<const SMDS_MeshNode *> nodes);

  /*!
   * \brief Raise an exception if free memory (ram+swap) too low
    * \param doNotRaise - if true, suppres exception, just return free memory size
    * \retval int - amount of available memory in MB or negative number in failure case
   */
  static int CheckMemory(const bool doNotRaise=false) throw (std::bad_alloc);

  int MaxNodeID() const;
  int MinNodeID() const;
  int MaxElementID() const;
  int MinElementID() const;

  const SMDS_MeshInfo& GetMeshInfo() const { return myInfo; }

  int NbNodes() const;
  int NbEdges() const;
  int NbFaces() const;
  int NbVolumes() const;
  int NbSubMesh() const;
  void DumpNodes() const;
  void DumpEdges() const;
  void DumpFaces() const;
  void DumpVolumes() const;
  void DebugStats() const;
  SMDS_Mesh *boundaryFaces();
  SMDS_Mesh *boundaryEdges();
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

  typedef NCollection_Map<SMDS_MeshNode *> SetOfNodes;
  typedef NCollection_Map<SMDS_MeshEdge *> SetOfEdges;
  typedef NCollection_Map<SMDS_MeshFace *> SetOfFaces;
  typedef NCollection_Map<SMDS_MeshVolume *> SetOfVolumes;

private:
  SMDS_Mesh(SMDS_Mesh * parent);

  SMDS_MeshFace * createTriangle(const SMDS_MeshNode * node1, 
				 const SMDS_MeshNode * node2, 
				 const SMDS_MeshNode * node3);
  SMDS_MeshFace * createQuadrangle(const SMDS_MeshNode * node1,
				   const SMDS_MeshNode * node2, 
				   const SMDS_MeshNode * node3, 
				   const SMDS_MeshNode * node4);
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
			    const SMDS_MeshElement * element, 
			    std::set<const SMDS_MeshElement*>& nodes);

  // Fields PRIVATE
  
  SetOfNodes             myNodes;
  SetOfEdges             myEdges;
  SetOfFaces             myFaces;
  SetOfVolumes           myVolumes;
  SMDS_Mesh *            myParent;
  std::list<SMDS_Mesh *> myChildren;
  SMDS_MeshElementIDFactory *myNodeIDFactory;
  SMDS_MeshElementIDFactory *myElementIDFactory;
  SMDS_MeshInfo          myInfo;

  bool myHasConstructionEdges;
  bool myHasConstructionFaces;
  bool myHasInverseElements;
};


#endif
