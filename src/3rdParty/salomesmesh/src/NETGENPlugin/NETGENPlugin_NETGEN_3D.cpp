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

//=============================================================================
// File      : NETGENPlugin_NETGEN_3D.cxx
//             Moved here from SMESH_NETGEN_3D.cxx
// Created   : lundi 27 Janvier 2003
// Author    : Nadir BOUHAMOU (CEA)
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_NETGEN_3D.hxx"

#include "NETGENPlugin_Hypothesis.hxx"

#include <SMDS_MeshElement.hxx>
#include <SMDS_MeshNode.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Comment.hxx>
#include <SMESH_ControlsDef.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_MeshEditor.hxx>
#include <SMESH_MesherHelper.hxx>
#include <SMESH_subMesh.hxx>
#include <StdMeshers_MaxElementVolume.hxx>
#include <StdMeshers_QuadToTriaAdaptor.hxx>
#include <StdMeshers_ViscousLayers.hxx>

#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>

#include <Standard_Failure.hxx>
#include <Standard_ErrorHandler.hxx>

#include <utilities.h>

#include <list>
#include <vector>
#include <map>

/*
  Netgen include files
*/

#ifndef OCCGEOMETRY
#define OCCGEOMETRY
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

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

namespace nglib {
#include <nglib.h>
}
namespace netgen {
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2,0)
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, shared_ptr<Mesh>&, MeshingParameters&);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(6,0,0)
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, shared_ptr<Mesh>&, MeshingParameters&, int, int);
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0,0)
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, MeshingParameters&, int, int);
#else
  DLL_HEADER extern int OCCGenerateMesh (OCCGeometry&, Mesh*&, int, int, char*);
