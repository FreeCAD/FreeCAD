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
//  File   : StdMeshers_SegmentAroundVertex_0D.hxx
//  Module : SMESH
//
#ifndef _SMESH_SegmentAroundVertex_0D_HXX_
#define _SMESH_SegmentAroundVertex_0D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_0D_Algo.hxx"

/*!
 * \brief Algorithm existing in order just to enable assignation of
 * StdMeshers_SegmentLengthAroundVertex hypothesis
 */
class STDMESHERS_EXPORT StdMeshers_SegmentAroundVertex_0D: public SMESH_0D_Algo
{
public:
  StdMeshers_SegmentAroundVertex_0D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_SegmentAroundVertex_0D();

  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);
  
};

#endif
