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
//  File   : StdMeshers_Hexa_3D.cxx
//           Moved here from SMESH_Hexa_3D.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_Hexa_3D.hxx"
#include "StdMeshers_CompositeHexa_3D.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_Penta_3D.hxx"
#include "StdMeshers_Prism_3D.hxx"
#include "StdMeshers_Quadrangle_2D.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_Comment.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_FacePosition.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMDS_VolumeOfNodes.hxx"

#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt2d.hxx>

#include "utilities.h"
#include "SMESH_ExceptHandlers.hxx"

typedef SMESH_Comment TComm;

using namespace std;

static SMESH_ComputeErrorPtr ComputePentahedralMesh(SMESH_Mesh &, const TopoDS_Shape &);

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Hexa_3D::StdMeshers_Hexa_3D(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_3D_Algo(hypId, studyId, gen)
{
  MESSAGE("StdMeshers_Hexa_3D::StdMeshers_Hexa_3D");
  _name = "Hexa_3D";
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);	// 1 bit /shape type
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Hexa_3D::~StdMeshers_Hexa_3D()
{
  MESSAGE("StdMeshers_Hexa_3D::~StdMeshers_Hexa_3D");
}

//================================================================================
/*!
 * \brief Clear fields and return the argument
  * \param res - the value to return
  * \retval bool - the argument value
 */
//================================================================================

bool StdMeshers_Hexa_3D::ClearAndReturn(FaceQuadStruct* theQuads[6], const bool res)
{
  for (int i = 0; i < 6; i++) {
    delete theQuads[i];
    theQuads[i] = NULL;
  }
  return res;
}


//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Hexa_3D::CheckHypothesis
                         (SMESH_Mesh&                          aMesh,
                          const TopoDS_Shape&                  aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  // check nb of faces in the shape
/*  PAL16229
  aStatus = SMESH_Hypothesis::HYP_BAD_GEOMETRY;
  int nbFaces = 0;
  for (TopExp_Explorer exp(aShape, TopAbs_FACE); exp.More(); exp.Next())
    if ( ++nbFaces > 6 )
      break;
  if ( nbFaces != 6 )
    return false;
*/
  aStatus = SMESH_Hypothesis::HYP_OK;
  return true;
}

//=======================================================================
//function : isCloser
//purpose  : 
//=======================================================================

inline bool isCloser(const int i, const int j, const int nbhoriz,
                     const FaceQuadStruct* quad, const gp_Pnt2d uv,
                     double & minDist)
{
  int ij = j * nbhoriz + i;
  gp_Pnt2d uv2( quad->uv_grid[ij].u, quad->uv_grid[ij].v );
  double dist = uv.SquareDistance( uv2 );
  if ( dist < minDist ) {
    minDist = dist;
    return true;
  }
  return false;
}

//=======================================================================
//function : findIJ
//purpose  : return i,j of the node
//=======================================================================

static bool findIJ (const SMDS_MeshNode* node, const FaceQuadStruct * quad, int& I, int& J)
{
  const SMDS_FacePosition* fpos =
    static_cast<const SMDS_FacePosition*>(node->GetPosition().get());
  if ( ! fpos ) return false;
  gp_Pnt2d uv( fpos->GetUParameter(), fpos->GetVParameter() );

  double minDist = DBL_MAX;
  const int nbhoriz  = quad->side[0]->NbPoints();
  const int nbvertic = quad->side[1]->NbPoints();
  I = nbhoriz/2; J = nbvertic/2;
  int oldI, oldJ;
  do {
    oldI = I; oldJ = J;
    while ( I + 2 < nbhoriz &&  isCloser( I + 1, J, nbhoriz, quad, uv, minDist ))
      I += 1;
    if ( I == oldI )
      while ( I - 1 > 0     &&  isCloser( I - 1, J, nbhoriz, quad, uv, minDist ))
        I -= 1;
    if ( minDist < DBL_MIN )
      break;

    while ( J + 2 < nbvertic && isCloser( I, J + 1, nbhoriz, quad, uv, minDist ))
      J += 1;
    if ( J == oldJ )
      while ( J - 1 > 0      && isCloser( I, J - 1, nbhoriz, quad, uv, minDist ))
        J -= 1;
    if ( minDist < DBL_MIN )
      break;

  } while ( I != oldI || J != oldJ );

  if ( minDist > DBL_MIN ) {
    for (int i = 1; i < nbhoriz - 1; i++)
      for (int j = 1; j < nbvertic - 1; j++)
        if ( isCloser( i, j, nbhoriz, quad, uv, minDist ))
          I = i, J = j;
  }
  return true;
}


//=============================================================================
/*!
 * Hexahedron mesh on hexaedron like form
 * -0.  - shape and face mesh verification
 * -1.  - identify faces and vertices of the "cube"
 * -2.  - Algorithm from:
 * "Application de l'interpolation transfinie à la création de maillages
 *  C0 ou G1 continus sur des triangles, quadrangles, tetraedres, pentaedres
 *  et hexaedres déformés."
 * Alain PERONNET - 8 janvier 1999
 */
//=============================================================================

bool StdMeshers_Hexa_3D::Compute(SMESH_Mesh &         aMesh,
                                 const TopoDS_Shape & aShape)// throw(SALOME_Exception)
{
  // PAL14921. Enable catching std::bad_alloc and Standard_OutOfMemory outside
  //Unexpect aCatch(SalomeException);
  MESSAGE("StdMeshers_Hexa_3D::Compute");
  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();

  // 0.  - shape and face mesh verification
  // 0.1 - shape must be a solid (or a shell) with 6 faces

  vector < SMESH_subMesh * >meshFaces;
  for (TopExp_Explorer exp(aShape, TopAbs_FACE); exp.More(); exp.Next()) {
    SMESH_subMesh *aSubMesh = aMesh.GetSubMeshContaining(exp.Current());
    ASSERT(aSubMesh);
    meshFaces.push_back(aSubMesh);
  }
  if (meshFaces.size() != 6) {
    //return error(COMPERR_BAD_SHAPE, TComm(meshFaces.size())<<" instead of 6 faces in a block");
    static StdMeshers_CompositeHexa_3D compositeHexa(-10, 0, aMesh.GetGen());
    if ( !compositeHexa.Compute( aMesh, aShape ))
      return error( compositeHexa.GetComputeError() );
    return true;
  }

  // 0.2 - is each face meshed with Quadrangle_2D? (so, with a wire of 4 edges)

  // tool for working with quadratic elements
  SMESH_MesherHelper aTool (aMesh);
  _quadraticMesh = aTool.IsQuadraticSubMesh(aShape);

  // cube structure
  typedef struct cubeStruct
  {
    TopoDS_Vertex V000;
    TopoDS_Vertex V001;
    TopoDS_Vertex V010;
    TopoDS_Vertex V011;
    TopoDS_Vertex V100;
    TopoDS_Vertex V101;
    TopoDS_Vertex V110;
    TopoDS_Vertex V111;
    faceQuadStruct* quad_X0;
    faceQuadStruct* quad_X1;
    faceQuadStruct* quad_Y0;
    faceQuadStruct* quad_Y1;
    faceQuadStruct* quad_Z0;
    faceQuadStruct* quad_Z1;
    Point3DStruct* np; // normalised 3D coordinates
  } CubeStruct;

  CubeStruct aCube;

  // bounding faces
  FaceQuadStruct* aQuads[6];
  for (int i = 0; i < 6; i++)
    aQuads[i] = 0;

  for (int i = 0; i < 6; i++)
  {
    TopoDS_Shape aFace = meshFaces[i]->GetSubShape();
    SMESH_Algo *algo = _gen->GetAlgo(aMesh, aFace);
    string algoName = algo->GetName();
    bool isAllQuad = false;
    if (algoName == "Quadrangle_2D") {
      SMESHDS_SubMesh * sm = meshDS->MeshElements( aFace );
      if ( sm ) {
        isAllQuad = true;
        SMDS_ElemIteratorPtr eIt = sm->GetElements();
        while ( isAllQuad && eIt->more() ) {
          const SMDS_MeshElement* elem =  eIt->next();
          isAllQuad = ( elem->NbNodes()==4 ||(_quadraticMesh && elem->NbNodes()==8) );
        }
      }
    }
    if ( ! isAllQuad ) {
      SMESH_ComputeErrorPtr err = ComputePentahedralMesh(aMesh, aShape);
      return ClearAndReturn( aQuads, error(err));
    }
    StdMeshers_Quadrangle_2D *quadAlgo =
      dynamic_cast < StdMeshers_Quadrangle_2D * >(algo);
    ASSERT(quadAlgo);
    try {
      aQuads[i] = quadAlgo->CheckAnd2Dcompute(aMesh, aFace, _quadraticMesh);
      if(!aQuads[i]) {
	return error( quadAlgo->GetComputeError());
      }
    }
    catch(SMESH_Exception & S_ex) {
      return ClearAndReturn( aQuads, error(COMPERR_SLM_EXCEPTION,TComm(S_ex.what()) <<
                                           " Raised by StdMeshers_Quadrangle_2D "
                                           " on face #" << meshDS->ShapeToIndex( aFace )));
    }

    // 0.2.1 - number of points on the opposite edges must be the same
    if (aQuads[i]->side[0]->NbPoints() != aQuads[i]->side[2]->NbPoints() ||
        aQuads[i]->side[1]->NbPoints() != aQuads[i]->side[3]->NbPoints()
        /*aQuads[i]->side[0]->NbEdges() != 1 ||
        aQuads[i]->side[1]->NbEdges() != 1 ||
        aQuads[i]->side[2]->NbEdges() != 1 ||
        aQuads[i]->side[3]->NbEdges() != 1*/) {
      MESSAGE("different number of points on the opposite edges of face " << i);
      // Try to go into penta algorithm 'cause it has been improved.
      SMESH_ComputeErrorPtr err = ComputePentahedralMesh(aMesh, aShape);
      return ClearAndReturn( aQuads, error(err));
    }
  }

  // 1.  - identify faces and vertices of the "cube"
  // 1.1 - ancestor maps vertex->edges in the cube

//   TopTools_IndexedDataMapOfShapeListOfShape MS;
//   TopExp::MapShapesAndAncestors(aShape, TopAbs_VERTEX, TopAbs_EDGE, MS);

  // 1.2 - first face is choosen as face Y=0 of the unit cube

  const TopoDS_Shape & aFace = meshFaces[0]->GetSubShape();
  //const TopoDS_Face & F = TopoDS::Face(aFace);

  // 1.3 - identify the 4 vertices of the face Y=0: V000, V100, V101, V001

  aCube.V000 = aQuads[0]->side[0]->FirstVertex(); // will be (0,0,0) on the unit cube
  aCube.V100 = aQuads[0]->side[0]->LastVertex();  // will be (1,0,0) on the unit cube
  aCube.V001 = aQuads[0]->side[2]->FirstVertex(); // will be (0,0,1) on the unit cube
  aCube.V101 = aQuads[0]->side[2]->LastVertex();  // will be (1,0,1) on the unit cube

  TopTools_IndexedMapOfShape MV0;
  TopExp::MapShapes(aFace, TopAbs_VERTEX, MV0);

  aCube.V010 = OppositeVertex( aCube.V000, MV0, aQuads);
  aCube.V110 = OppositeVertex( aCube.V100, MV0, aQuads);
  aCube.V011 = OppositeVertex( aCube.V001, MV0, aQuads);
  aCube.V111 = OppositeVertex( aCube.V101, MV0, aQuads);

  // 1.6 - find remaining faces given 4 vertices

  int _indY0 = 0;
  int _indY1 = GetFaceIndex(aMesh, aShape, meshFaces,
                            aCube.V010, aCube.V011, aCube.V110, aCube.V111);
  int _indZ0 = GetFaceIndex(aMesh, aShape, meshFaces,
                            aCube.V000, aCube.V010, aCube.V100, aCube.V110);
  int _indZ1 = GetFaceIndex(aMesh, aShape, meshFaces,
                            aCube.V001, aCube.V011, aCube.V101, aCube.V111);
  int _indX0 = GetFaceIndex(aMesh, aShape, meshFaces,
                            aCube.V000, aCube.V001, aCube.V010, aCube.V011);
  int _indX1 = GetFaceIndex(aMesh, aShape, meshFaces,
                            aCube.V100, aCube.V101, aCube.V110, aCube.V111);

  // IPAL21120:	SIGSEGV on Meshing attached Compound with Automatic Hexadralization
  if ( _indY1 < 1 || _indZ0 < 1 || _indZ1 < 1 || _indX0 < 1 || _indX1 < 1 )
    return error(COMPERR_BAD_SHAPE);

  aCube.quad_Y0 = aQuads[_indY0];
  aCube.quad_Y1 = aQuads[_indY1];
  aCube.quad_Z0 = aQuads[_indZ0];
  aCube.quad_Z1 = aQuads[_indZ1];
  aCube.quad_X0 = aQuads[_indX0];
  aCube.quad_X1 = aQuads[_indX1];

  // 1.7 - get convertion coefs from face 2D normalized to 3D normalized

  Conv2DStruct cx0;			// for face X=0
  Conv2DStruct cx1;			// for face X=1
  Conv2DStruct cy0;
  Conv2DStruct cy1;
  Conv2DStruct cz0;
  Conv2DStruct cz1;

  GetConv2DCoefs(*aCube.quad_X0, meshFaces[_indX0]->GetSubShape(),
                 aCube.V000, aCube.V010, aCube.V011, aCube.V001, cx0);
  GetConv2DCoefs(*aCube.quad_X1, meshFaces[_indX1]->GetSubShape(),
                 aCube.V100, aCube.V110, aCube.V111, aCube.V101, cx1);
  GetConv2DCoefs(*aCube.quad_Y0, meshFaces[_indY0]->GetSubShape(),
                 aCube.V000, aCube.V100, aCube.V101, aCube.V001, cy0);
  GetConv2DCoefs(*aCube.quad_Y1, meshFaces[_indY1]->GetSubShape(),
                 aCube.V010, aCube.V110, aCube.V111, aCube.V011, cy1);
  GetConv2DCoefs(*aCube.quad_Z0, meshFaces[_indZ0]->GetSubShape(),
                 aCube.V000, aCube.V100, aCube.V110, aCube.V010, cz0);
  GetConv2DCoefs(*aCube.quad_Z1, meshFaces[_indZ1]->GetSubShape(),
                 aCube.V001, aCube.V101, aCube.V111, aCube.V011, cz1);

  // 1.8 - create a 3D structure for normalized values
  
  int nbx = aCube.quad_Z0->side[0]->NbPoints();
  if (cz0.a1 == 0.) nbx = aCube.quad_Z0->side[1]->NbPoints();
 
  int nby = aCube.quad_X0->side[0]->NbPoints();
  if (cx0.a1 == 0.) nby = aCube.quad_X0->side[1]->NbPoints();
 
  int nbz = aCube.quad_Y0->side[0]->NbPoints();
  if (cy0.a1 != 0.) nbz = aCube.quad_Y0->side[1]->NbPoints();

  int i1, j1, nbxyz = nbx * nby * nbz;
  Point3DStruct *np = new Point3DStruct[nbxyz];

  // 1.9 - store node indexes of faces

  {
    const TopoDS_Face & F = TopoDS::Face(meshFaces[_indX0]->GetSubShape());

    faceQuadStruct *quad = aCube.quad_X0;
    int i = 0;				// j = x/face , k = y/face
    int nbdown = quad->side[0]->NbPoints();
    int nbright = quad->side[1]->NbPoints();

    SMDS_NodeIteratorPtr itf= aMesh.GetSubMesh(F)->GetSubMeshDS()->GetNodes();
			
    while(itf->more()) {
      const SMDS_MeshNode * node = itf->next();
      if(aTool.IsMedium(node))
        continue;
      if ( !findIJ( node, quad, i1, j1 ))
        return ClearAndReturn( aQuads, false );
      int ij1 = j1 * nbdown + i1;
      quad->uv_grid[ij1].node = node;
    }

    for (int i1 = 0; i1 < nbdown; i1++)
      for (int j1 = 0; j1 < nbright; j1++) {
        int ij1 = j1 * nbdown + i1;
        int j = cx0.ia * i1 + cx0.ib * j1 + cx0.ic;	// j = x/face
        int k = cx0.ja * i1 + cx0.jb * j1 + cx0.jc;	// k = y/face
        int ijk = k * nbx * nby + j * nbx + i;
        //MESSAGE(" "<<ij1<<" "<<i<<" "<<j<<" "<<ijk);
        np[ijk].node = quad->uv_grid[ij1].node;
        //SCRUTE(np[ijk].nodeId);
      }
  }

  {
    const TopoDS_Face & F = TopoDS::Face(meshFaces[_indX1]->GetSubShape());

    SMDS_NodeIteratorPtr itf= aMesh.GetSubMesh(F)->GetSubMeshDS()->GetNodes();

    faceQuadStruct *quad = aCube.quad_X1;
    int i = nbx - 1;		// j = x/face , k = y/face
    int nbdown = quad->side[0]->NbPoints();
    int nbright = quad->side[1]->NbPoints();

    while(itf->more()) {
      const SMDS_MeshNode * node = itf->next();
      if(aTool.IsMedium(node))
        continue;
      if ( !findIJ( node, quad, i1, j1 ))
        return ClearAndReturn( aQuads, false );
      int ij1 = j1 * nbdown + i1;
      quad->uv_grid[ij1].node = node;
    }

    for (int i1 = 0; i1 < nbdown; i1++)
      for (int j1 = 0; j1 < nbright; j1++) {
        int ij1 = j1 * nbdown + i1;
        int j = cx1.ia * i1 + cx1.ib * j1 + cx1.ic;	// j = x/face
        int k = cx1.ja * i1 + cx1.jb * j1 + cx1.jc;	// k = y/face
        int ijk = k * nbx * nby + j * nbx + i;
        //MESSAGE(" "<<ij1<<" "<<i<<" "<<j<<" "<<ijk);
        np[ijk].node = quad->uv_grid[ij1].node;
        //SCRUTE(np[ijk].nodeId);
      }
  }

  {
    const TopoDS_Face & F = TopoDS::Face(meshFaces[_indY0]->GetSubShape());

    SMDS_NodeIteratorPtr itf= aMesh.GetSubMesh(F)->GetSubMeshDS()->GetNodes();

    faceQuadStruct *quad = aCube.quad_Y0;
    int j = 0;				// i = x/face , k = y/face
    int nbdown = quad->side[0]->NbPoints();
    int nbright = quad->side[1]->NbPoints();

    while(itf->more()) {
      const SMDS_MeshNode * node = itf->next();
      if(aTool.IsMedium(node))
        continue;
      if ( !findIJ( node, quad, i1, j1 ))
        return ClearAndReturn( aQuads, false );
      int ij1 = j1 * nbdown + i1;
      quad->uv_grid[ij1].node = node;
    }

    for (int i1 = 0; i1 < nbdown; i1++)
      for (int j1 = 0; j1 < nbright; j1++) {
        int ij1 = j1 * nbdown + i1;
        int i = cy0.ia * i1 + cy0.ib * j1 + cy0.ic;	// i = x/face
        int k = cy0.ja * i1 + cy0.jb * j1 + cy0.jc;	// k = y/face
        int ijk = k * nbx * nby + j * nbx + i;
        //MESSAGE(" "<<ij1<<" "<<i<<" "<<j<<" "<<ijk);
        np[ijk].node = quad->uv_grid[ij1].node;
        //SCRUTE(np[ijk].nodeId);
      }
  }

  {
    const TopoDS_Face & F = TopoDS::Face(meshFaces[_indY1]->GetSubShape());

    SMDS_NodeIteratorPtr itf= aMesh.GetSubMesh(F)->GetSubMeshDS()->GetNodes();

    faceQuadStruct *quad = aCube.quad_Y1;
    int j = nby - 1;		// i = x/face , k = y/face
    int nbdown = quad->side[0]->NbPoints();
    int nbright = quad->side[1]->NbPoints();

    while(itf->more()) {
      const SMDS_MeshNode * node = itf->next();
      if(aTool.IsMedium(node))
        continue;
      if ( !findIJ( node, quad, i1, j1 ))
        return ClearAndReturn( aQuads, false );
      int ij1 = j1 * nbdown + i1;
      quad->uv_grid[ij1].node = node;
    }

    for (int i1 = 0; i1 < nbdown; i1++)
      for (int j1 = 0; j1 < nbright; j1++) {
        int ij1 = j1 * nbdown + i1;
        int i = cy1.ia * i1 + cy1.ib * j1 + cy1.ic;	// i = x/face
        int k = cy1.ja * i1 + cy1.jb * j1 + cy1.jc;	// k = y/face
        int ijk = k * nbx * nby + j * nbx + i;
        //MESSAGE(" "<<ij1<<" "<<i<<" "<<j<<" "<<ijk);
        np[ijk].node = quad->uv_grid[ij1].node;
        //SCRUTE(np[ijk].nodeId);
      }
  }

  {
    const TopoDS_Face & F = TopoDS::Face(meshFaces[_indZ0]->GetSubShape());

    SMDS_NodeIteratorPtr itf= aMesh.GetSubMesh(F)->GetSubMeshDS()->GetNodes();

    faceQuadStruct *quad = aCube.quad_Z0;
    int k = 0;				// i = x/face , j = y/face
    int nbdown = quad->side[0]->NbPoints();
    int nbright = quad->side[1]->NbPoints();

    while(itf->more()) {
      const SMDS_MeshNode * node = itf->next();
      if(aTool.IsMedium(node))
        continue;
      if ( !findIJ( node, quad, i1, j1 ))
        return ClearAndReturn( aQuads, false );
      int ij1 = j1 * nbdown + i1;
      quad->uv_grid[ij1].node = node;
    }

    for (int i1 = 0; i1 < nbdown; i1++)
      for (int j1 = 0; j1 < nbright; j1++) {
        int ij1 = j1 * nbdown + i1;
        int i = cz0.ia * i1 + cz0.ib * j1 + cz0.ic;	// i = x/face
        int j = cz0.ja * i1 + cz0.jb * j1 + cz0.jc;	// j = y/face
        int ijk = k * nbx * nby + j * nbx + i;
        //MESSAGE(" "<<ij1<<" "<<i<<" "<<j<<" "<<ijk);
        np[ijk].node = quad->uv_grid[ij1].node;
        //SCRUTE(np[ijk].nodeId);
      }
  }

  {
    const TopoDS_Face & F = TopoDS::Face(meshFaces[_indZ1]->GetSubShape());

    SMDS_NodeIteratorPtr itf= aMesh.GetSubMesh(F)->GetSubMeshDS()->GetNodes();

    faceQuadStruct *quad = aCube.quad_Z1;
    int k = nbz - 1;		// i = x/face , j = y/face
    int nbdown = quad->side[0]->NbPoints();
    int nbright = quad->side[1]->NbPoints();
    
    while(itf->more()) {
      const SMDS_MeshNode * node = itf->next();
      if(aTool.IsMedium(node))
        continue;
      if ( !findIJ( node, quad, i1, j1 ))
        return ClearAndReturn( aQuads, false );
      int ij1 = j1 * nbdown + i1;
      quad->uv_grid[ij1].node = node;
    }

    for (int i1 = 0; i1 < nbdown; i1++)
      for (int j1 = 0; j1 < nbright; j1++) {
        int ij1 = j1 * nbdown + i1;
        int i = cz1.ia * i1 + cz1.ib * j1 + cz1.ic;	// i = x/face
        int j = cz1.ja * i1 + cz1.jb * j1 + cz1.jc;	// j = y/face
        int ijk = k * nbx * nby + j * nbx + i;
        //MESSAGE(" "<<ij1<<" "<<i<<" "<<j<<" "<<ijk);
        np[ijk].node = quad->uv_grid[ij1].node;
        //SCRUTE(np[ijk].nodeId);
      }
  }

  // 2.0 - for each node of the cube:
  //       - get the 8 points 3D = 8 vertices of the cube
  //       - get the 12 points 3D on the 12 edges of the cube
  //       - get the 6 points 3D on the 6 faces with their ID
  //       - compute the point 3D
  //       - store the point 3D in SMESHDS, store its ID in 3D structure

  int shapeID = meshDS->ShapeToIndex( aShape );

  Pt3 p000, p001, p010, p011, p100, p101, p110, p111;
  Pt3 px00, px01, px10, px11;
  Pt3 p0y0, p0y1, p1y0, p1y1;
  Pt3 p00z, p01z, p10z, p11z;
  Pt3 pxy0, pxy1, px0z, px1z, p0yz, p1yz;

  GetPoint(p000, 0, 0, 0, nbx, nby, nbz, np, meshDS);
  GetPoint(p001, 0, 0, nbz - 1, nbx, nby, nbz, np, meshDS);
  GetPoint(p010, 0, nby - 1, 0, nbx, nby, nbz, np, meshDS);
  GetPoint(p011, 0, nby - 1, nbz - 1, nbx, nby, nbz, np, meshDS);
  GetPoint(p100, nbx - 1, 0, 0, nbx, nby, nbz, np, meshDS);
  GetPoint(p101, nbx - 1, 0, nbz - 1, nbx, nby, nbz, np, meshDS);
  GetPoint(p110, nbx - 1, nby - 1, 0, nbx, nby, nbz, np, meshDS);
  GetPoint(p111, nbx - 1, nby - 1, nbz - 1, nbx, nby, nbz, np, meshDS);

  for (int i = 1; i < nbx - 1; i++) {
    for (int j = 1; j < nby - 1; j++) {
      for (int k = 1; k < nbz - 1; k++) {
        // *** seulement maillage regulier
        // 12 points on edges
        GetPoint(px00, i, 0, 0, nbx, nby, nbz, np, meshDS);
        GetPoint(px01, i, 0, nbz - 1, nbx, nby, nbz, np, meshDS);
        GetPoint(px10, i, nby - 1, 0, nbx, nby, nbz, np, meshDS);
        GetPoint(px11, i, nby - 1, nbz - 1, nbx, nby, nbz, np, meshDS);

        GetPoint(p0y0, 0, j, 0, nbx, nby, nbz, np, meshDS);
        GetPoint(p0y1, 0, j, nbz - 1, nbx, nby, nbz, np, meshDS);
        GetPoint(p1y0, nbx - 1, j, 0, nbx, nby, nbz, np, meshDS);
        GetPoint(p1y1, nbx - 1, j, nbz - 1, nbx, nby, nbz, np, meshDS);

        GetPoint(p00z, 0, 0, k, nbx, nby, nbz, np, meshDS);
        GetPoint(p01z, 0, nby - 1, k, nbx, nby, nbz, np, meshDS);
        GetPoint(p10z, nbx - 1, 0, k, nbx, nby, nbz, np, meshDS);
        GetPoint(p11z, nbx - 1, nby - 1, k, nbx, nby, nbz, np, meshDS);

        // 12 points on faces
        GetPoint(pxy0, i, j, 0, nbx, nby, nbz, np, meshDS);
        GetPoint(pxy1, i, j, nbz - 1, nbx, nby, nbz, np, meshDS);
        GetPoint(px0z, i, 0, k, nbx, nby, nbz, np, meshDS);
        GetPoint(px1z, i, nby - 1, k, nbx, nby, nbz, np, meshDS);
        GetPoint(p0yz, 0, j, k, nbx, nby, nbz, np, meshDS);
        GetPoint(p1yz, nbx - 1, j, k, nbx, nby, nbz, np, meshDS);

        int ijk = k * nbx * nby + j * nbx + i;
        double x = double (i) / double (nbx - 1);	// *** seulement
        double y = double (j) / double (nby - 1);	// *** maillage
        double z = double (k) / double (nbz - 1);	// *** regulier

        Pt3 X;
        for (int i = 0; i < 3; i++) {
          X[i] = (1 - x) * p0yz[i] + x * p1yz[i]
                 + (1 - y) * px0z[i] + y * px1z[i]
                 + (1 - z) * pxy0[i] + z * pxy1[i]
                 - (1 - x) * ((1 - y) * p00z[i] + y * p01z[i])
                 - x * ((1 - y) * p10z[i] + y * p11z[i])
                 - (1 - y) * ((1 - z) * px00[i] + z * px01[i])
                 - y * ((1 - z) * px10[i] + z * px11[i])
                 - (1 - z) * ((1 - x) * p0y0[i] + x * p1y0[i])
                 - z * ((1 - x) * p0y1[i] + x * p1y1[i])
                 + (1 - x) * ((1 - y) * ((1 - z) * p000[i] + z * p001[i])
                 + y * ((1 - z) * p010[i] + z * p011[i]))
                 + x * ((1 - y) * ((1 - z) * p100[i] + z * p101[i])
                 + y * ((1 - z) * p110[i] + z * p111[i]));
        }

        SMDS_MeshNode * node = meshDS->AddNode(X[0], X[1], X[2]);
        np[ijk].node = node;
        meshDS->SetNodeInVolume(node, shapeID);
      }
    }
  }

  // find orientation of furute volumes according to MED convention
  vector< bool > forward( nbx * nby );
  SMDS_VolumeTool vTool;
  for (int i = 0; i < nbx - 1; i++) {
    for (int j = 0; j < nby - 1; j++) {
      int n1 = j * nbx + i;
      int n2 = j * nbx + i + 1;
      int n3 = (j + 1) * nbx + i + 1;
      int n4 = (j + 1) * nbx + i;
      int n5 = nbx * nby + j * nbx + i;
      int n6 = nbx * nby + j * nbx + i + 1;
      int n7 = nbx * nby + (j + 1) * nbx + i + 1;
      int n8 = nbx * nby + (j + 1) * nbx + i;

      SMDS_VolumeOfNodes tmpVol (np[n1].node,np[n2].node,np[n3].node,np[n4].node,
                                 np[n5].node,np[n6].node,np[n7].node,np[n8].node);
      vTool.Set( &tmpVol );
      forward[ n1 ] = vTool.IsForward();
    }
  }

  //2.1 - for each node of the cube (less 3 *1 Faces):
  //      - store hexahedron in SMESHDS
  MESSAGE("Storing hexahedron into the DS");
  for (int i = 0; i < nbx - 1; i++) {
    for (int j = 0; j < nby - 1; j++) {
      bool isForw = forward.at( j * nbx + i );
      for (int k = 0; k < nbz - 1; k++) {
        int n1 = k * nbx * nby + j * nbx + i;
        int n2 = k * nbx * nby + j * nbx + i + 1;
        int n3 = k * nbx * nby + (j + 1) * nbx + i + 1;
        int n4 = k * nbx * nby + (j + 1) * nbx + i;
        int n5 = (k + 1) * nbx * nby + j * nbx + i;
        int n6 = (k + 1) * nbx * nby + j * nbx + i + 1;
        int n7 = (k + 1) * nbx * nby + (j + 1) * nbx + i + 1;
        int n8 = (k + 1) * nbx * nby + (j + 1) * nbx + i;

        SMDS_MeshVolume * elt;
        if ( isForw ) {
          elt = aTool.AddVolume(np[n1].node, np[n2].node,
                                np[n3].node, np[n4].node,
                                np[n5].node, np[n6].node,
                                np[n7].node, np[n8].node);
        }
        else {
          elt = aTool.AddVolume(np[n1].node, np[n4].node,
                                np[n3].node, np[n2].node,
                                np[n5].node, np[n8].node,
                                np[n7].node, np[n6].node);
        }
        
        meshDS->SetMeshElementOnShape(elt, shapeID);
      }
    }
  }
  if ( np ) delete [] np;
  return ClearAndReturn( aQuads, true );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_Hexa_3D::GetPoint(Pt3 p, int i, int j, int k, int nbx, int nby, int nbz,
                                  Point3DStruct * np, const SMESHDS_Mesh * meshDS)
{
	int ijk = k * nbx * nby + j * nbx + i;
	const SMDS_MeshNode * node = np[ijk].node;
	p[0] = node->X();
	p[1] = node->Y();
	p[2] = node->Z();
	//MESSAGE(" "<<i<<" "<<j<<" "<<k<<" "<<p[0]<<" "<<p[1]<<" "<<p[2]);
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

int StdMeshers_Hexa_3D::GetFaceIndex(SMESH_Mesh & aMesh,
	const TopoDS_Shape & aShape,
	const vector < SMESH_subMesh * >&meshFaces,
	const TopoDS_Vertex & V0,
	const TopoDS_Vertex & V1,
	const TopoDS_Vertex & V2, const TopoDS_Vertex & V3)
{
	//MESSAGE("StdMeshers_Hexa_3D::GetFaceIndex");
	int faceIndex = -1;
	for (int i = 1; i < 6; i++)
	{
		const TopoDS_Shape & aFace = meshFaces[i]->GetSubShape();
		//const TopoDS_Face& F = TopoDS::Face(aFace);
		TopTools_IndexedMapOfShape M;
		TopExp::MapShapes(aFace, TopAbs_VERTEX, M);
		bool verticesInShape = false;
		if (M.Contains(V0))
			if (M.Contains(V1))
				if (M.Contains(V2))
					if (M.Contains(V3))
						verticesInShape = true;
		if (verticesInShape)
		{
			faceIndex = i;
			break;
		}
	}
	//IPAL21120 ASSERT(faceIndex > 0);
	//SCRUTE(faceIndex);
	return faceIndex;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

TopoDS_Edge
	StdMeshers_Hexa_3D::EdgeNotInFace(SMESH_Mesh & aMesh,
	const TopoDS_Shape & aShape,
	const TopoDS_Face & aFace,
	const TopoDS_Vertex & aVertex,
	const TopTools_IndexedDataMapOfShapeListOfShape & MS)
{
	//MESSAGE("StdMeshers_Hexa_3D::EdgeNotInFace");
	TopTools_IndexedDataMapOfShapeListOfShape MF;
	TopExp::MapShapesAndAncestors(aFace, TopAbs_VERTEX, TopAbs_EDGE, MF);
	const TopTools_ListOfShape & ancestorsInSolid = MS.FindFromKey(aVertex);
	const TopTools_ListOfShape & ancestorsInFace = MF.FindFromKey(aVertex);
// 	SCRUTE(ancestorsInSolid.Extent());
// 	SCRUTE(ancestorsInFace.Extent());
	ASSERT(ancestorsInSolid.Extent() == 6);	// 6 (edges doublees)
	ASSERT(ancestorsInFace.Extent() == 2);

	TopoDS_Edge E;
	E.Nullify();
	TopTools_ListIteratorOfListOfShape its(ancestorsInSolid);
	for (; its.More(); its.Next())
	{
		TopoDS_Shape ancestor = its.Value();
		TopTools_ListIteratorOfListOfShape itf(ancestorsInFace);
		bool isInFace = false;
		for (; itf.More(); itf.Next())
		{
			TopoDS_Shape ancestorInFace = itf.Value();
			if (ancestorInFace.IsSame(ancestor))
			{
				isInFace = true;
				break;
			}
		}
		if (!isInFace)
		{
			E = TopoDS::Edge(ancestor);
			break;
		}
	}
	return E;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_Hexa_3D::GetConv2DCoefs(const faceQuadStruct & quad,
	const TopoDS_Shape & aShape,
	const TopoDS_Vertex & V0,
	const TopoDS_Vertex & V1,
	const TopoDS_Vertex & V2, const TopoDS_Vertex & V3, Conv2DStruct & conv)
{
//	MESSAGE("StdMeshers_Hexa_3D::GetConv2DCoefs");
// 	const TopoDS_Face & F = TopoDS::Face(aShape);
// 	TopoDS_Edge E = quad.edge[0];
// 	double f, l;
// 	Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface(E, F, f, l);
// 	TopoDS_Vertex VFirst, VLast;
// 	TopExp::Vertices(E, VFirst, VLast);	// corresponds to f and l
// 	bool isForward = (((l - f) * (quad.last[0] - quad.first[0])) > 0);
  TopoDS_Vertex VA, VB;
// 	if (isForward)
// 	{
// 		VA = VFirst;
// 		VB = VLast;
// 	}
// 	else
// 	{
// 		VA = VLast;
// 		VB = VFirst;
// 	}
  VA = quad.side[0]->FirstVertex();
  VB = quad.side[0]->LastVertex();
  
	int a1, b1, c1, a2, b2, c2;
	if (VA.IsSame(V0))
		if (VB.IsSame(V1))
		{
			a1 = 1;
			b1 = 0;
			c1 = 0;				// x
			a2 = 0;
			b2 = 1;
			c2 = 0;				// y
		}
		else
		{
			ASSERT(VB.IsSame(V3));
			a1 = 0;
			b1 = 1;
			c1 = 0;				// y
			a2 = 1;
			b2 = 0;
			c2 = 0;				// x
		}
	if (VA.IsSame(V1))
		if (VB.IsSame(V2))
		{
			a1 = 0;
			b1 = -1;
			c1 = 1;				// 1-y
			a2 = 1;
			b2 = 0;
			c2 = 0;				// x
		}
		else
		{
			ASSERT(VB.IsSame(V0));
			a1 = -1;
			b1 = 0;
			c1 = 1;				// 1-x
			a2 = 0;
			b2 = 1;
			c2 = 0;				// y
		}
	if (VA.IsSame(V2))
		if (VB.IsSame(V3))
		{
			a1 = -1;
			b1 = 0;
			c1 = 1;				// 1-x
			a2 = 0;
			b2 = -1;
			c2 = 1;				// 1-y
		}
		else
		{
			ASSERT(VB.IsSame(V1));
			a1 = 0;
			b1 = -1;
			c1 = 1;				// 1-y
			a2 = -1;
			b2 = 0;
			c2 = 1;				// 1-x
		}
	if (VA.IsSame(V3))
		if (VB.IsSame(V0))
		{
			a1 = 0;
			b1 = 1;
			c1 = 0;				// y
			a2 = -1;
			b2 = 0;
			c2 = 1;				// 1-x
		}
		else
		{
			ASSERT(VB.IsSame(V2));
			a1 = 1;
			b1 = 0;
			c1 = 0;				// x
			a2 = 0;
			b2 = -1;
			c2 = 1;				// 1-y
		}
// 	MESSAGE("X = " << c1 << "+ " << a1 << "*x + " << b1 << "*y");
// 	MESSAGE("Y = " << c2 << "+ " << a2 << "*x + " << b2 << "*y");
	conv.a1 = a1;
	conv.b1 = b1;
	conv.c1 = c1;
	conv.a2 = a2;
	conv.b2 = b2;
	conv.c2 = c2;

	int nbdown = quad.side[0]->NbPoints();
	int nbright = quad.side[1]->NbPoints();
	conv.ia = int (a1);
	conv.ib = int (b1);
	conv.ic =
		int (c1 * a1 * a1) * (nbdown - 1) + int (c1 * b1 * b1) * (nbright - 1);
	conv.ja = int (a2);
	conv.jb = int (b2);
	conv.jc =
		int (c2 * a2 * a2) * (nbdown - 1) + int (c2 * b2 * b2) * (nbright - 1);
// 	MESSAGE("I " << conv.ia << " " << conv.ib << " " << conv.ic);
// 	MESSAGE("J " << conv.ja << " " << conv.jb << " " << conv.jc);
}

//================================================================================
/*!
 * \brief Find a vertex opposite to the given vertex of aQuads[0]
  * \param aVertex - the vertex
  * \param aFace - the face aVertex belongs to
  * \param aQuads - quads
  * \retval TopoDS_Vertex - found vertex
 */
//================================================================================

TopoDS_Vertex StdMeshers_Hexa_3D::OppositeVertex(const TopoDS_Vertex& aVertex,
                                                 const TopTools_IndexedMapOfShape& aQuads0Vertices,
                                                 FaceQuadStruct* aQuads[6])
{
  int i, j;
  for ( i = 1; i < 6; ++i )
  {
    TopoDS_Vertex VV[] = { aQuads[i]->side[0]->FirstVertex(),
                           aQuads[i]->side[0]->LastVertex() , 
                           aQuads[i]->side[2]->LastVertex() ,
                           aQuads[i]->side[2]->FirstVertex() };
    for ( j = 0; j < 4; ++j )
      if ( aVertex.IsSame( VV[ j ]))
        break;
    if ( j < 4 ) {
      int jPrev = j ? j - 1 : 3;
      int jNext = (j + 1) % 4;
      if ( aQuads0Vertices.Contains( VV[ jPrev ] ))
        return VV[ jNext ];
      else
        return VV[ jPrev ];
    }
  }
  return TopoDS_Vertex();
}

//modified by NIZNHY-PKV Wed Nov 17 15:34:13 2004 f
///////////////////////////////////////////////////////////////////////////////
//ZZ
//#include <stdio.h>

//=======================================================================
//function : ComputePentahedralMesh
//purpose  : 
//=======================================================================

SMESH_ComputeErrorPtr ComputePentahedralMesh(SMESH_Mesh &         aMesh,
                                             const TopoDS_Shape & aShape)
{
  //printf(" ComputePentahedralMesh HERE\n");
  //
  bool bOK;
  SMESH_ComputeErrorPtr err = SMESH_ComputeError::New();
  //int iErr;
  StdMeshers_Penta_3D anAlgo;
  //
  bOK=anAlgo.Compute(aMesh, aShape);
  //
  err = anAlgo.GetComputeError();
  //
  if ( !bOK && anAlgo.ErrorStatus() == 5 )
  {
    static StdMeshers_Prism_3D * aPrism3D = 0;
    if ( !aPrism3D ) {
      SMESH_Gen* gen = aMesh.GetGen();
      aPrism3D = new StdMeshers_Prism_3D( gen->GetANewId(), 0, gen );
    }
    SMESH_Hypothesis::Hypothesis_Status aStatus;
    if ( aPrism3D->CheckHypothesis( aMesh, aShape, aStatus ) ) {
      bOK = aPrism3D->Compute( aMesh, aShape );
      err = aPrism3D->GetComputeError();
    }
  }
  return err;
}


