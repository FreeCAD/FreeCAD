//  SMESH SMESH : implementaion of SMESH idl descriptions
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//
//
//  File   : StdMeshers_CompositeBlock_3D.hxx
//  Module : SMESH

#ifndef _SMESH_CompositeSegment_1D_HXX_
#define _SMESH_CompositeSegment_1D_HXX_

#include "SMESH_StdMeshers.hxx"
#include "SMESH_3D_Algo.hxx"

class SMESH_Mesh;
class StdMeshers_FaceSide;
class TopoDS_Edge;
class TopoDS_Face;

/*!
 * \brief Computes hexahedral mesh on a box with composite sides
 *
 * The algorithm expects faces of a box to be meshed with quadrangles so that
 * opposite box sides have equal number of quadrangles.
 */
class STDMESHERS_EXPORT StdMeshers_CompositeHexa_3D: public SMESH_3D_Algo
{
public:
  StdMeshers_CompositeHexa_3D(int hypId, int studyId, SMESH_Gen* gen);
  //virtual ~StdMeshers_CompositeHexa_3D();

  virtual bool Compute(SMESH_Mesh&         aMesh,
		       const TopoDS_Shape& aShape);

  virtual bool CheckHypothesis(SMESH_Mesh&         aMesh,
                               const TopoDS_Shape& aShape,
                               Hypothesis_Status&  aStatus);

private:
  // private fields
};

#endif
