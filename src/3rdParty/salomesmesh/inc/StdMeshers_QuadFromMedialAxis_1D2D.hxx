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
// File      : StdMeshers_QuadFromMedialAxis_1D2D.hxx
// Created   : Wed Jun  3 17:22:35 2015
// Author    : Edward AGAPOV (eap)


#ifndef __StdMeshers_QuadFromMedialAxis_1D2D_HXX__
#define __StdMeshers_QuadFromMedialAxis_1D2D_HXX__

#include "StdMeshers_Quadrangle_2D.hxx"

#include <vector>

/*!
 * \brief Quadrangle mesher using Medial Axis
 */
class STDMESHERS_EXPORT StdMeshers_QuadFromMedialAxis_1D2D: public StdMeshers_Quadrangle_2D
{
 public:
  StdMeshers_QuadFromMedialAxis_1D2D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_QuadFromMedialAxis_1D2D();

  virtual bool CheckHypothesis(SMESH_Mesh&         aMesh,
                               const TopoDS_Shape& aShape,
                               Hypothesis_Status&  aStatus);

  virtual bool Compute(SMESH_Mesh&         aMesh,
                       const TopoDS_Shape& aShape);

  virtual bool Evaluate(SMESH_Mesh &         aMesh,
                        const TopoDS_Shape & aShape,
                        MapShapeNbElems&     aResMap);

  virtual void SetEventListener(SMESH_subMesh* subMesh);

  static bool IsApplicable(const TopoDS_Shape & aShape, bool toCheckAll);

  class Algo1D;

 private:

  bool computeQuads( SMESH_MesherHelper& theHelper,
                     FaceQuadStruct::Ptr theQuad);

  Algo1D*                   _regular1D;
  const SMESHDS_Hypothesis* _hyp2D;
};

#endif
