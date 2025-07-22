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

//  SMESH SMESH : idl implementation based on 'SMESH' unit's classes
//  File   : StdMeshers_NumberOfLayers.cxx
//  Author : Edward AGAPOV
//  Module : SMESH
//
#include "StdMeshers_NumberOfLayers.hxx"


#include "SMESH_Mesh.hxx"
#include "utilities.h"

using namespace std;


//=============================================================================
/*!
 *  StdMeshers_NumberOfLayers::StdMeshers_NumberOfLayers
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_NumberOfLayers::StdMeshers_NumberOfLayers(int hypId, int studyId,
                                                     SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "NumberOfLayers"; // used by RadialPrism_3D
  _param_algo_dim = 3; // 3D
  _nbLayers = 1;
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfLayers::~StdMeshers_NumberOfLayers
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_NumberOfLayers::~StdMeshers_NumberOfLayers()
{
  MESSAGE( "StdMeshers_NumberOfLayers::~StdMeshers_NumberOfLayers" );
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfLayers::SetNumberOfLayers
 *
 *  Sets <number of segments> parameter value
 */
//=============================================================================

void StdMeshers_NumberOfLayers::SetNumberOfLayers(int numberOfLayers)
{
  if ( _nbLayers != numberOfLayers ) {
    if ( numberOfLayers <= 0 )
      throw SALOME_Exception(LOCALIZED("numberOfLayers must be positive"));
    _nbLayers = numberOfLayers;

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfLayers::GetNumberOfLayers
 *
 *  Returns <number of layers> parameter value
 */
//=============================================================================

int StdMeshers_NumberOfLayers::GetNumberOfLayers() const
{
  return _nbLayers;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_NumberOfLayers::SaveTo(ostream & save)
{
  save << _nbLayers;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_NumberOfLayers::LoadFrom(istream & load)
{
  bool isOK = true;
  isOK = (bool)(load >> _nbLayers);
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_NumberOfLayers & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_NumberOfLayers & hyp)
{
  return hyp.LoadFrom( load );
}

//================================================================================
/*!
 * \brief Initialize start and end length by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_NumberOfLayers::SetParametersByMesh(const SMESH_Mesh*   ,
                                                    const TopoDS_Shape& )
{
  return false;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_NumberOfLayers::SetParametersByDefaults(const TDefaults&  dflts,
                                                        const SMESH_Mesh* theMesh)
{
  if ( dflts._elemLength )
    return theMesh ? (_nbLayers = int( theMesh->GetShapeDiagonalSize() / dflts._elemLength/ 2.)) : 0;
  return false;
}

