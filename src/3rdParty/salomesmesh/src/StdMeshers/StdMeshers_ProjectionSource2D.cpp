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
//  File   : StdMeshers_ProjectionSource2D.cxx
//  Author : Edward AGAPOV
//  Module : SMESH
//
#include "StdMeshers_ProjectionSource2D.hxx"

#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "StdMeshers_ProjectionUtils.hxx"

#include "utilities.h"

#include <TopoDS.hxx>

using namespace std;

//=============================================================================
/*!
 *  StdMeshers_ProjectionSource2D::StdMeshers_ProjectionSource2D
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_ProjectionSource2D::StdMeshers_ProjectionSource2D(int hypId, int studyId,
                                                             SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "ProjectionSource2D"; // used by Projection_2D
  _param_algo_dim = 2; // 2D
  _sourceMesh = 0;
}

//=============================================================================
/*!
 *  StdMeshers_ProjectionSource2D::~StdMeshers_ProjectionSource2D
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_ProjectionSource2D::~StdMeshers_ProjectionSource2D()
{
  MESSAGE( "StdMeshers_ProjectionSource2D::~StdMeshers_ProjectionSource2D" );
}

//=============================================================================
  /*!
   * Sets a source <face> to take a mesh pattern from
   */
//=============================================================================

void StdMeshers_ProjectionSource2D::SetSourceFace(const TopoDS_Shape& Face)
{
  if ( Face.IsNull() )
    throw SALOME_Exception(LOCALIZED("Null Face is not allowed"));

  if ( Face.ShapeType() != TopAbs_FACE && Face.ShapeType() != TopAbs_COMPOUND )
    throw SALOME_Exception(LOCALIZED("Wrong shape type"));

  if ( !_sourceFace.IsSame( Face ) )
  {
    _sourceFace = Face;

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 * Sets vertex association between the source face and the target one.
 * This parameter is optional.
 * Two vertices must belong to one edge of a face
 */
//=============================================================================

void StdMeshers_ProjectionSource2D::SetVertexAssociation(const TopoDS_Shape& sourceVertex1,
                                                         const TopoDS_Shape& sourceVertex2,
                                                         const TopoDS_Shape& targetVertex1,
                                                         const TopoDS_Shape& targetVertex2)
{
  if ( sourceVertex1.IsNull() != targetVertex1.IsNull() ||
       sourceVertex2.IsNull() != targetVertex2.IsNull() )
    throw SALOME_Exception(LOCALIZED("Vertices must be provided in couples"));

  if ( sourceVertex1.IsNull() != sourceVertex2.IsNull() )
  {
    // possibly there is only 1 vertex in the face
    if ( !_sourceFace.IsNull() &&
         SMESH_MesherHelper::Count( _sourceFace, TopAbs_VERTEX, /*ignoreSame=*/true) != 1 )
      throw SALOME_Exception(LOCALIZED("Two or none pairs of vertices must be provided"));
  }

  if ( !sourceVertex1.IsNull() )
    if ( sourceVertex1.ShapeType() != TopAbs_VERTEX ||
         targetVertex1.ShapeType() != TopAbs_VERTEX )
      throw SALOME_Exception(LOCALIZED("Wrong shape type"));

  if ( !sourceVertex2.IsNull() )
    if ( sourceVertex2.ShapeType() != TopAbs_VERTEX ||
         targetVertex2.ShapeType() != TopAbs_VERTEX )
      throw SALOME_Exception(LOCALIZED("Wrong shape type"));


  if ( !_sourceVertex1.IsSame( sourceVertex1 ) ||
       !_sourceVertex2.IsSame( sourceVertex2 ) ||
       !_targetVertex1.IsSame( targetVertex1 ) ||
       !_targetVertex2.IsSame( targetVertex2 ) )
  {
    _sourceVertex1 = TopoDS::Vertex( sourceVertex1 );
    _sourceVertex2 = TopoDS::Vertex( sourceVertex2 );
    _targetVertex1 = TopoDS::Vertex( targetVertex1 );
    _targetVertex2 = TopoDS::Vertex( targetVertex2 );

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 * Sets source <mesh> to take a mesh pattern from
 */
//=============================================================================

void StdMeshers_ProjectionSource2D::SetSourceMesh(SMESH_Mesh* mesh)
{
  if ( _sourceMesh != mesh ) {
    _sourceMesh = mesh;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 * Returns the source face
 */
//=============================================================================

TopoDS_Shape StdMeshers_ProjectionSource2D::GetSourceFace() const
{
  return _sourceFace;
}

//=============================================================================
/*!
 * Returns the vertex associated with the target vertex.
 * Result may be nil if association not set
 */
//=============================================================================

TopoDS_Vertex StdMeshers_ProjectionSource2D::GetSourceVertex(int i) const
{
  if ( i == 1 )
    return _sourceVertex1;
  else if ( i == 2 )
    return _sourceVertex2;
  else
    throw SALOME_Exception(LOCALIZED("Wrong vertex index"));
}

//=============================================================================
/*!
 * Returns the <i>-th target vertex associated with the <i>-th source vertex.
 * Result may be nil if association not set.
 */
//=============================================================================

TopoDS_Vertex StdMeshers_ProjectionSource2D::GetTargetVertex(int i) const
{
  if ( i == 1 )
    return _targetVertex1;
  else if ( i == 2 )
    return _targetVertex2;
  else
    throw SALOME_Exception(LOCALIZED("Wrong vertex index"));
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_ProjectionSource2D::SaveTo(ostream & save)
{
  // we store it in order to be able to detect that hypo is really modified
  save << " " << _sourceFace.TShape().operator->()  ;
  save << " " << _sourceVertex1.TShape().operator->();
  save << " " << _targetVertex1.TShape().operator->();
  save << " " << _sourceVertex2.TShape().operator->();
  save << " " << _targetVertex2.TShape().operator->();
  save << " " << ( _sourceMesh ? _sourceMesh->GetId() : -1 );
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_ProjectionSource2D::LoadFrom(istream & load)
{
  // impossible to restore w/o any context
  // It is done by servant
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_ProjectionSource2D & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_ProjectionSource2D & hyp)
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

bool StdMeshers_ProjectionSource2D::SetParametersByMesh(const SMESH_Mesh*   ,
                                                        const TopoDS_Shape& )
{
  return false;
}

//================================================================================
/*!
 * \brief Return all parameters
 */
//================================================================================

void StdMeshers_ProjectionSource2D::GetStoreParams(TopoDS_Shape& s1,
                                                   TopoDS_Shape& s2,
                                                   TopoDS_Shape& s3,
                                                   TopoDS_Shape& s4,
                                                   TopoDS_Shape& s5) const
{
  s1 = _sourceFace;
  s2 = _sourceVertex1;
  s3 = _sourceVertex2;
  s4 = _targetVertex1;
  s5 = _targetVertex2;
}

//================================================================================
/*!
 * \brief Set all parameters without notifying on modification
 */
//================================================================================

void StdMeshers_ProjectionSource2D::RestoreParams(const TopoDS_Shape& s1,
                                                  const TopoDS_Shape& s2,
                                                  const TopoDS_Shape& s3,
                                                  const TopoDS_Shape& s4,
                                                  const TopoDS_Shape& s5,
                                                  SMESH_Mesh*         mesh)
{
  _sourceFace    = s1;
  _sourceVertex1 = TopoDS::Vertex( s2 );
  _sourceVertex2 = TopoDS::Vertex( s3 );
  _targetVertex1 = TopoDS::Vertex( s4 );
  _targetVertex2 = TopoDS::Vertex( s5 );
  _sourceMesh   = mesh;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_ProjectionSource2D::SetParametersByDefaults(const TDefaults&  /*dflts*/,
                                                            const SMESH_Mesh* /*theMesh*/)
{
  return false;
}

