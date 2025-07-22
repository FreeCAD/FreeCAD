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
//  File   : Mesh_Reader.h
//  Module : SMESH
//
#ifndef _INCLUDE_DRIVER_MESH
#define _INCLUDE_DRIVER_MESH

#include "SMESH_ComputeError.hxx"

#include <string>
#include <vector>

#ifdef WIN32
 #if defined MESHDRIVER_EXPORTS || defined MeshDriver_EXPORTS
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
    DRS_WARN_DESCENDING, // some elements were skipped due to descending connectivity
    DRS_FAIL            // general failure (exception etc.)
  };

  void                SetMeshId(int theMeshId);
  virtual void        SetFile(const std::string& theFileName);
  virtual void        SetMeshName(const std::string& theMeshName);
  virtual std::string GetMeshName() const;

  virtual void        SetOption(const std::string& optionName,
                                const std::string& optionValue) {}

  virtual Status Perform() = 0;

  virtual SMESH_ComputeErrorPtr GetError();

 protected:
  std::string myFile;
  std::string myMeshName;
  int         myMeshId;

  Status addMessage(const std::string& msg, const bool isFatal=false);
  std::vector< std::string > myErrorMessages;
  Status                     myStatus;
};

#endif
