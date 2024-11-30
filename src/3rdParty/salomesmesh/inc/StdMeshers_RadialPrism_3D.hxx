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
//  File   : StdMeshers_RadialPrism_3D.hxx
//  Module : SMESH
//
#ifndef _SMESH_RadialPrism_3D_HXX_
#define _SMESH_RadialPrism_3D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Algo.hxx"
#include "SMDS_MeshNode.hxx"

#include <vector>
#include <map>

class StdMeshers_NumberOfLayers;
class StdMeshers_LayerDistribution;
class SMESH_MesherHelper;
class gp_Pnt;

class STDMESHERS_EXPORT StdMeshers_RadialPrism_3D: public SMESH_3D_Algo
{
public:
  StdMeshers_RadialPrism_3D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_RadialPrism_3D();

  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);

  virtual bool Evaluate(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape,
                        MapShapeNbElems& aResMap);

  static bool IsApplicable(const TopoDS_Shape & aShape, bool toCheckAll);

protected:

  typedef std::vector<const SMDS_MeshNode* >            TNodeColumn;
  typedef std::map< const SMDS_MeshNode*, TNodeColumn > TNode2ColumnMap;

  TNodeColumn* makeNodeColumn( TNode2ColumnMap&     n2ColMap,
                               const SMDS_MeshNode* outNode,
                               const SMDS_MeshNode* inNode);

  bool computeLayerPositions(const gp_Pnt& pIn,
                             const gp_Pnt& pOut);


  const StdMeshers_NumberOfLayers*    myNbLayerHypo;
  const StdMeshers_LayerDistribution* myDistributionHypo;
  SMESH_MesherHelper*                 myHelper;
  std::vector< double >               myLayerPositions;
};

#endif