#endif
  DLL_HEADER extern MeshingParameters mparam;
  DLL_HEADER extern volatile multithreadt multithread;
}
using namespace nglib;
using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_3D::NETGENPlugin_NETGEN_3D(int hypId, int studyId,
                             SMESH_Gen* gen)
  : SMESH_3D_Algo(hypId, studyId, gen)
{
  MESSAGE("NETGENPlugin_NETGEN_3D::NETGENPlugin_NETGEN_3D");
  _name = "NETGEN_3D";
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);// 1 bit /shape type
  _compatibleHypothesis.push_back("MaxElementVolume");
  _compatibleHypothesis.push_back("NETGEN_Parameters");
  _compatibleHypothesis.push_back("ViscousLayers");

  _maxElementVolume = 0.;

  _hypMaxElementVolume = NULL;
  _hypParameters = NULL;
  _viscousLayersHyp = NULL;

  _requireShape = false; // can work without shape
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_3D::~NETGENPlugin_NETGEN_3D()
{
  MESSAGE("NETGENPlugin_NETGEN_3D::~NETGENPlugin_NETGEN_3D");
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool NETGENPlugin_NETGEN_3D::CheckHypothesis (SMESH_Mesh&         aMesh,
                                              const TopoDS_Shape& aShape,
                                              Hypothesis_Status&  aStatus)
{
  MESSAGE("NETGENPlugin_NETGEN_3D::CheckHypothesis");

  _hypMaxElementVolume = NULL;
  _hypParameters = NULL;
  _viscousLayersHyp = NULL;
  _maxElementVolume = DBL_MAX;

  // for correct work of GetProgress():
  netgen::multithread.percent = 0.;
  netgen::multithread.task = "Volume meshing";
  _progressByTic = -1.;

  list<const SMESHDS_Hypothesis*>::const_iterator itl;
  const SMESHDS_Hypothesis* theHyp;

  const list<const SMESHDS_Hypothesis*>& hyps =
    GetUsedHypothesis(aMesh, aShape, /*ignoreAuxiliary=*/false);
  list <const SMESHDS_Hypothesis* >::const_iterator h = hyps.begin();
  if ( h == hyps.end())
  {
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;  // can work with no hypothesis
  }

  aStatus = HYP_OK;
  for ( ; h != hyps.end(); ++h )
  {
    if ( !_hypMaxElementVolume )
      _hypMaxElementVolume = dynamic_cast< const StdMeshers_MaxElementVolume*> ( *h );
    if ( !_viscousLayersHyp ) // several _viscousLayersHyp's allowed
      _viscousLayersHyp = dynamic_cast< const StdMeshers_ViscousLayers*> ( *h );
    if ( ! _hypParameters )
      _hypParameters = dynamic_cast< const NETGENPlugin_Hypothesis*> ( *h );

    if ( *h != _hypMaxElementVolume &&
         *h != _viscousLayersHyp &&
         *h != _hypParameters &&
         !dynamic_cast< const StdMeshers_ViscousLayers*>(*h)) // several VL hyps allowed
      aStatus = HYP_INCOMPATIBLE;
  }
  if ( _hypMaxElementVolume && _hypParameters )
    aStatus = HYP_INCOMPATIBLE;
  else if ( aStatus == HYP_OK && _viscousLayersHyp )
    error( _viscousLayersHyp->CheckHypothesis( aMesh, aShape, aStatus ));

  if ( _hypMaxElementVolume )
    _maxElementVolume = _hypMaxElementVolume->GetMaxVolume();

  return aStatus == HYP_OK;
}

//=============================================================================
/*!
 *Here we are going to use the NETGEN mesher
 */
//=============================================================================

bool NETGENPlugin_NETGEN_3D::Compute(SMESH_Mesh&         aMesh,
                                     const TopoDS_Shape& aShape)
{
  netgen::multithread.terminate = 0;
  netgen::multithread.task = "Volume meshing";
  _progressByTic = -1.;

  SMESHDS_Mesh* meshDS = aMesh.GetMeshDS();

  SMESH_MesherHelper helper(aMesh);
  bool _quadraticMesh = helper.IsQuadraticSubMesh(aShape);
  helper.SetElementsOnShape( true );

  int Netgen_NbOfNodes = 0;
  double Netgen_point[3];
  int Netgen_triangle[3];

  NETGENPlugin_NetgenLibWrapper ngLib;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0,0)
  Ng_Mesh * Netgen_mesh = ngLib._ngMesh;
#else
  Ng_Mesh * Netgen_mesh = ngLib._ngMesh.get();
#endif

  // vector of nodes in which node index == netgen ID
  vector< const SMDS_MeshNode* > nodeVec;
  {
    const int invalid_ID = -1;

    SMESH::Controls::Area areaControl;
    SMESH::Controls::TSequenceOfXYZ nodesCoords;

    // maps nodes to ng ID
    typedef map< const SMDS_MeshNode*, int, TIDCompare > TNodeToIDMap;
    typedef TNodeToIDMap::value_type                     TN2ID;
    TNodeToIDMap nodeToNetgenID;

    // find internal shapes
    NETGENPlugin_Internals internals( aMesh, aShape, /*is3D=*/true );

    // ---------------------------------
    // Feed the Netgen with surface mesh
    // ---------------------------------

    TopAbs_ShapeEnum mainType = aMesh.GetShapeToMesh().ShapeType();
    bool checkReverse = ( mainType == TopAbs_COMPOUND || mainType == TopAbs_COMPSOLID );

    SMESH_ProxyMesh::Ptr proxyMesh( new SMESH_ProxyMesh( aMesh ));
    if ( _viscousLayersHyp )
    {
      netgen::multithread.percent = 3;
      proxyMesh = _viscousLayersHyp->Compute( aMesh, aShape );
      if ( !proxyMesh )
        return false;
    }
    if ( aMesh.NbQuadrangles() > 0 )
    {
      netgen::multithread.percent = 6;
      StdMeshers_QuadToTriaAdaptor* Adaptor = new StdMeshers_QuadToTriaAdaptor;
      Adaptor->Compute(aMesh,aShape,proxyMesh.get());
      proxyMesh.reset( Adaptor );
    }

    for ( TopExp_Explorer exFa( aShape, TopAbs_FACE ); exFa.More(); exFa.Next())
    {
      const TopoDS_Shape& aShapeFace = exFa.Current();
      int faceID = meshDS->ShapeToIndex( aShapeFace );
      bool isInternalFace = internals.isInternalShape( faceID );
      bool isRev = false;
      if ( checkReverse && !isInternalFace &&
           helper.NbAncestors(aShapeFace, aMesh, aShape.ShapeType()) > 1 )
        // IsReversedSubMesh() can work wrong on strongly curved faces,
        // so we use it as less as possible
        isRev = helper.IsReversedSubMesh( TopoDS::Face( aShapeFace ));

      const SMESHDS_SubMesh * aSubMeshDSFace = proxyMesh->GetSubMesh( aShapeFace );
      if ( !aSubMeshDSFace ) continue;
      SMDS_ElemIteratorPtr iteratorElem = aSubMeshDSFace->GetElements();
      while ( iteratorElem->more() ) // loop on elements on a geom face
      {
        // check mesh face
        const SMDS_MeshElement* elem = iteratorElem->next();
        if ( !elem )
          return error( COMPERR_BAD_INPUT_MESH, "Null element encounters");
        if ( elem->NbCornerNodes() != 3 )
          return error( COMPERR_BAD_INPUT_MESH, "Not triangle element encounters");

        // Add nodes of triangles and triangles them-selves to netgen mesh

        // add three nodes of triangle
        bool hasDegen = false;
        for ( int iN = 0; iN < 3; ++iN )
        {
          const SMDS_MeshNode* node = elem->GetNode( iN );
          const int shapeID = node->getshapeId();
          if ( node->GetPosition()->GetTypeOfPosition() == SMDS_TOP_EDGE &&
               helper.IsDegenShape( shapeID ))
          {
            // ignore all nodes on degeneraged edge and use node on its vertex instead
            TopoDS_Shape vertex = TopoDS_Iterator( meshDS->IndexToShape( shapeID )).Value();
            node = SMESH_Algo::VertexNode( TopoDS::Vertex( vertex ), meshDS );
            hasDegen = true;
          }
          int& ngID = nodeToNetgenID.insert(TN2ID( node, invalid_ID )).first->second;
          if ( ngID == invalid_ID )
          {
            ngID = ++Netgen_NbOfNodes;
            Netgen_point [ 0 ] = node->X();
            Netgen_point [ 1 ] = node->Y();
            Netgen_point [ 2 ] = node->Z();
            Ng_AddPoint(Netgen_mesh, Netgen_point);
          }
          Netgen_triangle[ isRev ? 2-iN : iN ] = ngID;
        }
        // add triangle
        if ( hasDegen && (Netgen_triangle[0] == Netgen_triangle[1] ||
                          Netgen_triangle[0] == Netgen_triangle[2] ||
                          Netgen_triangle[2] == Netgen_triangle[1] ))
          continue;

        Ng_AddSurfaceElement(Netgen_mesh, NG_TRIG, Netgen_triangle);

        if ( isInternalFace && !proxyMesh->IsTemporary( elem ))
        {
          swap( Netgen_triangle[1], Netgen_triangle[2] );
          Ng_AddSurfaceElement(Netgen_mesh, NG_TRIG, Netgen_triangle);
        }
      } // loop on elements on a face
    } // loop on faces of a SOLID or SHELL

    // insert old nodes into nodeVec
    nodeVec.resize( nodeToNetgenID.size() + 1, 0 );
    TNodeToIDMap::iterator n_id = nodeToNetgenID.begin();
    for ( ; n_id != nodeToNetgenID.end(); ++n_id )
      nodeVec[ n_id->second ] = n_id->first;
    nodeToNetgenID.clear();

    if ( internals.hasInternalVertexInSolid() )
    {
      netgen::OCCGeometry occgeo;
      NETGENPlugin_Mesher::AddIntVerticesInSolids( occgeo,
                                                   (netgen::Mesh&) *Netgen_mesh,
                                                   nodeVec,
                                                   internals);
    }
  }

  // -------------------------
  // Generate the volume mesh
  // -------------------------

  return ( ngLib._isComputeOk = compute( aMesh, helper, nodeVec, Netgen_mesh));
}

