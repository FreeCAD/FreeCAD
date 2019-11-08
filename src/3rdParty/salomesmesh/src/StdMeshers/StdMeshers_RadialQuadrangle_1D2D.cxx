// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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

// File  : StdMeshers_RadialQuadrangle_1D2D.cxx
// Module: SMESH

#include "StdMeshers_RadialQuadrangle_1D2D.hxx"

#include "StdMeshers_NumberOfLayers.hxx"
#include "StdMeshers_LayerDistribution.hxx"
#include "StdMeshers_Regular_1D.hxx"
#include "StdMeshers_NumberOfSegments.hxx"
#include "StdMeshers_FaceSide.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "SMESH_Block.hxx"

#include "utilities.h"

#include <BRepAdaptor_CompCurve.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <ShapeFix_Edge.hxx>
#include <TColgp_SequenceOfPnt.hxx>
#include <TColgp_SequenceOfPnt2d.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>


using namespace std;

#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }
#define gpXYZ(n) gp_XYZ(n->X(),n->Y(),n->Z())


//=======================================================================
//function : StdMeshers_RadialQuadrangle_1D2D
//purpose  : 
//=======================================================================

StdMeshers_RadialQuadrangle_1D2D::StdMeshers_RadialQuadrangle_1D2D(int        hypId,
                                                                   int        studyId,
                                                                   SMESH_Gen* gen)
  :StdMeshers_Quadrangle_2D( hypId, studyId, gen )
{
  _name = "RadialQuadrangle_1D2D";
  _shapeType = (1 << TopAbs_FACE);        // 1 bit per shape type

  _compatibleHypothesis.push_back("LayerDistribution2D");
  _compatibleHypothesis.push_back("NumberOfLayers2D");
  _requireDiscreteBoundary = false;
  _supportSubmeshes = true;
  _neededLowerHyps[ 1 ] = true;  // suppress warning on hiding a global 1D algo

  myNbLayerHypo      = 0;
  myDistributionHypo = 0;
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
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;  // can work with no hypothesis
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
  class TEdgeMarker : public SMESH_subMeshEventListener
  {
    TEdgeMarker(): SMESH_subMeshEventListener(/*isDeletable=*/false,
                                              "StdMeshers_RadialQuadrangle_1D2D::TEdgeMarker") {}
  public:
    //!<  Return static listener
    static SMESH_subMeshEventListener* getListener()
    {
      static TEdgeMarker theEdgeMarker;
      return &theEdgeMarker;
    }
    //! Clear face sumbesh if something happens on edges
    void ProcessEvent(const int          event,
                      const int          eventType,
                      SMESH_subMesh*     edgeSubMesh,
                      EventListenerData* data,
                      const SMESH_Hypothesis*  /*hyp*/)
    {
      if ( data && !data->mySubMeshes.empty() && eventType == SMESH_subMesh::ALGO_EVENT)
      {
        ASSERT( data->mySubMeshes.front() != edgeSubMesh );
        SMESH_subMesh* faceSubMesh = data->mySubMeshes.front();
        faceSubMesh->ComputeStateEngine( SMESH_subMesh::CLEAN );
      }
    }
  };

  //================================================================================
  /*!
   * \brief Mark an edge as computed by StdMeshers_RadialQuadrangle_1D2D
   */
  //================================================================================

  void markEdgeAsComputedByMe(const TopoDS_Edge& edge, SMESH_subMesh* faceSubMesh)
  {
    if ( SMESH_subMesh* edgeSM = faceSubMesh->GetFather()->GetSubMeshContaining( edge ))
    {
      if ( !edgeSM->GetEventListenerData( TEdgeMarker::getListener() ))
        faceSubMesh->SetEventListener( TEdgeMarker::getListener(),
                                       SMESH_subMeshEventListenerData::MakeData(faceSubMesh),
                                       edgeSM);
    }
  }

  //================================================================================
  /*!
   * \brief Return sides of the face connected in the order: aCircEdge, aLinEdge1, aLinEdge2
   *  \retval int - nb of sides
   */
  //================================================================================

  int analyseFace(const TopoDS_Shape&     aShape,
                  SMESH_Mesh*             aMesh,
                  StdMeshers_FaceSidePtr& aCircSide,
                  StdMeshers_FaceSidePtr& aLinSide1,
                  StdMeshers_FaceSidePtr& aLinSide2,
                  SMESH_MesherHelper*     helper)
  {
    const TopoDS_Face& face = TopoDS::Face( aShape );
    aCircSide.reset(); aLinSide1.reset(); aLinSide2.reset();

    list< TopoDS_Edge > edges;
    list< int > nbEdgesInWire;
    int nbWire = SMESH_Block::GetOrderedEdges ( face, edges, nbEdgesInWire );
    if ( nbWire > 2 || nbEdgesInWire.front() < 1 ) return 0;

    // remove degenerated EDGEs
    list<TopoDS_Edge>::iterator edge = edges.begin();
    while ( edge != edges.end() )
      if ( SMESH_Algo::isDegenerated( *edge ))
        edge = edges.erase( edge );
      else
        ++edge;
    int nbEdges = edges.size();

    // find VERTEXes between continues EDGEs
    TopTools_MapOfShape contVV;
    if ( nbEdges > 1 )
    {
      TopoDS_Edge ePrev = edges.back();
      for ( edge = edges.begin(); edge != edges.end(); ++edge )
      {
        if ( SMESH_Algo::IsContinuous( ePrev, *edge ))
          contVV.Add( SMESH_MesherHelper::IthVertex( 0, *edge ));
        ePrev = *edge;
      }
    }
    // make edges start from a non-continues VERTEX
    if ( 1 < contVV.Extent() && contVV.Extent() < nbEdges )
    {
      while ( contVV.Contains( SMESH_MesherHelper::IthVertex( 0, edges.front() )))
        edges.splice( edges.end(), edges, edges.begin() );
    }

    // make face sides
    TSideVector sides;
    while ( !edges.empty() )
    {
      list< TopoDS_Edge > sideEdges;
      sideEdges.splice( sideEdges.end(), edges, edges.begin() );
      while ( !edges.empty() &&
              contVV.Contains( SMESH_MesherHelper::IthVertex( 0, edges.front() )))
        sideEdges.splice( sideEdges.end(), edges, edges.begin() );

      StdMeshers_FaceSidePtr side;
      if ( aMesh ) 
        side = StdMeshers_FaceSide::New( face, sideEdges, aMesh,
                                         /*isFwd=*/true, /*skipMedium=*/ true, helper );
      sides.push_back( side );
    }

    if ( !aMesh ) // call from IsApplicable()
      return sides.size();

    if ( sides.size() > 3 )
      return sides.size();

    if ( nbWire == 2 && (( sides.size() != 2 ) ||
                         ( sides[0]->IsClosed() && sides[1]->IsClosed() ) ||
                         ( !sides[0]->IsClosed() && !sides[1]->IsClosed() )))
      return -1;

    // detect an elliptic side

    if ( sides.size() == 1 )
    {
      aCircSide = sides[0];
      return sides.size();
    }

    // sort sides by deviation from a straight line
    multimap< double, int > deviation2sideInd;
    const double nbSamples = 7;
    for ( size_t iS = 0; iS < sides.size(); ++iS )
    {
      gp_Pnt pf = BRep_Tool::Pnt( sides[iS]->FirstVertex() );
      gp_Pnt pl = BRep_Tool::Pnt( sides[iS]->LastVertex() );
      gp_Vec v1( pf, pl );
      double v1Len = v1.Magnitude();
      if ( v1Len < std::numeric_limits< double >::min() )
      {
        deviation2sideInd.insert( make_pair( sides[iS]->Length(), iS )); // the side seems closed
        continue;
      }
      double devia = 0;
      for ( int i = 0; i < nbSamples; ++i )
      {
        gp_Pnt pi( sides[iS]->Value3d(( i + 1 ) / nbSamples ));
        gp_Vec vi( pf, pi );
        double h = 0.5 * v1.Crossed( vi ).Magnitude() / v1Len;
        devia = Max( devia, h );
      }
      deviation2sideInd.insert( make_pair( devia, iS ));
    }
    double maxDevi = deviation2sideInd.rbegin()->first;
    if ( maxDevi < 1e-7 && sides.size() == 3 )
    {
      // a triangle FACE; use a side with the most outstanding length as an elliptic one
      deviation2sideInd.clear();
      multimap< double, int > len2sideInd;
      for ( size_t iS = 0; iS < sides.size(); ++iS )
        len2sideInd.insert( make_pair( sides[iS]->Length(), iS ));

      multimap< double, int >::iterator l2i = len2sideInd.begin();
      double len0 = l2i->first;
      double len1 = (++l2i)->first;
      double len2 = (++l2i)->first;
      if ( len1 - len0 > len2 - len1 )
        deviation2sideInd.insert( make_pair( 0., len2sideInd.begin()->second ));
      else
        deviation2sideInd.insert( make_pair( 0., len2sideInd.rbegin()->second ));
    }

    int iCirc = deviation2sideInd.rbegin()->second; 
    aCircSide = sides[ iCirc ];
    aLinSide1 = sides[( iCirc + 1 ) % sides.size() ];
    if ( sides.size() > 2 )
    {
      aLinSide2 = sides[( iCirc + 2 ) % sides.size() ];
      aLinSide2->Reverse(); // to be "parallel" to aLinSide1
    }

    if (( nbWire == 2 && aLinSide1 ) &&
        ( aLinSide1->Edge(0).Orientation() == TopAbs_INTERNAL ) &&
        ( aCircSide->IsClosed() ))
    {
      // assure that aCircSide starts at aLinSide1
      TopoDS_Vertex v0 = aLinSide1->FirstVertex();
      TopoDS_Vertex v1 = aLinSide1->LastVertex();
      if ( ! aCircSide->FirstVertex().IsSame( v0 ) &&
           ! aCircSide->FirstVertex().IsSame( v1 ))
      {
        int iE = 0;
        for ( ; iE < aCircSide->NbEdges(); ++iE )
          if ( aCircSide->FirstVertex(iE).IsSame( v0 ) ||
               aCircSide->FirstVertex(iE).IsSame( v1 ))
            break;
        if ( iE == aCircSide->NbEdges() )
          return -2;

        edges.clear();
        for ( int i = 0; i < aCircSide->NbEdges(); ++i, ++iE )
          edges.push_back( aCircSide->Edge( iE % aCircSide->NbEdges() ));

        aCircSide = StdMeshers_FaceSide::New( face, edges, aMesh,
                                              /*isFwd=*/true, /*skipMedium=*/ true, helper );
      }
    }

    return sides.size();
  }

  //================================================================================
  /*!
   * \brief Checks if the common vertex between LinSide's lies inside the circle
   *  and not outside
   *  \return bool - false if there are 3 EDGEs and the corner is outside
   */
  //================================================================================

  bool isCornerInsideCircle(const StdMeshers_FaceSidePtr& CircSide,
                            const StdMeshers_FaceSidePtr& LinSide1,
                            const StdMeshers_FaceSidePtr& LinSide2)
  {
    // if ( CircSide && LinSide1 && LinSide2 )
    // {
    //   Handle(Geom_Circle) aCirc = Handle(Geom_Circle)::DownCast( getCurve( CircSide ));
    //   TopoDS_Vertex aCommonV;
    //   if ( !aCirc.IsNull() &&
    //        TopExp::CommonVertex( LinSide1, LinSide2, aCommonV ))
    //   {
    //     gp_Pnt aCommonP = BRep_Tool::Pnt( aCommonV );
    //     gp_Pnt  aCenter = aCirc->Location();
    //     double     dist = aCenter.Distance( aCommonP );
    //     return dist < 0.1 * aCirc->Radius();
    //   }
    // }
    return true;
  }

  //================================================================================
  /*!
   * \brief Create an EDGE connecting the ellipse center with the most distant point
   *        of the ellipse.
   *  \param [in] circSide - the elliptic side
  *  \param [in] face - the FACE
  *  \param [out] circNode - a node on circSide most distant from the center
  *  \return TopoDS_Edge - the create EDGE
  */
  //================================================================================

  TopoDS_Edge makeEdgeToCenter( StdMeshers_FaceSidePtr& circSide,
                                const TopoDS_Face&      face,
                                const SMDS_MeshNode*&   circNode)
  {
    // find the center and a point most distant from it

    double maxDist = 0, normPar;
    gp_XY uv1, uv2;
    for ( int i = 0; i < 32; ++i )
    {
      double    u = 0.5 * i / 32.;
      gp_Pnt2d p1 = circSide->Value2d( u );
      gp_Pnt2d p2 = circSide->Value2d( u + 0.5 );
      double dist = p1.SquareDistance( p2 );
      if ( dist > maxDist )
      {
        maxDist = dist;
        uv1 = p1.XY();
        uv2 = p2.XY();
        normPar = u;
      }
    }
    gp_XY center = 0.5 * ( uv1 + uv2 );
    double   len = 0.5 * Sqrt( maxDist );
    bool  isCirc = ( Abs( len - circSide->Value2d( 0 ).Distance( center )) < 1e-3 * len );

    // find a node closest to the most distant point

    size_t iDist = 0;
    const UVPtStructVec& circNodes = circSide->GetUVPtStruct();
    if ( !isCirc )
    {
      double minDist = 1e100;
      for ( size_t i = 0; i <= circNodes.size(); ++i )
      {
        double dist = Abs( circNodes[i].normParam - normPar );
        if ( dist < minDist )
        {
          iDist = i;
          minDist = dist;
        }
      }
    }
    circNode = circNodes[iDist].node;
    uv1      = circNodes[iDist].UV();
    len      = ( uv1 - center ).Modulus();

    // make the EDGE between the most distant point and the center

    Handle(Geom2d_Line) line = new Geom2d_Line( uv1, gp_Dir2d( center - uv1 ) );
    Handle(Geom2d_Curve) pcu = new Geom2d_TrimmedCurve( line, 0, len );

    Handle(Geom_Surface) surface = BRep_Tool::Surface( face );
    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge( pcu, surface, 0, len );

    BRep_Builder().UpdateEdge( edge, pcu, face, 1e-7 );
    ShapeFix_Edge().FixAddCurve3d( edge );


    // assure that circSide starts at circNode
    if ( iDist != 0  &&  iDist != circNodes.size()-1 )
    {
      // create a new circSide
      UVPtStructVec nodesNew;
      nodesNew.reserve( circNodes.size() );
      nodesNew.insert( nodesNew.end(), circNodes.begin() + iDist, circNodes.end() );
      nodesNew.insert( nodesNew.end(), circNodes.begin() + 1, circNodes.begin() + iDist + 1 );
      circSide = StdMeshers_FaceSide::New( nodesNew );
    }

    return edge;
  }

  //================================================================================
  /*!
   * \brief Set nodes existing on a linSide to UVPtStructVec and create missing nodes
   *        corresponding to layerPositions
   */
  //================================================================================

  void makeMissingMesh( StdMeshers_FaceSidePtr& linSide,
                        UVPtStructVec&          nodes,
                        const vector< double >& layerPositions,
                        SMESH_MesherHelper*     helper )
  {
    // tolerance to compare normParam
    double tol = 1e100;
    for ( size_t i = 1; i < layerPositions.size(); ++i )
      tol = Min( tol, layerPositions[i] - layerPositions[i-1] );
    tol *= 0.05;

    // merge existing nodes with layerPositions into UVPtStructVec
    // ------------------------------------------------------------

    const UVPtStructVec& exiNodes = linSide->GetUVPtStruct();
    nodes.clear();
    nodes.reserve( layerPositions.size() + exiNodes.size() );
    vector< double >::const_iterator pos = layerPositions.begin(), posEnd = layerPositions.end();
    for ( size_t i = 0; i < exiNodes.size(); ++i )
    {
      switch ( exiNodes[i].node->GetPosition()->GetTypeOfPosition() )
      {
      case SMDS_TOP_VERTEX:
      {
        // allocate UVPtStruct's for non-existing nodes
        while ( pos != posEnd && *pos < exiNodes[i].normParam - tol )
        {
          UVPtStruct uvPS;
          uvPS.normParam = *pos++;
          nodes.push_back( uvPS );
        }
        // save existing node on a VERTEX
        nodes.push_back( exiNodes[i] );
        break;
      }
      case SMDS_TOP_EDGE:
      {
        // save existing nodes on an EDGE
        while ( i < exiNodes.size() && exiNodes[i].node->GetPosition()->GetDim() == 1 )
        {
          nodes.push_back( exiNodes[i++] );
        }
        // save existing node on a VERTEX
        if ( i < exiNodes.size() && exiNodes[i].node->GetPosition()->GetDim() == 0 )
        {
          nodes.push_back( exiNodes[i] );
        }
        break;
      }
      default:;
      }

      // skip layer positions covered by saved nodes
      while ( pos != posEnd && *pos < nodes.back().normParam + tol )
      {
        ++pos;
      }
    }
    // allocate UVPtStruct's for the rest non-existing nodes
    while ( pos != posEnd )
    {
      UVPtStruct uvPS;
      uvPS.normParam = *pos++;
      nodes.push_back( uvPS );
    }

    // create missing nodes
    // ---------------------

    SMESHDS_Mesh * meshDS = helper->GetMeshDS();
    const TopoDS_Face& F  = TopoDS::Face( helper->GetSubShape() );
    Handle(ShapeAnalysis_Surface) surface = helper->GetSurface( F );

    helper->SetElementsOnShape( false ); // we create nodes on EDGEs, not on the FACE

    for ( size_t i = 0; i < nodes.size(); ++i )
    {
      if ( nodes[ i ].node ) continue;

      gp_Pnt2d uv = linSide->Value2d( nodes[i].normParam );
      gp_Pnt  xyz = surface->Value( uv.X(), uv.Y() );

      nodes[ i ].SetUV( uv.XY() );
      nodes[ i ].node = helper->AddNode( xyz.X(), xyz.Y(), xyz.Z() );
    }

    // set nodes on VERTEXes
    for ( int iE = 0; iE < linSide->NbEdges(); ++iE )
    {
      TopoDS_Vertex v = linSide->LastVertex( iE );
      if ( SMESH_Algo::VertexNode( v, meshDS ))
        continue;

      double normPar = linSide->LastParameter( iE );
      size_t i = 0;
      while ( nodes[ i ].normParam < normPar )
        ++i;
      if (( nodes[ i ].normParam - normPar ) > ( normPar - nodes[ i-1 ].normParam ))
        --i;
      meshDS->SetNodeOnVertex( nodes[ i ].node, v );
    }

    // set nodes on EDGEs
    int edgeID;
    for ( size_t i = 0; i < nodes.size(); ++i )
    {
      if ( nodes[ i ].node->getshapeId() > 0 ) continue;

      double u = linSide->Parameter( nodes[i].normParam, edgeID );
      meshDS->SetNodeOnEdge( nodes[ i ].node, edgeID, u );
    }

    // create segments
    for ( size_t i = 1; i < nodes.size(); ++i )
    {
      if ( meshDS->FindEdge( nodes[i].node, nodes[i-1].node )) continue;

      const SMDS_MeshElement* seg = helper->AddEdge( nodes[i].node, nodes[i-1].node );

      double normParam = 0.5 * ( nodes[i].normParam + nodes[i-1].normParam );
      edgeID = linSide->EdgeID( linSide->EdgeIndex( normParam ));
      meshDS->SetMeshElementOnShape( seg, edgeID );
    }

    helper->SetElementsOnShape( true );

  } // end makeMissingMesh()

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
    const int myID = -1001;
    TNodeDistributor* myHyp = dynamic_cast<TNodeDistributor*>( aMesh.GetHypothesis( myID ));
    if ( !myHyp )
      myHyp = new TNodeDistributor( myID, 0, aMesh.GetGen() );
    return myHyp;
  }
  // -----------------------------------------------------------------------------
  //! Computes distribution of nodes on a straight line ending at pIn and pOut
  bool Compute( vector< double > &      positions,
                const TopoDS_Edge&      edge,
                Adaptor3d_Curve&        curve,
                double                  f,
                double                  l,
                SMESH_Mesh&             mesh,
                const SMESH_Hypothesis* hyp1d)
  {
    if ( !hyp1d ) return error( "Invalid LayerDistribution hypothesis");

    myUsedHyps.clear();
    myUsedHyps.push_back( hyp1d );

    SMESH_Hypothesis::Hypothesis_Status aStatus;
    if ( !StdMeshers_Regular_1D::CheckHypothesis( mesh, edge, aStatus ))
      return error( "StdMeshers_Regular_1D::CheckHypothesis() failed "
                    "with LayerDistribution hypothesis");

    double len = GCPnts_AbscissaPoint::Length( curve, f, l );

    list< double > params;
    if ( !StdMeshers_Regular_1D::computeInternalParameters( mesh, curve, len, f, l, params, false ))
      return error("StdMeshers_Regular_1D failed to compute layers distribution");

    params.push_front( f );
    params.push_back ( l );
    positions.clear();
    positions.reserve( params.size() );
    for (list<double>::iterator itU = params.begin(); itU != params.end(); itU++)
      positions.push_back(( *itU - f ) / ( l - f ));
    return true;
  }
  // -----------------------------------------------------------------------------
  //! Make mesh on an adge using assigned 1d hyp or defaut nb of segments
  bool ComputeCircularEdge( SMESH_Mesh&                   aMesh,
                            const StdMeshers_FaceSidePtr& aSide )
  {
    bool ok = true;
    for ( int i = 0; i < aSide->NbEdges(); ++i )
    {
      const TopoDS_Edge& edge = aSide->Edge( i );
      _gen->Compute( aMesh, edge );
      SMESH_subMesh *sm = aMesh.GetSubMesh( edge );
      if ( sm->GetComputeState() != SMESH_subMesh::COMPUTE_OK)
      {
        // find any 1d hyp assigned (there can be a hyp w/o algo)
        myUsedHyps = SMESH_Algo::GetUsedHypothesis( aMesh, edge, /*ignoreAux=*/true );
        Hypothesis_Status aStatus;
        if ( !StdMeshers_Regular_1D::CheckHypothesis( aMesh, edge, aStatus ))
        {
          // no valid 1d hyp assigned, use default nb of segments
          _hypType                    = NB_SEGMENTS;
          _ivalue[ DISTR_TYPE_IND ]   = StdMeshers_NumberOfSegments::DT_Regular;
          _ivalue[ NB_SEGMENTS_IND  ] = _gen->GetDefaultNbSegments();
        }
        ok &= StdMeshers_Regular_1D::Compute( aMesh, edge );
      }
    }
    return ok;
  }
  // -----------------------------------------------------------------------------
  //! Make mesh on an adge using assigned 1d hyp or defaut nb of segments
  bool EvaluateCircularEdge(SMESH_Mesh&                  aMesh,
                            const StdMeshers_FaceSidePtr aSide,
                            MapShapeNbElems&             aResMap)
  {
    bool ok = true;
    for ( int i = 0; i < aSide->NbEdges(); ++i )
    {
      const TopoDS_Edge& anEdge = aSide->Edge( i );
      _gen->Evaluate( aMesh, anEdge, aResMap );
      if ( aResMap.count( aMesh.GetSubMesh( anEdge )))
        continue;

      // find any 1d hyp assigned
      myUsedHyps = SMESH_Algo::GetUsedHypothesis(aMesh, anEdge, /*ignoreAux=*/true);
      Hypothesis_Status aStatus;
      if ( !StdMeshers_Regular_1D::CheckHypothesis( aMesh, anEdge, aStatus ))
      {
        // no valid 1d hyp assigned, use default nb of segments
        _hypType                    = NB_SEGMENTS;
        _ivalue[ DISTR_TYPE_IND ]   = StdMeshers_NumberOfSegments::DT_Regular;
        _ivalue[ NB_SEGMENTS_IND  ] = _gen->GetDefaultNbSegments();
      }
      ok &= StdMeshers_Regular_1D::Evaluate( aMesh, anEdge, aResMap );
    }
    return ok;
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
}

