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

//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_Mesher.cxx
// Author    : Michael Sazonov (OCN)
// Date      : 31/03/2006
// Project   : SALOME
//=============================================================================

#include "NETGENPlugin_Mesher.hxx"
#include "NETGENPlugin_Hypothesis_2D.hxx"
#include "NETGENPlugin_SimpleHypothesis_3D.hxx"

#include <SMDS_FaceOfNodes.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMDS_MeshNode.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Block.hxx>
#include <SMESH_Comment.hxx>
#include <SMESH_ComputeError.hxx>
#include <SMESH_File.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_MesherHelper.hxx>
#include <SMESH_subMesh.hxx>
#include <StdMeshers_QuadToTriaAdaptor.hxx>
#include <StdMeshers_ViscousLayers2D.hxx>

// #include <SALOMEDS_Tool.hxx>

#include <utilities.h>

#include <BRepBuilderAPI_Copy.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_B3d.hxx>
#include <NCollection_Map.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_ProgramError.hxx>
#include <TColStd_MapOfInteger.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeInteger.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeShape.hxx>
#include <TopTools_DataMapOfShapeInteger.hxx>
#include <TopTools_DataMapOfShapeShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>

#ifdef _MSC_VER
#pragma warning(disable : 4067)
#endif

namespace nglib {
#include <nglib.h>
}
#ifndef OCCGEOMETRY
#define OCCGEOMETRY
#endif

#include <occgeom.hpp>
#include <meshing.hpp>
//#include <ngexception.hpp>
namespace netgen {
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, shared_ptr<Mesh>&, MeshingParameters&);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(6,0)
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, shared_ptr<Mesh>&, MeshingParameters&, int, int);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, MeshingParameters&, int, int);
#else
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, int, int, char*);
#endif
  //extern void OCCSetLocalMeshSize(OCCGeometry & geom, Mesh & mesh);
  DLL_HEADER extern MeshingParameters mparam;
  DLL_HEADER extern volatile multithreadt multithread;
  DLL_HEADER extern bool merge_solids;
}

#include <vector>
#include <limits>

#ifdef WIN32
#include <process.h>
#endif
using namespace nglib;
using namespace std;

#ifdef _DEBUG_
#define nodeVec_ACCESS(index) ((SMDS_MeshNode*) nodeVec.at((index)))
#else
#define nodeVec_ACCESS(index) ((SMDS_MeshNode*) nodeVec[index])
#endif

#define NGPOINT_COORDS(p) p(0),p(1),p(2)

#ifdef _DEBUG_
// dump elements added to ng mesh
//#define DUMP_SEGMENTS
//#define DUMP_TRIANGLES
//#define DUMP_TRIANGLES_SCRIPT "/tmp/trias.py" //!< debug AddIntVerticesInSolids()
#endif

TopTools_IndexedMapOfShape ShapesWithLocalSize;
std::map<int,double> VertexId2LocalSize;
std::map<int,double> EdgeId2LocalSize;
std::map<int,double> FaceId2LocalSize;

//=============================================================================
/*!
 *
 */
//=============================================================================

NETGENPlugin_Mesher::NETGENPlugin_Mesher (SMESH_Mesh*         mesh,
                                          const TopoDS_Shape& aShape,
                                          const bool          isVolume)
  : _mesh    (mesh),
    _shape   (aShape),
    _isVolume(isVolume),
    _optimize(true),
    _fineness(NETGENPlugin_Hypothesis::GetDefaultFineness()),
    _isViscousLayers2D(false),
    _ngMesh(NULL),
    _occgeom(NULL),
    _curShapeIndex(-1),
    _progressTic(1),
    _totalTime(1.0),
    _simpleHyp(NULL),
    _ptrToMe(NULL)
{
  SetDefaultParameters();
  ShapesWithLocalSize.Clear();
  VertexId2LocalSize.clear();
  EdgeId2LocalSize.clear();
  FaceId2LocalSize.clear();
}

//================================================================================
/*!
 * Destuctor
 */
//================================================================================

NETGENPlugin_Mesher::~NETGENPlugin_Mesher()
{
  if ( _ptrToMe )
    *_ptrToMe = NULL;
  _ptrToMe = 0;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
  _ngMesh = NULL;
#endif
}

//================================================================================
/*!
 * Set pointer to NETGENPlugin_Mesher* field of the holder, that will be
 * nullified at destruction of this
 */
//================================================================================

void NETGENPlugin_Mesher::SetSelfPointer( NETGENPlugin_Mesher ** ptr )
{
  if ( _ptrToMe )
    *_ptrToMe = NULL;

  _ptrToMe = ptr;

  if ( _ptrToMe )
    *_ptrToMe = this;
}

//================================================================================
/*!
 * \brief Initialize global NETGEN parameters with default values
 */
//================================================================================

void NETGENPlugin_Mesher::SetDefaultParameters()
{
  netgen::MeshingParameters& mparams = netgen::mparam;
  // maximal mesh edge size
  mparams.maxh            = 0;//NETGENPlugin_Hypothesis::GetDefaultMaxSize();
  mparams.minh            = 0;
  // minimal number of segments per edge
  mparams.segmentsperedge = NETGENPlugin_Hypothesis::GetDefaultNbSegPerEdge();
  // rate of growth of size between elements
  mparams.grading         = NETGENPlugin_Hypothesis::GetDefaultGrowthRate();
  // safety factor for curvatures (elements per radius)
  mparams.curvaturesafety = NETGENPlugin_Hypothesis::GetDefaultNbSegPerRadius();
  // create elements of second order
  mparams.secondorder     = NETGENPlugin_Hypothesis::GetDefaultSecondOrder();
  // quad-dominated surface meshing
  if (_isVolume)
    mparams.quad          = 0;
  else
    mparams.quad          = NETGENPlugin_Hypothesis_2D::GetDefaultQuadAllowed();
  _fineness               = NETGENPlugin_Hypothesis::GetDefaultFineness();
  mparams.uselocalh       = NETGENPlugin_Hypothesis::GetDefaultSurfaceCurvature();
  netgen::merge_solids    = NETGENPlugin_Hypothesis::GetDefaultFuseEdges();
}

//=============================================================================
/*!
 *
 */
//=============================================================================

void SetLocalSize(TopoDS_Shape GeomShape, double LocalSize)
{
  if ( GeomShape.IsNull() ) return;
  TopAbs_ShapeEnum GeomType = GeomShape.ShapeType();
  if (GeomType == TopAbs_COMPOUND) {
    for (TopoDS_Iterator it (GeomShape); it.More(); it.Next()) {
      SetLocalSize(it.Value(), LocalSize);
    }
    return;
  }
  int key;
  if (! ShapesWithLocalSize.Contains(GeomShape))
    key = ShapesWithLocalSize.Add(GeomShape);
  else
    key = ShapesWithLocalSize.FindIndex(GeomShape);
  if (GeomType == TopAbs_VERTEX) {
    VertexId2LocalSize[key] = LocalSize;
  } else if (GeomType == TopAbs_EDGE) {
    EdgeId2LocalSize[key] = LocalSize;
  } else if (GeomType == TopAbs_FACE) {
    FaceId2LocalSize[key] = LocalSize;
  }
}

//=============================================================================
/*!
 * Pass parameters to NETGEN
 */
//=============================================================================
void NETGENPlugin_Mesher::SetParameters(const NETGENPlugin_Hypothesis* hyp)
{
  if (hyp)
  {
    netgen::MeshingParameters& mparams = netgen::mparam;
    // Initialize global NETGEN parameters:
    // maximal mesh segment size
    mparams.maxh            = hyp->GetMaxSize();
    // maximal mesh element linear size
    mparams.minh            = hyp->GetMinSize();
    // minimal number of segments per edge
    mparams.segmentsperedge = hyp->GetNbSegPerEdge();
    // rate of growth of size between elements
    mparams.grading         = hyp->GetGrowthRate();
    // safety factor for curvatures (elements per radius)
    mparams.curvaturesafety = hyp->GetNbSegPerRadius();
    // create elements of second order
    mparams.secondorder     = hyp->GetSecondOrder() ? 1 : 0;
    // quad-dominated surface meshing
    // only triangles are allowed for volumic mesh (before realizing IMP 0021676)
    //if (!_isVolume)
      mparams.quad          = hyp->GetQuadAllowed() ? 1 : 0;
    _optimize               = hyp->GetOptimize();
    _fineness               = hyp->GetFineness();
    mparams.uselocalh       = hyp->GetSurfaceCurvature();
    netgen::merge_solids    = hyp->GetFuseEdges();
    _simpleHyp = NULL;


/* vejmarie

    SMESH_Gen_i* smeshGen_i = SMESH_Gen_i::GetSMESHGen();
    CORBA::Object_var anObject = smeshGen_i->GetNS()->Resolve("/myStudyManager");
    SALOMEDS::StudyManager_var aStudyMgr = SALOMEDS::StudyManager::_narrow(anObject);
    SALOMEDS::Study_var myStudy = aStudyMgr->GetStudyByID(hyp->GetStudyId());
    const NETGENPlugin_Hypothesis::TLocalSize localSizes = hyp->GetLocalSizesAndEntries();
    NETGENPlugin_Hypothesis::TLocalSize::const_iterator it = localSizes.begin();
    for (it ; it != localSizes.end() ; it++)
      {
        std::string entry = (*it).first;
        double val = (*it).second;
        // --
        GEOM::GEOM_Object_var aGeomObj;
        TopoDS_Shape S = TopoDS_Shape();
        SALOMEDS::SObject_var aSObj = myStudy->FindObjectID( entry.c_str() );
        if (!aSObj->_is_nil()) {
          CORBA::Object_var obj = aSObj->GetObject();
          aGeomObj = GEOM::GEOM_Object::_narrow(obj);
          aSObj->UnRegister();
        }
        if ( !aGeomObj->_is_nil() )
          S = smeshGen_i->GeomObjectToShape( aGeomObj.in() );
        // --
        SetLocalSize(S, val);
      }
*/
  }
}

//=============================================================================
/*!
 * Pass simple parameters to NETGEN
 */
//=============================================================================

void NETGENPlugin_Mesher::SetParameters(const NETGENPlugin_SimpleHypothesis_2D* hyp)
{
  _simpleHyp = hyp;
  if ( _simpleHyp )
    SetDefaultParameters();
}

//=============================================================================
/*!
 *  Link - a pair of integer numbers
 */
//=============================================================================
struct Link
{
  int n1, n2;
  Link(int _n1, int _n2) : n1(_n1), n2(_n2) {}
  Link() : n1(0), n2(0) {}
  bool Contains( int n ) const { return n == n1 || n == n2; }
  bool IsConnected( const Link& other ) const
  {
    return (( Contains( other.n1 ) || Contains( other.n2 )) && ( this != &other ));
  }
};

int HashCode(const Link& aLink, int aLimit)
{
  return HashCode(aLink.n1 + aLink.n2, aLimit);
}

Standard_Boolean IsEqual(const Link& aLink1, const Link& aLink2)
{
  return ((aLink1.n1 == aLink2.n1 && aLink1.n2 == aLink2.n2) ||
          (aLink1.n1 == aLink2.n2 && aLink1.n2 == aLink2.n1));
}

namespace
{
  //================================================================================
  /*!
   * \brief return id of netgen point corresponding to SMDS node
   */
  //================================================================================
  typedef map< const SMDS_MeshNode*, int > TNode2IdMap;

  int ngNodeId( const SMDS_MeshNode* node,
                netgen::Mesh&        ngMesh,
                TNode2IdMap&         nodeNgIdMap)
  {
    int newNgId = ngMesh.GetNP() + 1;

    TNode2IdMap::iterator node_id = nodeNgIdMap.insert( make_pair( node, newNgId )).first;

    if ( node_id->second == newNgId)
    {
#if defined(DUMP_SEGMENTS) || defined(DUMP_TRIANGLES)
      cout << "Ng " << newNgId << " - " << node;
#endif
      netgen::MeshPoint p( netgen::Point<3> (node->X(), node->Y(), node->Z()) );
      ngMesh.AddPoint( p );
    }
    return node_id->second;
  }

  //================================================================================
  /*!
   * \brief Return computed EDGEs connected to the given one
   */
  //================================================================================

  list< TopoDS_Edge > getConnectedEdges( const TopoDS_Edge&                 edge,
                                         const TopoDS_Face&                 face,
                                         const set< SMESH_subMesh* > &      computedSM,
                                         const SMESH_MesherHelper&          helper,
                                         map< SMESH_subMesh*, set< int > >& addedEdgeSM2Faces)
  {
    // get ordered EDGEs
    list< TopoDS_Edge > edges;
    list< int > nbEdgesInWire;
    int nbWires = SMESH_Block::GetOrderedEdges( face, edges, nbEdgesInWire);

    // find <edge> within <edges>
    list< TopoDS_Edge >::iterator eItFwd = edges.begin();
    for ( ; eItFwd != edges.end(); ++eItFwd )
      if ( edge.IsSame( *eItFwd ))
        break;
    if ( eItFwd == edges.end()) return list< TopoDS_Edge>();

    if ( eItFwd->Orientation() >= TopAbs_INTERNAL )
    {
      // connected INTERNAL edges returned from GetOrderedEdges() are wrongly oriented
      // so treat each INTERNAL edge separately
      TopoDS_Edge e = *eItFwd;
      edges.clear();
      edges.push_back( e );
      return edges;
    }

    // get all computed EDGEs connected to <edge>

    list< TopoDS_Edge >::iterator eItBack = eItFwd, ePrev;
    TopoDS_Vertex vCommon;
    TopTools_MapOfShape eAdded; // map used not to add a seam edge twice to <edges>
    eAdded.Add( edge );

    // put edges before <edge> to <edges> back
    while ( edges.begin() != eItFwd )
      edges.splice( edges.end(), edges, edges.begin() );

    // search forward
    ePrev = eItFwd;
    while ( ++eItFwd != edges.end() )
    {
      SMESH_subMesh* sm = helper.GetMesh()->GetSubMesh( *eItFwd );

      bool connected = TopExp::CommonVertex( *ePrev, *eItFwd, vCommon );
      bool computed  = sm->IsMeshComputed();
      bool added     = addedEdgeSM2Faces[ sm ].count( helper.GetSubShapeID() );
      bool doubled   = !eAdded.Add( *eItFwd );
      bool orientOK  = (( ePrev ->Orientation() < TopAbs_INTERNAL ) ==
                        ( eItFwd->Orientation() < TopAbs_INTERNAL )    );
      if ( !connected || !computed || !orientOK || added || doubled )
      {
        // stop advancement; move edges from tail to head
        while ( edges.back() != *ePrev )
          edges.splice( edges.begin(), edges, --edges.end() );
        break;
      }
      ePrev = eItFwd;
    }
    // search backward
    while ( eItBack != edges.begin() )
    {
      ePrev = eItBack;
      --eItBack;
      SMESH_subMesh* sm = helper.GetMesh()->GetSubMesh( *eItBack );

      bool connected = TopExp::CommonVertex( *ePrev, *eItBack, vCommon );
      bool computed  = sm->IsMeshComputed();
      bool added     = addedEdgeSM2Faces[ sm ].count( helper.GetSubShapeID() );
      bool doubled   = !eAdded.Add( *eItBack );
      bool orientOK  = (( ePrev  ->Orientation() < TopAbs_INTERNAL ) ==
                        ( eItBack->Orientation() < TopAbs_INTERNAL )    );
      if ( !connected || !computed || !orientOK || added || doubled)
      {
        // stop advancement
        edges.erase( edges.begin(), ePrev );
        break;
      }
    }
    if ( edges.front() != edges.back() )
    {
      // assure that the 1st vertex is meshed
      TopoDS_Edge eLast = edges.back();
      while ( !SMESH_Algo::VertexNode( SMESH_MesherHelper::IthVertex( 0, edges.front()), helper.GetMeshDS())
              &&
              edges.front() != eLast )
        edges.splice( edges.end(), edges, edges.begin() );
    }
    return edges;
  }

  //================================================================================
  /*!
   * \brief Make triangulation of a shape precise enough
   */
  //================================================================================

  void updateTriangulation( const TopoDS_Shape& shape )
  {
    // static set< Poly_Triangulation* > updated;

    // TopLoc_Location loc;
    // TopExp_Explorer fExp( shape, TopAbs_FACE );
    // for ( ; fExp.More(); fExp.Next() )
    // {
    //   Handle(Poly_Triangulation) triangulation =
    //     BRep_Tool::Triangulation ( TopoDS::Face( fExp.Current() ), loc);
    //   if ( triangulation.IsNull() ||
    //        updated.insert( triangulation.operator->() ).second )
    //   {
    //     BRepTools::Clean (shape);
        try {
          OCC_CATCH_SIGNALS;
          BRepMesh_IncrementalMesh e(shape, 0.01, true);
        }
        catch (Standard_Failure)
        {
        }
  //       updated.erase( triangulation.operator->() );
  //       triangulation = BRep_Tool::Triangulation ( TopoDS::Face( fExp.Current() ), loc);
  //       updated.insert( triangulation.operator->() );
  //     }
  //   }
  }
  //================================================================================
  /*!
   * \brief Returns a medium node either existing in SMESH of created by NETGEN
   *  \param [in] corner1 - corner node 1
   *  \param [in] corner2 - corner node 2
   *  \param [in] defaultMedium - the node created by NETGEN
   *  \param [in] helper - holder of medium nodes existing in SMESH
   *  \return const SMDS_MeshNode* - the result node
   */
  //================================================================================

  const SMDS_MeshNode* mediumNode( const SMDS_MeshNode*      corner1,
                                   const SMDS_MeshNode*      corner2,
                                   const SMDS_MeshNode*      defaultMedium,
                                   const SMESH_MesherHelper* helper)
  {
    if ( helper )
    {
      TLinkNodeMap::const_iterator l2n =
        helper->GetTLinkNodeMap().find( SMESH_TLink( corner1, corner2 ));
      if ( l2n != helper->GetTLinkNodeMap().end() )
        defaultMedium = l2n->second;
    }
    return defaultMedium;
  }

  //================================================================================
  /*!
   * \brief Assure that mesh on given shapes is quadratic
   */
  //================================================================================

  inline void makeQuadratic( const TopTools_IndexedMapOfShape& shapes,
                      SMESH_Mesh*                       mesh )
  {
    for ( int i = 1; i <= shapes.Extent(); ++i )
    {
      SMESHDS_SubMesh* smDS = mesh->GetMeshDS()->MeshElements( shapes(i) );
      if ( !smDS ) continue;
      SMDS_ElemIteratorPtr elemIt = smDS->GetElements();
      if ( !elemIt->more() ) continue;
      const SMDS_MeshElement* e = elemIt->next();
      if ( !e || e->IsQuadratic() )
        continue;

      TIDSortedElemSet elems;
      elems.insert( e );
      while ( elemIt->more() )
        elems.insert( elems.end(), elemIt->next() );

      SMESH_MeshEditor( mesh ).ConvertToQuadratic( /*3d=*/false, elems, /*biQuad=*/false );
    }
  }

}

//================================================================================
/*!
 * \brief Initialize netgen::OCCGeometry with OCCT shape
 */
//================================================================================

