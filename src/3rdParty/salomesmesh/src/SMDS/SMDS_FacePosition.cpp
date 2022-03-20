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
//  File   : SMDS_FacePosition.cxx
//  Author : Jean-Michel BOULCOURT
//  Module : SMESH
//
#include "SMDS_FacePosition.hxx"

#include "utilities.h"

using namespace std;

//=======================================================================
//function : SMDS_FacePosition
//purpose  : 
//=======================================================================

SMDS_FacePosition::SMDS_FacePosition(const double aUParam,
                                     const double aVParam)
   : myUParameter(aUParam),myVParameter(aVParam)
{
  //MESSAGE("******************************************************** SMDS_FacePosition");
}

/**
*/
SMDS_TypeOfPosition SMDS_FacePosition::GetTypeOfPosition() const
{
        return SMDS_TOP_FACE;
}

void SMDS_FacePosition::SetUParameter(double aUparam)
{
        myUParameter = aUparam;
}

//=======================================================================
//function : SetVParameter
//purpose  : 
//=======================================================================

void SMDS_FacePosition::SetVParameter(double aVparam)
{
        myVParameter = aVparam;
}

//=======================================================================
//function : GetUParameter
//purpose  : 
//=======================================================================

double SMDS_FacePosition::GetUParameter() const 
{
        return myUParameter;
}

//=======================================================================
//function : GetVParameter
//purpose  : 
//=======================================================================

double SMDS_FacePosition::GetVParameter() const 
{
        return myVParameter;
}

//=======================================================================
//function : SetParameters
//purpose  : 
//=======================================================================

void SMDS_FacePosition::SetParameters(double aUparam, double aVparam)
{
  myUParameter = aUparam;
  myVParameter = aVparam;
}
