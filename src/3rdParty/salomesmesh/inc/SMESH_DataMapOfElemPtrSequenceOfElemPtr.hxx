//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
// File:      SMESH_DataMapOfElemPtrSequenceOfElemPtr.hxx
// Created:   26.09.05 17:41:10
// Author:    Sergey KUUL
//
#ifndef SMESH_DataMapOfElemPtrSequenceOfElemPtr_HeaderFile
#define SMESH_DataMapOfElemPtrSequenceOfElemPtr_HeaderFile

#include "SMESH_SMESH.hxx"

#include <SMESH_SequenceOfElemPtr.hxx>

#include <NCollection_DefineDataMap.hxx>

SMESH_EXPORT 
inline Standard_Integer HashCode(SMDS_MeshElementPtr theElem,
                                 const Standard_Integer theUpper)
{
  void* anElem = (void*) theElem;
  return HashCode(anElem,theUpper);
}

SMESH_EXPORT 
inline Standard_Boolean IsEqual(SMDS_MeshElementPtr theOne,
                                SMDS_MeshElementPtr theTwo)
{
  return theOne == theTwo;
}

DEFINE_BASECOLLECTION (SMESH_BaseCollectionSequenceOfElemPtr, SMESH_SequenceOfElemPtr)
DEFINE_DATAMAP (SMESH_DataMapOfElemPtrSequenceOfElemPtr,
                SMESH_BaseCollectionSequenceOfElemPtr,
                SMDS_MeshElementPtr, SMESH_SequenceOfElemPtr)
#endif 
