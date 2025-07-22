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
//  File   : SMDS_VolumeOfFaces.hxx
//  Module : SMESH
//
#ifndef _SMDS_VolumeOfFaces_HeaderFile
#define _SMDS_VolumeOfFaces_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshVolume.hxx"
#include "SMDS_MeshFace.hxx"
#include "SMDS_Iterator.hxx"
#include <iostream>


class SMDS_EXPORT SMDS_VolumeOfFaces:public SMDS_MeshVolume
{
        
  public:
        SMDS_VolumeOfFaces(const SMDS_MeshFace * face1,
                           const SMDS_MeshFace * face2,
                           const SMDS_MeshFace * face3,
                           const SMDS_MeshFace * face4);
        SMDS_VolumeOfFaces(const SMDS_MeshFace * face1,
                           const SMDS_MeshFace * face2,
                           const SMDS_MeshFace * face3,
                           const SMDS_MeshFace * face4,
                           const SMDS_MeshFace * face5);
        SMDS_VolumeOfFaces(const SMDS_MeshFace * face1,
                           const SMDS_MeshFace * face2,
                           const SMDS_MeshFace * face3,
                           const SMDS_MeshFace * face4,
                           const SMDS_MeshFace * face5,
                           const SMDS_MeshFace * face6);

        virtual SMDSAbs_EntityType GetEntityType() const;
        virtual SMDSAbs_GeometryType GetGeomType() const;
        virtual bool ChangeNodes(const SMDS_MeshNode* nodes[],
                                 const int            nbNodes) {return false;}
        virtual void Print(std::ostream & OS) const;

        virtual int NbFaces() const;

  protected:
        virtual SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType type) const;
        const SMDS_MeshFace * myFaces[6];
        int                   myNbFaces;
};
#endif
