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
// File      : StdMeshers_SegmentAroundVertex_0D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_SegmentAroundVertex_0D.hxx"

//=======================================================================
//function : StdMeshers_SegmentAroundVertex_0D
//purpose  : 
//=======================================================================

StdMeshers_SegmentAroundVertex_0D::StdMeshers_SegmentAroundVertex_0D
                                   (int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_0D_Algo(hypId, studyId, gen)
{
  _name = "SegmentAroundVertex_0D";
  // it is assigned to vertices but influence a state of EDGE submeshes 
  _shapeType = (1 << TopAbs_VERTEX);    // 1 bit per shape type

  _compatibleHypothesis.push_back("SegmentLengthAroundVertex");
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_SegmentAroundVertex_0D::~StdMeshers_SegmentAroundVertex_0D()
{}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_SegmentAroundVertex_0D::CheckHypothesis(SMESH_Mesh&                          aMesh,
                                                        const TopoDS_Shape&                  aShape,
                                                        SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  list <const SMESHDS_Hypothesis * >::const_iterator itl;

  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(aMesh, aShape);
  if ( hyps.size() == 0 )
  {
    aStatus = SMESH_Hypothesis::HYP_MISSING;
    return false;  // can't work with no hypothesis
  }

  if ( hyps.size() > 1 )
  {
    aStatus = SMESH_Hypothesis::HYP_ALREADY_EXIST;
  }
  else
  {
    aStatus = SMESH_Hypothesis::HYP_OK;
  }
  return ( aStatus == HYP_OK );
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_SegmentAroundVertex_0D::Compute(SMESH_Mesh&, const TopoDS_Shape&)
{
  // This algorithm exists in order just to enable assignation of
  // StdMeshers_SegmentLengthAroundVertex hypothesis
  return true;
}


//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================

bool StdMeshers_SegmentAroundVertex_0D::Evaluate(SMESH_Mesh&,
                                                 const TopoDS_Shape&,
                                                 MapShapeNbElems&)
{
  // This algorithm exists in order just to enable assignation of
  // StdMeshers_SegmentLengthAroundVertex hypothesis
  return false;
}
