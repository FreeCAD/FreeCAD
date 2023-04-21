// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

// File      : NETGENPlugin_NETGEN_2D_ONLY.cxx
// Author    : Edward AGAPOV (OCC)
// Project   : SALOME
//
#include "NETGENPlugin_NETGEN_2D_ONLY.hxx"

#include "NETGENPlugin_Mesher.hxx"
#include "NETGENPlugin_Hypothesis_2D.hxx"

#include <SMDS_MeshElement.hxx>
#include <SMDS_MeshNode.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Comment.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_MesherHelper.hxx>
#include <SMESH_subMesh.hxx>
#include <StdMeshers_FaceSide.hxx>
#include <StdMeshers_LengthFromEdges.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_ViscousLayers2D.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#include <Precision.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>

#include <utilities.h>

#include <list>
#include <memory>
#include <vector>
#include <limits>

/*
  Netgen include files
*/

namespace nglib {
#include <nglib.h>
}
#ifndef OCCGEOMETRY
# define OCCGEOMETRY
#endif

// DLL_HEADER is re-defined in netgen headers
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmacro-redefined"
#endif

#ifdef NETGEN_PYTHON
#undef NETGEN_PYTHON
#endif

#ifndef WIN32
#undef DLL_HEADER
#endif

#include <occgeom.hpp>
#include <meshing.hpp>
//#include <meshing/meshtype.hpp>

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

namespace netgen {
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2,2004)
  // https://github.com/NGSolve/netgen/commit/bee097b153b43d9346819789534536cd1b773428
  int OCCGenerateMesh(OCCGeometry& geo, shared_ptr<Mesh>& mesh, MeshingParameters& mparams)
  {
    //geo.SetOCCParameters(occparam);
    int perfstepsend = mparams.perfstepsend;
    if (perfstepsend == netgen::MESHCONST_OPTSURFACE) {
      mparams.perfstepsend = netgen::MESHCONST_MESHSURFACE;
    }
    auto result = geo.GenerateMesh(mesh, mparams);
    mparams.perfstepsend = perfstepsend;
    return result;
  }
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2,0)
  DLL_HEADER extern int OCCGenerateMesh(OCCGeometry&, shared_ptr<Mesh>&, MeshingParameters&);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(6,0,0)
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, shared_ptr<Mesh>&, MeshingParameters&, int, int);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0,0)
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, MeshingParameters&, int, int);
#else
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, int, int, char*);
#endif
  DLL_HEADER extern MeshingParameters mparam;
#if NETGEN_VERSION <= NETGEN_VERSION_STRING(6,2,1808)
  DLL_HEADER extern void OCCSetLocalMeshSize(OCCGeometry & geom, Mesh & mesh);
#endif
}

