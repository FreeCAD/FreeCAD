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
//  File   : StdMeshers_MEFISTO_2D.cxx
//           Moved here from SMESH_MEFISTO_2D.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_MEFISTO_2D.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_LengthFromEdges.hxx"
#include "StdMeshers_MaxElementArea.hxx"
#include "StdMeshers_ViscousLayers2D.hxx"

#include "utilities.h"

#include "Rn.h"
#include "aptrte.h"

#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Pnt2d.hxx>

using namespace std;

#ifdef _DEBUG_
//#define DUMP_POINTS // to print coordinates of MEFISTO input
#endif

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_MEFISTO_2D::StdMeshers_MEFISTO_2D(int hypId, int studyId, SMESH_Gen * gen):
  SMESH_2D_Algo(hypId, studyId, gen)
{
  MESSAGE("StdMeshers_MEFISTO_2D::StdMeshers_MEFISTO_2D");
  _name = "MEFISTO_2D";
  _shapeType = (1 << TopAbs_FACE);
  _compatibleHypothesis.push_back("MaxElementArea");
  _compatibleHypothesis.push_back("LengthFromEdges");
  _compatibleHypothesis.push_back("ViscousLayers2D");

  _edgeLength = 0;
  _maxElementArea = 0;
  _hypMaxElementArea = NULL;
  _hypLengthFromEdges = NULL;
  _helper = 0;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_MEFISTO_2D::~StdMeshers_MEFISTO_2D()
{
  MESSAGE("StdMeshers_MEFISTO_2D::~StdMeshers_MEFISTO_2D");
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_MEFISTO_2D::CheckHypothesis
                         (SMESH_Mesh&                          aMesh,
                          const TopoDS_Shape&                  aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  _hypMaxElementArea = NULL;
  _hypLengthFromEdges = NULL;
  _edgeLength = 0;
  _maxElementArea = 0;

  if ( !error( StdMeshers_ViscousLayers2D::CheckHypothesis( aMesh, aShape, aStatus )))
    return false;

  list <const SMESHDS_Hypothesis * >::const_iterator itl;
  const SMESHDS_Hypothesis *theHyp;

  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(aMesh, aShape);
  int nbHyp = hyps.size();
  if (!nbHyp)
  {
    aStatus = SMESH_Hypothesis::HYP_OK; //SMESH_Hypothesis::HYP_MISSING;
    return true;  // (PAL13464) can work with no hypothesis, LengthFromEdges is default one
  }

  itl = hyps.begin();
  theHyp = (*itl); // use only the first hypothesis

  string hypName = theHyp->GetName();

  bool isOk = false;

  if (hypName == "MaxElementArea")
  {
    _hypMaxElementArea = static_cast<const StdMeshers_MaxElementArea *>(theHyp);
    ASSERT(_hypMaxElementArea);
    _maxElementArea = _hypMaxElementArea->GetMaxArea();
    isOk = true;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if (hypName == "LengthFromEdges")
  {
    _hypLengthFromEdges = static_cast<const StdMeshers_LengthFromEdges *>(theHyp);
    ASSERT(_hypLengthFromEdges);
    isOk = true;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }
  else
    aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;

  if (isOk)
  {
    isOk = false;
    if (_maxElementArea > 0)
    {
      //_edgeLength = 2 * sqrt(_maxElementArea);        // triangles : minorant
      _edgeLength = sqrt(2. * _maxElementArea/sqrt(3.0));
      isOk = true;
    }
    else
      isOk = (_hypLengthFromEdges != NULL);     // **** check mode
    if (!isOk)
      aStatus = SMESH_Hypothesis::HYP_BAD_PARAMETER;
  }

  return isOk;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_MEFISTO_2D::Compute(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape)
{
  MESSAGE("StdMeshers_MEFISTO_2D::Compute");

  TopoDS_Face F = TopoDS::Face(aShape.Oriented(TopAbs_FORWARD));

  // helper builds quadratic mesh if necessary
  SMESH_MesherHelper helper(aMesh);
  _helper = &helper;
  _quadraticMesh = _helper->IsQuadraticSubMesh(aShape);
  const bool skipMediumNodes = _quadraticMesh;

  // build viscous layers if required
  SMESH_ProxyMesh::Ptr proxyMesh = StdMeshers_ViscousLayers2D::Compute( aMesh, F );
  if ( !proxyMesh )
    return false;

  // get all edges of a face
  TError problem;
  TWireVector wires =
    StdMeshers_FaceSide::GetFaceWires( F, aMesh, skipMediumNodes, problem, proxyMesh );
  int nbWires = wires.size();
  if ( problem && !problem->IsOK() ) return error( problem );
  if ( nbWires == 0 ) return error( "Problem in StdMeshers_FaceSide::GetFaceWires()");
  if ( wires[0]->NbSegments() < 3 ) // ex: a circle with 2 segments
    return error(COMPERR_BAD_INPUT_MESH,
                 SMESH_Comment("Too few segments: ")<<wires[0]->NbSegments());

  // compute average edge length
  if (!_hypMaxElementArea)
  {
    _edgeLength = 0;
    int nbSegments = 0;
    for ( int iW = 0; iW < nbWires; ++iW )
    {
      StdMeshers_FaceSidePtr wire = wires[ iW ];
      _edgeLength += wire->Length();
      nbSegments  += wire->NbSegments();
    }
    if ( nbSegments )
      _edgeLength /= nbSegments;
  }

  if (/*_hypLengthFromEdges &&*/ _edgeLength < DBL_MIN )
    _edgeLength = 100;

  Z nblf;                 //nombre de lignes fermees (enveloppe en tete)
  Z *nudslf = NULL;       //numero du dernier sommet de chaque ligne fermee
  R_2 *uvslf = NULL;
  Z nbpti = 0;            //nombre points internes futurs sommets de la triangulation
  R_2 *uvpti = NULL;
  
  Z nbst;
  R_2 *uvst = NULL;
  Z nbt;
  Z *nust = NULL;
  Z ierr = 0;

  Z nutysu = 1;           // 1: il existe un fonction areteideale_()
  // Z  nutysu=0;         // 0: on utilise aretmx
  R aretmx = _edgeLength; // longueur max aretes future triangulation
  if ( _hypMaxElementArea )
    aretmx *= 1.5;
  
  nblf = nbWires;
  
  nudslf = new Z[1 + nblf];
  nudslf[0] = 0;
  int iw = 1;
  int nbpnt = 0;

  // count nb of input points
  for ( int iW = 0; iW < nbWires; ++iW )
  {
    nbpnt += wires[iW]->NbPoints() - 1;
    nudslf[iw++] = nbpnt;
  }

  uvslf = new R_2[nudslf[nblf]];

  double scalex, scaley;
  ComputeScaleOnFace(aMesh, F, scalex, scaley);

  // correspondence mefisto index --> Nodes
  vector< const SMDS_MeshNode*> mefistoToDS(nbpnt, (const SMDS_MeshNode*)0);

  bool isOk = false;

  // fill input points UV
  if ( LoadPoints(wires, uvslf, mefistoToDS, scalex, scaley) )
  {
    // Compute
    aptrte(nutysu, aretmx,
           nblf, nudslf, uvslf, nbpti, uvpti, nbst, uvst, nbt, nust, ierr);

    if (ierr == 0)
    {
      MESSAGE("... End Triangulation Generated Triangle Number " << nbt);
      MESSAGE("                                    Node Number " << nbst);
      StoreResult(nbst, uvst, nbt, nust, mefistoToDS, scalex, scaley);
      isOk = true;
    }
    else
    {
      error(ierr,"Error in Triangulation (aptrte())");
    }
  }
  if (nudslf != NULL) delete[]nudslf;
  if (uvslf != NULL)  delete[]uvslf;
  if (uvst != NULL)   delete[]uvst;
  if (nust != NULL)   delete[]nust;

  return isOk;
}


//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_MEFISTO_2D::Evaluate(SMESH_Mesh & aMesh,
                                     const TopoDS_Shape & aShape,
                                     MapShapeNbElems& aResMap)
{
  MESSAGE("StdMeshers_MEFISTO_2D::Evaluate");

  TopoDS_Face F = TopoDS::Face(aShape.Oriented(TopAbs_FORWARD));

  double aLen = 0.0;
  int NbSeg = 0;
  bool IsQuadratic = false;
  bool IsFirst = true;
  TopExp_Explorer exp(F,TopAbs_EDGE);
  for(; exp.More(); exp.Next()) {
    TopoDS_Edge E = TopoDS::Edge(exp.Current());
    MapShapeNbElemsItr anIt = aResMap.find( aMesh.GetSubMesh(E) );
    if( anIt == aResMap.end() ) continue;
    std::vector<int> aVec = (*anIt).second;
    int nbe = Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
    NbSeg += nbe;
    if(IsFirst) {
      IsQuadratic = ( aVec[SMDSEntity_Quad_Edge] > aVec[SMDSEntity_Edge] );
      IsFirst = false;
    }
    double a,b;
    TopLoc_Location L;
    Handle(Geom_Curve) C = BRep_Tool::Curve(E,L,a,b);
    gp_Pnt P1;
    C->D0(a,P1);
    double dp = (b-a)/nbe;
    for(int i=1; i<=nbe; i++) {
      gp_Pnt P2;
      C->D0(a+i*dp,P2);
      aLen += P1.Distance(P2);
      P1 = P2;
    }
  }
  if(NbSeg<1) {
    std::vector<int> aResVec(SMDSEntity_Last);
    for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
    SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
    aResMap.insert(std::make_pair(sm,aResVec));
    SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
    smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,
                                          "Submesh can not be evaluated",this));
    return false;
  }
  aLen = aLen/NbSeg; // middle length

  _edgeLength = Precision::Infinite();
  double tmpLength = Min( _edgeLength, aLen );

  GProp_GProps G;
  BRepGProp::SurfaceProperties(aShape,G);
  double anArea = G.Mass();

  int nbFaces = Precision::IsInfinite( tmpLength ) ? 0 :
    (int)( anArea/(tmpLength*tmpLength*sqrt(3.)/4) );
  int nbNodes = (int) ( nbFaces*3 - (NbSeg-1)*2 ) / 6;

  std::vector<int> aVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i] = 0;
  if(IsQuadratic) {
    aVec[SMDSEntity_Quad_Triangle] = nbFaces;
    aVec[SMDSEntity_Node] = (int)( nbNodes + nbFaces*3 - (NbSeg-1) );
  }
  else {
    aVec[SMDSEntity_Node] = nbNodes;
    aVec[SMDSEntity_Triangle] = nbFaces;
  }
  SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
  aResMap.insert(std::make_pair(sm,aVec));

  return true;
}


