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

//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_Document.hxx
//  Module : SMESH
//
#ifndef _SMESHDS_Document_HeaderFile
#define _SMESHDS_Document_HeaderFile

#include "SMESH_SMESHDS.hxx"

#include "SMESHDS_Mesh.hxx"
#include "SMESHDS_Hypothesis.hxx"
#include <map>


class SMESHDS_EXPORT SMESHDS_Document
{
  public:
        SMESHDS_Document(int UserID);
        ~SMESHDS_Document();
        SMESHDS_Mesh *       NewMesh(bool theIsEmbeddedMode, int MeshID);
        void                 RemoveMesh(int MeshID);
        SMESHDS_Mesh *       GetMesh(int MeshID);
        void                 AddHypothesis(SMESHDS_Hypothesis * H);
        void                 RemoveHypothesis(int HypID);
        SMESHDS_Hypothesis * GetHypothesis(int HypID);
        int                  NbMeshes();
        int                  NbHypothesis();
        void                 InitMeshesIterator();
        SMESHDS_Mesh *       NextMesh();
        bool                 MoreMesh();        
        void                 InitHypothesisIterator();
        SMESHDS_Hypothesis * NextHypothesis();
        bool                 MoreHypothesis();  

  private:
        int                                         myUserID;
        std::map<int,SMESHDS_Mesh*>                 myMeshes;
        std::map<int,SMESHDS_Hypothesis*>           myHypothesis;
        std::map<int,SMESHDS_Mesh*>::iterator       myMeshesIt;
        std::map<int,SMESHDS_Hypothesis*>::iterator myHypothesisIt;
};

#endif
