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
//  File   : SMDS_MeshElement.hxx
//  Module : SMESH
//
#ifndef _SMDS_MeshElement_HeaderFile
#define _SMDS_MeshElement_HeaderFile

#include "SMESH_SMDS.hxx"
        
#include "SMDSAbs_ElementType.hxx"
#include "SMDS_MeshObject.hxx"
#include "SMDS_ElemIterator.hxx"
#include "SMDS_MeshElementIDFactory.hxx"
#include "SMDS_StdIterator.hxx"

#include <vector>
#include <iostream>

#include <vtkType.h>
#include <vtkCellType.h>
#include <vtkCellArray.h>

//typedef unsigned short UShortType;
typedef short ShortType;
typedef int   LongType;
#ifdef VTK_CELL_ARRAY_V2
typedef const vtkIdType* vtkIdTypePtr;
#else
typedef vtkIdType* vtkIdTypePtr;
#endif
class SMDS_MeshNode;
class SMDS_MeshEdge;
class SMDS_MeshFace;
class SMDS_Mesh;

// ============================================================
/*!
 * \brief Base class for elements
 */
// ============================================================


class SMDS_EXPORT SMDS_MeshElement:public SMDS_MeshObject
{
public:

  SMDS_ElemIteratorPtr nodesIterator() const;
  SMDS_ElemIteratorPtr edgesIterator() const;
  SMDS_ElemIteratorPtr facesIterator() const;
  virtual SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType type) const;
  virtual SMDS_ElemIteratorPtr interlacedNodesElemIterator() const;

  virtual SMDS_NodeIteratorPtr nodeIterator() const;
  virtual SMDS_NodeIteratorPtr interlacedNodesIterator() const;
  virtual SMDS_NodeIteratorPtr nodesIteratorToUNV() const;

  // std-like iteration on nodes
  typedef SMDS_StdIterator< const SMDS_MeshNode*, SMDS_ElemIteratorPtr > iterator;
  iterator begin_nodes() const { return iterator( nodesIterator() ); }
  iterator end_nodes()   const { return iterator(); }

  virtual int NbNodes() const;
  virtual int NbEdges() const;
  virtual int NbFaces() const;
  inline int GetID() const { return myID; }

  ///Return the type of the current element
  virtual SMDSAbs_ElementType GetType() const = 0;
  virtual SMDSAbs_EntityType GetEntityType() const = 0;
  virtual SMDSAbs_GeometryType GetGeomType() const = 0;
  virtual vtkIdType GetVtkType() const = 0;
  virtual bool IsPoly() const { return false; }
  virtual bool IsQuadratic() const;

  virtual bool IsMediumNode(const SMDS_MeshNode* node) const;
  virtual int  NbCornerNodes() const;

  friend SMDS_EXPORT std::ostream & operator <<(std::ostream & OS, const SMDS_MeshElement *);
  friend SMDS_EXPORT bool SMDS_MeshElementIDFactory::BindID(int ID,SMDS_MeshElement* elem);
  friend class SMDS_Mesh;
  friend class SMESHDS_Mesh;
  friend class SMESHDS_SubMesh;
  friend class SMDS_MeshElementIDFactory;

  // ===========================
  //  Access to nodes by index
  // ===========================
  /*!
   * \brief Return node by its index
    * \param ind - node index
    * \retval const SMDS_MeshNode* - the node
   */
  virtual const SMDS_MeshNode* GetNode(const int ind) const;

  /*!
   * \brief Return node by its index
    * \param ind - node index
    * \retval const SMDS_MeshNode* - the node
   * 
   * Index is wrapped if it is out of a valid range
   */
  const SMDS_MeshNode* GetNodeWrap(const int ind) const { return GetNode( WrappedIndex( ind )); }

  /*!
   * \brief Return true if index of node is valid (0 <= ind < NbNodes())
    * \param ind - node index
    * \retval bool - index check result
   */
  virtual bool IsValidIndex(const int ind) const;

  /*!
   * \brief Return a valid node index, fixing the given one if necessary
    * \param ind - node index
    * \retval int - valid node index
   */
  int WrappedIndex(const int ind) const {
    if ( ind < 0 ) return NbNodes() + ind % NbNodes();
    if ( ind >= NbNodes() ) return ind % NbNodes();
    return ind;
  }

  /*!
   * \brief Check if a node belongs to the element
    * \param node - the node to check
    * \retval int - node index within the element, -1 if not found
   */
  virtual int GetNodeIndex( const SMDS_MeshNode* node ) const;

  inline ShortType getMeshId()  const { return myMeshId; }
  inline LongType  getshapeId() const { return myShapeId; }
  inline int getIdInShape()     const { return myIdInShape; }
  inline int getVtkId()         const { return myVtkID; }

  /*!
   * \brief Filters of elements, to be used with SMDS_SetIterator
   */
  struct Filter
  {
    virtual bool operator()(const SMDS_MeshElement* e) const = 0;
    virtual ~Filter() {}
  };
  struct NonNullFilter: public Filter
  {
    bool operator()(const SMDS_MeshElement* e) const { return e != 0; }
  };
  struct TypeFilter : public Filter
  {
    SMDSAbs_ElementType _type;
    TypeFilter( SMDSAbs_ElementType t = SMDSAbs_NbElementTypes ):_type(t) {}
    bool operator()(const SMDS_MeshElement* e) const { return e && e->GetType() == _type; }
  };
  struct EntityFilter : public Filter
  {
    SMDSAbs_EntityType _type;
    EntityFilter( SMDSAbs_EntityType t = SMDSEntity_Last ):_type(t) {}
    bool operator()(const SMDS_MeshElement* e) const { return e && e->GetEntityType() == _type; }
  };
  struct GeomFilter : public Filter
  {
    SMDSAbs_GeometryType _type;
    GeomFilter( SMDSAbs_GeometryType t = SMDSGeom_NONE ):_type(t) {}
    bool operator()(const SMDS_MeshElement* e) const { return e && e->GetGeomType() == _type; }
  };

