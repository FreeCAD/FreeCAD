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
//  File   : SMDS_MeshElement.hxx
//  Module : SMESH
//  Created:   12.01.05 18:02:52
//  Author:    Michael Sazonov
//
#ifndef SMDS_ElemIterator_HeaderFile
#define SMDS_ElemIterator_HeaderFile

#include "SMDS_Iterator.hxx"
#include <boost/shared_ptr.hpp>

class SMDS_MeshElement;
class SMDS_MeshNode;
class SMDS_Mesh0DElement;
class SMDS_MeshEdge;
class SMDS_MeshFace;
class SMDS_MeshVolume;

typedef SMDS_Iterator<const SMDS_MeshElement *> SMDS_ElemIterator;
typedef boost::shared_ptr<SMDS_Iterator<const SMDS_MeshElement *> > SMDS_ElemIteratorPtr;

typedef SMDS_Iterator<const SMDS_MeshNode *> SMDS_NodeIterator;
typedef boost::shared_ptr<SMDS_Iterator<const SMDS_MeshNode *> > SMDS_NodeIteratorPtr;

typedef SMDS_Iterator<const SMDS_Mesh0DElement *> SMDS_0DElementIterator;
typedef boost::shared_ptr<SMDS_Iterator<const SMDS_Mesh0DElement *> > SMDS_0DElementIteratorPtr;

typedef SMDS_Iterator<const SMDS_MeshEdge *> SMDS_EdgeIterator;
typedef boost::shared_ptr<SMDS_Iterator<const SMDS_MeshEdge *> > SMDS_EdgeIteratorPtr;

typedef SMDS_Iterator<const SMDS_MeshFace *> SMDS_FaceIterator;
typedef boost::shared_ptr<SMDS_Iterator<const SMDS_MeshFace *> > SMDS_FaceIteratorPtr;

typedef SMDS_Iterator<const SMDS_MeshVolume *> SMDS_VolumeIterator;
typedef boost::shared_ptr<SMDS_Iterator<const SMDS_MeshVolume *> > SMDS_VolumeIteratorPtr;

#endif
