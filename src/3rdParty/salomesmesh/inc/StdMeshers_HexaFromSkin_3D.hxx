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

// File      : StdMeshers_HexaFromSkin_3D.hxx
// Created   : Wed Jan 27 12:23:21 2010
// Author    : Edward AGAPOV (eap)
//
#ifndef __StdMeshers_HexaFromSkin_3D_HXX__
#define __StdMeshers_HexaFromSkin_3D_HXX__

#include "SMESH_StdMeshers.hxx"
#include "SMESH_Algo.hxx"

/*!
 * \brief algorithm generating hexahedral mesh from 2D skin of block
 */

class STDMESHERS_EXPORT StdMeshers_HexaFromSkin_3D : public SMESH_3D_Algo
{
public:
  StdMeshers_HexaFromSkin_3D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_HexaFromSkin_3D();

  virtual bool Compute(SMESH_Mesh & aMesh, SMESH_MesherHelper* aHelper);

  virtual bool CheckHypothesis(SMESH_Mesh& aMesh,
                               const TopoDS_Shape& aShape,
                               Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);

  virtual bool Evaluate(SMESH_Mesh &         aMesh,
                        const TopoDS_Shape & aShape,
                        MapShapeNbElems&     aResMap);
  
};

#endif
