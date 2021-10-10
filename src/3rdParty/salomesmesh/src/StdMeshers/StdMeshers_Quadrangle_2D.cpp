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

//  File   : StdMeshers_Quadrangle_2D.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH

#include "StdMeshers_Quadrangle_2D.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMDS_FacePosition.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_QuadrangleParams.hxx"
#include "StdMeshers_ViscousLayers2D.hxx"

#include <BRepBndLib.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Surface.hxx>
#include <NCollection_DefineArray2.hxx>
#include <Precision.hxx>
#include <Standard_Real.hxx>
#include <TColStd_SequenceOfInteger.hxx>
#include <TColStd_SequenceOfReal.hxx>
#include <TColgp_SequenceOfXY.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_DataMapOfShapeReal.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>

#include "utilities.h"
#include "Utils_ExceptHandlers.hxx"

#ifndef StdMeshers_Array2OfNode_HeaderFile
#define StdMeshers_Array2OfNode_HeaderFile
typedef const SMDS_MeshNode* SMDS_MeshNodePtr;
typedef NCollection_Array2<SMDS_MeshNodePtr> StdMeshers_Array2OfNode;
#endif

using namespace std;

typedef gp_XY gp_UV;
typedef SMESH_Comment TComm;

//=============================================================================
/*!
 *
 */
//=============================================================================