//namespace
//{
//  inline void limitVolumeSize( netgen::Mesh* ngMesh,
//                        double        maxh )
//  {
//    // get average h of faces
//    double faceh = 0;
//    int nbh = 0;
//    for (int i = 1; i <= ngMesh->GetNSE(); i++)
//    {
//      const netgen::Element2d& face = ngMesh->SurfaceElement(i);
//      for (int j=1; j <= face.GetNP(); ++j)
//      {
//        const netgen::PointIndex & i1 = face.PNumMod(j);
//        const netgen::PointIndex & i2 = face.PNumMod(j+1);
//        if ( i1 < i2 )
//        {
//          const netgen::Point3d & p1 = ngMesh->Point( i1 );
//          const netgen::Point3d & p2 = ngMesh->Point( i2 );
//          faceh += netgen::Dist2( p1, p2 );
//          nbh++;
//        }
//      }
//    }
//    faceh = Sqrt( faceh / nbh );

//    double compareh;
//    if      ( faceh < 0.5 * maxh ) compareh = -1;
//    else if ( faceh > 1.5 * maxh ) compareh = 1;
//    else                           compareh = 0;
//    // cerr << "faceh " << faceh << std::endl;
//    // cerr << "init maxh " << maxh << std::endl;
//    // cerr << "compareh " << compareh << std::endl;

