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
//  File   : StdMeshers_Quadrangle_2D.cxx
//           Moved here from SMESH_Quadrangle_2D.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/StdMeshers/StdMeshers_Quadrangle_2D.cxx,v 1.14.2.5 2008/11/27 13:03:49 abd Exp $
//
#include "StdMeshers_Quadrangle_2D.hxx"

#include "StdMeshers_FaceSide.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Comment.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_EdgePosition.hxx"
#include "SMDS_FacePosition.hxx"

#include <BRepTools_WireExplorer.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <Precision.hxx>
#include <TColStd_SequenceOfReal.hxx>
#include <TColgp_SequenceOfXY.hxx>
#include <TopExp.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>

#ifndef __BORLANDC__
#include <NCollection_DefineArray2.hxx>
#else
#include <SMESH_DefineArray2.hxx>
#endif

#include "utilities.h"
#include "SMESH_ExceptHandlers.hxx"

#include <cmath>

#ifndef StdMeshers_Array2OfNode_HeaderFile
#define StdMeshers_Array2OfNode_HeaderFile
typedef const SMDS_MeshNode* SMDS_MeshNodePtr;
DEFINE_BASECOLLECTION (StdMeshers_BaseCollectionNodePtr, SMDS_MeshNodePtr)

#ifndef __BORLANDC__
DEFINE_ARRAY2(StdMeshers_Array2OfNode,
			  StdMeshers_BaseCollectionNodePtr, SMDS_MeshNodePtr)
#else
SMESH_DEFINE_ARRAY2(StdMeshers_Array2OfNode,
			  StdMeshers_BaseCollectionNodePtr, SMDS_MeshNodePtr)
#endif

#endif

using namespace std;

