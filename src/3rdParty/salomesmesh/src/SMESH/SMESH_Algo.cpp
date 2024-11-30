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
//  File   : SMESH_Algo.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH

#include "SMESH_Algo.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMDS_FacePosition.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_TypeDefs.hxx"
#include "SMESH_subMesh.hxx"

#include <Basics_OCCTVersion.hxx>

#include <BRepAdaptor_Curve.hxx>
#include <BRepLProp.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Surface.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>

#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>

#include "utilities.h"

#include <algorithm>
#include <limits>
#include "SMESH_ProxyMesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include <LDOMParser.hxx>
using namespace std;

//================================================================================
/*!
 * \brief Returns \a true if two algorithms (described by \a this and the given
 *        algo data) are compatible by their output and input types of elements.
 */
//================================================================================

bool SMESH_Algo::Features::IsCompatible( const SMESH_Algo::Features& algo2 ) const
{
  if ( _dim > algo2._dim ) return algo2.IsCompatible( *this );
  // algo2 is of highter dimension
  if ( _outElemTypes.empty() || algo2._inElemTypes.empty() )
    return false;
  bool compatible = true;
  set<SMDSAbs_GeometryType>::const_iterator myOutType = _outElemTypes.begin();
  for ( ; myOutType != _outElemTypes.end() && compatible; ++myOutType )
    compatible = algo2._inElemTypes.count( *myOutType );
  return compatible;
}

//================================================================================
/*!
 * \brief Return Data of the algorithm
 */
//================================================================================


const SMESH_Algo::Features& SMESH_Algo::GetFeatures( const std::string& algoType )
{
  static map< string, SMESH_Algo::Features > theFeaturesByName;
  if ( theFeaturesByName.empty() )
  {
    // Read Plugin.xml files
    vector< string > xmlPaths = SMESH_Gen::GetPluginXMLPaths();
    LDOMParser xmlParser;
    for ( size_t iXML = 0; iXML < xmlPaths.size(); ++iXML )
    {
      bool error = xmlParser.parse( xmlPaths[iXML].c_str() );
      if ( error )
      {
        TCollection_AsciiString data;
        INFOS( xmlParser.GetError(data) );
        continue;
      }
      // <algorithm type="Regular_1D"
      //            ...
      //            input="EDGE"
      //            output="QUAD,TRIA">
      //
      LDOM_Document xmlDoc = xmlParser.getDocument();
      LDOM_NodeList algoNodeList = xmlDoc.getElementsByTagName( "algorithm" );
      for ( int i = 0; i < algoNodeList.getLength(); ++i )
      {
        LDOM_Node     algoNode           = algoNodeList.item( i );
        LDOM_Element& algoElem           = (LDOM_Element&) algoNode;
        TCollection_AsciiString algoType = algoElem.getAttribute("type");
        TCollection_AsciiString input    = algoElem.getAttribute("input");
        TCollection_AsciiString output   = algoElem.getAttribute("output");
        TCollection_AsciiString dim      = algoElem.getAttribute("dim");
        TCollection_AsciiString label    = algoElem.getAttribute("label-id");
        if ( algoType.IsEmpty() ) continue;

        Features & data = theFeaturesByName[ algoType.ToCString() ];
        data._dim   = dim.IntegerValue();
        data._label = label.ToCString();
        for ( int isInput = 0; isInput < 2; ++isInput )
        {
          TCollection_AsciiString&   typeStr = isInput ? input : output;
          set<SMDSAbs_GeometryType>& typeSet = isInput ? data._inElemTypes : data._outElemTypes;
          int beg = 1, end;
          while ( beg <= typeStr.Length() )
          {
            while ( beg < typeStr.Length() && !isalpha( typeStr.Value( beg ) ))
              ++beg;
            end = beg;
            while ( end < typeStr.Length() && isalpha( typeStr.Value( end + 1 ) ))
              ++end;
            if ( end > beg )
            {
              TCollection_AsciiString typeName = typeStr.SubString( beg, end );
              if      ( typeName == "EDGE" ) typeSet.insert( SMDSGeom_EDGE );
              else if ( typeName == "TRIA" ) typeSet.insert( SMDSGeom_TRIANGLE );
              else if ( typeName == "QUAD" ) typeSet.insert( SMDSGeom_QUADRANGLE );
            }
            beg = end + 1;
          }
        }
      }
    }
  }
  return theFeaturesByName[ algoType ];
}


