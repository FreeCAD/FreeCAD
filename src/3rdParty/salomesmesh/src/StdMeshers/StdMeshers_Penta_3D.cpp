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

//  SMESH StdMeshers_Penta_3D implementation of SMESH idl descriptions
//  File   : StdMeshers_Penta_3D.cxx
//  Module : SMESH
//
#include "StdMeshers_Penta_3D.hxx"

#include "utilities.h"
#include "Utils_ExceptHandlers.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_VolumeOfNodes.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"

#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_SequenceOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <stdio.h>
#include <algorithm>

using namespace std;

typedef map < int, int, less<int> >::iterator   \
  StdMeshers_IteratorOfDataMapOfIntegerInteger;

enum { NB_WALL_FACES = 4 };

//=======================================================================
//function : StdMeshers_Penta_3D
//purpose  : 
//=======================================================================
StdMeshers_Penta_3D::StdMeshers_Penta_3D()
  : myErrorStatus(SMESH_ComputeError::New())
{
  myTol3D=0.1;
  myWallNodesMaps.resize( SMESH_Block::NbFaces() );
  myShapeXYZ.resize( SMESH_Block::NbSubShapes() );
  myTool = 0;
}

//=======================================================================
//function : ~StdMeshers_Penta_3D
//purpose  : 
//=======================================================================

