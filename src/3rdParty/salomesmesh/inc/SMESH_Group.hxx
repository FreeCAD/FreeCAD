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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : SMESH_Group.hxx
//  Author : Michael Sazonov (OCC)
//  Module : SMESH
//
#ifndef _SMESH_Group_HeaderFile
#define _SMESH_Group_HeaderFile

#include "SMESH_SMESH.hxx"

#include "SMDSAbs_ElementType.hxx"
#include "SMESH_Controls.hxx"

#include <string>
#include <TopoDS_Shape.hxx>

class SMESHDS_GroupBase;
class SMESH_Mesh;

class SMESH_EXPORT  SMESH_Group
{
 public:

  SMESH_Group (int                       theID,
               const SMESH_Mesh*         theMesh,
               const SMDSAbs_ElementType theType,
               const char*               theName,
               const TopoDS_Shape&       theShape = TopoDS_Shape(),
               const SMESH_PredicatePtr& thePredicate = SMESH_PredicatePtr());
  SMESH_Group (SMESHDS_GroupBase* groupDS);
  ~SMESH_Group ();

  void SetName (const char* theName);

  const char* GetName () const { return myName.c_str(); }

  SMESHDS_GroupBase * GetGroupDS () { return myGroupDS; }

 private:
  SMESH_Group (const SMESH_Group& theOther);
  // prohibited copy constructor
  SMESH_Group& operator = (const SMESH_Group& theOther);
  // prohibited assign operator

  SMESHDS_GroupBase * myGroupDS;
  std::string         myName;
};

#endif
