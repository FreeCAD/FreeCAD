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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : StdMeshers_MaxLength.cxx
//  Module : SMESH
//
#include "StdMeshers_MaxLength.hxx"

#include "SMESH_Mesh.hxx"
#include "SMESH_Algo.hxx"

#include "utilities.h"

#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <Precision.hxx>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_MaxLength::StdMeshers_MaxLength(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  _length = 1.;
  _preestimated = 0.;
  _preestimation = false;
  _name = "MaxLength";
  _param_algo_dim = 1; // is used by SMESH_Regular_1D
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_MaxLength::~StdMeshers_MaxLength()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_MaxLength::SetLength(double length)
{
  if (length <= 0)
    throw SALOME_Exception(LOCALIZED("length must be positive"));
  if ( _length != length ) {
    _length = length;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_MaxLength::GetLength() const
{
  return ( _preestimation && _preestimated > 0. ) ? _preestimated : _length;
}

//================================================================================
/*!
 * \brief Sets boolean parameter enabling/desabling usage of length computed
 * basing on size of bounding box of shape to mesh
 */
//================================================================================

void StdMeshers_MaxLength::SetUsePreestimatedLength(bool toUse)
{
  if ( toUse != _preestimation )
  {
    _preestimation = toUse;
    // this parameter is just to help the user
    //NotifySubMeshesHypothesisModification();
  }
}

//================================================================================
/*!
 * \brief Store preestemated length
 */
//================================================================================

void StdMeshers_MaxLength::SetPreestimatedLength(double length)
{
  if ( length > 0 )
    _preestimated = length;
}

//================================================================================
/*!
 * \brief Returns value of boolean parameter enabling/desabling usage of length computed
 * basing on size of bounding box of shape to mesh
 */
//================================================================================

bool StdMeshers_MaxLength::GetUsePreestimatedLength() const
{
  return _preestimation;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_MaxLength::SaveTo(ostream & save)
{
  save << _length << " " << _preestimated << " " << _preestimation;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_MaxLength::LoadFrom(istream & load)
{
  bool isOK = true;
  double a;

  isOK = (bool)(load >> a);
  if (isOK)
    _length = a;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (bool)(load >> a);
  if (isOK)
    _preestimated = a;
  else
    load.clear(ios::badbit | load.rdstate());

  bool pre;
  isOK = (bool)(load >> pre);
  if ( isOK )
    _preestimation = pre;
  else
    load.clear(ios::badbit | load.rdstate());

  return load;
}

//================================================================================
/*!
 * \brief Initialize segment length by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_MaxLength::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                               const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _length = 0.;

  Standard_Real UMin, UMax;
  TopLoc_Location L;

  int nbEdges = 0;
  TopTools_IndexedMapOfShape edgeMap;
  TopExp::MapShapes( theShape, TopAbs_EDGE, edgeMap );
  for ( int iE = 1; iE <= edgeMap.Extent(); ++iE )
  {
    const TopoDS_Edge& edge = TopoDS::Edge( edgeMap( iE ));
    Handle(Geom_Curve) C = BRep_Tool::Curve( edge, L, UMin, UMax );
    GeomAdaptor_Curve AdaptCurve(C, UMin, UMax);

    vector< double > params;
    SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* >( theMesh )->GetMeshDS();
    if ( SMESH_Algo::GetNodeParamOnEdge( aMeshDS, edge, params ))
    {
      for ( int i = 1; i < params.size(); ++i )
        _length += GCPnts_AbscissaPoint::Length( AdaptCurve, params[ i-1 ], params[ i ]);
      nbEdges += params.size() - 1;
    }
  }
  if ( nbEdges )
    _length /= nbEdges;

  return nbEdges;
}
//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_MaxLength::SetParametersByDefaults(const TDefaults&  dflts,
                                                   const SMESH_Mesh* /*theMesh*/)
{
  //_preestimation = ( dflts._elemLength > 0.);
  if ( dflts._elemLength > 0. )
    _preestimated = dflts._elemLength;
  return ( _length = dflts._elemLength );
}