//    if ( compareh > 0 )
//      maxh *= 1.2;
//    else
//      maxh *= 0.8;
//    // cerr << "maxh " << maxh << std::endl;

//    // get bnd box
//    netgen::Point3d pmin, pmax;
//    ngMesh->GetBox( pmin, pmax, 0 );
//    const double dx = pmax.X() - pmin.X();
//    const double dy = pmax.Y() - pmin.Y();
//    const double dz = pmax.Z() - pmin.Z();

//    if ( ! & ngMesh->LocalHFunction() )
//      ngMesh->SetLocalH( pmin, pmax, compareh <= 0 ? 0.1 : 0.5 );

//    // adjusted by SALOME_TESTS/Grids/smesh/bugs_08/I8
//    const int nbX = Max( 2, int( dx / maxh * 2 ));
//    const int nbY = Max( 2, int( dy / maxh * 2 ));
//    const int nbZ = Max( 2, int( dz / maxh * 2 ));

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

//================================================================================
/*!
 * \brief set parameters and generate the volume mesh
 */
//================================================================================

bool NETGENPlugin_NETGEN_3D::compute(SMESH_Mesh&                     aMesh,
                                     SMESH_MesherHelper&             helper,
                                     vector< const SMDS_MeshNode* >& nodeVec,
                                     Ng_Mesh *                       Netgen_mesh)
{
  netgen::multithread.terminate = 0;

  netgen::Mesh* ngMesh = (netgen::Mesh*)Netgen_mesh;
  int Netgen_NbOfNodes = Ng_GetNP(Netgen_mesh);

#if NETGEN_VERSION < NETGEN_VERSION_STRING(5,0,0)
  char *optstr = 0;
#endif

#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2,0)
  int startWith = netgen::MESHCONST_MESHVOLUME;
  int endWith   = netgen::MESHCONST_OPTVOLUME;
#else
  netgen::mparam.perfstepsstart = netgen::MESHCONST_MESHVOLUME;
  netgen::mparam.perfstepsend = netgen::MESHCONST_OPTVOLUME;
#endif
  int err = 1;

  NETGENPlugin_Mesher aMesher( &aMesh, helper.GetSubShape(), /*isVolume=*/true );
  netgen::OCCGeometry occgeo;
  
  if ( _hypParameters )
  {
    aMesher.SetParameters( _hypParameters );
    if ( !_hypParameters->GetOptimize() )
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,2,0)
      endWith = netgen::MESHCONST_MESHVOLUME;
#else
      netgen::mparam.perfstepsend = netgen::MESHCONST_MESHVOLUME;
#endif
  }
  else if ( _hypMaxElementVolume )
  {
    netgen::mparam.maxh = pow( 72, 1/6. ) * pow( _maxElementVolume, 1/3. );
    // limitVolumeSize( ngMesh, netgen::mparam.maxh ); // result is unpredictable
  }
  else if ( aMesh.HasShapeToMesh() )
  {
    aMesher.PrepareOCCgeometry( occgeo, helper.GetSubShape(), aMesh );
    netgen::mparam.maxh = occgeo.GetBoundingBox().Diam()/2;
  }
  else
  {
    netgen::Point3d pmin, pmax;
    ngMesh->GetBox (pmin, pmax);
    netgen::mparam.maxh = Dist(pmin, pmax)/2;
  }

  if ( !_hypParameters && aMesh.HasShapeToMesh() )
  {
    netgen::mparam.minh = aMesher.GetDefaultMinSize( helper.GetSubShape(), netgen::mparam.maxh );
  }

  try
  {
    OCC_CATCH_SIGNALS;
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,0,0)
    std::shared_ptr<netgen::Mesh> mesh_ptr(ngMesh,  [](netgen::Mesh*){});
#if NETGEN_VERSION >= NETGEN_VERSION_STRING(6,2,0)
    err = netgen::OCCGenerateMesh(occgeo, mesh_ptr, netgen::mparam);
