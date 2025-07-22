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

// File      : NETGENPlugin_NETGEN_2D_ONLY.hxx
// Project   : SALOME
// Author    : Edward AGAPOV (OCC)
//
#ifndef _NETGENPlugin_NETGEN_2D_ONLY_HXX_
#define _NETGENPlugin_NETGEN_2D_ONLY_HXX_

#include <SMESH_Algo.hxx>
#include <SMESH_Mesh.hxx>

class StdMeshers_MaxElementArea;
class StdMeshers_LengthFromEdges;
class NETGENPlugin_Hypothesis_2D;

/*!
 * \brief Mesher generating 2D elements on a geometrical face taking
 * into account pre-existing nodes on face boundaries
 *
 * Historically, NETGENPlugin_NETGEN_2D is actually 1D-2D, that is why
 * the class is named NETGENPlugin_NETGEN_2D_ONLY. Renaming is useless as
 * algorithm field "_name" can't be changed
 */
class NETGENPlugin_NETGEN_2D_ONLY: public SMESH_2D_Algo
{
public:
  NETGENPlugin_NETGEN_2D_ONLY(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~NETGENPlugin_NETGEN_2D_ONLY();

  virtual bool CheckHypothesis(SMESH_Mesh&         aMesh,
                               const TopoDS_Shape& aShape,
                               Hypothesis_Status&  aStatus);

  virtual bool Compute(SMESH_Mesh&         aMesh,
                       const TopoDS_Shape& aShape);

  virtual void CancelCompute();

  virtual double GetProgress() const;

  virtual bool Evaluate(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape,
                        MapShapeNbElems& aResMap);

protected:
  const StdMeshers_MaxElementArea*       _hypMaxElementArea;
  const StdMeshers_LengthFromEdges*      _hypLengthFromEdges;
  const SMESHDS_Hypothesis*              _hypQuadranglePreference;
  const NETGENPlugin_Hypothesis_2D*      _hypParameters;

  double                                 _progressByTic;
};

#endif
