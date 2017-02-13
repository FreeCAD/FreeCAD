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
//  File   : SMESHDS_GroupOnFilter.hxx
//  Module : SMESH
//
#ifndef _SMESHDS_GroupOnFilter_HeaderFile
#define _SMESHDS_GroupOnFilter_HeaderFile

#include "SMESH_SMESHDS.hxx"

#include "SMESHDS_GroupBase.hxx"
#include "SMESH_Controls.hxx"
  
/*!
 * \brief Groups whose contents is dynamically updated using the filter
 */
class SMESHDS_EXPORT SMESHDS_GroupOnFilter: public SMESHDS_GroupBase
{
 public:

  SMESHDS_GroupOnFilter (const int                 theID,
                         const SMESHDS_Mesh*       theMesh,
                         const SMDSAbs_ElementType theType,
                         const SMESH_PredicatePtr& thePredicate);

  void SetPredicate( const SMESH_PredicatePtr& thePredicate);

  SMESH_PredicatePtr GetPredicate() const { return myPredicate; }

  std::vector< int > GetMeshInfo() const;

  template< typename IDTYPE >
    int GetElementIds( IDTYPE* ids ) const
  {
    return getElementIds( (void*)ids, sizeof(IDTYPE));
  }


  virtual int  Extent() const;

  virtual bool IsEmpty();

  virtual bool Contains (const int theID);

  virtual bool Contains (const SMDS_MeshElement* elem);

  virtual SMDS_ElemIteratorPtr GetElements() const;

  virtual VTK_MTIME_TYPE GetTic() const;

  bool         IsUpToDate() const;

 private:

  void update() const;
  void setChanged(bool changed=true);
  const SMDS_MeshElement* setNbElemToSkip( SMDS_ElemIteratorPtr& elIt );
  int getElementIds( void* ids, size_t idSize ) const;

  // We use two ways of optimaization:
  // 1) The case of little free memory. Remember nb of KO elements (myNbElemToSkip)
  //    to skip before the first OK element. As well remember total nb of OK
  //    elements (myMeshInfo) to stop iteration as all OK elements are found.
  // 2) The case of enough free memory. Remember all OK elements (myElements).

  SMESH_PredicatePtr                    myPredicate;
  std::vector< int >                    myMeshInfo;
  std::vector< const SMDS_MeshElement*> myElements;
  bool                                  myElementsOK;
  VTK_MTIME_TYPE                        myMeshModifTime; // when myMeshInfo was updated
  int                                   myPredicateTic;
  size_t                                myNbElemToSkip;
};

#endif
