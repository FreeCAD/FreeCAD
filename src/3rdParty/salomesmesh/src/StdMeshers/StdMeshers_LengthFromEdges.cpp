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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : StdMeshers_LengthFromEdges.cxx
//           Moved here from SMESH_LengthFromEdges.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_LengthFromEdges.hxx"

#include "utilities.h"

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_LengthFromEdges::StdMeshers_LengthFromEdges(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _mode =1;
  _name = "LengthFromEdges";
  _param_algo_dim = 2; // is used by SMESH_MEFISTO_2D
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_LengthFromEdges::~StdMeshers_LengthFromEdges()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_LengthFromEdges::SetMode(int mode)
{
  int oldMode = _mode;
  if (mode <= 0) 
    throw SALOME_Exception(LOCALIZED("mode must be positive"));
  _mode = mode;
  if (oldMode != _mode)
    NotifySubMeshesHypothesisModification();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

int StdMeshers_LengthFromEdges::GetMode()
{
  return _mode;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_LengthFromEdges::SaveTo(ostream & save)
{
  save << this->_mode;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_LengthFromEdges::LoadFrom(istream & load)
{
  bool isOK = (bool)true;
  int a;
  isOK = (bool)(load >> a);
  if (isOK) 
    this->_mode = a;
  else 
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator << (ostream & save, StdMeshers_LengthFromEdges & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >> (istream & load, StdMeshers_LengthFromEdges & hyp)
{
  return hyp.LoadFrom( load );
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

bool StdMeshers_LengthFromEdges::SetParametersByMesh(const SMESH_Mesh* /*theMesh*/,
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

bool StdMeshers_LengthFromEdges::SetParametersByDefaults(const TDefaults&  /*dflts*/,
                                                         const SMESH_Mesh* /*theMesh*/)
{
  return true;
}
