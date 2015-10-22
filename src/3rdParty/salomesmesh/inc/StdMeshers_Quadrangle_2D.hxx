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
//  File   : StdMeshers_Quadrangle_2D.hxx
//           Moved here from SMESH_Quadrangle_2D.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/StdMeshers/StdMeshers_Quadrangle_2D.hxx,v 1.12.2.4 2008/11/27 13:03:50 abd Exp $
//
#ifndef _SMESH_QUADRANGLE_2D_HXX_
#define _SMESH_QUADRANGLE_2D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_2D_Algo.hxx"
#include "SMESH_Exception.hxx"

class SMESH_Mesh;
class SMESH_MesherHelper;
class StdMeshers_FaceSide;
class SMDS_MeshNode;
struct uvPtStruct;

enum TSideID { BOTTOM_SIDE=0, RIGHT_SIDE, TOP_SIDE, LEFT_SIDE, NB_SIDES };

typedef uvPtStruct UVPtStruct;
typedef struct faceQuadStruct
{
  std::vector< StdMeshers_FaceSide*> side;
  bool isEdgeOut[4]; // true, if an edge has more nodes, than the opposite
  UVPtStruct* uv_grid;
  ~faceQuadStruct();
} FaceQuadStruct;

class STDMESHERS_EXPORT StdMeshers_Quadrangle_2D: public SMESH_2D_Algo
{
public:
  StdMeshers_Quadrangle_2D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_Quadrangle_2D();

  virtual bool CheckHypothesis(SMESH_Mesh& aMesh,
                               const TopoDS_Shape& aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh,
		       const TopoDS_Shape& aShape);

  FaceQuadStruct* CheckAnd2Dcompute(SMESH_Mesh& aMesh,
				    const TopoDS_Shape& aShape,
                                    const bool CreateQuadratic);

protected:

  FaceQuadStruct* CheckNbEdges(SMESH_Mesh& aMesh,
                               const TopoDS_Shape& aShape);

  bool SetNormalizedGrid(SMESH_Mesh& aMesh,
			 const TopoDS_Shape& aShape,
			 FaceQuadStruct*& quad);
  
  void SplitQuad(SMESHDS_Mesh *theMeshDS,
                 const int theFaceID,
                 const SMDS_MeshNode* theNode1,
                 const SMDS_MeshNode* theNode2,
                 const SMDS_MeshNode* theNode3,
                 const SMDS_MeshNode* theNode4);

  /**
   * Special function for creation only quandrangle faces
   */
  bool ComputeQuadPref(SMESH_Mesh& aMesh,
                       const TopoDS_Shape& aShape,
                       FaceQuadStruct* quad);

  UVPtStruct* LoadEdgePoints2(SMESH_Mesh& aMesh,
			      const TopoDS_Face& F, const TopoDS_Edge& E,
                              bool IsReverse);

  UVPtStruct* LoadEdgePoints(SMESH_Mesh& aMesh,
			     const TopoDS_Face& F, const TopoDS_Edge& E,
			     double first, double last);

  UVPtStruct* MakeEdgePoints(SMESH_Mesh& aMesh,
			     const TopoDS_Face& F, const TopoDS_Edge& E,
			     double first, double last, int nb_segm);

  // true if QuadranglePreference hypothesis is assigned that forces
  // construction of quadrangles if the number of nodes on opposite edges
  // is not the same in the case where the global number of nodes on edges is even
  bool myQuadranglePreference;

  bool myTrianglePreference;

  
  SMESH_MesherHelper* myTool; // tool for working with quadratic elements
};

#endif