//=======================================================================
/*!
 * \brief Allow algo to do something after persistent restoration
 * \param subMesh - restored submesh
 *
 * call markEdgeAsComputedByMe()
 */
//=======================================================================

void StdMeshers_RadialQuadrangle_1D2D::SubmeshRestored(SMESH_subMesh* faceSubMesh)
{
  if ( !faceSubMesh->IsEmpty() )
  {
    for ( TopExp_Explorer e( faceSubMesh->GetSubShape(), TopAbs_EDGE ); e.More(); e.Next() )
    {
      markEdgeAsComputedByMe( TopoDS::Edge( e.Current() ), faceSubMesh );
    }
  }
}

//=======================================================================
//function : Compute
//purpose  :
//=======================================================================

bool StdMeshers_RadialQuadrangle_1D2D::Compute(SMESH_Mesh&         aMesh,
                                               const TopoDS_Shape& aShape)
{
  SMESH_MesherHelper helper( aMesh );
  StdMeshers_Quadrangle_2D::myHelper     = & helper;
  StdMeshers_Quadrangle_2D::myNeedSmooth = false;
  StdMeshers_Quadrangle_2D::myCheckOri   = false;
  StdMeshers_Quadrangle_2D::myQuadList.clear();

  myHelper->SetSubShape( aShape );
  myHelper->SetElementsOnShape( true );

  StdMeshers_FaceSidePtr circSide, linSide1, linSide2;
  int nbSides = analyseFace( aShape, &aMesh, circSide, linSide1, linSide2, myHelper );
  if( nbSides > 3 || nbSides < 1 )
    return error("The face must be a full ellipse or a part of ellipse (i.e. the number "
                 "of edges is less or equal to 3 and one of them is an ellipse curve)");

  // get not yet computed EDGEs
  list< TopoDS_Edge > emptyEdges;
  for ( TopExp_Explorer e( aShape, TopAbs_EDGE ); e.More(); e.Next() )
  {
    if ( aMesh.GetSubMesh( e.Current() )->IsEmpty() )
      emptyEdges.push_back( TopoDS::Edge( e.Current() ));
  }

  TNodeDistributor* algo1d = TNodeDistributor::GetDistributor(aMesh);

  if ( !algo1d->ComputeCircularEdge( aMesh, circSide ))
    return error( algo1d->GetComputeError() );


  TopoDS_Face F = TopoDS::Face(aShape);
  Handle(Geom_Surface) S = BRep_Tool::Surface(F);

  myHelper->IsQuadraticSubMesh( aShape );

  vector< double > layerPositions; // [0,1]

  const SMDS_MeshNode* centerNode = 0;
  gp_Pnt2d             centerUV(0,0);

  // ------------------------------------------------------------------------------------------
  if ( nbSides == 1 ) // full ellipse
  {
    const SMDS_MeshNode* circNode;
    TopoDS_Edge linEdge = makeEdgeToCenter( circSide, F, circNode );

    StdMeshers_FaceSidePtr tmpSide =
      StdMeshers_FaceSide::New( F, linEdge, &aMesh, /*isFrw=*/true, /*skipMedium=*/true, myHelper );

    if ( !computeLayerPositions( tmpSide, layerPositions ))
      return false;

    UVPtStructVec nodes( layerPositions.size() );
    nodes[0].node = circNode;
    for ( size_t i = 0; i < layerPositions.size(); ++i )
    {
      gp_Pnt2d uv = tmpSide->Value2d( layerPositions[i] );
      gp_Pnt  xyz = S->Value( uv.X(), uv.Y() );

      nodes[ i ].SetUV( uv.XY() );
      nodes[ i ].normParam = layerPositions[i];
      if ( i )
        nodes[ i ].node = myHelper->AddNode( xyz.X(), xyz.Y(), xyz.Z(), 0, uv.X(), uv.Y() );
    }

    linSide1 = StdMeshers_FaceSide::New( nodes );
    linSide2 = StdMeshers_FaceSide::New( nodes );

    centerNode = nodes.back().node;
    centerUV   = nodes.back().UV();
  }
  // ------------------------------------------------------------------------------------------
  else if ( nbSides == 2 && linSide1->Edge(0).Orientation() == TopAbs_INTERNAL )
  {
    // full ellipse with an internal radial side

    // eliminate INTERNAL orientation
    list< TopoDS_Edge > edges;
    for ( int iE = 0; iE < linSide1->NbEdges(); ++iE )
    {
      edges.push_back( linSide1->Edge( iE ));
      edges.back().Orientation( TopAbs_FORWARD );
    }

    // orient the internal side
    bool isVIn0Shared = false;
    TopoDS_Vertex vIn0 = myHelper->IthVertex( 0, edges.front() );
    for ( int iE = 0; iE < circSide->NbEdges() && !isVIn0Shared; ++iE )
      isVIn0Shared = vIn0.IsSame( circSide->FirstVertex( iE ));

    linSide1 = StdMeshers_FaceSide::New( F, edges, &aMesh,
                                         /*isFrw=*/isVIn0Shared, /*skipMedium=*/true, myHelper );

    int nbMeshedEdges;
    if ( !computeLayerPositions( linSide1, layerPositions, &nbMeshedEdges ))
      return false;

    // merge existing nodes with new nodes at layerPositions into a UVPtStructVec
    UVPtStructVec nodes;
    if ( nbMeshedEdges != linSide1->NbEdges() )
      makeMissingMesh( linSide1, nodes, layerPositions, myHelper );
    else
      nodes = linSide1->GetUVPtStruct();

    linSide1 = StdMeshers_FaceSide::New( nodes );
    linSide2 = StdMeshers_FaceSide::New( nodes );

    centerNode = nodes.back().node;
    centerUV   = nodes.back().UV();
  }
  // ------------------------------------------------------------------------------------------
  else if ( nbSides == 2 )
  {
    // find positions of layers for the first half of linSide1
    int nbMeshedEdges;
    if ( !computeLayerPositions( linSide1, layerPositions, &nbMeshedEdges, /*useHalf=*/true ))
      return false;

    // make positions for the whole linSide1
    for ( size_t i = 0; i < layerPositions.size(); ++i )
    {
      layerPositions[i] *= 0.5;
    }
    layerPositions.reserve( layerPositions.size() * 2 );
    for ( int nb = layerPositions.size()-1; nb > 0; --nb )
      layerPositions.push_back( layerPositions.back() + layerPositions[nb] - layerPositions[nb-1] );

    // merge existing nodes with new nodes at layerPositions into a UVPtStructVec
    UVPtStructVec nodes;
    if ( nbMeshedEdges != linSide1->NbEdges() )
      makeMissingMesh( linSide1, nodes, layerPositions, myHelper );
    else
      nodes = linSide1->GetUVPtStruct();

    // find a central node
    size_t i = 0;
    while ( nodes[ i ].normParam < 0.5 ) ++i;
    if (( nodes[ i ].normParam - 0.5 ) > ( 0.5 - nodes[ i-1 ].normParam )) --i;

    // distribute nodes between two linear sides
    UVPtStructVec nodes2( nodes.rbegin(), nodes.rbegin() + nodes.size() - i );
    nodes.resize( i + 1 );

    linSide1 = StdMeshers_FaceSide::New( nodes );
    linSide2 = StdMeshers_FaceSide::New( nodes2 );

    centerNode = nodes.back().node;
    centerUV   = nodes.back().UV();
  }
  // ------------------------------------------------------------------------------------------
  else // nbSides == 3 
  {
    // one curve must be a part of ellipse and 2 other curves must be segments of line

    int nbMeshedEdges1, nbMeshedEdges2;
    vector< double > layerPositions2;
    bool ok1 = computeLayerPositions( linSide1, layerPositions,  &nbMeshedEdges1 );
    bool ok2 = computeLayerPositions( linSide2, layerPositions2, &nbMeshedEdges2 );
    if ( !ok1 && !ok2 )
      return false;

    bool linSide1Computed = ( nbMeshedEdges1 == linSide1->NbEdges() );
    bool linSide2Computed = ( nbMeshedEdges2 == linSide2->NbEdges() );

    UVPtStructVec nodes;

    if ( linSide1Computed && !linSide2Computed )
    {
      // use layer positions of linSide1 to mesh linSide2
      makeMissingMesh( linSide2, nodes, layerPositions, myHelper );
      linSide2 = StdMeshers_FaceSide::New( nodes );
    }
    else if ( linSide2Computed && !linSide1Computed )
    {
      // use layer positions of linSide2 to mesh linSide1
      makeMissingMesh( linSide1, nodes, layerPositions2, myHelper );
      linSide1 = StdMeshers_FaceSide::New( nodes );
    }
    else if ( !linSide2Computed && !linSide1Computed )
    {
      // use layer positions of a longer side to mesh the shorter side
      vector< double >& lp =
        ( linSide1->Length() > linSide2->Length() ) ? layerPositions : layerPositions2;

      makeMissingMesh( linSide1, nodes, lp, myHelper );
      linSide1 = StdMeshers_FaceSide::New( nodes );
      makeMissingMesh( linSide2, nodes, lp, myHelper );
      linSide2 = StdMeshers_FaceSide::New( nodes );
    }

    const UVPtStructVec& nodes2 = linSide2->GetUVPtStruct();
    centerNode = nodes2.back().node;
    centerUV   = nodes2.back().UV();
  }

  list< TopoDS_Edge >::iterator ee = emptyEdges.begin();
  for ( ; ee != emptyEdges.end(); ++ee )
    markEdgeAsComputedByMe( *ee, aMesh.GetSubMesh( F ));

  circSide->GetUVPtStruct(); // let sides take into account just computed nodes
  linSide1->GetUVPtStruct();
  linSide2->GetUVPtStruct();

  FaceQuadStruct::Ptr quad( new FaceQuadStruct );
  quad->side.resize( 4 );
  quad->side[0] = circSide;
  quad->side[1] = linSide1;
  quad->side[2] = StdMeshers_FaceSide::New( circSide.get(), centerNode, &centerUV );
  quad->side[3] = linSide2;

  myQuadList.push_back( quad );

  // create quadrangles
  bool ok;
  if ( linSide1->NbPoints() == linSide2->NbPoints() )
    ok = StdMeshers_Quadrangle_2D::computeQuadDominant( aMesh, F, quad );
  else
    ok = StdMeshers_Quadrangle_2D::computeTriangles( aMesh, F, quad );

  StdMeshers_Quadrangle_2D::myHelper = 0;

  return ok;
}

