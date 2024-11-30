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
//  File   : SMDS_MeshGroup.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_MeshGroup.hxx"
#include "utilities.h"

using namespace std;

//=======================================================================
//function : SMDS_MeshGroup
//purpose  : 
//=======================================================================

SMDS_MeshGroup::SMDS_MeshGroup(const SMDS_Mesh * theMesh,
                               const SMDSAbs_ElementType theType)
  :myMesh(theMesh),myType(theType), myParent(NULL), myTic(0)
{
}

//=======================================================================
//function : SMDS_MeshGroup
//purpose  : 
//=======================================================================

SMDS_MeshGroup::SMDS_MeshGroup(SMDS_MeshGroup * theParent,
                               const SMDSAbs_ElementType theType)
        :myMesh(theParent->myMesh),myType(theType), myParent(theParent)
{
}

//=======================================================================
//function : AddSubGroup
//purpose  : 
//=======================================================================

const SMDS_MeshGroup *SMDS_MeshGroup::AddSubGroup
                (const SMDSAbs_ElementType theType)
{
        const SMDS_MeshGroup * subgroup = new SMDS_MeshGroup(this,theType);
        myChildren.insert(myChildren.end(),subgroup);
        return subgroup;
}

//=======================================================================
//function : RemoveSubGroup
//purpose  : 
//=======================================================================

bool SMDS_MeshGroup::RemoveSubGroup(const SMDS_MeshGroup * theGroup)
{
        bool found = false;     
        list<const SMDS_MeshGroup*>::iterator itgroup;
        for(itgroup=myChildren.begin(); itgroup!=myChildren.end(); itgroup++)
        {
                const SMDS_MeshGroup* subgroup=*itgroup;
                if (subgroup == theGroup)
                {
                        found = true;
                        myChildren.erase(itgroup);
                }
        }

        return found;
}

//=======================================================================
//function : RemoveFromParent
//purpose  : 
//=======================================================================

bool SMDS_MeshGroup::RemoveFromParent()
{
        
        if (myParent==NULL) return false;
        else
        {
                return (myParent->RemoveSubGroup(this));
        }
}
//=======================================================================
//function : Clear
//purpose  : 
//=======================================================================

void SMDS_MeshGroup::Clear()
{
  myElements.clear();
  myType = SMDSAbs_All;
  ++myTic;
}

//=======================================================================
//function : Add
//purpose  : 
//=======================================================================

bool SMDS_MeshGroup::Add(const SMDS_MeshElement * theElem)
{
  // the type of the group is determined by the first element added
  if (myElements.empty()) {
    myType = theElem->GetType();
  }
  else if (theElem->GetType() != myType) {
    MESSAGE("SMDS_MeshGroup::Add : Type Mismatch "<<theElem->GetType()<<"!="<<myType);
    return false;
  }
        
  myElements.insert(myElements.end(), theElem);
  ++myTic;

  return true;
}

//=======================================================================
//function : Remove
//purpose  : 
//=======================================================================

bool SMDS_MeshGroup::Remove(const SMDS_MeshElement * theElem)
{
  set<const SMDS_MeshElement *>::iterator found = myElements.find(theElem);
  if ( found != myElements.end() ) {
    myElements.erase(found);
    if (myElements.empty()) myType = SMDSAbs_All;
    ++myTic;
    return true;
  }
  return false;
}

//=======================================================================
//function : Contains
//purpose  : 
//=======================================================================

bool SMDS_MeshGroup::Contains(const SMDS_MeshElement * theElem) const
{
        return myElements.find(theElem)!=myElements.end();
}

//=======================================================================
//function : SetType
//purpose  : 
//=======================================================================

void SMDS_MeshGroup::SetType(const SMDSAbs_ElementType theType)
{
  if (IsEmpty())
    myType = theType;
}