//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_Algo::SMESH_Algo (int hypId, int studyId, SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _compatibleAllHypFilter = _compatibleNoAuxHypFilter = NULL;
  _onlyUnaryInput = _requireDiscreteBoundary = _requireShape = true;
  _quadraticMesh = _supportSubmeshes = false;
  _error = COMPERR_OK;
  for ( int i = 0; i < 4; ++i )
    _neededLowerHyps[ i ] = false;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_Algo::~SMESH_Algo()
{
  delete _compatibleNoAuxHypFilter;
  // delete _compatibleAllHypFilter; -- _compatibleNoAuxHypFilter does it!!!
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_0D_Algo::SMESH_0D_Algo(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Algo(hypId, studyId, gen)
{
  _shapeType = (1 << TopAbs_VERTEX);
  _type = ALGO_0D;
}
SMESH_1D_Algo::SMESH_1D_Algo(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Algo(hypId, studyId, gen)
{
  _shapeType = (1 << TopAbs_EDGE);
  _type = ALGO_1D;
}
SMESH_2D_Algo::SMESH_2D_Algo(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Algo(hypId, studyId, gen)
{
  _shapeType = (1 << TopAbs_FACE);
  _type = ALGO_2D;
}
SMESH_3D_Algo::SMESH_3D_Algo(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Algo(hypId, studyId, gen)
{
  _shapeType = (1 << TopAbs_SOLID);
  _type = ALGO_3D;
}

//=============================================================================
/*!
 * Usually an algoritm has nothing to save
 */
//=============================================================================

ostream & SMESH_Algo::SaveTo(ostream & save) { return save; }
istream & SMESH_Algo::LoadFrom(istream & load) { return load; }

//=============================================================================
/*!
 *  
 */
//=============================================================================

const vector < string > &SMESH_Algo::GetCompatibleHypothesis()
{
  return _compatibleHypothesis;
}

//=============================================================================
/*!
 *  List the hypothesis used by the algorithm associated to the shape.
 *  Hypothesis associated to father shape -are- taken into account (see
 *  GetAppliedHypothesis). Relevant hypothesis have a name (type) listed in
 *  the algorithm. This method could be surcharged by specific algorithms, in 
 *  case of several hypothesis simultaneously applicable.
 */
//=============================================================================

const list <const SMESHDS_Hypothesis *> &
SMESH_Algo::GetUsedHypothesis(SMESH_Mesh &         aMesh,
                              const TopoDS_Shape & aShape,
                              const bool           ignoreAuxiliary) const
{
  SMESH_Algo* me = const_cast< SMESH_Algo* >( this );
  me->_usedHypList.clear();
  if ( const SMESH_HypoFilter* filter = GetCompatibleHypoFilter( ignoreAuxiliary ))
  {
    aMesh.GetHypotheses( aShape, *filter, me->_usedHypList, true );
    if ( ignoreAuxiliary && _usedHypList.size() > 1 )
      me->_usedHypList.clear(); //only one compatible hypothesis allowed
  }
  return _usedHypList;
}

//=============================================================================
/*!
 *  List the relevant hypothesis associated to the shape. Relevant hypothesis
 *  have a name (type) listed in the algorithm. Hypothesis associated to
 *  father shape -are not- taken into account (see GetUsedHypothesis)
 */
//=============================================================================

const list<const SMESHDS_Hypothesis *> &
SMESH_Algo::GetAppliedHypothesis(SMESH_Mesh &         aMesh,
                                 const TopoDS_Shape & aShape,
                                 const bool           ignoreAuxiliary) const
{
  SMESH_Algo* me = const_cast< SMESH_Algo* >( this );
  me->_appliedHypList.clear();
  if ( const SMESH_HypoFilter* filter = GetCompatibleHypoFilter( ignoreAuxiliary ))
    aMesh.GetHypotheses( aShape, *filter, me->_appliedHypList, false );

  return _appliedHypList;
}

//=============================================================================
/*!
 *  Compute length of an edge
 */
//=============================================================================

double SMESH_Algo::EdgeLength(const TopoDS_Edge & E)
{
  double UMin = 0, UMax = 0;
  TopLoc_Location L;
  Handle(Geom_Curve) C = BRep_Tool::Curve(E, L, UMin, UMax);
  if ( C.IsNull() )
    return 0.;
  GeomAdaptor_Curve AdaptCurve(C, UMin, UMax); //range is important for periodic curves
  double length = GCPnts_AbscissaPoint::Length(AdaptCurve, UMin, UMax);
  return length;
}

//================================================================================
/*!
 * \brief Just return false as the algorithm does not hold parameters values
 */
//================================================================================

bool SMESH_Algo::SetParametersByMesh(const SMESH_Mesh* /*theMesh*/,
                                     const TopoDS_Shape& /*theShape*/)
{
  return false;
}
bool SMESH_Algo::SetParametersByDefaults(const TDefaults& , const SMESH_Mesh*)
{
  return false;
}
//================================================================================
/*!
 * \brief Fill vector of node parameters on geometrical edge, including vertex nodes
 * \param theMesh - The mesh containing nodes
 * \param theEdge - The geometrical edge of interest
 * \param theParams - The resulting vector of sorted node parameters
 * \retval bool - false if not all parameters are OK
 */
//================================================================================

bool SMESH_Algo::GetNodeParamOnEdge(const SMESHDS_Mesh* theMesh,
                                    const TopoDS_Edge&  theEdge,
                                    vector< double > &  theParams)
{
  theParams.clear();

  if ( !theMesh || theEdge.IsNull() )
    return false;

  SMESHDS_SubMesh * eSubMesh = theMesh->MeshElements( theEdge );
  if ( !eSubMesh || !eSubMesh->GetElements()->more() )
    return false; // edge is not meshed

  //int nbEdgeNodes = 0;
  set < double > paramSet;
  if ( eSubMesh )
  {
    // loop on nodes of an edge: sort them by param on edge
    SMDS_NodeIteratorPtr nIt = eSubMesh->GetNodes();
    while ( nIt->more() )
    {
      const SMDS_MeshNode* node = nIt->next();
      const SMDS_PositionPtr& pos = node->GetPosition();
      if ( pos->GetTypeOfPosition() != SMDS_TOP_EDGE )
        return false;
      const SMDS_EdgePosition* epos =
        static_cast<const SMDS_EdgePosition*>(node->GetPosition());
      if ( !paramSet.insert( epos->GetUParameter() ).second )
        return false; // equal parameters
    }
  }
  // add vertex nodes params
  TopoDS_Vertex V1,V2;
  TopExp::Vertices( theEdge, V1, V2);
  if ( VertexNode( V1, theMesh ) &&
       !paramSet.insert( BRep_Tool::Parameter(V1,theEdge) ).second )
    return false; // there are equal parameters
  if ( VertexNode( V2, theMesh ) &&
       !paramSet.insert( BRep_Tool::Parameter(V2,theEdge) ).second )
    return false; // there are equal parameters

  // fill the vector
  theParams.resize( paramSet.size() );
  set < double >::iterator   par    = paramSet.begin();
  vector< double >::iterator vecPar = theParams.begin();
  for ( ; par != paramSet.end(); ++par, ++vecPar )
    *vecPar = *par;

  return theParams.size() > 1;
}

//================================================================================
/*!
 * \brief Fill vector of node parameters on geometrical edge, including vertex nodes
 * \param theMesh - The mesh containing nodes
 * \param theEdge - The geometrical edge of interest
 * \param theParams - The resulting vector of sorted node parameters
 * \retval bool - false if not all parameters are OK
 */
//================================================================================

bool SMESH_Algo::GetSortedNodesOnEdge(const SMESHDS_Mesh*                   theMesh,
                                      const TopoDS_Edge&                    theEdge,
                                      const bool                            ignoreMediumNodes,
                                      map< double, const SMDS_MeshNode* > & theNodes,
                                      const SMDSAbs_ElementType             typeToCheck)
{
  theNodes.clear();

  if ( !theMesh || theEdge.IsNull() )
    return false;

  SMESHDS_SubMesh * eSubMesh = theMesh->MeshElements( theEdge );
  if ( !eSubMesh || ( eSubMesh->NbElements()==0 &&  eSubMesh->NbNodes() == 0))
    return false; // edge is not meshed

  int nbNodes = 0;
  set < double > paramSet;
  if ( eSubMesh )
  {
    // loop on nodes of an edge: sort them by param on edge
    SMDS_NodeIteratorPtr nIt = eSubMesh->GetNodes();
    while ( nIt->more() )
    {
      const SMDS_MeshNode* node = nIt->next();
      if ( ignoreMediumNodes && SMESH_MesherHelper::IsMedium( node, typeToCheck ))
        continue;
      const SMDS_PositionPtr& pos = node->GetPosition();
      if ( pos->GetTypeOfPosition() != SMDS_TOP_EDGE )
        return false;
      const SMDS_EdgePosition* epos =
        static_cast<const SMDS_EdgePosition*>(node->GetPosition());
      theNodes.insert( theNodes.end(), make_pair( epos->GetUParameter(), node ));
      ++nbNodes;
    }
  }
  // add vertex nodes
  TopoDS_Vertex v1, v2;
  TopExp::Vertices(theEdge, v1, v2);
  const SMDS_MeshNode* n1 = VertexNode( v1, eSubMesh, 0 );
  const SMDS_MeshNode* n2 = VertexNode( v2, eSubMesh, 0 );
  Standard_Real f, l;
  BRep_Tool::Range(theEdge, f, l);
  if ( v1.Orientation() != TopAbs_FORWARD )
    std::swap( f, l );
  if ( n1 && ++nbNodes )
    theNodes.insert( make_pair( f, n1 ));
  if ( n2 && ++nbNodes )
    theNodes.insert( make_pair( l, n2 ));

  return theNodes.size() == nbNodes;
}

//================================================================================
/*!
 * \brief Returns the filter recognizing only compatible hypotheses
 *  \param ignoreAuxiliary - make filter ignore auxiliary hypotheses
 *  \retval SMESH_HypoFilter* - the filter that can be NULL
 */
//================================================================================

const SMESH_HypoFilter*
SMESH_Algo::GetCompatibleHypoFilter(const bool ignoreAuxiliary) const
{
  if ( !_compatibleHypothesis.empty() )
  {
    if ( !_compatibleAllHypFilter )
    {
      SMESH_HypoFilter* filter = new SMESH_HypoFilter();
      filter->Init( filter->HasName( _compatibleHypothesis[0] ));
      for ( int i = 1; i < _compatibleHypothesis.size(); ++i )
        filter->Or( filter->HasName( _compatibleHypothesis[ i ] ));

      SMESH_HypoFilter* filterNoAux = new SMESH_HypoFilter( filter );
      filterNoAux->AndNot( filterNoAux->IsAuxiliary() );

      // _compatibleNoAuxHypFilter will detele _compatibleAllHypFilter!!!
      SMESH_Algo* me = const_cast< SMESH_Algo* >( this );
      me->_compatibleAllHypFilter   = filter;
      me->_compatibleNoAuxHypFilter = filterNoAux;
    }
    return ignoreAuxiliary ? _compatibleNoAuxHypFilter : _compatibleAllHypFilter;
  }
  return 0;
}

//================================================================================
/*!
 * \brief Return continuity of two edges
 * \param E1 - the 1st edge
 * \param E2 - the 2nd edge
 * \retval GeomAbs_Shape - regularity at the junction between E1 and E2
 */
//================================================================================

GeomAbs_Shape SMESH_Algo::Continuity(TopoDS_Edge E1,
                                     TopoDS_Edge E2)
{
  //E1.Orientation(TopAbs_FORWARD), E2.Orientation(TopAbs_FORWARD); // avoid pb with internal edges
  if (E1.Orientation() > TopAbs_REVERSED) // INTERNAL
    E1.Orientation( TopAbs_FORWARD );
  if (E2.Orientation() > TopAbs_REVERSED) // INTERNAL
    E2.Orientation( TopAbs_FORWARD );

  TopoDS_Vertex V, VV1[2], VV2[2];
  TopExp::Vertices( E1, VV1[0], VV1[1], true );
  TopExp::Vertices( E2, VV2[0], VV2[1], true );
  if      ( VV1[1].IsSame( VV2[0] ))  { V = VV1[1]; }
  else if ( VV1[0].IsSame( VV2[1] ))  { V = VV1[0]; }
  else if ( VV1[1].IsSame( VV2[1] ))  { V = VV1[1]; E1.Reverse(); }
  else if ( VV1[0].IsSame( VV2[0] ))  { V = VV1[0]; E1.Reverse(); }
  else { return GeomAbs_C0; }

  Standard_Real u1 = BRep_Tool::Parameter( V, E1 );
  Standard_Real u2 = BRep_Tool::Parameter( V, E2 );
  BRepAdaptor_Curve C1( E1 ), C2( E2 );
  Standard_Real tol = BRep_Tool::Tolerance( V );
  Standard_Real angTol = 2e-3;
  try {
    OCC_CATCH_SIGNALS;
    return BRepLProp::Continuity(C1, C2, u1, u2, tol, angTol);
  }
  catch (Standard_Failure&) {
  }
  return GeomAbs_C0;
}

//================================================================================
/*!
 * \brief Return true if an edge can be considered straight
 */
//================================================================================

bool SMESH_Algo::IsStraight( const TopoDS_Edge & E,
                             const bool          degenResult)
{
  {
    double f,l;
    if ( BRep_Tool::Curve( E, f, l ).IsNull())
      return degenResult;
  }
  BRepAdaptor_Curve curve( E );
  switch( curve.GetType() )
  {
  case GeomAbs_Line:
    return true;
  case GeomAbs_Circle:
  case GeomAbs_Ellipse:
  case GeomAbs_Hyperbola:
  case GeomAbs_Parabola:
    return false;
  // case GeomAbs_BezierCurve:
  // case GeomAbs_BSplineCurve:
  // case GeomAbs_OtherCurve:
  default:;
  }
  const double   f = curve.FirstParameter();
  const double   l = curve.LastParameter();
  const gp_Pnt  pf = curve.Value( f );
  const gp_Pnt  pl = curve.Value( l );
  const gp_Vec v1( pf, pl );
  const double v1Len = v1.Magnitude();
  if ( v1Len < std::numeric_limits< double >::min() )
    return false; // E seems closed
  const double tol = Min( 10 * curve.Tolerance(), v1Len * 1e-2 );
  const double nbSamples = 7;
  for ( int i = 0; i < nbSamples; ++i )
  {
    const double  r = ( i + 1 ) / nbSamples;
    const gp_Pnt pi = curve.Value( f * r + l * ( 1 - r ));
    const gp_Vec vi( pf, pi );
    const double  h = 0.5 * v1.Crossed( vi ).Magnitude() / v1Len;
    if ( h > tol )
      return false;
  }
  return true;
}

//================================================================================
/*!
 * \brief Return true if an edge has no 3D curve
 */
//================================================================================

bool SMESH_Algo::isDegenerated( const TopoDS_Edge & E )
{
  double f,l;
  TopLoc_Location loc;
  Handle(Geom_Curve) C = BRep_Tool::Curve( E, loc, f,l );
  return C.IsNull();
}

//================================================================================
/*!
 * \brief Return the node built on a vertex
 * \param V - the vertex
 * \param meshDS - mesh
 * \retval const SMDS_MeshNode* - found node or NULL
 * \sa SMESH_MesherHelper::GetSubShapeByNode( const SMDS_MeshNode*, SMESHDS_Mesh* )
 */
//================================================================================

const SMDS_MeshNode* SMESH_Algo::VertexNode(const TopoDS_Vertex& V,
                                            const SMESHDS_Mesh*  meshDS)
{
  if ( SMESHDS_SubMesh* sm = meshDS->MeshElements(V) ) {
    SMDS_NodeIteratorPtr nIt= sm->GetNodes();
    if (nIt->more())
      return nIt->next();
  }
  return 0;
}

//=======================================================================
/*!
 * \brief Return the node built on a vertex.
 *        A node moved to other geometry by MergeNodes() is also returned.
 * \param V - the vertex
 * \param mesh - mesh
 * \retval const SMDS_MeshNode* - found node or NULL
 */
//=======================================================================

const SMDS_MeshNode* SMESH_Algo::VertexNode(const TopoDS_Vertex& V,
                                            const SMESH_Mesh*    mesh)
{
  const SMDS_MeshNode* node = VertexNode( V, mesh->GetMeshDS() );

  if ( !node && mesh->HasModificationsToDiscard() )
  {
    PShapeIteratorPtr edgeIt = SMESH_MesherHelper::GetAncestors( V, *mesh, TopAbs_EDGE );
    while ( const TopoDS_Shape* edge = edgeIt->next() )
      if ( SMESHDS_SubMesh* edgeSM = mesh->GetMeshDS()->MeshElements( *edge ))
        if ( edgeSM->NbElements() > 0 )
          return VertexNode( V, edgeSM, mesh, /*checkV=*/false );
  }
  return node;
}

//=======================================================================
/*!
 * \brief Return the node built on a vertex.
 *        A node moved to other geometry by MergeNodes() is also returned.
 * \param V - the vertex
 * \param edgeSM - sub-mesh of a meshed EDGE sharing the vertex
 * \param checkV - if \c true, presence of a node on the vertex is checked
 * \retval const SMDS_MeshNode* - found node or NULL
 */
//=======================================================================

const SMDS_MeshNode* SMESH_Algo::VertexNode(const TopoDS_Vertex&   V,
                                            const SMESHDS_SubMesh* edgeSM,
                                            const SMESH_Mesh*      mesh,
                                            const bool             checkV)
{
  const SMDS_MeshNode* node = checkV ? VertexNode( V, edgeSM->GetParent() ) : 0;

  if ( !node && edgeSM )
  {
    // find nodes not shared by mesh segments
    typedef set< const SMDS_MeshNode* >                       TNodeSet;
    typedef map< const SMDS_MeshNode*, const SMDS_MeshNode* > TNodeMap;
    TNodeMap notSharedNodes;
    TNodeSet otherShapeNodes;
    vector< const SMDS_MeshNode* > segNodes(3);
    SMDS_ElemIteratorPtr segIt = edgeSM->GetElements();
    while ( segIt->more() )
    {
      const SMDS_MeshElement* seg = segIt->next();
      if ( seg->GetType() != SMDSAbs_Edge )
        return node;
      segNodes.assign( seg->begin_nodes(), seg->end_nodes() );
      for ( int i = 0; i < 2; ++i )
      {
        const SMDS_MeshNode* n1 = segNodes[i];
        const SMDS_MeshNode* n2 = segNodes[1-i];
        pair<TNodeMap::iterator, bool> it2new = notSharedNodes.insert( make_pair( n1, n2 ));
        if ( !it2new.second ) // n encounters twice
          notSharedNodes.erase( it2new.first );
        if ( n1->getshapeId() != edgeSM->GetID() )
          otherShapeNodes.insert( n1 );
      }
    }
    if ( otherShapeNodes.size() == 1 && notSharedNodes.empty() ) // a closed EDGE
      return *otherShapeNodes.begin();

    if ( notSharedNodes.size() == 2 ) // two end nodes found
    {
      SMESHDS_Mesh*  meshDS = edgeSM->GetParent();
      const TopoDS_Shape& E = meshDS->IndexToShape( edgeSM->GetID() );
      if ( E.IsNull() || E.ShapeType() != TopAbs_EDGE )
        return node;
      const SMDS_MeshNode* n1 = notSharedNodes.begin ()->first;
      const SMDS_MeshNode* n2 = notSharedNodes.rbegin()->first;
      TopoDS_Shape S1 = SMESH_MesherHelper::GetSubShapeByNode( n1, meshDS );
      if ( S1.ShapeType() == TopAbs_VERTEX && SMESH_MesherHelper::IsSubShape( S1, E ))
        return n2;
      TopoDS_Shape S2 = SMESH_MesherHelper::GetSubShapeByNode( n2, meshDS );
      if ( S2.ShapeType() == TopAbs_VERTEX && SMESH_MesherHelper::IsSubShape( S2, E ))
        return n1;
      if ( edgeSM->NbElements() <= 2 || !mesh ) // one-two segments
      {
        gp_Pnt pV = BRep_Tool::Pnt( V );
        double dist1 = pV.SquareDistance( SMESH_TNodeXYZ( n1 ));
        double dist2 = pV.SquareDistance( SMESH_TNodeXYZ( n2 ));
        return dist1 < dist2 ? n1 : n2;
      }
      if ( mesh )
      {
        SMESH_MesherHelper helper( const_cast<SMESH_Mesh&>( *mesh ));
        const SMDS_MeshNode* n1i = notSharedNodes.begin ()->second;
        const SMDS_MeshNode* n2i = notSharedNodes.rbegin()->second;
        const TopoDS_Edge&  edge = TopoDS::Edge( E );
        bool  posOK = true;
        double pos1 = helper.GetNodeU( edge, n1i, n2i, &posOK );
        double pos2 = helper.GetNodeU( edge, n2i, n1i, &posOK );
        double posV = BRep_Tool::Parameter( V, edge );
        if ( Abs( pos1 - posV ) < Abs( pos2 - posV )) return n1;
        else                                          return n2;
      }
    }
  }
  return node;
}

//=======================================================================
//function : GetMeshError
//purpose  : Finds topological errors of a sub-mesh
//WARNING  : 1D check is NOT implemented so far
//=======================================================================

SMESH_Algo::EMeshError SMESH_Algo::GetMeshError(SMESH_subMesh* subMesh)
{
  EMeshError err = MEr_OK;

  SMESHDS_SubMesh* smDS = subMesh->GetSubMeshDS();
  if ( !smDS )
    return MEr_EMPTY;

  switch ( subMesh->GetSubShape().ShapeType() )
  {
  case TopAbs_FACE: { // ====================== 2D =====================

    SMDS_ElemIteratorPtr fIt = smDS->GetElements();
    if ( !fIt->more() )
      return MEr_EMPTY;

    // We check that olny links on EDGEs encouter once, the rest links, twice
    set< SMESH_TLink > links;
    while ( fIt->more() )
    {
      const SMDS_MeshElement* f = fIt->next();
      int nbNodes = f->NbCornerNodes(); // ignore medium nodes
      for ( int i = 0; i < nbNodes; ++i )
      {
        const SMDS_MeshNode* n1 = f->GetNode( i );
        const SMDS_MeshNode* n2 = f->GetNode(( i+1 ) % nbNodes);
        std::pair< set< SMESH_TLink >::iterator, bool > it_added =
          links.insert( SMESH_TLink( n1, n2 ));
        if ( !it_added.second )
          // As we do NOT(!) check if mesh is manifold, we believe that a link can
          // encounter once or twice only (not three times), we erase a link as soon
          // as it encounters twice to speed up search in the <links> map.
          links.erase( it_added.first );
      }
    }
    // the links remaining in the <links> should all be on EDGE
    set< SMESH_TLink >::iterator linkIt = links.begin();
    for ( ; linkIt != links.end(); ++linkIt )
    {
      const SMESH_TLink& link = *linkIt;
      if ( link.node1()->GetPosition()->GetTypeOfPosition() > SMDS_TOP_EDGE ||
           link.node2()->GetPosition()->GetTypeOfPosition() > SMDS_TOP_EDGE )
        return MEr_HOLES;
    }
    // TODO: to check orientation
    break;
  }
  case TopAbs_SOLID: { // ====================== 3D =====================

    SMDS_ElemIteratorPtr vIt = smDS->GetElements();
    if ( !vIt->more() )
      return MEr_EMPTY;

    SMDS_VolumeTool vTool;
    while ( !vIt->more() )
    {
      if (!vTool.Set( vIt->next() ))
        continue; // strange

      for ( int iF = 0; iF < vTool.NbFaces(); ++iF )
        if ( vTool.IsFreeFace( iF ))
        {
          int nbN = vTool.NbFaceNodes( iF );
          const SMDS_MeshNode** nodes =  vTool.GetFaceNodes( iF );
          for ( int i = 0; i < nbN; ++i )
            if ( nodes[i]->GetPosition()->GetTypeOfPosition() > SMDS_TOP_FACE )
              return MEr_HOLES;
        }
    }
    break;
  }
  default:;
  }
  return err;
}

//================================================================================
/*!
 * \brief Sets event listener to submeshes if necessary
 * \param subMesh - submesh where algo is set
 * 
 * After being set, event listener is notified on each event of a submesh.
 * By default non listener is set
 */
//================================================================================

void SMESH_Algo::SetEventListener(SMESH_subMesh* /*subMesh*/)
{
}

//================================================================================
/*!
 * \brief Allow algo to do something after persistent restoration
 * \param subMesh - restored submesh
 *
 * This method is called only if a submesh has HYP_OK algo_state.
 */
//================================================================================

void SMESH_Algo::SubmeshRestored(SMESH_subMesh* /*subMesh*/)
{
}

//================================================================================
/*!
 * \brief Computes mesh without geometry
 * \param aMesh - the mesh
 * \param aHelper - helper that must be used for adding elements to \aaMesh
 * \retval bool - is a success
 */
//================================================================================

bool SMESH_Algo::Compute(SMESH_Mesh & /*aMesh*/, SMESH_MesherHelper* /*aHelper*/)
{
  return error( COMPERR_BAD_INPUT_MESH, "Mesh built on shape expected");
}

//=======================================================================
//function : CancelCompute
//purpose  : Sets _computeCanceled to true. It's usage depends on
//  *        implementation of a particular mesher.
//=======================================================================

void SMESH_Algo::CancelCompute()
{
  _computeCanceled = true;
  _error = COMPERR_CANCELED;
}

//================================================================================
/*
 * If possible, returns progress of computation [0.,1.]
 */
//================================================================================

double SMESH_Algo::GetProgress() const
{
  return _progress;
}

//================================================================================
/*!
 * \brief store error and comment and then return ( error == COMPERR_OK )
 */
//================================================================================

bool SMESH_Algo::error(int error, const SMESH_Comment& comment)
{
  _error   = error;
  _comment = comment;
  return ( error == COMPERR_OK );
}

//================================================================================
/*!
 * \brief store error and return ( error == COMPERR_OK )
 */
//================================================================================

bool SMESH_Algo::error(SMESH_ComputeErrorPtr error)
{
  if ( error ) {
    _error   = error->myName;
    _comment = error->myComment;
    _badInputElements = error->myBadElements;
    return error->IsOK();
  }
  return true;
}

//================================================================================
/*!
 * \brief return compute error
 */
//================================================================================

SMESH_ComputeErrorPtr SMESH_Algo::GetComputeError() const
{
  SMESH_ComputeErrorPtr err = SMESH_ComputeError::New( _error, _comment, this );
  // hope this method is called by only SMESH_subMesh after this->Compute()
  err->myBadElements.splice( err->myBadElements.end(),
                             (list<const SMDS_MeshElement*>&) _badInputElements );
  return err;
}

//================================================================================
/*!
 * \brief initialize compute error before call of Compute()
 */
//================================================================================

void SMESH_Algo::InitComputeError()
{
  _error = COMPERR_OK;
  _comment.clear();
  list<const SMDS_MeshElement*>::iterator elem = _badInputElements.begin();
  for ( ; elem != _badInputElements.end(); ++elem )
    if ( (*elem)->GetID() < 1 )
      delete *elem;
  _badInputElements.clear();

  _computeCanceled = false;
  _progressTic     = 0;
  _progress        = 0.;
}

//================================================================================
/*!
 * \brief Return compute progress by nb of calls of this method
 */
//================================================================================

double SMESH_Algo::GetProgressByTic() const
{
  int computeCost = 0;
  for ( size_t i = 0; i < _smToCompute.size(); ++i )
    computeCost += _smToCompute[i]->GetComputeCost();

  const_cast<SMESH_Algo*>( this )->_progressTic++;

  double x = 5 * _progressTic;
  x = ( x < computeCost ) ? ( x / computeCost ) : 1.;
  return 0.9 * sin( x * M_PI / 2 );
}

//================================================================================
/*!
 * \brief store a bad input element preventing computation,
 *        which may be a temporary one i.e. not residing the mesh,
 *        then it will be deleted by InitComputeError()
 */
//================================================================================

void SMESH_Algo::addBadInputElement(const SMDS_MeshElement* elem)
{
  if ( elem )
    _badInputElements.push_back( elem );
}

//=======================================================================
//function : addBadInputElements
//purpose  : store a bad input elements or nodes preventing computation
//=======================================================================

void SMESH_Algo::addBadInputElements(const SMESHDS_SubMesh* sm,
                                     const bool             addNodes)
{
  if ( sm )
  {
    if ( addNodes )
    {
      SMDS_NodeIteratorPtr nIt = sm->GetNodes();
      while ( nIt->more() ) addBadInputElement( nIt->next() );
    }
    else
    {
      SMDS_ElemIteratorPtr eIt = sm->GetElements();
      while ( eIt->more() ) addBadInputElement( eIt->next() );
    }
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

// int SMESH_Algo::NumberOfWires(const TopoDS_Shape& S)
// {
//   int i = 0;
//   for (TopExp_Explorer exp(S,TopAbs_WIRE); exp.More(); exp.Next())
//     i++;
//   return i;
// }

//=============================================================================
/*!
 *  
 */
//=============================================================================

int SMESH_Algo::NumberOfPoints(SMESH_Mesh& aMesh, const TopoDS_Wire& W)
{
  int nbPoints = 0;
  for (TopExp_Explorer exp(W,TopAbs_EDGE); exp.More(); exp.Next()) {
    const TopoDS_Edge& E = TopoDS::Edge(exp.Current());
    int nb = aMesh.GetSubMesh(E)->GetSubMeshDS()->NbNodes();
    if(_quadraticMesh)
      nb = nb/2;
    nbPoints += nb + 1; // internal points plus 1 vertex of 2 (last point ?)
  }
  return nbPoints;
}


//================================================================================
/*!
 * Method in which an algorithm generating a structured mesh
 * fixes positions of in-face nodes after there movement
 * due to insertion of viscous layers.
 */
//================================================================================

bool SMESH_2D_Algo::FixInternalNodes(const SMESH_ProxyMesh& mesh,
                                     const TopoDS_Face&     face)
{
  const SMESHDS_SubMesh* smDS = mesh.GetSubMesh(face);
  if ( !smDS || smDS->NbElements() < 1 )
    return false;

  SMESH_MesherHelper helper( *mesh.GetMesh() );

  // get all faces from a proxy sub-mesh
  typedef SMDS_StdIterator< const SMDS_MeshElement*, SMDS_ElemIteratorPtr > TIterator;
  TIDSortedElemSet allFaces( TIterator( smDS->GetElements() ), TIterator() );
  TIDSortedElemSet avoidSet, firstRowQuads;

  // indices of nodes to pass to a neighbour quad using SMESH_MeshAlgos::FindFaceInSet()
  int iN1, iN2;

  // get two first rows of nodes by passing through the first row of faces
  vector< vector< const SMDS_MeshNode* > > nodeRows;
  int iRow1 = 0, iRow2 = 1;
  const SMDS_MeshElement* quad;
  {
    // look for a corner quadrangle and it's corner node
    const SMDS_MeshElement* cornerQuad = 0;
    int                     cornerNodeInd = -1;
    SMDS_ElemIteratorPtr fIt = smDS->GetElements();
    while ( !cornerQuad && fIt->more() )
    {
      cornerQuad = fIt->next();
      if ( cornerQuad->NbCornerNodes() != 4 )
        return false;
      SMDS_NodeIteratorPtr nIt = cornerQuad->nodeIterator();
      for ( int i = 0; i < 4; ++i )
      {
        int nbInverseQuads = 0;
        SMDS_ElemIteratorPtr fIt = nIt->next()->GetInverseElementIterator(SMDSAbs_Face);
        while ( fIt->more() )
          nbInverseQuads += allFaces.count( fIt->next() );
        if ( nbInverseQuads == 1 )
          cornerNodeInd = i, i = 4;
      }
      if ( cornerNodeInd < 0 )
        cornerQuad = 0;
    }
    if ( !cornerQuad || cornerNodeInd < 0 )
      return false;

    iN1     = helper.WrapIndex( cornerNodeInd + 1, 4 );
    iN2     = helper.WrapIndex( cornerNodeInd + 2, 4 );
    int iN3 = helper.WrapIndex( cornerNodeInd + 3, 4 );
    nodeRows.resize(2);
    nodeRows[iRow1].push_back( cornerQuad->GetNode( cornerNodeInd ));
    nodeRows[iRow1].push_back( cornerQuad->GetNode( iN1 ));
    nodeRows[iRow2].push_back( cornerQuad->GetNode( iN3 ));
    nodeRows[iRow2].push_back( cornerQuad->GetNode( iN2 ));
    firstRowQuads.insert( cornerQuad );

    // pass through the rest quads in a face row
    quad = cornerQuad;
    while ( quad )
    {
      avoidSet.clear();
      avoidSet.insert( quad );
      if (( quad = SMESH_MeshAlgos::FindFaceInSet( nodeRows[iRow1].back(),
                                                   nodeRows[iRow2].back(),
                                                   allFaces, avoidSet, &iN1, &iN2)))
      {
        nodeRows[iRow1].push_back( quad->GetNode( helper.WrapIndex( iN2 + 2, 4 )));
        nodeRows[iRow2].push_back( quad->GetNode( helper.WrapIndex( iN1 + 2, 4 )));
        if ( quad->NbCornerNodes() != 4 )
          return false;
      }
    }
    if ( nodeRows[iRow1].size() < 3 )
      return true; // there is nothing to fix
  }

  nodeRows.reserve( smDS->NbElements() / nodeRows[iRow1].size() );

  // get the rest node rows
  while ( true )
  {
    ++iRow1, ++iRow2;

    // get the first quad in the next face row 
    if (( quad = SMESH_MeshAlgos::FindFaceInSet( nodeRows[iRow1][0],
                                                 nodeRows[iRow1][1],
                                                 allFaces, /*avoid=*/firstRowQuads,
                                                 &iN1, &iN2)))
    {
      if ( quad->NbCornerNodes() != 4 )
        return false;
      nodeRows.resize( iRow2+1 );
      nodeRows[iRow2].push_back( quad->GetNode( helper.WrapIndex( iN2 + 2, 4 )));
      nodeRows[iRow2].push_back( quad->GetNode( helper.WrapIndex( iN1 + 2, 4 )));
      firstRowQuads.insert( quad );
    }
    else
    {
      break; // no more rows
    }

    // pass through the rest quads in a face row
    while ( quad )
    {
      avoidSet.clear();
      avoidSet.insert( quad );
      if (( quad = SMESH_MeshAlgos::FindFaceInSet( nodeRows[iRow1][ nodeRows[iRow2].size()-1 ],
                                                   nodeRows[iRow2].back(),
                                                   allFaces, avoidSet, &iN1, &iN2)))
      {
        if ( quad->NbCornerNodes() != 4 )
          return false;
        nodeRows[iRow2].push_back( quad->GetNode( helper.WrapIndex( iN1 + 2, 4 )));
      }
    }
    if ( nodeRows[iRow1].size() != nodeRows[iRow2].size() )
      return false;
  }
  if ( nodeRows.size() < 3 )
    return true; // there is nothing to fix

  // get params of the first (bottom) and last (top) node rows
  UVPtStructVec uvB( nodeRows[0].size() ), uvT( nodeRows[0].size() );
  for ( int isBot = 0; isBot < 2; ++isBot )
  {
    UVPtStructVec &                  uvps = isBot ? uvB : uvT;
    vector< const SMDS_MeshNode* >& nodes = nodeRows[ isBot ? 0 : nodeRows.size()-1 ];
    for ( size_t i = 0; i < nodes.size(); ++i )
    {
      uvps[i].node = nodes[i];
      gp_XY uv = helper.GetNodeUV( face, uvps[i].node );
      uvps[i].u = uv.Coord(1);
      uvps[i].v = uv.Coord(2);
      uvps[i].x = 0;
    }
    // calculate x (normalized param)
    for ( size_t i = 1; i < nodes.size(); ++i )
      uvps[i].x = uvps[i-1].x + SMESH_TNodeXYZ( uvps[i-1].node ).Distance( uvps[i].node );
    for ( size_t i = 1; i < nodes.size(); ++i )
      uvps[i].x /= uvps.back().x;
  }

  // get params of the left and right node rows
  UVPtStructVec uvL( nodeRows.size() ), uvR( nodeRows.size() );
  for ( int isLeft = 0; isLeft < 2; ++isLeft )
  {
    UVPtStructVec & uvps = isLeft ? uvL : uvR;
    const int       iCol = isLeft ? 0 : nodeRows[0].size() - 1;
    for ( size_t i = 0; i < nodeRows.size(); ++i )
    {
      uvps[i].node = nodeRows[i][iCol];
      gp_XY uv = helper.GetNodeUV( face, uvps[i].node );
      uvps[i].u = uv.Coord(1);
      uvps[i].v = uv.Coord(2);
      uvps[i].y = 0;
    }
    // calculate y (normalized param)
    for ( size_t i = 1; i < nodeRows.size(); ++i )
      uvps[i].y = uvps[i-1].y + SMESH_TNodeXYZ( uvps[i-1].node ).Distance( uvps[i].node );
    for ( size_t i = 1; i < nodeRows.size(); ++i )
      uvps[i].y /= uvps.back().y;
  }

  // update node coordinates
  SMESHDS_Mesh*   meshDS = mesh.GetMeshDS();
  Handle(Geom_Surface) S = BRep_Tool::Surface( face );
  gp_XY a0 ( uvB.front().u, uvB.front().v );
  gp_XY a1 ( uvB.back().u,  uvB.back().v );
  gp_XY a2 ( uvT.back().u,  uvT.back().v );
  gp_XY a3 ( uvT.front().u, uvT.front().v );
  for ( size_t iRow = 1; iRow < nodeRows.size()-1; ++iRow )
  {
    gp_XY p1 ( uvR[ iRow ].u, uvR[ iRow ].v );
    gp_XY p3 ( uvL[ iRow ].u, uvL[ iRow ].v );
    const double y0 = uvL[ iRow ].y;
    const double y1 = uvR[ iRow ].y;
    for ( size_t iCol = 1; iCol < nodeRows[0].size()-1; ++iCol )
    {
      gp_XY p0 ( uvB[ iCol ].u, uvB[ iCol ].v );
      gp_XY p2 ( uvT[ iCol ].u, uvT[ iCol ].v );
      const double x0 = uvB[ iCol ].x;
      const double x1 = uvT[ iCol ].x;
      double x = (x0 + y0 * (x1 - x0)) / (1 - (y1 - y0) * (x1 - x0));
      double y = y0 + x * (y1 - y0);
      gp_XY uv = helper.calcTFI( x, y, a0,a1,a2,a3, p0,p1,p2,p3 );
      gp_Pnt p = S->Value( uv.Coord(1), uv.Coord(2));
      const SMDS_MeshNode* n = nodeRows[iRow][iCol];
      meshDS->MoveNode( n, p.X(), p.Y(), p.Z() );
      if ( SMDS_FacePosition* pos = dynamic_cast< SMDS_FacePosition*>( n->GetPosition() ))
        pos->SetParameters( uv.Coord(1), uv.Coord(2) );
    }
  }
  return true;
}
