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
// File      : StdMeshers_Prism_3D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_Prism_3D.hxx"

#include "StdMeshers_ProjectionUtils.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMDS_VolumeOfNodes.hxx"
#include "SMDS_EdgePosition.hxx"
#include "SMESH_Comment.hxx"

#include "utilities.h"

#include <BRep_Tool.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2d_Line.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>

using namespace std;

#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }
#define gpXYZ(n) gp_XYZ(n->X(),n->Y(),n->Z())
#define SHOWYXZ(msg, xyz) // {\
// gp_Pnt p (xyz); \
// cout << msg << " ("<< p.X() << "; " <<p.Y() << "; " <<p.Z() << ") " <<endl;\
// }

typedef StdMeshers_ProjectionUtils TAssocTool;
typedef SMESH_Comment              TCom;

enum { ID_BOT_FACE = SMESH_Block::ID_Fxy0,
       ID_TOP_FACE = SMESH_Block::ID_Fxy1,
       BOTTOM_EDGE = 0, TOP_EDGE, V0_EDGE, V1_EDGE, // edge IDs in face
       NB_WALL_FACES = 4 }; //

namespace {

  //================================================================================
  /*!
   * \brief Return iterator pointing to node column for the given parameter
   * \param columnsMap - node column map
   * \param parameter - parameter
   * \retval TParam2ColumnMap::iterator - result
   *
   * it returns closest left column
   */
  //================================================================================

  TParam2ColumnIt getColumn( const TParam2ColumnMap* columnsMap,
                             const double            parameter )
  {
    TParam2ColumnIt u_col = columnsMap->upper_bound( parameter );
    if ( u_col != columnsMap->begin() )
      --u_col;
    return u_col; // return left column
  }

  //================================================================================
  /*!
   * \brief Return nodes around given parameter and a ratio
   * \param column - node column
   * \param param - parameter
   * \param node1 - lower node
   * \param node2 - upper node
   * \retval double - ratio
   */
  //================================================================================

  double getRAndNodes( const TNodeColumn*     column,
                       const double           param,
                       const SMDS_MeshNode* & node1,
                       const SMDS_MeshNode* & node2)
  {
    if ( param >= 1.0 || column->size() == 1) {
      node1 = node2 = column->back();
      return 0;
    }

    int i = int( param * ( column->size() - 1 ));
    double u0 = double( i )/ double( column->size() - 1 );
    double r = ( param - u0 ) * ( column->size() - 1 );

    node1 = (*column)[ i ];
    node2 = (*column)[ i + 1];
    return r;
  }

  //================================================================================
  /*!
   * \brief Compute boundary parameters of face parts
    * \param nbParts - nb of parts to split columns into
    * \param columnsMap - node columns of the face to split
    * \param params - computed parameters
   */
  //================================================================================

  void splitParams( const int               nbParts,
                    const TParam2ColumnMap* columnsMap,
                    vector< double > &      params)
  {
    params.clear();
    params.reserve( nbParts + 1 );
    TParam2ColumnIt last_par_col = --columnsMap->end();
    double par = columnsMap->begin()->first; // 0.
    double parLast = last_par_col->first;
    params.push_back( par );
    for ( int i = 0; i < nbParts - 1; ++ i )
    {
      double partSize = ( parLast - par ) / double ( nbParts - i );
      TParam2ColumnIt par_col = getColumn( columnsMap, par + partSize );
      if ( par_col->first == par ) {
        ++par_col;
        if ( par_col == last_par_col ) {
          while ( i < nbParts - 1 )
            params.push_back( par + partSize * i++ );
          break;
        }
      }
      par = par_col->first;
      params.push_back( par );
    }
    params.push_back( parLast ); // 1.
  }
}

//=======================================================================
//function : StdMeshers_Prism_3D
//purpose  : 
//=======================================================================

