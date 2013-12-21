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
//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_Mesher.cxx
// Author    : Michael Sazonov (OCN)
// Date      : 31/03/2006
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_Mesher.hxx"
#include "NETGENPlugin_Hypothesis_2D.hxx"
#include "NETGENPlugin_SimpleHypothesis_3D.hxx"

#include <SMESH_Mesh.hxx>
#include <SMESH_Comment.hxx>
#include <SMESH_ComputeError.hxx>
#include <SMESH_subMesh.hxx>
#include <SMESH_MesherHelper.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMDS_MeshNode.hxx>
#include <utilities.h>

#include <vector>

#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <NCollection_Map.hxx>
#include <OSD_Path.hxx>
#include <OSD_File.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <Standard_ErrorHandler.hxx>

// Netgen include files
namespace nglib {
#include <nglib.h>
}
#define OCCGEOMETRY
#include <occgeom.hpp>
#include <meshing.hpp>
//#include <ngexception.hpp>
namespace netgen {
#ifdef NETGEN_V5
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, MeshingParameters&, int, int);
  DLL_HEADER extern MeshingParameters mparam;
#else
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, int, int, char*);
#endif
}

using namespace std;

//=============================================================================
/*!
 *
 */
//=============================================================================

NETGENPlugin_Mesher::NETGENPlugin_Mesher (SMESH_Mesh* mesh,
                                          const TopoDS_Shape& aShape,
                                          const bool isVolume)
  : _mesh    (mesh),
    _shape   (aShape),
    _isVolume(isVolume),
    _optimize(true),
    _simpleHyp(NULL)
{
  defaultParameters();
}

//================================================================================
/*!
 * \brief Initialize global NETGEN parameters with default values
 */
//================================================================================

void NETGENPlugin_Mesher::defaultParameters()
{
//#ifdef WNT
//  netgen::MeshingParameters& mparams = netgen::GlobalMeshingParameters();
//#else
  netgen::MeshingParameters& mparams = netgen::mparam;
//#endif
  // maximal mesh edge size
  mparams.maxh = NETGENPlugin_Hypothesis::GetDefaultMaxSize();
  // minimal number of segments per edge
  mparams.segmentsperedge = NETGENPlugin_Hypothesis::GetDefaultNbSegPerEdge();
  // rate of growth of size between elements
  mparams.grading = NETGENPlugin_Hypothesis::GetDefaultGrowthRate();
  // safety factor for curvatures (elements per radius)
  mparams.curvaturesafety = NETGENPlugin_Hypothesis::GetDefaultNbSegPerRadius();
  // create elements of second order
  mparams.secondorder = NETGENPlugin_Hypothesis::GetDefaultSecondOrder() ? 1 : 0;
  // quad-dominated surface meshing
  if (_isVolume)
    mparams.quad = 0;
  else
    mparams.quad = NETGENPlugin_Hypothesis_2D::GetDefaultQuadAllowed() ? 1 : 0;
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
//#ifdef WNT
//    netgen::MeshingParameters& mparams = netgen::GlobalMeshingParameters();
//#else
    netgen::MeshingParameters& mparams = netgen::mparam;
//#endif
    // Initialize global NETGEN parameters:
    // maximal mesh segment size
    mparams.maxh = hyp->GetMaxSize();
    // minimal number of segments per edge
    mparams.segmentsperedge = hyp->GetNbSegPerEdge();
    // rate of growth of size between elements
    mparams.grading = hyp->GetGrowthRate();
    // safety factor for curvatures (elements per radius)
    mparams.curvaturesafety = hyp->GetNbSegPerRadius();
    // create elements of second order
    mparams.secondorder = hyp->GetSecondOrder() ? 1 : 0;
    // quad-dominated surface meshing
    // only triangles are allowed for volumic mesh
    if (!_isVolume)
      mparams.quad = static_cast<const NETGENPlugin_Hypothesis_2D*>
        (hyp)->GetQuadAllowed() ? 1 : 0;
    _optimize = hyp->GetOptimize();
    _simpleHyp = NULL;
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
    defaultParameters();
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
};

int HashCode(const Link& aLink, int aLimit)
{
  return HashCode(aLink.n1 + aLink.n2, aLimit);
}

