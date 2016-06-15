//  SMESH  SMESH_MeshVSLink : Connection of SMESH with MeshVS from OCC 
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
// File      : SMESH_MeshVSLink.cxx
// Created   : Mon Dec 1 09:00:00 2008
// Author    : Sioutis Fotios
// Module    : SMESH

#if OCC_VERSION_HEX < 0x070000

#include <SMESH_MeshVSLink.jxx>

#ifndef _Standard_TypeMismatch_HeaderFile
#include <Standard_TypeMismatch.hxx>
#endif

Standard_EXPORT Handle_Standard_Type& SMESH_MeshVSLink_Type_()
{
  static Handle_Standard_Type aType1 = STANDARD_TYPE(MeshVS_DataSource3D);
  static Handle_Standard_Type aType2 = STANDARD_TYPE(MeshVS_DataSource);
  static Handle_Standard_Type aType3 = STANDARD_TYPE(MMgt_TShared);
  static Handle_Standard_Type aType4 = STANDARD_TYPE(Standard_Transient);
 

  static Handle_Standard_Transient _Ancestors[]= {aType1,aType2,aType3,aType4,NULL};
  static Handle_Standard_Type _aType = new Standard_Type("SMESH_MeshVSLink",
                                                         sizeof(SMESH_MeshVSLink),
                                                         1,
                                                         (Standard_Address)_Ancestors,
                                                         (Standard_Address)NULL);

  return _aType;
}


// DownCast method
//   allow safe downcasting
//
const Handle(SMESH_MeshVSLink) Handle(SMESH_MeshVSLink)::DownCast(const Handle(Standard_Transient)& AnObject) 
{
  Handle(SMESH_MeshVSLink) _anOtherObject;

  if (!AnObject.IsNull()) {
     if (AnObject->IsKind(STANDARD_TYPE(SMESH_MeshVSLink))) {
       _anOtherObject = Handle(SMESH_MeshVSLink)((Handle(SMESH_MeshVSLink)&)AnObject);
     }
  }

  return _anOtherObject ;
}

const Handle(Standard_Type)& SMESH_MeshVSLink::DynamicType() const 
{ 
   return STANDARD_TYPE(SMESH_MeshVSLink) ; 
}
#endif