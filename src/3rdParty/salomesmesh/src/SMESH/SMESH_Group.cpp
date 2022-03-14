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
//  File   : SMESH_Group.cxx
//  Author : Michael Sazonov (OCC)
//  Module : SMESH
//
#include "SMESH_Group.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESHDS_Group.hxx"
#include "SMESHDS_GroupOnGeom.hxx"
#include "SMESHDS_GroupOnFilter.hxx"

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_Group::SMESH_Group (int                       theID,
                          const SMESH_Mesh*         theMesh,
                          const SMDSAbs_ElementType theType,
                          const char*               theName,
                          const TopoDS_Shape&       theShape,
                          const SMESH_PredicatePtr& thePredicate)
     : myName(theName)
{
  if ( !theShape.IsNull() )
    myGroupDS = new SMESHDS_GroupOnGeom (theID,
                                         const_cast<SMESH_Mesh*>(theMesh)->GetMeshDS(),
                                         theType,
                                         theShape);
  else if ( thePredicate )
    myGroupDS = new SMESHDS_GroupOnFilter (theID,
                                           const_cast<SMESH_Mesh*>(theMesh)->GetMeshDS(),
                                           theType,
                                           thePredicate);
  else
    myGroupDS = new SMESHDS_Group (theID,
                                   const_cast<SMESH_Mesh*>(theMesh)->GetMeshDS(),
                                   theType);
  myGroupDS->SetStoreName( theName );
}

//================================================================================
/*!
 * \brief Constructor accesible to SMESH_Mesh only
 */
//================================================================================

SMESH_Group::SMESH_Group (SMESHDS_GroupBase* groupDS): myGroupDS( groupDS )
{
  if ( myGroupDS )
    myName = myGroupDS->GetStoreName();
}

//=============================================================================
/*!
 *  Destructor deletes myGroupDS
 */
//=============================================================================

SMESH_Group::~SMESH_Group ()
{
  delete myGroupDS; myGroupDS=0;
}

//================================================================================
/*!
 * \brief Sets a new name
 */
//================================================================================

void SMESH_Group::SetName (const char* theName)
{
  myName = theName;
  myGroupDS->SetStoreName( theName );
}
