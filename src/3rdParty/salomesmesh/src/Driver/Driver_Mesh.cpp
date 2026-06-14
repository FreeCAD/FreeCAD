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

//  SMESH Driver : implementation of driver for reading and writing      
//  File   : Mesh_Reader.cxx
//  Module : SMESH
//
#include "Driver_Mesh.h"

#include "SMESH_Comment.hxx"

#include <utilities.h>

using namespace std;

Driver_Mesh::Driver_Mesh():
  myFile(""),
  myMeshId(-1),
  myStatus( DRS_OK )
{}


void Driver_Mesh::SetMeshId(int theMeshId)
{
  myMeshId = theMeshId;
}

void Driver_Mesh::SetMeshName(const std::string& theMeshName)
{
  myMeshName = theMeshName;
}

std::string Driver_Mesh::GetMeshName() const
{
  return myMeshName;
}


void Driver_Mesh::SetFile(const std::string& theFileName)
{
  myFile = theFileName;
}


//================================================================================
/*!
 * \brief Stores an error message
 *
 * We consider an error fatal if none mesh can be read
 */
//================================================================================

Driver_Mesh::Status Driver_Mesh::addMessage(const std::string& msg,
                                            const bool         isFatal/*=false*/)
{
  if ( isFatal )
    myErrorMessages.clear(); // warnings are useless if a fatal error encounters

  myErrorMessages.push_back( msg );

  MESSAGE(msg);
#ifdef _DEBUG_
  cout << msg << endl;
#endif
  return ( myStatus = isFatal ? DRS_FAIL : DRS_WARN_SKIP_ELEM );
}

//================================================================================
/*!
 * \brief Return a structure containing description of errors
 */
//================================================================================

SMESH_ComputeErrorPtr Driver_Mesh::GetError()
{
  SMESH_Comment msg;
  for ( size_t i = 0; i < myErrorMessages.size(); ++i )
  {
    msg << myErrorMessages[i];
    if ( i+1 < myErrorMessages.size() )
      msg << "\n";
  }
  return SMESH_ComputeError::New( myStatus == DRS_OK ? int(COMPERR_OK) : int(myStatus), msg );
}