StdMeshers_Prism_3D::StdMeshers_Prism_3D(int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_3D_Algo(hypId, studyId, gen)
{
  _name = "Prism_3D";
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);	// 1 bit per shape type
  myProjectTriangles = false;
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_Prism_3D::~StdMeshers_Prism_3D()
{}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_Prism_3D::CheckHypothesis(SMESH_Mesh&                          aMesh,
                                          const TopoDS_Shape&                  aShape,
                                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  // Check shape geometry
/*  PAL16229
  aStatus = SMESH_Hypothesis::HYP_BAD_GEOMETRY;

  // find not quadrangle faces
  list< TopoDS_Shape > notQuadFaces;
  int nbEdge, nbWire, nbFace = 0;
  TopExp_Explorer exp( aShape, TopAbs_FACE );
  for ( ; exp.More(); exp.Next() ) {
    ++nbFace;
    const TopoDS_Shape& face = exp.Current();
    nbEdge = TAssocTool::Count( face, TopAbs_EDGE, 0 );
    nbWire = TAssocTool::Count( face, TopAbs_WIRE, 0 );
    if (  nbEdge!= 4 || nbWire!= 1 ) {
      if ( !notQuadFaces.empty() ) {
        if ( TAssocTool::Count( notQuadFaces.back(), TopAbs_EDGE, 0 ) != nbEdge ||
             TAssocTool::Count( notQuadFaces.back(), TopAbs_WIRE, 0 ) != nbWire )
          RETURN_BAD_RESULT("Different not quad faces");
      }
      notQuadFaces.push_back( face );
    }
  }
  if ( !notQuadFaces.empty() )
  {
    if ( notQuadFaces.size() != 2 )
      RETURN_BAD_RESULT("Bad nb not quad faces: " << notQuadFaces.size());

    // check total nb faces
    nbEdge = TAssocTool::Count( notQuadFaces.back(), TopAbs_EDGE, 0 );
    if ( nbFace != nbEdge + 2 )
      RETURN_BAD_RESULT("Bad nb of faces: " << nbFace << " but must be " << nbEdge + 2);
  }
*/
  // no hypothesis
  aStatus = SMESH_Hypothesis::HYP_OK;
  return true;
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_Prism_3D::Compute(SMESH_Mesh& theMesh, const TopoDS_Shape& theShape)
{
  SMESH_MesherHelper helper( theMesh );
  myHelper = &helper;

  myHelper->IsQuadraticSubMesh( theShape );

  // Analyse mesh and geomerty to find block subshapes and submeshes
  if ( !myBlock.Init( myHelper, theShape ))
    return error( myBlock.GetError());

  SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();

  int volumeID = meshDS->ShapeToIndex( theShape );


  // To compute coordinates of a node inside a block, it is necessary to know
  // 1. normalized parameters of the node by which
  // 2. coordinates of node projections on all block sub-shapes are computed

  // So we fill projections on vertices at once as they are same for all nodes
  myShapeXYZ.resize( myBlock.NbSubShapes() );
  for ( int iV = SMESH_Block::ID_FirstV; iV < SMESH_Block::ID_FirstE; ++iV ) {
    myBlock.VertexPoint( iV, myShapeXYZ[ iV ]);
    SHOWYXZ("V point " <<iV << " ", myShapeXYZ[ iV ]);
  }

  // Projections on the top and bottom faces are taken from nodes existing
  // on these faces; find correspondence between bottom and top nodes
  myBotToColumnMap.clear();
  if ( !assocOrProjBottom2Top() ) // it also fill myBotToColumnMap
    return false;


  // Create nodes inside the block

  // loop on nodes inside the bottom face
  TNode2ColumnMap::iterator bot_column = myBotToColumnMap.begin();
  for ( ; bot_column != myBotToColumnMap.end(); ++bot_column )
  {
    const TNode& tBotNode = bot_column->first; // bottom TNode
    if ( tBotNode.GetPositionType() != SMDS_TOP_FACE )
      continue; // node is not inside face 

    // column nodes; middle part of the column are zero pointers
    TNodeColumn& column = bot_column->second;

    // bottom node parameters and coords
    myShapeXYZ[ ID_BOT_FACE ] = tBotNode.GetCoords();
    gp_XYZ botParams          = tBotNode.GetParams();

    // compute top node parameters
    myShapeXYZ[ ID_TOP_FACE ] = gpXYZ( column.back() );
    gp_XYZ topParams = botParams;
    topParams.SetZ( 1 );
    if ( column.size() > 2 ) {
      gp_Pnt topCoords = myShapeXYZ[ ID_TOP_FACE ];
      if ( !myBlock.ComputeParameters( topCoords, topParams, ID_TOP_FACE, topParams ))
        return error(TCom("Can't compute normalized parameters ")
                     << "for node " << column.back()->GetID()
                     << " on the face #"<< column.back()->GetPosition()->GetShapeId() );
    }

    // vertical loop
    TNodeColumn::iterator columnNodes = column.begin();
    for ( int z = 0; columnNodes != column.end(); ++columnNodes, ++z)
    {
      const SMDS_MeshNode* & node = *columnNodes;
      if ( node ) continue; // skip bottom or top node

      // params of a node to create
      double rz = (double) z / (double) ( column.size() - 1 );
      gp_XYZ params = botParams * ( 1 - rz ) + topParams * rz;

      // set coords on all faces and nodes
      const int nbSideFaces = 4;
      int sideFaceIDs[nbSideFaces] = { SMESH_Block::ID_Fx0z,
                                       SMESH_Block::ID_Fx1z,
                                       SMESH_Block::ID_F0yz,
                                       SMESH_Block::ID_F1yz };
      for ( int iF = 0; iF < nbSideFaces; ++iF )
        if ( !setFaceAndEdgesXYZ( sideFaceIDs[ iF ], params, z ))
          return false;

      // compute coords for a new node
      gp_XYZ coords;
      if ( !SMESH_Block::ShellPoint( params, myShapeXYZ, coords ))
        return error("Can't compute coordinates by normalized parameters");

      SHOWYXZ("TOPFacePoint ",myShapeXYZ[ ID_TOP_FACE]);
      SHOWYXZ("BOT Node "<< tBotNode.myNode->GetID(),gpXYZ(tBotNode.myNode));
      SHOWYXZ("ShellPoint ",coords);

      // create a node
      node = meshDS->AddNode( coords.X(), coords.Y(), coords.Z() );
      meshDS->SetNodeInVolume( node, volumeID );
    }
  } // loop on bottom nodes


  // Create volumes

  SMESHDS_SubMesh* smDS = myBlock.SubMeshDS( ID_BOT_FACE );
  if ( !smDS ) return error(COMPERR_BAD_INPUT_MESH, "Null submesh");

  // loop on bottom mesh faces
  SMDS_ElemIteratorPtr faceIt = smDS->GetElements();
  while ( faceIt->more() )
  {
    const SMDS_MeshElement* face = faceIt->next();
    if ( !face || face->GetType() != SMDSAbs_Face )
      continue;
    int nbNodes = face->NbNodes();
    if ( face->IsQuadratic() )
      nbNodes /= 2;

    // find node columns for each node
    vector< const TNodeColumn* > columns( nbNodes );
    for ( int i = 0; i < nbNodes; ++i )
    {
      const SMDS_MeshNode* n = face->GetNode( i );
      if ( n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE ) {
        bot_column = myBotToColumnMap.find( n );
        if ( bot_column == myBotToColumnMap.end() )
          return error(TCom("No nodes found above node ") << n->GetID() );
        columns[ i ] = & bot_column->second;
      }
      else {
        columns[ i ] = myBlock.GetNodeColumn( n );
        if ( !columns[ i ] )
          return error(TCom("No side nodes found above node ") << n->GetID() );
      }
    }
    // create prisms
    AddPrisms( columns, myHelper );

  } // loop on bottom mesh faces
        
  return true;
}

//================================================================================
/*!
 * \brief Create prisms
 * \param columns - columns of nodes generated from nodes of a mesh face
 * \param helper - helper initialized by mesh and shape to add prisms to
 */
//================================================================================

void StdMeshers_Prism_3D::AddPrisms( vector<const TNodeColumn*> & columns,
                                     SMESH_MesherHelper*          helper)
{
  SMESHDS_Mesh * meshDS = helper->GetMeshDS();
  int shapeID = helper->GetSubShapeID();

  int nbNodes = columns.size();
  int nbZ     = columns[0]->size();
  if ( nbZ < 2 ) return;

  // find out orientation
  bool isForward = true;
  SMDS_VolumeTool vTool;
  int z = 1;
  switch ( nbNodes ) {
  case 3: {
    const SMDS_MeshNode* botNodes[3] = { (*columns[0])[z-1],
                                         (*columns[1])[z-1],
                                         (*columns[2])[z-1] };
    const SMDS_MeshNode* topNodes[3] = { (*columns[0])[z],
                                         (*columns[1])[z],
                                         (*columns[2])[z] };
    SMDS_VolumeOfNodes tmpVol ( botNodes[0], botNodes[1], botNodes[2],
                                topNodes[0], topNodes[1], topNodes[2]);
    vTool.Set( &tmpVol );
    isForward  = vTool.IsForward();
    break;
  }
  case 4: {
    const SMDS_MeshNode* botNodes[4] = { (*columns[0])[z-1], (*columns[1])[z-1],
                                         (*columns[2])[z-1], (*columns[3])[z-1] };
    const SMDS_MeshNode* topNodes[4] = { (*columns[0])[z], (*columns[1])[z],
                                         (*columns[2])[z], (*columns[3])[z] };
    SMDS_VolumeOfNodes tmpVol ( botNodes[0], botNodes[1], botNodes[2], botNodes[3],
                                topNodes[0], topNodes[1], topNodes[2], topNodes[3]);
    vTool.Set( &tmpVol );
    isForward  = vTool.IsForward();
    break;
  }
  }

  // vertical loop on columns
  for ( z = 1; z < nbZ; ++z )
  {
    SMDS_MeshElement* vol = 0;
    switch ( nbNodes ) {

    case 3: {
      const SMDS_MeshNode* botNodes[3] = { (*columns[0])[z-1],
                                           (*columns[1])[z-1],
                                           (*columns[2])[z-1] };
      const SMDS_MeshNode* topNodes[3] = { (*columns[0])[z],
                                           (*columns[1])[z],
                                           (*columns[2])[z] };
      if ( isForward )
        vol = helper->AddVolume( botNodes[0], botNodes[1], botNodes[2],
                                 topNodes[0], topNodes[1], topNodes[2]);
      else
        vol = helper->AddVolume( topNodes[0], topNodes[1], topNodes[2],
                                 botNodes[0], botNodes[1], botNodes[2]);
      break;
      }
    case 4: {
      const SMDS_MeshNode* botNodes[4] = { (*columns[0])[z-1], (*columns[1])[z-1],
                                           (*columns[2])[z-1], (*columns[3])[z-1] };
      const SMDS_MeshNode* topNodes[4] = { (*columns[0])[z], (*columns[1])[z],
                                           (*columns[2])[z], (*columns[3])[z] };
      if ( isForward )
        vol = helper->AddVolume( botNodes[0], botNodes[1], botNodes[2], botNodes[3],
                                 topNodes[0], topNodes[1], topNodes[2], topNodes[3]);
      else
        vol = helper->AddVolume( topNodes[0], topNodes[1], topNodes[2], topNodes[3],
                                 botNodes[0], botNodes[1], botNodes[2], botNodes[3]);
      break;
      }
    default:
      // polyhedron
      vector<const SMDS_MeshNode*> nodes( 2*nbNodes + 4*nbNodes);
      vector<int> quantities( 2 + nbNodes, 4 );
      quantities[0] = quantities[1] = nbNodes;
      columns.resize( nbNodes + 1 );
      columns[ nbNodes ] = columns[ 0 ];
      for ( int i = 0; i < nbNodes; ++i ) {
        nodes[ i         ] = (*columns[ i ])[z-1]; // bottom
        nodes[ i+nbNodes ] = (*columns[ i ])[z  ]; // top
        // side
        int di = 2*nbNodes + 4*i - 1;
        nodes[ di   ] = (*columns[i  ])[z-1];
        nodes[ di+1 ] = (*columns[i+1])[z-1];
        nodes[ di+2 ] = (*columns[i+1])[z  ];
        nodes[ di+3 ] = (*columns[i  ])[z  ];
      }
      vol = meshDS->AddPolyhedralVolume( nodes, quantities );
    }
    if ( vol && shapeID > 0 )
      meshDS->SetMeshElementOnShape( vol, shapeID );
  }
}

//================================================================================
/*!
 * \brief Find correspondence between bottom and top nodes
 *  If elements on the bottom and top faces are topologically different,
 *  and projection is possible and allowed, perform the projection
 *  \retval bool - is a success or not
 */
//================================================================================

bool StdMeshers_Prism_3D::assocOrProjBottom2Top()
{
  SMESH_subMesh * botSM = myBlock.SubMesh( ID_BOT_FACE );
  SMESH_subMesh * topSM = myBlock.SubMesh( ID_TOP_FACE );

  SMESHDS_SubMesh * botSMDS = botSM->GetSubMeshDS();
  SMESHDS_SubMesh * topSMDS = topSM->GetSubMeshDS();

  if ( !botSMDS || botSMDS->NbElements() == 0 )
    return error(TCom("No elememts on face #") << botSM->GetId());

  bool needProject = false;
  if ( !topSMDS || 
       botSMDS->NbElements() != topSMDS->NbElements() ||
       botSMDS->NbNodes()    != topSMDS->NbNodes())
  {
    if ( myBlock.HasNotQuadElemOnTop() )
      return error(TCom("Mesh on faces #") << botSM->GetId()
                   <<" and #"<< topSM->GetId() << " seems different" );
    needProject = true;
  }

  if ( 0/*needProject && !myProjectTriangles*/ )
    return error(TCom("Mesh on faces #") << botSM->GetId()
                 <<" and #"<< topSM->GetId() << " seems different" );
  ///RETURN_BAD_RESULT("Need to project but not allowed");

  if ( needProject )
  {
    return projectBottomToTop();
  }

  TopoDS_Face botFace = TopoDS::Face( myBlock.Shape( ID_BOT_FACE ));
  TopoDS_Face topFace = TopoDS::Face( myBlock.Shape( ID_TOP_FACE ));
  // associate top and bottom faces
  TAssocTool::TShapeShapeMap shape2ShapeMap;
  if ( !TAssocTool::FindSubShapeAssociation( botFace, myBlock.Mesh(),
                                             topFace, myBlock.Mesh(),
                                             shape2ShapeMap) )
    return error(TCom("Topology of faces #") << botSM->GetId()
                 <<" and #"<< topSM->GetId() << " seems different" );

  // Find matching nodes of top and bottom faces
  TNodeNodeMap n2nMap;
  if ( ! TAssocTool::FindMatchingNodesOnFaces( botFace, myBlock.Mesh(),
                                               topFace, myBlock.Mesh(),
                                               shape2ShapeMap, n2nMap ))
    return error(TCom("Mesh on faces #") << botSM->GetId()
                 <<" and #"<< topSM->GetId() << " seems different" );

  // Fill myBotToColumnMap

  int zSize = myBlock.VerticalSize();
  TNode prevTNode;
  TNodeNodeMap::iterator bN_tN = n2nMap.begin();
  for ( ; bN_tN != n2nMap.end(); ++bN_tN )
  {
    const SMDS_MeshNode* botNode = bN_tN->first;
    const SMDS_MeshNode* topNode = bN_tN->second;
    if ( botNode->GetPosition()->GetTypeOfPosition() != SMDS_TOP_FACE )
      continue; // wall columns are contained in myBlock
    // compute bottom node params
    TNode bN( botNode );
    if ( zSize > 2 ) {
      gp_XYZ paramHint(-1,-1,-1);
      if ( prevTNode.IsNeighbor( bN ))
        paramHint = prevTNode.GetParams();
      if ( !myBlock.ComputeParameters( bN.GetCoords(), bN.ChangeParams(),
                                       ID_BOT_FACE, paramHint ))
        return error(TCom("Can't compute normalized parameters for node ")
                     << botNode->GetID() << " on the face #"<< botSM->GetId() );
      prevTNode = bN;
    }
    // create node column
    TNode2ColumnMap::iterator bN_col = 
      myBotToColumnMap.insert( make_pair ( bN, TNodeColumn() )).first;
    TNodeColumn & column = bN_col->second;
    column.resize( zSize );
    column.front() = botNode;
    column.back()  = topNode;
  }
  return true;
}

//================================================================================
/*!
 * \brief Remove quadrangles from the top face and
 * create triangles there by projection from the bottom
 * \retval bool - a success or not
 */
//================================================================================

bool StdMeshers_Prism_3D::projectBottomToTop()
{
  SMESH_subMesh * botSM = myBlock.SubMesh( ID_BOT_FACE );
  SMESH_subMesh * topSM = myBlock.SubMesh( ID_TOP_FACE );

  SMESHDS_SubMesh * botSMDS = botSM->GetSubMeshDS();
  SMESHDS_SubMesh * topSMDS = topSM->GetSubMeshDS();

  if ( topSMDS )
    topSM->ComputeStateEngine( SMESH_subMesh::CLEAN );

  SMESHDS_Mesh* meshDS = myBlock.MeshDS();
  int shapeID = myHelper->GetSubShapeID();
  int topFaceID = meshDS->ShapeToIndex( topSM->GetSubShape() );

  // Fill myBotToColumnMap

  int zSize = myBlock.VerticalSize();
  TNode prevTNode;
  SMDS_NodeIteratorPtr nIt = botSMDS->GetNodes();
  while ( nIt->more() )
  {
    const SMDS_MeshNode* botNode = nIt->next();
    if ( botNode->GetPosition()->GetTypeOfPosition() != SMDS_TOP_FACE )
      continue; // strange
    // compute bottom node params
    TNode bN( botNode );
    gp_XYZ paramHint(-1,-1,-1);
    if ( prevTNode.IsNeighbor( bN ))
      paramHint = prevTNode.GetParams();
    if ( !myBlock.ComputeParameters( bN.GetCoords(), bN.ChangeParams(),
                                     ID_BOT_FACE, paramHint ))
      return error(TCom("Can't compute normalized parameters for node ")
                   << botNode->GetID() << " on the face #"<< botSM->GetId() );
    prevTNode = bN;
    // compute top node coords
    gp_XYZ topXYZ; gp_XY topUV;
    if ( !myBlock.FacePoint( ID_TOP_FACE, bN.GetParams(), topXYZ ) ||
         !myBlock.FaceUV   ( ID_TOP_FACE, bN.GetParams(), topUV ))
      return error(TCom("Can't compute coordinates "
                        "by normalized parameters on the face #")<< topSM->GetId() );
    SMDS_MeshNode * topNode = meshDS->AddNode( topXYZ.X(),topXYZ.Y(),topXYZ.Z() );
    meshDS->SetNodeOnFace( topNode, topFaceID, topUV.X(), topUV.Y() );
    // create node column
    TNode2ColumnMap::iterator bN_col = 
      myBotToColumnMap.insert( make_pair ( bN, TNodeColumn() )).first;
    TNodeColumn & column = bN_col->second;
    column.resize( zSize );
    column.front() = botNode;
    column.back()  = topNode;
  }

  // Create top faces

  // loop on bottom mesh faces
  SMDS_ElemIteratorPtr faceIt = botSMDS->GetElements();
  while ( faceIt->more() )
  {
    const SMDS_MeshElement* face = faceIt->next();
    if ( !face || face->GetType() != SMDSAbs_Face )
      continue;
    int nbNodes = face->NbNodes();
    if ( face->IsQuadratic() )
      nbNodes /= 2;

    // find top node in columns for each bottom node
    vector< const SMDS_MeshNode* > nodes( nbNodes );
    for ( int i = 0; i < nbNodes; ++i )
    {
      const SMDS_MeshNode* n = face->GetNode( nbNodes - i - 1 );
      if ( n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE ) {
        TNode2ColumnMap::iterator bot_column = myBotToColumnMap.find( n );
        if ( bot_column == myBotToColumnMap.end() )
          return error(TCom("No nodes found above node ") << n->GetID() );
        nodes[ i ] = bot_column->second.back();
      }
      else {
        const TNodeColumn* column = myBlock.GetNodeColumn( n );
        if ( !column )
          return error(TCom("No side nodes found above node ") << n->GetID() );
        nodes[ i ] = column->back();
      }
    }
    // create a face, with reversed orientation
    SMDS_MeshElement* newFace = 0;
    switch ( nbNodes ) {

    case 3: {
      newFace = myHelper->AddFace(nodes[0], nodes[1], nodes[2]);
      break;
      }
    case 4: {
      newFace = myHelper->AddFace( nodes[0], nodes[1], nodes[2], nodes[3] );
      break;
      }
    default:
      newFace = meshDS->AddPolygonalFace( nodes );
    }
    if ( newFace && shapeID > 0 )
      meshDS->SetMeshElementOnShape( newFace, shapeID );
  }

  return true;
}

//================================================================================
/*!
 * \brief Set projection coordinates of a node to a face and it's subshapes
 * \param faceID - the face given by in-block ID
 * \param params - node normalized parameters
 * \retval bool - is a success
 */
//================================================================================

bool StdMeshers_Prism_3D::setFaceAndEdgesXYZ( const int faceID, const gp_XYZ& params, int z )
{
  // find base and top edges of the face
  enum { BASE = 0, TOP, LEFT, RIGHT };
  vector< int > edgeVec; // 0-base, 1-top
  SMESH_Block::GetFaceEdgesIDs( faceID, edgeVec );

  myBlock.EdgePoint( edgeVec[ BASE ], params, myShapeXYZ[ edgeVec[ BASE ]]);
  myBlock.EdgePoint( edgeVec[ TOP ], params, myShapeXYZ[ edgeVec[ TOP ]]);

  SHOWYXZ("\nparams ", params);
  SHOWYXZ("TOP is "<<edgeVec[ TOP], myShapeXYZ[ edgeVec[ TOP]]);
  SHOWYXZ("BASE is "<<edgeVec[ BASE], myShapeXYZ[ edgeVec[ BASE]]);

  if ( faceID == SMESH_Block::ID_Fx0z || faceID == SMESH_Block::ID_Fx1z )
  {
    myBlock.EdgePoint( edgeVec[ LEFT ], params, myShapeXYZ[ edgeVec[ LEFT ]]);
    myBlock.EdgePoint( edgeVec[ RIGHT ], params, myShapeXYZ[ edgeVec[ RIGHT ]]);

    SHOWYXZ("VER "<<edgeVec[ LEFT], myShapeXYZ[ edgeVec[ LEFT]]);
    SHOWYXZ("VER "<<edgeVec[ RIGHT], myShapeXYZ[ edgeVec[ RIGHT]]);
  }
  myBlock.FacePoint( faceID, params, myShapeXYZ[ faceID ]);
  SHOWYXZ("FacePoint "<<faceID, myShapeXYZ[ faceID]);

  return true;
}

//================================================================================
/*!
 * \brief Return true if this node and other one belong to one face
 */
//================================================================================

bool TNode::IsNeighbor( const TNode& other ) const
{
  if ( !other.myNode || !myNode ) return false;

  SMDS_ElemIteratorPtr fIt = other.myNode->GetInverseElementIterator(SMDSAbs_Face);
  while ( fIt->more() )
    if ( fIt->next()->GetNodeIndex( myNode ) >= 0 )
      return true;
  return false;
}

//================================================================================
/*!
 * \brief Constructor. Initialization is needed
 */
//================================================================================

StdMeshers_PrismAsBlock::StdMeshers_PrismAsBlock()
{
  mySide = 0;
}

StdMeshers_PrismAsBlock::~StdMeshers_PrismAsBlock()
{
  if ( mySide ) {
    delete mySide; mySide = 0;
  }
}

//================================================================================
/*!
 * \brief Initialization.
 * \param helper - helper loaded with mesh and 3D shape
 * \param shape3D - a closed shell or solid
 * \retval bool - false if a mesh or a shape are KO
 */
//================================================================================

bool StdMeshers_PrismAsBlock::Init(SMESH_MesherHelper* helper,
                                   const TopoDS_Shape& shape3D)
{
  if ( mySide ) {
    delete mySide; mySide = 0;
  }
  vector< TSideFace* > sideFaces( NB_WALL_FACES, 0 );
  vector< pair< double, double> > params ( NB_WALL_FACES );
  mySide = new TSideFace( sideFaces, params );

  myHelper = helper;
  SMESHDS_Mesh* meshDS = myHelper->GetMeshDS();

  SMESH_Block::init();
  myShapeIDMap.Clear();
  myShapeIndex2ColumnMap.clear();
  
  int wallFaceIds[ NB_WALL_FACES ] = { // to walk around a block
    SMESH_Block::ID_Fx0z, SMESH_Block::ID_F1yz,
    SMESH_Block::ID_Fx1z, SMESH_Block::ID_F0yz
  };

  myError = SMESH_ComputeError::New();

  // -------------------------------------------------------------
  // Look for top and bottom faces: not quadrangle ones or meshed
  // with not quadrangle elements
  // -------------------------------------------------------------

  list< SMESH_subMesh* > notQuadGeomSubMesh;
  list< SMESH_subMesh* > notQuadElemSubMesh;
  int nbFaces = 0;
  //
  SMESH_subMesh* mainSubMesh = myHelper->GetMesh()->GetSubMeshContaining( shape3D );
  if ( !mainSubMesh ) return error(COMPERR_BAD_INPUT_MESH,"Null submesh of shape3D");

  // analyse face submeshes
  SMESH_subMeshIteratorPtr smIt = mainSubMesh->getDependsOnIterator(false,false);
  while ( smIt->more() )
  {
    SMESH_subMesh* sm = smIt->next();
    const TopoDS_Shape& face = sm->GetSubShape();
    if ( face.ShapeType() != TopAbs_FACE )
      continue;
    nbFaces++;

    // is quadrangle face?
    list< TopoDS_Edge > orderedEdges;
    list< int >         nbEdgesInWires;
    TopoDS_Vertex       V000;
    int nbWires = GetOrderedEdges( TopoDS::Face( face ),
                                   V000, orderedEdges, nbEdgesInWires );
    if ( nbWires != 1 || nbEdgesInWires.front() != 4 )
      notQuadGeomSubMesh.push_back( sm );

    // look for not quadrangle mesh elements
    if ( SMESHDS_SubMesh* smDS = sm->GetSubMeshDS() ) {
      bool hasNotQuad = false;
      SMDS_ElemIteratorPtr eIt = smDS->GetElements();
      while ( eIt->more() && !hasNotQuad ) {
        const SMDS_MeshElement* elem = eIt->next();
        if ( elem->GetType() == SMDSAbs_Face ) {
          int nbNodes = elem->NbNodes();
          if ( elem->IsQuadratic() )
            nbNodes /= 2;
          hasNotQuad = ( nbNodes != 4 );
        }
      }
      if ( hasNotQuad )
        notQuadElemSubMesh.push_back( sm );
    }
    else {
      return error(COMPERR_BAD_INPUT_MESH,TCom("Not meshed face #")<<sm->GetId());
    }
    // check if a quadrangle face is meshed with a quadranglar grid
    if ( notQuadGeomSubMesh.back() != sm &&
         notQuadElemSubMesh.back() != sm )
    {
      // count nb edges on face sides
      vector< int > nbEdges;
      nbEdges.reserve( nbEdgesInWires.front() );
      for ( list< TopoDS_Edge >::iterator edge = orderedEdges.begin();
            edge != orderedEdges.end(); ++edge )
      {
        if ( SMESHDS_SubMesh* smDS = meshDS->MeshElements( *edge ))
          nbEdges.push_back ( smDS->NbElements() );
        else
          nbEdges.push_back ( 0 );
      }
      int nbQuads = sm->GetSubMeshDS()->NbElements();
      if ( nbEdges[0] *  nbEdges[1] != nbQuads ||
           nbEdges[0] != nbEdges[2] ||
           nbEdges[1] != nbEdges[3] )
        notQuadElemSubMesh.push_back( sm );
    }
  }

  // ----------------------------------------------------------------------
  // Analyse faces mesh and topology: choose the bottom submesh.
  // If there are not quadrangle geom faces, they are top and bottom ones.
  // Not quadrangle geom faces must be only on top and bottom.
  // ----------------------------------------------------------------------

  SMESH_subMesh * botSM = 0;
  SMESH_subMesh * topSM = 0;

  int nbNotQuad       = notQuadGeomSubMesh.size();
  int nbNotQuadMeshed = notQuadElemSubMesh.size();
  bool hasNotQuad = ( nbNotQuad || nbNotQuadMeshed );

  // detect bad cases
  if ( nbNotQuad > 0 && nbNotQuad != 2 )
    return error(COMPERR_BAD_SHAPE,
                 TCom("More than 2 not quadrilateral faces: ")
                 <<nbNotQuad);
  if ( nbNotQuadMeshed > 2 )
    return error(COMPERR_BAD_INPUT_MESH,
                 TCom("More than 2 faces with not quadrangle elements: ")
                 <<nbNotQuadMeshed);

  // get found submeshes
  if ( hasNotQuad )
  {
    if ( nbNotQuadMeshed > 0 ) botSM = notQuadElemSubMesh.front();
    else                       botSM = notQuadGeomSubMesh.front();
    if ( nbNotQuadMeshed > 1 ) topSM = notQuadElemSubMesh.back();
    else if ( nbNotQuad  > 1 ) topSM = notQuadGeomSubMesh.back();
  }
  // detect other bad cases
  if ( nbNotQuad == 2 && nbNotQuadMeshed > 0 ) {
    bool ok = false;
    if ( nbNotQuadMeshed == 1 )
      ok = ( find( notQuadGeomSubMesh.begin(),
                   notQuadGeomSubMesh.end(), botSM ) != notQuadGeomSubMesh.end() );
    else
      ok = ( notQuadGeomSubMesh == notQuadElemSubMesh );
    if ( !ok )
      return error(COMPERR_BAD_INPUT_MESH, "Side face meshed with not quadrangle elements");
  }

  myNotQuadOnTop = ( nbNotQuadMeshed > 1 );
 
  // ----------------------------------------------------------

  if ( nbNotQuad == 0 ) // Standard block of 6 quadrangle faces ?
  {
    // SMESH_Block will perform geometry analysis, we need just to find 2
    // connected vertices on top and bottom

    TopoDS_Vertex Vbot, Vtop;
    if ( nbNotQuadMeshed > 0 ) // Look for vertices
    {
      TopTools_IndexedMapOfShape edgeMap;
      TopExp::MapShapes( botSM->GetSubShape(), TopAbs_EDGE, edgeMap );
      // vertex 1 is any vertex of the bottom face
      Vbot = TopExp::FirstVertex( TopoDS::Edge( edgeMap( 1 )));
      // vertex 2 is end vertex of edge sharing Vbot and not belonging to the bottom face
      TopTools_ListIteratorOfListOfShape ancestIt = Mesh()->GetAncestors( Vbot );
      for ( ; Vtop.IsNull() && ancestIt.More(); ancestIt.Next() )
      {
        const TopoDS_Shape & ancestor = ancestIt.Value();
        if ( ancestor.ShapeType() == TopAbs_EDGE && !edgeMap.FindIndex( ancestor ))
        {
          TopoDS_Vertex V1, V2;
          TopExp::Vertices( TopoDS::Edge( ancestor ), V1, V2);
          if      ( Vbot.IsSame ( V1 )) Vtop = V2;
          else if ( Vbot.IsSame ( V2 )) Vtop = V1;
          // check that Vtop belongs to shape3D
          TopExp_Explorer exp( shape3D, TopAbs_VERTEX );
          for ( ; exp.More(); exp.Next() )
            if ( Vtop.IsSame( exp.Current() ))
              break;
          if ( !exp.More() )
            Vtop.Nullify();
        }
      }
    }
    // get shell from shape3D
    TopoDS_Shell shell;
    TopExp_Explorer exp( shape3D, TopAbs_SHELL );
    int nbShell = 0;
    for ( ; exp.More(); exp.Next(), ++nbShell )
      shell = TopoDS::Shell( exp.Current() );
//     if ( nbShell != 1 )
//       RETURN_BAD_RESULT("There must be 1 shell in the block");

    // Load geometry in SMESH_Block
    if ( !SMESH_Block::FindBlockShapes( shell, Vbot, Vtop, myShapeIDMap )) {
      if ( !hasNotQuad )
        return error(COMPERR_BAD_SHAPE, "Can't detect top and bottom of a prism");
    }
    else {
      if ( !botSM ) botSM = Mesh()->GetSubMeshContaining( myShapeIDMap( ID_BOT_FACE ));
      if ( !topSM ) topSM = Mesh()->GetSubMeshContaining( myShapeIDMap( ID_TOP_FACE ));
    }

  } // end  Standard block of 6 quadrangle faces
  // --------------------------------------------------------

  // Here the top and bottom faces are found
  if ( nbNotQuadMeshed == 2 ) // roughly check correspondence of horiz meshes
  {
//     SMESHDS_SubMesh* topSMDS = topSM->GetSubMeshDS();
//     SMESHDS_SubMesh* botSMDS = botSM->GetSubMeshDS();
//     if ( topSMDS->NbNodes() != botSMDS->NbNodes() ||
//          topSMDS->NbElements() != botSMDS->NbElements() )
//       RETURN_BAD_RESULT("Top mesh doesn't correspond to bottom one");
  }

  // ---------------------------------------------------------
  // If there are not quadrangle geom faces, we emulate
  // a block of 6 quadrangle faces.
  // Load SMESH_Block with faces and edges geometry
  // ---------------------------------------------------------

  
  // find vertex 000 - the one with smallest coordinates (for easy DEBUG :-)
  TopoDS_Vertex V000;
  double minVal = DBL_MAX, minX, val;
  for ( TopExp_Explorer exp( botSM->GetSubShape(), TopAbs_VERTEX );
        exp.More(); exp.Next() )
  {
    const TopoDS_Vertex& v = TopoDS::Vertex( exp.Current() );
    gp_Pnt P = BRep_Tool::Pnt( v );
    val = P.X() + P.Y() + P.Z();
    if ( val < minVal || ( val == minVal && P.X() < minX )) {
      V000 = v;
      minVal = val;
      minX = P.X();
    }
  }

  // Get ordered bottom edges
  list< TopoDS_Edge > orderedEdges;
  list< int >         nbVertexInWires;
  SMESH_Block::GetOrderedEdges( TopoDS::Face( botSM->GetSubShape().Reversed() ),
                                V000, orderedEdges, nbVertexInWires );
//   if ( nbVertexInWires.size() != 1 )
//     RETURN_BAD_RESULT("Wrong prism geometry");

  // Get Wall faces corresponding to the ordered bottom edges
  list< TopoDS_Face > wallFaces;
  if ( !GetWallFaces( Mesh(), shape3D, botSM->GetSubShape(), orderedEdges, wallFaces))
    return error(COMPERR_BAD_SHAPE, "Can't find side faces");

  // Find columns of wall nodes and calculate edges' lengths
  // --------------------------------------------------------

  myParam2ColumnMaps.clear();
  myParam2ColumnMaps.resize( orderedEdges.size() ); // total nb edges

  int iE, nbEdges = nbVertexInWires.front(); // nb outer edges
  vector< double > edgeLength( nbEdges );
  map< double, int > len2edgeMap;

  list< TopoDS_Edge >::iterator edgeIt = orderedEdges.begin();
  list< TopoDS_Face >::iterator faceIt = wallFaces.begin();
  for ( iE = 0; iE < nbEdges; ++edgeIt, ++faceIt )
  {
    TParam2ColumnMap & faceColumns = myParam2ColumnMaps[ iE ];
    if ( !myHelper->LoadNodeColumns( faceColumns, *faceIt, *edgeIt, meshDS ))
      return error(COMPERR_BAD_INPUT_MESH, TCom("Can't find regular quadrangle mesh ")
                   << "on a side face #" << MeshDS()->ShapeToIndex( *faceIt ));

    SHOWYXZ("\np1 F "<<iE, gpXYZ(faceColumns.begin()->second.front() ));
    SHOWYXZ("p2 F "<<iE, gpXYZ(faceColumns.rbegin()->second.front() ));
    SHOWYXZ("V First "<<iE, BRep_Tool::Pnt( TopExp::FirstVertex(*edgeIt,true )));

    edgeLength[ iE ] = SMESH_Algo::EdgeLength( *edgeIt );

    if ( nbEdges < NB_WALL_FACES ) // fill map used to split faces
    {
      SMESHDS_SubMesh* smDS = meshDS->MeshElements( *edgeIt);
      if ( !smDS )
        return error(COMPERR_BAD_INPUT_MESH, TCom("Null submesh on the edge #")
                     << MeshDS()->ShapeToIndex( *edgeIt ));
      // assure length uniqueness
      edgeLength[ iE ] *= smDS->NbNodes() + edgeLength[ iE ] / ( 1000 + iE );
      len2edgeMap[ edgeLength[ iE ]] = iE;
    }
    ++iE;
  }
  // Load columns of internal edges (forming holes)
  // and fill map ShapeIndex to TParam2ColumnMap for them
  for ( ; edgeIt != orderedEdges.end() ; ++edgeIt, ++faceIt )
  {
    TParam2ColumnMap & faceColumns = myParam2ColumnMaps[ iE ];
    if ( !myHelper->LoadNodeColumns( faceColumns, *faceIt, *edgeIt, meshDS ))
      return error(COMPERR_BAD_INPUT_MESH, TCom("Can't find regular quadrangle mesh ")
                   << "on a side face #" << MeshDS()->ShapeToIndex( *faceIt ));
    // edge columns
    int id = MeshDS()->ShapeToIndex( *edgeIt );
    bool isForward = true; // meaningless for intenal wires
    myShapeIndex2ColumnMap[ id ] = make_pair( & faceColumns, isForward );
    // columns for vertices
    // 1
    const SMDS_MeshNode* n0 = faceColumns.begin()->second.front();
    id = n0->GetPosition()->GetShapeId();
    myShapeIndex2ColumnMap[ id ] = make_pair( & faceColumns, isForward );
    // 2
    const SMDS_MeshNode* n1 = faceColumns.rbegin()->second.front();
    id = n1->GetPosition()->GetShapeId();
    myShapeIndex2ColumnMap[ id ] = make_pair( & faceColumns, isForward );
//     SHOWYXZ("\np1 F "<<iE, gpXYZ(faceColumns.begin()->second.front() ));
//     SHOWYXZ("p2 F "<<iE, gpXYZ(faceColumns.rbegin()->second.front() ));
//     SHOWYXZ("V First "<<iE, BRep_Tool::Pnt( TopExp::FirstVertex(*edgeIt,true )));
    ++iE;
  }

  // Create 4 wall faces of a block
  // -------------------------------

  if ( nbEdges <= NB_WALL_FACES ) // ************* Split faces if necessary
  {
    map< int, int > iE2nbSplit;
    if ( nbEdges != NB_WALL_FACES ) // define how to split
    {
      if ( len2edgeMap.size() != nbEdges )
        RETURN_BAD_RESULT("Uniqueness of edge lengths not assured");
      map< double, int >::reverse_iterator maxLen_i = len2edgeMap.rbegin();
      map< double, int >::reverse_iterator midLen_i = ++len2edgeMap.rbegin();
      double maxLen = maxLen_i->first;
      double midLen = ( len2edgeMap.size() == 1 ) ? 0 : midLen_i->first;
      switch ( nbEdges ) {
      case 1: // 0-th edge is split into 4 parts
        iE2nbSplit.insert( make_pair( 0, 4 )); break;
      case 2: // either the longest edge is split into 3 parts, or both edges into halves
        if ( maxLen / 3 > midLen / 2 ) {
          iE2nbSplit.insert( make_pair( maxLen_i->second, 3 ));
        }
        else {
          iE2nbSplit.insert( make_pair( maxLen_i->second, 2 ));
          iE2nbSplit.insert( make_pair( midLen_i->second, 2 ));
        }
        break;
      case 3:
        // split longest into halves
        iE2nbSplit.insert( make_pair( maxLen_i->second, 2 ));
      }
    }
    // Create TSideFace's
    faceIt = wallFaces.begin();
    edgeIt = orderedEdges.begin();
    int iSide = 0;
    for ( iE = 0; iE < nbEdges; ++edgeIt, ++faceIt )
    {
     // split?
      map< int, int >::iterator i_nb = iE2nbSplit.find( iE );
      if ( i_nb != iE2nbSplit.end() ) {
        // split!
        int nbSplit = i_nb->second;
        vector< double > params;
        splitParams( nbSplit, &myParam2ColumnMaps[ iE ], params );
        bool isForward = ( edgeIt->Orientation() == TopAbs_FORWARD );
        for ( int i = 0; i < nbSplit; ++i ) {
          double f = ( isForward ? params[ i ] : params[ nbSplit - i-1 ]);
          double l = ( isForward ? params[ i+1 ] : params[ nbSplit - i ]);
          TSideFace* comp = new TSideFace( myHelper, wallFaceIds[ iSide ],
                                           *faceIt, *edgeIt,
                                           &myParam2ColumnMaps[ iE ], f, l );
          mySide->SetComponent( iSide++, comp );
        }
      }
      else {
        TSideFace* comp = new TSideFace( myHelper, wallFaceIds[ iSide ],
                                         *faceIt, *edgeIt,
                                         &myParam2ColumnMaps[ iE ]);
        mySide->SetComponent( iSide++, comp );
      }
      ++iE;
    }
  }
  else { // **************************** Unite faces

    // unite first faces
    int nbExraFaces = nbEdges - 3;
    int iSide = 0, iE;
    double u0 = 0, sumLen = 0;
    for ( iE = 0; iE < nbExraFaces; ++iE )
      sumLen += edgeLength[ iE ];

    vector< TSideFace* > components( nbExraFaces );
    vector< pair< double, double> > params( nbExraFaces );
    faceIt = wallFaces.begin();
    edgeIt = orderedEdges.begin();
    for ( iE = 0; iE < nbExraFaces; ++edgeIt, ++faceIt )
    {
      components[ iE ] = new TSideFace( myHelper, wallFaceIds[ iSide ],
                                        *faceIt, *edgeIt,
                                        &myParam2ColumnMaps[ iE ]);
      double u1 = u0 + edgeLength[ iE ] / sumLen;
      params[ iE ] = make_pair( u0 , u1 );
      u0 = u1;
      ++iE;
    }
    mySide->SetComponent( iSide++, new TSideFace( components, params ));

    // fill the rest faces
    for ( ; iE < nbEdges; ++faceIt, ++edgeIt )
    {
      TSideFace* comp = new TSideFace( myHelper, wallFaceIds[ iSide ],
                                       *faceIt, *edgeIt,
                                       &myParam2ColumnMaps[ iE ]);
      mySide->SetComponent( iSide++, comp );
      ++iE;
    }
  }


  // Fill geometry fields of SMESH_Block
  // ------------------------------------

  TopoDS_Face botF = TopoDS::Face( botSM->GetSubShape() );
  TopoDS_Face topF = TopoDS::Face( topSM->GetSubShape() );

  vector< int > botEdgeIdVec;
  SMESH_Block::GetFaceEdgesIDs( ID_BOT_FACE, botEdgeIdVec );

  bool isForward[NB_WALL_FACES] = { true, true, true, true };
  Adaptor2d_Curve2d* botPcurves[NB_WALL_FACES];
  Adaptor2d_Curve2d* topPcurves[NB_WALL_FACES];

  for ( int iF = 0; iF < NB_WALL_FACES; ++iF )
  {
    TSideFace * sideFace = mySide->GetComponent( iF );
    if ( !sideFace )
      RETURN_BAD_RESULT("NULL TSideFace");
    int fID = sideFace->FaceID();

    // fill myShapeIDMap
    if ( sideFace->InsertSubShapes( myShapeIDMap ) != 8 &&
         !sideFace->IsComplex())
      MESSAGE( ": Warning : InsertSubShapes() < 8 on side " << iF );

    // side faces geometry
    Adaptor2d_Curve2d* pcurves[NB_WALL_FACES];
    if ( !sideFace->GetPCurves( pcurves ))
      RETURN_BAD_RESULT("TSideFace::GetPCurves() failed");

    SMESH_Block::TFace& tFace = myFace[ fID - ID_FirstF ];
    tFace.Set( fID, sideFace->Surface(), pcurves, isForward );

    SHOWYXZ( endl<<"F "<< iF << " id " << fID << " FRW " << sideFace->IsForward(), sideFace->Value(0,0));
    // edges 3D geometry
    vector< int > edgeIdVec;
    SMESH_Block::GetFaceEdgesIDs( fID, edgeIdVec );
    for ( int isMax = 0; isMax < 2; ++isMax ) {
      {
        int eID = edgeIdVec[ isMax ];
        SMESH_Block::TEdge& tEdge = myEdge[ eID - ID_FirstE ];
        tEdge.Set( eID, sideFace->HorizCurve(isMax), true);
        SHOWYXZ(eID<<" HOR"<<isMax<<"(0)", sideFace->HorizCurve(isMax)->Value(0));
        SHOWYXZ(eID<<" HOR"<<isMax<<"(1)", sideFace->HorizCurve(isMax)->Value(1));
      }
      {
        int eID = edgeIdVec[ isMax+2 ];
        SMESH_Block::TEdge& tEdge = myEdge[ eID - ID_FirstE  ];
        tEdge.Set( eID, sideFace->VertiCurve(isMax), true);
        SHOWYXZ(eID<<" VER"<<isMax<<"(0)", sideFace->VertiCurve(isMax)->Value(0));
        SHOWYXZ(eID<<" VER"<<isMax<<"(1)", sideFace->VertiCurve(isMax)->Value(1));

        // corner points
        vector< int > vertexIdVec;
        SMESH_Block::GetEdgeVertexIDs( eID, vertexIdVec );
        myPnt[ vertexIdVec[0] - ID_FirstV ] = tEdge.GetCurve()->Value(0).XYZ();
        myPnt[ vertexIdVec[1] - ID_FirstV ] = tEdge.GetCurve()->Value(1).XYZ();
      }
    }
    // pcurves on horizontal faces
    for ( iE = 0; iE < NB_WALL_FACES; ++iE ) {
      if ( edgeIdVec[ BOTTOM_EDGE ] == botEdgeIdVec[ iE ] ) {
        botPcurves[ iE ] = sideFace->HorizPCurve( false, botF );
        topPcurves[ iE ] = sideFace->HorizPCurve( true,  topF );
        break;
      }
    }
  }
  // horizontal faces geometry
  {
    SMESH_Block::TFace& tFace = myFace[ ID_BOT_FACE - ID_FirstF ];
    tFace.Set( ID_BOT_FACE, new BRepAdaptor_Surface( botF ), botPcurves, isForward );
    SMESH_Block::Insert( botF, ID_BOT_FACE, myShapeIDMap );
  }
  {
    SMESH_Block::TFace& tFace = myFace[ ID_TOP_FACE - ID_FirstF ];
    tFace.Set( ID_TOP_FACE, new BRepAdaptor_Surface( topF ), topPcurves, isForward );
    SMESH_Block::Insert( topF, ID_TOP_FACE, myShapeIDMap );
  }

  // Fill map ShapeIndex to TParam2ColumnMap
  // ----------------------------------------

  list< TSideFace* > fList;
  list< TSideFace* >::iterator fListIt;
  fList.push_back( mySide );
  for ( fListIt = fList.begin(); fListIt != fList.end(); ++fListIt)
  {
    int nb = (*fListIt)->NbComponents();
    for ( int i = 0; i < nb; ++i ) {
      if ( TSideFace* comp = (*fListIt)->GetComponent( i ))
        fList.push_back( comp );
    }
    if ( TParam2ColumnMap* cols = (*fListIt)->GetColumns()) {
      // columns for a base edge
      int id = MeshDS()->ShapeToIndex( (*fListIt)->BaseEdge() );
      bool isForward = (*fListIt)->IsForward();
      myShapeIndex2ColumnMap[ id ] = make_pair( cols, isForward );

      // columns for vertices
      const SMDS_MeshNode* n0 = cols->begin()->second.front();
      id = n0->GetPosition()->GetShapeId();
      myShapeIndex2ColumnMap[ id ] = make_pair( cols, isForward );

      const SMDS_MeshNode* n1 = cols->rbegin()->second.front();
      id = n1->GetPosition()->GetShapeId();
      myShapeIndex2ColumnMap[ id ] = make_pair( cols, !isForward );
    }
  }

//   gp_XYZ testPar(0.25, 0.25, 0), testCoord;
//   if ( !FacePoint( ID_BOT_FACE, testPar, testCoord ))
//     RETURN_BAD_RESULT("TEST FacePoint() FAILED");
//   SHOWYXZ("IN TEST PARAM" , testPar);
//   SHOWYXZ("OUT TEST CORD" , testCoord);
//   if ( !ComputeParameters( testCoord, testPar , ID_BOT_FACE))
//     RETURN_BAD_RESULT("TEST ComputeParameters() FAILED");
//   SHOWYXZ("OUT TEST PARAM" , testPar);

  return true;
}

//================================================================================
/*!
 * \brief Return pointer to column of nodes
 * \param node - bottom node from which the returned column goes up
 * \retval const TNodeColumn* - the found column
 */
//================================================================================

const TNodeColumn* StdMeshers_PrismAsBlock::GetNodeColumn(const SMDS_MeshNode* node) const
{
  int sID = node->GetPosition()->GetShapeId();

  map<int, pair< TParam2ColumnMap*, bool > >::const_iterator col_frw =
    myShapeIndex2ColumnMap.find( sID );
  if ( col_frw != myShapeIndex2ColumnMap.end() ) {
    const TParam2ColumnMap* cols = col_frw->second.first;
    TParam2ColumnIt u_col = cols->begin();
    for ( ; u_col != cols->end(); ++u_col )
      if ( u_col->second[ 0 ] == node )
        return & u_col->second;
  }
  return 0;
}

//================================================================================
/*!
 * \brief Check curve orientation of a bootom edge
  * \param meshDS - mesh DS
  * \param columnsMap - node columns map of side face
  * \param bottomEdge - the bootom edge
  * \param sideFaceID - side face in-block ID
  * \retval bool - true if orientation coinside with in-block froward orientation
 */
//================================================================================

bool StdMeshers_PrismAsBlock::IsForwardEdge(SMESHDS_Mesh*           meshDS,
                                            const TParam2ColumnMap& columnsMap,
                                            const TopoDS_Edge &     bottomEdge,
                                            const int               sideFaceID)
{
  bool isForward = false;
  if ( TAssocTool::IsClosedEdge( bottomEdge ))
  {
    isForward = ( bottomEdge.Orientation() == TopAbs_FORWARD );
  }
  else
  {
    const TNodeColumn& firstCol = columnsMap.begin()->second;
    const SMDS_MeshNode* bottomNode = firstCol[0];
    TopoDS_Shape firstVertex = SMESH_MesherHelper::GetSubShapeByNode( bottomNode, meshDS );
    isForward = ( firstVertex.IsSame( TopExp::FirstVertex( bottomEdge, true )));
  }
  // on 2 of 4 sides first vertex is end
  if ( sideFaceID == ID_Fx1z || sideFaceID == ID_F0yz )
    isForward = !isForward;
  return isForward;
}

//================================================================================
  /*!
   * \brief Find wall faces by bottom edges
    * \param mesh - the mesh
    * \param mainShape - the prism
    * \param bottomFace - the bottom face
    * \param bottomEdges - edges bounding the bottom face
    * \param wallFaces - faces list to fill in
   */
//================================================================================

bool StdMeshers_PrismAsBlock::GetWallFaces( SMESH_Mesh*                     mesh,
                                            const TopoDS_Shape &            mainShape,
                                            const TopoDS_Shape &            bottomFace,
                                            const std::list< TopoDS_Edge >& bottomEdges,
                                            std::list< TopoDS_Face >&       wallFaces)
{
  wallFaces.clear();

  TopTools_IndexedMapOfShape faceMap;
  TopExp::MapShapes( mainShape, TopAbs_FACE, faceMap );

  list< TopoDS_Edge >::const_iterator edge = bottomEdges.begin();
  for ( ; edge != bottomEdges.end(); ++edge )
  {
    TopTools_ListIteratorOfListOfShape ancestIt = mesh->GetAncestors( *edge );
    for ( ; ancestIt.More(); ancestIt.Next() )
    {
      const TopoDS_Shape& ancestor = ancestIt.Value();
      if ( ancestor.ShapeType() == TopAbs_FACE && // face
           !bottomFace.IsSame( ancestor ) &&      // not bottom
           faceMap.FindIndex( ancestor ))         // belongs to the prism
      {
        wallFaces.push_back( TopoDS::Face( ancestor ));
        break;
      }
    }
  }
  return ( wallFaces.size() == bottomEdges.size() );
}

//================================================================================
/*!
 * \brief Constructor
  * \param faceID - in-block ID
  * \param face - geom face
  * \param columnsMap - map of node columns
  * \param first - first normalized param
  * \param last - last normalized param
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace::TSideFace(SMESH_MesherHelper* helper,
                                              const int           faceID,
                                              const TopoDS_Face&  face,
                                              const TopoDS_Edge&  baseEdge,
                                              TParam2ColumnMap*   columnsMap,
                                              const double        first,
                                              const double        last):
  myID( faceID ),
  myParamToColumnMap( columnsMap ),
  myBaseEdge( baseEdge ),
  myHelper( helper )
{
  mySurface.Initialize( face );
  myParams.resize( 1 );
  myParams[ 0 ] = make_pair( first, last );
  myIsForward = StdMeshers_PrismAsBlock::IsForwardEdge( myHelper->GetMeshDS(),
                                                        *myParamToColumnMap,
                                                        myBaseEdge, myID );
}

//================================================================================
/*!
 * \brief Constructor of complex side face
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace::
TSideFace(const vector< TSideFace* >&             components,
          const vector< pair< double, double> > & params)
  :myID( components[0] ? components[0]->myID : 0 ),
   myParamToColumnMap( 0 ),
   myParams( params ),
   myIsForward( true ),
   myComponents( components ),
   myHelper( components[0] ? components[0]->myHelper : 0 )
{}
//================================================================================
/*!
 * \brief Copy constructor
  * \param other - other side
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace::TSideFace( const TSideFace& other )
{
  myID               = other.myID;
  mySurface          = other.mySurface;
  myBaseEdge         = other.myBaseEdge;
  myParams           = other.myParams;
  myIsForward        = other.myIsForward;
  myHelper           = other.myHelper;
  myParamToColumnMap = other.myParamToColumnMap;

  myComponents.resize( other.myComponents.size());
  for (int i = 0 ; i < myComponents.size(); ++i )
    myComponents[ i ] = new TSideFace( *other.myComponents[ i ]);
}

//================================================================================
/*!
 * \brief Deletes myComponents
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace::~TSideFace()
{
  for (int i = 0 ; i < myComponents.size(); ++i )
    if ( myComponents[ i ] )
      delete myComponents[ i ];
}

//================================================================================
/*!
 * \brief Return geometry of the vertical curve
  * \param isMax - true means curve located closer to (1,1,1) block point
  * \retval Adaptor3d_Curve* - curve adaptor
 */
//================================================================================

Adaptor3d_Curve* StdMeshers_PrismAsBlock::TSideFace::VertiCurve(const bool isMax) const
{
  if ( !myComponents.empty() ) {
    if ( isMax )
      return myComponents.back()->VertiCurve(isMax);
    else
      return myComponents.front()->VertiCurve(isMax);
  }
  double f = myParams[0].first, l = myParams[0].second;
  if ( !myIsForward ) std::swap( f, l );
  return new TVerticalEdgeAdaptor( myParamToColumnMap, isMax ? l : f );
}

//================================================================================
/*!
 * \brief Return geometry of the top or bottom curve
  * \param isTop - 
  * \retval Adaptor3d_Curve* - 
 */
//================================================================================

Adaptor3d_Curve* StdMeshers_PrismAsBlock::TSideFace::HorizCurve(const bool isTop) const
{
  return new THorizontalEdgeAdaptor( this, isTop );
}

//================================================================================
/*!
 * \brief Return pcurves
  * \param pcurv - array of 4 pcurves
  * \retval bool - is a success
 */
//================================================================================

bool StdMeshers_PrismAsBlock::TSideFace::GetPCurves(Adaptor2d_Curve2d* pcurv[4]) const
{
  int iEdge[ 4 ] = { BOTTOM_EDGE, TOP_EDGE, V0_EDGE, V1_EDGE };

  for ( int i = 0 ; i < 4 ; ++i ) {
    Handle(Geom2d_Line) line;
    switch ( iEdge[ i ] ) {
    case TOP_EDGE:
      line = new Geom2d_Line( gp_Pnt2d( 0, 1 ), gp::DX2d() ); break;
    case BOTTOM_EDGE:
      line = new Geom2d_Line( gp::Origin2d(), gp::DX2d() ); break;
    case V0_EDGE:
      line = new Geom2d_Line( gp::Origin2d(), gp::DY2d() ); break;
    case V1_EDGE:
      line = new Geom2d_Line( gp_Pnt2d( 1, 0 ), gp::DY2d() ); break;
    }
    pcurv[ i ] = new Geom2dAdaptor_Curve( line, 0, 1 );
  }
  return true;
}

//================================================================================
/*!
 * \brief Returns geometry of pcurve on a horizontal face
  * \param isTop - is top or bottom face
  * \param horFace - a horizontal face
  * \retval Adaptor2d_Curve2d* - curve adaptor
 */
//================================================================================

Adaptor2d_Curve2d*
StdMeshers_PrismAsBlock::TSideFace::HorizPCurve(const bool         isTop,
                                                const TopoDS_Face& horFace) const
{
  return new TPCurveOnHorFaceAdaptor( this, isTop, horFace );
}

//================================================================================
/*!
 * \brief Return a component corresponding to parameter
  * \param U - parameter along a horizontal size
  * \param localU - parameter along a horizontal size of a component
  * \retval TSideFace* - found component
 */
//================================================================================

StdMeshers_PrismAsBlock::TSideFace*
StdMeshers_PrismAsBlock::TSideFace::GetComponent(const double U,double & localU) const
{
  localU = U;
  if ( myComponents.empty() )
    return const_cast<TSideFace*>( this );

  int i;
  for ( i = 0; i < myComponents.size(); ++i )
    if ( U < myParams[ i ].second )
      break;
  if ( i >= myComponents.size() )
    i = myComponents.size() - 1;

  double f = myParams[ i ].first, l = myParams[ i ].second;
  localU = ( U - f ) / ( l - f );
  return myComponents[ i ];
}

//================================================================================
/*!
 * \brief Find node columns for a parameter
  * \param U - parameter along a horizontal edge
  * \param col1 - the 1st found column
  * \param col2 - the 2nd found column
  * \retval r - normalized position of U between the found columns
 */
//================================================================================

double StdMeshers_PrismAsBlock::TSideFace::GetColumns(const double      U,
                                                      TParam2ColumnIt & col1,
                                                      TParam2ColumnIt & col2) const
{
  double u = U, r = 0;
  if ( !myComponents.empty() ) {
    TSideFace * comp = GetComponent(U,u);
    return comp->GetColumns( u, col1, col2 );
  }

  if ( !myIsForward )
    u = 1 - u;
  double f = myParams[0].first, l = myParams[0].second;
  u = f + u * ( l - f );

  col1 = col2 = getColumn( myParamToColumnMap, u );
  if ( ++col2 == myParamToColumnMap->end() ) {
    --col2;
    r = 0.5;
  }
  else {
//     if ( !myIsForward )
//       std::swap( col1, col2 );
    double uf = col1->first;
    double ul = col2->first;
    r = ( u - uf ) / ( ul - uf );
  }
  return r;
}

//================================================================================
/*!
 * \brief Return coordinates by normalized params
  * \param U - horizontal param
  * \param V - vertical param
  * \retval gp_Pnt - result point
 */
//================================================================================

gp_Pnt StdMeshers_PrismAsBlock::TSideFace::Value(const Standard_Real U,
                                                 const Standard_Real V) const
{
  double u;
  if ( !myComponents.empty() ) {
    TSideFace * comp = GetComponent(U,u);
    return comp->Value( u, V );
  }

  TParam2ColumnIt u_col1, u_col2;
  double vR, hR = GetColumns( U, u_col1, u_col2 );

  const SMDS_MeshNode* n1 = 0;
  const SMDS_MeshNode* n2 = 0;
  const SMDS_MeshNode* n3 = 0;
  const SMDS_MeshNode* n4 = 0;
  gp_XYZ pnt;

  vR = getRAndNodes( & u_col1->second, V, n1, n2 );
  vR = getRAndNodes( & u_col2->second, V, n3, n4 );
  
  gp_XY uv1 = myHelper->GetNodeUV( mySurface.Face(), n1, n4);
  gp_XY uv2 = myHelper->GetNodeUV( mySurface.Face(), n2, n3);
  gp_XY uv12 = uv1 * ( 1 - vR ) + uv2 * vR;

  gp_XY uv3 = myHelper->GetNodeUV( mySurface.Face(), n3, n2);
  gp_XY uv4 = myHelper->GetNodeUV( mySurface.Face(), n4, n1);
  gp_XY uv34 = uv3 * ( 1 - vR ) + uv4 * vR;

  gp_XY uv = uv12 * ( 1 - hR ) + uv34 * hR;
  
  return mySurface.Value( uv.X(), uv.Y() );
}


//================================================================================
/*!
 * \brief Return boundary edge
  * \param edge - edge index
  * \retval TopoDS_Edge - found edge
 */
//================================================================================

TopoDS_Edge StdMeshers_PrismAsBlock::TSideFace::GetEdge(const int iEdge) const
{
  if ( !myComponents.empty() ) {
    switch ( iEdge ) {
    case V0_EDGE : return myComponents.front()->GetEdge( iEdge );
    case V1_EDGE : return myComponents.back() ->GetEdge( iEdge );
    default: return TopoDS_Edge();
    }
  }
  TopoDS_Shape edge;
  const SMDS_MeshNode* node = 0;
  SMESHDS_Mesh * meshDS = myHelper->GetMesh()->GetMeshDS();
  TNodeColumn* column;

  switch ( iEdge ) {
  case TOP_EDGE:
  case BOTTOM_EDGE:
    column = & (( ++myParamToColumnMap->begin())->second );
    node = ( iEdge == TOP_EDGE ) ? column->back() : column->front();
    edge = myHelper->GetSubShapeByNode ( node, meshDS );
    if ( edge.ShapeType() == TopAbs_VERTEX ) {
      column = & ( myParamToColumnMap->begin()->second );
      node = ( iEdge == TOP_EDGE ) ? column->back() : column->front();
    }
    break;
  case V0_EDGE:
  case V1_EDGE: {
    bool back = ( iEdge == V1_EDGE );
    if ( !myIsForward ) back = !back;
    if ( back )
      column = & ( myParamToColumnMap->rbegin()->second );
    else
      column = & ( myParamToColumnMap->begin()->second );
    if ( column->size() > 0 )
      edge = myHelper->GetSubShapeByNode( (*column)[ 1 ], meshDS );
    if ( edge.IsNull() || edge.ShapeType() == TopAbs_VERTEX )
      node = column->front();
    break;
  }
  default:;
  }
  if ( !edge.IsNull() && edge.ShapeType() == TopAbs_EDGE )
    return TopoDS::Edge( edge );

  // find edge by 2 vertices
  TopoDS_Shape V1 = edge;
  TopoDS_Shape V2 = myHelper->GetSubShapeByNode( node, meshDS );
  if ( V2.ShapeType() == TopAbs_VERTEX && !V2.IsSame( V1 ))
  {
    TopTools_ListIteratorOfListOfShape ancestIt =
      myHelper->GetMesh()->GetAncestors( V1 );
    for ( ; ancestIt.More(); ancestIt.Next() )
    {
      const TopoDS_Shape & ancestor = ancestIt.Value();
      if ( ancestor.ShapeType() == TopAbs_EDGE )
        for ( TopExp_Explorer e( ancestor, TopAbs_VERTEX ); e.More(); e.Next() )
          if ( V2.IsSame( e.Current() ))
            return TopoDS::Edge( ancestor );
    }
  }
  return TopoDS_Edge();
}

//================================================================================
/*!
 * \brief Fill block subshapes
  * \param shapeMap - map to fill in
  * \retval int - nb inserted subshapes
 */
//================================================================================

int StdMeshers_PrismAsBlock::TSideFace::InsertSubShapes(TBlockShapes& shapeMap) const
{
  int nbInserted = 0;

  // Insert edges
  vector< int > edgeIdVec;
  SMESH_Block::GetFaceEdgesIDs( myID, edgeIdVec );

  for ( int i = BOTTOM_EDGE; i <=V1_EDGE ; ++i ) {
    TopoDS_Edge e = GetEdge( i );
    if ( !e.IsNull() ) {
      nbInserted += SMESH_Block::Insert( e, edgeIdVec[ i ], shapeMap);
    }
  }

  // Insert corner vertices

  TParam2ColumnIt col1, col2 ;
  vector< int > vertIdVec;

  // from V0 column
  SMESH_Block::GetEdgeVertexIDs( edgeIdVec[ V0_EDGE ], vertIdVec);
  GetColumns(0, col1, col2 );
  const SMDS_MeshNode* node0 = col1->second.front();
  const SMDS_MeshNode* node1 = col1->second.back();
  TopoDS_Shape v0 = myHelper->GetSubShapeByNode( node0, myHelper->GetMeshDS());
  TopoDS_Shape v1 = myHelper->GetSubShapeByNode( node1, myHelper->GetMeshDS());
  if ( v0.ShapeType() == TopAbs_VERTEX ) {
    nbInserted += SMESH_Block::Insert( v0, vertIdVec[ 0 ], shapeMap);
  }
  if ( v1.ShapeType() == TopAbs_VERTEX ) {
    nbInserted += SMESH_Block::Insert( v1, vertIdVec[ 1 ], shapeMap);
  }
  
  // from V1 column
  SMESH_Block::GetEdgeVertexIDs( edgeIdVec[ V1_EDGE ], vertIdVec);
  GetColumns(1, col1, col2 );
  node0 = col2->second.front();
  node1 = col2->second.back();
  v0 = myHelper->GetSubShapeByNode( node0, myHelper->GetMeshDS());
  v1 = myHelper->GetSubShapeByNode( node1, myHelper->GetMeshDS());
  if ( v0.ShapeType() == TopAbs_VERTEX ) {
    nbInserted += SMESH_Block::Insert( v0, vertIdVec[ 0 ], shapeMap);
  }
  if ( v1.ShapeType() == TopAbs_VERTEX ) {
    nbInserted += SMESH_Block::Insert( v1, vertIdVec[ 1 ], shapeMap);
  }

//   TopoDS_Vertex V0, V1, Vcom;
//   TopExp::Vertices( myBaseEdge, V0, V1, true );
//   if ( !myIsForward ) std::swap( V0, V1 );

//   // bottom vertex IDs
//   SMESH_Block::GetEdgeVertexIDs( edgeIdVec[ _u0 ], vertIdVec);
//   SMESH_Block::Insert( V0, vertIdVec[ 0 ], shapeMap);
//   SMESH_Block::Insert( V1, vertIdVec[ 1 ], shapeMap);

//   TopoDS_Edge sideEdge = GetEdge( V0_EDGE );
//   if ( sideEdge.IsNull() || !TopExp::CommonVertex( botEdge, sideEdge, Vcom ))
//     return false;

//   // insert one side edge
//   int edgeID;
//   if ( Vcom.IsSame( V0 )) edgeID = edgeIdVec[ _v0 ];
//   else                    edgeID = edgeIdVec[ _v1 ];
//   SMESH_Block::Insert( sideEdge, edgeID, shapeMap);

//   // top vertex of the side edge
//   SMESH_Block::GetEdgeVertexIDs( edgeID, vertIdVec);
//   TopoDS_Vertex Vtop = TopExp::FirstVertex( sideEdge );
//   if ( Vcom.IsSame( Vtop ))
//     Vtop = TopExp::LastVertex( sideEdge );
//   SMESH_Block::Insert( Vtop, vertIdVec[ 1 ], shapeMap);

//   // other side edge
//   sideEdge = GetEdge( V1_EDGE );
//   if ( sideEdge.IsNull() )
//     return false;
//   if ( edgeID = edgeIdVec[ _v1 ]) edgeID = edgeIdVec[ _v0 ];
//   else                            edgeID = edgeIdVec[ _v1 ];
//   SMESH_Block::Insert( sideEdge, edgeID, shapeMap);
  
//   // top edge
//   TopoDS_Edge topEdge = GetEdge( TOP_EDGE );
//   SMESH_Block::Insert( topEdge, edgeIdVec[ _u1 ], shapeMap);

//   // top vertex of the other side edge
//   if ( !TopExp::CommonVertex( topEdge, sideEdge, Vcom ))
//     return false;
//   SMESH_Block::GetEdgeVertexIDs( edgeID, vertIdVec );
//   SMESH_Block::Insert( Vcom, vertIdVec[ 1 ], shapeMap);

  return nbInserted;
}

//================================================================================
/*!
 * \brief Creates TVerticalEdgeAdaptor 
  * \param columnsMap - node column map
  * \param parameter - normalized parameter
 */
//================================================================================

StdMeshers_PrismAsBlock::TVerticalEdgeAdaptor::
TVerticalEdgeAdaptor( const TParam2ColumnMap* columnsMap, const double parameter)
{
  myNodeColumn = & getColumn( columnsMap, parameter )->second;
}

//================================================================================
/*!
 * \brief Return coordinates for the given normalized parameter
  * \param U - normalized parameter
  * \retval gp_Pnt - coordinates
 */
//================================================================================

gp_Pnt StdMeshers_PrismAsBlock::TVerticalEdgeAdaptor::Value(const Standard_Real U) const
{
  const SMDS_MeshNode* n1;
  const SMDS_MeshNode* n2;
  double r = getRAndNodes( myNodeColumn, U, n1, n2 );
  return gpXYZ(n1) * ( 1 - r ) + gpXYZ(n2) * r;
}

//================================================================================
/*!
 * \brief Return coordinates for the given normalized parameter
  * \param U - normalized parameter
  * \retval gp_Pnt - coordinates
 */
//================================================================================

gp_Pnt StdMeshers_PrismAsBlock::THorizontalEdgeAdaptor::Value(const Standard_Real U) const
{
  return mySide->TSideFace::Value( U, myV );
}

//================================================================================
/*!
 * \brief Return UV on pcurve for the given normalized parameter
  * \param U - normalized parameter
  * \retval gp_Pnt - coordinates
 */
//================================================================================

gp_Pnt2d StdMeshers_PrismAsBlock::TPCurveOnHorFaceAdaptor::Value(const Standard_Real U) const
{
  TParam2ColumnIt u_col1, u_col2;
  double r = mySide->GetColumns( U, u_col1, u_col2 );
  gp_XY uv1 = mySide->GetNodeUV( myFace, u_col1->second[ myZ ]);
  gp_XY uv2 = mySide->GetNodeUV( myFace, u_col2->second[ myZ ]);
  return uv1 * ( 1 - r ) + uv2 * r;
}
