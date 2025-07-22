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
//  File   : SMDS_SpacePosition.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//

#include "SMDS_SpacePosition.hxx"

SMDS_SpacePosition* SMDS_SpacePosition::_originPosition = new SMDS_SpacePosition();

SMDS_SpacePosition::SMDS_SpacePosition(double x, double y, double z)
{
}

SMDS_TypeOfPosition SMDS_SpacePosition::GetTypeOfPosition() const
{
  return SMDS_TOP_3DSPACE;
}

SMDS_PositionPtr SMDS_SpacePosition::originSpacePosition()
{
  return _originPosition;
}
