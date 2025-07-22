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
//  File   : StdMeshers_FixedPoints1D.cxx
//  Author : Damien COQUERET, OCC
//  Module : SMESH
//
#include "StdMeshers_FixedPoints1D.hxx"

#include "SMESH_Algo.hxx"
#include "SMESH_Mesh.hxx"

//#include <BRep_Tool.hxx>
//#include <GCPnts_AbscissaPoint.hxx>
//#include <GeomAdaptor_Curve.hxx>
//#include <Geom_Curve.hxx>
//#include <TopExp.hxx>
//#include <TopLoc_Location.hxx>
//#include <TopTools_IndexedMapOfShape.hxx>
//#include <TopoDS.hxx>
//#include <TopoDS_Edge.hxx>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_FixedPoints1D::StdMeshers_FixedPoints1D(int hypId, int studyId,
                                                   SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "FixedPoints1D";
  _param_algo_dim = 1; 
  _nbsegs.reserve( 1 );
  _nbsegs.push_back( 1 );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_FixedPoints1D::~StdMeshers_FixedPoints1D()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_FixedPoints1D::SetPoints(std::vector<double>& listParams)
{
  _params = listParams;
  NotifySubMeshesHypothesisModification();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_FixedPoints1D::SetNbSegments(std::vector<int>& listNbSeg) 
{
  _nbsegs = listNbSeg;
  NotifySubMeshesHypothesisModification();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_FixedPoints1D::SetReversedEdges( std::vector<int>& ids )
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

ostream & StdMeshers_FixedPoints1D::SaveTo(ostream & save)
{
  int listSize = _params.size();
  save << listSize;
  if ( listSize > 0 ) {
    for ( int i = 0; i < listSize; i++) save << " " << _params[i];
  }

  listSize = _nbsegs.size();
  save << " " << listSize;
  if ( listSize > 0 ) {
    for ( int i = 0; i < listSize; i++) save << " " << _nbsegs[i];
  }

  listSize = _edgeIDs.size();
  save << " " << listSize;
  if ( listSize > 0 ) {
    for ( int i = 0; i < listSize; i++)
      save << " " << _edgeIDs[i];
  }

  save << " " << _objEntry;

  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_FixedPoints1D::LoadFrom(istream & load)
{
  bool isOK = true;
  int intVal;
  double dblVal;

  isOK = (bool)(load >> intVal);
  if (isOK && intVal > 0) {
    _params.clear();
    _params.reserve( intVal );
    for (int i = 0; i < _params.capacity() && isOK; i++) {
      isOK = (bool)(load >> dblVal);
      if ( isOK ) _params.push_back( dblVal );
    }
  }

  isOK = (bool)(load >> intVal);
  if (isOK && intVal > 0) {
    _nbsegs.clear();
    _nbsegs.reserve( intVal );
    for (int i = 0; i < _nbsegs.capacity() && isOK; i++) {
      isOK = (bool)(load >> intVal);
      if ( isOK ) _nbsegs.push_back( intVal );
    }
  }

  isOK = (bool)(load >> intVal);
  if (isOK && intVal > 0) {
    _edgeIDs.clear();
    _edgeIDs.reserve( intVal );
    for (int i = 0; i < _edgeIDs.capacity() && isOK; i++) {
      isOK = (bool)(load >> intVal);
      if ( isOK ) _edgeIDs.push_back( intVal );
    }
  }

  isOK = (bool)(load >> _objEntry);

  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_FixedPoints1D & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_FixedPoints1D & hyp)
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

bool StdMeshers_FixedPoints1D::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                   const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _nbsegs.reserve( 1 );
  _nbsegs.push_back( 1 );
  return true;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_FixedPoints1D::SetParametersByDefaults(const TDefaults&  dflts,
                                                       const SMESH_Mesh* /*mesh*/)
{
  _nbsegs.reserve( 1 );
  _nbsegs.push_back( 1 );
  return true;
}

