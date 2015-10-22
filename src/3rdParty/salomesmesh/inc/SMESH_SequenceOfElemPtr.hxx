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
// File:      SMESH_SequenceOfElemPtr.hxx
// Created:   26.09.05 17:41:10
// Author:    Sergey KUUL
//
#ifndef SMESH_SequenceOfElemPtr_HeaderFile
#define SMESH_SequenceOfElemPtr_HeaderFile

#include "SMESH_SMESH.hxx"

#if OCC_VERSION_HEX >= 0x060703
#include <NCollection_Sequence.hxx>
#else
#include <NCollection_DefineSequence.hxx>
#endif

#include <SMDS_MeshElement.hxx>

typedef const SMDS_MeshElement* SMDS_MeshElementPtr;

#if OCC_VERSION_HEX >= 0x060703
typedef NCollection_Sequence<SMDS_MeshElementPtr> SMESH_SequenceOfElemPtr;
#else
DEFINE_BASECOLLECTION (SMESH_BaseCollectionElemPtr, SMDS_MeshElementPtr)
DEFINE_SEQUENCE (SMESH_SequenceOfElemPtr, SMESH_BaseCollectionElemPtr, SMDS_MeshElementPtr)
#endif

#endif 