void NETGENPlugin_Mesher::PrepareOCCgeometry(netgen::OCCGeometry&     occgeo,
                                             const TopoDS_Shape&      shape,
                                             SMESH_Mesh&              mesh,
                                             list< SMESH_subMesh* > * meshedSM,
                                             NETGENPlugin_Internals*  intern)
{
  updateTriangulation( shape );

  Bnd_Box bb;
  BRepBndLib::Add (shape, bb);
  double x1,y1,z1,x2,y2,z2;
  bb.Get (x1,y1,z1,x2,y2,z2);
  MESSAGE("shape bounding box:\n" <<
          "(" << x1 << " " << y1 << " " << z1 << ") " <<
          "(" << x2 << " " << y2 << " " << z2 << ")");
  netgen::Point<3> p1 = netgen::Point<3> (x1,y1,z1);
  netgen::Point<3> p2 = netgen::Point<3> (x2,y2,z2);
  occgeo.boundingbox = netgen::Box<3> (p1,p2);

  occgeo.shape = shape;
  occgeo.changed = 1;

  // fill maps of shapes of occgeo with not yet meshed subshapes

  // get root submeshes
  list< SMESH_subMesh* > rootSM;
  const int shapeID = mesh.GetMeshDS()->ShapeToIndex( shape );
  if ( shapeID > 0 ) { // SMESH_subMesh with ID 0 may exist, don't use it!
    rootSM.push_back( mesh.GetSubMesh( shape ));
  }
  else {
    for ( TopoDS_Iterator it( shape ); it.More(); it.Next() )
      rootSM.push_back( mesh.GetSubMesh( it.Value() ));
  }

  // add subshapes of empty submeshes
  list< SMESH_subMesh* >::iterator rootIt = rootSM.begin(), rootEnd = rootSM.end();
  for ( ; rootIt != rootEnd; ++rootIt ) {
    SMESH_subMesh * root = *rootIt;
    SMESH_subMeshIteratorPtr smIt = root->getDependsOnIterator(/*includeSelf=*/true,
                                                               /*complexShapeFirst=*/true);
    // to find a right orientation of subshapes (PAL20462)
    TopTools_IndexedMapOfShape subShapes;
    TopExp::MapShapes(root->GetSubShape(), subShapes);
    while ( smIt->more() )
    {
      SMESH_subMesh* sm = smIt->next();
      TopoDS_Shape shape = sm->GetSubShape();
      if ( intern && intern->isShapeToPrecompute( shape ))
        continue;
      if ( !meshedSM || sm->IsEmpty() )
      {
        if ( shape.ShapeType() != TopAbs_VERTEX )
          shape = subShapes( subShapes.FindIndex( shape ));// shape -> index -> oriented shape
        if ( shape.Orientation() >= TopAbs_INTERNAL )
          shape.Orientation( TopAbs_FORWARD ); // isuue 0020676
        switch ( shape.ShapeType() ) {
        case TopAbs_FACE  : occgeo.fmap.Add( shape ); break;
        case TopAbs_EDGE  : occgeo.emap.Add( shape ); break;
        case TopAbs_VERTEX: occgeo.vmap.Add( shape ); break;
        case TopAbs_SOLID :occgeo.somap.Add( shape ); break;
        default:;
        }
      }
      // collect submeshes of meshed shapes
      else if (meshedSM)
      {
        const int dim = SMESH_Gen::GetShapeDim( shape );
        meshedSM[ dim ].push_back( sm );
      }
    }
  }
  occgeo.facemeshstatus.SetSize (occgeo.fmap.Extent());
  occgeo.facemeshstatus = 0;
  occgeo.face_maxh_modified.SetSize(occgeo.fmap.Extent());
  occgeo.face_maxh_modified = 0;
  occgeo.face_maxh.SetSize(occgeo.fmap.Extent());
  occgeo.face_maxh = netgen::mparam.maxh;
}

//================================================================================
/*!
 * \brief Return a default min size value suitable for the given geometry.
 */
//================================================================================

double NETGENPlugin_Mesher::GetDefaultMinSize(const TopoDS_Shape& geom,
                                              const double        maxSize)
{
  updateTriangulation( geom );

  TopLoc_Location loc;
  int i1, i2, i3;
  const int* pi[4] = { &i1, &i2, &i3, &i1 };
  double minh = 1e100;
  Bnd_B3d bb;
  TopExp_Explorer fExp( geom, TopAbs_FACE );
  for ( ; fExp.More(); fExp.Next() )
  {
    Handle(Poly_Triangulation) triangulation =
      BRep_Tool::Triangulation ( TopoDS::Face( fExp.Current() ), loc);
    if ( triangulation.IsNull() ) continue;
    const double fTol = BRep_Tool::Tolerance( TopoDS::Face( fExp.Current() ));
    const TColgp_Array1OfPnt&   points = triangulation->Nodes();
    const Poly_Array1OfTriangle& trias = triangulation->Triangles();
    for ( int iT = trias.Lower(); iT <= trias.Upper(); ++iT )
    {
      trias(iT).Get( i1, i2, i3 );
      for ( int j = 0; j < 3; ++j )
      {
        double dist2 = points(*pi[j]).SquareDistance( points( *pi[j+1] ));
        if ( dist2 < minh && fTol*fTol < dist2 )
          minh = dist2;
        bb.Add( points(*pi[j]));
      }
    }
  }
  if ( minh > 0.25 * bb.SquareExtent() ) // simple geometry, rough triangulation
  {
    minh = 1e-3 * sqrt( bb.SquareExtent());
    //cout << "BND BOX minh = " <<minh << std::endl;
  }
  else
  {
    minh = 3 * sqrt( minh ); // triangulation for visualization is rather fine
    //cout << "TRIANGULATION minh = " <<minh << std::endl;
  }
  if ( minh > 0.5 * maxSize )
    minh = maxSize / 3.;

  return minh;
}

//================================================================================
/*!
 * \brief Restrict size of elements at a given point
 */
//================================================================================

void NETGENPlugin_Mesher::RestrictLocalSize(netgen::Mesh& ngMesh,
                                            const gp_XYZ& p,
                                            double        size,
                                            const bool    overrideMinH)
{
  if ( size <= std::numeric_limits<double>::min() )
    return;
  if ( netgen::mparam.minh > size )
  {
    if ( overrideMinH )
    {
      ngMesh.SetMinimalH( size );
      netgen::mparam.minh = size;
    }
    else
    {
      size = netgen::mparam.minh;
    }
  }
  netgen::Point3d pi(p.X(), p.Y(), p.Z());
  ngMesh.RestrictLocalH( pi, size );
}

//================================================================================
/*!
 * \brief fill ngMesh with nodes and elements of computed submeshes
 */
//================================================================================

bool NETGENPlugin_Mesher::FillNgMesh(netgen::OCCGeometry&           occgeom,
                                     netgen::Mesh&                  ngMesh,
                                     vector<const SMDS_MeshNode*>&  nodeVec,
                                     const list< SMESH_subMesh* > & meshedSM,
                                     SMESH_MesherHelper*            quadHelper,
                                     SMESH_ProxyMesh::Ptr           proxyMesh)
{
  TNode2IdMap nodeNgIdMap;
  for ( int i = 1; i < nodeVec.size(); ++i )
    nodeNgIdMap.insert( make_pair( nodeVec[i], i ));

  TopTools_MapOfShape visitedShapes;
  map< SMESH_subMesh*, set< int > > visitedEdgeSM2Faces;
  set< SMESH_subMesh* > computedSM( meshedSM.begin(), meshedSM.end() );

  SMESH_MesherHelper helper (*_mesh);

  int faceNgID = ngMesh.GetNFD();

  list< SMESH_subMesh* >::const_iterator smIt, smEnd = meshedSM.end();
  for ( smIt = meshedSM.begin(); smIt != smEnd; ++smIt )
  {
    SMESH_subMesh* sm = *smIt;
    if ( !visitedShapes.Add( sm->GetSubShape() ))
      continue;

    const SMESHDS_SubMesh * smDS = sm->GetSubMeshDS();
    if ( !smDS ) continue;

    switch ( sm->GetSubShape().ShapeType() )
    {
    case TopAbs_EDGE: { // EDGE
      // ----------------------
      TopoDS_Edge geomEdge  = TopoDS::Edge( sm->GetSubShape() );
      if ( geomEdge.Orientation() >= TopAbs_INTERNAL )
        geomEdge.Orientation( TopAbs_FORWARD ); // issue 0020676

      // Add ng segments for each not meshed FACE the EDGE bounds
      PShapeIteratorPtr fIt = helper.GetAncestors( geomEdge, *sm->GetFather(), TopAbs_FACE );
      while ( const TopoDS_Shape * anc = fIt->next() )
      {
        faceNgID = occgeom.fmap.FindIndex( *anc );
        if ( faceNgID < 1 )
          continue; // meshed face

        int faceSMDSId = helper.GetMeshDS()->ShapeToIndex( *anc );
        if ( visitedEdgeSM2Faces[ sm ].count( faceSMDSId ))
          continue; // already treated EDGE

        TopoDS_Face face = TopoDS::Face( occgeom.fmap( faceNgID ));
        if ( face.Orientation() >= TopAbs_INTERNAL )
          face.Orientation( TopAbs_FORWARD ); // issue 0020676

        // get all meshed EDGEs of the FACE connected to geomEdge (issue 0021140)
        helper.SetSubShape( face );
        list< TopoDS_Edge > edges = getConnectedEdges( geomEdge, face, computedSM, helper,
                                                       visitedEdgeSM2Faces );
        if ( edges.empty() )
          continue; // wrong ancestor?

        // find out orientation of <edges> within <face>
        TopoDS_Edge eNotSeam = edges.front();
        if ( helper.HasSeam() )
        {
          list< TopoDS_Edge >::iterator eIt = edges.begin();
          while ( helper.IsRealSeam( *eIt )) ++eIt;
          if ( eIt != edges.end() )
            eNotSeam = *eIt;
        }
        TopAbs_Orientation fOri = helper.GetSubShapeOri( face, eNotSeam );
        bool isForwad = ( fOri == eNotSeam.Orientation() || fOri >= TopAbs_INTERNAL );

        // get all nodes from connected <edges>
        const bool isQuad = smDS->IsQuadratic();
        StdMeshers_FaceSide fSide( face, edges, _mesh, isForwad, isQuad );
        const vector<UVPtStruct>& points = fSide.GetUVPtStruct();
        if ( points.empty() )
          return false; // invalid node params?
        int i, nbSeg = fSide.NbSegments();

        // remember EDGEs of fSide to treat only once
        for ( int iE = 0; iE < fSide.NbEdges(); ++iE )
          visitedEdgeSM2Faces[ helper.GetMesh()->GetSubMesh( fSide.Edge(iE )) ].insert(faceSMDSId);

        double otherSeamParam = 0;
        bool isSeam = false;

        // add segments

        int prevNgId = ngNodeId( points[0].node, ngMesh, nodeNgIdMap );

        for ( i = 0; i < nbSeg; ++i )
        {
          const UVPtStruct& p1 = points[ i ];
          const UVPtStruct& p2 = points[ i+1 ];

          if ( p1.node->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX ) //an EDGE begins
          {
            isSeam = false;
            if ( helper.IsRealSeam( p1.node->getshapeId() ))
            {
              TopoDS_Edge e = fSide.Edge( fSide.EdgeIndex( 0.5 * ( p1.normParam + p2.normParam )));
              isSeam = helper.IsRealSeam( e );
              if ( isSeam )
              {
                otherSeamParam = helper.GetOtherParam( helper.GetPeriodicIndex() & 1 ? p2.u : p2.v );
              }
            }
          }
          netgen::Segment seg;
          // ng node ids
          seg[0] = prevNgId;
          seg[1] = prevNgId = ngNodeId( p2.node, ngMesh, nodeNgIdMap );
          // node param on curve
          seg.epgeominfo[ 0 ].dist = p1.param;
          seg.epgeominfo[ 1 ].dist = p2.param;
          // uv on face
          seg.epgeominfo[ 0 ].u = p1.u;
          seg.epgeominfo[ 0 ].v = p1.v;
          seg.epgeominfo[ 1 ].u = p2.u;
          seg.epgeominfo[ 1 ].v = p2.v;

          //geomEdge = fSide.Edge( fSide.EdgeIndex( 0.5 * ( p1.normParam + p2.normParam )));
          //seg.epgeominfo[ 0 ].edgenr = seg.epgeominfo[ 1 ].edgenr = occgeom.emap.FindIndex( geomEdge );

          //seg.epgeominfo[ iEnd ].edgenr = edgeID; //  = geom.emap.FindIndex(edge);
          seg.si = faceNgID;                   // = geom.fmap.FindIndex (face);
          seg.edgenr = ngMesh.GetNSeg() + 1; // segment id
          ngMesh.AddSegment (seg);

          SMESH_TNodeXYZ np1( p1.node ), np2( p2.node );
          RestrictLocalSize( ngMesh, 0.5*(np1+np2), (np1-np2).Modulus() );

#ifdef DUMP_SEGMENTS
          cout << "Segment: " << seg.edgenr << " on SMESH face " << helper.GetMeshDS()->ShapeToIndex( face ) << std::endl
               << "\tface index: " << seg.si << std::endl
               << "\tp1: " << seg[0] << std::endl
               << "\tp2: " << seg[1] << std::endl
               << "\tp0 param: " << seg.epgeominfo[ 0 ].dist << std::endl
               << "\tp0 uv: " << seg.epgeominfo[ 0 ].u <<", "<< seg.epgeominfo[ 0 ].v << std::endl
            //<< "\tp0 edge: " << seg.epgeominfo[ 0 ].edgenr << std::endl
               << "\tp1 param: " << seg.epgeominfo[ 1 ].dist << std::endl
               << "\tp1 uv: " << seg.epgeominfo[ 1 ].u <<", "<< seg.epgeominfo[ 1 ].v << std::endl;
            //<< "\tp1 edge: " << seg.epgeominfo[ 1 ].edgenr << std::endl;
#endif
          if ( isSeam )
          {
            if ( helper.GetPeriodicIndex() & 1 ) {
              seg.epgeominfo[ 0 ].u = otherSeamParam;
              seg.epgeominfo[ 1 ].u = otherSeamParam;
              swap (seg.epgeominfo[0].v, seg.epgeominfo[1].v);
            } else {
              seg.epgeominfo[ 0 ].v = otherSeamParam;
              seg.epgeominfo[ 1 ].v = otherSeamParam;
              swap (seg.epgeominfo[0].u, seg.epgeominfo[1].u);
            }
            swap (seg[0], seg[1]);
            swap (seg.epgeominfo[0].dist, seg.epgeominfo[1].dist);
            seg.edgenr = ngMesh.GetNSeg() + 1; // segment id
            ngMesh.AddSegment (seg);
#ifdef DUMP_SEGMENTS
            cout << "Segment: " << seg.edgenr << std::endl
                 << "\t is SEAM (reverse) of the previous. "
                 << " Other " << (helper.GetPeriodicIndex() & 1 ? "U" : "V")
                 << " = " << otherSeamParam << std::endl;
#endif
          }
          else if ( fOri == TopAbs_INTERNAL )
          {
            swap (seg[0], seg[1]);
            swap( seg.epgeominfo[0], seg.epgeominfo[1] );
            seg.edgenr = ngMesh.GetNSeg() + 1; // segment id
            ngMesh.AddSegment (seg);
#ifdef DUMP_SEGMENTS
            cout << "Segment: " << seg.edgenr << std::endl << "\t is REVERSE of the previous" << std::endl;
#endif
          }
        }
      } // loop on geomEdge ancestors

      if ( quadHelper ) // remember medium nodes of sub-meshes
      {
        SMDS_ElemIteratorPtr edges = smDS->GetElements();
        while ( edges->more() )
        {
          const SMDS_MeshElement* e = edges->next();
          if ( !quadHelper->AddTLinks( static_cast< const SMDS_MeshEdge*>( e )))
            break;
        }
      }

      break;
    } // case TopAbs_EDGE

    case TopAbs_FACE: { // FACE
      // ----------------------
      const TopoDS_Face& geomFace  = TopoDS::Face( sm->GetSubShape() );
      helper.SetSubShape( geomFace );
      bool isInternalFace = ( geomFace.Orientation() == TopAbs_INTERNAL );

      // Find solids the geomFace bounds
      int solidID1 = 0, solidID2 = 0;
      StdMeshers_QuadToTriaAdaptor* quadAdaptor =
        dynamic_cast<StdMeshers_QuadToTriaAdaptor*>( proxyMesh.get() );
      if ( quadAdaptor )
      {
        solidID1 = occgeom.somap.FindIndex( quadAdaptor->GetShape() );
      }
      else
      {  
        PShapeIteratorPtr solidIt = helper.GetAncestors( geomFace, *sm->GetFather(), TopAbs_SOLID);
        while ( const TopoDS_Shape * solid = solidIt->next() )
        {
          int id = occgeom.somap.FindIndex ( *solid );
          if ( solidID1 && id != solidID1 ) solidID2 = id;
          else                              solidID1 = id;
        }
      }
      // Add ng face descriptors of meshed faces
      faceNgID++;
      ngMesh.AddFaceDescriptor (netgen::FaceDescriptor(faceNgID, solidID1, solidID2, 0));

      // if second oreder is required, even already meshed faces must be passed to NETGEN
      int fID = occgeom.fmap.Add( geomFace );
      while ( fID < faceNgID ) // geomFace is already in occgeom.fmap, add a copy
        fID = occgeom.fmap.Add( BRepBuilderAPI_Copy( geomFace, /*copyGeom=*/false ));
      // Problem with the second order in a quadrangular mesh remains.
      // 1) All quadrangles generated by NETGEN are moved to an inexistent face
      //    by FillSMesh() (find "AddFaceDescriptor")
      // 2) Temporary triangles generated by StdMeshers_QuadToTriaAdaptor
      //    are on faces where quadrangles were.
      // Due to these 2 points, wrong geom faces are used while conversion to qudratic
      // of the mentioned above quadrangles and triangles

      // Orient the face correctly in solidID1 (issue 0020206)
      bool reverse = false;
      if ( solidID1 ) {
        TopoDS_Shape solid = occgeom.somap( solidID1 );
        TopAbs_Orientation faceOriInSolid = helper.GetSubShapeOri( solid, geomFace );
        if ( faceOriInSolid >= 0 )
          reverse =
            helper.IsReversedSubMesh( TopoDS::Face( geomFace.Oriented( faceOriInSolid )));
      }

      // Add surface elements

      netgen::Element2d tri(3);
      tri.SetIndex ( faceNgID );
      SMESH_TNodeXYZ xyz[3];

#ifdef DUMP_TRIANGLES
      cout << "SMESH face " << helper.GetMeshDS()->ShapeToIndex( geomFace )
           << " internal="<<isInternalFace << std::endl;
#endif
      if ( proxyMesh )
        smDS = proxyMesh->GetSubMesh( geomFace );

      SMDS_ElemIteratorPtr faces = smDS->GetElements();
      while ( faces->more() )
      {
        const SMDS_MeshElement* f = faces->next();
        if ( f->NbNodes() % 3 != 0 ) // not triangle
        {
          PShapeIteratorPtr solidIt=helper.GetAncestors(geomFace,*sm->GetFather(),TopAbs_SOLID);
          if ( const TopoDS_Shape * solid = solidIt->next() )
            sm = _mesh->GetSubMesh( *solid );
          SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
          smError.reset( new SMESH_ComputeError(COMPERR_BAD_INPUT_MESH,"Not triangle sub-mesh"));
          smError->myBadElements.push_back( f );
          return false;
        }

        for ( int i = 0; i < 3; ++i )
        {
          const SMDS_MeshNode* node = f->GetNode( i ), * inFaceNode=0;
          xyz[i].Set( node );

          // get node UV on face
          int shapeID = node->getshapeId();
          if ( helper.IsSeamShape( shapeID ))
          {
            if ( helper.IsSeamShape( f->GetNodeWrap( i+1 )->getshapeId() ))
              inFaceNode = f->GetNodeWrap( i-1 );
            else
              inFaceNode = f->GetNodeWrap( i+1 );
          }
          gp_XY uv = helper.GetNodeUV( geomFace, node, inFaceNode );

          int ind = reverse ? 3-i : i+1;
          tri.GeomInfoPi(ind).u = uv.X();
          tri.GeomInfoPi(ind).v = uv.Y();
          tri.PNum      (ind) = ngNodeId( node, ngMesh, nodeNgIdMap );
        }

        // pass a triangle size to NG size-map
        double size = ( ( xyz[0] - xyz[1] ).Modulus() +
                        ( xyz[1] - xyz[2] ).Modulus() +
                        ( xyz[2] - xyz[0] ).Modulus() ) / 3;
        gp_XYZ gc = ( xyz[0] + xyz[1] + xyz[2] ) / 3;
        RestrictLocalSize( ngMesh, gc, size, /*overrideMinH=*/false );

        ngMesh.AddSurfaceElement (tri);
#ifdef DUMP_TRIANGLES
        cout << tri << std::endl;
#endif

        if ( isInternalFace )
        {
          swap( tri[1], tri[2] );
          ngMesh.AddSurfaceElement (tri);
#ifdef DUMP_TRIANGLES
          cout << tri << std::endl;
#endif
        }
      }

      if ( quadHelper ) // remember medium nodes of sub-meshes
      {
        SMDS_ElemIteratorPtr faces = smDS->GetElements();
        while ( faces->more() )
        {
          const SMDS_MeshElement* f = faces->next();
          if ( !quadHelper->AddTLinks( static_cast< const SMDS_MeshFace*>( f )))
            break;
        }
      }

      break;
    } // case TopAbs_FACE

    case TopAbs_VERTEX: { // VERTEX
      // --------------------------
      // issue 0021405. Add node only if a VERTEX is shared by a not meshed EDGE,
      // else netgen removes a free node and nodeVector becomes invalid
      PShapeIteratorPtr ansIt = helper.GetAncestors( sm->GetSubShape(),
                                                     *sm->GetFather(),
                                                     TopAbs_EDGE );
      bool toAdd = false;
      while ( const TopoDS_Shape* e = ansIt->next() )
      {
        SMESH_subMesh* eSub = helper.GetMesh()->GetSubMesh( *e );
        if (( toAdd = ( eSub->IsEmpty() && !SMESH_Algo::isDegenerated( TopoDS::Edge( *e )))))
          break;
      }
      if ( toAdd )
      {
        SMDS_NodeIteratorPtr nodeIt = smDS->GetNodes();
        if ( nodeIt->more() )
          ngNodeId( nodeIt->next(), ngMesh, nodeNgIdMap );
      }
      break;
    }
    default:;
    } // switch
  } // loop on submeshes

  // fill nodeVec
  nodeVec.resize( ngMesh.GetNP() + 1 );
  TNode2IdMap::iterator node_NgId, nodeNgIdEnd = nodeNgIdMap.end();
  for ( node_NgId = nodeNgIdMap.begin(); node_NgId != nodeNgIdEnd; ++node_NgId)
    nodeVec[ node_NgId->second ] = node_NgId->first;

  return true;
}

