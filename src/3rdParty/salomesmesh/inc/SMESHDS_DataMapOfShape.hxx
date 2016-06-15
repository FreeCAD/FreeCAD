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

// File:      SMESHDS_DataMapOfShape.hxx
// Created:   20.09.05 09:51:12
// Author:    Alexander BORODIN
//
#ifndef SMESHDS_DataMapOfShape_HeaderFile
#define SMESHDS_DataMapOfShape_HeaderFile

#include <TopoDS_Shape.hxx>

/*
 * This method needed for instance NCollection_DataMap with TopoDS_Shape as key
 */
struct SMESHDS_Hasher
{
  static inline Standard_Boolean IsEqual(const TopoDS_Shape& S1,
                                         const TopoDS_Shape& S2)
  {
    return S1.IsSame(S2);
  }
  static inline Standard_Integer HashCode(const TopoDS_Shape& S,
                                          const Standard_Integer Upper)
  {
    return ::HashCode( S, Upper);
  }
};


#endif
