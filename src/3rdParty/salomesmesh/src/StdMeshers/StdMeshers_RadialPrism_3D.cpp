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
// File      : StdMeshers_RadialPrism_3D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//

#include "StdMeshers_RadialPrism_3D.hxx"

#include <Basics_OCCTVersion.hxx>

#include "StdMeshers_ProjectionUtils.hxx"
#include "StdMeshers_NumberOfLayers.hxx"
#include "StdMeshers_LayerDistribution.hxx"
#include "StdMeshers_Prism_3D.hxx"
#include "StdMeshers_Regular_1D.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"

#include "utilities.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <gp.hxx>
#include <gp_Pnt.hxx>
#include <BRepClass3d.hxx>

using namespace std;

#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }
#define gpXYZ(n) gp_XYZ(n->X(),n->Y(),n->Z())

namespace ProjectionUtils = StdMeshers_ProjectionUtils;

//=======================================================================
//function : StdMeshers_RadialPrism_3D
//purpose  : 
//=======================================================================

StdMeshers_RadialPrism_3D::StdMeshers_RadialPrism_3D(int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_3D_Algo(hypId, studyId, gen)
{
  _name = "RadialPrism_3D";
  _shapeType = (1 << TopAbs_SOLID);     // 1 bit per shape type

  _compatibleHypothesis.push_back("LayerDistribution");
  _compatibleHypothesis.push_back("NumberOfLayers");
  myNbLayerHypo = 0;
  myDistributionHypo = 0;
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_RadialPrism_3D::~StdMeshers_RadialPrism_3D()
{}

//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_RadialPrism_3D::CheckHypothesis(SMESH_Mesh&                          aMesh,
                                                const TopoDS_Shape&                  aShape,
                                                SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  // check aShape that must have 2 shells
/*  PAL16229
  if ( ProjectionUtils::Count( aShape, TopAbs_SOLID, 0 ) != 1 ||
       ProjectionUtils::Count( aShape, TopAbs_SHELL, 0 ) != 2 )
  {
    aStatus = HYP_BAD_GEOMETRY;
    return false;
  }
*/
  myNbLayerHypo = 0;
  myDistributionHypo = 0;

  list <const SMESHDS_Hypothesis * >::const_iterator itl;

  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(aMesh, aShape);
  if ( hyps.size() == 0 )
  {
    aStatus = SMESH_Hypothesis::HYP_MISSING;
    return false;  // can't work with no hypothesis
  }

  if ( hyps.size() > 1 )
  {
    aStatus = SMESH_Hypothesis::HYP_ALREADY_EXIST;
    return false;
  }

  const SMESHDS_Hypothesis *theHyp = hyps.front();

  string hypName = theHyp->GetName();

  if (hypName == "NumberOfLayers")
  {
    myNbLayerHypo = static_cast<const StdMeshers_NumberOfLayers *>(theHyp);
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;
  }
  if (hypName == "LayerDistribution")
  {
    myDistributionHypo = static_cast<const StdMeshers_LayerDistribution *>(theHyp);
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;
  }
  aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;
  return true;
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_RadialPrism_3D::Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape)
{
  TopExp_Explorer exp;
  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();

  myHelper = new SMESH_MesherHelper( aMesh );
  myHelper->IsQuadraticSubMesh( aShape );
  // to delete helper at exit from Compute()
  std::unique_ptr<SMESH_MesherHelper> helperDeleter( myHelper );

  // get 2 shells
  TopoDS_Solid solid = TopoDS::Solid( aShape );
  TopoDS_Shell outerShell = BRepClass3d::OuterShell( solid );
  TopoDS_Shape innerShell;
  int nbShells = 0;
  for ( TopoDS_Iterator It (solid); It.More(); It.Next(), ++nbShells )
    if ( !outerShell.IsSame( It.Value() ))
      innerShell = It.Value();
  if ( nbShells != 2 )
    return error(COMPERR_BAD_SHAPE, SMESH_Comment("Must be 2 shells but not ")<<nbShells);

  // ----------------------------------
  // Associate sub-shapes of the shells
  // ----------------------------------

  ProjectionUtils::TShapeShapeMap shape2ShapeMaps[2];
  bool mapOk1 = ProjectionUtils::FindSubShapeAssociation( innerShell, &aMesh,
                                                          outerShell, &aMesh,
                                                          shape2ShapeMaps[0]);
  bool mapOk2 = ProjectionUtils::FindSubShapeAssociation( innerShell.Reversed(), &aMesh,
                                                          outerShell, &aMesh,
                                                          shape2ShapeMaps[1]);
  if ( !mapOk1 && !mapOk2 )
    return error(COMPERR_BAD_SHAPE,"Topology of inner and outer shells seems different" );

  int iMap;
  if ( shape2ShapeMaps[0].Extent() == shape2ShapeMaps[1].Extent() )
  {
    // choose an assiciation by shortest distance between VERTEXes
    double dist1 = 0, dist2 = 0;
    TopTools_DataMapIteratorOfDataMapOfShapeShape ssIt( shape2ShapeMaps[0]._map1to2 );
    for (; ssIt.More(); ssIt.Next() )
    {
      if ( ssIt.Key().ShapeType() != TopAbs_VERTEX ) continue;
      gp_Pnt pIn   = BRep_Tool::Pnt( TopoDS::Vertex( ssIt.Key() ));
      gp_Pnt pOut1 = BRep_Tool::Pnt( TopoDS::Vertex( ssIt.Value() ));
      gp_Pnt pOut2 = BRep_Tool::Pnt( TopoDS::Vertex( shape2ShapeMaps[1]( ssIt.Key() )));
      dist1 += pIn.SquareDistance( pOut1 );
      dist2 += pIn.SquareDistance( pOut2 );
    }
    iMap = ( dist1 < dist2 ) ? 0 : 1;
  }
  else
  {
    iMap = ( shape2ShapeMaps[0].Extent() > shape2ShapeMaps[1].Extent() ) ? 0 : 1;
  }
  ProjectionUtils::TShapeShapeMap& shape2ShapeMap = shape2ShapeMaps[iMap];

  // ------------------
  // Make mesh
  // ------------------

  TNode2ColumnMap node2columnMap;
  myLayerPositions.clear();

  for ( exp.Init( outerShell, TopAbs_FACE ); exp.More(); exp.Next() )
  {
    // Corresponding sub-shapes
    TopoDS_Face outFace = TopoDS::Face( exp.Current() );
    TopoDS_Face inFace;
    if ( !shape2ShapeMap.IsBound( outFace, /*isOut=*/true )) {
      return error(SMESH_Comment("Corresponding inner face not found for face #" )
                   << meshDS->ShapeToIndex( outFace ));
    } else {
      inFace = TopoDS::Face( shape2ShapeMap( outFace, /*isOut=*/true ));
    }

    // Find matching nodes of in and out faces
    ProjectionUtils::TNodeNodeMap nodeIn2OutMap;
    if ( ! ProjectionUtils::FindMatchingNodesOnFaces( inFace, &aMesh, outFace, &aMesh,
                                                      shape2ShapeMap, nodeIn2OutMap ))
      return error(COMPERR_BAD_INPUT_MESH,SMESH_Comment("Mesh on faces #")
                   << meshDS->ShapeToIndex( outFace ) << " and "
                   << meshDS->ShapeToIndex( inFace ) << " seems different" );

    // Create volumes

    SMDS_ElemIteratorPtr faceIt = meshDS->MeshElements( inFace )->GetElements();
    while ( faceIt->more() ) // loop on faces on inFace
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
        const SMDS_MeshNode* nIn = face->GetNode( i );
        TNode2ColumnMap::iterator n_col = node2columnMap.find( nIn );
        if ( n_col != node2columnMap.end() ) {
          columns[ i ] = & n_col->second;
        }
        else {
          TNodeNodeMap::iterator nInOut = nodeIn2OutMap.find( nIn );
          if ( nInOut == nodeIn2OutMap.end() )
            RETURN_BAD_RESULT("No matching node for "<< nIn->GetID() <<
                              " in face "<< face->GetID());
          columns[ i ] = makeNodeColumn( node2columnMap, nIn, nInOut->second );
        }
      }

      StdMeshers_Prism_3D::AddPrisms( columns, myHelper );
    }
  } // loop on faces of out shell

  return true;
}