//================================================================================
/*!
 * \brief Duplicate mesh faces on internal geom faces
 */
//================================================================================

void NETGENPlugin_Mesher::FixIntFaces(const netgen::OCCGeometry& occgeom,
                                      netgen::Mesh&              ngMesh,
                                      NETGENPlugin_Internals&    internalShapes)
{
  SMESHDS_Mesh* meshDS = internalShapes.getMesh().GetMeshDS();
  
  // find ng indices of internal faces
  set<int> ngFaceIds;
  for ( int ngFaceID = 1; ngFaceID <= occgeom.fmap.Extent(); ++ngFaceID )
  {
    int smeshID = meshDS->ShapeToIndex( occgeom.fmap( ngFaceID ));
    if ( internalShapes.isInternalShape( smeshID ))
      ngFaceIds.insert( ngFaceID );
  }
  if ( !ngFaceIds.empty() )
  {
    // duplicate faces
    int i, nbFaces = ngMesh.GetNSE();
    for (int i = 1; i <= nbFaces; ++i)
    {
      netgen::Element2d elem = ngMesh.SurfaceElement(i);
      if ( ngFaceIds.count( elem.GetIndex() ))
      {
        swap( elem[1], elem[2] );
        ngMesh.AddSurfaceElement (elem);
      }
    }
  }
}

//================================================================================
/*!
 * \brief Tries to heal the mesh on a FACE. The FACE is supposed to be partially
 *        meshed due to NETGEN failure
 *  \param [in] occgeom - geometry
 *  \param [in,out] ngMesh - the mesh to fix
 *  \param [inout] faceID - ID of the FACE to fix the mesh on
 *  \return bool - is mesh is or becomes OK
 */
//================================================================================

bool NETGENPlugin_Mesher::FixFaceMesh(const netgen::OCCGeometry& occgeom,
                                      netgen::Mesh&              ngMesh,
                                      const int                  faceID)
{
  // we address a case where the FACE is almost fully meshed except small holes
  // of usually triangular shape at FACE boundary (IPAL52861)

  // The case appeared to be not simple: holes only look triangular but
  // indeed are a self intersecting polygon. A reason of the bug was in coincident
  // NG points on a seam edge. But the code below is very nice, leave it for
  // another case.
  return false;


  if ( occgeom.fmap.Extent() < faceID )
    return false;
  const TopoDS_Face& face = TopoDS::Face( occgeom.fmap( faceID ));

  // find free links on the FACE
  NCollection_Map<Link> linkMap;
  for ( int iF = 1; iF <= ngMesh.GetNSE(); ++iF )
  {
    const netgen::Element2d& elem = ngMesh.SurfaceElement(iF);
    if ( faceID != elem.GetIndex() )
      continue;
    int n0 = elem[ elem.GetNP() - 1 ];
    for ( int i = 0; i < elem.GetNP(); ++i )
    {
      int n1 = elem[i];
      Link link( n0, n1 );
      if ( !linkMap.Add( link ))
        linkMap.Remove( link );
      n0 = n1;
    }
  }
  // add/remove boundary links
  for ( int iSeg = 1; iSeg <= ngMesh.GetNSeg(); ++iSeg )
  {
    const netgen::Segment& seg = ngMesh.LineSegment( iSeg );
    if ( seg.si != faceID ) // !edgeIDs.Contains( seg.edgenr ))
      continue;
    Link link( seg[1], seg[0] ); // reverse!!!
    if ( !linkMap.Add( link ))
      linkMap.Remove( link );
  }
  if ( linkMap.IsEmpty() )
    return true;
  if ( linkMap.Extent() < 3 )
    return false;

  // make triangles of the links

  netgen::Element2d tri(3);
  tri.SetIndex ( faceID );

  NCollection_Map<Link>::Iterator linkIt( linkMap );
  Link link1 = linkIt.Value();
  // look for a link connected to link1
  NCollection_Map<Link>::Iterator linkIt2 = linkIt;
  for ( linkIt2.Next(); linkIt2.More(); linkIt2.Next() )
  {
    const Link& link2 = linkIt2.Value();
    if ( link2.IsConnected( link1 ))
    {
      // look for a link connected to both link1 and link2
      NCollection_Map<Link>::Iterator linkIt3 = linkIt2;
      for ( linkIt3.Next(); linkIt3.More(); linkIt3.Next() )
      {
        const Link& link3 = linkIt3.Value();
        if ( link3.IsConnected( link1 ) &&
             link3.IsConnected( link2 ) )
        {
          // add a triangle
          tri[0] = link1.n2;
          tri[1] = link1.n1;
          tri[2] = ( link2.Contains( link1.n1 ) ? link2.n1 : link3.n1 );
          if ( tri[0] == tri[2] || tri[1] == tri[2] )
            return false;
          ngMesh.AddSurfaceElement( tri );

          // prepare for the next tria search
          if ( linkMap.Extent() == 3 )
            return true;
          linkMap.Remove( link3 );
          linkMap.Remove( link2 );
          linkIt.Next();
          linkMap.Remove( link1 );
          link1 = linkIt.Value();
          linkIt2 = linkIt;
          break;
        }
      }
    }
  }
  return false;

} // FixFaceMesh()

namespace
{
  //================================================================================
  // define gp_XY_Subtracted pointer to function calling gp_XY::Subtracted(gp_XY)
  gp_XY_FunPtr(Subtracted);
  //gp_XY_FunPtr(Added);

  //================================================================================
  /*!
   * \brief Evaluate distance between two 2d points along the surface
   */
  //================================================================================

  double evalDist( const gp_XY&                uv1,
                   const gp_XY&                uv2,
                   const Handle(Geom_Surface)& surf,
                   const int                   stopHandler=-1)
  {
    if ( stopHandler > 0 ) // continue recursion
    {
      gp_XY mid = SMESH_MesherHelper::GetMiddleUV( surf, uv1, uv2 );
      return evalDist( uv1,mid, surf, stopHandler-1 ) + evalDist( mid,uv2, surf, stopHandler-1 );
    }
    double dist3D = surf->Value( uv1.X(), uv1.Y() ).Distance( surf->Value( uv2.X(), uv2.Y() ));
    if ( stopHandler == 0 ) // stop recursion
      return dist3D;
    
    // start recursion if necessary
    double dist2D = SMESH_MesherHelper::ApplyIn2D(surf, uv1, uv2, gp_XY_Subtracted, 0).Modulus();
    if ( fabs( dist3D - dist2D ) < dist2D * 1e-10 )
      return dist3D; // equal parametrization of a planar surface

    return evalDist( uv1, uv2, surf, 3 ); // start recursion
  }

  //================================================================================
  /*!
   * \brief Data of vertex internal in geom face
   */
  //================================================================================

  struct TIntVData
  {
    gp_XY uv;        //!< UV in face parametric space
    int   ngId;      //!< ng id of corrsponding node
    gp_XY uvClose;   //!< UV of closest boundary node
    int   ngIdClose; //!< ng id of closest boundary node
  };

  //================================================================================
  /*!
   * \brief Data of vertex internal in solid
   */
  //================================================================================

  struct TIntVSoData
  {
    int   ngId;      //!< ng id of corresponding node
    int   ngIdClose; //!< ng id of closest 2d mesh element
    int   ngIdCloseN; //!< ng id of closest node of the closest 2d mesh element
  };

  inline double dist2(const netgen::MeshPoint& p1, const netgen::MeshPoint& p2)
  {
    return gp_Pnt( NGPOINT_COORDS(p1)).SquareDistance( gp_Pnt( NGPOINT_COORDS(p2)));
  }
}

//================================================================================
/*!
 * \brief Make netgen take internal vertices in faces into account by adding
 *        segments including internal vertices
 *
 * This function works in supposition that 1D mesh is already computed in ngMesh
 */
//================================================================================

void NETGENPlugin_Mesher::AddIntVerticesInFaces(const netgen::OCCGeometry&     occgeom,
                                                netgen::Mesh&                  ngMesh,
                                                vector<const SMDS_MeshNode*>&  nodeVec,
                                                NETGENPlugin_Internals&        internalShapes)
{
  if ( nodeVec.size() < ngMesh.GetNP() )
    nodeVec.resize( ngMesh.GetNP(), 0 );

  SMESHDS_Mesh* meshDS = internalShapes.getMesh().GetMeshDS();
  SMESH_MesherHelper helper( internalShapes.getMesh() );

  const map<int,list<int> >& face2Vert = internalShapes.getFacesWithVertices();
  map<int,list<int> >::const_iterator f2v = face2Vert.begin();
  for ( ; f2v != face2Vert.end(); ++f2v )
  {
    const TopoDS_Face& face = TopoDS::Face( meshDS->IndexToShape( f2v->first ));
    if ( face.IsNull() ) continue;
    int faceNgID = occgeom.fmap.FindIndex (face);
    if ( faceNgID < 0 ) continue;

    TopLoc_Location loc;
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face,loc);

    helper.SetSubShape( face );
    helper.SetElementsOnShape( true );

    // Get data of internal vertices and add them to ngMesh

    multimap< double, TIntVData > dist2VData; // sort vertices by distance from boundary nodes

    int i, nbSegInit = ngMesh.GetNSeg();

    // boundary characteristics
    double totSegLen2D = 0;
    int totNbSeg = 0;

    const list<int>& iVertices = f2v->second;
    list<int>::const_iterator iv = iVertices.begin();
    for ( int nbV = 0; iv != iVertices.end(); ++iv, nbV++ )
    {
      TIntVData vData;
      // get node on vertex
      const TopoDS_Vertex V = TopoDS::Vertex( meshDS->IndexToShape( *iv ));
      const SMDS_MeshNode * nV = SMESH_Algo::VertexNode( V, meshDS );
      if ( !nV )
      {
        SMESH_subMesh* sm = helper.GetMesh()->GetSubMesh( V );
        sm->ComputeStateEngine( SMESH_subMesh::COMPUTE );
        nV = SMESH_Algo::VertexNode( V, meshDS );
        if ( !nV ) continue;
      }
      // add ng node
      netgen::MeshPoint mp( netgen::Point<3> (nV->X(), nV->Y(), nV->Z()) );
      ngMesh.AddPoint ( mp, 1, netgen::EDGEPOINT );
      vData.ngId = ngMesh.GetNP();
      nodeVec.push_back( nV );

      // get node UV
      bool uvOK = true;
      vData.uv = helper.GetNodeUV( face, nV, 0, &uvOK );
      if ( !uvOK ) helper.CheckNodeUV( face, nV, vData.uv, BRep_Tool::Tolerance(V),/*force=*/1);

      // loop on all segments of the face to find the node closest to vertex and to count
      // average segment 2d length
      double closeDist2 = numeric_limits<double>::max(), dist2;
      int ngIdLast = 0;
      for (i = 1; i <= ngMesh.GetNSeg(); ++i)
      {
        netgen::Segment & seg = ngMesh.LineSegment(i);
        if ( seg.si != faceNgID ) continue;
        gp_XY uv[2];
        for ( int iEnd = 0; iEnd < 2; ++iEnd)
        {
          uv[iEnd].SetCoord( seg.epgeominfo[iEnd].u, seg.epgeominfo[iEnd].v );
          if ( ngIdLast == seg[ iEnd ] ) continue;
          dist2 = helper.ApplyIn2D(surf, uv[iEnd], vData.uv, gp_XY_Subtracted,0).SquareModulus();
          if ( dist2 < closeDist2 )
            vData.ngIdClose = seg[ iEnd ], vData.uvClose = uv[iEnd], closeDist2 = dist2;
          ngIdLast = seg[ iEnd ];
        }
        if ( !nbV )
        {
          totSegLen2D += helper.ApplyIn2D(surf, uv[0], uv[1], gp_XY_Subtracted, false).Modulus();
          totNbSeg++;
        }
      }
      dist2VData.insert( make_pair( closeDist2, vData ));
    }

    if ( totNbSeg == 0 ) break;
    double avgSegLen2d = totSegLen2D / totNbSeg;

    // Loop on vertices to add segments

    multimap< double, TIntVData >::iterator dist_vData = dist2VData.begin();
    for ( ; dist_vData != dist2VData.end(); ++dist_vData )
    {
      double closeDist2 = dist_vData->first, dist2;
      TIntVData & vData = dist_vData->second;

      // try to find more close node among segments added for internal vertices
      for (i = nbSegInit+1; i <= ngMesh.GetNSeg(); ++i)
      {
        netgen::Segment & seg = ngMesh.LineSegment(i);
        if ( seg.si != faceNgID ) continue;
        gp_XY uv[2];
        for ( int iEnd = 0; iEnd < 2; ++iEnd)
        {
          uv[iEnd].SetCoord( seg.epgeominfo[iEnd].u, seg.epgeominfo[iEnd].v );
          dist2 = helper.ApplyIn2D(surf, uv[iEnd], vData.uv, gp_XY_Subtracted,0).SquareModulus();
          if ( dist2 < closeDist2 )
            vData.ngIdClose = seg[ iEnd ], vData.uvClose = uv[iEnd], closeDist2 = dist2;
        }
      }
      // decide whether to use the closest node as the second end of segment or to
      // create a new point
      int segEnd1 = vData.ngId;
      int segEnd2 = vData.ngIdClose; // to use closest node
      gp_XY uvV = vData.uv, uvP = vData.uvClose;
      double segLenHint  = ngMesh.GetH( ngMesh.Point( vData.ngId ));
      double nodeDist2D  = sqrt( closeDist2 );
      double nodeDist3D  = evalDist( vData.uv, vData.uvClose, surf );
      bool avgLenOK  = ( avgSegLen2d < 0.75 * nodeDist2D );
      bool hintLenOK = ( segLenHint  < 0.75 * nodeDist3D );
      //cout << "uvV " << uvV.X() <<","<<uvV.Y() << " ";
      if ( hintLenOK || avgLenOK )
      {
        // create a point between the closest node and V

        // how far from V
        double r = min( 0.5, ( hintLenOK ? segLenHint/nodeDist3D : avgSegLen2d/nodeDist2D ));
        // direction from V to closet node in 2D
        gp_Dir2d v2n( helper.ApplyIn2D(surf, uvP, uvV, gp_XY_Subtracted, false ));
        // new point
        uvP = vData.uv + r * nodeDist2D * v2n.XY();
        gp_Pnt P = surf->Value( uvP.X(), uvP.Y() ).Transformed( loc );

        netgen::MeshPoint mp( netgen::Point<3> (P.X(), P.Y(), P.Z()));
        ngMesh.AddPoint ( mp, 1, netgen::EDGEPOINT );
        segEnd2 = ngMesh.GetNP();
        //cout << "Middle " << r << " uv " << uvP.X() << "," << uvP.Y() << "( " << ngMesh.Point(segEnd2).X()<<","<<ngMesh.Point(segEnd2).Y()<<","<<ngMesh.Point(segEnd2).Z()<<" )"<< std::endl;
        SMDS_MeshNode * nP = helper.AddNode(P.X(), P.Y(), P.Z());
        nodeVec.push_back( nP );
      }
      //else cout << "at Node " << " uv " << uvP.X() << "," << uvP.Y() << std::endl;

      // Add the segment
      netgen::Segment seg;

      if ( segEnd1 > segEnd2 ) swap( segEnd1, segEnd2 ), swap( uvV, uvP );
      seg[0] = segEnd1;  // ng node id
      seg[1] = segEnd2;  // ng node id
      seg.edgenr = ngMesh.GetNSeg() + 1;// segment id
      seg.si = faceNgID;

      seg.epgeominfo[ 0 ].dist = 0; // param on curve
      seg.epgeominfo[ 0 ].u    = uvV.X();
      seg.epgeominfo[ 0 ].v    = uvV.Y();
      seg.epgeominfo[ 1 ].dist = 1; // param on curve
      seg.epgeominfo[ 1 ].u    = uvP.X();
      seg.epgeominfo[ 1 ].v    = uvP.Y();

//       seg.epgeominfo[ 0 ].edgenr = 10; //  = geom.emap.FindIndex(edge);
//       seg.epgeominfo[ 1 ].edgenr = 10; //  = geom.emap.FindIndex(edge);

      ngMesh.AddSegment (seg);

      // add reverse segment
      swap (seg[0], seg[1]);
      swap( seg.epgeominfo[0], seg.epgeominfo[1] );
      seg.edgenr = ngMesh.GetNSeg() + 1; // segment id
      ngMesh.AddSegment (seg);
    }

  }
}

//================================================================================
/*!
 * \brief Make netgen take internal vertices in solids into account by adding
 *        faces including internal vertices
 *
 * This function works in supposition that 2D mesh is already computed in ngMesh
 */
//================================================================================