using namespace std;
using namespace netgen;
using namespace nglib;

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_2D_ONLY::NETGENPlugin_NETGEN_2D_ONLY(int        hypId,
                                                         int        studyId,
                                                         SMESH_Gen* gen)
  : SMESH_2D_Algo(hypId, studyId, gen)
{
  _name = "NETGEN_2D_ONLY";
  
  _shapeType = (1 << TopAbs_FACE);// 1 bit /shape type
  _onlyUnaryInput = false; // treat all FACEs at once

  _compatibleHypothesis.push_back("MaxElementArea");
  _compatibleHypothesis.push_back("LengthFromEdges");
  _compatibleHypothesis.push_back("QuadranglePreference");
  _compatibleHypothesis.push_back("NETGEN_Parameters_2D");
  _compatibleHypothesis.push_back("ViscousLayers2D");

  _hypMaxElementArea       = 0;
  _hypLengthFromEdges      = 0;
  _hypQuadranglePreference = 0;
  _hypParameters           = 0;
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
  _hypParameters = 0;
  _progressByTic = -1;

  const list<const SMESHDS_Hypothesis*>& hyps = GetUsedHypothesis(aMesh, aShape, false);

  if (hyps.empty())
  {
    aStatus = HYP_OK; //SMESH_Hypothesis::HYP_MISSING;
    return true;  // (PAL13464) can work with no hypothesis, LengthFromEdges is default one
  }

  aStatus = HYP_MISSING;

  bool hasVL = false;
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
    else if ( hypName == "NETGEN_Parameters_2D" )
      _hypParameters = static_cast<const NETGENPlugin_Hypothesis_2D*>(hyp);
    else if ( hypName == StdMeshers_ViscousLayers2D::GetHypType() )
      hasVL = true;
    else {
      aStatus = HYP_INCOMPATIBLE;
      return false;
    }
  }

  int nbHyps = bool(_hypMaxElementArea) + bool(_hypLengthFromEdges) + bool(_hypParameters );
  if ( nbHyps > 1 )
    aStatus = HYP_CONCURENT;
  else if ( hasVL )
    error( StdMeshers_ViscousLayers2D::CheckHypothesis( aMesh, aShape, aStatus ));
  else
    aStatus = HYP_OK;

  if ( aStatus == HYP_OK && _hypParameters && _hypQuadranglePreference )
  {
    aStatus = HYP_INCOMPAT_HYPS;
    return error(SMESH_Comment("\"") << _hypQuadranglePreference->GetName()
                 << "\" and \"" << _hypParameters->GetName()
                 << "\" are incompatible hypotheses");
  }

  return ( aStatus == HYP_OK );
}

//namespace
//{
//  inline void limitSize( netgen::Mesh* ngMesh,
//                  const double  maxh )
//  {
//    // get bnd box
//    netgen::Point3d pmin, pmax;
//    ngMesh->GetBox( pmin, pmax, 0 );
//    const double dx = pmax.X() - pmin.X();
//    const double dy = pmax.Y() - pmin.Y();
//    const double dz = pmax.Z() - pmin.Z();

//    const int nbX = Max( 2, int( dx / maxh * 3 ));
//    const int nbY = Max( 2, int( dy / maxh * 3 ));
//    const int nbZ = Max( 2, int( dz / maxh * 3 ));

//    if ( ! & ngMesh->LocalHFunction() )
//      ngMesh->SetLocalH( pmin, pmax, 0.1 );

//    netgen::Point3d p;
//    for ( int i = 0; i <= nbX; ++i )
//    {
//      p.X() = pmin.X() +  i * dx / nbX;
//      for ( int j = 0; j <= nbY; ++j )
//      {
//        p.Y() = pmin.Y() +  j * dy / nbY;
//        for ( int k = 0; k <= nbZ; ++k )
//        {
//          p.Z() = pmin.Z() +  k * dz / nbZ;
//          ngMesh->RestrictLocalH( p, maxh );
//        }
//      }
//    }
//  }
//}

