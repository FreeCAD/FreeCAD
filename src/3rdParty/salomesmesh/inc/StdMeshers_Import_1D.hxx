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
//  Module : SMESH
//
#ifndef _SMESH_Import_1D_HXX_
#define _SMESH_Import_1D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Algo.hxx"
#include "SMDS_MeshElement.hxx"

class StdMeshers_ImportSource1D;

/*!
 * \brief Copy elements from other the mesh
 */
class STDMESHERS_EXPORT StdMeshers_Import_1D: public SMESH_1D_Algo
{
public:
  StdMeshers_Import_1D(int hypId, int studyId, SMESH_Gen* gen);

  virtual bool CheckHypothesis(SMESH_Mesh& aMesh,
                               const TopoDS_Shape& aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute (SMESH_Mesh & aMesh, const TopoDS_Shape & aShape);
  virtual bool Evaluate(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape,
                        MapShapeNbElems& aResMap);

  virtual void SetEventListener(SMESH_subMesh* subMesh);
  virtual void SubmeshRestored(SMESH_subMesh* subMesh);

  // internal utilities

  typedef std::map<const SMDS_MeshNode*,   const SMDS_MeshNode*,   TIDCompare> TNodeNodeMap;
  typedef std::map<const SMDS_MeshElement*,const SMDS_MeshElement*,TIDCompare> TElemElemMap;

  static void getMaps(const SMESH_Mesh* srcMesh,
                      SMESH_Mesh*       tgtMesh,
                      TNodeNodeMap*&    n2n,
                      TElemElemMap*&    e2e);

  static void importMesh(const SMESH_Mesh*          srcMesh,
                         SMESH_Mesh &               tgtMesh,
                         StdMeshers_ImportSource1D* srcHyp,
                         const TopoDS_Shape&        tgtShape);

  static void setEventListener( SMESH_subMesh*             subMesh,
                                StdMeshers_ImportSource1D* sourceHyp );

  static SMESH_subMesh* getSubMeshOfCopiedMesh( SMESH_Mesh& tgtMesh,
                                                SMESH_Mesh& srcMesh );

 private:
  
  StdMeshers_ImportSource1D* _sourceHyp;
};

#endif
