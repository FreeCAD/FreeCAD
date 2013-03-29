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
//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_SimpleHypothesis_2D.cxx
// Author    : Edward AGAPOV
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_SimpleHypothesis_2D.hxx"
#include "NETGENPlugin_Hypothesis.hxx"

#include <SMESH_Mesh.hxx>
#include <SMESH_subMesh.hxx>
#include <SMESH_ControlsDef.hxx>

#include <TopExp_Explorer.hxx>

#include <utilities.h>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================
NETGENPlugin_SimpleHypothesis_2D::NETGENPlugin_SimpleHypothesis_2D (int         hypId,
                                                                    int         studyId,
                                                                    SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen),
    _nbSegments ((int)NETGENPlugin_Hypothesis::GetDefaultNbSegPerEdge()),
    _segmentLength(0),
    _area       (0.)
{
  _name = "NETGEN_SimpleParameters_2D";
  _param_algo_dim = 2;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_2D::SetNumberOfSegments(int nb) throw (SMESH_Exception)
{
  if ( nb < 1 )
    throw SMESH_Exception("Number of segments must be positive");
  if (nb != _nbSegments)
  {
    _nbSegments = nb;
    if ( _nbSegments ) _segmentLength = 0.;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_2D::SetLocalLength(double segmentLength)
  throw (SMESH_Exception)
{
  if ( segmentLength < DBL_MIN )
    throw SMESH_Exception("segment length must be more than zero");
  if (segmentLength != _segmentLength)
  {
    _segmentLength = segmentLength;
    if ( _segmentLength > DBL_MIN ) _nbSegments = 0;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_2D::LengthFromEdges()
{
  if (_area > DBL_MIN )
  {
    _area = 0;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_2D::SetMaxElementArea(double area)
{
  if ( area < DBL_MIN )
    area = 0.;
  if (_area != area)
  {
    _area = area;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
ostream & NETGENPlugin_SimpleHypothesis_2D::SaveTo(ostream & save)
{
  save << _nbSegments << " " << _segmentLength << " " << _area;

  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
istream & NETGENPlugin_SimpleHypothesis_2D::LoadFrom(istream & load)
{
  bool isOK = true;
  double val;

  isOK = (load >> val);
  if (isOK)
    _nbSegments = (int) val;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> val);
  if (isOK)
    _segmentLength = val;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> val);
  if (isOK)
    _area = val;
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
bool NETGENPlugin_SimpleHypothesis_2D::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                           const TopoDS_Shape& theShape)
{
  // Find out nb of segments.
  int nbSeg = 0, nbEdges = 0;
  TopExp_Explorer exp( theShape, TopAbs_EDGE );
  for ( ; exp.More(); exp.Next() ) {
    SMESH_subMesh* sm = theMesh->GetSubMeshContaining( exp.Current() );
    if ( sm && !sm->IsEmpty() ) {
      nbSeg += sm->GetSubMeshDS()->NbElements();
      nbEdges++;
    }
  }
  if ( nbEdges )
    _nbSegments = nbSeg / nbEdges;

  // Find out max face area
  _area = 0;
  SMESH::Controls::Area areaControl;
  SMESH::Controls::TSequenceOfXYZ nodesCoords;
  const int nbFacesToCheck = 100;
  for ( exp.Init( theShape, TopAbs_FACE ); exp.More(); exp.Next() ) {
    SMESH_subMesh* sm = theMesh->GetSubMeshContaining( exp.Current() );
    if ( sm && !sm->IsEmpty() ) {
      SMDS_ElemIteratorPtr fIt = sm->GetSubMeshDS()->GetElements();
      int nbCheckedFaces = 0;
      while ( fIt->more() && nbCheckedFaces++ < nbFacesToCheck ) {
        const SMDS_MeshElement* elem = fIt->next();
        areaControl.GetPoints( elem, nodesCoords );
        _area = max( _area, areaControl.GetValue( nodesCoords ));
      }
    }
  }
  return nbEdges;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool NETGENPlugin_SimpleHypothesis_2D::SetParametersByDefaults(const TDefaults&    dflts,
                                                               const SMESH_Mesh* /*theMesh*/)
{
  _nbSegments    = dflts._nbSegments;
  _segmentLength = dflts._elemLength;
  return _nbSegments && _segmentLength > 0;
}