#else
    err = netgen::OCCGenerateMesh(occgeo, mesh_ptr, netgen::mparam, startWith, endWith);
#endif
#elif NETGEN_VERSION >= NETGEN_VERSION_STRING(5,0,0)
    ngMesh->CalcLocalH(netgen::mparam.grading);
    err = netgen::OCCGenerateMesh(occgeo, ngMesh, netgen::mparam, startWith, endWith);
#else
    ngMesh->CalcLocalH();
    err = netgen::OCCGenerateMesh(occgeo, ngMesh, startWith, endWith, optstr);
#endif
    if(netgen::multithread.terminate)
      return false;
    if ( err )
      error(SMESH_Comment("Error in netgen::OCCGenerateMesh() at ") << netgen::multithread.task);
  }
  catch (Standard_Failure& ex)
  {
    SMESH_Comment str("Exception in  netgen::OCCGenerateMesh()");
    str << " at " << netgen::multithread.task
        << ": " << ex.DynamicType()->Name();
    if ( ex.GetMessageString() && strlen( ex.GetMessageString() ))
      str << ": " << ex.GetMessageString();
    error(str);
  }
  catch (netgen::NgException exc)
  {
    SMESH_Comment str("NgException");
    if ( strlen( netgen::multithread.task ) > 0 )
      str << " at " << netgen::multithread.task;
    str << ": " << exc.What();
    error(str);
  }
  catch (...)
  {
    SMESH_Comment str("Exception in  netgen::OCCGenerateMesh()");
    if ( strlen( netgen::multithread.task ) > 0 )
      str << " at " << netgen::multithread.task;
    error(str);
  }

  int Netgen_NbOfNodesNew = Ng_GetNP(Netgen_mesh);
  int Netgen_NbOfTetra    = Ng_GetNE(Netgen_mesh);

  MESSAGE("End of Volume Mesh Generation. err=" << err <<
          ", nb new nodes: " << Netgen_NbOfNodesNew - Netgen_NbOfNodes <<
          ", nb tetra: " << Netgen_NbOfTetra);

  // -------------------------------------------------------------------
  // Feed back the SMESHDS with the generated Nodes and Volume Elements
  // -------------------------------------------------------------------

  if ( err )
  {
    SMESH_ComputeErrorPtr ce = NETGENPlugin_Mesher::ReadErrors(nodeVec);
    if ( ce && !ce->myBadElements.empty() )
      error( ce );
  }

  bool isOK = ( /*status == NG_OK &&*/ Netgen_NbOfTetra > 0 );// get whatever built
  if ( isOK )
  {
    double Netgen_point[3];
    int    Netgen_tetrahedron[4];

    // create and insert new nodes into nodeVec
    nodeVec.resize( Netgen_NbOfNodesNew + 1, 0 );
    int nodeIndex = Netgen_NbOfNodes + 1;
    for ( ; nodeIndex <= Netgen_NbOfNodesNew; ++nodeIndex )
    {
      Ng_GetPoint( Netgen_mesh, nodeIndex, Netgen_point );
      nodeVec.at(nodeIndex) = helper.AddNode(Netgen_point[0], Netgen_point[1], Netgen_point[2]);
    }

    // create tetrahedrons
    for ( int elemIndex = 1; elemIndex <= Netgen_NbOfTetra; ++elemIndex )
    {
      Ng_GetVolumeElement(Netgen_mesh, elemIndex, Netgen_tetrahedron);
      try
      {
        helper.AddVolume (nodeVec.at( Netgen_tetrahedron[0] ),
                          nodeVec.at( Netgen_tetrahedron[1] ),
                          nodeVec.at( Netgen_tetrahedron[2] ),
                          nodeVec.at( Netgen_tetrahedron[3] ));
      }
      catch (...)
      {
      }
    }
  }

  return !err;
}

//================================================================================
/*!
 * \brief Compute tetrahedral mesh from 2D mesh without geometry
 */
//================================================================================

