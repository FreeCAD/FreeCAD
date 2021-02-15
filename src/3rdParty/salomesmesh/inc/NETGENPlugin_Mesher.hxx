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
// File      : NETGENPlugin_Mesher.hxx
// Author    : Michael Sazonov (OCN)
// Date      : 31/03/2006
// Project   : SALOME
//
#ifndef _NETGENPlugin_Mesher_HXX_
#define _NETGENPlugin_Mesher_HXX_
#include <TopTools_IndexedMapOfShape.hxx>
#include "NETGENPlugin_Defs.hxx"

#include <StdMeshers_FaceSide.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMESH_Algo.hxx>
#include <SMESH_ProxyMesh.hxx>

#define NETGEN_VERSION_STRING(a,b,c) (a << 16) + (b << 8) + (c)

namespace nglib {
#include <nglib.h>
}

#include <map>
#include <vector>
#include <set>
#include <memory>

class SMESHDS_Mesh;
class SMESH_Comment;
class SMESH_Mesh;
class SMESH_MesherHelper;
class TopoDS_Shape;
// class TopTools_IndexedMapOfShape;
class NETGENPlugin_Hypothesis;
class NETGENPlugin_SimpleHypothesis_2D;
class NETGENPlugin_Internals;
namespace netgen {
  class OCCGeometry;
  class Mesh;
}
//=============================================================================
/*!
 * \brief Struct storing nb of entities in netgen mesh
 */
//=============================================================================

struct NETGENPlugin_ngMeshInfo
{
  int _nbNodes, _nbSegments, _nbFaces, _nbVolumes;
  char* _copyOfLocalH;
  NETGENPlugin_ngMeshInfo( netgen::Mesh* ngMesh=0);
  void transferLocalH( netgen::Mesh* fromMesh, netgen::Mesh* toMesh );
  void restoreLocalH ( netgen::Mesh* ngMesh);
};

//================================================================================
/*!
 * \brief It correctly initializes netgen library at constructor and
 *        correctly finishes using netgen library at destructor
 */
//================================================================================

struct NETGENPLUGIN_EXPORT NETGENPlugin_NetgenLibWrapper
{
  bool             _isComputeOk;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0,0)
  nglib::Ng_Mesh * _ngMesh;
#else
  std::shared_ptr<nglib::Ng_Mesh> _ngMesh;
#endif

  NETGENPlugin_NetgenLibWrapper();
  ~NETGENPlugin_NetgenLibWrapper();
  void setMesh( nglib::Ng_Mesh* mesh );

 private:
  std::string getOutputFileName();
  void        removeOutputFile();
  std::string _outputFileName;

  std::streambuf* _coutBuffer;   // to re-/store cout.rdbuf()
};

//=============================================================================
/*!
 * \brief This class calls the NETGEN mesher of OCC geometry
 */
//=============================================================================

class NETGENPLUGIN_EXPORT NETGENPlugin_Mesher 
{
 public:
  // ---------- PUBLIC METHODS ----------

  NETGENPlugin_Mesher (SMESH_Mesh* mesh, const TopoDS_Shape& aShape,
                       const bool isVolume);
  ~NETGENPlugin_Mesher();
  void SetSelfPointer( NETGENPlugin_Mesher ** ptr );

  void SetParameters(const NETGENPlugin_Hypothesis*          hyp);
  void SetParameters(const NETGENPlugin_SimpleHypothesis_2D* hyp);
  void SetViscousLayers2DAssigned(bool isAssigned) { _isViscousLayers2D = isAssigned; }

  bool Compute();

  bool Evaluate(MapShapeNbElems& aResMap);

  double GetProgress(const SMESH_Algo* holder,
                     const int *       algoProgressTic,
                     const double *    algoProgress) const;

  static void PrepareOCCgeometry(netgen::OCCGeometry&          occgeom,
                                 const TopoDS_Shape&           shape,
                                 SMESH_Mesh&                   mesh,
                                 std::list< SMESH_subMesh* > * meshedSM=0,
                                 NETGENPlugin_Internals*       internalShapes=0);

  static double GetDefaultMinSize(const TopoDS_Shape& shape,
                                  const double        maxSize);

  static void RestrictLocalSize(netgen::Mesh& ngMesh,
                                const gp_XYZ& p,
                                double        size,
                                const bool    overrideMinH=true);

  static int FillSMesh(const netgen::OCCGeometry&          occgeom,
                       netgen::Mesh&                       ngMesh,
                       const NETGENPlugin_ngMeshInfo&      initState,
                       SMESH_Mesh&                         sMesh,
                       std::vector<const SMDS_MeshNode*>&  nodeVec,
                       SMESH_Comment&                      comment,
                       SMESH_MesherHelper*                 quadHelper=0);

  bool FillNgMesh(netgen::OCCGeometry&                occgeom,
                  netgen::Mesh&                       ngMesh,
                  std::vector<const SMDS_MeshNode*>&  nodeVec,
                  const std::list< SMESH_subMesh* > & meshedSM,
                  SMESH_MesherHelper*                 quadHelper=0,
                  SMESH_ProxyMesh::Ptr                proxyMesh=SMESH_ProxyMesh::Ptr());

  static void FixIntFaces(const netgen::OCCGeometry& occgeom,
                          netgen::Mesh&              ngMesh,
                          NETGENPlugin_Internals&    internalShapes);

  static bool FixFaceMesh(const netgen::OCCGeometry& occgeom,
                          netgen::Mesh&              ngMesh,
                          const int                  faceID);