//=============================================================================
/*!
 *Here we are going to use the NETGEN mesher
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D_ONLY::Compute(SMESH_Mesh&         aMesh,
                                          const TopoDS_Shape& aShape)
{
  netgen::multithread.terminate = 0;
  //netgen::multithread.task = "Surface meshing";

  SMESHDS_Mesh* meshDS = aMesh.GetMeshDS();
  SMESH_MesherHelper helper(aMesh);
  helper.SetElementsOnShape( true );

  NETGENPlugin_NetgenLibWrapper ngLib;
  ngLib._isComputeOk = false;

  netgen::Mesh   ngMeshNoLocSize;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0,0)
  netgen::Mesh * ngMeshes[2] = { (netgen::Mesh*) ngLib._ngMesh,  & ngMeshNoLocSize };
#else
  netgen::Mesh * ngMeshes[2] = { (netgen::Mesh*) ngLib._ngMesh.get(),  & ngMeshNoLocSize };
#endif
  netgen::OCCGeometry occgeoComm;

  // min / max sizes are set as follows:
  // if ( _hypParameters )
  //    min and max are defined by the user
  // else if ( _hypLengthFromEdges )
  //    min = aMesher.GetDefaultMinSize()
  //    max = average segment len of a FACE
  // else if ( _hypMaxElementArea )
  //    min = aMesher.GetDefaultMinSize()
  //    max = f( _hypMaxElementArea )
  // else
  //    min = aMesher.GetDefaultMinSize()
  //    max = max segment len of a FACE

  NETGENPlugin_Mesher aMesher( &aMesh, aShape, /*isVolume=*/false);
  aMesher.SetParameters( _hypParameters ); // _hypParameters -> netgen::mparam
  const bool toOptimize = _hypParameters ? _hypParameters->GetOptimize() : true;
  if ( _hypMaxElementArea )
  {
    netgen::mparam.maxh = sqrt( 2. * _hypMaxElementArea->GetMaxArea() / sqrt(3.0) );
  }
  if ( _hypQuadranglePreference )
    netgen::mparam.quad = true;

  // local size is common for all FACEs in aShape?
  const bool isCommonLocalSize = ( !_hypLengthFromEdges && !_hypMaxElementArea && netgen::mparam.uselocalh );
  const bool isDefaultHyp = ( !_hypLengthFromEdges && !_hypMaxElementArea && !_hypParameters );

  if ( isCommonLocalSize ) // compute common local size in ngMeshes[0]
  {
    //list< SMESH_subMesh* > meshedSM[4]; --> all sub-shapes are added to occgeoComm
    aMesher.PrepareOCCgeometry( occgeoComm, aShape, aMesh );//, meshedSM );

    // local size set at MESHCONST_ANALYSE step depends on
    // minh, face_maxh, grading and curvaturesafety; find minh if not set by the user
    if ( !_hypParameters || netgen::mparam.minh < DBL_MIN )
    {
      if ( !_hypParameters )
        netgen::mparam.maxh = occgeoComm.GetBoundingBox().Diam() / 3.;
      netgen::mparam.minh = aMesher.GetDefaultMinSize( aShape, netgen::mparam.maxh );
    }
    // set local size depending on curvature and NOT closeness of EDGEs
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2,2004)
    // https://github.com/NGSolve/netgen/commit/073e215bb6bc97d8712990cba9cc6e9e1e4d8b2a
    netgen::mparam.closeedgefac = std::nullopt;
#else
    netgen::occparam.resthcloseedgeenable = false;
#endif
    //netgen::occparam.resthcloseedgefac = 1.0 + netgen::mparam.grading;
    occgeoComm.face_maxh = netgen::mparam.maxh;
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2,2004)
    // https://github.com/NGSolve/netgen/commit/bee097b153b43d9346819789534536cd1b773428
    occgeoComm.Analyse(*ngMeshes[0], netgen::mparam);
#else
    netgen::OCCSetLocalMeshSize( occgeoComm, *ngMeshes[0] );
#endif
    occgeoComm.emap.Clear();
    occgeoComm.vmap.Clear();

    // set local size according to size of existing segments
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2,2004)
    const double factor = netgen::mparam.closeedgefac.value();
#else
    const double factor = netgen::occparam.resthcloseedgefac;
