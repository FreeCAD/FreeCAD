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
//  File   : StdMeshers_Geometric1D.cxx
//  Module : SMESH
//
#include "StdMeshers_Geometric1D.hxx"

#include "SMESH_Mesh.hxx"

#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <SMESH_Algo.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

//=============================================================================
/*!
 * Constructor
 */
//=============================================================================

StdMeshers_Geometric1D::StdMeshers_Geometric1D(int hypId, int studyId, SMESH_Gen * gen)
  :StdMeshers_Reversible1D(hypId, studyId, gen)
{
  _begLength = 1.;
  _ratio = 1.;
  _name = "GeometricProgression";
}

//=============================================================================
/*!
 * Sets length of the first segment
 */
//=============================================================================

void StdMeshers_Geometric1D::SetStartLength(double length)
{
  if ( _begLength != length )
  {
    if (length <= 0)
      throw SALOME_Exception(LOCALIZED("length must be positive"));
    _begLength = length;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 * Sets value of Common Ratio
 */
//=============================================================================

void StdMeshers_Geometric1D::SetCommonRatio(double factor)
{
  if ( _ratio != factor )
  {
    if (factor == 0)
      throw SALOME_Exception(LOCALIZED("Zero factor is not allowed"));
    _ratio = factor;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 * Returns length of the first segment 
 */
//=============================================================================

double StdMeshers_Geometric1D::GetStartLength() const
{
  return _begLength;
}

//=============================================================================
/*!
 * Returns value of Common Ratio
 */
//=============================================================================

double StdMeshers_Geometric1D::GetCommonRatio() const
{
  return _ratio;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_Geometric1D::SaveTo(ostream & save)
{
  save << _begLength << " " << _ratio << " ";

  StdMeshers_Reversible1D::SaveTo( save );

  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_Geometric1D::LoadFrom(istream & load)
{
  bool isOK = true;
  isOK = (bool)(load >> _begLength);
  isOK = (bool)(load >> _ratio);

  if (isOK)
    StdMeshers_Reversible1D::LoadFrom( load );

  return load;
}

//================================================================================
/*!
 * \brief Initialize start and end length by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_Geometric1D::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                 const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _begLength = _ratio = 0.;

  int nbEdges = 0;
  TopTools_IndexedMapOfShape edgeMap;
  TopExp::MapShapes( theShape, TopAbs_EDGE, edgeMap );
  for ( int i = 1; i <= edgeMap.Extent(); ++i )
  {
    const TopoDS_Edge& edge = TopoDS::Edge( edgeMap( i ));
    BRepAdaptor_Curve C( edge );

    vector< double > params;
    if ( SMESH_Algo::GetNodeParamOnEdge( theMesh->GetMeshDS(), edge, params ))
    {
      nbEdges++;
      double l1 = GCPnts_AbscissaPoint::Length( C, params[0], params[1] );
      _begLength += l1;
      if ( params.size() > 2 && l1 > 1e-100 )
        _ratio += GCPnts_AbscissaPoint::Length( C, params[1], params[2]) / l1;
      else
        _ratio += 1;
    }
  }
  if ( nbEdges ) {
    _begLength /= nbEdges;
    _ratio     /= nbEdges;
  }
  else {
    _begLength = 1;
    _ratio     = 1;
  }
  return nbEdges;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_Geometric1D::SetParametersByDefaults(const TDefaults&  dflts,
                                                     const SMESH_Mesh* /*mesh*/)
{
  return ( _begLength = dflts._elemLength );
}

