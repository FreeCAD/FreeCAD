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
// File      : StdMeshers_RadialQuadrangle_1D2D.cxx
// Module    : SMESH
// Created   : Fri Oct 20 11:37:07 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_RadialQuadrangle_1D2D.hxx"

//#include "StdMeshers_ProjectionUtils.hxx"
#include "StdMeshers_NumberOfLayers.hxx"
#include "StdMeshers_LayerDistribution.hxx"
//#include "StdMeshers_Prism_3D.hxx"
#include "StdMeshers_Regular_1D.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"

#include "utilities.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <TColgp_SequenceOfPnt.hxx>
#include <TColgp_SequenceOfPnt2d.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>


using namespace std;

#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }
#define gpXYZ(n) gp_XYZ(n->X(),n->Y(),n->Z())

//typedef StdMeshers_ProjectionUtils TAssocTool;


//=======================================================================
//function : StdMeshers_RadialQuadrangle_1D2D
//purpose  : 
//=======================================================================

StdMeshers_RadialQuadrangle_1D2D::StdMeshers_RadialQuadrangle_1D2D(int hypId,
                                                                   int studyId,
                                                                   SMESH_Gen* gen)
  :SMESH_2D_Algo(hypId, studyId, gen)
{
  _name = "RadialQuadrangle_1D2D";
  _shapeType = (1 << TopAbs_FACE);        // 1 bit per shape type

  _compatibleHypothesis.push_back("LayerDistribution2D");
  _compatibleHypothesis.push_back("NumberOfLayers2D");
  myNbLayerHypo = 0;
  myDistributionHypo = 0;
  _requireDescretBoundary = false;
}


//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_RadialQuadrangle_1D2D::~StdMeshers_RadialQuadrangle_1D2D()
{}


//=======================================================================
//function : CheckHypothesis
//purpose  : 
//=======================================================================

bool StdMeshers_RadialQuadrangle_1D2D::CheckHypothesis
                           (SMESH_Mesh&                          aMesh,
                            const TopoDS_Shape&                  aShape,
                            SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  // check aShape 
  myNbLayerHypo = 0;
  myDistributionHypo = 0;

  list <const SMESHDS_Hypothesis * >::const_iterator itl;

  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(aMesh, aShape);
  if ( hyps.size() == 0 ) {
    aStatus = SMESH_Hypothesis::HYP_MISSING;
    return false;  // can't work with no hypothesis
  }

  if ( hyps.size() > 1 ) {
    aStatus = SMESH_Hypothesis::HYP_ALREADY_EXIST;
    return false;
  }

  const SMESHDS_Hypothesis *theHyp = hyps.front();

  string hypName = theHyp->GetName();

  if (hypName == "NumberOfLayers2D") {
    myNbLayerHypo = static_cast<const StdMeshers_NumberOfLayers *>(theHyp);
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;
  }
  if (hypName == "LayerDistribution2D") {
    myDistributionHypo = static_cast<const StdMeshers_LayerDistribution *>(theHyp);
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;
  }
  aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;
  return true;
}

namespace
{
  // ------------------------------------------------------------------------------
  /*!
   * \brief Listener used to mark edges meshed by StdMeshers_RadialQuadrangle_1D2D
   */
  class TLinEdgeMarker : public SMESH_subMeshEventListener
  {
    TLinEdgeMarker(): SMESH_subMeshEventListener(/*isDeletable=*/false) {}
  public:
    static SMESH_subMeshEventListener* getListener()
    {
      static TLinEdgeMarker theEdgeMarker;
      return &theEdgeMarker;
    }
  };
  
  // ------------------------------------------------------------------------------
  /*!
   * \brief Mark an edge as computed by StdMeshers_RadialQuadrangle_1D2D
   */
  void markLinEdgeAsComputedByMe(const TopoDS_Edge& edge, SMESH_subMesh* faceSubMesh)
  {
    if ( SMESH_subMesh* edgeSM = faceSubMesh->GetFather()->GetSubMeshContaining( edge ))
    {
      if ( !edgeSM->GetEventListenerData( TLinEdgeMarker::getListener() ))
        faceSubMesh->SetEventListener( TLinEdgeMarker::getListener(),
                                       SMESH_subMeshEventListenerData::MakeData(faceSubMesh),
                                       edgeSM);
    }
  }
  // ------------------------------------------------------------------------------
  /*!
   * \brief Return true if a radial edge was meshed with StdMeshers_RadialQuadrangle_1D2D with
   * the same radial distribution
   */
  bool isEdgeCompitaballyMeshed(const TopoDS_Edge& edge, SMESH_subMesh* faceSubMesh)
  {
    if ( SMESH_subMesh* edgeSM = faceSubMesh->GetFather()->GetSubMeshContaining( edge ))
    {
      if ( SMESH_subMeshEventListenerData* otherFaceData =
           edgeSM->GetEventListenerData( TLinEdgeMarker::getListener() ))
      {
        // compare hypothesis aplied to two disk faces sharing radial edges
        SMESH_Mesh& mesh = *faceSubMesh->GetFather();
        SMESH_Algo* radialQuadAlgo = mesh.GetGen()->GetAlgo(mesh, faceSubMesh->GetSubShape() );
        SMESH_subMesh* otherFaceSubMesh = otherFaceData->mySubMeshes.front();
        const list <const SMESHDS_Hypothesis *> & hyps1 =
          radialQuadAlgo->GetUsedHypothesis( mesh, faceSubMesh->GetSubShape());
        const list <const SMESHDS_Hypothesis *> & hyps2 =
          radialQuadAlgo->GetUsedHypothesis( mesh, otherFaceSubMesh->GetSubShape());
        if( hyps1.empty() && hyps2.empty() )
          return true; // defaul hyps
        if ( hyps1.size() != hyps2.size() ||
             strcmp( hyps1.front()->GetName(), hyps2.front()->GetName() ))
          return false;
        ostringstream hypDump1, hypDump2;
        list <const SMESHDS_Hypothesis*>::const_iterator hyp1 = hyps1.begin();
        for ( ; hyp1 != hyps1.end(); ++hyp1 )
          const_cast<SMESHDS_Hypothesis*>(*hyp1)->SaveTo( hypDump1 );
        list <const SMESHDS_Hypothesis*>::const_iterator hyp2 = hyps2.begin();
        for ( ; hyp2 != hyps2.end(); ++hyp2 )
          const_cast<SMESHDS_Hypothesis*>(*hyp2)->SaveTo( hypDump2 );
        return hypDump1.str() == hypDump2.str();
      }
    }
    return false;
  }

