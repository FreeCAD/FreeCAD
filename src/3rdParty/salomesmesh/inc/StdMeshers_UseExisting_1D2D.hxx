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
//  File   : StdMeshers_UseExisting_1D2D.hxx
//  Module : SMESH
//
#ifndef _SMESH_UseExisting_1D2D_HXX_
#define _SMESH_UseExisting_1D2D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_1D_Algo.hxx"
#include "SMESH_2D_Algo.hxx"

/*!
 * \brief 1D and 2D algorithms doing nothing to allow mesh generation
 * by mesh edition functions in TUI mode
 */
class STDMESHERS_EXPORT StdMeshers_UseExisting_2D: public SMESH_2D_Algo
{
public:
  StdMeshers_UseExisting_2D(int hypId, int studyId, SMESH_Gen* gen);

  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);
  
};

class STDMESHERS_EXPORT StdMeshers_UseExisting_1D: public SMESH_1D_Algo
{
public:
  StdMeshers_UseExisting_1D(int hypId, int studyId, SMESH_Gen* gen);

  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);
  
};

#endif
