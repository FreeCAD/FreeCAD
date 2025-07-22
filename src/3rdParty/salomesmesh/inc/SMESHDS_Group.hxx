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
//  File   : SMESHDS_Group.hxx
//  Module : SMESH
//  $Header$
//
#ifndef _SMESHDS_Group_HeaderFile
#define _SMESHDS_Group_HeaderFile

#include "SMESH_SMESHDS.hxx"

#include <string>
#include "SMESHDS_GroupBase.hxx"
#include "SMDS_MeshGroup.hxx"

class SMESHDS_Mesh;

class SMESHDS_EXPORT SMESHDS_Group : public SMESHDS_GroupBase
{
 public:

  SMESHDS_Group (const int                 theID,
                 const SMESHDS_Mesh*       theMesh,
                 const SMDSAbs_ElementType theType);

  virtual void SetType(SMDSAbs_ElementType theType);

  virtual int Extent() const;

  virtual bool IsEmpty();

  virtual bool Contains (const int theID);

  virtual bool Contains (const SMDS_MeshElement* elem);

  virtual SMDS_ElemIteratorPtr GetElements() const;

  virtual VTK_MTIME_TYPE GetTic() const;

  bool Add (const int theID);

  bool Add (const SMDS_MeshElement* theElem );

  bool Remove (const int theID);

  void Clear();

  SMDS_MeshGroup& SMDSGroup() { return myGroup; }

 private:

  SMDS_MeshGroup myGroup;

};

#endif