//================================================================================
/*!
 * \brief Compute nodes on the radial edge
 * \retval int - nb of segments on the linSide
 */
//================================================================================

int StdMeshers_RadialQuadrangle_1D2D::computeLayerPositions(StdMeshers_FaceSidePtr linSide,
                                                            vector< double > &     positions,
                                                            int*                   nbMeshedEdges,
                                                            bool                   useHalf)
{
  // First, try to compute positions of layers

  positions.clear();

  SMESH_Mesh * mesh = myHelper->GetMesh();

  const SMESH_Hypothesis* hyp1D = myDistributionHypo ? myDistributionHypo->GetLayerDistribution() : 0;
  int                  nbLayers = myNbLayerHypo ? myNbLayerHypo->GetNumberOfLayers() : 0;

  if ( !hyp1D && !nbLayers )
  {
    // No own algo hypotheses assigned, so first try to find any 1D hypothesis.
    // find a hyp usable by TNodeDistributor
    TopoDS_Shape edge = linSide->Edge(0);
    const SMESH_HypoFilter* hypKind =
      TNodeDistributor::GetDistributor(*mesh)->GetCompatibleHypoFilter(/*ignoreAux=*/true);
    hyp1D = mesh->GetHypothesis( edge, *hypKind, /*fromAncestors=*/true);
  }
  if ( hyp1D ) // try to compute with hyp1D
  {
    BRepAdaptor_CompCurve* curve = linSide->GetCurve3d();
    SMESHUtils::Deleter< BRepAdaptor_CompCurve > delCurve( curve );
    double f = curve->FirstParameter();
    double l = curve->LastParameter();

    if ( useHalf )
      l = 0.5 * ( f + l ); // first part of linSide is used

    if ( !TNodeDistributor::GetDistributor(*mesh)->Compute( positions, linSide->Edge(0),
                                                            *curve, f, l, *mesh, hyp1D ))
    {
      if ( myDistributionHypo ) { // bad hyp assigned 
        return error( TNodeDistributor::GetDistributor(*mesh)->GetComputeError() );
      }
      else {
        // bad hyp found, its Ok, lets try with default nb of segments
      }
    }
  }
  
  if ( positions.empty() ) // try to use nb of layers
  {
    if ( !nbLayers )
      nbLayers = _gen->GetDefaultNbSegments();

    if ( nbLayers )
    {
      positions.resize( nbLayers + 1 );
      for ( int z = 0; z < nbLayers; ++z )
        positions[ z ] = double( z )/ double( nbLayers );
      positions.back() = 1;
    }
  }

  // Second, check presence of a mesh built by other algo on linSide

  int nbEdgesComputed = 0;
  for ( int i = 0; i < linSide->NbEdges(); ++i )
  {
    nbEdgesComputed += ( !mesh->GetSubMesh( linSide->Edge(i))->IsEmpty() );
  }

  if ( nbEdgesComputed == linSide->NbEdges() )
  {
    const UVPtStructVec& points = linSide->GetUVPtStruct();
    if ( points.size() >= 2 )
    {
      positions.resize( points.size() );
      for ( size_t i = 0; i < points.size(); ++i )
        positions[ i ] = points[i].normParam;
    }
  }

  if ( nbMeshedEdges ) *nbMeshedEdges = nbEdgesComputed;

  return positions.size();
}


