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

//  SMESH SMESHDS : idl implementation based on 'SMESH' unit's classes
//  File   : SMESHDS_Group.cxx
//  Module : SMESH
//  $Header$
//
#include "SMESHDS_Group.hxx"
#include "SMESHDS_Mesh.hxx"

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESHDS_Group::SMESHDS_Group (const int                 theID,
                              const SMESHDS_Mesh*       theMesh,
                              const SMDSAbs_ElementType theType)
     : SMESHDS_GroupBase(theID,theMesh,theType),
       myGroup(theMesh,theType)
{
}

//=======================================================================
//function : Extent
//purpose  : 
//=======================================================================

int SMESHDS_Group::Extent() const
{
  return myGroup.Extent();
}

//=======================================================================
//function : IsEmpty
//purpose  : 
//=======================================================================

bool SMESHDS_Group::IsEmpty()
{
  return myGroup.IsEmpty();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool SMESHDS_Group::Contains (const int theID)
{
  const SMDS_MeshElement* aElem = findInMesh (theID);
  if (aElem)
    return myGroup.Contains(aElem);
  return false;
}

//=======================================================================
//function : Contains
//purpose  : 
//=======================================================================

bool SMESHDS_Group::Contains (const SMDS_MeshElement* elem)
{
  if (elem)
    return myGroup.Contains(elem);
  return false;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool SMESHDS_Group::Add (const int theID)
{
  return Add( findInMesh( theID ));
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool SMESHDS_Group::Add (const SMDS_MeshElement* aElem )
{
  if (!aElem || myGroup.Contains(aElem))
    return false;

  if (myGroup.IsEmpty())
    SetType( aElem->GetType() );

  myGroup.Add (aElem);
  resetIterator();
  return true;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool SMESHDS_Group::Remove (const int theID)
{
  const SMDS_MeshElement* aElem = findInMesh (theID);
  if (!aElem || !myGroup.Contains(aElem))
    return false;
  myGroup.Remove (aElem);
  resetIterator();
  return true;
}


//======================================================================
//function : Clear
//purpose  : 
//=======================================================================

void SMESHDS_Group::Clear()
{
  myGroup.Clear();
  resetIterator();
}

// =====================
// class MyGroupIterator
// =====================

class MyGroupIterator: public SMDS_ElemIterator
{
  const SMDS_MeshGroup& myGroup;
 public:
  MyGroupIterator(const SMDS_MeshGroup& group): myGroup(group) { myGroup.InitIterator(); }
  bool more() { return myGroup.More(); }
  const SMDS_MeshElement* next() { return myGroup.Next(); }
};
   
//=======================================================================
//function : GetElements
//purpose  : 
//=======================================================================

SMDS_ElemIteratorPtr SMESHDS_Group::GetElements() const
{
  return SMDS_ElemIteratorPtr( new MyGroupIterator ( myGroup ));
}

//================================================================================
/*!
 * \brief Return a value allowing to find out if a group has changed or not
 */
//================================================================================

VTK_MTIME_TYPE SMESHDS_Group::GetTic() const
{
  return myGroup.Tic();
}

//=======================================================================
//function : SetType
//purpose  : 
//=======================================================================

void SMESHDS_Group::SetType(SMDSAbs_ElementType theType)
{
  if ( myGroup.IsEmpty() || GetType() == SMDSAbs_All ) {
    SMESHDS_GroupBase::SetType( theType );
    myGroup.SetType ( theType );
  }
  else
    SMESHDS_GroupBase::SetType( myGroup.GetType() );
}