protected:
  inline void setId(int id) {myID = id; }
  inline void setShapeId(LongType shapeId) {myShapeId = shapeId; }
  inline void setIdInShape(int id) { myIdInShape = id; }
  inline void setVtkId(int vtkId) { myVtkID = vtkId; }
  SMDS_MeshElement(int ID=-1);
  SMDS_MeshElement(int id, ShortType meshId, LongType shapeId = 0);
  virtual void init(int id = -1, ShortType meshId = -1, LongType shapeId = 0);
  virtual void Print(std::ostream & OS) const;

  //! Element index in vector SMDS_Mesh::myNodes or SMDS_Mesh::myCells
  int myID;
  //! index in vtkUnstructuredGrid
  vtkIdType myVtkID;
  //! SMDS_Mesh identification in SMESH
  ShortType myMeshId;
  //! SubShape and SubMesh identification in SMESHDS
  LongType myShapeId;
  //! Element index in SMESHDS_SubMesh vector
  int myIdInShape;
};


// ============================================================
/*!
 * \brief Comparator of elements by ID for usage in std containers
 */
// ============================================================

struct TIDTypeCompare {
  bool operator () (const SMDS_MeshElement* e1, const SMDS_MeshElement* e2) const
  { return e1->GetType() == e2->GetType() ? e1->GetID() < e2->GetID() : e1->GetType() < e2->GetType(); }
};

// WARNING: this comparator makes impossible to store both nodes and elements in the same set
// because there are nodes and elements with the same ID. Use TIDTypeCompare for such containers.
struct TIDCompare {
  template<typename T> bool operator () (const T* e1, const T* e2) const
  { return static_cast<const SMDS_MeshElement*>(e1)->GetID() < static_cast<const SMDS_MeshElement*>(e2)->GetID(); }
};

#endif