Standard_Boolean IsEqual(const Link& aLink1, const Link& aLink2)
{
  return (aLink1.n1 == aLink2.n1 && aLink1.n2 == aLink2.n2 ||
          aLink1.n1 == aLink2.n2 && aLink1.n2 == aLink2.n1);
}

//================================================================================
/*!
 * \brief Initialize netgen::OCCGeometry with OCCT shape
 */
//================================================================================

void NETGENPlugin_Mesher::PrepareOCCgeometry(netgen::OCCGeometry&     occgeo,
                                             const TopoDS_Shape&      shape,
                                             SMESH_Mesh&              mesh,
                                             list< SMESH_subMesh* > * meshedSM)
{
  BRepTools::Clean (shape);
  try {
#if (OCC_VERSION_MAJOR << 16 | OCC_VERSION_MINOR << 8 | OCC_VERSION_MAINTENANCE) > 0x060100
    OCC_CATCH_SIGNALS;
#endif
    BRepMesh_IncrementalMesh e(shape, 0.01, true);
  } catch (Standard_Failure) {
  }
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
  //occgeo.BuildFMap();

  // fill maps of shapes of occgeo with not yet meshed subshapes

  // get root submeshes
  list< SMESH_subMesh* > rootSM;
  if ( SMESH_subMesh* sm = mesh.GetSubMeshContaining( shape )) {
    rootSM.push_back( sm );
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
    while ( smIt->more() ) {
      SMESH_subMesh* sm = smIt->next();
      if ( sm->IsEmpty() ) {
        TopoDS_Shape shape = sm->GetSubShape();
        if ( shape.ShapeType() != TopAbs_VERTEX )
          shape = subShapes( subShapes.FindIndex( shape ));// - shape->index->oriented shape
        switch ( shape.ShapeType() ) {
        case TopAbs_FACE  : occgeo.fmap.Add( shape ); break;
        case TopAbs_EDGE  : occgeo.emap.Add( shape ); break;
        case TopAbs_VERTEX: occgeo.vmap.Add( shape ); break;
        case TopAbs_SOLID :occgeo.somap.Add( shape ); break;
        default:;
        }
      }
      // collect submeshes of meshed shapes
      else if (meshedSM) {
        meshedSM->push_back( sm );
      }
    }
  }
  occgeo.facemeshstatus.SetSize (occgeo.fmap.Extent());
  occgeo.facemeshstatus = 0;

  occgeo.face_maxh.DeleteAll();
  occgeo.face_maxh.SetSize (occgeo.fmap.Extent());
  occgeo.face_maxh = netgen::mparam.maxh;


}

//================================================================================
/*!
 * \brief return id of netgen point corresponding to SMDS node
 */
//================================================================================
typedef map< const SMDS_MeshNode*, int > TNode2IdMap;

static int ngNodeId( const SMDS_MeshNode* node,
                     netgen::Mesh&        ngMesh,
                     TNode2IdMap&         nodeNgIdMap)
{
  int newNgId = ngMesh.GetNP() + 1;

  pair< TNode2IdMap::iterator, bool > it_isNew = nodeNgIdMap.insert( make_pair( node, newNgId ));

  if ( it_isNew.second ) {
    netgen::MeshPoint p( netgen::Point<3> (node->X(), node->Y(), node->Z()) );
    ngMesh.AddPoint( p );
  }
  return it_isNew.first->second;
}

//================================================================================
/*!
 * \brief fill ngMesh with nodes and elements of computed submeshes
 */
//================================================================================

