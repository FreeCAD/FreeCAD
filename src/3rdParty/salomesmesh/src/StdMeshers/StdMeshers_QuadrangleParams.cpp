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
//  File   : StdMeshers_QuadrangleParams.cxx
//  Author : Sergey KUUL, OCC
//  Module : SMESH

#include "StdMeshers_QuadrangleParams.hxx"

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
StdMeshers_QuadrangleParams::StdMeshers_QuadrangleParams(int hypId, int studyId,
                                                         SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "QuadrangleParams";
  _param_algo_dim = 2;
  _triaVertexID = -1;
  _quadType = QUAD_STANDARD;
}

//=============================================================================
/*!
 *
 */
//=============================================================================
StdMeshers_QuadrangleParams::~StdMeshers_QuadrangleParams()
{
}

//=============================================================================
/*!
 *
 */
//=============================================================================
void StdMeshers_QuadrangleParams::SetTriaVertex (int id)
{
  if (id != _triaVertexID) {
    _triaVertexID = id;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *
 */
//=============================================================================
void StdMeshers_QuadrangleParams::SetQuadType (StdMeshers_QuadType type)
{
  if (type != _quadType) {
    _quadType = type;
    NotifySubMeshesHypothesisModification();
  }
}

//================================================================================
/*!
 * \brief Set positions of enforced nodes
 */
//================================================================================

void StdMeshers_QuadrangleParams::
SetEnforcedNodes( const std::vector< TopoDS_Shape >& shapes,
                  const std::vector< gp_Pnt >&       points )
{
  bool isChanged = ( shapes        != _enforcedVertices ||
                     points.size() != _enforcedPoints.size() );
  for ( size_t i = 0; i < points.size() && !isChanged; ++i )
    isChanged = ( _enforcedPoints[ i ].SquareDistance( points[i] ) > 1e-100 );
      
  if ( isChanged )
  {
    _enforcedVertices = shapes;
    _enforcedPoints   = points;
    NotifySubMeshesHypothesisModification();
  }
}

//================================================================================
/*!
 * \brief Returns positions of enforced nodes
 */
//================================================================================

void StdMeshers_QuadrangleParams::
GetEnforcedNodes( std::vector< TopoDS_Shape >& shapes,
                  std::vector< gp_Pnt >&       points ) const
{
  shapes = _enforcedVertices;
  points = _enforcedPoints;
}

//=============================================================================
/*!
 *
 */
//=============================================================================
ostream & StdMeshers_QuadrangleParams::SaveTo(ostream & save)
{
  if (_objEntry.size() == 0)
    save << _triaVertexID << " UNDEFINED " << int(_quadType);
  else
    save << _triaVertexID << " " << _objEntry << " " << int(_quadType);

  save << " " << _enforcedPoints.size();
  for ( size_t i = 0; i < _enforcedPoints.size(); ++i )
    save << " " << _enforcedPoints[i].X()
         << " " << _enforcedPoints[i].Y()
         << " " << _enforcedPoints[i].Z();

  return save;
}

//=============================================================================
/*!
 *
 */
//=============================================================================
istream & StdMeshers_QuadrangleParams::LoadFrom(istream & load)
{
  bool isOK = true;
  isOK = (bool)(load >> _triaVertexID);
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());

  isOK = (bool)(load >> _objEntry);
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());

  int type;
  isOK = (bool)(load >> type);
  if (isOK)
    _quadType = StdMeshers_QuadType(type);

  // _enforcedVertices are loaded at StdMeshers_I level
  // because GEOM objects are referred by study entry.

  int nbP = 0;
  double x,y,z;
  if ( load >> nbP && nbP > 0 )
  {
    _enforcedPoints.reserve( nbP );
    while ( _enforcedPoints.size() < _enforcedPoints.capacity() )
      if ( load >> x &&
           load >> y &&
           load >> z )
        _enforcedPoints.push_back( gp_Pnt( x,y,z ));
      else
        break;
  }
  return load;
}

//================================================================================
/*!
 * \brief Redifined method
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================
bool StdMeshers_QuadrangleParams::SetParametersByMesh(const SMESH_Mesh* theMesh,
                                                      const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  return true;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================
bool StdMeshers_QuadrangleParams::SetParametersByDefaults(const TDefaults&  dflts,
                                                          const SMESH_Mesh* /*mesh*/)
{
  return true;
}
