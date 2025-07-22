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
//  File   : SMESHDS_GroupOnGeom.cxx
//  Module : SMESH
//
#include "SMESHDS_GroupOnGeom.hxx"
#include "SMESHDS_Mesh.hxx"
#include "utilities.h"

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESHDS_GroupOnGeom::SMESHDS_GroupOnGeom (const int                 theID,
                                          const SMESHDS_Mesh*       theMesh,
                                          const SMDSAbs_ElementType theType,
                                          const TopoDS_Shape&       theShape)
     : SMESHDS_GroupBase(theID,theMesh,theType)
{
  SetShape( theShape );
}

void SMESHDS_GroupOnGeom::SetShape( const TopoDS_Shape& theShape)
{
  SMESHDS_Mesh* aMesh = const_cast<SMESHDS_Mesh*>( GetMesh() );
  mySubMesh = aMesh->MeshElements( aMesh->AddCompoundSubmesh( theShape ));
  myShape   = theShape;
}

// =====================
// class MyGroupIterator
// =====================

class MyIterator: public SMDS_ElemIterator
{
  SMDSAbs_ElementType     myType;
  SMDS_ElemIteratorPtr    myElemIt;
  SMDS_NodeIteratorPtr    myNodeIt;
  const SMDS_MeshElement* myElem;
 public:
  MyIterator(SMDSAbs_ElementType type, const SMESHDS_SubMesh* subMesh)
    : myType(type), myElem(0)
  {
    if ( subMesh ) {
      if ( myType == SMDSAbs_Node )
        myNodeIt = subMesh->GetNodes();
      else {
        myElemIt = subMesh->GetElements();
        next();
      }
    }
  }
  bool more()
  {
    if ( myType == SMDSAbs_Node && myNodeIt )
      return myNodeIt->more();
    return ( myElem != 0 );
  }
  const SMDS_MeshElement* next()
  {
    if ( myType == SMDSAbs_Node && myNodeIt )
      return myNodeIt->next();
    const SMDS_MeshElement* res = myElem;
    myElem = 0;
    while ( myElemIt && myElemIt->more() ) {
      myElem = myElemIt->next();
      if ( myElem && myElem->GetType() == myType )
        break;
      else
        myElem = 0;
    }
    return res;
  }
};

//=======================================================================
//function : GetElements
//purpose  : 
//=======================================================================

SMDS_ElemIteratorPtr SMESHDS_GroupOnGeom::GetElements() const
{
  return SMDS_ElemIteratorPtr( new MyIterator ( GetType(), mySubMesh ));
}

//=======================================================================
//function : Contains
//purpose  : 
//=======================================================================

bool SMESHDS_GroupOnGeom::Contains (const int theID)
{
  return mySubMesh->Contains( findInMesh( theID ));
}

//=======================================================================
//function : Contains
//purpose  : 
//=======================================================================

bool SMESHDS_GroupOnGeom::Contains (const SMDS_MeshElement* elem)
{
  return mySubMesh->Contains( elem );
}

//================================================================================
/*!
 * \brief Return a value allowing to find out if a group has changed or not
 */
//================================================================================

VTK_MTIME_TYPE SMESHDS_GroupOnGeom::GetTic() const
{
  return GetMesh()->GetMTime();
}