void NETGENPlugin_Mesher::AddIntVerticesInSolids(const netgen::OCCGeometry&     occgeom,
                                                 netgen::Mesh&                  ngMesh,
                                                 vector<const SMDS_MeshNode*>&  nodeVec,
                                                 NETGENPlugin_Internals&        internalShapes)
{
#ifdef DUMP_TRIANGLES_SCRIPT
  // create a python script making a mesh containing triangles added for internal vertices
  ofstream py(DUMP_TRIANGLES_SCRIPT);
  py << "import SMESH"<< std::endl
     << "from salome.smesh import smeshBuilder"<<std::endl
     << "smesh = smeshBuilder.New(salome.myStudy)"<<std::endl
     << "m = smesh.Mesh(name='triangles')" << std::endl;
#endif
  if ( nodeVec.size() < ngMesh.GetNP() )
    nodeVec.resize( ngMesh.GetNP(), 0 );

  SMESHDS_Mesh* meshDS = internalShapes.getMesh().GetMeshDS();
  SMESH_MesherHelper helper( internalShapes.getMesh() );

  const map<int,list<int> >& so2Vert = internalShapes.getSolidsWithVertices();
  map<int,list<int> >::const_iterator s2v = so2Vert.begin();
  for ( ; s2v != so2Vert.end(); ++s2v )
  {
    const TopoDS_Shape& solid = meshDS->IndexToShape( s2v->first );
    if ( solid.IsNull() ) continue;
    int solidNgID = occgeom.somap.FindIndex (solid);
    if ( solidNgID < 0 && !occgeom.somap.IsEmpty() ) continue;

    helper.SetSubShape( solid );
    helper.SetElementsOnShape( true );

    // find ng indices of faces within the solid
    set<int> ngFaceIds;
    for (TopExp_Explorer fExp(solid, TopAbs_FACE); fExp.More(); fExp.Next() )
      ngFaceIds.insert( occgeom.fmap.FindIndex( fExp.Current() ));
    if ( ngFaceIds.size() == 1 && *ngFaceIds.begin() == 0 )
      ngFaceIds.insert( 1 );

    // Get data of internal vertices and add them to ngMesh

    multimap< double, TIntVSoData > dist2VData; // sort vertices by distance from ng faces

    int i, nbFaceInit = ngMesh.GetNSE();

    // boundary characteristics
    double totSegLen = 0;
    int totNbSeg = 0;

    const list<int>& iVertices = s2v->second;
    list<int>::const_iterator iv = iVertices.begin();
    for ( int nbV = 0; iv != iVertices.end(); ++iv, nbV++ )
    {
      TIntVSoData vData;
      const TopoDS_Vertex V = TopoDS::Vertex( meshDS->IndexToShape( *iv ));

      // get node on vertex
      const SMDS_MeshNode * nV = SMESH_Algo::VertexNode( V, meshDS );
      if ( !nV )
      {
        SMESH_subMesh* sm = helper.GetMesh()->GetSubMesh( V );
        sm->ComputeStateEngine( SMESH_subMesh::COMPUTE );
        nV = SMESH_Algo::VertexNode( V, meshDS );
        if ( !nV ) continue;
      }
      // add ng node
      netgen::MeshPoint mpV( netgen::Point<3> (nV->X(), nV->Y(), nV->Z()) );
      ngMesh.AddPoint ( mpV, 1, netgen::FIXEDPOINT );
      vData.ngId = ngMesh.GetNP();
      nodeVec.push_back( nV );

      // loop on all 2d elements to find the one closest to vertex and to count
      // average segment length
      double closeDist2 = numeric_limits<double>::max(), avgDist2;
      for (i = 1; i <= ngMesh.GetNSE(); ++i)
      {
        const netgen::Element2d& elem = ngMesh.SurfaceElement(i);
        if ( !ngFaceIds.count( elem.GetIndex() )) continue;
        avgDist2 = 0;
        multimap< double, int> dist2nID; // sort nodes of element by distance from V
        for ( int j = 0; j < elem.GetNP(); ++j)
        {
          netgen::MeshPoint mp = ngMesh.Point( elem[j] );
          double d2 = dist2( mpV, mp );
          dist2nID.insert( make_pair( d2, elem[j] ));
          avgDist2 += d2 / elem.GetNP();
          if ( !nbV )
            totNbSeg++, totSegLen+= sqrt( dist2( mp, ngMesh.Point( elem[(j+1)%elem.GetNP()])));
        }
        double dist = dist2nID.begin()->first; //avgDist2;
        if ( dist < closeDist2 )
          vData.ngIdClose= i, vData.ngIdCloseN= dist2nID.begin()->second, closeDist2= dist;
      }
      dist2VData.insert( make_pair( closeDist2, vData ));
    }

    if ( totNbSeg == 0 ) break;
    double avgSegLen = totSegLen / totNbSeg;

    // Loop on vertices to add triangles

    multimap< double, TIntVSoData >::iterator dist_vData = dist2VData.begin();
    for ( ; dist_vData != dist2VData.end(); ++dist_vData )
    {
      double closeDist2   = dist_vData->first;
      TIntVSoData & vData = dist_vData->second;

      const netgen::MeshPoint& mpV = ngMesh.Point( vData.ngId );

      // try to find more close face among ones added for internal vertices
      for (i = nbFaceInit+1; i <= ngMesh.GetNSE(); ++i)
      {
        double avgDist2 = 0;
        multimap< double, int> dist2nID;
        const netgen::Element2d& elem = ngMesh.SurfaceElement(i);
        for ( int j = 0; j < elem.GetNP(); ++j)
        {
          double d = dist2( mpV, ngMesh.Point( elem[j] ));
          dist2nID.insert( make_pair( d, elem[j] ));
          avgDist2 += d / elem.GetNP();
          if ( avgDist2 < closeDist2 )
            vData.ngIdClose= i, vData.ngIdCloseN= dist2nID.begin()->second, closeDist2= avgDist2;
        }
      }
      // sort nodes of the closest face by angle with vector from V to the closest node
      const double tol = numeric_limits<double>::min();
      map< double, int > angle2ID;
      const netgen::Element2d& closeFace = ngMesh.SurfaceElement( vData.ngIdClose );
      netgen::MeshPoint mp[2];
      mp[0] = ngMesh.Point( vData.ngIdCloseN );
      gp_XYZ p1( NGPOINT_COORDS( mp[0] ));
      gp_XYZ pV( NGPOINT_COORDS( mpV ));
      gp_Vec v2p1( pV, p1 );
      double distN1 = v2p1.Magnitude();
      if ( distN1 <= tol ) continue;
      v2p1 /= distN1;
      for ( int j = 0; j < closeFace.GetNP(); ++j)
      {
        mp[1] = ngMesh.Point( closeFace[j] );
        gp_Vec v2p( pV, gp_Pnt( NGPOINT_COORDS( mp[1] )) );
        angle2ID.insert( make_pair( v2p1.Angle( v2p ), closeFace[j]));
      }
      // get node with angle of 60 degrees or greater
      map< double, int >::iterator angle_id = angle2ID.lower_bound( 60. * M_PI / 180. );
      if ( angle_id == angle2ID.end() ) angle_id = --angle2ID.end();
      const double minAngle = 30. * M_PI / 180.;
      const double angle = angle_id->first;
      bool angleOK = ( angle > minAngle );

      // find points to create a triangle
      netgen::Element2d tri(3);
      tri.SetIndex ( 1 );
      tri[0] = vData.ngId;
      tri[1] = vData.ngIdCloseN; // to use the closest nodes
      tri[2] = angle_id->second; // to use the node with best angle

      // decide whether to use the closest node and the node with best angle or to create new ones
      for ( int isBestAngleN = 0; isBestAngleN < 2; ++isBestAngleN )
      {
        bool createNew = !angleOK, distOK = true;
        double distFromV;
        int triInd = isBestAngleN ? 2 : 1;
        mp[isBestAngleN] = ngMesh.Point( tri[triInd] );
        if ( isBestAngleN )
        {
          if ( angleOK )
          {
            double distN2 = sqrt( dist2( mpV, mp[isBestAngleN]));
            createNew = ( fabs( distN2 - distN1 ) > 0.25 * distN1 );
          }
          else if ( angle < tol )
          {
            v2p1.SetX( v2p1.X() + 1e-3 );
          }
          distFromV = distN1;
        }
        else
        {
          double segLenHint = ngMesh.GetH( ngMesh.Point( vData.ngId ));
          bool avgLenOK  = ( avgSegLen < 0.75 * distN1 );
          bool hintLenOK = ( segLenHint  < 0.75 * distN1 );
          createNew = (createNew || avgLenOK || hintLenOK );
          // we create a new node not closer than 0.5 to the closest face
          // in order not to clash with other close face
          double r = min( 0.5, ( hintLenOK ? segLenHint : avgSegLen ) / distN1 );
          distFromV = r * distN1;
        }
        if ( createNew )
        {
          // create a new point, between the node and the vertex if angleOK
          gp_XYZ p( NGPOINT_COORDS( mp[isBestAngleN] ));
          gp_Vec v2p( pV, p ); v2p.Normalize();
          if ( isBestAngleN && !angleOK )
            p = p1 + gp_Dir( v2p.XYZ() - v2p1.XYZ()).XYZ() * distN1 * 0.95;
          else
            p = pV + v2p.XYZ() * distFromV;

          if ( !isBestAngleN ) p1 = p, distN1 = distFromV;

          mp[isBestAngleN].SetPoint( netgen::Point<3> (p.X(), p.Y(), p.Z()));
          ngMesh.AddPoint ( mp[isBestAngleN], 1, netgen::SURFACEPOINT );
          tri[triInd] = ngMesh.GetNP();
          nodeVec.push_back( helper.AddNode( p.X(), p.Y(), p.Z()) );
        }
      }
      ngMesh.AddSurfaceElement (tri);
      swap( tri[1], tri[2] );
      ngMesh.AddSurfaceElement (tri);

#ifdef DUMP_TRIANGLES_SCRIPT
      py << "n1 = m.AddNode( "<< mpV(0)<<", "<< mpV(1)<<", "<< mpV(2)<<") "<< std::endl
         << "n2 = m.AddNode( "<< mp[0](0)<<", "<< mp[0](1)<<", "<< mp[0](2)<<") "<< std::endl
         << "n3 = m.AddNode( "<< mp[1](0)<<", "<< mp[1](1)<<", "<< mp[1](2)<<" )" << std::endl
         << "m.AddFace([n1,n2,n3])" << std::endl;
#endif
    } // loop on internal vertices of a solid

  } // loop on solids with internal vertices
}

//================================================================================
/*!
 * \brief Fill netgen mesh with segments of a FACE
 *  \param ngMesh - netgen mesh
 *  \param geom - container of OCCT geometry to mesh
 *  \param wires - data of nodes on FACE boundary
 *  \param helper - mesher helper holding the FACE
 *  \param nodeVec - vector of nodes in which node index == netgen ID
 *  \retval SMESH_ComputeErrorPtr - error description 
 */
//================================================================================

SMESH_ComputeErrorPtr
NETGENPlugin_Mesher::AddSegmentsToMesh(netgen::Mesh&                    ngMesh,
                                       netgen::OCCGeometry&             geom,
                                       const TSideVector&               wires,
                                       SMESH_MesherHelper&              helper,
                                       vector< const SMDS_MeshNode* > & nodeVec,
                                       const bool                       overrideMinH)
{
  // ----------------------------
  // Check wires and count nodes
  // ----------------------------
  int nbNodes = 0;
  for ( int iW = 0; iW < wires.size(); ++iW )
  {
    StdMeshers_FaceSidePtr wire = wires[ iW ];
    if ( wire->MissVertexNode() )
    {
      // Commented for issue 0020960. It worked for the case, let's wait for case where it doesn't.
      // It seems that there is no reason for this limitation
//       return TError
//         (new SMESH_ComputeError(COMPERR_BAD_INPUT_MESH, "Missing nodes on vertices"));
    }
    const vector<UVPtStruct>& uvPtVec = wire->GetUVPtStruct();
    if ( uvPtVec.size() != wire->NbPoints() )
      return SMESH_ComputeError::New(COMPERR_BAD_INPUT_MESH,
                                     SMESH_Comment("Unexpected nb of points on wire ") << iW
                                     << ": " << uvPtVec.size()<<" != "<<wire->NbPoints());
    nbNodes += wire->NbPoints();
  }
  nodeVec.reserve( nodeVec.size() + nbNodes + 1 );
  if ( nodeVec.empty() )
    nodeVec.push_back( 0 );

  // -----------------
  // Fill netgen mesh
  // -----------------

  const bool wasNgMeshEmpty = ( ngMesh.GetNP() < 1 ); /* true => this method is called by
                                                         NETGENPlugin_NETGEN_2D_ONLY */

  // map for nodes on vertices since they can be shared between wires
  // ( issue 0020676, face_int_box.brep) and nodes built by NETGEN
  map<const SMDS_MeshNode*, int > node2ngID;
  if ( !wasNgMeshEmpty ) // fill node2ngID with nodes built by NETGEN
  {
    set< int > subIDs; // ids of sub-shapes of the FACE
    for ( int iW = 0; iW < wires.size(); ++iW )
    {
      StdMeshers_FaceSidePtr wire = wires[ iW ];
      for ( int iE = 0, nbE = wire->NbEdges(); iE < nbE; ++iE )
      {
        subIDs.insert( wire->EdgeID( iE ));
        subIDs.insert( helper.GetMeshDS()->ShapeToIndex( wire->FirstVertex( iE )));
      }
    }
    for ( size_t ngID = 1; ngID < nodeVec.size(); ++ngID )
      if ( subIDs.count( nodeVec[ngID]->getshapeId() ))
        node2ngID.insert( make_pair( nodeVec[ngID], ngID ));
  }

  const int solidID = 0, faceID = geom.fmap.FindIndex( helper.GetSubShape() );
  if ( ngMesh.GetNFD() < 1 )
    ngMesh.AddFaceDescriptor (netgen::FaceDescriptor(faceID, solidID, solidID, 0));

  for ( int iW = 0; iW < wires.size(); ++iW )
  {
    StdMeshers_FaceSidePtr wire = wires[ iW ];
    const vector<UVPtStruct>& uvPtVec = wire->GetUVPtStruct();
    const int nbSegments = wire->NbPoints() - 1;

    // assure the 1st node to be in node2ngID, which is needed to correctly
    // "close chain of segments" (see below) in case if the 1st node is not
    // onVertex because it is on a Viscous layer
    node2ngID.insert( make_pair( uvPtVec[ 0 ].node, ngMesh.GetNP() + 1 ));

    // compute length of every segment
    vector<double> segLen( nbSegments );
    for ( int i = 0; i < nbSegments; ++i )
      segLen[i] = SMESH_TNodeXYZ( uvPtVec[ i ].node ).Distance( uvPtVec[ i+1 ].node );

    int edgeID = 1, posID = -2;
    bool isInternalWire = false;
    double vertexNormPar = 0;
    const int prevNbNGSeg = ngMesh.GetNSeg();
    for ( int i = 0; i < nbSegments; ++i ) // loop on segments
    {
      // Add the first point of a segment

      const SMDS_MeshNode * n = uvPtVec[ i ].node;
      const int posShapeID = n->getshapeId();
      bool onVertex = ( n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX );
      bool onEdge   = ( n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_EDGE   );

      // skip nodes on degenerated edges
      if ( helper.IsDegenShape( posShapeID ) &&
           helper.IsDegenShape( uvPtVec[ i+1 ].node->getshapeId() ))
        continue;

      int ngID1 = ngMesh.GetNP() + 1, ngID2 = ngID1+1;
      if ( onVertex || ( !wasNgMeshEmpty && onEdge ) || helper.IsRealSeam( posShapeID ))
        ngID1 = node2ngID.insert( make_pair( n, ngID1 )).first->second;
      if ( ngID1 > ngMesh.GetNP() )
      {
        netgen::MeshPoint mp( netgen::Point<3> (n->X(), n->Y(), n->Z()) );
        ngMesh.AddPoint ( mp, 1, netgen::EDGEPOINT );
        nodeVec.push_back( n );
      }
      else // n is in ngMesh already, and ngID2 in prev segment is wrong
      {
        ngID2 = ngMesh.GetNP() + 1;
        if ( i > 0 ) // prev segment belongs to same wire
        {
          netgen::Segment& prevSeg = ngMesh.LineSegment( ngMesh.GetNSeg() );
          prevSeg[1] = ngID1;
        }
      }

      // Add the segment

      netgen::Segment seg;

      seg[0]     = ngID1;                // ng node id
      seg[1]     = ngID2;                // ng node id
      seg.edgenr = ngMesh.GetNSeg() + 1; // ng segment id
      seg.si     = faceID;               // = geom.fmap.FindIndex (face);

      for ( int iEnd = 0; iEnd < 2; ++iEnd)
      {
        const UVPtStruct& pnt = uvPtVec[ i + iEnd ];

        seg.epgeominfo[ iEnd ].dist = pnt.param; // param on curve
        seg.epgeominfo[ iEnd ].u    = pnt.u;
        seg.epgeominfo[ iEnd ].v    = pnt.v;

        // find out edge id and node parameter on edge
        onVertex = ( pnt.normParam + 1e-10 > vertexNormPar );
        if ( onVertex || posShapeID != posID )
        {
          // get edge id
          double normParam = pnt.normParam;
          if ( onVertex )
            normParam = 0.5 * ( uvPtVec[ i ].normParam + uvPtVec[ i+1 ].normParam );
          int edgeIndexInWire = wire->EdgeIndex( normParam );
          vertexNormPar = wire->LastParameter( edgeIndexInWire );
          const TopoDS_Edge& edge = wire->Edge( edgeIndexInWire );
          edgeID = geom.emap.FindIndex( edge );
          posID  = posShapeID;
          isInternalWire = ( edge.Orientation() == TopAbs_INTERNAL );
          // if ( onVertex ) // param on curve is different on each of two edges
          //   seg.epgeominfo[ iEnd ].dist = helper.GetNodeU( edge, pnt.node );
        }
        seg.epgeominfo[ iEnd ].edgenr = edgeID; //  = geom.emap.FindIndex(edge);
      }

      ngMesh.AddSegment (seg);
      {
        // restrict size of elements near the segment
        SMESH_TNodeXYZ np1( n ), np2( uvPtVec[ i+1 ].node );
        // get an average size of adjacent segments to avoid sharp change of
        // element size (regression on issue 0020452, note 0010898)
        int   iPrev = SMESH_MesherHelper::WrapIndex( i-1, nbSegments );
        int   iNext = SMESH_MesherHelper::WrapIndex( i+1, nbSegments );
        double sumH = segLen[ iPrev ] + segLen[ i ] + segLen[ iNext ];
        int   nbSeg = ( int( segLen[ iPrev ] > sumH / 100.)  +
                        int( segLen[ i     ] > sumH / 100.)  +
                        int( segLen[ iNext ] > sumH / 100.));
        if ( nbSeg > 0 )
          RestrictLocalSize( ngMesh, 0.5*(np1+np2), sumH / nbSeg, overrideMinH );
      }
      if ( isInternalWire )
      {
        swap (seg[0], seg[1]);
        swap( seg.epgeominfo[0], seg.epgeominfo[1] );
        seg.edgenr = ngMesh.GetNSeg() + 1; // segment id
        ngMesh.AddSegment (seg);
      }
    } // loop on segments on a wire

    // close chain of segments
    if ( nbSegments > 0 )
    {
      netgen::Segment& lastSeg = ngMesh.LineSegment( ngMesh.GetNSeg() - int( isInternalWire));
      const SMDS_MeshNode * lastNode = uvPtVec.back().node;
      lastSeg[1] = node2ngID.insert( make_pair( lastNode, lastSeg[1] )).first->second;
      if ( lastSeg[1] > ngMesh.GetNP() )
      {
        netgen::MeshPoint mp( netgen::Point<3> (lastNode->X(), lastNode->Y(), lastNode->Z()) );
        ngMesh.AddPoint ( mp, 1, netgen::EDGEPOINT );
        nodeVec.push_back( lastNode );
      }
      if ( isInternalWire )
      {
        netgen::Segment& realLastSeg = ngMesh.LineSegment( ngMesh.GetNSeg() );
        realLastSeg[0] = lastSeg[1];
      }
    }

#ifdef DUMP_SEGMENTS
    cout << "BEGIN WIRE " << iW << std::endl;
    for ( int i = prevNbNGSeg+1; i <= ngMesh.GetNSeg(); ++i )
    {
      netgen::Segment& seg = ngMesh.LineSegment( i );
      if ( i > 1 ) {
        netgen::Segment& prevSeg = ngMesh.LineSegment( i-1 );
        if ( seg[0] == prevSeg[1] && seg[1] == prevSeg[0] )
        {
          cout << "Segment: " << seg.edgenr << std::endl << "\tis REVRESE of the previous one" << std::endl;
          continue;
        }
      }
      cout << "Segment: " << seg.edgenr << std::endl
           << "\tp1: " << seg[0] << "   n" << nodeVec[ seg[0]]->GetID() << std::endl
           << "\tp2: " << seg[1] << "   n" << nodeVec[ seg[1]]->GetID() <<  std::endl
           << "\tp0 param: " << seg.epgeominfo[ 0 ].dist << std::endl
           << "\tp0 uv: " << seg.epgeominfo[ 0 ].u <<", "<< seg.epgeominfo[ 0 ].v << std::endl
           << "\tp0 edge: " << seg.epgeominfo[ 0 ].edgenr << std::endl
           << "\tp1 param: " << seg.epgeominfo[ 1 ].dist << std::endl
           << "\tp1 uv: " << seg.epgeominfo[ 1 ].u <<", "<< seg.epgeominfo[ 1 ].v << std::endl
           << "\tp1 edge: " << seg.epgeominfo[ 1 ].edgenr << std::endl;
    }
    cout << "--END WIRE " << iW << std::endl;
#endif

  } // loop on WIREs of a FACE

  // add a segment instead of an internal vertex
  if ( wasNgMeshEmpty )
  {
    NETGENPlugin_Internals intShapes( *helper.GetMesh(), helper.GetSubShape(), /*is3D=*/false );
    AddIntVerticesInFaces( geom, ngMesh, nodeVec, intShapes );
  }
  ngMesh.CalcSurfacesOfNode();

  return TError();
}

//================================================================================
/*!
 * \brief Fill SMESH mesh according to contents of netgen mesh
 *  \param occgeo - container of OCCT geometry to mesh
 *  \param ngMesh - netgen mesh
 *  \param initState - bn of entities in netgen mesh before computing
 *  \param sMesh - SMESH mesh to fill in
 *  \param nodeVec - vector of nodes in which node index == netgen ID
 *  \param comment - returns problem description
 *  \param quadHelper - holder of medium nodes of sub-meshes
 *  \retval int - error
 */
//================================================================================

