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

#ifndef _Handle_SMESH_MeshVSLink_HeaderFile
#define _Handle_SMESH_MeshVSLink_HeaderFile

#ifndef _Standard_Macro_HeaderFile
#include <Standard_Macro.hxx>
#endif
#ifndef _Standard_HeaderFile
#include <Standard.hxx>
#endif
#ifndef _Standard_Version_HeaderFile
#include <Standard_Version.hxx>
#endif

#if OCC_VERSION_HEX < 0x070000
#ifndef _Handle_MeshVS_DataSource3D_HeaderFile
//#include <Handle_MeshVS_DataSource3D.hxx>
#include <MeshVS_DataSource3D.hxx>
#include <SMESH_MeshVSLink.hxx>
#endif

class Standard_Transient;
class Standard_Type;
class SMESH_MeshVSLink;

Standard_EXPORT Handle(Standard_Type)& STANDARD_TYPE(SMESH_MeshVSLink);

class Handle(SMESH_MeshVSLink) : public Handle(MeshVS_DataSource3D) {
  public:
	Handle(SMESH_MeshVSLink)():Handle(MeshVS_DataSource3D)() {}
	Handle(SMESH_MeshVSLink)(const Handle(SMESH_MeshVSLink)& aHandle) : Handle(MeshVS_DataSource3D)(aHandle)
     {
     }

	Handle(SMESH_MeshVSLink)(const SMESH_MeshVSLink* anItem) : Handle(MeshVS_DataSource3D)((MeshVS_DataSource3D *)anItem)
     {
     }

    Handle(SMESH_MeshVSLink)& operator=(const Handle(SMESH_MeshVSLink)& aHandle)
     {
      Assign(aHandle.Access());
      return *this;
     }

    Handle(SMESH_MeshVSLink)& operator=(const SMESH_MeshVSLink* anItem)
     {
      Assign((Standard_Transient *)anItem);
      return *this;
     }

    SMESH_MeshVSLink* operator->() const
     {
      return (SMESH_MeshVSLink *)ControlAccess();
     }
 
   Standard_EXPORT static const Handle(SMESH_MeshVSLink) DownCast(const Handle(Standard_Transient)& AnObject);
};
#endif // OCC_VERSION_HEX < 0x070000
#endif