StdMeshers_Penta_3D::~StdMeshers_Penta_3D()
{
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================
bool StdMeshers_Penta_3D::Compute(SMESH_Mesh& aMesh, 
                                  const TopoDS_Shape& aShape)
{
  MESSAGE("StdMeshers_Penta_3D::Compute()");
  //
  bool bOK=false;
  //
  myShape=aShape;
  SetMesh(aMesh);
  //
  CheckData();
  if (!myErrorStatus->IsOK()) {
    return bOK;
  }

  //
  MakeBlock();
  if (!myErrorStatus->IsOK()) {
    return bOK;
  }
  //
  ClearMeshOnFxy1();
  if (!myErrorStatus->IsOK()) {
    return bOK;
  }

  // now unnecessary faces removed, we can load medium nodes
  SMESH_MesherHelper helper(aMesh);
  myTool = &helper;
  myCreateQuadratic = myTool->IsQuadraticSubMesh(aShape);

  //
  MakeNodes();
  if (!myErrorStatus->IsOK()) {
    return bOK;
  }
  //
  MakeConnectingMap();
  //
  MakeMeshOnFxy1();
  if (!myErrorStatus->IsOK()) {
    return bOK;
  }
  //
  MakeVolumeMesh();
  //
  return !bOK;
}

//=======================================================================
//function : MakeNodes
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::MakeNodes()
{
  const int aNbSIDs=9;
  int i, j, k, ij, iNbN, aNodeID, aSize, iErr;
  double aX, aY, aZ;
  SMESH_Block::TShapeID aSID, aSIDs[aNbSIDs]={
    SMESH_Block::ID_V000, SMESH_Block::ID_V100, 
    SMESH_Block::ID_V110, SMESH_Block::ID_V010,
    SMESH_Block::ID_Ex00, SMESH_Block::ID_E1y0, 
    SMESH_Block::ID_Ex10, SMESH_Block::ID_E0y0,
    SMESH_Block::ID_Fxy0
  }; 
  //
  SMESH_Mesh* pMesh=GetMesh();
  //
  // 1. Define the sizes of mesh
  //
  // 1.1 Horizontal size
  myJSize=0;
  for (i=0; i<aNbSIDs; ++i) {
    const TopoDS_Shape& aS = myBlock.Shape(aSIDs[i]);
    SMESH_subMesh *aSubMesh = pMesh->GetSubMeshContaining(aS);
    ASSERT(aSubMesh);
    SMESHDS_SubMesh *aSM = aSubMesh->GetSubMeshDS();
    if(!myCreateQuadratic) {
      iNbN = aSM->NbNodes();
    }
    else {
      iNbN = 0;
      SMDS_NodeIteratorPtr itn = aSM->GetNodes();
      while(itn->more()) {
        const SMDS_MeshNode* aNode = itn->next();
        if(myTool->IsMedium(aNode))
          continue;
        iNbN++;
      }
    }
    myJSize += iNbN;
  }
  //printf("***  Horizontal: number of nodes summary=%d\n", myJSize);
  //
  // 1.2 Vertical size
  myISize=2;
  {
    const TopoDS_Shape& aS=myBlock.Shape(SMESH_Block::ID_E00z);
    SMESH_subMesh *aSubMesh = pMesh->GetSubMeshContaining(aS);
    ASSERT(aSubMesh);
    SMESHDS_SubMesh *aSM = aSubMesh->GetSubMeshDS();
    if(!myCreateQuadratic) {
      iNbN = aSM->NbNodes();
    }
    else {
      iNbN = 0;
      SMDS_NodeIteratorPtr itn = aSM->GetNodes();
      while(itn->more()) {
        const SMDS_MeshNode* aNode = itn->next();
        if(myTool->IsMedium(aNode))
          continue;
        iNbN++;
      }
    }
    myISize += iNbN;
  }
  //printf("***  Vertical: number of nodes on edges and vertices=%d\n", myISize);
  //
  aSize=myISize*myJSize;
  myTNodes.resize(aSize);
  //
  StdMeshers_TNode aTNode;
  gp_XYZ aCoords;
  gp_Pnt aP3D;
  //
  // 2. Fill the repers on base face (Z=0)
  i=0; j=0;
  // vertices
  for (k=0; k<aNbSIDs; ++k) {
    aSID=aSIDs[k];
    const TopoDS_Shape& aS = myBlock.Shape(aSID);
    SMDS_NodeIteratorPtr ite = pMesh->GetSubMeshContaining(aS)->GetSubMeshDS()->GetNodes();
    while(ite->more()) {
      const SMDS_MeshNode* aNode = ite->next();
      if(myTool->IsMedium(aNode))
        continue;
      aNodeID=aNode->GetID();
      //
      aTNode.SetNode(aNode);
      aTNode.SetShapeSupportID(aSID);
      aTNode.SetBaseNodeID(aNodeID);
      //
      if ( SMESH_Block::IsEdgeID (aSID)) {
        const SMDS_EdgePosition* epos =
          static_cast<const SMDS_EdgePosition*>(aNode->GetPosition());
        myBlock.ComputeParameters( epos->GetUParameter(), aS, aCoords );
      }
      else {
        aX=aNode->X();
        aY=aNode->Y();
        aZ=aNode->Z();
        aP3D.SetCoord(aX, aY, aZ);
        myBlock.ComputeParameters(aP3D, aS, aCoords);
      }
      iErr = myBlock.ErrorStatus();
      if (iErr) {
        MESSAGE("StdMeshers_Penta_3D::MakeNodes()," <<
                "SMESHBlock: ComputeParameters operation failed");
        myErrorStatus=myBlock.GetError();
        return;
      }
      aTNode.SetNormCoord(aCoords);
      ij=i*myJSize+j;
      myTNodes[ij]=aTNode;
      ++j;
    }
  }

  // 3.1 Fill maps of wall nodes
  SMESH_Block::TShapeID wallFaceID[ NB_WALL_FACES ] = {
    SMESH_Block::ID_Fx0z, SMESH_Block::ID_Fx1z,
    SMESH_Block::ID_F0yz, SMESH_Block::ID_F1yz
  };
  SMESH_Block::TShapeID baseEdgeID[ NB_WALL_FACES ] = {
    SMESH_Block::ID_Ex00, SMESH_Block::ID_Ex10,
    SMESH_Block::ID_E0y0, SMESH_Block::ID_E1y0
  };
  for ( i = 0; i < NB_WALL_FACES ; ++i ) {
    int fIndex = SMESH_Block::ShapeIndex( wallFaceID[ i ]);
    bool ok = LoadIJNodes (myWallNodesMaps[ fIndex ],
                           TopoDS::Face( myBlock.Shape( wallFaceID[ i ] )),
                           TopoDS::Edge( myBlock.Shape( baseEdgeID[ i ] )),
                           pMesh->GetMeshDS());
    if ( !ok ) {
      myErrorStatus->myName = COMPERR_BAD_INPUT_MESH;
      myErrorStatus->myComment = SMESH_Comment() <<
        "Can't find regular quadrangle mesh on a side face #" <<
        pMesh->GetMeshDS()->ShapeToIndex( myBlock.Shape( wallFaceID[ i ]));
      return;
    }
  }

  // 3.2 find node columns for vertical edges and edge IDs
  vector<const SMDS_MeshNode*> * verticEdgeNodes[ NB_WALL_FACES ];
  SMESH_Block::TShapeID          verticEdgeID   [ NB_WALL_FACES ];
  for ( i = 0; i < NB_WALL_FACES ; ++i ) { // 4 first base nodes are nodes on vertices
    // edge ID
    SMESH_Block::TShapeID eID, vID = aSIDs[ i ];
    ShapeSupportID(false, vID, eID);
    verticEdgeID[ i ] = eID;
    // column nodes
    StdMeshers_TNode& aTNode = myTNodes[ i ];
    verticEdgeNodes[ i ] = 0;
    for ( j = 0; j < NB_WALL_FACES ; ++j ) { // loop on 4 wall faces
      int fIndex = SMESH_Block::ShapeIndex( wallFaceID[ j ]);
      StdMeshers_IJNodeMap & ijNodes= myWallNodesMaps[ fIndex ];
      if ( ijNodes.begin()->second[0] == aTNode.Node() )
        verticEdgeNodes[ i ] = & ijNodes.begin()->second;
      else if ( ijNodes.rbegin()->second[0] == aTNode.Node() )
        verticEdgeNodes[ i ] = & ijNodes.rbegin()->second;
      if ( verticEdgeNodes[ i ] )
        break;
    }
  }

  // 3.3 set XYZ of vertices, and initialize of the rest
  SMESHDS_Mesh* aMesh = GetMesh()->GetMeshDS();
  for ( int id = SMESH_Block::ID_V000; id < SMESH_Block::ID_Shell; ++id ) {
    if ( SMESH_Block::IsVertexID( id )) {
      TopoDS_Shape V = myBlock.Shape( id );
      SMESHDS_SubMesh* sm = aMesh->MeshElements( V );
      const SMDS_MeshNode* n = sm->GetNodes()->next();
      myShapeXYZ[ id ].SetCoord( n->X(), n->Y(), n->Z() );
    }
    else
      myShapeXYZ[ id ].SetCoord( 0., 0., 0. );
  }


  // 4. Fill the rest repers
  bool bIsUpperLayer;
  int iBNID;
  SMESH_Block::TShapeID aSSID, aBNSSID;
  StdMeshers_TNode aTN;
  //

  // create top face and find UV for it's corners
  const TopoDS_Face& TopFace = TopoDS::Face(myBlock.Shape(SMESH_Block::ID_Fxy1));
  SMESHDS_Mesh* meshDS = pMesh->GetMeshDS();
  int topfaceID = meshDS->ShapeToIndex(TopFace);
  const TopoDS_Vertex& v001 = TopoDS::Vertex(myBlock.Shape(SMESH_Block::ID_V001));
  SMDS_NodeIteratorPtr itn = pMesh->GetSubMeshContaining(v001)->GetSubMeshDS()->GetNodes();
  const SMDS_MeshNode* N = itn->next();
  gp_XY UV001 = myTool->GetNodeUV(TopFace,N);
  const TopoDS_Vertex& v101 = TopoDS::Vertex(myBlock.Shape(SMESH_Block::ID_V101));
  itn = pMesh->GetSubMeshContaining(v101)->GetSubMeshDS()->GetNodes();
  N = itn->next();
  gp_XY UV101 = myTool->GetNodeUV(TopFace,N);
  const TopoDS_Vertex& v011 = TopoDS::Vertex(myBlock.Shape(SMESH_Block::ID_V011));
  itn = pMesh->GetSubMeshContaining(v011)->GetSubMeshDS()->GetNodes();
  N = itn->next();
  gp_XY UV011 = myTool->GetNodeUV(TopFace,N);
  const TopoDS_Vertex& v111 = TopoDS::Vertex(myBlock.Shape(SMESH_Block::ID_V111));
  itn = pMesh->GetSubMeshContaining(v111)->GetSubMeshDS()->GetNodes();
  N = itn->next();
  gp_XY UV111 = myTool->GetNodeUV(TopFace,N);

  for (j=0; j<myJSize; ++j) { // loop on all nodes of the base face (ID_Fxy0)
    // base node info
    const StdMeshers_TNode& aBN = myTNodes[j];
    aBNSSID = (SMESH_Block::TShapeID)aBN.ShapeSupportID();
    iBNID = aBN.BaseNodeID();
    const gp_XYZ& aBNXYZ = aBN.NormCoord();
    bool createNode = ( aBNSSID == SMESH_Block::ID_Fxy0 ); // if base node is inside a bottom face
    //
    // set XYZ on horizontal edges and get node columns of faces:
    // 2 columns for each face, between which a base node is located
    vector<const SMDS_MeshNode*>* nColumns[8];
    double ratio[ NB_WALL_FACES ]; // base node position between columns [0.-1.]
    if ( createNode ) {
      for ( k = 0; k < NB_WALL_FACES ; ++k ) {
        ratio[ k ] = SetHorizEdgeXYZ (aBNXYZ, wallFaceID[ k ],
                                      nColumns[k*2], nColumns[k*2+1]);
      }
    }
    //
    // XYZ on the bottom and top faces
    const SMDS_MeshNode* n = aBN.Node();
    myShapeXYZ[ SMESH_Block::ID_Fxy0 ].SetCoord( n->X(), n->Y(), n->Z() );
    myShapeXYZ[ SMESH_Block::ID_Fxy1 ].SetCoord( 0., 0., 0. );
    //
    // first create or find a top node, then the rest ones in a column
    for (i=myISize-1; i>0; --i) // vertical loop, from top to bottom
    {
      bIsUpperLayer = (i==(myISize-1));
      gp_XY UV_Ex01, UV_Ex11, UV_E0y1, UV_E1y1;
      if ( createNode ) // a base node is inside a top face
      {
        // set XYZ on vertical edges and faces
        for ( k = 0; k < NB_WALL_FACES ; ++k ) {
          // XYZ on a vertical edge 
          const SMDS_MeshNode* n = (*verticEdgeNodes[ k ]) [ i ];
          myShapeXYZ[ verticEdgeID[ k ] ].SetCoord( n->X(), n->Y(), n->Z() );
          // XYZ on a face (part 1 from one column)
          n = (*nColumns[k*2]) [ i ];
          gp_XYZ xyz( n->X(), n->Y(), n->Z() );
          myShapeXYZ[ wallFaceID[ k ]] = ( 1. - ratio[ k ]) * xyz;
          gp_XY tmp1;
          if( bIsUpperLayer ) {
            tmp1 = myTool->GetNodeUV(TopFace,n);
            tmp1 = ( 1. - ratio[ k ]) * tmp1;
          }
          // XYZ on a face (part 2 from other column)
          n = (*nColumns[k*2+1]) [ i ];
          xyz.SetCoord( n->X(), n->Y(), n->Z() );
          myShapeXYZ[ wallFaceID[ k ]] += ratio[ k ] * xyz;
          if( bIsUpperLayer ) {
            gp_XY tmp2 = myTool->GetNodeUV(TopFace,n);
            tmp1 +=  ratio[ k ] * tmp2;
            if( k==0 )
              UV_Ex01 = tmp1;
            else if( k==1 )
              UV_Ex11 = tmp1;
            else if( k==2 )
              UV_E0y1 = tmp1;
            else
              UV_E1y1 = tmp1;
          }
        }
      }
      // fill current node info
      //   -index in aTNodes
      ij=i*myJSize+j; 
      //   -normalized coordinates  
      aX=aBNXYZ.X();  
      aY=aBNXYZ.Y();
      //aZ=aZL[i];
      aZ=(double)i/(double)(myISize-1);
      aCoords.SetCoord(aX, aY, aZ);
      //
      //   suporting shape ID
      ShapeSupportID(bIsUpperLayer, aBNSSID, aSSID);
      if (!myErrorStatus->IsOK()) {
        MESSAGE("StdMeshers_Penta_3D::MakeNodes() ");
        return;
      }
      //
      aTN.SetShapeSupportID(aSSID);
      aTN.SetNormCoord(aCoords);
      aTN.SetBaseNodeID(iBNID);
      //
      if (aSSID!=SMESH_Block::ID_NONE){
        // try to find the node
        const TopoDS_Shape& aS=myBlock.Shape((int)aSSID);
        FindNodeOnShape(aS, aCoords, i, aTN);
      }
      else{
        // create node and get its id
        CreateNode (bIsUpperLayer, aCoords, aTN);
        //
        if ( bIsUpperLayer ) {
          const SMDS_MeshNode* n = aTN.Node();
          myShapeXYZ[ SMESH_Block::ID_Fxy1 ].SetCoord( n->X(), n->Y(), n->Z() );
          // set node on top face:
          // find UV parameter for this node
          //              UV_Ex11
          //   UV011+-----+----------+UV111
          //        |                |
          //        |                |
          // UV_E0y1+     +node      +UV_E1y1
          //        |                |
          //        |                |
          //        |                |
          //   UV001+-----+----------+UV101
          //              UV_Ex01
          gp_Pnt2d aP;
          double u = aCoords.X(), v = aCoords.Y();
          double u1 = ( 1. - u ), v1 = ( 1. - v );
          aP.ChangeCoord()  = UV_Ex01 * v1;
          aP.ChangeCoord() += UV_Ex11 * v;
          aP.ChangeCoord() += UV_E0y1 * u1;
          aP.ChangeCoord() += UV_E1y1 * u;
          aP.ChangeCoord() -= UV001 * u1 * v1;
          aP.ChangeCoord() -= UV101 * u  * v1;
          aP.ChangeCoord() -= UV011 * u1 * v;
          aP.ChangeCoord() -= UV111 * u  * v;
          meshDS->SetNodeOnFace((SMDS_MeshNode*)n, topfaceID, aP.X(), aP.Y());
        }
      }
      if (!myErrorStatus->IsOK()) {
        MESSAGE("StdMeshers_Penta_3D::MakeNodes() ");
        return;
      }
      //
      myTNodes[ij]=aTN;
    }
  }
}


//=======================================================================
//function : FindNodeOnShape
//purpose  : 
//=======================================================================

void StdMeshers_Penta_3D::FindNodeOnShape(const TopoDS_Shape& aS,
                                          const gp_XYZ&       aParams,
                                          const int           z,
                                          StdMeshers_TNode&   aTN)
{
  double aX, aY, aZ, aD, aTol2, minD;
  gp_Pnt aP1, aP2;
  //
  SMESH_Mesh* pMesh = GetMesh();
  aTol2 = myTol3D*myTol3D;
  minD = 1.e100;
  SMDS_MeshNode* pNode = NULL;
  //
  if ( aS.ShapeType() == TopAbs_FACE ||
       aS.ShapeType() == TopAbs_EDGE ) {
    // find a face ID to which aTN belongs to
    int faceID;
    if ( aS.ShapeType() == TopAbs_FACE )
      faceID = myBlock.ShapeID( aS );
    else { // edge maybe vertical or top horizontal
      gp_XYZ aCoord = aParams;
      if ( aCoord.Z() == 1. )
        aCoord.SetZ( 0.5 ); // move from top down
      else
        aCoord.SetX( 0.5 ); // move along X
      faceID = SMESH_Block::GetShapeIDByParams( aCoord );
    }
    ASSERT( SMESH_Block::IsFaceID( faceID ));
    int fIndex = SMESH_Block::ShapeIndex( faceID );
    StdMeshers_IJNodeMap & ijNodes = myWallNodesMaps[ fIndex ];
    // look for a base node in ijNodes
    const SMDS_MeshNode* baseNode = pMesh->GetMeshDS()->FindNode( aTN.BaseNodeID() );
    StdMeshers_IJNodeMap::const_iterator par_nVec = ijNodes.begin();
    for ( ; par_nVec != ijNodes.end(); par_nVec++ )
      if ( par_nVec->second[ 0 ] == baseNode ) {
        pNode = (SMDS_MeshNode*)par_nVec->second.at( z );
        aTN.SetNode(pNode);
        return;
      }
  }
  //
  myBlock.Point(aParams, aS, aP1);
  //
  SMDS_NodeIteratorPtr ite=
    pMesh->GetSubMeshContaining(aS)->GetSubMeshDS()->GetNodes();
  while(ite->more()) {
    const SMDS_MeshNode* aNode = ite->next();
    if(myTool->IsMedium(aNode))
      continue;
    aX=aNode->X();
    aY=aNode->Y();
    aZ=aNode->Z();
    aP2.SetCoord(aX, aY, aZ);
    aD=(double)aP1.SquareDistance(aP2);
    //printf("** D=%lf ", aD, aTol2);
    if (aD < minD) {
      pNode=(SMDS_MeshNode*)aNode;
      aTN.SetNode(pNode);
      minD = aD;
      //printf(" Ok\n");
      if (aD<aTol2)
        return; 
    }
  }
  //
  //printf(" KO\n");
  //aTN.SetNode(pNode);
  //MESSAGE("StdMeshers_Penta_3D::FindNodeOnShape(), can not find the node");
  //myErrorStatus=11; // can not find the node;
}


//=======================================================================
//function : SetHorizEdgeXYZ
//purpose  : 
//=======================================================================

double StdMeshers_Penta_3D::SetHorizEdgeXYZ(const gp_XYZ&                  aBaseNodeParams,
                                            const int                      aFaceID,
                                            vector<const SMDS_MeshNode*>*& aCol1,
                                            vector<const SMDS_MeshNode*>*& aCol2)
{
  // find base and top edges of the face
  enum { BASE = 0, TOP };
  vector< int > edgeVec; // 0-base, 1-top
  SMESH_Block::GetFaceEdgesIDs( aFaceID, edgeVec );
  //
  int coord = SMESH_Block::GetCoordIndOnEdge( edgeVec[ BASE ] );
  bool isForward = myBlock.IsForwadEdge( edgeVec[ BASE ] );

  double param = aBaseNodeParams.Coord( coord );
  if ( !isForward)
    param = 1. - param;
  //
  // look for columns around param
  StdMeshers_IJNodeMap & ijNodes =
    myWallNodesMaps[ SMESH_Block::ShapeIndex( aFaceID )];
  StdMeshers_IJNodeMap::iterator par_nVec_1 = ijNodes.begin();
  while ( par_nVec_1->first < param )
    par_nVec_1++;
  StdMeshers_IJNodeMap::iterator par_nVec_2 = par_nVec_1;
  //
  double r = 0;
  if ( par_nVec_1 != ijNodes.begin() ) {
    par_nVec_1--;
    r = ( param - par_nVec_1->first ) / ( par_nVec_2->first - par_nVec_1->first );
  }
  aCol1 = & par_nVec_1->second;
  aCol2 = & par_nVec_2->second;

  // top edge
  if (1) {
    // this variant is better for cases with curved edges and
    // different nodes distribution on top and base edges
    const SMDS_MeshNode* n1 = aCol1->back();
    const SMDS_MeshNode* n2 = aCol2->back();
    gp_XYZ xyz1( n1->X(), n1->Y(), n1->Z() );
    gp_XYZ xyz2( n2->X(), n2->Y(), n2->Z() );
    myShapeXYZ[ edgeVec[ 1 ] ] = ( 1. - r ) * xyz1 + r * xyz2;
  }
  else {
    // this variant is better for other cases
    //   SMESH_MesherHelper helper( *GetMesh() );
    //   const TopoDS_Edge & edge = TopoDS::Edge( myBlock.Shape( edgeVec[ TOP ]));
    //   double u1 = helper.GetNodeU( edge, n1 );
    //   double u2 = helper.GetNodeU( edge, n2 );
    //   double u = ( 1. - r ) * u1 + r * u2;
    //   gp_XYZ topNodeParams;
    //   myBlock.Block().EdgeParameters( edgeVec[ TOP ], u, topNodeParams );
    //   myBlock.Block().EdgePoint( edgeVec[ TOP ],
    //                              topNodeParams,
    //                              myShapeXYZ[ edgeVec[ TOP ]]);
  }

  // base edge
  myBlock.Block().EdgePoint( edgeVec[ BASE ],
                             aBaseNodeParams,
                             myShapeXYZ[ edgeVec[ BASE ]]);
  return r;
}


//=======================================================================
//function : MakeVolumeMesh
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::MakeVolumeMesh()
{
  int i, j, ij, ik, i1, i2, aSSID; 
  //
  SMESH_Mesh*   pMesh = GetMesh();
  SMESHDS_Mesh* meshDS = pMesh->GetMeshDS();
  //
  int shapeID = meshDS->ShapeToIndex( myShape );
  //
  // 1. Set Node In Volume
  ik = myISize-1;
  for (i=1; i<ik; ++i){
    for (j=0; j<myJSize; ++j){
      ij=i*myJSize+j;
      const StdMeshers_TNode& aTN = myTNodes[ij];
      aSSID=aTN.ShapeSupportID();
      if (aSSID==SMESH_Block::ID_NONE) {
        SMDS_MeshNode* aNode = (SMDS_MeshNode*)aTN.Node();
        meshDS->SetNodeInVolume(aNode, shapeID);
      }
    }
  }
  //
  // 2. Make pentahedrons
  int aID0, k , aJ[4];
  vector<const SMDS_MeshNode*> aN;
  //
  SMDS_ElemIteratorPtr itf, aItNodes;
  //
  const TopoDS_Face& aFxy0=
    TopoDS::Face(myBlock.Shape(SMESH_Block::ID_Fxy0));
  SMESH_subMesh *aSubMesh0 = pMesh->GetSubMeshContaining(aFxy0);
  SMESHDS_SubMesh *aSM0 = aSubMesh0->GetSubMeshDS();
  //
  itf = aSM0->GetElements();
  while(itf->more()) {
    const SMDS_MeshElement* pE0 = itf->next();
    //
    int nbFaceNodes = pE0->NbNodes();
    if(myCreateQuadratic)
      nbFaceNodes = nbFaceNodes/2;
    if ( aN.size() < nbFaceNodes * 2 )
      aN.resize( nbFaceNodes * 2 );
    //
    for ( k=0; k<nbFaceNodes; ++k ) {
      const SMDS_MeshNode* pNode = pE0->GetNode(k);
//       if(myTool->IsMedium(pNode))
//         continue;
      aID0 = pNode->GetID();
      aJ[k] = GetIndexOnLayer(aID0);
      if (!myErrorStatus->IsOK()) {
        MESSAGE("StdMeshers_Penta_3D::MakeVolumeMesh");
        return;
      }
    }
    //
    bool forward = true;
    for (i=0; i<ik; ++i) {
      i1=i;
      i2=i+1;
      for(j=0; j<nbFaceNodes; ++j) {
        ij = i1*myJSize+aJ[j];
        const StdMeshers_TNode& aTN1 = myTNodes[ij];
        const SMDS_MeshNode* aN1 = aTN1.Node();
        aN[j]=aN1;
        //
        ij=i2*myJSize+aJ[j];
        const StdMeshers_TNode& aTN2 = myTNodes[ij];
        const SMDS_MeshNode* aN2 = aTN2.Node();
        aN[j+nbFaceNodes] = aN2;
      }
      // check if volume orientation will be ok
      if ( i == 0 ) {
        SMDS_VolumeTool vTool;
        switch ( nbFaceNodes ) {
        case 3: {
          SMDS_VolumeOfNodes tmpVol (aN[0], aN[1], aN[2],
                                     aN[3], aN[4], aN[5]);
          vTool.Set( &tmpVol );
          break;
        }
        case 4: {
          SMDS_VolumeOfNodes tmpVol(aN[0], aN[1], aN[2], aN[3],
                                    aN[4], aN[5], aN[6], aN[7]);
          vTool.Set( &tmpVol );
          break;
        }
        default:
          continue;
        }
        forward = vTool.IsForward();
      }
      // add volume
      SMDS_MeshVolume* aV = 0;
      switch ( nbFaceNodes ) {
      case 3:
        if ( forward ) {
          //aV = meshDS->AddVolume(aN[0], aN[1], aN[2],
          //                       aN[3], aN[4], aN[5]);
          aV = myTool->AddVolume(aN[0], aN[1], aN[2], aN[3], aN[4], aN[5]);
        }
        else {
          //aV = meshDS->AddVolume(aN[0], aN[2], aN[1],
          //                       aN[3], aN[5], aN[4]);
          aV = myTool->AddVolume(aN[0], aN[2], aN[1], aN[3], aN[5], aN[4]);
        }
        break;
      case 4:
        if ( forward ) {
          //aV = meshDS->AddVolume(aN[0], aN[1], aN[2], aN[3],
          //                       aN[4], aN[5], aN[6], aN[7]);
          aV = myTool->AddVolume(aN[0], aN[1], aN[2], aN[3],
                                 aN[4], aN[5], aN[6], aN[7]);
        }
        else {
          //aV = meshDS->AddVolume(aN[0], aN[3], aN[2], aN[1],
          //                       aN[4], aN[7], aN[6], aN[5]);
          aV = myTool->AddVolume(aN[0], aN[3], aN[2], aN[1],
                                 aN[4], aN[7], aN[6], aN[5]);
        }
        break;
      default:
        continue;
      }
      meshDS->SetMeshElementOnShape(aV, shapeID);
    }
  }
}

//=======================================================================
//function : MakeMeshOnFxy1
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::MakeMeshOnFxy1()
{
  int aID0, aJ, aLevel, ij, aNbNodes, k;
  //
  SMDS_NodeIteratorPtr itn;
  SMDS_ElemIteratorPtr itf, aItNodes;
  SMDSAbs_ElementType aElementType;
  //
  const TopoDS_Face& aFxy0=
    TopoDS::Face(myBlock.Shape(SMESH_Block::ID_Fxy0));
  const TopoDS_Face& aFxy1=
    TopoDS::Face(myBlock.Shape(SMESH_Block::ID_Fxy1));
  //
  SMESH_Mesh* pMesh = GetMesh();
  SMESHDS_Mesh * meshDS = pMesh->GetMeshDS();
  //
  SMESH_subMesh *aSubMesh1 = pMesh->GetSubMeshContaining(aFxy1);
  SMESH_subMesh *aSubMesh0 = pMesh->GetSubMeshContaining(aFxy0);
  SMESHDS_SubMesh *aSM0 = aSubMesh0->GetSubMeshDS();
  //
  // set nodes on aFxy1
  aLevel = myISize-1;
  itn = aSM0->GetNodes();
  aNbNodes = aSM0->NbNodes();
  //printf("** aNbNodes=%d\n", aNbNodes);
  myTool->SetSubShape( aFxy1 ); // to set medium nodes to aFxy1
  //
  // set elements on aFxy1
  vector<const SMDS_MeshNode*> aNodes1;
  //
  itf = aSM0->GetElements();
  while(itf->more()) {
    const SMDS_MeshElement* pE0 = itf->next();
    aElementType = pE0->GetType();
    if (!(aElementType==SMDSAbs_Face)) {
      continue;
    }
    aNbNodes = pE0->NbNodes();
    if(myCreateQuadratic)
      aNbNodes = aNbNodes/2;
    if ( aNodes1.size() < aNbNodes )
      aNodes1.resize( aNbNodes );
    //
    k = aNbNodes-1; // reverse a face
    aItNodes = pE0->nodesIterator();
    while (aItNodes->more()) {
      const SMDS_MeshNode* pNode =
        static_cast<const SMDS_MeshNode*> (aItNodes->next());
      if(myTool->IsMedium(pNode))
        continue;
      aID0 = pNode->GetID();
      aJ = GetIndexOnLayer(aID0);
      if (!myErrorStatus->IsOK()) {
        MESSAGE("StdMeshers_Penta_3D::MakeMeshOnFxy1() ");
        return;
      }
      //
      ij = aLevel*myJSize + aJ;
      const StdMeshers_TNode& aTN1 = myTNodes[ij];
      const SMDS_MeshNode* aN1 = aTN1.Node();
      aNodes1[k] = aN1;
      --k;
    }
    SMDS_MeshFace * face = 0;
    switch ( aNbNodes ) {
    case 3:
      face = myTool->AddFace(aNodes1[0], aNodes1[1], aNodes1[2]);
      break;
    case 4:
      face = myTool->AddFace(aNodes1[0], aNodes1[1], aNodes1[2], aNodes1[3]);
      break;
    default:
      continue;
    }
    meshDS->SetMeshElementOnShape(face, aFxy1);
  }
  myTool->SetSubShape( myShape );

  // update compute state of top face submesh
  aSubMesh1->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );

  // assure that mesh on the top face will be cleaned when it is cleaned
  // on the bottom face
  SMESH_subMesh* volSM = pMesh->GetSubMesh( myTool->GetSubShape() );
  volSM->SetEventListener( new SMESH_subMeshEventListener(true, // deletable by SMESH_subMesh
                                                          "StdMeshers_Penta_3D"),
                           SMESH_subMeshEventListenerData::MakeData( aSubMesh1 ),
                           aSubMesh0 ); // translate CLEAN event of aSubMesh0 to aSubMesh1
}

