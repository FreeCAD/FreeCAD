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
//
#ifndef _SMDS_FaceOfEdges_HeaderFile
#define _SMDS_FaceOfEdges_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshFace.hxx"
#include "SMDS_MeshEdge.hxx"
#include "SMDS_Iterator.hxx"

#include <iostream>


class SMDS_EXPORT SMDS_FaceOfEdges:public SMDS_MeshFace
{
  public:
        void Print(std::ostream & OS) const;
        SMDS_FaceOfEdges(const SMDS_MeshEdge* edge1,
                         const SMDS_MeshEdge* edge2,
                         const SMDS_MeshEdge* edge3);
        SMDS_FaceOfEdges(const SMDS_MeshEdge* edge1,
                         const SMDS_MeshEdge* edge2,
                         const SMDS_MeshEdge* edge3,
                         const SMDS_MeshEdge* edge4);
                
        virtual SMDSAbs_ElementType  GetType() const;
        virtual SMDSAbs_EntityType   GetEntityType() const;
        virtual SMDSAbs_GeometryType GetGeomType() const;
        virtual bool ChangeNodes(const SMDS_MeshNode* nodes[],
                                 const int            nbNodes) {return false;}
        virtual int NbNodes() const;
        virtual int NbEdges() const;
        virtual int NbFaces() const;
        virtual const SMDS_MeshNode* GetNode(const int ind) const;

  protected:
        virtual SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType type) const;

  private:
        const SMDS_MeshEdge* myEdges[4];
        int                  myNbEdges;

};

#endif
