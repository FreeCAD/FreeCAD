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

//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_Document.cxx
//  Author : Yves FRICAUD, OCC
//  Module : SMESH
//  $Header: 
//
#include "SMESHDS_Document.hxx"
#include "utilities.h"

using namespace std;

//=======================================================================
//function : Create
//purpose  : 
//=======================================================================
SMESHDS_Document::SMESHDS_Document(int UserID):myUserID(UserID)
{
}

//=======================================================================
//function : Destructor
//purpose  : 
//=======================================================================

SMESHDS_Document::~SMESHDS_Document()
{
  InitMeshesIterator();
  while ( MoreMesh() )
    delete NextMesh();
}

//=======================================================================
//function : NewMesh
//purpose  : 
//=======================================================================
SMESHDS_Mesh * SMESHDS_Document::NewMesh(bool theIsEmbeddedMode, int MeshID)
{
  std::map<int,SMESHDS_Mesh*>::iterator i_m =
    myMeshes.insert( make_pair( MeshID, (SMESHDS_Mesh*)0 )).first;
  if ( i_m->second )
    throw SALOME_Exception("SMESHDS_Document::NewMesh(): ID of existing mesh given");
  SMESHDS_Mesh *aNewMesh = new SMESHDS_Mesh(MeshID,theIsEmbeddedMode);
  i_m->second = aNewMesh;
  return aNewMesh;
}

//=======================================================================
//function : GetMesh
//purpose  :
//=======================================================================
SMESHDS_Mesh *SMESHDS_Document::GetMesh(int MeshID)
{
  map<int,SMESHDS_Mesh*>::iterator it=myMeshes.find(MeshID);
  if (it==myMeshes.end())
  {
    MESSAGE("SMESHDS_Document::GetMesh : ID not found");
    return NULL;
  }
  else return (*it).second;
}

//=======================================================================
//function : RemoveMesh
//purpose  :
//=======================================================================
void SMESHDS_Document::RemoveMesh(int MeshID)
{
  map<int,SMESHDS_Mesh*>::iterator it=myMeshes.find(MeshID);
  if (it!=myMeshes.end())
    myMeshes.erase(it);
}

//=======================================================================
//function : AddHypothesis
//purpose  : 
//=======================================================================
void SMESHDS_Document::AddHypothesis(SMESHDS_Hypothesis * H)
{
  myHypothesis[H->GetID()]=H;
}

//=======================================================================
//function : GetHypothesis
//purpose  : 
//=======================================================================
SMESHDS_Hypothesis * SMESHDS_Document::GetHypothesis(int HypID)
{
  map<int,SMESHDS_Hypothesis*>::iterator it=myHypothesis.find(HypID);
  if (it==myHypothesis.end())
  {
    MESSAGE("SMESHDS_Document::GetHypothesis : ID not found");
    return NULL;
  }
  else return (*it).second;
}

//=======================================================================
//function : RemoveHypothesis
//purpose  :
//=======================================================================
void SMESHDS_Document::RemoveHypothesis(int HypID)
{
  map<int,SMESHDS_Hypothesis*>::iterator it=myHypothesis.find(HypID);
  if (it==myHypothesis.end())
    MESSAGE("SMESHDS_Document::RemoveHypothesis : ID not found");
  myHypothesis.erase(it);
}

//=======================================================================
//function : NbMeshes
//purpose  :
//=======================================================================
int SMESHDS_Document::NbMeshes()
{
  return myMeshes.size();
}

//=======================================================================
//function : NbHypothesis
//purpose  :
//=======================================================================
int SMESHDS_Document::NbHypothesis()
{
  return myHypothesis.size();
}

//=======================================================================
//function : InitMeshesIterator
//purpose  :
//=======================================================================
void SMESHDS_Document::InitMeshesIterator()
{
  myMeshesIt=myMeshes.begin();
}

//=======================================================================
//function : NextMesh
//purpose  :
//=======================================================================
SMESHDS_Mesh * SMESHDS_Document::NextMesh()
{
  SMESHDS_Mesh * toReturn=(*myMeshesIt).second;
  myMeshesIt++;
  return toReturn;
}

//=======================================================================
//function : MoreMesh
//purpose  :
//=======================================================================
bool SMESHDS_Document::MoreMesh()
{
  return myMeshesIt!=myMeshes.end();
}

//=======================================================================
//function : InitHypothesisIterator
//purpose  :
//=======================================================================
void SMESHDS_Document::InitHypothesisIterator()
{
  myHypothesisIt=myHypothesis.begin();
}

//=======================================================================
//function : NextMesh
//purpose  :
//=======================================================================
SMESHDS_Hypothesis * SMESHDS_Document::NextHypothesis()
{
  SMESHDS_Hypothesis * toReturn=(*myHypothesisIt).second;
  myHypothesisIt++;
  return toReturn;
}

//=======================================================================
//function : MoreMesh
//purpose  :
//=======================================================================
bool SMESHDS_Document::MoreHypothesis()
{
  return myHypothesisIt!=myHypothesis.end();
}
