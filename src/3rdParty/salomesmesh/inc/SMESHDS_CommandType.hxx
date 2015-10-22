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
//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_CommandType.hxx
//  Module : SMESH
//
#ifndef _SMESHDS_CommandType_HeaderFile
#define _SMESHDS_CommandType_HeaderFile

//#include <Standard_PrimitiveTypes.hxx>

enum SMESHDS_CommandType { 
  SMESHDS_AddNode,
  SMESHDS_AddEdge,
  SMESHDS_AddTriangle,
  SMESHDS_AddQuadrangle,
  SMESHDS_AddPolygon,
  SMESHDS_AddTetrahedron,
  SMESHDS_AddPyramid,
  SMESHDS_AddPrism,
  SMESHDS_AddHexahedron,
  SMESHDS_AddPolyhedron,
  SMESHDS_RemoveNode,
  SMESHDS_RemoveElement,
  SMESHDS_MoveNode,
  SMESHDS_ChangeElementNodes,
  SMESHDS_ChangePolyhedronNodes,
  SMESHDS_Renumber,
  SMESHDS_ClearAll,
  // special types for quadratic elements
  SMESHDS_AddQuadEdge,
  SMESHDS_AddQuadTriangle,
  SMESHDS_AddQuadQuadrangle,
  SMESHDS_AddQuadTetrahedron,
  SMESHDS_AddQuadPyramid,
  SMESHDS_AddQuadPentahedron,
  SMESHDS_AddQuadHexahedron
};


#endif
