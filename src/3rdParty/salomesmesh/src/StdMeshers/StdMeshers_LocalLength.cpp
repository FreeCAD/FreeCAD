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
//  File   : StdMeshers_LocalLength.cxx
//           Moved here from SMESH_LocalLength.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_LocalLength.hxx"

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

StdMeshers_LocalLength::StdMeshers_LocalLength(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  _length = 1.;
  _precision = Precision::Confusion();
  _name = "LocalLength";
  _param_algo_dim = 1; // is used by SMESH_Regular_1D
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_LocalLength::~StdMeshers_LocalLength()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_LocalLength::SetLength(double length)
{
  double oldLength = _length;
  if (length <= 0)
    throw SALOME_Exception(LOCALIZED("length must be positive"));
  _length = length;
  const double precision = 1e-7;
  if (fabs(oldLength - _length) > precision)
    NotifySubMeshesHypothesisModification();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_LocalLength::GetLength() const
{
  return _length;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void StdMeshers_LocalLength::SetPrecision (double thePrecision)
{
  double oldPrecision = _precision;
  if (_precision < 0)
    throw SALOME_Exception(LOCALIZED("precision cannot be negative"));
  _precision = thePrecision;
  const double precision = 1e-8;
  if (fabs(oldPrecision - _precision) > precision)
    NotifySubMeshesHypothesisModification();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
double StdMeshers_LocalLength::GetPrecision() const
{
  return _precision;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_LocalLength::SaveTo(ostream & save)
{
  save << this->_length << " " << this->_precision;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_LocalLength::LoadFrom(istream & load)
{
  bool isOK = true;
  double a;

  isOK = (bool)(load >> a);
  if (isOK)
    this->_length = a;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (bool)(load >> a);
  if (isOK)
    this->_precision = a;
  else
  {
    load.clear(ios::badbit | load.rdstate());
    // old format, without precision
    _precision = 0.;
  }

  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_LocalLength & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_LocalLength & hyp)
{
  return hyp.LoadFrom( load );
}

//================================================================================
/*!
 * \brief Initialize segment length by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_LocalLength::SetParametersByMesh(const SMESH_Mesh*   theMesh,
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
    if ( C.IsNull() )
      continue;
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

  _precision = Precision::Confusion();

  return nbEdges;
}
//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_LocalLength::SetParametersByDefaults(const TDefaults&  dflts,
                                                     const SMESH_Mesh* /*theMesh*/)
{
  return ( _length = dflts._elemLength );
}

