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
//  File   : StdMeshers_QuadToTriaAdaptor.hxx
//  Module : SMESH
//
#ifndef _SMESH_QuadToTriaAdaptor_HXX_
#define _SMESH_QuadToTriaAdaptor_HXX_

#include <SMESH_Mesh.hxx>
#include <SMESH_StdMeshers.hxx>
#include <SMDS_FaceOfNodes.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TColgp_HArray1OfVec.hxx>

#include <map>
#include <list>
#include <vector>

class STDMESHERS_EXPORT StdMeshers_QuadToTriaAdaptor
{
public:

  StdMeshers_QuadToTriaAdaptor();

  ~StdMeshers_QuadToTriaAdaptor();

  bool Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);

  bool Compute(SMESH_Mesh& aMesh);

  const std::list<const SMDS_FaceOfNodes*>* GetTriangles(const SMDS_MeshElement* aFace);

protected:

  //bool CheckDegenerate(const SMDS_MeshElement* aFace);

  int Preparation(const SMDS_MeshElement* face,
                  Handle(TColgp_HArray1OfPnt)& PN,
                  Handle(TColgp_HArray1OfVec)& VN,
                  std::vector<const SMDS_MeshNode*>& FNodes,
                  gp_Pnt& PC, gp_Vec& VNorm);

  bool CheckIntersection(const gp_Pnt& P, const gp_Pnt& PC,
                         gp_Pnt& Pint, SMESH_Mesh& aMesh,
                         const TopoDS_Shape& aShape,
                         const TopoDS_Shape& NotCheckedFace);

  bool Compute2ndPart(SMESH_Mesh& aMesh);

  typedef std::map< const SMDS_MeshElement*, const SMDS_MeshElement*, TIDCompare > TF2PyramMap;

  std::map< const SMDS_MeshElement*, std::list<const SMDS_FaceOfNodes*> > myResMap;
  TF2PyramMap myMapFPyram;
  std::list< const SMDS_MeshNode* > myDegNodes;

};

#endif
