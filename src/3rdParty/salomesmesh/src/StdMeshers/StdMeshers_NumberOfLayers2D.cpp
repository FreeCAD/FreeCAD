// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SMESH SMESH : idl implementation based on 'SMESH' unit's classes
//  File   : StdMeshers_NumberOfLayers2D.cxx
//  Author : Edward AGAPOV
//  Module : SMESH
//
#include "StdMeshers_NumberOfLayers2D.hxx"

#include "utilities.h"


//=============================================================================
/*!
 *  StdMeshers_NumberOfLayers2D::StdMeshers_NumberOfLayers2D
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_NumberOfLayers2D::StdMeshers_NumberOfLayers2D(int hypId,
                                                         int studyId,
                                                         SMESH_Gen * gen)
  : StdMeshers_NumberOfLayers(hypId, studyId, gen)
{
  _name = "NumberOfLayers2D"; // used by RadialQuadrangle_1D2D
  _param_algo_dim = 2; // 2D
  _nbLayers = 1;
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfLayers2D::~StdMeshers_NumberOfLayers2D
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_NumberOfLayers2D::~StdMeshers_NumberOfLayers2D()
{
  MESSAGE( "StdMeshers_NumberOfLayers2D::~StdMeshers_NumberOfLayers2D" );
}

