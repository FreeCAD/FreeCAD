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

// File      : StdMeshers_ViscousLayers.cxx
// Created   : Wed Dec  1 15:15:34 2010
// Author    : Edward AGAPOV (eap)

#include "StdMeshers_ViscousLayers.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_FacePosition.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMESHDS_Group.hxx"
#include "SMESHDS_Hypothesis.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_ComputeError.hxx"
#include "SMESH_ControlsDef.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Group.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_ProxyMesh.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_ViscousLayers2D.hxx"

#include <Adaptor3d_HSurface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Curve2d.hxx>
#include <BRepAdaptor_Surface.hxx>
//#include <BRepLProp_CLProps.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepOffsetAPI_MakeOffsetShape.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_B2d.hxx>
#include <Bnd_B3d.hxx>
#include <ElCLib.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomLib.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_MapIteratorOfMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax1.hxx>
#include <gp_Cone.hxx>
#include <gp_Sphere.hxx>
#include <gp_Vec.hxx>
#include <gp_XY.hxx>

#include <cmath>
#include <limits>
#include <list>
#include <queue>
#include <string>

#ifdef _DEBUG_
//#define __myDEBUG
//#define __NOT_INVALIDATE_BAD_SMOOTH
//#define __NODES_AT_POS
#endif

#define INCREMENTAL_SMOOTH // smooth only if min angle is too small
#define BLOCK_INFLATION // of individual _LayerEdge's
#define OLD_NEF_POLYGON

using namespace std;

//================================================================================
namespace VISCOUS_3D
{
  typedef int TGeomID;

  enum UIndex { U_TGT = 1, U_SRC, LEN_TGT };

  const double theMinSmoothCosin = 0.1;
  const double theSmoothThickToElemSizeRatio = 0.3;
  const double theMinSmoothTriaAngle = 30;
  const double theMinSmoothQuadAngle = 45;

  // what part of thickness is allowed till intersection
  // (defined by SALOME_TESTS/Grids/smesh/viscous_layers_00/A5)
  const double theThickToIntersection = 1.5;

  bool needSmoothing( double cosin, double tgtThick, double elemSize )
  {
    return cosin * tgtThick > theSmoothThickToElemSizeRatio * elemSize;
  }
  double getSmoothingThickness( double cosin, double elemSize )
  {
    return theSmoothThickToElemSizeRatio * elemSize / cosin;
  }

  /*!
   * \brief SMESH_ProxyMesh computed by _ViscousBuilder for a SOLID.
   * It is stored in a SMESH_subMesh of the SOLID as SMESH_subMeshEventListenerData
   */
  struct _MeshOfSolid : public SMESH_ProxyMesh,
                        public SMESH_subMeshEventListenerData
  {
    bool                  _n2nMapComputed;
    SMESH_ComputeErrorPtr _warning;

    _MeshOfSolid( SMESH_Mesh* mesh)
      :SMESH_subMeshEventListenerData( /*isDeletable=*/true),_n2nMapComputed(false)
    {
      SMESH_ProxyMesh::setMesh( *mesh );
    }

    // returns submesh for a geom face
    SMESH_ProxyMesh::SubMesh* getFaceSubM(const TopoDS_Face& F, bool create=false)
    {
      TGeomID i = SMESH_ProxyMesh::shapeIndex(F);
      return create ? SMESH_ProxyMesh::getProxySubMesh(i) : findProxySubMesh(i);
    }
    void setNode2Node(const SMDS_MeshNode*                 srcNode,
                      const SMDS_MeshNode*                 proxyNode,
                      const SMESH_ProxyMesh::SubMesh* subMesh)
    {
      SMESH_ProxyMesh::setNode2Node( srcNode,proxyNode,subMesh);
    }
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Listener of events of 3D sub-meshes computed with viscous layers.
   * It is used to clear an inferior dim sub-meshes modified by viscous layers
   */
  class _ShrinkShapeListener : SMESH_subMeshEventListener
  {
    _ShrinkShapeListener()
      : SMESH_subMeshEventListener(/*isDeletable=*/false,
                                   "StdMeshers_ViscousLayers::_ShrinkShapeListener") {}
  public:
    static SMESH_subMeshEventListener* Get() { static _ShrinkShapeListener l; return &l; }
    virtual void ProcessEvent(const int                       event,
                              const int                       eventType,
                              SMESH_subMesh*                  solidSM,
                              SMESH_subMeshEventListenerData* data,
                              const SMESH_Hypothesis*         hyp)
    {
      if ( SMESH_subMesh::COMPUTE_EVENT == eventType && solidSM->IsEmpty() && data )
      {
        SMESH_subMeshEventListener::ProcessEvent(event,eventType,solidSM,data,hyp);
      }
    }
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Listener of events of 3D sub-meshes computed with viscous layers.
   * It is used to store data computed by _ViscousBuilder for a sub-mesh and to
   * delete the data as soon as it has been used
   */
  class _ViscousListener : SMESH_subMeshEventListener
  {
    _ViscousListener():
      SMESH_subMeshEventListener(/*isDeletable=*/false,
                                 "StdMeshers_ViscousLayers::_ViscousListener") {}
    static SMESH_subMeshEventListener* Get() { static _ViscousListener l; return &l; }
  public:
    virtual void ProcessEvent(const int                       event,
                              const int                       eventType,
                              SMESH_subMesh*                  subMesh,
                              SMESH_subMeshEventListenerData* data,
                              const SMESH_Hypothesis*         hyp)
    {
      if (( SMESH_subMesh::COMPUTE_EVENT       == eventType ) &&
          ( SMESH_subMesh::CHECK_COMPUTE_STATE != event &&
            SMESH_subMesh::SUBMESH_COMPUTED    != event ))
      {
        // delete SMESH_ProxyMesh containing temporary faces
        subMesh->DeleteEventListener( this );
      }
    }
    // Finds or creates proxy mesh of the solid
    static _MeshOfSolid* GetSolidMesh(SMESH_Mesh*         mesh,
                                      const TopoDS_Shape& solid,
                                      bool                toCreate=false)
    {
      if ( !mesh ) return 0;
      SMESH_subMesh* sm = mesh->GetSubMesh(solid);
      _MeshOfSolid* data = (_MeshOfSolid*) sm->GetEventListenerData( Get() );
      if ( !data && toCreate )
      {
        data = new _MeshOfSolid(mesh);
        data->mySubMeshes.push_back( sm ); // to find SOLID by _MeshOfSolid
        sm->SetEventListener( Get(), data, sm );
      }
      return data;
    }
    // Removes proxy mesh of the solid
    static void RemoveSolidMesh(SMESH_Mesh* mesh, const TopoDS_Shape& solid)
    {
      mesh->GetSubMesh(solid)->DeleteEventListener( _ViscousListener::Get() );
    }
  };
  
  //================================================================================
  /*!
   * \brief sets a sub-mesh event listener to clear sub-meshes of sub-shapes of
   * the main shape when sub-mesh of the main shape is cleared,
   * for example to clear sub-meshes of FACEs when sub-mesh of a SOLID
   * is cleared
   */
  //================================================================================

  void ToClearSubWithMain( SMESH_subMesh* sub, const TopoDS_Shape& main)
  {
    SMESH_subMesh* mainSM = sub->GetFather()->GetSubMesh( main );
    SMESH_subMeshEventListenerData* data =
      mainSM->GetEventListenerData( _ShrinkShapeListener::Get());
    if ( data )
    {
      if ( find( data->mySubMeshes.begin(), data->mySubMeshes.end(), sub ) ==
           data->mySubMeshes.end())
        data->mySubMeshes.push_back( sub );
    }
    else
    {
      data = SMESH_subMeshEventListenerData::MakeData( /*dependent=*/sub );
      sub->SetEventListener( _ShrinkShapeListener::Get(), data, /*whereToListenTo=*/mainSM );
    }
  }
  struct _SolidData;
  //--------------------------------------------------------------------------------
  /*!
   * \brief Simplex (triangle or tetrahedron) based on 1 (tria) or 2 (tet) nodes of
   * _LayerEdge and 2 nodes of the mesh surface beening smoothed.
   * The class is used to check validity of face or volumes around a smoothed node;
   * it stores only 2 nodes as the other nodes are stored by _LayerEdge.
   */
  struct _Simplex
  {
    const SMDS_MeshNode *_nPrev, *_nNext; // nodes on a smoothed mesh surface
    const SMDS_MeshNode *_nOpp; // in 2D case, a node opposite to a smoothed node in QUAD
    _Simplex(const SMDS_MeshNode* nPrev=0,
             const SMDS_MeshNode* nNext=0,
             const SMDS_MeshNode* nOpp=0)
      : _nPrev(nPrev), _nNext(nNext), _nOpp(nOpp) {}
    bool IsForward(const gp_XYZ* pntSrc, const gp_XYZ* pntTgt, double& vol) const
    {
      const double M[3][3] =
        {{ _nNext->X() - pntSrc->X(), _nNext->Y() - pntSrc->Y(), _nNext->Z() - pntSrc->Z() },
         { pntTgt->X() - pntSrc->X(), pntTgt->Y() - pntSrc->Y(), pntTgt->Z() - pntSrc->Z() },
         { _nPrev->X() - pntSrc->X(), _nPrev->Y() - pntSrc->Y(), _nPrev->Z() - pntSrc->Z() }};
      vol = ( + M[0][0] * M[1][1] * M[2][2]
              + M[0][1] * M[1][2] * M[2][0]
              + M[0][2] * M[1][0] * M[2][1]
              - M[0][0] * M[1][2] * M[2][1]
              - M[0][1] * M[1][0] * M[2][2]
              - M[0][2] * M[1][1] * M[2][0]);
      return vol > 1e-100;
    }
    bool IsForward(const SMDS_MeshNode* nSrc, const gp_XYZ& pTgt, double& vol) const
    {
      SMESH_TNodeXYZ pSrc( nSrc );
      return IsForward( &pSrc, &pTgt, vol );
    }
    bool IsForward(const gp_XY&         tgtUV,
                   const SMDS_MeshNode* smoothedNode,
                   const TopoDS_Face&   face,
                   SMESH_MesherHelper&  helper,
                   const double         refSign) const
    {
      gp_XY prevUV = helper.GetNodeUV( face, _nPrev, smoothedNode );
      gp_XY nextUV = helper.GetNodeUV( face, _nNext, smoothedNode );
      gp_Vec2d v1( tgtUV, prevUV ), v2( tgtUV, nextUV );
      double d = v1 ^ v2;
      return d*refSign > 1e-100;
    }
    bool IsMinAngleOK( const gp_XYZ& pTgt, double& minAngle ) const
    {
      SMESH_TNodeXYZ pPrev( _nPrev ), pNext( _nNext );
      if ( !_nOpp ) // triangle
      {
        gp_Vec tp( pPrev - pTgt ), pn( pNext - pPrev ), nt( pTgt - pNext );
        double tp2 = tp.SquareMagnitude();
        double pn2 = pn.SquareMagnitude();
        double nt2 = nt.SquareMagnitude();

        if ( tp2 < pn2 && tp2 < nt2 )
          minAngle = ( nt * -pn ) * ( nt * -pn ) / nt2 / pn2;
        else if ( pn2 < nt2 )
          minAngle = ( tp * -nt ) * ( tp * -nt ) / tp2 / nt2;
        else
          minAngle = ( pn * -tp ) * ( pn * -tp ) / pn2 / tp2;

        static double theMaxCos2 = ( Cos( theMinSmoothTriaAngle * M_PI / 180. ) *
                                     Cos( theMinSmoothTriaAngle * M_PI / 180. ));
        return minAngle < theMaxCos2;
      }
      else // quadrangle
      {
        SMESH_TNodeXYZ pOpp( _nOpp );
        gp_Vec tp( pPrev - pTgt ), po( pOpp - pPrev ), on( pNext - pOpp), nt( pTgt - pNext );
        double tp2 = tp.SquareMagnitude();
        double po2 = po.SquareMagnitude();
        double on2 = on.SquareMagnitude();
        double nt2 = nt.SquareMagnitude();
        minAngle = Max( Max((( tp * -nt ) * ( tp * -nt ) / tp2 / nt2 ),
                            (( po * -tp ) * ( po * -tp ) / po2 / tp2 )),
                        Max((( on * -po ) * ( on * -po ) / on2 / po2 ),
                            (( nt * -on ) * ( nt * -on ) / nt2 / on2 )));

        static double theMaxCos2 = ( Cos( theMinSmoothQuadAngle * M_PI / 180. ) *
                                     Cos( theMinSmoothQuadAngle * M_PI / 180. ));
        return minAngle < theMaxCos2;
      }
    }
    bool IsNeighbour(const _Simplex& other) const
    {
      return _nPrev == other._nNext || _nNext == other._nPrev;
    }
    bool Includes( const SMDS_MeshNode* node ) const { return _nPrev == node || _nNext == node; }
    static void GetSimplices( const SMDS_MeshNode* node,
                              vector<_Simplex>&   simplices,
                              const set<TGeomID>& ingnoreShapes,
                              const _SolidData*   dataToCheckOri = 0,
                              const bool          toSort = false);
    static void SortSimplices(vector<_Simplex>& simplices);
  };
  //--------------------------------------------------------------------------------
  /*!
   * Structure used to take into account surface curvature while smoothing
   */
  struct _Curvature
  {
    double   _r; // radius
    double   _k; // factor to correct node smoothed position
    double   _h2lenRatio; // avgNormProj / (2*avgDist)
    gp_Pnt2d _uv; // UV used in putOnOffsetSurface()
  public:
    static _Curvature* New( double avgNormProj, double avgDist )
    {
      _Curvature* c = 0;
      if ( fabs( avgNormProj / avgDist ) > 1./200 )
      {
        c = new _Curvature;
        c->_r = avgDist * avgDist / avgNormProj;
        c->_k = avgDist * avgDist / c->_r / c->_r;
        //c->_k = avgNormProj / c->_r;
        c->_k *= ( c->_r < 0 ? 1/1.1 : 1.1 ); // not to be too restrictive
        c->_h2lenRatio = avgNormProj / ( avgDist + avgDist );

        c->_uv.SetCoord( 0., 0. );
      }
      return c;
    }
    double lenDelta(double len) const { return _k * ( _r + len ); }
    double lenDeltaByDist(double dist) const { return dist * _h2lenRatio; }
  };
  //--------------------------------------------------------------------------------

  struct _2NearEdges;
  struct _LayerEdge;
  struct _EdgesOnShape;
  struct _Smoother1D;
  typedef map< const SMDS_MeshNode*, _LayerEdge*, TIDCompare > TNode2Edge;

  //--------------------------------------------------------------------------------
  /*!
   * \brief Edge normal to surface, connecting a node on solid surface (_nodes[0])
   * and a node of the most internal layer (_nodes.back())
   */
  struct _LayerEdge
  {
    typedef gp_XYZ (_LayerEdge::*PSmooFun)();

    vector< const SMDS_MeshNode*> _nodes;

    gp_XYZ              _normal;    // to boundary of solid
    vector<gp_XYZ>      _pos;       // points computed during inflation
    double              _len;       // length achieved with the last inflation step
    double              _maxLen;    // maximal possible length
    double              _cosin;     // of angle (_normal ^ surface)
    double              _minAngle;  // of _simplices
    double              _lenFactor; // to compute _len taking _cosin into account
    int                 _flags;

    // simplices connected to the source node (_nodes[0]);
    // used for smoothing and quality check of _LayerEdge's based on the FACE
    vector<_Simplex>    _simplices;
    vector<_LayerEdge*> _neibors; // all surrounding _LayerEdge's
    PSmooFun            _smooFunction; // smoothing function
    _Curvature*         _curvature;
    // data for smoothing of _LayerEdge's based on the EDGE
    _2NearEdges*        _2neibors;

    enum EFlags { TO_SMOOTH       = 0x0000001,
                  MOVED           = 0x0000002, // set by _neibors[i]->SetNewLength()
                  SMOOTHED        = 0x0000004, // set by this->Smooth()
                  DIFFICULT       = 0x0000008, // near concave VERTEX
                  ON_CONCAVE_FACE = 0x0000010,
                  BLOCKED         = 0x0000020, // not to inflate any more
                  INTERSECTED     = 0x0000040, // close intersection with a face found
                  NORMAL_UPDATED  = 0x0000080,
                  MARKED          = 0x0000100, // local usage
                  MULTI_NORMAL    = 0x0000200, // a normal is invisible by some of surrounding faces
                  NEAR_BOUNDARY   = 0x0000400, // is near FACE boundary forcing smooth
                  SMOOTHED_C1     = 0x0000800, // is on _eosC1
                  DISTORTED       = 0x0001000, // was bad before smoothing
                  RISKY_SWOL      = 0x0002000, // SWOL is parallel to a source FACE
                  SHRUNK          = 0x0004000, // target node reached a tgt position while shrink()
                  UNUSED_FLAG     = 0x0100000  // to add use flags after
    };
    bool Is   ( int flag ) const { return _flags & flag; }
    void Set  ( int flag ) { _flags |= flag; }
    void Unset( int flag ) { _flags &= ~flag; }
    std::string DumpFlags() const; // debug

    void SetNewLength( double len, _EdgesOnShape& eos, SMESH_MesherHelper& helper );
    bool SetNewLength2d( Handle(Geom_Surface)& surface,
                         const TopoDS_Face&    F,
                         _EdgesOnShape&        eos,
                         SMESH_MesherHelper&   helper );
    void SetDataByNeighbors( const SMDS_MeshNode* n1,
                             const SMDS_MeshNode* n2,
                             const _EdgesOnShape& eos,
                             SMESH_MesherHelper&  helper);
    void Block( _SolidData& data );
    void InvalidateStep( size_t curStep, const _EdgesOnShape& eos, bool restoreLength=false );
    void ChooseSmooFunction(const set< TGeomID >& concaveVertices,
                            const TNode2Edge&     n2eMap);
    void SmoothPos( const vector< double >& segLen, const double tol );
    int  GetSmoothedPos( const double tol );
    int  Smooth(const int step, const bool isConcaveFace, bool findBest);
    int  Smooth(const int step, bool findBest, vector< _LayerEdge* >& toSmooth );
    int  CheckNeiborsOnBoundary(vector< _LayerEdge* >* badNeibors = 0, bool * needSmooth = 0 );
    void SmoothWoCheck();
    bool SmoothOnEdge(Handle(ShapeAnalysis_Surface)& surface,
                      const TopoDS_Face&             F,
                      SMESH_MesherHelper&            helper);
    void MoveNearConcaVer( const _EdgesOnShape*    eov,
                           const _EdgesOnShape*    eos,
                           const int               step,
                           vector< _LayerEdge* > & badSmooEdges);
    bool FindIntersection( SMESH_ElementSearcher&   searcher,
                           double &                 distance,
                           const double&            epsilon,
                           _EdgesOnShape&           eos,
                           const SMDS_MeshElement** face = 0);
    bool SegTriaInter( const gp_Ax1&        lastSegment,
                       const gp_XYZ&        p0,
                       const gp_XYZ&        p1,
                       const gp_XYZ&        p2,
                       double&              dist,
                       const double&        epsilon) const;
    bool SegTriaInter( const gp_Ax1&        lastSegment,
                       const SMDS_MeshNode* n0,
                       const SMDS_MeshNode* n1,
                       const SMDS_MeshNode* n2,
                       double&              dist,
                       const double&        epsilon) const
    { return SegTriaInter( lastSegment,
                           SMESH_TNodeXYZ( n0 ), SMESH_TNodeXYZ( n1 ), SMESH_TNodeXYZ( n2 ),
                           dist, epsilon );
    }
    const gp_XYZ& PrevPos() const { return _pos[ _pos.size() - 2 ]; }
    gp_XYZ PrevCheckPos( _EdgesOnShape* eos=0 ) const;
    gp_Ax1 LastSegment(double& segLen, _EdgesOnShape& eos) const;
    gp_XY  LastUV( const TopoDS_Face& F, _EdgesOnShape& eos ) const;
    bool   IsOnEdge() const { return _2neibors; }
    gp_XYZ Copy( _LayerEdge& other, _EdgesOnShape& eos, SMESH_MesherHelper& helper );
    void   SetCosin( double cosin );
    void   SetNormal( const gp_XYZ& n ) { _normal = n; }
    int    NbSteps() const { return _pos.size() - 1; } // nb inlation steps
    bool   IsNeiborOnEdge( const _LayerEdge* edge ) const;
    void   SetSmooLen( double len ) { // set _len at which smoothing is needed
      _cosin = len; // as for _LayerEdge's on FACE _cosin is not used
    }
    double GetSmooLen() { return _cosin; } // for _LayerEdge's on FACE _cosin is not used

    gp_XYZ smoothLaplacian();
    gp_XYZ smoothAngular();
    gp_XYZ smoothLengthWeighted();
    gp_XYZ smoothCentroidal();
    gp_XYZ smoothNefPolygon();

    enum { FUN_LAPLACIAN, FUN_LENWEIGHTED, FUN_CENTROIDAL, FUN_NEFPOLY, FUN_ANGULAR, FUN_NB };
    static const int theNbSmooFuns = FUN_NB;
    static PSmooFun _funs[theNbSmooFuns];
    static const char* _funNames[theNbSmooFuns+1];
    int smooFunID( PSmooFun fun=0) const;
  };
  _LayerEdge::PSmooFun _LayerEdge::_funs[theNbSmooFuns] = { &_LayerEdge::smoothLaplacian,
                                                            &_LayerEdge::smoothLengthWeighted,
                                                            &_LayerEdge::smoothCentroidal,
                                                            &_LayerEdge::smoothNefPolygon,
                                                            &_LayerEdge::smoothAngular };
  const char* _LayerEdge::_funNames[theNbSmooFuns+1] = { "Laplacian",
                                                         "LengthWeighted",
                                                         "Centroidal",
                                                         "NefPolygon",
                                                         "Angular",
                                                         "None"};
  struct _LayerEdgeCmp
  {
    bool operator () (const _LayerEdge* e1, const _LayerEdge* e2) const
    {
      const bool cmpNodes = ( e1 && e2 && e1->_nodes.size() && e2->_nodes.size() );
      return cmpNodes ? ( e1->_nodes[0]->GetID() < e2->_nodes[0]->GetID()) : ( e1 < e2 );
    }
  };
  //--------------------------------------------------------------------------------
  /*!
   * A 2D half plane used by _LayerEdge::smoothNefPolygon()
   */
  struct _halfPlane
  {
    gp_XY _pos, _dir, _inNorm;
    bool IsOut( const gp_XY p, const double tol ) const
    {
      return _inNorm * ( p - _pos ) < -tol;
    }
    bool FindIntersection( const _halfPlane& hp, gp_XY & intPnt )
    {
      //const double eps = 1e-10;
      double D = _dir.Crossed( hp._dir );
      if ( fabs(D) < std::numeric_limits<double>::min())
        return false;
      gp_XY vec21 = _pos - hp._pos; 
      double u = hp._dir.Crossed( vec21 ) / D; 
      intPnt = _pos + _dir * u;
      return true;
    }
  };
  //--------------------------------------------------------------------------------
  /*!
   * Structure used to smooth a _LayerEdge based on an EDGE.
   */
  struct _2NearEdges
  {
    double               _wgt  [2]; // weights of _nodes
    _LayerEdge*          _edges[2];

     // normal to plane passing through _LayerEdge._normal and tangent of EDGE
    gp_XYZ*              _plnNorm;

    _2NearEdges() { _edges[0]=_edges[1]=0; _plnNorm = 0; }
    const SMDS_MeshNode* tgtNode(bool is2nd) {
      return _edges[is2nd] ? _edges[is2nd]->_nodes.back() : 0;
    }
    const SMDS_MeshNode* srcNode(bool is2nd) {
      return _edges[is2nd] ? _edges[is2nd]->_nodes[0] : 0;
    }
    void reverse() {
      std::swap( _wgt  [0], _wgt  [1] );
      std::swap( _edges[0], _edges[1] );
    }
    void set( _LayerEdge* e1, _LayerEdge* e2, double w1, double w2 ) {
      _edges[0] = e1; _edges[1] = e2; _wgt[0] = w1; _wgt[1] = w2;
    }
    bool include( const _LayerEdge* e ) {
      return ( _edges[0] == e || _edges[1] == e );
    }
  };


  //--------------------------------------------------------------------------------
  /*!
   * \brief Layers parameters got by averaging several hypotheses
   */
  struct AverageHyp
  {
    AverageHyp( const StdMeshers_ViscousLayers* hyp = 0 )
      :_nbLayers(0), _nbHyps(0), _method(0), _thickness(0), _stretchFactor(0)
    {
      Add( hyp );
    }
    void Add( const StdMeshers_ViscousLayers* hyp )
    {
      if ( hyp )
      {
        _nbHyps++;
        _nbLayers       = hyp->GetNumberLayers();
        //_thickness     += hyp->GetTotalThickness();
        _thickness      = Max( _thickness, hyp->GetTotalThickness() );
        _stretchFactor += hyp->GetStretchFactor();
        _method         = hyp->GetMethod();
      }
    }
    double GetTotalThickness() const { return _thickness; /*_nbHyps ? _thickness / _nbHyps : 0;*/ }
    double GetStretchFactor()  const { return _nbHyps ? _stretchFactor / _nbHyps : 0; }
    int    GetNumberLayers()   const { return _nbLayers; }
    int    GetMethod()         const { return _method; }

    bool   UseSurfaceNormal()  const
    { return _method == StdMeshers_ViscousLayers::SURF_OFFSET_SMOOTH; }
    bool   ToSmooth()          const
    { return _method == StdMeshers_ViscousLayers::SURF_OFFSET_SMOOTH; }
    bool   IsOffsetMethod()    const
    { return _method == StdMeshers_ViscousLayers::FACE_OFFSET; }

  private:
    int     _nbLayers, _nbHyps, _method;
    double  _thickness, _stretchFactor;
  };

  //--------------------------------------------------------------------------------
  /*!
   * \brief _LayerEdge's on a shape and other shape data
   */
  struct _EdgesOnShape
  {
    vector< _LayerEdge* > _edges;

    TopoDS_Shape          _shape;
    TGeomID               _shapeID;
    SMESH_subMesh *       _subMesh;
    // face or edge w/o layer along or near which _edges are inflated
    TopoDS_Shape          _sWOL;
    bool                  _isRegularSWOL; // w/o singularities
    // averaged StdMeshers_ViscousLayers parameters
    AverageHyp            _hyp;
    bool                  _toSmooth;
    _Smoother1D*          _edgeSmoother;
    vector< _EdgesOnShape* > _eosConcaVer; // edges at concave VERTEXes of a FACE
    vector< _EdgesOnShape* > _eosC1; // to smooth together several C1 continues shapes

    vector< gp_XYZ >         _faceNormals; // if _shape is FACE
    vector< _EdgesOnShape* > _faceEOS; // to get _faceNormals of adjacent FACEs

    Handle(ShapeAnalysis_Surface) _offsetSurf;
    _LayerEdge*                   _edgeForOffset;

    _SolidData*            _data; // parent SOLID

    TopAbs_ShapeEnum ShapeType() const
    { return _shape.IsNull() ? TopAbs_SHAPE : _shape.ShapeType(); }
    TopAbs_ShapeEnum SWOLType() const
    { return _sWOL.IsNull() ? TopAbs_SHAPE : _sWOL.ShapeType(); }
    bool             HasC1( const _EdgesOnShape* other ) const
    { return std::find( _eosC1.begin(), _eosC1.end(), other ) != _eosC1.end(); }
    bool             GetNormal( const SMDS_MeshElement* face, gp_Vec& norm );
    _SolidData&      GetData() const { return *_data; }

    _EdgesOnShape(): _shapeID(-1), _subMesh(0), _toSmooth(false), _edgeSmoother(0) {}
  };

  //--------------------------------------------------------------------------------
  /*!
   * \brief Convex FACE whose radius of curvature is less than the thickness of 
   *        layers. It is used to detect distortion of prisms based on a convex
   *        FACE and to update normals to enable further increasing the thickness
   */
  struct _ConvexFace
  {
    TopoDS_Face                     _face;

    // edges whose _simplices are used to detect prism distortion
    vector< _LayerEdge* >           _simplexTestEdges;

    // map a sub-shape to _SolidData::_edgesOnShape
    map< TGeomID, _EdgesOnShape* >  _subIdToEOS;

    bool                            _normalsFixed;

    bool GetCenterOfCurvature( _LayerEdge*         ledge,
                               BRepLProp_SLProps&  surfProp,
                               SMESH_MesherHelper& helper,
                               gp_Pnt &            center ) const;
    bool CheckPrisms() const;
  };

  //--------------------------------------------------------------------------------
  /*!
   * \brief Structure holding _LayerEdge's based on EDGEs that will collide
   *        at inflation up to the full thickness. A detected collision
   *        is fixed in updateNormals()
   */
  struct _CollisionEdges
  {
    _LayerEdge*           _edge;
    vector< _LayerEdge* > _intEdges; // each pair forms an intersected quadrangle
    const SMDS_MeshNode* nSrc(int i) const { return _intEdges[i]->_nodes[0]; }
    const SMDS_MeshNode* nTgt(int i) const { return _intEdges[i]->_nodes.back(); }
  };

  //--------------------------------------------------------------------------------
  /*!
   * \brief Data of a SOLID
   */
  struct _SolidData
  {
    typedef const StdMeshers_ViscousLayers* THyp;
    TopoDS_Shape                    _solid;
    TopTools_MapOfShape             _before; // SOLIDs to be computed before _solid
    TGeomID                         _index; // SOLID id
    _MeshOfSolid*                   _proxyMesh;
    list< THyp >                    _hyps;
    list< TopoDS_Shape >            _hypShapes;
    map< TGeomID, THyp >            _face2hyp; // filled if _hyps.size() > 1
    set< TGeomID >                  _reversedFaceIds;
    set< TGeomID >                  _ignoreFaceIds; // WOL FACEs and FACEs of other SOLIDs

    double                          _stepSize, _stepSizeCoeff, _geomSize;
    const SMDS_MeshNode*            _stepSizeNodes[2];

    TNode2Edge                      _n2eMap; // nodes and _LayerEdge's based on them

    // map to find _n2eMap of another _SolidData by a shrink shape shared by two _SolidData's
    map< TGeomID, TNode2Edge* >     _s2neMap;
    // _LayerEdge's with underlying shapes
    vector< _EdgesOnShape >         _edgesOnShape;

    // key:   an id of shape (EDGE or VERTEX) shared by a FACE with
    //        layers and a FACE w/o layers
    // value: the shape (FACE or EDGE) to shrink mesh on.
    //       _LayerEdge's basing on nodes on key shape are inflated along the value shape
    map< TGeomID, TopoDS_Shape >     _shrinkShape2Shape;

    // Convex FACEs whose radius of curvature is less than the thickness of layers
    map< TGeomID, _ConvexFace >      _convexFaces;

    // shapes (EDGEs and VERTEXes) srink from which is forbidden due to collisions with
    // the adjacent SOLID
    set< TGeomID >                   _noShrinkShapes;

    int                              _nbShapesToSmooth;

    //map< TGeomID,Handle(Geom_Curve)> _edge2curve;

    vector< _CollisionEdges >        _collisionEdges;
    set< TGeomID >                   _concaveFaces;

    double                           _maxThickness; // of all _hyps
    double                           _minThickness; // of all _hyps

    double                           _epsilon; // precision for SegTriaInter()

    SMESH_MesherHelper*              _helper;

    _SolidData(const TopoDS_Shape& s=TopoDS_Shape(),
               _MeshOfSolid*       m=0)
      :_solid(s), _proxyMesh(m), _helper(0) {}
    ~_SolidData();

    void SortOnEdge( const TopoDS_Edge& E, vector< _LayerEdge* >& edges);
    void Sort2NeiborsOnEdge( vector< _LayerEdge* >& edges );

    _ConvexFace* GetConvexFace( const TGeomID faceID ) {
      map< TGeomID, _ConvexFace >::iterator id2face = _convexFaces.find( faceID );
      return id2face == _convexFaces.end() ? 0 : & id2face->second;
    }
    _EdgesOnShape* GetShapeEdges(const TGeomID       shapeID );
    _EdgesOnShape* GetShapeEdges(const TopoDS_Shape& shape );
    _EdgesOnShape* GetShapeEdges(const _LayerEdge*   edge )
    { return GetShapeEdges( edge->_nodes[0]->getshapeId() ); }

    SMESH_MesherHelper& GetHelper() const { return *_helper; }

    void UnmarkEdges( int flag = _LayerEdge::MARKED ) {
      for ( size_t i = 0; i < _edgesOnShape.size(); ++i )
        for ( size_t j = 0; j < _edgesOnShape[i]._edges.size(); ++j )
          _edgesOnShape[i]._edges[j]->Unset( flag );
    }
    void AddShapesToSmooth( const set< _EdgesOnShape* >& shape,
                            const set< _EdgesOnShape* >* edgesNoAnaSmooth=0 );

    void PrepareEdgesToSmoothOnFace( _EdgesOnShape* eof, bool substituteSrcNodes );
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Offset plane used in getNormalByOffset()
   */
  struct _OffsetPlane
  {
    gp_Pln _plane;
    int    _faceIndex;
    int    _faceIndexNext[2];
    gp_Lin _lines[2]; // line of intersection with neighbor _OffsetPlane's
    bool   _isLineOK[2];
    _OffsetPlane() {
      _isLineOK[0] = _isLineOK[1] = false; _faceIndexNext[0] = _faceIndexNext[1] = -1;
    }
    void   ComputeIntersectionLine( _OffsetPlane&        pln, 
                                    const TopoDS_Edge&   E,
                                    const TopoDS_Vertex& V );
    gp_XYZ GetCommonPoint(bool& isFound, const TopoDS_Vertex& V) const;
    int    NbLines() const { return _isLineOK[0] + _isLineOK[1]; }
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Container of centers of curvature at nodes on an EDGE bounding _ConvexFace
   */
  struct _CentralCurveOnEdge
  {
    bool                  _isDegenerated;
    vector< gp_Pnt >      _curvaCenters;
    vector< _LayerEdge* > _ledges;
    vector< gp_XYZ >      _normals; // new normal for each of _ledges
    vector< double >      _segLength2;

    TopoDS_Edge           _edge;
    TopoDS_Face           _adjFace;
    bool                  _adjFaceToSmooth;

    void Append( const gp_Pnt& center, _LayerEdge* ledge )
    {
      if ( _curvaCenters.size() > 0 )
        _segLength2.push_back( center.SquareDistance( _curvaCenters.back() ));
      _curvaCenters.push_back( center );
      _ledges.push_back( ledge );
      _normals.push_back( ledge->_normal );
    }
    bool FindNewNormal( const gp_Pnt& center, gp_XYZ& newNormal );
    void SetShapes( const TopoDS_Edge&  edge,
                    const _ConvexFace&  convFace,
                    _SolidData&         data,
                    SMESH_MesherHelper& helper);
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Data of node on a shrinked FACE
   */
  struct _SmoothNode
  {
    const SMDS_MeshNode*         _node;
    vector<_Simplex>             _simplices; // for quality check

    enum SmoothType { LAPLACIAN, CENTROIDAL, ANGULAR, TFI };

    bool Smooth(int&                  badNb,
                Handle(Geom_Surface)& surface,
                SMESH_MesherHelper&   helper,
                const double          refSign,
                SmoothType            how,
                bool                  set3D);

    gp_XY computeAngularPos(vector<gp_XY>& uv,
                            const gp_XY&   uvToFix,
                            const double   refSign );
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Builder of viscous layers
   */
  class _ViscousBuilder
  {
  public:
    _ViscousBuilder();
    // does it's job
    SMESH_ComputeErrorPtr Compute(SMESH_Mesh&         mesh,
                                  const TopoDS_Shape& shape);
    // check validity of hypotheses
    SMESH_ComputeErrorPtr CheckHypotheses( SMESH_Mesh&         mesh,
                                           const TopoDS_Shape& shape );

    // restore event listeners used to clear an inferior dim sub-mesh modified by viscous layers
    void RestoreListeners();

    // computes SMESH_ProxyMesh::SubMesh::_n2n;
    bool MakeN2NMap( _MeshOfSolid* pm );

  private:

    bool findSolidsWithLayers();
    bool setBefore( _SolidData& solidBefore, _SolidData& solidAfter );
    bool findFacesWithLayers(const bool onlyWith=false);
    void getIgnoreFaces(const TopoDS_Shape&             solid,
                        const StdMeshers_ViscousLayers* hyp,
                        const TopoDS_Shape&             hypShape,
                        set<TGeomID>&                   ignoreFaces);
    bool makeLayer(_SolidData& data);
    void setShapeData( _EdgesOnShape& eos, SMESH_subMesh* sm, _SolidData& data );
    bool setEdgeData( _LayerEdge& edge, _EdgesOnShape& eos,
                      SMESH_MesherHelper& helper, _SolidData& data);
    gp_XYZ getFaceNormal(const SMDS_MeshNode* n,
                         const TopoDS_Face&   face,
                         SMESH_MesherHelper&  helper,
                         bool&                isOK,
                         bool                 shiftInside=false);
    bool getFaceNormalAtSingularity(const gp_XY&        uv,
                                    const TopoDS_Face&  face,
                                    SMESH_MesherHelper& helper,
                                    gp_Dir&             normal );
    gp_XYZ getWeigthedNormal( const _LayerEdge*                edge );
    gp_XYZ getNormalByOffset( _LayerEdge*                      edge,
                              std::pair< TopoDS_Face, gp_XYZ > fId2Normal[],
                              int                              nbFaces,
                              bool                             lastNoOffset = false);
    bool findNeiborsOnEdge(const _LayerEdge*     edge,
                           const SMDS_MeshNode*& n1,
                           const SMDS_MeshNode*& n2,
                           _EdgesOnShape&        eos,
                           _SolidData&           data);
    void findSimplexTestEdges( _SolidData&                    data,
                               vector< vector<_LayerEdge*> >& edgesByGeom);
    void computeGeomSize( _SolidData& data );
    bool findShapesToSmooth( _SolidData& data);
    void limitStepSizeByCurvature( _SolidData&  data );
    void limitStepSize( _SolidData&             data,
                        const SMDS_MeshElement* face,
                        const _LayerEdge*       maxCosinEdge );
    void limitStepSize( _SolidData& data, const double minSize);
    bool inflate(_SolidData& data);
    bool smoothAndCheck(_SolidData& data, const int nbSteps, double & distToIntersection);
    int  invalidateBadSmooth( _SolidData&               data,
                              SMESH_MesherHelper&       helper,
                              vector< _LayerEdge* >&    badSmooEdges,
                              vector< _EdgesOnShape* >& eosC1,
                              const int                 infStep );
    void makeOffsetSurface( _EdgesOnShape& eos, SMESH_MesherHelper& );
    void putOnOffsetSurface( _EdgesOnShape& eos, int infStep,
                             vector< _EdgesOnShape* >& eosC1,
                             int smooStep=0, bool moveAll=false );
    void findCollisionEdges( _SolidData& data, SMESH_MesherHelper& helper );
    void limitMaxLenByCurvature( _SolidData& data, SMESH_MesherHelper& helper );
    void limitMaxLenByCurvature( _LayerEdge* e1, _LayerEdge* e2,
                                 _EdgesOnShape& eos1, _EdgesOnShape& eos2,
                                 SMESH_MesherHelper& helper );
    bool updateNormals( _SolidData& data, SMESH_MesherHelper& helper, int stepNb, double stepSize );
    bool updateNormalsOfConvexFaces( _SolidData&         data,
                                     SMESH_MesherHelper& helper,
                                     int                 stepNb );
    void updateNormalsOfC1Vertices( _SolidData& data );
    bool updateNormalsOfSmoothed( _SolidData&         data,
                                  SMESH_MesherHelper& helper,
                                  const int           nbSteps,
                                  const double        stepSize );
    bool isNewNormalOk( _SolidData&   data,
                        _LayerEdge&   edge,
                        const gp_XYZ& newNormal);
    bool refine(_SolidData& data);
    bool shrink(_SolidData& data);
    bool prepareEdgeToShrink( _LayerEdge& edge, _EdgesOnShape& eos,
                              SMESH_MesherHelper& helper,
                              const SMESHDS_SubMesh* faceSubMesh );
    void restoreNoShrink( _LayerEdge& edge ) const;
    void fixBadFaces(const TopoDS_Face&          F,
                     SMESH_MesherHelper&         helper,
                     const bool                  is2D,
                     const int                   step,
                     set<const SMDS_MeshNode*> * involvedNodes=NULL);
    bool addBoundaryElements(_SolidData& data);

    bool error( const string& text, int solidID=-1 );
    SMESHDS_Mesh* getMeshDS() const { return _mesh->GetMeshDS(); }

    // debug
    void makeGroupOfLE();

    SMESH_Mesh*                _mesh;
    SMESH_ComputeErrorPtr      _error;

    vector<                    _SolidData >  _sdVec;
    TopTools_IndexedMapOfShape _solids; // to find _SolidData by a solid
    TopTools_MapOfShape        _shrinkedFaces;

    int                        _tmpFaceID;
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Shrinker of nodes on the EDGE
   */
  class _Shrinker1D
  {
    TopoDS_Edge                   _geomEdge;
    vector<double>                _initU;
    vector<double>                _normPar;
    vector<const SMDS_MeshNode*>  _nodes;
    const _LayerEdge*             _edges[2];
    bool                          _done;
  public:
    void AddEdge( const _LayerEdge* e, _EdgesOnShape& eos, SMESH_MesherHelper& helper );
    void Compute(bool set3D, SMESH_MesherHelper& helper);
    void RestoreParams();
    void SwapSrcTgtNodes(SMESHDS_Mesh* mesh);
    const TopoDS_Edge& GeomEdge() const { return _geomEdge; }
    const SMDS_MeshNode* TgtNode( bool is2nd ) const
    { return _edges[is2nd] ? _edges[is2nd]->_nodes.back() : 0; }
    const SMDS_MeshNode* SrcNode( bool is2nd ) const
    { return _edges[is2nd] ? _edges[is2nd]->_nodes[0] : 0; }
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Smoother of _LayerEdge's on EDGE.
   */
  struct _Smoother1D
  {
    struct OffPnt // point of the offsetted EDGE
    {
      gp_XYZ      _xyz;    // coord of a point inflated from EDGE w/o smooth
      double      _len;    // length reached at previous inflation step
      double      _param;  // on EDGE
      _2NearEdges _2edges; // 2 neighbor _LayerEdge's
      gp_XYZ      _edgeDir;// EDGE tangent at _param
      double Distance( const OffPnt& p ) const { return ( _xyz - p._xyz ).Modulus(); }
    };
    vector< OffPnt >   _offPoints;
    vector< double >   _leParams; // normalized param of _eos._edges on EDGE
    Handle(Geom_Curve) _anaCurve; // for analytic smooth
    _LayerEdge         _leOnV[2]; // _LayerEdge's holding normal to the EDGE at VERTEXes
    gp_XYZ             _edgeDir[2]; // tangent at VERTEXes
    size_t             _iSeg[2];  // index of segment where extreme tgt node is projected
    _EdgesOnShape&     _eos;
    double             _curveLen; // length of the EDGE

    static Handle(Geom_Curve) CurveForSmooth( const TopoDS_Edge&  E,
                                              _EdgesOnShape&      eos,
                                              SMESH_MesherHelper& helper);

    _Smoother1D( Handle(Geom_Curve) curveForSmooth,
                 _EdgesOnShape&     eos )
      : _anaCurve( curveForSmooth ), _eos( eos )
    {
    }
    bool Perform(_SolidData&                    data,
                 Handle(ShapeAnalysis_Surface)& surface,
                 const TopoDS_Face&             F,
                 SMESH_MesherHelper&            helper )
    {
      if ( _leParams.empty() || ( !isAnalytic() && _offPoints.empty() ))
        prepare( data );

      if ( isAnalytic() )
        return smoothAnalyticEdge( data, surface, F, helper );
      else
        return smoothComplexEdge ( data, surface, F, helper );
    }
    void prepare(_SolidData& data );

    bool smoothAnalyticEdge( _SolidData&                    data,
                             Handle(ShapeAnalysis_Surface)& surface,
                             const TopoDS_Face&             F,
                             SMESH_MesherHelper&            helper);

    bool smoothComplexEdge( _SolidData&                    data,
                            Handle(ShapeAnalysis_Surface)& surface,
                            const TopoDS_Face&             F,
                            SMESH_MesherHelper&            helper);

    gp_XYZ getNormalNormal( const gp_XYZ & normal,
                            const gp_XYZ&  edgeDir);

    _LayerEdge* getLEdgeOnV( bool is2nd )
    {
      return _eos._edges[ is2nd ? _eos._edges.size()-1 : 0 ]->_2neibors->_edges[ is2nd ];
    }
    bool isAnalytic() const { return !_anaCurve.IsNull(); }
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Class of temporary mesh face.
   * We can't use SMDS_FaceOfNodes since it's impossible to set it's ID which is
   * needed because SMESH_ElementSearcher internaly uses set of elements sorted by ID
   */
  struct _TmpMeshFace : public SMDS_MeshElement
  {
    vector<const SMDS_MeshNode* > _nn;
    _TmpMeshFace( const vector<const SMDS_MeshNode*>& nodes,
                  int id, int faceID=-1, int idInFace=-1):
      SMDS_MeshElement(id), _nn(nodes) { setShapeId(faceID); setIdInShape(idInFace); }
    virtual const SMDS_MeshNode* GetNode(const int ind) const { return _nn[ind]; }
    virtual SMDSAbs_ElementType  GetType() const              { return SMDSAbs_Face; }
    virtual vtkIdType GetVtkType() const                      { return -1; }
    virtual SMDSAbs_EntityType   GetEntityType() const        { return SMDSEntity_Last; }
    virtual SMDSAbs_GeometryType GetGeomType() const
    { return _nn.size() == 3 ? SMDSGeom_TRIANGLE : SMDSGeom_QUADRANGLE; }
    virtual SMDS_ElemIteratorPtr elementsIterator(SMDSAbs_ElementType) const
    { return SMDS_ElemIteratorPtr( new SMDS_NodeVectorElemIterator( _nn.begin(), _nn.end()));}
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Class of temporary mesh face storing _LayerEdge it's based on
   */
  struct _TmpMeshFaceOnEdge : public _TmpMeshFace
  {
    _LayerEdge *_le1, *_le2;
    _TmpMeshFaceOnEdge( _LayerEdge* le1, _LayerEdge* le2, int ID ):
      _TmpMeshFace( vector<const SMDS_MeshNode*>(4), ID ), _le1(le1), _le2(le2)
    {
      _nn[0]=_le1->_nodes[0];
      _nn[1]=_le1->_nodes.back();
      _nn[2]=_le2->_nodes.back();
      _nn[3]=_le2->_nodes[0];
    }
    gp_XYZ GetDir() const // return average direction of _LayerEdge's, normal to EDGE
    {
      SMESH_TNodeXYZ p0s( _nn[0] );
      SMESH_TNodeXYZ p0t( _nn[1] );
      SMESH_TNodeXYZ p1t( _nn[2] );
      SMESH_TNodeXYZ p1s( _nn[3] );
      gp_XYZ  v0 = p0t - p0s;
      gp_XYZ  v1 = p1t - p1s;
      gp_XYZ v01 = p1s - p0s;
      gp_XYZ   n = ( v0 ^ v01 ) + ( v1 ^ v01 );
      gp_XYZ   d = v01 ^ n;
      d.Normalize();
      return d;
    }
    gp_XYZ GetDir(_LayerEdge* le1, _LayerEdge* le2) // return average direction of _LayerEdge's
    {
      _nn[0]=le1->_nodes[0];
      _nn[1]=le1->_nodes.back();
      _nn[2]=le2->_nodes.back();
      _nn[3]=le2->_nodes[0];
      return GetDir();
    }
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Retriever of node coordinates either directly or from a surface by node UV.
   * \warning Location of a surface is ignored
   */
  struct _NodeCoordHelper
  {
    SMESH_MesherHelper&        _helper;
    const TopoDS_Face&         _face;
    Handle(Geom_Surface)       _surface;
    gp_XYZ (_NodeCoordHelper::* _fun)(const SMDS_MeshNode* n) const;

    _NodeCoordHelper(const TopoDS_Face& F, SMESH_MesherHelper& helper, bool is2D)
      : _helper( helper ), _face( F )
    {
      if ( is2D )
      {
        TopLoc_Location loc;
        _surface = BRep_Tool::Surface( _face, loc );
      }
      if ( _surface.IsNull() )
        _fun = & _NodeCoordHelper::direct;
      else
        _fun = & _NodeCoordHelper::byUV;
    }
    gp_XYZ operator()(const SMDS_MeshNode* n) const { return (this->*_fun)( n ); }

  private:
    gp_XYZ direct(const SMDS_MeshNode* n) const
    {
      return SMESH_TNodeXYZ( n );
    }
    gp_XYZ byUV  (const SMDS_MeshNode* n) const
    {
      gp_XY uv = _helper.GetNodeUV( _face, n );
      return _surface->Value( uv.X(), uv.Y() ).XYZ();
    }
  };

  //================================================================================
  /*!
   * \brief Check angle between vectors 
   */
  //================================================================================

  inline bool isLessAngle( const gp_Vec& v1, const gp_Vec& v2, const double cos )
  {
    double dot = v1 * v2; // cos * |v1| * |v2|
    double l1  = v1.SquareMagnitude();
    double l2  = v2.SquareMagnitude();
    return (( dot * cos >= 0 ) && 
            ( dot * dot ) / l1 / l2 >= ( cos * cos ));
  }

} // namespace VISCOUS_3D



//================================================================================
// StdMeshers_ViscousLayers hypothesis
//
StdMeshers_ViscousLayers::StdMeshers_ViscousLayers(int hypId, int studyId, SMESH_Gen* gen)
  :SMESH_Hypothesis(hypId, studyId, gen),
   _isToIgnoreShapes(1), _nbLayers(1), _thickness(1), _stretchFactor(1),
   _method( SURF_OFFSET_SMOOTH )
{
  _name = StdMeshers_ViscousLayers::GetHypType();
  _param_algo_dim = -3; // auxiliary hyp used by 3D algos
} // --------------------------------------------------------------------------------
void StdMeshers_ViscousLayers::SetBndShapes(const std::vector<int>& faceIds, bool toIgnore)
{
  if ( faceIds != _shapeIds )
    _shapeIds = faceIds, NotifySubMeshesHypothesisModification();
  if ( _isToIgnoreShapes != toIgnore )
    _isToIgnoreShapes = toIgnore, NotifySubMeshesHypothesisModification();
} // --------------------------------------------------------------------------------
void StdMeshers_ViscousLayers::SetTotalThickness(double thickness)
{
  if ( thickness != _thickness )
    _thickness = thickness, NotifySubMeshesHypothesisModification();
} // --------------------------------------------------------------------------------
void StdMeshers_ViscousLayers::SetNumberLayers(int nb)
{
  if ( _nbLayers != nb )
    _nbLayers = nb, NotifySubMeshesHypothesisModification();
} // --------------------------------------------------------------------------------
void StdMeshers_ViscousLayers::SetStretchFactor(double factor)
{
  if ( _stretchFactor != factor )
    _stretchFactor = factor, NotifySubMeshesHypothesisModification();
} // --------------------------------------------------------------------------------
void StdMeshers_ViscousLayers::SetMethod( ExtrusionMethod method )
{
  if ( _method != method )
    _method = method, NotifySubMeshesHypothesisModification();
} // --------------------------------------------------------------------------------
SMESH_ProxyMesh::Ptr
StdMeshers_ViscousLayers::Compute(SMESH_Mesh&         theMesh,
                                  const TopoDS_Shape& theShape,
                                  const bool          toMakeN2NMap) const
{
  using namespace VISCOUS_3D;
  _ViscousBuilder builder;
  SMESH_ComputeErrorPtr err = builder.Compute( theMesh, theShape );
  if ( err && !err->IsOK() )
    return SMESH_ProxyMesh::Ptr();

  vector<SMESH_ProxyMesh::Ptr> components;
  TopExp_Explorer exp( theShape, TopAbs_SOLID );
  for ( ; exp.More(); exp.Next() )
  {
    if ( _MeshOfSolid* pm =
         _ViscousListener::GetSolidMesh( &theMesh, exp.Current(), /*toCreate=*/false))
    {
      if ( toMakeN2NMap && !pm->_n2nMapComputed )
        if ( !builder.MakeN2NMap( pm ))
          return SMESH_ProxyMesh::Ptr();
      components.push_back( SMESH_ProxyMesh::Ptr( pm ));
      pm->myIsDeletable = false; // it will de deleted by boost::shared_ptr

      if ( pm->_warning && !pm->_warning->IsOK() )
      {
        SMESH_subMesh* sm = theMesh.GetSubMesh( exp.Current() );
        SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
        if ( !smError || smError->IsOK() )
          smError = pm->_warning;
      }
    }
    _ViscousListener::RemoveSolidMesh ( &theMesh, exp.Current() );
  }
  switch ( components.size() )
  {
  case 0: break;

  case 1: return components[0];

  default: return SMESH_ProxyMesh::Ptr( new SMESH_ProxyMesh( components ));
  }
  return SMESH_ProxyMesh::Ptr();
} // --------------------------------------------------------------------------------
std::ostream & StdMeshers_ViscousLayers::SaveTo(std::ostream & save)
{
  save << " " << _nbLayers
       << " " << _thickness
       << " " << _stretchFactor
       << " " << _shapeIds.size();
  for ( size_t i = 0; i < _shapeIds.size(); ++i )
    save << " " << _shapeIds[i];
  save << " " << !_isToIgnoreShapes; // negate to keep the behavior in old studies.
  save << " " << _method;
  return save;
} // --------------------------------------------------------------------------------
std::istream & StdMeshers_ViscousLayers::LoadFrom(std::istream & load)
{
  int nbFaces, faceID, shapeToTreat, method;
  load >> _nbLayers >> _thickness >> _stretchFactor >> nbFaces;
  while ( (int) _shapeIds.size() < nbFaces && load >> faceID )
    _shapeIds.push_back( faceID );
  if ( load >> shapeToTreat ) {
    _isToIgnoreShapes = !shapeToTreat;
    if ( load >> method )
      _method = (ExtrusionMethod) method;
  }
  else {
    _isToIgnoreShapes = true; // old behavior
  }
  return load;
} // --------------------------------------------------------------------------------
bool StdMeshers_ViscousLayers::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                   const TopoDS_Shape& theShape)
{
  // TODO
  return false;
} // --------------------------------------------------------------------------------
SMESH_ComputeErrorPtr
StdMeshers_ViscousLayers::CheckHypothesis(SMESH_Mesh&                          theMesh,
                                          const TopoDS_Shape&                  theShape,
                                          SMESH_Hypothesis::Hypothesis_Status& theStatus)
{
  VISCOUS_3D::_ViscousBuilder builder;
  SMESH_ComputeErrorPtr err = builder.CheckHypotheses( theMesh, theShape );
  if ( err && !err->IsOK() )
    theStatus = SMESH_Hypothesis::HYP_INCOMPAT_HYPS;
  else
    theStatus = SMESH_Hypothesis::HYP_OK;

  return err;
}
// --------------------------------------------------------------------------------
bool StdMeshers_ViscousLayers::IsShapeWithLayers(int shapeIndex) const
{
  bool isIn =
    ( std::find( _shapeIds.begin(), _shapeIds.end(), shapeIndex ) != _shapeIds.end() );
  return IsToIgnoreShapes() ? !isIn : isIn;
}
// END StdMeshers_ViscousLayers hypothesis
//================================================================================

namespace VISCOUS_3D
{
  gp_XYZ getEdgeDir( const TopoDS_Edge& E, const TopoDS_Vertex& fromV )
  {
    gp_Vec dir;
    double f,l;
    Handle(Geom_Curve) c = BRep_Tool::Curve( E, f, l );
    if ( c.IsNull() ) return gp_XYZ( Precision::Infinite(), 1e100, 1e100 );
    gp_Pnt p = BRep_Tool::Pnt( fromV );
    double distF = p.SquareDistance( c->Value( f ));
    double distL = p.SquareDistance( c->Value( l ));
    c->D1(( distF < distL ? f : l), p, dir );
    if ( distL < distF ) dir.Reverse();
    return dir.XYZ();
  }
  //--------------------------------------------------------------------------------
  gp_XYZ getEdgeDir( const TopoDS_Edge& E, const SMDS_MeshNode* atNode,
                     SMESH_MesherHelper& helper)
  {
    gp_Vec dir;
    double f,l; gp_Pnt p;
    Handle(Geom_Curve) c = BRep_Tool::Curve( E, f, l );
    if ( c.IsNull() ) return gp_XYZ( Precision::Infinite(), 1e100, 1e100 );
    double u = helper.GetNodeU( E, atNode );
    c->D1( u, p, dir );
    return dir.XYZ();
  }
  //--------------------------------------------------------------------------------
  gp_XYZ getFaceDir( const TopoDS_Face& F, const TopoDS_Vertex& fromV,
                     const SMDS_MeshNode* node, SMESH_MesherHelper& helper, bool& ok,
                     double* cosin=0);
  //--------------------------------------------------------------------------------
  gp_XYZ getFaceDir( const TopoDS_Face& F, const TopoDS_Edge& fromE,
                     const SMDS_MeshNode* node, SMESH_MesherHelper& helper, bool& ok)
  {
    double f,l;
    Handle(Geom_Curve) c = BRep_Tool::Curve( fromE, f, l );
    if ( c.IsNull() )
    {
      TopoDS_Vertex v = helper.IthVertex( 0, fromE );
      return getFaceDir( F, v, node, helper, ok );
    }
    gp_XY uv = helper.GetNodeUV( F, node, 0, &ok );
    Handle(Geom_Surface) surface = BRep_Tool::Surface( F );
    gp_Pnt p; gp_Vec du, dv, norm;
    surface->D1( uv.X(),uv.Y(), p, du,dv );
    norm = du ^ dv;

    double u = helper.GetNodeU( fromE, node, 0, &ok );
    c->D1( u, p, du );
    TopAbs_Orientation o = helper.GetSubShapeOri( F.Oriented(TopAbs_FORWARD), fromE);
    if ( o == TopAbs_REVERSED )
      du.Reverse();

    gp_Vec dir = norm ^ du;

    if ( node->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX &&
         helper.IsClosedEdge( fromE ))
    {
      if ( fabs(u-f) < fabs(u-l)) c->D1( l, p, dv );
      else                        c->D1( f, p, dv );
      if ( o == TopAbs_REVERSED )
        dv.Reverse();
      gp_Vec dir2 = norm ^ dv;
      dir = dir.Normalized() + dir2.Normalized();
    }
    return dir.XYZ();
  }
  //--------------------------------------------------------------------------------
  gp_XYZ getFaceDir( const TopoDS_Face& F, const TopoDS_Vertex& fromV,
                     const SMDS_MeshNode* node, SMESH_MesherHelper& helper,
                     bool& ok, double* cosin)
  {
    TopoDS_Face faceFrw = F;
    faceFrw.Orientation( TopAbs_FORWARD );
    //double f,l; TopLoc_Location loc;
    TopoDS_Edge edges[2]; // sharing a vertex
    size_t nbEdges = 0;
    {
      TopoDS_Vertex VV[2];
      TopExp_Explorer exp( faceFrw, TopAbs_EDGE );
      for ( ; exp.More() && nbEdges < 2; exp.Next() )
      {
        const TopoDS_Edge& e = TopoDS::Edge( exp.Current() );
        if ( SMESH_Algo::isDegenerated( e )) continue;
        TopExp::Vertices( e, VV[0], VV[1], /*CumOri=*/true );
        if ( VV[1].IsSame( fromV )) {
          nbEdges += edges[ 0 ].IsNull();
          edges[ 0 ] = e;
        }
        else if ( VV[0].IsSame( fromV )) {
          nbEdges += edges[ 1 ].IsNull();
          edges[ 1 ] = e;
        }
      }
    }
    gp_XYZ dir(0,0,0), edgeDir[2];
    if ( nbEdges == 2 )
    {
      // get dirs of edges going fromV
      ok = true;
      for ( size_t i = 0; i < nbEdges && ok; ++i )
      {
        edgeDir[i] = getEdgeDir( edges[i], fromV );
        double size2 = edgeDir[i].SquareModulus();
        if (( ok = size2 > numeric_limits<double>::min() ))
          edgeDir[i] /= sqrt( size2 );
      }
      if ( !ok ) return dir;

      // get angle between the 2 edges
      gp_Vec faceNormal;
      double angle = helper.GetAngle( edges[0], edges[1], faceFrw, fromV, &faceNormal );
      if ( Abs( angle ) < 5 * M_PI/180 )
      {
        dir = ( faceNormal.XYZ() ^ edgeDir[0].Reversed()) + ( faceNormal.XYZ() ^ edgeDir[1] );
      }
      else
      {
        dir = edgeDir[0] + edgeDir[1];
        if ( angle < 0 )
          dir.Reverse();
      }
      if ( cosin ) {
        double angle = gp_Vec( edgeDir[0] ).Angle( dir );
        *cosin = Cos( angle );
      }
    }
    else if ( nbEdges == 1 )
    {
      dir = getFaceDir( faceFrw, edges[ edges[0].IsNull() ], node, helper, ok );
      if ( cosin ) *cosin = 1.;
    }
    else
    {
      ok = false;
    }

    return dir;
  }

  //================================================================================
  /*!
   * \brief Finds concave VERTEXes of a FACE
   */
  //================================================================================

  bool getConcaveVertices( const TopoDS_Face&  F,
                           SMESH_MesherHelper& helper,
                           set< TGeomID >*     vertices = 0)
  {
    // check angles at VERTEXes
    TError error;
    TSideVector wires = StdMeshers_FaceSide::GetFaceWires( F, *helper.GetMesh(), 0, error );
    for ( size_t iW = 0; iW < wires.size(); ++iW )
    {
      const int nbEdges = wires[iW]->NbEdges();
      if ( nbEdges < 2 && SMESH_Algo::isDegenerated( wires[iW]->Edge(0)))
        continue;
      for ( int iE1 = 0; iE1 < nbEdges; ++iE1 )
      {
        if ( SMESH_Algo::isDegenerated( wires[iW]->Edge( iE1 ))) continue;
        int iE2 = ( iE1 + 1 ) % nbEdges;
        while ( SMESH_Algo::isDegenerated( wires[iW]->Edge( iE2 )))
          iE2 = ( iE2 + 1 ) % nbEdges;
        TopoDS_Vertex V = wires[iW]->FirstVertex( iE2 );
        double angle = helper.GetAngle( wires[iW]->Edge( iE1 ),
                                        wires[iW]->Edge( iE2 ), F, V );
        if ( angle < -5. * M_PI / 180. )
        {
          if ( !vertices )
            return true;
          vertices->insert( helper.GetMeshDS()->ShapeToIndex( V ));
        }
      }
    }
    return vertices ? !vertices->empty() : false;
  }

  //================================================================================
  /*!
   * \brief Returns true if a FACE is bound by a concave EDGE
   */
  //================================================================================

  bool isConcave( const TopoDS_Face&  F,
                  SMESH_MesherHelper& helper,
                  set< TGeomID >*     vertices = 0 )
  {
    bool isConcv = false;
    // if ( helper.Count( F, TopAbs_WIRE, /*useMap=*/false) > 1 )
    //   return true;
    gp_Vec2d drv1, drv2;
    gp_Pnt2d p;
    TopExp_Explorer eExp( F.Oriented( TopAbs_FORWARD ), TopAbs_EDGE );
    for ( ; eExp.More(); eExp.Next() )
    {
      const TopoDS_Edge& E = TopoDS::Edge( eExp.Current() );
      if ( SMESH_Algo::isDegenerated( E )) continue;
      // check if 2D curve is concave
      BRepAdaptor_Curve2d curve( E, F );
      const int nbIntervals = curve.NbIntervals( GeomAbs_C2 );
      TColStd_Array1OfReal intervals(1, nbIntervals + 1 );
      curve.Intervals( intervals, GeomAbs_C2 );
      bool isConvex = true;
      for ( int i = 1; i <= nbIntervals && isConvex; ++i )
      {
        double u1 = intervals( i );
        double u2 = intervals( i+1 );
        curve.D2( 0.5*( u1+u2 ), p, drv1, drv2 );
        double cross = drv1 ^ drv2;
        if ( E.Orientation() == TopAbs_REVERSED )
          cross = -cross;
        isConvex = ( cross > -1e-9 ); // 0.1 );
      }
      if ( !isConvex )
      {
        //cout << "Concave FACE " << helper.GetMeshDS()->ShapeToIndex( F ) << endl;
        isConcv = true;
        if ( vertices )
          break;
        else
          return true;
      }
    }

    // check angles at VERTEXes
    if ( getConcaveVertices( F, helper, vertices ))
      isConcv = true;

    return isConcv;
  }

  //================================================================================
  /*!
   * \brief Computes mimimal distance of face in-FACE nodes from an EDGE
   *  \param [in] face - the mesh face to treat
   *  \param [in] nodeOnEdge - a node on the EDGE
   *  \param [out] faceSize - the computed distance
   *  \return bool - true if faceSize computed
   */
  //================================================================================

  bool getDistFromEdge( const SMDS_MeshElement* face,
                        const SMDS_MeshNode*    nodeOnEdge,
                        double &                faceSize )
  {
    faceSize = Precision::Infinite();
    bool done = false;

    int nbN  = face->NbCornerNodes();
    int iOnE = face->GetNodeIndex( nodeOnEdge );
    int iNext[2] = { SMESH_MesherHelper::WrapIndex( iOnE+1, nbN ),
                     SMESH_MesherHelper::WrapIndex( iOnE-1, nbN ) };
    const SMDS_MeshNode* nNext[2] = { face->GetNode( iNext[0] ),
                                      face->GetNode( iNext[1] ) };
    gp_XYZ segVec, segEnd = SMESH_TNodeXYZ( nodeOnEdge ); // segment on EDGE
    double segLen = -1.;
    // look for two neighbor not in-FACE nodes of face
    for ( int i = 0; i < 2; ++i )
    {
      if ( nNext[i]->GetPosition()->GetDim() != 2 &&
           nNext[i]->GetID() < nodeOnEdge->GetID() )
      {
        // look for an in-FACE node
        for ( int iN = 0; iN < nbN; ++iN )
        {
          if ( iN == iOnE || iN == iNext[i] )
            continue;
          SMESH_TNodeXYZ pInFace = face->GetNode( iN );
          gp_XYZ v = pInFace - segEnd;
          if ( segLen < 0 )
          {
            segVec = SMESH_TNodeXYZ( nNext[i] ) - segEnd;
            segLen = segVec.Modulus();
          }
          double distToSeg = v.Crossed( segVec ).Modulus() / segLen;
          faceSize = Min( faceSize, distToSeg );
          done = true;
        }
        segLen = -1;
      }
    }
    return done;
  }
  //================================================================================
  /*!
   * \brief Return direction of axis or revolution of a surface
   */
  //================================================================================

  bool getRovolutionAxis( const Adaptor3d_Surface& surface,
                          gp_Dir &                 axis )
  {
    switch ( surface.GetType() ) {
    case GeomAbs_Cone:
    {
      gp_Cone cone = surface.Cone();
      axis = cone.Axis().Direction();
      break;
    }
    case GeomAbs_Sphere:
    {
      gp_Sphere sphere = surface.Sphere();
      axis = sphere.Position().Direction();
      break;
    }
    case GeomAbs_SurfaceOfRevolution:
    {
      axis = surface.AxeOfRevolution().Direction();
      break;
    }
    //case GeomAbs_SurfaceOfExtrusion:
    case GeomAbs_OffsetSurface:
    {
      Handle(Adaptor3d_HSurface) base = surface.BasisSurface();
      return getRovolutionAxis( base->Surface(), axis );
    }
    default: return false;
    }
    return true;
  }

  //--------------------------------------------------------------------------------
  // DEBUG. Dump intermediate node positions into a python script
  // HOWTO use: run python commands written in a console to see
  //  construction steps of viscous layers
#ifdef __myDEBUG
  ofstream* py;
  int       theNbPyFunc;
  struct PyDump {
    PyDump(SMESH_Mesh& m) {
      int tag = 3 + m.GetId();
      const char* fname = "/tmp/viscous.py";
      cout << "execfile('"<<fname<<"')"<<endl;
      py = new ofstream(fname);
      *py << "import SMESH" << endl
          << "from salome.smesh import smeshBuilder" << endl
          << "smesh  = smeshBuilder.New(salome.myStudy)" << endl
          << "meshSO = smesh.GetCurrentStudy().FindObjectID('0:1:2:" << tag <<"')" << endl
          << "mesh   = smesh.Mesh( meshSO.GetObject() )"<<endl;
      theNbPyFunc = 0;
    }
    void Finish() {
      if (py) {
        *py << "mesh.GroupOnFilter(SMESH.VOLUME,'Viscous Prisms',"
          "smesh.GetFilter(SMESH.VOLUME,SMESH.FT_ElemGeomType,'=',SMESH.Geom_PENTA))"<<endl;
        *py << "mesh.GroupOnFilter(SMESH.VOLUME,'Neg Volumes',"
          "smesh.GetFilter(SMESH.VOLUME,SMESH.FT_Volume3D,'<',0))"<<endl;
      }
      delete py; py=0;
    }
    ~PyDump() { Finish(); cout << "NB FUNCTIONS: " << theNbPyFunc << endl; }
  };
#define dumpFunction(f) { _dumpFunction(f, __LINE__);}
#define dumpMove(n)     { _dumpMove(n, __LINE__);}
#define dumpMoveComm(n,txt) { _dumpMove(n, __LINE__, txt);}
#define dumpCmd(txt)    { _dumpCmd(txt, __LINE__);}
  void _dumpFunction(const string& fun, int ln)
  { if (py) *py<< "def "<<fun<<"(): # "<< ln <<endl; cout<<fun<<"()"<<endl; ++theNbPyFunc; }
  void _dumpMove(const SMDS_MeshNode* n, int ln, const char* txt="")
  { if (py) *py<< "  mesh.MoveNode( "<<n->GetID()<< ", "<< n->X()
               << ", "<<n->Y()<<", "<< n->Z()<< ")\t\t # "<< ln <<" "<< txt << endl; }
  void _dumpCmd(const string& txt, int ln)
  { if (py) *py<< "  "<<txt<<" # "<< ln <<endl; }
  void dumpFunctionEnd()
  { if (py) *py<< "  return"<< endl; }
  void dumpChangeNodes( const SMDS_MeshElement* f )
  { if (py) { *py<< "  mesh.ChangeElemNodes( " << f->GetID()<<", [";
      for ( int i=1; i < f->NbNodes(); ++i ) *py << f->GetNode(i-1)->GetID()<<", ";
      *py << f->GetNode( f->NbNodes()-1 )->GetID() << " ])"<< endl; }}
#define debugMsg( txt ) { cout << "# "<< txt << " (line: " << __LINE__ << ")" << endl; }

#else

  struct PyDump { PyDump(SMESH_Mesh&) {} void Finish() {} };
#define dumpFunction(f) f
#define dumpMove(n)
#define dumpMoveComm(n,txt)
#define dumpCmd(txt)
#define dumpFunctionEnd()
#define dumpChangeNodes(f) { if(f) {} } // prevent "unused variable 'f'" warning
#define debugMsg( txt ) {}

#endif
}

using namespace VISCOUS_3D;

//================================================================================
/*!
 * \brief Constructor of _ViscousBuilder
 */
//================================================================================

_ViscousBuilder::_ViscousBuilder()
{
  _error = SMESH_ComputeError::New(COMPERR_OK);
  _tmpFaceID = 0;
}

//================================================================================
/*!
 * \brief Stores error description and returns false
 */
//================================================================================

bool _ViscousBuilder::error(const string& text, int solidId )
{
  const string prefix = string("Viscous layers builder: ");
  _error->myName    = COMPERR_ALGO_FAILED;
  _error->myComment = prefix + text;
  if ( _mesh )
  {
    SMESH_subMesh* sm = _mesh->GetSubMeshContaining( solidId );
    if ( !sm && !_sdVec.empty() )
      sm = _mesh->GetSubMeshContaining( solidId = _sdVec[0]._index );
    if ( sm && sm->GetSubShape().ShapeType() == TopAbs_SOLID )
    {
      SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
      if ( smError && smError->myAlgo )
        _error->myAlgo = smError->myAlgo;
      smError = _error;
      sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
    }
    // set KO to all solids
    for ( size_t i = 0; i < _sdVec.size(); ++i )
    {
      if ( _sdVec[i]._index == solidId )
        continue;
      sm = _mesh->GetSubMesh( _sdVec[i]._solid );
      if ( !sm->IsEmpty() )
        continue;
      SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
      if ( !smError || smError->IsOK() )
      {
        smError = SMESH_ComputeError::New( COMPERR_ALGO_FAILED, prefix + "failed");
        sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
      }
    }
  }
  makeGroupOfLE(); // debug

  return false;
}

//================================================================================
/*!
 * \brief At study restoration, restore event listeners used to clear an inferior
 *  dim sub-mesh modified by viscous layers
 */
//================================================================================

void _ViscousBuilder::RestoreListeners()
{
  // TODO
}

//================================================================================
/*!
 * \brief computes SMESH_ProxyMesh::SubMesh::_n2n
 */
//================================================================================

bool _ViscousBuilder::MakeN2NMap( _MeshOfSolid* pm )
{
  SMESH_subMesh* solidSM = pm->mySubMeshes.front();
  TopExp_Explorer fExp( solidSM->GetSubShape(), TopAbs_FACE );
  for ( ; fExp.More(); fExp.Next() )
  {
    SMESHDS_SubMesh* srcSmDS = pm->GetMeshDS()->MeshElements( fExp.Current() );
    const SMESH_ProxyMesh::SubMesh* prxSmDS = pm->GetProxySubMesh( fExp.Current() );

    if ( !srcSmDS || !prxSmDS || !srcSmDS->NbElements() || !prxSmDS->NbElements() )
      continue;
    if ( srcSmDS->GetElements()->next() == prxSmDS->GetElements()->next())
      continue;

    if ( srcSmDS->NbElements() != prxSmDS->NbElements() )
      return error( "Different nb elements in a source and a proxy sub-mesh", solidSM->GetId());

    SMDS_ElemIteratorPtr srcIt = srcSmDS->GetElements();
    SMDS_ElemIteratorPtr prxIt = prxSmDS->GetElements();
    while( prxIt->more() )
    {
      const SMDS_MeshElement* fSrc = srcIt->next();
      const SMDS_MeshElement* fPrx = prxIt->next();
      if ( fSrc->NbNodes() != fPrx->NbNodes())
        return error( "Different elements in a source and a proxy sub-mesh", solidSM->GetId());
      for ( int i = 0 ; i < fPrx->NbNodes(); ++i )
        pm->setNode2Node( fSrc->GetNode(i), fPrx->GetNode(i), prxSmDS );
    }
  }
  pm->_n2nMapComputed = true;
  return true;
}

//================================================================================
/*!
 * \brief Does its job
 */
//================================================================================

SMESH_ComputeErrorPtr _ViscousBuilder::Compute(SMESH_Mesh&         theMesh,
                                               const TopoDS_Shape& theShape)
{
  _mesh = & theMesh;

  // check if proxy mesh already computed
  TopExp_Explorer exp( theShape, TopAbs_SOLID );
  if ( !exp.More() )
    return error("No SOLID's in theShape"), _error;

  if ( _ViscousListener::GetSolidMesh( _mesh, exp.Current(), /*toCreate=*/false))
    return SMESH_ComputeErrorPtr(); // everything already computed

  PyDump debugDump( theMesh );

  // TODO: ignore already computed SOLIDs 
  if ( !findSolidsWithLayers())
    return _error;

  if ( !findFacesWithLayers() )
    return _error;

  for ( size_t i = 0; i < _sdVec.size(); ++i )
  {
    size_t iSD = 0;
    for ( iSD = 0; iSD < _sdVec.size(); ++iSD ) // find next SOLID to compute
      if ( _sdVec[iSD]._before.IsEmpty() &&
           !_sdVec[iSD]._solid.IsNull() &&
           _sdVec[iSD]._n2eMap.empty() )
        break;

    if ( ! makeLayer(_sdVec[iSD]) )   // create _LayerEdge's
      return _error;

    if ( _sdVec[iSD]._n2eMap.size() == 0 ) // no layers in a SOLID
    {
      _sdVec[iSD]._solid.Nullify();
      continue;
    }

    if ( ! inflate(_sdVec[iSD]) )     // increase length of _LayerEdge's
      return _error;

    if ( ! refine(_sdVec[iSD]) )      // create nodes and prisms
      return _error;

    if ( ! shrink(_sdVec[iSD]) )      // shrink 2D mesh on FACEs w/o layer
      return _error;

    addBoundaryElements(_sdVec[iSD]); // create quadrangles on prism bare sides

    const TopoDS_Shape& solid = _sdVec[iSD]._solid;
    for ( iSD = 0; iSD < _sdVec.size(); ++iSD )
      _sdVec[iSD]._before.Remove( solid );
  }

  makeGroupOfLE(); // debug
  debugDump.Finish();

  return _error;
}

//================================================================================
/*!
 * \brief Check validity of hypotheses
 */
//================================================================================

SMESH_ComputeErrorPtr _ViscousBuilder::CheckHypotheses( SMESH_Mesh&         mesh,
                                                        const TopoDS_Shape& shape )
{
  _mesh = & mesh;

  if ( _ViscousListener::GetSolidMesh( _mesh, shape, /*toCreate=*/false))
    return SMESH_ComputeErrorPtr(); // everything already computed


  findSolidsWithLayers();
  bool ok = findFacesWithLayers( true );

  // remove _MeshOfSolid's of _SolidData's
  for ( size_t i = 0; i < _sdVec.size(); ++i )
    _ViscousListener::RemoveSolidMesh( _mesh, _sdVec[i]._solid );

  if ( !ok )
    return _error;

  return SMESH_ComputeErrorPtr();
}

//================================================================================
/*!
 * \brief Finds SOLIDs to compute using viscous layers. Fills _sdVec
 */
//================================================================================

bool _ViscousBuilder::findSolidsWithLayers()
{
  // get all solids
  TopTools_IndexedMapOfShape allSolids;
  TopExp::MapShapes( _mesh->GetShapeToMesh(), TopAbs_SOLID, allSolids );
  _sdVec.reserve( allSolids.Extent());

  SMESH_HypoFilter filter;
  for ( int i = 1; i <= allSolids.Extent(); ++i )
  {
    // find StdMeshers_ViscousLayers hyp assigned to the i-th solid
    SMESH_subMesh* sm = _mesh->GetSubMesh( allSolids(i) );
    if ( sm->GetSubMeshDS() && sm->GetSubMeshDS()->NbElements() > 0 )
      continue; // solid is already meshed
    SMESH_Algo* algo = sm->GetAlgo();
    if ( !algo ) continue;
    // TODO: check if algo is hidden
    const list <const SMESHDS_Hypothesis *> & allHyps =
      algo->GetUsedHypothesis(*_mesh, allSolids(i), /*ignoreAuxiliary=*/false);
    _SolidData* soData = 0;
    list< const SMESHDS_Hypothesis *>::const_iterator hyp = allHyps.begin();
    const StdMeshers_ViscousLayers* viscHyp = 0;
    for ( ; hyp != allHyps.end(); ++hyp )
      if (( viscHyp = dynamic_cast<const StdMeshers_ViscousLayers*>( *hyp )))
      {
        TopoDS_Shape hypShape;
        filter.Init( filter.Is( viscHyp ));
        _mesh->GetHypothesis( allSolids(i), filter, true, &hypShape );

        if ( !soData )
        {
          _MeshOfSolid* proxyMesh = _ViscousListener::GetSolidMesh( _mesh,
                                                                    allSolids(i),
                                                                    /*toCreate=*/true);
          _sdVec.push_back( _SolidData( allSolids(i), proxyMesh ));
          soData = & _sdVec.back();
          soData->_index = getMeshDS()->ShapeToIndex( allSolids(i));
          soData->_helper = new SMESH_MesherHelper( *_mesh );
          soData->_helper->SetSubShape( allSolids(i) );
          _solids.Add( allSolids(i) );
        }
        soData->_hyps.push_back( viscHyp );
        soData->_hypShapes.push_back( hypShape );
      }
  }
  if ( _sdVec.empty() )
    return error
      ( SMESH_Comment(StdMeshers_ViscousLayers::GetHypType()) << " hypothesis not found",0);

  return true;
}

//================================================================================
/*!
 * \brief Set a _SolidData to be computed before another
 */
//================================================================================

bool _ViscousBuilder::setBefore( _SolidData& solidBefore, _SolidData& solidAfter )
{
  // check possibility to set this order; get all solids before solidBefore
  TopTools_IndexedMapOfShape allSolidsBefore;
  allSolidsBefore.Add( solidBefore._solid );
  for ( int i = 1; i <= allSolidsBefore.Extent(); ++i )
  {
    int iSD = _solids.FindIndex( allSolidsBefore(i) );
    if ( iSD )
    {
      TopTools_MapIteratorOfMapOfShape soIt( _sdVec[ iSD-1 ]._before );
      for ( ; soIt.More(); soIt.Next() )
        allSolidsBefore.Add( soIt.Value() );
    }
  }
  if ( allSolidsBefore.Contains( solidAfter._solid ))
    return false;

  for ( int i = 1; i <= allSolidsBefore.Extent(); ++i )
    solidAfter._before.Add( allSolidsBefore(i) );

  return true;
}

//================================================================================
/*!
 * \brief
 */
//================================================================================

bool _ViscousBuilder::findFacesWithLayers(const bool onlyWith)
{
  SMESH_MesherHelper helper( *_mesh );
  TopExp_Explorer exp;

  // collect all faces-to-ignore defined by hyp
  for ( size_t i = 0; i < _sdVec.size(); ++i )
  {
    // get faces-to-ignore defined by each hyp
    typedef const StdMeshers_ViscousLayers* THyp;
    typedef std::pair< set<TGeomID>, THyp > TFacesOfHyp;
    list< TFacesOfHyp > ignoreFacesOfHyps;
    list< THyp >::iterator              hyp = _sdVec[i]._hyps.begin();
    list< TopoDS_Shape >::iterator hypShape = _sdVec[i]._hypShapes.begin();
    for ( ; hyp != _sdVec[i]._hyps.end(); ++hyp, ++hypShape )
    {
      ignoreFacesOfHyps.push_back( TFacesOfHyp( set<TGeomID>(), *hyp ));
      getIgnoreFaces( _sdVec[i]._solid, *hyp, *hypShape, ignoreFacesOfHyps.back().first );
    }

    // fill _SolidData::_face2hyp and check compatibility of hypotheses
    const int nbHyps = _sdVec[i]._hyps.size();
    if ( nbHyps > 1 )
    {
      // check if two hypotheses define different parameters for the same FACE
      list< TFacesOfHyp >::iterator igFacesOfHyp;
      for ( exp.Init( _sdVec[i]._solid, TopAbs_FACE ); exp.More(); exp.Next() )
      {
        const TGeomID faceID = getMeshDS()->ShapeToIndex( exp.Current() );
        THyp hyp = 0;
        igFacesOfHyp = ignoreFacesOfHyps.begin();
        for ( ; igFacesOfHyp != ignoreFacesOfHyps.end(); ++igFacesOfHyp )
          if ( ! igFacesOfHyp->first.count( faceID ))
          {
            if ( hyp )
              return error(SMESH_Comment("Several hypotheses define "
                                         "Viscous Layers on the face #") << faceID );
            hyp = igFacesOfHyp->second;
          }
        if ( hyp )
          _sdVec[i]._face2hyp.insert( make_pair( faceID, hyp ));
        else
          _sdVec[i]._ignoreFaceIds.insert( faceID );
      }

      // check if two hypotheses define different number of viscous layers for
      // adjacent faces of a solid
      set< int > nbLayersSet;
      igFacesOfHyp = ignoreFacesOfHyps.begin();
      for ( ; igFacesOfHyp != ignoreFacesOfHyps.end(); ++igFacesOfHyp )
      {
        nbLayersSet.insert( igFacesOfHyp->second->GetNumberLayers() );
      }
      if ( nbLayersSet.size() > 1 )
      {
        for ( exp.Init( _sdVec[i]._solid, TopAbs_EDGE ); exp.More(); exp.Next() )
        {
          PShapeIteratorPtr fIt = helper.GetAncestors( exp.Current(), *_mesh, TopAbs_FACE );
          THyp hyp1 = 0, hyp2 = 0;
          while( const TopoDS_Shape* face = fIt->next() )
          {
            const TGeomID faceID = getMeshDS()->ShapeToIndex( *face );
            map< TGeomID, THyp >::iterator f2h = _sdVec[i]._face2hyp.find( faceID );
            if ( f2h != _sdVec[i]._face2hyp.end() )
            {
              ( hyp1 ? hyp2 : hyp1 ) = f2h->second;
            }
          }
          if ( hyp1 && hyp2 &&
               hyp1->GetNumberLayers() != hyp2->GetNumberLayers() )
          {
            return error("Two hypotheses define different number of "
                         "viscous layers on adjacent faces");
          }
        }
      }
    } // if ( nbHyps > 1 )
    else
    {
      _sdVec[i]._ignoreFaceIds.swap( ignoreFacesOfHyps.back().first );
    }
  } // loop on _sdVec

  if ( onlyWith ) // is called to check hypotheses compatibility only
    return true;

  // fill _SolidData::_reversedFaceIds
  for ( size_t i = 0; i < _sdVec.size(); ++i )
  {
    exp.Init( _sdVec[i]._solid.Oriented( TopAbs_FORWARD ), TopAbs_FACE );
    for ( ; exp.More(); exp.Next() )
    {
      const TopoDS_Face& face = TopoDS::Face( exp.Current() );
      const TGeomID faceID = getMeshDS()->ShapeToIndex( face );
      if ( //!sdVec[i]._ignoreFaceIds.count( faceID ) &&
          helper.NbAncestors( face, *_mesh, TopAbs_SOLID ) > 1 &&
          helper.IsReversedSubMesh( face ))
      {
        _sdVec[i]._reversedFaceIds.insert( faceID );
      }
    }
  }

  // Find FACEs to shrink mesh on (solution 2 in issue 0020832): fill in _shrinkShape2Shape
  TopTools_IndexedMapOfShape shapes;
  std::string structAlgoName = "Hexa_3D";
  for ( size_t i = 0; i < _sdVec.size(); ++i )
  {
    shapes.Clear();
    TopExp::MapShapes(_sdVec[i]._solid, TopAbs_EDGE, shapes);
    for ( int iE = 1; iE <= shapes.Extent(); ++iE )
    {
      const TopoDS_Shape& edge = shapes(iE);
      // find 2 FACEs sharing an EDGE
      TopoDS_Shape FF[2];
      PShapeIteratorPtr fIt = helper.GetAncestors(edge, *_mesh, TopAbs_FACE, &_sdVec[i]._solid);
      while ( fIt->more())
      {
        const TopoDS_Shape* f = fIt->next();
        FF[ int( !FF[0].IsNull()) ] = *f;
      }
      if( FF[1].IsNull() ) continue; // seam edge can be shared by 1 FACE only

      // check presence of layers on them
      int ignore[2];
      for ( int j = 0; j < 2; ++j )
        ignore[j] = _sdVec[i]._ignoreFaceIds.count( getMeshDS()->ShapeToIndex( FF[j] ));
      if ( ignore[0] == ignore[1] )
        continue; // nothing interesting
      TopoDS_Shape fWOL = FF[ ignore[0] ? 0 : 1 ];

      // add EDGE to maps
      if ( !fWOL.IsNull())
      {
        TGeomID edgeInd = getMeshDS()->ShapeToIndex( edge );
        _sdVec[i]._shrinkShape2Shape.insert( make_pair( edgeInd, fWOL ));
      }
    }
  }

  // Find the SHAPE along which to inflate _LayerEdge based on VERTEX

  for ( size_t i = 0; i < _sdVec.size(); ++i )
  {
    shapes.Clear();
    TopExp::MapShapes(_sdVec[i]._solid, TopAbs_VERTEX, shapes);
    for ( int iV = 1; iV <= shapes.Extent(); ++iV )
    {
      const TopoDS_Shape& vertex = shapes(iV);
      // find faces WOL sharing the vertex
      vector< TopoDS_Shape > facesWOL;
      size_t totalNbFaces = 0;
      PShapeIteratorPtr fIt = helper.GetAncestors(vertex, *_mesh, TopAbs_FACE, &_sdVec[i]._solid );
      while ( fIt->more())
      {
        const TopoDS_Shape* f = fIt->next();
        totalNbFaces++;
        const int fID = getMeshDS()->ShapeToIndex( *f );
        if ( _sdVec[i]._ignoreFaceIds.count ( fID ) /*&& !_sdVec[i]._noShrinkShapes.count( fID )*/)
          facesWOL.push_back( *f );
      }
      if ( facesWOL.size() == totalNbFaces || facesWOL.empty() )
        continue; // no layers at this vertex or no WOL
      TGeomID vInd = getMeshDS()->ShapeToIndex( vertex );
      switch ( facesWOL.size() )
      {
      case 1:
      {
        helper.SetSubShape( facesWOL[0] );
        if ( helper.IsRealSeam( vInd )) // inflate along a seam edge?
        {
          TopoDS_Shape seamEdge;
          PShapeIteratorPtr eIt = helper.GetAncestors(vertex, *_mesh, TopAbs_EDGE);
          while ( eIt->more() && seamEdge.IsNull() )
          {
            const TopoDS_Shape* e = eIt->next();
            if ( helper.IsRealSeam( *e ) )
              seamEdge = *e;
          }
          if ( !seamEdge.IsNull() )
          {
            _sdVec[i]._shrinkShape2Shape.insert( make_pair( vInd, seamEdge ));
            break;
          }
        }
        _sdVec[i]._shrinkShape2Shape.insert( make_pair( vInd, facesWOL[0] ));
        break;
      }
      case 2:
      {
        // find an edge shared by 2 faces
        PShapeIteratorPtr eIt = helper.GetAncestors(vertex, *_mesh, TopAbs_EDGE);
        while ( eIt->more())
        {
          const TopoDS_Shape* e = eIt->next();
          if ( helper.IsSubShape( *e, facesWOL[0]) &&
               helper.IsSubShape( *e, facesWOL[1]))
          {
            _sdVec[i]._shrinkShape2Shape.insert( make_pair( vInd, *e )); break;
          }
        }
        break;
      }
      default:
        return error("Not yet supported case", _sdVec[i]._index);
      }
    }
  }

  // Add to _noShrinkShapes sub-shapes of FACE's that can't be shrinked since
  // the algo of the SOLID sharing the FACE does not support it or for other reasons
  set< string > notSupportAlgos; notSupportAlgos.insert( structAlgoName );
  for ( size_t i = 0; i < _sdVec.size(); ++i )
  {
    map< TGeomID, TopoDS_Shape >::iterator e2f = _sdVec[i]._shrinkShape2Shape.begin();
    for ( ; e2f != _sdVec[i]._shrinkShape2Shape.end(); ++e2f )
    {
      const TopoDS_Shape& fWOL = e2f->second;
      const TGeomID     edgeID = e2f->first;
      TGeomID           faceID = getMeshDS()->ShapeToIndex( fWOL );
      TopoDS_Shape        edge = getMeshDS()->IndexToShape( edgeID );
      if ( edge.ShapeType() != TopAbs_EDGE )
        continue; // shrink shape is VERTEX

      TopoDS_Shape solid;
      PShapeIteratorPtr soIt = helper.GetAncestors(fWOL, *_mesh, TopAbs_SOLID);
      while ( soIt->more() && solid.IsNull() )
      {
        const TopoDS_Shape* so = soIt->next();
        if ( !so->IsSame( _sdVec[i]._solid ))
          solid = *so;
      }
      if ( solid.IsNull() )
        continue;

      bool noShrinkE = false;
      SMESH_Algo*  algo = _mesh->GetSubMesh( solid )->GetAlgo();
      bool isStructured = ( algo && algo->GetName() == structAlgoName );
      size_t     iSolid = _solids.FindIndex( solid ) - 1;
      if ( iSolid < _sdVec.size() && _sdVec[ iSolid ]._ignoreFaceIds.count( faceID ))
      {
        // the adjacent SOLID has NO layers on fWOL;
        // shrink allowed if
        // - there are layers on the EDGE in the adjacent SOLID
        // - there are NO layers in the adjacent SOLID && algo is unstructured and computed later
        bool hasWLAdj = (_sdVec[iSolid]._shrinkShape2Shape.count( edgeID ));
        bool shrinkAllowed = (( hasWLAdj ) ||
                              ( !isStructured && setBefore( _sdVec[ i ], _sdVec[ iSolid ] )));
        noShrinkE = !shrinkAllowed;
      }
      else if ( iSolid < _sdVec.size() )
      {
        // the adjacent SOLID has layers on fWOL;
        // check if SOLID's mesh is unstructured and then try to set it
        // to be computed after the i-th solid
        if ( isStructured || !setBefore( _sdVec[ i ], _sdVec[ iSolid ] ))
          noShrinkE = true; // don't shrink fWOL
      }
      else
      {
        // the adjacent SOLID has NO layers at all
        noShrinkE = isStructured;
      }

      if ( noShrinkE )
      {
        _sdVec[i]._noShrinkShapes.insert( edgeID );

        // check if there is a collision with to-shrink-from EDGEs in iSolid
        // if ( iSolid < _sdVec.size() )
        // {
        //   shapes.Clear();
        //   TopExp::MapShapes( fWOL, TopAbs_EDGE, shapes);
        //   for ( int iE = 1; iE <= shapes.Extent(); ++iE )
        //   {
        //     const TopoDS_Edge& E = TopoDS::Edge( shapes( iE ));
        //     const TGeomID    eID = getMeshDS()->ShapeToIndex( E );
        //     if ( eID == edgeID ||
        //          !_sdVec[iSolid]._shrinkShape2Shape.count( eID ) ||
        //          _sdVec[i]._noShrinkShapes.count( eID ))
        //       continue;
        //     for ( int is1st = 0; is1st < 2; ++is1st )
        //     {
        //       TopoDS_Vertex V = helper.IthVertex( is1st, E );
        //       if ( _sdVec[i]._noShrinkShapes.count( getMeshDS()->ShapeToIndex( V ) ))
        //       {
        //         return error("No way to make a conformal mesh with "
        //                      "the given set of faces with layers", _sdVec[i]._index);
        //       }
        //     }
        //   }
        // }
      }

      // add VERTEXes of the edge in _noShrinkShapes, which is necessary if
      // _shrinkShape2Shape is different in the adjacent SOLID
      for ( TopoDS_Iterator vIt( edge ); vIt.More(); vIt.Next() )
      {
        TGeomID vID = getMeshDS()->ShapeToIndex( vIt.Value() );
        bool noShrinkV = false;

        if ( iSolid < _sdVec.size() )
        {
          if ( _sdVec[ iSolid ]._ignoreFaceIds.count( faceID ))
          {
            map< TGeomID, TopoDS_Shape >::iterator i2S, i2SAdj;
            i2S    = _sdVec[i     ]._shrinkShape2Shape.find( vID );
            i2SAdj = _sdVec[iSolid]._shrinkShape2Shape.find( vID );
            if ( i2SAdj == _sdVec[iSolid]._shrinkShape2Shape.end() )
              noShrinkV = ( i2S->second.ShapeType() == TopAbs_EDGE || isStructured );
            else
              noShrinkV = ( ! i2S->second.IsSame( i2SAdj->second ));
          }
          else
          {
            noShrinkV = noShrinkE;
          }
        }
        else
        {
          // the adjacent SOLID has NO layers at all
          noShrinkV = ( isStructured ||
                        _sdVec[i]._shrinkShape2Shape[ vID ].ShapeType() == TopAbs_EDGE );
        }
        if ( noShrinkV )
          _sdVec[i]._noShrinkShapes.insert( vID );
      }

    } // loop on _sdVec[i]._shrinkShape2Shape
  } // loop on _sdVec to fill in _SolidData::_noShrinkShapes


    // add FACEs of other SOLIDs to _ignoreFaceIds
  for ( size_t i = 0; i < _sdVec.size(); ++i )
  {
    shapes.Clear();
    TopExp::MapShapes(_sdVec[i]._solid, TopAbs_FACE, shapes);

    for ( exp.Init( _mesh->GetShapeToMesh(), TopAbs_FACE ); exp.More(); exp.Next() )
    {
      if ( !shapes.Contains( exp.Current() ))
        _sdVec[i]._ignoreFaceIds.insert( getMeshDS()->ShapeToIndex( exp.Current() ));
    }
  }

  return true;
}

//================================================================================
/*!
 * \brief Finds FACEs w/o layers for a given SOLID by an hypothesis
 */
//================================================================================

void _ViscousBuilder::getIgnoreFaces(const TopoDS_Shape&             solid,
                                     const StdMeshers_ViscousLayers* hyp,
                                     const TopoDS_Shape&             hypShape,
                                     set<TGeomID>&                   ignoreFaceIds)
{
  TopExp_Explorer exp;

  vector<TGeomID> ids = hyp->GetBndShapes();
  if ( hyp->IsToIgnoreShapes() ) // FACEs to ignore are given
  {
    for ( size_t ii = 0; ii < ids.size(); ++ii )
    {
      const TopoDS_Shape& s = getMeshDS()->IndexToShape( ids[ii] );
      if ( !s.IsNull() && s.ShapeType() == TopAbs_FACE )
        ignoreFaceIds.insert( ids[ii] );
    }
  }
  else // FACEs with layers are given
  {
    exp.Init( solid, TopAbs_FACE );
    for ( ; exp.More(); exp.Next() )
    {
      TGeomID faceInd = getMeshDS()->ShapeToIndex( exp.Current() );
      if ( find( ids.begin(), ids.end(), faceInd ) == ids.end() )
        ignoreFaceIds.insert( faceInd );
    }
  }

  // ignore internal FACEs if inlets and outlets are specified
  if ( hyp->IsToIgnoreShapes() )
  {
    TopTools_IndexedDataMapOfShapeListOfShape solidsOfFace;
    TopExp::MapShapesAndAncestors( hypShape,
                                   TopAbs_FACE, TopAbs_SOLID, solidsOfFace);

    for ( exp.Init( solid, TopAbs_FACE ); exp.More(); exp.Next() )
    {
      const TopoDS_Face& face = TopoDS::Face( exp.Current() );
      if ( SMESH_MesherHelper::NbAncestors( face, *_mesh, TopAbs_SOLID ) < 2 )
        continue;

      int nbSolids = solidsOfFace.FindFromKey( face ).Extent();
      if ( nbSolids > 1 )
        ignoreFaceIds.insert( getMeshDS()->ShapeToIndex( face ));
    }
  }
}

//================================================================================
/*!
 * \brief Create the inner surface of the viscous layer and prepare data for infation
 */
//================================================================================

bool _ViscousBuilder::makeLayer(_SolidData& data)
{
  // get all sub-shapes to make layers on
  set<TGeomID> subIds, faceIds;
  subIds = data._noShrinkShapes;
  TopExp_Explorer exp( data._solid, TopAbs_FACE );
  for ( ; exp.More(); exp.Next() )
  {
    SMESH_subMesh* fSubM = _mesh->GetSubMesh( exp.Current() );
    if ( ! data._ignoreFaceIds.count( fSubM->GetId() ))
      faceIds.insert( fSubM->GetId() );
  }

  // make a map to find new nodes on sub-shapes shared with other SOLID
  map< TGeomID, TNode2Edge* >::iterator s2ne;
  map< TGeomID, TopoDS_Shape >::iterator s2s = data._shrinkShape2Shape.begin();
  for (; s2s != data._shrinkShape2Shape.end(); ++s2s )
  {
    TGeomID shapeInd = s2s->first;
    for ( size_t i = 0; i < _sdVec.size(); ++i )
    {
      if ( _sdVec[i]._index == data._index ) continue;
      map< TGeomID, TopoDS_Shape >::iterator s2s2 = _sdVec[i]._shrinkShape2Shape.find( shapeInd );
      if ( s2s2 != _sdVec[i]._shrinkShape2Shape.end() &&
           *s2s == *s2s2 && !_sdVec[i]._n2eMap.empty() )
      {
        data._s2neMap.insert( make_pair( shapeInd, &_sdVec[i]._n2eMap ));
        break;
      }
    }
  }

  // Create temporary faces and _LayerEdge's

  dumpFunction(SMESH_Comment("makeLayers_")<<data._index);

  data._stepSize = Precision::Infinite();
  data._stepSizeNodes[0] = 0;

  SMESH_MesherHelper helper( *_mesh );
  helper.SetSubShape( data._solid );
  helper.SetElementsOnShape( true );

  vector< const SMDS_MeshNode*> newNodes; // of a mesh face
  TNode2Edge::iterator n2e2;

  // collect _LayerEdge's of shapes they are based on
  vector< _EdgesOnShape >& edgesByGeom = data._edgesOnShape;
  const int nbShapes = getMeshDS()->MaxShapeIndex();
  edgesByGeom.resize( nbShapes+1 );

  // set data of _EdgesOnShape's
  if ( SMESH_subMesh* sm = _mesh->GetSubMesh( data._solid ))
  {
    SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(/*includeSelf=*/false);
    while ( smIt->more() )
    {
      sm = smIt->next();
      if ( sm->GetSubShape().ShapeType() == TopAbs_FACE &&
           !faceIds.count( sm->GetId() ))
        continue;
      setShapeData( edgesByGeom[ sm->GetId() ], sm, data );
    }
  }
  // make _LayerEdge's
  for ( set<TGeomID>::iterator id = faceIds.begin(); id != faceIds.end(); ++id )
  {
    const TopoDS_Face& F = TopoDS::Face( getMeshDS()->IndexToShape( *id ));
    SMESH_subMesh* sm = _mesh->GetSubMesh( F );
    SMESH_ProxyMesh::SubMesh* proxySub =
      data._proxyMesh->getFaceSubM( F, /*create=*/true);

    SMESHDS_SubMesh* smDS = sm->GetSubMeshDS();
    if ( !smDS ) return error(SMESH_Comment("Not meshed face ") << *id, data._index );

    SMDS_ElemIteratorPtr eIt = smDS->GetElements();
    while ( eIt->more() )
    {
      const SMDS_MeshElement* face = eIt->next();
      double          faceMaxCosin = -1;
      _LayerEdge*     maxCosinEdge = 0;
      int             nbDegenNodes = 0;

      newNodes.resize( face->NbCornerNodes() );
      for ( size_t i = 0 ; i < newNodes.size(); ++i )
      {
        const SMDS_MeshNode* n = face->GetNode( i );
        const int      shapeID = n->getshapeId();
        const bool onDegenShap = helper.IsDegenShape( shapeID );
        const bool onDegenEdge = ( onDegenShap && n->GetPosition()->GetDim() == 1 );
        if ( onDegenShap )
        {
          if ( onDegenEdge )
          {
            // substitute n on a degenerated EDGE with a node on a corresponding VERTEX
            const TopoDS_Shape& E = getMeshDS()->IndexToShape( shapeID );
            TopoDS_Vertex       V = helper.IthVertex( 0, TopoDS::Edge( E ));
            if ( const SMDS_MeshNode* vN = SMESH_Algo::VertexNode( V, getMeshDS() )) {
              n = vN;
              nbDegenNodes++;
            }
          }
          else
          {
            nbDegenNodes++;
          }
        }
        TNode2Edge::iterator n2e = data._n2eMap.insert( make_pair( n, (_LayerEdge*)0 )).first;
        if ( !(*n2e).second )
        {
          // add a _LayerEdge
          _LayerEdge* edge = new _LayerEdge();
          edge->_nodes.push_back( n );
          n2e->second = edge;
          edgesByGeom[ shapeID ]._edges.push_back( edge );
          const bool noShrink = data._noShrinkShapes.count( shapeID );

          SMESH_TNodeXYZ xyz( n );

          // set edge data or find already refined _LayerEdge and get data from it
          if (( !noShrink                                                     ) &&
              ( n->GetPosition()->GetTypeOfPosition() != SMDS_TOP_FACE        ) &&
              (( s2ne = data._s2neMap.find( shapeID )) != data._s2neMap.end() ) &&
              (( n2e2 = (*s2ne).second->find( n )) != s2ne->second->end()     ))
          {
            _LayerEdge* foundEdge = (*n2e2).second;
            gp_XYZ        lastPos = edge->Copy( *foundEdge, edgesByGeom[ shapeID ], helper );
            foundEdge->_pos.push_back( lastPos );
            // location of the last node is modified and we restore it by foundEdge->_pos.back()
            const_cast< SMDS_MeshNode* >
              ( edge->_nodes.back() )->setXYZ( xyz.X(), xyz.Y(), xyz.Z() );
          }
          else
          {
            if ( !noShrink )
            {
              edge->_nodes.push_back( helper.AddNode( xyz.X(), xyz.Y(), xyz.Z() ));
            }
            if ( !setEdgeData( *edge, edgesByGeom[ shapeID ], helper, data ))
              return false;

            if ( edge->_nodes.size() < 2 )
              edge->Block( data );
              //data._noShrinkShapes.insert( shapeID );
          }
          dumpMove(edge->_nodes.back());

          if ( edge->_cosin > faceMaxCosin )
          {
            faceMaxCosin = edge->_cosin;
            maxCosinEdge = edge;
          }
        }
        newNodes[ i ] = n2e->second->_nodes.back();

        if ( onDegenEdge )
          data._n2eMap.insert( make_pair( face->GetNode( i ), n2e->second ));
      }
      if ( newNodes.size() - nbDegenNodes < 2 )
        continue;

      // create a temporary face
      const SMDS_MeshElement* newFace =
        new _TmpMeshFace( newNodes, --_tmpFaceID, face->getshapeId(), face->getIdInShape() );
      proxySub->AddElement( newFace );

      // compute inflation step size by min size of element on a convex surface
      if ( faceMaxCosin > theMinSmoothCosin )
        limitStepSize( data, face, maxCosinEdge );

    } // loop on 2D elements on a FACE
  } // loop on FACEs of a SOLID to create _LayerEdge's


  // Set _LayerEdge::_neibors
  TNode2Edge::iterator n2e;
  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[iS];
    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      _LayerEdge* edge = eos._edges[i];
      TIDSortedNodeSet nearNodes;
      SMDS_ElemIteratorPtr fIt = edge->_nodes[0]->GetInverseElementIterator(SMDSAbs_Face);
      while ( fIt->more() )
      {
        const SMDS_MeshElement* f = fIt->next();
        if ( !data._ignoreFaceIds.count( f->getshapeId() ))
          nearNodes.insert( f->begin_nodes(), f->end_nodes() );
      }
      nearNodes.erase( edge->_nodes[0] );
      edge->_neibors.reserve( nearNodes.size() );
      TIDSortedNodeSet::iterator node = nearNodes.begin();
      for ( ; node != nearNodes.end(); ++node )
        if (( n2e = data._n2eMap.find( *node )) != data._n2eMap.end() )
          edge->_neibors.push_back( n2e->second );
    }
  }

  data._epsilon = 1e-7;
  if ( data._stepSize < 1. )
    data._epsilon *= data._stepSize;

  if ( !findShapesToSmooth( data )) // _LayerEdge::_maxLen is computed here
    return false;

  // limit data._stepSize depending on surface curvature and fill data._convexFaces
  limitStepSizeByCurvature( data ); // !!! it must be before node substitution in _Simplex

  // Set target nodes into _Simplex and _LayerEdge's to _2NearEdges
  const SMDS_MeshNode* nn[2];
  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[iS];
    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      _LayerEdge* edge = eos._edges[i];
      if ( edge->IsOnEdge() )
      {
        // get neighbor nodes
        bool hasData = ( edge->_2neibors->_edges[0] );
        if ( hasData ) // _LayerEdge is a copy of another one
        {
          nn[0] = edge->_2neibors->srcNode(0);
          nn[1] = edge->_2neibors->srcNode(1);
        }
        else if ( !findNeiborsOnEdge( edge, nn[0],nn[1], eos, data ))
        {
          return false;
        }
        // set neighbor _LayerEdge's
        for ( int j = 0; j < 2; ++j )
        {
          if (( n2e = data._n2eMap.find( nn[j] )) == data._n2eMap.end() )
            return error("_LayerEdge not found by src node", data._index);
          edge->_2neibors->_edges[j] = n2e->second;
        }
        if ( !hasData )
          edge->SetDataByNeighbors( nn[0], nn[1], eos, helper );
      }

      for ( size_t j = 0; j < edge->_simplices.size(); ++j )
      {
        _Simplex& s = edge->_simplices[j];
        s._nNext = data._n2eMap[ s._nNext ]->_nodes.back();
        s._nPrev = data._n2eMap[ s._nPrev ]->_nodes.back();
      }

      // For an _LayerEdge on a degenerated EDGE, copy some data from
      // a corresponding _LayerEdge on a VERTEX
      // (issue 52453, pb on a downloaded SampleCase2-Tet-netgen-mephisto.hdf)
      if ( helper.IsDegenShape( edge->_nodes[0]->getshapeId() ))
      {
        // Generally we should not get here
        if ( eos.ShapeType() != TopAbs_EDGE )
          continue;
        TopoDS_Vertex V = helper.IthVertex( 0, TopoDS::Edge( eos._shape ));
        const SMDS_MeshNode* vN = SMESH_Algo::VertexNode( V, getMeshDS() );
        if (( n2e = data._n2eMap.find( vN )) == data._n2eMap.end() )
          continue;
        const _LayerEdge* vEdge = n2e->second;
        edge->_normal    = vEdge->_normal;
        edge->_lenFactor = vEdge->_lenFactor;
        edge->_cosin     = vEdge->_cosin;
      }

    } // loop on data._edgesOnShape._edges
  } // loop on data._edgesOnShape

  // fix _LayerEdge::_2neibors on EDGEs to smooth
  // map< TGeomID,Handle(Geom_Curve)>::iterator e2c = data._edge2curve.begin();
  // for ( ; e2c != data._edge2curve.end(); ++e2c )
  //   if ( !e2c->second.IsNull() )
  //   {
  //     if ( _EdgesOnShape* eos = data.GetShapeEdges( e2c->first ))
  //       data.Sort2NeiborsOnEdge( eos->_edges );
  //   }

  dumpFunctionEnd();
  return true;
}

//================================================================================
/*!
 * \brief Compute inflation step size by min size of element on a convex surface
 */
//================================================================================

void _ViscousBuilder::limitStepSize( _SolidData&             data,
                                     const SMDS_MeshElement* face,
                                     const _LayerEdge*       maxCosinEdge )
{
  int iN = 0;
  double minSize = 10 * data._stepSize;
  const int nbNodes = face->NbCornerNodes();
  for ( int i = 0; i < nbNodes; ++i )
  {
    const SMDS_MeshNode* nextN = face->GetNode( SMESH_MesherHelper::WrapIndex( i+1, nbNodes ));
    const SMDS_MeshNode*  curN = face->GetNode( i );
    if ( nextN->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE ||
         curN-> GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE )
    {
      double dist = SMESH_TNodeXYZ( curN ).Distance( nextN );
      if ( dist < minSize )
        minSize = dist, iN = i;
    }
  }
  double newStep = 0.8 * minSize / maxCosinEdge->_lenFactor;
  if ( newStep < data._stepSize )
  {
    data._stepSize = newStep;
    data._stepSizeCoeff = 0.8 / maxCosinEdge->_lenFactor;
    data._stepSizeNodes[0] = face->GetNode( iN );
    data._stepSizeNodes[1] = face->GetNode( SMESH_MesherHelper::WrapIndex( iN+1, nbNodes ));
  }
}

//================================================================================
/*!
 * \brief Compute inflation step size by min size of element on a convex surface
 */
//================================================================================

void _ViscousBuilder::limitStepSize( _SolidData& data, const double minSize )
{
  if ( minSize < data._stepSize )
  {
    data._stepSize = minSize;
    if ( data._stepSizeNodes[0] )
    {
      double dist =
        SMESH_TNodeXYZ(data._stepSizeNodes[0]).Distance(data._stepSizeNodes[1]);
      data._stepSizeCoeff = data._stepSize / dist;
    }
  }
}

//================================================================================
/*!
 * \brief Limit data._stepSize by evaluating curvature of shapes and fill data._convexFaces
 */
//================================================================================

void _ViscousBuilder::limitStepSizeByCurvature( _SolidData& data )
{
  SMESH_MesherHelper helper( *_mesh );

  const int nbTestPnt = 5; // on a FACE sub-shape

  BRepLProp_SLProps surfProp( 2, 1e-6 );
  data._convexFaces.clear();

  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eof = data._edgesOnShape[iS];
    if ( eof.ShapeType() != TopAbs_FACE ||
         data._ignoreFaceIds.count( eof._shapeID ))
      continue;

    TopoDS_Face        F = TopoDS::Face( eof._shape );
    SMESH_subMesh *   sm = eof._subMesh;
    const TGeomID faceID = eof._shapeID;

    BRepAdaptor_Surface surface( F, false );
    surfProp.SetSurface( surface );

    bool isTooCurved = false;

    _ConvexFace cnvFace;
    const double        oriFactor = ( F.Orientation() == TopAbs_REVERSED ? +1. : -1. );
    SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(/*includeSelf=*/true);
    while ( smIt->more() )
    {
      sm = smIt->next();
      const TGeomID subID = sm->GetId();
      // find _LayerEdge's of a sub-shape
      _EdgesOnShape* eos;
      if (( eos = data.GetShapeEdges( subID )))
        cnvFace._subIdToEOS.insert( make_pair( subID, eos ));
      else
        continue;
      // check concavity and curvature and limit data._stepSize
      const double minCurvature =
        1. / ( eos->_hyp.GetTotalThickness() * ( 1 + theThickToIntersection ));
      size_t iStep = Max( 1, eos->_edges.size() / nbTestPnt );
      for ( size_t i = 0; i < eos->_edges.size(); i += iStep )
      {
        gp_XY uv = helper.GetNodeUV( F, eos->_edges[ i ]->_nodes[0] );
        surfProp.SetParameters( uv.X(), uv.Y() );
        if ( !surfProp.IsCurvatureDefined() )
          continue;
        if ( surfProp.MaxCurvature() * oriFactor > minCurvature )
        {
          limitStepSize( data, 0.9 / surfProp.MaxCurvature() * oriFactor );
          isTooCurved = true;
        }
        if ( surfProp.MinCurvature() * oriFactor > minCurvature )
        {
          limitStepSize( data, 0.9 / surfProp.MinCurvature() * oriFactor );
          isTooCurved = true;
        }
      }
    } // loop on sub-shapes of the FACE

    if ( !isTooCurved ) continue;

    _ConvexFace & convFace =
      data._convexFaces.insert( make_pair( faceID, cnvFace )).first->second;

    convFace._face = F;
    convFace._normalsFixed = false;

    // skip a closed surface (data._convexFaces is useful anyway)
    bool isClosedF = false;
    helper.SetSubShape( F );
    if ( helper.HasRealSeam() )
    {
      // in the closed surface there must be a closed EDGE
      for ( TopExp_Explorer eIt( F, TopAbs_EDGE ); eIt.More() && !isClosedF; eIt.Next() )
        isClosedF = helper.IsClosedEdge( TopoDS::Edge( eIt.Current() ));
    }
    if ( isClosedF )
    {
      // limit _LayerEdge::_maxLen on the FACE
      const double minCurvature =
        1. / ( eof._hyp.GetTotalThickness() * ( 1 + theThickToIntersection ));
      map< TGeomID, _EdgesOnShape* >::iterator id2eos = cnvFace._subIdToEOS.find( faceID );
      if ( id2eos != cnvFace._subIdToEOS.end() )
      {
        _EdgesOnShape& eos = * id2eos->second;
        for ( size_t i = 0; i < eos._edges.size(); ++i )
        {
          _LayerEdge* ledge = eos._edges[ i ];
          gp_XY uv = helper.GetNodeUV( F, ledge->_nodes[0] );
          surfProp.SetParameters( uv.X(), uv.Y() );
          if ( !surfProp.IsCurvatureDefined() )
            continue;

          if ( surfProp.MaxCurvature() * oriFactor > minCurvature )
            ledge->_maxLen = Min( ledge->_maxLen, 1. / surfProp.MaxCurvature() * oriFactor );

          if ( surfProp.MinCurvature() * oriFactor > minCurvature )
            ledge->_maxLen = Min( ledge->_maxLen, 1. / surfProp.MinCurvature() * oriFactor );
        }
      }
      continue;
    }

    // Fill _ConvexFace::_simplexTestEdges. These _LayerEdge's are used to detect
    // prism distortion.
    map< TGeomID, _EdgesOnShape* >::iterator id2eos = convFace._subIdToEOS.find( faceID );
    if ( id2eos != convFace._subIdToEOS.end() && !id2eos->second->_edges.empty() )
    {
      // there are _LayerEdge's on the FACE it-self;
      // select _LayerEdge's near EDGEs
      _EdgesOnShape& eos = * id2eos->second;
      for ( size_t i = 0; i < eos._edges.size(); ++i )
      {
        _LayerEdge* ledge = eos._edges[ i ];
        for ( size_t j = 0; j < ledge->_simplices.size(); ++j )
          if ( ledge->_simplices[j]._nNext->GetPosition()->GetDim() < 2 )
          {
            convFace._simplexTestEdges.push_back( ledge );
            break;
          }
      }
    }
    else
    {
      // where there are no _LayerEdge's on a _ConvexFace,
      // as e.g. on a fillet surface with no internal nodes - issue 22580,
      // so that collision of viscous internal faces is not detected by check of
      // intersection of _LayerEdge's with the viscous internal faces.

      set< const SMDS_MeshNode* > usedNodes;

      // look for _LayerEdge's with null _sWOL
      id2eos = convFace._subIdToEOS.begin();
      for ( ; id2eos != convFace._subIdToEOS.end(); ++id2eos )
      {
        _EdgesOnShape& eos = * id2eos->second;
        if ( !eos._sWOL.IsNull() )
          continue;
        for ( size_t i = 0; i < eos._edges.size(); ++i )
        {
          _LayerEdge* ledge = eos._edges[ i ];
          const SMDS_MeshNode* srcNode = ledge->_nodes[0];
          if ( !usedNodes.insert( srcNode ).second ) continue;

          for ( size_t i = 0; i < ledge->_simplices.size(); ++i )
          {
            usedNodes.insert( ledge->_simplices[i]._nPrev );
            usedNodes.insert( ledge->_simplices[i]._nNext );
          }
          convFace._simplexTestEdges.push_back( ledge );
        }
      }
    }
  } // loop on FACEs of data._solid
}

//================================================================================
/*!
 * \brief Detect shapes (and _LayerEdge's on them) to smooth
 */
//================================================================================

bool _ViscousBuilder::findShapesToSmooth( _SolidData& data )
{
  // define allowed thickness
  computeGeomSize( data ); // compute data._geomSize and _LayerEdge::_maxLen

  data._maxThickness = 0;
  data._minThickness = 1e100;
  list< const StdMeshers_ViscousLayers* >::iterator hyp = data._hyps.begin();
  for ( ; hyp != data._hyps.end(); ++hyp )
  {
    data._maxThickness = Max( data._maxThickness, (*hyp)->GetTotalThickness() );
    data._minThickness = Min( data._minThickness, (*hyp)->GetTotalThickness() );
  }
  //const double tgtThick = /*Min( 0.5 * data._geomSize, */data._maxThickness;

  // Find shapes needing smoothing; such a shape has _LayerEdge._normal on it's
  // boundary inclined to the shape at a sharp angle

  //list< TGeomID > shapesToSmooth;
  TopTools_MapOfShape edgesOfSmooFaces;

  SMESH_MesherHelper helper( *_mesh );
  bool ok = true;

  vector< _EdgesOnShape >& edgesByGeom = data._edgesOnShape;
  data._nbShapesToSmooth = 0;

  for ( size_t iS = 0; iS < edgesByGeom.size(); ++iS ) // check FACEs
  {
    _EdgesOnShape& eos = edgesByGeom[iS];
    eos._toSmooth = false;
    if ( eos._edges.empty() || eos.ShapeType() != TopAbs_FACE )
      continue;

    double tgtThick = eos._hyp.GetTotalThickness();
    TopExp_Explorer eExp( edgesByGeom[iS]._shape, TopAbs_EDGE );
    for ( ; eExp.More() && !eos._toSmooth; eExp.Next() )
    {
      TGeomID iE = getMeshDS()->ShapeToIndex( eExp.Current() );
      vector<_LayerEdge*>& eE = edgesByGeom[ iE ]._edges;
      if ( eE.empty() ) continue;

      double faceSize;
      for ( size_t i = 0; i < eE.size() && !eos._toSmooth; ++i )
        if ( eE[i]->_cosin > theMinSmoothCosin )
        {
          SMDS_ElemIteratorPtr fIt = eE[i]->_nodes[0]->GetInverseElementIterator(SMDSAbs_Face);
          while ( fIt->more() && !eos._toSmooth )
          {
            const SMDS_MeshElement* face = fIt->next();
            if ( face->getshapeId() == eos._shapeID &&
                 getDistFromEdge( face, eE[i]->_nodes[0], faceSize ))
            {
              eos._toSmooth = needSmoothing( eE[i]->_cosin, tgtThick, faceSize );
            }
          }
        }
    }
    if ( eos._toSmooth )
    {
      for ( eExp.ReInit(); eExp.More(); eExp.Next() )
        edgesOfSmooFaces.Add( eExp.Current() );

      data.PrepareEdgesToSmoothOnFace( &edgesByGeom[iS], /*substituteSrcNodes=*/false );
    }
    data._nbShapesToSmooth += eos._toSmooth;

  }  // check FACEs

  for ( size_t iS = 0; iS < edgesByGeom.size(); ++iS ) // check EDGEs
  {
    _EdgesOnShape& eos = edgesByGeom[iS];
    eos._edgeSmoother = NULL;
    if ( eos._edges.empty() || eos.ShapeType() != TopAbs_EDGE ) continue;
    if ( !eos._hyp.ToSmooth() ) continue;

    const TopoDS_Edge& E = TopoDS::Edge( edgesByGeom[iS]._shape );
    if ( SMESH_Algo::isDegenerated( E ) || !edgesOfSmooFaces.Contains( E ))
      continue;

    double tgtThick = eos._hyp.GetTotalThickness();
    for ( TopoDS_Iterator vIt( E ); vIt.More() && !eos._toSmooth; vIt.Next() )
    {
      TGeomID iV = getMeshDS()->ShapeToIndex( vIt.Value() );
      vector<_LayerEdge*>& eV = edgesByGeom[ iV ]._edges;
      if ( eV.empty() || eV[0]->Is( _LayerEdge::MULTI_NORMAL )) continue;
      gp_Vec  eDir    = getEdgeDir( E, TopoDS::Vertex( vIt.Value() ));
      double angle    = eDir.Angle( eV[0]->_normal );
      double cosin    = Cos( angle );
      double cosinAbs = Abs( cosin );
      if ( cosinAbs > theMinSmoothCosin )
      {
        // always smooth analytic EDGEs
        Handle(Geom_Curve) curve = _Smoother1D::CurveForSmooth( E, eos, helper );
        eos._toSmooth = ! curve.IsNull();

        // compare tgtThick with the length of an end segment
        SMDS_ElemIteratorPtr eIt = eV[0]->_nodes[0]->GetInverseElementIterator(SMDSAbs_Edge);
        while ( eIt->more() && !eos._toSmooth )
        {
          const SMDS_MeshElement* endSeg = eIt->next();
          if ( endSeg->getshapeId() == (int) iS )
          {
            double segLen =
              SMESH_TNodeXYZ( endSeg->GetNode(0) ).Distance( endSeg->GetNode(1 ));
            eos._toSmooth = needSmoothing( cosinAbs, tgtThick, segLen );
          }
        }
        if ( eos._toSmooth )
        {
          eos._edgeSmoother = new _Smoother1D( curve, eos );

          for ( size_t i = 0; i < eos._edges.size(); ++i )
            eos._edges[i]->Set( _LayerEdge::TO_SMOOTH );
        }
      }
    }
    data._nbShapesToSmooth += eos._toSmooth;

  } // check EDGEs

  // Reset _cosin if no smooth is allowed by the user
  for ( size_t iS = 0; iS < edgesByGeom.size(); ++iS )
  {
    _EdgesOnShape& eos = edgesByGeom[iS];
    if ( eos._edges.empty() ) continue;

    if ( !eos._hyp.ToSmooth() )
      for ( size_t i = 0; i < eos._edges.size(); ++i )
        eos._edges[i]->SetCosin( 0 );
  }


  // Fill _eosC1 to make that C1 FACEs and EGDEs between them to be smoothed as a whole

  TopTools_MapOfShape c1VV;

  for ( size_t iS = 0; iS < edgesByGeom.size(); ++iS ) // check FACEs
  {
    _EdgesOnShape& eos = edgesByGeom[iS];
    if ( eos._edges.empty() ||
         eos.ShapeType() != TopAbs_FACE ||
         !eos._toSmooth )
      continue;

    // check EDGEs of a FACE
    TopTools_MapOfShape checkedEE, allVV;
    list< SMESH_subMesh* > smQueue( 1, eos._subMesh ); // sm of FACEs
    while ( !smQueue.empty() )
    {
      SMESH_subMesh* sm = smQueue.front();
      smQueue.pop_front();
      SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(/*includeSelf=*/false);
      while ( smIt->more() )
      {
        sm = smIt->next();
        if ( sm->GetSubShape().ShapeType() == TopAbs_VERTEX )
          allVV.Add( sm->GetSubShape() );
        if ( sm->GetSubShape().ShapeType() != TopAbs_EDGE ||
             !checkedEE.Add( sm->GetSubShape() ))
          continue;

        _EdgesOnShape*      eoe = data.GetShapeEdges( sm->GetId() );
        vector<_LayerEdge*>& eE = eoe->_edges;
        if ( eE.empty() || !eoe->_sWOL.IsNull() )
          continue;

        bool isC1 = true; // check continuity along an EDGE
        for ( size_t i = 0; i < eE.size() && isC1; ++i )
          isC1 = ( Abs( eE[i]->_cosin ) < theMinSmoothCosin );
        if ( !isC1 )
          continue;

        // check that mesh faces are C1 as well
        {
          gp_XYZ norm1, norm2;
          const SMDS_MeshNode*   n = eE[ eE.size() / 2 ]->_nodes[0];
          SMDS_ElemIteratorPtr fIt = n->GetInverseElementIterator(SMDSAbs_Face);
          if ( !SMESH_MeshAlgos::FaceNormal( fIt->next(), norm1, /*normalized=*/true ))
            continue;
          while ( fIt->more() && isC1 )
            isC1 = ( SMESH_MeshAlgos::FaceNormal( fIt->next(), norm2, /*normalized=*/true ) &&
                     Abs( norm1 * norm2 ) >= ( 1. - theMinSmoothCosin ));
          if ( !isC1 )
            continue;
        }

        // add the EDGE and an adjacent FACE to _eosC1
        PShapeIteratorPtr fIt = helper.GetAncestors( sm->GetSubShape(), *_mesh, TopAbs_FACE );
        while ( const TopoDS_Shape* face = fIt->next() )
        {
          _EdgesOnShape* eof = data.GetShapeEdges( *face );
          if ( !eof ) continue; // other solid
          if ( !eos.HasC1( eoe ))
          {
            eos._eosC1.push_back( eoe );
            eoe->_toSmooth = false;
            data.PrepareEdgesToSmoothOnFace( eoe, /*substituteSrcNodes=*/false );
          }
          if ( eos._shapeID != eof->_shapeID && !eos.HasC1( eof )) 
          {
            eos._eosC1.push_back( eof );
            eof->_toSmooth = false;
            data.PrepareEdgesToSmoothOnFace( eof, /*substituteSrcNodes=*/false );
            smQueue.push_back( eof->_subMesh );
          }
        }
      }
    }
    if ( eos._eosC1.empty() )
      continue;

    // check VERTEXes of C1 FACEs
    TopTools_MapIteratorOfMapOfShape vIt( allVV );
    for ( ; vIt.More(); vIt.Next() )
    {
      _EdgesOnShape* eov = data.GetShapeEdges( vIt.Key() );
      if ( !eov || eov->_edges.empty() || !eov->_sWOL.IsNull() )
        continue;

      bool isC1 = true; // check if all adjacent FACEs are in eos._eosC1
      PShapeIteratorPtr fIt = helper.GetAncestors( vIt.Key(), *_mesh, TopAbs_FACE );
      while ( const TopoDS_Shape* face = fIt->next() )
      {
        _EdgesOnShape* eof = data.GetShapeEdges( *face );
        if ( !eof ) continue; // other solid
        isC1 = ( face->IsSame( eos._shape ) || eos.HasC1( eof ));
        if ( !isC1 )
          break;
      }
      if ( isC1 )
      {
        eos._eosC1.push_back( eov );
        data.PrepareEdgesToSmoothOnFace( eov, /*substituteSrcNodes=*/false );
        c1VV.Add( eov->_shape );
      }
    }

  } // fill _eosC1 of FACEs


  // Find C1 EDGEs

  vector< pair< _EdgesOnShape*, gp_XYZ > > dirOfEdges;

  for ( size_t iS = 0; iS < edgesByGeom.size(); ++iS ) // check VERTEXes
  {
    _EdgesOnShape& eov = edgesByGeom[iS];
    if ( eov._edges.empty() ||
         eov.ShapeType() != TopAbs_VERTEX ||
         c1VV.Contains( eov._shape ))
      continue;
    const TopoDS_Vertex& V = TopoDS::Vertex( eov._shape );

    // get directions of surrounding EDGEs
    dirOfEdges.clear();
    PShapeIteratorPtr fIt = helper.GetAncestors( eov._shape, *_mesh, TopAbs_EDGE );
    while ( const TopoDS_Shape* e = fIt->next() )
    {
      _EdgesOnShape* eoe = data.GetShapeEdges( *e );
      if ( !eoe ) continue; // other solid
      gp_XYZ eDir = getEdgeDir( TopoDS::Edge( *e ), V );
      if ( !Precision::IsInfinite( eDir.X() ))
        dirOfEdges.push_back( make_pair( eoe, eDir.Normalized() ));
    }

    // find EDGEs with C1 directions
    for ( size_t i = 0; i < dirOfEdges.size(); ++i )
      for ( size_t j = i+1; j < dirOfEdges.size(); ++j )
        if ( dirOfEdges[i].first && dirOfEdges[j].first )
        {
          double dot = dirOfEdges[i].second * dirOfEdges[j].second;
          bool isC1 = ( dot < - ( 1. - theMinSmoothCosin ));
          if ( isC1 )
          {
            double maxEdgeLen = 3 * Min( eov._edges[0]->_maxLen, eov._hyp.GetTotalThickness() );
            double eLen1 = SMESH_Algo::EdgeLength( TopoDS::Edge( dirOfEdges[i].first->_shape ));
            double eLen2 = SMESH_Algo::EdgeLength( TopoDS::Edge( dirOfEdges[j].first->_shape ));
            if ( eLen1 < maxEdgeLen ) eov._eosC1.push_back( dirOfEdges[i].first );
            if ( eLen2 < maxEdgeLen ) eov._eosC1.push_back( dirOfEdges[j].first );
            dirOfEdges[i].first = 0;
            dirOfEdges[j].first = 0;
          }
        }
  } // fill _eosC1 of VERTEXes



  return ok;
}

//================================================================================
/*!
 * \brief initialize data of _EdgesOnShape
 */
//================================================================================

void _ViscousBuilder::setShapeData( _EdgesOnShape& eos,
                                    SMESH_subMesh* sm,
                                    _SolidData&    data )
{
  if ( !eos._shape.IsNull() ||
       sm->GetSubShape().ShapeType() == TopAbs_WIRE )
    return;

  SMESH_MesherHelper helper( *_mesh );

  eos._subMesh = sm;
  eos._shapeID = sm->GetId();
  eos._shape   = sm->GetSubShape();
  if ( eos.ShapeType() == TopAbs_FACE )
    eos._shape.Orientation( helper.GetSubShapeOri( data._solid, eos._shape ));
  eos._toSmooth = false;
  eos._data = &data;

  // set _SWOL
  map< TGeomID, TopoDS_Shape >::const_iterator s2s =
    data._shrinkShape2Shape.find( eos._shapeID );
  if ( s2s != data._shrinkShape2Shape.end() )
    eos._sWOL = s2s->second;

  eos._isRegularSWOL = true;
  if ( eos.SWOLType() == TopAbs_FACE )
  {
    const TopoDS_Face& F = TopoDS::Face( eos._sWOL );
    Handle(ShapeAnalysis_Surface) surface = helper.GetSurface( F );
    eos._isRegularSWOL = ( ! surface->HasSingularities( 1e-7 ));
  }

  // set _hyp
  if ( data._hyps.size() == 1 )
  {
    eos._hyp = data._hyps.back();
  }
  else
  {
    // compute average StdMeshers_ViscousLayers parameters
    map< TGeomID, const StdMeshers_ViscousLayers* >::iterator f2hyp;
    if ( eos.ShapeType() == TopAbs_FACE )
    {
      if (( f2hyp = data._face2hyp.find( eos._shapeID )) != data._face2hyp.end() )
        eos._hyp = f2hyp->second;
    }
    else
    {
      PShapeIteratorPtr fIt = helper.GetAncestors( eos._shape, *_mesh, TopAbs_FACE );
      while ( const TopoDS_Shape* face = fIt->next() )
      {
        TGeomID faceID = getMeshDS()->ShapeToIndex( *face );
        if (( f2hyp = data._face2hyp.find( faceID )) != data._face2hyp.end() )
          eos._hyp.Add( f2hyp->second );
      }
    }
  }

  // set _faceNormals
  if ( ! eos._hyp.UseSurfaceNormal() )
  {
    if ( eos.ShapeType() == TopAbs_FACE ) // get normals to elements on a FACE
    {
      SMESHDS_SubMesh* smDS = sm->GetSubMeshDS();
      eos._faceNormals.resize( smDS->NbElements() );

      SMDS_ElemIteratorPtr eIt = smDS->GetElements();
      for ( int iF = 0; eIt->more(); ++iF )
      {
        const SMDS_MeshElement* face = eIt->next();
        if ( !SMESH_MeshAlgos::FaceNormal( face, eos._faceNormals[iF], /*normalized=*/true ))
          eos._faceNormals[iF].SetCoord( 0,0,0 );
      }

      if ( !helper.IsReversedSubMesh( TopoDS::Face( eos._shape )))
        for ( size_t iF = 0; iF < eos._faceNormals.size(); ++iF )
          eos._faceNormals[iF].Reverse();
    }
    else // find EOS of adjacent FACEs
    {
      PShapeIteratorPtr fIt = helper.GetAncestors( eos._shape, *_mesh, TopAbs_FACE );
      while ( const TopoDS_Shape* face = fIt->next() )
      {
        TGeomID faceID = getMeshDS()->ShapeToIndex( *face );
        eos._faceEOS.push_back( & data._edgesOnShape[ faceID ]);
        if ( eos._faceEOS.back()->_shape.IsNull() )
          // avoid using uninitialised _shapeID in GetNormal()
          eos._faceEOS.back()->_shapeID = faceID;
      }
    }
  }
}

//================================================================================
/*!
 * \brief Returns normal of a face
 */
//================================================================================

bool _EdgesOnShape::GetNormal( const SMDS_MeshElement* face, gp_Vec& norm )
{
  bool ok = false;
  const _EdgesOnShape* eos = 0;

  if ( face->getshapeId() == _shapeID )
  {
    eos = this;
  }
  else
  {
    for ( size_t iF = 0; iF < _faceEOS.size() && !eos; ++iF )
      if ( face->getshapeId() == _faceEOS[ iF ]->_shapeID )
        eos = _faceEOS[ iF ];
  }

  if (( eos ) &&
      ( ok = ( face->getIdInShape() < (int) eos->_faceNormals.size() )))
  {
    norm = eos->_faceNormals[ face->getIdInShape() ];
  }
  else if ( !eos )
  {
    debugMsg( "_EdgesOnShape::Normal() failed for face "<<face->GetID()
              << " on _shape #" << _shapeID );
  }
  return ok;
}


//================================================================================
/*!
 * \brief Set data of _LayerEdge needed for smoothing
 */
//================================================================================

bool _ViscousBuilder::setEdgeData(_LayerEdge&         edge,
                                  _EdgesOnShape&      eos,
                                  SMESH_MesherHelper& helper,
                                  _SolidData&         data)
{
  const SMDS_MeshNode* node = edge._nodes[0]; // source node

  edge._len       = 0;
  edge._maxLen    = Precision::Infinite();
  edge._minAngle  = 0;
  edge._2neibors  = 0;
  edge._curvature = 0;
  edge._flags     = 0;

  // --------------------------
  // Compute _normal and _cosin
  // --------------------------

  edge._cosin     = 0;
  edge._lenFactor = 1.;
  edge._normal.SetCoord(0,0,0);
  _Simplex::GetSimplices( node, edge._simplices, data._ignoreFaceIds, &data );

  int totalNbFaces = 0;
  TopoDS_Face F;
  std::pair< TopoDS_Face, gp_XYZ > face2Norm[20];
  gp_Vec geomNorm;
  bool normOK = true;

  const bool onShrinkShape = !eos._sWOL.IsNull();
  const bool useGeometry   = (( eos._hyp.UseSurfaceNormal() ) ||
                              ( eos.ShapeType() != TopAbs_FACE /*&& !onShrinkShape*/ ));

  // get geom FACEs the node lies on
  //if ( useGeometry )
  {
    set<TGeomID> faceIds;
    if  ( eos.ShapeType() == TopAbs_FACE )
    {
      faceIds.insert( eos._shapeID );
    }
    else
    {
      SMDS_ElemIteratorPtr fIt = node->GetInverseElementIterator(SMDSAbs_Face);
      while ( fIt->more() )
        faceIds.insert( fIt->next()->getshapeId() );
    }
    set<TGeomID>::iterator id = faceIds.begin();
    for ( ; id != faceIds.end(); ++id )
    {
      const TopoDS_Shape& s = getMeshDS()->IndexToShape( *id );
      if ( s.IsNull() || s.ShapeType() != TopAbs_FACE || data._ignoreFaceIds.count( *id ))
        continue;
      F = TopoDS::Face( s );
      face2Norm[ totalNbFaces ].first = F;
      totalNbFaces++;
    }
  }

  // find _normal
  if ( useGeometry )
  {
    bool fromVonF = ( eos.ShapeType() == TopAbs_VERTEX &&
                      eos.SWOLType()  == TopAbs_FACE  &&
                      totalNbFaces > 1 );

    if ( onShrinkShape && !fromVonF ) // one of faces the node is on has no layers
    {
      if ( eos.SWOLType() == TopAbs_EDGE )
      {
        // inflate from VERTEX along EDGE
        edge._normal = getEdgeDir( TopoDS::Edge( eos._sWOL ), TopoDS::Vertex( eos._shape ));
      }
      else if ( eos.ShapeType() == TopAbs_VERTEX )
      {
        // inflate from VERTEX along FACE
        edge._normal = getFaceDir( TopoDS::Face( eos._sWOL ), TopoDS::Vertex( eos._shape ),
                                   node, helper, normOK, &edge._cosin);
      }
      else
      {
        // inflate from EDGE along FACE
        edge._normal = getFaceDir( TopoDS::Face( eos._sWOL ), TopoDS::Edge( eos._shape ),
                                   node, helper, normOK);
      }
    }
    else // layers are on all FACEs of SOLID the node is on (or fromVonF)
    {
      if ( fromVonF )
        face2Norm[ totalNbFaces++ ].first = TopoDS::Face( eos._sWOL );

      int nbOkNorms = 0;
      for ( int iF = totalNbFaces - 1; iF >= 0; --iF )
      {
        F = face2Norm[ iF ].first;
        geomNorm = getFaceNormal( node, F, helper, normOK );
        if ( !normOK ) continue;
        nbOkNorms++;

        if ( helper.GetSubShapeOri( data._solid, F ) != TopAbs_REVERSED )
          geomNorm.Reverse();
        face2Norm[ iF ].second = geomNorm.XYZ();
        edge._normal += geomNorm.XYZ();
      }
      if ( nbOkNorms == 0 )
        return error(SMESH_Comment("Can't get normal to node ") << node->GetID(), data._index);

      if ( totalNbFaces >= 3 )
      {
        edge._normal = getNormalByOffset( &edge, face2Norm, totalNbFaces, fromVonF );
      }

      if ( edge._normal.Modulus() < 1e-3 && nbOkNorms > 1 )
      {
        // opposite normals, re-get normals at shifted positions (IPAL 52426)
        edge._normal.SetCoord( 0,0,0 );
        for ( int iF = 0; iF < totalNbFaces - fromVonF; ++iF )
        {
          const TopoDS_Face& F = face2Norm[iF].first;
          geomNorm = getFaceNormal( node, F, helper, normOK, /*shiftInside=*/true );
          if ( helper.GetSubShapeOri( data._solid, F ) != TopAbs_REVERSED )
            geomNorm.Reverse();
          if ( normOK )
            face2Norm[ iF ].second = geomNorm.XYZ();
          edge._normal += face2Norm[ iF ].second;
        }
      }
    }
  }
  else // !useGeometry - get _normal using surrounding mesh faces
  {
    edge._normal = getWeigthedNormal( &edge );

    // set<TGeomID> faceIds;
    //
    // SMDS_ElemIteratorPtr fIt = node->GetInverseElementIterator(SMDSAbs_Face);
    // while ( fIt->more() )
    // {
    //   const SMDS_MeshElement* face = fIt->next();
    //   if ( eos.GetNormal( face, geomNorm ))
    //   {
    //     if ( onShrinkShape && !faceIds.insert( face->getshapeId() ).second )
    //       continue; // use only one mesh face on FACE
    //     edge._normal += geomNorm.XYZ();
    //     totalNbFaces++;
    //   }
    // }
  }

  // compute _cosin
  //if ( eos._hyp.UseSurfaceNormal() )
  {
    switch ( eos.ShapeType() )
    {
    case TopAbs_FACE: {
      edge._cosin = 0;
      break;
    }
    case TopAbs_EDGE: {
      TopoDS_Edge E    = TopoDS::Edge( eos._shape );
      gp_Vec inFaceDir = getFaceDir( F, E, node, helper, normOK );
      double angle     = inFaceDir.Angle( edge._normal ); // [0,PI]
      edge._cosin      = Cos( angle );
      break;
    }
    case TopAbs_VERTEX: {
      //if ( eos.SWOLType() != TopAbs_FACE ) // else _cosin is set by getFaceDir()
      {
        TopoDS_Vertex V  = TopoDS::Vertex( eos._shape );
        gp_Vec inFaceDir = getFaceDir( F, V, node, helper, normOK );
        double angle     = inFaceDir.Angle( edge._normal ); // [0,PI]
        edge._cosin      = Cos( angle );
        if ( totalNbFaces > 2 || helper.IsSeamShape( node->getshapeId() ))
          for ( int iF = totalNbFaces-2; iF >=0; --iF )
          {
            F = face2Norm[ iF ].first;
            inFaceDir = getFaceDir( F, V, node, helper, normOK=true );
            if ( normOK ) {
              double angle = inFaceDir.Angle( edge._normal );
              double cosin = Cos( angle );
              if ( Abs( cosin ) > Abs( edge._cosin ))
                edge._cosin = cosin;
            }
          }
      }
      break;
    }
    default:
      return error(SMESH_Comment("Invalid shape position of node ")<<node, data._index);
    }
  }

  double normSize = edge._normal.SquareModulus();
  if ( normSize < numeric_limits<double>::min() )
    return error(SMESH_Comment("Bad normal at node ")<< node->GetID(), data._index );

  edge._normal /= sqrt( normSize );

  if ( edge.Is( _LayerEdge::MULTI_NORMAL ) && edge._nodes.size() == 2 )
  {
    getMeshDS()->RemoveFreeNode( edge._nodes.back(), 0, /*fromGroups=*/false );
    edge._nodes.resize( 1 );
    edge._normal.SetCoord( 0,0,0 );
    edge._maxLen = 0;
  }

  // Set the rest data
  // --------------------

  edge.SetCosin( edge._cosin ); // to update edge._lenFactor

  if ( onShrinkShape )
  {
    const SMDS_MeshNode* tgtNode = edge._nodes.back();
    if ( SMESHDS_SubMesh* sm = getMeshDS()->MeshElements( data._solid ))
      sm->RemoveNode( tgtNode , /*isNodeDeleted=*/false );

    // set initial position which is parameters on _sWOL in this case
    if ( eos.SWOLType() == TopAbs_EDGE )
    {
      double u = helper.GetNodeU( TopoDS::Edge( eos._sWOL ), node, 0, &normOK );
      edge._pos.push_back( gp_XYZ( u, 0, 0 ));
      if ( edge._nodes.size() > 1 )
        getMeshDS()->SetNodeOnEdge( tgtNode, TopoDS::Edge( eos._sWOL ), u );
    }
    else // eos.SWOLType() == TopAbs_FACE
    {
      gp_XY uv = helper.GetNodeUV( TopoDS::Face( eos._sWOL ), node, 0, &normOK );
      edge._pos.push_back( gp_XYZ( uv.X(), uv.Y(), 0));
      if ( edge._nodes.size() > 1 )
        getMeshDS()->SetNodeOnFace( tgtNode, TopoDS::Face( eos._sWOL ), uv.X(), uv.Y() );
    }

    if ( edge._nodes.size() > 1 )
    {
      // check if an angle between a FACE with layers and SWOL is sharp,
      // else the edge should not inflate
      F.Nullify();
      for ( int iF = 0; iF < totalNbFaces  &&  F.IsNull();  ++iF ) // find a FACE with VL
        if ( ! helper.IsSubShape( eos._sWOL, face2Norm[iF].first ))
          F = face2Norm[iF].first;
      if ( !F.IsNull())
      {
        geomNorm = getFaceNormal( node, F, helper, normOK );
        if ( helper.GetSubShapeOri( data._solid, F ) != TopAbs_REVERSED )
          geomNorm.Reverse(); // inside the SOLID
        if ( geomNorm * edge._normal < -0.001 )
        {
          getMeshDS()->RemoveFreeNode( tgtNode, 0, /*fromGroups=*/false );
          edge._nodes.resize( 1 );
        }
        else if ( edge._lenFactor > 3 )
        {
          edge._lenFactor = 2;
          edge.Set( _LayerEdge::RISKY_SWOL );
        }
      }
    }
  }
  else
  {
    edge._pos.push_back( SMESH_TNodeXYZ( node ));

    if ( eos.ShapeType() == TopAbs_FACE )
    {
      double angle;
      for ( size_t i = 0; i < edge._simplices.size(); ++i )
      {
        edge._simplices[i].IsMinAngleOK( edge._pos.back(), angle );
        edge._minAngle = Max( edge._minAngle, angle ); // "angle" is actually cosine
      }
    }
  }

  // Set neighbor nodes for a _LayerEdge based on EDGE

  if ( eos.ShapeType() == TopAbs_EDGE /*||
       ( onShrinkShape && posType == SMDS_TOP_VERTEX && fabs( edge._cosin ) < 1e-10 )*/)
  {
    edge._2neibors = new _2NearEdges;
    // target nodes instead of source ones will be set later
  }

  return true;
}

//================================================================================
/*!
 * \brief Return normal to a FACE at a node
 *  \param [in] n - node
 *  \param [in] face - FACE
 *  \param [in] helper - helper
 *  \param [out] isOK - true or false
 *  \param [in] shiftInside - to find normal at a position shifted inside the face
 *  \return gp_XYZ - normal
 */
//================================================================================

gp_XYZ _ViscousBuilder::getFaceNormal(const SMDS_MeshNode* node,
                                      const TopoDS_Face&   face,
                                      SMESH_MesherHelper&  helper,
                                      bool&                isOK,
                                      bool                 shiftInside)
{
  gp_XY uv;
  if ( shiftInside )
  {
    // get a shifted position
    gp_Pnt p = SMESH_TNodeXYZ( node );
    gp_XYZ shift( 0,0,0 );
    TopoDS_Shape S = helper.GetSubShapeByNode( node, helper.GetMeshDS() );
    switch ( S.ShapeType() ) {
    case TopAbs_VERTEX:
    {
      shift = getFaceDir( face, TopoDS::Vertex( S ), node, helper, isOK );
      break;
    }
    case TopAbs_EDGE:
    {
      shift = getFaceDir( face, TopoDS::Edge( S ), node, helper, isOK );
      break;
    }
    default:
      isOK = false;
    }
    if ( isOK )
      shift.Normalize();
    p.Translate( shift * 1e-5 );

    TopLoc_Location loc;
    GeomAPI_ProjectPointOnSurf& projector = helper.GetProjector( face, loc, 1e-7 );

    if ( !loc.IsIdentity() ) p.Transform( loc.Transformation().Inverted() );
    
    projector.Perform( p );
    if ( !projector.IsDone() || projector.NbPoints() < 1 )
    {
      isOK = false;
      return p.XYZ();
    }
    Standard_Real U,V;
    projector.LowerDistanceParameters(U,V);
    uv.SetCoord( U,V );
  }
  else
  {
    uv = helper.GetNodeUV( face, node, 0, &isOK );
  }

  gp_Dir normal;
  isOK = false;

  Handle(Geom_Surface) surface = BRep_Tool::Surface( face );

  if ( !shiftInside &&
       helper.IsDegenShape( node->getshapeId() ) &&
       getFaceNormalAtSingularity( uv, face, helper, normal ))
  {
    isOK = true;
    return normal.XYZ();
  }

  int pointKind = GeomLib::NormEstim( surface, uv, 1e-5, normal );
  enum { REGULAR = 0, QUASYSINGULAR, CONICAL, IMPOSSIBLE };

  if ( pointKind == IMPOSSIBLE &&
       node->GetPosition()->GetDim() == 2 ) // node inside the FACE
  {
    // probably NormEstim() failed due to a too high tolerance
    pointKind = GeomLib::NormEstim( surface, uv, 1e-20, normal );
    isOK = ( pointKind < IMPOSSIBLE );
  }
  if ( pointKind < IMPOSSIBLE )
  {
    if ( pointKind != REGULAR &&
         !shiftInside &&
         node->GetPosition()->GetDim() < 2 ) // FACE boundary
    {
      gp_XYZ normShift = getFaceNormal( node, face, helper, isOK, /*shiftInside=*/true );
      if ( normShift * normal.XYZ() < 0. )
        normal = normShift;
    }
    isOK = true;
  }

  if ( !isOK ) // hard singularity, to call with shiftInside=true ?
  {
    const TGeomID faceID = helper.GetMeshDS()->ShapeToIndex( face );

    SMDS_ElemIteratorPtr fIt = node->GetInverseElementIterator(SMDSAbs_Face);
    while ( fIt->more() )
    {
      const SMDS_MeshElement* f = fIt->next();
      if ( f->getshapeId() == faceID )
      {
        isOK = SMESH_MeshAlgos::FaceNormal( f, (gp_XYZ&) normal.XYZ(), /*normalized=*/true );
        if ( isOK )
        {
          TopoDS_Face ff = face;
          ff.Orientation( TopAbs_FORWARD );
          if ( helper.IsReversedSubMesh( ff ))
            normal.Reverse();
          break;
        }
      }
    }
  }
  return normal.XYZ();
}

//================================================================================
/*!
 * \brief Try to get normal at a singularity of a surface basing on it's nature
 */
//================================================================================

bool _ViscousBuilder::getFaceNormalAtSingularity( const gp_XY&        uv,
                                                  const TopoDS_Face&  face,
                                                  SMESH_MesherHelper& helper,
                                                  gp_Dir&             normal )
{
  BRepAdaptor_Surface surface( face );
  gp_Dir axis;
  if ( !getRovolutionAxis( surface, axis ))
    return false;

  double f,l, d, du, dv;
  f = surface.FirstUParameter();
  l = surface.LastUParameter();
  d = ( uv.X() - f ) / ( l - f );
  du = ( d < 0.5 ? +1. : -1 ) * 1e-5 * ( l - f );
  f = surface.FirstVParameter();
  l = surface.LastVParameter();
  d = ( uv.Y() - f ) / ( l - f );
  dv = ( d < 0.5 ? +1. : -1 ) * 1e-5 * ( l - f );

  gp_Dir refDir;
  gp_Pnt2d testUV = uv;
  enum { REGULAR = 0, QUASYSINGULAR, CONICAL, IMPOSSIBLE };
  double tol = 1e-5;
  Handle(Geom_Surface) geomsurf = surface.Surface().Surface();
  for ( int iLoop = 0; true ; ++iLoop )
  {
    testUV.SetCoord( testUV.X() + du, testUV.Y() + dv );
    if ( GeomLib::NormEstim( geomsurf, testUV, tol, refDir ) == REGULAR )
      break;
    if ( iLoop > 20 )
      return false;
    tol /= 10.;
  }

  if ( axis * refDir < 0. )
    axis.Reverse();

  normal = axis;

  return true;
}

//================================================================================
/*!
 * \brief Return a normal at a node weighted with angles taken by faces
 */
//================================================================================

gp_XYZ _ViscousBuilder::getWeigthedNormal( const _LayerEdge* edge )
{
  const SMDS_MeshNode* n = edge->_nodes[0];

  gp_XYZ resNorm(0,0,0);
  SMESH_TNodeXYZ p0( n ), pP, pN;
  for ( size_t i = 0; i < edge->_simplices.size(); ++i )
  {
    pP.Set( edge->_simplices[i]._nPrev );
    pN.Set( edge->_simplices[i]._nNext );
    gp_Vec v0P( p0, pP ), v0N( p0, pN ), vPN( pP, pN ), norm = v0P ^ v0N;
    double l0P = v0P.SquareMagnitude();
    double l0N = v0N.SquareMagnitude();
    double lPN = vPN.SquareMagnitude();
    if ( l0P < std::numeric_limits<double>::min() ||
         l0N < std::numeric_limits<double>::min() ||
         lPN < std::numeric_limits<double>::min() )
      continue;
    double lNorm = norm.SquareMagnitude();
    double  sin2 = lNorm / l0P / l0N;
    double angle = ACos(( v0P * v0N ) / Sqrt( l0P ) / Sqrt( l0N ));

    double weight = sin2 * angle / lPN;
    resNorm += weight * norm.XYZ() / Sqrt( lNorm );
  }

  return resNorm;
}

//================================================================================
/*!
 * \brief Return a normal at a node by getting a common point of offset planes
 *        defined by the FACE normals
 */
//================================================================================

gp_XYZ _ViscousBuilder::getNormalByOffset( _LayerEdge*                      edge,
                                           std::pair< TopoDS_Face, gp_XYZ > f2Normal[],
                                           int                              nbFaces,
                                           bool                             lastNoOffset)
{
  SMESH_TNodeXYZ p0 = edge->_nodes[0];

  gp_XYZ resNorm(0,0,0);
  TopoDS_Shape V = SMESH_MesherHelper::GetSubShapeByNode( p0._node, getMeshDS() );
  if ( V.ShapeType() != TopAbs_VERTEX || nbFaces < 3 )
  {
    for ( int i = 0; i < nbFaces; ++i )
      resNorm += f2Normal[i].second;
    return resNorm;
  }

  // prepare _OffsetPlane's
  vector< _OffsetPlane > pln( nbFaces );
  for ( int i = 0; i < nbFaces - lastNoOffset; ++i )
  {
    pln[i]._faceIndex = i;
    pln[i]._plane = gp_Pln( p0 + f2Normal[i].second, f2Normal[i].second );
  }
  if ( lastNoOffset )
  {
    pln[ nbFaces - 1 ]._faceIndex = nbFaces - 1;
    pln[ nbFaces - 1 ]._plane = gp_Pln( p0, f2Normal[ nbFaces - 1 ].second );
  }

  // intersect neighboring OffsetPlane's
  PShapeIteratorPtr edgeIt = SMESH_MesherHelper::GetAncestors( V, *_mesh, TopAbs_EDGE );
  while ( const TopoDS_Shape* edge = edgeIt->next() )
  {
    int f1 = -1, f2 = -1;
    for ( int i = 0; i < nbFaces &&  f2 < 0;  ++i )
      if ( SMESH_MesherHelper::IsSubShape( *edge, f2Normal[i].first ))
        (( f1 < 0 ) ? f1 : f2 ) = i;

    if ( f2 >= 0 )
      pln[ f1 ].ComputeIntersectionLine( pln[ f2 ], TopoDS::Edge( *edge ), TopoDS::Vertex( V ));
  }

  // get a common point
  gp_XYZ commonPnt( 0, 0, 0 );
  int nbPoints = 0;
  bool isPointFound;
  for ( int i = 0; i < nbFaces; ++i )
  {
    commonPnt += pln[ i ].GetCommonPoint( isPointFound, TopoDS::Vertex( V ));
    nbPoints  += isPointFound;
  }
  gp_XYZ wgtNorm = getWeigthedNormal( edge );
  if ( nbPoints == 0 )
    return wgtNorm;

  commonPnt /= nbPoints;
  resNorm = commonPnt - p0;
  if ( lastNoOffset )
    return resNorm;

  // choose the best among resNorm and wgtNorm
  resNorm.Normalize();
  wgtNorm.Normalize();
  double resMinDot = std::numeric_limits<double>::max();
  double wgtMinDot = std::numeric_limits<double>::max();
  for ( int i = 0; i < nbFaces - lastNoOffset; ++i )
  {
    resMinDot = Min( resMinDot, resNorm * f2Normal[i].second );
    wgtMinDot = Min( wgtMinDot, wgtNorm * f2Normal[i].second );
  }

  if ( Max( resMinDot, wgtMinDot ) < theMinSmoothCosin )
  {
    edge->Set( _LayerEdge::MULTI_NORMAL );
  }

  return ( resMinDot > wgtMinDot ) ? resNorm : wgtNorm;
}

//================================================================================
/*!
 * \brief Compute line of intersection of 2 planes
 */
//================================================================================

void _OffsetPlane::ComputeIntersectionLine( _OffsetPlane&        pln,
                                            const TopoDS_Edge&   E,
                                            const TopoDS_Vertex& V )
{
  int iNext = bool( _faceIndexNext[0] >= 0 );
  _faceIndexNext[ iNext ] = pln._faceIndex;

  gp_XYZ n1 = _plane.Axis().Direction().XYZ();
  gp_XYZ n2 = pln._plane.Axis().Direction().XYZ();

  gp_XYZ lineDir = n1 ^ n2;

  double x = Abs( lineDir.X() );
  double y = Abs( lineDir.Y() );
  double z = Abs( lineDir.Z() );

  int cooMax; // max coordinate
  if (x > y) {
    if (x > z) cooMax = 1;
    else       cooMax = 3;
  }
  else {
    if (y > z) cooMax = 2;
    else       cooMax = 3;
  }

  gp_Pnt linePos;
  if ( Abs( lineDir.Coord( cooMax )) < 0.05 )
  {
    // parallel planes - intersection is an offset of the common EDGE
    gp_Pnt p = BRep_Tool::Pnt( V );
    linePos  = 0.5 * (( p.XYZ() + n1 ) + ( p.XYZ() + n2 ));
    lineDir  = getEdgeDir( E, V );
  }
  else
  {
    // the constants in the 2 plane equations
    double d1 = - ( _plane.Axis().Direction().XYZ()     * _plane.Location().XYZ() );
    double d2 = - ( pln._plane.Axis().Direction().XYZ() * pln._plane.Location().XYZ() );

    switch ( cooMax ) {
    case 1:
      linePos.SetX(  0 );
      linePos.SetY(( d2*n1.Z() - d1*n2.Z()) / lineDir.X() );
      linePos.SetZ(( d1*n2.Y() - d2*n1.Y()) / lineDir.X() );
      break;
    case 2:
      linePos.SetX(( d1*n2.Z() - d2*n1.Z()) / lineDir.Y() );
      linePos.SetY(  0 );
      linePos.SetZ(( d2*n1.X() - d1*n2.X()) / lineDir.Y() );
      break;
    case 3:
      linePos.SetX(( d2*n1.Y() - d1*n2.Y()) / lineDir.Z() );
      linePos.SetY(( d1*n2.X() - d2*n1.X()) / lineDir.Z() );
      linePos.SetZ(  0 );
    }
  }
  gp_Lin& line = _lines[ iNext ];
  line.SetDirection( lineDir );
  line.SetLocation ( linePos );

  _isLineOK[ iNext ] = true;


  iNext = bool( pln._faceIndexNext[0] >= 0 );
  pln._lines        [ iNext ] = line;
  pln._faceIndexNext[ iNext ] = this->_faceIndex;
  pln._isLineOK     [ iNext ] = true;
}

//================================================================================
/*!
 * \brief Computes intersection point of two _lines
 */
//================================================================================

gp_XYZ _OffsetPlane::GetCommonPoint(bool&                 isFound,
                                    const TopoDS_Vertex & V) const
{
  gp_XYZ p( 0,0,0 );
  isFound = false;

  if ( NbLines() == 2 )
  {
    gp_Vec lPerp0 = _lines[0].Direction().XYZ() ^ _plane.Axis().Direction().XYZ();
    double  dot01 = lPerp0 * _lines[1].Direction().XYZ();
    if ( Abs( dot01 ) > 0.05 )
    {
      gp_Vec l0l1 = _lines[1].Location().XYZ() - _lines[0].Location().XYZ();
      double   u1 = - ( lPerp0 * l0l1 ) / dot01;
      p = ( _lines[1].Location().XYZ() + _lines[1].Direction().XYZ() * u1 );
      isFound = true;
    }
    else
    {
      gp_Pnt  pV ( BRep_Tool::Pnt( V ));
      gp_Vec  lv0( _lines[0].Location(), pV    ),  lv1(_lines[1].Location(), pV     );
      double dot0( lv0 * _lines[0].Direction() ), dot1( lv1 * _lines[1].Direction() );
      p += 0.5 * ( _lines[0].Location().XYZ() + _lines[0].Direction().XYZ() * dot0 );
      p += 0.5 * ( _lines[1].Location().XYZ() + _lines[1].Direction().XYZ() * dot1 );
      isFound = true;
    }
  }

  return p;
}

//================================================================================
/*!
 * \brief Find 2 neigbor nodes of a node on EDGE
 */
//================================================================================

bool _ViscousBuilder::findNeiborsOnEdge(const _LayerEdge*     edge,
                                        const SMDS_MeshNode*& n1,
                                        const SMDS_MeshNode*& n2,
                                        _EdgesOnShape&        eos,
                                        _SolidData&           data)
{
  const SMDS_MeshNode* node = edge->_nodes[0];
  const int        shapeInd = eos._shapeID;
  SMESHDS_SubMesh*   edgeSM = 0;
  if ( eos.ShapeType() == TopAbs_EDGE )
  {
    edgeSM = eos._subMesh->GetSubMeshDS();
    if ( !edgeSM || edgeSM->NbElements() == 0 )
      return error(SMESH_Comment("Not meshed EDGE ") << shapeInd, data._index);
  }
  int iN = 0;
  n2 = 0;
  SMDS_ElemIteratorPtr eIt = node->GetInverseElementIterator(SMDSAbs_Edge);
  while ( eIt->more() && !n2 )
  {
    const SMDS_MeshElement* e = eIt->next();
    const SMDS_MeshNode*   nNeibor = e->GetNode( 0 );
    if ( nNeibor == node ) nNeibor = e->GetNode( 1 );
    if ( edgeSM )
    {
      if (!edgeSM->Contains(e)) continue;
    }
    else
    {
      TopoDS_Shape s = SMESH_MesherHelper::GetSubShapeByNode( nNeibor, getMeshDS() );
      if ( !SMESH_MesherHelper::IsSubShape( s, eos._sWOL )) continue;
    }
    ( iN++ ? n2 : n1 ) = nNeibor;
  }
  if ( !n2 )
    return error(SMESH_Comment("Wrongly meshed EDGE ") << shapeInd, data._index);
  return true;
}

//================================================================================
/*!
 * \brief Set _curvature and _2neibors->_plnNorm by 2 neigbor nodes residing the same EDGE
 */
//================================================================================

void _LayerEdge::SetDataByNeighbors( const SMDS_MeshNode* n1,
                                     const SMDS_MeshNode* n2,
                                     const _EdgesOnShape& eos,
                                     SMESH_MesherHelper&  helper)
{
  if ( eos.ShapeType() != TopAbs_EDGE )
    return;

  gp_XYZ  pos = SMESH_TNodeXYZ( _nodes[0] );
  gp_XYZ vec1 = pos - SMESH_TNodeXYZ( n1 );
  gp_XYZ vec2 = pos - SMESH_TNodeXYZ( n2 );

  // Set _curvature

  double      sumLen = vec1.Modulus() + vec2.Modulus();
  _2neibors->_wgt[0] = 1 - vec1.Modulus() / sumLen;
  _2neibors->_wgt[1] = 1 - vec2.Modulus() / sumLen;
  double avgNormProj = 0.5 * ( _normal * vec1 + _normal * vec2 );
  double      avgLen = 0.5 * ( vec1.Modulus() + vec2.Modulus() );
  if ( _curvature ) delete _curvature;
  _curvature = _Curvature::New( avgNormProj, avgLen );
  // if ( _curvature )
  //   debugMsg( _nodes[0]->GetID()
  //             << " CURV r,k: " << _curvature->_r<<","<<_curvature->_k
  //             << " proj = "<<avgNormProj<< " len = " << avgLen << "| lenDelta(0) = "
  //             << _curvature->lenDelta(0) );

  // Set _plnNorm

  if ( eos._sWOL.IsNull() )
  {
    TopoDS_Edge  E = TopoDS::Edge( eos._shape );
    // if ( SMESH_Algo::isDegenerated( E ))
    //   return;
    gp_XYZ dirE    = getEdgeDir( E, _nodes[0], helper );
    gp_XYZ plnNorm = dirE ^ _normal;
    double proj0   = plnNorm * vec1;
    double proj1   = plnNorm * vec2;
    if ( fabs( proj0 ) > 1e-10 || fabs( proj1 ) > 1e-10 )
    {
      if ( _2neibors->_plnNorm ) delete _2neibors->_plnNorm;
      _2neibors->_plnNorm = new gp_XYZ( plnNorm.Normalized() );
    }
  }
}

//================================================================================
/*!
 * \brief Copy data from a _LayerEdge of other SOLID and based on the same node;
 * this and the other _LayerEdge are inflated along a FACE or an EDGE
 */
//================================================================================

gp_XYZ _LayerEdge::Copy( _LayerEdge&         other,
                         _EdgesOnShape&      eos,
                         SMESH_MesherHelper& helper )
{
  _nodes     = other._nodes;
  _normal    = other._normal;
  _len       = 0;
  _lenFactor = other._lenFactor;
  _cosin     = other._cosin;
  _2neibors  = other._2neibors;
  _curvature = 0; std::swap( _curvature, other._curvature );
  _2neibors  = 0; std::swap( _2neibors,  other._2neibors );

  gp_XYZ lastPos( 0,0,0 );
  if ( eos.SWOLType() == TopAbs_EDGE )
  {
    double u = helper.GetNodeU( TopoDS::Edge( eos._sWOL ), _nodes[0] );
    _pos.push_back( gp_XYZ( u, 0, 0));

    u = helper.GetNodeU( TopoDS::Edge( eos._sWOL ), _nodes.back() );
    lastPos.SetX( u );
  }
  else // TopAbs_FACE
  {
    gp_XY uv = helper.GetNodeUV( TopoDS::Face( eos._sWOL ), _nodes[0]);
    _pos.push_back( gp_XYZ( uv.X(), uv.Y(), 0));

    uv = helper.GetNodeUV( TopoDS::Face( eos._sWOL ), _nodes.back() );
    lastPos.SetX( uv.X() );
    lastPos.SetY( uv.Y() );
  }
  return lastPos;
}

//================================================================================
/*!
 * \brief Set _cosin and _lenFactor
 */
//================================================================================

void _LayerEdge::SetCosin( double cosin )
{
  _cosin = cosin;
  cosin = Abs( _cosin );
  //_lenFactor = ( cosin < 1.-1e-12 ) ?  Min( 2., 1./sqrt(1-cosin*cosin )) : 1.0;
  _lenFactor = ( cosin < 1.-1e-12 ) ?  1./sqrt(1-cosin*cosin ) : 1.0;
}

//================================================================================
/*!
 * \brief Check if another _LayerEdge is a neighbor on EDGE
 */
//================================================================================

bool _LayerEdge::IsNeiborOnEdge( const _LayerEdge* edge ) const
{
  return (( this->_2neibors && this->_2neibors->include( edge )) ||
          ( edge->_2neibors && edge->_2neibors->include( this )));
}

//================================================================================
/*!
 * \brief Fills a vector<_Simplex > 
 */
//================================================================================

void _Simplex::GetSimplices( const SMDS_MeshNode* node,
                             vector<_Simplex>&    simplices,
                             const set<TGeomID>&  ingnoreShapes,
                             const _SolidData*    dataToCheckOri,
                             const bool           toSort)
{
  simplices.clear();
  SMDS_ElemIteratorPtr fIt = node->GetInverseElementIterator(SMDSAbs_Face);
  while ( fIt->more() )
  {
    const SMDS_MeshElement* f = fIt->next();
    const TGeomID    shapeInd = f->getshapeId();
    if ( ingnoreShapes.count( shapeInd )) continue;
    const int nbNodes = f->NbCornerNodes();
    const int  srcInd = f->GetNodeIndex( node );
    const SMDS_MeshNode* nPrev = f->GetNode( SMESH_MesherHelper::WrapIndex( srcInd-1, nbNodes ));
    const SMDS_MeshNode* nNext = f->GetNode( SMESH_MesherHelper::WrapIndex( srcInd+1, nbNodes ));
    const SMDS_MeshNode* nOpp  = f->GetNode( SMESH_MesherHelper::WrapIndex( srcInd+2, nbNodes ));
    if ( dataToCheckOri && dataToCheckOri->_reversedFaceIds.count( shapeInd ))
      std::swap( nPrev, nNext );
    simplices.push_back( _Simplex( nPrev, nNext, ( nbNodes == 3 ? 0 : nOpp )));
  }

  if ( toSort )
    SortSimplices( simplices );
}

//================================================================================
/*!
 * \brief Set neighbor simplices side by side
 */
//================================================================================

void _Simplex::SortSimplices(vector<_Simplex>& simplices)
{
  vector<_Simplex> sortedSimplices( simplices.size() );
  sortedSimplices[0] = simplices[0];
  size_t nbFound = 0;
  for ( size_t i = 1; i < simplices.size(); ++i )
  {
    for ( size_t j = 1; j < simplices.size(); ++j )
      if ( sortedSimplices[i-1]._nNext == simplices[j]._nPrev )
      {
        sortedSimplices[i] = simplices[j];
        nbFound++;
        break;
      }
  }
  if ( nbFound == simplices.size() - 1 )
    simplices.swap( sortedSimplices );
}

//================================================================================
/*!
 * \brief DEBUG. Create groups contating temorary data of _LayerEdge's
 */
//================================================================================

void _ViscousBuilder::makeGroupOfLE()
{
#ifdef _DEBUG_
  for ( size_t i = 0 ; i < _sdVec.size(); ++i )
  {
    if ( _sdVec[i]._n2eMap.empty() ) continue;

    dumpFunction( SMESH_Comment("make_LayerEdge_") << i );
    TNode2Edge::iterator n2e;
    for ( n2e = _sdVec[i]._n2eMap.begin(); n2e != _sdVec[i]._n2eMap.end(); ++n2e )
    {
      _LayerEdge* le = n2e->second;
      // for ( size_t iN = 1; iN < le->_nodes.size(); ++iN )
      //   dumpCmd(SMESH_Comment("mesh.AddEdge([ ") <<le->_nodes[iN-1]->GetID()
      //           << ", " << le->_nodes[iN]->GetID() <<"])");
      if ( le ) {
        dumpCmd(SMESH_Comment("mesh.AddEdge([ ") <<le->_nodes[0]->GetID()
                << ", " << le->_nodes.back()->GetID() <<"]) # " << le->_flags );
      }
    }
    dumpFunctionEnd();

    dumpFunction( SMESH_Comment("makeNormals") << i );
    for ( n2e = _sdVec[i]._n2eMap.begin(); n2e != _sdVec[i]._n2eMap.end(); ++n2e )
    {
      _LayerEdge* edge = n2e->second;
      SMESH_TNodeXYZ nXYZ( edge->_nodes[0] );
      nXYZ += edge->_normal * _sdVec[i]._stepSize;
      dumpCmd(SMESH_Comment("mesh.AddEdge([ ") << edge->_nodes[0]->GetID()
              << ", mesh.AddNode( "<< nXYZ.X()<<","<< nXYZ.Y()<<","<< nXYZ.Z()<<")])");
    }
    dumpFunctionEnd();

    dumpFunction( SMESH_Comment("makeTmpFaces_") << i );
    dumpCmd( "faceId1 = mesh.NbElements()" );
    TopExp_Explorer fExp( _sdVec[i]._solid, TopAbs_FACE );
    for ( ; fExp.More(); fExp.Next() )
    {
      if ( const SMESHDS_SubMesh* sm = _sdVec[i]._proxyMesh->GetProxySubMesh( fExp.Current() ))
      {
        if ( sm->NbElements() == 0 ) continue;
        SMDS_ElemIteratorPtr fIt = sm->GetElements();
        while ( fIt->more())
        {
          const SMDS_MeshElement* e = fIt->next();
          SMESH_Comment cmd("mesh.AddFace([");
          for ( int j = 0; j < e->NbCornerNodes(); ++j )
            cmd << e->GetNode(j)->GetID() << (j+1 < e->NbCornerNodes() ? ",": "])");
          dumpCmd( cmd );
        }
      }
    }
    dumpCmd( "faceId2 = mesh.NbElements()" );
    dumpCmd( SMESH_Comment( "mesh.MakeGroup( 'tmpFaces_" ) << i << "',"
             << "SMESH.FACE, SMESH.FT_RangeOfIds,'=',"
             << "'%s-%s' % (faceId1+1, faceId2))");
    dumpFunctionEnd();
  }
#endif
}

//================================================================================
/*!
 * \brief Find maximal _LayerEdge length (layer thickness) limited by geometry
 */
//================================================================================

void _ViscousBuilder::computeGeomSize( _SolidData& data )
{
  data._geomSize = Precision::Infinite();
  double intersecDist;
  const SMDS_MeshElement* face;
  SMESH_MesherHelper helper( *_mesh );

  SMESHUtils::Deleter<SMESH_ElementSearcher> searcher
    ( SMESH_MeshAlgos::GetElementSearcher( *getMeshDS(),
                                           data._proxyMesh->GetFaces( data._solid )));

  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[ iS ];
    if ( eos._edges.empty() )
      continue;
    // get neighbor faces intersection with which should not be considered since
    // collisions are avoided by means of smoothing
    set< TGeomID > neighborFaces;
    if ( eos._hyp.ToSmooth() )
    {
      SMESH_subMeshIteratorPtr subIt =
        eos._subMesh->getDependsOnIterator(/*includeSelf=*/eos.ShapeType() != TopAbs_FACE );
      while ( subIt->more() )
      {
        SMESH_subMesh* sm = subIt->next();
        PShapeIteratorPtr fIt = helper.GetAncestors( sm->GetSubShape(), *_mesh, TopAbs_FACE );
        while ( const TopoDS_Shape* face = fIt->next() )
          neighborFaces.insert( getMeshDS()->ShapeToIndex( *face ));
      }
    }
    // find intersections
    double thinkness = eos._hyp.GetTotalThickness();
    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      if ( eos._edges[i]->Is( _LayerEdge::BLOCKED )) continue;
      eos._edges[i]->_maxLen = thinkness;
      eos._edges[i]->FindIntersection( *searcher, intersecDist, data._epsilon, eos, &face );
      if ( intersecDist > 0 && face )
      {
        data._geomSize = Min( data._geomSize, intersecDist );
        if ( !neighborFaces.count( face->getshapeId() ))
          eos._edges[i]->_maxLen = Min( thinkness, intersecDist / ( face->GetID() < 0 ? 3. : 2. ));
      }
    }
  }
}

//================================================================================
/*!
 * \brief Increase length of _LayerEdge's to reach the required thickness of layers
 */
//================================================================================

bool _ViscousBuilder::inflate(_SolidData& data)
{
  SMESH_MesherHelper helper( *_mesh );

  // Limit inflation step size by geometry size found by itersecting
  // normals of _LayerEdge's with mesh faces
  if ( data._stepSize > 0.3 * data._geomSize )
    limitStepSize( data, 0.3 * data._geomSize );

  const double tgtThick = data._maxThickness;
  if ( data._stepSize > data._minThickness )
    limitStepSize( data, data._minThickness );

  if ( data._stepSize < 1. )
    data._epsilon = data._stepSize * 1e-7;

  debugMsg( "-- geomSize = " << data._geomSize << ", stepSize = " << data._stepSize );

  findCollisionEdges( data, helper );

  limitMaxLenByCurvature( data, helper );

  // limit length of _LayerEdge's around MULTI_NORMAL _LayerEdge's
  for ( size_t i = 0; i < data._edgesOnShape.size(); ++i )
    if ( data._edgesOnShape[i].ShapeType() == TopAbs_VERTEX &&
         data._edgesOnShape[i]._edges.size() > 0 &&
         data._edgesOnShape[i]._edges[0]->Is( _LayerEdge::MULTI_NORMAL ))
    {
      data._edgesOnShape[i]._edges[0]->Unset( _LayerEdge::BLOCKED );
      data._edgesOnShape[i]._edges[0]->Block( data );
    }

  const double safeFactor = ( 2*data._maxThickness < data._geomSize ) ? 1 : theThickToIntersection;

  double avgThick = 0, curThick = 0, distToIntersection = Precision::Infinite();
  int nbSteps = 0, nbRepeats = 0;
  while ( avgThick < 0.99 )
  {
    // new target length
    double prevThick = curThick;
    curThick += data._stepSize;
    if ( curThick > tgtThick )
    {
      curThick = tgtThick + tgtThick*( 1.-avgThick ) * nbRepeats;
      nbRepeats++;
    }

    double stepSize = curThick - prevThick;
    updateNormalsOfSmoothed( data, helper, nbSteps, stepSize ); // to ease smoothing

    // Elongate _LayerEdge's
    dumpFunction(SMESH_Comment("inflate")<<data._index<<"_step"<<nbSteps); // debug
    for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
    {
      _EdgesOnShape& eos = data._edgesOnShape[iS];
      if ( eos._edges.empty() ) continue;

      const double shapeCurThick = Min( curThick, eos._hyp.GetTotalThickness() );
      for ( size_t i = 0; i < eos._edges.size(); ++i )
      {
        eos._edges[i]->SetNewLength( shapeCurThick, eos, helper );
      }
    }
    dumpFunctionEnd();

    if ( !updateNormals( data, helper, nbSteps, stepSize )) // to avoid collisions
      return false;

    // Improve and check quality
    if ( !smoothAndCheck( data, nbSteps, distToIntersection ))
    {
      if ( nbSteps > 0 )
      {
#ifdef __NOT_INVALIDATE_BAD_SMOOTH
        debugMsg("NOT INVALIDATED STEP!");
        return error("Smoothing failed", data._index);
#endif
        dumpFunction(SMESH_Comment("invalidate")<<data._index<<"_step"<<nbSteps); // debug
        for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
        {
          _EdgesOnShape& eos = data._edgesOnShape[iS];
          for ( size_t i = 0; i < eos._edges.size(); ++i )
            eos._edges[i]->InvalidateStep( nbSteps+1, eos );
        }
        dumpFunctionEnd();
      }
      break; // no more inflating possible
    }
    nbSteps++;

    // Evaluate achieved thickness
    avgThick = 0;
    int nbActiveEdges = 0;
    for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
    {
      _EdgesOnShape& eos = data._edgesOnShape[iS];
      if ( eos._edges.empty() ) continue;

      const double shapeTgtThick = eos._hyp.GetTotalThickness();
      for ( size_t i = 0; i < eos._edges.size(); ++i )
      {
        if ( eos._edges[i]->_nodes.size() > 1 )
          avgThick    += Min( 1., eos._edges[i]->_len / shapeTgtThick );
        else
          avgThick    += shapeTgtThick;
        nbActiveEdges += ( ! eos._edges[i]->Is( _LayerEdge::BLOCKED ));
      }
    }
    avgThick /= data._n2eMap.size();
    debugMsg( "-- Thickness " << curThick << " ("<< avgThick*100 << "%) reached" );

#ifdef BLOCK_INFLATION
    if ( nbActiveEdges == 0 )
    {
      debugMsg( "-- Stop inflation since all _LayerEdge's BLOCKED " );
      break;
    }
#else
    if ( distToIntersection < tgtThick * avgThick * safeFactor && avgThick < 0.9 )
    {
      debugMsg( "-- Stop inflation since "
                << " distToIntersection( "<<distToIntersection<<" ) < avgThick( "
                << tgtThick * avgThick << " ) * " << safeFactor );
      break;
    }
#endif
    // new step size
    limitStepSize( data, 0.25 * distToIntersection );
    if ( data._stepSizeNodes[0] )
      data._stepSize = data._stepSizeCoeff *
        SMESH_TNodeXYZ(data._stepSizeNodes[0]).Distance(data._stepSizeNodes[1]);

  } // while ( avgThick < 0.99 )

  if ( nbSteps == 0 )
    return error("failed at the very first inflation step", data._index);

  if ( avgThick < 0.99 )
  {
    if ( !data._proxyMesh->_warning || data._proxyMesh->_warning->IsOK() )
    {
      data._proxyMesh->_warning.reset
        ( new SMESH_ComputeError (COMPERR_WARNING,
                                  SMESH_Comment("Thickness ") << tgtThick <<
                                  " of viscous layers not reached,"
                                  " average reached thickness is " << avgThick*tgtThick));
    }
  }

  // Restore position of src nodes moved by inflation on _noShrinkShapes
  dumpFunction(SMESH_Comment("restoNoShrink_So")<<data._index); // debug
  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[iS];
    if ( !eos._edges.empty() && eos._edges[0]->_nodes.size() == 1 )
      for ( size_t i = 0; i < eos._edges.size(); ++i )
      {
        restoreNoShrink( *eos._edges[ i ] );
      }
  }
  dumpFunctionEnd();

  return safeFactor > 0; // == true (avoid warning: unused variable 'safeFactor')
}

//================================================================================
/*!
 * \brief Improve quality of layer inner surface and check intersection
 */
//================================================================================

bool _ViscousBuilder::smoothAndCheck(_SolidData& data,
                                     const int   infStep,
                                     double &    distToIntersection)
{
  if ( data._nbShapesToSmooth == 0 )
    return true; // no shapes needing smoothing

  bool moved, improved;
  double vol;
  vector< _LayerEdge* >    movedEdges, badEdges;
  vector< _EdgesOnShape* > eosC1; // C1 continues shapes
  vector< bool >           isConcaveFace;

  SMESH_MesherHelper helper(*_mesh);
  Handle(ShapeAnalysis_Surface) surface;
  TopoDS_Face F;

  for ( int isFace = 0; isFace < 2; ++isFace ) // smooth on [ EDGEs, FACEs ]
  {
    const TopAbs_ShapeEnum shapeType = isFace ? TopAbs_FACE : TopAbs_EDGE;

    for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
    {
      _EdgesOnShape& eos = data._edgesOnShape[ iS ];
      if ( !eos._toSmooth ||
           eos.ShapeType() != shapeType ||
           eos._edges.empty() )
        continue;

      // already smoothed?
      // bool toSmooth = ( eos._edges[ 0 ]->NbSteps() >= infStep+1 );
      // if ( !toSmooth ) continue;

      if ( !eos._hyp.ToSmooth() )
      {
        // smooth disabled by the user; check validy only
        if ( !isFace ) continue;
        badEdges.clear();
        for ( size_t i = 0; i < eos._edges.size(); ++i )
        {
          _LayerEdge* edge = eos._edges[i];
          for ( size_t iF = 0; iF < edge->_simplices.size(); ++iF )
            if ( !edge->_simplices[iF].IsForward( edge->_nodes[0], edge->_pos.back(), vol ))
            {
              // debugMsg( "-- Stop inflation. Bad simplex ("
              //           << " "<< edge->_nodes[0]->GetID()
              //           << " "<< edge->_nodes.back()->GetID()
              //           << " "<< edge->_simplices[iF]._nPrev->GetID()
              //           << " "<< edge->_simplices[iF]._nNext->GetID() << " ) ");
              // return false;
              badEdges.push_back( edge );
            }
        }
        if ( !badEdges.empty() )
        {
          eosC1.resize(1);
          eosC1[0] = &eos;
          int nbBad = invalidateBadSmooth( data, helper, badEdges, eosC1, infStep );
          if ( nbBad > 0 )
            return false;
        }
        continue; // goto the next EDGE or FACE
      }

      // prepare data
      if ( eos.SWOLType() == TopAbs_FACE )
      {
        if ( !F.IsSame( eos._sWOL )) {
          F = TopoDS::Face( eos._sWOL );
          helper.SetSubShape( F );
          surface = helper.GetSurface( F );
        }
      }
      else
      {
        F.Nullify(); surface.Nullify();
      }
      const TGeomID sInd = eos._shapeID;

      // perform smoothing

      if ( eos.ShapeType() == TopAbs_EDGE )
      {
        dumpFunction(SMESH_Comment("smooth")<<data._index << "_Ed"<<sInd <<"_InfStep"<<infStep);

        if ( !eos._edgeSmoother->Perform( data, surface, F, helper ))
        {
          // smooth on EDGE's (normally we should not get here)
          int step = 0;
          do {
            moved = false;
            for ( size_t i = 0; i < eos._edges.size(); ++i )
            {
              moved |= eos._edges[i]->SmoothOnEdge( surface, F, helper );
            }
            dumpCmd( SMESH_Comment("# end step ")<<step);
          }
          while ( moved && step++ < 5 );
        }
        dumpFunctionEnd();
      }

      else // smooth on FACE
      {
        eosC1.clear();
        eosC1.push_back( & eos );
        eosC1.insert( eosC1.end(), eos._eosC1.begin(), eos._eosC1.end() );

        movedEdges.clear();
        isConcaveFace.resize( eosC1.size() );
        for ( size_t iEOS = 0; iEOS < eosC1.size(); ++iEOS )
        {
          isConcaveFace[ iEOS ] = data._concaveFaces.count( eosC1[ iEOS ]->_shapeID  );
          vector< _LayerEdge* > & edges = eosC1[ iEOS ]->_edges;
          for ( size_t i = 0; i < edges.size(); ++i )
            if ( edges[i]->Is( _LayerEdge::MOVED ) ||
                 edges[i]->Is( _LayerEdge::NEAR_BOUNDARY ))
              movedEdges.push_back( edges[i] );

          makeOffsetSurface( *eosC1[ iEOS ], helper );
        }

        int step = 0, stepLimit = 5, nbBad = 0;
        while (( ++step <= stepLimit ) || improved )
        {
          dumpFunction(SMESH_Comment("smooth")<<data._index<<"_Fa"<<sInd
                       <<"_InfStep"<<infStep<<"_"<<step); // debug
          int oldBadNb = nbBad;
          badEdges.clear();

#ifdef INCREMENTAL_SMOOTH
          bool findBest = false; // ( step == stepLimit );
          for ( size_t i = 0; i < movedEdges.size(); ++i )
          {
            movedEdges[i]->Unset( _LayerEdge::SMOOTHED );
            if ( movedEdges[i]->Smooth( step, findBest, movedEdges ) > 0 )
              badEdges.push_back( movedEdges[i] );
          }
#else
          bool findBest = ( step == stepLimit || isConcaveFace[ iEOS ]);
          for ( size_t iEOS = 0; iEOS < eosC1.size(); ++iEOS )
          {
            vector< _LayerEdge* > & edges = eosC1[ iEOS ]->_edges;
            for ( size_t i = 0; i < edges.size(); ++i )
            {
              edges[i]->Unset( _LayerEdge::SMOOTHED );
              if ( edges[i]->Smooth( step, findBest, false ) > 0 )
                badEdges.push_back( eos._edges[i] );
            }
          }
#endif
          nbBad = badEdges.size();

          if ( nbBad > 0 )
            debugMsg(SMESH_Comment("nbBad = ") << nbBad );

          if ( !badEdges.empty() && step >= stepLimit / 2 )
          {
            if ( badEdges[0]->Is( _LayerEdge::ON_CONCAVE_FACE ))
              stepLimit = 9;

            // resolve hard smoothing situation around concave VERTEXes
            for ( size_t iEOS = 0; iEOS < eosC1.size(); ++iEOS )
            {
              vector< _EdgesOnShape* > & eosCoVe = eosC1[ iEOS ]->_eosConcaVer;
              for ( size_t i = 0; i < eosCoVe.size(); ++i )
                eosCoVe[i]->_edges[0]->MoveNearConcaVer( eosCoVe[i], eosC1[ iEOS ],
                                                         step, badEdges );
            }
            // look for the best smooth of _LayerEdge's neighboring badEdges
            nbBad = 0;
            for ( size_t i = 0; i < badEdges.size(); ++i )
            {
              _LayerEdge* ledge = badEdges[i];
              for ( size_t iN = 0; iN < ledge->_neibors.size(); ++iN )
              {
                ledge->_neibors[iN]->Unset( _LayerEdge::SMOOTHED );
                nbBad += ledge->_neibors[iN]->Smooth( step, true, /*findBest=*/true );
              }
              ledge->Unset( _LayerEdge::SMOOTHED );
              nbBad += ledge->Smooth( step, true, /*findBest=*/true );
            }
            debugMsg(SMESH_Comment("nbBad = ") << nbBad );
          }

          if ( nbBad == oldBadNb  &&
               nbBad > 0 &&
               step < stepLimit ) // smooth w/o chech of validity
          {
            dumpFunctionEnd();
            dumpFunction(SMESH_Comment("smoothWoCheck")<<data._index<<"_Fa"<<sInd
                         <<"_InfStep"<<infStep<<"_"<<step); // debug
            for ( size_t i = 0; i < movedEdges.size(); ++i )
            {
              movedEdges[i]->SmoothWoCheck();
            }
            if ( stepLimit < 9 )
              stepLimit++;
          }

          improved = ( nbBad < oldBadNb );

          dumpFunctionEnd();

          if (( step % 3 == 1 ) || ( nbBad > 0 && step >= stepLimit / 2 ))
            for ( size_t iEOS = 0; iEOS < eosC1.size(); ++iEOS )
            {
              putOnOffsetSurface( *eosC1[ iEOS ], infStep, eosC1, step, /*moveAll=*/step == 1 );
            }

        } // smoothing steps

        // project -- to prevent intersections or fix bad simplices
        for ( size_t iEOS = 0; iEOS < eosC1.size(); ++iEOS )
        {
          if ( ! eosC1[ iEOS ]->_eosConcaVer.empty() || nbBad > 0 )
            putOnOffsetSurface( *eosC1[ iEOS ], infStep, eosC1 );
        }

        //if ( !badEdges.empty() )
        {
          badEdges.clear();
          for ( size_t iEOS = 0; iEOS < eosC1.size(); ++iEOS )
          {
            for ( size_t i = 0; i < eosC1[ iEOS ]->_edges.size(); ++i )
            {
              if ( !eosC1[ iEOS ]->_sWOL.IsNull() ) continue;

              _LayerEdge* edge = eosC1[ iEOS ]->_edges[i];
              edge->CheckNeiborsOnBoundary( & badEdges );
              if (( nbBad > 0 ) ||
                  ( edge->Is( _LayerEdge::BLOCKED ) && edge->Is( _LayerEdge::NEAR_BOUNDARY )))
              {
                SMESH_TNodeXYZ tgtXYZ = edge->_nodes.back();
                gp_XYZ        prevXYZ = edge->PrevCheckPos();
                for ( size_t j = 0; j < edge->_simplices.size(); ++j )
                  if ( !edge->_simplices[j].IsForward( &prevXYZ, &tgtXYZ, vol ))
                  {
                    debugMsg("Bad simplex ( " << edge->_nodes[0]->GetID()
                             << " "<< tgtXYZ._node->GetID()
                             << " "<< edge->_simplices[j]._nPrev->GetID()
                             << " "<< edge->_simplices[j]._nNext->GetID() << " )" );
                    badEdges.push_back( edge );
                    break;
                  }
              }
            }
          }

          // try to fix bad simplices by removing the last inflation step of some _LayerEdge's
          nbBad = invalidateBadSmooth( data, helper, badEdges, eosC1, infStep );

          if ( nbBad > 0 )
            return false;
        }

      } // // smooth on FACE's
    } // loop on shapes
  } // smooth on [ EDGEs, FACEs ]

  // Check orientation of simplices of _LayerEdge's on EDGEs and VERTEXes
  eosC1.resize(1);
  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[ iS ];
    if ( eos.ShapeType() == TopAbs_FACE ||
         eos._edges.empty() ||
         !eos._sWOL.IsNull() )
      continue;

    badEdges.clear();
    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      _LayerEdge*      edge = eos._edges[i];
      if ( edge->_nodes.size() < 2 ) continue;
      SMESH_TNodeXYZ tgtXYZ = edge->_nodes.back();
      gp_XYZ        prevXYZ = edge->PrevCheckPos( &eos );
      //const gp_XYZ& prevXYZ = edge->PrevPos();
      for ( size_t j = 0; j < edge->_simplices.size(); ++j )
        if ( !edge->_simplices[j].IsForward( &prevXYZ, &tgtXYZ, vol ))
        {
          debugMsg("Bad simplex on bnd ( " << edge->_nodes[0]->GetID()
                   << " "<< tgtXYZ._node->GetID()
                   << " "<< edge->_simplices[j]._nPrev->GetID()
                   << " "<< edge->_simplices[j]._nNext->GetID() << " )" );
          badEdges.push_back( edge );
          break;
        }
    }

    // try to fix bad simplices by removing the last inflation step of some _LayerEdge's
    eosC1[0] = &eos;
    int nbBad = invalidateBadSmooth( data, helper, badEdges, eosC1, infStep );
    if ( nbBad > 0 )
      return false;
  }


  // Check if the last segments of _LayerEdge intersects 2D elements;
  // checked elements are either temporary faces or faces on surfaces w/o the layers

  SMESHUtils::Deleter<SMESH_ElementSearcher> searcher
    ( SMESH_MeshAlgos::GetElementSearcher( *getMeshDS(),
                                           data._proxyMesh->GetFaces( data._solid )) );

#ifdef BLOCK_INFLATION
  const bool toBlockInfaltion = true;
#else
  const bool toBlockInfaltion = false;
#endif
  distToIntersection = Precision::Infinite();
  double dist;
  const SMDS_MeshElement* intFace = 0;
  const SMDS_MeshElement* closestFace = 0;
  _LayerEdge* le = 0;
  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[ iS ];
    if ( eos._edges.empty() || !eos._sWOL.IsNull() )
      continue;
    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      if ( eos._edges[i]->Is( _LayerEdge::INTERSECTED ) ||
           eos._edges[i]->Is( _LayerEdge::MULTI_NORMAL ))
        continue;
      if ( eos._edges[i]->FindIntersection( *searcher, dist, data._epsilon, eos, &intFace ))
      {
        return false;
        // commented due to "Illegal hash-positionPosition" error in NETGEN
        // on Debian60 on viscous_layers_01/B2 case
        // Collision; try to deflate _LayerEdge's causing it
        // badEdges.clear();
        // badEdges.push_back( eos._edges[i] );
        // eosC1[0] = & eos;
        // int nbBad = invalidateBadSmooth( data, helper, badEdges, eosC1, infStep );
        // if ( nbBad > 0 )
        //   return false;

        // badEdges.clear();
        // if ( _EdgesOnShape* eof = data.GetShapeEdges( intFace->getshapeId() ))
        // {
        //   if ( const _TmpMeshFace* f = dynamic_cast< const _TmpMeshFace*>( intFace ))
        //   {
        //     const SMDS_MeshElement* srcFace =
        //       eof->_subMesh->GetSubMeshDS()->GetElement( f->getIdInShape() );
        //     SMDS_ElemIteratorPtr nIt = srcFace->nodesIterator();
        //     while ( nIt->more() )
        //     {
        //       const SMDS_MeshNode* srcNode = static_cast<const SMDS_MeshNode*>( nIt->next() );
        //       TNode2Edge::iterator n2e = data._n2eMap.find( srcNode );
        //       if ( n2e != data._n2eMap.end() )
        //         badEdges.push_back( n2e->second );
        //     }
        //     eosC1[0] = eof;
        //     nbBad = invalidateBadSmooth( data, helper, badEdges, eosC1, infStep );
        //     if ( nbBad > 0 )
        //       return false;
        //   }
        // }
        // if ( eos._edges[i]->FindIntersection( *searcher, dist, data._epsilon, eos, &intFace ))
        //   return false;
        // else
        //   continue;
      }
      if ( !intFace )
      {
        SMESH_Comment msg("Invalid? normal at node "); msg << eos._edges[i]->_nodes[0]->GetID();
        debugMsg( msg );
        continue;
      }

      const bool isShorterDist = ( distToIntersection > dist );
      if ( toBlockInfaltion || isShorterDist )
      {
        // ignore intersection of a _LayerEdge based on a _ConvexFace with a face
        // lying on this _ConvexFace
        if ( _ConvexFace* convFace = data.GetConvexFace( intFace->getshapeId() ))
          if ( convFace->_subIdToEOS.count ( eos._shapeID ))
            continue;

        // ignore intersection of a _LayerEdge based on a FACE with an element on this FACE
        // ( avoid limiting the thickness on the case of issue 22576)
        if ( intFace->getshapeId() == eos._shapeID  )
          continue;

        // ignore intersection with intFace of an adjacent FACE
        if ( dist > 0 )
        {
          bool toIgnore = false;
          if (  eos._edges[i]->Is( _LayerEdge::TO_SMOOTH ))
          {
            const TopoDS_Shape& S = getMeshDS()->IndexToShape( intFace->getshapeId() );
            if ( !S.IsNull() && S.ShapeType() == TopAbs_FACE )
            {
              TopExp_Explorer edge( eos._shape, TopAbs_EDGE );
              for ( ; !toIgnore && edge.More(); edge.Next() )
                // is adjacent - has a common EDGE
                toIgnore = ( helper.IsSubShape( edge.Current(), S ));

              if ( toIgnore ) // check angle between normals
              {
                gp_XYZ normal;
                if ( SMESH_MeshAlgos::FaceNormal( intFace, normal, /*normalized=*/true ))
                  toIgnore  = ( normal * eos._edges[i]->_normal > -0.5 );
              }
            }
          }
          if ( !toIgnore ) // check if the edge is a neighbor of intFace
          {
            for ( size_t iN = 0; !toIgnore &&  iN < eos._edges[i]->_neibors.size(); ++iN )
            {
              int nInd = intFace->GetNodeIndex( eos._edges[i]->_neibors[ iN ]->_nodes.back() );
              toIgnore = ( nInd >= 0 );
            }
          }
          if ( toIgnore )
            continue;
        }

        // intersection not ignored

        if ( toBlockInfaltion &&
             dist < ( eos._edges[i]->_len * theThickToIntersection ))
        {
          eos._edges[i]->Set( _LayerEdge::INTERSECTED ); // not to intersect
          eos._edges[i]->Block( data );                  // not to inflate

          if ( _EdgesOnShape* eof = data.GetShapeEdges( intFace->getshapeId() ))
          {
            // block _LayerEdge's, on top of which intFace is
            if ( const _TmpMeshFace* f = dynamic_cast< const _TmpMeshFace*>( intFace ))
            {
              const SMDS_MeshElement* srcFace =
                eof->_subMesh->GetSubMeshDS()->GetElement( f->getIdInShape() );
              SMDS_ElemIteratorPtr nIt = srcFace->nodesIterator();
              while ( nIt->more() )
              {
                const SMDS_MeshNode* srcNode = static_cast<const SMDS_MeshNode*>( nIt->next() );
                TNode2Edge::iterator n2e = data._n2eMap.find( srcNode );
                if ( n2e != data._n2eMap.end() )
                  n2e->second->Block( data );
              }
            }
          }
        }

        if ( isShorterDist )
        {
          distToIntersection = dist;
          le = eos._edges[i];
          closestFace = intFace;
        }

      } // if ( toBlockInfaltion || isShorterDist )
    } // loop on eos._edges
  } // loop on data._edgesOnShape

  if ( closestFace && le )
  {
#ifdef __myDEBUG
    SMDS_MeshElement::iterator nIt = closestFace->begin_nodes();
    cout << "Shortest distance: _LayerEdge nodes: tgt " << le->_nodes.back()->GetID()
         << " src " << le->_nodes[0]->GetID()<< ", intersection with face ("
         << (*nIt++)->GetID()<<" "<< (*nIt++)->GetID()<<" "<< (*nIt++)->GetID()
         << ") distance = " << distToIntersection<< endl;
#endif
  }

  return true;
}

//================================================================================
/*!
 * \brief try to fix bad simplices by removing the last inflation step of some _LayerEdge's
 *  \param [in,out] badSmooEdges - _LayerEdge's to fix
 *  \return int - resulting nb of bad _LayerEdge's
 */
//================================================================================

int _ViscousBuilder::invalidateBadSmooth( _SolidData&               data,
                                          SMESH_MesherHelper&       helper,
                                          vector< _LayerEdge* >&    badSmooEdges,
                                          vector< _EdgesOnShape* >& eosC1,
                                          const int                 infStep )
{
  if ( badSmooEdges.empty() || infStep == 0 ) return 0;

  dumpFunction(SMESH_Comment("invalidateBadSmooth")<<"_S"<<eosC1[0]->_shapeID<<"_InfStep"<<infStep);

  enum {
    INVALIDATED   = _LayerEdge::UNUSED_FLAG,
    TO_INVALIDATE = _LayerEdge::UNUSED_FLAG * 2,
    ADDED         = _LayerEdge::UNUSED_FLAG * 4
  };
  data.UnmarkEdges( TO_INVALIDATE & INVALIDATED & ADDED );

  double vol;
  bool haveInvalidated = true;
  while ( haveInvalidated )
  {
    haveInvalidated = false;
    for ( size_t i = 0; i < badSmooEdges.size(); ++i )
    {
      _LayerEdge*   edge = badSmooEdges[i];
      _EdgesOnShape* eos = data.GetShapeEdges( edge );
      edge->Set( ADDED );
      bool invalidated = false;
      if ( edge->Is( TO_INVALIDATE ) && edge->NbSteps() > 1 )
      {
        edge->InvalidateStep( edge->NbSteps(), *eos, /*restoreLength=*/true );
        edge->Block( data );
        edge->Set( INVALIDATED );
        edge->Unset( TO_INVALIDATE );
        invalidated = true;
        haveInvalidated = true;
      }

      // look for _LayerEdge's of bad _simplices
      int nbBad = 0;
      SMESH_TNodeXYZ tgtXYZ  = edge->_nodes.back();
      gp_XYZ        prevXYZ1 = edge->PrevCheckPos( eos );
      //const gp_XYZ& prevXYZ2 = edge->PrevPos();
      for ( size_t j = 0; j < edge->_simplices.size(); ++j )
      {
        if (( edge->_simplices[j].IsForward( &prevXYZ1, &tgtXYZ, vol ))/* &&
            ( &prevXYZ1 == &prevXYZ2 || edge->_simplices[j].IsForward( &prevXYZ2, &tgtXYZ, vol ))*/)
          continue;

        bool isBad = true;
        _LayerEdge* ee[2] = { 0,0 };
        for ( size_t iN = 0; iN < edge->_neibors.size() &&   !ee[1]  ; ++iN )
          if ( edge->_simplices[j].Includes( edge->_neibors[iN]->_nodes.back() ))
            ee[ ee[0] != 0 ] = edge->_neibors[iN];

        int maxNbSteps = Max( ee[0]->NbSteps(), ee[1]->NbSteps() );
        while ( maxNbSteps > edge->NbSteps() && isBad )
        {
          --maxNbSteps;
          for ( int iE = 0; iE < 2; ++iE )
          {
            if ( ee[ iE ]->NbSteps() > maxNbSteps &&
                 ee[ iE ]->NbSteps() > 1 )
            {
              _EdgesOnShape* eos = data.GetShapeEdges( ee[ iE ] );
              ee[ iE ]->InvalidateStep( ee[ iE ]->NbSteps(), *eos, /*restoreLength=*/true );
              ee[ iE ]->Block( data );
              ee[ iE ]->Set( INVALIDATED );
              haveInvalidated = true;
            }
          }
          if (( edge->_simplices[j].IsForward( &prevXYZ1, &tgtXYZ, vol )) /*&&
              ( &prevXYZ1 == &prevXYZ2 || edge->_simplices[j].IsForward( &prevXYZ2, &tgtXYZ, vol ))*/)
            isBad = false;
        }
        nbBad += isBad;
        if ( !ee[0]->Is( ADDED )) badSmooEdges.push_back( ee[0] );
        if ( !ee[1]->Is( ADDED )) badSmooEdges.push_back( ee[1] );
        ee[0]->Set( ADDED );
        ee[1]->Set( ADDED );
        if ( isBad )
        {
          ee[0]->Set( TO_INVALIDATE );
          ee[1]->Set( TO_INVALIDATE );
        }
      }

      if ( !invalidated &&  nbBad > 0  &&  edge->NbSteps() > 1 )
      {
        _EdgesOnShape* eos = data.GetShapeEdges( edge );
        edge->InvalidateStep( edge->NbSteps(), *eos, /*restoreLength=*/true );
        edge->Block( data );
        edge->Set( INVALIDATED );
        edge->Unset( TO_INVALIDATE );
        haveInvalidated = true;
      }
    } // loop on badSmooEdges
  } // while ( haveInvalidated )

  // re-smooth on analytical EDGEs
  for ( size_t i = 0; i < badSmooEdges.size(); ++i )
  {
    _LayerEdge* edge = badSmooEdges[i];
    if ( !edge->Is( INVALIDATED )) continue;

    _EdgesOnShape* eos = data.GetShapeEdges( edge );
    if ( eos->ShapeType() == TopAbs_VERTEX )
    {
      PShapeIteratorPtr eIt = helper.GetAncestors( eos->_shape, *_mesh, TopAbs_EDGE );
      while ( const TopoDS_Shape* e = eIt->next() )
        if ( _EdgesOnShape* eoe = data.GetShapeEdges( *e ))
          if ( eoe->_edgeSmoother && eoe->_edgeSmoother->isAnalytic() )
          {
            // TopoDS_Face F; Handle(ShapeAnalysis_Surface) surface;
            // if ( eoe->SWOLType() == TopAbs_FACE ) {
            //   F       = TopoDS::Face( eoe->_sWOL );
            //   surface = helper.GetSurface( F );
            // }
            // eoe->_edgeSmoother->Perform( data, surface, F, helper );
            eoe->_edgeSmoother->_anaCurve.Nullify();
          }
    }
  }


  // check result of invalidation

  int nbBad = 0;
  for ( size_t iEOS = 0; iEOS < eosC1.size(); ++iEOS )
  {
    for ( size_t i = 0; i < eosC1[ iEOS ]->_edges.size(); ++i )
    {
      if ( !eosC1[ iEOS ]->_sWOL.IsNull() ) continue;
      _LayerEdge*      edge = eosC1[ iEOS ]->_edges[i];
      SMESH_TNodeXYZ tgtXYZ = edge->_nodes.back();
      gp_XYZ        prevXYZ = edge->PrevCheckPos( eosC1[ iEOS ]);
      for ( size_t j = 0; j < edge->_simplices.size(); ++j )
        if ( !edge->_simplices[j].IsForward( &prevXYZ, &tgtXYZ, vol ))
        {
          ++nbBad;
          debugMsg("Bad simplex remains ( " << edge->_nodes[0]->GetID()
                   << " "<< tgtXYZ._node->GetID()
                   << " "<< edge->_simplices[j]._nPrev->GetID()
                   << " "<< edge->_simplices[j]._nNext->GetID() << " )" );
        }
    }
  }
  dumpFunctionEnd();

  return nbBad;
}

//================================================================================
/*!
 * \brief Create an offset surface
 */
//================================================================================

void _ViscousBuilder::makeOffsetSurface( _EdgesOnShape& eos, SMESH_MesherHelper& helper )
{
  if ( eos._offsetSurf.IsNull() ||
       eos._edgeForOffset == 0 ||
       eos._edgeForOffset->Is( _LayerEdge::BLOCKED ))
    return;

  Handle(ShapeAnalysis_Surface) baseSurface = helper.GetSurface( TopoDS::Face( eos._shape ));

  // find offset
  gp_Pnt   tgtP = SMESH_TNodeXYZ( eos._edgeForOffset->_nodes.back() );
  /*gp_Pnt2d uv=*/baseSurface->ValueOfUV( tgtP, Precision::Confusion() );
  double offset = baseSurface->Gap();

  eos._offsetSurf.Nullify();

  try
  {
    BRepOffsetAPI_MakeOffsetShape offsetMaker( eos._shape, -offset, Precision::Confusion() );
    if ( !offsetMaker.IsDone() ) return;

    TopExp_Explorer fExp( offsetMaker.Shape(), TopAbs_FACE );
    if ( !fExp.More() ) return;

    TopoDS_Face F = TopoDS::Face( fExp.Current() );
    Handle(Geom_Surface) surf = BRep_Tool::Surface( F );
    if ( surf.IsNull() ) return;

    eos._offsetSurf = new ShapeAnalysis_Surface( surf );
  }
  catch ( Standard_Failure )
  {
  }
}

//================================================================================
/*!
 * \brief Put nodes of a curved FACE to its offset surface
 */
//================================================================================

void _ViscousBuilder::putOnOffsetSurface( _EdgesOnShape&            eos,
                                          int                       infStep,
                                          vector< _EdgesOnShape* >& eosC1,
                                          int                       smooStep,
                                          bool                      moveAll )
{
  _EdgesOnShape * eof = & eos;
  if ( eos.ShapeType() != TopAbs_FACE ) // eos is a boundary of C1 FACE, look for the FACE eos
  {
    eof = 0;
    for ( size_t i = 0; i < eosC1.size() && !eof; ++i )
    {
      if ( eosC1[i]->_offsetSurf.IsNull() ||
           eosC1[i]->ShapeType() != TopAbs_FACE ||
           eosC1[i]->_edgeForOffset == 0 ||
           eosC1[i]->_edgeForOffset->Is( _LayerEdge::BLOCKED ))
        continue;
      if ( SMESH_MesherHelper::IsSubShape( eos._shape, eosC1[i]->_shape ))
        eof = eosC1[i];
    }
  }
  if ( !eof ||
       eof->_offsetSurf.IsNull() ||
       eof->ShapeType() != TopAbs_FACE ||
       eof->_edgeForOffset == 0 ||
       eof->_edgeForOffset->Is( _LayerEdge::BLOCKED ))
    return;

  double preci = BRep_Tool::Tolerance( TopoDS::Face( eof->_shape )), vol;
  for ( size_t i = 0; i < eos._edges.size(); ++i )
  {
    _LayerEdge* edge = eos._edges[i];
    edge->Unset( _LayerEdge::MARKED );
    if ( edge->Is( _LayerEdge::BLOCKED ) || !edge->_curvature )
      continue;
    if ( !moveAll && !edge->Is( _LayerEdge::MOVED ))
        continue;

    int nbBlockedAround = 0;
    for ( size_t iN = 0; iN < edge->_neibors.size(); ++iN )
      nbBlockedAround += edge->_neibors[iN]->Is( _LayerEdge::BLOCKED );
    if ( nbBlockedAround > 1 )
      continue;

    gp_Pnt tgtP = SMESH_TNodeXYZ( edge->_nodes.back() );
    gp_Pnt2d uv = eof->_offsetSurf->NextValueOfUV( edge->_curvature->_uv, tgtP, preci );
    if ( eof->_offsetSurf->Gap() > edge->_len ) continue; // NextValueOfUV() bug 
    edge->_curvature->_uv = uv;
    if ( eof->_offsetSurf->Gap() < 10 * preci ) continue; // same pos

    gp_XYZ  newP = eof->_offsetSurf->Value( uv ).XYZ();
    gp_XYZ prevP = edge->PrevCheckPos();
    bool      ok = true;
    if ( !moveAll )
      for ( size_t iS = 0; iS < edge->_simplices.size() && ok; ++iS )
      {
        ok = edge->_simplices[iS].IsForward( &prevP, &newP, vol );
      }
    if ( ok )
    {
      SMDS_MeshNode* n = const_cast< SMDS_MeshNode* >( edge->_nodes.back() );
      n->setXYZ( newP.X(), newP.Y(), newP.Z());
      edge->_pos.back() = newP;

      edge->Set( _LayerEdge::MARKED );
    }
  }

#ifdef _DEBUG_
  // dumpMove() for debug
  size_t i = 0;
  for ( ; i < eos._edges.size(); ++i )
    if ( eos._edges[i]->Is( _LayerEdge::MARKED ))
      break;
  if ( i < eos._edges.size() )
  {
    dumpFunction(SMESH_Comment("putOnOffsetSurface_F") << eos._shapeID
                 << "_InfStep" << infStep << "_" << smooStep );
    for ( ; i < eos._edges.size(); ++i )
    {
      if ( eos._edges[i]->Is( _LayerEdge::MARKED ))
        dumpMove( eos._edges[i]->_nodes.back() );
    }
    dumpFunctionEnd();
  }
#endif
}

//================================================================================
/*!
 * \brief Return a curve of the EDGE to be used for smoothing and arrange
 *        _LayerEdge's to be in a consequent order
 */
//================================================================================

Handle(Geom_Curve) _Smoother1D::CurveForSmooth( const TopoDS_Edge&  E,
                                                _EdgesOnShape&      eos,
                                                SMESH_MesherHelper& helper)
{
  SMESHDS_SubMesh* smDS = eos._subMesh->GetSubMeshDS();

  TopLoc_Location loc; double f,l;

  Handle(Geom_Line)   line;
  Handle(Geom_Circle) circle;
  bool isLine, isCirc;
  if ( eos._sWOL.IsNull() ) /////////////////////////////////////////// 3D case
  {
    // check if the EDGE is a line
    Handle(Geom_Curve) curve = BRep_Tool::Curve( E, f, l);
    if ( curve->IsKind( STANDARD_TYPE( Geom_TrimmedCurve )))
      curve = Handle(Geom_TrimmedCurve)::DownCast( curve )->BasisCurve();

    line   = Handle(Geom_Line)::DownCast( curve );
    circle = Handle(Geom_Circle)::DownCast( curve );
    isLine = (!line.IsNull());
    isCirc = (!circle.IsNull());

    if ( !isLine && !isCirc ) // Check if the EDGE is close to a line
    {
      isLine = SMESH_Algo::IsStraight( E );

      if ( isLine )
        line = new Geom_Line( gp::OX() ); // only type does matter
    }
    if ( !isLine && !isCirc && eos._edges.size() > 2) // Check if the EDGE is close to a circle
    {
      // TODO
    }
  }
  else //////////////////////////////////////////////////////////////////////// 2D case
  {
    if ( !eos._isRegularSWOL ) // 23190
      return NULL;

    const TopoDS_Face& F = TopoDS::Face( eos._sWOL );

    // check if the EDGE is a line
    Handle(Geom2d_Curve) curve = BRep_Tool::CurveOnSurface( E, F, f, l );
    if ( curve->IsKind( STANDARD_TYPE( Geom2d_TrimmedCurve )))
      curve = Handle(Geom2d_TrimmedCurve)::DownCast( curve )->BasisCurve();

    Handle(Geom2d_Line)   line2d   = Handle(Geom2d_Line)::DownCast( curve );
    Handle(Geom2d_Circle) circle2d = Handle(Geom2d_Circle)::DownCast( curve );
    isLine = (!line2d.IsNull());
    isCirc = (!circle2d.IsNull());

    if ( !isLine && !isCirc ) // Check if the EDGE is close to a line
    {
      Bnd_B2d bndBox;
      SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
      while ( nIt->more() )
        bndBox.Add( helper.GetNodeUV( F, nIt->next() ));
      gp_XY size = bndBox.CornerMax() - bndBox.CornerMin();

      const double lineTol = 1e-2 * sqrt( bndBox.SquareExtent() );
      for ( int i = 0; i < 2 && !isLine; ++i )
        isLine = ( size.Coord( i+1 ) <= lineTol );
    }
    if ( !isLine && !isCirc && eos._edges.size() > 2 ) // Check if the EDGE is close to a circle
    {
      // TODO
    }
    if ( isLine )
    {
      line = new Geom_Line( gp::OX() ); // only type does matter
    }
    else if ( isCirc )
    {
      gp_Pnt2d p = circle2d->Location();
      gp_Ax2 ax( gp_Pnt( p.X(), p.Y(), 0), gp::DX());
      circle = new Geom_Circle( ax, 1.); // only center position does matter
    }
  }

  if ( isLine )
    return line;
  if ( isCirc )
    return circle;

  return Handle(Geom_Curve)();
}

//================================================================================
/*!
 * \brief smooth _LayerEdge's on a staight EDGE or circular EDGE
 */
//================================================================================

bool _Smoother1D::smoothAnalyticEdge( _SolidData&                    data,
                                      Handle(ShapeAnalysis_Surface)& surface,
                                      const TopoDS_Face&             F,
                                      SMESH_MesherHelper&            helper)
{
  if ( !isAnalytic() ) return false;

  const size_t iFrom = 0, iTo = _eos._edges.size();

  if ( _anaCurve->IsKind( STANDARD_TYPE( Geom_Line )))
  {
    if ( F.IsNull() ) // 3D
    {
      SMESH_TNodeXYZ p0   ( _eos._edges[iFrom]->_2neibors->tgtNode(0) );
      SMESH_TNodeXYZ p1   ( _eos._edges[iTo-1]->_2neibors->tgtNode(1) );
      SMESH_TNodeXYZ pSrc0( _eos._edges[iFrom]->_2neibors->srcNode(0) );
      SMESH_TNodeXYZ pSrc1( _eos._edges[iTo-1]->_2neibors->srcNode(1) );
      gp_XYZ newPos, lineDir = pSrc1 - pSrc0;
      _LayerEdge* vLE0 = _eos._edges[iFrom]->_2neibors->_edges[0];
      _LayerEdge* vLE1 = _eos._edges[iTo-1]->_2neibors->_edges[1];
      bool shiftOnly = ( vLE0->Is( _LayerEdge::NORMAL_UPDATED ) ||
                         vLE0->Is( _LayerEdge::BLOCKED ) ||
                         vLE1->Is( _LayerEdge::NORMAL_UPDATED ) ||
                         vLE1->Is( _LayerEdge::BLOCKED ));
      for ( size_t i = iFrom; i < iTo; ++i )
      {
        _LayerEdge*       edge = _eos._edges[i];
        SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( edge->_nodes.back() );
        newPos = p0 * ( 1. - _leParams[i] ) + p1 * _leParams[i];

        if ( shiftOnly || edge->Is( _LayerEdge::NORMAL_UPDATED ))
        {
          gp_XYZ curPos = SMESH_TNodeXYZ ( tgtNode );
          double  shift = ( lineDir * ( newPos - pSrc0 ) -
                            lineDir * ( curPos - pSrc0 ));
          newPos = curPos + lineDir * shift / lineDir.SquareModulus();
        }
        if ( edge->Is( _LayerEdge::BLOCKED ))
        {
          SMESH_TNodeXYZ pSrc( edge->_nodes[0] );
          double curThick = pSrc.SquareDistance( tgtNode );
          double newThink = ( pSrc - newPos ).SquareModulus();
          if ( newThink > curThick )
            continue;
        }
        edge->_pos.back() = newPos;
        tgtNode->setXYZ( newPos.X(), newPos.Y(), newPos.Z() );
        dumpMove( tgtNode );
      }
    }
    else // 2D
    {
      _LayerEdge* e0 = getLEdgeOnV( 0 );
      _LayerEdge* e1 = getLEdgeOnV( 1 );
      gp_XY uv0 = e0->LastUV( F, *data.GetShapeEdges( e0 ));
      gp_XY uv1 = e1->LastUV( F, *data.GetShapeEdges( e1 ));
      if ( e0->_nodes.back() == e1->_nodes.back() ) // closed edge
      {
        int iPeriodic = helper.GetPeriodicIndex();
        if ( iPeriodic == 1 || iPeriodic == 2 )
        {
          uv1.SetCoord( iPeriodic, helper.GetOtherParam( uv1.Coord( iPeriodic )));
          if ( uv0.Coord( iPeriodic ) > uv1.Coord( iPeriodic ))
            std::swap( uv0, uv1 );
        }
      }
      const gp_XY rangeUV = uv1 - uv0;
      for ( size_t i = iFrom; i < iTo; ++i )
      {
        if ( _eos._edges[i]->Is( _LayerEdge::BLOCKED )) continue;
        gp_XY newUV = uv0 + _leParams[i] * rangeUV;
        _eos._edges[i]->_pos.back().SetCoord( newUV.X(), newUV.Y(), 0 );

        gp_Pnt newPos = surface->Value( newUV.X(), newUV.Y() );
        SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( _eos._edges[i]->_nodes.back() );
        tgtNode->setXYZ( newPos.X(), newPos.Y(), newPos.Z() );
        dumpMove( tgtNode );

        SMDS_FacePosition* pos = static_cast<SMDS_FacePosition*>( tgtNode->GetPosition() );
        pos->SetUParameter( newUV.X() );
        pos->SetVParameter( newUV.Y() );
      }
    }
    return true;
  }

  if ( _anaCurve->IsKind( STANDARD_TYPE( Geom_Circle )))
  {
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast( _anaCurve );
    gp_Pnt center3D = circle->Location();

    if ( F.IsNull() ) // 3D
    {
      if ( getLEdgeOnV( 0 )->_nodes.back() == getLEdgeOnV( 1 )->_nodes.back() )
        return true; // closed EDGE - nothing to do

      // circle is a real curve of EDGE
      gp_Circ circ = circle->Circ();

      // new center is shifted along its axis
      const gp_Dir& axis = circ.Axis().Direction();
      _LayerEdge*     e0 = getLEdgeOnV(0);
      _LayerEdge*     e1 = getLEdgeOnV(1);
      SMESH_TNodeXYZ  p0 = e0->_nodes.back();
      SMESH_TNodeXYZ  p1 = e1->_nodes.back();
      double      shift1 = axis.XYZ() * ( p0 - center3D.XYZ() );
      double      shift2 = axis.XYZ() * ( p1 - center3D.XYZ() );
      gp_Pnt   newCenter = center3D.XYZ() + axis.XYZ() * 0.5 * ( shift1 + shift2 );

      double newRadius = 0.5 * ( newCenter.Distance( p0 ) + newCenter.Distance( p1 ));

      gp_Ax2  newAxis( newCenter, axis, gp_Vec( newCenter, p0 ));
      gp_Circ newCirc( newAxis, newRadius );
      gp_Vec  vecC1  ( newCenter, p1 );

      double uLast = newAxis.XDirection().AngleWithRef( vecC1, newAxis.Direction() ); // -PI - +PI
      if ( uLast < 0 )
        uLast += 2 * M_PI;
      
      for ( size_t i = iFrom; i < iTo; ++i )
      {
        if ( _eos._edges[i]->Is( _LayerEdge::BLOCKED )) continue;
        double u = uLast * _leParams[i];
        gp_Pnt p = ElCLib::Value( u, newCirc );
        _eos._edges[i]->_pos.back() = p.XYZ();

        SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( _eos._edges[i]->_nodes.back() );
        tgtNode->setXYZ( p.X(), p.Y(), p.Z() );
        dumpMove( tgtNode );
      }
      return true;
    }
    else // 2D
    {
      const gp_XY center( center3D.X(), center3D.Y() );

      _LayerEdge* e0 = getLEdgeOnV(0);
      _LayerEdge* eM = _eos._edges[ 0 ];
      _LayerEdge* e1 = getLEdgeOnV(1);
      gp_XY      uv0 = e0->LastUV( F, *data.GetShapeEdges( e0 ) );
      gp_XY      uvM = eM->LastUV( F, *data.GetShapeEdges( eM ) );
      gp_XY      uv1 = e1->LastUV( F, *data.GetShapeEdges( e1 ) );
      gp_Vec2d vec0( center, uv0 );
      gp_Vec2d vecM( center, uvM );
      gp_Vec2d vec1( center, uv1 );
      double uLast = vec0.Angle( vec1 ); // -PI - +PI
      double uMidl = vec0.Angle( vecM );
      if ( uLast * uMidl <= 0. )
        uLast += ( uMidl > 0 ? +2. : -2. ) * M_PI;
      const double radius = 0.5 * ( vec0.Magnitude() + vec1.Magnitude() );

      gp_Ax2d   axis( center, vec0 );
      gp_Circ2d circ( axis, radius );
      for ( size_t i = iFrom; i < iTo; ++i )
      {
        if ( _eos._edges[i]->Is( _LayerEdge::BLOCKED )) continue;
        double    newU = uLast * _leParams[i];
        gp_Pnt2d newUV = ElCLib::Value( newU, circ );
        _eos._edges[i]->_pos.back().SetCoord( newUV.X(), newUV.Y(), 0 );

        gp_Pnt newPos = surface->Value( newUV.X(), newUV.Y() );
        SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( _eos._edges[i]->_nodes.back() );
        tgtNode->setXYZ( newPos.X(), newPos.Y(), newPos.Z() );
        dumpMove( tgtNode );

        SMDS_FacePosition* pos = static_cast<SMDS_FacePosition*>( tgtNode->GetPosition() );
        pos->SetUParameter( newUV.X() );
        pos->SetVParameter( newUV.Y() );
      }
    }
    return true;
  }

  return false;
}

//================================================================================
/*!
 * \brief smooth _LayerEdge's on a an EDGE
 */
//================================================================================

bool _Smoother1D::smoothComplexEdge( _SolidData&                    data,
                                     Handle(ShapeAnalysis_Surface)& surface,
                                     const TopoDS_Face&             F,
                                     SMESH_MesherHelper&            helper)
{
  if ( _offPoints.empty() )
    return false;

  // move _offPoints along normals of _LayerEdge's

  _LayerEdge* e[2] = { getLEdgeOnV(0), getLEdgeOnV(1) };
  if ( e[0]->Is( _LayerEdge::NORMAL_UPDATED ))
    _leOnV[0]._normal = getNormalNormal( e[0]->_normal, _edgeDir[0] );
  if ( e[1]->Is( _LayerEdge::NORMAL_UPDATED )) 
    _leOnV[1]._normal = getNormalNormal( e[1]->_normal, _edgeDir[1] );
  _leOnV[0]._len = e[0]->_len;
  _leOnV[1]._len = e[1]->_len;
  for ( size_t i = 0; i < _offPoints.size(); i++ )
  {
    _LayerEdge*  e0 = _offPoints[i]._2edges._edges[0];
    _LayerEdge*  e1 = _offPoints[i]._2edges._edges[1];
    const double w0 = _offPoints[i]._2edges._wgt[0];
    const double w1 = _offPoints[i]._2edges._wgt[1];
    gp_XYZ  avgNorm = ( e0->_normal    * w0 + e1->_normal    * w1 ).Normalized();
    double  avgLen  = ( e0->_len       * w0 + e1->_len       * w1 );
    double  avgFact = ( e0->_lenFactor * w0 + e1->_lenFactor * w1 );
    if ( e0->Is( _LayerEdge::NORMAL_UPDATED ) ||
         e1->Is( _LayerEdge::NORMAL_UPDATED ))
      avgNorm = getNormalNormal( avgNorm, _offPoints[i]._edgeDir );

    _offPoints[i]._xyz += avgNorm * ( avgLen - _offPoints[i]._len ) * avgFact;
    _offPoints[i]._len  = avgLen;
  }

  double fTol = 0;
  if ( !surface.IsNull() ) // project _offPoints to the FACE
  {
    fTol = 100 * BRep_Tool::Tolerance( F );
    //const double segLen = _offPoints[0].Distance( _offPoints[1] );

    gp_Pnt2d uv = surface->ValueOfUV( _offPoints[0]._xyz, fTol );
    //if ( surface->Gap() < 0.5 * segLen )
      _offPoints[0]._xyz = surface->Value( uv ).XYZ();

    for ( size_t i = 1; i < _offPoints.size(); ++i )
    {
      uv = surface->NextValueOfUV( uv, _offPoints[i]._xyz, fTol );
      //if ( surface->Gap() < 0.5 * segLen )
        _offPoints[i]._xyz = surface->Value( uv ).XYZ();
    }
  }

  // project tgt nodes of extreme _LayerEdge's to the offset segments

  if ( e[0]->Is( _LayerEdge::NORMAL_UPDATED )) _iSeg[0] = 0;
  if ( e[1]->Is( _LayerEdge::NORMAL_UPDATED )) _iSeg[1] = _offPoints.size()-2;

  gp_Pnt pExtreme[2], pProj[2];
  for ( int is2nd = 0; is2nd < 2; ++is2nd )
  {
    pExtreme[ is2nd ] = SMESH_TNodeXYZ( e[is2nd]->_nodes.back() );
    int  i = _iSeg[ is2nd ];
    int di = is2nd ? -1 : +1;
    bool projected = false;
    double uOnSeg, distMin = Precision::Infinite(), dist, distPrev = 0;
    int nbWorse = 0;
    do {
      gp_Vec v0p( _offPoints[i]._xyz, pExtreme[ is2nd ]    );
      gp_Vec v01( _offPoints[i]._xyz, _offPoints[i+1]._xyz );
      uOnSeg     = ( v0p * v01 ) / v01.SquareMagnitude();  // param [0,1] along v01
      projected  = ( Abs( uOnSeg - 0.5 ) <= 0.5 );
      dist       =  pExtreme[ is2nd ].SquareDistance( _offPoints[ i + ( uOnSeg > 0.5 )]._xyz );
      if ( dist < distMin || projected )
      {
        _iSeg[ is2nd ] = i;
        pProj[ is2nd ] = _offPoints[i]._xyz + ( v01 * uOnSeg ).XYZ();
        distMin = dist;
      }
      else if ( dist > distPrev )
      {
        if ( ++nbWorse > 3 ) // avoid projection to the middle of a closed EDGE
          break;
      }
      distPrev = dist;
      i += di;
    }
    while ( !projected &&
            i >= 0 && i+1 < (int)_offPoints.size() );

    if ( !projected )
    {
      if (( is2nd && _iSeg[1] != _offPoints.size()-2 ) || ( !is2nd && _iSeg[0] != 0 ))
      {
        _iSeg[0] = 0;
        _iSeg[1] = _offPoints.size()-2;
        debugMsg( "smoothComplexEdge() failed to project nodes of extreme _LayerEdge's" );
        return false;
      }
    }
  }
  if ( _iSeg[0] > _iSeg[1] )
  {
    debugMsg( "smoothComplexEdge() incorrectly projected nodes of extreme _LayerEdge's" );
    return false;
  }

  // adjust length of extreme LE (test viscous_layers_01/B7)
  gp_Vec vDiv0( pExtreme[0], pProj[0] );
  gp_Vec vDiv1( pExtreme[1], pProj[1] );
  double d0 = vDiv0.Magnitude();
  double d1 = vDiv1.Magnitude();
  if ( e[0]->_normal * vDiv0.XYZ() < 0 ) e[0]->_len += d0;
  else                                   e[0]->_len -= d0;
  if ( e[1]->_normal * vDiv1.XYZ() < 0 ) e[1]->_len += d1;
  else                                   e[1]->_len -= d1;

  // compute normalized length of the offset segments located between the projections

  size_t iSeg = 0, nbSeg = _iSeg[1] - _iSeg[0] + 1;
  vector< double > len( nbSeg + 1 );
  len[ iSeg++ ] = 0;
  len[ iSeg++ ] = pProj[ 0 ].Distance( _offPoints[ _iSeg[0]+1 ]._xyz )/* * e[0]->_lenFactor*/;
  for ( size_t i = _iSeg[0]+1; i <= _iSeg[1]; ++i, ++iSeg )
  {
    len[ iSeg ] = len[ iSeg-1 ] + _offPoints[i].Distance( _offPoints[i+1] );
  }
  len[ nbSeg ] -= pProj[ 1 ].Distance( _offPoints[ _iSeg[1]+1 ]._xyz )/* * e[1]->_lenFactor*/;

  // d0 *= e[0]->_lenFactor;
  // d1 *= e[1]->_lenFactor;
  double fullLen = len.back() - d0 - d1;
  for ( iSeg = 0; iSeg < len.size(); ++iSeg )
    len[iSeg] = ( len[iSeg] - d0 ) / fullLen;

  // temporary replace extreme _offPoints by pExtreme
  gp_XYZ op[2] = { _offPoints[ _iSeg[0]   ]._xyz,
                   _offPoints[ _iSeg[1]+1 ]._xyz };
  _offPoints[ _iSeg[0]   ]._xyz = pExtreme[0].XYZ();
  _offPoints[ _iSeg[1]+ 1]._xyz = pExtreme[1].XYZ();

  // distribute tgt nodes of _LayerEdge's between the projections

  iSeg = 0;
  for ( size_t i = 0; i < _eos._edges.size(); ++i )
  {
    if ( _eos._edges[i]->Is( _LayerEdge::BLOCKED )) continue;
    while ( iSeg+2 < len.size() && _leParams[i] > len[ iSeg+1 ] )
      iSeg++;
    double r = ( _leParams[i] - len[ iSeg ]) / ( len[ iSeg+1 ] - len[ iSeg ]);
    gp_XYZ p = ( _offPoints[ iSeg + _iSeg[0]     ]._xyz * ( 1 - r ) +
                 _offPoints[ iSeg + _iSeg[0] + 1 ]._xyz * r );

    if ( surface.IsNull() )
    {
      _eos._edges[i]->_pos.back() = p;
    }
    else // project a new node position to a FACE
    {
      gp_Pnt2d uv ( _eos._edges[i]->_pos.back().X(), _eos._edges[i]->_pos.back().Y() );
      gp_Pnt2d uv2( surface->NextValueOfUV( uv, p, fTol ));

      p = surface->Value( uv2 ).XYZ();
      _eos._edges[i]->_pos.back().SetCoord( uv2.X(), uv2.Y(), 0 );
    }
    SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( _eos._edges[i]->_nodes.back() );
    tgtNode->setXYZ( p.X(), p.Y(), p.Z() );
    dumpMove( tgtNode );
  }

  _offPoints[ _iSeg[0]   ]._xyz = op[0];
  _offPoints[ _iSeg[1]+1 ]._xyz = op[1];

  return true;
}

//================================================================================
/*!
 * \brief Prepare for smoothing
 */
//================================================================================

void _Smoother1D::prepare(_SolidData& data)
{
  const TopoDS_Edge& E = TopoDS::Edge( _eos._shape );
  _curveLen = SMESH_Algo::EdgeLength( E );

  // sort _LayerEdge's by position on the EDGE
  data.SortOnEdge( E, _eos._edges );

  // compute normalized param of _eos._edges on EDGE
  _leParams.resize( _eos._edges.size() + 1 );
  {
    double curLen;
    gp_Pnt pPrev = SMESH_TNodeXYZ( getLEdgeOnV( 0 )->_nodes[0] );
    _leParams[0] = 0;
    for ( size_t i = 0; i < _eos._edges.size(); ++i )
    {
      gp_Pnt p       = SMESH_TNodeXYZ( _eos._edges[i]->_nodes[0] );
      curLen         = p.Distance( pPrev );
      _leParams[i+1] = _leParams[i] + curLen;
      pPrev          = p;
    }
    double fullLen = _leParams.back() + pPrev.Distance( SMESH_TNodeXYZ( getLEdgeOnV(1)->_nodes[0]));
    for ( size_t i = 0; i < _leParams.size()-1; ++i )
      _leParams[i] = _leParams[i+1] / fullLen;
  }

  if ( isAnalytic() )
    return;

  // divide E to have offset segments with low deflection
  BRepAdaptor_Curve c3dAdaptor( E );
  const double curDeflect = 0.1; //0.3; // 0.01; // Curvature deflection
  const double angDeflect = 0.1; //0.2; // 0.09; // Angular deflection
  GCPnts_TangentialDeflection discret(c3dAdaptor, angDeflect, curDeflect);
  if ( discret.NbPoints() <= 2 )
  {
    _anaCurve = new Geom_Line( gp::OX() ); // only type does matter
    return;
  }

  const double u0 = c3dAdaptor.FirstParameter();
  gp_Pnt p; gp_Vec tangent;
  _offPoints.resize( discret.NbPoints() );
  for ( size_t i = 0; i < _offPoints.size(); i++ )
  {
    double u = discret.Parameter( i+1 );
    c3dAdaptor.D1( u, p, tangent );
    _offPoints[i]._xyz     = p.XYZ();
    _offPoints[i]._edgeDir = tangent.XYZ();
    _offPoints[i]._param = GCPnts_AbscissaPoint::Length( c3dAdaptor, u0, u ) / _curveLen;
  }

  _LayerEdge* leOnV[2] = { getLEdgeOnV(0), getLEdgeOnV(1) };

  // set _2edges
  _offPoints    [0]._2edges.set( &_leOnV[0], &_leOnV[0], 0.5, 0.5 );
  _offPoints.back()._2edges.set( &_leOnV[1], &_leOnV[1], 0.5, 0.5 );
  _2NearEdges tmp2edges;
  tmp2edges._edges[1] = _eos._edges[0];
  _leOnV[0]._2neibors = & tmp2edges;
  _leOnV[0]._nodes    = leOnV[0]->_nodes;
  _leOnV[1]._nodes    = leOnV[1]->_nodes;
  _LayerEdge* eNext, *ePrev = & _leOnV[0];
  for ( size_t iLE = 0, i = 1; i < _offPoints.size()-1; i++ )
  {
    // find _LayerEdge's located before and after an offset point
    // (_eos._edges[ iLE ] is next after ePrev)
    while ( iLE < _eos._edges.size() && _offPoints[i]._param > _leParams[ iLE ] )
      ePrev = _eos._edges[ iLE++ ];
    eNext = ePrev->_2neibors->_edges[1];

    gp_Pnt p0 = SMESH_TNodeXYZ( ePrev->_nodes[0] );
    gp_Pnt p1 = SMESH_TNodeXYZ( eNext->_nodes[0] );
    double  r = p0.Distance( _offPoints[i]._xyz ) / p0.Distance( p1 );
    _offPoints[i]._2edges.set( ePrev, eNext, 1-r, r );
  }

  // replace _LayerEdge's on VERTEX by _leOnV in _offPoints._2edges
  for ( size_t i = 0; i < _offPoints.size(); i++ )
    if ( _offPoints[i]._2edges._edges[0] == leOnV[0] )
      _offPoints[i]._2edges._edges[0] = & _leOnV[0];
    else break;
  for ( size_t i = _offPoints.size()-1; i > 0; i-- )
    if ( _offPoints[i]._2edges._edges[1] == leOnV[1] )
      _offPoints[i]._2edges._edges[1] = & _leOnV[1];
    else break;

  // set _normal of _leOnV[0] and _leOnV[1] to be normal to the EDGE

  int iLBO = _offPoints.size() - 2; // last but one

  _edgeDir[0] = getEdgeDir( E, leOnV[0]->_nodes[0], data.GetHelper() );
  _edgeDir[1] = getEdgeDir( E, leOnV[1]->_nodes[0], data.GetHelper() );

  _leOnV[ 0 ]._normal = getNormalNormal( leOnV[0]->_normal, _edgeDir[0] );
  _leOnV[ 1 ]._normal = getNormalNormal( leOnV[1]->_normal, _edgeDir[1] );
  _leOnV[ 0 ]._len = 0;
  _leOnV[ 1 ]._len = 0;
  _leOnV[ 0 ]._lenFactor = _offPoints[1   ]._2edges._edges[1]->_lenFactor;
  _leOnV[ 1 ]._lenFactor = _offPoints[iLBO]._2edges._edges[0]->_lenFactor;

  _iSeg[0] = 0;
  _iSeg[1] = _offPoints.size()-2;

  // initialize OffPnt::_len
  for ( size_t i = 0; i < _offPoints.size(); ++i )
    _offPoints[i]._len = 0;

  if ( _eos._edges[0]->NbSteps() > 1 ) // already inflated several times, init _xyz
  {
    _leOnV[0]._len = leOnV[0]->_len;
    _leOnV[1]._len = leOnV[1]->_len;
    for ( size_t i = 0; i < _offPoints.size(); i++ )
    {
      _LayerEdge*  e0 = _offPoints[i]._2edges._edges[0];
      _LayerEdge*  e1 = _offPoints[i]._2edges._edges[1];
      const double w0 = _offPoints[i]._2edges._wgt[0];
      const double w1 = _offPoints[i]._2edges._wgt[1];
      double  avgLen  = ( e0->_len * w0 + e1->_len * w1 );
      gp_XYZ  avgXYZ  = ( SMESH_TNodeXYZ( e0->_nodes.back() ) * w0 +
                          SMESH_TNodeXYZ( e1->_nodes.back() ) * w1 );
      _offPoints[i]._xyz = avgXYZ;
      _offPoints[i]._len = avgLen;
    }
  }
}

//================================================================================
/*!
 * \brief set _normal of _leOnV[is2nd] to be normal to the EDGE
 */
//================================================================================

gp_XYZ _Smoother1D::getNormalNormal( const gp_XYZ & normal,
                                     const gp_XYZ&  edgeDir)
{
  gp_XYZ cross = normal ^ edgeDir;
  gp_XYZ  norm = edgeDir ^ cross;
  double  size = norm.Modulus();

  return norm / size;
}

//================================================================================
/*!
 * \brief Sort _LayerEdge's by a parameter on a given EDGE
 */
//================================================================================

void _SolidData::SortOnEdge( const TopoDS_Edge&     E,
                             vector< _LayerEdge* >& edges)
{
  map< double, _LayerEdge* > u2edge;
  for ( size_t i = 0; i < edges.size(); ++i )
    u2edge.insert( u2edge.end(),
                   make_pair( _helper->GetNodeU( E, edges[i]->_nodes[0] ), edges[i] ));

  ASSERT( u2edge.size() == edges.size() );
  map< double, _LayerEdge* >::iterator u2e = u2edge.begin();
  for ( size_t i = 0; i < edges.size(); ++i, ++u2e )
    edges[i] = u2e->second;

  Sort2NeiborsOnEdge( edges );
}

//================================================================================
/*!
 * \brief Set _2neibors according to the order of _LayerEdge on EDGE
 */
//================================================================================

void _SolidData::Sort2NeiborsOnEdge( vector< _LayerEdge* >& edges )
{
  if ( edges.size() < 2 || !edges[0]->_2neibors ) return;

  for ( size_t i = 0; i < edges.size()-1; ++i )
    if ( edges[i]->_2neibors->tgtNode(1) != edges[i+1]->_nodes.back() )
      edges[i]->_2neibors->reverse();

  const size_t iLast = edges.size() - 1;
  if ( edges.size() > 1 &&
       edges[iLast]->_2neibors->tgtNode(0) != edges[iLast-1]->_nodes.back() )
    edges[iLast]->_2neibors->reverse();
}

//================================================================================
/*!
 * \brief Return _EdgesOnShape* corresponding to the shape
 */
//================================================================================

_EdgesOnShape* _SolidData::GetShapeEdges(const TGeomID shapeID )
{
  if ( shapeID < (int)_edgesOnShape.size() &&
       _edgesOnShape[ shapeID ]._shapeID == shapeID )
    return _edgesOnShape[ shapeID ]._subMesh ? & _edgesOnShape[ shapeID ] : 0;

  for ( size_t i = 0; i < _edgesOnShape.size(); ++i )
    if ( _edgesOnShape[i]._shapeID == shapeID )
      return _edgesOnShape[i]._subMesh ? & _edgesOnShape[i] : 0;

  return 0;
}

//================================================================================
/*!
 * \brief Return _EdgesOnShape* corresponding to the shape
 */
//================================================================================

_EdgesOnShape* _SolidData::GetShapeEdges(const TopoDS_Shape& shape )
{
  SMESHDS_Mesh* meshDS = _proxyMesh->GetMesh()->GetMeshDS();
  return GetShapeEdges( meshDS->ShapeToIndex( shape ));
}

//================================================================================
/*!
 * \brief Prepare data of the _LayerEdge for smoothing on FACE
 */
//================================================================================

void _SolidData::PrepareEdgesToSmoothOnFace( _EdgesOnShape* eos, bool substituteSrcNodes )
{
  SMESH_MesherHelper helper( *_proxyMesh->GetMesh() );

  set< TGeomID > vertices;
  TopoDS_Face F;
  if ( eos->ShapeType() == TopAbs_FACE )
  {
    // check FACE concavity and get concave VERTEXes
    F = TopoDS::Face( eos->_shape );
    if ( isConcave( F, helper, &vertices ))
      _concaveFaces.insert( eos->_shapeID );

    // set eos._eosConcaVer
    eos->_eosConcaVer.clear();
    eos->_eosConcaVer.reserve( vertices.size() );
    for ( set< TGeomID >::iterator v = vertices.begin(); v != vertices.end(); ++v )
    {
      _EdgesOnShape* eov = GetShapeEdges( *v );
      if ( eov && eov->_edges.size() == 1 )
      {
        eos->_eosConcaVer.push_back( eov );
        for ( size_t i = 0; i < eov->_edges[0]->_neibors.size(); ++i )
          eov->_edges[0]->_neibors[i]->Set( _LayerEdge::DIFFICULT );
      }
    }

    // SetSmooLen() to _LayerEdge's on FACE
    for ( size_t i = 0; i < eos->_edges.size(); ++i )
    {
      eos->_edges[i]->SetSmooLen( Precision::Infinite() );
    }
    SMESH_subMeshIteratorPtr smIt = eos->_subMesh->getDependsOnIterator(/*includeSelf=*/false);
    while ( smIt->more() ) // loop on sub-shapes of the FACE
    {
      _EdgesOnShape* eoe = GetShapeEdges( smIt->next()->GetId() );
      if ( !eoe ) continue;

      vector<_LayerEdge*>& eE = eoe->_edges;
      for ( size_t iE = 0; iE < eE.size(); ++iE ) // loop on _LayerEdge's on EDGE or VERTEX
      {
        if ( eE[iE]->_cosin <= theMinSmoothCosin )
          continue;

        SMDS_ElemIteratorPtr segIt = eE[iE]->_nodes[0]->GetInverseElementIterator(SMDSAbs_Edge);
        while ( segIt->more() )
        {
          const SMDS_MeshElement* seg = segIt->next();
          if ( !eos->_subMesh->DependsOn( seg->getshapeId() ))
            continue;
          if ( seg->GetNode(0) != eE[iE]->_nodes[0] )
            continue; // not to check a seg twice
          for ( size_t iN = 0; iN < eE[iE]->_neibors.size(); ++iN )
          {
            _LayerEdge* eN = eE[iE]->_neibors[iN];
            if ( eN->_nodes[0]->getshapeId() != eos->_shapeID )
              continue;
            double dist    = SMESH_MeshAlgos::GetDistance( seg, SMESH_TNodeXYZ( eN->_nodes[0] ));
            double smooLen = getSmoothingThickness( eE[iE]->_cosin, dist );
            eN->SetSmooLen( Min( smooLen, eN->GetSmooLen() ));
            eN->Set( _LayerEdge::NEAR_BOUNDARY );
          }
        }
      }
    }
  } // if ( eos->ShapeType() == TopAbs_FACE )

  for ( size_t i = 0; i < eos->_edges.size(); ++i )
  {
    eos->_edges[i]->_smooFunction = 0;
    eos->_edges[i]->Set( _LayerEdge::TO_SMOOTH );
  }
  bool isCurved = false;
  for ( size_t i = 0; i < eos->_edges.size(); ++i )
  {
    _LayerEdge* edge = eos->_edges[i];

    // get simplices sorted
    _Simplex::SortSimplices( edge->_simplices );

    // smoothing function
    edge->ChooseSmooFunction( vertices, _n2eMap );

    // set _curvature
    double avgNormProj = 0, avgLen = 0;
    for ( size_t iS = 0; iS < edge->_simplices.size(); ++iS )
    {
      _Simplex& s = edge->_simplices[iS];

      gp_XYZ  vec = edge->_pos.back() - SMESH_TNodeXYZ( s._nPrev );
      avgNormProj += edge->_normal * vec;
      avgLen      += vec.Modulus();
      if ( substituteSrcNodes )
      {
        s._nNext = _n2eMap[ s._nNext ]->_nodes.back();
        s._nPrev = _n2eMap[ s._nPrev ]->_nodes.back();
      }
    }
    avgNormProj /= edge->_simplices.size();
    avgLen      /= edge->_simplices.size();
    if (( edge->_curvature = _Curvature::New( avgNormProj, avgLen )))
    {
      isCurved = true;
      SMDS_FacePosition* fPos = dynamic_cast<SMDS_FacePosition*>( edge->_nodes[0]->GetPosition() );
      if ( !fPos )
        for ( size_t iS = 0; iS < edge->_simplices.size()  &&  !fPos; ++iS )
          fPos = dynamic_cast<SMDS_FacePosition*>( edge->_simplices[iS]._nPrev->GetPosition() );
      if ( fPos )
        edge->_curvature->_uv.SetCoord( fPos->GetUParameter(), fPos->GetVParameter() );
    }
  }

  // prepare for putOnOffsetSurface()
  if (( eos->ShapeType() == TopAbs_FACE ) &&
      ( isCurved || !eos->_eosConcaVer.empty() ))
  {
    eos->_offsetSurf = helper.GetSurface( TopoDS::Face( eos->_shape ));
    eos->_edgeForOffset = 0;

    double maxCosin = -1;
    for ( TopExp_Explorer eExp( eos->_shape, TopAbs_EDGE ); eExp.More(); eExp.Next() )
    {
      _EdgesOnShape* eoe = GetShapeEdges( eExp.Current() );
      if ( !eoe || eoe->_edges.empty() ) continue;

      vector<_LayerEdge*>& eE = eoe->_edges;
      _LayerEdge* e = eE[ eE.size() / 2 ];
      if ( e->_cosin > maxCosin )
      {
        eos->_edgeForOffset = e;
        maxCosin = e->_cosin;
      }
    }
  }
}

//================================================================================
/*!
 * \brief Add faces for smoothing
 */
//================================================================================

void _SolidData::AddShapesToSmooth( const set< _EdgesOnShape* >& eosToSmooth,
                                    const set< _EdgesOnShape* >* edgesNoAnaSmooth )
{
  set< _EdgesOnShape * >::const_iterator eos = eosToSmooth.begin();
  for ( ; eos != eosToSmooth.end(); ++eos )
  {
    if ( !*eos || (*eos)->_toSmooth ) continue;

    (*eos)->_toSmooth = true;

    if ( (*eos)->ShapeType() == TopAbs_FACE )
    {
      PrepareEdgesToSmoothOnFace( *eos, /*substituteSrcNodes=*/false );
      (*eos)->_toSmooth = true;
    }
  }

  // avoid _Smoother1D::smoothAnalyticEdge() of edgesNoAnaSmooth
  if ( edgesNoAnaSmooth )
    for ( eos = edgesNoAnaSmooth->begin(); eos != edgesNoAnaSmooth->end(); ++eos )
    {
      if ( (*eos)->_edgeSmoother )
        (*eos)->_edgeSmoother->_anaCurve.Nullify();
    }
}

//================================================================================
/*!
 * \brief Limit _LayerEdge::_maxLen according to local curvature
 */
//================================================================================

void _ViscousBuilder::limitMaxLenByCurvature( _SolidData& data, SMESH_MesherHelper& helper )
{
  // find intersection of neighbor _LayerEdge's to limit _maxLen
  // according to local curvature (IPAL52648)

  // This method must be called after findCollisionEdges() where _LayerEdge's
  // get _lenFactor initialized in the case of eos._hyp.IsOffsetMethod()

  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eosI = data._edgesOnShape[iS];
    if ( eosI._edges.empty() ) continue;
    if ( !eosI._hyp.ToSmooth() )
    {
      for ( size_t i = 0; i < eosI._edges.size(); ++i )
      {
        _LayerEdge* eI = eosI._edges[i];
        for ( size_t iN = 0; iN < eI->_neibors.size(); ++iN )
        {
          _LayerEdge* eN = eI->_neibors[iN];
          if ( eI->_nodes[0]->GetID() < eN->_nodes[0]->GetID() ) // treat this pair once
          {
            _EdgesOnShape* eosN = data.GetShapeEdges( eN );
            limitMaxLenByCurvature( eI, eN, eosI, *eosN, helper );
          }
        }
      }
    }
    else if ( eosI.ShapeType() == TopAbs_EDGE )
    {
      const TopoDS_Edge& E = TopoDS::Edge( eosI._shape );
      if ( SMESH_Algo::IsStraight( E, /*degenResult=*/true )) continue;

      _LayerEdge* e0 = eosI._edges[0];
      for ( size_t i = 1; i < eosI._edges.size(); ++i )
      {
        _LayerEdge* eI = eosI._edges[i];
        limitMaxLenByCurvature( eI, e0, eosI, eosI, helper );
        e0 = eI;
      }
    }
  }
}

//================================================================================
/*!
 * \brief Limit _LayerEdge::_maxLen according to local curvature
 */
//================================================================================

void _ViscousBuilder::limitMaxLenByCurvature( _LayerEdge*         e1,
                                              _LayerEdge*         e2,
                                              _EdgesOnShape&      eos1,
                                              _EdgesOnShape&      eos2,
                                              SMESH_MesherHelper& helper )
{
  gp_XYZ plnNorm = e1->_normal ^ e2->_normal;
  double norSize = plnNorm.SquareModulus();
  if ( norSize < std::numeric_limits<double>::min() )
    return; // parallel normals

  // find closest points of skew _LayerEdge's
  SMESH_TNodeXYZ src1( e1->_nodes[0] ), src2( e2->_nodes[0] );
  gp_XYZ dir12 = src2 - src1;
  gp_XYZ perp1 = e1->_normal ^ plnNorm;
  gp_XYZ perp2 = e2->_normal ^ plnNorm;
  double  dot1 = perp2 * e1->_normal;
  double  dot2 = perp1 * e2->_normal;
  double    u1 =   ( perp2 * dir12 ) / dot1;
  double    u2 = - ( perp1 * dir12 ) / dot2;
  if ( u1 > 0 && u2 > 0 )
  {
    double ovl = ( u1 * e1->_normal * dir12 -
                   u2 * e2->_normal * dir12 ) / dir12.SquareModulus();
    if ( ovl > theSmoothThickToElemSizeRatio )
    {    
      e1->_maxLen = Min( e1->_maxLen, 0.75 * u1 / e1->_lenFactor );
      e2->_maxLen = Min( e2->_maxLen, 0.75 * u2 / e2->_lenFactor );
    }
  }
}

//================================================================================
/*!
 * \brief Fill data._collisionEdges
 */
//================================================================================

void _ViscousBuilder::findCollisionEdges( _SolidData& data, SMESH_MesherHelper& helper )
{
  data._collisionEdges.clear();

  // set the full thickness of the layers to LEs
  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[iS];
    if ( eos._edges.empty() ) continue;
    if ( eos.ShapeType() != TopAbs_EDGE && eos.ShapeType() != TopAbs_VERTEX ) continue;

    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      if ( eos._edges[i]->Is( _LayerEdge::BLOCKED )) continue;
      double maxLen = eos._edges[i]->_maxLen;
      eos._edges[i]->_maxLen = Precision::Infinite(); // avoid blocking
      eos._edges[i]->SetNewLength( 1.5 * maxLen, eos, helper );
      eos._edges[i]->_maxLen = maxLen;
    }
  }

  // make temporary quadrangles got by extrusion of
  // mesh edges along _LayerEdge._normal's

  vector< const SMDS_MeshElement* > tmpFaces;

  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[ iS ];
    if ( eos.ShapeType() != TopAbs_EDGE )
      continue;
    if ( eos._edges.empty() )
    {
      _LayerEdge* edge[2] = { 0, 0 }; // LE of 2 VERTEX'es
      SMESH_subMeshIteratorPtr smIt = eos._subMesh->getDependsOnIterator(/*includeSelf=*/false);
      while ( smIt->more() )
        if ( _EdgesOnShape* eov = data.GetShapeEdges( smIt->next()->GetId() ))
          if ( eov->_edges.size() == 1 )
            edge[ bool( edge[0]) ] = eov->_edges[0];

      if ( edge[1] )
      {
        _TmpMeshFaceOnEdge* f = new _TmpMeshFaceOnEdge( edge[0], edge[1], --_tmpFaceID );
        tmpFaces.push_back( f );
      }
    }
    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      _LayerEdge* edge = eos._edges[i];
      for ( int j = 0; j < 2; ++j ) // loop on _2NearEdges
      {
        const SMDS_MeshNode* src2 = edge->_2neibors->srcNode(j);
        if ( src2->GetPosition()->GetDim() > 0 &&
             src2->GetID() < edge->_nodes[0]->GetID() )
          continue; // avoid using same segment twice

        // a _LayerEdge containg tgt2
        _LayerEdge* neiborEdge = edge->_2neibors->_edges[j];

        _TmpMeshFaceOnEdge* f = new _TmpMeshFaceOnEdge( edge, neiborEdge, --_tmpFaceID );
        tmpFaces.push_back( f );
      }
    }
  }

  // Find _LayerEdge's intersecting tmpFaces.

  SMDS_ElemIteratorPtr fIt( new SMDS_ElementVectorIterator( tmpFaces.begin(),
                                                            tmpFaces.end()));
  SMESHUtils::Deleter<SMESH_ElementSearcher> searcher
    ( SMESH_MeshAlgos::GetElementSearcher( *getMeshDS(), fIt ));

  double dist1, dist2, segLen, eps = 0.5;
  _CollisionEdges collEdges;
  vector< const SMDS_MeshElement* > suspectFaces;
  const double angle45 = Cos( 45. * M_PI / 180. );

  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[ iS ];
    if ( eos.ShapeType() == TopAbs_FACE || !eos._sWOL.IsNull() )
      continue;
    // find sub-shapes whose VL can influence VL on eos
    set< TGeomID > neighborShapes;
    PShapeIteratorPtr fIt = helper.GetAncestors( eos._shape, *_mesh, TopAbs_FACE );
    while ( const TopoDS_Shape* face = fIt->next() )
    {
      TGeomID faceID = getMeshDS()->ShapeToIndex( *face );
      if ( _EdgesOnShape* eof = data.GetShapeEdges( faceID ))
      {
        SMESH_subMeshIteratorPtr subIt = eof->_subMesh->getDependsOnIterator(/*includeSelf=*/false);
        while ( subIt->more() )
          neighborShapes.insert( subIt->next()->GetId() );
      }
    }
    if ( eos.ShapeType() == TopAbs_VERTEX )
    {
      PShapeIteratorPtr eIt = helper.GetAncestors( eos._shape, *_mesh, TopAbs_EDGE );
      while ( const TopoDS_Shape* edge = eIt->next() )
        neighborShapes.erase( getMeshDS()->ShapeToIndex( *edge ));
    }
    // find intersecting _LayerEdge's
    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      if ( eos._edges[i]->Is( _LayerEdge::MULTI_NORMAL )) continue;
      _LayerEdge*   edge = eos._edges[i];
      gp_Ax1 lastSegment = edge->LastSegment( segLen, eos );
      segLen *= 1.2;

      gp_Vec eSegDir0, eSegDir1;
      if ( edge->IsOnEdge() )
      {
        SMESH_TNodeXYZ eP( edge->_nodes[0] );
        eSegDir0 = SMESH_TNodeXYZ( edge->_2neibors->srcNode(0) ) - eP;
        eSegDir1 = SMESH_TNodeXYZ( edge->_2neibors->srcNode(1) ) - eP;
      }
      suspectFaces.clear();
      searcher->GetElementsInSphere( SMESH_TNodeXYZ( edge->_nodes.back()), edge->_len * 2,
                                     SMDSAbs_Face, suspectFaces );
      collEdges._intEdges.clear();
      for ( size_t j = 0 ; j < suspectFaces.size(); ++j )
      {
        const _TmpMeshFaceOnEdge* f = (const _TmpMeshFaceOnEdge*) suspectFaces[j];
        if ( f->_le1 == edge || f->_le2 == edge ) continue;
        if ( !neighborShapes.count( f->_le1->_nodes[0]->getshapeId() )) continue;
        if ( !neighborShapes.count( f->_le2->_nodes[0]->getshapeId() )) continue;
        if ( edge->IsOnEdge() ) {
          if ( edge->_2neibors->include( f->_le1 ) ||
               edge->_2neibors->include( f->_le2 )) continue;
        }
        else {
          if (( f->_le1->IsOnEdge() && f->_le1->_2neibors->include( edge )) ||
              ( f->_le2->IsOnEdge() && f->_le2->_2neibors->include( edge )))  continue;
        }
        dist1 = dist2 = Precision::Infinite();
        if ( !edge->SegTriaInter( lastSegment, f->_nn[0], f->_nn[1], f->_nn[2], dist1, eps ))
          dist1 = Precision::Infinite();
        if ( !edge->SegTriaInter( lastSegment, f->_nn[3], f->_nn[2], f->_nn[0], dist2, eps ))
          dist2 = Precision::Infinite();
        if (( dist1 > segLen ) && ( dist2 > segLen ))
          continue;

        if ( edge->IsOnEdge() )
        {
          // skip perpendicular EDGEs
          gp_Vec fSegDir  = SMESH_TNodeXYZ( f->_nn[0] ) - SMESH_TNodeXYZ( f->_nn[3] );
          bool isParallel = ( isLessAngle( eSegDir0, fSegDir, angle45 ) ||
                              isLessAngle( eSegDir1, fSegDir, angle45 ) ||
                              isLessAngle( eSegDir0, fSegDir.Reversed(), angle45 ) ||
                              isLessAngle( eSegDir1, fSegDir.Reversed(), angle45 ));
          if ( !isParallel )
            continue;
        }

        // either limit inflation of edges or remember them for updating _normal
        // double dot = edge->_normal * f->GetDir();
        // if ( dot > 0.1 )
        {
          collEdges._intEdges.push_back( f->_le1 );
          collEdges._intEdges.push_back( f->_le2 );
        }
        // else
        // {
        //   double shortLen = 0.75 * ( Min( dist1, dist2 ) / edge->_lenFactor );
        //   edge->_maxLen = Min( shortLen, edge->_maxLen );
        // }
      }

      if ( !collEdges._intEdges.empty() )
      {
        collEdges._edge = edge;
        data._collisionEdges.push_back( collEdges );
      }
    }
  }

  for ( size_t i = 0 ; i < tmpFaces.size(); ++i )
    delete tmpFaces[i];

  // restore the zero thickness
  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[iS];
    if ( eos._edges.empty() ) continue;
    if ( eos.ShapeType() != TopAbs_EDGE && eos.ShapeType() != TopAbs_VERTEX ) continue;

    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      eos._edges[i]->InvalidateStep( 1, eos );
      eos._edges[i]->_len = 0;
    }
  }
}

//================================================================================
/*!
 * \brief Modify normals of _LayerEdge's on EDGE's to avoid intersection with
 * _LayerEdge's on neighbor EDGE's
 */
//================================================================================

bool _ViscousBuilder::updateNormals( _SolidData&         data,
                                     SMESH_MesherHelper& helper,
                                     int                 stepNb,
                                     double              stepSize)
{
  updateNormalsOfC1Vertices( data );

  if ( stepNb > 0 && !updateNormalsOfConvexFaces( data, helper, stepNb ))
    return false;

  // map to store new _normal and _cosin for each intersected edge
  map< _LayerEdge*, _LayerEdge, _LayerEdgeCmp >           edge2newEdge;
  map< _LayerEdge*, _LayerEdge, _LayerEdgeCmp >::iterator e2neIt;
  _LayerEdge zeroEdge;
  zeroEdge._normal.SetCoord( 0,0,0 );
  zeroEdge._maxLen = Precision::Infinite();
  zeroEdge._nodes.resize(1); // to init _TmpMeshFaceOnEdge

  set< _EdgesOnShape* > shapesToSmooth, edgesNoAnaSmooth;

  double segLen, dist1, dist2, dist;
  vector< pair< _LayerEdge*, double > > intEdgesDist;
  _TmpMeshFaceOnEdge quad( &zeroEdge, &zeroEdge, 0 );

  for ( int iter = 0; iter < 5; ++iter )
  {
    edge2newEdge.clear();

    for ( size_t iE = 0; iE < data._collisionEdges.size(); ++iE )
    {
      _CollisionEdges& ce = data._collisionEdges[iE];
      _LayerEdge*   edge1 = ce._edge;
      if ( !edge1 /*|| edge1->Is( _LayerEdge::BLOCKED )*/) continue;
      _EdgesOnShape* eos1 = data.GetShapeEdges( edge1 );
      if ( !eos1 ) continue;

      // detect intersections
      gp_Ax1 lastSeg = edge1->LastSegment( segLen, *eos1 );
      double testLen = 1.5 * edge1->_maxLen * edge1->_lenFactor;
      double     eps = 0.5;
      intEdgesDist.clear();
      double minIntDist = Precision::Infinite();
      for ( size_t i = 0; i < ce._intEdges.size(); i += 2 )
      {
        if ( edge1->Is( _LayerEdge::BLOCKED ) &&
             ce._intEdges[i  ]->Is( _LayerEdge::BLOCKED ) &&
             ce._intEdges[i+1]->Is( _LayerEdge::BLOCKED ))
          continue;
        double dot  = edge1->_normal * quad.GetDir( ce._intEdges[i], ce._intEdges[i+1] );
        double fact = ( 1.1 + dot * dot );
        SMESH_TNodeXYZ pSrc0( ce.nSrc(i) ), pSrc1( ce.nSrc(i+1) );
        SMESH_TNodeXYZ pTgt0( ce.nTgt(i) ), pTgt1( ce.nTgt(i+1) );
        gp_XYZ pLast0 = pSrc0 + ( pTgt0 - pSrc0 ) * fact;
        gp_XYZ pLast1 = pSrc1 + ( pTgt1 - pSrc1 ) * fact;
        dist1 = dist2 = Precision::Infinite();
        if ( !edge1->SegTriaInter( lastSeg, pSrc0, pLast0, pSrc1,  dist1, eps ) &&
             !edge1->SegTriaInter( lastSeg, pSrc1, pLast1, pLast0, dist2, eps ))
          continue;
        dist = dist1;
        if ( dist > testLen || dist <= 0 )
        {
          dist = dist2;
          if ( dist > testLen || dist <= 0 )
            continue;
        }
        // choose a closest edge
        gp_Pnt intP( lastSeg.Location().XYZ() + lastSeg.Direction().XYZ() * ( dist + segLen ));
        double d1 = intP.SquareDistance( pSrc0 );
        double d2 = intP.SquareDistance( pSrc1 );
        int iClose = i + ( d2 < d1 );
        _LayerEdge* edge2 = ce._intEdges[iClose];
        edge2->Unset( _LayerEdge::MARKED );

        // choose a closest edge among neighbors
        gp_Pnt srcP( SMESH_TNodeXYZ( edge1->_nodes[0] ));
        d1 = srcP.SquareDistance( SMESH_TNodeXYZ( edge2->_nodes[0] ));
        for ( size_t j = 0; j < intEdgesDist.size(); ++j )
        {
          _LayerEdge * edgeJ = intEdgesDist[j].first;
          if ( edge2->IsNeiborOnEdge( edgeJ ))
          {
            d2 = srcP.SquareDistance( SMESH_TNodeXYZ( edgeJ->_nodes[0] ));
            ( d1 < d2 ? edgeJ : edge2 )->Set( _LayerEdge::MARKED );
          }
        }
        intEdgesDist.push_back( make_pair( edge2, dist ));
        // if ( Abs( d2 - d1 ) / Max( d2, d1 ) < 0.5 )
        // {
        //   iClose = i + !( d2 < d1 );
        //   intEdges.push_back( ce._intEdges[iClose] );
        //   ce._intEdges[iClose]->Unset( _LayerEdge::MARKED );
        // }
        minIntDist = Min( edge1->_len * edge1->_lenFactor - segLen + dist, minIntDist );
      }

      //ce._edge = 0;

      // compute new _normals
      for ( size_t i = 0; i < intEdgesDist.size(); ++i )
      {
        _LayerEdge* edge2    = intEdgesDist[i].first;
        double       distWgt = edge1->_len / intEdgesDist[i].second;
        // if ( edge1->Is( _LayerEdge::BLOCKED ) &&
        //      edge2->Is( _LayerEdge::BLOCKED )) continue;        
        if ( edge2->Is( _LayerEdge::MARKED )) continue;
        edge2->Set( _LayerEdge::MARKED );

        // get a new normal
        gp_XYZ dir1 = edge1->_normal, dir2 = edge2->_normal;

        double cos1 = Abs( edge1->_cosin ), cos2 = Abs( edge2->_cosin );
        double wgt1 = ( cos1 + 0.001 ) / ( cos1 + cos2 + 0.002 );
        double wgt2 = ( cos2 + 0.001 ) / ( cos1 + cos2 + 0.002 );
        // double cos1 = Abs( edge1->_cosin ),        cos2 = Abs( edge2->_cosin );
        // double sgn1 = 0.1 * ( 1 + edge1->_cosin ), sgn2 = 0.1 * ( 1 + edge2->_cosin );
        // double wgt1 = ( cos1 + sgn1 ) / ( cos1 + cos2 + sgn1 + sgn2 );
        // double wgt2 = ( cos2 + sgn2 ) / ( cos1 + cos2 + sgn1 + sgn2 );
        gp_XYZ newNormal = wgt1 * dir1 + wgt2 * dir2;
        newNormal.Normalize();

        // get new cosin
        double newCos;
        double sgn1 = edge1->_cosin / cos1, sgn2 = edge2->_cosin / cos2;
        if ( cos1 < theMinSmoothCosin )
        {
          newCos = cos2 * sgn1;
        }
        else if ( cos2 > theMinSmoothCosin ) // both cos1 and cos2 > theMinSmoothCosin
        {
          newCos = ( wgt1 * cos1 + wgt2 * cos2 ) * edge1->_cosin / cos1;
        }
        else
        {
          newCos = edge1->_cosin;
        }

        e2neIt = edge2newEdge.insert( make_pair( edge1, zeroEdge )).first;
        e2neIt->second._normal += distWgt * newNormal;
        e2neIt->second._cosin   = newCos;
        e2neIt->second._maxLen  = 0.7 * minIntDist / edge1->_lenFactor;
        if ( iter > 0 && sgn1 * sgn2 < 0 && edge1->_cosin < 0 )
          e2neIt->second._normal += dir2;
        e2neIt = edge2newEdge.insert( make_pair( edge2, zeroEdge )).first;
        e2neIt->second._normal += distWgt * newNormal;
        e2neIt->second._cosin   = edge2->_cosin;
        if ( iter > 0 && sgn1 * sgn2 < 0 && edge2->_cosin < 0 )
          e2neIt->second._normal += dir1;
      }
    }

    if ( edge2newEdge.empty() )
      break; //return true;

    dumpFunction(SMESH_Comment("updateNormals")<< data._index << "_" << stepNb << "_it" << iter);

    // Update data of edges depending on a new _normal

    data.UnmarkEdges();
    for ( e2neIt = edge2newEdge.begin(); e2neIt != edge2newEdge.end(); ++e2neIt )
    {
      _LayerEdge*    edge = e2neIt->first;
      if ( edge->Is( _LayerEdge::BLOCKED )) continue;
      _LayerEdge& newEdge = e2neIt->second;
      _EdgesOnShape*  eos = data.GetShapeEdges( edge );

      // Check if a new _normal is OK:
      newEdge._normal.Normalize();
      if ( !isNewNormalOk( data, *edge, newEdge._normal ))
      {
        if ( newEdge._maxLen < edge->_len && iter > 0 ) // limit _maxLen
        {
          edge->InvalidateStep( stepNb + 1, *eos, /*restoreLength=*/true  );
          edge->_maxLen = newEdge._maxLen;
          edge->SetNewLength( newEdge._maxLen, *eos, helper );
        }
        continue; // the new _normal is bad
      }
      // the new _normal is OK

      // find shapes that need smoothing due to change of _normal
      if ( edge->_cosin   < theMinSmoothCosin &&
           newEdge._cosin > theMinSmoothCosin )
      {
        if ( eos->_sWOL.IsNull() )
        {
          SMDS_ElemIteratorPtr fIt = edge->_nodes[0]->GetInverseElementIterator(SMDSAbs_Face);
          while ( fIt->more() )
            shapesToSmooth.insert( data.GetShapeEdges( fIt->next()->getshapeId() ));
        }
        else // edge inflates along a FACE
        {
          TopoDS_Shape V = helper.GetSubShapeByNode( edge->_nodes[0], getMeshDS() );
          PShapeIteratorPtr eIt = helper.GetAncestors( V, *_mesh, TopAbs_EDGE, &eos->_sWOL );
          while ( const TopoDS_Shape* E = eIt->next() )
          {
            gp_Vec edgeDir = getEdgeDir( TopoDS::Edge( *E ), TopoDS::Vertex( V ));
            double   angle = edgeDir.Angle( newEdge._normal ); // [0,PI]
            if ( angle < M_PI / 2 )
              shapesToSmooth.insert( data.GetShapeEdges( *E ));
          }
        }
      }

      double len = edge->_len;
      edge->InvalidateStep( stepNb + 1, *eos, /*restoreLength=*/true  );
      edge->SetNormal( newEdge._normal );
      edge->SetCosin( newEdge._cosin );
      edge->SetNewLength( len, *eos, helper );
      edge->Set( _LayerEdge::MARKED );
      edge->Set( _LayerEdge::NORMAL_UPDATED );
      edgesNoAnaSmooth.insert( eos );
    }

    // Update normals and other dependent data of not intersecting _LayerEdge's
    // neighboring the intersecting ones

    for ( e2neIt = edge2newEdge.begin(); e2neIt != edge2newEdge.end(); ++e2neIt )
    {
      _LayerEdge*   edge1 = e2neIt->first;
      _EdgesOnShape* eos1 = data.GetShapeEdges( edge1 );
      if ( !edge1->Is( _LayerEdge::MARKED ))
        continue;

      if ( edge1->IsOnEdge() )
      {
        const SMDS_MeshNode * n1 = edge1->_2neibors->srcNode(0);
        const SMDS_MeshNode * n2 = edge1->_2neibors->srcNode(1);
        edge1->SetDataByNeighbors( n1, n2, *eos1, helper );
      }

      if ( !edge1->_2neibors || !eos1->_sWOL.IsNull() )
        continue;
      for ( int j = 0; j < 2; ++j ) // loop on 2 neighbors
      {
        _LayerEdge* neighbor = edge1->_2neibors->_edges[j];
        if ( neighbor->Is( _LayerEdge::MARKED ) /*edge2newEdge.count ( neighbor )*/)
          continue; // j-th neighbor is also intersected
        _LayerEdge* prevEdge = edge1;
        const int nbSteps = 10;
        for ( int step = nbSteps; step; --step ) // step from edge1 in j-th direction
        {
          if ( neighbor->Is( _LayerEdge::BLOCKED ) ||
               neighbor->Is( _LayerEdge::MARKED ))
            break;
          _EdgesOnShape* eos = data.GetShapeEdges( neighbor );
          if ( !eos ) continue;
          _LayerEdge* nextEdge = neighbor;
          if ( neighbor->_2neibors )
          {
            int iNext = 0;
            nextEdge = neighbor->_2neibors->_edges[iNext];
            if ( nextEdge == prevEdge )
              nextEdge = neighbor->_2neibors->_edges[ ++iNext ];
          }
          double r = double(step-1)/nbSteps/(iter+1);
          if ( !nextEdge->_2neibors )
            r = Min( r, 0.5 );

          gp_XYZ newNorm = prevEdge->_normal * r + nextEdge->_normal * (1-r);
          newNorm.Normalize();
          if ( !isNewNormalOk( data, *neighbor, newNorm ))
            break;

          double len = neighbor->_len;
          neighbor->InvalidateStep( stepNb + 1, *eos, /*restoreLength=*/true  );
          neighbor->SetNormal( newNorm );
          neighbor->SetCosin( prevEdge->_cosin * r + nextEdge->_cosin * (1-r) );
          if ( neighbor->_2neibors )
            neighbor->SetDataByNeighbors( prevEdge->_nodes[0], nextEdge->_nodes[0], *eos, helper );
          neighbor->SetNewLength( len, *eos, helper );
          neighbor->Set( _LayerEdge::MARKED );
          neighbor->Set( _LayerEdge::NORMAL_UPDATED );
          edgesNoAnaSmooth.insert( eos );

          if ( !neighbor->_2neibors )
            break; // neighbor is on VERTEX

          // goto the next neighbor
          prevEdge = neighbor;
          neighbor = nextEdge;
        }
      }
    }
    dumpFunctionEnd();
  } // iterations

  data.AddShapesToSmooth( shapesToSmooth, &edgesNoAnaSmooth );

  return true;
}

//================================================================================
/*!
 * \brief Check if a new normal is OK
 */
//================================================================================

bool _ViscousBuilder::isNewNormalOk( _SolidData&   data,
                                     _LayerEdge&   edge,
                                     const gp_XYZ& newNormal)
{
  // check a min angle between the newNormal and surrounding faces
  vector<_Simplex> simplices;
  SMESH_TNodeXYZ n0( edge._nodes[0] ), n1, n2;
  _Simplex::GetSimplices( n0._node, simplices, data._ignoreFaceIds, &data );
  double newMinDot = 1, curMinDot = 1;
  for ( size_t i = 0; i < simplices.size(); ++i )
  {
    n1.Set( simplices[i]._nPrev );
    n2.Set( simplices[i]._nNext );
    gp_XYZ normFace = ( n1 - n0 ) ^ ( n2 - n0 );
    double normLen2 = normFace.SquareModulus();
    if ( normLen2 < std::numeric_limits<double>::min() )
      continue;
    normFace /= Sqrt( normLen2 );
    newMinDot = Min( newNormal    * normFace, newMinDot );
    curMinDot = Min( edge._normal * normFace, curMinDot );
  }
  bool ok = true;
  if ( newMinDot < 0.5 )
  {
    ok = ( newMinDot >= curMinDot * 0.9 );
    //return ( newMinDot >= ( curMinDot * ( 0.8 + 0.1 * edge.NbSteps() )));
    // double initMinDot2 = 1. - edge._cosin * edge._cosin;
    // return ( newMinDot * newMinDot ) >= ( 0.8 * initMinDot2 );
  }

  return ok;
}

//================================================================================
/*!
 * \brief Modify normals of _LayerEdge's on FACE to reflex smoothing
 */
//================================================================================

bool _ViscousBuilder::updateNormalsOfSmoothed( _SolidData&         data,
                                               SMESH_MesherHelper& helper,
                                               const int           nbSteps,
                                               const double        stepSize )
{
  if ( data._nbShapesToSmooth == 0 || nbSteps == 0 )
    return true; // no shapes needing smoothing

  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[ iS ];
    if ( //!eos._toSmooth ||  _eosC1 have _toSmooth == false
         !eos._hyp.ToSmooth() ||
         eos.ShapeType() != TopAbs_FACE ||
         eos._edges.empty() )
      continue;

    bool toSmooth = ( eos._edges[ 0 ]->NbSteps() >= nbSteps+1 );
    if ( !toSmooth ) continue;

    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      _LayerEdge* edge = eos._edges[i];
      if ( !edge->Is( _LayerEdge::SMOOTHED ))
        continue;
      if ( edge->Is( _LayerEdge::DIFFICULT ) && nbSteps != 1 )
        continue;

      const gp_XYZ& pPrev = edge->PrevPos();
      const gp_XYZ& pLast = edge->_pos.back();
      gp_XYZ      stepVec = pLast - pPrev;
      double realStepSize = stepVec.Modulus();
      if ( realStepSize < numeric_limits<double>::min() )
        continue;

      edge->_lenFactor = realStepSize / stepSize;
      edge->_normal    = stepVec / realStepSize;
      edge->Set( _LayerEdge::NORMAL_UPDATED );
    }
  }

  return true;
}

//================================================================================
/*!
 * \brief Modify normals of _LayerEdge's on C1 VERTEXes
 */
//================================================================================

void _ViscousBuilder::updateNormalsOfC1Vertices( _SolidData& data )
{
  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eov = data._edgesOnShape[ iS ];
    if ( eov._eosC1.empty() ||
         eov.ShapeType() != TopAbs_VERTEX ||
         eov._edges.empty() )
      continue;

    gp_XYZ newNorm   = eov._edges[0]->_normal;
    double curThick  = eov._edges[0]->_len * eov._edges[0]->_lenFactor;
    bool normChanged = false;

    for ( size_t i = 0; i < eov._eosC1.size(); ++i )
    {
      _EdgesOnShape*   eoe = eov._eosC1[i];
      const TopoDS_Edge& e = TopoDS::Edge( eoe->_shape );
      const double    eLen = SMESH_Algo::EdgeLength( e );
      TopoDS_Shape    oppV = SMESH_MesherHelper::IthVertex( 0, e );
      if ( oppV.IsSame( eov._shape ))
        oppV = SMESH_MesherHelper::IthVertex( 1, e );
      _EdgesOnShape* eovOpp = data.GetShapeEdges( oppV );
      if ( !eovOpp || eovOpp->_edges.empty() ) continue;
      if ( eov._edges[0]->Is( _LayerEdge::BLOCKED )) continue;

      double curThickOpp = eovOpp->_edges[0]->_len * eovOpp->_edges[0]->_lenFactor;
      if ( curThickOpp + curThick < eLen )
        continue;

      double wgt = 2. * curThick / eLen;
      newNorm += wgt * eovOpp->_edges[0]->_normal;
      normChanged = true;
    }
    if ( normChanged )
    {
      eov._edges[0]->SetNormal( newNorm.Normalized() );
      eov._edges[0]->Set( _LayerEdge::NORMAL_UPDATED );
    }
  }
}

//================================================================================
/*!
 * \brief Modify normals of _LayerEdge's on _ConvexFace's
 */
//================================================================================

bool _ViscousBuilder::updateNormalsOfConvexFaces( _SolidData&         data,
                                                  SMESH_MesherHelper& helper,
                                                  int                 stepNb )
{
  SMESHDS_Mesh* meshDS = helper.GetMeshDS();
  bool isOK;

  map< TGeomID, _ConvexFace >::iterator id2face = data._convexFaces.begin();
  for ( ; id2face != data._convexFaces.end(); ++id2face )
  {
    _ConvexFace & convFace = (*id2face).second;
    if ( convFace._normalsFixed )
      continue; // already fixed
    if ( convFace.CheckPrisms() )
      continue; // nothing to fix

    convFace._normalsFixed = true;

    BRepAdaptor_Surface surface ( convFace._face, false );
    BRepLProp_SLProps   surfProp( surface, 2, 1e-6 );

    // check if the convex FACE is of spherical shape

    Bnd_B3d centersBox; // bbox of centers of curvature of _LayerEdge's on VERTEXes
    Bnd_B3d nodesBox;
    gp_Pnt  center;

    map< TGeomID, _EdgesOnShape* >::iterator id2eos = convFace._subIdToEOS.begin();
    for ( ; id2eos != convFace._subIdToEOS.end(); ++id2eos )
    {
      _EdgesOnShape& eos = *(id2eos->second);
      if ( eos.ShapeType() == TopAbs_VERTEX )
      {
        _LayerEdge* ledge = eos._edges[ 0 ];
        if ( convFace.GetCenterOfCurvature( ledge, surfProp, helper, center ))
          centersBox.Add( center );
      }
      for ( size_t i = 0; i < eos._edges.size(); ++i )
        nodesBox.Add( SMESH_TNodeXYZ( eos._edges[ i ]->_nodes[0] ));
    }
    if ( centersBox.IsVoid() )
    {
      debugMsg( "Error: centersBox.IsVoid()" );
      return false;
    }
    const bool isSpherical =
      ( centersBox.SquareExtent() < 1e-6 * nodesBox.SquareExtent() );

    int nbEdges = helper.Count( convFace._face, TopAbs_EDGE, /*ignoreSame=*/false );
    vector < _CentralCurveOnEdge > centerCurves( nbEdges );

    if ( isSpherical )
    {
      // set _LayerEdge::_normal as average of all normals

      // WARNING: different density of nodes on EDGEs is not taken into account that
      // can lead to an improper new normal

      gp_XYZ avgNormal( 0,0,0 );
      nbEdges = 0;
      id2eos = convFace._subIdToEOS.begin();
      for ( ; id2eos != convFace._subIdToEOS.end(); ++id2eos )
      {
        _EdgesOnShape& eos = *(id2eos->second);
        // set data of _CentralCurveOnEdge
        if ( eos.ShapeType() == TopAbs_EDGE )
        {
          _CentralCurveOnEdge& ceCurve = centerCurves[ nbEdges++ ];
          ceCurve.SetShapes( TopoDS::Edge( eos._shape ), convFace, data, helper );
          if ( !eos._sWOL.IsNull() )
            ceCurve._adjFace.Nullify();
          else
            ceCurve._ledges.insert( ceCurve._ledges.end(),
                                    eos._edges.begin(), eos._edges.end());
        }
        // summarize normals
        for ( size_t i = 0; i < eos._edges.size(); ++i )
          avgNormal += eos._edges[ i ]->_normal;
      }
      double normSize = avgNormal.SquareModulus();
      if ( normSize < 1e-200 )
      {
        debugMsg( "updateNormalsOfConvexFaces(): zero avgNormal" );
        return false;
      }
      avgNormal /= Sqrt( normSize );

      // compute new _LayerEdge::_cosin on EDGEs
      double avgCosin = 0;
      int     nbCosin = 0;
      gp_Vec inFaceDir;
      for ( size_t iE = 0; iE < centerCurves.size(); ++iE )
      {
        _CentralCurveOnEdge& ceCurve = centerCurves[ iE ];
        if ( ceCurve._adjFace.IsNull() )
          continue;
        for ( size_t iLE = 0; iLE < ceCurve._ledges.size(); ++iLE )
        {
          const SMDS_MeshNode* node = ceCurve._ledges[ iLE ]->_nodes[0];
          inFaceDir = getFaceDir( ceCurve._adjFace, ceCurve._edge, node, helper, isOK );
          if ( isOK )
          {
            double angle = inFaceDir.Angle( avgNormal ); // [0,PI]
            ceCurve._ledges[ iLE ]->_cosin = Cos( angle );
            avgCosin += ceCurve._ledges[ iLE ]->_cosin;
            nbCosin++;
          }
        }
      }
      if ( nbCosin > 0 )
        avgCosin /= nbCosin;

      // set _LayerEdge::_normal = avgNormal
      id2eos = convFace._subIdToEOS.begin();
      for ( ; id2eos != convFace._subIdToEOS.end(); ++id2eos )
      {
        _EdgesOnShape& eos = *(id2eos->second);
        if ( eos.ShapeType() != TopAbs_EDGE )
          for ( size_t i = 0; i < eos._edges.size(); ++i )
            eos._edges[ i ]->_cosin = avgCosin;

        for ( size_t i = 0; i < eos._edges.size(); ++i )
        {
          eos._edges[ i ]->SetNormal( avgNormal );
          eos._edges[ i ]->Set( _LayerEdge::NORMAL_UPDATED );
        }
      }
    }
    else // if ( isSpherical )
    {
      // We suppose that centers of curvature at all points of the FACE
      // lie on some curve, let's call it "central curve". For all _LayerEdge's
      // having a common center of curvature we define the same new normal
      // as a sum of normals of _LayerEdge's on EDGEs among them.

      // get all centers of curvature for each EDGE

      helper.SetSubShape( convFace._face );
      _LayerEdge* vertexLEdges[2], **edgeLEdge, **edgeLEdgeEnd;

      TopExp_Explorer edgeExp( convFace._face, TopAbs_EDGE );
      for ( int iE = 0; edgeExp.More(); edgeExp.Next(), ++iE )
      {
        const TopoDS_Edge& edge = TopoDS::Edge( edgeExp.Current() );

        // set adjacent FACE
        centerCurves[ iE ].SetShapes( edge, convFace, data, helper );

        // get _LayerEdge's of the EDGE
        TGeomID edgeID = meshDS->ShapeToIndex( edge );
        _EdgesOnShape* eos = data.GetShapeEdges( edgeID );
        if ( !eos || eos->_edges.empty() )
        {
          // no _LayerEdge's on EDGE, use _LayerEdge's on VERTEXes
          for ( int iV = 0; iV < 2; ++iV )
          {
            TopoDS_Vertex v = helper.IthVertex( iV, edge );
            TGeomID     vID = meshDS->ShapeToIndex( v );
            eos = data.GetShapeEdges( vID );
            vertexLEdges[ iV ] = eos->_edges[ 0 ];
          }
          edgeLEdge    = &vertexLEdges[0];
          edgeLEdgeEnd = edgeLEdge + 2;

          centerCurves[ iE ]._adjFace.Nullify();
        }
        else
        {
          if ( ! eos->_toSmooth )
            data.SortOnEdge( edge, eos->_edges );
          edgeLEdge    = &eos->_edges[ 0 ];
          edgeLEdgeEnd = edgeLEdge + eos->_edges.size();
          vertexLEdges[0] = eos->_edges.front()->_2neibors->_edges[0];
          vertexLEdges[1] = eos->_edges.back() ->_2neibors->_edges[1];

          if ( ! eos->_sWOL.IsNull() )
            centerCurves[ iE ]._adjFace.Nullify();
        }

        // Get curvature centers

        centersBox.Clear();

        if ( edgeLEdge[0]->IsOnEdge() &&
             convFace.GetCenterOfCurvature( vertexLEdges[0], surfProp, helper, center ))
        { // 1st VERTEX
          centerCurves[ iE ].Append( center, vertexLEdges[0] );
          centersBox.Add( center );
        }
        for ( ; edgeLEdge < edgeLEdgeEnd; ++edgeLEdge )
          if ( convFace.GetCenterOfCurvature( *edgeLEdge, surfProp, helper, center ))
          { // EDGE or VERTEXes
            centerCurves[ iE ].Append( center, *edgeLEdge );
            centersBox.Add( center );
          }
        if ( edgeLEdge[-1]->IsOnEdge() &&
             convFace.GetCenterOfCurvature( vertexLEdges[1], surfProp, helper, center ))
        { // 2nd VERTEX
          centerCurves[ iE ].Append( center, vertexLEdges[1] );
          centersBox.Add( center );
        }
        centerCurves[ iE ]._isDegenerated =
          ( centersBox.IsVoid() || centersBox.SquareExtent() < 1e-6 * nodesBox.SquareExtent() );

      } // loop on EDGES of convFace._face to set up data of centerCurves

      // Compute new normals for _LayerEdge's on EDGEs

      double avgCosin = 0;
      int     nbCosin = 0;
      gp_Vec inFaceDir;
      for ( size_t iE1 = 0; iE1 < centerCurves.size(); ++iE1 )
      {
        _CentralCurveOnEdge& ceCurve = centerCurves[ iE1 ];
        if ( ceCurve._isDegenerated )
          continue;
        const vector< gp_Pnt >& centers = ceCurve._curvaCenters;
        vector< gp_XYZ > &   newNormals = ceCurve._normals;
        for ( size_t iC1 = 0; iC1 < centers.size(); ++iC1 )
        {
          isOK = false;
          for ( size_t iE2 = 0; iE2 < centerCurves.size() && !isOK; ++iE2 )
          {
            if ( iE1 != iE2 )
              isOK = centerCurves[ iE2 ].FindNewNormal( centers[ iC1 ], newNormals[ iC1 ]);
          }
          if ( isOK && !ceCurve._adjFace.IsNull() )
          {
            // compute new _LayerEdge::_cosin
            const SMDS_MeshNode* node = ceCurve._ledges[ iC1 ]->_nodes[0];
            inFaceDir = getFaceDir( ceCurve._adjFace, ceCurve._edge, node, helper, isOK );
            if ( isOK )
            {
              double angle = inFaceDir.Angle( newNormals[ iC1 ] ); // [0,PI]
              ceCurve._ledges[ iC1 ]->_cosin = Cos( angle );
              avgCosin += ceCurve._ledges[ iC1 ]->_cosin;
              nbCosin++;
            }
          }
        }
      }
      // set new normals to _LayerEdge's of NOT degenerated central curves
      for ( size_t iE = 0; iE < centerCurves.size(); ++iE )
      {
        if ( centerCurves[ iE ]._isDegenerated )
          continue;
        for ( size_t iLE = 0; iLE < centerCurves[ iE ]._ledges.size(); ++iLE )
        {
          centerCurves[ iE ]._ledges[ iLE ]->SetNormal( centerCurves[ iE ]._normals[ iLE ]);
          centerCurves[ iE ]._ledges[ iLE ]->Set( _LayerEdge::NORMAL_UPDATED );
        }
      }
      // set new normals to _LayerEdge's of     degenerated central curves
      for ( size_t iE = 0; iE < centerCurves.size(); ++iE )
      {
        if ( !centerCurves[ iE ]._isDegenerated ||
             centerCurves[ iE ]._ledges.size() < 3 )
          continue;
        // new normal is an average of new normals at VERTEXes that
        // was computed on non-degenerated _CentralCurveOnEdge's
        gp_XYZ newNorm = ( centerCurves[ iE ]._ledges.front()->_normal +
                           centerCurves[ iE ]._ledges.back ()->_normal );
        double sz = newNorm.Modulus();
        if ( sz < 1e-200 )
          continue;
        newNorm /= sz;
        double newCosin = ( 0.5 * centerCurves[ iE ]._ledges.front()->_cosin +
                            0.5 * centerCurves[ iE ]._ledges.back ()->_cosin );
        for ( size_t iLE = 1, nb = centerCurves[ iE ]._ledges.size() - 1; iLE < nb; ++iLE )
        {
          centerCurves[ iE ]._ledges[ iLE ]->SetNormal( newNorm );
          centerCurves[ iE ]._ledges[ iLE ]->_cosin   = newCosin;
          centerCurves[ iE ]._ledges[ iLE ]->Set( _LayerEdge::NORMAL_UPDATED );
        }
      }

      // Find new normals for _LayerEdge's based on FACE

      if ( nbCosin > 0 )
        avgCosin /= nbCosin;
      const TGeomID faceID = meshDS->ShapeToIndex( convFace._face );
      map< TGeomID, _EdgesOnShape* >::iterator id2eos = convFace._subIdToEOS.find( faceID );
      if ( id2eos != convFace._subIdToEOS.end() )
      {
        int iE = 0;
        gp_XYZ newNorm;
        _EdgesOnShape& eos = * ( id2eos->second );
        for ( size_t i = 0; i < eos._edges.size(); ++i )
        {
          _LayerEdge* ledge = eos._edges[ i ];
          if ( !convFace.GetCenterOfCurvature( ledge, surfProp, helper, center ))
            continue;
          for ( size_t i = 0; i < centerCurves.size(); ++i, ++iE )
          {
            iE = iE % centerCurves.size();
            if ( centerCurves[ iE ]._isDegenerated )
              continue;
            newNorm.SetCoord( 0,0,0 );
            if ( centerCurves[ iE ].FindNewNormal( center, newNorm ))
            {
              ledge->SetNormal( newNorm );
              ledge->_cosin  = avgCosin;
              ledge->Set( _LayerEdge::NORMAL_UPDATED );
              break;
            }
          }
        }
      }

    } // not a quasi-spherical FACE

    // Update _LayerEdge's data according to a new normal

    dumpFunction(SMESH_Comment("updateNormalsOfConvexFaces")<<data._index
                 <<"_F"<<meshDS->ShapeToIndex( convFace._face ));

    id2eos = convFace._subIdToEOS.begin();
    for ( ; id2eos != convFace._subIdToEOS.end(); ++id2eos )
    {
      _EdgesOnShape& eos = * ( id2eos->second );
      for ( size_t i = 0; i < eos._edges.size(); ++i )
      {
        _LayerEdge* & ledge = eos._edges[ i ];
        double len = ledge->_len;
        ledge->InvalidateStep( stepNb + 1, eos, /*restoreLength=*/true );
        ledge->SetCosin( ledge->_cosin );
        ledge->SetNewLength( len, eos, helper );
      }
      if ( eos.ShapeType() != TopAbs_FACE )
        for ( size_t i = 0; i < eos._edges.size(); ++i )
        {
          _LayerEdge* ledge = eos._edges[ i ];
          for ( size_t iN = 0; iN < ledge->_neibors.size(); ++iN )
          {
            _LayerEdge* neibor = ledge->_neibors[iN];
            if ( neibor->_nodes[0]->GetPosition()->GetDim() == 2 )
            {
              neibor->Set( _LayerEdge::NEAR_BOUNDARY );
              neibor->Set( _LayerEdge::MOVED );
              neibor->SetSmooLen( neibor->_len );
            }
          }
        }
    } // loop on sub-shapes of convFace._face

    // Find FACEs adjacent to convFace._face that got necessity to smooth
    // as a result of normals modification

    set< _EdgesOnShape* > adjFacesToSmooth;
    for ( size_t iE = 0; iE < centerCurves.size(); ++iE )
    {
      if ( centerCurves[ iE ]._adjFace.IsNull() ||
           centerCurves[ iE ]._adjFaceToSmooth )
        continue;
      for ( size_t iLE = 0; iLE < centerCurves[ iE ]._ledges.size(); ++iLE )
      {
        if ( centerCurves[ iE ]._ledges[ iLE ]->_cosin > theMinSmoothCosin )
        {
          adjFacesToSmooth.insert( data.GetShapeEdges( centerCurves[ iE ]._adjFace ));
          break;
        }
      }
    }
    data.AddShapesToSmooth( adjFacesToSmooth );

    dumpFunctionEnd();


  } // loop on data._convexFaces

  return true;
}

//================================================================================
/*!
 * \brief Finds a center of curvature of a surface at a _LayerEdge
 */
//================================================================================

bool _ConvexFace::GetCenterOfCurvature( _LayerEdge*         ledge,
                                        BRepLProp_SLProps&  surfProp,
                                        SMESH_MesherHelper& helper,
                                        gp_Pnt &            center ) const
{
  gp_XY uv = helper.GetNodeUV( _face, ledge->_nodes[0] );
  surfProp.SetParameters( uv.X(), uv.Y() );
  if ( !surfProp.IsCurvatureDefined() )
    return false;

  const double oriFactor = ( _face.Orientation() == TopAbs_REVERSED ? +1. : -1. );
  double surfCurvatureMax = surfProp.MaxCurvature() * oriFactor;
  double surfCurvatureMin = surfProp.MinCurvature() * oriFactor;
  if ( surfCurvatureMin > surfCurvatureMax )
    center = surfProp.Value().Translated( surfProp.Normal().XYZ() / surfCurvatureMin * oriFactor );
  else
    center = surfProp.Value().Translated( surfProp.Normal().XYZ() / surfCurvatureMax * oriFactor );

  return true;
}

//================================================================================
/*!
 * \brief Check that prisms are not distorted
 */
//================================================================================

bool _ConvexFace::CheckPrisms() const
{
  double vol = 0;
  for ( size_t i = 0; i < _simplexTestEdges.size(); ++i )
  {
    const _LayerEdge* edge = _simplexTestEdges[i];
    SMESH_TNodeXYZ tgtXYZ( edge->_nodes.back() );
    for ( size_t j = 0; j < edge->_simplices.size(); ++j )
      if ( !edge->_simplices[j].IsForward( edge->_nodes[0], tgtXYZ, vol ))
      {
        debugMsg( "Bad simplex of _simplexTestEdges ("
                  << " "<< edge->_nodes[0]->GetID()<< " "<< tgtXYZ._node->GetID()
                  << " "<< edge->_simplices[j]._nPrev->GetID()
                  << " "<< edge->_simplices[j]._nNext->GetID() << " )" );
        return false;
      }
  }
  return true;
}

//================================================================================
/*!
 * \brief Try to compute a new normal by interpolating normals of _LayerEdge's
 *        stored in this _CentralCurveOnEdge.
 *  \param [in] center - curvature center of a point of another _CentralCurveOnEdge.
 *  \param [in,out] newNormal - current normal at this point, to be redefined
 *  \return bool - true if succeeded.
 */
//================================================================================

bool _CentralCurveOnEdge::FindNewNormal( const gp_Pnt& center, gp_XYZ& newNormal )
{
  if ( this->_isDegenerated )
    return false;

  // find two centers the given one lies between

  for ( size_t i = 0, nb = _curvaCenters.size()-1;  i < nb;  ++i )
  {
    double sl2 = 1.001 * _segLength2[ i ];

    double d1 = center.SquareDistance( _curvaCenters[ i ]);
    if ( d1 > sl2 )
      continue;
    
    double d2 = center.SquareDistance( _curvaCenters[ i+1 ]);
    if ( d2 > sl2 || d2 + d1 < 1e-100 )
      continue;

    d1 = Sqrt( d1 );
    d2 = Sqrt( d2 );
    double r = d1 / ( d1 + d2 );
    gp_XYZ norm = (( 1. - r ) * _ledges[ i   ]->_normal +
                   (      r ) * _ledges[ i+1 ]->_normal );
    norm.Normalize();

    newNormal += norm;
    double sz = newNormal.Modulus();
    if ( sz < 1e-200 )
      break;
    newNormal /= sz;
    return true;
  }
  return false;
}

//================================================================================
/*!
 * \brief Set shape members
 */
//================================================================================

void _CentralCurveOnEdge::SetShapes( const TopoDS_Edge&  edge,
                                     const _ConvexFace&  convFace,
                                     _SolidData&         data,
                                     SMESH_MesherHelper& helper)
{
  _edge = edge;

  PShapeIteratorPtr fIt = helper.GetAncestors( edge, *helper.GetMesh(), TopAbs_FACE );
  while ( const TopoDS_Shape* F = fIt->next())
    if ( !convFace._face.IsSame( *F ))
    {
      _adjFace = TopoDS::Face( *F );
      _adjFaceToSmooth = false;
      // _adjFace already in a smoothing queue ?
      if ( _EdgesOnShape* eos = data.GetShapeEdges( _adjFace ))
        _adjFaceToSmooth = eos->_toSmooth;
      break;
    }
}

//================================================================================
/*!
 * \brief Looks for intersection of it's last segment with faces
 *  \param distance - returns shortest distance from the last node to intersection
 */
//================================================================================

bool _LayerEdge::FindIntersection( SMESH_ElementSearcher&   searcher,
                                   double &                 distance,
                                   const double&            epsilon,
                                   _EdgesOnShape&           eos,
                                   const SMDS_MeshElement** intFace)
{
  vector< const SMDS_MeshElement* > suspectFaces;
  double segLen;
  gp_Ax1 lastSegment = LastSegment( segLen, eos );
  searcher.GetElementsNearLine( lastSegment, SMDSAbs_Face, suspectFaces );

  bool segmentIntersected = false;
  distance = Precision::Infinite();
  int iFace = -1; // intersected face
  for ( size_t j = 0 ; j < suspectFaces.size() /*&& !segmentIntersected*/; ++j )
  {
    const SMDS_MeshElement* face = suspectFaces[j];
    if ( face->GetNodeIndex( _nodes.back() ) >= 0 ||
         face->GetNodeIndex( _nodes[0]     ) >= 0 )
      continue; // face sharing _LayerEdge node
    const int nbNodes = face->NbCornerNodes();
    bool intFound = false;
    double dist;
    SMDS_MeshElement::iterator nIt = face->begin_nodes();
    if ( nbNodes == 3 )
    {
      intFound = SegTriaInter( lastSegment, *nIt++, *nIt++, *nIt++, dist, epsilon );
    }
    else
    {
      const SMDS_MeshNode* tria[3];
      tria[0] = *nIt++;
      tria[1] = *nIt++;
      for ( int n2 = 2; n2 < nbNodes && !intFound; ++n2 )
      {
        tria[2] = *nIt++;
        intFound = SegTriaInter(lastSegment, tria[0], tria[1], tria[2], dist, epsilon );
        tria[1] = tria[2];
      }
    }
    if ( intFound )
    {
      if ( dist < segLen*(1.01) && dist > -(_len*_lenFactor-segLen) )
        segmentIntersected = true;
      if ( distance > dist )
        distance = dist, iFace = j;
    }
  }
  if ( intFace ) *intFace = ( iFace != -1 ) ? suspectFaces[iFace] : 0;

  distance -= segLen;

  if ( segmentIntersected )
  {
#ifdef __myDEBUG
    SMDS_MeshElement::iterator nIt = suspectFaces[iFace]->begin_nodes();
    gp_XYZ intP( lastSegment.Location().XYZ() + lastSegment.Direction().XYZ() * ( distance+segLen ));
    cout << "nodes: tgt " << _nodes.back()->GetID() << " src " << _nodes[0]->GetID()
         << ", intersection with face ("
         << (*nIt++)->GetID()<<" "<< (*nIt++)->GetID()<<" "<< (*nIt++)->GetID()
         << ") at point (" << intP.X() << ", " << intP.Y() << ", " << intP.Z()
         << ") distance = " << distance << endl;
#endif
  }

  return segmentIntersected;
}

//================================================================================
/*!
 * \brief Returns a point used to check orientation of _simplices
 */
//================================================================================

gp_XYZ _LayerEdge::PrevCheckPos( _EdgesOnShape* eos ) const
{
  size_t i = Is( NORMAL_UPDATED ) ? _pos.size()-2 : 0;

  if ( !eos || eos->_sWOL.IsNull() )
    return _pos[ i ];

  if ( eos->SWOLType() == TopAbs_EDGE )
  {
    return BRepAdaptor_Curve( TopoDS::Edge( eos->_sWOL )).Value( _pos[i].X() ).XYZ();
  }
  //else //  TopAbs_FACE

  return BRepAdaptor_Surface( TopoDS::Face( eos->_sWOL )).Value(_pos[i].X(), _pos[i].Y() ).XYZ();
}

//================================================================================
/*!
 * \brief Returns size and direction of the last segment
 */
//================================================================================

gp_Ax1 _LayerEdge::LastSegment(double& segLen, _EdgesOnShape& eos) const
{
  // find two non-coincident positions
  gp_XYZ orig = _pos.back();
  gp_XYZ vec;
  int iPrev = _pos.size() - 2;
  //const double tol = ( _len > 0 ) ? 0.3*_len : 1e-100; // adjusted for IPAL52478 + PAL22576
  const double tol = ( _len > 0 ) ? ( 1e-6 * _len ) : 1e-100;
  while ( iPrev >= 0 )
  {
    vec = orig - _pos[iPrev];
    if ( vec.SquareModulus() > tol*tol )
      break;
    else
      iPrev--;
  }

  // make gp_Ax1
  gp_Ax1 segDir;
  if ( iPrev < 0 )
  {
    segDir.SetLocation( SMESH_TNodeXYZ( _nodes[0] ));
    segDir.SetDirection( _normal );
    segLen = 0;
  }
  else
  {
    gp_Pnt pPrev = _pos[ iPrev ];
    if ( !eos._sWOL.IsNull() )
    {
      TopLoc_Location loc;
      if ( eos.SWOLType() == TopAbs_EDGE )
      {
        double f,l;
        Handle(Geom_Curve) curve = BRep_Tool::Curve( TopoDS::Edge( eos._sWOL ), loc, f,l);
        pPrev = curve->Value( pPrev.X() ).Transformed( loc );
      }
      else
      {
        Handle(Geom_Surface) surface = BRep_Tool::Surface( TopoDS::Face( eos._sWOL ), loc );
        pPrev = surface->Value( pPrev.X(), pPrev.Y() ).Transformed( loc );
      }
      vec = SMESH_TNodeXYZ( _nodes.back() ) - pPrev.XYZ();
    }
    segDir.SetLocation( pPrev );
    segDir.SetDirection( vec );
    segLen = vec.Modulus();
  }

  return segDir;
}

//================================================================================
/*!
 * \brief Return the last position of the target node on a FACE. 
 *  \param [in] F - the FACE this _LayerEdge is inflated along
 *  \return gp_XY - result UV
 */
//================================================================================

gp_XY _LayerEdge::LastUV( const TopoDS_Face& F, _EdgesOnShape& eos ) const
{
  if ( F.IsSame( eos._sWOL )) // F is my FACE
    return gp_XY( _pos.back().X(), _pos.back().Y() );

  if ( eos.SWOLType() != TopAbs_EDGE ) // wrong call
    return gp_XY( 1e100, 1e100 );

  // _sWOL is EDGE of F; _pos.back().X() is the last U on the EDGE
  double f, l, u = _pos.back().X();
  Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface( TopoDS::Edge(eos._sWOL), F, f,l);
  if ( !C2d.IsNull() && f <= u && u <= l )
    return C2d->Value( u ).XY();

  return gp_XY( 1e100, 1e100 );
}

//================================================================================
/*!
 * \brief Test intersection of the last segment with a given triangle
 *   using Moller-Trumbore algorithm
 * Intersection is detected if distance to intersection is less than _LayerEdge._len
 */
//================================================================================

bool _LayerEdge::SegTriaInter( const gp_Ax1& lastSegment,
                               const gp_XYZ& vert0,
                               const gp_XYZ& vert1,
                               const gp_XYZ& vert2,
                               double&       t,
                               const double& EPSILON) const
{
  const gp_Pnt& orig = lastSegment.Location();
  const gp_Dir& dir  = lastSegment.Direction();

  /* calculate distance from vert0 to ray origin */
  //gp_XYZ tvec = orig.XYZ() - vert0;

  //if ( tvec * dir > EPSILON )
    // intersected face is at back side of the temporary face this _LayerEdge belongs to
    //return false;

  gp_XYZ edge1 = vert1 - vert0;
  gp_XYZ edge2 = vert2 - vert0;

  /* begin calculating determinant - also used to calculate U parameter */
  gp_XYZ pvec = dir.XYZ() ^ edge2;

  /* if determinant is near zero, ray lies in plane of triangle */
  double det = edge1 * pvec;

  const double ANGL_EPSILON = 1e-12;
  if ( det > -ANGL_EPSILON && det < ANGL_EPSILON )
    return false;

  /* calculate distance from vert0 to ray origin */
  gp_XYZ tvec = orig.XYZ() - vert0;

  /* calculate U parameter and test bounds */
  double u = ( tvec * pvec ) / det;
  //if (u < 0.0 || u > 1.0)
  if ( u < -EPSILON || u > 1.0 + EPSILON )
    return false;

  /* prepare to test V parameter */
  gp_XYZ qvec = tvec ^ edge1;

  /* calculate V parameter and test bounds */
  double v = (dir.XYZ() * qvec) / det;
  //if ( v < 0.0 || u + v > 1.0 )
  if ( v < -EPSILON || u + v > 1.0 + EPSILON )
    return false;

  /* calculate t, ray intersects triangle */
  t = (edge2 * qvec) / det;

  //return true;
  return t > 0.;
}

//================================================================================
/*!
 * \brief _LayerEdge, located at a concave VERTEX of a FACE, moves target nodes of
 *        neighbor _LayerEdge's by it's own inflation vector.
 *  \param [in] eov - EOS of the VERTEX
 *  \param [in] eos - EOS of the FACE
 *  \param [in] step - inflation step
 *  \param [in,out] badSmooEdges - not untangled _LayerEdge's
 */
//================================================================================

void _LayerEdge::MoveNearConcaVer( const _EdgesOnShape*    eov,
                                   const _EdgesOnShape*    eos,
                                   const int               step,
                                   vector< _LayerEdge* > & badSmooEdges )
{
  // check if any of _neibors is in badSmooEdges
  if ( std::find_first_of( _neibors.begin(), _neibors.end(),
                           badSmooEdges.begin(), badSmooEdges.end() ) == _neibors.end() )
    return;

  // get all edges to move

  set< _LayerEdge* > edges;

  // find a distance between _LayerEdge on VERTEX and its neighbors
  gp_XYZ  curPosV = SMESH_TNodeXYZ( _nodes.back() );
  double dist2 = 0;
  for ( size_t i = 0; i < _neibors.size(); ++i )
  {
    _LayerEdge* nEdge = _neibors[i];
    if ( nEdge->_nodes[0]->getshapeId() == eos->_shapeID )
    {
      edges.insert( nEdge );
      dist2 = Max( dist2, ( curPosV - nEdge->_pos.back() ).SquareModulus() );
    }
  }
  // add _LayerEdge's close to curPosV
  size_t nbE;
  do {
    nbE = edges.size();
    for ( set< _LayerEdge* >::iterator e = edges.begin(); e != edges.end(); ++e )
    {
      _LayerEdge* edgeF = *e;
      for ( size_t i = 0; i < edgeF->_neibors.size(); ++i )
      {
        _LayerEdge* nEdge = edgeF->_neibors[i];
        if ( nEdge->_nodes[0]->getshapeId() == eos->_shapeID &&
             dist2 > ( curPosV - nEdge->_pos.back() ).SquareModulus() )
          edges.insert( nEdge );
      }
    }
  }
  while ( nbE < edges.size() );

  // move the target node of the got edges

  gp_XYZ prevPosV = PrevPos();
  if ( eov->SWOLType() == TopAbs_EDGE )
  {
    BRepAdaptor_Curve curve ( TopoDS::Edge( eov->_sWOL ));
    prevPosV = curve.Value( prevPosV.X() ).XYZ();
  }
  else if ( eov->SWOLType() == TopAbs_FACE )
  {
    BRepAdaptor_Surface surface( TopoDS::Face( eov->_sWOL ));
    prevPosV = surface.Value( prevPosV.X(), prevPosV.Y() ).XYZ();
  }

  SMDS_FacePosition* fPos;
  //double r = 1. - Min( 0.9, step / 10. );
  for ( set< _LayerEdge* >::iterator e = edges.begin(); e != edges.end(); ++e )
  {
    _LayerEdge*       edgeF = *e;
    const gp_XYZ     prevVF = edgeF->PrevPos() - prevPosV;
    const gp_XYZ    newPosF = curPosV + prevVF;
    SMDS_MeshNode* tgtNodeF = const_cast<SMDS_MeshNode*>( edgeF->_nodes.back() );
    tgtNodeF->setXYZ( newPosF.X(), newPosF.Y(), newPosF.Z() );
    edgeF->_pos.back() = newPosF;
    dumpMoveComm( tgtNodeF, "MoveNearConcaVer" ); // debug

    // set _curvature to make edgeF updated by putOnOffsetSurface()
    if ( !edgeF->_curvature )
      if (( fPos = dynamic_cast<SMDS_FacePosition*>( edgeF->_nodes[0]->GetPosition() )))
      {
        edgeF->_curvature = new _Curvature;
        edgeF->_curvature->_r = 0;
        edgeF->_curvature->_k = 0;
        edgeF->_curvature->_h2lenRatio = 0;
        edgeF->_curvature->_uv.SetCoord( fPos->GetUParameter(), fPos->GetVParameter() );
      }
  }
  // gp_XYZ inflationVec( SMESH_TNodeXYZ( _nodes.back() ) -
  //                      SMESH_TNodeXYZ( _nodes[0]    ));
  // for ( set< _LayerEdge* >::iterator e = edges.begin(); e != edges.end(); ++e )
  // {
  //   _LayerEdge*      edgeF = *e;
  //   gp_XYZ          newPos = SMESH_TNodeXYZ( edgeF->_nodes[0] ) + inflationVec;
  //   SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( edgeF->_nodes.back() );
  //   tgtNode->setXYZ( newPos.X(), newPos.Y(), newPos.Z() );
  //   edgeF->_pos.back() = newPosF;
  //   dumpMoveComm( tgtNode, "MoveNearConcaVer" ); // debug
  // }

  // smooth _LayerEdge's around moved nodes
  //size_t nbBadBefore = badSmooEdges.size();
  for ( set< _LayerEdge* >::iterator e = edges.begin(); e != edges.end(); ++e )
  {
    _LayerEdge* edgeF = *e;
    for ( size_t j = 0; j < edgeF->_neibors.size(); ++j )
      if ( edgeF->_neibors[j]->_nodes[0]->getshapeId() == eos->_shapeID )
        //&& !edges.count( edgeF->_neibors[j] ))
      {
        _LayerEdge* edgeFN = edgeF->_neibors[j];
        edgeFN->Unset( SMOOTHED );
        int nbBad = edgeFN->Smooth( step, /*isConcaFace=*/true, /*findBest=*/true );
        // if ( nbBad > 0 )
        // {
        //   gp_XYZ         newPos = SMESH_TNodeXYZ( edgeFN->_nodes[0] ) + inflationVec;
        //   const gp_XYZ& prevPos = edgeFN->_pos[ edgeFN->_pos.size()-2 ];
        //   int        nbBadAfter = edgeFN->_simplices.size();
        //   double vol;
        //   for ( size_t iS = 0; iS < edgeFN->_simplices.size(); ++iS )
        //   {
        //     nbBadAfter -= edgeFN->_simplices[iS].IsForward( &prevPos, &newPos, vol );
        //   }
        //   if ( nbBadAfter <= nbBad )
        //   {
        //     SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( edgeFN->_nodes.back() );
        //     tgtNode->setXYZ( newPos.X(), newPos.Y(), newPos.Z() );
        //     edgeF->_pos.back() = newPosF;
        //     dumpMoveComm( tgtNode, "MoveNearConcaVer 2" ); // debug
        //     nbBad = nbBadAfter;
        //   }
        // }
        if ( nbBad > 0 )
          badSmooEdges.push_back( edgeFN );
      }
  }
    // move a bit not smoothed around moved nodes
  //   for ( size_t i = nbBadBefore; i < badSmooEdges.size(); ++i )
  //   {
  //   _LayerEdge*      edgeF = badSmooEdges[i];
  //   SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( edgeF->_nodes.back() );
  //   gp_XYZ         newPos1 = SMESH_TNodeXYZ( edgeF->_nodes[0] ) + inflationVec;
  //   gp_XYZ         newPos2 = 0.5 * ( newPos1 + SMESH_TNodeXYZ( tgtNode ));
  //   tgtNode->setXYZ( newPos2.X(), newPos2.Y(), newPos2.Z() );
  //   edgeF->_pos.back() = newPosF;
  //   dumpMoveComm( tgtNode, "MoveNearConcaVer 2" ); // debug
  // }
}

//================================================================================
/*!
 * \brief Perform smooth of _LayerEdge's based on EDGE's
 *  \retval bool - true if node has been moved
 */
//================================================================================

bool _LayerEdge::SmoothOnEdge(Handle(ShapeAnalysis_Surface)& surface,
                              const TopoDS_Face&             F,
                              SMESH_MesherHelper&            helper)
{
  ASSERT( IsOnEdge() );

  SMDS_MeshNode* tgtNode = const_cast<SMDS_MeshNode*>( _nodes.back() );
  SMESH_TNodeXYZ oldPos( tgtNode );
  double dist01, distNewOld;
  
  SMESH_TNodeXYZ p0( _2neibors->tgtNode(0));
  SMESH_TNodeXYZ p1( _2neibors->tgtNode(1));
  dist01 = p0.Distance( _2neibors->tgtNode(1) );

  gp_Pnt newPos = p0 * _2neibors->_wgt[0] + p1 * _2neibors->_wgt[1];
  double lenDelta = 0;
  if ( _curvature )
  {
    //lenDelta = _curvature->lenDelta( _len );
    lenDelta = _curvature->lenDeltaByDist( dist01 );
    newPos.ChangeCoord() += _normal * lenDelta;
  }

  distNewOld = newPos.Distance( oldPos );

  if ( F.IsNull() )
  {
    if ( _2neibors->_plnNorm )
    {
      // put newPos on the plane defined by source node and _plnNorm
      gp_XYZ new2src = SMESH_TNodeXYZ( _nodes[0] ) - newPos.XYZ();
      double new2srcProj = (*_2neibors->_plnNorm) * new2src;
      newPos.ChangeCoord() += (*_2neibors->_plnNorm) * new2srcProj;
    }
    tgtNode->setXYZ( newPos.X(), newPos.Y(), newPos.Z() );
    _pos.back() = newPos.XYZ();
  }
  else
  {
    tgtNode->setXYZ( newPos.X(), newPos.Y(), newPos.Z() );
    gp_XY uv( Precision::Infinite(), 0 );
    helper.CheckNodeUV( F, tgtNode, uv, 1e-10, /*force=*/true );
    _pos.back().SetCoord( uv.X(), uv.Y(), 0 );

    newPos = surface->Value( uv );
    tgtNode->setXYZ( newPos.X(), newPos.Y(), newPos.Z() );
  }

  // commented for IPAL0052478
  // if ( _curvature && lenDelta < 0 )
  // {
  //   gp_Pnt prevPos( _pos[ _pos.size()-2 ]);
  //   _len -= prevPos.Distance( oldPos );
  //   _len += prevPos.Distance( newPos );
  // }
  bool moved = distNewOld > dist01/50;
  //if ( moved )
  dumpMove( tgtNode ); // debug

  return moved;
}

//================================================================================
/*!
 * \brief Perform 3D smooth of nodes inflated from FACE. No check of validity
 */
//================================================================================

void _LayerEdge::SmoothWoCheck()
{
  if ( Is( DIFFICULT ))
    return;

  bool moved = Is( SMOOTHED );
  for ( size_t i = 0; i < _neibors.size()  &&  !moved; ++i )
    moved = _neibors[i]->Is( SMOOTHED );
  if ( !moved )
    return;

  gp_XYZ newPos = (this->*_smooFunction)(); // fun chosen by ChooseSmooFunction()

  SMDS_MeshNode* n = const_cast< SMDS_MeshNode* >( _nodes.back() );
  n->setXYZ( newPos.X(), newPos.Y(), newPos.Z());
  _pos.back() = newPos;

  dumpMoveComm( n, SMESH_Comment("No check - ") << _funNames[ smooFunID() ]);
}

//================================================================================
/*!
 * \brief Checks validity of _neibors on EDGEs and VERTEXes
 */
//================================================================================

int _LayerEdge::CheckNeiborsOnBoundary( vector< _LayerEdge* >* badNeibors, bool * needSmooth )
{
  if ( ! Is( NEAR_BOUNDARY ))
    return 0;

  int nbBad = 0;
  double vol;
  for ( size_t iN = 0; iN < _neibors.size(); ++iN )
  {
    _LayerEdge* eN = _neibors[iN];
    if ( eN->_nodes[0]->getshapeId() == _nodes[0]->getshapeId() )
      continue;
    if ( needSmooth )
      *needSmooth |= ( eN->Is( _LayerEdge::BLOCKED ) ||
                       eN->Is( _LayerEdge::NORMAL_UPDATED ) ||
                       eN->_pos.size() != _pos.size() );

    SMESH_TNodeXYZ curPosN ( eN->_nodes.back() );
    SMESH_TNodeXYZ prevPosN( eN->_nodes[0] );
    for ( size_t i = 0; i < eN->_simplices.size(); ++i )
      if ( eN->_nodes.size() > 1 &&
           eN->_simplices[i].Includes( _nodes.back() ) &&
           !eN->_simplices[i].IsForward( &prevPosN, &curPosN, vol ))
      {
        ++nbBad;
        if ( badNeibors )
        {
          badNeibors->push_back( eN );
          debugMsg("Bad boundary simplex ( "
                   << " "<< eN->_nodes[0]->GetID()
                   << " "<< eN->_nodes.back()->GetID()
                   << " "<< eN->_simplices[i]._nPrev->GetID()
                   << " "<< eN->_simplices[i]._nNext->GetID() << " )" );
        }
        else
        {
          break;
        }
      }
  }
  return nbBad;
}

//================================================================================
/*!
 * \brief Perform 'smart' 3D smooth of nodes inflated from FACE
 *  \retval int - nb of bad simplices around this _LayerEdge
 */
//================================================================================

int _LayerEdge::Smooth(const int step, bool findBest, vector< _LayerEdge* >& toSmooth )
{
  if ( !Is( MOVED ) || Is( SMOOTHED ) || Is( BLOCKED ))
    return 0; // shape of simplices not changed
  if ( _simplices.size() < 2 )
    return 0; // _LayerEdge inflated along EDGE or FACE

  if ( Is( DIFFICULT )) // || Is( ON_CONCAVE_FACE )
    findBest = true;

  const gp_XYZ& curPos  = _pos.back();
  const gp_XYZ& prevPos = _pos[0]; //PrevPos();

  // quality metrics (orientation) of tetras around _tgtNode
  int nbOkBefore = 0;
  double vol, minVolBefore = 1e100;
  for ( size_t i = 0; i < _simplices.size(); ++i )
  {
    nbOkBefore += _simplices[i].IsForward( &prevPos, &curPos, vol );
    minVolBefore = Min( minVolBefore, vol );
  }
  int nbBad = _simplices.size() - nbOkBefore;

  bool bndNeedSmooth = false;
  if ( nbBad == 0 )
    nbBad = CheckNeiborsOnBoundary( 0, & bndNeedSmooth );
  if ( nbBad > 0 )
    Set( DISTORTED );

  // evaluate min angle
  if ( nbBad == 0 && !findBest && !bndNeedSmooth )
  {
    size_t nbGoodAngles = _simplices.size();
    double angle;
    for ( size_t i = 0; i < _simplices.size(); ++i )
    {
      if ( !_simplices[i].IsMinAngleOK( curPos, angle ) && angle > _minAngle )
        --nbGoodAngles;
    }
    if ( nbGoodAngles == _simplices.size() )
    {
      Unset( MOVED );
      return 0;
    }
  }
  if ( Is( ON_CONCAVE_FACE ))
    findBest = true;

  if ( step % 2 == 0 )
    findBest = false;

  if ( Is( ON_CONCAVE_FACE ) && !findBest ) // alternate FUN_CENTROIDAL and FUN_LAPLACIAN
  {
    if ( _smooFunction == _funs[ FUN_LAPLACIAN ] )
      _smooFunction = _funs[ FUN_CENTROIDAL ];
    else
      _smooFunction = _funs[ FUN_LAPLACIAN ];
  }

  // compute new position for the last _pos using different _funs
  gp_XYZ newPos;
  bool moved = false;
  for ( int iFun = -1; iFun < theNbSmooFuns; ++iFun )
  {
    if ( iFun < 0 )
      newPos = (this->*_smooFunction)(); // fun chosen by ChooseSmooFunction()
    else if ( _funs[ iFun ] == _smooFunction )
      continue; // _smooFunction again
    else if ( step > 1 )
      newPos = (this->*_funs[ iFun ])(); // try other smoothing fun
    else
      break; // let "easy" functions improve elements around distorted ones

    if ( _curvature )
    {
      double delta  = _curvature->lenDelta( _len );
      if ( delta > 0 )
        newPos += _normal * delta;
      else
      {
        double segLen = _normal * ( newPos - prevPos );
        if ( segLen + delta > 0 )
          newPos += _normal * delta;
      }
      // double segLenChange = _normal * ( curPos - newPos );
      // newPos += 0.5 * _normal * segLenChange;
    }

    int nbOkAfter = 0;
    double minVolAfter = 1e100;
    for ( size_t i = 0; i < _simplices.size(); ++i )
    {
      nbOkAfter += _simplices[i].IsForward( &prevPos, &newPos, vol );
      minVolAfter = Min( minVolAfter, vol );
    }
    // get worse?
    if ( nbOkAfter < nbOkBefore )
      continue;

    if (( findBest ) &&
        ( nbOkAfter == nbOkBefore ) &&
        ( minVolAfter <= minVolBefore ))
      continue;

    nbBad        = _simplices.size() - nbOkAfter;
    minVolBefore = minVolAfter;
    nbOkBefore   = nbOkAfter;
    moved        = true;

    SMDS_MeshNode* n = const_cast< SMDS_MeshNode* >( _nodes.back() );
    n->setXYZ( newPos.X(), newPos.Y(), newPos.Z());
    _pos.back() = newPos;

    dumpMoveComm( n, SMESH_Comment( _funNames[ iFun < 0 ? smooFunID() : iFun ] )
                  << (nbBad ? " --BAD" : ""));

    if ( iFun > -1 )
    {
      continue; // look for a better function
    }

    if ( !findBest )
      break;

  } // loop on smoothing functions

  if ( moved ) // notify _neibors
  {
    Set( SMOOTHED );
    for ( size_t i = 0; i < _neibors.size(); ++i )
      if ( !_neibors[i]->Is( MOVED ))
      {
        _neibors[i]->Set( MOVED );
        toSmooth.push_back( _neibors[i] );
      }
  }

  return nbBad;
}

//================================================================================
/*!
 * \brief Perform 'smart' 3D smooth of nodes inflated from FACE
 *  \retval int - nb of bad simplices around this _LayerEdge
 */
//================================================================================

int _LayerEdge::Smooth(const int step, const bool isConcaveFace, bool findBest )
{
  if ( !_smooFunction )
    return 0; // _LayerEdge inflated along EDGE or FACE
  if ( Is( BLOCKED ))
    return 0; // not inflated

  const gp_XYZ& curPos  = _pos.back();
  const gp_XYZ& prevPos = _pos[0]; //PrevCheckPos();

  // quality metrics (orientation) of tetras around _tgtNode
  int nbOkBefore = 0;
  double vol, minVolBefore = 1e100;
  for ( size_t i = 0; i < _simplices.size(); ++i )
  {
    nbOkBefore += _simplices[i].IsForward( &prevPos, &curPos, vol );
    minVolBefore = Min( minVolBefore, vol );
  }
  int nbBad = _simplices.size() - nbOkBefore;

  if ( isConcaveFace ) // alternate FUN_CENTROIDAL and FUN_LAPLACIAN
  {
    if      ( _smooFunction == _funs[ FUN_CENTROIDAL ] && step % 2 )
      _smooFunction = _funs[ FUN_LAPLACIAN ];
    else if ( _smooFunction == _funs[ FUN_LAPLACIAN ] && !( step % 2 ))
      _smooFunction = _funs[ FUN_CENTROIDAL ];
  }

  // compute new position for the last _pos using different _funs
  gp_XYZ newPos;
  for ( int iFun = -1; iFun < theNbSmooFuns; ++iFun )
  {
    if ( iFun < 0 )
      newPos = (this->*_smooFunction)(); // fun chosen by ChooseSmooFunction()
    else if ( _funs[ iFun ] == _smooFunction )
      continue; // _smooFunction again
    else if ( step > 1 )
      newPos = (this->*_funs[ iFun ])(); // try other smoothing fun
    else
      break; // let "easy" functions improve elements around distorted ones

    if ( _curvature )
    {
      double delta  = _curvature->lenDelta( _len );
      if ( delta > 0 )
        newPos += _normal * delta;
      else
      {
        double segLen = _normal * ( newPos - prevPos );
        if ( segLen + delta > 0 )
          newPos += _normal * delta;
      }
      // double segLenChange = _normal * ( curPos - newPos );
      // newPos += 0.5 * _normal * segLenChange;
    }

    int nbOkAfter = 0;
    double minVolAfter = 1e100;
    for ( size_t i = 0; i < _simplices.size(); ++i )
    {
      nbOkAfter += _simplices[i].IsForward( &prevPos, &newPos, vol );
      minVolAfter = Min( minVolAfter, vol );
    }
    // get worse?
    if ( nbOkAfter < nbOkBefore )
      continue;
    if (( isConcaveFace || findBest ) &&
        ( nbOkAfter == nbOkBefore ) &&
        ( minVolAfter <= minVolBefore )
        )
      continue;

    nbBad        = _simplices.size() - nbOkAfter;
    minVolBefore = minVolAfter;
    nbOkBefore   = nbOkAfter;

    SMDS_MeshNode* n = const_cast< SMDS_MeshNode* >( _nodes.back() );
    n->setXYZ( newPos.X(), newPos.Y(), newPos.Z());
    _pos.back() = newPos;

    dumpMoveComm( n, SMESH_Comment( _funNames[ iFun < 0 ? smooFunID() : iFun ] )
                  << ( nbBad ? "--BAD" : ""));

    // commented for IPAL0052478
    // _len -= prevPos.Distance(SMESH_TNodeXYZ( n ));
    // _len += prevPos.Distance(newPos);

    if ( iFun > -1 ) // findBest || the chosen _fun makes worse
    {
      //_smooFunction = _funs[ iFun ];
      // cout << "# " << _funNames[ iFun ] << "\t N:" << _nodes.back()->GetID()
      // << "\t nbBad: " << _simplices.size() - nbOkAfter
      // << " minVol: " << minVolAfter
      // << " " << newPos.X() << " " << newPos.Y() << " " << newPos.Z()
      // << endl;
      continue; // look for a better function
    }

    if ( !findBest )
      break;

  } // loop on smoothing functions

  return nbBad;
}

//================================================================================
/*!
 * \brief Chooses a smoothing technic giving a position most close to an initial one.
 *        For a correct result, _simplices must contain nodes lying on geometry.
 */
//================================================================================

void _LayerEdge::ChooseSmooFunction( const set< TGeomID >& concaveVertices,
                                     const TNode2Edge&     n2eMap)
{
  if ( _smooFunction ) return;

  // use smoothNefPolygon() near concaveVertices
  if ( !concaveVertices.empty() )
  {
    _smooFunction = _funs[ FUN_CENTROIDAL ];

    Set( ON_CONCAVE_FACE );

    for ( size_t i = 0; i < _simplices.size(); ++i )
    {
      if ( concaveVertices.count( _simplices[i]._nPrev->getshapeId() ))
      {
        _smooFunction = _funs[ FUN_NEFPOLY ];

        // set FUN_CENTROIDAL to neighbor edges
        for ( i = 0; i < _neibors.size(); ++i )
        {
          if ( _neibors[i]->_nodes[0]->GetPosition()->GetDim() == 2 )
          {
            _neibors[i]->_smooFunction = _funs[ FUN_CENTROIDAL ];
          }
        }
        return;
      }
    }

    // // this coice is done only if ( !concaveVertices.empty() ) for Grids/smesh/bugs_19/X1
    // // where the nodes are smoothed too far along a sphere thus creating
    // // inverted _simplices
    // double dist[theNbSmooFuns];
    // //double coef[theNbSmooFuns] = { 1., 1.2, 1.4, 1.4 };
    // double coef[theNbSmooFuns] = { 1., 1., 1., 1. };

    // double minDist = Precision::Infinite();
    // gp_Pnt p = SMESH_TNodeXYZ( _nodes[0] );
    // for ( int i = 0; i < FUN_NEFPOLY; ++i )
    // {
    //   gp_Pnt newP = (this->*_funs[i])();
    //   dist[i] = p.SquareDistance( newP );
    //   if ( dist[i]*coef[i] < minDist )
    //   {
    //     _smooFunction = _funs[i];
    //     minDist = dist[i]*coef[i];
    //   }
    // }
  }
  else
  {
    _smooFunction = _funs[ FUN_LAPLACIAN ];
  }
  // int minDim = 3;
  // for ( size_t i = 0; i < _simplices.size(); ++i )
  //   minDim = Min( minDim, _simplices[i]._nPrev->GetPosition()->GetDim() );
  // if ( minDim == 0 )
  //   _smooFunction = _funs[ FUN_CENTROIDAL ];
  // else if ( minDim == 1 )
  //   _smooFunction = _funs[ FUN_CENTROIDAL ];


  // int iMin;
  // for ( int i = 0; i < FUN_NB; ++i )
  // {
  //   //cout << dist[i] << " ";
  //   if ( _smooFunction == _funs[i] ) {
  //     iMin = i;
  //     //debugMsg( fNames[i] );
  //     break;
  //   }
  // }
  // cout << _funNames[ iMin ] << "\t N:" << _nodes.back()->GetID() << endl;
}

//================================================================================
/*!
 * \brief Returns a name of _SmooFunction
 */
//================================================================================

int _LayerEdge::smooFunID( _LayerEdge::PSmooFun fun) const
{
  if ( !fun )
    fun = _smooFunction;
  for ( int i = 0; i < theNbSmooFuns; ++i )
    if ( fun == _funs[i] )
      return i;

  return theNbSmooFuns;
}

//================================================================================
/*!
 * \brief Computes a new node position using Laplacian smoothing
 */
//================================================================================

gp_XYZ _LayerEdge::smoothLaplacian()
{
  gp_XYZ newPos (0,0,0);
  for ( size_t i = 0; i < _simplices.size(); ++i )
    newPos += SMESH_TNodeXYZ( _simplices[i]._nPrev );
  newPos /= _simplices.size();

  return newPos;
}

//================================================================================
/*!
 * \brief Computes a new node position using angular-based smoothing
 */
//================================================================================

gp_XYZ _LayerEdge::smoothAngular()
{
  vector< gp_Vec > edgeDir;  edgeDir. reserve( _simplices.size() + 1 );
  vector< double > edgeSize; edgeSize.reserve( _simplices.size()     );
  vector< gp_XYZ > points;   points.  reserve( _simplices.size() + 1 );

  gp_XYZ pPrev = SMESH_TNodeXYZ( _simplices.back()._nPrev );
  gp_XYZ pN( 0,0,0 );
  for ( size_t i = 0; i < _simplices.size(); ++i )
  {
    gp_XYZ p = SMESH_TNodeXYZ( _simplices[i]._nPrev );
    edgeDir.push_back( p - pPrev );
    edgeSize.push_back( edgeDir.back().Magnitude() );
    if ( edgeSize.back() < numeric_limits<double>::min() )
    {
      edgeDir.pop_back();
      edgeSize.pop_back();
    }
    else
    {
      edgeDir.back() /= edgeSize.back();
      points.push_back( p );
      pN += p;
    }
    pPrev = p;
  }
  edgeDir.push_back ( edgeDir[0] );
  edgeSize.push_back( edgeSize[0] );
  pN /= points.size();

  gp_XYZ newPos(0,0,0);
  double sumSize = 0;
  for ( size_t i = 0; i < points.size(); ++i )
  {
    gp_Vec toN    = pN - points[i];
    double toNLen = toN.Magnitude();
    if ( toNLen < numeric_limits<double>::min() )
    {
      newPos += pN;
      continue;
    }
    gp_Vec bisec    = edgeDir[i] + edgeDir[i+1];
    double bisecLen = bisec.SquareMagnitude();
    if ( bisecLen < numeric_limits<double>::min() )
    {
      gp_Vec norm = edgeDir[i] ^ toN;
      bisec = norm ^ edgeDir[i];
      bisecLen = bisec.SquareMagnitude();
    }
    bisecLen = Sqrt( bisecLen );
    bisec /= bisecLen;

#if 1
    gp_XYZ pNew = ( points[i] + bisec.XYZ() * toNLen ) * bisecLen;
    sumSize += bisecLen;
#else
    gp_XYZ pNew = ( points[i] + bisec.XYZ() * toNLen ) * ( edgeSize[i] + edgeSize[i+1] );
    sumSize += ( edgeSize[i] + edgeSize[i+1] );
#endif
    newPos += pNew;
  }
  newPos /= sumSize;

  // project newPos to an average plane

  gp_XYZ norm(0,0,0); // plane normal
  points.push_back( points[0] );
  for ( size_t i = 1; i < points.size(); ++i )
  {
    gp_XYZ vec1 = points[ i-1 ] - pN;
    gp_XYZ vec2 = points[ i   ] - pN;
    gp_XYZ cross = vec1 ^ vec2;
    try {
      cross.Normalize();
      if ( cross * norm < numeric_limits<double>::min() )
        norm += cross.Reversed();
      else
        norm += cross;
    }
    catch (Standard_Failure) { // if |cross| == 0.
    }
  }
  gp_XYZ vec = newPos - pN;
  double r   = ( norm * vec ) / norm.SquareModulus();  // param [0,1] on norm
  newPos     = newPos - r * norm;

  return newPos;
}

//================================================================================
/*!
 * \brief Computes a new node position using weigthed node positions
 */
//================================================================================

gp_XYZ _LayerEdge::smoothLengthWeighted()
{
  vector< double > edgeSize; edgeSize.reserve( _simplices.size() + 1);
  vector< gp_XYZ > points;   points.  reserve( _simplices.size() );

  gp_XYZ pPrev = SMESH_TNodeXYZ( _simplices.back()._nPrev );
  for ( size_t i = 0; i < _simplices.size(); ++i )
  {
    gp_XYZ p = SMESH_TNodeXYZ( _simplices[i]._nPrev );
    edgeSize.push_back( ( p - pPrev ).Modulus() );
    if ( edgeSize.back() < numeric_limits<double>::min() )
    {
      edgeSize.pop_back();
    }
    else
    {
      points.push_back( p );
    }
    pPrev = p;
  }
  edgeSize.push_back( edgeSize[0] );

  gp_XYZ newPos(0,0,0);
  double sumSize = 0;
  for ( size_t i = 0; i < points.size(); ++i )
  {
    newPos += points[i] * ( edgeSize[i] + edgeSize[i+1] );
    sumSize += edgeSize[i] + edgeSize[i+1];
  }
  newPos /= sumSize;
  return newPos;
}

//================================================================================
/*!
 * \brief Computes a new node position using angular-based smoothing
 */
//================================================================================

gp_XYZ _LayerEdge::smoothCentroidal()
{
  gp_XYZ newPos(0,0,0);
  gp_XYZ pN = SMESH_TNodeXYZ( _nodes.back() );
  double sumSize = 0;
  for ( size_t i = 0; i < _simplices.size(); ++i )
  {
    gp_XYZ p1 = SMESH_TNodeXYZ( _simplices[i]._nPrev );
    gp_XYZ p2 = SMESH_TNodeXYZ( _simplices[i]._nNext );
    gp_XYZ gc = ( pN + p1 + p2 ) / 3.;
    double size = (( p1 - pN ) ^ ( p2 - pN )).Modulus();

    sumSize += size;
    newPos += gc * size;
  }
  newPos /= sumSize;

  return newPos;
}

//================================================================================
/*!
 * \brief Computes a new node position located inside a Nef polygon
 */
//================================================================================

gp_XYZ _LayerEdge::smoothNefPolygon()
#ifdef OLD_NEF_POLYGON
{
  gp_XYZ newPos(0,0,0);

  // get a plane to search a solution on

  vector< gp_XYZ > vecs( _simplices.size() + 1 );
  size_t i;
  const double tol = numeric_limits<double>::min();
  gp_XYZ center(0,0,0);
  for ( i = 0; i < _simplices.size(); ++i )
  {
    vecs[i] = ( SMESH_TNodeXYZ( _simplices[i]._nNext ) -
                SMESH_TNodeXYZ( _simplices[i]._nPrev ));
    center += SMESH_TNodeXYZ( _simplices[i]._nPrev );
  }
  vecs.back() = vecs[0];
  center /= _simplices.size();

  gp_XYZ zAxis(0,0,0);
  for ( i = 0; i < _simplices.size(); ++i )
    zAxis += vecs[i] ^ vecs[i+1];

  gp_XYZ yAxis;
  for ( i = 0; i < _simplices.size(); ++i )
  {
    yAxis = vecs[i];
    if ( yAxis.SquareModulus() > tol )
      break;
  }
  gp_XYZ xAxis = yAxis ^ zAxis;
  // SMESH_TNodeXYZ p0( _simplices[0]._nPrev );
  // const double tol = 1e-6 * ( p0.Distance( _simplices[1]._nPrev ) +
  //                             p0.Distance( _simplices[2]._nPrev ));
  // gp_XYZ center = smoothLaplacian();
  // gp_XYZ xAxis, yAxis, zAxis;
  // for ( i = 0; i < _simplices.size(); ++i )
  // {
  //   xAxis = SMESH_TNodeXYZ( _simplices[i]._nPrev ) - center;
  //   if ( xAxis.SquareModulus() > tol*tol )
  //     break;
  // }
  // for ( i = 1; i < _simplices.size(); ++i )
  // {
  //   yAxis = SMESH_TNodeXYZ( _simplices[i]._nPrev ) - center;
  //   zAxis = xAxis ^ yAxis;
  //   if ( zAxis.SquareModulus() > tol*tol )
  //     break;
  // }
  // if ( i == _simplices.size() ) return newPos;

  yAxis = zAxis ^ xAxis;
  xAxis /= xAxis.Modulus();
  yAxis /= yAxis.Modulus();

  // get half-planes of _simplices

  vector< _halfPlane > halfPlns( _simplices.size() );
  int nbHP = 0;
  for ( size_t i = 0; i < _simplices.size(); ++i )
  {
    gp_XYZ OP1 = SMESH_TNodeXYZ( _simplices[i]._nPrev ) - center;
    gp_XYZ OP2 = SMESH_TNodeXYZ( _simplices[i]._nNext ) - center;
    gp_XY  p1( OP1 * xAxis, OP1 * yAxis );
    gp_XY  p2( OP2 * xAxis, OP2 * yAxis );
    gp_XY  vec12 = p2 - p1;
    double dist12 = vec12.Modulus();
    if ( dist12 < tol )
      continue;
    vec12 /= dist12;
    halfPlns[ nbHP ]._pos = p1;
    halfPlns[ nbHP ]._dir = vec12;
    halfPlns[ nbHP ]._inNorm.SetCoord( -vec12.Y(), vec12.X() );
    ++nbHP;
  }

  // intersect boundaries of half-planes, define state of intersection points
  // in relation to all half-planes and calculate internal point of a 2D polygon

  double sumLen = 0;
  gp_XY newPos2D (0,0);

  enum { UNDEF = -1, NOT_OUT, IS_OUT, NO_INT };
  typedef std::pair< gp_XY, int > TIntPntState; // coord and isOut state
  TIntPntState undefIPS( gp_XY(1e100,1e100), UNDEF );

  vector< vector< TIntPntState > > allIntPnts( nbHP );
  for ( int iHP1 = 0; iHP1 < nbHP; ++iHP1 )
  {
    vector< TIntPntState > & intPnts1 = allIntPnts[ iHP1 ];
    if ( intPnts1.empty() ) intPnts1.resize( nbHP, undefIPS );

    int iPrev = SMESH_MesherHelper::WrapIndex( iHP1 - 1, nbHP );
    int iNext = SMESH_MesherHelper::WrapIndex( iHP1 + 1, nbHP );

    int nbNotOut = 0;
    const gp_XY* segEnds[2] = { 0, 0 }; // NOT_OUT points

    for ( int iHP2 = 0; iHP2 < nbHP; ++iHP2 )
    {
      if ( iHP1 == iHP2 ) continue;

      TIntPntState & ips1 = intPnts1[ iHP2 ];
      if ( ips1.second == UNDEF )
      {
        // find an intersection point of boundaries of iHP1 and iHP2

        if ( iHP2 == iPrev ) // intersection with neighbors is known
          ips1.first = halfPlns[ iHP1 ]._pos;
        else if ( iHP2 == iNext )
          ips1.first = halfPlns[ iHP2 ]._pos;
        else if ( !halfPlns[ iHP1 ].FindIntersection( halfPlns[ iHP2 ], ips1.first ))
          ips1.second = NO_INT;

        // classify the found intersection point
        if ( ips1.second != NO_INT )
        {
          ips1.second = NOT_OUT;
          for ( int i = 0; i < nbHP && ips1.second == NOT_OUT; ++i )
            if ( i != iHP1 && i != iHP2 &&
                 halfPlns[ i ].IsOut( ips1.first, tol ))
              ips1.second = IS_OUT;
        }
        vector< TIntPntState > & intPnts2 = allIntPnts[ iHP2 ];
        if ( intPnts2.empty() ) intPnts2.resize( nbHP, undefIPS );
        TIntPntState & ips2 = intPnts2[ iHP1 ];
        ips2 = ips1;
      }
      if ( ips1.second == NOT_OUT )
      {
        ++nbNotOut;
        segEnds[ bool(segEnds[0]) ] = & ips1.first;
      }
    }

    // find a NOT_OUT segment of boundary which is located between
    // two NOT_OUT int points

    if ( nbNotOut < 2 )
      continue; // no such a segment

    if ( nbNotOut > 2 )
    {
      // sort points along the boundary
      map< double, TIntPntState* > ipsByParam;
      for ( int iHP2 = 0; iHP2 < nbHP; ++iHP2 )
      {
        TIntPntState & ips1 = intPnts1[ iHP2 ];
        if ( ips1.second != NO_INT )
        {
          gp_XY     op = ips1.first - halfPlns[ iHP1 ]._pos;
          double param = op * halfPlns[ iHP1 ]._dir;
          ipsByParam.insert( make_pair( param, & ips1 ));
        }
      }
      // look for two neighboring NOT_OUT points
      nbNotOut = 0;
      map< double, TIntPntState* >::iterator u2ips = ipsByParam.begin();
      for ( ; u2ips != ipsByParam.end(); ++u2ips )
      {
        TIntPntState & ips1 = *(u2ips->second);
        if ( ips1.second == NOT_OUT )
          segEnds[ bool( nbNotOut++ ) ] = & ips1.first;
        else if ( nbNotOut >= 2 )
          break;
        else
          nbNotOut = 0;
      }
    }

    if ( nbNotOut >= 2 )
    {
      double len = ( *segEnds[0] - *segEnds[1] ).Modulus();
      sumLen += len;

      newPos2D += 0.5 * len * ( *segEnds[0] + *segEnds[1] );
    }
  }

  if ( sumLen > 0 )
  {
    newPos2D /= sumLen;
    newPos = center + xAxis * newPos2D.X() + yAxis * newPos2D.Y();
  }
  else
  {
    newPos = center;
  }

  return newPos;
}
#else // OLD_NEF_POLYGON
{ ////////////////////////////////// NEW
  gp_XYZ newPos(0,0,0);

  // get a plane to search a solution on

  size_t i;
  gp_XYZ center(0,0,0);
  for ( i = 0; i < _simplices.size(); ++i )
    center += SMESH_TNodeXYZ( _simplices[i]._nPrev );
  center /= _simplices.size();

  vector< gp_XYZ > vecs( _simplices.size() + 1 );
  for ( i = 0; i < _simplices.size(); ++i )
    vecs[i] = SMESH_TNodeXYZ( _simplices[i]._nPrev ) - center;
  vecs.back() = vecs[0];

  const double tol = numeric_limits<double>::min();
  gp_XYZ zAxis(0,0,0);
  for ( i = 0; i < _simplices.size(); ++i )
  {
    gp_XYZ cross = vecs[i] ^ vecs[i+1];
    try {
      cross.Normalize();
      if ( cross * zAxis < tol )
        zAxis += cross.Reversed();
      else
        zAxis += cross;
    }
    catch (Standard_Failure) { // if |cross| == 0.
    }
  }

  gp_XYZ yAxis;
  for ( i = 0; i < _simplices.size(); ++i )
  {
    yAxis = vecs[i];
    if ( yAxis.SquareModulus() > tol )
      break;
  }
  gp_XYZ xAxis = yAxis ^ zAxis;
  // SMESH_TNodeXYZ p0( _simplices[0]._nPrev );
  // const double tol = 1e-6 * ( p0.Distance( _simplices[1]._nPrev ) +
  //                             p0.Distance( _simplices[2]._nPrev ));
  // gp_XYZ center = smoothLaplacian();
  // gp_XYZ xAxis, yAxis, zAxis;
  // for ( i = 0; i < _simplices.size(); ++i )
  // {
  //   xAxis = SMESH_TNodeXYZ( _simplices[i]._nPrev ) - center;
  //   if ( xAxis.SquareModulus() > tol*tol )
  //     break;
  // }
  // for ( i = 1; i < _simplices.size(); ++i )
  // {
  //   yAxis = SMESH_TNodeXYZ( _simplices[i]._nPrev ) - center;
  //   zAxis = xAxis ^ yAxis;
  //   if ( zAxis.SquareModulus() > tol*tol )
  //     break;
  // }
  // if ( i == _simplices.size() ) return newPos;

  yAxis = zAxis ^ xAxis;
  xAxis /= xAxis.Modulus();
  yAxis /= yAxis.Modulus();

  // get half-planes of _simplices

  vector< _halfPlane > halfPlns( _simplices.size() );
  int nbHP = 0;
  for ( size_t i = 0; i < _simplices.size(); ++i )
  {
    const gp_XYZ& OP1 = vecs[ i   ];
    const gp_XYZ& OP2 = vecs[ i+1 ];
    gp_XY  p1( OP1 * xAxis, OP1 * yAxis );
    gp_XY  p2( OP2 * xAxis, OP2 * yAxis );
    gp_XY  vec12 = p2 - p1;
    double dist12 = vec12.Modulus();
    if ( dist12 < tol )
      continue;
    vec12 /= dist12;
    halfPlns[ nbHP ]._pos = p1;
    halfPlns[ nbHP ]._dir = vec12;
    halfPlns[ nbHP ]._inNorm.SetCoord( -vec12.Y(), vec12.X() );
    ++nbHP;
  }

  // intersect boundaries of half-planes, define state of intersection points
  // in relation to all half-planes and calculate internal point of a 2D polygon

  double sumLen = 0;
  gp_XY newPos2D (0,0);

  enum { UNDEF = -1, NOT_OUT, IS_OUT, NO_INT };
  typedef std::pair< gp_XY, int > TIntPntState; // coord and isOut state
  TIntPntState undefIPS( gp_XY(1e100,1e100), UNDEF );

  vector< vector< TIntPntState > > allIntPnts( nbHP );
  for ( int iHP1 = 0; iHP1 < nbHP; ++iHP1 )
  {
    vector< TIntPntState > & intPnts1 = allIntPnts[ iHP1 ];
    if ( intPnts1.empty() ) intPnts1.resize( nbHP, undefIPS );

    int iPrev = SMESH_MesherHelper::WrapIndex( iHP1 - 1, nbHP );
    int iNext = SMESH_MesherHelper::WrapIndex( iHP1 + 1, nbHP );

    int nbNotOut = 0;
    const gp_XY* segEnds[2] = { 0, 0 }; // NOT_OUT points

    for ( int iHP2 = 0; iHP2 < nbHP; ++iHP2 )
    {
      if ( iHP1 == iHP2 ) continue;

      TIntPntState & ips1 = intPnts1[ iHP2 ];
      if ( ips1.second == UNDEF )
      {
        // find an intersection point of boundaries of iHP1 and iHP2

        if ( iHP2 == iPrev ) // intersection with neighbors is known
          ips1.first = halfPlns[ iHP1 ]._pos;
        else if ( iHP2 == iNext )
          ips1.first = halfPlns[ iHP2 ]._pos;
        else if ( !halfPlns[ iHP1 ].FindIntersection( halfPlns[ iHP2 ], ips1.first ))
          ips1.second = NO_INT;

        // classify the found intersection point
        if ( ips1.second != NO_INT )
        {
          ips1.second = NOT_OUT;
          for ( int i = 0; i < nbHP && ips1.second == NOT_OUT; ++i )
            if ( i != iHP1 && i != iHP2 &&
                 halfPlns[ i ].IsOut( ips1.first, tol ))
              ips1.second = IS_OUT;
        }
        vector< TIntPntState > & intPnts2 = allIntPnts[ iHP2 ];
        if ( intPnts2.empty() ) intPnts2.resize( nbHP, undefIPS );
        TIntPntState & ips2 = intPnts2[ iHP1 ];
        ips2 = ips1;
      }
      if ( ips1.second == NOT_OUT )
      {
        ++nbNotOut;
        segEnds[ bool(segEnds[0]) ] = & ips1.first;
      }
    }

    // find a NOT_OUT segment of boundary which is located between
    // two NOT_OUT int points

    if ( nbNotOut < 2 )
      continue; // no such a segment

    if ( nbNotOut > 2 )
    {
      // sort points along the boundary
      map< double, TIntPntState* > ipsByParam;
      for ( int iHP2 = 0; iHP2 < nbHP; ++iHP2 )
      {
        TIntPntState & ips1 = intPnts1[ iHP2 ];
        if ( ips1.second != NO_INT )
        {
          gp_XY     op = ips1.first - halfPlns[ iHP1 ]._pos;
          double param = op * halfPlns[ iHP1 ]._dir;
          ipsByParam.insert( make_pair( param, & ips1 ));
        }
      }
      // look for two neighboring NOT_OUT points
      nbNotOut = 0;
      map< double, TIntPntState* >::iterator u2ips = ipsByParam.begin();
      for ( ; u2ips != ipsByParam.end(); ++u2ips )
      {
        TIntPntState & ips1 = *(u2ips->second);
        if ( ips1.second == NOT_OUT )
          segEnds[ bool( nbNotOut++ ) ] = & ips1.first;
        else if ( nbNotOut >= 2 )
          break;
        else
          nbNotOut = 0;
      }
    }

    if ( nbNotOut >= 2 )
    {
      double len = ( *segEnds[0] - *segEnds[1] ).Modulus();
      sumLen += len;

      newPos2D += 0.5 * len * ( *segEnds[0] + *segEnds[1] );
    }
  }

  if ( sumLen > 0 )
  {
    newPos2D /= sumLen;
    newPos = center + xAxis * newPos2D.X() + yAxis * newPos2D.Y();
  }
  else
  {
    newPos = center;
  }

  return newPos;
}
#endif // OLD_NEF_POLYGON

//================================================================================
/*!
 * \brief Add a new segment to _LayerEdge during inflation
 */
//================================================================================

void _LayerEdge::SetNewLength( double len, _EdgesOnShape& eos, SMESH_MesherHelper& helper )
{
  if ( Is( BLOCKED ))
    return;

  if ( len > _maxLen )
  {
    len = _maxLen;
    Block( eos.GetData() );
  }
  const double lenDelta = len - _len;
  if ( lenDelta < len * 1e-3  )
  {
    Block( eos.GetData() );
    return;
  }

  SMDS_MeshNode* n = const_cast< SMDS_MeshNode*>( _nodes.back() );
  gp_XYZ oldXYZ = SMESH_TNodeXYZ( n );
  gp_XYZ newXYZ;
  if ( eos._hyp.IsOffsetMethod() )
  {
    newXYZ = oldXYZ;
    gp_Vec faceNorm;
    SMDS_ElemIteratorPtr faceIt = _nodes[0]->GetInverseElementIterator( SMDSAbs_Face );
    while ( faceIt->more() )
    {
      const SMDS_MeshElement* face = faceIt->next();
      if ( !eos.GetNormal( face, faceNorm ))
        continue;

      // translate plane of a face
      gp_XYZ baryCenter = oldXYZ + faceNorm.XYZ() * lenDelta;

      // find point of intersection of the face plane located at baryCenter
      // and _normal located at newXYZ
      double d   = -( faceNorm.XYZ() * baryCenter ); // d of plane equation ax+by+cz+d=0
      double dot =  ( faceNorm.XYZ() * _normal );
      if ( dot < std::numeric_limits<double>::min() )
        dot = lenDelta * 1e-3;
      double step = -( faceNorm.XYZ() * newXYZ + d ) / dot;
      newXYZ += step * _normal;
    }
    _lenFactor = _normal * ( newXYZ - oldXYZ ) / lenDelta; // _lenFactor is used in InvalidateStep()
  }
  else
  {
    newXYZ = oldXYZ + _normal * lenDelta * _lenFactor;
  }

  n->setXYZ( newXYZ.X(), newXYZ.Y(), newXYZ.Z() );
  _pos.push_back( newXYZ );

  if ( !eos._sWOL.IsNull() )
  {
    double distXYZ[4];
    bool uvOK = false;
    if ( eos.SWOLType() == TopAbs_EDGE )
    {
      double u = Precision::Infinite(); // to force projection w/o distance check
      uvOK = helper.CheckNodeU( TopoDS::Edge( eos._sWOL ), n, u,
                                /*tol=*/2*lenDelta, /*force=*/true, distXYZ );
      _pos.back().SetCoord( u, 0, 0 );
      if ( _nodes.size() > 1 && uvOK )
      {
        SMDS_EdgePosition* pos = static_cast<SMDS_EdgePosition*>( n->GetPosition() );
        pos->SetUParameter( u );
      }
    }
    else //  TopAbs_FACE
    {
      gp_XY uv( Precision::Infinite(), 0 );
      uvOK = helper.CheckNodeUV( TopoDS::Face( eos._sWOL ), n, uv,
                                 /*tol=*/2*lenDelta, /*force=*/true, distXYZ );
      _pos.back().SetCoord( uv.X(), uv.Y(), 0 );
      if ( _nodes.size() > 1 && uvOK )
      {
        SMDS_FacePosition* pos = static_cast<SMDS_FacePosition*>( n->GetPosition() );
        pos->SetUParameter( uv.X() );
        pos->SetVParameter( uv.Y() );
      }
    }
    if ( uvOK )
    {
      n->setXYZ( distXYZ[1], distXYZ[2], distXYZ[3]);
    }
    else
    {
      n->setXYZ( oldXYZ.X(), oldXYZ.Y(), oldXYZ.Z() );
      _pos.pop_back();
      Block( eos.GetData() );
      return;
    }
  }

  _len = len;

  // notify _neibors
  if ( eos.ShapeType() != TopAbs_FACE )
  {
    for ( size_t i = 0; i < _neibors.size(); ++i )
      //if (  _len > _neibors[i]->GetSmooLen() )
        _neibors[i]->Set( MOVED );

    Set( MOVED );
  }
  dumpMove( n ); //debug
}

//================================================================================
/*!
 * \brief Set BLOCKED flag and propagate limited _maxLen to _neibors
 */
//================================================================================

void _LayerEdge::Block( _SolidData& data )
{
  //if ( Is( BLOCKED )) return;
  Set( BLOCKED );

  _maxLen = _len;
  std::queue<_LayerEdge*> queue;
  queue.push( this );

  gp_Pnt pSrc, pTgt, pSrcN, pTgtN;
  while ( !queue.empty() )
  {
    _LayerEdge* edge = queue.front(); queue.pop();
    pSrc = SMESH_TNodeXYZ( edge->_nodes[0] );
    pTgt = SMESH_TNodeXYZ( edge->_nodes.back() );
    for ( size_t iN = 0; iN < edge->_neibors.size(); ++iN )
    {
      _LayerEdge* neibor = edge->_neibors[iN];
      if ( neibor->_maxLen < edge->_maxLen * 1.01 )
        continue;
      pSrcN = SMESH_TNodeXYZ( neibor->_nodes[0] );
      pTgtN = SMESH_TNodeXYZ( neibor->_nodes.back() );
      double minDist = pSrc.SquareDistance( pSrcN );
      minDist   = Min( pTgt.SquareDistance( pTgtN ), minDist );
      minDist   = Min( pSrc.SquareDistance( pTgtN ), minDist );
      minDist   = Min( pTgt.SquareDistance( pSrcN ), minDist );
      double newMaxLen = edge->_maxLen + 0.5 * Sqrt( minDist );
      //if ( edge->_nodes[0]->getshapeId() == neibor->_nodes[0]->getshapeId() ) viscous_layers_00/A3
      {
        newMaxLen *= edge->_lenFactor / neibor->_lenFactor;
      }
      if ( neibor->_maxLen > newMaxLen )
      {
        neibor->_maxLen = newMaxLen;
        if ( neibor->_maxLen < neibor->_len )
        {
          _EdgesOnShape* eos = data.GetShapeEdges( neibor );
          while ( neibor->_len > neibor->_maxLen &&
                  neibor->NbSteps() > 1 )
            neibor->InvalidateStep( neibor->NbSteps(), *eos, /*restoreLength=*/true );
          neibor->SetNewLength( neibor->_maxLen, *eos, data.GetHelper() );
          //neibor->Block( data );
        }
        queue.push( neibor );
      }
    }
  }
}

//================================================================================
/*!
 * \brief Remove last inflation step
 */
//================================================================================

void _LayerEdge::InvalidateStep( size_t curStep, const _EdgesOnShape& eos, bool restoreLength )
{
  if ( _pos.size() > curStep && _nodes.size() > 1 )
  {
    _pos.resize( curStep );

    gp_Pnt      nXYZ = _pos.back();
    SMDS_MeshNode* n = const_cast< SMDS_MeshNode*>( _nodes.back() );
    SMESH_TNodeXYZ curXYZ( n );
    if ( !eos._sWOL.IsNull() )
    {
      TopLoc_Location loc;
      if ( eos.SWOLType() == TopAbs_EDGE )
      {
        SMDS_EdgePosition* pos = static_cast<SMDS_EdgePosition*>( n->GetPosition() );
        pos->SetUParameter( nXYZ.X() );
        double f,l;
        Handle(Geom_Curve) curve = BRep_Tool::Curve( TopoDS::Edge( eos._sWOL ), loc, f,l);
        nXYZ = curve->Value( nXYZ.X() ).Transformed( loc );
      }
      else
      {
        SMDS_FacePosition* pos = static_cast<SMDS_FacePosition*>( n->GetPosition() );
        pos->SetUParameter( nXYZ.X() );
        pos->SetVParameter( nXYZ.Y() );
        Handle(Geom_Surface) surface = BRep_Tool::Surface( TopoDS::Face(eos._sWOL), loc );
        nXYZ = surface->Value( nXYZ.X(), nXYZ.Y() ).Transformed( loc );
      }
    }
    n->setXYZ( nXYZ.X(), nXYZ.Y(), nXYZ.Z() );
    dumpMove( n );

    if ( restoreLength )
    {
      _len -= ( nXYZ.XYZ() - curXYZ ).Modulus() / _lenFactor;
    }
  }
}

//================================================================================
/*!
 * \brief Return index of a _pos distant from _normal
 */
//================================================================================

int _LayerEdge::GetSmoothedPos( const double tol )
{
  int iSmoothed = 0;
  for ( size_t i = 1; i < _pos.size() && !iSmoothed; ++i )
  {
    double normDist = ( _pos[i] - _pos[0] ).Crossed( _normal ).SquareModulus();
    if ( normDist > tol * tol )
      iSmoothed = i;
  }
  return iSmoothed;
}

//================================================================================
/*!
 * \brief Smooth a path formed by _pos of a _LayerEdge smoothed on FACE
 */
//================================================================================

void _LayerEdge::SmoothPos( const vector< double >& segLen, const double tol )
{
  if ( /*Is( NORMAL_UPDATED ) ||*/ _pos.size() <= 2 )
    return;

  // find the 1st smoothed _pos
  int iSmoothed = GetSmoothedPos( tol );
  if ( !iSmoothed ) return;

  //if ( 1 || Is( DISTORTED ))
  {
    gp_XYZ normal = _normal;
    if ( Is( NORMAL_UPDATED ))
      for ( size_t i = 1; i < _pos.size(); ++i )
      {
        normal = _pos[i] - _pos[0];
        double size = normal.Modulus();
        if ( size > RealSmall() )
        {
          normal /= size;
          break;
        }
      }
    const double r = 0.2;
    for ( int iter = 0; iter < 50; ++iter )
    {
      double minDot = 1;
      for ( size_t i = Max( 1, iSmoothed-1-iter ); i < _pos.size()-1; ++i )
      {
        gp_XYZ midPos = 0.5 * ( _pos[i-1] + _pos[i+1] );
        gp_XYZ newPos = ( 1-r ) * midPos + r * _pos[i];
        _pos[i] = newPos;
        double midLen = 0.5 * ( segLen[i-1] + segLen[i+1] );
        double newLen = ( 1-r ) * midLen + r * segLen[i];
        const_cast< double& >( segLen[i] ) = newLen;
        // check angle between normal and (_pos[i+1], _pos[i] )
        gp_XYZ posDir = _pos[i+1] - _pos[i];
        double size   = posDir.SquareModulus();
        if ( size > RealSmall() )
          minDot = Min( minDot, ( normal * posDir ) * ( normal * posDir ) / size );
      }
      if ( minDot > 0.5 * 0.5 )
        break;
    }
  }
  // else
  // {
  //   for ( size_t i = 1; i < _pos.size()-1; ++i )
  //   {
  //     if ((int) i < iSmoothed  &&  ( segLen[i] / segLen.back() < 0.5 ))
  //       continue;

  //     double     wgt = segLen[i] / segLen.back();
  //     gp_XYZ normPos = _pos[0] + _normal * wgt * _len;
  //     gp_XYZ tgtPos  = ( 1 - wgt ) * _pos[0] +  wgt * _pos.back();
  //     gp_XYZ newPos  = ( 1 - wgt ) * normPos +  wgt * tgtPos;
  //     _pos[i] = newPos;
  //   }
  // }
}

//================================================================================
/*!
 * \brief Print flags
 */
//================================================================================

std::string _LayerEdge::DumpFlags() const
{
  SMESH_Comment dump;
  for ( int flag = 1; flag < 0x1000000; flag *= 2 )
    if ( _flags & flag )
    {
      EFlags f = (EFlags) flag;
      switch ( f ) {
      case TO_SMOOTH:       dump << "TO_SMOOTH";       break;
      case MOVED:           dump << "MOVED";           break;
      case SMOOTHED:        dump << "SMOOTHED";        break;
      case DIFFICULT:       dump << "DIFFICULT";       break;
      case ON_CONCAVE_FACE: dump << "ON_CONCAVE_FACE"; break;
      case BLOCKED:         dump << "BLOCKED";         break;
      case INTERSECTED:     dump << "INTERSECTED";     break;
      case NORMAL_UPDATED:  dump << "NORMAL_UPDATED";  break;
      case MARKED:          dump << "MARKED";          break;
      case MULTI_NORMAL:    dump << "MULTI_NORMAL";    break;
      case NEAR_BOUNDARY:   dump << "NEAR_BOUNDARY";   break;
      case SMOOTHED_C1:     dump << "SMOOTHED_C1";     break;
      case DISTORTED:       dump << "DISTORTED";       break;
      case RISKY_SWOL:      dump << "RISKY_SWOL";      break;
      case SHRUNK:          dump << "SHRUNK";          break;
      case UNUSED_FLAG:     dump << "UNUSED_FLAG";     break;
      }
      dump << " ";
    }
  cout << dump << endl;
  return dump;
}

//================================================================================
/*!
  case brief:
  default:
*/
//================================================================================

bool _ViscousBuilder::refine(_SolidData& data)
{
  SMESH_MesherHelper& helper = data.GetHelper();
  helper.SetElementsOnShape(false);

  Handle(Geom_Curve) curve;
  Handle(ShapeAnalysis_Surface) surface;
  TopoDS_Edge geomEdge;
  TopoDS_Face geomFace;
  TopLoc_Location loc;
  double f,l, u = 0;
  gp_XY uv;
  vector< gp_XYZ > pos3D;
  bool isOnEdge;
  TGeomID prevBaseId = -1;
  TNode2Edge* n2eMap = 0;
  TNode2Edge::iterator n2e;

  // Create intermediate nodes on each _LayerEdge

  for ( size_t iS = 0; iS < data._edgesOnShape.size(); ++iS )
  {
    _EdgesOnShape& eos = data._edgesOnShape[iS];
    if ( eos._edges.empty() ) continue;

    if ( eos._edges[0]->_nodes.size() < 2 )
      continue; // on _noShrinkShapes

    // get data of a shrink shape
    isOnEdge = false;
    geomEdge.Nullify(); geomFace.Nullify();
    curve.Nullify(); surface.Nullify();
    if ( !eos._sWOL.IsNull() )
    {
      isOnEdge = ( eos.SWOLType() == TopAbs_EDGE );
      if ( isOnEdge )
      {
        geomEdge = TopoDS::Edge( eos._sWOL );
        curve    = BRep_Tool::Curve( geomEdge, loc, f,l);
      }
      else
      {
        geomFace = TopoDS::Face( eos._sWOL );
        surface  = helper.GetSurface( geomFace );
      }
    }
    else if ( eos.ShapeType() == TopAbs_FACE && eos._toSmooth )
    {
      geomFace = TopoDS::Face( eos._shape );
      surface  = helper.GetSurface( geomFace );
      // propagate _toSmooth back to _eosC1, which was unset in findShapesToSmooth()
      for ( size_t i = 0; i < eos._eosC1.size(); ++i )
      {
        eos._eosC1[ i ]->_toSmooth = true;
        for ( size_t j = 0; j < eos._eosC1[i]->_edges.size(); ++j )
          eos._eosC1[i]->_edges[j]->Set( _LayerEdge::SMOOTHED_C1 );
      }
    }

    vector< double > segLen;
    for ( size_t i = 0; i < eos._edges.size(); ++i )
    {
      _LayerEdge& edge = *eos._edges[i];
      if ( edge._pos.size() < 2 )
        continue;

      // get accumulated length of segments
      segLen.resize( edge._pos.size() );
      segLen[0] = 0.0;
      if ( eos._sWOL.IsNull() )
      {
        bool useNormal = true;
        bool   usePos  = false;
        bool smoothed  = false;
        double   preci = 0.1 * edge._len;
        if ( eos._toSmooth && edge._pos.size() > 2 )
        {
          smoothed = edge.GetSmoothedPos( preci );
        }
        if ( smoothed )
        {
          if ( !surface.IsNull() &&
               !data._convexFaces.count( eos._shapeID )) // edge smoothed on FACE
          {
            useNormal = usePos = false;
            gp_Pnt2d uv = helper.GetNodeUV( geomFace, edge._nodes[0] );
            for ( size_t j = 1; j < edge._pos.size() && !useNormal; ++j )
            {
              uv = surface->NextValueOfUV( uv, edge._pos[j], preci );
              if ( surface->Gap() < 2. * edge._len )
                segLen[j] = surface->Gap();
              else
                useNormal = true;
            }
          }
        }
        else if ( !edge.Is( _LayerEdge::NORMAL_UPDATED ))
        {
#ifndef __NODES_AT_POS
          useNormal = usePos = false;
          edge._pos[1] = edge._pos.back();
          edge._pos.resize( 2 );
          segLen.resize( 2 );
          segLen[ 1 ] = edge._len;
#endif
        }
        if ( useNormal && edge.Is( _LayerEdge::NORMAL_UPDATED ))
        {
          useNormal = usePos = false;
          _LayerEdge tmpEdge; // get original _normal
          tmpEdge._nodes.push_back( edge._nodes[0] );
          if ( !setEdgeData( tmpEdge, eos, helper, data ))
            usePos = true;
          else
            for ( size_t j = 1; j < edge._pos.size(); ++j )
              segLen[j] = ( edge._pos[j] - edge._pos[0] ) * tmpEdge._normal;
        }
        if ( useNormal )
        {
          for ( size_t j = 1; j < edge._pos.size(); ++j )
            segLen[j] = ( edge._pos[j] - edge._pos[0] ) * edge._normal;
        }
        if ( usePos )
        {
          for ( size_t j = 1; j < edge._pos.size(); ++j )
            segLen[j] = segLen[j-1] + ( edge._pos[j-1] - edge._pos[j] ).Modulus();
        }
        else
        {
          bool swapped = ( edge._pos.size() > 2 );
          while ( swapped )
          {
            swapped = false;
            for ( size_t j = 1; j < edge._pos.size()-1; ++j )
              if ( segLen[j] > segLen.back() )
              {
                segLen.erase( segLen.begin() + j );
                edge._pos.erase( edge._pos.begin() + j );
                --j;
              }
              else if ( segLen[j] < segLen[j-1] )
              {
                std::swap( segLen[j], segLen[j-1] );
                std::swap( edge._pos[j], edge._pos[j-1] );
                swapped = true;
              }
          }
        }
        // smooth a path formed by edge._pos
#ifndef __NODES_AT_POS
        if (( smoothed ) /*&&
            ( eos.ShapeType() == TopAbs_FACE || edge.Is( _LayerEdge::SMOOTHED_C1 ))*/)
          edge.SmoothPos( segLen, preci );
#endif
      }
      else if ( eos._isRegularSWOL ) // usual SWOL
      {
        for ( size_t j = 1; j < edge._pos.size(); ++j )
          segLen[j] = segLen[j-1] + (edge._pos[j-1] - edge._pos[j] ).Modulus();
      }
      else if ( !surface.IsNull() ) // SWOL surface with singularities
      {
        pos3D.resize( edge._pos.size() );
        for ( size_t j = 0; j < edge._pos.size(); ++j )
          pos3D[j] = surface->Value( edge._pos[j].X(), edge._pos[j].Y() ).XYZ();

        for ( size_t j = 1; j < edge._pos.size(); ++j )
          segLen[j] = segLen[j-1] + ( pos3D[j-1] - pos3D[j] ).Modulus();
      }

      // allocate memory for new nodes if it is not yet refined
      const SMDS_MeshNode* tgtNode = edge._nodes.back();
      if ( edge._nodes.size() == 2 )
      {
#ifdef __NODES_AT_POS
        int nbNodes = edge._pos.size();
#else
        int nbNodes = eos._hyp.GetNumberLayers() + 1;
#endif
        edge._nodes.resize( nbNodes, 0 );
        edge._nodes[1] = 0;
        edge._nodes.back() = tgtNode;
      }
      // restore shapePos of the last node by already treated _LayerEdge of another _SolidData
      const TGeomID baseShapeId = edge._nodes[0]->getshapeId();
      if ( baseShapeId != prevBaseId )
      {
        map< TGeomID, TNode2Edge* >::iterator s2ne = data._s2neMap.find( baseShapeId );
        n2eMap = ( s2ne == data._s2neMap.end() ) ? 0 : s2ne->second;
        prevBaseId = baseShapeId;
      }
      _LayerEdge* edgeOnSameNode = 0;
      bool        useExistingPos = false;
      if ( n2eMap && (( n2e = n2eMap->find( edge._nodes[0] )) != n2eMap->end() ))
      {
        edgeOnSameNode = n2e->second;
        useExistingPos = ( edgeOnSameNode->_len < edge._len );
        const gp_XYZ& otherTgtPos = edgeOnSameNode->_pos.back();
        SMDS_PositionPtr  lastPos = tgtNode->GetPosition();
        if ( isOnEdge )
        {
          SMDS_EdgePosition* epos = static_cast<SMDS_EdgePosition*>( lastPos );
          epos->SetUParameter( otherTgtPos.X() );
        }
        else
        {
          SMDS_FacePosition* fpos = static_cast<SMDS_FacePosition*>( lastPos );
          fpos->SetUParameter( otherTgtPos.X() );
          fpos->SetVParameter( otherTgtPos.Y() );
        }
      }
      // calculate height of the first layer
      double h0;
      const double T = segLen.back(); //data._hyp.GetTotalThickness();
      const double f = eos._hyp.GetStretchFactor();
      const int    N = eos._hyp.GetNumberLayers();
      const double fPowN = pow( f, N );
      if ( fPowN - 1 <= numeric_limits<double>::min() )
        h0 = T / N;
      else
        h0 = T * ( f - 1 )/( fPowN - 1 );

      const double zeroLen = std::numeric_limits<double>::min();

      // create intermediate nodes
      double hSum = 0, hi = h0/f;
      size_t iSeg = 1;
      for ( size_t iStep = 1; iStep < edge._nodes.size(); ++iStep )
      {
        // compute an intermediate position
        hi *= f;
        hSum += hi;
        while ( hSum > segLen[iSeg] && iSeg < segLen.size()-1 )
          ++iSeg;
        int iPrevSeg = iSeg-1;
        while ( fabs( segLen[iPrevSeg] - segLen[iSeg]) <= zeroLen && iPrevSeg > 0 )
          --iPrevSeg;
        double   r = ( segLen[iSeg] - hSum ) / ( segLen[iSeg] - segLen[iPrevSeg] );
        gp_Pnt pos = r * edge._pos[iPrevSeg] + (1-r) * edge._pos[iSeg];
#ifdef __NODES_AT_POS
        pos = edge._pos[ iStep ];
#endif
        SMDS_MeshNode*& node = const_cast< SMDS_MeshNode*& >( edge._nodes[ iStep ]);
        if ( !eos._sWOL.IsNull() )
        {
          // compute XYZ by parameters <pos>
          if ( isOnEdge )
          {
            u = pos.X();
            if ( !node )
              pos = curve->Value( u ).Transformed(loc);
          }
          else if ( eos._isRegularSWOL )
          {
            uv.SetCoord( pos.X(), pos.Y() );
            if ( !node )
              pos = surface->Value( pos.X(), pos.Y() );
          }
          else
          {
            uv.SetCoord( pos.X(), pos.Y() );
            gp_Pnt p = r * pos3D[ iPrevSeg ] + (1-r) * pos3D[ iSeg ];
            uv = surface->NextValueOfUV( uv, p, BRep_Tool::Tolerance( geomFace )).XY();
            if ( !node )
              pos = surface->Value( uv );
          }
        }
        // create or update the node
        if ( !node )
        {
          node = helper.AddNode( pos.X(), pos.Y(), pos.Z());
          if ( !eos._sWOL.IsNull() )
          {
            if ( isOnEdge )
              getMeshDS()->SetNodeOnEdge( node, geomEdge, u );
            else
              getMeshDS()->SetNodeOnFace( node, geomFace, uv.X(), uv.Y() );
          }
          else
          {
            getMeshDS()->SetNodeInVolume( node, helper.GetSubShapeID() );
          }
        }
        else
        {
          if ( !eos._sWOL.IsNull() )
          {
            // make average pos from new and current parameters
            if ( isOnEdge )
            {
              //u = 0.5 * ( u + helper.GetNodeU( geomEdge, node ));
              if ( useExistingPos )
                u = helper.GetNodeU( geomEdge, node );
              pos = curve->Value( u ).Transformed(loc);

              SMDS_EdgePosition* epos = static_cast<SMDS_EdgePosition*>( node->GetPosition() );
              epos->SetUParameter( u );
            }
            else
            {
              //uv = 0.5 * ( uv + helper.GetNodeUV( geomFace, node ));
              if ( useExistingPos )
                uv = helper.GetNodeUV( geomFace, node );
              pos = surface->Value( uv );

              SMDS_FacePosition* fpos = static_cast<SMDS_FacePosition*>( node->GetPosition() );
              fpos->SetUParameter( uv.X() );
              fpos->SetVParameter( uv.Y() );
            }
          }
          node->setXYZ( pos.X(), pos.Y(), pos.Z() );
        }
      } // loop on edge._nodes

      if ( !eos._sWOL.IsNull() ) // prepare for shrink()
      {
        if ( isOnEdge )
          edge._pos.back().SetCoord( u, 0,0);
        else
          edge._pos.back().SetCoord( uv.X(), uv.Y() ,0);

        if ( edgeOnSameNode )
          edgeOnSameNode->_pos.back() = edge._pos.back();
      }

    } // loop on eos._edges to create nodes


    if ( !getMeshDS()->IsEmbeddedMode() )
      // Log node movement
      for ( size_t i = 0; i < eos._edges.size(); ++i )
      {
        SMESH_TNodeXYZ p ( eos._edges[i]->_nodes.back() );
        getMeshDS()->MoveNode( p._node, p.X(), p.Y(), p.Z() );
      }
  }


  // Create volumes

  helper.SetElementsOnShape(true);

  vector< vector<const SMDS_MeshNode*>* > nnVec;
  set< vector<const SMDS_MeshNode*>* >    nnSet;
  set< int >                       degenEdgeInd;
  vector<const SMDS_MeshElement*>     degenVols;

  TopExp_Explorer exp( data._solid, TopAbs_FACE );
  for ( ; exp.More(); exp.Next() )
  {
    const TGeomID faceID = getMeshDS()->ShapeToIndex( exp.Current() );
    if ( data._ignoreFaceIds.count( faceID ))
      continue;
    const bool isReversedFace = data._reversedFaceIds.count( faceID );
    SMESHDS_SubMesh*    fSubM = getMeshDS()->MeshElements( exp.Current() );
    SMDS_ElemIteratorPtr  fIt = fSubM->GetElements();
    while ( fIt->more() )
    {
      const SMDS_MeshElement* face = fIt->next();
      const int            nbNodes = face->NbCornerNodes();
      nnVec.resize( nbNodes );
      nnSet.clear();
      degenEdgeInd.clear();
      size_t maxZ = 0, minZ = std::numeric_limits<size_t>::max();
      SMDS_NodeIteratorPtr nIt = face->nodeIterator();
      for ( int iN = 0; iN < nbNodes; ++iN )
      {
        const SMDS_MeshNode* n = nIt->next();
        _LayerEdge*       edge = data._n2eMap[ n ];
        const int i = isReversedFace ? nbNodes-1-iN : iN;
        nnVec[ i ] = & edge->_nodes;
        maxZ = std::max( maxZ, nnVec[ i ]->size() );
        minZ = std::min( minZ, nnVec[ i ]->size() );

        if ( helper.HasDegeneratedEdges() )
          nnSet.insert( nnVec[ i ]);
      }

      if ( maxZ == 0 )
        continue;
      if ( 0 < nnSet.size() && nnSet.size() < 3 )
        continue;

      switch ( nbNodes )
      {
      case 3: // TRIA
      {
        // PENTA
        for ( size_t iZ = 1; iZ < minZ; ++iZ )
          helper.AddVolume( (*nnVec[0])[iZ-1], (*nnVec[1])[iZ-1], (*nnVec[2])[iZ-1],
                            (*nnVec[0])[iZ],   (*nnVec[1])[iZ],   (*nnVec[2])[iZ]);

        for ( size_t iZ = minZ; iZ < maxZ; ++iZ )
        {
          for ( int iN = 0; iN < nbNodes; ++iN )
            if ( nnVec[ iN ]->size() < iZ+1 )
              degenEdgeInd.insert( iN );

          if ( degenEdgeInd.size() == 1 )  // PYRAM
          {
            int i2 = *degenEdgeInd.begin();
            int i0 = helper.WrapIndex( i2 - 1, nbNodes );
            int i1 = helper.WrapIndex( i2 + 1, nbNodes );
            helper.AddVolume( (*nnVec[i0])[iZ-1], (*nnVec[i1])[iZ-1],
                              (*nnVec[i1])[iZ  ], (*nnVec[i0])[iZ  ], (*nnVec[i2]).back());
          }
          else  // TETRA
          {
            int i3 = !degenEdgeInd.count(0) ? 0 : !degenEdgeInd.count(1) ? 1 : 2;
            helper.AddVolume( (*nnVec[  0 ])[ i3 == 0 ? iZ-1 : nnVec[0]->size()-1 ],
                              (*nnVec[  1 ])[ i3 == 1 ? iZ-1 : nnVec[1]->size()-1 ],
                              (*nnVec[  2 ])[ i3 == 2 ? iZ-1 : nnVec[2]->size()-1 ],
                              (*nnVec[ i3 ])[ iZ ]);
          }
        }
        break; // TRIA
      }
      case 4: // QUAD
      {
        // HEX
        for ( size_t iZ = 1; iZ < minZ; ++iZ )
          helper.AddVolume( (*nnVec[0])[iZ-1], (*nnVec[1])[iZ-1],
                            (*nnVec[2])[iZ-1], (*nnVec[3])[iZ-1],
                            (*nnVec[0])[iZ],   (*nnVec[1])[iZ],
                            (*nnVec[2])[iZ],   (*nnVec[3])[iZ]);

        for ( size_t iZ = minZ; iZ < maxZ; ++iZ )
        {
          for ( int iN = 0; iN < nbNodes; ++iN )
            if ( nnVec[ iN ]->size() < iZ+1 )
              degenEdgeInd.insert( iN );

          switch ( degenEdgeInd.size() )
          {
          case 2: // PENTA
          {
            int i2 = *degenEdgeInd.begin();
            int i3 = *degenEdgeInd.rbegin();
            bool ok = ( i3 - i2 == 1 );
            if ( i2 == 0 && i3 == 3 ) { i2 = 3; i3 = 0; ok = true; }
            int i0 = helper.WrapIndex( i3 + 1, nbNodes );
            int i1 = helper.WrapIndex( i0 + 1, nbNodes );

            const SMDS_MeshElement* vol =
              helper.AddVolume( nnVec[i3]->back(), (*nnVec[i0])[iZ], (*nnVec[i0])[iZ-1],
                                nnVec[i2]->back(), (*nnVec[i1])[iZ], (*nnVec[i1])[iZ-1]);
            if ( !ok && vol )
              degenVols.push_back( vol );
          }
          break;

          default: // degen HEX
          {
            const SMDS_MeshElement* vol =
              helper.AddVolume( nnVec[0]->size() > iZ-1 ? (*nnVec[0])[iZ-1] : nnVec[0]->back(),
                                nnVec[1]->size() > iZ-1 ? (*nnVec[1])[iZ-1] : nnVec[1]->back(),
                                nnVec[2]->size() > iZ-1 ? (*nnVec[2])[iZ-1] : nnVec[2]->back(),
                                nnVec[3]->size() > iZ-1 ? (*nnVec[3])[iZ-1] : nnVec[3]->back(),
                                nnVec[0]->size() > iZ   ? (*nnVec[0])[iZ]   : nnVec[0]->back(),
                                nnVec[1]->size() > iZ   ? (*nnVec[1])[iZ]   : nnVec[1]->back(),
                                nnVec[2]->size() > iZ   ? (*nnVec[2])[iZ]   : nnVec[2]->back(),
                                nnVec[3]->size() > iZ   ? (*nnVec[3])[iZ]   : nnVec[3]->back());
            degenVols.push_back( vol );
          }
          }
        }
        break; // HEX
      }
      default:
        return error("Not supported type of element", data._index);

      } // switch ( nbNodes )
    } // while ( fIt->more() )
  } // loop on FACEs

  if ( !degenVols.empty() )
  {
    SMESH_ComputeErrorPtr& err = _mesh->GetSubMesh( data._solid )->GetComputeError();
    if ( !err || err->IsOK() )
    {
      err.reset( new SMESH_ComputeError( COMPERR_WARNING,
                                         "Bad quality volumes created" ));
      err->myBadElements.insert( err->myBadElements.end(),
                                 degenVols.begin(),degenVols.end() );
    }
  }

  return true;
}

//================================================================================
/*!
 * \brief Shrink 2D mesh on faces to let space for inflated layers
 */
//================================================================================

bool _ViscousBuilder::shrink(_SolidData& theData)
{
  // make map of (ids of FACEs to shrink mesh on) to (list of _SolidData containing
  // _LayerEdge's inflated along FACE or EDGE)
  map< TGeomID, list< _SolidData* > > f2sdMap;
  for ( size_t i = 0 ; i < _sdVec.size(); ++i )
  {
    _SolidData& data = _sdVec[i];
    map< TGeomID, TopoDS_Shape >::iterator s2s = data._shrinkShape2Shape.begin();
    for (; s2s != data._shrinkShape2Shape.end(); ++s2s )
      if ( s2s->second.ShapeType() == TopAbs_FACE && !_shrinkedFaces.Contains( s2s->second ))
      {
        f2sdMap[ getMeshDS()->ShapeToIndex( s2s->second )].push_back( &data );

        // Put mesh faces on the shrinked FACE to the proxy sub-mesh to avoid
        // usage of mesh faces made in addBoundaryElements() by the 3D algo or
        // by StdMeshers_QuadToTriaAdaptor
        if ( SMESHDS_SubMesh* smDS = getMeshDS()->MeshElements( s2s->second ))
        {
          SMESH_ProxyMesh::SubMesh* proxySub =
            data._proxyMesh->getFaceSubM( TopoDS::Face( s2s->second ), /*create=*/true);
          if ( proxySub->NbElements() == 0 )
          {
            SMDS_ElemIteratorPtr fIt = smDS->GetElements();
            while ( fIt->more() )
            {
              const SMDS_MeshElement* f = fIt->next();
              // as a result 3D algo will use elements from proxySub and not from smDS
              proxySub->AddElement( f );
              f->setIsMarked( true );

              // Mark nodes on the FACE to discriminate them from nodes
              // added by addBoundaryElements(); marked nodes are to be smoothed while shrink()
              for ( int iN = 0, nbN = f->NbNodes(); iN < nbN; ++iN )
              {
                const SMDS_MeshNode* n = f->GetNode( iN );
                if ( n->GetPosition()->GetDim() == 2 )
                  n->setIsMarked( true );
              }
            }
          }
        }
      }
  }

  SMESH_MesherHelper helper( *_mesh );
  helper.ToFixNodeParameters( true );

  // EDGEs to shrink
  map< TGeomID, _Shrinker1D > e2shrMap;
  vector< _EdgesOnShape* > subEOS;
  vector< _LayerEdge* > lEdges;

  // loop on FACEs to srink mesh on
  map< TGeomID, list< _SolidData* > >::iterator f2sd = f2sdMap.begin();
  for ( ; f2sd != f2sdMap.end(); ++f2sd )
  {
    list< _SolidData* > & dataList = f2sd->second;
    if ( dataList.front()->_n2eMap.empty() ||
         dataList.back() ->_n2eMap.empty() )
      continue; // not yet computed
    if ( dataList.front() != &theData &&
         dataList.back()  != &theData )
      continue;

    _SolidData&      data = *dataList.front();
    _SolidData*     data2 = dataList.size() > 1 ? dataList.back() : 0;
    const TopoDS_Face&  F = TopoDS::Face( getMeshDS()->IndexToShape( f2sd->first ));
    SMESH_subMesh*     sm = _mesh->GetSubMesh( F );
    SMESHDS_SubMesh* smDS = sm->GetSubMeshDS();

    Handle(Geom_Surface) surface = BRep_Tool::Surface( F );

    _shrinkedFaces.Add( F );
    helper.SetSubShape( F );

    // ===========================
    // Prepare data for shrinking
    // ===========================

    // Collect nodes to smooth (they are marked at the beginning of this method)
    vector < const SMDS_MeshNode* > smoothNodes;
    {
      SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
      while ( nIt->more() )
      {
        const SMDS_MeshNode* n = nIt->next();
        if ( n->isMarked() )
          smoothNodes.push_back( n );
      }
    }
    // Find out face orientation
    double refSign = 1;
    const set<TGeomID> ignoreShapes;
    bool isOkUV;
    if ( !smoothNodes.empty() )
    {
      vector<_Simplex> simplices;
      _Simplex::GetSimplices( smoothNodes[0], simplices, ignoreShapes );
      helper.GetNodeUV( F, simplices[0]._nPrev, 0, &isOkUV ); // fix UV of simplex nodes
      helper.GetNodeUV( F, simplices[0]._nNext, 0, &isOkUV );
      gp_XY uv = helper.GetNodeUV( F, smoothNodes[0], 0, &isOkUV );
      if ( !simplices[0].IsForward(uv, smoothNodes[0], F, helper, refSign ))
        refSign = -1;
    }

    // Find _LayerEdge's inflated along F
    subEOS.clear();
    lEdges.clear();
    {
      SMESH_subMeshIteratorPtr subIt = sm->getDependsOnIterator(/*includeSelf=*/false,
                                                                /*complexFirst=*/true); //!!!
      while ( subIt->more() )
      {
        const TGeomID subID = subIt->next()->GetId();
        if ( data._noShrinkShapes.count( subID ))
          continue;
        _EdgesOnShape* eos = data.GetShapeEdges( subID );
        if ( !eos || eos->_sWOL.IsNull() )
          if ( data2 ) // check in adjacent SOLID
          {
            eos = data2->GetShapeEdges( subID );
            if ( !eos || eos->_sWOL.IsNull() )
              continue;
          }
        subEOS.push_back( eos );

        for ( size_t i = 0; i < eos->_edges.size(); ++i )
        {
          lEdges.push_back( eos->_edges[ i ] );
          prepareEdgeToShrink( *eos->_edges[ i ], *eos, helper, smDS );
        }
      }
    }

    dumpFunction(SMESH_Comment("beforeShrinkFace")<<f2sd->first); // debug
    SMDS_ElemIteratorPtr fIt = smDS->GetElements();
    while ( fIt->more() )
      if ( const SMDS_MeshElement* f = fIt->next() )
        dumpChangeNodes( f );
    dumpFunctionEnd();

    // Replace source nodes by target nodes in mesh faces to shrink
    dumpFunction(SMESH_Comment("replNodesOnFace")<<f2sd->first); // debug
    const SMDS_MeshNode* nodes[20];
    for ( size_t iS = 0; iS < subEOS.size(); ++iS )
    {
      _EdgesOnShape& eos = * subEOS[ iS ];
      for ( size_t i = 0; i < eos._edges.size(); ++i )
      {
        _LayerEdge& edge = *eos._edges[i];
        const SMDS_MeshNode* srcNode = edge._nodes[0];
        const SMDS_MeshNode* tgtNode = edge._nodes.back();
        SMDS_ElemIteratorPtr fIt = srcNode->GetInverseElementIterator(SMDSAbs_Face);
        while ( fIt->more() )
        {
          const SMDS_MeshElement* f = fIt->next();
          if ( !smDS->Contains( f ) || !f->isMarked() )
            continue;
          SMDS_NodeIteratorPtr nIt = f->nodeIterator();
          for ( int iN = 0; nIt->more(); ++iN )
          {
            const SMDS_MeshNode* n = nIt->next();
            nodes[iN] = ( n == srcNode ? tgtNode : n );
          }
          helper.GetMeshDS()->ChangeElementNodes( f, nodes, f->NbNodes() );
          dumpChangeNodes( f );
        }
      }
    }
    dumpFunctionEnd();

    // find out if a FACE is concave
    const bool isConcaveFace = isConcave( F, helper );

    // Create _SmoothNode's on face F
    vector< _SmoothNode > nodesToSmooth( smoothNodes.size() );
    {
      dumpFunction(SMESH_Comment("fixUVOnFace")<<f2sd->first); // debug
      const bool sortSimplices = isConcaveFace;
      for ( size_t i = 0; i < smoothNodes.size(); ++i )
      {
        const SMDS_MeshNode* n = smoothNodes[i];
        nodesToSmooth[ i ]._node = n;
        // src nodes must be already replaced by tgt nodes to have tgt nodes in _simplices
        _Simplex::GetSimplices( n, nodesToSmooth[ i ]._simplices, ignoreShapes, 0, sortSimplices);
        // fix up incorrect uv of nodes on the FACE
        helper.GetNodeUV( F, n, 0, &isOkUV);
        dumpMove( n );
      }
      dumpFunctionEnd();
    }
    //if ( nodesToSmooth.empty() ) continue;

    // Find EDGE's to shrink and set simpices to LayerEdge's
    set< _Shrinker1D* > eShri1D;
    {
      for ( size_t iS = 0; iS < subEOS.size(); ++iS )
      {
        _EdgesOnShape& eos = * subEOS[ iS ];
        if ( eos.SWOLType() == TopAbs_EDGE )
        {
          SMESH_subMesh* edgeSM = _mesh->GetSubMesh( eos._sWOL );
          _Shrinker1D& srinker  = e2shrMap[ edgeSM->GetId() ];
          eShri1D.insert( & srinker );
          srinker.AddEdge( eos._edges[0], eos, helper );
          VISCOUS_3D::ToClearSubWithMain( edgeSM, data._solid );
          // restore params of nodes on EGDE if the EDGE has been already
          // srinked while srinking other FACE
          srinker.RestoreParams();
        }
        for ( size_t i = 0; i < eos._edges.size(); ++i )
        {
          _LayerEdge& edge = * eos._edges[i];
          _Simplex::GetSimplices( /*tgtNode=*/edge._nodes.back(), edge._simplices, ignoreShapes );

          // additionally mark tgt node; only marked nodes will be used in SetNewLength2d()
          // not-marked nodes are those added by refine()
          edge._nodes.back()->setIsMarked( true );
        }
      }
    }

    bool toFixTria = false; // to improve quality of trias by diagonal swap
    if ( isConcaveFace )
    {
      const bool hasTria = _mesh->NbTriangles(), hasQuad = _mesh->NbQuadrangles();
      if ( hasTria != hasQuad ) {
        toFixTria = hasTria;
      }
      else {
        set<int> nbNodesSet;
        SMDS_ElemIteratorPtr fIt = smDS->GetElements();
        while ( fIt->more() && nbNodesSet.size() < 2 )
          nbNodesSet.insert( fIt->next()->NbCornerNodes() );
        toFixTria = ( *nbNodesSet.begin() == 3 );
      }
    }

    // ==================
    // Perform shrinking
    // ==================

    bool shrinked = true;
    int nbBad, shriStep=0, smooStep=0;
    _SmoothNode::SmoothType smoothType
      = isConcaveFace ? _SmoothNode::ANGULAR : _SmoothNode::LAPLACIAN;
    SMESH_Comment errMsg;
    while ( shrinked )
    {
      shriStep++;
      // Move boundary nodes (actually just set new UV)
      // -----------------------------------------------
      dumpFunction(SMESH_Comment("moveBoundaryOnF")<<f2sd->first<<"_st"<<shriStep ); // debug
      shrinked = false;
      for ( size_t iS = 0; iS < subEOS.size(); ++iS )
      {
        _EdgesOnShape& eos = * subEOS[ iS ];
        for ( size_t i = 0; i < eos._edges.size(); ++i )
        {
          shrinked |= eos._edges[i]->SetNewLength2d( surface, F, eos, helper );
        }
      }
      dumpFunctionEnd();

      // Move nodes on EDGE's
      // (XYZ is set as soon as a needed length reached in SetNewLength2d())
      set< _Shrinker1D* >::iterator shr = eShri1D.begin();
      for ( ; shr != eShri1D.end(); ++shr )
        (*shr)->Compute( /*set3D=*/false, helper );

      // Smoothing in 2D
      // -----------------
      int nbNoImpSteps = 0;
      bool       moved = true;
      nbBad = 1;
      while (( nbNoImpSteps < 5 && nbBad > 0) && moved)
      {
        dumpFunction(SMESH_Comment("shrinkFace")<<f2sd->first<<"_st"<<++smooStep); // debug

        int oldBadNb = nbBad;
        nbBad = 0;
        moved = false;
        // '% 5' minimizes NB FUNCTIONS on viscous_layers_00/B2 case
        _SmoothNode::SmoothType smooTy = ( smooStep % 5 ) ? smoothType : _SmoothNode::LAPLACIAN;
        for ( size_t i = 0; i < nodesToSmooth.size(); ++i )
        {
          moved |= nodesToSmooth[i].Smooth( nbBad, surface, helper, refSign,
                                            smooTy, /*set3D=*/isConcaveFace);
        }
        if ( nbBad < oldBadNb )
          nbNoImpSteps = 0;
        else
          nbNoImpSteps++;

        dumpFunctionEnd();
      }

      errMsg.clear();
      if ( nbBad > 0 )
        errMsg << "Can't shrink 2D mesh on face " << f2sd->first;
      if ( shriStep > 200 )
        errMsg << "Infinite loop at shrinking 2D mesh on face " << f2sd->first;
      if ( !errMsg.empty() )
        break;

      // Fix narrow triangles by swapping diagonals
      // ---------------------------------------
      if ( toFixTria )
      {
        set<const SMDS_MeshNode*> usedNodes;
        fixBadFaces( F, helper, /*is2D=*/true, shriStep, & usedNodes); // swap diagonals

        // update working data
        set<const SMDS_MeshNode*>::iterator n;
        for ( size_t i = 0; i < nodesToSmooth.size() && !usedNodes.empty(); ++i )
        {
          n = usedNodes.find( nodesToSmooth[ i ]._node );
          if ( n != usedNodes.end())
          {
            _Simplex::GetSimplices( nodesToSmooth[ i ]._node,
                                    nodesToSmooth[ i ]._simplices,
                                    ignoreShapes, NULL,
                                    /*sortSimplices=*/ smoothType == _SmoothNode::ANGULAR );
            usedNodes.erase( n );
          }
        }
        for ( size_t i = 0; i < lEdges.size() && !usedNodes.empty(); ++i )
        {
          n = usedNodes.find( /*tgtNode=*/ lEdges[i]->_nodes.back() );
          if ( n != usedNodes.end())
          {
            _Simplex::GetSimplices( lEdges[i]->_nodes.back(),
                                    lEdges[i]->_simplices,
                                    ignoreShapes );
            usedNodes.erase( n );
          }
        }
      }
      // TODO: check effect of this additional smooth
      // additional laplacian smooth to increase allowed shrink step
      // for ( int st = 1; st; --st )
      // {
      //   dumpFunction(SMESH_Comment("shrinkFace")<<f2sd->first<<"_st"<<++smooStep); // debug
      //   for ( size_t i = 0; i < nodesToSmooth.size(); ++i )
      //   {
      //     nodesToSmooth[i].Smooth( nbBad,surface,helper,refSign,
      //                              _SmoothNode::LAPLACIAN,/*set3D=*/false);
      //   }
      // }

    } // while ( shrinked )

    if ( !errMsg.empty() ) // Try to re-compute the shrink FACE
    {
      debugMsg( "Re-compute FACE " << f2sd->first << " because " << errMsg );

      // remove faces
      SMESHDS_SubMesh* psm = data._proxyMesh->getFaceSubM( F );
      {
        vector< const SMDS_MeshElement* > facesToRm;
        if ( psm )
        {
          facesToRm.reserve( psm->NbElements() );
          for ( SMDS_ElemIteratorPtr ite = psm->GetElements(); ite->more(); )
            facesToRm.push_back( ite->next() );

          for ( size_t i = 0 ; i < _sdVec.size(); ++i )
            if (( psm = _sdVec[i]._proxyMesh->getFaceSubM( F )))
              psm->Clear();
        }
        for ( size_t i = 0; i < facesToRm.size(); ++i )
          getMeshDS()->RemoveFreeElement( facesToRm[i], smDS, /*fromGroups=*/false );
      }
      // remove nodes
      {
        TIDSortedNodeSet nodesToKeep; // nodes of _LayerEdge to keep
        for ( size_t iS = 0; iS < subEOS.size(); ++iS ) {
          for ( size_t i = 0; i < subEOS[iS]->_edges.size(); ++i )
            nodesToKeep.insert( ++( subEOS[iS]->_edges[i]->_nodes.begin() ),
                                subEOS[iS]->_edges[i]->_nodes.end() );
        }
        SMDS_NodeIteratorPtr itn = smDS->GetNodes();
        while ( itn->more() ) {
          const SMDS_MeshNode* n = itn->next();
          if ( !nodesToKeep.count( n ))
            getMeshDS()->RemoveFreeNode( n, smDS, /*fromGroups=*/false );
        }
      }
      // restore position and UV of target nodes
      gp_Pnt p;
      for ( size_t iS = 0; iS < subEOS.size(); ++iS )
        for ( size_t i = 0; i < subEOS[iS]->_edges.size(); ++i )
        {
          _LayerEdge*       edge = subEOS[iS]->_edges[i];
          SMDS_MeshNode* tgtNode = const_cast< SMDS_MeshNode*& >( edge->_nodes.back() );
          if ( edge->_pos.empty() ||
               edge->Is( _LayerEdge::SHRUNK )) continue;
          if ( subEOS[iS]->SWOLType() == TopAbs_FACE )
          {
            SMDS_FacePosition* pos = static_cast<SMDS_FacePosition*>( tgtNode->GetPosition() );
            pos->SetUParameter( edge->_pos[0].X() );
            pos->SetVParameter( edge->_pos[0].Y() );
            p = surface->Value( edge->_pos[0].X(), edge->_pos[0].Y() );
          }
          else
          {
            SMDS_EdgePosition* pos = static_cast<SMDS_EdgePosition*>( tgtNode->GetPosition() );
            pos->SetUParameter( edge->_pos[0].Coord( U_TGT ));
            p = BRepAdaptor_Curve( TopoDS::Edge( subEOS[iS]->_sWOL )).Value( pos->GetUParameter() );
          }
          tgtNode->setXYZ( p.X(), p.Y(), p.Z() );
          dumpMove( tgtNode );
        }
      // shrink EDGE sub-meshes and set proxy sub-meshes
      UVPtStructVec uvPtVec;
      set< _Shrinker1D* >::iterator shrIt = eShri1D.begin();
      for ( shrIt = eShri1D.begin(); shrIt != eShri1D.end(); ++shrIt )
      {
        _Shrinker1D* shr = (*shrIt);
        shr->Compute( /*set3D=*/true, helper );

        // set proxy mesh of EDGEs w/o layers
        map< double, const SMDS_MeshNode* > nodes;
        SMESH_Algo::GetSortedNodesOnEdge( getMeshDS(), shr->GeomEdge(),/*skipMedium=*/true, nodes);
        // remove refinement nodes
        const SMDS_MeshNode* sn0 = shr->SrcNode(0), *sn1 = shr->SrcNode(1);
        const SMDS_MeshNode* tn0 = shr->TgtNode(0), *tn1 = shr->TgtNode(1);
        map< double, const SMDS_MeshNode* >::iterator u2n = nodes.begin();
        if ( u2n->second == sn0 || u2n->second == sn1 )
        {
          while ( u2n->second != tn0 && u2n->second != tn1 )
            ++u2n;
          nodes.erase( nodes.begin(), u2n );
        }
        u2n = --nodes.end();
        if ( u2n->second == sn0 || u2n->second == sn1 )
        {
          while ( u2n->second != tn0 && u2n->second != tn1 )
            --u2n;
          nodes.erase( ++u2n, nodes.end() );
        }
        // set proxy sub-mesh
        uvPtVec.resize( nodes.size() );
        u2n = nodes.begin();
        BRepAdaptor_Curve2d curve( shr->GeomEdge(), F );
        for ( size_t i = 0; i < nodes.size(); ++i, ++u2n )
        {
          uvPtVec[ i ].node = u2n->second;
          uvPtVec[ i ].param = u2n->first;
          uvPtVec[ i ].SetUV( curve.Value( u2n->first ).XY() );
        }
        StdMeshers_FaceSide fSide( uvPtVec, F, shr->GeomEdge(), _mesh );
        StdMeshers_ViscousLayers2D::SetProxyMeshOfEdge( fSide );
      }

      // set proxy mesh of EDGEs with layers
      vector< _LayerEdge* > edges;
      for ( size_t iS = 0; iS < subEOS.size(); ++iS )
      {
        _EdgesOnShape& eos = * subEOS[ iS ];
        if ( eos.ShapeType() != TopAbs_EDGE ) continue;

        const TopoDS_Edge& E = TopoDS::Edge( eos._shape );
        data.SortOnEdge( E, eos._edges );

        edges.clear();
        if ( _EdgesOnShape* eov = data.GetShapeEdges( helper.IthVertex( 0, E, /*CumOri=*/false )))
          if ( !eov->_edges.empty() )
            edges.push_back( eov->_edges[0] ); // on 1st VERTEX

        edges.insert( edges.end(), eos._edges.begin(), eos._edges.end() );

        if ( _EdgesOnShape* eov = data.GetShapeEdges( helper.IthVertex( 1, E, /*CumOri=*/false )))
          if ( !eov->_edges.empty() )
            edges.push_back( eov->_edges[0] ); // on last VERTEX

        uvPtVec.resize( edges.size() );
        for ( size_t i = 0; i < edges.size(); ++i )
        {
          uvPtVec[ i ].node = edges[i]->_nodes.back();
          uvPtVec[ i ].param = helper.GetNodeU( E, edges[i]->_nodes[0] );
          uvPtVec[ i ].SetUV( helper.GetNodeUV( F, edges[i]->_nodes.back() ));
        }
        BRep_Tool::Range( E, uvPtVec[0].param, uvPtVec.back().param );
        StdMeshers_FaceSide fSide( uvPtVec, F, E, _mesh );
        StdMeshers_ViscousLayers2D::SetProxyMeshOfEdge( fSide );
      }
      // temporary clear the FACE sub-mesh from faces made by refine()
      vector< const SMDS_MeshElement* > elems;
      elems.reserve( smDS->NbElements() + smDS->NbNodes() );
      for ( SMDS_ElemIteratorPtr ite = smDS->GetElements(); ite->more(); )
        elems.push_back( ite->next() );
      for ( SMDS_NodeIteratorPtr ite = smDS->GetNodes(); ite->more(); )
        elems.push_back( ite->next() );
      smDS->Clear();

      // compute the mesh on the FACE
      sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
      sm->ComputeStateEngine( SMESH_subMesh::COMPUTE_SUBMESH );

      // re-fill proxy sub-meshes of the FACE
      for ( size_t i = 0 ; i < _sdVec.size(); ++i )
        if (( psm = _sdVec[i]._proxyMesh->getFaceSubM( F )))
          for ( SMDS_ElemIteratorPtr ite = smDS->GetElements(); ite->more(); )
            psm->AddElement( ite->next() );

      // re-fill smDS
      for ( size_t i = 0; i < elems.size(); ++i )
        smDS->AddElement( elems[i] );

      if ( sm->GetComputeState() != SMESH_subMesh::COMPUTE_OK )
        return error( errMsg );

    } // end of re-meshing in case of failed smoothing
    else
    {
      // No wrongly shaped faces remain; final smooth. Set node XYZ.
      bool isStructuredFixed = false;
      if ( SMESH_2D_Algo* algo = dynamic_cast<SMESH_2D_Algo*>( sm->GetAlgo() ))
        isStructuredFixed = algo->FixInternalNodes( *data._proxyMesh, F );
      if ( !isStructuredFixed )
      {
        if ( isConcaveFace ) // fix narrow faces by swapping diagonals
          fixBadFaces( F, helper, /*is2D=*/false, ++shriStep );

        for ( int st = 3; st; --st )
        {
          switch( st ) {
          case 1: smoothType = _SmoothNode::LAPLACIAN; break;
          case 2: smoothType = _SmoothNode::LAPLACIAN; break;
          case 3: smoothType = _SmoothNode::ANGULAR; break;
          }
          dumpFunction(SMESH_Comment("shrinkFace")<<f2sd->first<<"_st"<<++smooStep); // debug
          for ( size_t i = 0; i < nodesToSmooth.size(); ++i )
          {
            nodesToSmooth[i].Smooth( nbBad,surface,helper,refSign,
                                     smoothType,/*set3D=*/st==1 );
          }
          dumpFunctionEnd();
        }
      }
      if ( !getMeshDS()->IsEmbeddedMode() )
        // Log node movement
        for ( size_t i = 0; i < nodesToSmooth.size(); ++i )
        {
          SMESH_TNodeXYZ p ( nodesToSmooth[i]._node );
          getMeshDS()->MoveNode( nodesToSmooth[i]._node, p.X(), p.Y(), p.Z() );
        }
    }

    // Set an event listener to clear FACE sub-mesh together with SOLID sub-mesh
    VISCOUS_3D::ToClearSubWithMain( sm, data._solid );
    if ( data2 )
      VISCOUS_3D::ToClearSubWithMain( sm, data2->_solid );

  } // loop on FACES to srink mesh on


  // Replace source nodes by target nodes in shrinked mesh edges

  map< int, _Shrinker1D >::iterator e2shr = e2shrMap.begin();
  for ( ; e2shr != e2shrMap.end(); ++e2shr )
    e2shr->second.SwapSrcTgtNodes( getMeshDS() );

  return true;
}

//================================================================================
/*!
 * \brief Computes 2d shrink direction and finds nodes limiting shrinking
 */
//================================================================================

bool _ViscousBuilder::prepareEdgeToShrink( _LayerEdge&            edge,
                                           _EdgesOnShape&         eos,
                                           SMESH_MesherHelper&    helper,
                                           const SMESHDS_SubMesh* faceSubMesh)
{
  const SMDS_MeshNode* srcNode = edge._nodes[0];
  const SMDS_MeshNode* tgtNode = edge._nodes.back();

  if ( eos.SWOLType() == TopAbs_FACE )
  {
    if ( tgtNode->GetPosition()->GetDim() != 2 ) // not inflated edge
    {
      edge._pos.clear();
      edge.Set( _LayerEdge::SHRUNK );
      return srcNode == tgtNode;
    }
    gp_XY srcUV ( edge._pos[0].X(), edge._pos[0].Y() );          //helper.GetNodeUV( F, srcNode );
    gp_XY tgtUV = edge.LastUV( TopoDS::Face( eos._sWOL ), eos ); //helper.GetNodeUV( F, tgtNode );
    gp_Vec2d uvDir( srcUV, tgtUV );
    double uvLen = uvDir.Magnitude();
    uvDir /= uvLen;
    edge._normal.SetCoord( uvDir.X(),uvDir.Y(), 0 );
    edge._len = uvLen;

    //edge._pos.resize(1);
    edge._pos[0].SetCoord( tgtUV.X(), tgtUV.Y(), 0 );

    // set UV of source node to target node
    SMDS_FacePosition* pos = static_cast<SMDS_FacePosition*>( tgtNode->GetPosition() );
    pos->SetUParameter( srcUV.X() );
    pos->SetVParameter( srcUV.Y() );
  }
  else // _sWOL is TopAbs_EDGE
  {
    if ( tgtNode->GetPosition()->GetDim() != 1 ) // not inflated edge
    {
      edge._pos.clear();
      edge.Set( _LayerEdge::SHRUNK );
      return srcNode == tgtNode;
    }
    const TopoDS_Edge&    E = TopoDS::Edge( eos._sWOL );
    SMESHDS_SubMesh* edgeSM = getMeshDS()->MeshElements( E );
    if ( !edgeSM || edgeSM->NbElements() == 0 )
      return error(SMESH_Comment("Not meshed EDGE ") << getMeshDS()->ShapeToIndex( E ));

    const SMDS_MeshNode* n2 = 0;
    SMDS_ElemIteratorPtr eIt = srcNode->GetInverseElementIterator(SMDSAbs_Edge);
    while ( eIt->more() && !n2 )
    {
      const SMDS_MeshElement* e = eIt->next();
      if ( !edgeSM->Contains(e)) continue;
      n2 = e->GetNode( 0 );
      if ( n2 == srcNode ) n2 = e->GetNode( 1 );
    }
    if ( !n2 )
      return error(SMESH_Comment("Wrongly meshed EDGE ") << getMeshDS()->ShapeToIndex( E ));

    double uSrc = helper.GetNodeU( E, srcNode, n2 );
    double uTgt = helper.GetNodeU( E, tgtNode, srcNode );
    double u2   = helper.GetNodeU( E, n2,      srcNode );

    //edge._pos.clear();

    if ( fabs( uSrc-uTgt ) < 0.99 * fabs( uSrc-u2 ))
    {
      // tgtNode is located so that it does not make faces with wrong orientation
      edge.Set( _LayerEdge::SHRUNK );
      return true;
    }
    //edge._pos.resize(1);
    edge._pos[0].SetCoord( U_TGT, uTgt );
    edge._pos[0].SetCoord( U_SRC, uSrc );
    edge._pos[0].SetCoord( LEN_TGT, fabs( uSrc-uTgt ));

    edge._simplices.resize( 1 );
    edge._simplices[0]._nPrev = n2;

    // set U of source node to the target node
    SMDS_EdgePosition* pos = static_cast<SMDS_EdgePosition*>( tgtNode->GetPosition() );
    pos->SetUParameter( uSrc );
  }
  return true;
}

//================================================================================
/*!
 * \brief Restore position of a sole node of a _LayerEdge based on _noShrinkShapes
 */
//================================================================================

void _ViscousBuilder::restoreNoShrink( _LayerEdge& edge ) const
{
  if ( edge._nodes.size() == 1 )
  {
    edge._pos.clear();
    edge._len = 0;

    const SMDS_MeshNode* srcNode = edge._nodes[0];
    TopoDS_Shape S = SMESH_MesherHelper::GetSubShapeByNode( srcNode, getMeshDS() );
    if ( S.IsNull() ) return;

    gp_Pnt p;

    switch ( S.ShapeType() )
    {
    case TopAbs_EDGE:
    {
      double f,l;
      TopLoc_Location loc;
      Handle(Geom_Curve) curve = BRep_Tool::Curve( TopoDS::Edge( S ), loc, f, l );
      if ( curve.IsNull() ) return;
      SMDS_EdgePosition* ePos = static_cast<SMDS_EdgePosition*>( srcNode->GetPosition() );
      p = curve->Value( ePos->GetUParameter() );
      break;
    }
    case TopAbs_VERTEX:
    {
      p = BRep_Tool::Pnt( TopoDS::Vertex( S ));
      break;
    }
    default: return;
    }
    getMeshDS()->MoveNode( srcNode, p.X(), p.Y(), p.Z() );
    dumpMove( srcNode );
  }
}

//================================================================================
/*!
 * \brief Try to fix triangles with high aspect ratio by swaping diagonals
 */
//================================================================================

void _ViscousBuilder::fixBadFaces(const TopoDS_Face&          F,
                                  SMESH_MesherHelper&         helper,
                                  const bool                  is2D,
                                  const int                   step,
                                  set<const SMDS_MeshNode*> * involvedNodes)
{
  SMESH::Controls::AspectRatio qualifier;
  SMESH::Controls::TSequenceOfXYZ points(3), points1(3), points2(3);
  const double maxAspectRatio = is2D ? 4. : 2;
  _NodeCoordHelper xyz( F, helper, is2D );

  // find bad triangles

  vector< const SMDS_MeshElement* > badTrias;
  vector< double >                  badAspects;
  SMESHDS_SubMesh*      sm = helper.GetMeshDS()->MeshElements( F );
  SMDS_ElemIteratorPtr fIt = sm->GetElements();
  while ( fIt->more() )
  {
    const SMDS_MeshElement * f = fIt->next();
    if ( f->NbCornerNodes() != 3 ) continue;
    for ( int iP = 0; iP < 3; ++iP ) points(iP+1) = xyz( f->GetNode(iP));
    double aspect = qualifier.GetValue( points );
    if ( aspect > maxAspectRatio )
    {
      badTrias.push_back( f );
      badAspects.push_back( aspect );
    }
  }
  if ( step == 1 )
  {
    dumpFunction(SMESH_Comment("beforeSwapDiagonals_F")<<helper.GetSubShapeID());
    SMDS_ElemIteratorPtr fIt = sm->GetElements();
    while ( fIt->more() )
    {
      const SMDS_MeshElement * f = fIt->next();
      if ( f->NbCornerNodes() == 3 )
        dumpChangeNodes( f );
    }
    dumpFunctionEnd();
  }
  if ( badTrias.empty() )
    return;

  // find couples of faces to swap diagonal

  typedef pair < const SMDS_MeshElement* , const SMDS_MeshElement* > T2Trias;
  vector< T2Trias > triaCouples; 

  TIDSortedElemSet involvedFaces, emptySet;
  for ( size_t iTia = 0; iTia < badTrias.size(); ++iTia )
  {
    T2Trias trias    [3];
    double  aspRatio [3];
    int i1, i2, i3;

    if ( !involvedFaces.insert( badTrias[iTia] ).second )
      continue;
    for ( int iP = 0; iP < 3; ++iP )
      points(iP+1) = xyz( badTrias[iTia]->GetNode(iP));

    // find triangles adjacent to badTrias[iTia] with better aspect ratio after diag-swaping
    int bestCouple = -1;
    for ( int iSide = 0; iSide < 3; ++iSide )
    {
      const SMDS_MeshNode* n1 = badTrias[iTia]->GetNode( iSide );
      const SMDS_MeshNode* n2 = badTrias[iTia]->GetNode(( iSide+1 ) % 3 );
      trias [iSide].first  = badTrias[iTia];
      trias [iSide].second = SMESH_MeshAlgos::FindFaceInSet( n1, n2, emptySet, involvedFaces,
                                                             & i1, & i2 );
      if (( ! trias[iSide].second ) ||
          ( trias[iSide].second->NbCornerNodes() != 3 ) ||
          ( ! sm->Contains( trias[iSide].second )))
        continue;

      // aspect ratio of an adjacent tria
      for ( int iP = 0; iP < 3; ++iP )
        points2(iP+1) = xyz( trias[iSide].second->GetNode(iP));
      double aspectInit = qualifier.GetValue( points2 );

      // arrange nodes as after diag-swaping
      if ( helper.WrapIndex( i1+1, 3 ) == i2 )
        i3 = helper.WrapIndex( i1-1, 3 );
      else
        i3 = helper.WrapIndex( i1+1, 3 );
      points1 = points;
      points1( 1+ iSide ) = points2( 1+ i3 );
      points2( 1+ i2    ) = points1( 1+ ( iSide+2 ) % 3 );

      // aspect ratio after diag-swaping
      aspRatio[ iSide ] = qualifier.GetValue( points1 ) + qualifier.GetValue( points2 );
      if ( aspRatio[ iSide ] > aspectInit + badAspects[ iTia ] )
        continue;

      // prevent inversion of a triangle
      gp_Vec norm1 = gp_Vec( points1(1), points1(3) ) ^ gp_Vec( points1(1), points1(2) );
      gp_Vec norm2 = gp_Vec( points2(1), points2(3) ) ^ gp_Vec( points2(1), points2(2) );
      if ( norm1 * norm2 < 0. && norm1.Angle( norm2 ) > 70./180.*M_PI )
        continue;

      if ( bestCouple < 0 || aspRatio[ bestCouple ] > aspRatio[ iSide ] )
        bestCouple = iSide;
    }

    if ( bestCouple >= 0 )
    {
      triaCouples.push_back( trias[bestCouple] );
      involvedFaces.insert ( trias[bestCouple].second );
    }
    else
    {
      involvedFaces.erase( badTrias[iTia] );
    }
  }
  if ( triaCouples.empty() )
    return;

  // swap diagonals

  SMESH_MeshEditor editor( helper.GetMesh() );
  dumpFunction(SMESH_Comment("beforeSwapDiagonals_F")<<helper.GetSubShapeID()<<"_"<<step);
  for ( size_t i = 0; i < triaCouples.size(); ++i )
  {
    dumpChangeNodes( triaCouples[i].first );
    dumpChangeNodes( triaCouples[i].second );
    editor.InverseDiag( triaCouples[i].first, triaCouples[i].second );
  }

  if ( involvedNodes )
    for ( size_t i = 0; i < triaCouples.size(); ++i )
    {
      involvedNodes->insert( triaCouples[i].first->begin_nodes(),
                             triaCouples[i].first->end_nodes() );
      involvedNodes->insert( triaCouples[i].second->begin_nodes(),
                             triaCouples[i].second->end_nodes() );
    }

  // just for debug dump resulting triangles
  dumpFunction(SMESH_Comment("swapDiagonals_F")<<helper.GetSubShapeID()<<"_"<<step);
  for ( size_t i = 0; i < triaCouples.size(); ++i )
  {
    dumpChangeNodes( triaCouples[i].first );
    dumpChangeNodes( triaCouples[i].second );
  }
}

//================================================================================
/*!
 * \brief Move target node to it's final position on the FACE during shrinking
 */
//================================================================================

bool _LayerEdge::SetNewLength2d( Handle(Geom_Surface)& surface,
                                 const TopoDS_Face&    F,
                                 _EdgesOnShape&        eos,
                                 SMESH_MesherHelper&   helper )
{
  if ( Is( SHRUNK ))
    return false; // already at the target position

  SMDS_MeshNode* tgtNode = const_cast< SMDS_MeshNode*& >( _nodes.back() );

  if ( eos.SWOLType() == TopAbs_FACE )
  {
    gp_XY    curUV = helper.GetNodeUV( F, tgtNode );
    gp_Pnt2d tgtUV( _pos[0].X(), _pos[0].Y() );
    gp_Vec2d uvDir( _normal.X(), _normal.Y() );
    const double uvLen = tgtUV.Distance( curUV );
    const double kSafe = Max( 0.5, 1. - 0.1 * _simplices.size() );

    // Select shrinking step such that not to make faces with wrong orientation.
    double stepSize = 1e100;
    for ( size_t i = 0; i < _simplices.size(); ++i )
    {
      if ( !_simplices[i]._nPrev->isMarked() ||
           !_simplices[i]._nNext->isMarked() )
        continue; // simplex of quadrangle created by addBoundaryElements()

      // find intersection of 2 lines: curUV-tgtUV and that connecting simplex nodes
      gp_XY uvN1 = helper.GetNodeUV( F, _simplices[i]._nPrev );
      gp_XY uvN2 = helper.GetNodeUV( F, _simplices[i]._nNext );
      gp_XY dirN = uvN2 - uvN1;
      double det = uvDir.Crossed( dirN );
      if ( Abs( det )  < std::numeric_limits<double>::min() ) continue;
      gp_XY dirN2Cur = curUV - uvN1;
      double step = dirN.Crossed( dirN2Cur ) / det;
      if ( step > 0 )
        stepSize = Min( step, stepSize );
    }
    gp_Pnt2d newUV;
    if ( uvLen <= stepSize )
    {
      newUV = tgtUV;
      Set( SHRUNK );
      //_pos.clear();
    }
    else if ( stepSize > 0 )
    {
      newUV = curUV + uvDir.XY() * stepSize * kSafe;
    }
    else
    {
      return true;
    }
    SMDS_FacePosition* pos = static_cast<SMDS_FacePosition*>( tgtNode->GetPosition() );
    pos->SetUParameter( newUV.X() );
    pos->SetVParameter( newUV.Y() );

#ifdef __myDEBUG
    gp_Pnt p = surface->Value( newUV.X(), newUV.Y() );
    tgtNode->setXYZ( p.X(), p.Y(), p.Z() );
    dumpMove( tgtNode );
#endif
  }
  else // _sWOL is TopAbs_EDGE
  {
    const TopoDS_Edge&      E = TopoDS::Edge( eos._sWOL );
    const SMDS_MeshNode*   n2 = _simplices[0]._nPrev;
    SMDS_EdgePosition* tgtPos = static_cast<SMDS_EdgePosition*>( tgtNode->GetPosition() );

    const double u2     = helper.GetNodeU( E, n2, tgtNode );
    const double uSrc   = _pos[0].Coord( U_SRC );
    const double lenTgt = _pos[0].Coord( LEN_TGT );

    double newU = _pos[0].Coord( U_TGT );
    if ( lenTgt < 0.99 * fabs( uSrc-u2 )) // n2 got out of src-tgt range
    {
      Set( _LayerEdge::SHRUNK );
      //_pos.clear();
    }
    else
    {
      newU = 0.1 * tgtPos->GetUParameter() + 0.9 * u2;
    }
    tgtPos->SetUParameter( newU );
#ifdef __myDEBUG
    gp_XY newUV = helper.GetNodeUV( F, tgtNode, _nodes[0]);
    gp_Pnt p = surface->Value( newUV.X(), newUV.Y() );
    tgtNode->setXYZ( p.X(), p.Y(), p.Z() );
    dumpMove( tgtNode );
#endif
  }

  return true;
}

//================================================================================
/*!
 * \brief Perform smooth on the FACE
 *  \retval bool - true if the node has been moved
 */
//================================================================================

bool _SmoothNode::Smooth(int&                  nbBad,
                         Handle(Geom_Surface)& surface,
                         SMESH_MesherHelper&   helper,
                         const double          refSign,
                         SmoothType            how,
                         bool                  set3D)
{
  const TopoDS_Face& face = TopoDS::Face( helper.GetSubShape() );

  // get uv of surrounding nodes
  vector<gp_XY> uv( _simplices.size() );
  for ( size_t i = 0; i < _simplices.size(); ++i )
    uv[i] = helper.GetNodeUV( face, _simplices[i]._nPrev, _node );

  // compute new UV for the node
  gp_XY newPos (0,0);
  if ( how == TFI && _simplices.size() == 4 )
  {
    gp_XY corners[4];
    for ( size_t i = 0; i < _simplices.size(); ++i )
      if ( _simplices[i]._nOpp )
        corners[i] = helper.GetNodeUV( face, _simplices[i]._nOpp, _node );
      else
        throw SALOME_Exception(LOCALIZED("TFI smoothing: _Simplex::_nOpp not set!"));

    newPos = helper.calcTFI ( 0.5, 0.5,
                              corners[0], corners[1], corners[2], corners[3],
                              uv[1], uv[2], uv[3], uv[0] );
  }
  else if ( how == ANGULAR )
  {
    newPos = computeAngularPos( uv, helper.GetNodeUV( face, _node ), refSign );
  }
  else if ( how == CENTROIDAL && _simplices.size() > 3 )
  {
    // average centers of diagonals wieghted with their reciprocal lengths
    if ( _simplices.size() == 4 )
    {
      double w1 = 1. / ( uv[2]-uv[0] ).SquareModulus();
      double w2 = 1. / ( uv[3]-uv[1] ).SquareModulus();
      newPos = ( w1 * ( uv[2]+uv[0] ) + w2 * ( uv[3]+uv[1] )) / ( w1+w2 ) / 2;
    }
    else
    {
      double sumWeight = 0;
      int nb = _simplices.size() == 4 ? 2 : _simplices.size();
      for ( int i = 0; i < nb; ++i )
      {
        int iFrom = i + 2;
        int iTo   = i + _simplices.size() - 1;
        for ( int j = iFrom; j < iTo; ++j )
        {
          int i2 = SMESH_MesherHelper::WrapIndex( j, _simplices.size() );
          double w = 1. / ( uv[i]-uv[i2] ).SquareModulus();
          sumWeight += w;
          newPos += w * ( uv[i]+uv[i2] );
        }
      }
      newPos /= 2 * sumWeight; // 2 is to get a middle between uv's
    }
  }
  else
  {
    // Laplacian smooth
    for ( size_t i = 0; i < _simplices.size(); ++i )
      newPos += uv[i];
    newPos /= _simplices.size();
  }

  // count quality metrics (orientation) of triangles around the node
  int nbOkBefore = 0;
  gp_XY tgtUV = helper.GetNodeUV( face, _node );
  for ( size_t i = 0; i < _simplices.size(); ++i )
    nbOkBefore += _simplices[i].IsForward( tgtUV, _node, face, helper, refSign );

  int nbOkAfter = 0;
  for ( size_t i = 0; i < _simplices.size(); ++i )
    nbOkAfter += _simplices[i].IsForward( newPos, _node, face, helper, refSign );

  if ( nbOkAfter < nbOkBefore )
  {
    nbBad += _simplices.size() - nbOkBefore;
    return false;
  }

  SMDS_FacePosition* pos = static_cast<SMDS_FacePosition*>( _node->GetPosition() );
  pos->SetUParameter( newPos.X() );
  pos->SetVParameter( newPos.Y() );

#ifdef __myDEBUG
  set3D = true;
#endif
  if ( set3D )
  {
    gp_Pnt p = surface->Value( newPos.X(), newPos.Y() );
    const_cast< SMDS_MeshNode* >( _node )->setXYZ( p.X(), p.Y(), p.Z() );
    dumpMove( _node );
  }

  nbBad += _simplices.size() - nbOkAfter;
  return ( (tgtUV-newPos).SquareModulus() > 1e-10 );
}

//================================================================================
/*!
 * \brief Computes new UV using angle based smoothing technic
 */
//================================================================================

gp_XY _SmoothNode::computeAngularPos(vector<gp_XY>& uv,
                                     const gp_XY&   uvToFix,
                                     const double   refSign)
{
  uv.push_back( uv.front() );

  vector< gp_XY >  edgeDir ( uv.size() );
  vector< double > edgeSize( uv.size() );
  for ( size_t i = 1; i < edgeDir.size(); ++i )
  {
    edgeDir [i-1] = uv[i] - uv[i-1];
    edgeSize[i-1] = edgeDir[i-1].Modulus();
    if ( edgeSize[i-1] < numeric_limits<double>::min() )
      edgeDir[i-1].SetX( 100 );
    else
      edgeDir[i-1] /= edgeSize[i-1] * refSign;
  }
  edgeDir.back()  = edgeDir.front();
  edgeSize.back() = edgeSize.front();

  gp_XY  newPos(0,0);
  //int    nbEdges = 0;
  double sumSize = 0;
  for ( size_t i = 1; i < edgeDir.size(); ++i )
  {
    if ( edgeDir[i-1].X() > 1. ) continue;
    int i1 = i-1;
    while ( edgeDir[i].X() > 1. && ++i < edgeDir.size() );
    if ( i == edgeDir.size() ) break;
    gp_XY p = uv[i];
    gp_XY norm1( -edgeDir[i1].Y(), edgeDir[i1].X() );
    gp_XY norm2( -edgeDir[i].Y(),  edgeDir[i].X() );
    gp_XY bisec = norm1 + norm2;
    double bisecSize = bisec.Modulus();
    if ( bisecSize < numeric_limits<double>::min() )
    {
      bisec = -edgeDir[i1] + edgeDir[i];
      bisecSize = bisec.Modulus();
    }
    bisec /= bisecSize;

    gp_XY  dirToN  = uvToFix - p;
    double distToN = dirToN.Modulus();
    if ( bisec * dirToN < 0 )
      distToN = -distToN;

    newPos += ( p + bisec * distToN ) * ( edgeSize[i1] + edgeSize[i] );
    //++nbEdges;
    sumSize += edgeSize[i1] + edgeSize[i];
  }
  newPos /= /*nbEdges * */sumSize;
  return newPos;
}

//================================================================================
/*!
 * \brief Delete _SolidData
 */
//================================================================================

_SolidData::~_SolidData()
{
  TNode2Edge::iterator n2e = _n2eMap.begin();
  for ( ; n2e != _n2eMap.end(); ++n2e )
  {
    _LayerEdge* & e = n2e->second;
    if ( e )
    {
      delete e->_curvature;
      if ( e->_2neibors )
        delete e->_2neibors->_plnNorm;
      delete e->_2neibors;
    }
    delete e;
    e = 0;
  }
  _n2eMap.clear();

  delete _helper;
  _helper = 0;
}

//================================================================================
/*!
 * \brief Keep a _LayerEdge inflated along the EDGE
 */
//================================================================================

void _Shrinker1D::AddEdge( const _LayerEdge*   e,
                           _EdgesOnShape&      eos,
                           SMESH_MesherHelper& helper )
{
  // init
  if ( _nodes.empty() )
  {
    _edges[0] = _edges[1] = 0;
    _done = false;
  }
  // check _LayerEdge
  if ( e == _edges[0] || e == _edges[1] || e->_nodes.size() < 2 )
    return;
  if ( eos.SWOLType() != TopAbs_EDGE )
    throw SALOME_Exception(LOCALIZED("Wrong _LayerEdge is added"));
  if ( _edges[0] && !_geomEdge.IsSame( eos._sWOL ))
    throw SALOME_Exception(LOCALIZED("Wrong _LayerEdge is added"));

  // store _LayerEdge
  _geomEdge = TopoDS::Edge( eos._sWOL );
  double f,l;
  BRep_Tool::Range( _geomEdge, f,l );
  double u = helper.GetNodeU( _geomEdge, e->_nodes[0], e->_nodes.back());
  _edges[ u < 0.5*(f+l) ? 0 : 1 ] = e;

  // Update _nodes

  const SMDS_MeshNode* tgtNode0 = TgtNode( 0 );
  const SMDS_MeshNode* tgtNode1 = TgtNode( 1 );

  if ( _nodes.empty() )
  {
    SMESHDS_SubMesh * eSubMesh = helper.GetMeshDS()->MeshElements( _geomEdge );
    if ( !eSubMesh || eSubMesh->NbNodes() < 1 )
      return;
    TopLoc_Location loc;
    Handle(Geom_Curve) C = BRep_Tool::Curve( _geomEdge, loc, f,l );
    GeomAdaptor_Curve aCurve(C, f,l);
    const double totLen = GCPnts_AbscissaPoint::Length(aCurve, f, l);

    int nbExpectNodes = eSubMesh->NbNodes();
    _initU  .reserve( nbExpectNodes );
    _normPar.reserve( nbExpectNodes );
    _nodes  .reserve( nbExpectNodes );
    SMDS_NodeIteratorPtr nIt = eSubMesh->GetNodes();
    while ( nIt->more() )
    {
      const SMDS_MeshNode* node = nIt->next();

      // skip refinement nodes
      if ( node->NbInverseElements(SMDSAbs_Edge) == 0 ||
           node == tgtNode0 || node == tgtNode1 )
        continue;
      bool hasMarkedFace = false;
      SMDS_ElemIteratorPtr fIt = node->GetInverseElementIterator(SMDSAbs_Face);
      while ( fIt->more() && !hasMarkedFace )
        hasMarkedFace = fIt->next()->isMarked();
      if ( !hasMarkedFace )
        continue;

      _nodes.push_back( node );
      _initU.push_back( helper.GetNodeU( _geomEdge, node ));
      double len = GCPnts_AbscissaPoint::Length(aCurve, f, _initU.back());
      _normPar.push_back(  len / totLen );
    }
  }
  else
  {
    // remove target node of the _LayerEdge from _nodes
    size_t nbFound = 0;
    for ( size_t i = 0; i < _nodes.size(); ++i )
      if ( !_nodes[i] || _nodes[i] == tgtNode0 || _nodes[i] == tgtNode1 )
        _nodes[i] = 0, nbFound++;
    if ( nbFound == _nodes.size() )
      _nodes.clear();
  }
}

//================================================================================
/*!
 * \brief Move nodes on EDGE from ends where _LayerEdge's are inflated
 */
//================================================================================

void _Shrinker1D::Compute(bool set3D, SMESH_MesherHelper& helper)
{
  if ( _done || _nodes.empty())
    return;
  const _LayerEdge* e = _edges[0];
  if ( !e ) e = _edges[1];
  if ( !e ) return;

  _done =  (( !_edges[0] || _edges[0]->Is( _LayerEdge::SHRUNK )) &&
            ( !_edges[1] || _edges[1]->Is( _LayerEdge::SHRUNK )));

  double f,l;
  if ( set3D || _done )
  {
    Handle(Geom_Curve) C = BRep_Tool::Curve(_geomEdge, f,l);
    GeomAdaptor_Curve aCurve(C, f,l);

    if ( _edges[0] )
      f = helper.GetNodeU( _geomEdge, _edges[0]->_nodes.back(), _nodes[0] );
    if ( _edges[1] )
      l = helper.GetNodeU( _geomEdge, _edges[1]->_nodes.back(), _nodes.back() );
    double totLen = GCPnts_AbscissaPoint::Length( aCurve, f, l );

    for ( size_t i = 0; i < _nodes.size(); ++i )
    {
      if ( !_nodes[i] ) continue;
      double len = totLen * _normPar[i];
      GCPnts_AbscissaPoint discret( aCurve, len, f );
      if ( !discret.IsDone() )
        return throw SALOME_Exception(LOCALIZED("GCPnts_AbscissaPoint failed"));
      double u = discret.Parameter();
      SMDS_EdgePosition* pos = static_cast<SMDS_EdgePosition*>( _nodes[i]->GetPosition() );
      pos->SetUParameter( u );
      gp_Pnt p = C->Value( u );
      const_cast< SMDS_MeshNode*>( _nodes[i] )->setXYZ( p.X(), p.Y(), p.Z() );
    }
  }
  else
  {
    BRep_Tool::Range( _geomEdge, f,l );
    if ( _edges[0] )
      f = helper.GetNodeU( _geomEdge, _edges[0]->_nodes.back(), _nodes[0] );
    if ( _edges[1] )
      l = helper.GetNodeU( _geomEdge, _edges[1]->_nodes.back(), _nodes.back() );
    
    for ( size_t i = 0; i < _nodes.size(); ++i )
    {
      if ( !_nodes[i] ) continue;
      double u = f * ( 1-_normPar[i] ) + l * _normPar[i];
      SMDS_EdgePosition* pos = static_cast<SMDS_EdgePosition*>( _nodes[i]->GetPosition() );
      pos->SetUParameter( u );
    }
  }
}

//================================================================================
/*!
 * \brief Restore initial parameters of nodes on EDGE
 */
//================================================================================

void _Shrinker1D::RestoreParams()
{
  if ( _done )
    for ( size_t i = 0; i < _nodes.size(); ++i )
    {
      if ( !_nodes[i] ) continue;
      SMDS_EdgePosition* pos = static_cast<SMDS_EdgePosition*>( _nodes[i]->GetPosition() );
      pos->SetUParameter( _initU[i] );
    }
  _done = false;
}

//================================================================================
/*!
 * \brief Replace source nodes by target nodes in shrinked mesh edges
 */
//================================================================================

void _Shrinker1D::SwapSrcTgtNodes( SMESHDS_Mesh* mesh )
{
  const SMDS_MeshNode* nodes[3];
  for ( int i = 0; i < 2; ++i )
  {
    if ( !_edges[i] ) continue;

    SMESHDS_SubMesh * eSubMesh = mesh->MeshElements( _geomEdge );
    if ( !eSubMesh ) return;
    const SMDS_MeshNode* srcNode = _edges[i]->_nodes[0];
    const SMDS_MeshNode* tgtNode = _edges[i]->_nodes.back();
    const SMDS_MeshNode* scdNode = _edges[i]->_nodes[1];
    SMDS_ElemIteratorPtr eIt = srcNode->GetInverseElementIterator(SMDSAbs_Edge);
    while ( eIt->more() )
    {
      const SMDS_MeshElement* e = eIt->next();
      if ( !eSubMesh->Contains( e ) || e->GetNodeIndex( scdNode ) >= 0 )
          continue;
      SMDS_ElemIteratorPtr nIt = e->nodesIterator();
      for ( int iN = 0; iN < e->NbNodes(); ++iN )
      {
        const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nIt->next() );
        nodes[iN] = ( n == srcNode ? tgtNode : n );
      }
      mesh->ChangeElementNodes( e, nodes, e->NbNodes() );
    }
  }
}

//================================================================================
/*!
 * \brief Creates 2D and 1D elements on boundaries of new prisms
 */
//================================================================================

bool _ViscousBuilder::addBoundaryElements(_SolidData& data)
{
  SMESH_MesherHelper helper( *_mesh );

  vector< const SMDS_MeshNode* > faceNodes;

  //for ( size_t i = 0; i < _sdVec.size(); ++i )
  {
    //_SolidData& data = _sdVec[i];
    TopTools_IndexedMapOfShape geomEdges;
    TopExp::MapShapes( data._solid, TopAbs_EDGE, geomEdges );
    for ( int iE = 1; iE <= geomEdges.Extent(); ++iE )
    {
      const TopoDS_Edge& E = TopoDS::Edge( geomEdges(iE));
      const TGeomID edgeID = getMeshDS()->ShapeToIndex( E );
      if ( data._noShrinkShapes.count( edgeID ))
        continue;

      // Get _LayerEdge's based on E

      map< double, const SMDS_MeshNode* > u2nodes;
      if ( !SMESH_Algo::GetSortedNodesOnEdge( getMeshDS(), E, /*ignoreMedium=*/false, u2nodes))
        continue;

      vector< _LayerEdge* > ledges; ledges.reserve( u2nodes.size() );
      TNode2Edge & n2eMap = data._n2eMap;
      map< double, const SMDS_MeshNode* >::iterator u2n = u2nodes.begin();
      {
        //check if 2D elements are needed on E
        TNode2Edge::iterator n2e = n2eMap.find( u2n->second );
        if ( n2e == n2eMap.end() ) continue; // no layers on vertex
        ledges.push_back( n2e->second );
        u2n++;
        if (( n2e = n2eMap.find( u2n->second )) == n2eMap.end() )
          continue; // no layers on E
        ledges.push_back( n2eMap[ u2n->second ]);

        const SMDS_MeshNode* tgtN0 = ledges[0]->_nodes.back();
        const SMDS_MeshNode* tgtN1 = ledges[1]->_nodes.back();
        int nbSharedPyram = 0;
        SMDS_ElemIteratorPtr vIt = tgtN0->GetInverseElementIterator(SMDSAbs_Volume);
        while ( vIt->more() )
        {
          const SMDS_MeshElement* v = vIt->next();
          nbSharedPyram += int( v->GetNodeIndex( tgtN1 ) >= 0 );
        }
        if ( nbSharedPyram > 1 )
          continue; // not free border of the pyramid

        faceNodes.clear();
        faceNodes.push_back( ledges[0]->_nodes[0] );
        faceNodes.push_back( ledges[1]->_nodes[0] );
        if ( ledges[0]->_nodes.size() > 1 ) faceNodes.push_back( ledges[0]->_nodes[1] );
        if ( ledges[1]->_nodes.size() > 1 ) faceNodes.push_back( ledges[1]->_nodes[1] );

        if ( getMeshDS()->FindElement( faceNodes, SMDSAbs_Face, /*noMedium=*/true))
          continue; // faces already created
      }
      for ( ++u2n; u2n != u2nodes.end(); ++u2n )
        ledges.push_back( n2eMap[ u2n->second ]);

      // Find out orientation and type of face to create

      bool reverse = false, isOnFace;
      TopoDS_Shape F;

      map< TGeomID, TopoDS_Shape >::iterator e2f = data._shrinkShape2Shape.find( edgeID );
      if (( isOnFace = ( e2f != data._shrinkShape2Shape.end() )))
      {
        F = e2f->second.Oriented( TopAbs_FORWARD );
        reverse = ( helper.GetSubShapeOri( F, E ) == TopAbs_REVERSED );
        if ( helper.GetSubShapeOri( data._solid, F ) == TopAbs_REVERSED )
          reverse = !reverse, F.Reverse();
        if ( helper.IsReversedSubMesh( TopoDS::Face(F) ))
          reverse = !reverse;
      }
      else if ( !data._ignoreFaceIds.count( e2f->first ))
      {
        // find FACE with layers sharing E
        PShapeIteratorPtr fIt = helper.GetAncestors( E, *_mesh, TopAbs_FACE, &data._solid );
        if ( fIt->more() )
          F = *( fIt->next() );
      }
      // Find the sub-mesh to add new faces
      SMESHDS_SubMesh* sm = 0;
      if ( isOnFace )
        sm = getMeshDS()->MeshElements( F );
      else
        sm = data._proxyMesh->getFaceSubM( TopoDS::Face(F), /*create=*/true );
      if ( !sm )
        return error("error in addBoundaryElements()", data._index);

      // Find a proxy sub-mesh of the FACE of an adjacent SOLID, which will use the new boundary
      // faces for 3D meshing (PAL23414)
      SMESHDS_SubMesh* adjSM = 0;
      if ( isOnFace )
      {
        const TGeomID   faceID = sm->GetID();
        PShapeIteratorPtr soIt = helper.GetAncestors( F, *_mesh, TopAbs_SOLID );
        while ( const TopoDS_Shape* solid = soIt->next() )
          if ( !solid->IsSame( data._solid ))
          {
            size_t iData = _solids.FindIndex( *solid ) - 1;
            if ( iData < _sdVec.size() &&
                 _sdVec[ iData ]._ignoreFaceIds.count( faceID ) &&
                 _sdVec[ iData ]._shrinkShape2Shape.count( edgeID ) == 0 )
            {
              SMESH_ProxyMesh::SubMesh* proxySub =
                _sdVec[ iData ]._proxyMesh->getFaceSubM( TopoDS::Face( F ), /*create=*/false);
              if ( proxySub && proxySub->NbElements() > 0 )
                adjSM = proxySub;
            }
          }
      }

      // Make faces
      const int dj1 = reverse ? 0 : 1;
      const int dj2 = reverse ? 1 : 0;
      vector< const SMDS_MeshElement*> ff; // new faces row
      SMESHDS_Mesh* m = getMeshDS();
      for ( size_t j = 1; j < ledges.size(); ++j )
      {
        vector< const SMDS_MeshNode*>&  nn1 = ledges[j-dj1]->_nodes;
        vector< const SMDS_MeshNode*>&  nn2 = ledges[j-dj2]->_nodes;
        ff.resize( std::max( nn1.size(), nn2.size() ), NULL );
        if ( nn1.size() == nn2.size() )
        {
          if ( isOnFace )
            for ( size_t z = 1; z < nn1.size(); ++z )
              sm->AddElement( ff[z-1] = m->AddFace( nn1[z-1], nn2[z-1], nn2[z], nn1[z] ));
          else
            for ( size_t z = 1; z < nn1.size(); ++z )
              sm->AddElement( new SMDS_FaceOfNodes( nn1[z-1], nn2[z-1], nn2[z], nn1[z] ));
        }
        else if ( nn1.size() == 1 )
        {
          if ( isOnFace )
            for ( size_t z = 1; z < nn2.size(); ++z )
              sm->AddElement( ff[z-1] = m->AddFace( nn1[0], nn2[z-1], nn2[z] ));
          else
            for ( size_t z = 1; z < nn2.size(); ++z )
              sm->AddElement( new SMDS_FaceOfNodes( nn1[0], nn2[z-1], nn2[z] ));
        }
        else
        {
          if ( isOnFace )
            for ( size_t z = 1; z < nn1.size(); ++z )
              sm->AddElement( ff[z-1] = m->AddFace( nn1[z-1], nn2[0], nn1[z] ));
          else
            for ( size_t z = 1; z < nn1.size(); ++z )
              sm->AddElement( new SMDS_FaceOfNodes( nn1[z-1], nn2[0], nn2[z] ));
        }

        if ( adjSM ) // add faces to a proxy SM of the adjacent SOLID
        {
          for ( size_t z = 0; z < ff.size(); ++z )
            if ( ff[ z ])
              adjSM->AddElement( ff[ z ]);
          ff.clear();
        }
      }

      // Make edges
      for ( int isFirst = 0; isFirst < 2; ++isFirst )
      {
        _LayerEdge* edge = isFirst ? ledges.front() : ledges.back();
        _EdgesOnShape* eos = data.GetShapeEdges( edge );
        if ( eos && eos->SWOLType() == TopAbs_EDGE )
        {
          vector< const SMDS_MeshNode*>&  nn = edge->_nodes;
          if ( nn.size() < 2 || nn[1]->NbInverseElements( SMDSAbs_Edge ) >= 2 )
            continue;
          helper.SetSubShape( eos->_sWOL );
          helper.SetElementsOnShape( true );
          for ( size_t z = 1; z < nn.size(); ++z )
            helper.AddEdge( nn[z-1], nn[z] );
        }
      }

    } // loop on EDGE's
  } // loop on _SolidData's

  return true;
}
