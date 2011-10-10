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
//  SMESH SMESH : implementaion of SMESH idl descriptions
//  File   : StdMeshers_QuadrangleParams.cxx
//  Author : Sergey KUUL, OCC
//  Module : SMESH
//
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
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "QuadrangleParams";
  _param_algo_dim = 2;
  _triaVertexID = -1;
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

void StdMeshers_QuadrangleParams::SetTriaVertex(int id)
{
  if ( id != _triaVertexID ) {
    _triaVertexID = id;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_QuadrangleParams::SaveTo(ostream & save)
{
  save << _triaVertexID << " " << _objEntry;
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
  isOK = (load >> _triaVertexID);
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> _objEntry);

  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_QuadrangleParams & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_QuadrangleParams & hyp)
{
  return hyp.LoadFrom( load );
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

