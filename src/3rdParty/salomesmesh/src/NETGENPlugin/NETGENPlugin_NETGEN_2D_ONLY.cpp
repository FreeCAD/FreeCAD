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
// File      : NETGENPlugin_NETGEN_2D_ONLY.cxx
// Author    : Edward AGAPOV (OCC)
// Project   : SALOME
//
#include "NETGENPlugin_NETGEN_2D_ONLY.hxx"

#include "NETGENPlugin_Mesher.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_MaxElementArea.hxx"
#include "StdMeshers_LengthFromEdges.hxx"
#include "StdMeshers_QuadranglePreference.hxx"

#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>

#include "utilities.h"

#include <list>
#include <vector>

/*
  Netgen include files
*/
namespace nglib {
#include <nglib.h>
}
#define OCCGEOMETRY
#include <occgeom.hpp>
#include <meshing.hpp>
//#include <meshtype.hpp>
namespace netgen {
#ifdef NETGEN_V5
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, MeshingParameters&, int, int);
#else
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, int, int, char*);
#endif
  DLL_HEADER extern MeshingParameters mparam;
}

using namespace std;
using namespace netgen;
using namespace nglib;

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_2D_ONLY::NETGENPlugin_NETGEN_2D_ONLY(int hypId, int studyId,
                                                         SMESH_Gen* gen)
  : SMESH_2D_Algo(hypId, studyId, gen)
{
  MESSAGE("NETGENPlugin_NETGEN_2D_ONLY::NETGENPlugin_NETGEN_2D_ONLY");
  _name = "NETGEN_2D_ONLY";

  _shapeType = (1 << TopAbs_FACE);// 1 bit /shape type

  _compatibleHypothesis.push_back("MaxElementArea");
  _compatibleHypothesis.push_back("LengthFromEdges");
  _compatibleHypothesis.push_back("QuadranglePreference");

  _hypMaxElementArea = 0;
  _hypLengthFromEdges = 0;
  _hypQuadranglePreference = 0;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_2D_ONLY::~NETGENPlugin_NETGEN_2D_ONLY()
{
  MESSAGE("NETGENPlugin_NETGEN_2D_ONLY::~NETGENPlugin_NETGEN_2D_ONLY");
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D_ONLY::CheckHypothesis (SMESH_Mesh&         aMesh,
                                                   const TopoDS_Shape& aShape,
                                                   Hypothesis_Status&  aStatus)
{
  _hypMaxElementArea = 0;
  _hypLengthFromEdges = 0;
  _hypQuadranglePreference = 0;

  const list<const SMESHDS_Hypothesis*>& hyps = GetUsedHypothesis(aMesh, aShape, false);

  if (hyps.empty())
  {
    aStatus = HYP_OK; //SMESH_Hypothesis::HYP_MISSING;
    return true;  // (PAL13464) can work with no hypothesis, LengthFromEdges is default one
  }

  aStatus = HYP_MISSING;

  list<const SMESHDS_Hypothesis*>::const_iterator ith;
  for (ith = hyps.begin(); ith != hyps.end(); ++ith )
  {
    const SMESHDS_Hypothesis* hyp = (*ith);

    string hypName = hyp->GetName();

    if      ( hypName == "MaxElementArea")
      _hypMaxElementArea = static_cast<const StdMeshers_MaxElementArea*> (hyp);
    else if ( hypName == "LengthFromEdges" )
      _hypLengthFromEdges = static_cast<const StdMeshers_LengthFromEdges*> (hyp);
    else if ( hypName == "QuadranglePreference" )
      _hypQuadranglePreference = static_cast<const StdMeshers_QuadranglePreference*>(hyp);
    else {
      aStatus = HYP_INCOMPATIBLE;
      return false;
    }
  }

  if ( _hypMaxElementArea && _hypLengthFromEdges ) {
    aStatus = HYP_CONCURENT;
    return false;
  }

  if ( _hypMaxElementArea || _hypLengthFromEdges )
    aStatus = HYP_OK;

  return aStatus == HYP_OK;
}

//================================================================================
/*!
 * \brief Fill netgen mesh with segments
  * \retval SMESH_ComputeErrorPtr - error description
 */
//================================================================================

static TError AddSegmentsToMesh(netgen::Mesh&                    ngMesh,
                                OCCGeometry&                     geom,
                                const TSideVector&               wires,
                                SMESH_MesherHelper&              helper,
                                vector< const SMDS_MeshNode* > & nodeVec)
{
  // ----------------------------
  // Check wires and count nodes
  // ----------------------------
  int nbNodes = 0;
  for ( int iW = 0; iW < wires.size(); ++iW )
  {
    StdMeshers_FaceSidePtr wire = wires[ iW ];
    if ( wire->MissVertexNode() )
      return TError
        (new SMESH_ComputeError(COMPERR_BAD_INPUT_MESH, "Missing nodes on vertices"));
      
    const vector<UVPtStruct>& uvPtVec = wire->GetUVPtStruct();
    if ( uvPtVec.size() != wire->NbPoints() )
      return TError
        (new SMESH_ComputeError(COMPERR_BAD_INPUT_MESH,
                                SMESH_Comment("Unexpected nb of points on wire ") << iW
                                << ": " << uvPtVec.size()<<" != "<<wire->NbPoints()));
    nbNodes += wire->NbSegments();
  }
  nodeVec.reserve( nbNodes );

  // -----------------
  // Fill netgen mesh
  // -----------------

//   netgen::Box<3> bb = geom.GetBoundingBox();
//   bb.Increase (bb.Diam()/10);
//   ngMesh.SetLocalH (bb.PMin(), bb.PMax(), 0.5); // set grading

  const int faceID = 1, solidID = 0;
  ngMesh.AddFaceDescriptor (FaceDescriptor(faceID, solidID, solidID, 0));

  for ( int iW = 0; iW < wires.size(); ++iW )
  {
    StdMeshers_FaceSidePtr wire = wires[ iW ];
    const vector<UVPtStruct>& uvPtVec = wire->GetUVPtStruct();

    int firstPointID = ngMesh.GetNP() + 1;
    int edgeID = 1, posID = -2;
    for ( int i = 0; i < wire->NbSegments(); ++i ) // loop on segments
    {
      // Add the first point of a segment
      const SMDS_MeshNode * n = uvPtVec[ i ].node;
      const int posShapeID = n->GetPosition()->GetShapeId();

      // skip nodes on degenerated edges
      if ( helper.IsDegenShape( posShapeID ) &&
           helper.IsDegenShape( uvPtVec[ i+1 ].node->GetPosition()->GetShapeId() ))
        continue;

      nodeVec.push_back( n );

      MeshPoint mp( Point<3> (n->X(), n->Y(), n->Z()) );
      ngMesh.AddPoint ( mp, 1, EDGEPOINT );

      // Add the segment
      Segment seg;

      seg.pnums[0] = ngMesh.GetNP();          // ng node id
      seg.pnums[1] = seg.pnums[0] + 1;              // ng node id
      seg.edgenr = ngMesh.GetNSeg() + 1;// segment id
      seg.si = faceID;                  // = geom.fmap.FindIndex (face);

      for ( int iEnd = 0; iEnd < 2; ++iEnd)
      {
        const UVPtStruct& pnt = uvPtVec[ i + iEnd ];

        seg.epgeominfo[ iEnd ].dist = pnt.param; // param on curve
        seg.epgeominfo[ iEnd ].u    = pnt.u;
        seg.epgeominfo[ iEnd ].v    = pnt.v;

        // find out edge id and node parameter on edge
        bool onVertex = ( pnt.node->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX );
        if ( onVertex || posShapeID != posID )
        {
          // get edge id
          double normParam = pnt.normParam;
          if ( onVertex )
            normParam = 0.5 * ( uvPtVec[ i ].normParam + uvPtVec[ i+1 ].normParam );
          const TopoDS_Edge& edge = wire->Edge( wire->EdgeIndex( normParam ));
          edgeID = geom.emap.FindIndex( edge );
          posID  = posShapeID;
          if ( onVertex ) // param on curve is different on each of two edges
            seg.epgeominfo[ iEnd ].dist = helper.GetNodeU( edge, pnt.node );
        }
        seg.epgeominfo[ iEnd ].edgenr = edgeID; //  = geom.emap.FindIndex(edge);
      }

      ngMesh.AddSegment (seg);

//       cout << "Segment: " << seg.edgenr << endl
//            << "\tp1: " << seg.p1 << endl
//            << "\tp2: " << seg.p2 << endl
//            << "\tp0 param: " << seg.epgeominfo[ 0 ].dist << endl
//            << "\tp0 uv: " << seg.epgeominfo[ 0 ].u <<", "<< seg.epgeominfo[ 0 ].v << endl
//            << "\tp0 edge: " << seg.epgeominfo[ 0 ].edgenr << endl
//            << "\tp1 param: " << seg.epgeominfo[ 1 ].dist << endl
//            << "\tp1 uv: " << seg.epgeominfo[ 1 ].u <<", "<< seg.epgeominfo[ 1 ].v << endl
//            << "\tp1 edge: " << seg.epgeominfo[ 1 ].edgenr << endl;
    }
    Segment& seg = ngMesh.LineSegment( ngMesh.GetNSeg() );
    seg.pnums[1] = firstPointID;
  }

  ngMesh.CalcSurfacesOfNode();  

  return TError();
}

//=============================================================================
/*!
 *Here we are going to use the NETGEN mesher
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D_ONLY::Compute(SMESH_Mesh&         aMesh,
                                          const TopoDS_Shape& aShape)
{
  MESSAGE("NETGENPlugin_NETGEN_2D_ONLY::Compute()");

  SMESHDS_Mesh* meshDS = aMesh.GetMeshDS();
  int faceID = meshDS->ShapeToIndex( aShape );

  SMESH_MesherHelper helper(aMesh);
  _quadraticMesh = helper.IsQuadraticSubMesh(aShape);
  helper.SetElementsOnShape( true );
  const bool ignoreMediumNodes = _quadraticMesh;
  
  // ------------------------
  // get all edges of a face
  // ------------------------
  const TopoDS_Face F = TopoDS::Face( aShape.Oriented( TopAbs_FORWARD ));
  TError problem;
  TSideVector wires = StdMeshers_FaceSide::GetFaceWires( F, aMesh, ignoreMediumNodes, problem );
  if ( problem && !problem->IsOK() )
    return error( problem );
  int nbWires = wires.size();
  if ( nbWires == 0 )
    return error( "Problem in StdMeshers_FaceSide::GetFaceWires()");
  if ( wires[0]->NbSegments() < 3 ) // ex: a circle with 2 segments
    return error(COMPERR_BAD_INPUT_MESH,
                 SMESH_Comment("Too few segments: ")<<wires[0]->NbSegments());

  // -------------------------
  // Make input netgen mesh
  // -------------------------

  Ng_Init();
  netgen::Mesh * ngMesh = new netgen::Mesh ();

  netgen::OCCGeometry occgeo;
  NETGENPlugin_Mesher::PrepareOCCgeometry( occgeo, F, aMesh );
  occgeo.fmap.Clear(); // face can be reversed, which is wrong in this case (issue 19978)
  occgeo.fmap.Add( F );

  vector< const SMDS_MeshNode* > nodeVec;
  problem = AddSegmentsToMesh( *ngMesh, occgeo, wires, helper, nodeVec );
  if ( problem && !problem->IsOK() ) {
    delete ngMesh; Ng_Exit();
    return error( problem );
  }

  // --------------------
  // compute edge length
  // --------------------

  double edgeLength = 0;
  if (_hypLengthFromEdges || !_hypLengthFromEdges && !_hypMaxElementArea)
  {
    int nbSegments = 0;
    for ( int iW = 0; iW < nbWires; ++iW )
    {
      edgeLength += wires[ iW ]->Length();
      nbSegments += wires[ iW ]->NbSegments();
    }
    if ( nbSegments )
      edgeLength /= nbSegments;
  }
  if ( _hypMaxElementArea )
  {
    double maxArea = _hypMaxElementArea->GetMaxArea();
    edgeLength = sqrt(2. * maxArea/sqrt(3.0));
  }
  if ( edgeLength < DBL_MIN )
    edgeLength = occgeo.GetBoundingBox().Diam();

  //cout << " edgeLength = " << edgeLength << endl;

  netgen::mparam.maxh = edgeLength;
  netgen::mparam.quad = _hypQuadranglePreference ? 1 : 0;
  //ngMesh->SetGlobalH ( edgeLength );

  // -------------------------
  // Generate surface mesh
  // -------------------------

  char *optstr;
  int startWith = MESHCONST_MESHSURFACE;
  int endWith   = MESHCONST_OPTSURFACE;
  int err = 1;

  try {
#if (OCC_VERSION_MAJOR << 16 | OCC_VERSION_MINOR << 8 | OCC_VERSION_MAINTENANCE) > 0x060100
    OCC_CATCH_SIGNALS;
#endif
#ifdef NETGEN_V5
    err = netgen::OCCGenerateMesh(occgeo, ngMesh,netgen::mparam, startWith, endWith);
#else
    err = netgen::OCCGenerateMesh(occgeo, ngMesh, startWith, endWith, optstr);
#endif
  }
  catch (Standard_Failure& ex) {
    string comment = ex.DynamicType()->Name();
    if ( ex.GetMessageString() && strlen( ex.GetMessageString() )) {
      comment += ": ";
      comment += ex.GetMessageString();
    }
    error(COMPERR_OCC_EXCEPTION, comment);
  }
  catch (NgException exc) {
    error( SMESH_Comment("NgException: ") << exc.What() );
  }
  catch (...) {
    error(COMPERR_EXCEPTION,"Exception in netgen::OCCGenerateMesh()");
  }

  // ----------------------------------------------------
  // Fill the SMESHDS with the generated nodes and faces
  // ----------------------------------------------------

  int nbNodes = ngMesh->GetNP();
  int nbFaces = ngMesh->GetNSE();

  int nbInputNodes = nodeVec.size();
  nodeVec.resize( nbNodes, 0 );

  // add nodes
  for ( int i = nbInputNodes + 1; i <= nbNodes; ++i )
  {
    const MeshPoint& ngPoint = ngMesh->Point(i);
    SMDS_MeshNode * node = meshDS->AddNode(ngPoint(0), ngPoint(1), ngPoint(2));
    nodeVec[ i-1 ] = node;
  }

  // create faces
  bool reverse = ( aShape.Orientation() == TopAbs_REVERSED );
  for ( int i = 1; i <= nbFaces ; ++i )
  {
    const Element2d& elem = ngMesh->SurfaceElement(i);
    vector<const SMDS_MeshNode*> nodes( elem.GetNP() );
    for (int j=1; j <= elem.GetNP(); ++j)
    {
      int pind = elem.PNum(j);
      const SMDS_MeshNode* node = nodeVec.at(pind-1);
      if ( reverse )
        nodes[ nodes.size()-j ] = node;
      else
        nodes[ j-1 ] = node;
      if ( node->GetPosition()->GetTypeOfPosition() == SMDS_TOP_3DSPACE )
      {
        const PointGeomInfo& pgi = elem.GeomInfoPi(j);
        meshDS->SetNodeOnFace((SMDS_MeshNode*)node, faceID, pgi.u, pgi.v);
      }
    }
    SMDS_MeshFace* face = 0;
    if ( elem.GetType() == TRIG )
      face = helper.AddFace(nodes[0],nodes[1],nodes[2]);
    else
      face = helper.AddFace(nodes[0],nodes[1],nodes[2],nodes[3]);
  }

  Ng_DeleteMesh((nglib::Ng_Mesh*)ngMesh);
  Ng_Exit();

  NETGENPlugin_Mesher::RemoveTmpFiles();

  return !err;
}