#endif
    TopTools_IndexedMapOfShape edgeMap;
    TopExp::MapShapes( aMesh.GetShapeToMesh(), TopAbs_EDGE, edgeMap );
    for ( int iE = 1; iE <= edgeMap.Extent(); ++iE )
    {
      const TopoDS_Shape& edge = edgeMap( iE );
      if ( SMESH_Algo::isDegenerated( TopoDS::Edge( edge ))/* ||
           helper.IsSubShape( edge, aShape )*/)
        continue;
      SMESHDS_SubMesh* smDS = meshDS->MeshElements( edge );
      if ( !smDS ) continue;
      SMDS_ElemIteratorPtr segIt = smDS->GetElements();
      while ( segIt->more() )
      {
        const SMDS_MeshElement* seg = segIt->next();
        SMESH_TNodeXYZ n1 = seg->GetNode(0);
        SMESH_TNodeXYZ n2 = seg->GetNode(1);
        gp_XYZ p = 0.5 * ( n1 + n2 );
        netgen::Point3d pi(p.X(), p.Y(), p.Z());
        ngMeshes[0]->RestrictLocalH( pi, factor * ( n1 - n2 ).Modulus() );
      }
    }
  }
  netgen::mparam.uselocalh = toOptimize; // restore as it is used at surface optimization

  // ==================
  // Loop on all FACEs
  // ==================

  vector< const SMDS_MeshNode* > nodeVec;

  TopExp_Explorer fExp( aShape, TopAbs_FACE );
  for ( int iF = 0; fExp.More(); fExp.Next(), ++iF )
  {
    TopoDS_Face F = TopoDS::Face( fExp.Current() /*.Oriented( TopAbs_FORWARD )*/);
    int    faceID = meshDS->ShapeToIndex( F );
    SMESH_ComputeErrorPtr& faceErr = aMesh.GetSubMesh( F )->GetComputeError();

    _quadraticMesh = helper.IsQuadraticSubMesh( F );
    const bool ignoreMediumNodes = _quadraticMesh;

    // build viscous layers if required
    if ( F.Orientation() != TopAbs_FORWARD &&
         F.Orientation() != TopAbs_REVERSED )
      F.Orientation( TopAbs_FORWARD ); // avoid pb with TopAbs_INTERNAL
    SMESH_ProxyMesh::Ptr proxyMesh = StdMeshers_ViscousLayers2D::Compute( aMesh, F );
    if ( !proxyMesh )
      continue;

    // ------------------------
    // get all EDGEs of a FACE
    // ------------------------
    TSideVector wires =
      StdMeshers_FaceSide::GetFaceWires( F, aMesh, ignoreMediumNodes, faceErr, proxyMesh );
    if ( faceErr && !faceErr->IsOK() )
      continue;
    int nbWires = wires.size();
    if ( nbWires == 0 )
    {
      faceErr.reset
        ( new SMESH_ComputeError
          ( COMPERR_ALGO_FAILED, "Problem in StdMeshers_FaceSide::GetFaceWires()" ));
      continue;
    }
    if ( wires[0]->NbSegments() < 3 ) // ex: a circle with 2 segments
    {
      faceErr.reset
        ( new SMESH_ComputeError
          ( COMPERR_BAD_INPUT_MESH, SMESH_Comment("Too few segments: ")<<wires[0]->NbSegments()) );
      continue;
    }

    // ----------------------
    // compute maxh of a FACE
    // ----------------------

    if ( !_hypParameters )
    {
      double edgeLength = 0;
      if (_hypLengthFromEdges )
      {
        // compute edgeLength as an average segment length
        int nbSegments = 0;
        for ( int iW = 0; iW < nbWires; ++iW )
        {
          edgeLength += wires[ iW ]->Length();
          nbSegments += wires[ iW ]->NbSegments();
        }
        if ( nbSegments )
          edgeLength /= nbSegments;
        netgen::mparam.maxh = edgeLength;
      }
      else if ( isDefaultHyp )
      {
        // set edgeLength by a longest segment
        double maxSeg2 = 0;
        for ( int iW = 0; iW < nbWires; ++iW )
        {
          const UVPtStructVec& points = wires[ iW ]->GetUVPtStruct();
          if ( points.empty() )
            return error( COMPERR_BAD_INPUT_MESH );
          gp_Pnt pPrev = SMESH_TNodeXYZ( points[0].node );
          for ( size_t i = 1; i < points.size(); ++i )
          {
            gp_Pnt p = SMESH_TNodeXYZ( points[i].node );
            maxSeg2 = Max( maxSeg2, p.SquareDistance( pPrev ));
            pPrev = p;
          }
        }
        edgeLength = sqrt( maxSeg2 ) * 1.05;
        netgen::mparam.maxh = edgeLength;
      }
      if ( netgen::mparam.maxh < DBL_MIN )
        netgen::mparam.maxh = occgeoComm.GetBoundingBox().Diam();

      if ( !isCommonLocalSize )
      {
        netgen::mparam.minh = aMesher.GetDefaultMinSize( F, netgen::mparam.maxh );
      }
    }

    // prepare occgeom
    netgen::OCCGeometry occgeom;
    occgeom.shape = F;
    occgeom.fmap.Add( F );
    occgeom.CalcBoundingBox();
    occgeom.facemeshstatus.SetSize(1);
    occgeom.facemeshstatus = 0;
    occgeom.face_maxh_modified.SetSize(1);
    occgeom.face_maxh_modified = 0;
    occgeom.face_maxh.SetSize(1);
    occgeom.face_maxh = netgen::mparam.maxh;

    // -------------------------
    // Fill netgen mesh
    // -------------------------

    // MESHCONST_ANALYSE step may lead to a failure, so we make an attempt
    // w/o MESHCONST_ANALYSE at the second loop
    int err = 0;
    enum { LOC_SIZE, NO_LOC_SIZE };
    int iLoop = isCommonLocalSize ? 0 : 1;
    for ( ; iLoop < 2; iLoop++ )
    {
      //bool isMESHCONST_ANALYSE = false;
      InitComputeError();

      netgen::Mesh * ngMesh = ngMeshes[ iLoop ];
      ngMesh->DeleteMesh();

      if ( iLoop == NO_LOC_SIZE )
      {
        ngMesh->SetGlobalH ( mparam.maxh );
        ngMesh->SetMinimalH( mparam.minh );
        Box<3> bb = occgeom.GetBoundingBox();
        bb.Increase (bb.Diam()/10);
        ngMesh->SetLocalH (bb.PMin(), bb.PMax(), mparam.grading);
      }

      nodeVec.clear();
      faceErr = aMesher.AddSegmentsToMesh( *ngMesh, occgeom, wires, helper, nodeVec,
                                           /*overrideMinH=*/!_hypParameters);
      if ( faceErr && !faceErr->IsOK() )
        break;

      //if ( !isCommonLocalSize )
      //limitSize( ngMesh, mparam.maxh * 0.8);

      // -------------------------
      // Generate surface mesh
      // -------------------------
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2,0)
      const int startWith = MESHCONST_MESHSURFACE;
      const int endWith   = toOptimize ? MESHCONST_OPTSURFACE : MESHCONST_MESHSURFACE;
#else
      netgen::mparam.perfstepsstart = MESHCONST_MESHEDGES;
      netgen::mparam.perfstepsend = toOptimize ? MESHCONST_OPTSURFACE : MESHCONST_MESHSURFACE;
#endif
      SMESH_Comment str;
      try {
        OCC_CATCH_SIGNALS;
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,0,0)
        std::shared_ptr<netgen::Mesh> mesh_ptr(ngMesh,  [](netgen::Mesh*){});
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2,0)
        err = netgen::OCCGenerateMesh(occgeom, mesh_ptr, netgen::mparam);
