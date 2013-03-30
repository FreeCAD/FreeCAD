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
//  File   : StdMeshers_Hexa_3D.hxx
//           Moved here from SMESH_Hexa_3D.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#ifndef _SMESH_HEXA_3D_HXX_
#define _SMESH_HEXA_3D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_3D_Algo.hxx"
#include "SMESH_Mesh.hxx"
#include "StdMeshers_Quadrangle_2D.hxx"
#include "SMESH_Exception.hxx"

#include "SMESH_MesherHelper.hxx"

class TopTools_IndexedMapOfShape;

typedef struct point3Dstruct
{
  const SMDS_MeshNode * node;
} Point3DStruct;

typedef double Pt3[3];

typedef struct conv2dstruct
{
  double a1; // X = a1*x + b1*y + c1 
  double b1; // Y = a2*x + b2*y + c2
  double c1; // a1, b1 a2, b2 in {-1,0,1}
  double a2; // c1, c2 in {0,1}
  double b2;
  double c2;
  int ia;    // I = ia*i + ib*j + ic
  int ib;
  int ic;
  int ja;    // J = ja*i + jb*j + jc
  int jb;
  int jc;
} Conv2DStruct;

class STDMESHERS_EXPORT StdMeshers_Hexa_3D:
  public SMESH_3D_Algo
{
public:
  StdMeshers_Hexa_3D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_Hexa_3D();

  virtual bool CheckHypothesis(SMESH_Mesh& aMesh,
                               const TopoDS_Shape& aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh,
		       const TopoDS_Shape& aShape)
    /*throw (SMESH_Exception)*/;

  static TopoDS_Vertex OppositeVertex(const TopoDS_Vertex& aVertex,
                                      const TopTools_IndexedMapOfShape& aQuads0Vertices,
                                      FaceQuadStruct* aQuads[6]);

protected:
  TopoDS_Edge
  EdgeNotInFace(SMESH_Mesh& aMesh,
		const TopoDS_Shape& aShape,
		const TopoDS_Face& aFace,
		const TopoDS_Vertex& aVertex,
		const TopTools_IndexedDataMapOfShapeListOfShape& MS);

  int GetFaceIndex(SMESH_Mesh& aMesh,
		   const TopoDS_Shape& aShape,
		   const std::vector<SMESH_subMesh*>& meshFaces,
		   const TopoDS_Vertex& V0,
		   const TopoDS_Vertex& V1,
		   const TopoDS_Vertex& V2,
		   const TopoDS_Vertex& V3);

  void GetConv2DCoefs(const faceQuadStruct& quad,
		      const TopoDS_Shape& aShape,
		      const TopoDS_Vertex& V0,
		      const TopoDS_Vertex& V1,
		      const TopoDS_Vertex& V2,
		      const TopoDS_Vertex& V3,
		      Conv2DStruct& conv);

  void GetPoint(Pt3 p,
		int i, int j, int k,
		int nbx, int nby, int nbz,
		Point3DStruct *np,
		const SMESHDS_Mesh* meshDS);

  bool ClearAndReturn(FaceQuadStruct* theQuads[6], const bool res);
};

#endif