StdMeshers_Quadrangle_2D::StdMeshers_Quadrangle_2D (int hypId, int studyId,
                                                    SMESH_Gen* gen)
  : SMESH_2D_Algo(hypId, studyId, gen),
    myQuadranglePreference(false),
    myTrianglePreference(false),
    myTriaVertexID(-1),
    myNeedSmooth(false),
    myCheckOri(false),
    myParams( NULL ),
    myQuadType(QUAD_STANDARD),
    myHelper( NULL )
{
  MESSAGE("StdMeshers_Quadrangle_2D::StdMeshers_Quadrangle_2D");
  _name = "Quadrangle_2D";
  _shapeType = (1 << TopAbs_FACE);
  _compatibleHypothesis.push_back("QuadrangleParams");
  _compatibleHypothesis.push_back("QuadranglePreference");
  _compatibleHypothesis.push_back("TrianglePreference");
  _compatibleHypothesis.push_back("ViscousLayers2D");
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
  myTriaVertexID         = -1;
  myQuadType             = QUAD_STANDARD;
  myQuadranglePreference = false;
  myTrianglePreference   = false;
  myHelper               = (SMESH_MesherHelper*)NULL;
  myParams               = NULL;
  myQuadList.clear();

  bool isOk = true;
  aStatus   = SMESH_Hypothesis::HYP_OK;

  const list <const SMESHDS_Hypothesis * >& hyps =
    GetUsedHypothesis(aMesh, aShape, false);
  const SMESHDS_Hypothesis * aHyp = 0;

  bool isFirstParams = true;

  // First assigned hypothesis (if any) is processed now
  if (hyps.size() > 0) {
    aHyp = hyps.front();
    if (strcmp("QuadrangleParams", aHyp->GetName()) == 0)
    {
      myParams = (const StdMeshers_QuadrangleParams*)aHyp;
      myTriaVertexID = myParams->GetTriaVertex();
      myQuadType     = myParams->GetQuadType();
      if (myQuadType == QUAD_QUADRANGLE_PREF ||
          myQuadType == QUAD_QUADRANGLE_PREF_REVERSED)
        myQuadranglePreference = true;
      else if (myQuadType == QUAD_TRIANGLE_PREF)
        myTrianglePreference = true;
    }
    else if (strcmp("QuadranglePreference", aHyp->GetName()) == 0) {
      isFirstParams = false;
      myQuadranglePreference = true;
    }
    else if (strcmp("TrianglePreference", aHyp->GetName()) == 0){
      isFirstParams = false;
      myTrianglePreference = true; 
    }
    else {
      isFirstParams = false;
    }
  }

  // Second(last) assigned hypothesis (if any) is processed now
  if (hyps.size() > 1) {
    aHyp = hyps.back();
    if (isFirstParams) {
      if (strcmp("QuadranglePreference", aHyp->GetName()) == 0) {
        myQuadranglePreference = true;
        myTrianglePreference = false; 
        myQuadType = QUAD_STANDARD;
      }
      else if (strcmp("TrianglePreference", aHyp->GetName()) == 0){
        myQuadranglePreference = false;
        myTrianglePreference = true; 
        myQuadType = QUAD_STANDARD;
      }
    }
    else {
      const StdMeshers_QuadrangleParams* aHyp2 = 
        (const StdMeshers_QuadrangleParams*)aHyp;
      myTriaVertexID = aHyp2->GetTriaVertex();

      if (!myQuadranglePreference && !myTrianglePreference) { // priority of hypos
        myQuadType = aHyp2->GetQuadType();
        if (myQuadType == QUAD_QUADRANGLE_PREF ||
            myQuadType == QUAD_QUADRANGLE_PREF_REVERSED)
          myQuadranglePreference = true;
        else if (myQuadType == QUAD_TRIANGLE_PREF)
          myTrianglePreference = true;
      }
    }
  }

  error( StdMeshers_ViscousLayers2D::CheckHypothesis( aMesh, aShape, aStatus ));

  return aStatus == HYP_OK;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Quadrangle_2D::Compute (SMESH_Mesh&         aMesh,
                                        const TopoDS_Shape& aShape)
{
  const TopoDS_Face& F = TopoDS::Face(aShape);
  aMesh.GetSubMesh( F );

  // do not initialize my fields before this as StdMeshers_ViscousLayers2D
  // can call Compute() recursively
  SMESH_ProxyMesh::Ptr proxyMesh = StdMeshers_ViscousLayers2D::Compute( aMesh, F );
  if ( !proxyMesh )
    return false;

  myProxyMesh = proxyMesh;

  SMESH_MesherHelper helper (aMesh);
  myHelper = &helper;

  _quadraticMesh = myHelper->IsQuadraticSubMesh(aShape);
  myHelper->SetElementsOnShape( true );
  myNeedSmooth = false;
  myCheckOri   = false;

  FaceQuadStruct::Ptr quad = CheckNbEdges( aMesh, F, /*considerMesh=*/true );
  if (!quad)
    return false;
  myQuadList.clear();
  myQuadList.push_back( quad );

  if ( !getEnforcedUV() )
    return false;

  updateDegenUV( quad );

  int n1 = quad->side[0].NbPoints();
  int n2 = quad->side[1].NbPoints();
  int n3 = quad->side[2].NbPoints();
  int n4 = quad->side[3].NbPoints();

  enum { NOT_COMPUTED = -1, COMPUTE_FAILED = 0, COMPUTE_OK = 1 };
  int res = NOT_COMPUTED;
  if ( myQuadranglePreference )
  {
    int nfull = n1+n2+n3+n4;
    if ((nfull % 2) == 0 && ((n1 != n3) || (n2 != n4)))
    {
      // special path genarating only quandrangle faces
      res = computeQuadPref( aMesh, F, quad );
    }
  }
  else if ( myQuadType == QUAD_REDUCED )
  {
    int n13    = n1 - n3;
    int n24    = n2 - n4;
    int n13tmp = n13/2; n13tmp = n13tmp*2;
    int n24tmp = n24/2; n24tmp = n24tmp*2;
    if ((n1 == n3 && n2 != n4 && n24tmp == n24) ||
        (n2 == n4 && n1 != n3 && n13tmp == n13))
    {
      res = computeReduced( aMesh, F, quad );
    }
    else
    {
      if ( n1 != n3 && n2 != n4 )
        error( COMPERR_WARNING,
               "To use 'Reduced' transition, "
               "two opposite sides should have same number of segments, "
               "but actual number of segments is different on all sides. "
               "'Standard' transion has been used.");
      else if ( ! ( n1 == n3 && n2 == n4 ))
        error( COMPERR_WARNING,
               "To use 'Reduced' transition, "
               "two opposite sides should have an even difference in number of segments. "
               "'Standard' transion has been used.");
    }
  }

  if ( res == NOT_COMPUTED )
  {
    if ( n1 != n3 || n2 != n4 )
      res = computeTriangles( aMesh, F, quad );
    else
      res = computeQuadDominant( aMesh, F );
  }

  if ( res == COMPUTE_OK && myNeedSmooth )
    smooth( quad );

  if ( res == COMPUTE_OK )
    res = check();

  return ( res == COMPUTE_OK );
}

//================================================================================
/*!
 * \brief Compute quadrangles and triangles on the quad
 */
//================================================================================

bool StdMeshers_Quadrangle_2D::computeTriangles(SMESH_Mesh&         aMesh,
                                                const TopoDS_Face&  aFace,
                                                FaceQuadStruct::Ptr quad)
{
  int nb = quad->side[0].grid->NbPoints();
  int nr = quad->side[1].grid->NbPoints();
  int nt = quad->side[2].grid->NbPoints();
  int nl = quad->side[3].grid->NbPoints();

  // rotate the quad to have nbNodeOut sides on TOP [and LEFT]
  if ( nb > nt )
    quad->shift( nl > nr ? 3 : 2, true );
  else if ( nr > nl )
    quad->shift( 1, true );
  else if ( nl > nr )
    quad->shift( nt > nb ? 0 : 3, true );

  if ( !setNormalizedGrid( quad ))
    return false;

  if ( quad->nbNodeOut( QUAD_TOP_SIDE    ))
  {
    splitQuad( quad, 0, quad->jSize-2 );
  }
  if ( quad->nbNodeOut( QUAD_BOTTOM_SIDE )) // this should not happen
  {
    splitQuad( quad, 0, 1 );
  }
  FaceQuadStruct::Ptr newQuad = myQuadList.back();
  if ( quad != newQuad ) // split done
  {
    { // update left side limit till where to make triangles
      FaceQuadStruct::Ptr botQuad = // a bottom part
        ( quad->side[ QUAD_LEFT_SIDE ].from == 0 ) ? quad : newQuad;
      if ( botQuad->nbNodeOut( QUAD_LEFT_SIDE ) > 0 )
        botQuad->side[ QUAD_LEFT_SIDE ].to += botQuad->nbNodeOut( QUAD_LEFT_SIDE );
      else if ( botQuad->nbNodeOut( QUAD_RIGHT_SIDE ) > 0 )
        botQuad->side[ QUAD_RIGHT_SIDE ].to += botQuad->nbNodeOut( QUAD_RIGHT_SIDE );
    }
    // make quad be a greatest one
    if ( quad->side[ QUAD_LEFT_SIDE ].NbPoints() == 2 ||
         quad->side[ QUAD_RIGHT_SIDE ].NbPoints() == 2  )
      quad = newQuad;
    if ( !setNormalizedGrid( quad ))
      return false;
  }

  if ( quad->nbNodeOut( QUAD_RIGHT_SIDE ))
  {
    splitQuad( quad, quad->iSize-2, 0 );
  }
  if ( quad->nbNodeOut( QUAD_LEFT_SIDE  ))
  {
    splitQuad( quad, 1, 0 );

    if ( quad->nbNodeOut( QUAD_TOP_SIDE ))
    {
      newQuad = myQuadList.back();
      if ( newQuad == quad ) // too narrow to split
      {
        // update left side limit till where to make triangles
        quad->side[ QUAD_LEFT_SIDE ].to--;
      }
      else
      {
        FaceQuadStruct::Ptr leftQuad =
          ( quad->side[ QUAD_BOTTOM_SIDE ].from == 0 ) ? quad : newQuad;
        leftQuad->nbNodeOut( QUAD_TOP_SIDE ) = 0;
      }
    }
  }

  if ( ! computeQuadDominant( aMesh, aFace ))
    return false;

  // try to fix zero-area triangles near straight-angle corners

  return true;
}

//================================================================================
/*!
 * \brief Compute quadrangles and possibly triangles on all quads of myQuadList
 */
//================================================================================

bool StdMeshers_Quadrangle_2D::computeQuadDominant(SMESH_Mesh&         aMesh,
                                                   const TopoDS_Face&  aFace)
{
  if ( !addEnforcedNodes() )
    return false;

  std::list< FaceQuadStruct::Ptr >::iterator quad = myQuadList.begin();
  for ( ; quad != myQuadList.end(); ++quad )
    if ( !computeQuadDominant( aMesh, aFace, *quad ))
      return false;

  return true;
}

//================================================================================
/*!
 * \brief Compute quadrangles and possibly triangles
 */
//================================================================================

bool StdMeshers_Quadrangle_2D::computeQuadDominant(SMESH_Mesh&         aMesh,
                                                   const TopoDS_Face&  aFace,
                                                   FaceQuadStruct::Ptr quad)
{
  // --- set normalized grid on unit square in parametric domain

  if ( !setNormalizedGrid( quad ))
    return false;

  // --- create nodes on points, and create quadrangles

  int nbhoriz  = quad->iSize;
  int nbvertic = quad->jSize;

  // internal mesh nodes
  SMESHDS_Mesh *  meshDS = aMesh.GetMeshDS();
  Handle(Geom_Surface) S = BRep_Tool::Surface(aFace);
  int i,j,    geomFaceID = meshDS->ShapeToIndex(aFace);
  for (i = 1; i < nbhoriz - 1; i++)
    for (j = 1; j < nbvertic - 1; j++)
    {
      UVPtStruct& uvPnt = quad->UVPt( i, j );
      gp_Pnt P          = S->Value( uvPnt.u, uvPnt.v );
      uvPnt.node        = meshDS->AddNode(P.X(), P.Y(), P.Z());
      meshDS->SetNodeOnFace( uvPnt.node, geomFaceID, uvPnt.u, uvPnt.v );
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
  
  int ilow = 0;
  int iup = nbhoriz - 1;
  if (quad->nbNodeOut(3)) { ilow++; } else { if (quad->nbNodeOut(1)) iup--; }
  
  int jlow = 0;
  int jup = nbvertic - 1;
  if (quad->nbNodeOut(0)) { jlow++; } else { if (quad->nbNodeOut(2)) jup--; }
  
  // regular quadrangles
  for (i = ilow; i < iup; i++) {
    for (j = jlow; j < jup; j++) {
      const SMDS_MeshNode *a, *b, *c, *d;
      a = quad->uv_grid[ j      * nbhoriz + i    ].node;
      b = quad->uv_grid[ j      * nbhoriz + i + 1].node;
      c = quad->uv_grid[(j + 1) * nbhoriz + i + 1].node;
      d = quad->uv_grid[(j + 1) * nbhoriz + i    ].node;
      myHelper->AddFace(a, b, c, d);
    }
  }

  // Boundary elements (must always be on an outer boundary of the FACE)
  
  const vector<UVPtStruct>& uv_e0 = quad->side[0].grid->GetUVPtStruct();
  const vector<UVPtStruct>& uv_e1 = quad->side[1].grid->GetUVPtStruct();
  const vector<UVPtStruct>& uv_e2 = quad->side[2].grid->GetUVPtStruct();
  const vector<UVPtStruct>& uv_e3 = quad->side[3].grid->GetUVPtStruct();

  if (uv_e0.empty() || uv_e1.empty() || uv_e2.empty() || uv_e3.empty())
    return error(COMPERR_BAD_INPUT_MESH);

  double eps = Precision::Confusion();

  int nbdown  = (int) uv_e0.size();
  int nbup    = (int) uv_e2.size();
  int nbright = (int) uv_e1.size();
  int nbleft  = (int) uv_e3.size();

  if (quad->nbNodeOut(0) && nbvertic == 2) // this should not occure
  {
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
    //if (quad->nbNodeOut(1)) stop--;
    if ( quad->nbNodeOut( QUAD_RIGHT_SIDE ))
      quad->UVPt( nbhoriz-1, 1 ).node = uv_e1[1].node;
    if ( quad->nbNodeOut( QUAD_LEFT_SIDE ))
      quad->UVPt( 0, 1 ).node = uv_e3[1].node;

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
        myHelper->AddFace(a, b, c);
      }
      else { // make quadrangle
        if (near - 1 < ilow)
          d = uv_e3[1].node;
        else
          d = quad->uv_grid[nbhoriz + near - 1].node;
        //SMDS_MeshFace* face = meshDS->AddFace(a, b, c, d);
        
        if (!myTrianglePreference){
          myHelper->AddFace(a, b, c, d);
        }
        else {
          splitQuadFace(meshDS, geomFaceID, a, b, c, d);
        }

        // if node d is not at position g - make additional triangles
        if (near - 1 > g) {
          for (int k = near - 1; k > g; k--) {
            c = quad->uv_grid[nbhoriz + k].node;
            if (k - 1 < ilow)
              d = uv_e3[1].node;
            else
              d = quad->uv_grid[nbhoriz + k - 1].node;
            myHelper->AddFace(a, c, d);
          }
        }
        g = near;
      }
    }
  } else {
    if (quad->nbNodeOut(2) && nbvertic == 2)
    {
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

      ilow = 0;
      iup  = nbhoriz - 1;

      int stop = 0;
      if ( quad->side[3].grid->Edge(0).IsNull() ) // left side is simulated one
      {
        // quad divided at I but not at J, as nbvertic==nbright==2
        stop++; // we stop at a second node
      }
      else
      {
        if ( quad->nbNodeOut( QUAD_RIGHT_SIDE ))
          quad->UVPt( nbhoriz-1, 0 ).node = uv_e1[ nbright-2 ].node;
        if ( quad->nbNodeOut( QUAD_LEFT_SIDE ))
          quad->UVPt( 0, 0 ).node = uv_e3[ nbleft-2 ].node;

        if ( nbright > 2 ) // there was a split at J
          quad->nbNodeOut( QUAD_LEFT_SIDE ) = 0;
      }
      const SMDS_MeshNode *a, *b, *c, *d;
      i = nbup - 1;
      // avoid creating zero-area triangles near a straight-angle corner
      {
        a = uv_e2[i].node;
        b = uv_e2[i-1].node;
        c = uv_e1[nbright-2].node;
        SMESH_TNodeXYZ pa( a ), pb( b ), pc( c );
        double area = 0.5 * (( pb - pa ) ^ ( pc - pa )).Modulus();
        if ( Abs( area ) < 1e-20 )
        {
          --g;
          d = quad->UVPt( g, nbvertic-2 ).node;
          if ( myTrianglePreference )
          {
            myHelper->AddFace(a, d, c);
          }
          else
          {
            if ( SMDS_MeshFace* face = myHelper->AddFace(a, b, d, c))
            {
              SMESH_ComputeErrorPtr& err = aMesh.GetSubMesh( aFace )->GetComputeError();
              if ( !err || err->IsOK() || err->myName < COMPERR_WARNING )
              {
                err.reset( new SMESH_ComputeError( COMPERR_WARNING,
                                                   "Bad quality quad created"));
                err->myBadElements.push_back( face );
              }
            }
            --i;
          }
        }
      }
      // for each node of the up edge find nearest node
      // in the first row of the regular grid and link them
      for ( ; i > stop; i--) {
        a = uv_e2[i].node;
        b = uv_e2[i - 1].node;
        gp_Pnt pb = SMESH_TNodeXYZ( b );

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
            gp_Pnt pnk = SMESH_TNodeXYZ( nk );
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
          myHelper->AddFace(a, b, c);
        }
        else { // make quadrangle
          if (near + 1 > iup)
            d = uv_e1[nbright - 2].node;
          else
            d = quad->uv_grid[nbhoriz*(nbvertic - 2) + near + 1].node;
          //SMDS_MeshFace* face = meshDS->AddFace(a, b, c, d);
          if (!myTrianglePreference){
            myHelper->AddFace(a, b, c, d);
          }
          else {
            splitQuadFace(meshDS, geomFaceID, a, b, c, d);
          }

          if (near + 1 < g) { // if d is not at g - make additional triangles
            for (int k = near + 1; k < g; k++) {
              c = quad->uv_grid[nbhoriz*(nbvertic - 2) + k].node;
              if (k + 1 > iup)
                d = uv_e1[nbright - 2].node;
              else
                d = quad->uv_grid[nbhoriz*(nbvertic - 2) + k + 1].node;
              myHelper->AddFace(a, c, d);
            }
          }
          g = near;
        }
      }
    }
  }

  // right or left boundary quadrangles
  if (quad->nbNodeOut( QUAD_RIGHT_SIDE ) && nbhoriz == 2) // this should not occure
  {
    int g = 0; // last processed node in the grid
    int stop = nbright - 1;
    i = 0;
    if (quad->side[ QUAD_RIGHT_SIDE ].from != i    ) i++;
    if (quad->side[ QUAD_RIGHT_SIDE ].to   != stop ) stop--;
    for ( ; i < stop; i++) {
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
        myHelper->AddFace(a, b, c);
      }
      else { // make quadrangle
        if (near - 1 < jlow)
          d = uv_e0[nbdown - 2].node;
        else
          d = quad->uv_grid[nbhoriz*near - 2].node;
        //SMDS_MeshFace* face = meshDS->AddFace(a, b, c, d);

        if (!myTrianglePreference){
          myHelper->AddFace(a, b, c, d);
        }
        else {
          splitQuadFace(meshDS, geomFaceID, a, b, c, d);
        }

        if (near - 1 > g) { // if d not is at g - make additional triangles
          for (int k = near - 1; k > g; k--) {
            c = quad->uv_grid[nbhoriz*(k + 1) - 2].node;
            if (k - 1 < jlow)
              d = uv_e0[nbdown - 2].node;
            else
              d = quad->uv_grid[nbhoriz*k - 2].node;
            myHelper->AddFace(a, c, d);
          }
        }
        g = near;
      }
    }
  } else {
    if (quad->nbNodeOut(3) && nbhoriz == 2) {
//      MESSAGE("left edge is out");
      int g = nbvertic - 1; // last processed node in the grid
      int stop = 0;
      i = quad->side[ QUAD_LEFT_SIDE ].to-1; // nbleft - 1;

      const SMDS_MeshNode *a, *b, *c, *d;
      // avoid creating zero-area triangles near a straight-angle corner
      {
        a = uv_e3[i].node;
        b = uv_e3[i-1].node;
        c = quad->UVPt( 1, g ).node;
        SMESH_TNodeXYZ pa( a ), pb( b ), pc( c );
        double area = 0.5 * (( pb - pa ) ^ ( pc - pa )).Modulus();
        if ( Abs( area ) < 1e-20 )
        {
          --g;
          d = quad->UVPt( 1, g ).node;
          if ( myTrianglePreference )
          {
            myHelper->AddFace(a, d, c);
          }
          else
          {
            if ( SMDS_MeshFace* face = myHelper->AddFace(a, b, d, c))
            {
              SMESH_ComputeErrorPtr& err = aMesh.GetSubMesh( aFace )->GetComputeError();
              if ( !err || err->IsOK() || err->myName < COMPERR_WARNING )
              {
                err.reset( new SMESH_ComputeError( COMPERR_WARNING,
                                                   "Bad quality quad created"));
                err->myBadElements.push_back( face );
              }
            }
            --i;
          }
        }
      }
      for (; i > stop; i--) // loop on nodes on the left side
      {
        a = uv_e3[i].node;
        b = uv_e3[i - 1].node;
        gp_Pnt pb (b->X(), b->Y(), b->Z());

        // find node c in the grid, nearest to the b
        int near = g;
        if (i == stop + 1) { // down bondary reached
          c = quad->uv_grid[nbhoriz*jlow + 1].node;
          near = jlow;
        }
        else {
          double mind = RealLast();
          for (int k = g; k >= jlow; k--) {
            const SMDS_MeshNode *nk;
            if (k > jup)
              nk = quad->uv_grid[nbhoriz*jup + 1].node; //uv_e2[1].node;
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
          myHelper->AddFace(a, b, c);
        }
        else { // make quadrangle
          if (near + 1 > jup)
            d = quad->uv_grid[nbhoriz*jup + 1].node; //uv_e2[1].node;
          else
            d = quad->uv_grid[nbhoriz*(near + 1) + 1].node;
          if (!myTrianglePreference) {
            myHelper->AddFace(a, b, c, d);
          }
          else {
            splitQuadFace(meshDS, geomFaceID, a, b, c, d);
          }

          if (near + 1 < g) { // if d not is at g - make additional triangles
            for (int k = near + 1; k < g; k++) {
              c = quad->uv_grid[nbhoriz*k + 1].node;
              if (k + 1 > jup)
                d = quad->uv_grid[nbhoriz*jup + 1].node; //uv_e2[1].node;
              else
                d = quad->uv_grid[nbhoriz*(k + 1) + 1].node;
              myHelper->AddFace(a, c, d);
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


//=============================================================================
/*!
 *  Evaluate
 */
//=============================================================================

bool StdMeshers_Quadrangle_2D::Evaluate(SMESH_Mesh&         aMesh,
                                        const TopoDS_Shape& aFace,
                                        MapShapeNbElems&    aResMap)

{
  aMesh.GetSubMesh(aFace);

  std::vector<int> aNbNodes(4);
  bool IsQuadratic = false;
  if (!checkNbEdgesForEvaluate(aMesh, aFace, aResMap, aNbNodes, IsQuadratic)) {
    std::vector<int> aResVec(SMDSEntity_Last);
    for (int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
    SMESH_subMesh * sm = aMesh.GetSubMesh(aFace);
    aResMap.insert(std::make_pair(sm,aResVec));
    SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
    smError.reset(new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
    return false;
  }

  if (myQuadranglePreference) {
    int n1 = aNbNodes[0];
    int n2 = aNbNodes[1];
    int n3 = aNbNodes[2];
    int n4 = aNbNodes[3];
    int nfull = n1+n2+n3+n4;
    int ntmp = nfull/2;
    ntmp = ntmp*2;
    if (nfull==ntmp && ((n1!=n3) || (n2!=n4))) {
      // special path for using only quandrangle faces
      return evaluateQuadPref(aMesh, aFace, aNbNodes, aResMap, IsQuadratic);
      //return true;
    }
  }

  int nbdown  = aNbNodes[0];
  int nbup    = aNbNodes[2];

  int nbright = aNbNodes[1];
  int nbleft  = aNbNodes[3];

  int nbhoriz  = Min(nbdown, nbup);
  int nbvertic = Min(nbright, nbleft);

  int dh = Max(nbdown, nbup) - nbhoriz;
  int dv = Max(nbright, nbleft) - nbvertic;

  //int kdh = 0;
  //if (dh>0) kdh = 1;
  //int kdv = 0;
  //if (dv>0) kdv = 1;

  int nbNodes = (nbhoriz-2)*(nbvertic-2);
  //int nbFaces3 = dh + dv + kdh*(nbvertic-1)*2 + kdv*(nbhoriz-1)*2;
  int nbFaces3 = dh + dv;
  //if (kdh==1 && kdv==1) nbFaces3 -= 2;
  //if (dh>0 && dv>0) nbFaces3 -= 2;
  //int nbFaces4 = (nbhoriz-1-kdh)*(nbvertic-1-kdv);
  int nbFaces4 = (nbhoriz-1)*(nbvertic-1);

  std::vector<int> aVec(SMDSEntity_Last);
  for (int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i] = 0;
  if (IsQuadratic) {
    aVec[SMDSEntity_Quad_Triangle] = nbFaces3;
    aVec[SMDSEntity_Quad_Quadrangle] = nbFaces4;
    int nbbndedges = nbdown + nbup + nbright + nbleft -4;
    int nbintedges = (nbFaces4*4 + nbFaces3*3 - nbbndedges) / 2;
    aVec[SMDSEntity_Node] = nbNodes + nbintedges;
    if (aNbNodes.size()==5) {
      aVec[SMDSEntity_Quad_Triangle] = nbFaces3 + aNbNodes[3] -1;
      aVec[SMDSEntity_Quad_Quadrangle] = nbFaces4 - aNbNodes[3] +1;
    }
  }
  else {
    aVec[SMDSEntity_Node] = nbNodes;
    aVec[SMDSEntity_Triangle] = nbFaces3;
    aVec[SMDSEntity_Quadrangle] = nbFaces4;
    if (aNbNodes.size()==5) {
      aVec[SMDSEntity_Triangle] = nbFaces3 + aNbNodes[3] - 1;
      aVec[SMDSEntity_Quadrangle] = nbFaces4 - aNbNodes[3] + 1;
    }
  }
  SMESH_subMesh * sm = aMesh.GetSubMesh(aFace);
  aResMap.insert(std::make_pair(sm,aVec));

  return true;
}

//================================================================================
/*!
 * \brief Return true if the algorithm can mesh this shape
 *  \param [in] aShape - shape to check
 *  \param [in] toCheckAll - if true, this check returns OK if all shapes are OK,
 *              else, returns OK if at least one shape is OK
 */
//================================================================================

bool StdMeshers_Quadrangle_2D::IsApplicable( const TopoDS_Shape & aShape, bool toCheckAll )
{
  int nbFoundFaces = 0;
  for (TopExp_Explorer exp( aShape, TopAbs_FACE ); exp.More(); exp.Next(), ++nbFoundFaces )
  {
    const TopoDS_Shape& aFace = exp.Current();
    int nbWire = SMESH_MesherHelper::Count( aFace, TopAbs_WIRE, false );
    if ( nbWire != 1 ) {
      if ( toCheckAll ) return false;
      continue;
    }

    int nbNoDegenEdges = 0;
    TopExp_Explorer eExp( aFace, TopAbs_EDGE );
    for ( ; eExp.More() && nbNoDegenEdges < 3; eExp.Next() ) {
      if ( !SMESH_Algo::isDegenerated( TopoDS::Edge( eExp.Current() )))
        ++nbNoDegenEdges;
    }
    if ( toCheckAll  && nbNoDegenEdges <  3 ) return false;
    if ( !toCheckAll && nbNoDegenEdges >= 3 ) return true;
  }
  return ( toCheckAll && nbFoundFaces != 0 );
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
  if (!TopExp::CommonVertex(e1, e2, v))
    return false;
  TopTools_ListIteratorOfListOfShape ancestIt(mesh.GetAncestors(v));
  for (; ancestIt.More() ; ancestIt.Next())
    if (ancestIt.Value().ShapeType() == TopAbs_EDGE)
      if (!e1.IsSame(ancestIt.Value()) && !e2.IsSame(ancestIt.Value()))
        return false;
  return true;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

FaceQuadStruct::Ptr StdMeshers_Quadrangle_2D::CheckNbEdges(SMESH_Mesh &         aMesh,
                                                           const TopoDS_Shape & aShape,
                                                           const bool           considerMesh)
{
  if ( !myQuadList.empty() && myQuadList.front()->face.IsSame( aShape ))
    return myQuadList.front();

  TopoDS_Face F = TopoDS::Face(aShape);
  if ( F.Orientation() >= TopAbs_INTERNAL ) F.Orientation( TopAbs_FORWARD );
  const bool ignoreMediumNodes = _quadraticMesh;

  // verify 1 wire only
  list< TopoDS_Edge > edges;
  list< int > nbEdgesInWire;
  int nbWire = SMESH_Block::GetOrderedEdges (F, edges, nbEdgesInWire);
  if (nbWire != 1) {
    error(COMPERR_BAD_SHAPE, TComm("Wrong number of wires: ") << nbWire);
    return FaceQuadStruct::Ptr();
  }

  // find corner vertices of the quad
  vector<TopoDS_Vertex> corners;
  int nbDegenEdges, nbSides = getCorners( F, aMesh, edges, corners, nbDegenEdges, considerMesh );
  if ( nbSides == 0 )
  {
    return FaceQuadStruct::Ptr();
  }
  FaceQuadStruct::Ptr quad( new FaceQuadStruct );
  quad->side.reserve(nbEdgesInWire.front());
  quad->face = F;

  list< TopoDS_Edge >::iterator edgeIt = edges.begin();
  if ( nbSides == 3 ) // 3 sides and corners[0] is a vertex with myTriaVertexID
  {
    for ( int iSide = 0; iSide < 3; ++iSide )
    {
      list< TopoDS_Edge > sideEdges;
      TopoDS_Vertex nextSideV = corners[( iSide + 1 ) % 3 ];
      while ( edgeIt != edges.end() &&
              !nextSideV.IsSame( SMESH_MesherHelper::IthVertex( 0, *edgeIt )))
        if ( SMESH_Algo::isDegenerated( *edgeIt ))
          ++edgeIt;
        else
          sideEdges.push_back( *edgeIt++ );
      if ( !sideEdges.empty() )
        quad->side.push_back( StdMeshers_FaceSide::New(F, sideEdges, &aMesh, iSide < QUAD_TOP_SIDE,
                                                       ignoreMediumNodes, myProxyMesh));
      else
        --iSide;
    }
    const vector<UVPtStruct>& UVPSleft  = quad->side[0].GetUVPtStruct(true,0);
    /*  vector<UVPtStruct>& UVPStop   = */quad->side[1].GetUVPtStruct(false,1);
    /*  vector<UVPtStruct>& UVPSright = */quad->side[2].GetUVPtStruct(true,1);
    const SMDS_MeshNode* aNode = UVPSleft[0].node;
    gp_Pnt2d aPnt2d = UVPSleft[0].UV();
    quad->side.push_back( StdMeshers_FaceSide::New( quad->side[1].grid.get(), aNode, &aPnt2d ));
    myNeedSmooth = ( nbDegenEdges > 0 );
    return quad;
  }
  else // 4 sides
  {
    myNeedSmooth = ( corners.size() == 4 && nbDegenEdges > 0 );
    int iSide = 0, nbUsedDegen = 0, nbLoops = 0;
    for ( ; edgeIt != edges.end(); ++nbLoops )
    {
      list< TopoDS_Edge > sideEdges;
      TopoDS_Vertex nextSideV = corners[( iSide + 1 - nbUsedDegen ) % corners.size() ];
      bool nextSideVReached = false;
      do
      {
        const TopoDS_Edge& edge = *edgeIt;
        nextSideVReached = nextSideV.IsSame( myHelper->IthVertex( 1, edge ));
        if ( SMESH_Algo::isDegenerated( edge ))
        {
          if ( !myNeedSmooth ) // need to make a side on a degen edge
          {
            if ( sideEdges.empty() )
            {
              sideEdges.push_back( edge );
              ++nbUsedDegen;
              nextSideVReached = true;
            }
            else
            {
              break;
            }
          }
        }
        else
        {
          sideEdges.push_back( edge );
        }
        ++edgeIt;
      }
      while ( edgeIt != edges.end() && !nextSideVReached );

      if ( !sideEdges.empty() )
      {
        quad->side.push_back
          ( StdMeshers_FaceSide::New( F, sideEdges, &aMesh, iSide < QUAD_TOP_SIDE,
                                      ignoreMediumNodes, myProxyMesh ));
        ++iSide;
      }
      if ( quad->side.size() == 4 )
        break;
      if ( nbLoops > 8 )
      {
        error(TComm("Bug: infinite loop in StdMeshers_Quadrangle_2D::CheckNbEdges()"));
        quad.reset();
        break;
      }
    }
    if ( quad && quad->side.size() != 4 )
    {
      error(TComm("Bug: ") << quad->side.size()  << " sides found instead of 4");
      quad.reset();
    }
  }

  return quad;
}


//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Quadrangle_2D::checkNbEdgesForEvaluate(SMESH_Mesh&          aMesh,
                                                       const TopoDS_Shape & aShape,
                                                       MapShapeNbElems&     aResMap,
                                                       std::vector<int>&    aNbNodes,
                                                       bool&                IsQuadratic)

{
  const TopoDS_Face & F = TopoDS::Face(aShape);

  // verify 1 wire only, with 4 edges
  list< TopoDS_Edge > edges;
  list< int > nbEdgesInWire;
  int nbWire = SMESH_Block::GetOrderedEdges (F, edges, nbEdgesInWire);
  if (nbWire != 1) {
    return false;
  }

  aNbNodes.resize(4);

  int nbSides = 0;
  list< TopoDS_Edge >::iterator edgeIt = edges.begin();
  SMESH_subMesh * sm = aMesh.GetSubMesh(*edgeIt);
  MapShapeNbElemsItr anIt = aResMap.find(sm);
  if (anIt==aResMap.end()) {
    return false;
  }
  std::vector<int> aVec = (*anIt).second;
  IsQuadratic = (aVec[SMDSEntity_Quad_Edge] > aVec[SMDSEntity_Edge]);
  if (nbEdgesInWire.front() == 3) { // exactly 3 edges
    if (myTriaVertexID>0) {
      SMESHDS_Mesh* meshDS = aMesh.GetMeshDS();
      TopoDS_Vertex V = TopoDS::Vertex(meshDS->IndexToShape(myTriaVertexID));
      if (!V.IsNull()) {
        TopoDS_Edge E1,E2,E3;
        for (; edgeIt != edges.end(); ++edgeIt) {
          TopoDS_Edge E =  TopoDS::Edge(*edgeIt);
          TopoDS_Vertex VF, VL;
          TopExp::Vertices(E, VF, VL, true);
          if (VF.IsSame(V))
            E1 = E;
          else if (VL.IsSame(V))
            E3 = E;
          else
            E2 = E;
        }
        SMESH_subMesh * sm = aMesh.GetSubMesh(E1);
        MapShapeNbElemsItr anIt = aResMap.find(sm);
        if (anIt==aResMap.end()) return false;
        std::vector<int> aVec = (*anIt).second;
        if (IsQuadratic)
          aNbNodes[0] = (aVec[SMDSEntity_Node]-1)/2 + 2;
        else
          aNbNodes[0] = aVec[SMDSEntity_Node] + 2;
        sm = aMesh.GetSubMesh(E2);
        anIt = aResMap.find(sm);
        if (anIt==aResMap.end()) return false;
        aVec = (*anIt).second;
        if (IsQuadratic)
          aNbNodes[1] = (aVec[SMDSEntity_Node]-1)/2 + 2;
        else
          aNbNodes[1] = aVec[SMDSEntity_Node] + 2;
        sm = aMesh.GetSubMesh(E3);
        anIt = aResMap.find(sm);
        if (anIt==aResMap.end()) return false;
        aVec = (*anIt).second;
        if (IsQuadratic)
          aNbNodes[2] = (aVec[SMDSEntity_Node]-1)/2 + 2;
        else
          aNbNodes[2] = aVec[SMDSEntity_Node] + 2;
        aNbNodes[3] = aNbNodes[1];
        aNbNodes.resize(5);
        nbSides = 4;
      }
    }
  }
  if (nbEdgesInWire.front() == 4) { // exactly 4 edges
    for (; edgeIt != edges.end(); edgeIt++) {
      SMESH_subMesh * sm = aMesh.GetSubMesh(*edgeIt);
      MapShapeNbElemsItr anIt = aResMap.find(sm);
      if (anIt==aResMap.end()) {
        return false;
      }
      std::vector<int> aVec = (*anIt).second;
      if (IsQuadratic)
        aNbNodes[nbSides] = (aVec[SMDSEntity_Node]-1)/2 + 2;
      else
        aNbNodes[nbSides] = aVec[SMDSEntity_Node] + 2;
      nbSides++;
    }
  }
  else if (nbEdgesInWire.front() > 4) { // more than 4 edges - try to unite some
    list< TopoDS_Edge > sideEdges;
    while (!edges.empty()) {
      sideEdges.clear();
      sideEdges.splice(sideEdges.end(), edges, edges.begin()); // edges.front() -> sideEdges.end()
      bool sameSide = true;
      while (!edges.empty() && sameSide) {
        sameSide = SMESH_Algo::IsContinuous(sideEdges.back(), edges.front());
        if (sameSide)
          sideEdges.splice(sideEdges.end(), edges, edges.begin());
      }
      if (nbSides == 0) { // go backward from the first edge
        sameSide = true;
        while (!edges.empty() && sameSide) {
          sameSide = SMESH_Algo::IsContinuous(sideEdges.front(), edges.back());
          if (sameSide)
            sideEdges.splice(sideEdges.begin(), edges, --edges.end());
        }
      }
      list<TopoDS_Edge>::iterator ite = sideEdges.begin();
      aNbNodes[nbSides] = 1;
      for (; ite!=sideEdges.end(); ite++) {
        SMESH_subMesh * sm = aMesh.GetSubMesh(*ite);
        MapShapeNbElemsItr anIt = aResMap.find(sm);
        if (anIt==aResMap.end()) {
          return false;
        }
        std::vector<int> aVec = (*anIt).second;
        if (IsQuadratic)
          aNbNodes[nbSides] += (aVec[SMDSEntity_Node]-1)/2 + 1;
        else
          aNbNodes[nbSides] += aVec[SMDSEntity_Node] + 1;
      }
      ++nbSides;
    }
    // issue 20222. Try to unite only edges shared by two same faces
    if (nbSides < 4) {
      nbSides = 0;
      SMESH_Block::GetOrderedEdges (F, edges, nbEdgesInWire);
      while (!edges.empty()) {
        sideEdges.clear();
        sideEdges.splice(sideEdges.end(), edges, edges.begin());
        bool sameSide = true;
        while (!edges.empty() && sameSide) {
          sameSide =
            SMESH_Algo::IsContinuous(sideEdges.back(), edges.front()) &&
            twoEdgesMeatAtVertex(sideEdges.back(), edges.front(), aMesh);
          if (sameSide)
            sideEdges.splice(sideEdges.end(), edges, edges.begin());
        }
        if (nbSides == 0) { // go backward from the first edge
          sameSide = true;
          while (!edges.empty() && sameSide) {
            sameSide =
              SMESH_Algo::IsContinuous(sideEdges.front(), edges.back()) &&
              twoEdgesMeatAtVertex(sideEdges.front(), edges.back(), aMesh);
            if (sameSide)
              sideEdges.splice(sideEdges.begin(), edges, --edges.end());
          }
        }
        list<TopoDS_Edge>::iterator ite = sideEdges.begin();
        aNbNodes[nbSides] = 1;
        for (; ite!=sideEdges.end(); ite++) {
          SMESH_subMesh * sm = aMesh.GetSubMesh(*ite);
          MapShapeNbElemsItr anIt = aResMap.find(sm);
          if (anIt==aResMap.end()) {
            return false;
          }
          std::vector<int> aVec = (*anIt).second;
          if (IsQuadratic)
            aNbNodes[nbSides] += (aVec[SMDSEntity_Node]-1)/2 + 1;
          else
            aNbNodes[nbSides] += aVec[SMDSEntity_Node] + 1;
        }
        ++nbSides;
      }
    }
  }
  if (nbSides != 4) {
    if (!nbSides)
      nbSides = nbEdgesInWire.front();
    error(COMPERR_BAD_SHAPE, TComm("Face must have 4 sides but not ") << nbSides);
    return false;
  }

  return true;
}


//=============================================================================
/*!
 *  CheckAnd2Dcompute
 */
//=============================================================================

FaceQuadStruct::Ptr
StdMeshers_Quadrangle_2D::CheckAnd2Dcompute (SMESH_Mesh &         aMesh,
                                             const TopoDS_Shape & aShape,
                                             const bool           CreateQuadratic)
{
  _quadraticMesh = CreateQuadratic;

  FaceQuadStruct::Ptr quad = CheckNbEdges(aMesh, aShape);
  if ( quad )
  {
    // set normalized grid on unit square in parametric domain
    if ( ! setNormalizedGrid( quad ))
      quad.reset();
  }
  return quad;
}

namespace
{
  inline const vector<UVPtStruct>& getUVPtStructIn(FaceQuadStruct::Ptr& quad, int i, int nbSeg)
  {
    bool   isXConst   = (i == QUAD_BOTTOM_SIDE || i == QUAD_TOP_SIDE);
    double constValue = (i == QUAD_BOTTOM_SIDE || i == QUAD_LEFT_SIDE) ? 0 : 1;
    return
      quad->nbNodeOut(i) ?
      quad->side[i].grid->SimulateUVPtStruct(nbSeg,isXConst,constValue) :
      quad->side[i].grid->GetUVPtStruct     (isXConst,constValue);
  }
  inline gp_UV calcUV(double x, double y,
                      const gp_UV& a0,const gp_UV& a1,const gp_UV& a2,const gp_UV& a3,
                      const gp_UV& p0,const gp_UV& p1,const gp_UV& p2,const gp_UV& p3)
  {
    return
      ((1 - y) * p0 + x * p1 + y * p2 + (1 - x) * p3 ) -
      ((1 - x) * (1 - y) * a0 + x * (1 - y) * a1 + x * y * a2 + (1 - x) * y * a3);
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Quadrangle_2D::setNormalizedGrid (FaceQuadStruct::Ptr quad)
{
  if ( !quad->uv_grid.empty() )
    return true;

  // Algorithme décrit dans "Génération automatique de maillages"
  // P.L. GEORGE, MASSON, § 6.4.1 p. 84-85
  // traitement dans le domaine paramétrique 2d u,v
  // transport - projection sur le carré unité

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
  const FaceQuadStruct::Side & bSide = quad->side[0];
  const FaceQuadStruct::Side & rSide = quad->side[1];
  const FaceQuadStruct::Side & tSide = quad->side[2];
  const FaceQuadStruct::Side & lSide = quad->side[3];

  int nbhoriz  = Min( bSide.NbPoints(), tSide.NbPoints() );
  int nbvertic = Min( rSide.NbPoints(), lSide.NbPoints() );
  if ( nbhoriz < 1 || nbvertic < 1 )
    return error("Algo error: empty quad");

  if ( myQuadList.size() == 1 )
  {
    // all sub-quads must have NO sides with nbNodeOut > 0
    quad->nbNodeOut(0) = Max( 0, bSide.grid->NbPoints() - tSide.grid->NbPoints() );
    quad->nbNodeOut(1) = Max( 0, rSide.grid->NbPoints() - lSide.grid->NbPoints() );
    quad->nbNodeOut(2) = Max( 0, tSide.grid->NbPoints() - bSide.grid->NbPoints() );
    quad->nbNodeOut(3) = Max( 0, lSide.grid->NbPoints() - rSide.grid->NbPoints() );
  }
  const vector<UVPtStruct>& uv_e0 = bSide.GetUVPtStruct();
  const vector<UVPtStruct>& uv_e1 = rSide.GetUVPtStruct();
  const vector<UVPtStruct>& uv_e2 = tSide.GetUVPtStruct();
  const vector<UVPtStruct>& uv_e3 = lSide.GetUVPtStruct();
  if (uv_e0.empty() || uv_e1.empty() || uv_e2.empty() || uv_e3.empty())
    //return error("Can't find nodes on sides");
    return error(COMPERR_BAD_INPUT_MESH);

  quad->uv_grid.resize( nbvertic * nbhoriz );
  quad->iSize = nbhoriz;
  quad->jSize = nbvertic;
  UVPtStruct *uv_grid = & quad->uv_grid[0];

  quad->uv_box.Clear();

  // copy data of face boundary

  FaceQuadStruct::SideIterator sideIter;

  { // BOTTOM
    const int     j = 0;
    const double x0 = bSide.First().normParam;
    const double dx = bSide.Last().normParam - bSide.First().normParam;
    for ( sideIter.Init( bSide ); sideIter.More(); sideIter.Next() ) {
      sideIter.UVPt().x = ( sideIter.UVPt().normParam - x0 ) / dx;
      sideIter.UVPt().y = 0.;
      uv_grid[ j * nbhoriz + sideIter.Count() ] = sideIter.UVPt();
      quad->uv_box.Add( sideIter.UVPt().UV() );
    }
  }
  { // RIGHT
    const int     i = nbhoriz - 1;
    const double y0 = rSide.First().normParam;
    const double dy = rSide.Last().normParam - rSide.First().normParam;
    sideIter.Init( rSide );
    if ( quad->UVPt( i, sideIter.Count() ).node )
      sideIter.Next(); // avoid copying from a split emulated side
    for ( ; sideIter.More(); sideIter.Next() ) {
      sideIter.UVPt().x = 1.;
      sideIter.UVPt().y = ( sideIter.UVPt().normParam - y0 ) / dy;
      uv_grid[ sideIter.Count() * nbhoriz + i ] = sideIter.UVPt();
      quad->uv_box.Add( sideIter.UVPt().UV() );
    }
  }
  { // TOP
    const int     j = nbvertic - 1;
    const double x0 = tSide.First().normParam;
    const double dx = tSide.Last().normParam - tSide.First().normParam;
    int i = 0, nb = nbhoriz;
    sideIter.Init( tSide );
    if ( quad->UVPt( nb-1, j ).node ) --nb; // avoid copying from a split emulated side
    for ( ; i < nb; i++, sideIter.Next()) {
      sideIter.UVPt().x = ( sideIter.UVPt().normParam - x0 ) / dx;
      sideIter.UVPt().y = 1.;
      uv_grid[ j * nbhoriz + i ] = sideIter.UVPt();
      quad->uv_box.Add( sideIter.UVPt().UV() );
    }
  }
  { // LEFT
    const int i = 0;
    const double y0 = lSide.First().normParam;
    const double dy = lSide.Last().normParam - lSide.First().normParam;
    int j = 0, nb = nbvertic;
    sideIter.Init( lSide );
    if ( quad->UVPt( i, j    ).node )
      ++j, sideIter.Next(); // avoid copying from a split emulated side
    if ( quad->UVPt( i, nb-1 ).node )
      --nb;
    for ( ; j < nb; j++, sideIter.Next()) {
      sideIter.UVPt().x = 0.;
      sideIter.UVPt().y = ( sideIter.UVPt().normParam - y0 ) / dy;
      uv_grid[ j * nbhoriz + i ] = sideIter.UVPt();
      quad->uv_box.Add( sideIter.UVPt().UV() );
    }
  }

  // normalized 2d parameters on grid

  for (int i = 1; i < nbhoriz-1; i++)
  {
    const double x0 = quad->UVPt( i, 0          ).x;
    const double x1 = quad->UVPt( i, nbvertic-1 ).x;
    for (int j = 1; j < nbvertic-1; j++)
    {
      const double y0 = quad->UVPt( 0,         j ).y;
      const double y1 = quad->UVPt( nbhoriz-1, j ).y;
      // --- intersection : x=x0+(y0+x(y1-y0))(x1-x0)
      double x = (x0 + y0 * (x1 - x0)) / (1 - (y1 - y0) * (x1 - x0));
      double y = y0 + x * (y1 - y0);
      int   ij = j * nbhoriz + i;
      uv_grid[ij].x = x;
      uv_grid[ij].y = y;
      uv_grid[ij].node = NULL;
    }
  }

  // projection on 2d domain (u,v)

  gp_UV a0 = quad->UVPt( 0,         0          ).UV();
  gp_UV a1 = quad->UVPt( nbhoriz-1, 0          ).UV();
  gp_UV a2 = quad->UVPt( nbhoriz-1, nbvertic-1 ).UV();
  gp_UV a3 = quad->UVPt( 0,         nbvertic-1 ).UV();

  for (int i = 1; i < nbhoriz-1; i++)
  {
    gp_UV p0 = quad->UVPt( i, 0          ).UV();
    gp_UV p2 = quad->UVPt( i, nbvertic-1 ).UV();
    for (int j = 1; j < nbvertic-1; j++)
    {
      gp_UV p1 = quad->UVPt( nbhoriz-1, j ).UV();
      gp_UV p3 = quad->UVPt( 0,         j ).UV();

      int ij = j * nbhoriz + i;
      double x = uv_grid[ij].x;
      double y = uv_grid[ij].y;

      gp_UV uv = calcUV(x,y, a0,a1,a2,a3, p0,p1,p2,p3);

      uv_grid[ij].u = uv.X();
      uv_grid[ij].v = uv.Y();
    }
  }
  return true;
}

//=======================================================================
//function : ShiftQuad
//purpose  : auxilary function for computeQuadPref
//=======================================================================

void StdMeshers_Quadrangle_2D::shiftQuad(FaceQuadStruct::Ptr& quad, const int num )
{
  quad->shift( num, /*ori=*/true, /*keepGrid=*/myQuadList.size() > 1 );
}

//================================================================================
/*!
 * \brief Rotate sides of a quad CCW by given nb of quartes
 *  \param nb  - number of rotation quartes
 *  \param ori - to keep orientation of sides as in an unit quad or not
 *  \param keepGrid - if \c true Side::grid is not changed, Side::from and Side::to
 *         are altered instead
 */
//================================================================================

void FaceQuadStruct::shift( size_t nb, bool ori, bool keepGrid )
{
  if ( nb == 0 ) return;

  nb = nb % NB_QUAD_SIDES;

  vector< Side > newSides( side.size() );
  vector< Side* > sidePtrs( side.size() );
  for (int i = QUAD_BOTTOM_SIDE; i < NB_QUAD_SIDES; ++i)
  {
    int id = (i + nb) % NB_QUAD_SIDES;
    if ( ori )
    {
      bool wasForward = (i  < QUAD_TOP_SIDE);
      bool newForward = (id < QUAD_TOP_SIDE);
      if ( wasForward != newForward )
        side[ i ].Reverse( keepGrid );
    }
    newSides[ id ] = side[ i ];
    sidePtrs[ i ] = & side[ i ];
  }
  // make newSides refer newSides via Side::Contact's
  for ( size_t i = 0; i < newSides.size(); ++i )
  {
    FaceQuadStruct::Side& ns = newSides[ i ];
    for ( size_t iC = 0; iC < ns.contacts.size(); ++iC )
    {
      FaceQuadStruct::Side* oSide = ns.contacts[iC].other_side;
      vector< Side* >::iterator sIt = std::find( sidePtrs.begin(), sidePtrs.end(), oSide );
      if ( sIt != sidePtrs.end() )
        ns.contacts[iC].other_side = & newSides[ *sIt - sidePtrs[0] ];
    }
  }
  newSides.swap( side );

  if ( keepGrid && !uv_grid.empty() )
  {
    if ( nb == 2 ) // "PI"
    {
      std::reverse( uv_grid.begin(), uv_grid.end() );
    }
    else
    {
      FaceQuadStruct newQuad;
      newQuad.uv_grid.resize( uv_grid.size() );
      newQuad.iSize = jSize;
      newQuad.jSize = iSize;
      int i, j, iRev, jRev;
      int *iNew = ( nb == 1 ) ? &jRev : &j;
      int *jNew = ( nb == 1 ) ? &i : &iRev;
      for ( i = 0, iRev = iSize-1; i < iSize; ++i, --iRev )
        for ( j = 0, jRev = jSize-1; j < jSize; ++j, --jRev )
          newQuad.UVPt( *iNew, *jNew ) = UVPt( i, j );

      std::swap( iSize, jSize );
      std::swap( uv_grid, newQuad.uv_grid );
    }
  }
  else
  {
    uv_grid.clear();
  }
}

//=======================================================================
//function : calcUV
//purpose  : auxilary function for computeQuadPref
//=======================================================================

static gp_UV calcUV(double x0, double x1, double y0, double y1,
                    FaceQuadStruct::Ptr& quad,
                    const gp_UV& a0, const gp_UV& a1,
                    const gp_UV& a2, const gp_UV& a3)
{
  double x = (x0 + y0 * (x1 - x0)) / (1 - (y1 - y0) * (x1 - x0));
  double y = y0 + x * (y1 - y0);

  gp_UV p0 = quad->side[QUAD_BOTTOM_SIDE].grid->Value2d(x).XY();
  gp_UV p1 = quad->side[QUAD_RIGHT_SIDE ].grid->Value2d(y).XY();
  gp_UV p2 = quad->side[QUAD_TOP_SIDE   ].grid->Value2d(x).XY();
  gp_UV p3 = quad->side[QUAD_LEFT_SIDE  ].grid->Value2d(y).XY();

  gp_UV uv = calcUV(x,y, a0,a1,a2,a3, p0,p1,p2,p3);

  return uv;
}

//=======================================================================
//function : calcUV2
//purpose  : auxilary function for computeQuadPref
//=======================================================================

static gp_UV calcUV2(double x, double y,
                     FaceQuadStruct::Ptr& quad,
                     const gp_UV& a0, const gp_UV& a1,
                     const gp_UV& a2, const gp_UV& a3)
{
  gp_UV p0 = quad->side[QUAD_BOTTOM_SIDE].grid->Value2d(x).XY();
  gp_UV p1 = quad->side[QUAD_RIGHT_SIDE ].grid->Value2d(y).XY();
  gp_UV p2 = quad->side[QUAD_TOP_SIDE   ].grid->Value2d(x).XY();
  gp_UV p3 = quad->side[QUAD_LEFT_SIDE  ].grid->Value2d(y).XY();

  gp_UV uv = calcUV(x,y, a0,a1,a2,a3, p0,p1,p2,p3);

  return uv;
}


//=======================================================================
/*!
 * Create only quandrangle faces
 */
//=======================================================================

bool StdMeshers_Quadrangle_2D::computeQuadPref (SMESH_Mesh &        aMesh,
                                                const TopoDS_Face&  aFace,
                                                FaceQuadStruct::Ptr quad)
{
  const bool OldVersion = (myQuadType == QUAD_QUADRANGLE_PREF_REVERSED);
  const bool WisF = true;

  SMESHDS_Mesh *  meshDS = aMesh.GetMeshDS();
  Handle(Geom_Surface) S = BRep_Tool::Surface(aFace);
  int i,j,    geomFaceID = meshDS->ShapeToIndex(aFace);

  int nb = quad->side[0].NbPoints();
  int nr = quad->side[1].NbPoints();
  int nt = quad->side[2].NbPoints();
  int nl = quad->side[3].NbPoints();
  int dh = abs(nb-nt);
  int dv = abs(nr-nl);

  if ( myForcedPnts.empty() )
  {
    // rotate sides to be as in the picture below and to have
    // dh >= dv and nt > nb
    if ( dh >= dv )
      shiftQuad( quad, ( nt > nb ) ? 0 : 2 );
    else
      shiftQuad( quad, ( nr > nl ) ? 1 : 3 );
  }
  else
  {
    // rotate the quad to have nt > nb [and nr > nl]
    if ( nb > nt )
      shiftQuad ( quad, nr > nl ? 1 : 2 );
    else if ( nr > nl )
      shiftQuad( quad, nb == nt ? 1 : 0 );
    else if ( nl > nr )
      shiftQuad( quad, 3 );
  }

  nb = quad->side[0].NbPoints();
  nr = quad->side[1].NbPoints();
  nt = quad->side[2].NbPoints();
  nl = quad->side[3].NbPoints();
  dh = abs(nb-nt);
  dv = abs(nr-nl);
  int nbh  = Max(nb,nt);
  int nbv  = Max(nr,nl);
  int addh = 0;
  int addv = 0;

  // Orientation of face and 3 main domain for future faces
  // ----------- Old version ---------------
  //       0   top    1
  //      1------------1
  //       |   |  |   |
  //       |   |C |   |
  //       | L |  | R |
  //  left |   |__|   | rigth
  //       |  /    \  |
  //       | /  C   \ |
  //       |/        \|
  //      0------------0
  //       0  bottom  1

  // ----------- New version ---------------
  //       0   top    1
  //      1------------1
  //       |   |__|   |
  //       |  /    \  |
  //       | /  C   \ |
  //  left |/________\| rigth
  //       |          |
  //       |    C     |
  //       |          |
  //      0------------0
  //       0  bottom  1


  const int bfrom = quad->side[0].from;
  const int rfrom = quad->side[1].from;
  const int tfrom = quad->side[2].from;
  const int lfrom = quad->side[3].from;
  {
    const vector<UVPtStruct>& uv_eb_vec = quad->side[0].GetUVPtStruct(true,0);
    const vector<UVPtStruct>& uv_er_vec = quad->side[1].GetUVPtStruct(false,1);
    const vector<UVPtStruct>& uv_et_vec = quad->side[2].GetUVPtStruct(true,1);
    const vector<UVPtStruct>& uv_el_vec = quad->side[3].GetUVPtStruct(false,0);
    if (uv_eb_vec.empty() ||
        uv_er_vec.empty() ||
        uv_et_vec.empty() ||
        uv_el_vec.empty())
      return error(COMPERR_BAD_INPUT_MESH);
  }
  FaceQuadStruct::SideIterator uv_eb, uv_er, uv_et, uv_el;
  uv_eb.Init( quad->side[0] );
  uv_er.Init( quad->side[1] );
  uv_et.Init( quad->side[2] );
  uv_el.Init( quad->side[3] );

  gp_UV a0,a1,a2,a3, p0,p1,p2,p3, uv;
  double x,y;

  a0 = uv_eb[ 0 ].UV();
  a1 = uv_er[ 0 ].UV();
  a2 = uv_er[ nr-1 ].UV();
  a3 = uv_et[ 0 ].UV();

  if ( !myForcedPnts.empty() )
  {
    if ( dv != 0 && dh != 0 ) // here myQuadList.size() == 1
    {
      const int dmin = Min( dv, dh );

      // Make a side separating domains L and Cb
      StdMeshers_FaceSidePtr sideLCb;
      UVPtStruct p3dom; // a point where 3 domains meat
      {                                                     //   dmin
        vector<UVPtStruct> pointsLCb( dmin+1 );             // 1--------1
        pointsLCb[0] = uv_eb[0];                            //  |   |  |
        for ( int i = 1; i <= dmin; ++i )                   //  |   |Ct|
        {                                                   //  | L |  |
          x  = uv_et[ i ].normParam;                        //  |   |__|
          y  = uv_er[ i ].normParam;                        //  |  /   |
          p0 = quad->side[0].grid->Value2d( x ).XY();       //  | / Cb |dmin
          p1 = uv_er[ i ].UV();                             //  |/     |
          p2 = uv_et[ i ].UV();                             // 0--------0
          p3 = quad->side[3].grid->Value2d( y ).XY();
          uv = calcUV( x,y, a0,a1,a2,a3, p0,p1,p2,p3 );
          pointsLCb[ i ].u = uv.X();
          pointsLCb[ i ].v = uv.Y();
        }
        sideLCb = StdMeshers_FaceSide::New( pointsLCb, aFace );
        p3dom   = pointsLCb.back();

        gp_Pnt xyz = S->Value( p3dom.u, p3dom.v );
        p3dom.node = myHelper->AddNode( xyz.X(), xyz.Y(), xyz.Z(), 0, p3dom.u, p3dom.v );
        pointsLCb.back() = p3dom;
      }
      // Make a side separating domains L and Ct
      StdMeshers_FaceSidePtr sideLCt;
      {
        vector<UVPtStruct> pointsLCt( nl );
        pointsLCt[0]     = p3dom;
        pointsLCt.back() = uv_et[ dmin ];
        x  = uv_et[ dmin ].normParam;
        p0 = quad->side[0].grid->Value2d( x ).XY();
        p2 = uv_et[ dmin ].UV();
        double y0 = uv_er[ dmin ].normParam;
        for ( int i = 1; i < nl-1; ++i )
        {
          y  = y0 + i / ( nl-1. ) * ( 1. - y0 );
          p1 = quad->side[1].grid->Value2d( y ).XY();
          p3 = quad->side[3].grid->Value2d( y ).XY();
          uv = calcUV( x,y, a0,a1,a2,a3, p0,p1,p2,p3 );
          pointsLCt[ i ].u = uv.X();
          pointsLCt[ i ].v = uv.Y();
        }
        sideLCt = StdMeshers_FaceSide::New( pointsLCt, aFace );
      }
      // Make a side separating domains Cb and Ct
      StdMeshers_FaceSidePtr sideCbCt;
      {
        vector<UVPtStruct> pointsCbCt( nb );
        pointsCbCt[0]     = p3dom;
        pointsCbCt.back() = uv_er[ dmin ];
        y  = uv_er[ dmin ].normParam;
        p1 = uv_er[ dmin ].UV();
        p3 = quad->side[3].grid->Value2d( y ).XY();
        double x0 = uv_et[ dmin ].normParam;
        for ( int i = 1; i < nb-1; ++i )
        {
          x  = x0 + i / ( nb-1. ) * ( 1. - x0 );
          p2 = quad->side[2].grid->Value2d( x ).XY();
          p0 = quad->side[0].grid->Value2d( x ).XY();
          uv = calcUV( x,y, a0,a1,a2,a3, p0,p1,p2,p3 );
          pointsCbCt[ i ].u = uv.X();
          pointsCbCt[ i ].v = uv.Y();
        }
        sideCbCt = StdMeshers_FaceSide::New( pointsCbCt, aFace );
      }
      // Make Cb quad
      FaceQuadStruct* qCb = new FaceQuadStruct( quad->face, "Cb" );
      myQuadList.push_back( FaceQuadStruct::Ptr( qCb ));
      qCb->side.resize(4);
      qCb->side[0] = quad->side[0];
      qCb->side[1] = quad->side[1];
      qCb->side[2] = sideCbCt;
      qCb->side[3] = sideLCb;
      qCb->side[1].to = dmin+1;
      // Make L quad
      FaceQuadStruct* qL = new FaceQuadStruct( quad->face, "L" );
      myQuadList.push_back( FaceQuadStruct::Ptr( qL ));
      qL->side.resize(4);
      qL->side[0] = sideLCb;
      qL->side[1] = sideLCt;
      qL->side[2] = quad->side[2];
      qL->side[3] = quad->side[3];
      qL->side[2].to = dmin+1;
      // Make Ct from the main quad
      FaceQuadStruct::Ptr qCt = quad;
      qCt->side[0] = sideCbCt;
      qCt->side[3] = sideLCt;
      qCt->side[1].from = dmin;
      qCt->side[2].from = dmin;
      qCt->uv_grid.clear();
      qCt->name = "Ct";

      // Connect sides
      qCb->side[3].AddContact( dmin, & qCb->side[2], 0 );
      qCb->side[3].AddContact( dmin, & qCt->side[3], 0 );
      qCt->side[3].AddContact(    0, & qCt->side[0], 0 );
      qCt->side[0].AddContact(    0, & qL ->side[0], dmin );
      qL ->side[0].AddContact( dmin, & qL ->side[1], 0 );
      qL ->side[0].AddContact( dmin, & qCb->side[2], 0 );

      if ( dh == dv )
        return computeQuadDominant( aMesh, aFace );
      else
        return computeQuadPref( aMesh, aFace, qCt );

    } // if ( dv != 0 && dh != 0 )

    const int db = quad->side[0].IsReversed() ? -1 : +1;
    const int dr = quad->side[1].IsReversed() ? -1 : +1;
    const int dt = quad->side[2].IsReversed() ? -1 : +1;
    const int dl = quad->side[3].IsReversed() ? -1 : +1;

    // Case dv == 0,  here possibly myQuadList.size() > 1
    //
    //     lw   nb  lw = dh/2
    //    +------------+
    //    |   |    |   |
    //    |   | Ct |   |
    //    | L |    | R |
    //    |   |____|   |
    //    |  /      \  |
    //    | /   Cb   \ |
    //    |/          \|
    //    +------------+
    const int lw = dh/2; // lateral width

    double yCbL, yCbR;
    {
      double   lL = quad->side[3].Length();
      double lLwL = quad->side[2].Length( tfrom,
                                          tfrom + ( lw ) * dt );
      yCbL = lLwL / ( lLwL + lL );

      double   lR = quad->side[1].Length();
      double lLwR = quad->side[2].Length( tfrom + ( lw + nb-1 ) * dt,
                                          tfrom + ( lw + nb-1 + lw ) * dt);
      yCbR = lLwR / ( lLwR + lR );
    }
    // Make sides separating domains Cb and L and R
    StdMeshers_FaceSidePtr sideLCb, sideRCb;
    UVPtStruct pTBL, pTBR; // points where 3 domains meat
    {
      vector<UVPtStruct> pointsLCb( lw+1 ), pointsRCb( lw+1 );
      pointsLCb[0] = uv_eb[ 0    ];
      pointsRCb[0] = uv_eb[ nb-1 ];
      for ( int i = 1, i2 = nt-2; i <= lw; ++i, --i2 )
      {
        x  = quad->side[2].Param( i );
        y  = yCbL * i / lw;
        p0 = quad->side[0].Value2d( x );
        p1 = quad->side[1].Value2d( y );
        p2 = uv_et[ i ].UV();
        p3 = quad->side[3].Value2d( y );
        uv = calcUV( x,y, a0,a1,a2,a3, p0,p1,p2,p3 );
        pointsLCb[ i ].u = uv.X();
        pointsLCb[ i ].v = uv.Y();
        pointsLCb[ i ].x = x;

        x  = quad->side[2].Param( i2 );
        y  = yCbR * i / lw;
        p1 = quad->side[1].Value2d( y );
        p0 = quad->side[0].Value2d( x );
        p2 = uv_et[ i2 ].UV();
        p3 = quad->side[3].Value2d( y );
        uv = calcUV( x,y, a0,a1,a2,a3, p0,p1,p2,p3 );
        pointsRCb[ i ].u = uv.X();
        pointsRCb[ i ].v = uv.Y();
        pointsRCb[ i ].x = x;
      }
      sideLCb = StdMeshers_FaceSide::New( pointsLCb, aFace );
      sideRCb = StdMeshers_FaceSide::New( pointsRCb, aFace );
      pTBL    = pointsLCb.back();
      pTBR    = pointsRCb.back();
      {
        gp_Pnt xyz = S->Value( pTBL.u, pTBL.v );
        pTBL.node = myHelper->AddNode( xyz.X(), xyz.Y(), xyz.Z(), 0, pTBL.u, pTBL.v );
        pointsLCb.back() = pTBL;
      }
      {
        gp_Pnt xyz = S->Value( pTBR.u, pTBR.v );
        pTBR.node = myHelper->AddNode( xyz.X(), xyz.Y(), xyz.Z(), 0, pTBR.u, pTBR.v );
        pointsRCb.back() = pTBR;
      }
    }
    // Make sides separating domains Ct and L and R
    StdMeshers_FaceSidePtr sideLCt, sideRCt;
    {
      vector<UVPtStruct> pointsLCt( nl ), pointsRCt( nl );
      pointsLCt[0]     = pTBL;
      pointsLCt.back() = uv_et[ lw ];
      pointsRCt[0]     = pTBR;
      pointsRCt.back() = uv_et[ lw + nb - 1 ];
      x  = pTBL.x;
      p0 = quad->side[0].Value2d( x );
      p2 = uv_et[ lw ].UV();
      int     iR = lw + nb - 1;
      double  xR = pTBR.x;
      gp_UV  p0R = quad->side[0].Value2d( xR );
      gp_UV  p2R = uv_et[ iR ].UV();
      for ( int i = 1; i < nl-1; ++i )
      {
        y  = yCbL + ( 1. - yCbL ) * i / (nl-1.);
        p1 = quad->side[1].Value2d( y );
        p3 = quad->side[3].Value2d( y );
        uv = calcUV( x,y, a0,a1,a2,a3, p0,p1,p2,p3 );
        pointsLCt[ i ].u = uv.X();
        pointsLCt[ i ].v = uv.Y();

        y  = yCbR + ( 1. - yCbR ) * i / (nl-1.);
        p1 = quad->side[1].Value2d( y );
        p3 = quad->side[3].Value2d( y );
        uv = calcUV( xR,y, a0,a1,a2,a3, p0R,p1,p2R,p3 );
        pointsRCt[ i ].u = uv.X();
        pointsRCt[ i ].v = uv.Y();
      }
      sideLCt = StdMeshers_FaceSide::New( pointsLCt, aFace );
      sideRCt = StdMeshers_FaceSide::New( pointsRCt, aFace );
    }
    // Make a side separating domains Cb and Ct
    StdMeshers_FaceSidePtr sideCbCt;
    {
      vector<UVPtStruct> pointsCbCt( nb );
      pointsCbCt[0]     = pTBL;
      pointsCbCt.back() = pTBR;
      p1 = quad->side[1].Value2d( yCbR );
      p3 = quad->side[3].Value2d( yCbL );
      for ( int i = 1; i < nb-1; ++i )
      {
        x  = quad->side[2].Param( i + lw );
        y  = yCbL + ( yCbR - yCbL ) * i / (nb-1.);
        p2 = uv_et[ i + lw ].UV();
        p0 = quad->side[0].Value2d( x );
        uv = calcUV( x,y, a0,a1,a2,a3, p0,p1,p2,p3 );
        pointsCbCt[ i ].u = uv.X();
        pointsCbCt[ i ].v = uv.Y();
      }
      sideCbCt = StdMeshers_FaceSide::New( pointsCbCt, aFace );
    }
    // Make Cb quad
    FaceQuadStruct* qCb = new FaceQuadStruct( quad->face, "Cb" );
    myQuadList.push_back( FaceQuadStruct::Ptr( qCb ));
    qCb->side.resize(4);
    qCb->side[0] = quad->side[0];
    qCb->side[1] = sideRCb;
    qCb->side[2] = sideCbCt;
    qCb->side[3] = sideLCb;
    // Make L quad
    FaceQuadStruct* qL = new FaceQuadStruct( quad->face, "L" );
    myQuadList.push_back( FaceQuadStruct::Ptr( qL ));
    qL->side.resize(4);
    qL->side[0] = sideLCb;
    qL->side[1] = sideLCt;
    qL->side[2] = quad->side[2];
    qL->side[3] = quad->side[3];
    qL->side[2].to = ( lw + 1 ) * dt + tfrom;
    // Make R quad
    FaceQuadStruct* qR = new FaceQuadStruct( quad->face, "R" );
    myQuadList.push_back( FaceQuadStruct::Ptr( qR ));
    qR->side.resize(4);
    qR->side[0] = sideRCb;
    qR->side[0].from = lw;
    qR->side[0].to   = -1;
    qR->side[0].di   = -1;
    qR->side[1] = quad->side[1];
    qR->side[2] = quad->side[2];
    qR->side[2].from = ( lw + nb-1 ) * dt + tfrom;
    qR->side[3] = sideRCt;
    // Make Ct from the main quad
    FaceQuadStruct::Ptr qCt = quad;
    qCt->side[0] = sideCbCt;
    qCt->side[1] = sideRCt;
    qCt->side[2].from = ( lw ) * dt + tfrom;
    qCt->side[2].to   = ( lw + nb ) * dt + tfrom;
    qCt->side[3] = sideLCt;
    qCt->uv_grid.clear();
    qCt->name = "Ct";

    // Connect sides
    qCb->side[3].AddContact( lw, & qCb->side[2], 0 );
    qCb->side[3].AddContact( lw, & qCt->side[3], 0 );
    qCt->side[3].AddContact( 0,  & qCt->side[0], 0 );
    qCt->side[0].AddContact( 0,  & qL ->side[0], lw );
    qL ->side[0].AddContact( lw, & qL ->side[1], 0 );
    qL ->side[0].AddContact( lw, & qCb->side[2], 0 );
    //
    qCb->side[1].AddContact( lw,   & qCb->side[2], nb-1 );
    qCb->side[1].AddContact( lw,   & qCt->side[1], 0 );
    qCt->side[0].AddContact( nb-1, & qCt->side[1], 0 );
    qCt->side[0].AddContact( nb-1, & qR ->side[0], lw );
    qR ->side[3].AddContact( 0,    & qR ->side[0], lw );
    qR ->side[3].AddContact( 0,    & qCb->side[2], nb-1 );

    return computeQuadDominant( aMesh, aFace );

  } // if ( !myForcedPnts.empty() )

  if ( dh > dv ) {
    addv = (dh-dv)/2;
    nbv  = nbv + addv;
  }
  else { // dv >= dh
    addh = (dv-dh)/2;
    nbh  = nbh + addh;
  }

  // arrays for normalized params
  TColStd_SequenceOfReal npb, npr, npt, npl;
  for (i=0; i<nb; i++) {
    npb.Append(uv_eb[i].normParam);
  }
  for (i=0; i<nr; i++) {
    npr.Append(uv_er[i].normParam);
  }
  for (i=0; i<nt; i++) {
    npt.Append(uv_et[i].normParam);
  }
  for (i=0; i<nl; i++) {
    npl.Append(uv_el[i].normParam);
  }

  int dl,dr;
  if (OldVersion) {
    // add some params to right and left after the first param
    // insert to right
    dr = nbv - nr;
    double dpr = (npr.Value(2) - npr.Value(1))/(dr+1);
    for (i=1; i<=dr; i++) {
      npr.InsertAfter(1,npr.Value(2)-dpr);
    }
    // insert to left
    dl = nbv - nl;
    dpr = (npl.Value(2) - npl.Value(1))/(dl+1);
    for (i=1; i<=dl; i++) {
      npl.InsertAfter(1,npl.Value(2)-dpr);
    }
  }

  int nnn = Min(nr,nl);
  // auxilary sequence of XY for creation nodes
  // in the bottom part of central domain
  // Length of UVL and UVR must be == nbv-nnn
  TColgp_SequenceOfXY UVL, UVR, UVT;

  if (OldVersion) {
    // step1: create faces for left domain
    StdMeshers_Array2OfNode NodesL(1,dl+1,1,nl);
    // add left nodes
    for (j=1; j<=nl; j++)
      NodesL.SetValue(1,j,uv_el[j-1].node);
    if (dl>0) {
      // add top nodes
      for (i=1; i<=dl; i++)
        NodesL.SetValue(i+1,nl,uv_et[i].node);
      // create and add needed nodes
      TColgp_SequenceOfXY UVtmp;
      for (i=1; i<=dl; i++) {
        double x0 = npt.Value(i+1);
        double x1 = x0;
        // diagonal node
        double y0 = npl.Value(i+1);
        double y1 = npr.Value(i+1);
        gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode * N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesL.SetValue(i+1,1,N);
        if (UVL.Length()<nbv-nnn) UVL.Append(UV);
        // internal nodes
        for (j=2; j<nl; j++) {
          double y0 = npl.Value(dl+j);
          double y1 = npr.Value(dl+j);
          gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
          gp_Pnt P = S->Value(UV.X(),UV.Y());
          SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
          meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
          NodesL.SetValue(i+1,j,N);
          if (i==dl) UVtmp.Append(UV);
        }
      }
      for (i=1; i<=UVtmp.Length() && UVL.Length()<nbv-nnn; i++) {
        UVL.Append(UVtmp.Value(i));
      }
      // create faces
      for (i=1; i<=dl; i++) {
        for (j=1; j<nl; j++) {
          if (WisF) {
            myHelper->AddFace(NodesL.Value(i,j), NodesL.Value(i+1,j),
                              NodesL.Value(i+1,j+1), NodesL.Value(i,j+1));
          }
        }
      }
    }
    else {
      // fill UVL using c2d
      for (i=1; i<npl.Length() && UVL.Length()<nbv-nnn; i++) {
        UVL.Append(gp_UV (uv_el[i].u, uv_el[i].v));
      }
    }

    // step2: create faces for right domain
    StdMeshers_Array2OfNode NodesR(1,dr+1,1,nr);
    // add right nodes
    for (j=1; j<=nr; j++)
      NodesR.SetValue(1,j,uv_er[nr-j].node);
    if (dr>0) {
      // add top nodes
      for (i=1; i<=dr; i++)
        NodesR.SetValue(i+1,1,uv_et[nt-1-i].node);
      // create and add needed nodes
      TColgp_SequenceOfXY UVtmp;
      for (i=1; i<=dr; i++) {
        double x0 = npt.Value(nt-i);
        double x1 = x0;
        // diagonal node
        double y0 = npl.Value(i+1);
        double y1 = npr.Value(i+1);
        gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode * N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesR.SetValue(i+1,nr,N);
        if (UVR.Length()<nbv-nnn) UVR.Append(UV);
        // internal nodes
        for (j=2; j<nr; j++) {
          double y0 = npl.Value(nbv-j+1);
          double y1 = npr.Value(nbv-j+1);
          gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
          gp_Pnt P = S->Value(UV.X(),UV.Y());
          SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
          meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
          NodesR.SetValue(i+1,j,N);
          if (i==dr) UVtmp.Prepend(UV);
        }
      }
      for (i=1; i<=UVtmp.Length() && UVR.Length()<nbv-nnn; i++) {
        UVR.Append(UVtmp.Value(i));
      }
      // create faces
      for (i=1; i<=dr; i++) {
        for (j=1; j<nr; j++) {
          if (WisF) {
            myHelper->AddFace(NodesR.Value(i,j), NodesR.Value(i+1,j),
                              NodesR.Value(i+1,j+1), NodesR.Value(i,j+1));
          }
        }
      }
    }
    else {
      // fill UVR using c2d
      for (i=1; i<npr.Length() && UVR.Length()<nbv-nnn; i++) {
        UVR.Append(gp_UV(uv_er[i].u, uv_er[i].v));
      }
    }

    // step3: create faces for central domain
    StdMeshers_Array2OfNode NodesC(1,nb,1,nbv);
    // add first line using NodesL
    for (i=1; i<=dl+1; i++)
      NodesC.SetValue(1,i,NodesL(i,1));
    for (i=2; i<=nl; i++)
      NodesC.SetValue(1,dl+i,NodesL(dl+1,i));
    // add last line using NodesR
    for (i=1; i<=dr+1; i++)
      NodesC.SetValue(nb,i,NodesR(i,nr));
    for (i=1; i<nr; i++)
      NodesC.SetValue(nb,dr+i+1,NodesR(dr+1,nr-i));
    // add top nodes (last columns)
    for (i=dl+2; i<nbh-dr; i++)
      NodesC.SetValue(i-dl,nbv,uv_et[i-1].node);
    // add bottom nodes (first columns)
    for (i=2; i<nb; i++)
      NodesC.SetValue(i,1,uv_eb[i-1].node);

    // create and add needed nodes
    // add linear layers
    for (i=2; i<nb; i++) {
      double x0 = npt.Value(dl+i);
      double x1 = x0;
      for (j=1; j<nnn; j++) {
        double y0 = npl.Value(nbv-nnn+j);
        double y1 = npr.Value(nbv-nnn+j);
        gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesC.SetValue(i,nbv-nnn+j,N);
        if ( j==1 )
          UVT.Append( UV );
      }
    }
    // add diagonal layers
    gp_UV A2 = UVR.Value(nbv-nnn);
    gp_UV A3 = UVL.Value(nbv-nnn);
    for (i=1; i<nbv-nnn; i++) {
      gp_UV p1 = UVR.Value(i);
      gp_UV p3 = UVL.Value(i);
      double y = i / double(nbv-nnn);
      for (j=2; j<nb; j++) {
        double x = npb.Value(j);
        gp_UV p0( uv_eb[j-1].u, uv_eb[j-1].v );
        gp_UV p2 = UVT.Value( j-1 );
        gp_UV UV = calcUV(x, y, a0, a1, A2, A3, p0,p1,p2,p3 );
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(),UV.Y());
        NodesC.SetValue(j,i+1,N);
      }
    }
    // create faces
    for (i=1; i<nb; i++) {
      for (j=1; j<nbv; j++) {
        if (WisF) {
          myHelper->AddFace(NodesC.Value(i,j), NodesC.Value(i+1,j),
                            NodesC.Value(i+1,j+1), NodesC.Value(i,j+1));
        }
      }
    }
  }

  else { // New version (!OldVersion)
    // step1: create faces for bottom rectangle domain
    StdMeshers_Array2OfNode NodesBRD(1,nb,1,nnn-1);
    // fill UVL and UVR using c2d
    for (j=0; j<nb; j++) {
      NodesBRD.SetValue(j+1,1,uv_eb[j].node);
    }
    for (i=1; i<nnn-1; i++) {
      NodesBRD.SetValue(1,i+1,uv_el[i].node);
      NodesBRD.SetValue(nb,i+1,uv_er[i].node);
      for (j=2; j<nb; j++) {
        double x = npb.Value(j);
        double y = (1-x) * npl.Value(i+1) + x * npr.Value(i+1);
        gp_UV UV = calcUV2(x, y, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(),UV.Y());
        NodesBRD.SetValue(j,i+1,N);
      }
    }
    for (j=1; j<nnn-1; j++) {
      for (i=1; i<nb; i++) {
        if (WisF) {
          myHelper->AddFace(NodesBRD.Value(i,j), NodesBRD.Value(i+1,j),
                            NodesBRD.Value(i+1,j+1), NodesBRD.Value(i,j+1));
        }
      }
    }
    int drl = abs(nr-nl);
    // create faces for region C
    StdMeshers_Array2OfNode NodesC(1,nb,1,drl+1+addv);
    // add nodes from previous region
    for (j=1; j<=nb; j++) {
      NodesC.SetValue(j,1,NodesBRD.Value(j,nnn-1));
    }
    if ((drl+addv) > 0) {
      int n1,n2;
      if (nr>nl) {
        n1 = 1;
        n2 = drl + 1;
        TColgp_SequenceOfXY UVtmp;
        double drparam = npr.Value(nr) - npr.Value(nnn-1);
        double dlparam = npl.Value(nnn) - npl.Value(nnn-1);
        double y0,y1;
        for (i=1; i<=drl; i++) {
          // add existed nodes from right edge
          NodesC.SetValue(nb,i+1,uv_er[nnn+i-2].node);
          //double dtparam = npt.Value(i+1);
          y1 = npr.Value(nnn+i-1); // param on right edge
          double dpar = (y1 - npr.Value(nnn-1))/drparam;
          y0 = npl.Value(nnn-1) + dpar*dlparam; // param on left edge
          double dy = y1 - y0;
          for (j=1; j<nb; j++) {
            double x = npt.Value(i+1) + npb.Value(j)*(1-npt.Value(i+1));
            double y = y0 + dy*x;
            gp_UV UV = calcUV2(x, y, quad, a0, a1, a2, a3);
            gp_Pnt P = S->Value(UV.X(),UV.Y());
            SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
            meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
            NodesC.SetValue(j,i+1,N);
          }
        }
        double dy0 = (1-y0)/(addv+1);
        double dy1 = (1-y1)/(addv+1);
        for (i=1; i<=addv; i++) {
          double yy0 = y0 + dy0*i;
          double yy1 = y1 + dy1*i;
          double dyy = yy1 - yy0;
          for (j=1; j<=nb; j++) {
            double x = npt.Value(i+1+drl) +
              npb.Value(j) * (npt.Value(nt-i) - npt.Value(i+1+drl));
            double y = yy0 + dyy*x;
            gp_UV UV = calcUV2(x, y, quad, a0, a1, a2, a3);
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
        for (i=1; i<=drl; i++) {
          // add existed nodes from right edge
          NodesC.SetValue(1,i+1,uv_el[nnn+i-2].node);
          y0 = npl.Value(nnn+i-1); // param on left edge
          double dpar = (y0 - npl.Value(nnn-1))/dlparam;
          y1 = npr.Value(nnn-1) + dpar*drparam; // param on right edge
          double dy = y1 - y0;
          for (j=2; j<=nb; j++) {
            double x = npb.Value(j)*npt.Value(nt-i);
            double y = y0 + dy*x;
            gp_UV UV = calcUV2(x, y, quad, a0, a1, a2, a3);
            gp_Pnt P = S->Value(UV.X(),UV.Y());
            SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
            meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
            NodesC.SetValue(j,i+1,N);
          }
        }
        double dy0 = (1-y0)/(addv+1);
        double dy1 = (1-y1)/(addv+1);
        for (i=1; i<=addv; i++) {
          double yy0 = y0 + dy0*i;
          double yy1 = y1 + dy1*i;
          double dyy = yy1 - yy0;
          for (j=1; j<=nb; j++) {
            double x = npt.Value(i+1) +
              npb.Value(j) * (npt.Value(nt-i-drl) - npt.Value(i+1));
            double y = yy0 + dyy*x;
            gp_UV UV = calcUV2(x, y, quad, a0, a1, a2, a3);
            gp_Pnt P = S->Value(UV.X(),UV.Y());
            SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
            meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
            NodesC.SetValue(j,i+drl+1,N);
          }
        }
      }
      // create faces
      for (j=1; j<=drl+addv; j++) {
        for (i=1; i<nb; i++) {
          if (WisF) {
            myHelper->AddFace(NodesC.Value(i,j), NodesC.Value(i+1,j),
                              NodesC.Value(i+1,j+1), NodesC.Value(i,j+1));
          }
        }
      } // end nr<nl

      StdMeshers_Array2OfNode NodesLast(1,nt,1,2);
      for (i=1; i<=nt; i++) {
        NodesLast.SetValue(i,2,uv_et[i-1].node);
      }
      int nnn=0;
      for (i=n1; i<drl+addv+1; i++) {
        nnn++;
        NodesLast.SetValue(nnn,1,NodesC.Value(1,i));
      }
      for (i=1; i<=nb; i++) {
        nnn++;
        NodesLast.SetValue(nnn,1,NodesC.Value(i,drl+addv+1));
      }
      for (i=drl+addv; i>=n2; i--) {
        nnn++;
        NodesLast.SetValue(nnn,1,NodesC.Value(nb,i));
      }
      for (i=1; i<nt; i++) {
        if (WisF) {
          myHelper->AddFace(NodesLast.Value(i,1), NodesLast.Value(i+1,1),
                            NodesLast.Value(i+1,2), NodesLast.Value(i,2));
        }
      }
    } // if ((drl+addv) > 0)

  } // end new version implementation

  bool isOk = true;
  return isOk;
}


//=======================================================================
/*!
 * Evaluate only quandrangle faces
 */
//=======================================================================

bool StdMeshers_Quadrangle_2D::evaluateQuadPref(SMESH_Mesh &        aMesh,
                                                const TopoDS_Shape& aShape,
                                                std::vector<int>&   aNbNodes,
                                                MapShapeNbElems&    aResMap,
                                                bool                IsQuadratic)
{
  // Auxilary key in order to keep old variant
  // of meshing after implementation new variant
  // for bug 0016220 from Mantis.
  bool OldVersion = false;
  if (myQuadType == QUAD_QUADRANGLE_PREF_REVERSED)
    OldVersion = true;

  const TopoDS_Face& F = TopoDS::Face(aShape);
  Handle(Geom_Surface) S = BRep_Tool::Surface(F);

  int nb = aNbNodes[0];
  int nr = aNbNodes[1];
  int nt = aNbNodes[2];
  int nl = aNbNodes[3];
  int dh = abs(nb-nt);
  int dv = abs(nr-nl);

  if (dh>=dv) {
    if (nt>nb) {
      // it is a base case => not shift 
    }
    else {
      // we have to shift on 2
      nb = aNbNodes[2];
      nr = aNbNodes[3];
      nt = aNbNodes[0];
      nl = aNbNodes[1];
    }
  }
  else {
    if (nr>nl) {
      // we have to shift quad on 1
      nb = aNbNodes[3];
      nr = aNbNodes[0];
      nt = aNbNodes[1];
      nl = aNbNodes[2];
    }
    else {
      // we have to shift quad on 3
      nb = aNbNodes[1];
      nr = aNbNodes[2];
      nt = aNbNodes[3];
      nl = aNbNodes[0];
    }
  }

  dh = abs(nb-nt);
  dv = abs(nr-nl);
  int nbh  = Max(nb,nt);
  int nbv = Max(nr,nl);
  int addh = 0;
  int addv = 0;

  if (dh>dv) {
    addv = (dh-dv)/2;
    nbv = nbv + addv;
  }
  else { // dv>=dh
    addh = (dv-dh)/2;
    nbh = nbh + addh;
  }

  int dl,dr;
  if (OldVersion) {
    // add some params to right and left after the first param
    // insert to right
    dr = nbv - nr;
    // insert to left
    dl = nbv - nl;
  }
  
  int nnn = Min(nr,nl);

  int nbNodes = 0;
  int nbFaces = 0;
  if (OldVersion) {
    // step1: create faces for left domain
    if (dl>0) {
      nbNodes += dl*(nl-1);
      nbFaces += dl*(nl-1);
    }
    // step2: create faces for right domain
    if (dr>0) {
      nbNodes += dr*(nr-1);
      nbFaces += dr*(nr-1);
    }
    // step3: create faces for central domain
    nbNodes += (nb-2)*(nnn-1) + (nbv-nnn-1)*(nb-2);
    nbFaces += (nb-1)*(nbv-1);
  }
  else { // New version (!OldVersion)
    nbNodes += (nnn-2)*(nb-2);
    nbFaces += (nnn-2)*(nb-1);
    int drl = abs(nr-nl);
    nbNodes += drl*(nb-1) + addv*nb;
    nbFaces += (drl+addv)*(nb-1) + (nt-1);
  } // end new version implementation

  std::vector<int> aVec(SMDSEntity_Last);
  for (int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i] = 0;
  if (IsQuadratic) {
    aVec[SMDSEntity_Quad_Quadrangle] = nbFaces;
    aVec[SMDSEntity_Node] = nbNodes + nbFaces*4;
    if (aNbNodes.size()==5) {
      aVec[SMDSEntity_Quad_Triangle] = aNbNodes[3] - 1;
      aVec[SMDSEntity_Quad_Quadrangle] = nbFaces - aNbNodes[3] + 1;
    }
  }
  else {
    aVec[SMDSEntity_Node] = nbNodes;
    aVec[SMDSEntity_Quadrangle] = nbFaces;
    if (aNbNodes.size()==5) {
      aVec[SMDSEntity_Triangle] = aNbNodes[3] - 1;
      aVec[SMDSEntity_Quadrangle] = nbFaces - aNbNodes[3] + 1;
    }
  }
  SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
  aResMap.insert(std::make_pair(sm,aVec));

  return true;
}

//=============================================================================
/*! Split quadrangle in to 2 triangles by smallest diagonal
 *   
 */
//=============================================================================

void StdMeshers_Quadrangle_2D::splitQuadFace(SMESHDS_Mesh *       theMeshDS,
                                             int                  theFaceID,
                                             const SMDS_MeshNode* theNode1,
                                             const SMDS_MeshNode* theNode2,
                                             const SMDS_MeshNode* theNode3,
                                             const SMDS_MeshNode* theNode4)
{
  if ( SMESH_TNodeXYZ( theNode1 ).SquareDistance( theNode3 ) >
       SMESH_TNodeXYZ( theNode2 ).SquareDistance( theNode4 ) )
  {
    myHelper->AddFace(theNode2, theNode4 , theNode1);
    myHelper->AddFace(theNode2, theNode3, theNode4);
  }
  else
  {
    myHelper->AddFace(theNode1, theNode2 ,theNode3);
    myHelper->AddFace(theNode1, theNode3, theNode4);
  }
}

namespace
{
  enum uvPos { UV_A0, UV_A1, UV_A2, UV_A3, UV_B, UV_R, UV_T, UV_L, UV_SIZE };

  inline  SMDS_MeshNode* makeNode( UVPtStruct &         uvPt,
                                   const double         y,
                                   FaceQuadStruct::Ptr& quad,
                                   const gp_UV*         UVs,
                                   SMESH_MesherHelper*  helper,
                                   Handle(Geom_Surface) S)
  {
    const vector<UVPtStruct>& uv_eb = quad->side[QUAD_BOTTOM_SIDE].GetUVPtStruct();
    const vector<UVPtStruct>& uv_et = quad->side[QUAD_TOP_SIDE   ].GetUVPtStruct();
    double rBot = ( uv_eb.size() - 1 ) * uvPt.normParam;
    double rTop = ( uv_et.size() - 1 ) * uvPt.normParam;
    int iBot = int( rBot );
    int iTop = int( rTop );
    double xBot = uv_eb[ iBot ].normParam + ( rBot - iBot ) * ( uv_eb[ iBot+1 ].normParam - uv_eb[ iBot ].normParam );
    double xTop = uv_et[ iTop ].normParam + ( rTop - iTop ) * ( uv_et[ iTop+1 ].normParam - uv_et[ iTop ].normParam );
    double x = xBot + y * ( xTop - xBot );
    
    gp_UV uv = calcUV(/*x,y=*/x, y,
                      /*a0,...=*/UVs[UV_A0], UVs[UV_A1], UVs[UV_A2], UVs[UV_A3],
                      /*p0=*/quad->side[QUAD_BOTTOM_SIDE].grid->Value2d( x ).XY(),
                      /*p1=*/UVs[ UV_R ],
                      /*p2=*/quad->side[QUAD_TOP_SIDE   ].grid->Value2d( x ).XY(),
                      /*p3=*/UVs[ UV_L ]);
    gp_Pnt P = S->Value( uv.X(), uv.Y() );
    uvPt.u = uv.X();
    uvPt.v = uv.Y();
    return helper->AddNode(P.X(), P.Y(), P.Z(), 0, uv.X(), uv.Y() );
  }

  void reduce42( const vector<UVPtStruct>& curr_base,
                 vector<UVPtStruct>&       next_base,
                 const int                 j,
                 int &                     next_base_len,
                 FaceQuadStruct::Ptr&      quad,
                 gp_UV*                    UVs,
                 const double              y,
                 SMESH_MesherHelper*       helper,
                 Handle(Geom_Surface)&     S)
  {
    // add one "HH": nodes a,b,c,d,e and faces 1,2,3,4,5,6
    //
    //  .-----a-----b i + 1
    //  |\ 5  | 6  /|
    //  | \   |   / |
    //  |  c--d--e  |
    //  |1 |2 |3 |4 |
    //  |  |  |  |  |
    //  .--.--.--.--. i
    //
    //  j     j+2   j+4

    // a (i + 1, j + 2)
    const SMDS_MeshNode*& Na = next_base[ ++next_base_len ].node;
    if ( !Na )
      Na = makeNode( next_base[ next_base_len ], y, quad, UVs, helper, S );

    // b (i + 1, j + 4)
    const SMDS_MeshNode*& Nb = next_base[ ++next_base_len ].node;
    if ( !Nb )
      Nb = makeNode( next_base[ next_base_len ], y, quad, UVs, helper, S );

    // c
    double u = (curr_base[j + 2].u + next_base[next_base_len - 2].u) / 2.0;
    double v = (curr_base[j + 2].v + next_base[next_base_len - 2].v) / 2.0;
    gp_Pnt P = S->Value(u,v);
    SMDS_MeshNode* Nc = helper->AddNode(P.X(), P.Y(), P.Z(), 0, u, v);

    // d
    u = (curr_base[j + 2].u + next_base[next_base_len - 1].u) / 2.0;
    v = (curr_base[j + 2].v + next_base[next_base_len - 1].v) / 2.0;
    P = S->Value(u,v);
    SMDS_MeshNode* Nd = helper->AddNode(P.X(), P.Y(), P.Z(), 0, u, v);

    // e
    u = (curr_base[j + 2].u + next_base[next_base_len].u) / 2.0;
    v = (curr_base[j + 2].v + next_base[next_base_len].v) / 2.0;
    P = S->Value(u,v);
    SMDS_MeshNode* Ne = helper->AddNode(P.X(), P.Y(), P.Z(), 0, u, v);

    // Faces
    helper->AddFace(curr_base[j + 0].node,
                    curr_base[j + 1].node, Nc,
                    next_base[next_base_len - 2].node);

    helper->AddFace(curr_base[j + 1].node,
                    curr_base[j + 2].node, Nd, Nc);

    helper->AddFace(curr_base[j + 2].node,
                    curr_base[j + 3].node, Ne, Nd);

    helper->AddFace(curr_base[j + 3].node,
                    curr_base[j + 4].node, Nb, Ne);

    helper->AddFace(Nc, Nd, Na, next_base[next_base_len - 2].node);

    helper->AddFace(Nd, Ne, Nb, Na);
  }

  void reduce31( const vector<UVPtStruct>& curr_base,
                 vector<UVPtStruct>&       next_base,
                 const int                 j,
                 int &                     next_base_len,
                 FaceQuadStruct::Ptr&      quad,
                 gp_UV*                    UVs,
                 const double              y,
                 SMESH_MesherHelper*       helper,
                 Handle(Geom_Surface)&     S)
  {
    // add one "H": nodes b,c,e and faces 1,2,4,5
    //
    //  .---------b i + 1
    //  |\   5   /|
    //  | \     / |
    //  |  c---e  |
    //  |1 |2  |4 |
    //  |  |   |  |
    //  .--.---.--. i
    //
    //  j j+1 j+2 j+3

    // b (i + 1, j + 3)
    const SMDS_MeshNode*& Nb = next_base[ ++next_base_len ].node;
    if ( !Nb )
      Nb = makeNode( next_base[ next_base_len ], y, quad, UVs, helper, S );

    // c and e
    double u1 = (curr_base[ j   ].u + next_base[ next_base_len-1 ].u ) / 2.0;
    double u2 = (curr_base[ j+3 ].u + next_base[ next_base_len   ].u ) / 2.0;
    double u3 = (u2 - u1) / 3.0;
    //
    double v1 = (curr_base[ j   ].v + next_base[ next_base_len-1 ].v ) / 2.0;
    double v2 = (curr_base[ j+3 ].v + next_base[ next_base_len   ].v ) / 2.0;
    double v3 = (v2 - v1) / 3.0;
    // c
    double u = u1 + u3;
    double v = v1 + v3;
    gp_Pnt P = S->Value(u,v);
    SMDS_MeshNode* Nc = helper->AddNode( P.X(), P.Y(), P.Z(), 0, u, v );
    // e
    u = u1 + u3 + u3;
    v = v1 + v3 + v3;
    P = S->Value(u,v);
    SMDS_MeshNode* Ne = helper->AddNode( P.X(), P.Y(), P.Z(), 0, u, v );

    // Faces
    // 1
    helper->AddFace( curr_base[ j + 0 ].node,
                     curr_base[ j + 1 ].node,
                     Nc,
                     next_base[ next_base_len - 1 ].node);
    // 2
    helper->AddFace( curr_base[ j + 1 ].node,
                     curr_base[ j + 2 ].node, Ne, Nc);
    // 4
    helper->AddFace( curr_base[ j + 2 ].node,
                     curr_base[ j + 3 ].node, Nb, Ne);
    // 5
    helper->AddFace(Nc, Ne, Nb,
                    next_base[ next_base_len - 1 ].node);
  }

  typedef void (* PReduceFunction) ( const vector<UVPtStruct>& curr_base,
                                     vector<UVPtStruct>&       next_base,
                                     const int                 j,
                                     int &                     next_base_len,
                                     FaceQuadStruct::Ptr &     quad,
                                     gp_UV*                    UVs,
                                     const double              y,
                                     SMESH_MesherHelper*       helper,
                                     Handle(Geom_Surface)&     S);

} // namespace

//=======================================================================
/*!
 *  Implementation of Reduced algorithm (meshing with quadrangles only)
 */
//=======================================================================

bool StdMeshers_Quadrangle_2D::computeReduced (SMESH_Mesh &        aMesh,
                                               const TopoDS_Face&  aFace,
                                               FaceQuadStruct::Ptr quad)
{
  SMESHDS_Mesh * meshDS  = aMesh.GetMeshDS();
  Handle(Geom_Surface) S = BRep_Tool::Surface(aFace);
  int i,j,geomFaceID     = meshDS->ShapeToIndex(aFace);

  int nb = quad->side[0].NbPoints(); // bottom
  int nr = quad->side[1].NbPoints(); // right
  int nt = quad->side[2].NbPoints(); // top
  int nl = quad->side[3].NbPoints(); // left

  //  Simple Reduce 10->8->6->4 (3 steps)     Multiple Reduce 10->4 (1 step)
  //
  //  .-----.-----.-----.-----.               .-----.-----.-----.-----.
  //  |    / \    |    / \    |               |    / \    |    / \    |
  //  |   /    .--.--.    \   |               |    / \    |    / \    |
  //  |   /   /   |   \   \   |               |   /  .----.----.  \   |
  //  .---.---.---.---.---.---.               |   / / \   |   / \ \   |
  //  |   /  / \  |  / \  \   |               |   / / \   |   / \ \   |
  //  |  /  /   .-.-.   \  \  |               |  / /  .---.---.  \ \  |
  //  |  /  /  /  |  \  \  \  |               |  / / / \  |  / \ \ \  |
  //  .--.--.--.--.--.--.--.--.               |  / / /  \ | /  \ \ \  |
  //  |  / /  / \ | / \  \ \  |               | / / /   .-.-.   \ \ \ |
  //  | / /  /  .-.-.  \  \ \ |               | / / /  /  |  \  \ \ \ |
  //  | / / /  /  |  \  \ \ \ |               | / / /  /  |  \  \ \ \ |
  //  .-.-.-.--.--.--.--.-.-.-.               .-.-.-.--.--.--.--.-.-.-.

  bool MultipleReduce = false;
  {
    int nb1 = nb;
    int nr1 = nr;
    int nt1 = nt;

    if (nr == nl) {
      if (nb < nt) {
        nt1 = nb;
        nb1 = nt;
      }
    }
    else if (nb == nt) {
      nr1 = nb; // and == nt
      if (nl < nr) {
        nt1 = nl;
        nb1 = nr;
      }
      else {
        nt1 = nr;
        nb1 = nl;
      }
    }
    else {
      return false;
    }

    // number of rows and columns
    int nrows    = nr1 - 1;
    int ncol_top = nt1 - 1;
    int ncol_bot = nb1 - 1;
    // number of rows needed to reduce ncol_bot to ncol_top using simple 3->1 "tree" (see below)
    int nrows_tree31 =
      int( ceil( log( double(ncol_bot) / ncol_top) / log( 3.))); // = log x base 3
    if ( nrows < nrows_tree31 )
    {
      MultipleReduce = true;
      error( COMPERR_WARNING,
             SMESH_Comment("To use 'Reduced' transition, "
                           "number of face rows should be at least ")
             << nrows_tree31 << ". Actual number of face rows is " << nrows << ". "
             "'Quadrangle preference (reversed)' transion has been used.");
    }
  }

  if (MultipleReduce) { // == computeQuadPref QUAD_QUADRANGLE_PREF_REVERSED
    //==================================================
    int dh = abs(nb-nt);
    int dv = abs(nr-nl);

    if (dh >= dv) {
      if (nt > nb) {
        // it is a base case => not shift quad but may be replacement is need
        shiftQuad(quad,0);
      }
      else {
        // we have to shift quad on 2
        shiftQuad(quad,2);
      }
    }
    else {
      if (nr > nl) {
        // we have to shift quad on 1
        shiftQuad(quad,1);
      }
      else {
        // we have to shift quad on 3
        shiftQuad(quad,3);
      }
    }

    nb = quad->side[0].NbPoints();
    nr = quad->side[1].NbPoints();
    nt = quad->side[2].NbPoints();
    nl = quad->side[3].NbPoints();
    dh = abs(nb-nt);
    dv = abs(nr-nl);
    int nbh = Max(nb,nt);
    int nbv = Max(nr,nl);
    int addh = 0;
    int addv = 0;

    if (dh>dv) {
      addv = (dh-dv)/2;
      nbv = nbv + addv;
    }
    else { // dv>=dh
      addh = (dv-dh)/2;
      nbh = nbh + addh;
    }

    const vector<UVPtStruct>& uv_eb = quad->side[0].GetUVPtStruct(true,0);
    const vector<UVPtStruct>& uv_er = quad->side[1].GetUVPtStruct(false,1);
    const vector<UVPtStruct>& uv_et = quad->side[2].GetUVPtStruct(true,1);
    const vector<UVPtStruct>& uv_el = quad->side[3].GetUVPtStruct(false,0);

    if (uv_eb.size() != nb || uv_er.size() != nr || uv_et.size() != nt || uv_el.size() != nl)
      return error(COMPERR_BAD_INPUT_MESH);

    // arrays for normalized params
    TColStd_SequenceOfReal npb, npr, npt, npl;
    for (j = 0; j < nb; j++) {
      npb.Append(uv_eb[j].normParam);
    }
    for (i = 0; i < nr; i++) {
      npr.Append(uv_er[i].normParam);
    }
    for (j = 0; j < nt; j++) {
      npt.Append(uv_et[j].normParam);
    }
    for (i = 0; i < nl; i++) {
      npl.Append(uv_el[i].normParam);
    }

    int dl,dr;
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

    // add some params to right and left after the first param
    // insert to right
    dr = nbv - nr;
    double dpr = (npr.Value(2) - npr.Value(1))/(dr+1);
    for (i=1; i<=dr; i++) {
      npr.InsertAfter(1,npr.Value(2)-dpr);
    }
    // insert to left
    dl = nbv - nl;
    dpr = (npl.Value(2) - npl.Value(1))/(dl+1);
    for (i=1; i<=dl; i++) {
      npl.InsertAfter(1,npl.Value(2)-dpr);
    }
  
    gp_XY a0 (uv_eb.front().u, uv_eb.front().v);
    gp_XY a1 (uv_eb.back().u,  uv_eb.back().v);
    gp_XY a2 (uv_et.back().u,  uv_et.back().v);
    gp_XY a3 (uv_et.front().u, uv_et.front().v);

    int nnn = Min(nr,nl);
    // auxilary sequence of XY for creation of nodes
    // in the bottom part of central domain
    // it's length must be == nbv-nnn-1
    TColgp_SequenceOfXY UVL;
    TColgp_SequenceOfXY UVR;
    //==================================================

    // step1: create faces for left domain
    StdMeshers_Array2OfNode NodesL(1,dl+1,1,nl);
    // add left nodes
    for (j=1; j<=nl; j++)
      NodesL.SetValue(1,j,uv_el[j-1].node);
    if (dl>0) {
      // add top nodes
      for (i=1; i<=dl; i++) 
        NodesL.SetValue(i+1,nl,uv_et[i].node);
      // create and add needed nodes
      TColgp_SequenceOfXY UVtmp;
      for (i=1; i<=dl; i++) {
        double x0 = npt.Value(i+1);
        double x1 = x0;
        // diagonal node
        double y0 = npl.Value(i+1);
        double y1 = npr.Value(i+1);
        gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode * N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesL.SetValue(i+1,1,N);
        if (UVL.Length()<nbv-nnn-1) UVL.Append(UV);
        // internal nodes
        for (j=2; j<nl; j++) {
          double y0 = npl.Value(dl+j);
          double y1 = npr.Value(dl+j);
          gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
          gp_Pnt P = S->Value(UV.X(),UV.Y());
          SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
          meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
          NodesL.SetValue(i+1,j,N);
          if (i==dl) UVtmp.Append(UV);
        }
      }
      for (i=1; i<=UVtmp.Length() && UVL.Length()<nbv-nnn-1; i++) {
        UVL.Append(UVtmp.Value(i));
      }
      // create faces
      for (i=1; i<=dl; i++) {
        for (j=1; j<nl; j++) {
          myHelper->AddFace(NodesL.Value(i,j), NodesL.Value(i+1,j),
                            NodesL.Value(i+1,j+1), NodesL.Value(i,j+1));
        }
      }
    }
    else {
      // fill UVL using c2d
      for (i=1; i<npl.Length() && UVL.Length()<nbv-nnn-1; i++) {
        UVL.Append(gp_UV (uv_el[i].u, uv_el[i].v));
      }
    }
    
    // step2: create faces for right domain
    StdMeshers_Array2OfNode NodesR(1,dr+1,1,nr);
    // add right nodes
    for (j=1; j<=nr; j++) 
      NodesR.SetValue(1,j,uv_er[nr-j].node);
    if (dr>0) {
      // add top nodes
      for (i=1; i<=dr; i++) 
        NodesR.SetValue(i+1,1,uv_et[nt-1-i].node);
      // create and add needed nodes
      TColgp_SequenceOfXY UVtmp;
      for (i=1; i<=dr; i++) {
        double x0 = npt.Value(nt-i);
        double x1 = x0;
        // diagonal node
        double y0 = npl.Value(i+1);
        double y1 = npr.Value(i+1);
        gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode * N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesR.SetValue(i+1,nr,N);
        if (UVR.Length()<nbv-nnn-1) UVR.Append(UV);
        // internal nodes
        for (j=2; j<nr; j++) {
          double y0 = npl.Value(nbv-j+1);
          double y1 = npr.Value(nbv-j+1);
          gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
          gp_Pnt P = S->Value(UV.X(),UV.Y());
          SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
          meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
          NodesR.SetValue(i+1,j,N);
          if (i==dr) UVtmp.Prepend(UV);
        }
      }
      for (i=1; i<=UVtmp.Length() && UVR.Length()<nbv-nnn-1; i++) {
        UVR.Append(UVtmp.Value(i));
      }
      // create faces
      for (i=1; i<=dr; i++) {
        for (j=1; j<nr; j++) {
          myHelper->AddFace(NodesR.Value(i,j), NodesR.Value(i+1,j),
                            NodesR.Value(i+1,j+1), NodesR.Value(i,j+1));
        }
      }
    }
    else {
      // fill UVR using c2d
      for (i=1; i<npr.Length() && UVR.Length()<nbv-nnn-1; i++) {
        UVR.Append(gp_UV(uv_er[i].u, uv_er[i].v));
      }
    }
    
    // step3: create faces for central domain
    StdMeshers_Array2OfNode NodesC(1,nb,1,nbv);
    // add first line using NodesL
    for (i=1; i<=dl+1; i++)
      NodesC.SetValue(1,i,NodesL(i,1));
    for (i=2; i<=nl; i++)
      NodesC.SetValue(1,dl+i,NodesL(dl+1,i));
    // add last line using NodesR
    for (i=1; i<=dr+1; i++)
      NodesC.SetValue(nb,i,NodesR(i,nr));
    for (i=1; i<nr; i++)
      NodesC.SetValue(nb,dr+i+1,NodesR(dr+1,nr-i));
    // add top nodes (last columns)
    for (i=dl+2; i<nbh-dr; i++) 
      NodesC.SetValue(i-dl,nbv,uv_et[i-1].node);
    // add bottom nodes (first columns)
    for (i=2; i<nb; i++)
      NodesC.SetValue(i,1,uv_eb[i-1].node);

    // create and add needed nodes
    // add linear layers
    for (i=2; i<nb; i++) {
      double x0 = npt.Value(dl+i);
      double x1 = x0;
      for (j=1; j<nnn; j++) {
        double y0 = npl.Value(nbv-nnn+j);
        double y1 = npr.Value(nbv-nnn+j);
        gp_UV UV = calcUV(x0, x1, y0, y1, quad, a0, a1, a2, a3);
        gp_Pnt P = S->Value(UV.X(),UV.Y());
        SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, UV.X(), UV.Y());
        NodesC.SetValue(i,nbv-nnn+j,N);
      }
    }
    // add diagonal layers
    for (i=1; i<nbv-nnn; i++) {
      double du = UVR.Value(i).X() - UVL.Value(i).X();
      double dv = UVR.Value(i).Y() - UVL.Value(i).Y();
      for (j=2; j<nb; j++) {
        double u = UVL.Value(i).X() + du*npb.Value(j);
        double v = UVL.Value(i).Y() + dv*npb.Value(j);
        gp_Pnt P = S->Value(u,v);
        SMDS_MeshNode* N = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(N, geomFaceID, u, v);
        NodesC.SetValue(j,i+1,N);
      }
    }
    // create faces
    for (i=1; i<nb; i++) {
      for (j=1; j<nbv; j++) {
        myHelper->AddFace(NodesC.Value(i,j), NodesC.Value(i+1,j),
                          NodesC.Value(i+1,j+1), NodesC.Value(i,j+1));
      }
    }
  } // end Multiple Reduce implementation
  else { // Simple Reduce (!MultipleReduce)
    //=========================================================
    if (nr == nl) {
      if (nt < nb) {
        // it is a base case => not shift quad
        //shiftQuad(quad,0,true);
      }
      else {
        // we have to shift quad on 2
        shiftQuad(quad,2);
      }
    }
    else {
      if (nl > nr) {
        // we have to shift quad on 1
        shiftQuad(quad,1);
      }
      else {
        // we have to shift quad on 3
        shiftQuad(quad,3);
      }
    }

    nb = quad->side[0].NbPoints();
    nr = quad->side[1].NbPoints();
    nt = quad->side[2].NbPoints();
    nl = quad->side[3].NbPoints();

    // number of rows and columns
    int nrows = nr - 1; // and also == nl - 1
    int ncol_top = nt - 1;
    int ncol_bot = nb - 1;
    int npair_top = ncol_top / 2;
    // maximum number of bottom elements for "linear" simple reduce 4->2
    int max_lin42 = ncol_top + npair_top * 2 * nrows;
    // maximum number of bottom elements for "linear" simple reduce 3->1
    int max_lin31 = ncol_top + ncol_top * 2 * nrows;
    // maximum number of bottom elements for "tree" simple reduce 4->2
    int max_tree42 = 0;
    // number of rows needed to reduce ncol_bot to ncol_top using simple 4->2 "tree"
    int nrows_tree42 = int( log( (double)(ncol_bot / ncol_top) )/log((double)2)  ); // needed to avoid overflow at pow(2) while computing max_tree42
    if (nrows_tree42 < nrows) {
      max_tree42 = npair_top * pow(2.0, nrows + 1);
      if ( ncol_top > npair_top * 2 ) {
        int delta = ncol_bot - max_tree42;
        for (int irow = 1; irow < nrows; irow++) {
          int nfour = delta / 4;
          delta -= nfour * 2;
        }
        if (delta <= (ncol_top - npair_top * 2))
          max_tree42 = ncol_bot;
      }
    }
    // maximum number of bottom elements for "tree" simple reduce 3->1
    //int max_tree31 = ncol_top * pow(3.0, nrows);
    bool is_lin_31 = false;
    bool is_lin_42 = false;
    bool is_tree_31 = false;
    bool is_tree_42 = false;
    int max_lin = max_lin42;
    if (ncol_bot > max_lin42) {
      if (ncol_bot <= max_lin31) {
        is_lin_31 = true;
        max_lin = max_lin31;
      }
    }
    else {
      // if ncol_bot is a 3*n or not 2*n
      if ((ncol_bot/3)*3 == ncol_bot || (ncol_bot/2)*2 != ncol_bot) {
        is_lin_31 = true;
        max_lin = max_lin31;
      }
      else {
        is_lin_42 = true;
      }
    }
    if (ncol_bot > max_lin) { // not "linear"
      is_tree_31 = (ncol_bot > max_tree42);
      if (ncol_bot <= max_tree42) {
        if ((ncol_bot/3)*3 == ncol_bot || (ncol_bot/2)*2 != ncol_bot) {
          is_tree_31 = true;
        }
        else {
          is_tree_42 = true;
        }
      }
    }

    const vector<UVPtStruct>& uv_eb = quad->side[0].GetUVPtStruct(true,0);
    const vector<UVPtStruct>& uv_er = quad->side[1].GetUVPtStruct(false,1);
    const vector<UVPtStruct>& uv_et = quad->side[2].GetUVPtStruct(true,1);
    const vector<UVPtStruct>& uv_el = quad->side[3].GetUVPtStruct(false,0);

    if (uv_eb.size() != nb || uv_er.size() != nr || uv_et.size() != nt || uv_el.size() != nl)
      return error(COMPERR_BAD_INPUT_MESH);

    gp_UV uv[ UV_SIZE ];
    uv[ UV_A0 ].SetCoord( uv_eb.front().u, uv_eb.front().v);
    uv[ UV_A1 ].SetCoord( uv_eb.back().u,  uv_eb.back().v );
    uv[ UV_A2 ].SetCoord( uv_et.back().u,  uv_et.back().v );
    uv[ UV_A3 ].SetCoord( uv_et.front().u, uv_et.front().v);

    vector<UVPtStruct> curr_base = uv_eb, next_base;

    UVPtStruct nullUVPtStruct; nullUVPtStruct.node = 0;

    int curr_base_len = nb;
    int next_base_len = 0;

    if ( true )
    { // ------------------------------------------------------------------
      // New algorithm implemented by request of IPAL22856
      // "2D quadrangle mesher of reduced type works wrong"
      // http://bugtracker.opencascade.com/show_bug.cgi?id=22856

      // the algorithm is following: all reduces are centred in horizontal
      // direction and are distributed among all rows

      if (ncol_bot > max_tree42) {
        is_lin_31 = true;
      }
      else {
        if ((ncol_top/3)*3 == ncol_top ) {
          is_lin_31 = true;
        }
        else {
          is_lin_42 = true;
        }
      }

      const int col_top_size  = is_lin_42 ? 2 : 1;
      const int col_base_size = is_lin_42 ? 4 : 3;

      // Compute nb of "columns" (like in "linear" simple reducing) in all rows

      vector<int> nb_col_by_row;

      int delta_all     = nb - nt;
      int delta_one_col = nrows * 2;
      int nb_col        = delta_all / delta_one_col;
      int remainder     = delta_all - nb_col * delta_one_col;
      if (remainder > 0) {
        nb_col++;
      }
      if ( nb_col * col_top_size >= nt ) // == "tree" reducing situation
      {
        // top row is full (all elements reduced), add "columns" one by one
        // in rows below until all bottom elements are reduced
        nb_col = ( nt - 1 ) / col_top_size;
        nb_col_by_row.resize( nrows, nb_col );
        int nbrows_not_full = nrows - 1;
        int cur_top_size    = nt - 1;
        remainder = delta_all - nb_col * delta_one_col;
        while ( remainder > 0 )
        {
          delta_one_col   = nbrows_not_full * 2;
          int nb_col_add  = remainder / delta_one_col;
          cur_top_size   += 2 * nb_col_by_row[ nbrows_not_full ];
          int nb_col_free = cur_top_size / col_top_size - nb_col_by_row[ nbrows_not_full-1 ];
          if ( nb_col_add > nb_col_free )
            nb_col_add = nb_col_free;
          for ( int irow = 0; irow < nbrows_not_full; ++irow )
            nb_col_by_row[ irow ] += nb_col_add;
          nbrows_not_full --;
          remainder -=  nb_col_add * delta_one_col;
        }
      }
      else // == "linear" reducing situation
      {
        nb_col_by_row.resize( nrows, nb_col );
        if (remainder > 0)
          for ( int irow = remainder / 2; irow < nrows; ++irow )
            nb_col_by_row[ irow ]--;
      }

      // Make elements

      PReduceFunction reduceFunction = & ( is_lin_42 ? reduce42 : reduce31 );

      const int reduce_grp_size = is_lin_42 ? 4 : 3;

      for (i = 1; i < nr; i++) // layer by layer
      {
        nb_col = nb_col_by_row[ i-1 ];
        int nb_next = curr_base_len - nb_col * 2;
        if (nb_next < nt) nb_next = nt;

        const double y = uv_el[ i ].normParam;

        if ( i + 1 == nr ) // top
        {
          next_base = uv_et;
        }
        else
        {
          next_base.clear();
          next_base.resize( nb_next, nullUVPtStruct );
          next_base.front() = uv_el[i];
          next_base.back()  = uv_er[i];

          // compute normalized param u
          double du = 1. / ( nb_next - 1 );
          next_base[0].normParam = 0.;
          for ( j = 1; j < nb_next; ++j )
            next_base[j].normParam = next_base[j-1].normParam + du;
        }
        uv[ UV_L ].SetCoord( next_base.front().u, next_base.front().v );
        uv[ UV_R ].SetCoord( next_base.back().u,  next_base.back().v );

        int free_left = ( curr_base_len - 1 - nb_col * col_base_size ) / 2;
        int free_middle = curr_base_len - 1 - nb_col * col_base_size - 2 * free_left;

        // not reduced left elements
        for (j = 0; j < free_left; j++)
        {
          // f (i + 1, j + 1)
          const SMDS_MeshNode*& Nf = next_base[++next_base_len].node;
          if ( !Nf )
            Nf = makeNode( next_base[ next_base_len ], y, quad, uv, myHelper, S );

          myHelper->AddFace(curr_base[ j ].node,
                            curr_base[ j+1 ].node,
                            Nf,
                            next_base[ next_base_len-1 ].node);
        }

        for (int icol = 1; icol <= nb_col; icol++)
        {
          // add "H"
          reduceFunction( curr_base, next_base, j, next_base_len, quad, uv, y, myHelper, S );

          j += reduce_grp_size;

          // elements in the middle of "columns" added for symmetry
          if ( free_middle > 0 && ( nb_col % 2 == 0 ) && icol == nb_col / 2 )
          {
            for (int imiddle = 1; imiddle <= free_middle; imiddle++) {
              // f (i + 1, j + imiddle)
              const SMDS_MeshNode*& Nf = next_base[++next_base_len].node;
              if ( !Nf )
                Nf = makeNode( next_base[ next_base_len ], y, quad, uv, myHelper, S );

              myHelper->AddFace(curr_base[ j-1+imiddle ].node,
                                curr_base[ j  +imiddle ].node,
                                Nf,
                                next_base[ next_base_len-1 ].node);
            }
            j += free_middle;
          }
        }

        // not reduced right elements
        for (; j < curr_base_len-1; j++) {
          // f (i + 1, j + 1)
          const SMDS_MeshNode*& Nf = next_base[++next_base_len].node;
          if ( !Nf )
            Nf = makeNode( next_base[ next_base_len ], y, quad, uv, myHelper, S );

          myHelper->AddFace(curr_base[ j ].node,
                            curr_base[ j+1 ].node,
                            Nf,
                            next_base[ next_base_len-1 ].node);
        }

        curr_base_len = next_base_len + 1;
        next_base_len = 0;
        curr_base.swap( next_base );
      }

    }
    else if ( is_tree_42 || is_tree_31 )
    {
      // "tree" simple reduce "42": 2->4->8->16->32->...
      //
      //  .-------------------------------.-------------------------------. nr
      //  |    \                          |                          /    |
      //  |         \     .---------------.---------------.     /         |
      //  |               |               |               |               |
      //  .---------------.---------------.---------------.---------------.
      //  | \             |             / | \             |             / |
      //  |     \ .-------.-------. /     |     \ .-------.-------. /     |
      //  |       |       |       |       |       |       |       |       |
      //  .-------.-------.-------.-------.-------.-------.-------.-------. i
      //  |\      |      /|\      |      /|\      |      /|\      |      /|
      //  |  \.---.---./  |  \.---.---./  |  \.---.---./  |  \.---.---./  |
      //  |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
      //  .---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.
      //  |\  |  /|\  |  /|\  |  /|\  |  /|\  |  /|\  |  /|\  |  /|\  |  /|
      //  | .-.-. | .-.-. | .-.-. | .-.-. | .-.-. | .-.-. | .-.-. | .-.-. |
      //  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
      //  .-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-. 1
      //  1                               j                               nb

      // "tree" simple reduce "31": 1->3->9->27->...
      //
      //  .-----------------------------------------------------. nr
      //  |        \                                   /        |
      //  |                 .-----------------.                 |
      //  |                 |                 |                 |
      //  .-----------------.-----------------.-----------------.
      //  |   \         /   |   \         /   |   \         /   |
      //  |     .-----.     |     .-----.     |     .-----.     | i
      //  |     |     |     |     |     |     |     |     |     |
      //  .-----.-----.-----.-----.-----.-----.-----.-----.-----.
      //  |\   /|\   /|\   /|\   /|\   /|\   /|\   /|\   /|\   /|
      //  | .-. | .-. | .-. | .-. | .-. | .-. | .-. | .-. | .-. |
      //  | | | | | | | | | | | | | | | | | | | | | | | | | | | |
      //  .-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-. 1
      //  1                          j                          nb

      PReduceFunction reduceFunction = & ( is_tree_42 ? reduce42 : reduce31 );

      const int reduce_grp_size = is_tree_42 ? 4 : 3;

      for (i = 1; i < nr; i++) // layer by layer
      {
        // to stop reducing, if number of nodes reaches nt
        int delta = curr_base_len - nt;

        // to calculate normalized parameter, we must know number of points in next layer
        int nb_reduce_groups = (curr_base_len - 1) / reduce_grp_size;
        int nb_next = nb_reduce_groups * (reduce_grp_size-2) + (curr_base_len - nb_reduce_groups*reduce_grp_size);
        if (nb_next < nt) nb_next = nt;

        const double y = uv_el[ i ].normParam;

        if ( i + 1 == nr ) // top
        {
          next_base = uv_et;
        }
        else
        {
          next_base.clear();
          next_base.resize( nb_next, nullUVPtStruct );
          next_base.front() = uv_el[i];
          next_base.back()  = uv_er[i];

          // compute normalized param u
          double du = 1. / ( nb_next - 1 );
          next_base[0].normParam = 0.;
          for ( j = 1; j < nb_next; ++j )
            next_base[j].normParam = next_base[j-1].normParam + du;
        }
        uv[ UV_L ].SetCoord( next_base.front().u, next_base.front().v );
        uv[ UV_R ].SetCoord( next_base.back().u,  next_base.back().v );

        for (j = 0; j+reduce_grp_size < curr_base_len && delta > 0; j+=reduce_grp_size, delta-=2)
        {
          reduceFunction( curr_base, next_base, j, next_base_len, quad, uv, y, myHelper, S );
        }

        // not reduced side elements (if any)
        for (; j < curr_base_len-1; j++)
        {
          // f (i + 1, j + 1)
          const SMDS_MeshNode*& Nf = next_base[++next_base_len].node;
          if ( !Nf )
            Nf = makeNode( next_base[ next_base_len ], y, quad, uv, myHelper, S );
          
          myHelper->AddFace(curr_base[ j ].node,
                            curr_base[ j+1 ].node,
                            Nf,
                            next_base[ next_base_len-1 ].node);
        }
        curr_base_len = next_base_len + 1;
        next_base_len = 0;
        curr_base.swap( next_base );
      }
    } // end "tree" simple reduce

    else if ( is_lin_42 || is_lin_31 ) {
      // "linear" simple reduce "31": 2->6->10->14
      //
      //  .-----------------------------.-----------------------------. nr
      //  |     \                 /     |     \                 /     |
      //  |         .---------.         |         .---------.         |
      //  |         |         |         |         |         |         |
      //  .---------.---------.---------.---------.---------.---------.
      //  |        / \       / \        |        / \       / \        |
      //  |       /   .-----.   \       |       /   .-----.   \       | i
      //  |      /    |     |    \      |      /    |     |    \      |
      //  .-----.-----.-----.-----.-----.-----.-----.-----.-----.-----.
      //  |    /     / \   / \     \    |    /     / \   / \     \    |
      //  |   /     /   .-.   \     \   |   /     /   .-.   \     \   |
      //  |  /     /   /   \   \     \  |  /     /   /   \   \     \  |
      //  .--.----.---.-----.---.-----.-.--.----.---.-----.---.-----.-. 1
      //  1                             j                             nb

      // "linear" simple reduce "42": 4->8->12->16
      //
      //  .---------------.---------------.---------------.---------------. nr
      //  | \             |             / | \             |             / |
      //  |     \ .-------.-------. /     |     \ .-------.-------. /     |
      //  |       |       |       |       |       |       |       |       |
      //  .-------.-------.-------.-------.-------.-------.-------.-------.
      //  |      / \      |      / \      |      / \      |      / \      |
      //  |     /   \.----.----./   \     |     /   \.----.----./   \     | i
      //  |     /    |    |    |    \     |     /    |    |    |    \     |
      //  .-----.----.----.----.----.-----.-----.----.----.----.----.-----.
      //  |     /   / \   |  /  \   \     |     /   / \   |  /  \   \     |
      //  |    /   /    .-.-.    \   \    |    /   /    .-.-.    \   \    |
      //  |   /   /    /  |  \    \   \   |   /   /    /  |  \    \   \   |
      //  .---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---. 1
      //  1                               j                               nb

      // nt = 5, nb = 7, nr = 4
      //int delta_all = 2;
      //int delta_one_col = 6;
      //int nb_col = 0;
      //int remainder = 2;
      //if (remainder > 0) nb_col++;
      //nb_col = 1;
      //int free_left = 1;
      //free_left += 2;
      //int free_middle = 4;

      int delta_all = nb - nt;
      int delta_one_col = (nr - 1) * 2;
      int nb_col = delta_all / delta_one_col;
      int remainder = delta_all - nb_col * delta_one_col;
      if (remainder > 0) {
        nb_col++;
      }
      const int col_top_size = is_lin_42 ? 2 : 1;
      int free_left = ((nt - 1) - nb_col * col_top_size) / 2;
      free_left += nr - 2;
      int free_middle = (nr - 2) * 2;
      if (remainder > 0 && nb_col == 1) {
        int nb_rows_short_col = remainder / 2;
        int nb_rows_thrown = (nr - 1) - nb_rows_short_col;
        free_left -= nb_rows_thrown;
      }

      // nt = 5, nb = 17, nr = 4
      //int delta_all = 12;
      //int delta_one_col = 6;
      //int nb_col = 2;
      //int remainder = 0;
      //int free_left = 2;
      //int free_middle = 4;

      PReduceFunction reduceFunction = & ( is_lin_42 ? reduce42 : reduce31 );

      const int reduce_grp_size = is_lin_42 ? 4 : 3;

      for (i = 1; i < nr; i++, free_middle -= 2, free_left -= 1) // layer by layer
      {
        // to calculate normalized parameter, we must know number of points in next layer
        int nb_next = curr_base_len - nb_col * 2;
        if (remainder > 0 && i > remainder / 2)
          // take into account short "column"
          nb_next += 2;
        if (nb_next < nt) nb_next = nt;

        const double y = uv_el[ i ].normParam;

        if ( i + 1 == nr ) // top
        {
          next_base = uv_et;
        }
        else
        {
          next_base.clear();
          next_base.resize( nb_next, nullUVPtStruct );
          next_base.front() = uv_el[i];
          next_base.back()  = uv_er[i];

          // compute normalized param u
          double du = 1. / ( nb_next - 1 );
          next_base[0].normParam = 0.;
          for ( j = 1; j < nb_next; ++j )
            next_base[j].normParam = next_base[j-1].normParam + du;
        }
        uv[ UV_L ].SetCoord( next_base.front().u, next_base.front().v );
        uv[ UV_R ].SetCoord( next_base.back().u,  next_base.back().v );

        // not reduced left elements
        for (j = 0; j < free_left; j++)
        {
          // f (i + 1, j + 1)
          const SMDS_MeshNode*& Nf = next_base[++next_base_len].node;
          if ( !Nf )
            Nf = makeNode( next_base[ next_base_len ], y, quad, uv, myHelper, S );

          myHelper->AddFace(curr_base[ j ].node,
                            curr_base[ j+1 ].node,
                            Nf,
                            next_base[ next_base_len-1 ].node);
        }

        for (int icol = 1; icol <= nb_col; icol++) {

          if (remainder > 0 && icol == nb_col && i > remainder / 2)
            // stop short "column"
            break;

          // add "H"
          reduceFunction( curr_base, next_base, j, next_base_len, quad, uv, y, myHelper, S );

          j += reduce_grp_size;

          // not reduced middle elements
          if (icol < nb_col) {
            if (remainder > 0 && icol == nb_col - 1 && i > remainder / 2)
              // pass middle elements before stopped short "column"
              break;

            int free_add = free_middle;
            if (remainder > 0 && icol == nb_col - 1)
              // next "column" is short
              free_add -= (nr - 1) - (remainder / 2);

            for (int imiddle = 1; imiddle <= free_add; imiddle++) {
              // f (i + 1, j + imiddle)
              const SMDS_MeshNode*& Nf = next_base[++next_base_len].node;
              if ( !Nf )
                Nf = makeNode( next_base[ next_base_len ], y, quad, uv, myHelper, S );

              myHelper->AddFace(curr_base[ j-1+imiddle ].node,
                                curr_base[ j  +imiddle ].node,
                                Nf,
                                next_base[ next_base_len-1 ].node);
            }
            j += free_add;
          }
        }

        // not reduced right elements
        for (; j < curr_base_len-1; j++) {
          // f (i + 1, j + 1)
          const SMDS_MeshNode*& Nf = next_base[++next_base_len].node;
          if ( !Nf )
            Nf = makeNode( next_base[ next_base_len ], y, quad, uv, myHelper, S );

          myHelper->AddFace(curr_base[ j ].node,
                            curr_base[ j+1 ].node,
                            Nf,
                            next_base[ next_base_len-1 ].node);
        }

        curr_base_len = next_base_len + 1;
        next_base_len = 0;
        curr_base.swap( next_base );
      }

    } // end "linear" simple reduce

    else {
      return false;
    }
  } // end Simple Reduce implementation

  bool isOk = true;
  return isOk;
}

//================================================================================
namespace // data for smoothing
{
  struct TSmoothNode;
  // --------------------------------------------------------------------------------
  /*!
   * \brief Structure used to check validity of node position after smoothing.
   *        It holds two nodes connected to a smoothed node and belonging to
   *        one mesh face
   */
  struct TTriangle
  {
    TSmoothNode* _n1;
    TSmoothNode* _n2;
    TTriangle( TSmoothNode* n1=0, TSmoothNode* n2=0 ): _n1(n1), _n2(n2) {}

    inline bool IsForward( gp_UV uv ) const;
  };
  // --------------------------------------------------------------------------------
  /*!
   * \brief Data of a smoothed node
   */
  struct TSmoothNode
  {
    gp_XY  _uv;
    gp_XYZ _xyz;
    vector< TTriangle > _triangles; // if empty, then node is not movable
  };
  // --------------------------------------------------------------------------------
  inline bool TTriangle::IsForward( gp_UV uv ) const
  {
    gp_Vec2d v1( uv, _n1->_uv ), v2( uv, _n2->_uv );
    double d = v1 ^ v2;
    return d > 1e-100;
  }
  //================================================================================
  /*!
   * \brief Returns area of a triangle
   */
  //================================================================================

  double getArea( const gp_UV uv1, const gp_UV uv2, const gp_UV uv3 )
  {
    gp_XY v1 = uv1 - uv2, v2 = uv3 - uv2;
    double a = v2 ^ v1;
    return a;
  }
}

//================================================================================
/*!
 * \brief Set UV of nodes on degenerated VERTEXes in the middle of degenerated EDGE
 *
 * WARNING: this method must be called AFTER retrieving UVPtStruct's from quad
 */
//================================================================================

void StdMeshers_Quadrangle_2D::updateDegenUV(FaceQuadStruct::Ptr quad)
{
  if ( myNeedSmooth )

    // Set UV of nodes on degenerated VERTEXes in the middle of degenerated EDGE
    // --------------------------------------------------------------------------
    for ( unsigned i = 0; i < quad->side.size(); ++i )
    {
      const vector<UVPtStruct>& uvVec = quad->side[i].GetUVPtStruct();

      // find which end of the side is on degenerated shape
      int degenInd = -1;
      if ( myHelper->IsDegenShape( uvVec[0].node->getshapeId() ))
        degenInd = 0;
      else if ( myHelper->IsDegenShape( uvVec.back().node->getshapeId() ))
        degenInd = uvVec.size() - 1;
      else
        continue;

      // find another side sharing the degenerated shape
      bool isPrev = ( degenInd == 0 );
      if ( i >= QUAD_TOP_SIDE )
        isPrev = !isPrev;
      int i2 = ( isPrev ? ( i + 3 ) : ( i + 1 )) % 4;
      const vector<UVPtStruct>& uvVec2 = quad->side[ i2 ].GetUVPtStruct();
      int degenInd2 = -1;
      if (      uvVec[ degenInd ].node == uvVec2.front().node )
        degenInd2 = 0;
      else if ( uvVec[ degenInd ].node == uvVec2.back().node )
        degenInd2 = uvVec2.size() - 1;
      else
        throw SALOME_Exception( LOCALIZED( "Logical error" ));

      // move UV in the middle
      uvPtStruct& uv1 = const_cast<uvPtStruct&>( uvVec [ degenInd  ]);
      uvPtStruct& uv2 = const_cast<uvPtStruct&>( uvVec2[ degenInd2 ]);
      uv1.u = uv2.u = 0.5 * ( uv1.u + uv2.u );
      uv1.v = uv2.v = 0.5 * ( uv1.v + uv2.v );
    }

  else if ( quad->side.size() == 4 /*&& myQuadType == QUAD_STANDARD*/)

    // Set number of nodes on a degenerated side to be same as on an opposite side
    // ----------------------------------------------------------------------------
    for ( size_t i = 0; i < quad->side.size(); ++i )
    {
      StdMeshers_FaceSidePtr degSide = quad->side[i];
      if ( !myHelper->IsDegenShape( degSide->EdgeID(0) ))
        continue;
      StdMeshers_FaceSidePtr oppSide = quad->side[( i+2 ) % quad->side.size() ];
      if ( degSide->NbSegments() == oppSide->NbSegments() )
        continue;

      // make new side data
      const vector<UVPtStruct>& uvVecDegOld = degSide->GetUVPtStruct();
      const SMDS_MeshNode*   n = uvVecDegOld[0].node;
      Handle(Geom2d_Curve) c2d = degSide->Curve2d(0);
      double f = degSide->FirstU(0), l = degSide->LastU(0);
      gp_Pnt2d p1 = uvVecDegOld.front().UV();
      gp_Pnt2d p2 = uvVecDegOld.back().UV();

      quad->side[i] = StdMeshers_FaceSide::New( oppSide.get(), n, &p1, &p2, c2d, f, l );
    }
}

//================================================================================
/*!
 * \brief Perform smoothing of 2D elements on a FACE with ignored degenerated EDGE
 */
//================================================================================

void StdMeshers_Quadrangle_2D::smooth (FaceQuadStruct::Ptr quad)
{
  if ( !myNeedSmooth ) return;

  SMESHDS_Mesh* meshDS = myHelper->GetMeshDS();
  const double     tol = BRep_Tool::Tolerance( quad->face );
  Handle(ShapeAnalysis_Surface) surface = myHelper->GetSurface( quad->face );

  if ( myHelper->HasDegeneratedEdges() && myForcedPnts.empty() )
  {
    // "smooth" by computing node positions using 3D TFI and further projection

    int nbhoriz  = quad->iSize;
    int nbvertic = quad->jSize;

    SMESH_TNodeXYZ a0( quad->UVPt( 0,         0          ).node );
    SMESH_TNodeXYZ a1( quad->UVPt( nbhoriz-1, 0          ).node );
    SMESH_TNodeXYZ a2( quad->UVPt( nbhoriz-1, nbvertic-1 ).node );
    SMESH_TNodeXYZ a3( quad->UVPt( 0,         nbvertic-1 ).node );

    for (int i = 1; i < nbhoriz-1; i++)
    {
      SMESH_TNodeXYZ p0( quad->UVPt( i, 0          ).node );
      SMESH_TNodeXYZ p2( quad->UVPt( i, nbvertic-1 ).node );
      for (int j = 1; j < nbvertic-1; j++)
      {
        SMESH_TNodeXYZ p1( quad->UVPt( nbhoriz-1, j ).node );
        SMESH_TNodeXYZ p3( quad->UVPt( 0,         j ).node );

        UVPtStruct& uvp = quad->UVPt( i, j );

        gp_Pnt    p = myHelper->calcTFI(uvp.x,uvp.y, a0,a1,a2,a3, p0,p1,p2,p3);
        gp_Pnt2d uv = surface->NextValueOfUV( uvp.UV(), p, 10*tol );
        gp_Pnt pnew = surface->Value( uv );

        meshDS->MoveNode( uvp.node, pnew.X(), pnew.Y(), pnew.Z() );
        uvp.u = uv.X();
        uvp.v = uv.Y();
      }
    }
    return;
  }

  // Get nodes to smooth

  typedef map< const SMDS_MeshNode*, TSmoothNode, TIDCompare > TNo2SmooNoMap;
  TNo2SmooNoMap smooNoMap;

  // fixed nodes
  set< const SMDS_MeshNode* > fixedNodes;
  for ( size_t i = 0; i < myForcedPnts.size(); ++i )
  {
    fixedNodes.insert( myForcedPnts[i].node );
    if ( myForcedPnts[i].node->getshapeId() != myHelper->GetSubShapeID() )
    {
      TSmoothNode & sNode = smooNoMap[ myForcedPnts[i].node ];
      sNode._uv  = myForcedPnts[i].uv;
      sNode._xyz = SMESH_TNodeXYZ( myForcedPnts[i].node );
    }
  }
  SMESHDS_SubMesh* fSubMesh = meshDS->MeshElements( quad->face );
  SMDS_NodeIteratorPtr  nIt = fSubMesh->GetNodes();
  while ( nIt->more() ) // loop on nodes bound to a FACE
  {
    const SMDS_MeshNode* node = nIt->next();
    TSmoothNode & sNode = smooNoMap[ node ];
    sNode._uv  = myHelper->GetNodeUV( quad->face, node );
    sNode._xyz = SMESH_TNodeXYZ( node );
    if ( fixedNodes.count( node ))
      continue; // fixed - no triangles

    // set sNode._triangles
    SMDS_ElemIteratorPtr fIt = node->GetInverseElementIterator( SMDSAbs_Face );
    while ( fIt->more() )
    {
      const SMDS_MeshElement* face = fIt->next();
      const int nbN     = face->NbCornerNodes();
      const int nInd    = face->GetNodeIndex( node );
      const int prevInd = myHelper->WrapIndex( nInd - 1, nbN );
      const int nextInd = myHelper->WrapIndex( nInd + 1, nbN );
      const SMDS_MeshNode* prevNode = face->GetNode( prevInd );
      const SMDS_MeshNode* nextNode = face->GetNode( nextInd );
      sNode._triangles.push_back( TTriangle( & smooNoMap[ prevNode ],
                                             & smooNoMap[ nextNode ]));
    }
  }
  // set _uv of smooth nodes on FACE boundary
  set< StdMeshers_FaceSide* > sidesOnEdge;
  list< FaceQuadStruct::Ptr >::iterator q = myQuadList.begin();
  for ( ; q != myQuadList.end() ; ++q )
    for ( size_t i = 0; i < (*q)->side.size(); ++i )
      if ( ! (*q)->side[i].grid->Edge(0).IsNull() &&
           //(*q)->nbNodeOut( i ) == 0 &&
           sidesOnEdge.insert( (*q)->side[i].grid.get() ).second )
      {
        const vector<UVPtStruct>& uvVec = (*q)->side[i].grid->GetUVPtStruct();
        for ( unsigned j = 0; j < uvVec.size(); ++j )
        {
          TSmoothNode & sNode = smooNoMap[ uvVec[j].node ];
          sNode._uv  = uvVec[j].UV();
          sNode._xyz = SMESH_TNodeXYZ( uvVec[j].node );
        }
      }

  // define refernce orientation in 2D
  TNo2SmooNoMap::iterator n2sn = smooNoMap.begin();
  for ( ; n2sn != smooNoMap.end(); ++n2sn )
    if ( !n2sn->second._triangles.empty() )
      break;
  if ( n2sn == smooNoMap.end() ) return;
  const TSmoothNode & sampleNode = n2sn->second;
  const bool refForward = ( sampleNode._triangles[0].IsForward( sampleNode._uv ));

  // Smoothing

  for ( int iLoop = 0; iLoop < 5; ++iLoop )
  {
    for ( n2sn = smooNoMap.begin(); n2sn != smooNoMap.end(); ++n2sn )
    {
      TSmoothNode& sNode = n2sn->second;
      if ( sNode._triangles.empty() )
        continue; // not movable node

      gp_XY newUV;
      bool isValid = false;
      bool use3D   = ( iLoop > 2 ); // 3 loops in 2D and 2, in 3D

      if ( use3D )
      {
        // compute a new XYZ
        gp_XYZ newXYZ (0,0,0);
        for ( size_t i = 0; i < sNode._triangles.size(); ++i )
          newXYZ += sNode._triangles[i]._n1->_xyz;
        newXYZ /= sNode._triangles.size();

        // compute a new UV by projection
        newUV = surface->NextValueOfUV( sNode._uv, newXYZ, 10*tol ).XY();

        // check validity of the newUV
        for ( size_t i = 0; i < sNode._triangles.size() && isValid; ++i )
          isValid = ( sNode._triangles[i].IsForward( newUV ) == refForward );
      }
      if ( !isValid )
      {
        // compute a new UV by averaging
        newUV.SetCoord(0.,0.);
        for ( unsigned i = 0; i < sNode._triangles.size(); ++i )
          newUV += sNode._triangles[i]._n1->_uv;
        newUV /= sNode._triangles.size();

        // check validity of the newUV
        isValid = true;
        for ( unsigned i = 0; i < sNode._triangles.size() && isValid; ++i )
          isValid = ( sNode._triangles[i].IsForward( newUV ) == refForward );
      }
      if ( isValid )
      {
        sNode._uv = newUV;
        sNode._xyz = surface->Value( newUV ).XYZ();
      }
    }
  }

  // Set new XYZ to the smoothed nodes

  for ( n2sn = smooNoMap.begin(); n2sn != smooNoMap.end(); ++n2sn )
  {
    TSmoothNode& sNode = n2sn->second;
    if ( sNode._triangles.empty() )
      continue; // not movable node

    SMDS_MeshNode* node = const_cast< SMDS_MeshNode*>( n2sn->first );
    gp_Pnt xyz = surface->Value( sNode._uv );
    meshDS->MoveNode( node, xyz.X(), xyz.Y(), xyz.Z() );

    // store the new UV
    node->SetPosition( SMDS_PositionPtr( new SMDS_FacePosition( sNode._uv.X(), sNode._uv.Y() )));
  }

  // Move medium nodes in quadratic mesh
  if ( _quadraticMesh )
  {
    const TLinkNodeMap& links = myHelper->GetTLinkNodeMap();
    TLinkNodeMap::const_iterator linkIt = links.begin();
    for ( ; linkIt != links.end(); ++linkIt )
    {
      const SMESH_TLink& link = linkIt->first;
      SMDS_MeshNode*     node = const_cast< SMDS_MeshNode*>( linkIt->second );

      if ( node->getshapeId() != myHelper->GetSubShapeID() )
        continue; // medium node is on EDGE or VERTEX

      gp_XYZ pm = 0.5 * ( SMESH_TNodeXYZ( link.node1() ) + SMESH_TNodeXYZ( link.node2() ));
      gp_XY uvm = myHelper->GetNodeUV( quad->face, node );

      gp_Pnt2d uv = surface->NextValueOfUV( uvm, pm, 10*tol );
      gp_Pnt  xyz = surface->Value( uv );

      node->SetPosition( SMDS_PositionPtr( new SMDS_FacePosition( uv.X(), uv.Y() )));
      meshDS->MoveNode( node, xyz.X(), xyz.Y(), xyz.Z() );
    }
  }
}

//================================================================================
/*!
 * \brief Checks validity of generated faces
 */
//================================================================================

bool StdMeshers_Quadrangle_2D::check()
{
  const bool isOK = true;
  if ( !myCheckOri || myQuadList.empty() || !myQuadList.front() || !myHelper )
    return isOK;

  TopoDS_Face      geomFace = TopoDS::Face( myHelper->GetSubShape() );
  SMESHDS_Mesh*    meshDS   = myHelper->GetMeshDS();
  SMESHDS_SubMesh* fSubMesh = meshDS->MeshElements( geomFace );
  bool toCheckUV;
  if ( geomFace.Orientation() >= TopAbs_INTERNAL ) geomFace.Orientation( TopAbs_FORWARD );

  // Get a reference orientation sign

  double okSign;
  {
    TError err;
    TSideVector wireVec =
      StdMeshers_FaceSide::GetFaceWires( geomFace, *myHelper->GetMesh(), true, err );
    StdMeshers_FaceSidePtr wire = wireVec[0];

    // find a right angle VERTEX
    int iVertex;
    double maxAngle = -1e100;
    for ( int i = 0; i < wire->NbEdges(); ++i )
    {
      int iPrev = myHelper->WrapIndex( i-1, wire->NbEdges() );
      const TopoDS_Edge& e1 = wire->Edge( iPrev );
      const TopoDS_Edge& e2 = wire->Edge( i );
      double angle = myHelper->GetAngle( e1, e2, geomFace, wire->FirstVertex( i ));
      if (( maxAngle < angle ) &&
          ( 5.* M_PI/180 < angle && angle < 175.* M_PI/180  ))
      {
        maxAngle = angle;
        iVertex = i;
      }
    }
    if ( maxAngle < -2*M_PI ) return isOK;

    // get a sign of 2D area of a corner face

    int iPrev = myHelper->WrapIndex( iVertex-1, wire->NbEdges() );
    const TopoDS_Edge& e1 = wire->Edge( iPrev );
    const TopoDS_Edge& e2 = wire->Edge( iVertex );

    gp_Vec2d v1, v2; gp_Pnt2d p;
    double u[2];
    {
      bool rev = ( e1.Orientation() == TopAbs_REVERSED );
      Handle(Geom2d_Curve) c = BRep_Tool::CurveOnSurface( e1, geomFace, u[0], u[1] );
      c->D1( u[ !rev ], p, v1 );
      if ( !rev )
        v1.Reverse();
    }
    {
      bool rev = ( e2.Orientation() == TopAbs_REVERSED );
      Handle(Geom2d_Curve) c = BRep_Tool::CurveOnSurface( e2, geomFace, u[0], u[1] );
      c->D1( u[ rev ], p, v2 );
      if ( rev )
        v2.Reverse();
    }

    okSign = v2 ^ v1;

    if ( maxAngle < 0 )
      okSign *= -1;
  }

  // Look for incorrectly oriented faces

  std::list<const SMDS_MeshElement*> badFaces;

  const SMDS_MeshNode* nn [ 8 ]; // 8 is just for safety
  gp_UV                uv [ 8 ];
  SMDS_ElemIteratorPtr fIt = fSubMesh->GetElements();
  while ( fIt->more() ) // loop on faces bound to a FACE
  {
    const SMDS_MeshElement* f = fIt->next();

    const int nbN = f->NbCornerNodes();
    for ( int i = 0; i < nbN; ++i )
      nn[ i ] = f->GetNode( i );

    const SMDS_MeshNode* nInFace = 0;
    if ( myHelper->HasSeam() )
      for ( int i = 0; i < nbN && !nInFace; ++i )
        if ( !myHelper->IsSeamShape( nn[i]->getshapeId() ))
          nInFace = nn[i];

    toCheckUV = true;
    for ( int i = 0; i < nbN; ++i )
      uv[ i ] = myHelper->GetNodeUV( geomFace, nn[i], nInFace, &toCheckUV );

    switch ( nbN ) {
    case 4:
    {
      double sign1 = getArea( uv[0], uv[1], uv[2] );
      double sign2 = getArea( uv[0], uv[2], uv[3] );
      if ( sign1 * sign2 < 0 )
      {
        sign2 = getArea( uv[1], uv[2], uv[3] );
        sign1 = getArea( uv[1], uv[3], uv[0] );
        if ( sign1 * sign2 < 0 )
          continue; // this should not happen
      }
      if ( sign1 * okSign < 0 )
        badFaces.push_back ( f );
      break;
    }
    case 3:
    {
      double sign = getArea( uv[0], uv[1], uv[2] );
      if ( sign * okSign < 0 )
        badFaces.push_back ( f );
      break;
    }
    default:;
    }
  }

  if ( !badFaces.empty() )
  {
    SMESH_subMesh* fSM = myHelper->GetMesh()->GetSubMesh( geomFace );
    SMESH_ComputeErrorPtr& err = fSM->GetComputeError();
    err.reset ( new SMESH_ComputeError( COMPERR_ALGO_FAILED,
                                        "Inverted elements generated"));
    err->myBadElements.swap( badFaces );

    return !isOK;
  }

  return isOK;
}

/*//================================================================================
/*!
 * \brief Finds vertices at the most sharp face corners
 *  \param [in] theFace - the FACE
 *  \param [in,out] theWire - the ordered edges of the face. It can be modified to
 *         have the first VERTEX of the first EDGE in \a vertices
 *  \param [out] theVertices - the found corner vertices in the order corresponding to
 *         the order of EDGEs in \a theWire
 *  \param [out] theNbDegenEdges - nb of degenerated EDGEs in theFace
 *  \param [in] theConsiderMesh - if \c true, only meshed VERTEXes are considered
 *         as possible corners
 *  \return int - number of quad sides found: 0, 3 or 4
 */
//================================================================================

int StdMeshers_Quadrangle_2D::getCorners(const TopoDS_Face&          theFace,
                                         SMESH_Mesh &                theMesh,
                                         std::list<TopoDS_Edge>&     theWire,
                                         std::vector<TopoDS_Vertex>& theVertices,
                                         int &                       theNbDegenEdges,
                                         const bool                  theConsiderMesh)
{
  theNbDegenEdges = 0;

  SMESH_MesherHelper helper( theMesh );
  StdMeshers_FaceSide faceSide( theFace, theWire, &theMesh, /*isFwd=*/true, /*skipMedium=*/true);

  // sort theVertices by angle
  multimap<double, TopoDS_Vertex> vertexByAngle;
  TopTools_DataMapOfShapeReal     angleByVertex;
  TopoDS_Edge prevE = theWire.back();
  if ( SMESH_Algo::isDegenerated( prevE ))
  {
    list<TopoDS_Edge>::reverse_iterator edge = ++theWire.rbegin();
    while ( SMESH_Algo::isDegenerated( *edge ))
      ++edge;
    if ( edge == theWire.rend() )
      return false;
    prevE = *edge;
  }
  list<TopoDS_Edge>::iterator edge = theWire.begin();
  for ( int iE = 0; edge != theWire.end(); ++edge, ++iE )
  {
    if ( SMESH_Algo::isDegenerated( *edge ))
    {
      ++theNbDegenEdges;
      continue;
    }
    if ( !theConsiderMesh || faceSide.VertexNode( iE ))
    {
      TopoDS_Vertex v = helper.IthVertex( 0, *edge );
      double    angle = helper.GetAngle( prevE, *edge, theFace, v );
      vertexByAngle.insert( make_pair( angle, v ));
      angleByVertex.Bind( v, angle );
    }
    prevE = *edge;
  }

  // find out required nb of corners (3 or 4)
  int nbCorners = 4;
  TopoDS_Shape triaVertex = helper.GetMeshDS()->IndexToShape( myTriaVertexID );
  if ( !triaVertex.IsNull() &&
       triaVertex.ShapeType() == TopAbs_VERTEX &&
       helper.IsSubShape( triaVertex, theFace ) &&
       ( vertexByAngle.size() != 4 || vertexByAngle.begin()->first < 5 * M_PI/180. ))
    nbCorners = 3;
  else
    triaVertex.Nullify();

  // check nb of available corners
  if ( faceSide.NbEdges() < nbCorners )
    return error(COMPERR_BAD_SHAPE,
                 TComm("Face must have 4 sides but not ") << faceSide.NbEdges() );

  if ( theConsiderMesh )
  {
    const int nbSegments = Max( faceSide.NbPoints()-1, faceSide.NbSegments() );
    if ( nbSegments < nbCorners )
      return error(COMPERR_BAD_INPUT_MESH, TComm("Too few boundary nodes: ") << nbSegments);
  }

  if ( nbCorners == 3 )
  {
    if ( vertexByAngle.size() < 3 )
      return error(COMPERR_BAD_SHAPE,
                   TComm("Face must have 3 sides but not ") << vertexByAngle.size() );
  }
  else
  {
    if ( vertexByAngle.size() == 3 && theNbDegenEdges == 0 )
    {
      if ( myTriaVertexID < 1 )
        return error(COMPERR_BAD_PARMETERS,
                     "No Base vertex provided for a trilateral geometrical face");
        
      TComm comment("Invalid Base vertex: ");
      comment << myTriaVertexID << " its ID is not among [ ";
      multimap<double, TopoDS_Vertex>::iterator a2v = vertexByAngle.begin();
      comment << helper.GetMeshDS()->ShapeToIndex( a2v->second ) << ", "; a2v++;
      comment << helper.GetMeshDS()->ShapeToIndex( a2v->second ) << ", "; a2v++;
      comment << helper.GetMeshDS()->ShapeToIndex( a2v->second ) << " ]";
      return error(COMPERR_BAD_PARMETERS, comment );
    }
    if ( vertexByAngle.size() + ( theNbDegenEdges > 0 ) < 4 &&
         vertexByAngle.size() + theNbDegenEdges != 4 )
      return error(COMPERR_BAD_SHAPE,
                   TComm("Face must have 4 sides but not ") << vertexByAngle.size() );
  }

  // put all corner vertices in a map
  TopTools_MapOfShape vMap;
  if ( nbCorners == 3 )
    vMap.Add( triaVertex );
  multimap<double, TopoDS_Vertex>::reverse_iterator a2v = vertexByAngle.rbegin();
  for ( int iC = 0; a2v != vertexByAngle.rend() && iC < nbCorners; ++a2v, ++iC )
    vMap.Add( (*a2v).second );

  // check if there are possible variations in choosing corners
  bool haveVariants = false;
  if ( vertexByAngle.size() > nbCorners )
  {
    double lostAngle = a2v->first;
    double lastAngle = ( --a2v, a2v->first );
    haveVariants  = ( lostAngle * 1.1 >= lastAngle );
  }

  const double angleTol = 5.* M_PI/180;
  myCheckOri = ( vertexByAngle.size() > nbCorners ||
                 vertexByAngle.begin()->first < angleTol );

  // make theWire begin from a corner vertex or triaVertex
  if ( nbCorners == 3 )
    while ( !triaVertex.IsSame( ( helper.IthVertex( 0, theWire.front() ))) ||
            SMESH_Algo::isDegenerated( theWire.front() ))
      theWire.splice( theWire.end(), theWire, theWire.begin() );
  else
    while ( !vMap.Contains( helper.IthVertex( 0, theWire.front() )) ||
            SMESH_Algo::isDegenerated( theWire.front() ))
      theWire.splice( theWire.end(), theWire, theWire.begin() );

  // fill the result vector and prepare for its refinement
  theVertices.clear();
  vector< double >      angles;
  vector< TopoDS_Edge > edgeVec;
  vector< int >         cornerInd, nbSeg;
  int nbSegTot = 0;
  angles .reserve( vertexByAngle.size() );
  edgeVec.reserve( vertexByAngle.size() );
  nbSeg  .reserve( vertexByAngle.size() );
  cornerInd.reserve( nbCorners );
  for ( edge = theWire.begin(); edge != theWire.end(); ++edge )
  {
    if ( SMESH_Algo::isDegenerated( *edge ))
      continue;
    TopoDS_Vertex v = helper.IthVertex( 0, *edge );
    bool   isCorner = vMap.Contains( v );
    if ( isCorner )
    {
      theVertices.push_back( v );
      cornerInd.push_back( angles.size() );
    }
    angles .push_back( angleByVertex.IsBound( v ) ? angleByVertex( v ) : -M_PI );
    edgeVec.push_back( *edge );
    if ( theConsiderMesh && haveVariants )
    {
      if ( SMESHDS_SubMesh* sm = helper.GetMeshDS()->MeshElements( *edge ))
        nbSeg.push_back( sm->NbNodes() + 1 );
      else
        nbSeg.push_back( 0 );
      nbSegTot += nbSeg.back();
    }
  }

  // refine the result vector - make sides equal by length if
  // there are several equal angles
  if ( haveVariants )
  {
    if ( nbCorners == 3 )
      angles[0] = 2 * M_PI; // not to move the base triangle VERTEX

    // here we refer to VERTEX'es and EDGEs by indices in angles and edgeVec vectors
    typedef int TGeoIndex;

    // for each vertex find a vertex till which there are nbSegHalf segments
    const int nbSegHalf = ( nbSegTot % 2 || nbCorners == 3 ) ? 0 : nbSegTot / 2;
    vector< TGeoIndex > halfDivider( angles.size(), -1 );
    int nbHalfDividers = 0;
    if ( nbSegHalf )
    {
      // get min angle of corners
      double minAngle = 10.;
      for ( size_t iC = 0; iC < cornerInd.size(); ++iC )
        minAngle = Min( minAngle, angles[ cornerInd[ iC ]]);

      // find halfDivider's
      for ( TGeoIndex iV1 = 0; iV1 < TGeoIndex( angles.size() ); ++iV1 )
      {
        int nbSegs = 0;
        TGeoIndex iV2 = iV1;
        do {
          nbSegs += nbSeg[ iV2 ];
          iV2 = helper.WrapIndex( iV2 + 1, nbSeg.size() );
        } while ( nbSegs < nbSegHalf );

        if ( nbSegs == nbSegHalf &&
             angles[ iV1 ] + angleTol >= minAngle &&
             angles[ iV2 ] + angleTol >= minAngle )
        {
          halfDivider[ iV1 ] = iV2;
          ++nbHalfDividers;
        }
      }
    }

    set< TGeoIndex > refinedCorners, treatedCorners;
    for ( size_t iC = 0; iC < cornerInd.size(); ++iC )
    {
      TGeoIndex iV = cornerInd[iC];
      if ( !treatedCorners.insert( iV ).second )
        continue;
      list< TGeoIndex > equVerts; // inds of vertices that can become corners
      equVerts.push_back( iV );
      int nbC[2] = { 0, 0 };
      // find equal angles backward and forward from the iV-th corner vertex
      for ( int isFwd = 0; isFwd < 2; ++isFwd )
      {
        int           dV = isFwd ? +1 : -1;
        int       iCNext = helper.WrapIndex( iC + dV, cornerInd.size() );
        TGeoIndex iVNext = helper.WrapIndex( iV + dV, angles.size() );
        while ( iVNext != iV )
        {
          bool equal = Abs( angles[iV] - angles[iVNext] ) < angleTol;
          if ( equal )
            equVerts.insert( isFwd ? equVerts.end() : equVerts.begin(), iVNext );
          if ( iVNext == cornerInd[ iCNext ])
          {
            if ( !equal )
            {
              if ( angles[iV] < angles[iVNext] )
                refinedCorners.insert( iVNext );
              break;
            }
            nbC[ isFwd ]++;
            treatedCorners.insert( cornerInd[ iCNext ] );
            iCNext = helper.WrapIndex( iCNext + dV, cornerInd.size() );
          }
          iVNext = helper.WrapIndex( iVNext + dV, angles.size() );
        }
        if ( iVNext == iV )
          break; // all angles equal
      }

      const bool allCornersSame = ( nbC[0] == 3 );
      if ( allCornersSame && nbHalfDividers > 0 )
      {
        // select two halfDivider's as corners
        TGeoIndex hd1, hd2 = -1;
        int iC2;
        for ( iC2 = 0; iC2 < cornerInd.size() && hd2 < 0; ++iC2 )
        {
          hd1 = cornerInd[ iC2 ];
          hd2 = halfDivider[ hd1 ];
          if ( std::find( equVerts.begin(), equVerts.end(), hd2 ) == equVerts.end() )
            hd2 = -1; // hd2-th vertex can't become a corner
          else
            break;
        }
        if ( hd2 >= 0 )
        {
          angles[ hd1 ] = 2 * M_PI; // make hd1-th vertex no more "equal"
          angles[ hd2 ] = 2 * M_PI;
          refinedCorners.insert( hd1 );
          refinedCorners.insert( hd2 );
          treatedCorners = refinedCorners;
          // update cornerInd
          equVerts.push_front( equVerts.back() );
          equVerts.push_back( equVerts.front() );
          list< TGeoIndex >::iterator hdPos =
            std::find( equVerts.begin(), equVerts.end(), hd2 );
          if ( hdPos == equVerts.end() ) break;
          cornerInd[ helper.WrapIndex( iC2 + 0, cornerInd.size()) ] = hd1;
          cornerInd[ helper.WrapIndex( iC2 + 1, cornerInd.size()) ] = *( --hdPos );
          cornerInd[ helper.WrapIndex( iC2 + 2, cornerInd.size()) ] = hd2;
          cornerInd[ helper.WrapIndex( iC2 + 3, cornerInd.size()) ] = *( ++hdPos, ++hdPos );

          theVertices[ 0 ] = helper.IthVertex( 0, edgeVec[ cornerInd[0] ]);
          theVertices[ 1 ] = helper.IthVertex( 0, edgeVec[ cornerInd[1] ]);
          theVertices[ 2 ] = helper.IthVertex( 0, edgeVec[ cornerInd[2] ]);
          theVertices[ 3 ] = helper.IthVertex( 0, edgeVec[ cornerInd[3] ]);
          iC = -1;
          continue;
        }
      }

      // move corners to make sides equal by length
      int nbEqualV  = equVerts.size();
      int nbExcessV = nbEqualV - ( 1 + nbC[0] + nbC[1] );
      if ( nbExcessV > 0 ) // there is nbExcessV vertices that can become corners
      {
        // calculate normalized length of each "side" enclosed between neighbor equVerts
        vector< double > accuLength;
        double totalLen = 0;
        vector< TGeoIndex > evVec( equVerts.begin(), equVerts.end() );
        int          iEV = 0;
        TGeoIndex    iE = cornerInd[ helper.WrapIndex( iC - nbC[0] - 1, cornerInd.size() )];
        TGeoIndex iEEnd = cornerInd[ helper.WrapIndex( iC + nbC[1] + 1, cornerInd.size() )];
        while ( accuLength.size() < nbEqualV + int( !allCornersSame ) )
        {
          // accumulate length of edges before iEV-th equal vertex
          accuLength.push_back( totalLen );
          do {
            accuLength.back() += SMESH_Algo::EdgeLength( edgeVec[ iE ]);
            iE = helper.WrapIndex( iE + 1, edgeVec.size());
            if ( iEV < evVec.size() && iE == evVec[ iEV ] ) {
              iEV++;
              break; // equal vertex reached
            }
          }
          while( iE != iEEnd );
          totalLen = accuLength.back();
        }
        accuLength.resize( equVerts.size() );
        for ( size_t iS = 0; iS < accuLength.size(); ++iS )
          accuLength[ iS ] /= totalLen;

        // find equVerts most close to the ideal sub-division of all sides
        int iBestEV = 0;
        int iCorner = helper.WrapIndex( iC - nbC[0], cornerInd.size() );
        int nbSides = Min( nbCorners, 2 + nbC[0] + nbC[1] );
        for ( int iS = 1; iS < nbSides; ++iS, ++iBestEV )
        {
          double idealLen = iS / double( nbSides );
          double d, bestDist = 2.;
          for ( iEV = iBestEV; iEV < accuLength.size(); ++iEV )
          {
            d = Abs( idealLen - accuLength[ iEV ]);

            // take into account presence of a coresponding halfDivider
            const double cornerWgt = 0.5  / nbSides;
            const double vertexWgt = 0.25 / nbSides;
            TGeoIndex hd = halfDivider[ evVec[ iEV ]];
            if ( hd < 0 )
              d += vertexWgt;
            else if( refinedCorners.count( hd ))
              d -= cornerWgt;
            else
              d -= vertexWgt;

            // choose vertex with the best d
            if ( d < bestDist )
            {
              bestDist = d;
              iBestEV  = iEV;
            }
          }
          if ( iBestEV > iS-1 + nbExcessV )
            iBestEV = iS-1 + nbExcessV;
          theVertices[ iCorner ] = helper.IthVertex( 0, edgeVec[ evVec[ iBestEV ]]);
          refinedCorners.insert( evVec[ iBestEV ]);
          iCorner = helper.WrapIndex( iCorner + 1, cornerInd.size() );
        }

      } // if ( nbExcessV > 0 )
      else
      {
        refinedCorners.insert( cornerInd[ iC ]);
      }
    } // loop on cornerInd

    // make theWire begin from the cornerInd[0]-th EDGE
    while ( !theWire.front().IsSame( edgeVec[ cornerInd[0] ]))
      theWire.splice( theWire.begin(), theWire, --theWire.end() );

  } // if ( haveVariants )

  return nbCorners;
}

//================================================================================
/*!
 * \brief Constructor of a side of quad
 */
//================================================================================

FaceQuadStruct::Side::Side(StdMeshers_FaceSidePtr theGrid)
  : grid(theGrid), nbNodeOut(0), from(0), to(theGrid ? theGrid->NbPoints() : 0 ), di(1)
{
}

//=============================================================================
/*!
 * \brief Constructor of a quad
 */
//=============================================================================

FaceQuadStruct::FaceQuadStruct(const TopoDS_Face& F, const std::string& theName)
  : face( F ), name( theName )
{
  side.reserve(4);
}

//================================================================================
/*!
 * \brief Fills myForcedPnts
 */
//================================================================================

bool StdMeshers_Quadrangle_2D::getEnforcedUV()
{
  myForcedPnts.clear();
  if ( !myParams ) return true; // missing hypothesis

  std::vector< TopoDS_Shape > shapes;
  std::vector< gp_Pnt >       points;
  myParams->GetEnforcedNodes( shapes, points );

  TopTools_IndexedMapOfShape vMap;
  for ( size_t i = 0; i < shapes.size(); ++i )
    if ( !shapes[i].IsNull() )
      TopExp::MapShapes( shapes[i], TopAbs_VERTEX, vMap );

  size_t nbPoints = points.size();
  for ( int i = 1; i <= vMap.Extent(); ++i )
    points.push_back( BRep_Tool::Pnt( TopoDS::Vertex( vMap( i ))));

  // find out if all points must be in the FACE, which is so if
  // myParams is a local hypothesis on the FACE being meshed
  bool isStrictCheck = false;
  {
    SMESH_HypoFilter paramFilter( SMESH_HypoFilter::Is( myParams ));
    TopoDS_Shape assignedTo;
    if ( myHelper->GetMesh()->GetHypothesis( myHelper->GetSubShape(),
                                             paramFilter,
                                             /*ancestors=*/true,
                                             &assignedTo ))
      isStrictCheck = ( assignedTo.IsSame( myHelper->GetSubShape() ));
  }

  multimap< double, ForcedPoint > sortedFP; // sort points by distance from EDGEs

  Standard_Real u1,u2,v1,v2;
  const TopoDS_Face&   face = TopoDS::Face( myHelper->GetSubShape() );
  const double          tol = BRep_Tool::Tolerance( face );
  Handle(ShapeAnalysis_Surface) project = myHelper->GetSurface( face );
  project->Bounds( u1,u2,v1,v2 );
  Bnd_Box bbox;
  BRepBndLib::Add( face, bbox );
  double farTol = 0.01 * sqrt( bbox.SquareExtent() );

  // get internal VERTEXes of the FACE to use them instead of equal points
  typedef map< pair< double, double >, TopoDS_Vertex > TUV2VMap;
  TUV2VMap uv2intV;
  for ( TopExp_Explorer vExp( face, TopAbs_VERTEX, TopAbs_EDGE ); vExp.More(); vExp.Next() )
  {
    TopoDS_Vertex v = TopoDS::Vertex( vExp.Current() );
    gp_Pnt2d     uv = project->ValueOfUV( BRep_Tool::Pnt( v ), tol );
    uv2intV.insert( make_pair( make_pair( uv.X(), uv.Y() ), v ));
  }

  for ( size_t iP = 0; iP < points.size(); ++iP )
  {
    gp_Pnt2d uv = project->ValueOfUV( points[ iP ], tol );
    if ( project->Gap() > farTol )
    {
      if ( isStrictCheck && iP < nbPoints )
        return error
          (COMPERR_BAD_PARMETERS, TComm("An enforced point is too far from the face, dist = ")
           << points[ iP ].Distance( project->Value( uv )) << " - ("
           << points[ iP ].X() << ", "<< points[ iP ].Y() << ", "<< points[ iP ].Z() << " )");
      continue;
    }
    BRepClass_FaceClassifier clsf ( face, uv, tol );
    switch ( clsf.State() ) {
    case TopAbs_IN:
    {
      double edgeDist = ( Min( Abs( uv.X() - u1 ), Abs( uv.X() - u2 )) +
                          Min( Abs( uv.Y() - v1 ), Abs( uv.Y() - v2 )));
      ForcedPoint fp;
      fp.uv  = uv.XY();
      fp.xyz = points[ iP ].XYZ();
      if ( iP >= nbPoints )
        fp.vertex = TopoDS::Vertex( vMap( iP - nbPoints + 1 ));

      TUV2VMap::iterator uv2v = uv2intV.lower_bound( make_pair( uv.X()-tol, uv.Y()-tol ));
      for ( ; uv2v != uv2intV.end() && uv2v->first.first <= uv.X()+tol;  ++uv2v )
        if ( uv.SquareDistance( gp_Pnt2d( uv2v->first.first, uv2v->first.second )) < tol*tol )
        {
          fp.vertex = uv2v->second;
          break;
        }

      fp.node = 0;
      if ( myHelper->IsSubShape( fp.vertex, myHelper->GetMesh() ))
      {
        SMESH_subMesh* sm = myHelper->GetMesh()->GetSubMesh( fp.vertex );
        sm->ComputeStateEngine( SMESH_subMesh::COMPUTE );
        fp.node = SMESH_Algo::VertexNode( fp.vertex, myHelper->GetMeshDS() );
      }
      else
      {
        fp.node = myHelper->AddNode( fp.xyz.X(), fp.xyz.Y(), fp.xyz.Z(),
                                     0, fp.uv.X(), fp.uv.Y() );
      }
      sortedFP.insert( make_pair( edgeDist, fp ));
      break;
    }
    case TopAbs_OUT:
    {
      if ( isStrictCheck && iP < nbPoints )
        return error
          (COMPERR_BAD_PARMETERS, TComm("An enforced point is out of the face boundary - ")
           << points[ iP ].X() << ", "<< points[ iP ].Y() << ", "<< points[ iP ].Z() << " )");
      break;
    }
    case TopAbs_ON:
    {
      if ( isStrictCheck && iP < nbPoints )
        return error
          (COMPERR_BAD_PARMETERS, TComm("An enforced point is on the face boundary - ")
           << points[ iP ].X() << ", "<< points[ iP ].Y() << ", "<< points[ iP ].Z() << " )");
      break;
    }
    default:
    {
      if ( isStrictCheck && iP < nbPoints )
        return error
          (TComm("Classification of an enforced point ralative to the face boundary failed - ")
           << points[ iP ].X() << ", "<< points[ iP ].Y() << ", "<< points[ iP ].Z() << " )");
    }
    }
  }

  multimap< double, ForcedPoint >::iterator d2uv = sortedFP.begin();
  for ( ; d2uv != sortedFP.end(); ++d2uv )
    myForcedPnts.push_back( (*d2uv).second );

  return true;
}

//================================================================================
/*!
 * \brief Splits quads by adding points of enforced nodes and create nodes on
 *        the sides shared by quads
 */
//================================================================================

bool StdMeshers_Quadrangle_2D::addEnforcedNodes()
{
  // if ( myForcedPnts.empty() )
  //   return true;

  // make a map of quads sharing a side
  map< StdMeshers_FaceSidePtr, vector< FaceQuadStruct::Ptr > > quadsBySide;
  list< FaceQuadStruct::Ptr >::iterator quadIt = myQuadList.begin();
  for ( ; quadIt != myQuadList.end(); ++quadIt )
    for ( size_t iSide = 0; iSide < (*quadIt)->side.size(); ++iSide )
    {
      if ( !setNormalizedGrid( *quadIt ))
        return false;
      quadsBySide[ (*quadIt)->side[iSide] ].push_back( *quadIt );
    }

  SMESH_Mesh*          mesh = myHelper->GetMesh();
  SMESHDS_Mesh*      meshDS = myHelper->GetMeshDS();
  const TopoDS_Face&   face = TopoDS::Face( myHelper->GetSubShape() );
  Handle(Geom_Surface) surf = BRep_Tool::Surface( face );

  for ( size_t iFP = 0; iFP < myForcedPnts.size(); ++iFP )
  {
    bool isNodeEnforced = false;

    // look for a quad enclosing an enforced point
    for ( quadIt = myQuadList.begin(); quadIt != myQuadList.end(); ++quadIt )
    {
      FaceQuadStruct::Ptr quad = *quadIt;
      if ( !setNormalizedGrid( *quadIt ))
        return false;
      int i,j;
      if ( !quad->findCell( myForcedPnts[ iFP ], i, j ))
        continue;

      // a grid cell is found, select a node of the cell to move
      // to the enforced point to and to split the quad at
      multimap< double, pair< int, int > > ijByDist;
      for ( int di = 0; di < 2; ++di )
        for ( int dj = 0; dj < 2; ++dj )
        {
          double dist2 = ( myForcedPnts[ iFP ].uv - quad->UVPt( i+di,j+dj ).UV() ).SquareModulus();
          ijByDist.insert( make_pair( dist2, make_pair( di,dj )));
        }
      // try all nodes starting from the closest one
      set< FaceQuadStruct::Ptr > changedQuads;
      multimap< double, pair< int, int > >::iterator d2ij = ijByDist.begin();
      for ( ; !isNodeEnforced  &&  d2ij != ijByDist.end(); ++d2ij )
      {
        int di = d2ij->second.first;
        int dj = d2ij->second.second;

        // check if a node is at a side
        int iSide = -1;
        if ( dj== 0 && j == 0 )
          iSide = QUAD_BOTTOM_SIDE;
        else if ( dj == 1 && j+2 == quad->jSize )
          iSide = QUAD_TOP_SIDE;
        else if ( di == 0 && i == 0 )
          iSide = QUAD_LEFT_SIDE;
        else if ( di == 1 && i+2 == quad->iSize )
          iSide = QUAD_RIGHT_SIDE;

        if ( iSide > -1 ) // ----- node is at a side
        {
          FaceQuadStruct::Side& side = quad->side[ iSide ];
          // check if this node can be moved
          if ( quadsBySide[ side ].size() < 2 )
            continue; // its a face boundary -> can't move the node

          int quadNodeIndex = ( iSide % 2 ) ? j : i;
          int sideNodeIndex = side.ToSideIndex( quadNodeIndex );
          if ( side.IsForced( sideNodeIndex ))
          {
            // the node is already moved to another enforced point
            isNodeEnforced = quad->isEqual( myForcedPnts[ iFP ], i, j );
            continue;
          }
          // make a node of a side forced
          vector<UVPtStruct>& points = (vector<UVPtStruct>&) side.GetUVPtStruct();
          points[ sideNodeIndex ].u    = myForcedPnts[ iFP ].U();
          points[ sideNodeIndex ].v    = myForcedPnts[ iFP ].V();
          points[ sideNodeIndex ].node = myForcedPnts[ iFP ].node;

          updateSideUV( side, sideNodeIndex, quadsBySide );

          // update adjacent sides
          set< StdMeshers_FaceSidePtr > updatedSides;
          updatedSides.insert( side );
          for ( size_t i = 0; i < side.contacts.size(); ++i )
            if ( side.contacts[i].point == sideNodeIndex )
            {
              const vector< FaceQuadStruct::Ptr >& adjQuads =
                quadsBySide[ *side.contacts[i].other_side ];
              if ( adjQuads.size() > 1 &&
                   updatedSides.insert( * side.contacts[i].other_side ).second )
              {
                updateSideUV( *side.contacts[i].other_side,
                              side.contacts[i].other_point,
                              quadsBySide );
              }
              changedQuads.insert( adjQuads.begin(), adjQuads.end() );
            }
          const vector< FaceQuadStruct::Ptr >& adjQuads = quadsBySide[ side ];
          changedQuads.insert( adjQuads.begin(), adjQuads.end() );

          isNodeEnforced = true;
        }
        else // ------------------ node is inside the quad
        {
          i += di;
          j += dj;
          // make a new side passing through IJ node and split the quad
          int indForced, iNewSide;
          if ( quad->iSize < quad->jSize ) // split vertically
          {
            quad->updateUV( myForcedPnts[ iFP ].uv, i, j, /*isVert=*/true );
            indForced = j;
            iNewSide  = splitQuad( quad, i, 0 );
          }
          else
          {
            quad->updateUV( myForcedPnts[ iFP ].uv, i, j, /*isVert=*/false );
            indForced = i;
            iNewSide  = splitQuad( quad, 0, j );
          }
          FaceQuadStruct::Ptr   newQuad = myQuadList.back();
          FaceQuadStruct::Side& newSide = newQuad->side[ iNewSide ];

          vector<UVPtStruct>& points = (vector<UVPtStruct>&) newSide.GetUVPtStruct();
          points[ indForced ].node = myForcedPnts[ iFP ].node;

          newSide.forced_nodes.insert( indForced );
          quad->side[( iNewSide+2 ) % 4 ].forced_nodes.insert( indForced );

          quadsBySide[ newSide ].push_back( quad );
          quadsBySide[ newQuad->side[0] ].push_back( newQuad );
          quadsBySide[ newQuad->side[1] ].push_back( newQuad );
          quadsBySide[ newQuad->side[2] ].push_back( newQuad );
          quadsBySide[ newQuad->side[3] ].push_back( newQuad );

          isNodeEnforced = true;

        } // end of "node is inside the quad"

      } // loop on nodes of the cell

      // remove out-of-date uv grid of changedQuads
      set< FaceQuadStruct::Ptr >::iterator qIt = changedQuads.begin();
      for ( ; qIt != changedQuads.end(); ++qIt )
        (*qIt)->uv_grid.clear();

      if ( isNodeEnforced )
        break;

    } // loop on quads

    if ( !isNodeEnforced )
    {
      if ( !myForcedPnts[ iFP ].vertex.IsNull() )
        return error(TComm("Unable to move any node to vertex #")
                     <<myHelper->GetMeshDS()->ShapeToIndex( myForcedPnts[ iFP ].vertex ));
      else
        return error(TComm("Unable to move any node to point ( ")
                     << myForcedPnts[iFP].xyz.X() << ", "
                     << myForcedPnts[iFP].xyz.Y() << ", "
                     << myForcedPnts[iFP].xyz.Z() << " )");
    }
    myNeedSmooth = true;

  } // loop on enforced points

  // Compute nodes on all sides, where not yet present

  for ( quadIt = myQuadList.begin(); quadIt != myQuadList.end(); ++quadIt )
  {
    FaceQuadStruct::Ptr quad = *quadIt;
    for ( int iSide = 0; iSide < 4; ++iSide )
    {
      FaceQuadStruct::Side & side = quad->side[ iSide ];
      if ( side.nbNodeOut > 0 )
        continue; // emulated side
      vector< FaceQuadStruct::Ptr >& quadVec = quadsBySide[ side ];
      if ( quadVec.size() <= 1 )
        continue; // outer side

      const vector<UVPtStruct>& points = side.grid->GetUVPtStruct();
      for ( size_t iC = 0; iC < side.contacts.size(); ++iC )
      {
        if ( side.contacts[iC].point <  side.from ||
             side.contacts[iC].point >= side.to )
          continue;
        if ( side.contacts[iC].other_point <  side.contacts[iC].other_side->from ||
             side.contacts[iC].other_point >= side.contacts[iC].other_side->to )
          continue;
        const vector<UVPtStruct>& oGrid = side.contacts[iC].other_side->grid->GetUVPtStruct();
        const UVPtStruct&         uvPt  = points[ side.contacts[iC].point ];
        if ( side.contacts[iC].other_point >= oGrid .size() ||
             side.contacts[iC].point       >= points.size() )
          throw SALOME_Exception( "StdMeshers_Quadrangle_2D::addEnforcedNodes(): wrong contact" );
        if ( oGrid[ side.contacts[iC].other_point ].node )
          (( UVPtStruct& ) uvPt).node = oGrid[ side.contacts[iC].other_point ].node;
      }

      bool missedNodesOnSide = false;
      for ( size_t iP = 0; iP < points.size(); ++iP )
        if ( !points[ iP ].node )
        {
          UVPtStruct& uvPnt = ( UVPtStruct& ) points[ iP ];
          gp_Pnt          P = surf->Value( uvPnt.u, uvPnt.v );
          uvPnt.node = myHelper->AddNode(P.X(), P.Y(), P.Z(), 0, uvPnt.u, uvPnt.v );
          missedNodesOnSide = true;
        }
      if ( missedNodesOnSide )
      {
        // clear uv_grid where nodes are missing
        for ( size_t iQ = 0; iQ < quadVec.size(); ++iQ )
          quadVec[ iQ ]->uv_grid.clear();
      }
    }
  }

  return true;
}

//================================================================================
/*!
 * \brief Splits a quad at I or J. Returns an index of a new side in the new quad
 */
//================================================================================

int StdMeshers_Quadrangle_2D::splitQuad(FaceQuadStruct::Ptr quad, int I, int J)
{
  FaceQuadStruct* newQuad = new FaceQuadStruct( quad->face );
  myQuadList.push_back( FaceQuadStruct::Ptr( newQuad ));

  vector<UVPtStruct> points;
  if ( I > 0 && I <= quad->iSize-2 )
  {
    points.reserve( quad->jSize );
    for ( int jP = 0; jP < quad->jSize; ++jP )
      points.push_back( quad->UVPt( I, jP ));

    newQuad->side.resize( 4 );
    newQuad->side[ QUAD_BOTTOM_SIDE ] = quad->side[ QUAD_BOTTOM_SIDE ];
    newQuad->side[ QUAD_RIGHT_SIDE  ] = quad->side[ QUAD_RIGHT_SIDE  ];
    newQuad->side[ QUAD_TOP_SIDE    ] = quad->side[ QUAD_TOP_SIDE    ];
    newQuad->side[ QUAD_LEFT_SIDE   ] = StdMeshers_FaceSide::New( points, quad->face );

    FaceQuadStruct::Side& newSide  = newQuad->side[ QUAD_LEFT_SIDE ];
    FaceQuadStruct::Side& newSide2 = quad->side   [ QUAD_RIGHT_SIDE ];

    quad->side[ QUAD_RIGHT_SIDE  ] = newSide;

    int iBot = quad->side[ QUAD_BOTTOM_SIDE ].ToSideIndex( I );
    int iTop = quad->side[ QUAD_TOP_SIDE    ].ToSideIndex( I );

    newSide.AddContact ( 0,               & quad->side[ QUAD_BOTTOM_SIDE ], iBot );
    newSide2.AddContact( 0,               & quad->side[ QUAD_BOTTOM_SIDE ], iBot );
    newSide.AddContact ( quad->jSize - 1, & quad->side[ QUAD_TOP_SIDE    ], iTop );
    newSide2.AddContact( quad->jSize - 1, & quad->side[ QUAD_TOP_SIDE    ], iTop );
    // cout << "Contact: L " << &newSide << " "<< newSide.NbPoints()
    //      << " R " << &newSide2 << " "<< newSide2.NbPoints()
    //      << " B " << &quad->side[ QUAD_BOTTOM_SIDE ] << " "<< quad->side[ QUAD_BOTTOM_SIDE].NbPoints()
    //      << " T " << &quad->side[ QUAD_TOP_SIDE ]  << " "<< quad->side[ QUAD_TOP_SIDE].NbPoints()<< endl;

    newQuad->side[ QUAD_BOTTOM_SIDE ].from = iBot;
    newQuad->side[ QUAD_TOP_SIDE    ].from = iTop;
    newQuad->name = ( TComm("Right of I=") << I );

    bool bRev = quad->side[ QUAD_BOTTOM_SIDE ].IsReversed();
    bool tRev = quad->side[ QUAD_TOP_SIDE    ].IsReversed();
    quad->side[ QUAD_BOTTOM_SIDE ].to = iBot + ( bRev ? -1 : +1 );
    quad->side[ QUAD_TOP_SIDE    ].to = iTop + ( tRev ? -1 : +1 );
    quad->uv_grid.clear();

    return QUAD_LEFT_SIDE;
  }
  else if ( J > 0  && J <= quad->jSize-2 ) //// split horizontally, a new quad is below an old one
  {
    points.reserve( quad->iSize );
    for ( int iP = 0; iP < quad->iSize; ++iP )
      points.push_back( quad->UVPt( iP, J ));

    newQuad->side.resize( 4 );
    newQuad->side[ QUAD_BOTTOM_SIDE ] = quad->side[ QUAD_BOTTOM_SIDE ];
    newQuad->side[ QUAD_RIGHT_SIDE  ] = quad->side[ QUAD_RIGHT_SIDE  ];
    newQuad->side[ QUAD_TOP_SIDE    ] = StdMeshers_FaceSide::New( points, quad->face );
    newQuad->side[ QUAD_LEFT_SIDE   ] = quad->side[ QUAD_LEFT_SIDE   ];

    FaceQuadStruct::Side& newSide  = newQuad->side[ QUAD_TOP_SIDE    ];
    FaceQuadStruct::Side& newSide2 = quad->side   [ QUAD_BOTTOM_SIDE ];

    quad->side[ QUAD_BOTTOM_SIDE ] = newSide;

    int iLft = quad->side[ QUAD_LEFT_SIDE  ].ToSideIndex( J );
    int iRgt = quad->side[ QUAD_RIGHT_SIDE ].ToSideIndex( J );

    newSide.AddContact ( 0,               & quad->side[ QUAD_LEFT_SIDE  ], iLft );
    newSide2.AddContact( 0,               & quad->side[ QUAD_LEFT_SIDE  ], iLft );
    newSide.AddContact ( quad->iSize - 1, & quad->side[ QUAD_RIGHT_SIDE ], iRgt );
    newSide2.AddContact( quad->iSize - 1, & quad->side[ QUAD_RIGHT_SIDE ], iRgt );
    // cout << "Contact: T " << &newSide << " "<< newSide.NbPoints()
    //      << " B " << &newSide2 << " "<< newSide2.NbPoints()
    //      << " L " << &quad->side[ QUAD_LEFT_SIDE ] << " "<< quad->side[ QUAD_LEFT_SIDE].NbPoints()
    //      << " R " << &quad->side[ QUAD_RIGHT_SIDE ]  << " "<< quad->side[ QUAD_RIGHT_SIDE].NbPoints()<< endl;

    bool rRev = newQuad->side[ QUAD_RIGHT_SIDE ].IsReversed();
    bool lRev = newQuad->side[ QUAD_LEFT_SIDE  ].IsReversed();
    newQuad->side[ QUAD_RIGHT_SIDE ].to = iRgt + ( rRev ? -1 : +1 );
    newQuad->side[ QUAD_LEFT_SIDE  ].to = iLft + ( lRev ? -1 : +1 );
    newQuad->name = ( TComm("Below J=") << J );

    quad->side[ QUAD_RIGHT_SIDE ].from = iRgt;
    quad->side[ QUAD_LEFT_SIDE  ].from = iLft;
    quad->uv_grid.clear();

    return QUAD_TOP_SIDE;
  }

  myQuadList.pop_back();
  return -1;
}

//================================================================================
/*!
 * \brief Updates UV of a side after moving its node
 */
//================================================================================

void StdMeshers_Quadrangle_2D::updateSideUV( FaceQuadStruct::Side&  side,
                                             int                    iForced,
                                             const TQuadsBySide&    quadsBySide,
                                             int *                  iNext)
{
  if ( !iNext )
  {
    side.forced_nodes.insert( iForced );

    // update parts of the side before and after iForced

    set<int>::iterator iIt = side.forced_nodes.upper_bound( iForced );
    int iEnd = Min( side.NbPoints()-1, ( iIt == side.forced_nodes.end() ) ? int(1e7) : *iIt );
    if ( iForced + 1 < iEnd )
      updateSideUV( side, iForced, quadsBySide, &iEnd );

    iIt = side.forced_nodes.lower_bound( iForced );
    int iBeg = Max( 0, ( iIt == side.forced_nodes.begin() ) ? 0 : *--iIt );
    if ( iForced - 1 > iBeg )
      updateSideUV( side, iForced, quadsBySide, &iBeg );

    return;
  }

  const int iFrom    = Min ( iForced, *iNext );
  const int iTo      = Max ( iForced, *iNext ) + 1;
  const int sideSize = iTo - iFrom;

  vector<UVPtStruct> points[4]; // side points of a temporary quad

  // from the quads get grid points adjacent to the side
  // to make two sides of a temporary quad
  vector< FaceQuadStruct::Ptr > quads = quadsBySide.find( side )->second; // copy!
  for ( int is2nd = 0; is2nd < 2; ++is2nd )
  {
    points[ is2nd ].reserve( sideSize );
    int nbLoops = 0;
    while ( points[is2nd].size() < sideSize )
    {
      int iCur = iFrom + points[is2nd].size() - int( !points[is2nd].empty() );

      // look for a quad adjacent to iCur-th point of the side
      for ( size_t iQ = 0; iQ < quads.size(); ++iQ )
      {
        FaceQuadStruct::Ptr q = quads[ iQ ];
        if ( !q )
          continue;
        size_t iS;
        for ( iS = 0; iS < q->side.size(); ++iS )
          if ( side.grid == q->side[ iS ].grid )
            break;
        bool isOut;
        if ( !q->side[ iS ].IsReversed() )
          isOut = ( q->side[ iS ].from > iCur || q->side[ iS ].to-1 <= iCur );
        else
          isOut = ( q->side[ iS ].to  >= iCur || q->side[ iS ].from <= iCur );
        if ( isOut )
          continue;
        if ( !setNormalizedGrid( q ))
          continue;

        // found - copy points
        int i,j,di,dj,nb;
        if ( iS % 2 ) // right or left
        {
          i  = ( iS == QUAD_LEFT_SIDE ) ? 1 : q->iSize-2;
          j  = q->side[ iS ].ToQuadIndex( iCur );
          di = 0;
          dj = ( q->side[ iS ].IsReversed() ) ? -1  : +1;
          nb = ( q->side[ iS ].IsReversed() ) ? j+1 : q->jSize-j;
        }
        else // bottom or top
        {
          i  = q->side[ iS ].ToQuadIndex( iCur );
          j  = ( iS == QUAD_BOTTOM_SIDE )  ?  1 : q->jSize-2;
          di = ( q->side[ iS ].IsReversed() ) ? -1  : +1;
          dj = 0;
          nb = ( q->side[ iS ].IsReversed() ) ? i+1 : q->iSize-i;
        }
        if ( !points[is2nd].empty() )
        {
          gp_UV lastUV = points[is2nd].back().UV();
          gp_UV quadUV = q->UVPt( i, j ).UV();
          if ( ( lastUV - quadUV ).SquareModulus() > 1e-10 )
            continue; // quad is on the other side of the side
          i += di; j += dj; --nb;
        }
        for ( ; nb > 0 ; --nb )
        {
          points[ is2nd ].push_back( q->UVPt( i, j ));
          if ( points[is2nd].size() >= sideSize )
            break;
          i += di; j += dj;
        }
        quads[ iQ ].reset(); // not to use this quad anymore

        if ( points[is2nd].size() >= sideSize )
          break;
      } // loop on quads

      if ( nbLoops++ > quads.size() )
        throw SALOME_Exception( "StdMeshers_Quadrangle_2D::updateSideUV() bug: infinite loop" );

    } // while ( points[is2nd].size() < sideSize )
  } // two loops to fill points[0] and points[1]

  // points for other pair of opposite sides of the temporary quad

  enum { L,R,B,T }; // side index of points[]

  points[B].push_back( points[L].front() );
  points[B].push_back( side.GetUVPtStruct()[ iFrom ]);
  points[B].push_back( points[R].front() );

  points[T].push_back( points[L].back() );
  points[T].push_back( side.GetUVPtStruct()[ iTo-1 ]);
  points[T].push_back( points[R].back() );

  // make the temporary quad
  FaceQuadStruct::Ptr tmpQuad
    ( new FaceQuadStruct( TopoDS::Face( myHelper->GetSubShape() ), "tmpQuad"));
  tmpQuad->side.push_back( StdMeshers_FaceSide::New( points[B] )); // bottom
  tmpQuad->side.push_back( StdMeshers_FaceSide::New( points[R] )); // right
  tmpQuad->side.push_back( StdMeshers_FaceSide::New( points[T] ));
  tmpQuad->side.push_back( StdMeshers_FaceSide::New( points[L] ));

  // compute new UV of the side
  setNormalizedGrid( tmpQuad );
  gp_UV uv = tmpQuad->UVPt(1,0).UV();
  tmpQuad->updateUV( uv, 1,0, /*isVertical=*/true );

  // update UV of the side
  vector<UVPtStruct>& sidePoints = (vector<UVPtStruct>&) side.GetUVPtStruct();
  for ( int i = iFrom; i < iTo; ++i )
  {
    const uvPtStruct& uvPt = tmpQuad->UVPt( 1, i-iFrom );
    sidePoints[ i ].u = uvPt.u;
    sidePoints[ i ].v = uvPt.v;
  }
}

//================================================================================
/*!
 * \brief Finds indices of a grid quad enclosing the given enforced UV
 */
//================================================================================

bool FaceQuadStruct::findCell( const gp_XY& UV, int & I, int & J )
{
  // setNormalizedGrid() must be called before!
  if ( uv_box.IsOut( UV ))
    return false;

  // find an approximate position
  double x = 0.5, y = 0.5;
  gp_XY t0 = UVPt( iSize - 1, 0 ).UV();
  gp_XY t1 = UVPt( 0, jSize - 1 ).UV();
  gp_XY t2 = UVPt( 0, 0         ).UV();
  SMESH_MeshAlgos::GetBarycentricCoords( UV, t0, t1, t2, x, y );
  x = Min( 1., Max( 0., x ));
  y = Min( 1., Max( 0., y ));

  // precise the position
  normPa2IJ( x,y, I,J );
  if ( !isNear( UV, I,J ))
  {
    // look for the most close IJ by traversing uv_grid in the middle
    double dist2, minDist2 = ( UV - UVPt( I,J ).UV() ).SquareModulus();
    for ( int isU = 0; isU < 2; ++isU )
    {
      int ind1 = isU ? 0 : iSize / 2;
      int ind2 = isU ? jSize / 2 : 0;
      int di1  = isU ? Max( 2, iSize / 20 ) : 0;
      int di2  = isU ? 0 : Max( 2, jSize / 20 );
      int i,nb = isU ? iSize / di1 : jSize / di2;
      for ( i = 0; i < nb; ++i, ind1 += di1, ind2 += di2 )
        if (( dist2 = ( UV - UVPt( ind1,ind2 ).UV() ).SquareModulus() ) < minDist2 )
        {
          I = ind1;
          J = ind2;
          if ( isNear( UV, I,J ))
            return true;
          minDist2 = ( UV - UVPt( I,J ).UV() ).SquareModulus();
        }
    }
    if ( !isNear( UV, I,J, Max( iSize, jSize ) /2 ))
      return false;
  }
  return true;
}

//================================================================================
/*!
 * \brief Find indices (i,j) of a point in uv_grid by normalized parameters (x,y)
 */
//================================================================================

void FaceQuadStruct::normPa2IJ(double X, double Y, int & I, int & J )
{

  I = Min( int ( iSize * X ), iSize - 2 );
  J = Min( int ( jSize * Y ), jSize - 2 );

  int oldI, oldJ;
  do
  {
    oldI = I, oldJ = J;
    while ( X <= UVPt( I,J ).x   && I != 0 )
      --I;
    while ( X >  UVPt( I+1,J ).x && I+2 < iSize )
      ++I;
    while ( Y <= UVPt( I,J ).y   && J != 0 )
      --J;
    while ( Y >  UVPt( I,J+1 ).y && J+2 < jSize )
      ++J;
  } while ( oldI != I || oldJ != J );
}

//================================================================================
/*!
 * \brief Looks for UV in quads around a given (I,J) and precise (I,J)
 */
//================================================================================

bool FaceQuadStruct::isNear( const gp_XY& UV, int & I, int & J, int nbLoops )
{
  if ( I+1 >= iSize ) I = iSize - 2;
  if ( J+1 >= jSize ) J = jSize - 2;

  double bcI, bcJ;
  gp_XY uvI, uvJ, uv0, uv1;
  for ( int iLoop = 0; iLoop < nbLoops; ++iLoop )
  {
    int oldI = I, oldJ = J;

    uvI = UVPt( I+1, J ).UV();
    uvJ = UVPt( I, J+1 ).UV();
    uv0 = UVPt( I, J   ).UV();
    SMESH_MeshAlgos::GetBarycentricCoords( UV, uvI, uvJ, uv0, bcI, bcJ );
    if ( bcI >= 0. && bcJ >= 0. && bcI + bcJ <= 1.)
      return true;

    if ( I > 0       && bcI < 0. ) --I;
    if ( I+2 < iSize && bcI > 1. ) ++I;
    if ( J > 0       && bcJ < 0. ) --J;
    if ( J+2 < jSize && bcJ > 1. ) ++J;

    uv1 = UVPt( I+1,J+1).UV();
    if ( I != oldI || J != oldJ )
    {
      uvI = UVPt( I+1, J ).UV();
      uvJ = UVPt( I, J+1 ).UV();
    }
    SMESH_MeshAlgos::GetBarycentricCoords( UV, uvI, uvJ, uv1, bcI, bcJ );
    if ( bcI >= 0. && bcJ >= 0. && bcI + bcJ <= 1.)
      return true;

    if ( I > 0       && bcI > 1. ) --I;
    if ( I+2 < iSize && bcI < 0. ) ++I;
    if ( J > 0       && bcJ > 1. ) --J;
    if ( J+2 < jSize && bcJ < 0. ) ++J;

    if ( I == oldI && J == oldJ )
      return false;

    if ( iLoop+1 == nbLoops )
    {
      uvI = UVPt( I+1, J ).UV();
      uvJ = UVPt( I, J+1 ).UV();
      uv0 = UVPt( I, J   ).UV();
      SMESH_MeshAlgos::GetBarycentricCoords( UV, uvI, uvJ, uv0, bcI, bcJ );
      if ( bcI >= 0. && bcJ >= 0. && bcI + bcJ <= 1.)
        return true;

      uv1 = UVPt( I+1,J+1).UV();
      SMESH_MeshAlgos::GetBarycentricCoords( UV, uvI, uvJ, uv1, bcI, bcJ );
      if ( bcI >= 0. && bcJ >= 0. && bcI + bcJ <= 1.)
        return true;
    }
  }
  return false;
}

//================================================================================
/*!
 * \brief Checks if a given UV is equal to a given grid point
 */
//================================================================================

bool FaceQuadStruct::isEqual( const gp_XY& UV, int I, int J )
{
  TopLoc_Location loc;
  Handle(Geom_Surface) surf = BRep_Tool::Surface( face, loc );
  gp_Pnt p1 = surf->Value( UV.X(), UV.Y() );
  gp_Pnt p2 = surf->Value( UVPt( I,J ).u, UVPt( I,J ).v );

  double dist2 = 1e100;
  for ( int di = -1; di < 2; di += 2 )
  {
    int i = I + di;
    if ( i < 0 || i+1 >= iSize ) continue;
    for ( int dj = -1; dj < 2; dj += 2 )
    {
      int j = J + dj;
      if ( j < 0 || j+1 >= jSize ) continue;

      dist2 = Min( dist2,
                   p2.SquareDistance( surf->Value( UVPt( i,j ).u, UVPt( i,j ).v )));
    }
  }
  double tol2 = dist2 / 1000.;
  return p1.SquareDistance( p2 ) < tol2;
}

//================================================================================
/*!
 * \brief Recompute UV of grid points around a moved point in one direction
 */
//================================================================================

void FaceQuadStruct::updateUV( const gp_XY& UV, int I, int J, bool isVertical )
{
  UVPt( I, J ).u = UV.X();
  UVPt( I, J ).v = UV.Y();

  if ( isVertical )
  {
    // above J
    if ( J+1 < jSize-1 )
    {
      gp_UV a0 = UVPt( 0,       J       ).UV();
      gp_UV a1 = UVPt( iSize-1, J       ).UV();
      gp_UV a2 = UVPt( iSize-1, jSize-1 ).UV();
      gp_UV a3 = UVPt( 0,       jSize-1 ).UV();

      gp_UV p0 = UVPt( I, J       ).UV();
      gp_UV p2 = UVPt( I, jSize-1 ).UV();
      const double y0 = UVPt( I, J ).y, dy = 1. - y0;
      for (int j = J+1; j < jSize-1; j++)
      {
        gp_UV p1 = UVPt( iSize-1, j ).UV();
        gp_UV p3 = UVPt( 0,       j ).UV();

        UVPtStruct& uvPt = UVPt( I, j );
        gp_UV uv = calcUV( uvPt.x, ( uvPt.y - y0 ) / dy, a0,a1,a2,a3, p0,p1,p2,p3);
        uvPt.u = uv.X();
        uvPt.v = uv.Y();
      }
    }
    // under J
    if ( J-1 > 0 )
    {
      gp_UV a0 = UVPt( 0,       0 ).UV();
      gp_UV a1 = UVPt( iSize-1, 0 ).UV();
      gp_UV a2 = UVPt( iSize-1, J ).UV();
      gp_UV a3 = UVPt( 0,       J ).UV();

      gp_UV p0 = UVPt( I, 0 ).UV();
      gp_UV p2 = UVPt( I, J ).UV();
      const double y0 = 0., dy = UVPt( I, J ).y - y0;
      for (int j = 1; j < J; j++)
      {
        gp_UV p1 = UVPt( iSize-1, j ).UV();
        gp_UV p3 = UVPt( 0,       j ).UV();

        UVPtStruct& uvPt = UVPt( I, j );
        gp_UV uv = calcUV( uvPt.x, ( uvPt.y - y0 ) / dy, a0,a1,a2,a3, p0,p1,p2,p3);
        uvPt.u = uv.X();
        uvPt.v = uv.Y();
      }
    }
  }
  else  // horizontally
  {
    // before I
    if ( I-1 > 0 )
    {
      gp_UV a0 = UVPt( 0, 0 ).UV();
      gp_UV a1 = UVPt( I, 0 ).UV();
      gp_UV a2 = UVPt( I, jSize-1 ).UV();
      gp_UV a3 = UVPt( 0, jSize-1 ).UV();

      gp_UV p1 = UVPt( I, J ).UV();
      gp_UV p3 = UVPt( 0, J ).UV();
      const double x0 = 0., dx = UVPt( I, J ).x - x0;
      for (int i = 1; i < I; i++)
      {
        gp_UV p0 = UVPt( i, 0       ).UV();
        gp_UV p2 = UVPt( i, jSize-1 ).UV();

        UVPtStruct& uvPt = UVPt( i, J );
        gp_UV uv = calcUV(( uvPt.x - x0 ) / dx , uvPt.y, a0,a1,a2,a3, p0,p1,p2,p3);
        uvPt.u = uv.X();
        uvPt.v = uv.Y();
      }
    }
    // after I
    if ( I+1 < iSize-1 )
    {
      gp_UV a0 = UVPt( I,       0 ).UV();
      gp_UV a1 = UVPt( iSize-1, 0 ).UV();
      gp_UV a2 = UVPt( iSize-1, jSize-1 ).UV();
      gp_UV a3 = UVPt( I,       jSize-1 ).UV();

      gp_UV p1 = UVPt( iSize-1, J ).UV();
      gp_UV p3 = UVPt( I,       J ).UV();
      const double x0 = UVPt( I, J ).x, dx = 1. - x0;
      for (int i = I+1; i < iSize-1; i++)
      {
        gp_UV p0 = UVPt( i, 0       ).UV();
        gp_UV p2 = UVPt( i, jSize-1 ).UV();

        UVPtStruct& uvPt = UVPt( i, J );
        gp_UV uv = calcUV(( uvPt.x - x0 ) / dx , uvPt.y, a0,a1,a2,a3, p0,p1,p2,p3);
        uvPt.u = uv.X();
        uvPt.v = uv.Y();
      }
    }
  }
}

//================================================================================
/*!
 * \brief Side copying
 */
//================================================================================

FaceQuadStruct::Side& FaceQuadStruct::Side::operator=(const Side& otherSide)
{
  grid = otherSide.grid;
  from = otherSide.from;
  to   = otherSide.to;
  di   = otherSide.di;
  forced_nodes = otherSide.forced_nodes;
  contacts     = otherSide.contacts;
  nbNodeOut    = otherSide.nbNodeOut;

  for ( size_t iC = 0; iC < contacts.size(); ++iC )
  {
    FaceQuadStruct::Side* oSide = contacts[iC].other_side;
    for ( size_t iOC = 0; iOC < oSide->contacts.size(); ++iOC )
      if ( oSide->contacts[iOC].other_side == & otherSide )
      {
        // cout << "SHIFT old " << &otherSide << " " << otherSide.NbPoints()
        //      << " -> new " << this << " " << this->NbPoints() << endl;
        oSide->contacts[iOC].other_side = this;
      }
  }
  return *this;
}

//================================================================================
/*!
 * \brief Converts node index of a quad to node index of this side
 */
//================================================================================

int FaceQuadStruct::Side::ToSideIndex( int quadNodeIndex ) const
{
  return from + di * quadNodeIndex;
}

//================================================================================
/*!
 * \brief Converts node index of this side to node index of a quad
 */
//================================================================================

int FaceQuadStruct::Side::ToQuadIndex( int sideNodeIndex ) const
{
  return ( sideNodeIndex - from ) * di;
}

//================================================================================
/*!
 * \brief Reverse the side
 */
//================================================================================

bool FaceQuadStruct::Side::Reverse(bool keepGrid)
{
  if ( grid )
  {
    if ( keepGrid )
    {
      from -= di;
      to   -= di;
      std::swap( from, to );
      di   *= -1;
    }
    else
    {
      grid->Reverse();
    }
  }
  return (bool)grid;
}

//================================================================================
/*!
 * \brief Checks if a node is enforced
 *  \param [in] nodeIndex - an index of a node in a size
 *  \return bool - \c true if the node is forced
 */
//================================================================================

bool FaceQuadStruct::Side::IsForced( int nodeIndex ) const
{
  if ( nodeIndex < 0 || nodeIndex >= grid->NbPoints() )
    throw SALOME_Exception( " FaceQuadStruct::Side::IsForced(): wrong index" );

  if ( forced_nodes.count( nodeIndex ) )
    return true;

  for ( size_t i = 0; i < this->contacts.size(); ++i )
    if ( contacts[ i ].point == nodeIndex &&
         contacts[ i ].other_side->forced_nodes.count( contacts[ i ].other_point ))
      return true;

  return false;
}

//================================================================================
/*!
 * \brief Sets up a contact between this and another side
 */
//================================================================================

void FaceQuadStruct::Side::AddContact( int ip, Side* side, int iop )
{
  if ( ip  >= GetUVPtStruct().size()      ||
       iop >= side->GetUVPtStruct().size() )
    throw SALOME_Exception( "FaceQuadStruct::Side::AddContact(): wrong point" );
  if ( ip < from || ip >= to )
    return;
  {
    contacts.resize( contacts.size() + 1 );
    Contact&    c = contacts.back();
    c.point       = ip;
    c.other_side  = side;
    c.other_point = iop;
  }
  {
    side->contacts.resize( side->contacts.size() + 1 );
    Contact&    c = side->contacts.back();
    c.point       = iop;
    c.other_side  = this;
    c.other_point = ip;
  }
}

//================================================================================
/*!
 * \brief Returns a normalized parameter of a point indexed within a quadrangle
 */
//================================================================================

double FaceQuadStruct::Side::Param( int i ) const
{
  const vector<UVPtStruct>& points = GetUVPtStruct();
  return (( points[ from + i * di ].normParam - points[ from ].normParam ) /
          ( points[ to   - 1 * di ].normParam - points[ from ].normParam ));
}

//================================================================================
/*!
 * \brief Returns UV by a parameter normalized within a quadrangle
 */
//================================================================================

gp_XY FaceQuadStruct::Side::Value2d( double x ) const
{
  const vector<UVPtStruct>& points = GetUVPtStruct();
  double u = ( points[ from ].normParam +
               x * ( points[ to-di ].normParam - points[ from ].normParam ));
  return grid->Value2d( u ).XY();
}

//================================================================================
/*!
 * \brief Returns side length
 */
//================================================================================

double FaceQuadStruct::Side::Length(int theFrom, int theTo) const
{
  if ( IsReversed() != ( theTo < theFrom ))
    std::swap( theTo, theFrom );

  const vector<UVPtStruct>& points = GetUVPtStruct();
  double r;
  if ( theFrom == theTo && theTo == -1 )
    r = Abs( First().normParam -
             Last ().normParam );
  else if ( IsReversed() )
    r = Abs( points[ Max( to,   theTo+1 ) ].normParam -
             points[ Min( from, theFrom ) ].normParam );
  else
    r = Abs( points[ Min( to,   theTo-1 ) ].normParam -
             points[ Max( from, theFrom ) ].normParam );
  return r * grid->Length();
}
