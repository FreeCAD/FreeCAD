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

//  SMESH StdMeshers_StartEndLength : implementation of SMESH idl descriptions
//  File   : StdMeshers_StartEndLength.cxx
//  Module : SMESH
//
#include "StdMeshers_StartEndLength.hxx"

#include "SMESH_Algo.hxx"
#include "SMESH_Mesh.hxx"

#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_StartEndLength::StdMeshers_StartEndLength(int         hypId,
                                                     int         studyId,
                                                     SMESH_Gen * gen)
     :SMESH_Hypothesis(hypId, studyId, gen)
{
  _begLength = 1.;
  _endLength = 10.;
  _name = "StartEndLength";
  _param_algo_dim = 1; // is used by SMESH_Regular_1D
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_StartEndLength::~StdMeshers_StartEndLength()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_StartEndLength::SetLength(double length, bool isStartLength)
{
  if ( (isStartLength ? _begLength : _endLength) != length ) {
    if (length <= 0)
      throw SALOME_Exception(LOCALIZED("length must be positive"));
    if ( isStartLength )
      _begLength = length;
    else
      _endLength = length;

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_StartEndLength::GetLength(bool isStartLength) const
{
  return isStartLength ? _begLength : _endLength;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_StartEndLength::SetReversedEdges( std::vector<int>& ids )
{
  if ( ids != _edgeIDs ) {
    _edgeIDs = ids;

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_StartEndLength::SaveTo(ostream & save)
{
  int listSize = _edgeIDs.size();
  save << _begLength << " " << _endLength << " " << listSize;

  if ( listSize > 0 ) {
    for ( int i = 0; i < listSize; i++) {
      save << " " << _edgeIDs[i];
    }
    save << " " << _objEntry;
  }

  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_StartEndLength::LoadFrom(istream & load)
{
  bool isOK = true;
  int intVal;
  isOK = (bool)(load >> _begLength);
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());
  isOK = (bool)(load >> _endLength);

  if (!isOK)
    load.clear(ios::badbit | load.rdstate());
  
  isOK = (bool)(load >> intVal);
  if (isOK && intVal > 0) {
    _edgeIDs.reserve( intVal );
    for (int i = 0; i < _edgeIDs.capacity() && isOK; i++) {
      isOK = (bool)(load >> intVal);
      if ( isOK ) _edgeIDs.push_back( intVal );
    }
    isOK = (bool)(load >> _objEntry);
  }

  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_StartEndLength & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_StartEndLength & hyp)
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

bool StdMeshers_StartEndLength::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                    const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _begLength = _endLength = 0.;

  Standard_Real UMin, UMax;
  TopLoc_Location L;

  int nbEdges = 0;
  TopTools_IndexedMapOfShape edgeMap;
  TopExp::MapShapes( theShape, TopAbs_EDGE, edgeMap );
  for ( int i = 1; i <= edgeMap.Extent(); ++i )
  {
    const TopoDS_Edge& edge = TopoDS::Edge( edgeMap( i ));
    Handle(Geom_Curve) C = BRep_Tool::Curve(edge, L, UMin, UMax);
    GeomAdaptor_Curve AdaptCurve(C, UMin, UMax);

    vector< double > params;
    SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* >( theMesh )->GetMeshDS();
    if ( SMESH_Algo::GetNodeParamOnEdge( aMeshDS, edge, params ))
    {
      nbEdges++;
      _begLength += GCPnts_AbscissaPoint::Length( AdaptCurve, params[0], params[1]);
      int nb = params.size();
      _endLength += GCPnts_AbscissaPoint::Length( AdaptCurve, params[nb-2], params[nb-1]);
    }
  }
  if ( nbEdges ) {
    _begLength /= nbEdges;
    _endLength /= nbEdges;
  }
  return nbEdges;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_StartEndLength::SetParametersByDefaults(const TDefaults&  dflts,
                                                        const SMESH_Mesh* /*theMesh*/)
{
  return (_begLength = _endLength = dflts._elemLength );
}

