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
//  File   : SMDS_SpacePosition.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#include "SMDS_SpacePosition.hxx"

//=======================================================================
//function : SMDS_SpacePosition
//purpose  : 
//=======================================================================

SMDS_SpacePosition::SMDS_SpacePosition(double x, double y, double z):
	SMDS_Position(0)
{
	myCoords[0]=x;
	myCoords[1]=y;
	myCoords[2]=z;
}

/**
*/
SMDS_TypeOfPosition SMDS_SpacePosition::GetTypeOfPosition() const
{
	return SMDS_TOP_3DSPACE;
}

const double * SMDS_SpacePosition::Coords() const
{
	return myCoords;
}

SMDS_PositionPtr SMDS_SpacePosition::originSpacePosition()
{
  static SMDS_PositionPtr staticpos (new SMDS_SpacePosition());
  return staticpos;
}
