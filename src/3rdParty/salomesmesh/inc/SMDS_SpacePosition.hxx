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

//  SMESH SMDS : implementation of Salome mesh data structure
//  File   : SMDS_SpacePosition.hxx
//  Module : SMESH
//
#ifndef _SMDS_SpacePosition_HeaderFile
#define _SMDS_SpacePosition_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_Position.hxx"

class SMDS_EXPORT SMDS_SpacePosition:public SMDS_Position
{

public:
  SMDS_SpacePosition(double x=0, double y=0, double z=0);
  virtual SMDS_TypeOfPosition GetTypeOfPosition() const;
  static SMDS_PositionPtr originSpacePosition();
private:
  static SMDS_SpacePosition* _originPosition;
};

#endif
