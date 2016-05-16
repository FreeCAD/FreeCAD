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

// File      : StdMeshers_PolygonPerFace_2D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_PolygonPerFace_2D.hxx"

#include "SMESH_Comment.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_ProxyMesh.hxx"
#include "SMESH_subMesh.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_ViscousLayers2D.hxx"

#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>

#include <vector>
#include <TopoDS.hxx>

using namespace std;

//=======================================================================
//function : StdMeshers_PolygonPerFace_2D
//purpose  : 
//=======================================================================

StdMeshers_PolygonPerFace_2D::StdMeshers_PolygonPerFace_2D(int        hypId,
                                                           int        studyId,
                                                           SMESH_Gen* gen)
  :SMESH_2D_Algo(hypId, studyId, gen)
{
  _name = "PolygonPerFace_2D";
}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_PolygonPerFace_2D::CheckHypothesis(SMESH_Mesh&                          theMesh,
                                                   const TopoDS_Shape&                  theShape,
                                                   SMESH_Hypothesis::Hypothesis_Status& theStatus)
{
  theStatus = HYP_OK;
  return true;
}

//=======================================================================
//function : Compute
//purpose  :
//=======================================================================

bool StdMeshers_PolygonPerFace_2D::Compute(SMESH_Mesh&         theMesh,
                                           const TopoDS_Shape& theShape)
{
  const TopoDS_Face& face = TopoDS::Face( theShape );

  SMESH_MesherHelper helper( theMesh );
  helper.SetElementsOnShape( true );
  _quadraticMesh = helper.IsQuadraticSubMesh( face );

  SMESH_ProxyMesh::Ptr proxyMesh = StdMeshers_ViscousLayers2D::Compute( theMesh, face );
  if ( !proxyMesh )
    return false;

  TError erorr;
  TSideVector wires = StdMeshers_FaceSide::GetFaceWires(face, theMesh,
                                                        /*skipMediumNodes=*/_quadraticMesh,
                                                        erorr, proxyMesh,
                                                        /*checkVertexNodes=*/false);
  if ( wires.size() != 1 )
    return error( COMPERR_BAD_SHAPE, SMESH_Comment("One wire required, not ") << wires.size() );

  vector<const SMDS_MeshNode*> nodes = wires[0]->GetOrderedNodes();
  int nbNodes = int( nodes.size() ) - 1; // 1st node is repeated at end

  switch ( nbNodes ) {
  case 3:
    helper.AddFace( nodes[0], nodes[1], nodes[2] );
    break;
  case 4:
    helper.AddFace( nodes[0], nodes[1], nodes[2], nodes[3] );
    break;
  default:
    if ( nbNodes < 3 )
      return error( COMPERR_BAD_INPUT_MESH, "Less that 3 nodes on the wire" );
    nodes.resize( nodes.size() - 1 );
    helper.AddPolygonalFace ( nodes );
  }

  return true;
}

//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================

bool StdMeshers_PolygonPerFace_2D::Evaluate(SMESH_Mesh&         theMesh,
                                            const TopoDS_Shape& theShape,
                                            MapShapeNbElems&    theResMap)
{
  // count nb segments
  int nbLinSegs = 0, nbQuadSegs = 0;
  TopExp_Explorer edge( theShape, TopAbs_EDGE );
  for ( ; edge.More(); edge.Next() )
  {
    SMESH_subMesh* sm = theMesh.GetSubMesh( edge.Current() );
    MapShapeNbElems::iterator sm2vec = theResMap.find( sm );
    if ( sm2vec == theResMap.end() )
      continue;
    nbLinSegs  += sm2vec->second.at( SMDSEntity_Edge );
    nbQuadSegs += sm2vec->second.at( SMDSEntity_Quad_Edge );
  }

  std::vector<int> aVec( SMDSEntity_Last, 0 );
  switch ( nbLinSegs + nbQuadSegs ) {
  case 3:
    aVec[ nbQuadSegs ? SMDSEntity_Quad_Triangle : SMDSEntity_Triangle ] = 1;
    break;
  case 4:
    aVec[ nbQuadSegs ? SMDSEntity_Quad_Quadrangle : SMDSEntity_Quadrangle ] = 1;
    break;
  default:
    if ( nbLinSegs + nbQuadSegs < 3 )
      return error( COMPERR_BAD_INPUT_MESH, "Less that 3 nodes on the wire" );  
#ifndef VTK_NO_QUAD_POLY
    aVec[ nbQuadSegs ? SMDSEntity_Quad_Polygon : SMDSEntity_Polygon ] = 1;
#else
    if(nbQuadSegs)
        throw SALOME_Exception("Quadratic polygon not supported with VTK <6.2");
    
    aVec[ SMDSEntity_Polygon ] = 1;
#endif
  }

  SMESH_subMesh * sm = theMesh.GetSubMesh(theShape);
  theResMap.insert(std::make_pair(sm,aVec));

  return true;
}