//================================================================================
/*!
 * \brief Create a column of nodes from outNode to inNode
  * \param n2ColMap - map of node columns to add a created column
  * \param outNode - botton node of a column
  * \param inNode - top node of a column
  * \retval const TNodeColumn* - a new column pointer
 */
//================================================================================

TNodeColumn* StdMeshers_RadialPrism_3D::makeNodeColumn( TNode2ColumnMap&     n2ColMap,
                                                        const SMDS_MeshNode* outNode,
                                                        const SMDS_MeshNode* inNode)
{
  SMESHDS_Mesh * meshDS = myHelper->GetMeshDS();
  int shapeID = myHelper->GetSubShapeID();

  if ( myLayerPositions.empty() ) {
    gp_Pnt pIn = gpXYZ( inNode ), pOut = gpXYZ( outNode );
    computeLayerPositions( pIn, pOut );
  }
  int nbSegments = myLayerPositions.size() + 1;

  TNode2ColumnMap::iterator n_col =
    n2ColMap.insert( make_pair( outNode, TNodeColumn() )).first;
  TNodeColumn & column = n_col->second;
  column.resize( nbSegments + 1 );
  column.front() = outNode;
  column.back() =  inNode;

  gp_XYZ p1 = gpXYZ( outNode );
  gp_XYZ p2 = gpXYZ( inNode );
  for ( int z = 1; z < nbSegments; ++z )
  {
    double r = myLayerPositions[ z - 1 ];
    gp_XYZ p = ( 1 - r ) * p1 + r * p2;
    SMDS_MeshNode* n = meshDS->AddNode( p.X(), p.Y(), p.Z() );
    meshDS->SetNodeInVolume( n, shapeID );
    column[ z ] = n;
  }

  return & column;
}

