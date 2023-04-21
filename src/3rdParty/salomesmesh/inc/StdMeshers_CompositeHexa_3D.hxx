// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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
//  File   : StdMeshers_CompositeBlock_3D.hxx
//  Module : SMESH
//
#ifndef _SMESH_CompositeSegment_1D_HXX_
#define _SMESH_CompositeSegment_1D_HXX_

#include "SMESH_StdMeshers.hxx"
#include "SMESH_Algo.hxx"

class SMESH_Mesh;
class StdMeshers_FaceSide;
class TopoDS_Edge;
class TopoDS_Face;
class _QuadFaceGrid;

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

  virtual bool Evaluate(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape,
                        MapShapeNbElems& aResMap);

  virtual bool CheckHypothesis(SMESH_Mesh&         aMesh,
                               const TopoDS_Shape& aShape,
                               Hypothesis_Status&  aStatus);

private:

  bool findBoxFaces( const TopoDS_Shape&    shape,
                     std::list< _QuadFaceGrid >& boxFaceContainer,
                     SMESH_Mesh&            mesh,
                     _QuadFaceGrid * &      fBottom,
                     _QuadFaceGrid * &      fTop,
                     _QuadFaceGrid * &      fFront,
                     _QuadFaceGrid * &      fBack,
                     _QuadFaceGrid * &      fLeft,
                     _QuadFaceGrid * &      fRight);
};

#endif