  static void AddIntVerticesInFaces(const netgen::OCCGeometry&          occgeom,
                                    netgen::Mesh&                       ngMesh,
                                    std::vector<const SMDS_MeshNode*>&  nodeVec,
                                    NETGENPlugin_Internals&             internalShapes);

  static void AddIntVerticesInSolids(const netgen::OCCGeometry&         occgeom,
                                    netgen::Mesh&                       ngMesh,
                                    std::vector<const SMDS_MeshNode*>&  nodeVec,
                                    NETGENPlugin_Internals&             internalShapes);

  static SMESH_ComputeErrorPtr
    AddSegmentsToMesh(netgen::Mesh&                         ngMesh,
                      netgen::OCCGeometry&                  geom,
                      const TSideVector&                    wires,
                      SMESH_MesherHelper&                   helper,
                      std::vector< const SMDS_MeshNode* > & nodeVec,
                      const bool                            overrideMinH=true);

  void SetDefaultParameters();

  static void RemoveTmpFiles();

  static SMESH_ComputeErrorPtr ReadErrors(const std::vector< const SMDS_MeshNode* >& nodeVec);


  static void toPython( const netgen::Mesh* ngMesh,
                        const std::string&  pyFile); // debug

 private:

  SMESH_Mesh*          _mesh;
  const TopoDS_Shape&  _shape;
  bool                 _isVolume;
  bool                 _optimize;
  int                  _fineness;
  bool                 _isViscousLayers2D;
#if NETGEN_VERSION < NETGEN_VERSION_STRING(6,0,0)
  netgen::Mesh*        _ngMesh;
#else
  std::shared_ptr<netgen::Mesh> _ngMesh;
#endif
  netgen::OCCGeometry* _occgeom;

  int                  _curShapeIndex;
  volatile int         _progressTic;
  volatile double      _ticTime; // normalized [0,1] compute time per a SMESH_Algo::_progressTic
  volatile double      _totalTime;

  const NETGENPlugin_SimpleHypothesis_2D * _simpleHyp;

  // a pointer to NETGENPlugin_Mesher* field of the holder, that will be
  // nullified at destruction of this
  NETGENPlugin_Mesher ** _ptrToMe; 
};

//=============================================================================
/*!
 * \brief Container of info needed to solve problems with internal shapes.
 *
 * Issue 0020676. It is made up as a class to be ready to extract from NETGEN
 * and put in SMESH as soon as the same solution is needed somewhere else.
 * The approach is to precompute internal edges in 2D and internal faces in 3D
 * and put their mesh correctly (twice) into netgen mesh.
 * In 2D, this class finds internal edges in faces and their vertices.
 * In 3D, it additionally finds internal faces, their edges shared with other faces,
 * and their vertices shared by several internal edges. Nodes built on the found
 * shapes and mesh faces built on the found internal faces are to be doubled in
 * netgen mesh to emulate a "crack"
 *
 * For internal faces a more simple solution is found, which is just to duplicate
 * mesh faces on internal geom faces without modeling a "real crack". For this
 * reason findBorderElements() is no more used anywhere.
 */
//=============================================================================

class NETGENPLUGIN_EXPORT NETGENPlugin_Internals
{
  SMESH_Mesh&       _mesh;
  bool              _is3D;
  //2D
  std::map<int,int> _e2face;//!<edges and their vertices in faces where they are TopAbs_INTERNAL
  std::map<int,std::list<int> > _f2v;//!<faces with internal vertices
  // 3D
  std::set<int>     _intShapes;
  std::set<int>     _borderFaces; //!< non-internal faces sharing the internal edge
  std::map<int,std::list<int> > _s2v;//!<solids with internal vertices

public:
  NETGENPlugin_Internals( SMESH_Mesh& mesh, const TopoDS_Shape& shape, bool is3D );

  SMESH_Mesh& getMesh() const;

  bool isShapeToPrecompute(const TopoDS_Shape& s);

  // 2D meshing
  // edges 
  bool hasInternalEdges() const { return !_e2face.empty(); }
  bool isInternalEdge( int id ) const { return _e2face.count( id ); }
  const std::map<int,int>& getEdgesAndVerticesWithFaces() const { return _e2face; }
  void getInternalEdges( TopTools_IndexedMapOfShape&  fmap,
                         TopTools_IndexedMapOfShape&  emap,
                         TopTools_IndexedMapOfShape&  vmap,
                         std::list< SMESH_subMesh* > smToPrecompute[]);
  // vertices
  bool hasInternalVertexInFace() const { return !_f2v.empty(); }
  const std::map<int,std::list<int> >& getFacesWithVertices() const { return _f2v; }

  // 3D meshing
  // faces
  bool hasInternalFaces() const { return !_intShapes.empty(); }
  bool isInternalShape( int id ) const { return _intShapes.count( id ); }
  void findBorderElements( std::set< const SMDS_MeshElement*, TIDCompare > & borderElems );
  bool isBorderFace( int faceID ) const { return _borderFaces.count( faceID ); }
  void getInternalFaces( TopTools_IndexedMapOfShape&  fmap,
                         TopTools_IndexedMapOfShape&  emap,
                         std::list< SMESH_subMesh* >& facesSM,
                         std::list< SMESH_subMesh* >& boundarySM);
  // vertices
  bool hasInternalVertexInSolid() const { return !_s2v.empty(); }
  bool hasInternalVertexInSolid(int soID ) const { return _s2v.count(soID); }
  const std::map<int,std::list<int> >& getSolidsWithVertices() const { return _s2v; }


};

#endif
