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
#include "SMESHDS_GroupBase.hxx"
#include "SMESHDS_Mesh.hxx"

#include "utilities.h"

using namespace std;

Quantity_Color SMESHDS_GroupBase::myDefaultColor = Quantity_Color( 0.0, 0.0, 0.0, Quantity_TOC_RGB );

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESHDS_GroupBase::SMESHDS_GroupBase (const int                 theID,
                                      const SMESHDS_Mesh*       theMesh,
                                      const SMDSAbs_ElementType theType):
       myID(theID), myMesh(theMesh), myType(theType), myStoreName(""),
       myCurIndex(0), myCurID(-1)
{
  myColor = myDefaultColor;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

int SMESHDS_GroupBase::GetID (const int theIndex)
{
  if (myCurIndex < 1 || myCurIndex > theIndex) {
    myIterator = GetElements();
    myCurIndex = 0;
    myCurID = -1;
  }
  while (myCurIndex < theIndex && myIterator->more()) {
    myCurIndex++;
    myCurID = myIterator->next()->GetID();
  }
  return myCurIndex == theIndex ? myCurID : -1;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

const SMDS_MeshElement* SMESHDS_GroupBase::findInMesh (const int theID) const
{
  SMDSAbs_ElementType aType = GetType();
  const SMDS_MeshElement* aElem = NULL;
  if (aType == SMDSAbs_Node) {
    aElem = GetMesh()->FindNode(theID);
  }
  else if (aType != SMDSAbs_All) {
    aElem = GetMesh()->FindElement(theID);
    if (aElem && aType != aElem->GetType())
      aElem = NULL;
  }
  return aElem;
}

//=============================================================================
/*!
 *  Internal method: resets cached iterator, should be called by ancestors
 *  when they are modified (ex: Add() or Remove() )
 */
//=============================================================================
void SMESHDS_GroupBase::resetIterator()
{
  myCurIndex = 0;
  myCurID = -1;
}

//=======================================================================
//function : Extent
//purpose  : 
//=======================================================================

int SMESHDS_GroupBase::Extent() const
{
  SMDS_ElemIteratorPtr it = GetElements();
  int nb = 0;
  if ( it )
    for ( ; it->more(); it->next() ) 
      nb++;
  return nb;
}

//=======================================================================
//function : IsEmpty
//purpose  : 
//=======================================================================

bool SMESHDS_GroupBase::IsEmpty()
{
  SMDS_ElemIteratorPtr it = GetElements();
  return ( !it || !it->more() );
}

//=======================================================================
//function : Contains
//purpose  : 
//=======================================================================

bool SMESHDS_GroupBase::Contains (const int theID)
{
  if ( SMDS_ElemIteratorPtr it = GetElements() ) {
    while ( it->more() )
      if ( it->next()->GetID() == theID )
        return true;
  }
  return false;
}

//=======================================================================
//function : Contains
//purpose  : 
//=======================================================================

bool SMESHDS_GroupBase::Contains (const SMDS_MeshElement* elem)
{
  if ( elem )
    return Contains( elem->GetID() );
  return false;
}

//=======================================================================
//function : SetType
//purpose  : 
//=======================================================================

void SMESHDS_GroupBase::SetType(SMDSAbs_ElementType theType)
{
  myType = theType;
}

//=======================================================================
//function : SetType
//purpose  : 
//=======================================================================

void SMESHDS_GroupBase::SetColorGroup(int theColorGroup)
{
  int aRed = ( theColorGroup/1000000 );
  int aGreen = ( theColorGroup -aRed*1000000)/1000;
  int aBlue = ( theColorGroup - aRed*1000000 - aGreen*1000 );
  double aR = aRed/255.0;
  double aG = aGreen/255.0;
  double aB = aBlue/255.0;
  if ( aR < 0. || aR > 1. || // PAL19395
       aG < 0. || aG > 1. ||
       aB < 0. || aB > 1. )
// #ifdef _DEBUG_
//     cout << "SMESHDS_GroupBase::SetColorGroup("<<theColorGroup<<"), invalid color ignored"<<endl;
// #endif
    return;
  Quantity_Color aColor( aR, aG, aB, Quantity_TOC_RGB );
  SetColor( aColor );
}
  
//=======================================================================
//function : SetType
//purpose  : 
//=======================================================================

int SMESHDS_GroupBase::GetColorGroup() const
{
  Quantity_Color aColor = GetColor();
  double aRed = aColor.Red();
  double aGreen = aColor.Green();
  double aBlue = aColor.Blue();
  int aR = int( aRed  *255 );
  int aG = int( aGreen*255 );
  int aB = int( aBlue *255 );
  int aRet = (int)(aR*1000000 + aG*1000 + aB);

  return aRet;
}
  
