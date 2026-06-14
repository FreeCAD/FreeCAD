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
//  File   : StdMeshers_SegmentLengthAroundVertex.cxx
//  Module : SMESH
//
#include "StdMeshers_SegmentLengthAroundVertex.hxx"

#include "SMESH_Mesh.hxx"
#include "SMESH_Algo.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_MesherHelper.hxx"

#include "utilities.h"

#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_SegmentLengthAroundVertex::StdMeshers_SegmentLengthAroundVertex
                                       (int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  _length = 1.;
  _name = "SegmentLengthAroundVertex";
  _param_algo_dim = 0; // is used by StdMeshers_SegmentAroundVertex_0D
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_SegmentLengthAroundVertex::~StdMeshers_SegmentLengthAroundVertex()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_SegmentLengthAroundVertex::SetLength(double length)
{
  if (length <= 0)
    throw SALOME_Exception(LOCALIZED("length must be positive"));
  if (_length != length) {
    _length = length;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_SegmentLengthAroundVertex::GetLength() const
{
  return _length;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_SegmentLengthAroundVertex::SaveTo(ostream & save)
{
  save << this->_length;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_SegmentLengthAroundVertex::LoadFrom(istream & load)
{
  bool isOK = true;
  double a;
  isOK = (bool)(load >> a);
  if (isOK)
    this->_length = a;
  else
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_SegmentLengthAroundVertex & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_SegmentLengthAroundVertex & hyp)
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

bool StdMeshers_SegmentLengthAroundVertex::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                               const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() || theShape.ShapeType() != TopAbs_VERTEX )
    return false;

  SMESH_MeshEditor editor( const_cast<SMESH_Mesh*>( theMesh ) );
  SMESH_MesherHelper helper( *editor.GetMesh() );

  // get node built on theShape vertex
  SMESHDS_Mesh* meshDS = editor.GetMeshDS();
  SMESHDS_SubMesh* smV = meshDS->MeshElements( theShape );
  if ( !smV || smV->NbNodes() == 0 )
    return false;
  const SMDS_MeshNode* vNode = smV->GetNodes()->next();

  // calculate average length of segments sharing vNode

  _length = 0.;
  int nbSegs = 0;

  SMDS_ElemIteratorPtr segIt = vNode->GetInverseElementIterator(SMDSAbs_Edge);
  while ( segIt->more() ) {
    const SMDS_MeshElement* seg = segIt->next();
    // get geom edge
    int shapeID = editor.FindShape( seg );
    if (!shapeID) continue;
    const TopoDS_Shape& s = meshDS->IndexToShape( shapeID );
    if ( s.IsNull() || s.ShapeType() != TopAbs_EDGE ) continue;
    const TopoDS_Edge& edge = TopoDS::Edge( s );
    // params of edge ends
    double u0 = helper.GetNodeU( edge, seg->GetNode(0) );
    double u1 = helper.GetNodeU( edge, seg->GetNode(1) );
    // length
    BRepAdaptor_Curve AdaptCurve( edge );
    _length += GCPnts_AbscissaPoint::Length( AdaptCurve, u0, u1);
    nbSegs++;
  }
  
  if ( nbSegs > 1 )
    _length /= nbSegs;

  return nbSegs;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_SegmentLengthAroundVertex::SetParametersByDefaults(const TDefaults&,
                                                                   const SMESH_Mesh*)
{
  return false;
}

