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
//  File   : StdMeshers_MaxElementVolume.cxx
//           Moved here from SMESH_MaxElementVolume.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_MaxElementVolume.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_ControlsDef.hxx"
#include "SMESH_Mesh.hxx"

#include "utilities.h"

#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_MaxElementVolume::StdMeshers_MaxElementVolume(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _maxVolume = 1.;
  _name = "MaxElementVolume";
  _param_algo_dim = 3;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_MaxElementVolume::~StdMeshers_MaxElementVolume()
{
  MESSAGE("StdMeshers_MaxElementVolume::~StdMeshers_MaxElementVolume");
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_MaxElementVolume::SetMaxVolume(double maxVolume)
{
  double oldVolume = _maxVolume;
  if (maxVolume <= 0) 
    throw SALOME_Exception(LOCALIZED("maxVolume must be positive"));
  _maxVolume = maxVolume;
  if (_maxVolume != oldVolume)
    NotifySubMeshesHypothesisModification();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_MaxElementVolume::GetMaxVolume() const
{
  return _maxVolume;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_MaxElementVolume::SaveTo(ostream & save)
{
  save << this->_maxVolume;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_MaxElementVolume::LoadFrom(istream & load)
{
  bool isOK = true;
  double a;
  isOK = (bool)(load >> a);
  if (isOK)
    this->_maxVolume = a;
  else 
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator << (ostream & save, StdMeshers_MaxElementVolume & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >> (istream & load, StdMeshers_MaxElementVolume & hyp)
{
  return hyp.LoadFrom( load );
}


//================================================================================
/*!
 * \brief Initialize maximal area by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_MaxElementVolume::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                      const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _maxVolume = 0;

  SMESH::Controls::Volume volumeControl;

  TopTools_IndexedMapOfShape volMap;
  TopExp::MapShapes( theShape, TopAbs_SOLID, volMap );
  if ( volMap.IsEmpty() )
    TopExp::MapShapes( theShape, TopAbs_SHELL, volMap );
  if ( volMap.IsEmpty() )
    return false;

  SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* >( theMesh )->GetMeshDS();

  for ( int iV = 1; iV <= volMap.Extent(); ++iV )
  {
    const TopoDS_Shape& S = volMap( iV );
    SMESHDS_SubMesh * subMesh = aMeshDS->MeshElements( S );
    if ( !subMesh && S.ShapeType() == TopAbs_SOLID ) {
      TopExp_Explorer shellExp( S, TopAbs_SHELL );
      if ( shellExp.More() )
        subMesh = aMeshDS->MeshElements( shellExp.Current() );
    }
    if ( !subMesh) 
      return false;
    SMDS_ElemIteratorPtr vIt = subMesh->GetElements();
    while ( vIt->more() )
    {
      const SMDS_MeshElement* elem = vIt->next();
      if ( elem->GetType() == SMDSAbs_Volume ) {
        _maxVolume = max( _maxVolume, volumeControl.GetValue( elem->GetID() ));
      }
    }
  }
  return _maxVolume > 0;
}
//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_MaxElementVolume::SetParametersByDefaults(const TDefaults&  dflts,
                                                          const SMESH_Mesh* /*theMesh*/)
{
  return ( _maxVolume = dflts._elemLength*dflts._elemLength*dflts._elemLength );
}