  //================================================================================
  /*!
   * \brief Return base curve of the edge and extremum parameters
   */
  //================================================================================

  Handle(Geom_Curve) getCurve(const TopoDS_Edge& edge, double* f=0, double* l=0)
  {
    Handle(Geom_Curve) C;
    if ( !edge.IsNull() )
    {
      double first = 0., last = 0.;
      C = BRep_Tool::Curve(edge, first, last);
      if ( !C.IsNull() )
      {
        Handle(Geom_TrimmedCurve) tc = Handle(Geom_TrimmedCurve)::DownCast(C);
        while( !tc.IsNull() ) {
          C = tc->BasisCurve();
          tc = Handle(Geom_TrimmedCurve)::DownCast(C);
        }
        if ( f ) *f = first;
        if ( l ) *l = last;
      }
    }
    return C;
  }

  //================================================================================
  /*!
   * \brief Return edges of the face
   *  \retval int - nb of edges
   */
  //================================================================================

  int analyseFace(const TopoDS_Shape& face,
                  TopoDS_Edge&        CircEdge,
                  TopoDS_Edge&        LinEdge1,
                  TopoDS_Edge&        LinEdge2)
  {
    CircEdge.Nullify(); LinEdge1.Nullify(); LinEdge2.Nullify();
    int nbe = 0;

    for ( TopExp_Explorer exp( face, TopAbs_EDGE ); exp.More(); exp.Next(), ++nbe )
    {
      const TopoDS_Edge& E = TopoDS::Edge( exp.Current() );
      double f,l;
      Handle(Geom_Curve) C = getCurve(E,&f,&l);
      if ( !C.IsNull() )
      {
        if ( C->IsKind( STANDARD_TYPE(Geom_Circle)))
        {
          if ( CircEdge.IsNull() )
            CircEdge = E;
          else
            return 0;
        }
        else if ( LinEdge1.IsNull() )
          LinEdge1 = E;
        else
          LinEdge2 = E;
      }
    }
    return nbe;
  }
}

//=======================================================================
/*!
 * \brief Allow algo to do something after persistent restoration
 * \param subMesh - restored submesh
 *
 * call markLinEdgeAsComputedByMe()
 */
//=======================================================================

void StdMeshers_RadialQuadrangle_1D2D::SubmeshRestored(SMESH_subMesh* faceSubMesh)
{
  if ( !faceSubMesh->IsEmpty() )
  {
    TopoDS_Edge CircEdge, LinEdge1, LinEdge2;
    analyseFace( faceSubMesh->GetSubShape(), CircEdge, LinEdge1, LinEdge2 );
    if ( !LinEdge1.IsNull() ) markLinEdgeAsComputedByMe( LinEdge1, faceSubMesh );
    if ( !LinEdge2.IsNull() ) markLinEdgeAsComputedByMe( LinEdge2, faceSubMesh );
  }
}