bool NETGENPlugin_Mesher::fillNgMesh(netgen::OCCGeometry&           occgeom,
                                     netgen::Mesh&                  ngMesh,
                                     vector<SMDS_MeshNode*>&        nodeVec,
                                     const list< SMESH_subMesh* > & meshedSM)
{
  TNode2IdMap nodeNgIdMap;

  TopTools_MapOfShape visitedShapes;

  SMESH_MesherHelper helper (*_mesh);

  int faceID = occgeom.fmap.Extent();
  _faceDescriptors.clear();

  list< SMESH_subMesh* >::const_iterator smIt, smEnd = meshedSM.end();
  for ( smIt = meshedSM.begin(); smIt != smEnd; ++smIt )
  {
    SMESH_subMesh* sm = *smIt;
    if ( !visitedShapes.Add( sm->GetSubShape() ))
      continue;

    SMESHDS_SubMesh * smDS = sm->GetSubMeshDS();

    switch ( sm->GetSubShape().ShapeType() )
    {
    case TopAbs_EDGE: { // EDGE
      // ----------------------
      const TopoDS_Edge& geomEdge  = TopoDS::Edge( sm->GetSubShape() );

      // Add ng segments for each not meshed face the edge bounds
      TopTools_MapOfShape visitedAncestors;
      const TopTools_ListOfShape& ancestors = _mesh->GetAncestors( geomEdge );
      TopTools_ListIteratorOfListOfShape ancestorIt ( ancestors );
      for ( ; ancestorIt.More(); ancestorIt.Next() )
      {
        const TopoDS_Shape & ans = ancestorIt.Value();
        if ( ans.ShapeType() != TopAbs_FACE || !visitedAncestors.Add( ans ))
          continue;
        const TopoDS_Face& face = TopoDS::Face( ans );

        int faceID = occgeom.fmap.FindIndex( face );
        if ( faceID < 1 )
          continue; // meshed face

        // find out orientation of geomEdge within face
        bool isForwad = false;
        for ( TopExp_Explorer exp( face, TopAbs_EDGE ); exp.More(); exp.Next() ) {
          if ( geomEdge.IsSame( exp.Current() )) {
            isForwad = ( exp.Current().Orientation() == geomEdge.Orientation() );
            break;
          }
        }
        bool isQuad = smDS->GetElements()->next()->IsQuadratic();

        // get all nodes from geomEdge
        StdMeshers_FaceSide fSide( face, geomEdge, _mesh, isForwad, isQuad );
        const vector<UVPtStruct>& points = fSide.GetUVPtStruct();
        int i, nbSeg = fSide.NbSegments();

        double otherSeamParam = 0;
        helper.SetSubShape( face );
        bool isSeam = helper.IsRealSeam( geomEdge );
        if ( isSeam )
          otherSeamParam =
            helper.GetOtherParam( helper.GetPeriodicIndex() == 1 ? points[0].u : points[0].v );

        // add segments

        int prevNgId = ngNodeId( points[0].node, ngMesh, nodeNgIdMap );

        for ( i = 0; i < nbSeg; ++i )
        {
          const UVPtStruct& p1 = points[ i ];
          const UVPtStruct& p2 = points[ i+1 ];

          netgen::Segment seg;
          // ng node ids
          seg.pnums[0] = prevNgId;
          seg.pnums[1] = prevNgId = ngNodeId( p2.node, ngMesh, nodeNgIdMap );
          // node param on curve
          seg.epgeominfo[ 0 ].dist = p1.param;
          seg.epgeominfo[ 1 ].dist = p2.param;
          // uv on face
          seg.epgeominfo[ 0 ].u = p1.u;
          seg.epgeominfo[ 0 ].v = p1.v;
          seg.epgeominfo[ 1 ].u = p2.u;
          seg.epgeominfo[ 1 ].v = p2.v;

          //seg.epgeominfo[ iEnd ].edgenr = edgeID; //  = geom.emap.FindIndex(edge);
          seg.si = faceID;                   // = geom.fmap.FindIndex (face);
          seg.edgenr = ngMesh.GetNSeg() + 1; // segment id
          ngMesh.AddSegment (seg);

          if ( isSeam )
          {
            if ( helper.GetPeriodicIndex() == 1 ) {
              seg.epgeominfo[ 0 ].u = otherSeamParam;
              seg.epgeominfo[ 1 ].u = otherSeamParam;
              swap (seg.epgeominfo[0].v, seg.epgeominfo[1].v);
            } else {
              seg.epgeominfo[ 0 ].v = otherSeamParam;
              seg.epgeominfo[ 1 ].v = otherSeamParam;
              swap (seg.epgeominfo[0].u, seg.epgeominfo[1].u);
            }
            swap (seg.pnums[0], seg.pnums[1]);
            swap (seg.epgeominfo[0].dist, seg.epgeominfo[1].dist);
            seg.edgenr = ngMesh.GetNSeg() + 1; // segment id
            ngMesh.AddSegment (seg);
          }
        }
      } // loop on geomEdge ancestors

      break;
    } // case TopAbs_EDGE

    case TopAbs_FACE: { // FACE
      // ----------------------
      const TopoDS_Face& geomFace  = TopoDS::Face( sm->GetSubShape() );
      helper.SetSubShape( geomFace );

      // Find solids the geomFace bounds
      int solidID1 = 0, solidID2 = 0;
      const TopTools_ListOfShape& ancestors = _mesh->GetAncestors( geomFace );
      TopTools_ListIteratorOfListOfShape ancestorIt ( ancestors );
      for ( ; ancestorIt.More(); ancestorIt.Next() )
      {
        const TopoDS_Shape & solid = ancestorIt.Value();
        if ( solid.ShapeType() == TopAbs_SOLID  ) {
          int id = occgeom.somap.FindIndex ( solid );
          if ( solidID1 && id != solidID1 ) solidID2 = id;
          else                              solidID1 = id;
        }
      }
      faceID++;
      _faceDescriptors[ faceID ].first  = solidID1;
      _faceDescriptors[ faceID ].second = solidID2;

      // Orient the face correctly in solidID1 (issue 0020206)
      bool reverse = false;
      if ( solidID1 ) {
        TopoDS_Shape solid = occgeom.somap( solidID1 );
        for ( TopExp_Explorer f( solid, TopAbs_FACE ); f.More(); f.Next() ) {
          if ( geomFace.IsSame( f.Current() )) {
            reverse = SMESH_Algo::IsReversedSubMesh( TopoDS::Face( f.Current()), helper.GetMeshDS() );
            break;
          }
        }
      }

      // Add surface elements
      SMDS_ElemIteratorPtr faces = smDS->GetElements();
      while ( faces->more() ) {

        const SMDS_MeshElement* f = faces->next();
        if ( f->NbNodes() % 3 != 0 ) { // not triangle
          for ( ancestorIt.Initialize(ancestors); ancestorIt.More(); ancestorIt.Next() )
            if ( ancestorIt.Value().ShapeType() == TopAbs_SOLID  ) {
              sm = _mesh->GetSubMesh( ancestorIt.Value() );
              break;
            }
          SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
          smError.reset( new SMESH_ComputeError(COMPERR_BAD_INPUT_MESH,"Not triangle submesh"));
          smError->myBadElements.push_back( f );
          return false;
        }

        netgen::Element2d tri(3);
        tri.SetIndex ( faceID );

        for ( int i = 0; i < 3; ++i ) {
          const SMDS_MeshNode* node = f->GetNode( i ), * inFaceNode=0;
          if ( helper.IsSeamShape( node->GetPosition()->GetShapeId() ))
            if ( helper.IsSeamShape( f->GetNodeWrap( i+1 )->GetPosition()->GetShapeId() ))
              inFaceNode = f->GetNodeWrap( i-1 );
            else 
              inFaceNode = f->GetNodeWrap( i+1 );

          gp_XY uv = helper.GetNodeUV( geomFace, node, inFaceNode );
          if ( reverse ) {
            tri.GeomInfoPi(3-i).u = uv.X();
            tri.GeomInfoPi(3-i).v = uv.Y();
            tri.PNum      (3-i) = ngNodeId( node, ngMesh, nodeNgIdMap );
          } else {
            tri.GeomInfoPi(i+1).u = uv.X();
            tri.GeomInfoPi(i+1).v = uv.Y();
            tri.PNum      (i+1) = ngNodeId( node, ngMesh, nodeNgIdMap );
          }
        }

        ngMesh.AddSurfaceElement (tri);

      }
      break;
    } //

    case TopAbs_VERTEX: { // VERTEX
      // --------------------------
      SMDS_NodeIteratorPtr nodeIt = smDS->GetNodes();
      if ( nodeIt->more() )
        ngNodeId( nodeIt->next(), ngMesh, nodeNgIdMap );
      break;
    }
    default:;
    } // switch
  } // loop on submeshes

  // fill nodeVec
  nodeVec.resize( ngMesh.GetNP() + 1 );
  TNode2IdMap::iterator node_NgId, nodeNgIdEnd = nodeNgIdMap.end();
  for ( node_NgId = nodeNgIdMap.begin(); node_NgId != nodeNgIdEnd; ++node_NgId)
    nodeVec[ node_NgId->second ] = (SMDS_MeshNode*) node_NgId->first;

  return true;
}