//=======================================================================
//function : ClearMeshOnFxy1
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::ClearMeshOnFxy1()
{
  SMESH_subMesh* aSubMesh;
  SMESH_Mesh* pMesh=GetMesh();
  //
  const TopoDS_Shape& aFxy1=myBlock.Shape(SMESH_Block::ID_Fxy1);
  aSubMesh = pMesh->GetSubMeshContaining(aFxy1);
  if (aSubMesh)
    aSubMesh->ComputeStateEngine( SMESH_subMesh::CLEAN );
}

//=======================================================================
//function : GetIndexOnLayer
//purpose  : 
//=======================================================================
int StdMeshers_Penta_3D::GetIndexOnLayer(const int aID)
{
  int j=-1;
  StdMeshers_IteratorOfDataMapOfIntegerInteger aMapIt;
  //
  aMapIt=myConnectingMap.find(aID);
  if (aMapIt==myConnectingMap.end()) {
    myErrorStatus->myName    = 200;
    myErrorStatus->myComment = "Internal error of StdMeshers_Penta_3D";
    return j;
  }
  j=(*aMapIt).second;
  return j;
}

//=======================================================================
//function : MakeConnectingMap
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::MakeConnectingMap()
{
  int j, aBNID;
  //
  for (j=0; j<myJSize; ++j) {
    const StdMeshers_TNode& aBN=myTNodes[j];
    aBNID=aBN.BaseNodeID();
    myConnectingMap[aBNID]=j;
  }
}