//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_RadialQuadrangle_1D2D::Compute(SMESH_Mesh&         aMesh,
                                               const TopoDS_Shape& aShape)
{
  TopExp_Explorer exp;
  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();

  myHelper = new SMESH_MesherHelper( aMesh );
  myHelper->IsQuadraticSubMesh( aShape );
  // to delete helper at exit from Compute()
  auto_ptr<SMESH_MesherHelper> helperDeleter( myHelper );

  myLayerPositions.clear();

  TopoDS_Edge CircEdge, LinEdge1, LinEdge2;
  int nbe = analyseFace( aShape, CircEdge, LinEdge1, LinEdge2 );
  if( nbe>3 || nbe < 1 || CircEdge.IsNull() )
    return error(COMPERR_BAD_SHAPE);
  
  gp_Pnt P0,P1;
  // points for rotation
  TColgp_SequenceOfPnt Points;
  // angles for rotation
  TColStd_SequenceOfReal Angles;
  // Nodes1 and Nodes2 - nodes along radiuses
  // CNodes - nodes on circle edge
  vector< const SMDS_MeshNode* > Nodes1, Nodes2, CNodes;
  SMDS_MeshNode * NC;
  // parameters edge nodes on face
  TColgp_SequenceOfPnt2d Pnts2d1;
  gp_Pnt2d PC;

  int faceID = meshDS->ShapeToIndex(aShape);
  TopoDS_Face F = TopoDS::Face(aShape);
  Handle(Geom_Surface) S = BRep_Tool::Surface(F);

  if(nbe==1)
  {
    Handle(Geom_Circle) aCirc = Handle(Geom_Circle)::DownCast( getCurve( CircEdge ));

    bool ok = _gen->Compute( aMesh, CircEdge );
    if( !ok ) return false;
    map< double, const SMDS_MeshNode* > theNodes;
    ok = GetSortedNodesOnEdge(aMesh.GetMeshDS(),CircEdge,true,theNodes);
    if( !ok ) return false;

    CNodes.clear();
    map< double, const SMDS_MeshNode* >::iterator itn = theNodes.begin();
    const SMDS_MeshNode* NF = (*itn).second;
    CNodes.push_back( (*itn).second );
    double fang = (*itn).first;
    if ( itn != theNodes.end() ) {
      itn++;
      for(; itn != theNodes.end(); itn++ ) {
        CNodes.push_back( (*itn).second );
        double ang = (*itn).first - fang;
        if( ang>PI ) ang = ang - 2*PI;
        if( ang<-PI ) ang = ang + 2*PI;
        Angles.Append( ang ); 
      }
    }
    P1 = gp_Pnt( NF->X(), NF->Y(), NF->Z() );
    P0 = aCirc->Location();

    myLayerPositions.clear();
    computeLayerPositions(P0,P1);

    exp.Init( CircEdge, TopAbs_VERTEX );
    TopoDS_Vertex V1 = TopoDS::Vertex( exp.Current() );
    gp_Pnt2d p2dV = BRep_Tool::Parameters( V1, TopoDS::Face(aShape) );

    NC = meshDS->AddNode(P0.X(), P0.Y(), P0.Z());
    GeomAPI_ProjectPointOnSurf PPS(P0,S);
    double U0,V0;
    PPS.Parameters(1,U0,V0);
    meshDS->SetNodeOnFace(NC, faceID, U0, V0);
    PC = gp_Pnt2d(U0,V0);

    gp_Vec aVec(P0,P1);
    gp_Vec2d aVec2d(PC,p2dV);
    Nodes1.resize( myLayerPositions.size()+1 );
    Nodes2.resize( myLayerPositions.size()+1 );
    int i = 0;
    for(; i<myLayerPositions.size(); i++) {
      gp_Pnt P( P0.X() + aVec.X()*myLayerPositions[i],
                P0.Y() + aVec.Y()*myLayerPositions[i],
                P0.Z() + aVec.Z()*myLayerPositions[i] );
      Points.Append(P);
      SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
      Nodes1[i] = node;
      Nodes2[i] = node;
      double U = PC.X() + aVec2d.X()*myLayerPositions[i];
      double V = PC.Y() + aVec2d.Y()*myLayerPositions[i];
      meshDS->SetNodeOnFace( node, faceID, U, V );
      Pnts2d1.Append(gp_Pnt2d(U,V));
    }
    Nodes1[Nodes1.size()-1] = NF;
    Nodes2[Nodes1.size()-1] = NF;
  }
  else if(nbe==2 && LinEdge1.Orientation() != TopAbs_INTERNAL )
  {
    // one curve must be a half of circle and other curve must be
    // a segment of line
    double fp, lp;
    Handle(Geom_Circle) aCirc = Handle(Geom_Circle)::DownCast( getCurve( CircEdge, &fp, &lp ));
    if( fabs(fabs(lp-fp)-PI) > Precision::Confusion() ) {
      // not half of circle
      return error(COMPERR_BAD_SHAPE);
    }
    Handle(Geom_Line) aLine = Handle(Geom_Line)::DownCast( getCurve( LinEdge1 ));
    if( aLine.IsNull() ) {
      // other curve not line
      return error(COMPERR_BAD_SHAPE);
    }
    bool linEdgeComputed = false;
    if( SMESH_subMesh* sm1 = aMesh.GetSubMesh(LinEdge1) ) {
      if( !sm1->IsEmpty() )
        if( isEdgeCompitaballyMeshed( LinEdge1, aMesh.GetSubMesh(F) ))
          linEdgeComputed = true;
        else
          return error("Invalid set of hypotheses");
    }

    bool ok = _gen->Compute( aMesh, CircEdge );
    if( !ok ) return false;
    map< double, const SMDS_MeshNode* > theNodes;
    GetSortedNodesOnEdge(aMesh.GetMeshDS(),CircEdge,true,theNodes);

    CNodes.clear();
    map< double, const SMDS_MeshNode* >::iterator itn = theNodes.begin();
    double fang = (*itn).first;
    itn++;
    for(; itn != theNodes.end(); itn++ ) {
      CNodes.push_back( (*itn).second );
      double ang = (*itn).first - fang;
      if( ang>PI ) ang = ang - 2*PI;
      if( ang<-PI ) ang = ang + 2*PI;
      Angles.Append( ang );
    }
    const SMDS_MeshNode* NF = theNodes.begin()->second;
    const SMDS_MeshNode* NL = theNodes.rbegin()->second;
    CNodes.push_back( NF );
    P1 = gp_Pnt( NF->X(), NF->Y(), NF->Z() );
    gp_Pnt P2( NL->X(), NL->Y(), NL->Z() );
    P0 = aCirc->Location();

    myLayerPositions.clear();
    computeLayerPositions(P0,P1);

    if ( linEdgeComputed )
    {
      if (!GetSortedNodesOnEdge(aMesh.GetMeshDS(),LinEdge1,true,theNodes))
        return error("Invalid mesh on a straight edge");

      vector< const SMDS_MeshNode* > *pNodes1 = &Nodes1, *pNodes2 = &Nodes2;
      bool nodesFromP0ToP1 = ( theNodes.rbegin()->second == NF );
      if ( !nodesFromP0ToP1 ) std::swap( pNodes1, pNodes2 );

      map< double, const SMDS_MeshNode* >::reverse_iterator ritn = theNodes.rbegin();
      itn = theNodes.begin();
      for ( int i = Nodes1.size()-1; i > -1; ++itn, ++ritn, --i )
      {
        (*pNodes1)[i] = ritn->second;
        (*pNodes2)[i] =  itn->second;
        Points.Append( gpXYZ( Nodes1[i]));
        Pnts2d1.Append( myHelper->GetNodeUV( F, Nodes1[i]));
      }
      NC = const_cast<SMDS_MeshNode*>( itn->second );
      Points.Remove( Nodes1.size() );
    }
    else
    {
      gp_Vec aVec(P0,P1);
      int edgeID = meshDS->ShapeToIndex(LinEdge1);
      // check orientation
      Handle(Geom_Curve) Crv = BRep_Tool::Curve(LinEdge1,fp,lp);
      gp_Pnt Ptmp;
      Crv->D0(fp,Ptmp);
      bool ori = true;
      if( P1.Distance(Ptmp) > Precision::Confusion() )
        ori = false;
      // get UV points for edge
      gp_Pnt2d PF,PL;
      BRep_Tool::UVPoints( LinEdge1, TopoDS::Face(aShape), PF, PL );
      PC = gp_Pnt2d( (PF.X()+PL.X())/2, (PF.Y()+PL.Y())/2 );
      gp_Vec2d V2d;
      if(ori) V2d = gp_Vec2d(PC,PF);
      else V2d = gp_Vec2d(PC,PL);
      // add nodes on edge
      double cp = (fp+lp)/2;
      double dp2 = (lp-fp)/2;
      NC = meshDS->AddNode(P0.X(), P0.Y(), P0.Z());
      meshDS->SetNodeOnEdge(NC, edgeID, cp);
      Nodes1.resize( myLayerPositions.size()+1 );
      Nodes2.resize( myLayerPositions.size()+1 );
      int i = 0;
      for(; i<myLayerPositions.size(); i++) {
        gp_Pnt P( P0.X() + aVec.X()*myLayerPositions[i],
                  P0.Y() + aVec.Y()*myLayerPositions[i],
                  P0.Z() + aVec.Z()*myLayerPositions[i] );
        Points.Append(P);
        SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
        Nodes1[i] = node;
        double param;
        if(ori)
          param = fp + dp2*(1-myLayerPositions[i]);
        else
          param = cp + dp2*myLayerPositions[i];
        meshDS->SetNodeOnEdge(node, edgeID, param);
        P = gp_Pnt( P0.X() - aVec.X()*myLayerPositions[i],
                    P0.Y() - aVec.Y()*myLayerPositions[i],
                    P0.Z() - aVec.Z()*myLayerPositions[i] );
        node = meshDS->AddNode(P.X(), P.Y(), P.Z());
        Nodes2[i] = node;
        if(!ori)
          param = fp + dp2*(1-myLayerPositions[i]);
        else
          param = cp + dp2*myLayerPositions[i];
        meshDS->SetNodeOnEdge(node, edgeID, param);
        // parameters on face
        gp_Pnt2d P2d( PC.X() + V2d.X()*myLayerPositions[i],
                      PC.Y() + V2d.Y()*myLayerPositions[i] );
        Pnts2d1.Append(P2d);
      }
      Nodes1[ myLayerPositions.size() ] = NF;
      Nodes2[ myLayerPositions.size() ] = NL;
      // create 1D elements on edge
      vector< const SMDS_MeshNode* > tmpNodes;
      tmpNodes.resize(2*Nodes1.size()+1);
      for(i=0; i<Nodes2.size(); i++)
        tmpNodes[Nodes2.size()-i-1] = Nodes2[i];
      tmpNodes[Nodes2.size()] = NC;
      for(i=0; i<Nodes1.size(); i++)
        tmpNodes[Nodes2.size()+1+i] = Nodes1[i];
      for(i=1; i<tmpNodes.size(); i++) {
        SMDS_MeshEdge* ME = myHelper->AddEdge( tmpNodes[i-1], tmpNodes[i] );
        if(ME) meshDS->SetMeshElementOnShape(ME, edgeID);
      }
      markLinEdgeAsComputedByMe( LinEdge1, aMesh.GetSubMesh( F ));
    }
  }
  else // nbe==3 or ( nbe==2 && linEdge is INTERNAL )
  {
    if (nbe==2 && LinEdge1.Orientation() == TopAbs_INTERNAL )
      LinEdge2 = LinEdge1;

    // one curve must be a part of circle and other curves must be
    // segments of line
    double fp, lp;
    Handle(Geom_Circle) aCirc = Handle(Geom_Circle)::DownCast( getCurve( CircEdge ));
    Handle(Geom_Line) aLine1 = Handle(Geom_Line)::DownCast( getCurve( LinEdge1 ));
    Handle(Geom_Line) aLine2 = Handle(Geom_Line)::DownCast( getCurve( LinEdge2 ));
    if( aLine1.IsNull() || aLine2.IsNull() ) {
      // other curve not line
      return error(COMPERR_BAD_SHAPE);
    }

    bool linEdge1Computed = false;
    if ( SMESH_subMesh* sm1 = aMesh.GetSubMesh(LinEdge1))
      if( !sm1->IsEmpty() )
        if( isEdgeCompitaballyMeshed( LinEdge1, aMesh.GetSubMesh(F) ))
          linEdge1Computed = true;
        else
          return error("Invalid set of hypotheses");

    bool linEdge2Computed = false;
    if ( SMESH_subMesh* sm2 = aMesh.GetSubMesh(LinEdge2))
      if( !sm2->IsEmpty() )
        if( isEdgeCompitaballyMeshed( LinEdge2, aMesh.GetSubMesh(F)  ))
          linEdge2Computed = true;
        else
          return error("Invalid set of hypotheses");

    bool ok = _gen->Compute( aMesh, CircEdge );
    if( !ok ) return false;
    map< double, const SMDS_MeshNode* > theNodes;
    GetSortedNodesOnEdge(aMesh.GetMeshDS(),CircEdge,true,theNodes);

    const SMDS_MeshNode* NF = theNodes.begin()->second;
    const SMDS_MeshNode* NL = theNodes.rbegin()->second;
    CNodes.clear();
    CNodes.push_back( NF );
    map< double, const SMDS_MeshNode* >::iterator itn = theNodes.begin();
    double fang = (*itn).first;
    itn++;
    for(; itn != theNodes.end(); itn++ ) {
      CNodes.push_back( (*itn).second );
      double ang = (*itn).first - fang;
      if( ang>PI ) ang = ang - 2*PI;
      if( ang<-PI ) ang = ang + 2*PI;
      Angles.Append( ang );
    }
    P1 = gp_Pnt( NF->X(), NF->Y(), NF->Z() );
    gp_Pnt P2( NL->X(), NL->Y(), NL->Z() );
    P0 = aCirc->Location();

    myLayerPositions.clear();
    computeLayerPositions(P0,P1);

    Nodes1.resize( myLayerPositions.size()+1 );
    Nodes2.resize( myLayerPositions.size()+1 );

    exp.Init( LinEdge1, TopAbs_VERTEX );
    TopoDS_Vertex V1 = TopoDS::Vertex( exp.Current() );
    exp.Next();
    TopoDS_Vertex V2 = TopoDS::Vertex( exp.Current() );
    gp_Pnt PE1 = BRep_Tool::Pnt(V1);
    gp_Pnt PE2 = BRep_Tool::Pnt(V2);
    if( ( P1.Distance(PE1) > Precision::Confusion() ) &&
        ( P1.Distance(PE2) > Precision::Confusion() ) )
    {
      std::swap( LinEdge1, LinEdge2 );
      std::swap( linEdge1Computed, linEdge2Computed );
    }
    TopoDS_Vertex VC = V2;
    if( ( P1.Distance(PE1) > Precision::Confusion() ) &&
        ( P2.Distance(PE1) > Precision::Confusion() ) )
      VC = V1;
    int vertID = meshDS->ShapeToIndex(VC);

    // LinEdge1
    if ( linEdge1Computed )
    {
      if (!GetSortedNodesOnEdge(aMesh.GetMeshDS(),LinEdge1,true,theNodes))
        return error("Invalid mesh on a straight edge");

      bool nodesFromP0ToP1 = ( theNodes.rbegin()->second == NF );
      NC = const_cast<SMDS_MeshNode*>
        ( nodesFromP0ToP1 ? theNodes.begin()->second : theNodes.rbegin()->second );
      int i = 0, ir = Nodes1.size()-1;
      int * pi = nodesFromP0ToP1 ? &i : &ir;
      itn = theNodes.begin();
      if ( nodesFromP0ToP1 ) ++itn;
      for ( ; i < Nodes1.size(); ++i, --ir, ++itn )
      {
        Nodes1[*pi] = itn->second;
      }
      for ( i = 0; i < Nodes1.size()-1; ++i )
      {
        Points.Append( gpXYZ( Nodes1[i]));
        Pnts2d1.Append( myHelper->GetNodeUV( F, Nodes1[i]));
      }
    }
    else
    {
      int edgeID = meshDS->ShapeToIndex(LinEdge1);
      gp_Vec aVec(P0,P1);
      // check orientation
      Handle(Geom_Curve) Crv = BRep_Tool::Curve(LinEdge1,fp,lp);
      gp_Pnt Ptmp = Crv->Value(fp);
      bool ori = false;
      if( P1.Distance(Ptmp) > Precision::Confusion() )
        ori = true;
      // get UV points for edge
      gp_Pnt2d PF,PL;
      BRep_Tool::UVPoints( LinEdge1, TopoDS::Face(aShape), PF, PL );
      gp_Vec2d V2d;
      if(ori) {
        V2d = gp_Vec2d(PF,PL);
        PC = PF;
      }
      else {
        V2d = gp_Vec2d(PL,PF);
        PC = PL;
      }
      NC = const_cast<SMDS_MeshNode*>( VertexNode( VC, meshDS ));
      if ( !NC )
      {
        NC = meshDS->AddNode(P0.X(), P0.Y(), P0.Z());
        meshDS->SetNodeOnVertex(NC, vertID);
      }
      double dp = lp-fp;
      int i = 0;
      for(; i<myLayerPositions.size(); i++) {
        gp_Pnt P( P0.X() + aVec.X()*myLayerPositions[i],
                  P0.Y() + aVec.Y()*myLayerPositions[i],
                  P0.Z() + aVec.Z()*myLayerPositions[i] );
        Points.Append(P);
        SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
        Nodes1[i] = node;
        double param;
        if(!ori)
          param = fp + dp*(1-myLayerPositions[i]);
        else
          param = fp + dp*myLayerPositions[i];
        meshDS->SetNodeOnEdge(node, edgeID, param);
        // parameters on face
        gp_Pnt2d P2d( PC.X() + V2d.X()*myLayerPositions[i],
                      PC.Y() + V2d.Y()*myLayerPositions[i] );
        Pnts2d1.Append(P2d);
      }
      Nodes1[ myLayerPositions.size() ] = NF;
      // create 1D elements on edge
      SMDS_MeshEdge* ME = myHelper->AddEdge( NC, Nodes1[0] );
      if(ME) meshDS->SetMeshElementOnShape(ME, edgeID);
      for(i=1; i<Nodes1.size(); i++) {
        ME = myHelper->AddEdge( Nodes1[i-1], Nodes1[i] );
        if(ME) meshDS->SetMeshElementOnShape(ME, edgeID);
      }
      if (nbe==2 && LinEdge1.Orientation() == TopAbs_INTERNAL )
        Nodes2 = Nodes1;
    }
    markLinEdgeAsComputedByMe( LinEdge1, aMesh.GetSubMesh( F ));

    // LinEdge2
    if ( linEdge2Computed )
    {
      if (!GetSortedNodesOnEdge(aMesh.GetMeshDS(),LinEdge2,true,theNodes))
        return error("Invalid mesh on a straight edge");

      bool nodesFromP0ToP2 = ( theNodes.rbegin()->second == NL );
      int i = 0, ir = Nodes1.size()-1;
      int * pi = nodesFromP0ToP2 ? &i : &ir;
      itn = theNodes.begin();
      if ( nodesFromP0ToP2 ) ++itn;
      for ( ; i < Nodes2.size(); ++i, --ir, ++itn )
        Nodes2[*pi] = itn->second;
    }
    else
    {
      int edgeID = meshDS->ShapeToIndex(LinEdge2);
      gp_Vec aVec = gp_Vec(P0,P2);
      // check orientation
      Handle(Geom_Curve) Crv = BRep_Tool::Curve(LinEdge2,fp,lp);
      gp_Pnt Ptmp = Crv->Value(fp);
      bool ori = false;
      if( P2.Distance(Ptmp) > Precision::Confusion() )
        ori = true;
      // get UV points for edge
      gp_Pnt2d PF,PL;
      BRep_Tool::UVPoints( LinEdge2, TopoDS::Face(aShape), PF, PL );
      gp_Vec2d V2d;
      if(ori) {
        V2d = gp_Vec2d(PF,PL);
        PC = PF;
      }
      else {
        V2d = gp_Vec2d(PL,PF);
        PC = PL;
      }
      double dp = lp-fp;
      for(int i=0; i<myLayerPositions.size(); i++) {
        gp_Pnt P( P0.X() + aVec.X()*myLayerPositions[i],
                  P0.Y() + aVec.Y()*myLayerPositions[i],
                  P0.Z() + aVec.Z()*myLayerPositions[i] );
        SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
        Nodes2[i] = node;
        double param;
        if(!ori)
          param = fp + dp*(1-myLayerPositions[i]);
        else
          param = fp + dp*myLayerPositions[i];
        meshDS->SetNodeOnEdge(node, edgeID, param);
        // parameters on face
        gp_Pnt2d P2d( PC.X() + V2d.X()*myLayerPositions[i],
                      PC.Y() + V2d.Y()*myLayerPositions[i] );
      }
      Nodes2[ myLayerPositions.size() ] = NL;
      // create 1D elements on edge
      SMDS_MeshEdge* ME = myHelper->AddEdge( NC, Nodes2[0] );
      if(ME) meshDS->SetMeshElementOnShape(ME, edgeID);
      for(int i=1; i<Nodes2.size(); i++) {
        ME = myHelper->AddEdge( Nodes2[i-1], Nodes2[i] );
        if(ME) meshDS->SetMeshElementOnShape(ME, edgeID);
      }
    }
    markLinEdgeAsComputedByMe( LinEdge2, aMesh.GetSubMesh( F ));
  }

  // orientation
  bool IsForward = ( CircEdge.Orientation()==TopAbs_FORWARD );

  // create nodes and mesh elements on face
  // find axis of rotation
  gp_Pnt P2 = gp_Pnt( CNodes[1]->X(), CNodes[1]->Y(), CNodes[1]->Z() );
  gp_Vec Vec1(P0,P1);
  gp_Vec Vec2(P0,P2);
  gp_Vec Axis = Vec1.Crossed(Vec2);
  // create elements
  int i = 1;
  //cout<<"Angles.Length() = "<<Angles.Length()<<"   Points.Length() = "<<Points.Length()<<endl;
  //cout<<"Nodes1.size() = "<<Nodes1.size()<<"   Pnts2d1.Length() = "<<Pnts2d1.Length()<<endl;
  for(; i<Angles.Length(); i++) {
    vector< const SMDS_MeshNode* > tmpNodes;
    tmpNodes.reserve(Nodes1.size());
    gp_Trsf aTrsf;
    gp_Ax1 theAxis(P0,gp_Dir(Axis));
    aTrsf.SetRotation( theAxis, Angles.Value(i) );
    gp_Trsf2d aTrsf2d;
    aTrsf2d.SetRotation( PC, Angles.Value(i) );
    // create nodes
    int j = 1;
    for(; j<=Points.Length(); j++) {
      double cx,cy,cz;
      Points.Value(j).Coord( cx, cy, cz );
      aTrsf.Transforms( cx, cy, cz );
      SMDS_MeshNode* node = myHelper->AddNode( cx, cy, cz );
      // find parameters on face
      Pnts2d1.Value(j).Coord( cx, cy );
      aTrsf2d.Transforms( cx, cy );
      // set node on face
      meshDS->SetNodeOnFace( node, faceID, cx, cy );
      tmpNodes[j-1] = node;
    }
    // create faces
    tmpNodes[Points.Length()] = CNodes[i];
    // quad
    for(j=0; j<Nodes1.size()-1; j++) {
      SMDS_MeshFace* MF;
      if(IsForward)
        MF = myHelper->AddFace( tmpNodes[j], Nodes1[j],
                                Nodes1[j+1], tmpNodes[j+1] );
      else
        MF = myHelper->AddFace( tmpNodes[j], tmpNodes[j+1],
                                Nodes1[j+1], Nodes1[j] );
      if(MF) meshDS->SetMeshElementOnShape(MF, faceID);
    }
    // tria
    SMDS_MeshFace* MF;
    if(IsForward)
      MF = myHelper->AddFace( NC, Nodes1[0], tmpNodes[0] );
    else
      MF = myHelper->AddFace( NC, tmpNodes[0], Nodes1[0] );
    if(MF) meshDS->SetMeshElementOnShape(MF, faceID);
    for(j=0; j<Nodes1.size(); j++) {
      Nodes1[j] = tmpNodes[j];
    }
  }
  // create last faces
  // quad
  for(i=0; i<Nodes1.size()-1; i++) {
    SMDS_MeshFace* MF;
    if(IsForward)
      MF = myHelper->AddFace( Nodes2[i], Nodes1[i],
                              Nodes1[i+1], Nodes2[i+1] );
    else
      MF = myHelper->AddFace( Nodes2[i],  Nodes2[i+1],
                              Nodes1[i+1], Nodes1[i] );
    if(MF) meshDS->SetMeshElementOnShape(MF, faceID);
  }
  // tria
  SMDS_MeshFace* MF;
  if(IsForward)
    MF = myHelper->AddFace( NC, Nodes1[0], Nodes2[0] );
  else
    MF = myHelper->AddFace( NC, Nodes2[0], Nodes1[0] );
  if(MF) meshDS->SetMeshElementOnShape(MF, faceID);

  return true;
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
    map < int, SMESH_1D_Algo * > & algoMap = aMesh.GetGen()->_map1D_Algo;
    map < int, SMESH_1D_Algo * >::iterator id_algo = algoMap.find( myID );
    if ( id_algo == algoMap.end() )
      return new TNodeDistributor( myID, 0, aMesh.GetGen() );
    return static_cast< TNodeDistributor* >( id_algo->second );
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

bool StdMeshers_RadialQuadrangle_1D2D::computeLayerPositions(const gp_Pnt& pIn,
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

bool StdMeshers_RadialQuadrangle_1D2D::Evaluate(SMESH_Mesh& aMesh,
                                                const TopoDS_Shape& aShape,
                                                MapShapeNbElems& aResMap)
{
  if( aShape.ShapeType() != TopAbs_FACE ) {
    return false;
  }
  SMESH_subMesh * smf = aMesh.GetSubMesh(aShape);
  MapShapeNbElemsItr anIt = aResMap.find(smf);
  if( anIt != aResMap.end() ) {
    return false;
  }

  myLayerPositions.clear();
  gp_Pnt P0(0,0,0);
  gp_Pnt P1(100,0,0);
  computeLayerPositions(P0,P1);

  TopoDS_Edge E1,E2,E3;
  Handle(Geom_Curve) C1,C2,C3;
  double f1,l1,f2,l2,f3,l3;
  int nbe = 0;
  TopExp_Explorer exp;
  for ( exp.Init( aShape, TopAbs_EDGE ); exp.More(); exp.Next() ) {
    nbe++;
    TopoDS_Edge E = TopoDS::Edge( exp.Current() );
    if(nbe==1) {
      E1 = E;
      C1 = BRep_Tool::Curve(E,f1,l1);
    }
    else if(nbe==2) {
      E2 = E;
      C2 = BRep_Tool::Curve(E,f2,l2);
    }
    else if(nbe==3) {
      E3 = E;
      C3 = BRep_Tool::Curve(E,f3,l3);
    }
  }

  TopoDS_Edge CircEdge, LinEdge1, LinEdge2;
  int nb0d=0, nb2d_tria=0, nb2d_quad=0;
  bool isQuadratic = false;
  if(nbe==1) {
    // C1 must be a circle
    Handle(Geom_Circle) aCirc = Handle(Geom_Circle)::DownCast(C1);
    if( !aCirc.IsNull() ) {
      bool ok = _gen->Evaluate( aMesh, CircEdge, aResMap );
      if(ok) {
        SMESH_subMesh * sm = aMesh.GetSubMesh(CircEdge);
        MapShapeNbElemsItr anIt = aResMap.find(sm);
        vector<int> aVec = (*anIt).second;
        isQuadratic = aVec[SMDSEntity_Quad_Edge]>aVec[SMDSEntity_Edge];
        if(isQuadratic) {
          // main nodes
          nb0d = (aVec[SMDSEntity_Node]+1) * myLayerPositions.size();
          // radial medium nodes
          nb0d += (aVec[SMDSEntity_Node]+1) * (myLayerPositions.size()+1);
          // other medium nodes
          nb0d += (aVec[SMDSEntity_Node]+1) * myLayerPositions.size();
        }
        else {
          nb0d = (aVec[SMDSEntity_Node]+1) * myLayerPositions.size();
        }
        nb2d_tria = aVec[SMDSEntity_Node] + 1;
        nb2d_quad = nb0d;
      }
    }
  }
  else if(nbe==2) {
    // one curve must be a half of circle and other curve must be
    // a segment of line
    Handle(Geom_TrimmedCurve) tc = Handle(Geom_TrimmedCurve)::DownCast(C1);
    while( !tc.IsNull() ) {
      C1 = tc->BasisCurve();
      tc = Handle(Geom_TrimmedCurve)::DownCast(C1);
    }
    tc = Handle(Geom_TrimmedCurve)::DownCast(C2);
    while( !tc.IsNull() ) {
      C2 = tc->BasisCurve();
      tc = Handle(Geom_TrimmedCurve)::DownCast(C2);
    }
    Handle(Geom_Circle) aCirc = Handle(Geom_Circle)::DownCast(C1);
    Handle(Geom_Line) aLine = Handle(Geom_Line)::DownCast(C2);
    CircEdge = E1;
    LinEdge1 = E2;
    double fp = f1;
    double lp = l1;
    if( aCirc.IsNull() ) {
      aCirc = Handle(Geom_Circle)::DownCast(C2);
      CircEdge = E2;
      LinEdge1 = E1;
      fp = f2;
      lp = l2;
      aLine = Handle(Geom_Line)::DownCast(C3);
    }
    bool ok = !aCirc.IsNull() && !aLine.IsNull();
    if( fabs(fabs(lp-fp)-PI) > Precision::Confusion() ) {
      // not half of circle
      ok = false;
    }
    SMESH_subMesh* sm1 = aMesh.GetSubMesh(LinEdge1);
    MapShapeNbElemsItr anIt = aResMap.find(sm1);
    if( anIt!=aResMap.end() ) {
      ok = false;
    }
    if(ok) {
      ok = _gen->Evaluate( aMesh, CircEdge, aResMap );
    }
    if(ok) {
      SMESH_subMesh * sm = aMesh.GetSubMesh(CircEdge);
      MapShapeNbElemsItr anIt = aResMap.find(sm);
      vector<int> aVec = (*anIt).second;
      isQuadratic = aVec[SMDSEntity_Quad_Edge]>aVec[SMDSEntity_Edge];
      if(isQuadratic) {
        // main nodes
        nb0d = aVec[SMDSEntity_Node] * myLayerPositions.size();
        // radial medium nodes
        nb0d += aVec[SMDSEntity_Node] * (myLayerPositions.size()+1);
        // other medium nodes
        nb0d += (aVec[SMDSEntity_Node]+1) * myLayerPositions.size();
      }
      else {
        nb0d = aVec[SMDSEntity_Node] * myLayerPositions.size();
      }
      nb2d_tria = aVec[SMDSEntity_Node] + 1;
      nb2d_quad = nb2d_tria * myLayerPositions.size();
      // add evaluation for edges
      vector<int> aResVec(SMDSEntity_Last);
      for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
      if(isQuadratic) {
        aResVec[SMDSEntity_Node] = 4*myLayerPositions.size() + 3;
        aResVec[SMDSEntity_Quad_Edge] = 2*myLayerPositions.size() + 2;
      }
      else {
        aResVec[SMDSEntity_Node] = 2*myLayerPositions.size() + 1;
        aResVec[SMDSEntity_Edge] = 2*myLayerPositions.size() + 2;
      }
      sm = aMesh.GetSubMesh(LinEdge1);
      aResMap.insert(make_pair(sm,aResVec));
    }
  }
  else { // nbe==3
    // one curve must be a part of circle and other curves must be
    // segments of line
    Handle(Geom_TrimmedCurve) tc = Handle(Geom_TrimmedCurve)::DownCast(C1);
    while( !tc.IsNull() ) {
      C1 = tc->BasisCurve();
      tc = Handle(Geom_TrimmedCurve)::DownCast(C1);
    }
    tc = Handle(Geom_TrimmedCurve)::DownCast(C2);
    while( !tc.IsNull() ) {
      C2 = tc->BasisCurve();
      tc = Handle(Geom_TrimmedCurve)::DownCast(C2);
    }
    tc = Handle(Geom_TrimmedCurve)::DownCast(C3);
    while( !tc.IsNull() ) {
      C3 = tc->BasisCurve();
      tc = Handle(Geom_TrimmedCurve)::DownCast(C3);
    }
    Handle(Geom_Circle) aCirc = Handle(Geom_Circle)::DownCast(C1);
    Handle(Geom_Line) aLine1 = Handle(Geom_Line)::DownCast(C2);
    Handle(Geom_Line) aLine2 = Handle(Geom_Line)::DownCast(C3);
    CircEdge = E1;
    LinEdge1 = E2;
    LinEdge2 = E3;
    double fp = f1;
    double lp = l1;
    if( aCirc.IsNull() ) {
      aCirc = Handle(Geom_Circle)::DownCast(C2);
      CircEdge = E2;
      LinEdge1 = E3;
      LinEdge2 = E1;
      fp = f2;
      lp = l2;
      aLine1 = Handle(Geom_Line)::DownCast(C3);
      aLine2 = Handle(Geom_Line)::DownCast(C1);
      if( aCirc.IsNull() ) {
        aCirc = Handle(Geom_Circle)::DownCast(C3);
        CircEdge = E3;
        LinEdge1 = E1;
        LinEdge2 = E2;
        fp = f3;
        lp = l3;
        aLine1 = Handle(Geom_Line)::DownCast(C1);
        aLine2 = Handle(Geom_Line)::DownCast(C2);
      }
    }
    bool ok = !aCirc.IsNull() && !aLine1.IsNull() && !aLine1.IsNull();
    SMESH_subMesh* sm = aMesh.GetSubMesh(LinEdge1);
    MapShapeNbElemsItr anIt = aResMap.find(sm);
    if( anIt!=aResMap.end() ) {
      ok = false;
    }
    sm = aMesh.GetSubMesh(LinEdge2);
    anIt = aResMap.find(sm);
    if( anIt!=aResMap.end() ) {
      ok = false;
    }
    if(ok) {
      ok = _gen->Evaluate( aMesh, CircEdge, aResMap );
    }
    if(ok) {
      SMESH_subMesh * sm = aMesh.GetSubMesh(CircEdge);
      MapShapeNbElemsItr anIt = aResMap.find(sm);
      vector<int> aVec = (*anIt).second;
      isQuadratic = aVec[SMDSEntity_Quad_Edge]>aVec[SMDSEntity_Edge];
      if(isQuadratic) {
        // main nodes
        nb0d = aVec[SMDSEntity_Node] * myLayerPositions.size();
        // radial medium nodes
        nb0d += aVec[SMDSEntity_Node] * (myLayerPositions.size()+1);
        // other medium nodes
        nb0d += (aVec[SMDSEntity_Node]+1) * myLayerPositions.size();
      }
      else {
        nb0d = aVec[SMDSEntity_Node] * myLayerPositions.size();
      }
      nb2d_tria = aVec[SMDSEntity_Node] + 1;
      nb2d_quad = nb2d_tria * myLayerPositions.size();
      // add evaluation for edges
      vector<int> aResVec(SMDSEntity_Last);
      for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
      if(isQuadratic) {
        aResVec[SMDSEntity_Node] = 2*myLayerPositions.size() + 1;
        aResVec[SMDSEntity_Quad_Edge] = myLayerPositions.size() + 1;
      }
      else {
        aResVec[SMDSEntity_Node] = myLayerPositions.size();
        aResVec[SMDSEntity_Edge] = myLayerPositions.size() + 1;
      }
      sm = aMesh.GetSubMesh(LinEdge1);
      aResMap.insert(make_pair(sm,aResVec));
      sm = aMesh.GetSubMesh(LinEdge2);
      aResMap.insert(make_pair(sm,aResVec));
    }
  }

  vector<int> aResVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
  SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);

  //cout<<"nb0d = "<<nb0d<<"   nb2d_tria = "<<nb2d_tria<<"   nb2d_quad = "<<nb2d_quad<<endl;
  if(nb0d>0) {
    aResVec[0] = nb0d;
    if(isQuadratic) {
      aResVec[SMDSEntity_Quad_Triangle] = nb2d_tria;
      aResVec[SMDSEntity_Quad_Quadrangle] = nb2d_quad;
    }
    else {
      aResVec[SMDSEntity_Triangle] = nb2d_tria;
      aResVec[SMDSEntity_Quadrangle] = nb2d_quad;
    }
    aResMap.insert(make_pair(sm,aResVec));
    return true;
  }

  // invalid case
  aResMap.insert(make_pair(sm,aResVec));
  SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
  smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,
                                        "Submesh can not be evaluated",this));
  return false;

}