bool NETGENPlugin_NETGEN_3D::Compute(SMESH_Mesh&         aMesh,
                                     SMESH_MesherHelper* aHelper)
{
  const int invalid_ID = -1;

  netgen::multithread.terminate = 0;
  _progressByTic = -1.;

  SMESH_MesherHelper::MType MeshType = aHelper->IsQuadraticMesh();
  if ( MeshType == SMESH_MesherHelper::COMP )
    return error( COMPERR_BAD_INPUT_MESH,
                  SMESH_Comment("Mesh with linear and quadratic elements given"));

  aHelper->SetIsQuadratic( MeshType == SMESH_MesherHelper::QUADRATIC );

  // ---------------------------------
  // Feed the Netgen with surface mesh
  // ---------------------------------

  int Netgen_NbOfNodes = 0;
  int Netgen_param2ndOrder = 0;
  double Netgen_paramFine = 1.;
  double Netgen_paramSize = pow( 72, 1/6. ) * pow( _maxElementVolume, 1/3. );

  double Netgen_point[3];
  int Netgen_triangle[3];
  int Netgen_tetrahedron[4];

  NETGENPlugin_NetgenLibWrapper ngLib;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0,0)
  Ng_Mesh * Netgen_mesh = ngLib._ngMesh;
#else
  Ng_Mesh * Netgen_mesh = ngLib._ngMesh.get();
#endif

  SMESH_ProxyMesh::Ptr proxyMesh( new SMESH_ProxyMesh( aMesh ));
  if ( aMesh.NbQuadrangles() > 0 )
  {
    StdMeshers_QuadToTriaAdaptor* Adaptor = new StdMeshers_QuadToTriaAdaptor;
    Adaptor->Compute(aMesh);
    proxyMesh.reset( Adaptor );
  }

  // maps nodes to ng ID
  typedef map< const SMDS_MeshNode*, int, TIDCompare > TNodeToIDMap;
  typedef TNodeToIDMap::value_type                     TN2ID;
  TNodeToIDMap nodeToNetgenID;

  SMDS_ElemIteratorPtr fIt = proxyMesh->GetFaces();
  while( fIt->more())
  {
    // check element
    const SMDS_MeshElement* elem = fIt->next();
    if ( !elem )
      return error( COMPERR_BAD_INPUT_MESH, "Null element encounters");
    if ( elem->NbCornerNodes() != 3 )
      return error( COMPERR_BAD_INPUT_MESH, "Not triangle element encounters");
      
    // add three nodes of triangle
    for ( int iN = 0; iN < 3; ++iN )
    {
      const SMDS_MeshNode* node = elem->GetNode( iN );
      int& ngID = nodeToNetgenID.insert(TN2ID( node, invalid_ID )).first->second;
      if ( ngID == invalid_ID )
      {
        ngID = ++Netgen_NbOfNodes;
        Netgen_point [ 0 ] = node->X();
        Netgen_point [ 1 ] = node->Y();
        Netgen_point [ 2 ] = node->Z();
        Ng_AddPoint(Netgen_mesh, Netgen_point);
      }
      Netgen_triangle[ iN ] = ngID;
    }
    Ng_AddSurfaceElement(Netgen_mesh, NG_TRIG, Netgen_triangle);
  }
  proxyMesh.reset(); // delete tmp faces

  // vector of nodes in which node index == netgen ID
  vector< const SMDS_MeshNode* > nodeVec ( nodeToNetgenID.size() + 1 );
  // insert old nodes into nodeVec
  TNodeToIDMap::iterator n_id = nodeToNetgenID.begin();
  for ( ; n_id != nodeToNetgenID.end(); ++n_id )
    nodeVec.at( n_id->second ) = n_id->first;
  nodeToNetgenID.clear();

  // -------------------------
  // Generate the volume mesh
  // -------------------------

  return ( ngLib._isComputeOk = compute( aMesh, *aHelper, nodeVec, Netgen_mesh));
}

void NETGENPlugin_NETGEN_3D::CancelCompute()
{
  SMESH_Algo::CancelCompute();
  netgen::multithread.terminate = 1;
}

//================================================================================
/*!
 * \brief Return Compute progress
 */
//================================================================================

