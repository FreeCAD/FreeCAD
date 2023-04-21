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

//  SMESH StdMeshers_QuadranglePreference : implementation of SMESH idl descriptions
//  File   : StdMeshers_QuadranglePreference.cxx
//  Module : SMESH
//
#include "StdMeshers_QuadranglePreference.hxx"
#include "utilities.h"

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_QuadranglePreference::StdMeshers_QuadranglePreference(int         hypId,
                                                                 int         studyId,
                                                                 SMESH_Gen * gen)
     :SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "QuadranglePreference";
  _param_algo_dim = -2; // auxiliary used by NETGEN 2D
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_QuadranglePreference::~StdMeshers_QuadranglePreference()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_QuadranglePreference::SaveTo(ostream & save)
{
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_QuadranglePreference::LoadFrom(istream & load)
{
  return load;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 *
 * Just return false as this hypothesis does not have parameters values
 */
//================================================================================

bool StdMeshers_QuadranglePreference::SetParametersByMesh(const SMESH_Mesh* /*theMesh*/,
                                                          const TopoDS_Shape& /*theShape*/)
{
  return false;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_QuadranglePreference::SetParametersByDefaults(const TDefaults&  /*dflts*/,
                                                              const SMESH_Mesh* /*theMesh*/)
{
  return false;
}