//=======================================================================
//function : Evaluate
//purpose  : 
//=======================================================================

bool StdMeshers_RadialQuadrangle_1D2D::Evaluate(SMESH_Mesh&         aMesh,
                                                const TopoDS_Shape& aShape,
                                                MapShapeNbElems&    aResMap)
{
  if( aShape.ShapeType() != TopAbs_FACE ) {
    return false;
  }
  SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
  if( aResMap.count(sm) )
    return false;

  vector<int>& aResVec =
    aResMap.insert( make_pair(sm, vector<int>(SMDSEntity_Last,0))).first->second;

  myHelper = new SMESH_MesherHelper( aMesh );
  myHelper->SetSubShape( aShape );
  SMESHUtils::Deleter<SMESH_MesherHelper> helperDeleter( myHelper );

  TNodeDistributor* algo1d = TNodeDistributor::GetDistributor(aMesh);

  StdMeshers_FaceSidePtr circSide, linSide1, linSide2;
  int nbSides = analyseFace( aShape, &aMesh, circSide, linSide1, linSide2, myHelper );
  if( nbSides > 3 || nbSides < 1 )
    return false;

  if ( algo1d->EvaluateCircularEdge( aMesh, circSide, aResMap ))
    return false;

  vector< double > layerPositions; // [0,1]

  // ------------------------------------------------------------------------------------------
  if ( nbSides == 1 )
  {
    const TopoDS_Face& F = TopoDS::Face( aShape );

    const SMDS_MeshNode* circNode;
    TopoDS_Edge linEdge = makeEdgeToCenter( circSide, F, circNode );

    StdMeshers_FaceSidePtr tmpSide =
      StdMeshers_FaceSide::New( F, linEdge, &aMesh, /*isFrw=*/true, /*skipMedium=*/true, myHelper );

    if ( !computeLayerPositions( tmpSide, layerPositions ))
      return false;
  }
  // ------------------------------------------------------------------------------------------
  else if ( nbSides == 2 && linSide1->Edge(0).Orientation() == TopAbs_INTERNAL )
  {
    if ( !computeLayerPositions( linSide1, layerPositions ))
      return false;
  }
  // ------------------------------------------------------------------------------------------
  else if ( nbSides == 2 )
  {
    // find positions of layers for the first half of linSide1
    if ( !computeLayerPositions( linSide1, layerPositions, 0, /*useHalf=*/true ))
      return false;
  }
  // ------------------------------------------------------------------------------------------
  else // nbSides == 3 
  {
    if ( !computeLayerPositions(( linSide1->Length() > linSide2->Length() ) ? linSide1 : linSide2,
                                 layerPositions ))
      return false;
  }

  bool isQuadratic = false;
  for ( TopExp_Explorer edge( aShape, TopAbs_EDGE ); edge.More() &&  !isQuadratic ; edge.Next() )
  {
    sm = aMesh.GetSubMesh( edge.Current() );
    vector<int>& nbElems = aResMap[ sm ];
    if ( SMDSEntity_Quad_Edge < (int) nbElems.size() )
      isQuadratic = nbElems[ SMDSEntity_Quad_Edge ];
  }

  int nbCircSegments = 0;
  for ( int iE = 0; iE < circSide->NbEdges(); ++iE )
  {
    sm = aMesh.GetSubMesh( circSide->Edge( iE ));
    vector<int>& nbElems = aResMap[ sm ];
    if ( SMDSEntity_Quad_Edge < (int) nbElems.size() )
      nbCircSegments += ( nbElems[ SMDSEntity_Edge ] + nbElems[ SMDSEntity_Quad_Edge ]);
  }
  
  int nbQuads = nbCircSegments * ( layerPositions.size() - 1 );
  int nbTria  = nbCircSegments;
  int nbNodes = ( nbCircSegments - 1 ) * ( layerPositions.size() - 2 );
  if ( isQuadratic )
  {
    nbNodes += (( nbCircSegments - 1 ) * ( layerPositions.size() - 1 ) + // radial
                ( nbCircSegments     ) * ( layerPositions.size() - 2 )); // circular
    aResVec[SMDSEntity_Quad_Triangle  ] = nbTria;
    aResVec[SMDSEntity_Quad_Quadrangle] = nbQuads;
  }
  else
  {
    aResVec[SMDSEntity_Triangle  ] = nbTria;
    aResVec[SMDSEntity_Quadrangle] = nbQuads;
  }
  aResVec[SMDSEntity_Node] = nbNodes;

  if ( linSide1 )
  {
    // evaluation for linSides
    vector<int> aResVec(SMDSEntity_Last, 0);
    if ( isQuadratic ) {
      aResVec[SMDSEntity_Node     ] = 2 * ( layerPositions.size() - 1 ) + 1;
      aResVec[SMDSEntity_Quad_Edge] = layerPositions.size() - 1;
    }
    else {
      aResVec[SMDSEntity_Node] = layerPositions.size() - 2;
      aResVec[SMDSEntity_Edge] = layerPositions.size() - 1;
    }
    sm = aMesh.GetSubMesh( linSide1->Edge(0) );
    aResMap[sm] = aResVec;
    if ( linSide2 )
    {
      sm = aMesh.GetSubMesh( linSide2->Edge(0) );
      aResMap[sm] = aResVec;
    }
  }

  return true;
}

//================================================================================
/*!
 * \brief Return true if the algorithm can compute mesh on this shape
 */
//================================================================================

bool StdMeshers_RadialQuadrangle_1D2D::IsApplicable( const TopoDS_Shape & aShape, bool toCheckAll )
{
  int nbFoundFaces = 0;
  for (TopExp_Explorer exp( aShape, TopAbs_FACE ); exp.More(); exp.Next(), ++nbFoundFaces )
  {
    StdMeshers_FaceSidePtr circSide, linSide1, linSide2;
    int nbSides = analyseFace( exp.Current(), NULL, circSide, linSide1, linSide2, NULL );
    bool ok = ( 0 < nbSides && nbSides <= 3 &&
                isCornerInsideCircle( circSide, linSide1, linSide2 ));
    if( toCheckAll  && !ok ) return false;
    if( !toCheckAll && ok  ) return true;
  }
  if( toCheckAll && nbFoundFaces != 0 ) return true;
  return false;
};