//=======================================================================
//function : fixOverlappedLinkUV
//purpose  : prevent failure due to overlapped adjacent links
//=======================================================================

static bool fixOverlappedLinkUV( R_2& uv0, const R_2& uv1, const R_2& uv2 )
{
  gp_XY v1( uv0.x - uv1.x, uv0.y - uv1.y );
  gp_XY v2( uv2.x - uv1.x, uv2.y - uv1.y );

  double tol2 = DBL_MIN * DBL_MIN;
  double sqMod1 = v1.SquareModulus();
  if ( sqMod1 <= tol2 ) return false;
  double sqMod2 = v2.SquareModulus();
  if ( sqMod2 <= tol2 ) return false;

  double dot = v1*v2;

  // check sinus >= 1.e-3
  const double minSin = 1.e-3;
  if ( dot > 0 && 1 - dot * dot / ( sqMod1 * sqMod2 ) < minSin * minSin ) {
    MESSAGE(" ___ FIX UV ____" << uv0.x << " " << uv0.y);
    v1.SetCoord( -v1.Y(), v1.X() );
    double delta = sqrt( sqMod1 ) * minSin;
    if ( v1.X() < 0 )
      uv0.x -= delta;
    else
      uv0.x += delta;
    if ( v1.Y() < 0 )
      uv0.y -= delta;
    else
      uv0.y += delta;
// #ifdef _DEBUG_
//     MESSAGE(" -> " << uv0.x << " " << uv0.y << " ");
//     MESSAGE("v1( " << v1.X() << " " << v1.Y() << " ) " <<
//       "v2( " << v2.X() << " " << v2.Y() << " ) ");
//    MESSAGE("SIN: " << sqrt(1 - dot * dot / (sqMod1 * sqMod2)));
//     v1.SetCoord( uv0.x - uv1.x, uv0.y - uv1.y );
//     v2.SetCoord( uv2.x - uv1.x, uv2.y - uv1.y );
//     gp_XY v3( uv2.x - uv0.x, uv2.y - uv0.y );
//     sqMod1 = v1.SquareModulus();
//     sqMod2 = v2.SquareModulus();
//     dot = v1*v2;
//     double sin = sqrt(1 - dot * dot / (sqMod1 * sqMod2));
//     MESSAGE("NEW SIN: " << sin);
// #endif
    return true;
  }
  return false;
}

