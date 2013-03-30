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
//  File   : Mesh_Reader.h
//  Module : SMESH
//
#ifndef _INCLUDE_DRIVER_MESH
#define _INCLUDE_DRIVER_MESH

#include <string>

#ifdef WNT
 #if defined MESHDRIVER_EXPORTS
  #define MESHDRIVER_EXPORT __declspec( dllexport )
 #else
  #define MESHDRIVER_EXPORT __declspec( dllimport )
 #endif
#else
 #define MESHDRIVER_EXPORT
#endif

class MESHDRIVER_EXPORT Driver_Mesh
{
 public:
  Driver_Mesh();
  virtual ~Driver_Mesh(){}

  enum Status {
    DRS_OK,
    DRS_EMPTY,          // a file contains no mesh with the given name
    DRS_WARN_RENUMBER,  // a file has overlapped ranges of element numbers,
                        // so the numbers from the file are ignored
    DRS_WARN_SKIP_ELEM, // some elements were skipped due to incorrect file data
    DRS_FAIL            // general failure (exception etc.)
  };

  virtual Status Perform() = 0;
  void SetMeshId(int theMeshId);
  void SetFile(const std::string& theFileName);

 protected:
  std::string myFile;
  int myMeshId;

};

#endif