int NETGENPlugin_Mesher::FillSMesh(const netgen::OCCGeometry&          occgeo,
                                   netgen::Mesh&                       ngMesh,
                                   const NETGENPlugin_ngMeshInfo&      initState,
                                   SMESH_Mesh&                         sMesh,
                                   std::vector<const SMDS_MeshNode*>&  nodeVec,
                                   SMESH_Comment&                      comment,
                                   SMESH_MesherHelper*                 quadHelper)
{
  int nbNod = ngMesh.GetNP();
  int nbSeg = ngMesh.GetNSeg();
  int nbFac = ngMesh.GetNSE();
  int nbVol = ngMesh.GetNE();

  SMESHDS_Mesh* meshDS = sMesh.GetMeshDS();

  // quadHelper is used for either
  // 1) making quadratic elements when a lower dimention mesh is loaded
  //    to SMESH before convertion to quadratic by NETGEN
  // 2) sewing of quadratic elements with quadratic elements of sub-meshes
  if ( quadHelper && !quadHelper->GetIsQuadratic() && quadHelper->GetTLinkNodeMap().empty() )
    quadHelper = 0;

  // -------------------------------------
  // Create and insert nodes into nodeVec
  // -------------------------------------

  nodeVec.resize( nbNod + 1 );
  int i, nbInitNod = initState._nbNodes;
  for (i = nbInitNod+1; i <= nbNod; ++i )
  {
    const netgen::MeshPoint& ngPoint = ngMesh.Point(i);
    SMDS_MeshNode* node = NULL;
    TopoDS_Vertex aVert;
    // First, netgen creates nodes on vertices in occgeo.vmap,
    // so node index corresponds to vertex index
    // but (issue 0020776) netgen does not create nodes with equal coordinates
    if ( i-nbInitNod <= occgeo.vmap.Extent() )
    {
      gp_Pnt p ( NGPOINT_COORDS(ngPoint) );
      for (int iV = i-nbInitNod; aVert.IsNull() && iV <= occgeo.vmap.Extent(); ++iV)
      {
        aVert = TopoDS::Vertex( occgeo.vmap( iV ));
        gp_Pnt pV = BRep_Tool::Pnt( aVert );
        if ( p.SquareDistance( pV ) > 1e-20 )
          aVert.Nullify();
        else
          node = const_cast<SMDS_MeshNode*>( SMESH_Algo::VertexNode( aVert, meshDS ));
      }
    }
    if (!node) // node not found on vertex
    {
      node = meshDS->AddNode( NGPOINT_COORDS( ngPoint ));
      if (!aVert.IsNull())
        meshDS->SetNodeOnVertex(node, aVert);
    }
    nodeVec[i] = node;
  }

  // -------------------------------------------
  // Create mesh segments along geometric edges
  // -------------------------------------------

  int nbInitSeg = initState._nbSegments;
  for (i = nbInitSeg+1; i <= nbSeg; ++i )
  {
    const netgen::Segment& seg = ngMesh.LineSegment(i);
    TopoDS_Edge aEdge;
    int pinds[3] = { seg.pnums[0], seg.pnums[1], seg.pnums[2] };
    int nbp = 0;
    double param2 = 0;
    for (int j=0; j < 3; ++j)
    {
      int pind = pinds[j];
      if (pind <= 0 || !nodeVec_ACCESS(pind))
        break;
      ++nbp;
      double param;
      if (j < 2)
      {
        if (aEdge.IsNull())
        {
          int aGeomEdgeInd = seg.epgeominfo[j].edgenr;
          if (aGeomEdgeInd > 0 && aGeomEdgeInd <= occgeo.emap.Extent())
            aEdge = TopoDS::Edge(occgeo.emap(aGeomEdgeInd));
        }
        param = seg.epgeominfo[j].dist;
        param2 += param;
      }
      else // middle point
      {
        param = param2 * 0.5;
      }
      if (!aEdge.IsNull() && nodeVec_ACCESS(pind)->getshapeId() < 1)
      {
        meshDS->SetNodeOnEdge(nodeVec_ACCESS(pind), aEdge, param);
      }
    }
    if ( nbp > 1 )
    {
      SMDS_MeshEdge* edge = 0;
      if (nbp == 2) // second order ?
      {
        if ( meshDS->FindEdge( nodeVec_ACCESS(pinds[0]), nodeVec_ACCESS(pinds[1])))
          continue;
        if ( quadHelper ) // final mesh must be quadratic
          edge = quadHelper->AddEdge(nodeVec_ACCESS(pinds[0]), nodeVec_ACCESS(pinds[1]));
        else
          edge = meshDS->AddEdge(nodeVec_ACCESS(pinds[0]), nodeVec_ACCESS(pinds[1]));
      }
      else
      {
        if ( meshDS->FindEdge( nodeVec_ACCESS(pinds[0]), nodeVec_ACCESS(pinds[1]),
                               nodeVec_ACCESS(pinds[2])))
          continue;
        edge = meshDS->AddEdge(nodeVec_ACCESS(pinds[0]), nodeVec_ACCESS(pinds[1]),
                               nodeVec_ACCESS(pinds[2]));
      }
      if (!edge)
      {
        if ( comment.empty() ) comment << "Cannot create a mesh edge";
        MESSAGE("Cannot create a mesh edge");
        nbSeg = nbFac = nbVol = 0;
        break;
      }
      if ( !aEdge.IsNull() && edge->getshapeId() < 1 )
        meshDS->SetMeshElementOnShape(edge, aEdge);
    }
    else if ( comment.empty() )
    {
      comment << "Invalid netgen segment #" << i;
    }
  }

  // ----------------------------------------
  // Create mesh faces along geometric faces
  // ----------------------------------------

  int nbInitFac = initState._nbFaces;
  int quadFaceID = ngMesh.GetNFD() + 1;
  if ( nbInitFac < nbFac )
    // add a faces descriptor to exclude qudrangle elements generated by NETGEN
    // from computation of 3D mesh
    ngMesh.AddFaceDescriptor (netgen::FaceDescriptor(quadFaceID, /*solid1=*/0, /*solid2=*/0, 0));

  vector<const SMDS_MeshNode*> nodes;
  for (i = nbInitFac+1; i <= nbFac; ++i )
  {
    const netgen::Element2d& elem = ngMesh.SurfaceElement(i);
    int aGeomFaceInd = elem.GetIndex();
    TopoDS_Face aFace;
    if (aGeomFaceInd > 0 && aGeomFaceInd <= occgeo.fmap.Extent())
      aFace = TopoDS::Face(occgeo.fmap(aGeomFaceInd));
    nodes.clear();
    for (int j=1; j <= elem.GetNP(); ++j)
    {
      int pind = elem.PNum(j);
      if ( pind < 1 || pind >= nodeVec.size() )
        break;
      if ( SMDS_MeshNode* node = nodeVec_ACCESS(pind))
      {
        nodes.push_back( node );
        if (!aFace.IsNull() && node->getshapeId() < 1)
        {
          const netgen::PointGeomInfo& pgi = elem.GeomInfoPi(j);
          meshDS->SetNodeOnFace(node, aFace, pgi.u, pgi.v);
        }
      }
    }
    if ( nodes.size() != elem.GetNP() )
    {
      if ( comment.empty() )
        comment << "Invalid netgen 2d element #" << i;
      continue; // bad node ids
    }
    SMDS_MeshFace* face = NULL;
    switch (elem.GetType())
    {
    case netgen::TRIG:
      if ( quadHelper ) // final mesh must be quadratic
        face = quadHelper->AddFace(nodes[0],nodes[1],nodes[2]);
      else
        face = meshDS->AddFace(nodes[0],nodes[1],nodes[2]);
      break;
    case netgen::QUAD:
      if ( quadHelper ) // final mesh must be quadratic
        face = quadHelper->AddFace(nodes[0],nodes[1],nodes[2],nodes[3]);
      else
        face = meshDS->AddFace(nodes[0],nodes[1],nodes[2],nodes[3]);
      // exclude qudrangle elements from computation of 3D mesh
      const_cast< netgen::Element2d& >( elem ).SetIndex( quadFaceID );
      break;
    case netgen::TRIG6:
      nodes[5] = mediumNode( nodes[0],nodes[1],nodes[5], quadHelper );
      nodes[3] = mediumNode( nodes[1],nodes[2],nodes[3], quadHelper );
      nodes[4] = mediumNode( nodes[2],nodes[0],nodes[4], quadHelper );
      face = meshDS->AddFace(nodes[0],nodes[1],nodes[2],nodes[5],nodes[3],nodes[4]);
      break;
    case netgen::QUAD8:
      nodes[4] = mediumNode( nodes[0],nodes[1],nodes[4], quadHelper );
      nodes[7] = mediumNode( nodes[1],nodes[2],nodes[7], quadHelper );
      nodes[5] = mediumNode( nodes[2],nodes[3],nodes[5], quadHelper );
      nodes[6] = mediumNode( nodes[3],nodes[0],nodes[6], quadHelper );
      face = meshDS->AddFace(nodes[0],nodes[1],nodes[2],nodes[3],
                             nodes[4],nodes[7],nodes[5],nodes[6]);
      // exclude qudrangle elements from computation of 3D mesh
      const_cast< netgen::Element2d& >( elem ).SetIndex( quadFaceID );
      break;
    default:
      MESSAGE("NETGEN created a face of unexpected type, ignoring");
      continue;
    }
    if (!face)
    {
      if ( comment.empty() ) comment << "Cannot create a mesh face";
      MESSAGE("Cannot create a mesh face");
      nbSeg = nbFac = nbVol = 0;
      break;
    }
    if (!aFace.IsNull())
      meshDS->SetMeshElementOnShape(face, aFace);
  }

  // ------------------
  // Create tetrahedra
  // ------------------

  for (i = 1; i <= nbVol; ++i)
  {
    const netgen::Element& elem = ngMesh.VolumeElement(i);      
    int aSolidInd = elem.GetIndex();
    TopoDS_Solid aSolid;
    if (aSolidInd > 0 && aSolidInd <= occgeo.somap.Extent())
      aSolid = TopoDS::Solid(occgeo.somap(aSolidInd));
    nodes.clear();
    for (int j=1; j <= elem.GetNP(); ++j)
    {
      int pind = elem.PNum(j);
      if ( pind < 1 || pind >= nodeVec.size() )
        break;
      if ( SMDS_MeshNode* node = nodeVec_ACCESS(pind) )
      {
        nodes.push_back(node);
        if ( !aSolid.IsNull() && node->getshapeId() < 1 )
          meshDS->SetNodeInVolume(node, aSolid);
      }
    }
    if ( nodes.size() != elem.GetNP() )
    {
      if ( comment.empty() )
        comment << "Invalid netgen 3d element #" << i;
      continue;
    }
    SMDS_MeshVolume* vol = NULL;
    switch (elem.GetType())
    {
    case netgen::TET:
      vol = meshDS->AddVolume(nodes[0],nodes[1],nodes[2],nodes[3]);
      break;
    case netgen::TET10:
      nodes[4] = mediumNode( nodes[0],nodes[1],nodes[4], quadHelper );
      nodes[7] = mediumNode( nodes[1],nodes[2],nodes[7], quadHelper );
      nodes[5] = mediumNode( nodes[2],nodes[0],nodes[5], quadHelper );
      nodes[6] = mediumNode( nodes[0],nodes[3],nodes[6], quadHelper );
      nodes[8] = mediumNode( nodes[1],nodes[3],nodes[8], quadHelper );
      nodes[9] = mediumNode( nodes[2],nodes[3],nodes[9], quadHelper );
      vol = meshDS->AddVolume(nodes[0],nodes[1],nodes[2],nodes[3],
                              nodes[4],nodes[7],nodes[5],nodes[6],nodes[8],nodes[9]);
      break;
    default:
      MESSAGE("NETGEN created a volume of unexpected type, ignoring");
      continue;
    }
    if (!vol)
    {
      if ( comment.empty() ) comment << "Cannot create a mesh volume";
      MESSAGE("Cannot create a mesh volume");
      nbSeg = nbFac = nbVol = 0;
      break;
    }
    if (!aSolid.IsNull())
      meshDS->SetMeshElementOnShape(vol, aSolid);
  }
  return comment.empty() ? 0 : 1;
}

namespace
{
  //================================================================================
  /*!
   * \brief Restrict size of elements on the given edge 
   */
  //================================================================================

  void setLocalSize(const TopoDS_Edge& edge,
                    double             size,
                    netgen::Mesh&      mesh)
  {
    if ( size <= std::numeric_limits<double>::min() )
      return;
    Standard_Real u1, u2;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, u1, u2);
    if ( curve.IsNull() )
    {
      TopoDS_Iterator vIt( edge );
      if ( !vIt.More() ) return;
      gp_Pnt p = BRep_Tool::Pnt( TopoDS::Vertex( vIt.Value() ));
      NETGENPlugin_Mesher::RestrictLocalSize( mesh, p.XYZ(), size );
    }
    else
    {
      const int nb = (int)( 1.5 * SMESH_Algo::EdgeLength( edge ) / size );
      Standard_Real delta = (u2-u1)/nb;
      for(int i=0; i<nb; i++)
      {
        Standard_Real u = u1 + delta*i;
        gp_Pnt p = curve->Value(u);
        NETGENPlugin_Mesher::RestrictLocalSize( mesh, p.XYZ(), size );
        netgen::Point3d pi(p.X(), p.Y(), p.Z());
        double resultSize = mesh.GetH(pi);
        if ( resultSize - size > 0.1*size )
          // netgen does restriction iff oldH/newH > 1.2 (localh.cpp:136)
          NETGENPlugin_Mesher::RestrictLocalSize( mesh, p.XYZ(), resultSize/1.201 );
      }
    }
  }

  //================================================================================
  /*!
   * \brief Convert error into text
   */
  //================================================================================

  std::string text(int err)
  {
    if ( !err )
      return string("");
    return
      SMESH_Comment("Error in netgen::OCCGenerateMesh() at ") << netgen::multithread.task;
  }

  //================================================================================
  /*!
   * \brief Convert exception into text
   */
  //================================================================================

  std::string text(Standard_Failure& ex)
  {
    SMESH_Comment str("Exception in netgen::OCCGenerateMesh()");
    str << " at " << netgen::multithread.task
        << ": " << ex.DynamicType()->Name();
    if ( ex.GetMessageString() && strlen( ex.GetMessageString() ))
      str << ": " << ex.GetMessageString();
    return str;
  }
  //================================================================================
  /*!
   * \brief Convert exception into text
   */
  //================================================================================

  std::string text(netgen::NgException& ex)
  {
    SMESH_Comment str("NgException");
    if ( strlen( netgen::multithread.task ) > 0 )
      str << " at " << netgen::multithread.task;
    str << ": " << ex.What();
    return str;
  }

  //================================================================================
  /*!
   * \brief Looks for triangles lying on a SOLID
   */
  //================================================================================

  bool hasBadElemOnSolid( const list<const SMDS_MeshElement*>& elems,
                          SMESH_subMesh*                       solidSM )
  {
    TopTools_IndexedMapOfShape solidSubs;
    TopExp::MapShapes( solidSM->GetSubShape(), solidSubs );
    SMESHDS_Mesh* mesh = solidSM->GetFather()->GetMeshDS();

    list<const SMDS_MeshElement*>::const_iterator e = elems.begin();
    for ( ; e != elems.end(); ++e )
    {
      const SMDS_MeshElement* elem = *e;
      // if ( elem->GetType() != SMDSAbs_Face ) -- 23047
      //   continue;
      int nbNodesOnSolid = 0, nbNodes = elem->NbNodes();
      SMDS_NodeIteratorPtr nIt = elem->nodeIterator();
      while ( nIt->more() )
      {
        const SMDS_MeshNode* n = nIt->next();
        const TopoDS_Shape&  s = mesh->IndexToShape( n->getshapeId() );
        nbNodesOnSolid += ( !s.IsNull() && solidSubs.Contains( s ));
        if ( nbNodesOnSolid > 2 ||
             nbNodesOnSolid == nbNodes)
          return true;
      }
    }
    return false;
  }

  const double edgeMeshingTime = 0.001;
  const double faceMeshingTime = 0.019;
  const double edgeFaceMeshingTime = edgeMeshingTime + faceMeshingTime;
  const double faceOptimizTime = 0.06;
  const double voluMeshingTime = 0.15;
  const double volOptimizeTime = 0.77;
}

//=============================================================================
/*!
 * Here we are going to use the NETGEN mesher
 */
//=============================================================================

bool NETGENPlugin_Mesher::Compute()
{
  NETGENPlugin_NetgenLibWrapper ngLib;

  netgen::MeshingParameters& mparams = netgen::mparam;
  MESSAGE("Compute with:\n"
          " max size = " << mparams.maxh << "\n"
          " segments per edge = " << mparams.segmentsperedge);
  MESSAGE("\n"
          " growth rate = " << mparams.grading << "\n"
          " elements per radius = " << mparams.curvaturesafety << "\n"
          " second order = " << mparams.secondorder << "\n"
          " quad allowed = " << mparams.quad << "\n"
          " surface curvature = " << mparams.uselocalh << "\n"
          " fuse edges = " << netgen::merge_solids);

  SMESH_ComputeErrorPtr error = SMESH_ComputeError::New();
  SMESH_MesherHelper quadHelper( *_mesh );
  quadHelper.SetIsQuadratic( mparams.secondorder );

  static string debugFile = "/tmp/ngMesh.py"; /* to call toPython( _ngMesh, debugFile )
                                                 while debugging netgen */
  // -------------------------
  // Prepare OCC geometry
  // -------------------------

  netgen::OCCGeometry occgeo;
  list< SMESH_subMesh* > meshedSM[3]; // for 0-2 dimensions
  NETGENPlugin_Internals internals( *_mesh, _shape, _isVolume );
  PrepareOCCgeometry( occgeo, _shape, *_mesh, meshedSM, &internals );
  _occgeom = &occgeo;

  _totalTime = edgeFaceMeshingTime;
  if ( _optimize )
    _totalTime += faceOptimizTime;
  if ( _isVolume )
    _totalTime += voluMeshingTime + ( _optimize ? volOptimizeTime : 0 );
  double doneTime = 0;
  _ticTime = -1;
  _progressTic = 1;
  _curShapeIndex = -1;

  // -------------------------
  // Generate the mesh
  // -------------------------
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
  _ngMesh = NULL;
#endif
  NETGENPlugin_ngMeshInfo initState; // it remembers size of ng mesh equal to size of Smesh

  SMESH_Comment comment;
  int err = 0;

  // vector of nodes in which node index == netgen ID
  vector< const SMDS_MeshNode* > nodeVec;
  
  {
    // ----------------
    // compute 1D mesh
    // ----------------
    if ( _simpleHyp )
    {
      // not to RestrictLocalH() according to curvature during MESHCONST_ANALYSE
      mparams.uselocalh = false;
      mparams.grading = 0.8; // not limitited size growth

      if ( _simpleHyp->GetNumberOfSegments() )
        // nb of segments
        mparams.maxh = occgeo.boundingbox.Diam();
      else
        // segment length
        mparams.maxh = _simpleHyp->GetLocalLength();
    }

    if ( mparams.maxh == 0.0 )
      mparams.maxh = occgeo.boundingbox.Diam();
    if ( _simpleHyp || ( mparams.minh == 0.0 && _fineness != NETGENPlugin_Hypothesis::UserDefined))
      mparams.minh = GetDefaultMinSize( _shape, mparams.maxh );

    // Local size on faces
    occgeo.face_maxh = mparams.maxh;

    // Let netgen create _ngMesh and calculate element size on not meshed shapes
#if NETGEN_VERSION < NETGEN_VERSION_STRING(5,0)
    char *optstr = 0;
#endif
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2)
    int startWith = netgen::MESHCONST_ANALYSE;
    int endWith   = netgen::MESHCONST_ANALYSE;
#else
    mparams.perfstepsstart = mparams.perfstepsend = netgen::MESHCONST_ANALYSE;
#endif
    try
    {
      OCC_CATCH_SIGNALS;
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
      err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
      err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams, startWith, endWith);
#else
      err = netgen::OCCGenerateMesh(occgeo, _ngMesh, startWith, endWith, optstr);
#endif
      if(netgen::multithread.terminate)
        return false;

      comment << text(err);
    }
    catch (Standard_Failure& ex)
    {
      comment << text(ex);
    }
    err = 0; //- MESHCONST_ANALYSE isn't so important step
    if ( !_ngMesh )
      return false;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
    ngLib.setMesh(( Ng_Mesh*) _ngMesh);
#else
    ngLib.setMesh(( Ng_Mesh*) _ngMesh.get() );