//================================================================================
//================================================================================
/*!
 * \brief Class computing layers distribution using data of
 *        StdMeshers_LayerDistribution hypothesis
 */
//================================================================================
//================================================================================

class TNodeDistributor: public StdMeshers_Regular_1D
{
  list <const SMESHDS_Hypothesis *> myUsedHyps;
public:
  // -----------------------------------------------------------------------------
  static TNodeDistributor* GetDistributor(SMESH_Mesh& aMesh)
  {
    const int myID = -1000;
    TNodeDistributor* myHyp = dynamic_cast<TNodeDistributor*>( aMesh.GetHypothesis( myID ));
    if ( !myHyp )
      myHyp = new TNodeDistributor( myID, 0, aMesh.GetGen() );
    return myHyp;
  }
  // -----------------------------------------------------------------------------
  bool Compute( vector< double > &                  positions,
                gp_Pnt                              pIn,
                gp_Pnt                              pOut,
                SMESH_Mesh&                         aMesh,
                const StdMeshers_LayerDistribution* hyp)
  {
    double len = pIn.Distance( pOut );
    if ( len <= DBL_MIN ) return error("Too close points of inner and outer shells");

    if ( !hyp || !hyp->GetLayerDistribution() )
      return error( "Invalid LayerDistribution hypothesis");
    myUsedHyps.clear();
    myUsedHyps.push_back( hyp->GetLayerDistribution() );

    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge( pIn, pOut );
    SMESH_Hypothesis::Hypothesis_Status aStatus;
    if ( !StdMeshers_Regular_1D::CheckHypothesis( aMesh, edge, aStatus ))
      return error( "StdMeshers_Regular_1D::CheckHypothesis() failed "
                    "with LayerDistribution hypothesis");

    BRepAdaptor_Curve C3D(edge);
    double f = C3D.FirstParameter(), l = C3D.LastParameter();
    list< double > params;
    if ( !StdMeshers_Regular_1D::computeInternalParameters( aMesh, C3D, len, f, l, params, false ))
      return error("StdMeshers_Regular_1D failed to compute layers distribution");

    positions.clear();
    positions.reserve( params.size() );
    for (list<double>::iterator itU = params.begin(); itU != params.end(); itU++)
      positions.push_back( *itU / len );
    return true;
  }
protected:
  // -----------------------------------------------------------------------------
  TNodeDistributor( int hypId, int studyId, SMESH_Gen* gen)
    : StdMeshers_Regular_1D( hypId, studyId, gen)
  {
  }
  // -----------------------------------------------------------------------------
  virtual const list <const SMESHDS_Hypothesis *> &
    GetUsedHypothesis(SMESH_Mesh &, const TopoDS_Shape &, const bool)
  {
    return myUsedHyps;
  }
  // -----------------------------------------------------------------------------
};

