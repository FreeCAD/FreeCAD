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
//  SMESH SMDS : implementaion of Salome mesh data structure
//  File   : SMDSAbs_ElementType.hxx
//  Module : SMESH
//
#ifndef _SMDSAbs_ElementType_HeaderFile
#define _SMDSAbs_ElementType_HeaderFile

///////////////////////////////////////////////////////////////////////////////
/// Type (node, edge, face or volume) of elements
///////////////////////////////////////////////////////////////////////////////
enum SMDSAbs_ElementType
{
	SMDSAbs_All,
	SMDSAbs_Node,
	SMDSAbs_Edge,
	SMDSAbs_Face,
	SMDSAbs_Volume,
        SMDSAbs_NbElementTypes
};

/*! enumeration for element geometry type */
enum SMDSAbs_GeometryType
{
  // 0D element
  SMDSGeom_POINT,
  // 1D element
  SMDSGeom_EDGE,
  // 2D element
  SMDSGeom_TRIANGLE,
  SMDSGeom_QUADRANGLE,
  SMDSGeom_POLYGON,
  // 3D element
  SMDSGeom_TETRA,
  SMDSGeom_PYRAMID,
  SMDSGeom_HEXA,
  SMDSGeom_PENTA,
  SMDSGeom_POLYHEDRA,
};


enum SMDSAbs_ElementOrder {
  ORDER_ANY,          /*! entities of any order */
  ORDER_LINEAR,       /*! entities of 1st order */
  ORDER_QUADRATIC     /*! entities of 2nd order */
};

#endif
