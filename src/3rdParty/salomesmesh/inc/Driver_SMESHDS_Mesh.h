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

#ifndef _INCLUDE_DRIVER_SMESHDS_MESH
#define _INCLUDE_DRIVER_SMESHDS_MESH

#include "Driver_Mesh.h"

class SMESHDS_Mesh;

class MESHDRIVER_EXPORT Driver_SMESHDS_Mesh: public Driver_Mesh
{
 public:
  Driver_SMESHDS_Mesh();
  void SetMesh(SMESHDS_Mesh *theMesh);
  
 protected:
  SMESHDS_Mesh *myMesh;
};

#endif