double NETGENPlugin_NETGEN_3D::GetProgress() const
{
  double res;
  const char* volMeshing = "Volume meshing";
  const char* dlnMeshing = "Delaunay meshing";
  const double meshingRatio = 0.15;
  const_cast<NETGENPlugin_NETGEN_3D*>( this )->_progressTic++;

  if ( _progressByTic < 0. &&
       ( strncmp( netgen::multithread.task, dlnMeshing, 3 ) == 0 ||
         strncmp( netgen::multithread.task, volMeshing, 3 ) == 0 ))
  {
    res = 0.001 + meshingRatio * netgen::multithread.percent / 100.;
    //cout << netgen::multithread.task << " " <<_progressTic << "-" << netgen::multithread.percent << std::endl;
  }
  else // different otimizations
  {
    if ( _progressByTic < 0. )
      ((NETGENPlugin_NETGEN_3D*)this)->_progressByTic = meshingRatio / _progressTic;
    res = _progressByTic * _progressTic;
    //cout << netgen::multithread.task << " " << _progressTic << " " << res << std::endl;
  }
  return Min ( res, 0.98 );
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool NETGENPlugin_NETGEN_3D::Evaluate(SMESH_Mesh& aMesh,
                                      const TopoDS_Shape& aShape,
                                      MapShapeNbElems& aResMap)
{
  int nbtri = 0, nbqua = 0;
  double fullArea = 0.0;
  for (TopExp_Explorer expF(aShape, TopAbs_FACE); expF.More(); expF.Next()) {
    TopoDS_Face F = TopoDS::Face( expF.Current() );
    SMESH_subMesh *sm = aMesh.GetSubMesh(F);
    MapShapeNbElemsItr anIt = aResMap.find(sm);
    if( anIt==aResMap.end() ) {
      SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
      smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
      return false;
    }
    std::vector<int> aVec = (*anIt).second;
    nbtri += Max(aVec[SMDSEntity_Triangle],aVec[SMDSEntity_Quad_Triangle]);
    nbqua += Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
    GProp_GProps G;
    BRepGProp::SurfaceProperties(F,G);
    double anArea = G.Mass();
    fullArea += anArea;
  }

  // collect info from edges
  int nb0d_e = 0, nb1d_e = 0;
  bool IsQuadratic = false;
  bool IsFirst = true;
  TopTools_MapOfShape tmpMap;
  for (TopExp_Explorer expF(aShape, TopAbs_EDGE); expF.More(); expF.Next()) {
    TopoDS_Edge E = TopoDS::Edge(expF.Current());
    if( tmpMap.Contains(E) )
      continue;
    tmpMap.Add(E);
    SMESH_subMesh *aSubMesh = aMesh.GetSubMesh(expF.Current());
    MapShapeNbElemsItr anIt = aResMap.find(aSubMesh);
    if( anIt==aResMap.end() ) {
      SMESH_ComputeErrorPtr& smError = aSubMesh->GetComputeError();
      smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,
                                            "Submesh can not be evaluated",this));
      return false;
    }
    std::vector<int> aVec = (*anIt).second;
    nb0d_e += aVec[SMDSEntity_Node];
    nb1d_e += Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
    if(IsFirst) {
      IsQuadratic = (aVec[SMDSEntity_Quad_Edge] > aVec[SMDSEntity_Edge]);
      IsFirst = false;
    }
  }
  tmpMap.Clear();

  double ELen_face = sqrt(2.* ( fullArea/(nbtri+nbqua*2) ) / sqrt(3.0) );
  double ELen_vol = pow( 72, 1/6. ) * pow( _maxElementVolume, 1/3. );
  double ELen = Min(ELen_vol,ELen_face*2);

  GProp_GProps G;
  BRepGProp::VolumeProperties(aShape,G);
  double aVolume = G.Mass();
  double tetrVol = 0.1179*ELen*ELen*ELen;
  double CoeffQuality = 0.9;
  int nbVols = int( aVolume/tetrVol/CoeffQuality );
  int nb1d_f = (nbtri*3 + nbqua*4 - nb1d_e) / 2;
  int nb1d_in = (nbVols*6 - nb1d_e - nb1d_f ) / 5;
  std::vector<int> aVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i]=0;
  if( IsQuadratic ) {
    aVec[SMDSEntity_Node] = nb1d_in/6 + 1 + nb1d_in;
    aVec[SMDSEntity_Quad_Tetra] = nbVols - nbqua*2;
    aVec[SMDSEntity_Quad_Pyramid] = nbqua;
  }
  else {
    aVec[SMDSEntity_Node] = nb1d_in/6 + 1;
    aVec[SMDSEntity_Tetra] = nbVols - nbqua*2;
    aVec[SMDSEntity_Pyramid] = nbqua;
  }
  SMESH_subMesh *sm = aMesh.GetSubMesh(aShape);
  aResMap.insert(std::make_pair(sm,aVec));
  
  return true;
}


