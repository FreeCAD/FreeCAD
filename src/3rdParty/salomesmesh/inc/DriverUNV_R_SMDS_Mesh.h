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

#ifndef _INCLUDE_DRIVERUNV_R_SMDS_MESH
#define _INCLUDE_DRIVERUNV_R_SMDS_MESH

#include "SMESH_DriverUNV.hxx"

#include "Driver_SMDS_Mesh.h"
#include <map>
#include <string>


class SMDS_Mesh;
class SMDS_MeshGroup;


typedef std::map<SMDS_MeshGroup*, std::string> TGroupNamesMap;
typedef std::map<SMDS_MeshGroup*, int> TGroupIdMap;

typedef std::map<SMDS_MeshGroup*, std::string> TGroupNamesMap;
typedef std::map<SMDS_MeshGroup*, int> TGroupIdMap;

class MESHDRIVERUNV_EXPORT DriverUNV_R_SMDS_Mesh: public Driver_SMDS_Mesh
{
 public:
  DriverUNV_R_SMDS_Mesh():Driver_SMDS_Mesh(),myGroup(0) {};
  ~DriverUNV_R_SMDS_Mesh();
 
  virtual Status Perform();

  const SMDS_MeshGroup* GetGroup()         const { return myGroup;}
  const TGroupNamesMap& GetGroupNamesMap() const { return myGroupNames; }
  const TGroupIdMap&    GetGroupIdMap() const { return myGroupId; }

 private:
  SMDS_MeshGroup* myGroup;
  TGroupNamesMap myGroupNames;
  TGroupIdMap    myGroupId;
};

#endif