#else
        err = netgen::OCCGenerateMesh(occgeom, mesh_ptr, netgen::mparam, startWith, endWith);
#endif
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0,0)
        err = netgen::OCCGenerateMesh(occgeom, ngMesh, netgen::mparam, startWith, endWith);
#else
        char *optstr = 0;
        err = netgen::OCCGenerateMesh(occgeom, ngMesh, startWith, endWith, optstr);
#endif
        if ( netgen::multithread.terminate )
          return false;
        if ( err )
          str << "Error in netgen::OCCGenerateMesh() at " << netgen::multithread.task;
      }
      catch (Standard_Failure& ex)
      {
        err = 1;
        str << "Exception in  netgen::OCCGenerateMesh()"
            << " at " << netgen::multithread.task
            << ": " << ex.DynamicType()->Name();
        if ( ex.GetMessageString() && strlen( ex.GetMessageString() ))
          str << ": " << ex.GetMessageString();
      }
      catch (...) {
        err = 1;
        str << "Exception in  netgen::OCCGenerateMesh()"
            << " at " << netgen::multithread.task;
      }
      if ( err )
      {
        if ( aMesher.FixFaceMesh( occgeom, *ngMesh, 1 ))
          break;
        if ( iLoop == LOC_SIZE )
        {
          netgen::mparam.minh = netgen::mparam.maxh;
          netgen::mparam.maxh = 0;
          for ( int iW = 0; iW < wires.size(); ++iW )
          {
            StdMeshers_FaceSidePtr wire = wires[ iW ];
            const vector<UVPtStruct>& uvPtVec = wire->GetUVPtStruct();
            for ( size_t iP = 1; iP < uvPtVec.size(); ++iP )
            {
              SMESH_TNodeXYZ   p( uvPtVec[ iP ].node );
              netgen::Point3d np( p.X(),p.Y(),p.Z());
              double segLen = p.Distance( uvPtVec[ iP-1 ].node );
              double   size = ngMesh->GetH( np );
              netgen::mparam.minh = Min( netgen::mparam.minh, size );
              netgen::mparam.maxh = Max( netgen::mparam.maxh, segLen );
            }
          }
          //cerr << "min " << netgen::mparam.minh << " max " << netgen::mparam.maxh << std::endl;
          netgen::mparam.minh *= 0.9;
          netgen::mparam.maxh *= 1.1;
          continue;
        }
        else
        {
          faceErr.reset( new SMESH_ComputeError( COMPERR_ALGO_FAILED, str ));
        }
      }


      // ----------------------------------------------------
      // Fill the SMESHDS with the generated nodes and faces
      // ----------------------------------------------------

      int nbNodes = ngMesh->GetNP();
      int nbFaces = ngMesh->GetNSE();

      int nbInputNodes = nodeVec.size()-1;
      nodeVec.resize( nbNodes+1, 0 );

      // add nodes
      for ( int ngID = nbInputNodes + 1; ngID <= nbNodes; ++ngID )
      {
        const MeshPoint& ngPoint = ngMesh->Point( ngID );
        SMDS_MeshNode * node = meshDS->AddNode(ngPoint(0), ngPoint(1), ngPoint(2));
        nodeVec[ ngID ] = node;
      }

      // create faces
      int i,j;
      vector<const SMDS_MeshNode*> nodes;
      for ( i = 1; i <= nbFaces ; ++i )
      {
        const Element2d& elem = ngMesh->SurfaceElement(i);
        nodes.resize( elem.GetNP() );
        for (j=1; j <= elem.GetNP(); ++j)
        {
          int pind = elem.PNum(j);
          if ( pind < 1 )
            break;
          nodes[ j-1 ] = nodeVec[ pind ];
          if ( nodes[ j-1 ]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_3DSPACE )
          {
            const PointGeomInfo& pgi = elem.GeomInfoPi(j);
            meshDS->SetNodeOnFace( nodes[ j-1 ], faceID, pgi.u, pgi.v);
          }
        }
        if ( j > elem.GetNP() )
        {
          SMDS_MeshFace* face = 0;
          if ( elem.GetType() == TRIG )
            face = helper.AddFace(nodes[0],nodes[1],nodes[2]);
          else
            face = helper.AddFace(nodes[0],nodes[1],nodes[2],nodes[3]);
        }
      }

      break;
    } // two attempts
  } // loop on FACEs

  return true;
}