//================================================================================
/*!
 * \brief Compute positions of nodes between the internal and the external surfaces
  * \retval bool - is a success
 */
//================================================================================

bool StdMeshers_RadialPrism_3D::computeLayerPositions(const gp_Pnt& pIn,
                                                      const gp_Pnt& pOut)
{
  if ( myNbLayerHypo )
  {
    int nbSegments = myNbLayerHypo->GetNumberOfLayers();
    myLayerPositions.resize( nbSegments - 1 );
    for ( int z = 1; z < nbSegments; ++z )
      myLayerPositions[ z - 1 ] = double( z )/ double( nbSegments );
    return true;
  }
  if ( myDistributionHypo ) {
    SMESH_Mesh * mesh = myHelper->GetMesh();
    if ( !TNodeDistributor::GetDistributor(*mesh)->Compute( myLayerPositions, pIn, pOut,
                                                            *mesh, myDistributionHypo ))
    {
      error( TNodeDistributor::GetDistributor(*mesh)->GetComputeError() );
      return false;
    }
  }
  RETURN_BAD_RESULT("Bad hypothesis");
}


//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================

bool StdMeshers_RadialPrism_3D::Evaluate(SMESH_Mesh& aMesh,
                                         const TopoDS_Shape& aShape,
                                         MapShapeNbElems& aResMap)
{
  // get 2 shells
  TopoDS_Solid solid = TopoDS::Solid( aShape );
  TopoDS_Shell outerShell = BRepClass3d::OuterShell( solid );
  TopoDS_Shape innerShell;
  int nbShells = 0;
  for ( TopoDS_Iterator It (solid); It.More(); It.Next(), ++nbShells )
    if ( !outerShell.IsSame( It.Value() ))
      innerShell = It.Value();
  if ( nbShells != 2 ) {
    std::vector<int> aResVec(SMDSEntity_Last);
    for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
    SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
    aResMap.insert(std::make_pair(sm,aResVec));
    SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
    smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
    return false;
  }

  // Associate sub-shapes of the shells
  ProjectionUtils::TShapeShapeMap shape2ShapeMap;
  if ( !ProjectionUtils::FindSubShapeAssociation( outerShell, &aMesh,
                                                  innerShell, &aMesh,
                                                  shape2ShapeMap) ) {
    std::vector<int> aResVec(SMDSEntity_Last);
    for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
    SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
    aResMap.insert(std::make_pair(sm,aResVec));
    SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
    smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
    return false;
  }

  // get info for outer shell
  int nb0d_Out=0, nb2d_3_Out=0, nb2d_4_Out=0;
  //TopTools_SequenceOfShape FacesOut;
  for (TopExp_Explorer exp(outerShell, TopAbs_FACE); exp.More(); exp.Next()) {
    //FacesOut.Append(exp.Current());
    SMESH_subMesh *aSubMesh = aMesh.GetSubMesh(exp.Current());
    MapShapeNbElemsItr anIt = aResMap.find(aSubMesh);
    std::vector<int> aVec = (*anIt).second;
    nb0d_Out += aVec[SMDSEntity_Node];
    nb2d_3_Out += Max(aVec[SMDSEntity_Triangle],aVec[SMDSEntity_Quad_Triangle]);
    nb2d_4_Out += Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
  }
  int nb1d_Out = 0;
  TopTools_MapOfShape tmpMap;
  for (TopExp_Explorer exp(outerShell, TopAbs_EDGE); exp.More(); exp.Next()) {
    if( tmpMap.Contains( exp.Current() ) )
      continue;
    tmpMap.Add( exp.Current() );
    SMESH_subMesh *aSubMesh = aMesh.GetSubMesh(exp.Current());
    MapShapeNbElemsItr anIt = aResMap.find(aSubMesh);
    std::vector<int> aVec = (*anIt).second;
    nb0d_Out += aVec[SMDSEntity_Node];
    nb1d_Out += Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
  }
  tmpMap.Clear();
  for (TopExp_Explorer exp(outerShell, TopAbs_VERTEX); exp.More(); exp.Next()) {
    if( tmpMap.Contains( exp.Current() ) )
      continue;
    tmpMap.Add( exp.Current() );
    nb0d_Out++;
  }

  // get info for inner shell
  int nb0d_In=0, nb2d_3_In=0, nb2d_4_In=0;
  //TopTools_SequenceOfShape FacesIn;
  for (TopExp_Explorer exp(innerShell, TopAbs_FACE); exp.More(); exp.Next()) {
    //FacesIn.Append(exp.Current());
    SMESH_subMesh *aSubMesh = aMesh.GetSubMesh(exp.Current());
    MapShapeNbElemsItr anIt = aResMap.find(aSubMesh);
    std::vector<int> aVec = (*anIt).second;
    nb0d_In += aVec[SMDSEntity_Node];
    nb2d_3_In += Max(aVec[SMDSEntity_Triangle],aVec[SMDSEntity_Quad_Triangle]);
    nb2d_4_In += Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
  }
  int nb1d_In = 0;
  tmpMap.Clear();
  bool IsQuadratic = false;
  bool IsFirst = true;
  for (TopExp_Explorer exp(innerShell, TopAbs_EDGE); exp.More(); exp.Next()) {
    if( tmpMap.Contains( exp.Current() ) )
      continue;
    tmpMap.Add( exp.Current() );
    SMESH_subMesh *aSubMesh = aMesh.GetSubMesh(exp.Current());
    MapShapeNbElemsItr anIt = aResMap.find(aSubMesh);
    std::vector<int> aVec = (*anIt).second;
    nb0d_In += aVec[SMDSEntity_Node];
    nb1d_In += Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
    if(IsFirst) {
      IsQuadratic = (aVec[SMDSEntity_Quad_Edge] > aVec[SMDSEntity_Edge]);
      IsFirst = false;
    }
  }
  tmpMap.Clear();
  for (TopExp_Explorer exp(innerShell, TopAbs_VERTEX); exp.More(); exp.Next()) {
    if( tmpMap.Contains( exp.Current() ) )
      continue;
    tmpMap.Add( exp.Current() );
    nb0d_In++;
  }

  bool IsOK = (nb0d_Out==nb0d_In) && (nb1d_Out==nb1d_In) && 
              (nb2d_3_Out==nb2d_3_In) && (nb2d_4_Out==nb2d_4_In);
  if(!IsOK) {
    std::vector<int> aResVec(SMDSEntity_Last);
    for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
    SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
    aResMap.insert(std::make_pair(sm,aResVec));
    SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
    smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
    return false;
  }

  int nbLayers = 0;
  if( myNbLayerHypo ) {
    nbLayers = myNbLayerHypo->GetNumberOfLayers();
  }
  if ( myDistributionHypo ) {
    if ( !myDistributionHypo->GetLayerDistribution() ) {
      std::vector<int> aResVec(SMDSEntity_Last);
      for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
      SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
      aResMap.insert(std::make_pair(sm,aResVec));
      SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
      smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
      return false;
    }
    TopExp_Explorer exp(outerShell, TopAbs_VERTEX);
    TopoDS_Vertex Vout = TopoDS::Vertex(exp.Current());
    TopoDS_Vertex Vin = TopoDS::Vertex( shape2ShapeMap(Vout) );
    if ( myLayerPositions.empty() ) {
      gp_Pnt pIn = BRep_Tool::Pnt(Vin);
      gp_Pnt pOut = BRep_Tool::Pnt(Vout);
      computeLayerPositions( pIn, pOut );
    }
    nbLayers = myLayerPositions.size() + 1;
  }

  std::vector<int> aResVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
  if(IsQuadratic) {
    aResVec[SMDSEntity_Quad_Penta] = nb2d_3_Out * nbLayers;
    aResVec[SMDSEntity_Quad_Hexa] = nb2d_4_Out * nbLayers;
    int nb1d = ( nb2d_3_Out*3 + nb2d_4_Out*4 ) / 2;
    aResVec[SMDSEntity_Node] = nb0d_Out * ( 2*nbLayers - 1 ) - nb1d * nbLayers;
  }
  else {
    aResVec[SMDSEntity_Node] = nb0d_Out * ( nbLayers - 1 );
    aResVec[SMDSEntity_Penta] = nb2d_3_Out * nbLayers;
    aResVec[SMDSEntity_Hexa] = nb2d_4_Out * nbLayers;
  }
  SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
  aResMap.insert(std::make_pair(sm,aResVec));

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

bool StdMeshers_RadialPrism_3D::IsApplicable( const TopoDS_Shape & aShape, bool toCheckAll )
{
  bool isCurShellApp;
  int nbFoundSolids = 0;
  for (TopExp_Explorer exp( aShape, TopAbs_SOLID ); exp.More(); exp.Next(), ++nbFoundSolids )
  {
    TopoDS_Shape shell[2];
    int nbShells = 0;
    for ( TopoDS_Iterator It (exp.Current()); It.More(); It.Next() )
    {
      nbShells++;
      if ( nbShells > 2 ) {
        if ( toCheckAll ) return false;
        break;
      }
      shell[ nbShells-1 ] = It.Value();
    }
    if ( nbShells != 2 ) { 
      if ( toCheckAll ) return false;  
      continue; 
    }

    int nbFaces1 = SMESH_MesherHelper:: Count( shell[0], TopAbs_FACE, 0 );
    int nbFaces2 = SMESH_MesherHelper:: Count( shell[1], TopAbs_FACE, 0 );
    if ( nbFaces1 != nbFaces2 ){
      if( toCheckAll ) return false;
      continue;
    }
    int nbEdges1 = SMESH_MesherHelper:: Count( shell[0], TopAbs_EDGE, 0 );
    int nbEdges2 = SMESH_MesherHelper:: Count( shell[1], TopAbs_EDGE, 0 );
    if ( nbEdges1 != nbEdges2 ){
      if( toCheckAll ) return false;
      continue;
    }
    int nbVertices1 = SMESH_MesherHelper:: Count( shell[0], TopAbs_VERTEX, 0 );
    int nbVertices2 = SMESH_MesherHelper:: Count( shell[1], TopAbs_VERTEX, 0 );
    if ( nbVertices1 != nbVertices2 ){
      if( toCheckAll ) return false;
      continue;
    }
    if ( !toCheckAll ) return true;
  }
  return ( toCheckAll && nbFoundSolids != 0);
};
