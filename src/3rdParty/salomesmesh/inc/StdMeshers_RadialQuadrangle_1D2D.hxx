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
//  File   : StdMeshers_RadialQuadrangle_1D2D.hxx
//  Module : SMESH
//
#ifndef _SMESH_RadialQuadrangle_1D2D_HXX_
#define _SMESH_RadialQuadrangle_1D2D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Algo.hxx"

#include <TopoDS_Edge.hxx>

#include <vector>

class StdMeshers_NumberOfLayers;
class StdMeshers_LayerDistribution;
class SMESH_MesherHelper;
class gp_Pnt;

class STDMESHERS_EXPORT StdMeshers_RadialQuadrangle_1D2D: public SMESH_2D_Algo
{
public:
  StdMeshers_RadialQuadrangle_1D2D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_RadialQuadrangle_1D2D();

  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);

  virtual bool Evaluate(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape,
                        MapShapeNbElems& aResMap);
  /*!
   * \brief Allow algo to do something after persistent restoration
    * \param subMesh - restored submesh
   *
   * This method is called only if a submesh has HYP_OK algo_state.
   */
  virtual void SubmeshRestored(SMESH_subMesh* subMesh);
  
  static bool IsApplicable(const TopoDS_Shape & aShape, bool toCheckAll);

protected:

  bool computeLayerPositions(const gp_Pnt&      p1,
                             const gp_Pnt&      p2,
                             const TopoDS_Edge& linEdge=TopoDS_Edge(),
                             bool*              linEdgeComputed = 0);


  const StdMeshers_NumberOfLayers*    myNbLayerHypo;
  const StdMeshers_LayerDistribution* myDistributionHypo;
  SMESH_MesherHelper*                 myHelper;
  std::vector< double >               myLayerPositions;
};

#endif
