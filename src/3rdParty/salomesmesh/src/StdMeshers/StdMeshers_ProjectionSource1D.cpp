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

//  SMESH SMESH : idl implementation based on 'SMESH' unit's classes
//  File   : StdMeshers_ProjectionSource1D.cxx
//  Author : Edward AGAPOV
//  Module : SMESH
//
#include "StdMeshers_ProjectionSource1D.hxx"

#include "SMESH_Mesh.hxx"

#include "utilities.h"

#include <TopoDS.hxx>

using namespace std;


//=============================================================================
/*!
 *  StdMeshers_ProjectionSource1D::StdMeshers_ProjectionSource1D
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_ProjectionSource1D::StdMeshers_ProjectionSource1D(int hypId, int studyId,
                                                             SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "ProjectionSource1D"; // used by Projection_1D
  _param_algo_dim = 1; // 1D
  _sourceMesh = 0;
}

//=============================================================================
/*!
 *  StdMeshers_ProjectionSource1D::~StdMeshers_ProjectionSource1D
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_ProjectionSource1D::~StdMeshers_ProjectionSource1D()
{
  MESSAGE( "StdMeshers_ProjectionSource1D::~StdMeshers_ProjectionSource1D" );
}

//=============================================================================
  /*!
   * Sets source <edge> to take a mesh pattern from
   */
//=============================================================================

void StdMeshers_ProjectionSource1D::SetSourceEdge(const TopoDS_Shape& edge)
{
  if ( edge.IsNull() )
    throw SALOME_Exception(LOCALIZED("Null edge is not allowed"));

  if ( edge.ShapeType() != TopAbs_EDGE && edge.ShapeType() != TopAbs_COMPOUND )
    throw SALOME_Exception(LOCALIZED("Wrong shape type"));

  if ( !_sourceEdge.IsSame( edge ) )
  {
    _sourceEdge = edge;

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 * Sets vertex association between the source edge and the target one.
 * This parameter is optional
 */
//=============================================================================

void StdMeshers_ProjectionSource1D::SetVertexAssociation(const TopoDS_Shape& sourceVertex,
                                                         const TopoDS_Shape& targetVertex)
{
  if ( sourceVertex.IsNull() != targetVertex.IsNull() )
    throw SALOME_Exception(LOCALIZED("Two or none vertices must be provided"));

  if ( !sourceVertex.IsNull() ) {
    if ( sourceVertex.ShapeType() != TopAbs_VERTEX ||
         targetVertex.ShapeType() != TopAbs_VERTEX )
      throw SALOME_Exception(LOCALIZED("Wrong shape type"));
  }

  if ( !_sourceVertex.IsSame( sourceVertex ) ||
       !_targetVertex.IsSame( targetVertex ) )
  {
    _sourceVertex = TopoDS::Vertex( sourceVertex );
    _targetVertex = TopoDS::Vertex( targetVertex );

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 * Sets source <mesh> to take a mesh pattern from
 */
//=============================================================================

void StdMeshers_ProjectionSource1D::SetSourceMesh(SMESH_Mesh* mesh)
{
  if ( _sourceMesh != mesh ) {
    _sourceMesh = mesh;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_ProjectionSource1D::SaveTo(ostream & save)
{
  // we store it in order to be able to detect that hypo is really modified
  save << " " << _sourceEdge.TShape().operator->()  ;
  save << " " << _sourceVertex.TShape().operator->();
  save << " " << _targetVertex.TShape().operator->();
  save << " " << ( _sourceMesh ? _sourceMesh->GetId() : -1 );
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_ProjectionSource1D::LoadFrom(istream & load)
{
  // impossible to restore w/o any context
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_ProjectionSource1D & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_ProjectionSource1D & hyp)
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

bool StdMeshers_ProjectionSource1D::SetParametersByMesh(const SMESH_Mesh*   ,
                                                        const TopoDS_Shape& )
{
  return false;
}

//================================================================================
/*!
 * \brief Return all parameters
 */
//================================================================================

void StdMeshers_ProjectionSource1D::GetStoreParams(TopoDS_Shape& s1,
                                                   TopoDS_Shape& s2,
                                                   TopoDS_Shape& s3) const
{
  s1 = _sourceEdge;
  s2 = _sourceVertex;
  s3 = _targetVertex;
}

//================================================================================
/*!
 * \brief Set all parameters without notifying on modification
 */
//================================================================================

void StdMeshers_ProjectionSource1D::RestoreParams(const TopoDS_Shape& s1,
                                                  const TopoDS_Shape& s2,
                                                  const TopoDS_Shape& s3,
                                                  SMESH_Mesh*         mesh)
{
  _sourceEdge   = s1;
  _sourceVertex = TopoDS::Vertex( s2 );
  _targetVertex = TopoDS::Vertex( s3 );
  _sourceMesh   = mesh;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_ProjectionSource1D::SetParametersByDefaults(const TDefaults&  /*dflts*/,
                                                            const SMESH_Mesh* /*theMesh*/)
{
  return false;
}