typedef gp_XY gp_UV;
typedef SMESH_Comment TComm;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Quadrangle_2D::StdMeshers_Quadrangle_2D (int hypId, int studyId, SMESH_Gen* gen)
     : SMESH_2D_Algo(hypId, studyId, gen)
{
  MESSAGE("StdMeshers_Quadrangle_2D::StdMeshers_Quadrangle_2D");
  _name = "Quadrangle_2D";
  _shapeType = (1 << TopAbs_FACE);
  _compatibleHypothesis.push_back("QuadranglePreference");
  _compatibleHypothesis.push_back("TrianglePreference");
  myTool = 0;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Quadrangle_2D::~StdMeshers_Quadrangle_2D()
{
  MESSAGE("StdMeshers_Quadrangle_2D::~StdMeshers_Quadrangle_2D");
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Quadrangle_2D::CheckHypothesis
                         (SMESH_Mesh&                          aMesh,
                          const TopoDS_Shape&                  aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  bool isOk = true;
  aStatus = SMESH_Hypothesis::HYP_OK;


  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(aMesh, aShape, false);
  const SMESHDS_Hypothesis *theHyp = 0;
  
  if(hyps.size() > 0){
    theHyp = *hyps.begin();
    if(strcmp("QuadranglePreference", theHyp->GetName()) == 0) {
      myQuadranglePreference= true;
      myTrianglePreference= false; 
    }
    else if(strcmp("TrianglePreference", theHyp->GetName()) == 0){
      myQuadranglePreference= false;
      myTrianglePreference= true; 
    }
  }
  else {
    myQuadranglePreference = false;
    myTrianglePreference = false;
  }
  return isOk;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Quadrangle_2D::Compute (SMESH_Mesh& aMesh,
                                        const TopoDS_Shape& aShape)// throw (SMESH_Exception)
{
  // PAL14921. Enable catching std::bad_alloc and Standard_OutOfMemory outside
  //Unexpect aCatchSalomeException);

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  aMesh.GetSubMesh(aShape);

  SMESH_MesherHelper helper(aMesh);
  myTool = &helper;

  _quadraticMesh = myTool->IsQuadraticSubMesh(aShape);

  FaceQuadStruct *quad = CheckNbEdges( aMesh, aShape );
  std::auto_ptr<FaceQuadStruct> quadDeleter( quad ); // to delete quad at exit from Compute()
  if (!quad)
    return false;

  if(myQuadranglePreference) {
    int n1 = quad->side[0]->NbPoints();
    int n2 = quad->side[1]->NbPoints();
    int n3 = quad->side[2]->NbPoints();
    int n4 = quad->side[3]->NbPoints();
    int nfull = n1+n2+n3+n4;
    int ntmp = nfull/2;
    ntmp = ntmp*2;
    if( nfull==ntmp && ( (n1!=n3) || (n2!=n4) ) ) {
      // special path for using only quandrangle faces
      bool ok = ComputeQuadPref(aMesh, aShape, quad);
      return ok;
    }
  }

  // set normalized grid on unit square in parametric domain
  
  if (!SetNormalizedGrid(aMesh, aShape, quad))
    return false;

  // --- compute 3D values on points, store points & quadrangles

  int nbdown  = quad->side[0]->NbPoints();
  int nbup    = quad->side[2]->NbPoints();

  int nbright = quad->side[1]->NbPoints();
  int nbleft  = quad->side[3]->NbPoints();

  int nbhoriz  = Min(nbdown, nbup);
  int nbvertic = Min(nbright, nbleft);

  const TopoDS_Face& F = TopoDS::Face(aShape);
  Handle(Geom_Surface) S = BRep_Tool::Surface(F);

  // internal mesh nodes
  int i, j, geomFaceID = meshDS->ShapeToIndex( F );
  for (i = 1; i < nbhoriz - 1; i++) {
    for (j = 1; j < nbvertic - 1; j++) {
      int ij = j * nbhoriz + i;
      double u = quad->uv_grid[ij].u;
      double v = quad->uv_grid[ij].v;
      gp_Pnt P = S->Value(u, v);
      SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
      meshDS->SetNodeOnFace(node, geomFaceID, u, v);
      quad->uv_grid[ij].node = node;
    }
  }
  
  // mesh faces

  //             [2]
  //      --.--.--.--.--.--  nbvertic
  //     |                 | ^
  //     |                 | ^
  // [3] |                 | ^ j  [1]
  //     |                 | ^
  //     |                 | ^
  //      ---.----.----.---  0
  //     0 > > > > > > > > nbhoriz
  //              i
  //             [0]
  
  i = 0;
  int ilow = 0;
  int iup = nbhoriz - 1;
  if (quad->isEdgeOut[3]) { ilow++; } else { if (quad->isEdgeOut[1]) iup--; }
  
  int jlow = 0;
  int jup = nbvertic - 1;
  if (quad->isEdgeOut[0]) { jlow++; } else { if (quad->isEdgeOut[2]) jup--; }
  
  // regular quadrangles
  for (i = ilow; i < iup; i++) {
    for (j = jlow; j < jup; j++) {
      const SMDS_MeshNode *a, *b, *c, *d;
      a = quad->uv_grid[j * nbhoriz + i].node;
      b = quad->uv_grid[j * nbhoriz + i + 1].node;
      c = quad->uv_grid[(j + 1) * nbhoriz + i + 1].node;
      d = quad->uv_grid[(j + 1) * nbhoriz + i].node;
      //SMDS_MeshFace* face = myTool->AddFace(a, b, c, d);
      //meshDS->SetMeshElementOnShape(face, geomFaceID);
	   if(!myTrianglePreference){
          SMDS_MeshFace* face = myTool->AddFace(a, b, c, d);
          meshDS->SetMeshElementOnShape(face, geomFaceID);
        }
        else {
          SplitQuad(meshDS, geomFaceID, a, b, c, d);
        }
    }
  }

  const vector<UVPtStruct>& uv_e0 = quad->side[0]->GetUVPtStruct(true,0 );
  const vector<UVPtStruct>& uv_e1 = quad->side[1]->GetUVPtStruct(false,1);
  const vector<UVPtStruct>& uv_e2 = quad->side[2]->GetUVPtStruct(true,1 );
  const vector<UVPtStruct>& uv_e3 = quad->side[3]->GetUVPtStruct(false,0);

  if ( uv_e0.empty() || uv_e1.empty() || uv_e2.empty() || uv_e3.empty() )
    return error( COMPERR_BAD_INPUT_MESH );

  double eps = Precision::Confusion();

  // Boundary quadrangles
  
  if (quad->isEdgeOut[0]) {
    // Down edge is out
    // 
    // |___|___|___|___|___|___|
    // |   |   |   |   |   |   |
    // |___|___|___|___|___|___|
    // |   |   |   |   |   |   |
    // |___|___|___|___|___|___| __ first row of the regular grid
    // .  .  .  .  .  .  .  .  . __ down edge nodes
    // 
    // >->->->->->->->->->->->-> -- direction of processing
      
    int g = 0; // number of last processed node in the regular grid
    
    // number of last node of the down edge to be processed
    int stop = nbdown - 1;
    // if right edge is out, we will stop at a node, previous to the last one
    if (quad->isEdgeOut[1]) stop--;
    
    // for each node of the down edge find nearest node
    // in the first row of the regular grid and link them
    for (i = 0; i < stop; i++) {
      const SMDS_MeshNode *a, *b, *c, *d;
      a = uv_e0[i].node;
      b = uv_e0[i + 1].node;
      gp_Pnt pb (b->X(), b->Y(), b->Z());
      
      // find node c in the regular grid, which will be linked with node b
      int near = g;
      if (i == stop - 1) {
        // right bound reached, link with the rightmost node
        near = iup;
        c = quad->uv_grid[nbhoriz + iup].node;
      }
      else {
        // find in the grid node c, nearest to the b
        double mind = RealLast();
        for (int k = g; k <= iup; k++) {
          
          const SMDS_MeshNode *nk;
          if (k < ilow) // this can be, if left edge is out
            nk = uv_e3[1].node; // get node from the left edge
          else
            nk = quad->uv_grid[nbhoriz + k].node; // get one of middle nodes

          gp_Pnt pnk (nk->X(), nk->Y(), nk->Z());
          double dist = pb.Distance(pnk);
          if (dist < mind - eps) {
            c = nk;
            near = k;
            mind = dist;
          } else {
            break;
          }
        }
      }

      if (near == g) { // make triangle
        //SMDS_MeshFace* face = meshDS->AddFace(a, b, c);
        SMDS_MeshFace* face = myTool->AddFace(a, b, c);
        meshDS->SetMeshElementOnShape(face, geomFaceID);
      }
      else { // make quadrangle
        if (near - 1 < ilow)
          d = uv_e3[1].node;
        else
          d = quad->uv_grid[nbhoriz + near - 1].node;
        //SMDS_MeshFace* face = meshDS->AddFace(a, b, c, d);
        
        if(!myTrianglePreference){
          SMDS_MeshFace* face = myTool->AddFace(a, b, c, d);
          meshDS->SetMeshElementOnShape(face, geomFaceID);
        }
        else {
          SplitQuad(meshDS, geomFaceID, a, b, c, d);
        }

        // if node d is not at position g - make additional triangles
        if (near - 1 > g) {
          for (int k = near - 1; k > g; k--) {
            c = quad->uv_grid[nbhoriz + k].node;
            if (k - 1 < ilow)
              d = uv_e3[1].node;
            else
              d = quad->uv_grid[nbhoriz + k - 1].node;
            //SMDS_MeshFace* face = meshDS->AddFace(a, c, d);
            SMDS_MeshFace* face = myTool->AddFace(a, c, d);
            meshDS->SetMeshElementOnShape(face, geomFaceID);
          }
        }
        g = near;
      }
    }
  } else {
    if (quad->isEdgeOut[2]) {
      // Up edge is out
      // 
      // <-<-<-<-<-<-<-<-<-<-<-<-< -- direction of processing
      // 
      // .  .  .  .  .  .  .  .  . __ up edge nodes
      //  ___ ___ ___ ___ ___ ___  __ first row of the regular grid
      // |   |   |   |   |   |   |
      // |___|___|___|___|___|___|
      // |   |   |   |   |   |   |
      // |___|___|___|___|___|___|
      // |   |   |   |   |   |   |

      int g = nbhoriz - 1; // last processed node in the regular grid

      int stop = 0;
      // if left edge is out, we will stop at a second node
      if (quad->isEdgeOut[3]) stop++;

      // for each node of the up edge find nearest node
      // in the first row of the regular grid and link them
      for (i = nbup - 1; i > stop; i--) {
        const SMDS_MeshNode *a, *b, *c, *d;
        a = uv_e2[i].node;
        b = uv_e2[i - 1].node;
        gp_Pnt pb (b->X(), b->Y(), b->Z());

        // find node c in the grid, which will be linked with node b
        int near = g;
        if (i == stop + 1) { // left bound reached, link with the leftmost node
          c = quad->uv_grid[nbhoriz*(nbvertic - 2) + ilow].node;
          near = ilow;
        } else {
          // find node c in the grid, nearest to the b
          double mind = RealLast();
          for (int k = g; k >= ilow; k--) {
            const SMDS_MeshNode *nk;
            if (k > iup)
              nk = uv_e1[nbright - 2].node;
            else
              nk = quad->uv_grid[nbhoriz*(nbvertic - 2) + k].node;
            gp_Pnt pnk (nk->X(), nk->Y(), nk->Z());
            double dist = pb.Distance(pnk);
            if (dist < mind - eps) {
              c = nk;
              near = k;
              mind = dist;
            } else {
              break;
            }
          }
        }

        if (near == g) { // make triangle
          //SMDS_MeshFace* face = meshDS->AddFace(a, b, c);
          SMDS_MeshFace* face = myTool->AddFace(a, b, c);
          meshDS->SetMeshElementOnShape(face, geomFaceID);
        }
        else { // make quadrangle
          if (near + 1 > iup)
            d = uv_e1[nbright - 2].node;
          else
            d = quad->uv_grid[nbhoriz*(nbvertic - 2) + near + 1].node;
          //SMDS_MeshFace* face = meshDS->AddFace(a, b, c, d);
          if(!myTrianglePreference){
            SMDS_MeshFace* face = myTool->AddFace(a, b, c, d);
            meshDS->SetMeshElementOnShape(face, geomFaceID);
          }
          else {
            SplitQuad(meshDS, geomFaceID, a, b, c, d);
          }

          if (near + 1 < g) { // if d not is at g - make additional triangles
            for (int k = near + 1; k < g; k++) {
              c = quad->uv_grid[nbhoriz*(nbvertic - 2) + k].node;
              if (k + 1 > iup)
                d = uv_e1[nbright - 2].node;
              else
                d = quad->uv_grid[nbhoriz*(nbvertic - 2) + k + 1].node;
              //SMDS_MeshFace* face = meshDS->AddFace(a, c, d);
              SMDS_MeshFace* face = myTool->AddFace(a, c, d);
              meshDS->SetMeshElementOnShape(face, geomFaceID);
            }
          }
          g = near;
        }
      }
    }
  }

  // right or left boundary quadrangles
  if (quad->isEdgeOut[1]) {
//    MESSAGE("right edge is out");
    int g = 0; // last processed node in the grid
    int stop = nbright - 1;
    if (quad->isEdgeOut[2]) stop--;
    for (i = 0; i < stop; i++) {
      const SMDS_MeshNode *a, *b, *c, *d;
      a = uv_e1[i].node;
      b = uv_e1[i + 1].node;
      gp_Pnt pb (b->X(), b->Y(), b->Z());

      // find node c in the grid, nearest to the b
      int near = g;
      if (i == stop - 1) { // up bondary reached
        c = quad->uv_grid[nbhoriz*(jup + 1) - 2].node;
        near = jup;
      } else {
        double mind = RealLast();
        for (int k = g; k <= jup; k++) {
          const SMDS_MeshNode *nk;
          if (k < jlow)
            nk = uv_e0[nbdown - 2].node;
          else
            nk = quad->uv_grid[nbhoriz*(k + 1) - 2].node;
          gp_Pnt pnk (nk->X(), nk->Y(), nk->Z());
          double dist = pb.Distance(pnk);
          if (dist < mind - eps) {
            c = nk;
            near = k;
            mind = dist;
          } else {
            break;
          }
        }
      }

      if (near == g) { // make triangle
        //SMDS_MeshFace* face = meshDS->AddFace(a, b, c);
        SMDS_MeshFace* face = myTool->AddFace(a, b, c);
        meshDS->SetMeshElementOnShape(face, geomFaceID);
      }
      else { // make quadrangle
        if (near - 1 < jlow)
          d = uv_e0[nbdown - 2].node;
        else
          d = quad->uv_grid[nbhoriz*near - 2].node;
        //SMDS_MeshFace* face = meshDS->AddFace(a, b, c, d);

        if(!myTrianglePreference){
          SMDS_MeshFace* face = myTool->AddFace(a, b, c, d);
          meshDS->SetMeshElementOnShape(face, geomFaceID);
        }
        else {
          SplitQuad(meshDS, geomFaceID, a, b, c, d);
        }

        if (near - 1 > g) { // if d not is at g - make additional triangles
          for (int k = near - 1; k > g; k--) {
            c = quad->uv_grid[nbhoriz*(k + 1) - 2].node;
            if (k - 1 < jlow)
              d = uv_e0[nbdown - 2].node;
            else
              d = quad->uv_grid[nbhoriz*k - 2].node;
            //SMDS_MeshFace* face = meshDS->AddFace(a, c, d);
            SMDS_MeshFace* face = myTool->AddFace(a, c, d);
            meshDS->SetMeshElementOnShape(face, geomFaceID);
          }
        }
        g = near;
      }
    }
  } else {
    if (quad->isEdgeOut[3]) {
//      MESSAGE("left edge is out");
      int g = nbvertic - 1; // last processed node in the grid
      int stop = 0;
      if (quad->isEdgeOut[0]) stop++;
      for (i = nbleft - 1; i > stop; i--) {
        const SMDS_MeshNode *a, *b, *c, *d;
        a = uv_e3[i].node;
        b = uv_e3[i - 1].node;
        gp_Pnt pb (b->X(), b->Y(), b->Z());

        // find node c in the grid, nearest to the b
        int near = g;
        if (i == stop + 1) { // down bondary reached
          c = quad->uv_grid[nbhoriz*jlow + 1].node;
          near = jlow;
        } else {
          double mind = RealLast();
          for (int k = g; k >= jlow; k--) {
            const SMDS_MeshNode *nk;
            if (k > jup)
              nk = uv_e2[1].node;
            else
              nk = quad->uv_grid[nbhoriz*k + 1].node;
            gp_Pnt pnk (nk->X(), nk->Y(), nk->Z());
            double dist = pb.Distance(pnk);
            if (dist < mind - eps) {
              c = nk;
              near = k;
              mind = dist;
            } else {
              break;
            }
          }
        }

        if (near == g) { // make triangle
          //SMDS_MeshFace* face = meshDS->AddFace(a, b, c);
          SMDS_MeshFace* face = myTool->AddFace(a, b, c);
          meshDS->SetMeshElementOnShape(face, geomFaceID);
        }
        else { // make quadrangle
          if (near + 1 > jup)
            d = uv_e2[1].node;
          else
            d = quad->uv_grid[nbhoriz*(near + 1) + 1].node;
          //SMDS_MeshFace* face = meshDS->AddFace(a, b, c, d);
          if(!myTrianglePreference){
            SMDS_MeshFace* face = myTool->AddFace(a, b, c, d);
            meshDS->SetMeshElementOnShape(face, geomFaceID);
          }
          else {
            SplitQuad(meshDS, geomFaceID, a, b, c, d);
          }

          if (near + 1 < g) { // if d not is at g - make additional triangles
            for (int k = near + 1; k < g; k++) {
              c = quad->uv_grid[nbhoriz*k + 1].node;
              if (k + 1 > jup)
                d = uv_e2[1].node;
              else
                d = quad->uv_grid[nbhoriz*(k + 1) + 1].node;
              //SMDS_MeshFace* face = meshDS->AddFace(a, c, d);
              SMDS_MeshFace* face = myTool->AddFace(a, c, d);
              meshDS->SetMeshElementOnShape(face, geomFaceID);
            }
          }
          g = near;
        }
      }
    }
  }

  bool isOk = true;
  return isOk;
}

//================================================================================
/*!
 * \brief Return true if only two given edges meat at their common vertex
 */
//================================================================================

static bool twoEdgesMeatAtVertex(const TopoDS_Edge& e1,
                                 const TopoDS_Edge& e2,
                                 SMESH_Mesh &       mesh)
{
  TopoDS_Vertex v;
  if ( !TopExp::CommonVertex( e1, e2, v ))
    return false;
  TopTools_ListIteratorOfListOfShape ancestIt( mesh.GetAncestors( v ));
  for ( ; ancestIt.More() ; ancestIt.Next() )
    if ( ancestIt.Value().ShapeType() == TopAbs_EDGE )
      if ( !e1.IsSame( ancestIt.Value() ) && !e2.IsSame( ancestIt.Value() ))
        return false;
  return true;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

FaceQuadStruct* StdMeshers_Quadrangle_2D::CheckNbEdges(SMESH_Mesh &         aMesh,
                                                       const TopoDS_Shape & aShape)
  //throw(SMESH_Exception)
{
  const TopoDS_Face & F = TopoDS::Face(aShape);
  const bool ignoreMediumNodes = _quadraticMesh;

  // verify 1 wire only, with 4 edges
  TopoDS_Vertex V;
  list< TopoDS_Edge > edges;
  list< int > nbEdgesInWire;
  int nbWire = SMESH_Block::GetOrderedEdges (F, V, edges, nbEdgesInWire);
  if (nbWire != 1) {
    error(COMPERR_BAD_SHAPE, TComm("Wrong number of wires: ") << nbWire);
    return 0;
  }
  FaceQuadStruct* quad = new FaceQuadStruct;
  quad->uv_grid = 0;
  quad->side.reserve(nbEdgesInWire.front());

  int nbSides = 0;
  list< TopoDS_Edge >::iterator edgeIt = edges.begin();
  if ( nbEdgesInWire.front() == 4 ) { // exactly 4 edges
    for ( ; edgeIt != edges.end(); ++edgeIt, nbSides++ )
      quad->side.push_back( new StdMeshers_FaceSide(F, *edgeIt, &aMesh,
                                                    nbSides<TOP_SIDE, ignoreMediumNodes));
  }
  else if ( nbEdgesInWire.front() > 4 ) { // more than 4 edges - try to unite some
    list< TopoDS_Edge > sideEdges;
    while ( !edges.empty()) {
      sideEdges.clear();
      sideEdges.splice( sideEdges.end(), edges, edges.begin()); // edges.front() -> sideEdges.end()
      bool sameSide = true;
      while ( !edges.empty() && sameSide ) {
        sameSide = SMESH_Algo::IsContinuous( sideEdges.back(), edges.front() );
        if ( sameSide )
          sideEdges.splice( sideEdges.end(), edges, edges.begin());
      }
      if ( nbSides == 0 ) { // go backward from the first edge
        sameSide = true;
        while ( !edges.empty() && sameSide ) {
          sameSide = SMESH_Algo::IsContinuous( sideEdges.front(), edges.back() );
          if ( sameSide )
            sideEdges.splice( sideEdges.begin(), edges, --edges.end());
        }
      }
      quad->side.push_back( new StdMeshers_FaceSide(F, sideEdges, &aMesh,
                                                    nbSides<TOP_SIDE, ignoreMediumNodes));
      ++nbSides;
    }
    // issue 20222. Try to unite only edges shared by two same faces
    if (nbSides < 4) {
      // delete found sides
      { FaceQuadStruct cleaner( *quad ); }
      quad->side.clear();
      quad->side.reserve(nbEdgesInWire.front());
      nbSides = 0;

      SMESH_Block::GetOrderedEdges (F, V, edges, nbEdgesInWire);
      while ( !edges.empty()) {
        sideEdges.clear();
        sideEdges.splice( sideEdges.end(), edges, edges.begin());
        bool sameSide = true;
        while ( !edges.empty() && sameSide ) {
          sameSide =
            SMESH_Algo::IsContinuous( sideEdges.back(), edges.front() ) &&
            twoEdgesMeatAtVertex( sideEdges.back(), edges.front(), aMesh );
          if ( sameSide )
            sideEdges.splice( sideEdges.end(), edges, edges.begin());
        }
        if ( nbSides == 0 ) { // go backward from the first edge
          sameSide = true;
          while ( !edges.empty() && sameSide ) {
            sameSide =
              SMESH_Algo::IsContinuous( sideEdges.front(), edges.back() ) &&
              twoEdgesMeatAtVertex( sideEdges.front(), edges.back(), aMesh );
            if ( sameSide )
              sideEdges.splice( sideEdges.begin(), edges, --edges.end());
          }
        }
        quad->side.push_back( new StdMeshers_FaceSide(F, sideEdges, &aMesh,
                                                      nbSides<TOP_SIDE, ignoreMediumNodes));
        ++nbSides;
      }
    }
  }
  if (nbSides != 4) {
#ifdef _DEBUG_
    MESSAGE ( "StdMeshers_Quadrangle_2D. Edge IDs of " << nbSides << " sides:\n" );
    for ( int i = 0; i < nbSides; ++i ) {
      MESSAGE ( " ( " );
      for ( int e = 0; e < quad->side[i]->NbEdges(); ++e )
        MESSAGE ( myTool->GetMeshDS()->ShapeToIndex( quad->side[i]->Edge( e )) << " " );
      MESSAGE ( ")\n" );
    }
    //cout << endl;
#endif
    if ( !nbSides )
      nbSides = nbEdgesInWire.front();
    error(COMPERR_BAD_SHAPE, TComm("Face must have 4 sides but not ") << nbSides);
    delete quad;
    quad = 0;
  }

  return quad;
}

//=============================================================================
/*!
 *  CheckAnd2Dcompute
 */
//=============================================================================

FaceQuadStruct *StdMeshers_Quadrangle_2D::CheckAnd2Dcompute
                           (SMESH_Mesh &         aMesh,
                            const TopoDS_Shape & aShape,
                            const bool           CreateQuadratic) //throw(SMESH_Exception)
{
  _quadraticMesh = CreateQuadratic;

  FaceQuadStruct *quad = CheckNbEdges(aMesh, aShape);

  if(!quad) return 0;

  // set normalized grid on unit square in parametric domain
  bool stat = SetNormalizedGrid(aMesh, aShape, quad);
  if(!stat) {
    if(!quad)
      delete quad;
    quad = 0;
  }

  return quad;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

faceQuadStruct::~faceQuadStruct()
{
  for (int i = 0; i < side.size(); i++) {
    if (side[i])     delete side[i];
  }
  if (uv_grid)       delete [] uv_grid;
}

namespace {
  inline const vector<UVPtStruct>& GetUVPtStructIn(FaceQuadStruct* quad, int i, int nbSeg)
  {
    bool   isXConst   = ( i == BOTTOM_SIDE || i == TOP_SIDE );
    double constValue = ( i == BOTTOM_SIDE || i == LEFT_SIDE ) ? 0 : 1;
    return
      quad->isEdgeOut[i] ?
      quad->side[i]->SimulateUVPtStruct(nbSeg,isXConst,constValue) :
      quad->side[i]->GetUVPtStruct(isXConst,constValue);
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Quadrangle_2D::SetNormalizedGrid (SMESH_Mesh & aMesh,
                                                  const TopoDS_Shape& aShape,
                                                  FaceQuadStruct* & quad) //throw (SMESH_Exception)
{
  // Algorithme décrit dans "Génération automatique de maillages"
  // P.L. GEORGE, MASSON, § 6.4.1 p. 84-85
  // traitement dans le domaine paramétrique 2d u,v
  // transport - projection sur le carré unité

//  MESSAGE("StdMeshers_Quadrangle_2D::SetNormalizedGrid");
//  const TopoDS_Face& F = TopoDS::Face(aShape);

  // 1 --- find orientation of the 4 edges, by test on extrema

  //      max             min                    0     x1     1
  //     |<----north-2-------^                a3 -------------> a2
  //     |                   |                   ^1          1^
  //    west-3            east-1 =right          |            |
  //     |                   |         ==>       |            |
  //  y0 |                   | y1                |            |
  //     |                   |                   |0          0|
  //     v----south-0-------->                a0 -------------> a1
  //      min             max                    0     x0     1
  //             =down
  //

  // 3 --- 2D normalized values on unit square [0..1][0..1]

  int nbhoriz  = Min(quad->side[0]->NbPoints(), quad->side[2]->NbPoints());
  int nbvertic = Min(quad->side[1]->NbPoints(), quad->side[3]->NbPoints());

  quad->isEdgeOut[0] = (quad->side[0]->NbPoints() > quad->side[2]->NbPoints());
  quad->isEdgeOut[1] = (quad->side[1]->NbPoints() > quad->side[3]->NbPoints());
  quad->isEdgeOut[2] = (quad->side[2]->NbPoints() > quad->side[0]->NbPoints());
  quad->isEdgeOut[3] = (quad->side[3]->NbPoints() > quad->side[1]->NbPoints());

  UVPtStruct *uv_grid = quad->uv_grid = new UVPtStruct[nbvertic * nbhoriz];

  const vector<UVPtStruct>& uv_e0 = GetUVPtStructIn( quad, 0, nbhoriz - 1 );
  const vector<UVPtStruct>& uv_e1 = GetUVPtStructIn( quad, 1, nbvertic - 1 );
  const vector<UVPtStruct>& uv_e2 = GetUVPtStructIn( quad, 2, nbhoriz - 1 );
  const vector<UVPtStruct>& uv_e3 = GetUVPtStructIn( quad, 3, nbvertic - 1 );

  if ( uv_e0.empty() || uv_e1.empty() || uv_e2.empty() || uv_e3.empty() )
    //return error( "Can't find nodes on sides");
    return error( COMPERR_BAD_INPUT_MESH );

  // nodes Id on "in" edges
  if (! quad->isEdgeOut[0]) {
    int j = 0;
    for (int i = 0; i < nbhoriz; i++) { // down
      int ij = j * nbhoriz + i;
      uv_grid[ij].node = uv_e0[i].node;
    }
  }
  if (! quad->isEdgeOut[1]) {
    int i = nbhoriz - 1;
    for (int j = 0; j < nbvertic; j++) { // right
      int ij = j * nbhoriz + i;
      uv_grid[ij].node = uv_e1[j].node;
    }
  }
  if (! quad->isEdgeOut[2]) {
    int j = nbvertic - 1;
    for (int i = 0; i < nbhoriz; i++) { // up
      int ij = j * nbhoriz + i;
      uv_grid[ij].node = uv_e2[i].node;
    }
  }
  if (! quad->isEdgeOut[3]) {
    int i = 0;
    for (int j = 0; j < nbvertic; j++) { // left
      int ij = j * nbhoriz + i;
      uv_grid[ij].node = uv_e3[j].node;
    }
  }

  // normalized 2d values on grid
  for (int i = 0; i < nbhoriz; i++)
  {
    for (int j = 0; j < nbvertic; j++)
    {
      int ij = j * nbhoriz + i;
      // --- droite i cste : x = x0 + y(x1-x0)
      double x0 = uv_e0[i].normParam;	// bas - sud
      double x1 = uv_e2[i].normParam;	// haut - nord
      // --- droite j cste : y = y0 + x(y1-y0)
      double y0 = uv_e3[j].normParam;	// gauche-ouest
      double y1 = uv_e1[j].normParam;	// droite - est
      // --- intersection : x=x0+(y0+x(y1-y0))(x1-x0)
      double x = (x0 + y0 * (x1 - x0)) / (1 - (y1 - y0) * (x1 - x0));
      double y = y0 + x * (y1 - y0);
      uv_grid[ij].x = x;
      uv_grid[ij].y = y;
      //MESSAGE("-xy-01 "<<x0<<" "<<x1<<" "<<y0<<" "<<y1);
      //MESSAGE("-xy-norm "<<i<<" "<<j<<" "<<x<<" "<<y);
    }
  }

  // 4 --- projection on 2d domain (u,v)
  gp_UV a0( uv_e0.front().u, uv_e0.front().v );
  gp_UV a1( uv_e0.back().u,  uv_e0.back().v );
  gp_UV a2( uv_e2.back().u,  uv_e2.back().v );
  gp_UV a3( uv_e2.front().u, uv_e2.front().v );

  for (int i = 0; i < nbhoriz; i++)
  {
    for (int j = 0; j < nbvertic; j++)
    {
      int ij = j * nbhoriz + i;
      double x = uv_grid[ij].x;
      double y = uv_grid[ij].y;
      double param_0 = uv_e0[0].normParam + x * (uv_e0.back().normParam - uv_e0[0].normParam); // sud
      double param_2 = uv_e2[0].normParam + x * (uv_e2.back().normParam - uv_e2[0].normParam); // nord
      double param_1 = uv_e1[0].normParam + y * (uv_e1.back().normParam - uv_e1[0].normParam); // est
      double param_3 = uv_e3[0].normParam + y * (uv_e3.back().normParam - uv_e3[0].normParam); // ouest

      //MESSAGE("params "<<param_0<<" "<<param_1<<" "<<param_2<<" "<<param_3);
      gp_UV p0 = quad->side[0]->Value2d(param_0).XY();
      gp_UV p1 = quad->side[1]->Value2d(param_1).XY();
      gp_UV p2 = quad->side[2]->Value2d(param_2).XY();
      gp_UV p3 = quad->side[3]->Value2d(param_3).XY();

      gp_UV uv = (1 - y) * p0 + x * p1 + y * p2 + (1 - x) * p3;
      uv -= (1 - x) * (1 - y) * a0 + x * (1 - y) * a1 + x * y * a2 + (1 - x) * y * a3;

      uv_grid[ij].u = uv.X();
      uv_grid[ij].v = uv.Y();
    }
  }
  return true;
}

//=======================================================================
//function : ShiftQuad
//purpose  : auxilary function for ComputeQuadPref
//=======================================================================

static void ShiftQuad(FaceQuadStruct* quad, const int num, bool)
{
  StdMeshers_FaceSide* side[4] = { quad->side[0], quad->side[1], quad->side[2], quad->side[3] };
  for (int i = BOTTOM_SIDE; i < NB_SIDES; ++i ) {
    int id = ( i + num ) % NB_SIDES;
    bool wasForward = ( i < TOP_SIDE );
    bool newForward = ( id < TOP_SIDE );
    if ( wasForward != newForward )
      side[ i ]->Reverse();
    quad->side[ id ] = side[ i ];
  }
}

//=======================================================================
//function : CalcUV
//purpose  : auxilary function for ComputeQuadPref
//=======================================================================

static gp_UV CalcUV(double x0, double x1, double y0, double y1,
                    FaceQuadStruct* quad,
                    const gp_UV& a0, const gp_UV& a1,
                    const gp_UV& a2, const gp_UV& a3)
{
  const vector<UVPtStruct>& uv_eb = quad->side[0]->GetUVPtStruct(true,0 );
  const vector<UVPtStruct>& uv_er = quad->side[1]->GetUVPtStruct(false,1);
  const vector<UVPtStruct>& uv_et = quad->side[2]->GetUVPtStruct(true,1 );
  const vector<UVPtStruct>& uv_el = quad->side[3]->GetUVPtStruct(false,0);

  double x = (x0 + y0 * (x1 - x0)) / (1 - (y1 - y0) * (x1 - x0));
  double y = y0 + x * (y1 - y0);

  double param_b = uv_eb[0].normParam + x * (uv_eb.back().normParam - uv_eb[0].normParam);
  double param_t = uv_et[0].normParam + x * (uv_et.back().normParam - uv_et[0].normParam);
  double param_r = uv_er[0].normParam + y * (uv_er.back().normParam - uv_er[0].normParam);
  double param_l = uv_el[0].normParam + y * (uv_el.back().normParam - uv_el[0].normParam);

  gp_UV p0 = quad->side[BOTTOM_SIDE]->Value2d(param_b).XY();
  gp_UV p1 = quad->side[RIGHT_SIDE ]->Value2d(param_r).XY();
  gp_UV p2 = quad->side[TOP_SIDE   ]->Value2d(param_t).XY();
  gp_UV p3 = quad->side[LEFT_SIDE  ]->Value2d(param_l).XY();

  gp_UV uv = p0 * (1 - y) + p1 * x + p2 * y + p3 * (1 - x);

  uv -= (1 - x) * (1 - y) * a0 + x * (1 - y) * a1 + x * y * a2 + (1 - x) * y * a3;

  return uv;
}


//=======================================================================
//function : CalcUV2
//purpose  : auxilary function for ComputeQuadPref
//=======================================================================

static gp_UV CalcUV2(double x, double y,
                     FaceQuadStruct* quad,
                     const gp_UV& a0, const gp_UV& a1,
                     const gp_UV& a2, const gp_UV& a3)
{
  const vector<UVPtStruct>& uv_eb = quad->side[0]->GetUVPtStruct(true,0 );
  const vector<UVPtStruct>& uv_er = quad->side[1]->GetUVPtStruct(false,1);
  const vector<UVPtStruct>& uv_et = quad->side[2]->GetUVPtStruct(true,1 );
  const vector<UVPtStruct>& uv_el = quad->side[3]->GetUVPtStruct(false,0);

  //double x = (x0 + y0 * (x1 - x0)) / (1 - (y1 - y0) * (x1 - x0));
  //double y = y0 + x * (y1 - y0);

  double param_b = uv_eb[0].normParam + x * (uv_eb.back().normParam - uv_eb[0].normParam);
  double param_t = uv_et[0].normParam + x * (uv_et.back().normParam - uv_et[0].normParam);
  double param_r = uv_er[0].normParam + y * (uv_er.back().normParam - uv_er[0].normParam);
  double param_l = uv_el[0].normParam + y * (uv_el.back().normParam - uv_el[0].normParam);

  gp_UV p0 = quad->side[BOTTOM_SIDE]->Value2d(param_b).XY();
  gp_UV p1 = quad->side[RIGHT_SIDE ]->Value2d(param_r).XY();
  gp_UV p2 = quad->side[TOP_SIDE   ]->Value2d(param_t).XY();
  gp_UV p3 = quad->side[LEFT_SIDE  ]->Value2d(param_l).XY();

  gp_UV uv = p0 * (1 - y) + p1 * x + p2 * y + p3 * (1 - x);

  uv -= (1 - x) * (1 - y) * a0 + x * (1 - y) * a1 + x * y * a2 + (1 - x) * y * a3;

  return uv;
}

//=======================================================================
/*!
 * Create only quandrangle faces
 */
//=======================================================================

bool StdMeshers_Quadrangle_2D::ComputeQuadPref (SMESH_Mesh &        aMesh,
                                                const TopoDS_Shape& aShape,
                                                FaceQuadStruct*     quad)
{
  // Auxilary key in order to keep old variant
  // of meshing after implementation new variant
  // for bug 0016220 from Mantis.
  bool OldVersion = false;

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  const TopoDS_Face& F = TopoDS::Face(aShape);
  Handle(Geom_Surface) S = BRep_Tool::Surface(F);
//  const TopoDS_Wire& W = BRepTools::OuterWire(F);
  bool WisF = true;
//   if(W.Orientation()==TopAbs_FORWARD) 
//     WisF = true;
  //if(WisF) cout<<"W is FORWARD"<<endl;
  //else cout<<"W is REVERSED"<<endl;
//   bool FisF = (F.Orientation()==TopAbs_FORWARD);
//   if(!FisF) WisF = !WisF;
//  WisF = FisF;
  int i,j,geomFaceID = meshDS->ShapeToIndex( F );

  int nb = quad->side[0]->NbPoints();
  int nr = quad->side[1]->NbPoints();
  int nt = quad->side[2]->NbPoints();
  int nl = quad->side[3]->NbPoints();
  int dh = abs(nb-nt);
  int dv = abs(nr-nl);

  if( dh>=dv ) {
    if( nt>nb ) {
      // it is a base case => not shift quad but me be replacement is need
      ShiftQuad(quad,0,WisF);
    }
    else {
      // we have to shift quad on 2
      ShiftQuad(quad,2,WisF);
    }
  }
  else {
    if( nr>nl ) {
      // we have to shift quad on 1
      ShiftQuad(quad,1,WisF);
    }
    else {
      // we have to shift quad on 3
      ShiftQuad(quad,3,WisF);
    }
  }

  nb = quad->side[0]->NbPoints();
  nr = quad->side[1]->NbPoints();
  nt = quad->side[2]->NbPoints();
  nl = quad->side[3]->NbPoints();
  dh = abs(nb-nt);
  dv = abs(nr-nl);
  int nbh  = Max(nb,nt);
  int nbv = Max(nr,nl);
  int addh = 0;
  int addv = 0;

  // ----------- Old version ---------------
  // orientation of face and 3 main domain for future faces
  //       0   top    1
  //      1------------1
  //       |   |  |   |
  //       |   |  |   |
  //       | L |  | R |
  //  left |   |  |   | rigth
  //       |  /    \  |
  //       | /  C   \ |
  //       |/        \|
  //      0------------0
  //       0  bottom  1

  // ----------- New version ---------------
  // orientation of face and 3 main domain for future faces
  //       0   top    1
  //      1------------1
  //       |  |____|  |
  //       |L /    \ R|
  //       | /  C   \ |
  //  left |/________\| rigth
  //       |          |
  //       |          |
  //       |          |
  //      0------------0
  //       0  bottom  1

  if(dh>dv) {
    addv = (dh-dv)/2;
    nbv = nbv + addv;
  }
  else { // dv>=dh
    addh = (dv-dh)/2;
    nbh = nbh + addh;
  }

  const vector<UVPtStruct>& uv_eb = quad->side[0]->GetUVPtStruct(true,0 );
  const vector<UVPtStruct>& uv_er = quad->side[1]->GetUVPtStruct(false,1);
  const vector<UVPtStruct>& uv_et = quad->side[2]->GetUVPtStruct(true,1 );
  const vector<UVPtStruct>& uv_el = quad->side[3]->GetUVPtStruct(false,0);

  // arrays for normalized params
  //cout<<"Dump B:"<<endl;
  TColStd_SequenceOfReal npb, npr, npt, npl;
  for(i=0; i<nb; i++) {
    npb.Append(uv_eb[i].normParam);
    //cout<<"i="<<i<<" par="<<uv_eb[i].normParam<<" npar="<<uv_eb[i].normParam;
    //const SMDS_MeshNode* N = uv_eb[i].node;
    //cout<<" node("<<N->X()<<","<<N->Y()<<","<<N->Z()<<")"<<endl;
  }
  for(i=0; i<nr; i++) {
    npr.Append(uv_er[i].normParam);
  }
  for(i=0; i<nt; i++) {
    npt.Append(uv_et[i].normParam);
  }
  for(i=0; i<nl; i++) {
    npl.Append(uv_el[i].normParam);
  }

  int dl,dr;
  if(OldVersion) {
    // add some params to right and left after the first param
    // insert to right
    dr = nbv - nr;
    double dpr = (npr.Value(2) - npr.Value(1))/(dr+1);
    for(i=1; i<=dr; i++) {
      npr.InsertAfter(1,npr.Value(2)-dpr);
    }
    // insert to left
    dl = nbv - nl;
    dpr = (npl.Value(2) - npl.Value(1))/(dl+1);
    for(i=1; i<=dl; i++) {
      npl.InsertAfter(1,npl.Value(2)-dpr);
    }
  }
  //cout<<"npb:";
  //for(i=1; i<=npb.Length(); i++) {
  //  cout<<" "<<npb.Value(i);
  //}
  //cout<<endl;
  
  gp_XY a0( uv_eb.front().u, uv_eb.front().v );
  gp_XY a1( uv_eb.back().u,  uv_eb.back().v );
  gp_XY a2( uv_et.back().u,  uv_et.back().v );
  gp_XY a3( uv_et.front().u, uv_et.front().v );
  //cout<<" a0("<<a0.X()<<","<<a0.Y()<<")"<<" a1("<<a1.X()<<","<<a1.Y()<<")"
  //    <<" a2("<<a2.X()<<","<<a2.Y()<<")"<<" a3("<<a3.X()<<","<<a3.Y()<<")"<<endl;

  int nnn = Min(nr,nl);
  // auxilary sequence of XY for creation nodes
  // in the bottom part of central domain
  // it's length must be == nbv-nnn-1
  TColgp_SequenceOfXY UVL;
  TColgp_SequenceOfXY UVR;

  if(OldVersion) {
    // step1: create faces for left domain
    StdMeshers_Array2OfNode NodesL(1,dl+1,1,nl);
    // add left nodes
    for(j=1; j<=nl; j++)
      NodesL.SetValue(1,j,uv_el[j-1].node);
    if(dl>0) {
      // add top nodes
      for(i=1; i<=dl; i++) 
        NodesL.SetValue(i+1,nl,uv_et[i].node);
      // create and add needed nodes
      TColgp_SequenceOfXY UVtmp;
      for(i=1; i<=dl; i++) {
        double x0 = npt.Value(i+1);
        double x1 = x0;
        // diagonal node
        double y0 = npl.Value(i+1);
        double y1 = npr.Value(i+1);
        gp_UV UV = CalcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode * N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesL.SetValue(i+1,1,N);
        if(UVL.Length()<nbv-nnn-1) UVL.Append(UV);
        // internal nodes
        for(j=2; j<nl; j++) {
          double y0 = npl.Value(dl+j);
          double y1 = npr.Value(dl+j);
          gp_UV UV = CalcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
          gp_Pnt P = S->Value(UV.X(),UV.Y());
          SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
          meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
          NodesL.SetValue(i+1,j,N);
          if( i==dl ) UVtmp.Append(UV);
        }
      }
      for(i=1; i<=UVtmp.Length() && UVL.Length()<nbv-nnn-1; i++) {
        UVL.Append(UVtmp.Value(i));
      }
      //cout<<"Dump NodesL:"<<endl;
      //for(i=1; i<=dl+1; i++) {
      //  cout<<"i="<<i;
      //  for(j=1; j<=nl; j++) {
      //    cout<<" ("<<NodesL.Value(i,j)->X()<<","<<NodesL.Value(i,j)->Y()<<","<<NodesL.Value(i,j)->Z()<<")";
      //  }
      //  cout<<endl;
      //}
      // create faces
      for(i=1; i<=dl; i++) {
        for(j=1; j<nl; j++) {
          if(WisF) {
            SMDS_MeshFace* F =
              myTool->AddFace(NodesL.Value(i,j), NodesL.Value(i+1,j),
                              NodesL.Value(i+1,j+1), NodesL.Value(i,j+1));
            meshDS->SetMeshElementOnShape(F, geomFaceID);
          }
          else {
            SMDS_MeshFace* F =
              myTool->AddFace(NodesL.Value(i,j), NodesL.Value(i,j+1),
                              NodesL.Value(i+1,j+1), NodesL.Value(i+1,j));
            meshDS->SetMeshElementOnShape(F, geomFaceID);
          }
        }
      }
    }
    else {
      // fill UVL using c2d
      for(i=1; i<npl.Length() && UVL.Length()<nbv-nnn-1; i++) {
        UVL.Append( gp_UV ( uv_el[i].u, uv_el[i].v ));
      }
    }
    
    // step2: create faces for right domain
    StdMeshers_Array2OfNode NodesR(1,dr+1,1,nr);
    // add right nodes
    for(j=1; j<=nr; j++) 
      NodesR.SetValue(1,j,uv_er[nr-j].node);
    if(dr>0) {
      // add top nodes
      for(i=1; i<=dr; i++) 
        NodesR.SetValue(i+1,1,uv_et[nt-1-i].node);
      // create and add needed nodes
      TColgp_SequenceOfXY UVtmp;
      for(i=1; i<=dr; i++) {
        double x0 = npt.Value(nt-i);
        double x1 = x0;
        // diagonal node
        double y0 = npl.Value(i+1);
        double y1 = npr.Value(i+1);
        gp_UV UV = CalcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode * N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesR.SetValue(i+1,nr,N);
        if(UVR.Length()<nbv-nnn-1) UVR.Append(UV);
        // internal nodes
        for(j=2; j<nr; j++) {
          double y0 = npl.Value(nbv-j+1);
          double y1 = npr.Value(nbv-j+1);
          gp_UV UV = CalcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
          gp_Pnt P = S->Value(UV.X(),UV.Y());
          SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
          meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
          NodesR.SetValue(i+1,j,N);
          if( i==dr ) UVtmp.Prepend(UV);
        }
      }
      for(i=1; i<=UVtmp.Length() && UVR.Length()<nbv-nnn-1; i++) {
        UVR.Append(UVtmp.Value(i));
      }
      // create faces
      for(i=1; i<=dr; i++) {
        for(j=1; j<nr; j++) {
          if(WisF) {
            SMDS_MeshFace* F =
              myTool->AddFace(NodesR.Value(i,j), NodesR.Value(i+1,j),
                              NodesR.Value(i+1,j+1), NodesR.Value(i,j+1));
            meshDS->SetMeshElementOnShape(F, geomFaceID);
          }
          else {
            SMDS_MeshFace* F =
              myTool->AddFace(NodesR.Value(i,j), NodesR.Value(i,j+1),
                              NodesR.Value(i+1,j+1), NodesR.Value(i+1,j));
            meshDS->SetMeshElementOnShape(F, geomFaceID);
          }
        }
      }
    }
    else {
      // fill UVR using c2d
      for(i=1; i<npr.Length() && UVR.Length()<nbv-nnn-1; i++) {
        UVR.Append( gp_UV( uv_er[i].u, uv_er[i].v ));
      }
    }
    
    // step3: create faces for central domain
    StdMeshers_Array2OfNode NodesC(1,nb,1,nbv);
    // add first string using NodesL
    for(i=1; i<=dl+1; i++)
      NodesC.SetValue(1,i,NodesL(i,1));
    for(i=2; i<=nl; i++)
      NodesC.SetValue(1,dl+i,NodesL(dl+1,i));
    // add last string using NodesR
    for(i=1; i<=dr+1; i++)
      NodesC.SetValue(nb,i,NodesR(i,nr));
    for(i=1; i<nr; i++)
      NodesC.SetValue(nb,dr+i+1,NodesR(dr+1,nr-i));
    // add top nodes (last columns)
    for(i=dl+2; i<nbh-dr; i++) 
      NodesC.SetValue(i-dl,nbv,uv_et[i-1].node);
    // add bottom nodes (first columns)
    for(i=2; i<nb; i++)
      NodesC.SetValue(i,1,uv_eb[i-1].node);
    
    // create and add needed nodes
    // add linear layers
    for(i=2; i<nb; i++) {
      double x0 = npt.Value(dl+i);
      double x1 = x0;
      for(j=1; j<nnn; j++) {
        double y0 = npl.Value(nbv-nnn+j);
        double y1 = npr.Value(nbv-nnn+j);
        gp_UV UV = CalcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesC.SetValue(i,nbv-nnn+j,N);
      }
    }
    // add diagonal layers
    //cout<<"UVL.Length()="<<UVL.Length()<<" UVR.Length()="<<UVR.Length()<<endl;
    //cout<<"Dump UVL:"<<endl;
    //for(i=1; i<=UVL.Length(); i++) {
    //  cout<<" ("<<UVL.Value(i).X()<<","<<UVL.Value(i).Y()<<")";
    //}
    //cout<<endl;
    for(i=1; i<nbv-nnn; i++) {
      double du = UVR.Value(i).X() - UVL.Value(i).X();
      double dv = UVR.Value(i).Y() - UVL.Value(i).Y();
      for(j=2; j<nb; j++) {
        double u = UVL.Value(i).X() + du*npb.Value(j);
        double v = UVL.Value(i).Y() + dv*npb.Value(j);
        gp_Pnt P = S->Value(u,v);
        SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, u, v);
        NodesC.SetValue(j,i+1,N);
      }
    }
    // create faces
    for(i=1; i<nb; i++) {
      for(j=1; j<nbv; j++) {
        if(WisF) {
          SMDS_MeshFace* F =
            myTool->AddFace(NodesC.Value(i,j), NodesC.Value(i+1,j),
                            NodesC.Value(i+1,j+1), NodesC.Value(i,j+1));
          meshDS->SetMeshElementOnShape(F, geomFaceID);
        }
        else {
          SMDS_MeshFace* F =
            myTool->AddFace(NodesC.Value(i,j), NodesC.Value(i,j+1),
                            NodesC.Value(i+1,j+1), NodesC.Value(i+1,j));
          meshDS->SetMeshElementOnShape(F, geomFaceID);
        }
      }
    }
  }

  else { // New version (!OldVersion)
    // step1: create faces for bottom rectangle domain
    StdMeshers_Array2OfNode NodesBRD(1,nb,1,nnn-1);
    // fill UVL and UVR using c2d
    for(j=0; j<nb; j++) {
      NodesBRD.SetValue(j+1,1,uv_eb[j].node);
    }
    for(i=1; i<nnn-1; i++) {
      NodesBRD.SetValue(1,i+1,uv_el[i].node);
      NodesBRD.SetValue(nb,i+1,uv_er[i].node);
      double du = uv_er[i].u - uv_el[i].u;
      double dv = uv_er[i].v - uv_el[i].v;
      for(j=2; j<nb; j++) {
        double u = uv_el[i].u + du*npb.Value(j);
        double v = uv_el[i].v + dv*npb.Value(j);
        gp_Pnt P = S->Value(u,v);
        SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, u, v);
        NodesBRD.SetValue(j,i+1,N);

      }
    }
    for(j=1; j<nnn-1; j++) {
      for(i=1; i<nb; i++) {
        if(WisF) {
          SMDS_MeshFace* F =
            myTool->AddFace(NodesBRD.Value(i,j), NodesBRD.Value(i+1,j),
                            NodesBRD.Value(i+1,j+1), NodesBRD.Value(i,j+1));
          meshDS->SetMeshElementOnShape(F, geomFaceID);
        }
        else {
          SMDS_MeshFace* F =
            myTool->AddFace(NodesBRD.Value(i,j), NodesBRD.Value(i,j+1),
                            NodesBRD.Value(i+1,j+1), NodesBRD.Value(i+1,j));
          meshDS->SetMeshElementOnShape(F, geomFaceID);
        }
      }
    }

    int drl = abs(nr-nl);
    // create faces for region C
    StdMeshers_Array2OfNode NodesC(1,nb,1,drl+1+addv);
    // add nodes from previous region
    for(j=1; j<=nb; j++) {
      NodesC.SetValue(j,1,NodesBRD.Value(j,nnn-1));
    }
    if( (drl+addv) > 0 ) {
      int n1,n2;
      if(nr>nl) {
        n1 = 1;
        n2 = drl + 1;
        TColgp_SequenceOfXY UVtmp;
        double drparam = npr.Value(nr) - npr.Value(nnn-1);
        double dlparam = npl.Value(nnn) - npl.Value(nnn-1);
        double y0,y1;
        for(i=1; i<=drl; i++) {
          // add existed nodes from right edge
          NodesC.SetValue(nb,i+1,uv_er[nnn+i-2].node);
          //double dtparam = npt.Value(i+1);
          y1 = npr.Value(nnn+i-1); // param on right edge
          double dpar = (y1 - npr.Value(nnn-1))/drparam;
          y0 = npl.Value(nnn-1) + dpar*dlparam; // param on left edge
          double dy = y1 - y0;
          for(j=1; j<nb; j++) {
            double x = npt.Value(i+1) + npb.Value(j)*(1-npt.Value(i+1));
            double y = y0 + dy*x;
            gp_UV UV = CalcUV2(x, y, quad, a0, a1, a2, a3);
            gp_Pnt P = S->Value(UV.X(),UV.Y());
            SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
            meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
            NodesC.SetValue(j,i+1,N);
          }
        }
        double dy0 = (1-y0)/(addv+1);
        double dy1 = (1-y1)/(addv+1);
        for(i=1; i<=addv; i++) {
          double yy0 = y0 + dy0*i;
          double yy1 = y1 + dy1*i;
          double dyy = yy1 - yy0;
          for(j=1; j<=nb; j++) {
            double x = npt.Value(i+1+drl) + 
              npb.Value(j) * ( npt.Value(nt-i) - npt.Value(i+1+drl) );
            double y = yy0 + dyy*x;
            gp_UV UV = CalcUV2(x, y, quad, a0, a1, a2, a3);
            gp_Pnt P = S->Value(UV.X(),UV.Y());
            SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
            meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
            NodesC.SetValue(j,i+drl+1,N);
          }
        }
      }
      else { // nr<nl
        n2 = 1;
        n1 = drl + 1;
        TColgp_SequenceOfXY UVtmp;
        double dlparam = npl.Value(nl) - npl.Value(nnn-1);
        double drparam = npr.Value(nnn) - npr.Value(nnn-1);
        double y0 = npl.Value(nnn-1);
        double y1 = npr.Value(nnn-1);
        for(i=1; i<=drl; i++) {
          // add existed nodes from right edge
          NodesC.SetValue(1,i+1,uv_el[nnn+i-2].node);
          y0 = npl.Value(nnn+i-1); // param on left edge
          double dpar = (y0 - npl.Value(nnn-1))/dlparam;
          y1 = npr.Value(nnn-1) + dpar*drparam; // param on right edge
          double dy = y1 - y0;
          for(j=2; j<=nb; j++) {
            double x = npb.Value(j)*npt.Value(nt-i);
            double y = y0 + dy*x;
            gp_UV UV = CalcUV2(x, y, quad, a0, a1, a2, a3);
            gp_Pnt P = S->Value(UV.X(),UV.Y());
            SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
            meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
            NodesC.SetValue(j,i+1,N);
          }
        }
        double dy0 = (1-y0)/(addv+1);
        double dy1 = (1-y1)/(addv+1);
        for(i=1; i<=addv; i++) {
          double yy0 = y0 + dy0*i;
          double yy1 = y1 + dy1*i;
          double dyy = yy1 - yy0;
          for(j=1; j<=nb; j++) {
            double x = npt.Value(i+1) + 
              npb.Value(j) * ( npt.Value(nt-i-drl) - npt.Value(i+1) );
            double y = yy0 + dyy*x;
            gp_UV UV = CalcUV2(x, y, quad, a0, a1, a2, a3);
            gp_Pnt P = S->Value(UV.X(),UV.Y());
            SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
            meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
            NodesC.SetValue(j,i+drl+1,N);
          }
        }
      }
      // create faces
      for(j=1; j<=drl+addv; j++) {
        for(i=1; i<nb; i++) {
          if(WisF) {
            SMDS_MeshFace* F =
              myTool->AddFace(NodesC.Value(i,j), NodesC.Value(i+1,j),
                              NodesC.Value(i+1,j+1), NodesC.Value(i,j+1));
            meshDS->SetMeshElementOnShape(F, geomFaceID);
          }
          else {
            SMDS_MeshFace* F =
              myTool->AddFace(NodesC.Value(i,j), NodesC.Value(i,j+1),
                              NodesC.Value(i+1,j+1), NodesC.Value(i+1,j));
            meshDS->SetMeshElementOnShape(F, geomFaceID);
          }
        }
      } // end nr<nl

      StdMeshers_Array2OfNode NodesLast(1,nt,1,2);
      for(i=1; i<=nt; i++) {
        NodesLast.SetValue(i,2,uv_et[i-1].node);
      }
      int nnn=0;
      for(i=n1; i<drl+addv+1; i++) {
        nnn++;
        NodesLast.SetValue(nnn,1,NodesC.Value(1,i));
      }
      for(i=1; i<=nb; i++) {
        nnn++;
        NodesLast.SetValue(nnn,1,NodesC.Value(i,drl+addv+1));
      }
      for(i=drl+addv; i>=n2; i--) {
        nnn++;
        NodesLast.SetValue(nnn,1,NodesC.Value(nb,i));
      }
      for(i=1; i<nt; i++) {
        if(WisF) {
          SMDS_MeshFace* F =
            myTool->AddFace(NodesLast.Value(i,1), NodesLast.Value(i+1,1),
                            NodesLast.Value(i+1,2), NodesLast.Value(i,2));
          meshDS->SetMeshElementOnShape(F, geomFaceID);
        }
        else {
          SMDS_MeshFace* F =
            myTool->AddFace(NodesLast.Value(i,1), NodesLast.Value(i,2),
                            NodesLast.Value(i+1,2), NodesLast.Value(i+1,2));
          meshDS->SetMeshElementOnShape(F, geomFaceID);
        }
      }
    } // if( (drl+addv) > 0 )

  } // end new version implementation

  bool isOk = true;
  return isOk;
}

//=============================================================================
/*! Split quadrangle in to 2 triangles by smallest diagonal
 *   
 */
//=============================================================================
void StdMeshers_Quadrangle_2D::SplitQuad(SMESHDS_Mesh *theMeshDS,
                                    int theFaceID,
                                    const SMDS_MeshNode* theNode1,
                                    const SMDS_MeshNode* theNode2,
                                    const SMDS_MeshNode* theNode3,
                                    const SMDS_MeshNode* theNode4)
{
  gp_Pnt a(theNode1->X(),theNode1->Y(),theNode1->Z());
  gp_Pnt b(theNode2->X(),theNode2->Y(),theNode2->Z());
  gp_Pnt c(theNode3->X(),theNode3->Y(),theNode3->Z());
  gp_Pnt d(theNode4->X(),theNode4->Y(),theNode4->Z());
  SMDS_MeshFace* face;
  Standard_Real d1 = a.Distance(c);
  Standard_Real d2 = b.Distance(d);
  Standard_Real d3 = d2 - d1;
  if(abs(d3) < gp::Resolution() || d1 > d2){
    face = myTool->AddFace(theNode2, theNode4, theNode1);
    theMeshDS->SetMeshElementOnShape(face, theFaceID );
    face = myTool->AddFace(theNode2, theNode3, theNode4);
    theMeshDS->SetMeshElementOnShape(face, theFaceID );
  }
  else
  {
    face = myTool->AddFace(theNode1, theNode2 ,theNode3);
    theMeshDS->SetMeshElementOnShape(face, theFaceID );
    face = myTool->AddFace(theNode1, theNode3, theNode4);
    theMeshDS->SetMeshElementOnShape(face, theFaceID );
  }
}
