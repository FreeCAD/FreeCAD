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

//  File      : SMESH_Type.h
//  Created   : Mon Jun 03 15:14:15 2002
//  Author    : Nicolas REJNERI
//  Project   : SALOME
//  Module    : SMESH
//
#ifndef SMESH_TYPE_HEADER
#define SMESH_TYPE_HEADER

#ifdef WIN32
 #if defined SMESHFILTERSSELECTION_EXPORTS || defined SMESHFiltersSelection_EXPORTS
  #define SMESHFILTERSSELECTION_EXPORT __declspec( dllexport )
 #else
  #define SMESHFILTERSSELECTION_EXPORT __declspec( dllimport )
 #endif
#else
 #define SMESHFILTERSSELECTION_EXPORT
#endif

namespace SMESH {
  enum MeshObjectType {
    HYPOTHESIS,
    ALGORITHM,
    MESH,
    SUBMESH,
    MESHorSUBMESH,
    SUBMESH_VERTEX,
    SUBMESH_EDGE,
    SUBMESH_FACE,
    SUBMESH_SOLID,
    SUBMESH_COMPOUND,
    GROUP,
    GROUP_NODE,
    GROUP_EDGE,
    GROUP_FACE,
    GROUP_VOLUME,
    GROUP_0D,
    GROUP_BALL,
    COMPONENT,
    IDSOURCE,
    IDSOURCE_EDGE, // IDSource including edges
    IDSOURCE_FACE,
    IDSOURCE_VOLUME
  };
}
#endif