#endif


    _ngMesh->ClearFaceDescriptors(); // we make descriptors our-self

    if ( _simpleHyp )
    {
      // Pass 1D simple parameters to NETGEN
      // --------------------------------
      int      nbSeg = _simpleHyp->GetNumberOfSegments();
      double segSize = _simpleHyp->GetLocalLength();
      for ( int iE = 1; iE <= occgeo.emap.Extent(); ++iE )
      {
        const TopoDS_Edge& e = TopoDS::Edge( occgeo.emap(iE));
        if ( nbSeg )
          segSize = SMESH_Algo::EdgeLength( e ) / ( nbSeg - 0.4 );
        setLocalSize( e, segSize, *_ngMesh );
      }
    }
    else // if ( ! _simpleHyp )
    {
      // Local size on vertices and edges
      // --------------------------------
      for(std::map<int,double>::const_iterator it=EdgeId2LocalSize.begin(); it!=EdgeId2LocalSize.end(); it++)
      {
        int key = (*it).first;
        double hi = (*it).second;
        const TopoDS_Shape& shape = ShapesWithLocalSize.FindKey(key);
        const TopoDS_Edge& e = TopoDS::Edge(shape);
        setLocalSize( e, hi, *_ngMesh );
      }
      for(std::map<int,double>::const_iterator it=VertexId2LocalSize.begin(); it!=VertexId2LocalSize.end(); it++)
      {
        int key = (*it).first;
        double hi = (*it).second;
        const TopoDS_Shape& shape = ShapesWithLocalSize.FindKey(key);
        const TopoDS_Vertex& v = TopoDS::Vertex(shape);
        gp_Pnt p = BRep_Tool::Pnt(v);
        NETGENPlugin_Mesher::RestrictLocalSize( *_ngMesh, p.XYZ(), hi );
      }
      for(map<int,double>::const_iterator it=FaceId2LocalSize.begin();
          it!=FaceId2LocalSize.end(); it++)
      {
        int key = (*it).first;
        double val = (*it).second;
        const TopoDS_Shape& shape = ShapesWithLocalSize.FindKey(key);
        int faceNgID = occgeo.fmap.FindIndex(shape);
        occgeo.SetFaceMaxH(faceNgID, val);
        for ( TopExp_Explorer edgeExp( shape, TopAbs_EDGE ); edgeExp.More(); edgeExp.Next() )
          setLocalSize( TopoDS::Edge( edgeExp.Current() ), val, *_ngMesh );
      }
    }

    // Precompute internal edges (issue 0020676) in order to
    // add mesh on them correctly (twice) to netgen mesh
    if ( !err && internals.hasInternalEdges() )
    {
      // load internal shapes into OCCGeometry
      netgen::OCCGeometry intOccgeo;
      internals.getInternalEdges( intOccgeo.fmap, intOccgeo.emap, intOccgeo.vmap, meshedSM );
      intOccgeo.boundingbox = occgeo.boundingbox;
      intOccgeo.shape = occgeo.shape;
      intOccgeo.face_maxh.SetSize(intOccgeo.fmap.Extent());
      intOccgeo.face_maxh = netgen::mparam.maxh;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
      netgen::Mesh *tmpNgMesh = NULL;
#else
      std::shared_ptr<netgen::Mesh> tmpNgMesh; // = std::make_shared<netgen::Mesh>();
#endif
      try
      {
        OCC_CATCH_SIGNALS;
        // compute local H on internal shapes in the main mesh
        //OCCSetLocalMeshSize(intOccgeo, *_ngMesh); it deletes _ngMesh->localH

        // let netgen create a temporary mesh

#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
        netgen::OCCGenerateMesh(intOccgeo, tmpNgMesh, mparams);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
        netgen::OCCGenerateMesh(intOccgeo, tmpNgMesh, mparams, startWith, endWith);
#else
        netgen::OCCGenerateMesh(intOccgeo, tmpNgMesh, startWith, endWith, optstr);
#endif
        if(netgen::multithread.terminate)
          return false;

        // copy LocalH from the main to temporary mesh
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
        initState.transferLocalH( _ngMesh, tmpNgMesh );
#else
        initState.transferLocalH( _ngMesh.get(), tmpNgMesh.get() );
#endif
        // compute mesh on internal edges
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2)
        startWith = endWith = netgen::MESHCONST_MESHEDGES;
#else
        mparams.perfstepsstart = mparams.perfstepsend = netgen::MESHCONST_MESHEDGES;
#endif


#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
        err = netgen::OCCGenerateMesh(intOccgeo, tmpNgMesh, mparams);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
        err = netgen::OCCGenerateMesh(intOccgeo, tmpNgMesh, mparams, startWith, endWith);
#else
        err = netgen::OCCGenerateMesh(intOccgeo, tmpNgMesh, startWith, endWith, optstr);
#endif
        comment << text(err);
      }
      catch (Standard_Failure& ex)
      {
        comment << text(ex);
        err = 1;
      }
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
      initState.restoreLocalH( tmpNgMesh );
#else
      initState.restoreLocalH( tmpNgMesh.get() );
#endif

      // fill SMESH by netgen mesh
      vector< const SMDS_MeshNode* > tmpNodeVec;
      FillSMesh( intOccgeo, *tmpNgMesh, initState, *_mesh, tmpNodeVec, comment );
      err = ( err || !comment.empty() );

#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
      nglib::Ng_DeleteMesh((nglib::Ng_Mesh*)tmpNgMesh);
#else
      tmpNgMesh.reset();
#endif
    }

    // Fill _ngMesh with nodes and segments of computed submeshes
    if ( !err )
    {
      err = ! ( FillNgMesh(occgeo, *_ngMesh, nodeVec, meshedSM[ MeshDim_0D ]) &&
                FillNgMesh(occgeo, *_ngMesh, nodeVec, meshedSM[ MeshDim_1D ], &quadHelper));
    }
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
    initState = NETGENPlugin_ngMeshInfo(_ngMesh);
#else
    initState = NETGENPlugin_ngMeshInfo(_ngMesh.get());
#endif

    // Compute 1d mesh
    if (!err)
    {
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2)
      startWith = endWith = netgen::MESHCONST_MESHEDGES;
#else
      mparams.perfstepsstart = mparams.perfstepsend = netgen::MESHCONST_MESHEDGES;
#endif
      try
      {
        OCC_CATCH_SIGNALS;
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams, startWith, endWith);
#else
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, startWith, endWith, optstr);
#endif
        if(netgen::multithread.terminate)
          return false;

        comment << text(err);
      }
      catch (Standard_Failure& ex)
      {
        comment << text(ex);
        err = 1;
      }
    }
    if ( _isVolume )
      _ticTime = ( doneTime += edgeMeshingTime ) / _totalTime / _progressTic;

    mparams.uselocalh = true; // restore as it is used at surface optimization

    // ---------------------
    // compute surface mesh
    // ---------------------
    if (!err)
    {
      // Pass 2D simple parameters to NETGEN
      if ( _simpleHyp ) {
        if ( double area = _simpleHyp->GetMaxElementArea() ) {
          // face area
          mparams.maxh = sqrt(2. * area/sqrt(3.0));
          mparams.grading = 0.4; // moderate size growth
        }
        else {
          // length from edges
          if ( _ngMesh->GetNSeg() ) {
            double edgeLength = 0;
            TopTools_MapOfShape visitedEdges;
            for ( TopExp_Explorer exp( _shape, TopAbs_EDGE ); exp.More(); exp.Next() )
              if( visitedEdges.Add(exp.Current()) )
                edgeLength += SMESH_Algo::EdgeLength( TopoDS::Edge( exp.Current() ));
            // we have to multiply length by 2 since for each TopoDS_Edge there
            // are double set of NETGEN edges, in other words, we have to
            // divide _ngMesh->GetNSeg() by 2.
            mparams.maxh = 2*edgeLength / _ngMesh->GetNSeg();
          }
          else {
            mparams.maxh = 1000;
          }
          mparams.grading = 0.2; // slow size growth
        }
        mparams.quad = _simpleHyp->GetAllowQuadrangles();
        mparams.maxh = min( mparams.maxh, occgeo.boundingbox.Diam()/2 );
        _ngMesh->SetGlobalH (mparams.maxh);
        netgen::Box<3> bb = occgeo.GetBoundingBox();
        bb.Increase (bb.Diam()/20);
        _ngMesh->SetLocalH (bb.PMin(), bb.PMax(), mparams.grading);
      }

      // Care of vertices internal in faces (issue 0020676)
      if ( internals.hasInternalVertexInFace() )
      {
        // store computed segments in SMESH in order not to create SMESH
        // edges for ng segments added by AddIntVerticesInFaces()
        FillSMesh( occgeo, *_ngMesh, initState, *_mesh, nodeVec, comment );
        // add segments to faces with internal vertices
        AddIntVerticesInFaces( occgeo, *_ngMesh, nodeVec, internals );
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
        initState = NETGENPlugin_ngMeshInfo(_ngMesh);
#else
        initState = NETGENPlugin_ngMeshInfo(_ngMesh.get());
#endif
      }

      // Build viscous layers
      if ( _isViscousLayers2D )
      {
        if ( !internals.hasInternalVertexInFace() ) {
          FillSMesh( occgeo, *_ngMesh, initState, *_mesh, nodeVec, comment );
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
          initState = NETGENPlugin_ngMeshInfo(_ngMesh);
#else
          initState = NETGENPlugin_ngMeshInfo(_ngMesh.get());
#endif
        }
        SMESH_ProxyMesh::Ptr viscousMesh;
        SMESH_MesherHelper   helper( *_mesh );
        for ( int faceID = 1; faceID <= occgeo.fmap.Extent(); ++faceID )
        {
          const TopoDS_Face& F = TopoDS::Face( occgeo.fmap( faceID ));
          viscousMesh = StdMeshers_ViscousLayers2D::Compute( *_mesh, F );
          if ( !viscousMesh )
            return false;
          // exclude from computation ng segments built on EDGEs of F
          for (int i = 1; i <= _ngMesh->GetNSeg(); i++)
          {
            netgen::Segment & seg = _ngMesh->LineSegment(i);
            if (seg.si == faceID)
              seg.si = 0;
          }
          // add new segments to _ngMesh instead of excluded ones
          helper.SetSubShape( F );
          TSideVector wires =
            StdMeshers_FaceSide::GetFaceWires( F, *_mesh, /*skipMediumNodes=*/true,
                                               error, viscousMesh );
          error = AddSegmentsToMesh( *_ngMesh, occgeo, wires, helper, nodeVec );

          if ( !error ) error = SMESH_ComputeError::New();
        }
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
        initState = NETGENPlugin_ngMeshInfo(_ngMesh);
#else
        initState = NETGENPlugin_ngMeshInfo(_ngMesh.get());
#endif
      }

      // Let netgen compute 2D mesh
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2)
      startWith = netgen::MESHCONST_MESHSURFACE;
      endWith = _optimize ? netgen::MESHCONST_OPTSURFACE : netgen::MESHCONST_MESHSURFACE;
#else
      mparams.perfstepsstart = netgen::MESHCONST_MESHSURFACE;
      mparams.perfstepsend = _optimize ? netgen::MESHCONST_OPTSURFACE : netgen::MESHCONST_MESHSURFACE;
#endif
      try
      {
        OCC_CATCH_SIGNALS;
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams, startWith, endWith);
#else
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, startWith, endWith, optstr);
#endif
        if(netgen::multithread.terminate)
          return false;
        comment << text (err);
      }
      catch (Standard_Failure& ex)
      {
        comment << text(ex);
        //err = 1; -- try to make volumes anyway
      }
      catch (netgen::NgException exc)
      {
        comment << text(exc);
        //err = 1; -- try to make volumes anyway
      }
    }
    if ( _isVolume )
    {
      doneTime += faceMeshingTime + ( _optimize ? faceOptimizTime : 0 );
      _ticTime = doneTime / _totalTime / _progressTic;
    }
    // ---------------------
    // generate volume mesh
    // ---------------------
    // Fill _ngMesh with nodes and faces of computed 2D submeshes
    if ( !err && _isVolume && ( !meshedSM[ MeshDim_2D ].empty() || mparams.quad ))
    {
      // load SMESH with computed segments and faces
      FillSMesh( occgeo, *_ngMesh, initState, *_mesh, nodeVec, comment, &quadHelper );

      // compute pyramids on quadrangles
      SMESH_ProxyMesh::Ptr proxyMesh;
      if ( _mesh->NbQuadrangles() > 0 )
        for ( int iS = 1; iS <= occgeo.somap.Extent(); ++iS )
        {
          StdMeshers_QuadToTriaAdaptor* Adaptor = new StdMeshers_QuadToTriaAdaptor;
          proxyMesh.reset( Adaptor );

          int nbPyrams = _mesh->NbPyramids();
          Adaptor->Compute( *_mesh, occgeo.somap(iS) );
          if ( nbPyrams != _mesh->NbPyramids() )
          {
            list< SMESH_subMesh* > quadFaceSM;
            for (TopExp_Explorer face(occgeo.somap(iS), TopAbs_FACE); face.More(); face.Next())
              if ( Adaptor->GetProxySubMesh( face.Current() ))
              {
                quadFaceSM.push_back( _mesh->GetSubMesh( face.Current() ));
                meshedSM[ MeshDim_2D ].remove( quadFaceSM.back() );
              }
            FillNgMesh(occgeo, *_ngMesh, nodeVec, quadFaceSM, &quadHelper, proxyMesh);
          }
        }
      // fill _ngMesh with faces of sub-meshes
      err = ! ( FillNgMesh(occgeo, *_ngMesh, nodeVec, meshedSM[ MeshDim_2D ], &quadHelper));

#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
      initState = NETGENPlugin_ngMeshInfo(_ngMesh);
#else
      initState = NETGENPlugin_ngMeshInfo(_ngMesh.get());
#endif
      //toPython( _ngMesh, "/tmp/ngPython.py");
    }
    if (!err && _isVolume)
    {
      // Pass 3D simple parameters to NETGEN
      const NETGENPlugin_SimpleHypothesis_3D* simple3d =
        dynamic_cast< const NETGENPlugin_SimpleHypothesis_3D* > ( _simpleHyp );
      if ( simple3d ) {
        if ( double vol = simple3d->GetMaxElementVolume() ) {
          // max volume
          mparams.maxh = pow( 72, 1/6. ) * pow( vol, 1/3. );
          mparams.maxh = min( mparams.maxh, occgeo.boundingbox.Diam()/2 );
        }
        else {
          // length from faces
          mparams.maxh = _ngMesh->AverageH();
        }
        _ngMesh->SetGlobalH (mparams.maxh);
        mparams.grading = 0.4;

#if NETGEN_VERSION < NETGEN_VERSION_STRING(5,0)
        _ngMesh->CalcLocalH();
#else
        _ngMesh->CalcLocalH(mparams.grading);
#endif
      }
      // Care of vertices internal in solids and internal faces (issue 0020676)
      if ( internals.hasInternalVertexInSolid() || internals.hasInternalFaces() )
      {
        // store computed faces in SMESH in order not to create SMESH
        // faces for ng faces added here
        FillSMesh( occgeo, *_ngMesh, initState, *_mesh, nodeVec, comment, &quadHelper );
        // add ng faces to solids with internal vertices
        AddIntVerticesInSolids( occgeo, *_ngMesh, nodeVec, internals );
        // duplicate mesh faces on internal faces
        FixIntFaces( occgeo, *_ngMesh, internals );
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
        initState = NETGENPlugin_ngMeshInfo(_ngMesh);
#else
        initState = NETGENPlugin_ngMeshInfo(_ngMesh.get());
#endif
      }
      // Let netgen compute 3D mesh
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2)
      startWith = endWith = netgen::MESHCONST_MESHVOLUME;
#else
      mparams.perfstepsstart = mparams.perfstepsend = netgen::MESHCONST_MESHVOLUME;
#endif
      try
      {
        OCC_CATCH_SIGNALS;
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams, startWith, endWith);
#else
        err = netgen::OCCGenerateMesh(occgeo, _ngMesh, startWith, endWith, optstr);
#endif
        if(netgen::multithread.terminate)
          return false;

        if ( comment.empty() ) // do not overwrite a previos error
          comment << text(err);
      }
      catch (Standard_Failure& ex)
      {
        if ( comment.empty() ) // do not overwrite a previos error
          comment << text(ex);
        err = 1;
      }
      catch (netgen::NgException exc)
      {
        if ( comment.empty() ) // do not overwrite a previos error
          comment << text(exc);
        err = 1;
      }
      _ticTime = ( doneTime += voluMeshingTime ) / _totalTime / _progressTic;

      // Let netgen optimize 3D mesh
      if ( !err && _optimize )
      {
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2)
        startWith = endWith = netgen::MESHCONST_OPTVOLUME;
#else
        mparams.perfstepsstart = mparams.perfstepsend = netgen::MESHCONST_OPTVOLUME;
#endif
        try
        {
          OCC_CATCH_SIGNALS;
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
          err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
          err = netgen::OCCGenerateMesh(occgeo, _ngMesh, mparams, startWith, endWith);
#else
          err = netgen::OCCGenerateMesh(occgeo, _ngMesh, startWith, endWith, optstr);
#endif
          if(netgen::multithread.terminate)
            return false;

          if ( comment.empty() ) // do not overwrite a previos error
            comment << text(err);
        }
        catch (Standard_Failure& ex)
        {
          if ( comment.empty() ) // do not overwrite a previos error
            comment << text(ex);
        }
        catch (netgen::NgException exc)
        {
          if ( comment.empty() ) // do not overwrite a previos error
            comment << text(exc);
        }
      }
    }
    if (!err && mparams.secondorder > 0)
    {
      try
      {
        OCC_CATCH_SIGNALS;
        if ( !meshedSM[ MeshDim_1D ].empty() )
        {
          // remove segments not attached to geometry (IPAL0052479)
          for (int i = 1; i <= _ngMesh->GetNSeg(); ++i)
          {
            const netgen::Segment & seg = _ngMesh->LineSegment (i);
            if ( seg.epgeominfo[ 0 ].edgenr == 0 )
              _ngMesh->DeleteSegment( i );
          }
          _ngMesh->Compress();
        }
        // convert to quadratic
        netgen::OCCRefinementSurfaces ref (occgeo);
        ref.MakeSecondOrder (*_ngMesh);

        // care of elements already loaded to SMESH
        // if ( initState._nbSegments > 0 )
        //   makeQuadratic( occgeo.emap, _mesh );
        // if ( initState._nbFaces > 0 )
        //   makeQuadratic( occgeo.fmap, _mesh );
      }
      catch (Standard_Failure& ex)
      {
        if ( comment.empty() ) // do not overwrite a previos error
          comment << "Exception in netgen at passing to 2nd order ";
      }
      catch (netgen::NgException exc)
      {
        if ( comment.empty() ) // do not overwrite a previos error
          comment << exc.What();
      }
    }
  }

  _ticTime = 0.98 / _progressTic;

  int nbNod = _ngMesh->GetNP();
  int nbSeg = _ngMesh->GetNSeg();
  int nbFac = _ngMesh->GetNSE();
  int nbVol = _ngMesh->GetNE();
  bool isOK = ( !err && (_isVolume ? (nbVol > 0) : (nbFac > 0)) );

  MESSAGE((err ? "Mesh Generation failure" : "End of Mesh Generation") <<
          ", nb nodes: "    << nbNod <<
          ", nb segments: " << nbSeg <<
          ", nb faces: "    << nbFac <<
          ", nb volumes: "  << nbVol);

  // Feed back the SMESHDS with the generated Nodes and Elements
  if ( true /*isOK*/ ) // get whatever built
  {
    FillSMesh( occgeo, *_ngMesh, initState, *_mesh, nodeVec, comment, &quadHelper );

    if ( quadHelper.GetIsQuadratic() ) // remove free nodes
      for ( size_t i = 0; i < nodeVec.size(); ++i )
        if ( nodeVec[i] && nodeVec[i]->NbInverseElements() == 0 )
          _mesh->GetMeshDS()->RemoveFreeNode( nodeVec[i], 0, /*fromGroups=*/false );
  }
  SMESH_ComputeErrorPtr readErr = ReadErrors(nodeVec);
  if ( readErr && !readErr->myBadElements.empty() )
  {
    error = readErr;
    if ( !comment.empty() && !readErr->myComment.empty() ) comment += "\n";
    comment += readErr->myComment;
  }
  if ( error->IsOK() && ( !isOK || comment.size() > 0 ))
    error->myName = COMPERR_ALGO_FAILED;
  if ( !comment.empty() )
    error->myComment = comment;

  // SetIsAlwaysComputed( true ) to empty sub-meshes, which
  // appear if the geometry contains coincident sub-shape due
  // to bool merge_solids = 1; in netgen/libsrc/occ/occgenmesh.cpp
  const int nbMaps = 2;
  const TopTools_IndexedMapOfShape* geoMaps[nbMaps] =
    { & occgeo.vmap, & occgeo.emap/*, & occgeo.fmap*/ };
  for ( int iMap = 0; iMap < nbMaps; ++iMap )
    for (int i = 1; i <= geoMaps[iMap]->Extent(); i++)
      if ( SMESH_subMesh* sm = _mesh->GetSubMeshContaining( geoMaps[iMap]->FindKey(i)))
        if ( !sm->IsMeshComputed() )
          sm->SetIsAlwaysComputed( true );

  // set bad compute error to subshapes of all failed sub-shapes
  if ( !error->IsOK() )
  {
    bool pb2D = false, pb3D = false;
    for (int i = 1; i <= occgeo.fmap.Extent(); i++) {
      int status = occgeo.facemeshstatus[i-1];
      if (status == 1 ) continue;
      if ( SMESH_subMesh* sm = _mesh->GetSubMeshContaining( occgeo.fmap( i ))) {
        SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
        if ( !smError || smError->IsOK() ) {
          if ( status == -1 )
            smError.reset( new SMESH_ComputeError( *error ));
          else
            smError.reset( new SMESH_ComputeError( COMPERR_ALGO_FAILED, "Ignored" ));
          if ( SMESH_Algo::GetMeshError( sm ) == SMESH_Algo::MEr_OK )
            smError->myName = COMPERR_WARNING;
        }
        pb2D = pb2D || smError->IsKO();
      }
    }
    if ( !pb2D ) // all faces are OK
      for (int i = 1; i <= occgeo.somap.Extent(); i++)
        if ( SMESH_subMesh* sm = _mesh->GetSubMeshContaining( occgeo.somap( i )))
        {
          bool smComputed = nbVol && !sm->IsEmpty();
          if ( smComputed && internals.hasInternalVertexInSolid( sm->GetId() ))
          {
            int nbIntV = internals.getSolidsWithVertices().find( sm->GetId() )->second.size();
            SMESHDS_SubMesh* smDS = sm->GetSubMeshDS();
            smComputed = ( smDS->NbElements() > 0 || smDS->NbNodes() > nbIntV );
          }
          SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
          if ( !smComputed && ( !smError || smError->IsOK() ))
          {
            smError.reset( new SMESH_ComputeError( *error ));
            if ( nbVol && SMESH_Algo::GetMeshError( sm ) == SMESH_Algo::MEr_OK )
            {
              smError->myName = COMPERR_WARNING;
            }
            else if ( !smError->myBadElements.empty() ) // bad surface mesh
            {
              if ( !hasBadElemOnSolid( smError->myBadElements, sm ))
                smError.reset();
            }
          }
          pb3D = pb3D || ( smError && smError->IsKO() );
        }
    if ( !pb2D && !pb3D )
      err = 0; // no fatal errors, only warnings
  }

  ngLib._isComputeOk = !err;

  return !err;
}