void NETGENPlugin_NETGEN_2D_ONLY::CancelCompute()
{
  SMESH_Algo::CancelCompute();
  netgen::multithread.terminate = 1;
}

//================================================================================
/*!
 * \brief Return progress of Compute() [0.,1]
 */
//================================================================================

double NETGENPlugin_NETGEN_2D_ONLY::GetProgress() const
{
  return -1;
  // const char* task1 = "Surface meshing";
  // //const char* task2 = "Optimizing surface";
  // double& progress = const_cast<NETGENPlugin_NETGEN_2D_ONLY*>( this )->_progress;
  // if ( _progressByTic < 0. &&
  //      strncmp( netgen::multithread.task, task1, 3 ) == 0 )
  // {
  //   progress = Min( 0.25, SMESH_Algo::GetProgressByTic() ); // [0, 0.25]
  // }
  // else //if ( strncmp( netgen::multithread.task, task2, 3 ) == 0)
  // {
  //   if ( _progressByTic < 0 )
  //   {
  //     NETGENPlugin_NETGEN_2D_ONLY* me = (NETGENPlugin_NETGEN_2D_ONLY*) this;
  //     me->_progressByTic = 0.25 / (_progressTic+1);
  //   }
  //   const_cast<NETGENPlugin_NETGEN_2D_ONLY*>( this )->_progressTic++;
  //   progress = Max( progress, _progressByTic * _progressTic );
  // }
  // //cout << netgen::multithread.task << " " << _progressTic << std::endl;
  // return Min( progress, 0.99 );
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D_ONLY::Evaluate(SMESH_Mesh& aMesh,
                                           const TopoDS_Shape& aShape,
                                           MapShapeNbElems& aResMap)
{
  TopoDS_Face F = TopoDS::Face(aShape);
  if(F.IsNull())
    return false;

  // collect info from edges
  int nb0d = 0, nb1d = 0;
  bool IsQuadratic = false;
  bool IsFirst = true;
  double fullLen = 0.0;
  TopTools_MapOfShape tmpMap;
  for (TopExp_Explorer exp(F, TopAbs_EDGE); exp.More(); exp.Next()) {
    TopoDS_Edge E = TopoDS::Edge(exp.Current());
    if( tmpMap.Contains(E) )
      continue;
    tmpMap.Add(E);
    SMESH_subMesh *aSubMesh = aMesh.GetSubMesh(exp.Current());
    MapShapeNbElemsItr anIt = aResMap.find(aSubMesh);
    if( anIt==aResMap.end() ) {
      SMESH_subMesh *sm = aMesh.GetSubMesh(F);
      SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
      smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
      return false;
    }
    std::vector<int> aVec = (*anIt).second;
    nb0d += aVec[SMDSEntity_Node];
    nb1d += Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
    double aLen = SMESH_Algo::EdgeLength(E);
    fullLen += aLen;
    if(IsFirst) {
      IsQuadratic = (aVec[SMDSEntity_Quad_Edge] > aVec[SMDSEntity_Edge]);
      IsFirst = false;
    }
  }
  tmpMap.Clear();

  // compute edge length
  double ELen = 0;
  if (_hypLengthFromEdges || (!_hypLengthFromEdges && !_hypMaxElementArea)) {
    if ( nb1d > 0 )
      ELen = fullLen / nb1d;
  }
  if ( _hypMaxElementArea ) {
    double maxArea = _hypMaxElementArea->GetMaxArea();
    ELen = sqrt(2. * maxArea/sqrt(3.0));
  }
  GProp_GProps G;
  BRepGProp::SurfaceProperties(F,G);
  double anArea = G.Mass();

  const int hugeNb = numeric_limits<int>::max()/10;
  if ( anArea / hugeNb > ELen*ELen )
  {
    SMESH_subMesh *sm = aMesh.GetSubMesh(F);
    SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
    smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated.\nToo small element length",this));
    return false;
  }
  int nbFaces = (int) ( anArea / ( ELen*ELen*sqrt(3.) / 4 ) );
  int nbNodes = (int) ( ( nbFaces*3 - (nb1d-1)*2 ) / 6 + 1 );
  std::vector<int> aVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i]=0;
  if( IsQuadratic ) {
    aVec[SMDSEntity_Node] = nbNodes;
    aVec[SMDSEntity_Quad_Triangle] = nbFaces;
  }
  else {
    aVec[SMDSEntity_Node] = nbNodes;
    aVec[SMDSEntity_Triangle] = nbFaces;
  }
  SMESH_subMesh *sm = aMesh.GetSubMesh(F);
  aResMap.insert(std::make_pair(sm,aVec));

  return true;
}