//=======================================================================
//function : CreateNode
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::CreateNode(const bool bIsUpperLayer,
                                     const gp_XYZ& aParams,
                                     StdMeshers_TNode& aTN)
{
  double aX, aY, aZ;
  //
  gp_Pnt aP;
  //
  SMDS_MeshNode* pNode=NULL; 
  aTN.SetNode(pNode);  
  //
  //   if (bIsUpperLayer) {
  //     // point on face Fxy1
  //     const TopoDS_Shape& aS=myBlock.Shape(SMESH_Block::ID_Fxy1);
  //     myBlock.Point(aParams, aS, aP);
  //   }
  //   else {
  //     // point inside solid
  //     myBlock.Point(aParams, aP);
  //   }
  if (bIsUpperLayer) {
    double u = aParams.X(), v = aParams.Y();
    double u1 = ( 1. - u ), v1 = ( 1. - v );
    aP.ChangeCoord()  = myShapeXYZ[ SMESH_Block::ID_Ex01 ] * v1;
    aP.ChangeCoord() += myShapeXYZ[ SMESH_Block::ID_Ex11 ] * v;
    aP.ChangeCoord() += myShapeXYZ[ SMESH_Block::ID_E0y1 ] * u1;
    aP.ChangeCoord() += myShapeXYZ[ SMESH_Block::ID_E1y1 ] * u;

    aP.ChangeCoord() -= myShapeXYZ[ SMESH_Block::ID_V001 ] * u1 * v1;
    aP.ChangeCoord() -= myShapeXYZ[ SMESH_Block::ID_V101 ] * u  * v1;
    aP.ChangeCoord() -= myShapeXYZ[ SMESH_Block::ID_V011 ] * u1 * v;
    aP.ChangeCoord() -= myShapeXYZ[ SMESH_Block::ID_V111 ] * u  * v;
  }
  else {
    SMESH_Block::ShellPoint( aParams, myShapeXYZ, aP.ChangeCoord() );
  }
  //
  //   iErr=myBlock.ErrorStatus();
  //   if (iErr) {
  //     myErrorStatus=12; // can not find the node point;
  //     return;
  //   }
  //
  aX=aP.X(); aY=aP.Y(); aZ=aP.Z(); 
  //
  SMESH_Mesh* pMesh = GetMesh();
  SMESHDS_Mesh* pMeshDS = pMesh->GetMeshDS();
  //
  pNode = pMeshDS->AddNode(aX, aY, aZ);

  aTN.SetNode(pNode);
}