//=============================================================================
/*!
 * Evaluate
 */
//=============================================================================
bool NETGENPlugin_Mesher::Evaluate(MapShapeNbElems& aResMap)
{
  netgen::MeshingParameters& mparams = netgen::mparam;


  // -------------------------
  // Prepare OCC geometry
  // -------------------------
  netgen::OCCGeometry occgeo;
  list< SMESH_subMesh* > meshedSM[4]; // for 0-3 dimensions
  NETGENPlugin_Internals internals( *_mesh, _shape, _isVolume );
  PrepareOCCgeometry( occgeo, _shape, *_mesh, meshedSM, &internals );

  bool tooManyElems = false;
  const int hugeNb = std::numeric_limits<int>::max() / 100;

  // ----------------
  // evaluate 1D 
  // ----------------
  // pass 1D simple parameters to NETGEN
  if ( _simpleHyp )
  {
    // not to RestrictLocalH() according to curvature during MESHCONST_ANALYSE
    mparams.uselocalh = false;
    mparams.grading = 0.8; // not limitited size growth

    if ( _simpleHyp->GetNumberOfSegments() )
      // nb of segments
      mparams.maxh = occgeo.boundingbox.Diam();
    else
      // segment length
      mparams.maxh = _simpleHyp->GetLocalLength();
  }

  if ( mparams.maxh == 0.0 )
    mparams.maxh = occgeo.boundingbox.Diam();
  if ( _simpleHyp || ( mparams.minh == 0.0 && _fineness != NETGENPlugin_Hypothesis::UserDefined))
    mparams.minh = GetDefaultMinSize( _shape, mparams.maxh );

  // let netgen create _ngMesh and calculate element size on not meshed shapes
  NETGENPlugin_NetgenLibWrapper ngLib;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
  netgen::Mesh *ngMesh = NULL;
#else
  std::shared_ptr<netgen::Mesh> ngMesh; // = std::make_shared<netgen::Mesh>();
#endif
#if NETGEN_VERSION < NETGEN_VERSION_STRING(5,0)
  char *optstr = 0;
#endif
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2)
  int startWith = netgen::MESHCONST_ANALYSE;
  int endWith   = netgen::MESHCONST_MESHEDGES;
#else
  mparams.perfstepsstart = netgen::MESHCONST_ANALYSE;
  mparams.perfstepsend = netgen::MESHCONST_MESHEDGES;
#endif

#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2)
  int err = netgen::OCCGenerateMesh(occgeo, ngMesh, mparams);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0)
  int err = netgen::OCCGenerateMesh(occgeo, ngMesh, mparams, startWith, endWith);
#else
  int err = netgen::OCCGenerateMesh(occgeo, ngMesh, startWith, endWith, optstr);
#endif

  if(netgen::multithread.terminate)
    return false;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
  ngLib.setMesh(( Ng_Mesh*) ngMesh);
#else
  ngLib.setMesh(( Ng_Mesh*) ngMesh.get());
#endif
  if (err) {
    if ( SMESH_subMesh* sm = _mesh->GetSubMeshContaining( _shape ))
      sm->GetComputeError().reset( new SMESH_ComputeError( COMPERR_ALGO_FAILED ));
    return false;
  }
  if ( _simpleHyp )
  {
    // Pass 1D simple parameters to NETGEN
    // --------------------------------
    int      nbSeg = _simpleHyp->GetNumberOfSegments();
    double segSize = _simpleHyp->GetLocalLength();
    for ( int iE = 1; iE <= occgeo.emap.Extent(); ++iE )
    {
      const TopoDS_Edge& e = TopoDS::Edge( occgeo.emap(iE));
      if ( nbSeg )
        segSize = SMESH_Algo::EdgeLength( e ) / ( nbSeg - 0.4 );
      setLocalSize( e, segSize, *ngMesh );
    }
  }
  else // if ( ! _simpleHyp )
  {
    // Local size on vertices and edges
    // --------------------------------
    for(std::map<int,double>::const_iterator it=EdgeId2LocalSize.begin(); it!=EdgeId2LocalSize.end(); it++)
    {
      int key = (*it).first;
      double hi = (*it).second;
      const TopoDS_Shape& shape = ShapesWithLocalSize.FindKey(key);
      const TopoDS_Edge& e = TopoDS::Edge(shape);
      setLocalSize( e, hi, *ngMesh );
    }
    for(std::map<int,double>::const_iterator it=VertexId2LocalSize.begin(); it!=VertexId2LocalSize.end(); it++)
    {
      int key = (*it).first;
      double hi = (*it).second;
      const TopoDS_Shape& shape = ShapesWithLocalSize.FindKey(key);
      const TopoDS_Vertex& v = TopoDS::Vertex(shape);
      gp_Pnt p = BRep_Tool::Pnt(v);
      NETGENPlugin_Mesher::RestrictLocalSize( *ngMesh, p.XYZ(), hi );
    }
    for(map<int,double>::const_iterator it=FaceId2LocalSize.begin();
        it!=FaceId2LocalSize.end(); it++)
    {
      int key = (*it).first;
      double val = (*it).second;
      const TopoDS_Shape& shape = ShapesWithLocalSize.FindKey(key);
      int faceNgID = occgeo.fmap.FindIndex(shape);
      occgeo.SetFaceMaxH(faceNgID, val);
      for ( TopExp_Explorer edgeExp( shape, TopAbs_EDGE ); edgeExp.More(); edgeExp.Next() )
        setLocalSize( TopoDS::Edge( edgeExp.Current() ), val, *ngMesh );
    }
  }
  // calculate total nb of segments and length of edges
  double fullLen = 0.0;
  int fullNbSeg = 0;
  int entity = mparams.secondorder > 0 ? SMDSEntity_Quad_Edge : SMDSEntity_Edge;
  TopTools_DataMapOfShapeInteger Edge2NbSeg;
  for (TopExp_Explorer exp(_shape, TopAbs_EDGE); exp.More(); exp.Next())
  {
    TopoDS_Edge E = TopoDS::Edge( exp.Current() );
    if( !Edge2NbSeg.Bind(E,0) )
      continue;

    double aLen = SMESH_Algo::EdgeLength(E);
    fullLen += aLen;

    vector<int>& aVec = aResMap[_mesh->GetSubMesh(E)];
    if ( aVec.empty() )
      aVec.resize( SMDSEntity_Last, 0);
    else
      fullNbSeg += aVec[ entity ];
  }

  // store nb of segments computed by Netgen
  NCollection_Map<Link> linkMap;
  for (int i = 1; i <= ngMesh->GetNSeg(); ++i )
  {
    const netgen::Segment& seg = ngMesh->LineSegment(i);
    Link link(seg[0], seg[1]);
    if ( !linkMap.Add( link )) continue;
    int aGeomEdgeInd = seg.epgeominfo[0].edgenr;
    if (aGeomEdgeInd > 0 && aGeomEdgeInd <= occgeo.emap.Extent())
    {
      vector<int>& aVec = aResMap[_mesh->GetSubMesh(occgeo.emap(aGeomEdgeInd))];
      aVec[ entity ]++;
    }
  }
  // store nb of nodes on edges computed by Netgen
  TopTools_DataMapIteratorOfDataMapOfShapeInteger Edge2NbSegIt(Edge2NbSeg);
  for (; Edge2NbSegIt.More(); Edge2NbSegIt.Next())
  {
    vector<int>& aVec = aResMap[_mesh->GetSubMesh(Edge2NbSegIt.Key())];
    if ( aVec[ entity ] > 1 && aVec[ SMDSEntity_Node ] == 0 )
      aVec[SMDSEntity_Node] = mparams.secondorder > 0  ? 2*aVec[ entity ]-1 : aVec[ entity ]-1;

    fullNbSeg += aVec[ entity ];
    Edge2NbSeg( Edge2NbSegIt.Key() ) = aVec[ entity ];
  }
  if ( fullNbSeg == 0 )
    return false;

  // ----------------
  // evaluate 2D 
  // ----------------
  if ( _simpleHyp ) {
    if ( double area = _simpleHyp->GetMaxElementArea() ) {
      // face area
      mparams.maxh = sqrt(2. * area/sqrt(3.0));
      mparams.grading = 0.4; // moderate size growth
    }
    else {
      // length from edges
      mparams.maxh = fullLen/fullNbSeg;
      mparams.grading = 0.2; // slow size growth
    }
  }
  mparams.maxh = min( mparams.maxh, occgeo.boundingbox.Diam()/2 );
  mparams.maxh = min( mparams.maxh, fullLen/fullNbSeg * (1. + mparams.grading));

  for (TopExp_Explorer exp(_shape, TopAbs_FACE); exp.More(); exp.Next())
  {
    TopoDS_Face F = TopoDS::Face( exp.Current() );
    SMESH_subMesh *sm = _mesh->GetSubMesh(F);
    GProp_GProps G;
    BRepGProp::SurfaceProperties(F,G);
    double anArea = G.Mass();
    tooManyElems = tooManyElems || ( anArea/hugeNb > mparams.maxh*mparams.maxh );
    int nb1d = 0;
    if ( !tooManyElems )
    {
      TopTools_MapOfShape egdes;
      for (TopExp_Explorer exp1(F,TopAbs_EDGE); exp1.More(); exp1.Next())
        if ( egdes.Add( exp1.Current() ))
          nb1d += Edge2NbSeg.Find(exp1.Current());
    }
    int nbFaces = tooManyElems ? hugeNb : int( 4*anArea / (mparams.maxh*mparams.maxh*sqrt(3.)));
    int nbNodes = tooManyElems ? hugeNb : (( nbFaces*3 - (nb1d-1)*2 ) / 6 + 1 );

    vector<int> aVec(SMDSEntity_Last, 0);
    if( mparams.secondorder > 0 ) {
      int nb1d_in = (nbFaces*3 - nb1d) / 2;
      aVec[SMDSEntity_Node] = nbNodes + nb1d_in;
      aVec[SMDSEntity_Quad_Triangle] = nbFaces;
    }
    else {
      aVec[SMDSEntity_Node] = Max ( nbNodes, 0  );
      aVec[SMDSEntity_Triangle] = nbFaces;
    }
    aResMap[sm].swap(aVec);
  }

  // ----------------
  // evaluate 3D
  // ----------------
  if(_isVolume) {
    // pass 3D simple parameters to NETGEN
    const NETGENPlugin_SimpleHypothesis_3D* simple3d =
      dynamic_cast< const NETGENPlugin_SimpleHypothesis_3D* > ( _simpleHyp );
    if ( simple3d ) {
      if ( double vol = simple3d->GetMaxElementVolume() ) {
        // max volume
        mparams.maxh = pow( 72, 1/6. ) * pow( vol, 1/3. );
        mparams.maxh = min( mparams.maxh, occgeo.boundingbox.Diam()/2 );
      }
      else {
        // using previous length from faces
      }
      mparams.grading = 0.4;
      mparams.maxh = min( mparams.maxh, fullLen/fullNbSeg * (1. + mparams.grading));
    }
    GProp_GProps G;
    BRepGProp::VolumeProperties(_shape,G);
    double aVolume = G.Mass();
    double tetrVol = 0.1179*mparams.maxh*mparams.maxh*mparams.maxh;
    tooManyElems = tooManyElems || ( aVolume/hugeNb > tetrVol );
    int nbVols = tooManyElems ? hugeNb : int(aVolume/tetrVol);
    int nb1d_in = int(( nbVols*6 - fullNbSeg ) / 6 );
    vector<int> aVec(SMDSEntity_Last, 0 );
    if ( tooManyElems ) // avoid FPE
    {
      aVec[SMDSEntity_Node] = hugeNb;
      aVec[ mparams.secondorder > 0 ? SMDSEntity_Quad_Tetra : SMDSEntity_Tetra] = hugeNb;
    }
    else
    {
      if( mparams.secondorder > 0 ) {
        aVec[SMDSEntity_Node] = nb1d_in/3 + 1 + nb1d_in;
        aVec[SMDSEntity_Quad_Tetra] = nbVols;
      }
      else {
        aVec[SMDSEntity_Node] = nb1d_in/3 + 1;
        aVec[SMDSEntity_Tetra] = nbVols;
      }
    }
    SMESH_subMesh *sm = _mesh->GetSubMesh(_shape);
    aResMap[sm].swap(aVec);
  }

  return true;
}

double NETGENPlugin_Mesher::GetProgress(const SMESH_Algo* holder,
                                        const int *       algoProgressTic,
                                        const double *    algoProgress) const
{
  ((int&) _progressTic ) = *algoProgressTic + 1;

  if ( !_occgeom ) return 0;

  double progress = -1;
  if ( !_isVolume )
  {
    if ( _ticTime < 0 && netgen::multithread.task[0] == 'O'/*Optimizing surface*/ )
    {
      ((double&) _ticTime ) = edgeFaceMeshingTime / _totalTime / _progressTic;
    }
    else if ( !_optimize /*&& _occgeom->fmap.Extent() > 1*/ )
    {
      int doneShapeIndex = -1;
      while ( doneShapeIndex+1 < _occgeom->facemeshstatus.Size() &&
              _occgeom->facemeshstatus[ doneShapeIndex+1 ])
        doneShapeIndex++;
      if ( doneShapeIndex+1 != _curShapeIndex )
      {
        ((int&) _curShapeIndex) = doneShapeIndex+1;
        double    doneShapeRate = _curShapeIndex / double( _occgeom->fmap.Extent() );
        double         doneTime = edgeMeshingTime + doneShapeRate * faceMeshingTime;
        ((double&)    _ticTime) = doneTime / _totalTime / _progressTic;
        // cout << "shape " << _curShapeIndex << " _ticTime " << _ticTime
        //      << " " << doneTime / _totalTime / _progressTic << std::endl;
      }
    }
  }
  else if ( !_optimize && _occgeom->somap.Extent() > 1 )
  {
    int curShapeIndex = _curShapeIndex;
    if ( _ngMesh->GetNE() > 0 )
    {
      netgen::Element el = (*_ngMesh)[netgen::ElementIndex( _ngMesh->GetNE()-1 )];
      curShapeIndex = el.GetIndex();
    }
    if ( curShapeIndex != _curShapeIndex )
    {
      ((int&) _curShapeIndex) = curShapeIndex;
      double    doneShapeRate = _curShapeIndex / double( _occgeom->somap.Extent() );
      double         doneTime = edgeFaceMeshingTime + doneShapeRate * voluMeshingTime;
      ((double&)    _ticTime) = doneTime / _totalTime / _progressTic;
      // cout << "shape " << _curShapeIndex << " _ticTime " << _ticTime
      //      << " " << doneTime / _totalTime / _progressTic << std::endl;
    }
  }
  if ( _ticTime > 0 )
    progress  = Max( *algoProgressTic * _ticTime, *algoProgress );
  if ( progress > 0 )
  {
    ((int&) *algoProgressTic )++;
    ((double&) *algoProgress) = progress;
  }
  //cout << progress << " "  << *algoProgressTic << " " << netgen::multithread.task << " "<< _ticTime << std::endl;

  return Min( progress, 0.99 );
}

//================================================================================
/*!
 * \brief Remove "test.out" and "problemfaces" files in current directory
 */
//================================================================================

void NETGENPlugin_Mesher::RemoveTmpFiles()
{
  bool rm =  SMESH_File("test.out").remove() ;
#ifndef WIN32
  if (rm && netgen::testout)
  {
    delete netgen::testout;
    netgen::testout = 0;
  }
#endif
  SMESH_File("problemfaces").remove();
  SMESH_File("occmesh.rep").remove();
}

//================================================================================
/*!
 * \brief Read mesh entities preventing successful computation from "test.out" file
 */
//================================================================================

SMESH_ComputeErrorPtr
NETGENPlugin_Mesher::ReadErrors(const vector<const SMDS_MeshNode* >& nodeVec)
{
  SMESH_ComputeErrorPtr err = SMESH_ComputeError::New
    (COMPERR_BAD_INPUT_MESH, "Some edges multiple times in surface mesh");
  SMESH_File file("test.out");
  vector<int> two(2);
  vector<int> three1(3), three2(3);
  const char* badEdgeStr = " multiple times in surface mesh";
  const int   badEdgeStrLen = strlen( badEdgeStr );

  while( !file.eof() )
  {
    if ( strncmp( file, "Edge ", 5 ) == 0 &&
         file.getInts( two ) &&
         strncmp( file, badEdgeStr, badEdgeStrLen ) == 0 &&
         two[0] < nodeVec.size() && two[1] < nodeVec.size())
    {
      err->myBadElements.push_back( new SMDS_LinearEdge( nodeVec[ two[0]], nodeVec[ two[1]] ));
      file += badEdgeStrLen;
    }
    else if ( strncmp( file, "Intersecting: ", 14 ) == 0 )
    {
// Intersecting: 
// openelement 18 with open element 126
// 41  36  38  
// 69  70  72
      file.getLine();
      const char* pos = file;
      bool ok = ( strncmp( file, "openelement ", 12 ) == 0 );
      ok = ok && file.getInts( two );
      ok = ok && file.getInts( three1 );
      ok = ok && file.getInts( three2 );
      for ( int i = 0; ok && i < 3; ++i )
        ok = ( three1[i] < nodeVec.size() && nodeVec[ three1[i]]);
      for ( int i = 0; ok && i < 3; ++i ) 
        ok = ( three2[i] < nodeVec.size() && nodeVec[ three2[i]]);
      if ( ok )
      {
        err->myBadElements.push_back( new SMDS_FaceOfNodes( nodeVec[ three1[0]],
                                                            nodeVec[ three1[1]],
                                                            nodeVec[ three1[2]]));
        err->myBadElements.push_back( new SMDS_FaceOfNodes( nodeVec[ three2[0]],
                                                            nodeVec[ three2[1]],
                                                            nodeVec[ three2[2]]));
        err->myComment = "Intersecting triangles";
      }
      else
      {
        file.setPos( pos );
      }
    }
    else
    {
      ++file;
    }
  }

#ifdef _DEBUG_
  size_t nbBadElems = err->myBadElements.size();
  nbBadElems = 0;
#endif

  return err;
}

//================================================================================
/*!
 * \brief Write a python script creating an equivalent SALOME mesh.
 * This is useful to see what mesh is passed as input for the next step of mesh
 * generation (of mesh of higher dimension)
 */
//================================================================================