//=======================================================================
//function : fixCommonVertexUV
//purpose  : 
//=======================================================================

static bool fixCommonVertexUV (R_2 &                 theUV,
                               const TopoDS_Vertex& theV,
                               const TopoDS_Face&   theF,
                               const TopTools_IndexedDataMapOfShapeListOfShape & theVWMap,
                               SMESH_Mesh &         theMesh,
                               const double         theScaleX,
                               const double         theScaleY,
                               const bool           theCreateQuadratic)
{
  if( !theVWMap.Contains( theV )) return false;

  // check if there is another wire sharing theV
  const TopTools_ListOfShape& WList = theVWMap.FindFromKey( theV );
  TopTools_ListIteratorOfListOfShape aWIt;
  TopTools_MapOfShape aWires;
  for ( aWIt.Initialize( WList ); aWIt.More(); aWIt.Next() )
    aWires.Add( aWIt.Value() );
  if ( aWires.Extent() < 2 ) return false;

  TopoDS_Shape anOuterWire = BRepTools::OuterWire(theF);
  TopoDS_Shape anInnerWire;
  for ( aWIt.Initialize( WList ); aWIt.More() && anInnerWire.IsNull(); aWIt.Next() )
    if ( !anOuterWire.IsSame( aWIt.Value() ))
      anInnerWire = aWIt.Value();

  TopTools_ListOfShape EList;
  list< double >       UList;

  // find edges of theW sharing theV
  // and find 2d normal to them at theV
  gp_Vec2d N(0.,0.);
  TopoDS_Iterator itE( anInnerWire );
  for (  ; itE.More(); itE.Next() )
  {
    const TopoDS_Edge& E = TopoDS::Edge( itE.Value() );
    TopoDS_Iterator itV( E );
    for ( ; itV.More(); itV.Next() )
    {
      const TopoDS_Vertex & V = TopoDS::Vertex( itV.Value() );
      if ( !V.IsSame( theV ))
        continue;
      EList.Append( E );
      Standard_Real u = BRep_Tool::Parameter( V, E );
      UList.push_back( u );
      double f, l;
      Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface(E, theF, f, l);
      gp_Vec2d d1;
      gp_Pnt2d p;
      C2d->D1( u, p, d1 );
      gp_Vec2d n( d1.Y() * theScaleX, -d1.X() * theScaleY);
      if ( E.Orientation() == TopAbs_REVERSED )
        n.Reverse();
      N += n.Normalized();
    }
  }

  // define step size by which to move theUV

  gp_Pnt2d nextUV; // uv of next node on edge, most distant of the four
  gp_Pnt2d thisUV( theUV.x, theUV.y );
  double maxDist = -DBL_MAX;
  TopTools_ListIteratorOfListOfShape aEIt (EList);
  list< double >::iterator aUIt = UList.begin();
  for ( ; aEIt.More(); aEIt.Next(), aUIt++ )
  {
    const TopoDS_Edge& E = TopoDS::Edge( aEIt.Value() );
    double f, l;
    Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface(E, theF, f, l);

    double umin = DBL_MAX, umax = -DBL_MAX;
    SMDS_NodeIteratorPtr nIt = theMesh.GetSubMesh(E)->GetSubMeshDS()->GetNodes();
    if ( !nIt->more() ) // no nodes on edge, only on vertices
    {
      umin = l;
      umax = f;
    }
    else {
      while ( nIt->more() ) {
        const SMDS_MeshNode* node = nIt->next();
        // check if node is medium
        if ( theCreateQuadratic && SMESH_MesherHelper::IsMedium( node, SMDSAbs_Edge ))
          continue;
        const SMDS_EdgePosition* epos =
          static_cast<const SMDS_EdgePosition*>(node->GetPosition());
        double u = epos->GetUParameter();
        if ( u < umin )
          umin = u;
        if ( u > umax )
          umax = u;
      }
    }
    bool isFirstCommon = ( *aUIt == f );
    gp_Pnt2d uv = C2d->Value( isFirstCommon ? umin : umax );
    double dist = thisUV.SquareDistance( uv );
    if ( dist > maxDist ) {
      maxDist = dist;
      nextUV  = uv;
    }
  }
  R_2 uv0, uv1, uv2;
  uv0.x = thisUV.X();   uv0.y = thisUV.Y();
  uv1.x = nextUV.X();   uv1.y = nextUV.Y(); 
  uv2.x = thisUV.X();   uv2.y = thisUV.Y();

  uv1.x *= theScaleX;   uv1.y *= theScaleY; 

  if ( fixOverlappedLinkUV( uv0, uv1, uv2 ))
  {
    double step = thisUV.Distance( gp_Pnt2d( uv0.x, uv0.y ));

    // move theUV along the normal by the step

    N *= step;

    MESSAGE("--fixCommonVertexUV move(" << theUV.x << " " << theUV.x
            << ") by (" << N.X() << " " << N.Y() << ")" 
            << endl << "--- MAX DIST " << maxDist);

    theUV.x += N.X();
    theUV.y += N.Y();

    return true;
  }
  return false;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_MEFISTO_2D::LoadPoints(TWireVector &                 wires,
                                       R_2 *                          uvslf,
                                       vector<const SMDS_MeshNode*>& mefistoToDS,
                                       double                        scalex,
                                       double                        scaley)
{
  // to avoid passing same uv points for a vertex common to 2 wires
  TopoDS_Face F;
  TopTools_IndexedDataMapOfShapeListOfShape VWMap;
  if ( wires.size() > 1 )
  {
    F = TopoDS::Face( _helper->GetSubShape() );
    TopExp::MapShapesAndAncestors( F, TopAbs_VERTEX, TopAbs_WIRE, VWMap );
    int nbVertices = 0;
    for ( int iW = 0; iW < wires.size(); ++iW )
      nbVertices += wires[ iW ]->NbEdges();
    if ( nbVertices == VWMap.Extent() )
      VWMap.Clear(); // wires have no common vertices
  }

  int m = 0;

  for ( int iW = 0; iW < wires.size(); ++iW )
  {
    const vector<UVPtStruct>& uvPtVec = wires[ iW ]->GetUVPtStruct();
    if ( uvPtVec.size() != wires[ iW ]->NbPoints() ) {
      return error(COMPERR_BAD_INPUT_MESH,SMESH_Comment("Unexpected nb of points on wire ")
                   << iW << ": " << uvPtVec.size()<<" != "<<wires[ iW ]->NbPoints()
                   << ", probably because of invalid node parameters on geom edges");
    }
    if ( m + uvPtVec.size()-1 > mefistoToDS.size() ) {
      MESSAGE("Wrong mefistoToDS.size: "<<mefistoToDS.size()<<" < "<<m + uvPtVec.size()-1);
      return error("Internal error");
    }

    list< int > mOnVertex;
    vector<UVPtStruct>::const_iterator uvPt = uvPtVec.begin();
    for ( ++uvPt; uvPt != uvPtVec.end(); ++uvPt )
    {
      // bind mefisto ID to node
      mefistoToDS[m] = uvPt->node;
      // set UV
      uvslf[m].x = uvPt->u * scalex;
      uvslf[m].y = uvPt->v * scaley;
      switch ( uvPt->node->GetPosition()->GetTypeOfPosition())
      {
      case SMDS_TOP_VERTEX:
        mOnVertex.push_back( m );
        break;
      case SMDS_TOP_EDGE:
        // In order to detect degenerated faces easily, we replace
        // nodes on a degenerated edge by node on the vertex of that edge
        if ( _helper->IsDegenShape( uvPt->node->getshapeId() ))
        {
          int edgeID = uvPt->node->getshapeId();
          SMESH_subMesh* edgeSM = _helper->GetMesh()->GetSubMeshContaining( edgeID );
          SMESH_subMeshIteratorPtr smIt = edgeSM->getDependsOnIterator( /*includeSelf=*/0,
                                                                        /*complexShapeFirst=*/0);
          if ( smIt->more() )
          {
            SMESH_subMesh* vertexSM = smIt->next();
            SMDS_NodeIteratorPtr nIt = vertexSM->GetSubMeshDS()->GetNodes();
            if ( nIt->more() )
              mefistoToDS[m] = nIt->next();
          }
        }
        break;
      default:;
      }
      m++;
    }

    int mFirst = mOnVertex.front(), mLast = m - 1;
    list< int >::iterator mIt = mOnVertex.begin();
    for ( ; mIt != mOnVertex.end(); ++mIt)
    {
      int m = *mIt;
      if ( iW && !VWMap.IsEmpty()) { // except outer wire
        // avoid passing same uv point for a vertex common to 2 wires
        int vID = mefistoToDS[m]->getshapeId();
        TopoDS_Vertex V = TopoDS::Vertex( _helper->GetMeshDS()->IndexToShape( vID ));
        if ( fixCommonVertexUV( uvslf[m], V, F, VWMap, *_helper->GetMesh(),
                                scalex, scaley, _quadraticMesh )) {
          myNodesOnCommonV.push_back( mefistoToDS[m] );
          continue;
        }
      }
      // prevent failure on overlapped adjacent links,
      // check only links ending in vertex nodes
      int mB = m - 1, mA = m + 1; // indices Before and After
      if ( mB < mFirst ) mB = mLast;
      if ( mA > mLast )  mA = mFirst;
      fixOverlappedLinkUV (uvslf[ mB ], uvslf[ m ], uvslf[ mA ]);
    }
  }

#ifdef DUMP_POINTS
  cout << "MEFISTO INPUT************" << endl;
  for ( int i =0; i < m; ++i )
    cout << i << ": \t" << uvslf[i].x << ", " << uvslf[i].y
         << " Node " << mefistoToDS[i]->GetID()<< endl;
#endif

  return true;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_MEFISTO_2D::ComputeScaleOnFace(SMESH_Mesh &        aMesh,
                                               const TopoDS_Face & aFace,
                                               double &            scalex,
                                               double &            scaley)
{
  TopoDS_Wire W = BRepTools::OuterWire(aFace);

  double xmin = 1.e300;         // min & max of face 2D parametric coord.
  double xmax = -1.e300;
  double ymin = 1.e300;
  double ymax = -1.e300;
  const int nbp = 23;
  scalex = 1;
  scaley = 1;

  TopExp_Explorer wexp(W, TopAbs_EDGE);
  for ( ; wexp.More(); wexp.Next())
  {
    const TopoDS_Edge & E = TopoDS::Edge( wexp.Current() );
    double f, l;
    Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface(E, aFace, f, l);
    if ( C2d.IsNull() ) continue;
    double du = (l - f) / double (nbp);
    for (int i = 0; i <= nbp; i++)
    {
      double param = f + double (i) * du;
      gp_Pnt2d p = C2d->Value(param);
      if (p.X() < xmin)
        xmin = p.X();
      if (p.X() > xmax)
        xmax = p.X();
      if (p.Y() < ymin)
        ymin = p.Y();
      if (p.Y() > ymax)
        ymax = p.Y();
    }
  }
  double xmoy = (xmax + xmin) / 2.;
  double ymoy = (ymax + ymin) / 2.;
  double xsize = xmax - xmin;
  double ysize = ymax - ymin;

  TopLoc_Location L;
  Handle(Geom_Surface) S = BRep_Tool::Surface(aFace,L);       // 3D surface

  double length_x = 0;
  double length_y = 0;
  gp_Pnt PX0 = S->Value(xmin, ymoy);
  gp_Pnt PY0 = S->Value(xmoy, ymin);
  double dx = xsize / double (nbp);
  double dy = ysize / double (nbp);
  for (int i = 1; i <= nbp; i++)
  {
    double x = xmin + double (i) * dx;
    gp_Pnt PX = S->Value(x, ymoy);
    double y = ymin + double (i) * dy;
    gp_Pnt PY = S->Value(xmoy, y);
    length_x += PX.Distance(PX0);
    length_y += PY.Distance(PY0);
    PX0 = PX;
    PY0 = PY;
  }
  scalex = length_x / xsize;
  scaley = length_y / ysize;
  double xyratio = xsize*scalex/(ysize*scaley);
  const double maxratio = 1.e2;
  if (xyratio > maxratio) {
    scaley *= xyratio / maxratio;
  }
  else if (xyratio < 1./maxratio) {
    scalex *= 1 / xyratio / maxratio;
  }
}

// namespace
// {
//   bool isDegenTria( const SMDS_MeshNode * nn[3] )
//   {
//     SMESH_TNodeXYZ p1( nn[0] );
//     SMESH_TNodeXYZ p2( nn[1] );
//     SMESH_TNodeXYZ p3( nn[2] );
//     gp_XYZ vec1 = p2 - p1;
//     gp_XYZ vec2 = p3 - p1;
//     gp_XYZ cross = vec1 ^ vec2;
//     const double eps = 1e-100;
//     return ( fabs( cross.X() ) < eps &&
//              fabs( cross.Y() ) < eps &&
//              fabs( cross.Z() ) < eps );
//   }
// }

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_MEFISTO_2D::StoreResult(Z nbst, R_2 * uvst, Z nbt, Z * nust,
                                        vector< const SMDS_MeshNode*>&mefistoToDS,
                                        double scalex, double scaley)
{
  _helper->SetElementsOnShape( true );

  TopoDS_Face F = TopoDS::Face( _helper->GetSubShape() );
  Handle(Geom_Surface) S = BRep_Tool::Surface( F );

  //const size_t nbInputNodes = mefistoToDS.size();

  Z n = mefistoToDS.size(); // nb input points
  mefistoToDS.resize( nbst );
  for ( ; n < nbst; n++)
  {
    if (!mefistoToDS[n])
    {
      double u = uvst[n][0] / scalex;
      double v = uvst[n][1] / scaley;
      gp_Pnt P = S->Value(u, v);

      mefistoToDS[n] = _helper->AddNode( P.X(), P.Y(), P.Z(), 0, u, v );
    }
  }

  Z m = 0;

  // triangle points must be in trigonometric order if face is Forward
  // else they must be put clockwise

  int i1 = 1, i2 = 2;
  if ( F.Orientation() != TopAbs_FORWARD )
    std::swap( i1, i2 );

  const SMDS_MeshNode * nn[3];
  for (n = 1; n <= nbt; n++)
  {
    // const bool allNodesAreOld = ( nust[m + 0] <= nbInputNodes &&
    //                               nust[m + 1] <= nbInputNodes &&
    //                               nust[m + 2] <= nbInputNodes );
    nn[ 0 ] = mefistoToDS[ nust[m++] - 1 ];
    nn[ 1 ] = mefistoToDS[ nust[m++] - 1 ];
    nn[ 2 ] = mefistoToDS[ nust[m++] - 1 ];
    m++;

    // avoid creating degenetrated faces
    bool isDegen = ( _helper->HasDegeneratedEdges() &&
                     ( nn[0] == nn[1] || nn[1] == nn[2] || nn[2] == nn[0] ));

    // It was an attemp to fix a problem of a zero area face whose all nodes
    // are on one staight EDGE. But omitting this face makes a hole in the mesh :(
    // if ( !isDegen && allNodesAreOld )
    //   isDegen = isDegenTria( nn );

    if ( !isDegen )
      _helper->AddFace( nn[0], nn[i1], nn[i2] );
  }

  // remove bad elements built on vertices shared by wires

  list<const SMDS_MeshNode*>::iterator itN = myNodesOnCommonV.begin();
  for ( ; itN != myNodesOnCommonV.end(); itN++ )
  {
    const SMDS_MeshNode* node = *itN;
    SMDS_ElemIteratorPtr invElemIt = node->GetInverseElementIterator();
    while ( invElemIt->more() )
    {
      const SMDS_MeshElement* elem = invElemIt->next();
      SMDS_ElemIteratorPtr itN = elem->nodesIterator();
      int nbSame = 0;
      while ( itN->more() )
        if ( itN->next() == node)
          nbSame++;
      if (nbSame > 1) {
        MESSAGE( "RM bad element " << elem->GetID());
        _helper->GetMeshDS()->RemoveElement( elem );
      }
    }
  }
}