//=======================================================================
//function : ShapeSupportID
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::ShapeSupportID(const bool bIsUpperLayer,
                                         const SMESH_Block::TShapeID aBNSSID,
                                         SMESH_Block::TShapeID& aSSID)
{
  switch (aBNSSID) {
  case SMESH_Block::ID_V000:
    aSSID=(bIsUpperLayer) ?  SMESH_Block::ID_V001 : SMESH_Block::ID_E00z;
    break;
  case SMESH_Block::ID_V100:
    aSSID=(bIsUpperLayer) ?  SMESH_Block::ID_V101 : SMESH_Block::ID_E10z;
    break; 
  case SMESH_Block::ID_V110:
    aSSID=(bIsUpperLayer) ?  SMESH_Block::ID_V111 : SMESH_Block::ID_E11z;
    break;
  case SMESH_Block::ID_V010:
    aSSID=(bIsUpperLayer) ?  SMESH_Block::ID_V011 : SMESH_Block::ID_E01z;
    break;
  case SMESH_Block::ID_Ex00:
    aSSID=(bIsUpperLayer) ?  SMESH_Block::ID_Ex01 : SMESH_Block::ID_Fx0z;
    break;
  case SMESH_Block::ID_Ex10:
    aSSID=(bIsUpperLayer) ?  SMESH_Block::ID_Ex11 : SMESH_Block::ID_Fx1z;
    break; 
  case SMESH_Block::ID_E0y0:
    aSSID=(bIsUpperLayer) ?  SMESH_Block::ID_E0y1 : SMESH_Block::ID_F0yz;
    break; 
  case SMESH_Block::ID_E1y0:
    aSSID=(bIsUpperLayer) ?  SMESH_Block::ID_E1y1 : SMESH_Block::ID_F1yz;
    break; 
  case SMESH_Block::ID_Fxy0:
    aSSID=SMESH_Block::ID_NONE;//(bIsUpperLayer) ?  Shape_ID_Fxy1 : Shape_ID_NONE;
    break;   
  default:
    aSSID=SMESH_Block::ID_NONE;
    myErrorStatus->myName=10; // Can not find supporting shape ID
    myErrorStatus->myComment = "Internal error of StdMeshers_Penta_3D";
    break;
  }
  return;
}
//=======================================================================
//function : MakeBlock
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::MakeBlock()
{
  bool bFound;
  int i, j, iNbEV, iNbE, iErr, iCnt, iNbNodes, iNbF;
  //
  TopoDS_Vertex aV000, aV001;
  TopoDS_Shape aFTr;
  TopTools_IndexedDataMapOfShapeListOfShape aMVES;
  TopTools_IndexedMapOfShape aME ,aMEV, aM;
  TopTools_ListIteratorOfListOfShape aIt;
  //
  TopExp::MapShapes(myShape, TopAbs_FACE, aM);
  //
  // 0. Find triangulated face aFTr
  SMDSAbs_ElementType aElementType;
  SMESH_Mesh* pMesh=GetMesh();
  //
  iCnt = 0;
  iNbF = aM.Extent();
  for (i=1; i<=iNbF; ++i) {
    const TopoDS_Shape& aF = aM(i);
    SMESH_subMesh *aSubMesh = pMesh->GetSubMeshContaining(aF);
    ASSERT(aSubMesh);
    SMESHDS_SubMesh *aSM = aSubMesh->GetSubMeshDS();
    SMDS_ElemIteratorPtr itf = aSM->GetElements();
    while(itf->more()) {
      const SMDS_MeshElement * pElement = itf->next();
      aElementType = pElement->GetType();
      if (aElementType==SMDSAbs_Face) {
        iNbNodes = pElement->NbNodes();
        if ( iNbNodes==3 || (pElement->IsQuadratic() && iNbNodes==6) ) {
          aFTr = aF;
          ++iCnt;
          if (iCnt>1) {
            // \begin{E.A.}
            // The current algorithm fails if there is more that one
            // face which contains triangles ...
            // In that case, replace return by break to try another
            // method (coded in "if (iCnt != 1) { ... }")
            //
            // MESSAGE("StdMeshers_Penta_3D::MakeBlock() ");
            // myErrorStatus=5; // more than one face has triangulation
            // return;
            break;
            // \end{E.A.}
          }
          break; // next face
        }
      }
    }
  }
  //
  // \begin{E.A.}
  // The current algorithm fails if "iCnt != 1", the case "iCnt == 0"
  // was not reached 'cause it was not called from Hexa_3D ... Now it
  // can occurs and in my opinion, it is the most common case.
  //
  if (iCnt != 1) {
    // The suggested algorithm is the following :
    //
    // o Check that nb_of_faces == 6 and nb_of_edges == 12
    //   then the shape is tologically equivalent to a box
    // o In a box, there are three set of four // edges ...
    //   In the cascade notation, it seems to be the edges
    //   numbered : 
    //     - 1, 3, 5, 7
    //     - 2, 4, 6, 8
    //     - 9, 10, 11, 12
    // o For each one of this set, check if the four edges
    //   have the same number of element.
    // o If so, check if the "corresponding" // faces contains
    //   only quads. It's the faces numbered:
    //     - 1, 2, 3, 4
    //     - 1, 2, 5, 6
    //     - 3, 4, 5, 6
    // o If so, check if the opposite edges of each // faces
    //   have the same number of elements. It is the edges
    //   numbered :
    //     - 2 and 4, 6 and 8, 9 and 10, 11 and 12
    //     - 1 and 3, 5 and 7, 9 and 11, 10 and 12
    //     - 1 and 5, 3 and 7, 4 and 8, 2 and 6
    // o If so, check if the two other faces have the same
    //   number of elements. It is the faces numbered:
    //     - 5, 6
    //     - 3, 4
    //     - 1, 2
    //   This test should be improved to test if the nodes
    //   of the two faces are really "en face".
    // o If so, one of the two faces is a candidate to an extrusion,
    //   It is the faces numbered :
    //     - 5
    //     - 3
    //     - 1
    // o Finally, if there is only one candidate, let do the
    //   extrusion job for the corresponding face
    //
    int isOK = 0;
    //
    int iNbF = aM.Extent();
    if (iNbF == 6) {
      //
      int nb_f1 = pMesh->GetSubMeshContaining(aM(1))->GetSubMeshDS()->NbElements();
      int nb_f2 = pMesh->GetSubMeshContaining(aM(2))->GetSubMeshDS()->NbElements();
      int nb_f3 = pMesh->GetSubMeshContaining(aM(3))->GetSubMeshDS()->NbElements();
      int nb_f4 = pMesh->GetSubMeshContaining(aM(4))->GetSubMeshDS()->NbElements();
      int nb_f5 = pMesh->GetSubMeshContaining(aM(5))->GetSubMeshDS()->NbElements();
      int nb_f6 = pMesh->GetSubMeshContaining(aM(6))->GetSubMeshDS()->NbElements();
      //
      int has_only_quad_f1 = 1;
      int has_only_quad_f2 = 1;
      int has_only_quad_f3 = 1;
      int has_only_quad_f4 = 1;
      int has_only_quad_f5 = 1;
      int has_only_quad_f6 = 1;
      //
      for (i=1; i<=iNbF; ++i) {
        int ok = 1;
        const TopoDS_Shape& aF = aM(i);
        SMESH_subMesh *aSubMesh = pMesh->GetSubMeshContaining(aF);
        SMESHDS_SubMesh *aSM = aSubMesh->GetSubMeshDS();
        SMDS_ElemIteratorPtr itf = aSM->GetElements();
        while(itf->more()) {
          const SMDS_MeshElement * pElement = itf->next();
          aElementType = pElement->GetType();
          if (aElementType==SMDSAbs_Face) {
            iNbNodes = pElement->NbNodes();
            if ( iNbNodes!=4 ) {
              ok = 0;
              break ;
            }
          }
        }
        if (i==1) has_only_quad_f1 = ok ;
        if (i==2) has_only_quad_f2 = ok ;
        if (i==3) has_only_quad_f3 = ok ;
        if (i==4) has_only_quad_f4 = ok ;
        if (i==5) has_only_quad_f5 = ok ;
        if (i==6) has_only_quad_f6 = ok ;
      }
      //
      TopTools_IndexedMapOfShape aE;
      TopExp::MapShapes(myShape, TopAbs_EDGE, aE);
      int iNbE = aE.Extent();
      if (iNbE == 12) {
        //
        int nb_e01 = pMesh->GetSubMeshContaining(aE(1))->GetSubMeshDS()->NbElements();
        int nb_e02 = pMesh->GetSubMeshContaining(aE(2))->GetSubMeshDS()->NbElements();
        int nb_e03 = pMesh->GetSubMeshContaining(aE(3))->GetSubMeshDS()->NbElements();
        int nb_e04 = pMesh->GetSubMeshContaining(aE(4))->GetSubMeshDS()->NbElements();
        int nb_e05 = pMesh->GetSubMeshContaining(aE(5))->GetSubMeshDS()->NbElements();
        int nb_e06 = pMesh->GetSubMeshContaining(aE(6))->GetSubMeshDS()->NbElements();
        int nb_e07 = pMesh->GetSubMeshContaining(aE(7))->GetSubMeshDS()->NbElements();
        int nb_e08 = pMesh->GetSubMeshContaining(aE(8))->GetSubMeshDS()->NbElements();
        int nb_e09 = pMesh->GetSubMeshContaining(aE(9))->GetSubMeshDS()->NbElements();
        int nb_e10 = pMesh->GetSubMeshContaining(aE(10))->GetSubMeshDS()->NbElements();
        int nb_e11 = pMesh->GetSubMeshContaining(aE(11))->GetSubMeshDS()->NbElements();
        int nb_e12 = pMesh->GetSubMeshContaining(aE(12))->GetSubMeshDS()->NbElements();
        //
        int nb_ok = 0 ;
        //
        if ( (nb_e01==nb_e03) && (nb_e03==nb_e05) && (nb_e05==nb_e07) ) {
          if ( has_only_quad_f1 && has_only_quad_f2 && has_only_quad_f3 && has_only_quad_f4 ) {
            if ( (nb_e09==nb_e10) && (nb_e08==nb_e06) && (nb_e11==nb_e12) && (nb_e04==nb_e02) ) {
              if (nb_f5==nb_f6) {
                nb_ok += 1;
                aFTr = aM(5);
              }
            }
          }
        }
        if ( (nb_e02==nb_e04) && (nb_e04==nb_e06) && (nb_e06==nb_e08) ) {
          if ( has_only_quad_f1 && has_only_quad_f2 && has_only_quad_f5 && has_only_quad_f6 ) {
            if ( (nb_e01==nb_e03) && (nb_e10==nb_e12) && (nb_e05==nb_e07) && (nb_e09==nb_e11) ) {
              if (nb_f3==nb_f4) {
                nb_ok += 1;
                aFTr = aM(3);
              }
            }
          }
        }
        if ( (nb_e09==nb_e10) && (nb_e10==nb_e11) && (nb_e11==nb_e12) ) {
          if ( has_only_quad_f3 && has_only_quad_f4 && has_only_quad_f5 && has_only_quad_f6 ) {
            if ( (nb_e01==nb_e05) && (nb_e02==nb_e06) && (nb_e03==nb_e07) && (nb_e04==nb_e08) ) {
              if (nb_f1==nb_f2) {
                nb_ok += 1;
                aFTr = aM(1);
              }
            }
          }
        }
        //
        if ( nb_ok == 1 ) {
          isOK = 1;
        }
        //
      }
    }
    if (!isOK) {
      myErrorStatus->myName=5; // more than one face has triangulation
      myErrorStatus->myComment="Incorrect input mesh";
      return;
    }
  }
  // \end{E.A.}
  // 
  // 1. Vetrices V00, V001;
  //
  TopExp::MapShapes(aFTr, TopAbs_EDGE, aME);
  TopExp::MapShapesAndAncestors(myShape, TopAbs_VERTEX, TopAbs_EDGE, aMVES);
  //
  // 1.1 Base vertex V000
  iNbE = aME.Extent();
  if (iNbE!= NB_WALL_FACES ){
    MESSAGE("StdMeshers_Penta_3D::MakeBlock() ");
    myErrorStatus->myName=7; // too few edges are in base face aFTr
    myErrorStatus->myComment=SMESH_Comment("Not a quadrilateral face #")
      <<pMesh->GetMeshDS()->ShapeToIndex( aFTr )<<": "<<iNbE<<" edges" ;
    return;
  }
  const TopoDS_Edge& aE1=TopoDS::Edge(aME(1));
  aV000=TopExp::FirstVertex(aE1);
  //
  const TopTools_ListOfShape& aLE=aMVES.FindFromKey(aV000);
  aIt.Initialize(aLE);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aEx=aIt.Value();
    aMEV.Add(aEx);
  }
  iNbEV=aMEV.Extent();
  if (iNbEV!=3){
    MESSAGE("StdMeshers_Penta_3D::MakeBlock() ");
    myErrorStatus->myName=7; // too few edges meet in base vertex 
    myErrorStatus->myComment=SMESH_Comment("3 edges must share vertex #")
      <<pMesh->GetMeshDS()->ShapeToIndex( aV000 )<<" but there are "<<iNbEV<<" edges";
    return;
  }
  //
  // 1.2 Vertex V001
  bFound=false;
  for (j=1; j<=iNbEV; ++j) {
    const TopoDS_Shape& aEx=aMEV(j);
    if (!aME.Contains(aEx)) {
      TopoDS_Vertex aV[2];
      //
      const TopoDS_Edge& aE=TopoDS::Edge(aEx);
      TopExp::Vertices(aE, aV[0], aV[1]);
      for (i=0; i<2; ++i) {
        if (!aV[i].IsSame(aV000)) {
          aV001=aV[i];
          bFound=!bFound;
          break;
        }
      }
    }
  }
  //
  if (!bFound) {
    MESSAGE("StdMeshers_Penta_3D::MakeBlock() ");
    myErrorStatus->myName=8; // can not find reper V001
    myErrorStatus->myComment=SMESH_Comment("Can't find opposite vertex for vertex #")
      <<pMesh->GetMeshDS()->ShapeToIndex( aV000 );
    return;
  }
  //DEB
  //gp_Pnt aP000, aP001;
  //
  //aP000=BRep_Tool::Pnt(TopoDS::Vertex(aV000));
  //printf("*** aP000 { %lf, %lf, %lf }\n", aP000.X(), aP000.Y(), aP000.Z());
  //aP001=BRep_Tool::Pnt(TopoDS::Vertex(aV001));
  //printf("*** aP001 { %lf, %lf, %lf }\n", aP001.X(), aP001.Y(), aP001.Z());
  //DEB
  //
  aME.Clear();
  TopExp::MapShapes(myShape, TopAbs_SHELL, aME);
  iNbE=aME.Extent();
  if (iNbE!=1) {
    MESSAGE("StdMeshers_Penta_3D::MakeBlock() ");
    myErrorStatus->myName=9; // number of shells in source shape !=1
    myErrorStatus->myComment=SMESH_Comment("Unexpected nb of shells ")<<iNbE;
    return;
  }
  //
  // 2. Load Block
  const TopoDS_Shell& aShell=TopoDS::Shell(aME(1));
  myBlock.Load(aShell, aV000, aV001);
  iErr = myBlock.ErrorStatus();
  if (iErr) {
    MESSAGE("StdMeshers_Penta_3D::MakeBlock() ");
    myErrorStatus=myBlock.GetError(); // SMESHBlock: Load operation failed
    return;
  }
}
//=======================================================================
//function : CheckData
//purpose  : 
//=======================================================================
void StdMeshers_Penta_3D::CheckData()
{
  int i, iNb;
  int iNbEx[]={8, 12, 6};
  //
  TopAbs_ShapeEnum aST;
  TopAbs_ShapeEnum aSTEx[]={
    TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE
  }; 
  TopTools_IndexedMapOfShape aM;
  //
  if (myShape.IsNull()){
    MESSAGE("StdMeshers_Penta_3D::CheckData() ");
    myErrorStatus->myName=2; // null shape
    myErrorStatus->myComment="Null shape";
    return;
  }
  //
  aST=myShape.ShapeType();
  if (!(aST==TopAbs_SOLID || aST==TopAbs_SHELL)) {
    MESSAGE("StdMeshers_Penta_3D::CheckData() ");
    myErrorStatus->myName=3; // not compatible type of shape
    myErrorStatus->myComment=SMESH_Comment("Wrong shape type (TopAbs_ShapeEnum) ")<<aST;
    return;
  }
  //
  for (i=0; i<3; ++i) {
    aM.Clear();
    TopExp::MapShapes(myShape, aSTEx[i], aM);
    iNb=aM.Extent();
    if (iNb!=iNbEx[i]){
      MESSAGE("StdMeshers_Penta_3D::CheckData() ");
      myErrorStatus->myName=4; // number of sub-shape is not compatible
      myErrorStatus->myComment="Wrong number of sub-shapes of a block";
      return;
    }
  }
}