void NETGENPlugin_Mesher::toPython( const netgen::Mesh* ngMesh,
                                    const std::string&  pyFile)
{
  ofstream outfile(pyFile.c_str(), ios::out);
  if ( !outfile ) return;

  outfile << "import SMESH" << std::endl
          << "from salome.smesh import smeshBuilder" << std::endl
          << "smesh = smeshBuilder.New(salome.myStudy)" << std::endl
          << "mesh = smesh.Mesh()" << std::endl << std::endl;

  using namespace netgen;
  PointIndex pi;
  for (pi = PointIndex::BASE; 
       pi < ngMesh->GetNP()+PointIndex::BASE; pi++)
  {
    outfile << "mesh.AddNode( ";
    outfile << (*ngMesh)[pi](0) << ", ";
    outfile << (*ngMesh)[pi](1) << ", ";
    outfile << (*ngMesh)[pi](2) << ") ## "<< pi << std::endl;
  }

  int nbDom = ngMesh->GetNDomains();
  for ( int i = 0; i < nbDom; ++i )
    outfile<< "grp" << i+1 << " = mesh.CreateEmptyGroup( SMESH.FACE, 'domain"<< i+1 << "')"<< std::endl;

  SurfaceElementIndex sei;
  for (sei = 0; sei < ngMesh->GetNSE(); sei++)
  {
    outfile << "mesh.AddFace([ ";
    Element2d sel = (*ngMesh)[sei];
    for (int j = 0; j < sel.GetNP(); j++)
      outfile << sel[j] << ( j+1 < sel.GetNP() ? ", " : " ])");
    if ( sel.IsDeleted() ) outfile << " ## IsDeleted ";
    outfile << std::endl;

    if ((*ngMesh)[sei].GetIndex())
    {
      if ( int dom1 = ngMesh->GetFaceDescriptor((*ngMesh)[sei].GetIndex ()).DomainIn())
        outfile << "grp"<< dom1 <<".Add([ " << (int)sei+1 << " ])" << std::endl;
      if ( int dom2 = ngMesh->GetFaceDescriptor((*ngMesh)[sei].GetIndex ()).DomainOut())
        outfile << "grp"<< dom2 <<".Add([ " << (int)sei+1 << " ])" << std::endl;
    }
  }

  for (ElementIndex ei = 0; ei < ngMesh->GetNE(); ei++)
  {
    Element el = (*ngMesh)[ei];
    outfile << "mesh.AddVolume([ ";
    for (int j = 0; j < el.GetNP(); j++)
      outfile << el[j] << ( j+1 < el.GetNP() ? ", " : " ])");
    outfile << std::endl;
  }

  for (int i = 1; i <= ngMesh->GetNSeg(); i++)
  {
    const Segment & seg = ngMesh->LineSegment (i);
    outfile << "mesh.AddEdge([ "
            << seg[0] << ", "
            << seg[1] << " ])" << std::endl;
  }
  cout << "Write " << pyFile << std::endl;
}

//================================================================================
/*!
 * \brief Constructor of NETGENPlugin_ngMeshInfo
 */
//================================================================================

NETGENPlugin_ngMeshInfo::NETGENPlugin_ngMeshInfo( netgen::Mesh* ngMesh):
  _copyOfLocalH(0)
{
  if ( ngMesh )
  {
    _nbNodes    = ngMesh->GetNP();
    _nbSegments = ngMesh->GetNSeg();
    _nbFaces    = ngMesh->GetNSE();
    _nbVolumes  = ngMesh->GetNE();
  }
  else
  {
    _nbNodes = _nbSegments = _nbFaces = _nbVolumes = 0;
  }
}

//================================================================================
/*!
 * \brief Copy LocalH member from one netgen mesh to another
 */
//================================================================================

void NETGENPlugin_ngMeshInfo::transferLocalH( netgen::Mesh* fromMesh,
                                              netgen::Mesh* toMesh )
{
  if ( !fromMesh->LocalHFunctionGenerated() ) return;
  if ( !toMesh->LocalHFunctionGenerated() )
#if NETGEN_VERSION < NETGEN_VERSION_STRING(5,0)
    toMesh->CalcLocalH();
#else
    toMesh->CalcLocalH(netgen::mparam.grading);
#endif

  const size_t size = sizeof( netgen::LocalH );
  _copyOfLocalH = new char[ size ];
  memcpy( (void*)_copyOfLocalH, (void*)&toMesh->LocalHFunction(), size );
  memcpy( (void*)&toMesh->LocalHFunction(), (void*)&fromMesh->LocalHFunction(), size );
}

//================================================================================
/*!
 * \brief Restore LocalH member of a netgen mesh
 */
//================================================================================

void NETGENPlugin_ngMeshInfo::restoreLocalH( netgen::Mesh* toMesh )
{
  if ( _copyOfLocalH )
  {
    const size_t size = sizeof( netgen::LocalH );
    memcpy( (void*)&toMesh->LocalHFunction(), (void*)_copyOfLocalH, size );
    delete [] _copyOfLocalH;
    _copyOfLocalH = 0;
  }
}

//================================================================================
/*!
 * \brief Find "internal" sub-shapes
 */
//================================================================================

NETGENPlugin_Internals::NETGENPlugin_Internals( SMESH_Mesh&         mesh,
                                                const TopoDS_Shape& shape,
                                                bool                is3D )
  : _mesh( mesh ), _is3D( is3D )
{
  SMESHDS_Mesh* meshDS = mesh.GetMeshDS();

  TopExp_Explorer f,e;
  for ( f.Init( shape, TopAbs_FACE ); f.More(); f.Next() )
  {
    int faceID = meshDS->ShapeToIndex( f.Current() );

    // find not computed internal edges

    for ( e.Init( f.Current().Oriented(TopAbs_FORWARD), TopAbs_EDGE ); e.More(); e.Next() )
      if ( e.Current().Orientation() == TopAbs_INTERNAL )
      {
        SMESH_subMesh* eSM = mesh.GetSubMesh( e.Current() );
        if ( eSM->IsEmpty() )
        {
          _e2face.insert( make_pair( eSM->GetId(), faceID ));
          for ( TopoDS_Iterator v(e.Current()); v.More(); v.Next() )
            _e2face.insert( make_pair( meshDS->ShapeToIndex( v.Value() ), faceID ));
        }
      }

    // find internal vertices in a face
    set<int> intVV; // issue 0020850 where same vertex is twice in a face
    for ( TopoDS_Iterator fSub( f.Current() ); fSub.More(); fSub.Next())
      if ( fSub.Value().ShapeType() == TopAbs_VERTEX )
      {
        int vID = meshDS->ShapeToIndex( fSub.Value() );
        if ( intVV.insert( vID ).second )
          _f2v[ faceID ].push_back( vID );
      }

    if ( is3D )
    {
      // find internal faces and their subshapes where nodes are to be doubled
      //  to make a crack with non-sewed borders

      if ( f.Current().Orientation() == TopAbs_INTERNAL )
      {
        _intShapes.insert( meshDS->ShapeToIndex( f.Current() ));

        // egdes
        list< TopoDS_Shape > edges;
        for ( e.Init( f.Current(), TopAbs_EDGE ); e.More(); e.Next())
          if ( SMESH_MesherHelper::NbAncestors( e.Current(), mesh, TopAbs_FACE ) > 1 )
          {
            _intShapes.insert( meshDS->ShapeToIndex( e.Current() ));
            edges.push_back( e.Current() );
            // find border faces
            PShapeIteratorPtr fIt =
              SMESH_MesherHelper::GetAncestors( edges.back(),mesh,TopAbs_FACE );
            while ( const TopoDS_Shape* pFace = fIt->next() )
              if ( !pFace->IsSame( f.Current() ))
                _borderFaces.insert( meshDS->ShapeToIndex( *pFace ));
          }
        // vertices
        // we consider vertex internal if it is shared by more than one internal edge
        list< TopoDS_Shape >::iterator edge = edges.begin();
        for ( ; edge != edges.end(); ++edge )
          for ( TopoDS_Iterator v( *edge ); v.More(); v.Next() )
          {
            set<int> internalEdges;
            PShapeIteratorPtr eIt =
              SMESH_MesherHelper::GetAncestors( v.Value(),mesh,TopAbs_EDGE );
            while ( const TopoDS_Shape* pEdge = eIt->next() )
            {
              int edgeID = meshDS->ShapeToIndex( *pEdge );
              if ( isInternalShape( edgeID ))
                internalEdges.insert( edgeID );
            }
            if ( internalEdges.size() > 1 )
              _intShapes.insert( meshDS->ShapeToIndex( v.Value() ));
          }
      }
    }
  } // loop on geom faces

  // find vertices internal in solids
  if ( is3D )
  {
    for ( TopExp_Explorer so(shape, TopAbs_SOLID); so.More(); so.Next())
    {
      int soID = meshDS->ShapeToIndex( so.Current() );
      for ( TopoDS_Iterator soSub( so.Current() ); soSub.More(); soSub.Next())
        if ( soSub.Value().ShapeType() == TopAbs_VERTEX )
          _s2v[ soID ].push_back( meshDS->ShapeToIndex( soSub.Value() ));
    }
  }
}

//================================================================================
/*!
 * \brief Find mesh faces on non-internal geom faces sharing internal edge
 * some nodes of which are to be doubled to make the second border of the "crack"
 */
//================================================================================

void NETGENPlugin_Internals::findBorderElements( TIDSortedElemSet & borderElems )
{
  if ( _intShapes.empty() ) return;

  SMESH_Mesh& mesh = const_cast<SMESH_Mesh&>(_mesh);
  SMESHDS_Mesh* meshDS = mesh.GetMeshDS();

  // loop on internal geom edges
  set<int>::const_iterator intShapeId = _intShapes.begin();
  for ( ; intShapeId != _intShapes.end(); ++intShapeId )
  {
    const TopoDS_Shape& s = meshDS->IndexToShape( *intShapeId );
    if ( s.ShapeType() != TopAbs_EDGE ) continue;

    // get internal and non-internal geom faces sharing the internal edge <s>
    int intFace = 0;
    set<int>::iterator bordFace = _borderFaces.end();
    PShapeIteratorPtr faces = SMESH_MesherHelper::GetAncestors( s, _mesh, TopAbs_FACE );
    while ( const TopoDS_Shape* pFace = faces->next() )
    {
      int faceID = meshDS->ShapeToIndex( *pFace );
      if ( isInternalShape( faceID ))
        intFace = faceID;
      else
        bordFace = _borderFaces.insert( faceID ).first;
    }
    if ( bordFace == _borderFaces.end() || !intFace ) continue;

    // get all links of mesh faces on internal geom face sharing nodes on edge <s>
    set< SMESH_OrientedLink > links; //!< links of faces on internal geom face
    list<const SMDS_MeshElement*> suspectFaces[2]; //!< mesh faces on border geom faces
    int nbSuspectFaces = 0;
    SMESHDS_SubMesh* intFaceSM = meshDS->MeshElements( intFace );
    if ( !intFaceSM || intFaceSM->NbElements() == 0 ) continue;
    SMESH_subMeshIteratorPtr smIt = mesh.GetSubMesh( s )->getDependsOnIterator(true,true);
    while ( smIt->more() )
    {
      SMESHDS_SubMesh* sm = smIt->next()->GetSubMeshDS();
      if ( !sm ) continue;
      SMDS_NodeIteratorPtr nIt = sm->GetNodes();
      while ( nIt->more() )
      {
        const SMDS_MeshNode* nOnEdge = nIt->next();
        SMDS_ElemIteratorPtr fIt = nOnEdge->GetInverseElementIterator(SMDSAbs_Face);
        while ( fIt->more() )
        {
          const SMDS_MeshElement* f = fIt->next();
          const int nbNodes = f->NbCornerNodes();
          if ( intFaceSM->Contains( f ))
          {
            for ( int i = 0; i < nbNodes; ++i )
              links.insert( SMESH_OrientedLink( f->GetNode(i), f->GetNode((i+1)%nbNodes)));
          }
          else
          {
            int nbDblNodes = 0;
            for ( int i = 0; i < nbNodes; ++i )
              nbDblNodes += isInternalShape( f->GetNode(i)->getshapeId() );
            if ( nbDblNodes )
              suspectFaces[ nbDblNodes < 2 ].push_back( f );
            nbSuspectFaces++;
          }
        }
      }
    }
    // suspectFaces[0] having link with same orientation as mesh faces on
    // the internal geom face are <borderElems>. suspectFaces[1] have
    // only one node on edge <s>, we decide on them later (at the 2nd loop)
    // by links of <borderElems> found at the 1st and 2nd loops
    set< SMESH_OrientedLink > borderLinks;
    for ( int isPostponed = 0; isPostponed < 2; ++isPostponed )
    {
      list<const SMDS_MeshElement*>::iterator fIt = suspectFaces[isPostponed].begin();
      for ( int nbF = 0; fIt != suspectFaces[isPostponed].end(); ++fIt, ++nbF )
      {
        const SMDS_MeshElement* f = *fIt;
        bool isBorder = false, linkFound = false, borderLinkFound = false;
        list< SMESH_OrientedLink > faceLinks;
        int nbNodes = f->NbCornerNodes();
        for ( int i = 0; i < nbNodes; ++i )
        {
          SMESH_OrientedLink link( f->GetNode(i), f->GetNode((i+1)%nbNodes));
          faceLinks.push_back( link );
          if ( !linkFound )
          {
            set< SMESH_OrientedLink >::iterator foundLink = links.find( link );
            if ( foundLink != links.end() )
            {
              linkFound= true;
              isBorder = ( foundLink->_reversed == link._reversed );
              if ( !isBorder && !isPostponed ) break;
              faceLinks.pop_back();
            }
            else if ( isPostponed && !borderLinkFound )
            {
              foundLink = borderLinks.find( link );
              if ( foundLink != borderLinks.end() )
              {
                borderLinkFound = true;
                isBorder = ( foundLink->_reversed != link._reversed );
              }
            }
          }
        }
        if ( isBorder )
        {
          borderElems.insert( f );
          borderLinks.insert( faceLinks.begin(), faceLinks.end() );
        }
        else if ( !linkFound && !borderLinkFound )
        {
          suspectFaces[1].push_back( f );
          if ( nbF > 2 * nbSuspectFaces )
            break; // dead loop protection
        }
      }
    }
  }
}

//================================================================================
/*!
 * \brief put internal shapes in maps and fill in submeshes to precompute
 */
//================================================================================

void NETGENPlugin_Internals::getInternalEdges( TopTools_IndexedMapOfShape& fmap,
                                               TopTools_IndexedMapOfShape& emap,
                                               TopTools_IndexedMapOfShape& vmap,
                                               list< SMESH_subMesh* > smToPrecompute[])
{
  if ( !hasInternalEdges() ) return;
  map<int,int>::const_iterator ev_face = _e2face.begin();
  for ( ; ev_face != _e2face.end(); ++ev_face )
  {
    const TopoDS_Shape& ev   = _mesh.GetMeshDS()->IndexToShape( ev_face->first );
    const TopoDS_Shape& face = _mesh.GetMeshDS()->IndexToShape( ev_face->second );

    ( ev.ShapeType() == TopAbs_EDGE ? emap : vmap ).Add( ev );
    fmap.Add( face );
    //cout<<"INTERNAL EDGE or VERTEX "<<ev_face->first<<" on face "<<ev_face->second<<std::endl;

    smToPrecompute[ MeshDim_1D ].push_back( _mesh.GetSubMeshContaining( ev_face->first ));
  }
}

//================================================================================
/*!
 * \brief return shapes and submeshes to be meshed and already meshed boundary submeshes
 */
//================================================================================

void NETGENPlugin_Internals::getInternalFaces( TopTools_IndexedMapOfShape& fmap,
                                               TopTools_IndexedMapOfShape& emap,
                                               list< SMESH_subMesh* >&     intFaceSM,
                                               list< SMESH_subMesh* >&     boundarySM)
{
  if ( !hasInternalFaces() ) return;

  // <fmap> and <emap> are for not yet meshed shapes
  // <intFaceSM> is for submeshes of faces
  // <boundarySM> is for meshed edges and vertices

  intFaceSM.clear();
  boundarySM.clear();

  set<int> shapeIDs ( _intShapes );
  if ( !_borderFaces.empty() )
    shapeIDs.insert( _borderFaces.begin(), _borderFaces.end() );

  set<int>::const_iterator intS = shapeIDs.begin();
  for ( ; intS != shapeIDs.end(); ++intS )
  {
    SMESH_subMesh* sm = _mesh.GetSubMeshContaining( *intS );

    if ( sm->GetSubShape().ShapeType() != TopAbs_FACE ) continue;

    intFaceSM.push_back( sm );

    // add submeshes of not computed internal faces
    if ( !sm->IsEmpty() ) continue;

    SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(true,true);
    while ( smIt->more() )
    {
      sm = smIt->next();
      const TopoDS_Shape& s = sm->GetSubShape();

      if ( sm->IsEmpty() )
      {
        // not yet meshed
        switch ( s.ShapeType() ) {
        case TopAbs_FACE: fmap.Add ( s ); break;
        case TopAbs_EDGE: emap.Add ( s ); break;
        default:;
        }
      }
      else
      {
        if ( s.ShapeType() != TopAbs_FACE )
          boundarySM.push_back( sm );
      }
    }
  }
}

//================================================================================
/*!
 * \brief Return true if given shape is to be precomputed in order to be correctly
 * added to netgen mesh
 */
//================================================================================

bool NETGENPlugin_Internals::isShapeToPrecompute(const TopoDS_Shape& s)
{
  int shapeID = _mesh.GetMeshDS()->ShapeToIndex( s );
  switch ( s.ShapeType() ) {
  case TopAbs_FACE  : break; //return isInternalShape( shapeID ) || isBorderFace( shapeID );
  case TopAbs_EDGE  : return isInternalEdge( shapeID );
  case TopAbs_VERTEX: break;
  default:;
  }
  return false;
}

//================================================================================
/*!
 * \brief Return SMESH
 */
//================================================================================

SMESH_Mesh& NETGENPlugin_Internals::getMesh() const
{
  return const_cast<SMESH_Mesh&>( _mesh );
}

//================================================================================
/*!
 * \brief Initialize netgen library
 */
//================================================================================

NETGENPlugin_NetgenLibWrapper::NETGENPlugin_NetgenLibWrapper()
{
  Ng_Init();

  _isComputeOk      = false;
  _coutBuffer       = NULL;
  if ( !getenv( "KEEP_NETGEN_OUTPUT" ))
  {
    // redirect all netgen output (mycout,myerr,cout) to _outputFileName
    _outputFileName = getOutputFileName();
    netgen::mycout  = new ofstream ( _outputFileName.c_str() );
    netgen::myerr   = netgen::mycout;
    _coutBuffer     = std::cout.rdbuf();
#ifdef _DEBUG_
    cout << "NOTE: netgen output is redirected to file " << _outputFileName << std::endl;
#else
    std::cout.rdbuf( netgen::mycout->rdbuf() );
#endif
  }
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
  _ngMesh = Ng_NewMesh();
#else
  _ngMesh = std::make_shared<Ng_Mesh>();
#endif
}

//================================================================================
/*!
 * \brief Finish using netgen library
 */
//================================================================================

NETGENPlugin_NetgenLibWrapper::~NETGENPlugin_NetgenLibWrapper()
{
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
  Ng_DeleteMesh( _ngMesh );
#else
  _ngMesh.reset();
#endif
  Ng_Exit();
  NETGENPlugin_Mesher::RemoveTmpFiles();
  if ( _coutBuffer )
    std::cout.rdbuf( _coutBuffer );
#ifdef _DEBUG_
  if( _isComputeOk )
#endif
    removeOutputFile();
}

//================================================================================
/*!
 * \brief Set netgen mesh to delete at destruction
 */
//================================================================================

void NETGENPlugin_NetgenLibWrapper::setMesh( Ng_Mesh* mesh )
{
  if ( _ngMesh )
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0)
    Ng_DeleteMesh( _ngMesh );
  _ngMesh = mesh;
#else
    _ngMesh = std::make_shared<Ng_Mesh>(mesh);
#endif
}

//================================================================================
/*!
 * \brief Return a unique file name
 */
//================================================================================

std::string NETGENPlugin_NetgenLibWrapper::getOutputFileName()
{
//  std::string aTmpDir = SALOMEDS_Tool::GetTmpDir();
  std::string aTmpDir = "/tmp";

  TCollection_AsciiString aGenericName = (char*)aTmpDir.c_str();
  aGenericName += "NETGEN_";
#ifndef WIN32
  aGenericName += getpid();
#else
  aGenericName += _getpid();
#endif
  aGenericName += "_";
  aGenericName += Abs((Standard_Integer)(long) aGenericName.ToCString());
  aGenericName += ".out";

  return aGenericName.ToCString();
}

//================================================================================
/*!
 * \brief Remove file with netgen output
 */
//================================================================================

void NETGENPlugin_NetgenLibWrapper::removeOutputFile()
{
/*  if ( !_outputFileName.empty() )
  {
    if ( netgen::mycout )
    {
      delete netgen::mycout;
      netgen::mycout = 0;
      netgen::myerr = 0;
    }
    string    tmpDir = SALOMEDS_Tool::GetDirFromPath ( _outputFileName );
    string aFileName = SALOMEDS_Tool::GetNameFromPath( _outputFileName ) + ".out";
    SALOMEDS::ListOfFileNames_var aFiles = new SALOMEDS::ListOfFileNames;
    aFiles->length(1);
    aFiles[0] = aFileName.c_str();

    SALOMEDS_Tool::RemoveTemporaryFiles( tmpDir.c_str(), aFiles.in(), true );
  }
*/
}
