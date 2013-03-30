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
//  File   : SMDS_EdgePosition.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#include "SMDS_EdgePosition.hxx"

#include "utilities.h"

using namespace std;

//=======================================================================
//function : SMDS_EdgePosition
//purpose  : 
//=======================================================================

SMDS_EdgePosition::SMDS_EdgePosition(const int aEdgeId,
	const double aUParam):SMDS_Position(aEdgeId), myUParameter(aUParam)
{
}

//=======================================================================
//function : Coords
//purpose  : 
//=======================================================================

const double *SMDS_EdgePosition::Coords() const
{
	static double origin[]={0,0,0};
	MESSAGE("SMDS_EdgePosition::Coords not implemented");
	return origin;
}

/**
*/
SMDS_TypeOfPosition SMDS_EdgePosition::GetTypeOfPosition() const
{
	return SMDS_TOP_EDGE;
}

void SMDS_EdgePosition::SetUParameter(double aUparam)
{
	myUParameter = aUparam;
}

//=======================================================================
//function : GetUParameter
//purpose  : 
//=======================================================================

double SMDS_EdgePosition::GetUParameter() const 
{
	return myUParameter;
}