//=======================================================================
//function : LoadIJNodes
//purpose  : Load nodes bound to theFace into column (vectors) and rows
//           of theIJNodes.
//           The value of theIJNodes map is a vector of ordered nodes so
//           that the 0-the one lies on theBaseEdge.
//           The key of theIJNodes map is a normalized parameter of each
//           0-the node on theBaseEdge.
//=======================================================================

bool StdMeshers_Penta_3D::LoadIJNodes(StdMeshers_IJNodeMap & theIJNodes,
                                      const TopoDS_Face&     theFace,
                                      const TopoDS_Edge&     theBaseEdge,
                                      SMESHDS_Mesh*          theMesh)
{
  // get vertices of theBaseEdge
  TopoDS_Vertex vfb, vlb, vft; // first and last, bottom and top vertices
  TopoDS_Edge eFrw = TopoDS::Edge( theBaseEdge.Oriented( TopAbs_FORWARD ));
  TopExp::Vertices( eFrw, vfb, vlb );

  // find the other edges of theFace and orientation of e1
  TopoDS_Edge e1, e2, eTop;
  bool rev1, CumOri = false;
  TopExp_Explorer exp( theFace, TopAbs_EDGE );
  int nbEdges = 0;
  for ( ; exp.More(); exp.Next() ) {
    if ( ++nbEdges > NB_WALL_FACES ) {
      return false; // more than 4 edges in theFace
    }
    TopoDS_Edge e = TopoDS::Edge( exp.Current() );
    if ( theBaseEdge.IsSame( e ))
      continue;
    TopoDS_Vertex vCommon;
    if ( !TopExp::CommonVertex( theBaseEdge, e, vCommon ))
      eTop = e;
    else if ( vCommon.IsSame( vfb )) {
      e1 = e;
      vft = TopExp::LastVertex( e1, CumOri );
      rev1 = vfb.IsSame( vft );
      if ( rev1 )
        vft = TopExp::FirstVertex( e1, CumOri );
    }
    else
      e2 = e;
  }
  if ( nbEdges < NB_WALL_FACES ) {
    return false; // less than 4 edges in theFace
  }

  // submeshes corresponding to shapes
  SMESHDS_SubMesh* smFace = theMesh->MeshElements( theFace );
  SMESHDS_SubMesh* smb = theMesh->MeshElements( theBaseEdge );
  SMESHDS_SubMesh* smt = theMesh->MeshElements( eTop );
  SMESHDS_SubMesh* sm1 = theMesh->MeshElements( e1 );
  SMESHDS_SubMesh* sm2 = theMesh->MeshElements( e2 );
  SMESHDS_SubMesh* smVfb = theMesh->MeshElements( vfb );
  SMESHDS_SubMesh* smVlb = theMesh->MeshElements( vlb );
  SMESHDS_SubMesh* smVft = theMesh->MeshElements( vft );
  if (!smFace || !smb || !smt || !sm1 || !sm2 || !smVfb || !smVlb || !smVft ) {
    MESSAGE( "NULL submesh " <<smFace<<" "<<smb<<" "<<smt<<" "<<
             sm1<<" "<<sm2<<" "<<smVfb<<" "<<smVlb<<" "<<smVft);
    return false;
  }
  if ( smb->NbNodes() != smt->NbNodes() || sm1->NbNodes() != sm2->NbNodes() ) {
    MESSAGE(" Diff nb of nodes on opposite edges" );
    return false;
  }
  if (smVfb->NbNodes() != 1 || smVlb->NbNodes() != 1 || smVft->NbNodes() != 1) {
    MESSAGE("Empty submesh of vertex");
    return false;
  }
  if ( sm1->NbNodes() * smb->NbNodes() != smFace->NbNodes() ) {
    // check quadratic case
    if ( myCreateQuadratic ) {
      int n1 = sm1->NbNodes()/2;
      int n2 = smb->NbNodes()/2;
      int n3 = sm1->NbNodes() - n1;
      int n4 = smb->NbNodes() - n2;
      int nf = sm1->NbNodes()*smb->NbNodes() - n3*n4;
      if( nf != smFace->NbNodes() ) {
        MESSAGE( "Wrong nb face nodes: " <<
                 sm1->NbNodes()<<" "<<smb->NbNodes()<<" "<<smFace->NbNodes());
        return false;
      }
    }
    else {
      MESSAGE( "Wrong nb face nodes: " <<
               sm1->NbNodes()<<" "<<smb->NbNodes()<<" "<<smFace->NbNodes());
      return false;
    }
  }
  // IJ size
  int vsize = sm1->NbNodes() + 2;
  int hsize = smb->NbNodes() + 2;
  if(myCreateQuadratic) {
    vsize = vsize - sm1->NbNodes()/2 -1;
    hsize = hsize - smb->NbNodes()/2 -1;
  }

  // load nodes from theBaseEdge

  set<const SMDS_MeshNode*> loadedNodes;
  const SMDS_MeshNode* nullNode = 0;

  vector<const SMDS_MeshNode*> & nVecf = theIJNodes[ 0.];
  nVecf.resize( vsize, nullNode );
  loadedNodes.insert( nVecf[ 0 ] = smVfb->GetNodes()->next() );

  vector<const SMDS_MeshNode*> & nVecl = theIJNodes[ 1.];
  nVecl.resize( vsize, nullNode );
  loadedNodes.insert( nVecl[ 0 ] = smVlb->GetNodes()->next() );

  double f, l;
  BRep_Tool::Range( eFrw, f, l );
  double range = l - f;
  SMDS_NodeIteratorPtr nIt = smb->GetNodes();
  const SMDS_MeshNode* node;
  while ( nIt->more() ) {
    node = nIt->next();
    if(myTool->IsMedium(node))
      continue;
    const SMDS_EdgePosition* pos =
      dynamic_cast<const SMDS_EdgePosition*>( node->GetPosition() );
    if ( !pos ) {
      return false;
    }
    double u = ( pos->GetUParameter() - f ) / range;
    vector<const SMDS_MeshNode*> & nVec = theIJNodes[ u ];
    nVec.resize( vsize, nullNode );
    loadedNodes.insert( nVec[ 0 ] = node );
  }
  if ( theIJNodes.size() != hsize ) {
    MESSAGE( "Wrong node positions on theBaseEdge" );
    return false;
  }

  // load nodes from e1

  map< double, const SMDS_MeshNode*> sortedNodes; // sort by param on edge
  nIt = sm1->GetNodes();
  while ( nIt->more() ) {
    node = nIt->next();
    if(myTool->IsMedium(node))
      continue;
    const SMDS_EdgePosition* pos =
      dynamic_cast<const SMDS_EdgePosition*>( node->GetPosition() );
    if ( !pos ) {
      return false;
    }
    sortedNodes.insert( make_pair( pos->GetUParameter(), node ));
  }
  loadedNodes.insert( nVecf[ vsize - 1 ] = smVft->GetNodes()->next() );
  map< double, const SMDS_MeshNode*>::iterator u_n = sortedNodes.begin();
  int row = rev1 ? vsize - 1 : 0;
  for ( ; u_n != sortedNodes.end(); u_n++ ) {
    if ( rev1 ) row--;
    else        row++;
    loadedNodes.insert( nVecf[ row ] = u_n->second );
  }

  // try to load the rest nodes

  // get all faces from theFace
  TIDSortedElemSet allFaces, foundFaces;
  SMDS_ElemIteratorPtr eIt = smFace->GetElements();
  while ( eIt->more() ) {
    const SMDS_MeshElement* e = eIt->next();
    if ( e->GetType() == SMDSAbs_Face )
      allFaces.insert( e );
  }
  // Starting from 2 neighbour nodes on theBaseEdge, look for a face
  // the nodes belong to, and between the nodes of the found face,
  // look for a not loaded node considering this node to be the next
  // in a column of the starting second node. Repeat, starting
  // from nodes next to the previous starting nodes in their columns,
  // and so on while a face can be found. Then go the the next pair
  // of nodes on theBaseEdge.
  StdMeshers_IJNodeMap::iterator par_nVec_1 = theIJNodes.begin();
  StdMeshers_IJNodeMap::iterator par_nVec_2 = par_nVec_1;
  // loop on columns
  int col = 0;
  for ( par_nVec_2++; par_nVec_2 != theIJNodes.end(); par_nVec_1++, par_nVec_2++ ) {
    col++;
    row = 0;
    const SMDS_MeshNode* n1 = par_nVec_1->second[ row ];
    const SMDS_MeshNode* n2 = par_nVec_2->second[ row ];
    const SMDS_MeshElement* face = 0;
    do {
      // look for a face by 2 nodes
      face = SMESH_MeshAlgos::FindFaceInSet( n1, n2, allFaces, foundFaces );
      if ( face ) {
        int nbFaceNodes = face->NbNodes();
        if ( (!myCreateQuadratic && nbFaceNodes>4) ||
             (myCreateQuadratic && nbFaceNodes>8) ) {
          MESSAGE(" Too many nodes in a face: " << nbFaceNodes );
          return false;
        }
        // look for a not loaded node of the <face>
        bool found = false;
        const SMDS_MeshNode* n3 = 0; // a node defferent from n1 and n2
        eIt = face->nodesIterator() ;
        while ( !found && eIt->more() ) {
          node = static_cast<const SMDS_MeshNode*>( eIt->next() );
          if(myTool->IsMedium(node))
            continue;
          found = loadedNodes.insert( node ).second;
          if ( !found && node != n1 && node != n2 )
            n3 = node;
        }
        if ( found ) {
          if ( ++row > vsize - 1 ) {
            MESSAGE( "Too many nodes in column "<< col <<": "<< row+1);
            return false;
          }
          par_nVec_2->second[ row ] = node;
          foundFaces.insert( face );
          n2 = node;
          if ( nbFaceNodes==4 || (myCreateQuadratic && nbFaceNodes==8) ) {
            n1 = par_nVec_1->second[ row ];
          }
        }
        else if ( (nbFaceNodes==3 || (myCreateQuadratic && nbFaceNodes==6) )  &&
                  n3 == par_nVec_1->second[ row ] ) {
          n1 = n3;
        }
        else {
          MESSAGE( "Not quad mesh, column "<< col );
          return false;
        }
      }
    }
    while ( face && n1 && n2 );

    if ( row < vsize - 1 ) {
      MESSAGE( "Too few nodes in column "<< col <<": "<< row+1);
      MESSAGE( "Base node 1: "<< par_nVec_1->second[0]);
      MESSAGE( "Base node 2: "<< par_nVec_2->second[0]);
      MESSAGE( "Current node 1: "<< n1);
      MESSAGE( "Current node 2: "<< n2);
      MESSAGE( "first base node: "<< theIJNodes.begin()->second[0]);
      MESSAGE( "last base node: "<< theIJNodes.rbegin()->second[0]);
      return false;
    }
  } // loop on columns

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
//   StdMeshers_SMESHBlock
//
//////////////////////////////////////////////////////////////////////////

//=======================================================================
//function : StdMeshers_SMESHBlock
//purpose  : 
//=======================================================================
StdMeshers_SMESHBlock::StdMeshers_SMESHBlock()
{
  myErrorStatus=1;
  myIsEdgeForward.resize( SMESH_Block::NbEdges(), -1 );
}

//=======================================================================
//function : IsForwadEdge
//purpose  : 
//=======================================================================

bool StdMeshers_SMESHBlock::IsForwadEdge(const int theEdgeID)
{
  int index = myTBlock.ShapeIndex( theEdgeID );
  if ( !myTBlock.IsEdgeID( theEdgeID ))
    return false;

  if ( myIsEdgeForward[ index ] < 0 )
    myIsEdgeForward[ index ] =
      myTBlock.IsForwardEdge( TopoDS::Edge( Shape( theEdgeID )), myShapeIDMap );

  return myIsEdgeForward[ index ];
}

//=======================================================================
//function : ErrorStatus
//purpose  : 
//=======================================================================
int StdMeshers_SMESHBlock::ErrorStatus() const
{
  return myErrorStatus;
}

//================================================================================
/*!
 * \brief Return problem description
 */
//================================================================================

SMESH_ComputeErrorPtr StdMeshers_SMESHBlock::GetError() const
{
  SMESH_ComputeErrorPtr err = SMESH_ComputeError::New();
  string & text = err->myComment;
  switch ( myErrorStatus ) {
  case 2:
  case 3: text = "Internal error of StdMeshers_Penta_3D"; break; 
  case 4: text = "Can't compute normalized parameters of a point inside a block"; break;
  case 5: text = "Can't compute coordinates by normalized parameters inside a block"; break;
  case 6: text = "Can't detect block sub-shapes. Not a block?"; break;
  }
  if (!text.empty())
    err->myName = myErrorStatus;
  return err;
}

//=======================================================================
//function : Load
//purpose  : 
//=======================================================================
void StdMeshers_SMESHBlock::Load(const TopoDS_Shell& theShell)
{
  TopoDS_Vertex aV000, aV001;
  //
  Load(theShell, aV000, aV001);
}

//=======================================================================
//function : Load
//purpose  : 
//=======================================================================
void StdMeshers_SMESHBlock::Load(const TopoDS_Shell& theShell,
                                 const TopoDS_Vertex& theV000,
                                 const TopoDS_Vertex& theV001)
{
  myErrorStatus=0;
  //
  myShell=theShell;
  //
  bool bOk;
  //
  myShapeIDMap.Clear();  
  bOk = myTBlock.LoadBlockShapes(myShell, theV000, theV001, myShapeIDMap);
  if (!bOk) {
    myErrorStatus=6;
    return;
  }
}

//=======================================================================
//function : ComputeParameters
//purpose  : 
//=======================================================================
void StdMeshers_SMESHBlock::ComputeParameters(const gp_Pnt& thePnt, 
                                              gp_XYZ& theXYZ)
{
  ComputeParameters(thePnt, myShell, theXYZ);
}

//=======================================================================
//function : ComputeParameters
//purpose  : 
//=======================================================================
void StdMeshers_SMESHBlock::ComputeParameters(const gp_Pnt& thePnt,
                                              const TopoDS_Shape& theShape,
                                              gp_XYZ& theXYZ)
{
  myErrorStatus=0;
  //
  int aID;
  bool bOk;
  //
  aID = ShapeID(theShape);
  if (myErrorStatus) {
    return;
  }
  bOk = myTBlock.ComputeParameters(thePnt, theXYZ, aID);
  if (!bOk) {
    myErrorStatus=4; // problems with computation Parameters 
    return;
  }
}

//=======================================================================
//function : ComputeParameters
//purpose  : 
//=======================================================================

void StdMeshers_SMESHBlock::ComputeParameters(const double& theU,
                                              const TopoDS_Shape& theShape,
                                              gp_XYZ& theXYZ)
{
  myErrorStatus=0;
  //
  int aID;
  bool bOk=false;
  //
  aID = ShapeID(theShape);
  if (myErrorStatus) {
    return;
  }
  if ( SMESH_Block::IsEdgeID( aID ))
    bOk = myTBlock.EdgeParameters( aID, theU, theXYZ );
  if (!bOk) {
    myErrorStatus=4; // problems with computation Parameters 
    return;
  }
}

//=======================================================================
//function : Point
//purpose  : 
//=======================================================================
void StdMeshers_SMESHBlock::Point(const gp_XYZ& theParams, gp_Pnt& aP3D)
{
  TopoDS_Shape aS;
  //
  Point(theParams, aS, aP3D);
}

//=======================================================================
//function : Point
//purpose  : 
//=======================================================================
void StdMeshers_SMESHBlock::Point(const gp_XYZ& theParams,
                                  const TopoDS_Shape& theShape,
                                  gp_Pnt& aP3D)
{
  myErrorStatus = 0;
  //
  int aID;
  bool bOk = false;
  gp_XYZ aXYZ(99.,99.,99.);
  aP3D.SetXYZ(aXYZ);
  //
  if (theShape.IsNull()) {
    bOk = myTBlock.ShellPoint(theParams, aXYZ);
  }
  //
  else {
    aID=ShapeID(theShape);
    if (myErrorStatus) {
      return;
    }
    //
    if (SMESH_Block::IsVertexID(aID)) {
      bOk = myTBlock.VertexPoint(aID, aXYZ);
    }
    else if (SMESH_Block::IsEdgeID(aID)) {
      bOk = myTBlock.EdgePoint(aID, theParams, aXYZ);
    }
    //
    else if (SMESH_Block::IsFaceID(aID)) {
      bOk = myTBlock.FacePoint(aID, theParams, aXYZ);
    }
  }
  if (!bOk) {
    myErrorStatus=5; // problems with point computation 
    return;
  }
  aP3D.SetXYZ(aXYZ);
}

//=======================================================================
//function : ShapeID
//purpose  : 
//=======================================================================
int StdMeshers_SMESHBlock::ShapeID(const TopoDS_Shape& theShape)
{
  myErrorStatus=0;
  //
  int aID=-1;
  TopoDS_Shape aSF, aSR;
  //
  aSF=theShape;
  aSF.Orientation(TopAbs_FORWARD);
  aSR=theShape;
  aSR.Orientation(TopAbs_REVERSED);
  //
  if (myShapeIDMap.Contains(aSF)) {
    aID=myShapeIDMap.FindIndex(aSF);
    return aID;
  }
  if (myShapeIDMap.Contains(aSR)) {
    aID=myShapeIDMap.FindIndex(aSR);
    return aID;
  }
  myErrorStatus=2; // unknown shape;
  return aID;
}

//=======================================================================
//function : Shape
//purpose  : 
//=======================================================================
const TopoDS_Shape& StdMeshers_SMESHBlock::Shape(const int theID)
{
  myErrorStatus=0;
  //
  int aNb;
  //
  aNb=myShapeIDMap.Extent();
  if (theID<1 || theID>aNb) {
    myErrorStatus=3; // ID is out of range
    return myEmptyShape;
  }
  //
  const TopoDS_Shape& aS=myShapeIDMap.FindKey(theID);
  return aS;
}


//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================
bool StdMeshers_Penta_3D::Evaluate(SMESH_Mesh& aMesh, 
                                   const TopoDS_Shape& aShape,
                                   MapShapeNbElems& aResMap)
{
  MESSAGE("StdMeshers_Penta_3D::Evaluate()");

  // find face contains only triangles
  vector < SMESH_subMesh * >meshFaces;
  TopTools_SequenceOfShape aFaces;
  int NumBase = 0, i = 0;
  for (TopExp_Explorer exp(aShape, TopAbs_FACE); exp.More(); exp.Next()) {
    i++;
    aFaces.Append(exp.Current());
    SMESH_subMesh *aSubMesh = aMesh.GetSubMesh(exp.Current());
    meshFaces.push_back(aSubMesh);
    MapShapeNbElemsItr anIt = aResMap.find(meshFaces[i]);
    if( anIt == aResMap.end() ) {
      NumBase = 0;
      break;
    }
    std::vector<int> aVec = (*anIt).second;
    int nbtri = Max(aVec[SMDSEntity_Triangle],aVec[SMDSEntity_Quad_Triangle]);
    int nbqua = Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
    if( nbtri>0 && nbqua==0 ) {
      NumBase = i;
    }
  }

  if(NumBase==0) {
    std::vector<int> aResVec(SMDSEntity_Last);
    for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
    SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
    aResMap.insert(std::make_pair(sm,aResVec));
    myErrorStatus->myName    = COMPERR_ALGO_FAILED;
    myErrorStatus->myComment = "Submesh can not be evaluated";
    return false;
  }

  // find number of 1d elems for base face
  int nb1d = 0;
  TopTools_MapOfShape Edges1;
  for (TopExp_Explorer exp(aFaces.Value(NumBase), TopAbs_EDGE); exp.More(); exp.Next()) {
    Edges1.Add(exp.Current());
    SMESH_subMesh *sm = aMesh.GetSubMesh(exp.Current());
    if( sm ) {
      MapShapeNbElemsItr anIt = aResMap.find(sm);
      if( anIt == aResMap.end() ) continue;
      std::vector<int> aVec = (*anIt).second;
      nb1d += Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
    }
  }
  // find face opposite to base face
  int OppNum = 0;
  for(i=1; i<=6; i++) {
    if(i==NumBase) continue;
    bool IsOpposite = true;
    for(TopExp_Explorer exp(aFaces.Value(i), TopAbs_EDGE); exp.More(); exp.Next()) {
      if( Edges1.Contains(exp.Current()) ) {
        IsOpposite = false;
        break;
      }
    }
    if(IsOpposite) {
      OppNum = i;
      break;
    }
  }
  // find number of 2d elems on side faces
  int nb2d = 0;
  for(i=1; i<=6; i++) {
    if( i==OppNum || i==NumBase ) continue;
    MapShapeNbElemsItr anIt = aResMap.find( meshFaces[i-1] );
    if( anIt == aResMap.end() ) continue;
    std::vector<int> aVec = (*anIt).second;
    nb2d += Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
  }

  MapShapeNbElemsItr anIt = aResMap.find( meshFaces[NumBase-1] );
  std::vector<int> aVec = (*anIt).second;
  int nb2d_face0 = Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
  int nb0d_face0 = aVec[SMDSEntity_Node];

  anIt = aResMap.find( meshFaces[OppNum-1] );
  for(i=SMDSEntity_Node; i<SMDSEntity_Last; i++)
    (*anIt).second[i] = aVec[i];

  SMESH_MesherHelper aTool (aMesh);
  bool _quadraticMesh = aTool.IsQuadraticSubMesh(aShape);

  std::vector<int> aResVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
  if(_quadraticMesh) {
    aResVec[SMDSEntity_Quad_Penta] = nb2d_face0 * ( nb2d/nb1d );
    aResVec[SMDSEntity_Node] = nb0d_face0 * ( 2*nb2d/nb1d - 1 );
  }
  else {
    aResVec[SMDSEntity_Node] = nb0d_face0 * ( nb2d/nb1d - 1 );
    aResVec[SMDSEntity_Penta] = nb2d_face0 * ( nb2d/nb1d );
  }
  SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
  aResMap.insert(std::make_pair(sm,aResVec));

  return true;
}