//=============================================================================
/*!
 * Here we are going to use the NETGEN mesher
 */
//=============================================================================
bool NETGENPlugin_Mesher::Compute()
{
//#ifdef WNT
//  netgen::MeshingParameters& mparams = netgen::GlobalMeshingParameters();
//#else
  netgen::MeshingParameters& mparams = netgen::mparam;
//#endif  
  MESSAGE("Compute with:\n"
          " max size = " << mparams.maxh << "\n"
          " segments per edge = " << mparams.segmentsperedge);
  MESSAGE("\n"
          " growth rate = " << mparams.grading << "\n"
          " elements per radius = " << mparams.curvaturesafety << "\n"
          " second order = " << mparams.secondorder << "\n"
          " quad allowed = " << mparams.quad);

  SMESH_ComputeErrorPtr error = SMESH_ComputeError::New();
  nglib::Ng_Init();

  // -------------------------
  // Prepare OCC geometry
  // -------------------------

  netgen::OCCGeometry occgeo;
  list< SMESH_subMesh* > meshedSM;
  PrepareOCCgeometry( occgeo, _shape, *_mesh, &meshedSM );

  // -------------------------
  // Generate the mesh
  // -------------------------

  netgen::Mesh *ngMesh = NULL;

  SMESH_Comment comment;
  int err = 0;
  int nbInitNod = 0;
  int nbInitSeg = 0;
  int nbInitFac = 0;
  // vector of nodes in which node index == netgen ID
  vector< SMDS_MeshNode* > nodeVec;
  try
  {
    // ----------------
    // compute 1D mesh
    // ----------------
    // pass 1D simple parameters to NETGEN
    if ( _simpleHyp ) {
      if ( int nbSeg = _simpleHyp->GetNumberOfSegments() ) {
        // nb of segments
        mparams.segmentsperedge = nbSeg + 0.1;
        mparams.maxh = occgeo.boundingbox.Diam();
        mparams.grading = 0.01;
      }
      else {
        // segment length
        mparams.segmentsperedge = 1;
        mparams.maxh = _simpleHyp->GetLocalLength();
      }
    }
    // let netgen create ngMesh and calculate element size on not meshed shapes
#ifndef NETGEN_V5
    char *optstr = 0;
#endif
    int startWith = netgen::MESHCONST_ANALYSE;
    int endWith   = netgen::MESHCONST_ANALYSE;
#ifdef NETGEN_V5
    err = netgen::OCCGenerateMesh(occgeo, ngMesh,mparams, startWith, endWith);
#else
    err = netgen::OCCGenerateMesh(occgeo, ngMesh, startWith, endWith, optstr);
#endif
    if (err) comment << "Error in netgen::OCCGenerateMesh() at MESHCONST_ANALYSE step";

    // fill ngMesh with nodes and elements of computed submeshes
    err = ! fillNgMesh(occgeo, *ngMesh, nodeVec, meshedSM);
    nbInitNod = ngMesh->GetNP();
    nbInitSeg = ngMesh->GetNSeg();
    nbInitFac = ngMesh->GetNSE();

    // compute mesh
    if (!err)
    {
      startWith = endWith = netgen::MESHCONST_MESHEDGES;
#ifdef NETGEN_V5
      err = netgen::OCCGenerateMesh(occgeo, ngMesh,mparams, startWith, endWith);
#else
      err = netgen::OCCGenerateMesh(occgeo, ngMesh, startWith, endWith, optstr);
#endif
      if (err) comment << "Error in netgen::OCCGenerateMesh() at 1D mesh generation";
    }
    // ---------------------
    // compute surface mesh
    // ---------------------
    if (!err)
    {
      // pass 2D simple parameters to NETGEN
      if ( _simpleHyp ) {
        if ( double area = _simpleHyp->GetMaxElementArea() ) {
          // face area
          mparams.maxh = sqrt(2. * area/sqrt(3.0));
          mparams.grading = 0.4; // moderate size growth
        }
        else {
          // length from edges
          double length = 0;
          for ( TopExp_Explorer exp( _shape, TopAbs_EDGE ); exp.More(); exp.Next() )
            length += SMESH_Algo::EdgeLength( TopoDS::Edge( exp.Current() ));
          if ( ngMesh->GetNSeg() )
            mparams.maxh = length / ngMesh->GetNSeg();
          else
            mparams.maxh = 1000;
          mparams.grading = 0.2; // slow size growth
        }
        mparams.maxh = min( mparams.maxh, occgeo.boundingbox.Diam()/2 );
        ngMesh->SetGlobalH (mparams.maxh);
	netgen::Box<3> bb = occgeo.GetBoundingBox();
	bb.Increase (bb.Diam()/20);
        ngMesh->SetLocalH (bb.PMin(), bb.PMax(), mparams.grading);
      }
      // let netgen compute 2D mesh
      startWith = netgen::MESHCONST_MESHSURFACE;
      endWith = _optimize ? netgen::MESHCONST_OPTSURFACE : netgen::MESHCONST_MESHSURFACE;
#ifdef NETGEN_V5
      err = netgen::OCCGenerateMesh(occgeo, ngMesh,mparams, startWith, endWith);
#else
      err = netgen::OCCGenerateMesh(occgeo, ngMesh, startWith, endWith, optstr);
#endif
      if (err) comment << "Error in netgen::OCCGenerateMesh() at surface mesh generation";
    }
    // ---------------------
    // generate volume mesh
    // ---------------------
    if (!err && _isVolume)
    {
      // add ng face descriptors of meshed faces
      std::map< int, std::pair<int,int> >::iterator fId_soIds = _faceDescriptors.begin();
      for ( ; fId_soIds != _faceDescriptors.end(); ++fId_soIds ) {
        int faceID   = fId_soIds->first;
        int solidID1 = fId_soIds->second.first;
        int solidID2 = fId_soIds->second.second;
        ngMesh->AddFaceDescriptor (netgen::FaceDescriptor(faceID, solidID1, solidID2, 0));
      }
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
          // length from faces
          mparams.maxh = ngMesh->AverageH();
        }
// 	netgen::ARRAY<double> maxhdom;
// 	maxhdom.SetSize (occgeo.NrSolids());
// 	maxhdom = mparams.maxh;
// 	ngMesh->SetMaxHDomain (maxhdom);
        ngMesh->SetGlobalH (mparams.maxh);
        mparams.grading = 0.4;
#ifdef NETGEN_V5
        ngMesh->CalcLocalH(mparams.grading);
#else
        ngMesh->CalcLocalH();
#endif
      }
      // let netgen compute 3D mesh
      startWith = netgen::MESHCONST_MESHVOLUME;
      endWith = _optimize ? netgen::MESHCONST_OPTVOLUME : netgen::MESHCONST_MESHVOLUME;
#ifdef NETGEN_V5
      err = netgen::OCCGenerateMesh(occgeo, ngMesh,mparams, startWith, endWith);
#else
      err = netgen::OCCGenerateMesh(occgeo, ngMesh, startWith, endWith, optstr);
#endif
      if (err) comment << "Error in netgen::OCCGenerateMesh()";
    }
    if (!err && mparams.secondorder > 0)
    {
      netgen::OCCRefinementSurfaces ref (occgeo);
      ref.MakeSecondOrder (*ngMesh);
    }
  }
  catch (netgen::NgException exc)
  {
    error->myName = err = COMPERR_ALGO_FAILED;
    comment << exc.What();
  }

  int nbNod = ngMesh->GetNP();
  int nbSeg = ngMesh->GetNSeg();
  int nbFac = ngMesh->GetNSE();
  int nbVol = ngMesh->GetNE();

  MESSAGE((err ? "Mesh Generation failure" : "End of Mesh Generation") <<
          ", nb nodes: " << nbNod <<
          ", nb segments: " << nbSeg <<
          ", nb faces: " << nbFac <<
          ", nb volumes: " << nbVol);

  // -----------------------------------------------------------
  // Feed back the SMESHDS with the generated Nodes and Elements
  // -----------------------------------------------------------

  SMESHDS_Mesh* meshDS = _mesh->GetMeshDS();
  bool isOK = ( !err && (_isVolume ? (nbVol > 0) : (nbFac > 0)) );
  if ( true /*isOK*/ ) // get whatever built
  {
    // map of nodes assigned to submeshes
    NCollection_Map<int> pindMap;
    // create and insert nodes into nodeVec
    nodeVec.resize( nbNod + 1 );
    int i;
    for (i = nbInitNod+1; i <= nbNod /*&& isOK*/; ++i )
    {
      const netgen::MeshPoint& ngPoint = ngMesh->Point(i);
      SMDS_MeshNode* node = NULL;
      bool newNodeOnVertex = false;
      TopoDS_Vertex aVert;
      if (i-nbInitNod <= occgeo.vmap.Extent())
      {
        // point on vertex
        aVert = TopoDS::Vertex(occgeo.vmap(i-nbInitNod));
        SMESHDS_SubMesh * submesh = meshDS->MeshElements(aVert);
        if (submesh)
        {
          SMDS_NodeIteratorPtr it = submesh->GetNodes();
          if (it->more())
          {
            node = const_cast<SMDS_MeshNode*> (it->next());
            pindMap.Add(i);
          }
        }
        if (!node)
          newNodeOnVertex = true;
      }
      if (!node)
        node = meshDS->AddNode(ngPoint(0), ngPoint(1), ngPoint(2));
      if (!node)
      {
        MESSAGE("Cannot create a mesh node");
        if ( !comment.size() ) comment << "Cannot create a mesh node";
        nbSeg = nbFac = nbVol = isOK = 0;
        break;
      }
      nodeVec.at(i) = node;
      if (newNodeOnVertex)
      {
        // point on vertex
        meshDS->SetNodeOnVertex(node, aVert);
        pindMap.Add(i);
      }
    }

    // create mesh segments along geometric edges
    NCollection_Map<Link> linkMap;
    for (i = nbInitSeg+1; i <= nbSeg/* && isOK*/; ++i )
    {
      const netgen::Segment& seg = ngMesh->LineSegment(i);
      Link link(seg.pnums[0], seg.pnums[1]);
      if (linkMap.Contains(link))
        continue;
      linkMap.Add(link);
      TopoDS_Edge aEdge;
      int pinds[3] = { seg.pnums[0], seg.pnums[1], seg.pnums[2] };
      int nbp = 0;
      double param2 = 0;
      for (int j=0; j < 3; ++j)
      {
        int pind = pinds[j];
        if (pind <= 0) continue;
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
        else
          param = param2 * 0.5;
        if (pind <= nbInitNod || pindMap.Contains(pind))
          continue;
        if (!aEdge.IsNull())
        {
          meshDS->SetNodeOnEdge(nodeVec.at(pind), aEdge, param);
          pindMap.Add(pind);
        }
      }
      SMDS_MeshEdge* edge;
      if (nbp < 3) // second order ?
        edge = meshDS->AddEdge(nodeVec.at(pinds[0]), nodeVec.at(pinds[1]));
      else
        edge = meshDS->AddEdge(nodeVec.at(pinds[0]), nodeVec.at(pinds[1]),
                                nodeVec.at(pinds[2]));
      if (!edge)
      {
        if ( !comment.size() ) comment << "Cannot create a mesh edge";
        MESSAGE("Cannot create a mesh edge");
        nbSeg = nbFac = nbVol = isOK = 0;
        break;
      }
      if (!aEdge.IsNull())
        meshDS->SetMeshElementOnShape(edge, aEdge);
    }

    // create mesh faces along geometric faces
    for (i = nbInitFac+1; i <= nbFac/* && isOK*/; ++i )
    {
      const netgen::Element2d& elem = ngMesh->SurfaceElement(i);
      int aGeomFaceInd = elem.GetIndex();
      TopoDS_Face aFace;
      if (aGeomFaceInd > 0 && aGeomFaceInd <= occgeo.fmap.Extent())
        aFace = TopoDS::Face(occgeo.fmap(aGeomFaceInd));
      vector<SMDS_MeshNode*> nodes;
      for (int j=1; j <= elem.GetNP(); ++j)
      {
        int pind = elem.PNum(j);
        SMDS_MeshNode* node = nodeVec.at(pind);
        nodes.push_back(node);
        if (pind <= nbInitNod || pindMap.Contains(pind))
          continue;
        if (!aFace.IsNull())
        {
          const netgen::PointGeomInfo& pgi = elem.GeomInfoPi(j);
          meshDS->SetNodeOnFace(node, aFace, pgi.u, pgi.v);
          pindMap.Add(pind);
        }
      }
      SMDS_MeshFace* face = NULL;
      switch (elem.GetType())
      {
      case netgen::TRIG:
        face = meshDS->AddFace(nodes[0],nodes[1],nodes[2]);
        break;
      case netgen::QUAD:
        face = meshDS->AddFace(nodes[0],nodes[1],nodes[2],nodes[3]);
        break;
      case netgen::TRIG6:
        face = meshDS->AddFace(nodes[0],nodes[1],nodes[2],nodes[5],nodes[3],nodes[4]);
        break;
      case netgen::QUAD8:
        face = meshDS->AddFace(nodes[0],nodes[1],nodes[2],nodes[3],
                               nodes[4],nodes[7],nodes[5],nodes[6]);
        break;
      default:
        MESSAGE("NETGEN created a face of unexpected type, ignoring");
        continue;
      }
      if (!face)
      {
        if ( !comment.size() ) comment << "Cannot create a mesh face";
        MESSAGE("Cannot create a mesh face");
        nbSeg = nbFac = nbVol = isOK = 0;
        break;
      }
      if (!aFace.IsNull())
        meshDS->SetMeshElementOnShape(face, aFace);
    }

    // create tetrahedra
    for (i = 1; i <= nbVol/* && isOK*/; ++i)
    {
      const netgen::Element& elem = ngMesh->VolumeElement(i);      
      int aSolidInd = elem.GetIndex();
      TopoDS_Solid aSolid;
      if (aSolidInd > 0 && aSolidInd <= occgeo.somap.Extent())
        aSolid = TopoDS::Solid(occgeo.somap(aSolidInd));
      vector<SMDS_MeshNode*> nodes;
      for (int j=1; j <= elem.GetNP(); ++j)
      {
        int pind = elem.PNum(j);
        SMDS_MeshNode* node = nodeVec.at(pind);
        nodes.push_back(node);
        if (pind <= nbInitNod || pindMap.Contains(pind))
          continue;
        if (!aSolid.IsNull())
        {
          // point in solid
          meshDS->SetNodeInVolume(node, aSolid);
          pindMap.Add(pind);
        }
      }
      SMDS_MeshVolume* vol = NULL;
      switch (elem.GetType())
      {
      case netgen::TET:
        vol = meshDS->AddVolume(nodes[0],nodes[1],nodes[2],nodes[3]);
        break;
      case netgen::TET10:
        vol = meshDS->AddVolume(nodes[0],nodes[1],nodes[2],nodes[3],
                                nodes[4],nodes[7],nodes[5],nodes[6],nodes[8],nodes[9]);
        break;
      default:
        MESSAGE("NETGEN created a volume of unexpected type, ignoring");
        continue;
      }
      if (!vol)
      {
        if ( !comment.size() ) comment << "Cannot create a mesh volume";
        MESSAGE("Cannot create a mesh volume");
        nbSeg = nbFac = nbVol = isOK = 0;
        break;
      }
      if (!aSolid.IsNull())
        meshDS->SetMeshElementOnShape(vol, aSolid);
    }
  }

  if ( error->IsOK() && ( !isOK || comment.size() > 0 ))
    error->myName = COMPERR_ALGO_FAILED;
  if ( !comment.empty() )
    error->myComment = comment;

  // set bad compute error to subshapes of all failed subshapes shapes
  if ( !error->IsOK() && err )
  {
    for (int i = 1; i <= occgeo.fmap.Extent(); i++) {
      int status = occgeo.facemeshstatus[i-1];
      if (status == 1 ) continue;
      if ( SMESH_subMesh* sm = _mesh->GetSubMeshContaining( occgeo.fmap( i ))) {
        SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
        if ( !smError || smError->IsOK() ) {
          if ( status == -1 )
            smError.reset( new SMESH_ComputeError( error->myName, error->myComment ));
          else
            smError.reset( new SMESH_ComputeError( COMPERR_ALGO_FAILED, "Ignored" ));
        }
      }
    }
  }

  nglib::Ng_DeleteMesh((nglib::Ng_Mesh*)ngMesh);
  nglib::Ng_Exit();

  //RemoveTmpFiles();

  return error->IsOK();
}

//================================================================================
/*!
 * \brief Remove "test.out" and "problemfaces" files in current directory
 */
//================================================================================

void NETGENPlugin_Mesher::RemoveTmpFiles()
{
  TCollection_AsciiString str("test.out");
  OSD_Path path1( str );
  OSD_File file1( path1 );
  file1.Remove();
  str = "problemfaces";
  OSD_Path path2( str );
  OSD_File file2( path2 );
  file2.Remove();
}
