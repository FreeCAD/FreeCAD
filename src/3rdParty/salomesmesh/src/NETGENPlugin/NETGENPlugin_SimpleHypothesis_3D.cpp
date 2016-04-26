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

//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_SimpleHypothesis_3D.cxx
// Author    : Edward AGAPOV
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_SimpleHypothesis_3D.hxx"
#include "NETGENPlugin_Hypothesis.hxx"

#include <SMESH_ControlsDef.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_subMesh.hxx>

#include <TopExp_Explorer.hxx>

#include <utilities.h>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================
NETGENPlugin_SimpleHypothesis_3D::NETGENPlugin_SimpleHypothesis_3D (int         hypId,
                                                                    int         studyId,
                                                                    SMESH_Gen * gen)
  : NETGENPlugin_SimpleHypothesis_2D(hypId, studyId, gen),
  _volume(0)
{
  _name = "NETGEN_SimpleParameters_3D";
  _param_algo_dim = 3;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_3D::LengthFromFaces()
{
  if (_volume > DBL_MIN )
  {
    _volume = 0;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_3D::SetMaxElementVolume(double value)
{
  if ( value < DBL_MIN )
    value = 0.;
  if (_volume != value)
  {
    _volume = value;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
ostream & NETGENPlugin_SimpleHypothesis_3D::SaveTo(ostream & save)
{
  NETGENPlugin_SimpleHypothesis_2D::SaveTo( save );
  save << " " << _volume;

  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
istream & NETGENPlugin_SimpleHypothesis_3D::LoadFrom(istream & load)
{
  NETGENPlugin_SimpleHypothesis_2D::LoadFrom(load);

  bool isOK = true;
  double val;

  isOK = (bool)(load >> val);
  if (isOK)
    _volume = val;
  else
    load.clear(ios::badbit | load.rdstate());

  return load;
}

//================================================================================
/*!
 * \brief Does nothing
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - always false
 */
//================================================================================
bool NETGENPlugin_SimpleHypothesis_3D::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                           const TopoDS_Shape& theShape)
{
  if ( NETGENPlugin_SimpleHypothesis_2D::SetParametersByMesh(theMesh, theShape) )
  {
    // Find out max volume
    _volume = 0;
    SMESH::Controls::Volume volControl;
    volControl.SetMesh( ((SMESH_Mesh*)theMesh)->GetMeshDS() );
    const int nbElemToCheck = 100;
    for ( TopExp_Explorer exp( theShape, TopAbs_SOLID ); exp.More(); exp.Next() ) {
      SMESH_subMesh* sm = theMesh->GetSubMeshContaining( exp.Current() );
      if ( sm && !sm->IsEmpty() ) {
        SMDS_ElemIteratorPtr fIt = sm->GetSubMeshDS()->GetElements();
        int nbCheckedElems = 0;
        while ( fIt->more() && nbCheckedElems++ < nbElemToCheck ) {
          const SMDS_MeshElement* elem = fIt->next();
          _volume = max( _volume, volControl.GetValue( elem->GetID() ));
        }
      }
    }
    return int( _volume );
  }
  return false;
}
