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
//  SMESH Driver : implementaion of driver for reading and writing	
//  File   : Mesh_Reader.cxx
//  Module : SMESH
//
#include "Driver_Mesh.h"

using namespace std;

Driver_Mesh::Driver_Mesh():
  myFile(""),
  myMeshId(-1)
{}


void Driver_Mesh::SetMeshId(int theMeshId)
{
  myMeshId = theMeshId;
}


void Driver_Mesh::SetFile(const std::string& theFileName)
{
  myFile = theFileName;
}
