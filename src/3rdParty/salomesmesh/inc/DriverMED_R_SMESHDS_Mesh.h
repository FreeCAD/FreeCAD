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

//  SMESH DriverMED : driver to read and write 'med' files
//  File   : DriverMED_R_SMESHDS_Mesh.h
//  Module : SMESH
//
#ifndef _INCLUDE_DRIVERMED_R_SMESHDS_MESH
#define _INCLUDE_DRIVERMED_R_SMESHDS_MESH

#include "SMESH_DriverMED.hxx"

#include "DriverMED.hxx"
#include "Driver_SMESHDS_Mesh.h"
#include "SMDSAbs_ElementType.hxx"

#include <list>
#include <map>

class SMESHDS_Mesh;
class SMESHDS_Group;
class SMESHDS_SubMesh;

typedef std::pair< std::string, SMDSAbs_ElementType > TNameAndType;

class MESHDRIVERMED_EXPORT DriverMED_R_SMESHDS_Mesh: public Driver_SMESHDS_Mesh
{
 public:
  virtual Status Perform();

  std::list< TNameAndType > GetGroupNamesAndTypes();
  void GetGroup(SMESHDS_Group* theGroup);
  void CreateAllSubMeshes();
  void GetSubMesh(SMESHDS_SubMesh* theSubMesh, const int theId);

  std::list<std::string> GetMeshNames(Status& theStatus);
  void SetMeshName(std::string theMeshName);

 private:
  std::string myMeshName;
  std::map<int, DriverMED_FamilyPtr> myFamilies;

};

#endif
