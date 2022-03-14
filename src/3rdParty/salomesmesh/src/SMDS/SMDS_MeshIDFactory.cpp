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
//  File   : SMDS_MeshIDFactory.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#include "SMDS_MeshIDFactory.hxx"
#include "SMDS_Mesh.hxx"
#include "utilities.h"

using namespace std;

//=======================================================================
//function : SMDS_MeshIDFactory
//purpose  : 
//=======================================================================

SMDS_MeshIDFactory::SMDS_MeshIDFactory():myMaxID(0), myMesh(0)
{
}

int SMDS_MeshIDFactory::GetFreeID()
{
        int newid;
        if (myPoolOfID.empty())
        {
            newid = ++myMaxID;
            //MESSAGE("GetFreeID new " << newid);
        }
        else
        {
                set<int>::iterator i = myPoolOfID.begin();
                newid = *i;//myPoolOfID.top();
                myPoolOfID.erase( i );//myPoolOfID.pop();
                //MESSAGE("GetFreeID pool " << newid);
        }
    return newid;
}

//=======================================================================
//function : ReleaseID
//purpose  : 
//=======================================================================
void SMDS_MeshIDFactory::ReleaseID(int ID, int vtkId)
{
  if ( ID > 0 )
  {
    if ( ID < myMaxID )
    {
      myPoolOfID.insert(ID);
    }
    else if ( ID == myMaxID )
    {
      --myMaxID;
      if ( !myPoolOfID.empty() ) // assure that myMaxID is not in myPoolOfID
      {
        set<int>::iterator i = --myPoolOfID.end();
        while ( i != myPoolOfID.begin() && myMaxID == *i ) {
          --myMaxID; --i;
        }
        if ( myMaxID == *i )
          --myMaxID; // begin of myPoolOfID reached
        else
          ++i;
        myPoolOfID.erase( i, myPoolOfID.end() );
      }
    }
  }
}

void SMDS_MeshIDFactory::Clear()
{
        myMaxID = 0;
        myPoolOfID.clear();
}

void SMDS_MeshIDFactory::SetMesh(SMDS_Mesh *mesh)
{
        myMesh = mesh;
}

SMDS_Mesh* SMDS_MeshIDFactory::GetMesh()
{
        return myMesh;
}

void SMDS_MeshIDFactory::emptyPool(int maxId)
{
        MESSAGE("SMDS_MeshIDFactory::emptyPool " << myMaxID << " --> " << maxId);
        myMaxID = maxId;
        myPoolOfID.clear();
}

