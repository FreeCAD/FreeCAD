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
//  File   : StdMeshers_LayerDistribution.cxx
//  Author : Edward AGAPOV
//  Module : SMESH
//
#include "StdMeshers_LayerDistribution.hxx"

#include "utilities.h"

using namespace std;


//=============================================================================
/*!
 *  StdMeshers_LayerDistribution::StdMeshers_LayerDistribution
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_LayerDistribution::StdMeshers_LayerDistribution(int hypId, int studyId,
                                                           SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "LayerDistribution"; // used by RadialPrism_3D
  _param_algo_dim = 3; // 3D
  myHyp = 0;
}

//=============================================================================
/*!
 *  StdMeshers_LayerDistribution::~StdMeshers_LayerDistribution
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_LayerDistribution::~StdMeshers_LayerDistribution()
{
  MESSAGE( "StdMeshers_LayerDistribution::~StdMeshers_LayerDistribution" );
}

//=============================================================================
  /*!
   * \brief Sets  1D hypothesis specifying distribution of layers
    * \param hyp1D - 1D hypothesis
   */
//=============================================================================

void StdMeshers_LayerDistribution::SetLayerDistribution(SMESH_Hypothesis* hyp1D)
{
  if ( myHyp != hyp1D ) {
    if ( myHyp && hyp1D->GetDim() != 1 )
      throw SALOME_Exception(LOCALIZED("1D hypothesis is expected"));
    myHyp = hyp1D;
  }
  std::ostringstream os;
  if ( myHyp )
    myHyp->SaveTo( os );

  if ( mySavedHyp != os.str() )
    NotifySubMeshesHypothesisModification();

  mySavedHyp = os.str();
}

//=============================================================================
/*!
 *  Servant saves and loads my hypothesis
 */
//=============================================================================

ostream & StdMeshers_LayerDistribution::SaveTo(ostream & save)
{
  return save;
}

//=============================================================================
/*!
 *   Servant saves and loads my hypothesis
 */
//=============================================================================

istream & StdMeshers_LayerDistribution::LoadFrom(istream & load)
{
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_LayerDistribution & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_LayerDistribution & hyp)
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

bool StdMeshers_LayerDistribution::SetParametersByMesh(const SMESH_Mesh*   ,
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

bool StdMeshers_LayerDistribution::SetParametersByDefaults(const TDefaults&  dflts,
                                                           const SMESH_Mesh* theMesh)
{
  return myHyp ? myHyp->SetParametersByDefaults(dflts,theMesh) : false;
}
