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
// File:      SMESH_IndexedDataMapOfShapeIndexedMapOfShape.hxx
// Created:   20.09.05 09:51:12
// Author:    Sergey KUUL
//
#ifndef SMESH_IndexedMapOfShape_HeaderFile
#define SMESH_IndexedMapOfShape_HeaderFile

#include "SMESH_SMESH.hxx"

#include "SMESHDS_DataMapOfShape.hxx"

#ifndef __BORLANDC__
#include <NCollection_DefineIndexedMap.hxx>
#else
#include <SMESH_DefineIndexedMap.hxx>
#endif

#include <TopoDS_Shape.hxx>

///  Class SMESH_IndexedMapOfShape

DEFINE_BASECOLLECTION (SMESH_BaseCollectionShape, TopoDS_Shape)

#ifndef __BORLANDC__
DEFINE_INDEXEDMAP (SMESH_IndexedMapOfShape, SMESH_BaseCollectionShape, TopoDS_Shape)
#else
SMESH_DEFINE_INDEXEDMAP (SMESH_IndexedMapOfShape, SMESH_BaseCollectionShape, TopoDS_Shape)
#endif



#endif 

#ifndef SMESH_IndexedDataMapOfShapeIndexedMapOfShape_HeaderFile
#define SMESH_IndexedDataMapOfShapeIndexedMapOfShape_HeaderFile

#include <NCollection_DefineIndexedDataMap.hxx>

///  Class SMESH_IndexedDataMapOfShapeIndexedMapOfShape

DEFINE_BASECOLLECTION (SMESH_BaseCollectionIndexedMapOfShape, SMESH_IndexedMapOfShape)
DEFINE_INDEXEDDATAMAP (SMESH_IndexedDataMapOfShapeIndexedMapOfShape,
                       SMESH_BaseCollectionIndexedMapOfShape, TopoDS_Shape,
                       SMESH_IndexedMapOfShape)
#endif 
