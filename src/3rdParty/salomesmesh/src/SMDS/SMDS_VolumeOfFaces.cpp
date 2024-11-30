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
//  File   : SMDS_VolumeOfFaces.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_VolumeOfFaces.hxx"
#include "SMDS_IteratorOfElements.hxx"
#include "utilities.h"

using namespace std;

//=======================================================================
//function : Print
//purpose  : 
//=======================================================================

void SMDS_VolumeOfFaces::Print(ostream & OS) const
{
        OS << "volume <" << GetID() << "> : ";
        int i;
        for (i = 0; i < NbFaces()-1; ++i) OS << myFaces[i] << ",";
        OS << myFaces[i]<< ") " << endl;
}


int SMDS_VolumeOfFaces::NbFaces() const
{
        return myNbFaces;
}

class SMDS_VolumeOfFaces_MyIterator:public SMDS_ElemIterator
{
  const SMDS_MeshFace* const *mySet;
  int myLength;
  int index;
 public:
  SMDS_VolumeOfFaces_MyIterator(const SMDS_MeshFace* const *s, int l):
    mySet(s),myLength(l),index(0) {}

  bool more()
  {
    return index<myLength;
  }

  const SMDS_MeshElement* next()
  {
    index++;
    return mySet[index-1];
  }
};

SMDS_ElemIteratorPtr SMDS_VolumeOfFaces::
        elementsIterator(SMDSAbs_ElementType type) const
{
  switch(type)
  {
  case SMDSAbs_Volume:
    return SMDS_MeshElement::elementsIterator(SMDSAbs_Volume);
  case SMDSAbs_Face:
    return SMDS_ElemIteratorPtr(new SMDS_VolumeOfFaces_MyIterator(myFaces,myNbFaces));
  default:
    return SMDS_ElemIteratorPtr
      (new SMDS_IteratorOfElements
       (this,type,SMDS_ElemIteratorPtr
        (new SMDS_VolumeOfFaces_MyIterator(myFaces,myNbFaces))));
  }
}

SMDS_VolumeOfFaces::SMDS_VolumeOfFaces(const SMDS_MeshFace * face1,
                                       const SMDS_MeshFace * face2,
                                       const SMDS_MeshFace * face3,
                                       const SMDS_MeshFace * face4)
{
  //MESSAGE("****************************************************** SMDS_VolumeOfFaces");
        myNbFaces = 4;
        myFaces[0]=face1;
        myFaces[1]=face2;
        myFaces[2]=face3;
        myFaces[3]=face4;
        myFaces[4]=0;
        myFaces[5]=0;
}

SMDS_VolumeOfFaces::SMDS_VolumeOfFaces(const SMDS_MeshFace * face1,
                                       const SMDS_MeshFace * face2,
                                       const SMDS_MeshFace * face3,
                                       const SMDS_MeshFace * face4,
                                       const SMDS_MeshFace * face5)
{
  //MESSAGE("****************************************************** SMDS_VolumeOfFaces");
        myNbFaces = 5;
        myFaces[0]=face1;
        myFaces[1]=face2;
        myFaces[2]=face3;
        myFaces[3]=face4;
        myFaces[4]=face5;
        myFaces[5]=0;
}

SMDS_VolumeOfFaces::SMDS_VolumeOfFaces(const SMDS_MeshFace * face1,
                                       const SMDS_MeshFace * face2,
                                       const SMDS_MeshFace * face3,
                                       const SMDS_MeshFace * face4,
                                       const SMDS_MeshFace * face5,
                                       const SMDS_MeshFace * face6)
{
  //MESSAGE("****************************************************** SMDS_VolumeOfFaces");
        myNbFaces = 6;
        myFaces[0]=face1;
        myFaces[1]=face2;
        myFaces[2]=face3;
        myFaces[3]=face4;
        myFaces[4]=face5;
        myFaces[5]=face6;
}

SMDSAbs_EntityType SMDS_VolumeOfFaces::GetEntityType() const
{
  SMDSAbs_EntityType aType = SMDSEntity_Tetra;
  switch(myNbFaces)
  {
  case 4: aType = SMDSEntity_Tetra;   break;
  case 5: aType = SMDSEntity_Pyramid; break;
  case 6: aType = SMDSEntity_Penta;   break;
  case 8:
  default: aType = SMDSEntity_Hexa;    break;
  }
  return aType;
}

SMDSAbs_GeometryType SMDS_VolumeOfFaces::GetGeomType() const
{
  SMDSAbs_GeometryType aType = SMDSGeom_NONE;
  switch(myNbFaces)
  {
  case 4: aType = SMDSGeom_TETRA;   break;
  case 5: aType = SMDSGeom_PYRAMID; break;
  case 6: aType = SMDSGeom_PENTA;   break;
  case 8:
  default: aType = SMDSGeom_HEXA;   break;
  }
  return aType;
}
