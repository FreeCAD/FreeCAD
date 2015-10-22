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
//  File   : SMDS_VertexPosition.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#include "SMDS_VertexPosition.hxx"

#include "utilities.h"

using namespace std;

//=======================================================================
//function : SMDS_VertexPosition
//purpose  : 
//=======================================================================

SMDS_VertexPosition:: SMDS_VertexPosition(const int aVertexId)
	:SMDS_Position(aVertexId)
{
}

//=======================================================================
//function : Coords
//purpose  : 
//=======================================================================

const double *SMDS_VertexPosition::Coords() const
{
	const static double origin[]={0,0,0};
	MESSAGE("SMDS_VertexPosition::Coords not implemented");
	return origin;
}


SMDS_TypeOfPosition SMDS_VertexPosition::GetTypeOfPosition() const
{
	return SMDS_TOP_VERTEX;
}
