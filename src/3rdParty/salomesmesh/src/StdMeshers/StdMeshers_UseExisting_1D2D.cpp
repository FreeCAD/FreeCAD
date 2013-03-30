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
// File      : StdMeshers_UseExisting_1D2D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_UseExisting_1D2D.hxx"

//=======================================================================
//function : StdMeshers_UseExisting_1D
//purpose  : 
//=======================================================================

StdMeshers_UseExisting_1D::StdMeshers_UseExisting_1D
                                   (int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_1D_Algo(hypId, studyId, gen)
{
  _name = "UseExisting_1D";
  _shapeType = (1 << TopAbs_EDGE); // 1 bit per shape type
  _requireShape = false;
}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_UseExisting_1D::CheckHypothesis(SMESH_Mesh& ,
                                                const TopoDS_Shape& ,
                                                Hypothesis_Status& aStatus)
{
  return (aStatus = HYP_OK) == HYP_OK;
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_UseExisting_1D::Compute(SMESH_Mesh&, const TopoDS_Shape&)
{
  // This algorithm exists to allow mesh generation by mesh edition functions in TUI mode
  return true;
}

//=======================================================================
//function : StdMeshers_UseExisting_2D
//purpose  : 
//=======================================================================

StdMeshers_UseExisting_2D::StdMeshers_UseExisting_2D
                                   (int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_2D_Algo(hypId, studyId, gen)
{
  _name = "UseExisting_2D";
  _shapeType = (1 << TopAbs_FACE); // 1 bit per shape type
  _requireShape = false;
}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_UseExisting_2D::CheckHypothesis(SMESH_Mesh& ,
                                                const TopoDS_Shape& ,
                                                Hypothesis_Status& aStatus)
{
  return (aStatus = HYP_OK) == HYP_OK;
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_UseExisting_2D::Compute(SMESH_Mesh&, const TopoDS_Shape&)
{
  // This algorithm exists to allow mesh generation by mesh edition functions in TUI mode
  return true;
}
