// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_SubMesh.hxx
//  Module : SMESH
//
#ifndef _SMESHDS_SubMesh_HeaderFile
#define _SMESHDS_SubMesh_HeaderFile

#include "SMESH_SMESHDS.hxx"

#include "SMDS_Mesh.hxx"
#include <set>
#include <vector>

class SMESHDS_SubMesh;
typedef SMDS_Iterator<const SMESHDS_SubMesh*> SMESHDS_SubMeshIterator;
typedef boost::shared_ptr< SMESHDS_SubMeshIterator > SMESHDS_SubMeshIteratorPtr;

class SMESHDS_Mesh;

class SMESHDS_EXPORT SMESHDS_SubMesh
{
 public:
  SMESHDS_SubMesh(SMESHDS_Mesh *parent, int index);
  virtual ~SMESHDS_SubMesh();

  virtual bool IsComplexSubmesh() const { return !mySubMeshes.empty(); }

  // if !IsComplexSubmesh()
  virtual void AddElement(const SMDS_MeshElement * ME);
  virtual bool RemoveElement(const SMDS_MeshElement * ME, bool isElemDeleted); // ret true if ME was in
  virtual void AddNode(const SMDS_MeshNode * ME);
  virtual bool RemoveNode(const SMDS_MeshNode * ME, bool isNodeDeleted);       // ret true if ME was in
  virtual const SMDS_MeshElement* GetElement( size_t idInShape ) const;
  virtual const SMDS_MeshNode*    GetNode   ( size_t idInShape ) const;

  // if IsComplexSubmesh()
  void AddSubMesh( const SMESHDS_SubMesh* theSubMesh );
  bool RemoveSubMesh( const SMESHDS_SubMesh* theSubMesh );
  void RemoveAllSubmeshes();
  bool ContainsSubMesh( const SMESHDS_SubMesh* theSubMesh ) const;
  int  NbSubMeshes() const { return mySubMeshes.size(); }
  SMESHDS_SubMeshIteratorPtr GetSubMeshIterator() const;

  // for both types
  virtual int NbElements() const;
  virtual SMDS_ElemIteratorPtr GetElements() const;
  virtual int NbNodes() const;
  virtual SMDS_NodeIteratorPtr GetNodes() const;
  virtual bool Contains(const SMDS_MeshElement * ME) const;      // check if elem or node is in
  virtual bool IsQuadratic() const;

  // clear the contents
  virtual void Clear();
  int  getSize();
  void compactList();

  SMESHDS_Mesh* GetParent() const { return const_cast< SMESHDS_Mesh*>( myParent ); }
  int           GetID()     const { return myIndex; }

 private:
  SMESHDS_Mesh *                       myParent;
  std::vector<const SMDS_MeshElement*> myElements;
  std::vector<const SMDS_MeshNode*>    myNodes;

  int myUnusedIdNodes;
  int myUnusedIdElements;
  int myIndex;

  std::set<const SMESHDS_SubMesh*> mySubMeshes;
};
#endif
