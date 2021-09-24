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
//  File   : StdMeshers_Cartesian_3D.cxx
//  Module : SMESH
//

// Suppress warning due to use of #import an macOS inside Aspect_RenderingContext.hxx
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wimport-preprocessor-directive-pedantic"
#endif

#include "StdMeshers_Cartesian_3D.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "StdMeshers_CartesianParameters3D.hxx"

#include <utilities.h>
#include <Utils_ExceptHandlers.hxx>
#include <Basics_OCCTVersion.hxx>

#include <GEOMUtils.hxx>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepTools.hxx>
#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_B3d.hxx>
#include <Bnd_Box.hxx>
#include <ElSLib.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomLib.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <IntAna_IntConicQuad.hxx>
#include <IntAna_IntLinTorus.hxx>
#include <IntAna_Quadric.hxx>
#include <IntCurveSurface_TransitionOnCurve.hxx>
#include <IntCurvesFace_Intersector.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_TShape.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Sphere.hxx>
#include <gp_Torus.hxx>

#include <limits>

//#undef WITH_TBB
#ifdef WITH_TBB
#include <tbb/parallel_for.h>
//#include <tbb/enumerable_thread_specific.h>
#endif

using namespace std;

#ifdef _DEBUG_
//#define _MY_DEBUG_
#endif

//=============================================================================
/*!
 * Constructor
 */
//=============================================================================

StdMeshers_Cartesian_3D::StdMeshers_Cartesian_3D(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_3D_Algo(hypId, studyId, gen)
{
  _name = "Cartesian_3D";
  _shapeType = (1 << TopAbs_SOLID);       // 1 bit /shape type
  _compatibleHypothesis.push_back("CartesianParameters3D");

  _onlyUnaryInput = false;          // to mesh all SOLIDs at once
  _requireDiscreteBoundary = false; // 2D mesh not needed
  _supportSubmeshes = false;        // do not use any existing mesh
}

//=============================================================================
/*!
 * Check presence of a hypothesis
 */
//=============================================================================

bool StdMeshers_Cartesian_3D::CheckHypothesis (SMESH_Mesh&          aMesh,
                                               const TopoDS_Shape&  aShape,
                                               Hypothesis_Status&   aStatus)
{
  aStatus = SMESH_Hypothesis::HYP_MISSING;

  const list<const SMESHDS_Hypothesis*>& hyps = GetUsedHypothesis(aMesh, aShape);
  list <const SMESHDS_Hypothesis* >::const_iterator h = hyps.begin();
  if ( h == hyps.end())
  {
    return false;
  }

  for ( ; h != hyps.end(); ++h )
  {
    if (( _hyp = dynamic_cast<const StdMeshers_CartesianParameters3D*>( *h )))
    {
      aStatus = _hyp->IsDefined() ? HYP_OK : HYP_BAD_PARAMETER;
      break;
    }
  }

  return aStatus == HYP_OK;
}

namespace
{
  typedef int TGeomID;

  //=============================================================================
  // Definitions of internal utils
  // --------------------------------------------------------------------------
  enum Transition {
    Trans_TANGENT = IntCurveSurface_Tangent,
    Trans_IN      = IntCurveSurface_In,
    Trans_OUT     = IntCurveSurface_Out,
    Trans_APEX
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Common data of any intersection between a Grid and a shape
   */
  struct B_IntersectPoint
  {
    mutable const SMDS_MeshNode* _node;
    mutable vector< TGeomID >    _faceIDs;

    B_IntersectPoint(): _node(NULL) {}
    void Add( const vector< TGeomID >& fIDs, const SMDS_MeshNode* n=0 ) const;
    int HasCommonFace( const B_IntersectPoint * other, int avoidFace=-1 ) const;
    bool IsOnFace( int faceID ) const;
    virtual ~B_IntersectPoint() {}
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Data of intersection between a GridLine and a TopoDS_Face
   */
  struct F_IntersectPoint : public B_IntersectPoint
  {
    double             _paramOnLine;
    mutable Transition _transition;
    mutable size_t     _indexOnLine;

    bool operator< ( const F_IntersectPoint& o ) const { return _paramOnLine < o._paramOnLine; }
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Data of intersection between GridPlanes and a TopoDS_EDGE
   */
  struct E_IntersectPoint : public B_IntersectPoint
  {
    gp_Pnt  _point;
    double  _uvw[3];
    TGeomID _shapeID;
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief A line of the grid and its intersections with 2D geometry
   */
  struct GridLine
  {
    gp_Lin _line;
    double _length; // line length
    multiset< F_IntersectPoint > _intPoints;

    void RemoveExcessIntPoints( const double tol );
    bool GetIsOutBefore( multiset< F_IntersectPoint >::iterator ip, bool prevIsOut );
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Planes of the grid used to find intersections of an EDGE with a hexahedron
   */
  struct GridPlanes
  {
    gp_XYZ           _zNorm;
    vector< gp_XYZ > _origins; // origin points of all planes in one direction
    vector< double > _zProjs;  // projections of origins to _zNorm
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Iterator on the parallel grid lines of one direction
   */
  struct LineIndexer
  {
    size_t _size  [3];
    size_t _curInd[3];
    size_t _iVar1, _iVar2, _iConst;
    string _name1, _name2, _nameConst;
    LineIndexer() {}
    LineIndexer( size_t sz1, size_t sz2, size_t sz3,
                 size_t iv1, size_t iv2, size_t iConst,
                 const string& nv1, const string& nv2, const string& nConst )
    {
      _size[0] = sz1; _size[1] = sz2; _size[2] = sz3;
      _curInd[0] = _curInd[1] = _curInd[2] = 0;
      _iVar1 = iv1; _iVar2 = iv2; _iConst = iConst; 
      _name1 = nv1; _name2 = nv2; _nameConst = nConst;
    }

    size_t I() const { return _curInd[0]; }
    size_t J() const { return _curInd[1]; }
    size_t K() const { return _curInd[2]; }
    void SetIJK( size_t i, size_t j, size_t k )
    {
      _curInd[0] = i; _curInd[1] = j; _curInd[2] = k;
    }
    void operator++()
    {
      if ( ++_curInd[_iVar1] == _size[_iVar1] )
        _curInd[_iVar1] = 0, ++_curInd[_iVar2];
    }
    bool More() const { return _curInd[_iVar2] < _size[_iVar2]; }
    size_t LineIndex   () const { return _curInd[_iVar1] + _curInd[_iVar2]* _size[_iVar1]; }
    size_t LineIndex10 () const { return (_curInd[_iVar1] + 1 ) + _curInd[_iVar2]* _size[_iVar1]; }
    size_t LineIndex01 () const { return _curInd[_iVar1] + (_curInd[_iVar2] + 1 )* _size[_iVar1]; }
    size_t LineIndex11 () const { return (_curInd[_iVar1] + 1 ) + (_curInd[_iVar2] + 1 )* _size[_iVar1]; }
    void SetIndexOnLine (size_t i)  { _curInd[ _iConst ] = i; }
    size_t NbLines() const { return _size[_iVar1] * _size[_iVar2]; }
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Container of GridLine's
   */
  struct Grid
  {
    vector< double >   _coords[3]; // coordinates of grid nodes
    gp_XYZ             _axes  [3]; // axis directions
    vector< GridLine > _lines [3]; //    in 3 directions
    double             _tol, _minCellSize;
    gp_XYZ             _origin;
    gp_Mat             _invB; // inverted basis of _axes

    vector< const SMDS_MeshNode* >    _nodes; // mesh nodes at grid nodes
    vector< const F_IntersectPoint* > _gridIntP; // grid node intersection with geometry

    list< E_IntersectPoint >          _edgeIntP; // intersections with EDGEs
    TopTools_IndexedMapOfShape        _shapes;

    SMESH_MesherHelper*               _helper;

    size_t CellIndex( size_t i, size_t j, size_t k ) const
    {
      return i + j*(_coords[0].size()-1) + k*(_coords[0].size()-1)*(_coords[1].size()-1);
    }
    size_t NodeIndex( size_t i, size_t j, size_t k ) const
    {
      return i + j*_coords[0].size() + k*_coords[0].size()*_coords[1].size();
    }
    size_t NodeIndexDX() const { return 1; }
    size_t NodeIndexDY() const { return _coords[0].size(); }
    size_t NodeIndexDZ() const { return _coords[0].size() * _coords[1].size(); }

    LineIndexer GetLineIndexer(size_t iDir) const;

    void SetCoordinates(const vector<double>& xCoords,
                        const vector<double>& yCoords,
                        const vector<double>& zCoords,
                        const double*         axesDirs,
                        const Bnd_Box&        bndBox );
    void ComputeUVW(const gp_XYZ& p, double uvw[3]);
    void ComputeNodes(SMESH_MesherHelper& helper);
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Intersector of TopoDS_Face with all GridLine's
   */
  struct FaceGridIntersector
  {
    TopoDS_Face _face;
    TGeomID     _faceID;
    Grid*       _grid;
    Bnd_Box     _bndBox;
    IntCurvesFace_Intersector* _surfaceInt;
    vector< std::pair< GridLine*, F_IntersectPoint > > _intersections;

    FaceGridIntersector(): _grid(0), _surfaceInt(0) {}
    void Intersect();

    void StoreIntersections()
    {
      for ( size_t i = 0; i < _intersections.size(); ++i )
      {
        multiset< F_IntersectPoint >::iterator ip = 
          _intersections[i].first->_intPoints.insert( _intersections[i].second );
        ip->_faceIDs.reserve( 1 );
        ip->_faceIDs.push_back( _faceID );
      }
    }
    const Bnd_Box& GetFaceBndBox()
    {
      GetCurveFaceIntersector();
      return _bndBox;
    }
    IntCurvesFace_Intersector* GetCurveFaceIntersector()
    {
      if ( !_surfaceInt )
      {
        _surfaceInt = new IntCurvesFace_Intersector( _face, Precision::PConfusion() );
        _bndBox     = _surfaceInt->Bounding();
        if ( _bndBox.IsVoid() )
          BRepBndLib::Add (_face, _bndBox);
      }
      return _surfaceInt;
    }
#ifdef WITH_TBB
    bool IsThreadSafe(set< const Standard_Transient* >& noSafeTShapes) const;
#endif
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Intersector of a surface with a GridLine
   */
  struct FaceLineIntersector
  {
    double      _tol;
    double      _u, _v, _w; // params on the face and the line
    Transition  _transition; // transition of at intersection (see IntCurveSurface.cdl)
    Transition  _transIn, _transOut; // IN and OUT transitions depending of face orientation

    gp_Pln      _plane;
    gp_Cylinder _cylinder;
    gp_Cone     _cone;
    gp_Sphere   _sphere;
    gp_Torus    _torus;
    IntCurvesFace_Intersector* _surfaceInt;

    vector< F_IntersectPoint > _intPoints;

    void IntersectWithPlane   (const GridLine& gridLine);
    void IntersectWithCylinder(const GridLine& gridLine);
    void IntersectWithCone    (const GridLine& gridLine);
    void IntersectWithSphere  (const GridLine& gridLine);
    void IntersectWithTorus   (const GridLine& gridLine);
    void IntersectWithSurface (const GridLine& gridLine);

    bool UVIsOnFace() const;
    void addIntPoint(const bool toClassify=true);
    bool isParamOnLineOK( const double linLength )
    {
      return -_tol < _w && _w < linLength + _tol;
    }
    FaceLineIntersector():_surfaceInt(0) {}
    ~FaceLineIntersector() { if (_surfaceInt ) delete _surfaceInt; _surfaceInt = 0; }
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Class representing topology of the hexahedron and creating a mesh
   *        volume basing on analysis of hexahedron intersection with geometry
   */
  class Hexahedron
  {
    // --------------------------------------------------------------------------------
    struct _Face;
    struct _Link;
    // --------------------------------------------------------------------------------
    struct _Node //!< node either at a hexahedron corner or at intersection
    {
      const SMDS_MeshNode*    _node; // mesh node at hexahedron corner
      const B_IntersectPoint* _intPoint;
      const _Face*            _usedInFace;

      _Node(const SMDS_MeshNode* n=0, const B_IntersectPoint* ip=0)
        :_node(n), _intPoint(ip), _usedInFace(0) {} 
      const SMDS_MeshNode*    Node() const
      { return ( _intPoint && _intPoint->_node ) ? _intPoint->_node : _node; }
      const E_IntersectPoint* EdgeIntPnt() const
      { return static_cast< const E_IntersectPoint* >( _intPoint ); }
      bool IsUsedInFace( const _Face* polygon = 0 )
      {
        return polygon ? ( _usedInFace == polygon ) : bool( _usedInFace );
      }
      void Add( const E_IntersectPoint* ip )
      {
        if ( !_intPoint ) {
          _intPoint = ip;
        }
        else if ( !_intPoint->_node ) {
          ip->Add( _intPoint->_faceIDs );
          _intPoint = ip;
        }
        else  {
          _intPoint->Add( ip->_faceIDs );
        }
      }
      TGeomID IsLinked( const B_IntersectPoint* other,
                        TGeomID                 avoidFace=-1 ) const // returns id of a common face
      {
        return _intPoint ? _intPoint->HasCommonFace( other, avoidFace ) : 0;
      }
      bool IsOnFace( TGeomID faceID ) const // returns true if faceID is found
      {
        return _intPoint ? _intPoint->IsOnFace( faceID ) : false;
      }
      gp_Pnt Point() const
      {
        if ( const SMDS_MeshNode* n = Node() )
          return SMESH_TNodeXYZ( n );
        if ( const E_IntersectPoint* eip =
             dynamic_cast< const E_IntersectPoint* >( _intPoint ))
          return eip->_point;
        return gp_Pnt( 1e100, 0, 0 );
      }
      TGeomID ShapeID() const
      {
        if ( const E_IntersectPoint* eip = dynamic_cast< const E_IntersectPoint* >( _intPoint ))
          return eip->_shapeID;
        return 0;
      }
    };
    // --------------------------------------------------------------------------------
    struct _Link // link connecting two _Node's
    {
      _Node* _nodes[2];
      _Face* _faces[2]; // polygons sharing a link
      vector< const F_IntersectPoint* > _fIntPoints; // GridLine intersections with FACEs
      vector< _Node* >                  _fIntNodes;   // _Node's at _fIntPoints
      vector< _Link >                   _splits;
      _Link() { _faces[0] = 0; }
    };
    // --------------------------------------------------------------------------------
    struct _OrientedLink
    {
      _Link* _link;
      bool   _reverse;
      _OrientedLink( _Link* link=0, bool reverse=false ): _link(link), _reverse(reverse) {}
      void Reverse() { _reverse = !_reverse; }
      int NbResultLinks() const { return _link->_splits.size(); }
      _OrientedLink ResultLink(int i) const
      {
        return _OrientedLink(&_link->_splits[_reverse ? NbResultLinks()-i-1 : i],_reverse);
      }
      _Node* FirstNode() const { return _link->_nodes[ _reverse ]; }
      _Node* LastNode()  const { return _link->_nodes[ !_reverse ]; }
      operator bool() const { return _link; }
      vector< TGeomID > GetNotUsedFace(const set<TGeomID>& usedIDs ) const // returns supporting FACEs
      {
        vector< TGeomID > faces;
        const B_IntersectPoint *ip0, *ip1;
        if (( ip0 = _link->_nodes[0]->_intPoint ) &&
            ( ip1 = _link->_nodes[1]->_intPoint ))
        {
          for ( size_t i = 0; i < ip0->_faceIDs.size(); ++i )
            if ( ip1->IsOnFace ( ip0->_faceIDs[i] ) &&
                 !usedIDs.count( ip0->_faceIDs[i] ) )
              faces.push_back( ip0->_faceIDs[i] );
        }
        return faces;
      }
      bool HasEdgeNodes() const
      {
        return ( dynamic_cast< const E_IntersectPoint* >( _link->_nodes[0]->_intPoint ) ||
                 dynamic_cast< const E_IntersectPoint* >( _link->_nodes[1]->_intPoint ));
      }
      int NbFaces() const
      {
        return !_link->_faces[0] ? 0 : 1 + bool( _link->_faces[1] );
      }
      void AddFace( _Face* f )
      {
        if ( _link->_faces[0] )
        {
          _link->_faces[1] = f;
        }
        else
        {
          _link->_faces[0] = f;
          _link->_faces[1] = 0;
        }
      }
      void RemoveFace( _Face* f )
      {
        if ( !_link->_faces[0] ) return;

        if ( _link->_faces[1] == f )
        {
          _link->_faces[1] = 0;
        }
        else if ( _link->_faces[0] == f )
        {
          _link->_faces[0] = 0;
          if ( _link->_faces[1] )
          {
            _link->_faces[0] = _link->_faces[1];
            _link->_faces[1] = 0;
          }
        }
      }
    };
    // --------------------------------------------------------------------------------
    struct _Face
    {
      vector< _OrientedLink > _links;       // links on GridLine's
      vector< _Link >         _polyLinks;   // links added to close a polygonal face
      vector< _Node* >        _eIntNodes;   // nodes at intersection with EDGEs
      bool IsPolyLink( const _OrientedLink& ol )
      {
        return _polyLinks.empty() ? false :
          ( &_polyLinks[0] <= ol._link &&  ol._link <= &_polyLinks.back() );
      }
      void AddPolyLink(_Node* n0, _Node* n1, _Face* faceToFindEqual=0)
      {
        if ( faceToFindEqual && faceToFindEqual != this ) {
          for ( size_t iL = 0; iL < faceToFindEqual->_polyLinks.size(); ++iL )
            if ( faceToFindEqual->_polyLinks[iL]._nodes[0] == n1 &&
                 faceToFindEqual->_polyLinks[iL]._nodes[1] == n0 )
            {
              _links.push_back
                ( _OrientedLink( & faceToFindEqual->_polyLinks[iL], /*reverse=*/true ));
              return;
            }
        }
        _Link l;
        l._nodes[0] = n0;
        l._nodes[1] = n1;
        _polyLinks.push_back( l );
        _links.push_back( _OrientedLink( &_polyLinks.back() ));
      }
    };
    // --------------------------------------------------------------------------------
    struct _volumeDef // holder of nodes of a volume mesh element
    {
      vector< _Node* > _nodes;
      vector< int >    _quantities;
      typedef boost::shared_ptr<_volumeDef> Ptr;
      void set( const vector< _Node* >& nodes,
                const vector< int >&    quant = vector< int >() )
      { _nodes = nodes; _quantities = quant; }
      void set( _Node** nodes, int nb )
      { _nodes.assign( nodes, nodes + nb ); }
    };

    // topology of a hexahedron
    int   _nodeShift[8];
    _Node _hexNodes [8];
    _Link _hexLinks [12];
    _Face _hexQuads [6];

    // faces resulted from hexahedron intersection
    vector< _Face > _polygons;

    // intresections with EDGEs
    vector< const E_IntersectPoint* > _eIntPoints;

    // additional nodes created at intersection points
    vector< _Node > _intNodes;

    // nodes inside the hexahedron (at VERTEXes)
    vector< _Node* > _vIntNodes;

    // computed volume elements
    //vector< _volumeDef::Ptr > _volumeDefs;
    _volumeDef _volumeDefs;

    Grid*       _grid;
    double      _sizeThreshold, _sideLength[3];
    int         _nbCornerNodes, _nbFaceIntNodes, _nbBndNodes;
    int         _origNodeInd; // index of _hexNodes[0] node within the _grid
    size_t      _i,_j,_k;

  public:
    Hexahedron(const double sizeThreshold, Grid* grid);
    int MakeElements(SMESH_MesherHelper&                      helper,
                     const map< TGeomID, vector< TGeomID > >& edge2faceIDsMap);
    void ComputeElements();
    void Init() { init( _i, _j, _k ); }

  private:
    Hexahedron(const Hexahedron& other );
    void init( size_t i, size_t j, size_t k );
    void init( size_t i );
    void addEdges(SMESH_MesherHelper&                      helper,
                  vector< Hexahedron* >&                   intersectedHex,
                  const map< TGeomID, vector< TGeomID > >& edge2faceIDsMap);
    gp_Pnt findIntPoint( double u1, double proj1, double u2, double proj2,
                         double proj, BRepAdaptor_Curve& curve,
                         const gp_XYZ& axis, const gp_XYZ& origin );
    int  getEntity( const E_IntersectPoint* ip, int* facets, int& sub );
    bool addIntersection( const E_IntersectPoint& ip,
                          vector< Hexahedron* >&  hexes,
                          int ijk[], int dIJK[] );
    bool findChain( _Node* n1, _Node* n2, _Face& quad, vector<_Node*>& chainNodes );
    bool closePolygon( _Face* polygon, vector<_Node*>& chainNodes ) const;
    bool findChainOnEdge( const vector< _OrientedLink >& splits,
                          const _OrientedLink&           prevSplit,
                          const _OrientedLink&           avoidSplit,
                          size_t &                       iS,
                          _Face&                         quad,
                          vector<_Node*>&                chn);
    int  addElements(SMESH_MesherHelper& helper);
    bool isOutPoint( _Link& link, int iP, SMESH_MesherHelper& helper ) const;
    void sortVertexNodes(vector<_Node*>& nodes, _Node* curNode, TGeomID face);
    bool isInHole() const;
    bool checkPolyhedronSize() const;
    bool addHexa ();
    bool addTetra();
    bool addPenta();
    bool addPyra ();
    bool debugDumpLink( _Link* link );
    _Node* findEqualNode( vector< _Node* >&       nodes,
                          const E_IntersectPoint* ip,
                          const double            tol2 )
    {
      for ( size_t i = 0; i < nodes.size(); ++i )
        if ( nodes[i]->EdgeIntPnt() == ip ||
             nodes[i]->Point().SquareDistance( ip->_point ) <= tol2 )
          return nodes[i];
      return 0;
    }
    bool isImplementEdges() const { return !_grid->_edgeIntP.empty(); }
    bool isOutParam(const double uvw[3]) const;
  };

#ifdef WITH_TBB
  // --------------------------------------------------------------------------
  /*!
   * \brief Hexahedron computing volumes in one thread
   */
  struct ParallelHexahedron
  {
    vector< Hexahedron* >& _hexVec;
    ParallelHexahedron( vector< Hexahedron* >& hv ): _hexVec(hv) {}
    void operator() ( const tbb::blocked_range<size_t>& r ) const
    {
      for ( size_t i = r.begin(); i != r.end(); ++i )
        if ( Hexahedron* hex = _hexVec[ i ] )
          hex->ComputeElements();
    }
  };
  // --------------------------------------------------------------------------
  /*!
   * \brief Structure intersecting certain nb of faces with GridLine's in one thread
   */
  struct ParallelIntersector
  {
    vector< FaceGridIntersector >& _faceVec;
    ParallelIntersector( vector< FaceGridIntersector >& faceVec): _faceVec(faceVec){}
    void operator() ( const tbb::blocked_range<size_t>& r ) const
    {
      for ( size_t i = r.begin(); i != r.end(); ++i )
        _faceVec[i].Intersect();
    }
  };
#endif

  //=============================================================================
  // Implementation of internal utils
  //=============================================================================
  /*!
   * \brief adjust \a i to have \a val between values[i] and values[i+1]
   */
  inline void locateValue( int & i, double val, const vector<double>& values,
                           int& di, double tol )
  {
    //val += values[0]; // input \a val is measured from 0.
    if ( i > values.size()-2 )
      i = values.size()-2;
    else
      while ( i+2 < values.size() && val > values[ i+1 ])
        ++i;
    while ( i > 0 && val < values[ i ])
      --i;

    if ( i > 0 && val - values[ i ] < tol )
      di = -1;
    else if ( i+2 < values.size() && values[ i+1 ] - val < tol )
      di = 1;
    else
      di = 0;
  }
  //=============================================================================
  /*
   * Remove coincident intersection points
   */
  void GridLine::RemoveExcessIntPoints( const double tol )
  {
    if ( _intPoints.size() < 2 ) return;

    set< Transition > tranSet;
    multiset< F_IntersectPoint >::iterator ip1, ip2 = _intPoints.begin();
    while ( ip2 != _intPoints.end() )
    {
      tranSet.clear();
      ip1 = ip2++;
      while ( ip2 != _intPoints.end() && ip2->_paramOnLine - ip1->_paramOnLine <= tol )
      {
        tranSet.insert( ip1->_transition );
        tranSet.insert( ip2->_transition );
        ip2->Add( ip1->_faceIDs );
        _intPoints.erase( ip1 );
        ip1 = ip2++;
      }
      if ( tranSet.size() > 1 ) // points with different transition coincide
      {
        bool isIN  = tranSet.count( Trans_IN );
        bool isOUT = tranSet.count( Trans_OUT );
        if ( isIN && isOUT )
          (*ip1)._transition = Trans_TANGENT;
        else
          (*ip1)._transition = isIN ? Trans_IN : Trans_OUT;
      }
    }
  }
  //================================================================================
  /*
   * Return "is OUT" state for nodes before the given intersection point
   */
  bool GridLine::GetIsOutBefore( multiset< F_IntersectPoint >::iterator ip, bool prevIsOut )
  {
    if ( ip->_transition == Trans_IN )
      return true;
    if ( ip->_transition == Trans_OUT )
      return false;
    if ( ip->_transition == Trans_APEX )
    {
      // singularity point (apex of a cone)
      if ( _intPoints.size() == 1 || ip == _intPoints.begin() )
        return true;
      multiset< F_IntersectPoint >::iterator ipBef = ip, ipAft = ++ip;
      if ( ipAft == _intPoints.end() )
        return false;
      --ipBef;
      if ( ipBef->_transition != ipAft->_transition )
        return ( ipBef->_transition == Trans_OUT );
      return ( ipBef->_transition != Trans_OUT );
    }
    // _transition == Trans_TANGENT
    return !prevIsOut;
  }
  //================================================================================
  /*
   * Adds face IDs
   */
  void B_IntersectPoint::Add( const vector< TGeomID >& fIDs,
                              const SMDS_MeshNode*     n) const
  {
    if ( _faceIDs.empty() )
      _faceIDs = fIDs;
    else
      for ( size_t i = 0; i < fIDs.size(); ++i )
      {
        vector< TGeomID >::iterator it =
          std::find( _faceIDs.begin(), _faceIDs.end(), fIDs[i] );
        if ( it == _faceIDs.end() )
          _faceIDs.push_back( fIDs[i] );
      }
    if ( !_node )
      _node = n;
  }
  //================================================================================
  /*
   * Returns index of a common face if any, else zero
   */
  int B_IntersectPoint::HasCommonFace( const B_IntersectPoint * other, int avoidFace ) const
  {
    if ( other )
      for ( size_t i = 0; i < other->_faceIDs.size(); ++i )
        if ( avoidFace != other->_faceIDs[i] &&
             IsOnFace   ( other->_faceIDs[i] ))
          return other->_faceIDs[i];
    return 0;
  }
  //================================================================================
  /*
   * Returns \c true if \a faceID in in this->_faceIDs
   */
  bool B_IntersectPoint::IsOnFace( int faceID ) const // returns true if faceID is found
  {
    vector< TGeomID >::const_iterator it =
      std::find( _faceIDs.begin(), _faceIDs.end(), faceID );
    return ( it != _faceIDs.end() );
  }
  //================================================================================
  /*
   * Return an iterator on GridLine's in a given direction
   */
  LineIndexer Grid::GetLineIndexer(size_t iDir) const
  {
    const size_t indices[] = { 1,2,0, 0,2,1, 0,1,2 };
    const string s      [] = { "X", "Y", "Z" };
    LineIndexer li( _coords[0].size(),  _coords[1].size(),    _coords[2].size(),
                    indices[iDir*3],    indices[iDir*3+1],    indices[iDir*3+2],
                    s[indices[iDir*3]], s[indices[iDir*3+1]], s[indices[iDir*3+2]]);
    return li;
  }
  //=============================================================================
  /*
   * Creates GridLine's of the grid
   */
  void Grid::SetCoordinates(const vector<double>& xCoords,
                            const vector<double>& yCoords,
                            const vector<double>& zCoords,
                            const double*         axesDirs,
                            const Bnd_Box&        shapeBox)
  {
    _coords[0] = xCoords;
    _coords[1] = yCoords;
    _coords[2] = zCoords;

    _axes[0].SetCoord( axesDirs[0],
                       axesDirs[1],
                       axesDirs[2]);
    _axes[1].SetCoord( axesDirs[3],
                       axesDirs[4],
                       axesDirs[5]);
    _axes[2].SetCoord( axesDirs[6],
                       axesDirs[7],
                       axesDirs[8]);
    _axes[0].Normalize();
    _axes[1].Normalize();
    _axes[2].Normalize();

    _invB.SetCols( _axes[0], _axes[1], _axes[2] );
    _invB.Invert();

    // compute tolerance
    _minCellSize = Precision::Infinite();
    for ( int iDir = 0; iDir < 3; ++iDir ) // loop on 3 line directions
    {
      for ( size_t i = 1; i < _coords[ iDir ].size(); ++i )
      {
        double cellLen = _coords[ iDir ][ i ] - _coords[ iDir ][ i-1 ];
        if ( cellLen < _minCellSize )
          _minCellSize = cellLen;
      }
    }
    if ( _minCellSize < Precision::Confusion() )
      throw SMESH_ComputeError (COMPERR_ALGO_FAILED,
                                SMESH_Comment("Too small cell size: ") << _minCellSize );
    _tol = _minCellSize / 1000.;

    // attune grid extremities to shape bounding box

    double sP[6]; // aXmin, aYmin, aZmin, aXmax, aYmax, aZmax
    shapeBox.Get(sP[0],sP[1],sP[2],sP[3],sP[4],sP[5]);
    double* cP[6] = { &_coords[0].front(), &_coords[1].front(), &_coords[2].front(),
                      &_coords[0].back(),  &_coords[1].back(),  &_coords[2].back() };
    for ( int i = 0; i < 6; ++i )
      if ( fabs( sP[i] - *cP[i] ) < _tol )
        *cP[i] = sP[i];// + _tol/1000. * ( i < 3 ? +1 : -1 );

    for ( int iDir = 0; iDir < 3; ++iDir )
    {
      if ( _coords[iDir][0] - sP[iDir] > _tol )
      {
        _minCellSize = Min( _minCellSize, _coords[iDir][0] - sP[iDir] );
        _coords[iDir].insert( _coords[iDir].begin(), sP[iDir] + _tol/1000.);
      }
      if ( sP[iDir+3] - _coords[iDir].back() > _tol  )
      {
        _minCellSize = Min( _minCellSize, sP[iDir+3] - _coords[iDir].back() );
        _coords[iDir].push_back( sP[iDir+3] - _tol/1000.);
      }
    }
    _tol = _minCellSize / 1000.;

    _origin = ( _coords[0][0] * _axes[0] +
                _coords[1][0] * _axes[1] +
                _coords[2][0] * _axes[2] );

    // create lines
    for ( int iDir = 0; iDir < 3; ++iDir ) // loop on 3 line directions
    {
      LineIndexer li = GetLineIndexer( iDir );
      _lines[iDir].resize( li.NbLines() );
      double len = _coords[ iDir ].back() - _coords[iDir].front();
      for ( ; li.More(); ++li )
      {
        GridLine& gl = _lines[iDir][ li.LineIndex() ];
        gl._line.SetLocation( _coords[0][li.I()] * _axes[0] +
                              _coords[1][li.J()] * _axes[1] +
                              _coords[2][li.K()] * _axes[2] );
        gl._line.SetDirection( _axes[ iDir ]);
        gl._length = len;
      }
    }
  }
  //================================================================================
  /*
   * Computes coordinates of a point in the grid CS
   */
  void Grid::ComputeUVW(const gp_XYZ& P, double UVW[3])
  {
    gp_XYZ p = P * _invB;
    p.Coord( UVW[0], UVW[1], UVW[2] );
  }
  //================================================================================
  /*
   * Creates all nodes
   */
  void Grid::ComputeNodes(SMESH_MesherHelper& helper)
  {
    // state of each node of the grid relative to the geometry
    const size_t nbGridNodes = _coords[0].size() * _coords[1].size() * _coords[2].size();
    vector< bool > isNodeOut( nbGridNodes, false );
    _nodes.resize( nbGridNodes, 0 );
    _gridIntP.resize( nbGridNodes, NULL );

    for ( int iDir = 0; iDir < 3; ++iDir ) // loop on 3 line directions
    {
      LineIndexer li = GetLineIndexer( iDir );

      // find out a shift of node index while walking along a GridLine in this direction
      li.SetIndexOnLine( 0 );
      size_t nIndex0 = NodeIndex( li.I(), li.J(), li.K() );
      li.SetIndexOnLine( 1 );
      const size_t nShift = NodeIndex( li.I(), li.J(), li.K() ) - nIndex0;
      
      const vector<double> & coords = _coords[ iDir ];
      for ( ; li.More(); ++li ) // loop on lines in iDir
      {
        li.SetIndexOnLine( 0 );
        nIndex0 = NodeIndex( li.I(), li.J(), li.K() );

        GridLine& line = _lines[ iDir ][ li.LineIndex() ];
        const gp_XYZ lineLoc = line._line.Location().XYZ();
        const gp_XYZ lineDir = line._line.Direction().XYZ();
        line.RemoveExcessIntPoints( _tol );
        multiset< F_IntersectPoint >& intPnts = line._intPoints;
        multiset< F_IntersectPoint >::iterator ip = intPnts.begin();

        bool isOut = true;
        const double* nodeCoord = & coords[0];
        const double* coord0    = nodeCoord;
        const double* coordEnd  = coord0 + coords.size();
        double nodeParam = 0;
        for ( ; ip != intPnts.end(); ++ip )
        {
          // set OUT state or just skip IN nodes before ip
          if ( nodeParam < ip->_paramOnLine - _tol )
          {
            isOut = line.GetIsOutBefore( ip, isOut );

            while ( nodeParam < ip->_paramOnLine - _tol )
            {
              if ( isOut )
                isNodeOut[ nIndex0 + nShift * ( nodeCoord-coord0 ) ] = isOut;
              if ( ++nodeCoord <  coordEnd )
                nodeParam = *nodeCoord - *coord0;
              else
                break;
            }
            if ( nodeCoord == coordEnd ) break;
          }
          // create a mesh node on a GridLine at ip if it does not coincide with a grid node
          if ( nodeParam > ip->_paramOnLine + _tol )
          {
            // li.SetIndexOnLine( 0 );
            // double xyz[3] = { _coords[0][ li.I() ], _coords[1][ li.J() ], _coords[2][ li.K() ]};
            // xyz[ li._iConst ] += ip->_paramOnLine;
            gp_XYZ xyz = lineLoc + ip->_paramOnLine * lineDir;
            ip->_node = helper.AddNode( xyz.X(), xyz.Y(), xyz.Z() );
            ip->_indexOnLine = nodeCoord-coord0-1;
          }
          // create a mesh node at ip concident with a grid node
          else
          {
            int nodeIndex = nIndex0 + nShift * ( nodeCoord-coord0 );
            if ( !_nodes[ nodeIndex ] )
            {
              //li.SetIndexOnLine( nodeCoord-coord0 );
              //double xyz[3] = { _coords[0][ li.I() ], _coords[1][ li.J() ], _coords[2][ li.K() ]};
              gp_XYZ xyz = lineLoc + nodeParam * lineDir;
              _nodes   [ nodeIndex ] = helper.AddNode( xyz.X(), xyz.Y(), xyz.Z() );
              _gridIntP[ nodeIndex ] = & * ip;
            }
            if ( _gridIntP[ nodeIndex ] )
              _gridIntP[ nodeIndex ]->Add( ip->_faceIDs );
            else
              _gridIntP[ nodeIndex ] = & * ip;
            // ip->_node        = _nodes[ nodeIndex ]; -- to differ from ip on links
            ip->_indexOnLine = nodeCoord-coord0;
            if ( ++nodeCoord < coordEnd )
              nodeParam = *nodeCoord - *coord0;
          }
        }
        // set OUT state to nodes after the last ip
        for ( ; nodeCoord < coordEnd; ++nodeCoord )
          isNodeOut[ nIndex0 + nShift * ( nodeCoord-coord0 ) ] = true;
      }
    }

    // Create mesh nodes at !OUT nodes of the grid

    for ( size_t z = 0; z < _coords[2].size(); ++z )
      for ( size_t y = 0; y < _coords[1].size(); ++y )
        for ( size_t x = 0; x < _coords[0].size(); ++x )
        {
          size_t nodeIndex = NodeIndex( x, y, z );
          if ( !isNodeOut[ nodeIndex ] && !_nodes[ nodeIndex] )
          {
            //_nodes[ nodeIndex ] = helper.AddNode( _coords[0][x], _coords[1][y], _coords[2][z] );
            gp_XYZ xyz = ( _coords[0][x] * _axes[0] +
                           _coords[1][y] * _axes[1] +
                           _coords[2][z] * _axes[2] );
            _nodes[ nodeIndex ] = helper.AddNode( xyz.X(), xyz.Y(), xyz.Z() );
          }
        }

#ifdef _MY_DEBUG_
    // check validity of transitions
    const char* trName[] = { "TANGENT", "IN", "OUT", "APEX" };
    for ( int iDir = 0; iDir < 3; ++iDir ) // loop on 3 line directions
    {
      LineIndexer li = GetLineIndexer( iDir );
      for ( ; li.More(); ++li )
      {
        multiset< F_IntersectPoint >& intPnts = _lines[ iDir ][ li.LineIndex() ]._intPoints;
        if ( intPnts.empty() ) continue;
        if ( intPnts.size() == 1 )
        {
          if ( intPnts.begin()->_transition != Trans_TANGENT &&
               intPnts.begin()->_transition != Trans_APEX )
          throw SMESH_ComputeError (COMPERR_ALGO_FAILED,
                                    SMESH_Comment("Wrong SOLE transition of GridLine (")
                                    << li._curInd[li._iVar1] << ", " << li._curInd[li._iVar2]
                                    << ") along " << li._nameConst
                                    << ": " << trName[ intPnts.begin()->_transition] );
        }
        else
        {
          if ( intPnts.begin()->_transition == Trans_OUT )
            throw SMESH_ComputeError (COMPERR_ALGO_FAILED,
                                      SMESH_Comment("Wrong START transition of GridLine (")
                                      << li._curInd[li._iVar1] << ", " << li._curInd[li._iVar2]
                                      << ") along " << li._nameConst
                                      << ": " << trName[ intPnts.begin()->_transition ]);
          if ( intPnts.rbegin()->_transition == Trans_IN )
            throw SMESH_ComputeError (COMPERR_ALGO_FAILED,
                                      SMESH_Comment("Wrong END transition of GridLine (")
                                      << li._curInd[li._iVar1] << ", " << li._curInd[li._iVar2]
                                      << ") along " << li._nameConst
                                    << ": " << trName[ intPnts.rbegin()->_transition ]);
        }
      }
    }
#endif
  }

  //=============================================================================
  /*
   * Intersects TopoDS_Face with all GridLine's
   */
  void FaceGridIntersector::Intersect()
  {
    FaceLineIntersector intersector;
    intersector._surfaceInt = GetCurveFaceIntersector();
    intersector._tol        = _grid->_tol;
    intersector._transOut   = _face.Orientation() == TopAbs_REVERSED ? Trans_IN : Trans_OUT;
    intersector._transIn    = _face.Orientation() == TopAbs_REVERSED ? Trans_OUT : Trans_IN;

    typedef void (FaceLineIntersector::* PIntFun )(const GridLine& gridLine);
    PIntFun interFunction;

    bool isDirect = true;
    BRepAdaptor_Surface surf( _face );
    switch ( surf.GetType() ) {
    case GeomAbs_Plane:
      intersector._plane = surf.Plane();
      interFunction = &FaceLineIntersector::IntersectWithPlane;
      isDirect = intersector._plane.Direct();
      break;
    case GeomAbs_Cylinder:
      intersector._cylinder = surf.Cylinder();
      interFunction = &FaceLineIntersector::IntersectWithCylinder;
      isDirect = intersector._cylinder.Direct();
      break;
    case GeomAbs_Cone:
      intersector._cone = surf.Cone();
      interFunction = &FaceLineIntersector::IntersectWithCone;
      //isDirect = intersector._cone.Direct();
      break;
    case GeomAbs_Sphere:
      intersector._sphere = surf.Sphere();
      interFunction = &FaceLineIntersector::IntersectWithSphere;
      isDirect = intersector._sphere.Direct();
      break;
    case GeomAbs_Torus:
      intersector._torus = surf.Torus();
      interFunction = &FaceLineIntersector::IntersectWithTorus;
      //isDirect = intersector._torus.Direct();
      break;
    default:
      interFunction = &FaceLineIntersector::IntersectWithSurface;
    }
    if ( !isDirect )
      std::swap( intersector._transOut, intersector._transIn );

    _intersections.clear();
    for ( int iDir = 0; iDir < 3; ++iDir ) // loop on 3 line directions
    {
      if ( surf.GetType() == GeomAbs_Plane )
      {
        // check if all lines in this direction are parallel to a plane
        if ( intersector._plane.Axis().IsNormal( _grid->_lines[iDir][0]._line.Position(),
                                                 Precision::Angular()))
          continue;
        // find out a transition, that is the same for all lines of a direction
        gp_Dir plnNorm = intersector._plane.Axis().Direction();
        gp_Dir lineDir = _grid->_lines[iDir][0]._line.Direction();
        intersector._transition =
          ( plnNorm * lineDir < 0 ) ? intersector._transIn : intersector._transOut;
      }
      if ( surf.GetType() == GeomAbs_Cylinder )
      {
        // check if all lines in this direction are parallel to a cylinder
        if ( intersector._cylinder.Axis().IsParallel( _grid->_lines[iDir][0]._line.Position(),
                                                      Precision::Angular()))
          continue;
      }

      // intersect the grid lines with the face
      for ( size_t iL = 0; iL < _grid->_lines[iDir].size(); ++iL )
      {
        GridLine& gridLine = _grid->_lines[iDir][iL];
        if ( _bndBox.IsOut( gridLine._line )) continue;

        intersector._intPoints.clear();
        (intersector.*interFunction)( gridLine ); // <- intersection with gridLine
        for ( size_t i = 0; i < intersector._intPoints.size(); ++i )
          _intersections.push_back( make_pair( &gridLine, intersector._intPoints[i] ));
      }
    }
  }
  //================================================================================
  /*
   * Return true if (_u,_v) is on the face
   */
  bool FaceLineIntersector::UVIsOnFace() const
  {
    TopAbs_State state = _surfaceInt->ClassifyUVPoint(gp_Pnt2d( _u,_v ));
    return ( state == TopAbs_IN || state == TopAbs_ON );
  }
  //================================================================================
  /*
   * Store an intersection if it is IN or ON the face
   */
  void FaceLineIntersector::addIntPoint(const bool toClassify)
  {
    if ( !toClassify || UVIsOnFace() )
    {
      F_IntersectPoint p;
      p._paramOnLine = _w;
      p._transition  = _transition;
      _intPoints.push_back( p );
    }
  }
  //================================================================================
  /*
   * Intersect a line with a plane
   */
  void FaceLineIntersector::IntersectWithPlane(const GridLine& gridLine)
  {
    IntAna_IntConicQuad linPlane( gridLine._line, _plane, Precision::Angular());
    _w = linPlane.ParamOnConic(1);
    if ( isParamOnLineOK( gridLine._length ))
    {
      ElSLib::Parameters(_plane, linPlane.Point(1) ,_u,_v);
      addIntPoint();
    }
  }
  //================================================================================
  /*
   * Intersect a line with a cylinder
   */
  void FaceLineIntersector::IntersectWithCylinder(const GridLine& gridLine)
  {
    IntAna_IntConicQuad linCylinder( gridLine._line, _cylinder );
    if ( linCylinder.IsDone() && linCylinder.NbPoints() > 0 )
    {
      _w = linCylinder.ParamOnConic(1);
      if ( linCylinder.NbPoints() == 1 )
        _transition = Trans_TANGENT;
      else
        _transition = _w < linCylinder.ParamOnConic(2) ? _transIn : _transOut;
      if ( isParamOnLineOK( gridLine._length ))
      {
        ElSLib::Parameters(_cylinder, linCylinder.Point(1) ,_u,_v);
        addIntPoint();
      }
      if ( linCylinder.NbPoints() > 1 )
      {
        _w = linCylinder.ParamOnConic(2);
        if ( isParamOnLineOK( gridLine._length ))
        {
          ElSLib::Parameters(_cylinder, linCylinder.Point(2) ,_u,_v);
          _transition = ( _transition == Trans_OUT ) ? Trans_IN : Trans_OUT;
          addIntPoint();
        }
      }
    }
  }
  //================================================================================
  /*
   * Intersect a line with a cone
   */
  void FaceLineIntersector::IntersectWithCone (const GridLine& gridLine)
  {
    IntAna_IntConicQuad linCone(gridLine._line,_cone);
    if ( !linCone.IsDone() ) return;
    gp_Pnt P;
    gp_Vec du, dv, norm;
    for ( int i = 1; i <= linCone.NbPoints(); ++i )
    {
      _w = linCone.ParamOnConic( i );
      if ( !isParamOnLineOK( gridLine._length )) continue;
      ElSLib::Parameters(_cone, linCone.Point(i) ,_u,_v);
      if ( UVIsOnFace() )
      {
        ElSLib::D1( _u, _v, _cone, P, du, dv );
        norm = du ^ dv;
        double normSize2 = norm.SquareMagnitude();
        if ( normSize2 > Precision::Angular() * Precision::Angular() )
        {
          double cos = norm.XYZ() * gridLine._line.Direction().XYZ();
          cos /= sqrt( normSize2 );
          if ( cos < -Precision::Angular() )
            _transition = _transIn;
          else if ( cos > Precision::Angular() )
            _transition = _transOut;
          else
            _transition = Trans_TANGENT;
        }
        else
        {
          _transition = Trans_APEX;
        }
        addIntPoint( /*toClassify=*/false);
      }
    }
  }
  //================================================================================
  /*
   * Intersect a line with a sphere
   */
  void FaceLineIntersector::IntersectWithSphere  (const GridLine& gridLine)
  {
    IntAna_IntConicQuad linSphere(gridLine._line,_sphere);
    if ( linSphere.IsDone() && linSphere.NbPoints() > 0 )
    {
      _w = linSphere.ParamOnConic(1);
      if ( linSphere.NbPoints() == 1 )
        _transition = Trans_TANGENT;
      else
        _transition = _w < linSphere.ParamOnConic(2) ? _transIn : _transOut;
      if ( isParamOnLineOK( gridLine._length ))
      {
        ElSLib::Parameters(_sphere, linSphere.Point(1) ,_u,_v);
        addIntPoint();
      }
      if ( linSphere.NbPoints() > 1 )
      {
        _w = linSphere.ParamOnConic(2);
        if ( isParamOnLineOK( gridLine._length ))
        {
          ElSLib::Parameters(_sphere, linSphere.Point(2) ,_u,_v);
          _transition = ( _transition == Trans_OUT ) ? Trans_IN : Trans_OUT;
          addIntPoint();
        }
      }
    }
  }
  //================================================================================
  /*
   * Intersect a line with a torus
   */
  void FaceLineIntersector::IntersectWithTorus   (const GridLine& gridLine)
  {
    IntAna_IntLinTorus linTorus(gridLine._line,_torus);
    if ( !linTorus.IsDone()) return;
    gp_Pnt P;
    gp_Vec du, dv, norm;
    for ( int i = 1; i <= linTorus.NbPoints(); ++i )
    {
      _w = linTorus.ParamOnLine( i );
      if ( !isParamOnLineOK( gridLine._length )) continue;
      linTorus.ParamOnTorus( i, _u,_v );
      if ( UVIsOnFace() )
      {
        ElSLib::D1( _u, _v, _torus, P, du, dv );
        norm = du ^ dv;
        double normSize = norm.Magnitude();
        double cos = norm.XYZ() * gridLine._line.Direction().XYZ();
        cos /= normSize;
        if ( cos < -Precision::Angular() )
          _transition = _transIn;
        else if ( cos > Precision::Angular() )
          _transition = _transOut;
        else
          _transition = Trans_TANGENT;
        addIntPoint( /*toClassify=*/false);
      }
    }
  }
  //================================================================================
  /*
   * Intersect a line with a non-analytical surface
   */
  void FaceLineIntersector::IntersectWithSurface (const GridLine& gridLine)
  {
    _surfaceInt->Perform( gridLine._line, 0.0, gridLine._length );
    if ( !_surfaceInt->IsDone() ) return;
    for ( int i = 1; i <= _surfaceInt->NbPnt(); ++i )
    {
      _transition = Transition( _surfaceInt->Transition( i ) );
      _w = _surfaceInt->WParameter( i );
      addIntPoint(/*toClassify=*/false);
    }
  }
#ifdef WITH_TBB
  //================================================================================
  /*
   * check if its face can be safely intersected in a thread
   */
  bool FaceGridIntersector::IsThreadSafe(set< const Standard_Transient* >& noSafeTShapes) const
  {
    bool isSafe = true;

    // check surface
    TopLoc_Location loc;
    Handle(Geom_Surface) surf = BRep_Tool::Surface( _face, loc );
    Handle(Geom_RectangularTrimmedSurface) ts =
      Handle(Geom_RectangularTrimmedSurface)::DownCast( surf );
    while( !ts.IsNull() ) {
      surf = ts->BasisSurface();
      ts = Handle(Geom_RectangularTrimmedSurface)::DownCast(surf);
    }
    if ( surf->IsKind( STANDARD_TYPE(Geom_BSplineSurface )) ||
         surf->IsKind( STANDARD_TYPE(Geom_BezierSurface )))
      // if ( !noSafeTShapes.insert((const Standard_Transient*) _face.TShape() ).second )
        isSafe = false;

    double f, l;
    TopExp_Explorer exp( _face, TopAbs_EDGE );
    for ( ; exp.More(); exp.Next() )
    {
      bool edgeIsSafe = true;
      const TopoDS_Edge& e = TopoDS::Edge( exp.Current() );
      // check 3d curve
      {
        Handle(Geom_Curve) c = BRep_Tool::Curve( e, loc, f, l);
        if ( !c.IsNull() )
        {
          Handle(Geom_TrimmedCurve) tc = Handle(Geom_TrimmedCurve)::DownCast(c);
          while( !tc.IsNull() ) {
            c = tc->BasisCurve();
            tc = Handle(Geom_TrimmedCurve)::DownCast(c);
          }
          if ( c->IsKind( STANDARD_TYPE(Geom_BSplineCurve )) ||
               c->IsKind( STANDARD_TYPE(Geom_BezierCurve )))
            edgeIsSafe = false;
        }
      }
      // check 2d curve
      if ( edgeIsSafe )
      {
        Handle(Geom2d_Curve) c2 = BRep_Tool::CurveOnSurface( e, surf, loc, f, l);
        if ( !c2.IsNull() )
        {
          Handle(Geom2d_TrimmedCurve) tc = Handle(Geom2d_TrimmedCurve)::DownCast(c2);
          while( !tc.IsNull() ) {
            c2 = tc->BasisCurve();
            tc = Handle(Geom2d_TrimmedCurve)::DownCast(c2);
          }
          if ( c2->IsKind( STANDARD_TYPE(Geom2d_BSplineCurve )) ||
               c2->IsKind( STANDARD_TYPE(Geom2d_BezierCurve )))
            edgeIsSafe = false;
        }
      }
      if ( !edgeIsSafe ) // && !noSafeTShapes.insert((const Standard_Transient*) e.TShape() ).second )
        isSafe = false;
    }
    return isSafe;
  }
#endif
  //================================================================================
  /*!
   * \brief Creates topology of the hexahedron
   */
  Hexahedron::Hexahedron(const double sizeThreshold, Grid* grid)
    : _grid( grid ), _sizeThreshold( sizeThreshold ), _nbFaceIntNodes(0)
  {
    _polygons.reserve(100); // to avoid reallocation;

    //set nodes shift within grid->_nodes from the node 000 
    size_t dx = _grid->NodeIndexDX();
    size_t dy = _grid->NodeIndexDY();
    size_t dz = _grid->NodeIndexDZ();
    size_t i000 = 0;
    size_t i100 = i000 + dx;
    size_t i010 = i000 + dy;
    size_t i110 = i010 + dx;
    size_t i001 = i000 + dz;
    size_t i101 = i100 + dz;
    size_t i011 = i010 + dz;
    size_t i111 = i110 + dz;
    _nodeShift[ SMESH_Block::ShapeIndex( SMESH_Block::ID_V000 )] = i000;
    _nodeShift[ SMESH_Block::ShapeIndex( SMESH_Block::ID_V100 )] = i100;
    _nodeShift[ SMESH_Block::ShapeIndex( SMESH_Block::ID_V010 )] = i010;
    _nodeShift[ SMESH_Block::ShapeIndex( SMESH_Block::ID_V110 )] = i110;
    _nodeShift[ SMESH_Block::ShapeIndex( SMESH_Block::ID_V001 )] = i001;
    _nodeShift[ SMESH_Block::ShapeIndex( SMESH_Block::ID_V101 )] = i101;
    _nodeShift[ SMESH_Block::ShapeIndex( SMESH_Block::ID_V011 )] = i011;
    _nodeShift[ SMESH_Block::ShapeIndex( SMESH_Block::ID_V111 )] = i111;

    vector< int > idVec;
    // set nodes to links
    for ( int linkID = SMESH_Block::ID_Ex00; linkID <= SMESH_Block::ID_E11z; ++linkID )
    {
      SMESH_Block::GetEdgeVertexIDs( linkID, idVec );
      _Link& link = _hexLinks[ SMESH_Block::ShapeIndex( linkID )];
      link._nodes[0] = &_hexNodes[ SMESH_Block::ShapeIndex( idVec[0] )];
      link._nodes[1] = &_hexNodes[ SMESH_Block::ShapeIndex( idVec[1] )];
    }

    // set links to faces
    int interlace[4] = { 0, 3, 1, 2 }; // to walk by links around a face: { u0, 1v, u1, 0v }
    for ( int faceID = SMESH_Block::ID_Fxy0; faceID <= SMESH_Block::ID_F1yz; ++faceID )
    {
      SMESH_Block::GetFaceEdgesIDs( faceID, idVec );
      _Face& quad = _hexQuads[ SMESH_Block::ShapeIndex( faceID )];
      bool revFace = ( faceID == SMESH_Block::ID_Fxy0 ||
                       faceID == SMESH_Block::ID_Fx1z ||
                       faceID == SMESH_Block::ID_F0yz );
      quad._links.resize(4);
      vector<_OrientedLink>::iterator         frwLinkIt = quad._links.begin();
      vector<_OrientedLink>::reverse_iterator revLinkIt = quad._links.rbegin();
      for ( int i = 0; i < 4; ++i )
      {
        bool revLink = revFace;
        if ( i > 1 ) // reverse links u1 and v0
          revLink = !revLink;
        _OrientedLink& link = revFace ? *revLinkIt++ : *frwLinkIt++;
        link = _OrientedLink( & _hexLinks[ SMESH_Block::ShapeIndex( idVec[interlace[i]] )],
                              revLink );
      }
    }
  }
  //================================================================================
  /*!
   * \brief Copy constructor
   */
  Hexahedron::Hexahedron( const Hexahedron& other )
    :_grid( other._grid ), _sizeThreshold( other._sizeThreshold ), _nbFaceIntNodes(0)
  {
    _polygons.reserve(100); // to avoid reallocation;

    for ( int i = 0; i < 8; ++i )
      _nodeShift[i] = other._nodeShift[i];

    for ( int i = 0; i < 12; ++i )
    {
      const _Link& srcLink = other._hexLinks[ i ];
      _Link&       tgtLink = this->_hexLinks[ i ];
      tgtLink._nodes[0] = _hexNodes + ( srcLink._nodes[0] - other._hexNodes );
      tgtLink._nodes[1] = _hexNodes + ( srcLink._nodes[1] - other._hexNodes );
    }

    for ( int i = 0; i < 6; ++i )
    {
      const _Face& srcQuad = other._hexQuads[ i ];
      _Face&       tgtQuad = this->_hexQuads[ i ];
      tgtQuad._links.resize(4);
      for ( int j = 0; j < 4; ++j )
      {
        const _OrientedLink& srcLink = srcQuad._links[ j ];
        _OrientedLink&       tgtLink = tgtQuad._links[ j ];
        tgtLink._reverse = srcLink._reverse;
        tgtLink._link    = _hexLinks + ( srcLink._link - other._hexLinks );
      }
    }
  }

  //================================================================================
  /*!
   * \brief Initializes its data by given grid cell
   */
  void Hexahedron::init( size_t i, size_t j, size_t k )
  {
    _i = i; _j = j; _k = k;
    // set nodes of grid to nodes of the hexahedron and
    // count nodes at hexahedron corners located IN and ON geometry
    _nbCornerNodes = _nbBndNodes = 0;
    _origNodeInd   = _grid->NodeIndex( i,j,k );
    for ( int iN = 0; iN < 8; ++iN )
    {
      _hexNodes[iN]._node     = _grid->_nodes   [ _origNodeInd + _nodeShift[iN] ];
      _hexNodes[iN]._intPoint = _grid->_gridIntP[ _origNodeInd + _nodeShift[iN] ];
      _nbCornerNodes += bool( _hexNodes[iN]._node );
      _nbBndNodes    += bool( _hexNodes[iN]._intPoint );
    }
    _sideLength[0] = _grid->_coords[0][i+1] - _grid->_coords[0][i];
    _sideLength[1] = _grid->_coords[1][j+1] - _grid->_coords[1][j];
    _sideLength[2] = _grid->_coords[2][k+1] - _grid->_coords[2][k];

    _intNodes.clear();
    _vIntNodes.clear();

    if ( _nbFaceIntNodes + _eIntPoints.size() > 0 &&
         _nbFaceIntNodes + _nbCornerNodes + _eIntPoints.size() > 3)
    {
      _intNodes.reserve( 3 * _nbBndNodes + _nbFaceIntNodes + _eIntPoints.size() );

      // this method can be called in parallel, so use own helper
      SMESH_MesherHelper helper( *_grid->_helper->GetMesh() );

      // create sub-links (_splits) by splitting links with _fIntPoints
      _Link split;
      for ( int iLink = 0; iLink < 12; ++iLink )
      {
        _Link& link = _hexLinks[ iLink ];
        link._fIntNodes.resize( link._fIntPoints.size() );
        for ( size_t i = 0; i < link._fIntPoints.size(); ++i )
        {
          _intNodes.push_back( _Node( 0, link._fIntPoints[i] ));
          link._fIntNodes[ i ] = & _intNodes.back();
        }

        link._splits.clear();
        split._nodes[ 0 ] = link._nodes[0];
        bool isOut = ( ! link._nodes[0]->Node() );
        bool checkTransition;
        for ( size_t i = 0; i < link._fIntNodes.size(); ++i )
        {
          const bool isGridNode = ( ! link._fIntNodes[i]->Node() );
          if ( !isGridNode ) // intersection non-coincident with a grid node
          {
            if ( split._nodes[ 0 ]->Node() && !isOut )
            {
              split._nodes[ 1 ] = link._fIntNodes[i];
              link._splits.push_back( split );
            }
            split._nodes[ 0 ] = link._fIntNodes[i];
            checkTransition = true;
          }
          else // FACE intersection coincident with a grid node (at link ends)
          {
            checkTransition = ( i == 0 && link._nodes[0]->Node() );
          }
          if ( checkTransition )
          {
            if ( link._fIntPoints[i]->_faceIDs.size() > 1 || _eIntPoints.size() > 0 )
              isOut = isOutPoint( link, i, helper );
            else
              switch ( link._fIntPoints[i]->_transition ) {
              case Trans_OUT: isOut = true;  break;
              case Trans_IN : isOut = false; break;
              default:
                isOut = isOutPoint( link, i, helper );
              }
          }
        }
        if ( link._nodes[ 1 ]->Node() && split._nodes[ 0 ]->Node() && !isOut )
        {
          split._nodes[ 1 ] = link._nodes[1];
          link._splits.push_back( split );
        }
      }

      // Create _Node's at intersections with EDGEs.

      const double tol2 = _grid->_tol * _grid->_tol;
      int facets[3], nbFacets, subEntity;

      for ( size_t iP = 0; iP < _eIntPoints.size(); ++iP )
      {
        nbFacets = getEntity( _eIntPoints[iP], facets, subEntity );
        _Node* equalNode = 0;
        switch( nbFacets ) {
        case 1: // in a _Face
        {
          _Face& quad = _hexQuads[ facets[0] - SMESH_Block::ID_FirstF ];
          equalNode = findEqualNode( quad._eIntNodes, _eIntPoints[ iP ], tol2 );
          if ( equalNode ) {
            equalNode->Add( _eIntPoints[ iP ] );
          }
          else {
            _intNodes.push_back( _Node( 0, _eIntPoints[ iP ]));
            quad._eIntNodes.push_back( & _intNodes.back() );
          }
          break;
        }
        case 2: // on a _Link
        {
          _Link& link = _hexLinks[ subEntity - SMESH_Block::ID_FirstE ];
          if ( link._splits.size() > 0 )
          {
            equalNode = findEqualNode( link._fIntNodes, _eIntPoints[ iP ], tol2 );
            if ( equalNode )
              equalNode->Add( _eIntPoints[ iP ] );
          }
          else
          {
            _intNodes.push_back( _Node( 0, _eIntPoints[ iP ]));
            for ( int iF = 0; iF < 2; ++iF )
            {
              _Face& quad = _hexQuads[ facets[iF] - SMESH_Block::ID_FirstF ];
              equalNode = findEqualNode( quad._eIntNodes, _eIntPoints[ iP ], tol2 );
              if ( equalNode ) {
                equalNode->Add( _eIntPoints[ iP ] );
              }
              else {
                quad._eIntNodes.push_back( & _intNodes.back() );
              }
            }
          }
          break;
        }
        case 3: // at a corner
        {
          _Node& node = _hexNodes[ subEntity - SMESH_Block::ID_FirstV ];
          if ( node.Node() != 0 )
          {
            if ( node._intPoint )
              node._intPoint->Add( _eIntPoints[ iP ]->_faceIDs, _eIntPoints[ iP ]->_node );
          }
          else
          {
            _intNodes.push_back( _Node( 0, _eIntPoints[ iP ]));
            for ( int iF = 0; iF < 3; ++iF )
            {
              _Face& quad = _hexQuads[ facets[iF] - SMESH_Block::ID_FirstF ];
              equalNode = findEqualNode( quad._eIntNodes, _eIntPoints[ iP ], tol2 );
              if ( equalNode ) {
                equalNode->Add( _eIntPoints[ iP ] );
              }
              else {
                quad._eIntNodes.push_back( & _intNodes.back() );
              }
            }
          }
          break;
        }
        } // switch( nbFacets )

        if ( nbFacets == 0 ||
             _grid->_shapes( _eIntPoints[ iP ]->_shapeID ).ShapeType() == TopAbs_VERTEX )
        {
          equalNode = findEqualNode( _vIntNodes, _eIntPoints[ iP ], tol2 );
          if ( equalNode ) {
            equalNode->Add( _eIntPoints[ iP ] );
          }
          else if ( nbFacets == 0 ) {
            if ( _intNodes.empty() || _intNodes.back().EdgeIntPnt() != _eIntPoints[ iP ])
              _intNodes.push_back( _Node( 0, _eIntPoints[ iP ]));
            _vIntNodes.push_back( & _intNodes.back() );
          }
        }
      } // loop on _eIntPoints
    }
    else if ( 3 < _nbCornerNodes && _nbCornerNodes < 8 ) // _nbFaceIntNodes == 0
    {
      _Link split;
      // create sub-links (_splits) of whole links
      for ( int iLink = 0; iLink < 12; ++iLink )
      {
        _Link& link = _hexLinks[ iLink ];
        link._splits.clear();
        if ( link._nodes[ 0 ]->Node() && link._nodes[ 1 ]->Node() )
        {
          split._nodes[ 0 ] = link._nodes[0];
          split._nodes[ 1 ] = link._nodes[1];
          link._splits.push_back( split );
        }
      }
    }

  }
  //================================================================================
  /*!
   * \brief Initializes its data by given grid cell (countered from zero)
   */
  void Hexahedron::init( size_t iCell )
  {
    size_t iNbCell = _grid->_coords[0].size() - 1;
    size_t jNbCell = _grid->_coords[1].size() - 1;
    _i = iCell % iNbCell;
    _j = ( iCell % ( iNbCell * jNbCell )) / iNbCell;
    _k = iCell / iNbCell / jNbCell;
    init( _i, _j, _k );
  }

  //================================================================================
  /*!
   * \brief Compute mesh volumes resulted from intersection of the Hexahedron
   */
  void Hexahedron::ComputeElements()
  {
    Init();

    int nbIntersections = _nbFaceIntNodes + _eIntPoints.size();
    if ( _nbCornerNodes + nbIntersections < 4 )
      return;

    if ( _nbBndNodes == _nbCornerNodes && nbIntersections == 0 && isInHole() )
      return;

    _polygons.clear();
    _polygons.reserve( 20 );

    // Create polygons from quadrangles
    // --------------------------------

    vector< _OrientedLink > splits;
    vector<_Node*>          chainNodes;
    _Face*                  coplanarPolyg;

    bool hasEdgeIntersections = !_eIntPoints.empty();

    for ( int iF = 0; iF < 6; ++iF ) // loop on 6 sides of a hexahedron
    {
      _Face& quad = _hexQuads[ iF ] ;

      _polygons.resize( _polygons.size() + 1 );
      _Face* polygon = &_polygons.back();
      polygon->_polyLinks.reserve( 20 );

      splits.clear();
      for ( int iE = 0; iE < 4; ++iE ) // loop on 4 sides of a quadrangle
        for ( int iS = 0; iS < quad._links[ iE ].NbResultLinks(); ++iS )
          splits.push_back( quad._links[ iE ].ResultLink( iS ));

      // add splits of links to a polygon and add _polyLinks to make
      // polygon's boundary closed

      int nbSplits = splits.size();
      if (( nbSplits == 1 ) &&
          ( quad._eIntNodes.empty() ||
            splits[0].FirstNode()->IsLinked( splits[0].LastNode()->_intPoint )))
          //( quad._eIntNodes.empty() || _nbCornerNodes + nbIntersections > 6 ))
        nbSplits = 0;

#ifdef _DEBUG_
      for ( size_t iP = 0; iP < quad._eIntNodes.size(); ++iP )
        if ( quad._eIntNodes[ iP ]->IsUsedInFace( polygon ))
          quad._eIntNodes[ iP ]->_usedInFace = 0;
#endif
      int nbUsedEdgeNodes = 0;
      _Face* prevPolyg = 0; // polygon previously created from this quad

      while ( nbSplits > 0 )
      {
        size_t iS = 0;
        while ( !splits[ iS ] )
          ++iS;

        if ( !polygon->_links.empty() )
        {
          _polygons.resize( _polygons.size() + 1 );
          polygon = &_polygons.back();
          polygon->_polyLinks.reserve( 20 );
        }
        polygon->_links.push_back( splits[ iS ] );
        splits[ iS++ ]._link = 0;
        --nbSplits;

        _Node* nFirst = polygon->_links.back().FirstNode();
        _Node *n1,*n2 = polygon->_links.back().LastNode();
        for ( ; nFirst != n2 && iS < splits.size(); ++iS )
        {
          _OrientedLink& split = splits[ iS ];
          if ( !split ) continue;

          n1 = split.FirstNode();
          if ( n1 == n2 &&
               n1->_intPoint &&
               n1->_intPoint->_faceIDs.size() > 1 )
          {
            // n1 is at intersection with EDGE
            if ( findChainOnEdge( splits, polygon->_links.back(), split, iS, quad, chainNodes ))
            {
              for ( size_t i = 1; i < chainNodes.size(); ++i )
                polygon->AddPolyLink( chainNodes[i-1], chainNodes[i], prevPolyg );
              prevPolyg = polygon;
              n2 = chainNodes.back();
              continue;
            }
          }
          else if ( n1 != n2 )
          {
            // try to connect to intersections with EDGEs
            if ( quad._eIntNodes.size() > nbUsedEdgeNodes  &&
                 findChain( n2, n1, quad, chainNodes ))
            {
              for ( size_t i = 1; i < chainNodes.size(); ++i )
              {
                polygon->AddPolyLink( chainNodes[i-1], chainNodes[i] );
                nbUsedEdgeNodes += ( chainNodes[i]->IsUsedInFace( polygon ));
              }
              if ( chainNodes.back() != n1 )
              {
                n2 = chainNodes.back();
                --iS;
                continue;
              }
            }
            // try to connect to a split ending on the same FACE
            else
            {
              _OrientedLink foundSplit;
              for ( int i = iS; i < splits.size() && !foundSplit; ++i )
                if (( foundSplit = splits[ i ]) &&
                    ( n2->IsLinked( foundSplit.FirstNode()->_intPoint )))
                {
                  iS = i - 1;
                }
                else
                {
                  foundSplit._link = 0;
                }
              if ( foundSplit )
              {
                if ( n2 != foundSplit.FirstNode() )
                {
                  polygon->AddPolyLink( n2, foundSplit.FirstNode() );
                  n2 = foundSplit.FirstNode();
                }
                continue;
              }
              else
              {
                if ( n2->IsLinked( nFirst->_intPoint ))
                  break;
                polygon->AddPolyLink( n2, n1, prevPolyg );
              }
            }
          } // if ( n1 != n2 )

          polygon->_links.push_back( split );
          split._link = 0;
          --nbSplits;
          n2 = polygon->_links.back().LastNode();

        } // loop on splits

        if ( nFirst != n2 ) // close a polygon
        {
          if ( !findChain( n2, nFirst, quad, chainNodes ))
          {
            if ( !closePolygon( polygon, chainNodes ))
              if ( !isImplementEdges() )
                chainNodes.push_back( nFirst );
          }
          for ( size_t i = 1; i < chainNodes.size(); ++i )
          {
            polygon->AddPolyLink( chainNodes[i-1], chainNodes[i], prevPolyg );
            nbUsedEdgeNodes += bool( chainNodes[i]->IsUsedInFace( polygon ));
          }
        }

        if ( polygon->_links.size() < 3 && nbSplits > 0 )
        {
          polygon->_polyLinks.clear();
          polygon->_links.clear();
        }
      } // while ( nbSplits > 0 )

      if ( polygon->_links.size() < 3 )
      {
        _polygons.pop_back();
      }
    }  // loop on 6 hexahedron sides

    // Create polygons closing holes in a polyhedron
    // ----------------------------------------------

    // clear _usedInFace
    for ( size_t iN = 0; iN < _intNodes.size(); ++iN )
      _intNodes[ iN ]._usedInFace = 0;

    // add polygons to their links and mark used nodes
    for ( size_t iP = 0; iP < _polygons.size(); ++iP )
    {
      _Face& polygon = _polygons[ iP ];
      for ( size_t iL = 0; iL < polygon._links.size(); ++iL )
      {
        polygon._links[ iL ].AddFace( &polygon );
        polygon._links[ iL ].FirstNode()->_usedInFace = &polygon;
      }
    }
    // find free links
    vector< _OrientedLink* > freeLinks;
    freeLinks.reserve(20);
    for ( size_t iP = 0; iP < _polygons.size(); ++iP )
    {
      _Face& polygon = _polygons[ iP ];
      for ( size_t iL = 0; iL < polygon._links.size(); ++iL )
        if ( polygon._links[ iL ].NbFaces() < 2 )
          freeLinks.push_back( & polygon._links[ iL ]);
    }
    int nbFreeLinks = freeLinks.size();
    if ( nbFreeLinks == 1 ) return;

    // put not used intersection nodes to _vIntNodes
    int nbVertexNodes = 0; // nb not used vertex nodes
    {
      for ( size_t iN = 0; iN < _vIntNodes.size(); ++iN )
        nbVertexNodes += ( !_vIntNodes[ iN ]->IsUsedInFace() );

      const double tol = 1e-3 * Min( Min( _sideLength[0], _sideLength[1] ), _sideLength[0] );
      for ( size_t iN = _nbFaceIntNodes; iN < _intNodes.size(); ++iN )
      {
        if ( _intNodes[ iN ].IsUsedInFace() ) continue;
        if ( dynamic_cast< const F_IntersectPoint* >( _intNodes[ iN ]._intPoint )) continue;
        _Node* equalNode =
          findEqualNode( _vIntNodes, _intNodes[ iN ].EdgeIntPnt(), tol*tol );
        if ( !equalNode )
        {
          _vIntNodes.push_back( &_intNodes[ iN ]);
          ++nbVertexNodes;
        }
      }
    }

    set<TGeomID> usedFaceIDs;
    vector< TGeomID > faces;
    TGeomID curFace = 0;
    const size_t nbQuadPolygons = _polygons.size();
    E_IntersectPoint ipTmp;

    // create polygons by making closed chains of free links
    size_t iPolygon = _polygons.size();
    while ( nbFreeLinks > 0 )
    {
      if ( iPolygon == _polygons.size() )
      {
        _polygons.resize( _polygons.size() + 1 );
        _polygons[ iPolygon ]._polyLinks.reserve( 20 );
        _polygons[ iPolygon ]._links.reserve( 20 );
      }
      _Face& polygon = _polygons[ iPolygon ];

      _OrientedLink* curLink = 0;
      _Node*         curNode;
      if (( !hasEdgeIntersections ) ||
          ( nbFreeLinks < 4 && nbVertexNodes == 0 ))
      {
        // get a remaining link to start from
        for ( size_t iL = 0; iL < freeLinks.size() && !curLink; ++iL )
          if (( curLink = freeLinks[ iL ] ))
            freeLinks[ iL ] = 0;
        polygon._links.push_back( *curLink );
        --nbFreeLinks;
        do
        {
          // find all links connected to curLink
          curNode = curLink->FirstNode();
          curLink = 0;
          for ( size_t iL = 0; iL < freeLinks.size() && !curLink; ++iL )
            if ( freeLinks[ iL ] && freeLinks[ iL ]->LastNode() == curNode )
            {
              curLink = freeLinks[ iL ];
              freeLinks[ iL ] = 0;
              --nbFreeLinks;
              polygon._links.push_back( *curLink );
            }
        } while ( curLink );
      }
      else // there are intersections with EDGEs
      {
        // get a remaining link to start from, one lying on minimal nb of FACEs
        {
          typedef pair< TGeomID, int > TFaceOfLink;
          TFaceOfLink faceOfLink( -1, -1 );
          TFaceOfLink facesOfLink[3] = { faceOfLink, faceOfLink, faceOfLink };
          for ( size_t iL = 0; iL < freeLinks.size(); ++iL )
            if ( freeLinks[ iL ] )
            {
              faces = freeLinks[ iL ]->GetNotUsedFace( usedFaceIDs );
              if ( faces.size() == 1 )
              {
                faceOfLink = TFaceOfLink( faces[0], iL );
                if ( !freeLinks[ iL ]->HasEdgeNodes() )
                  break;
                facesOfLink[0] = faceOfLink;
              }
              else if ( facesOfLink[0].first < 0 )
              {
                faceOfLink = TFaceOfLink(( faces.empty() ? -1 : faces[0]), iL );
                facesOfLink[ 1 + faces.empty() ] = faceOfLink;
              }
            }
          for ( int i = 0; faceOfLink.first < 0 && i < 3; ++i )
            faceOfLink = facesOfLink[i];

          if ( faceOfLink.first < 0 ) // all faces used
          {
            for ( size_t iL = 0; iL < freeLinks.size() && faceOfLink.first < 1; ++iL )
              if (( curLink = freeLinks[ iL ]))
              {
                faceOfLink.first = 
                  curLink->FirstNode()->IsLinked( curLink->LastNode()->_intPoint );
                faceOfLink.second = iL;
              }
            usedFaceIDs.clear();
          }
          curFace = faceOfLink.first;
          curLink = freeLinks[ faceOfLink.second ];
          freeLinks[ faceOfLink.second ] = 0;
        }
        usedFaceIDs.insert( curFace );
        polygon._links.push_back( *curLink );
        --nbFreeLinks;

        // find all links lying on a curFace
        do
        {
          // go forward from curLink
          curNode = curLink->LastNode();
          curLink = 0;
          for ( size_t iL = 0; iL < freeLinks.size() && !curLink; ++iL )
            if ( freeLinks[ iL ] &&
                 freeLinks[ iL ]->FirstNode() == curNode &&
                 freeLinks[ iL ]->LastNode()->IsOnFace( curFace ))
            {
              curLink = freeLinks[ iL ];
              freeLinks[ iL ] = 0;
              polygon._links.push_back( *curLink );
              --nbFreeLinks;
            }
        } while ( curLink );

        std::reverse( polygon._links.begin(), polygon._links.end() );

        curLink = & polygon._links.back();
        do
        {
          // go backward from curLink
          curNode = curLink->FirstNode();
          curLink = 0;
          for ( size_t iL = 0; iL < freeLinks.size() && !curLink; ++iL )
            if ( freeLinks[ iL ] &&
                 freeLinks[ iL ]->LastNode() == curNode &&
                 freeLinks[ iL ]->FirstNode()->IsOnFace( curFace ))
            {
              curLink = freeLinks[ iL ];
              freeLinks[ iL ] = 0;
              polygon._links.push_back( *curLink );
              --nbFreeLinks;
            }
        } while ( curLink );

        curNode = polygon._links.back().FirstNode();

        if ( polygon._links[0].LastNode() != curNode )
        {
          if ( nbVertexNodes > 0 )
          {
            // add links with _vIntNodes if not already used
            chainNodes.clear();
            for ( size_t iN = 0; iN < _vIntNodes.size(); ++iN )
              if ( !_vIntNodes[ iN ]->IsUsedInFace() &&
                   _vIntNodes[ iN ]->IsOnFace( curFace ))
              {
                _vIntNodes[ iN ]->_usedInFace = &polygon;
                chainNodes.push_back( _vIntNodes[ iN ] );
              }
            if ( chainNodes.size() > 1 )
            {
              sortVertexNodes( chainNodes, curNode, curFace );
            }
            for ( int i = 0; i < chainNodes.size(); ++i )
            {
              polygon.AddPolyLink( chainNodes[ i ], curNode );
              curNode = chainNodes[ i ];
              freeLinks.push_back( &polygon._links.back() );
              ++nbFreeLinks;
            }
            nbVertexNodes -= chainNodes.size();
          }
          // if ( polygon._links.size() > 1 )
          {
            polygon.AddPolyLink( polygon._links[0].LastNode(), curNode );
            freeLinks.push_back( &polygon._links.back() );
            ++nbFreeLinks;
          }
        }
      } // if there are intersections with EDGEs

      if ( polygon._links.size() < 2 ||
           polygon._links[0].LastNode() != polygon._links.back().FirstNode() )
        return; // closed polygon not found -> invalid polyhedron

      if ( polygon._links.size() == 2 )
      {
        if ( freeLinks.back() == &polygon._links.back() )
        {
          freeLinks.pop_back();
          --nbFreeLinks;
        }
        if ( polygon._links.front().NbFaces() > 0 )
          polygon._links.back().AddFace( polygon._links.front()._link->_faces[0] );
        if ( polygon._links.back().NbFaces() > 0 )
          polygon._links.front().AddFace( polygon._links.back()._link->_faces[0] );

        if ( iPolygon == _polygons.size()-1 )
          _polygons.pop_back();
      }
      else // polygon._links.size() >= 2
      {
        // add polygon to its links
        for ( size_t iL = 0; iL < polygon._links.size(); ++iL )
        {
          polygon._links[ iL ].AddFace( &polygon );
          polygon._links[ iL ].Reverse();
        }
        if ( /*hasEdgeIntersections &&*/ iPolygon == _polygons.size() - 1 )
        {
          // check that a polygon does not lie on a hexa side
          coplanarPolyg = 0;
          for ( size_t iL = 0; iL < polygon._links.size() && !coplanarPolyg; ++iL )
          {
            if ( polygon._links[ iL ].NbFaces() < 2 )
              continue; // it's a just added free link
            // look for a polygon made on a hexa side and sharing
            // two or more haxa links
            size_t iL2;
            coplanarPolyg = polygon._links[ iL ]._link->_faces[0];
            for ( iL2 = iL + 1; iL2 < polygon._links.size(); ++iL2 )
              if ( polygon._links[ iL2 ]._link->_faces[0] == coplanarPolyg &&
                   !coplanarPolyg->IsPolyLink( polygon._links[ iL  ]) &&
                   !coplanarPolyg->IsPolyLink( polygon._links[ iL2 ]) &&
                   coplanarPolyg < & _polygons[ nbQuadPolygons ])
                break;
            if ( iL2 == polygon._links.size() )
              coplanarPolyg = 0;
          }
          if ( coplanarPolyg ) // coplanar polygon found
          {
            freeLinks.resize( freeLinks.size() - polygon._polyLinks.size() );
            nbFreeLinks -= polygon._polyLinks.size();

            // an E_IntersectPoint used to mark nodes of coplanarPolyg
            // as lying on curFace while they are not at intersection with geometry
            ipTmp._faceIDs.resize(1);
            ipTmp._faceIDs[0] = curFace;

            // fill freeLinks with links not shared by coplanarPolyg and polygon
            for ( size_t iL = 0; iL < polygon._links.size(); ++iL )
              if ( polygon._links[ iL ]._link->_faces[1] &&
                   polygon._links[ iL ]._link->_faces[0] != coplanarPolyg )
              {
                _Face* p = polygon._links[ iL ]._link->_faces[0];
                for ( size_t iL2 = 0; iL2 < p->_links.size(); ++iL2 )
                  if ( p->_links[ iL2 ]._link == polygon._links[ iL ]._link )
                  {
                    freeLinks.push_back( & p->_links[ iL2 ] );
                    ++nbFreeLinks;
                    freeLinks.back()->RemoveFace( &polygon );
                    break;
                  }
              }
            for ( size_t iL = 0; iL < coplanarPolyg->_links.size(); ++iL )
              if ( coplanarPolyg->_links[ iL ]._link->_faces[1] &&
                   coplanarPolyg->_links[ iL ]._link->_faces[1] != &polygon )
              {
                _Face* p = coplanarPolyg->_links[ iL ]._link->_faces[0];
                if ( p == coplanarPolyg )
                  p = coplanarPolyg->_links[ iL ]._link->_faces[1];
                for ( size_t iL2 = 0; iL2 < p->_links.size(); ++iL2 )
                  if ( p->_links[ iL2 ]._link == coplanarPolyg->_links[ iL ]._link )
                  {
                    // set links of coplanarPolyg in place of used freeLinks
                    // to re-create coplanarPolyg next
                    size_t iL3 = 0;
                    for ( ; iL3 < freeLinks.size() && freeLinks[ iL3 ]; ++iL3 );
                    if ( iL3 < freeLinks.size() )
                      freeLinks[ iL3 ] = ( & p->_links[ iL2 ] );
                    else
                      freeLinks.push_back( & p->_links[ iL2 ] );
                    ++nbFreeLinks;
                    freeLinks[ iL3 ]->RemoveFace( coplanarPolyg );
                    //  mark nodes of coplanarPolyg as lying on curFace
                    for ( int iN = 0; iN < 2; ++iN )
                    {
                      _Node* n = freeLinks[ iL3 ]->_link->_nodes[ iN ];
                      if ( n->_intPoint ) n->_intPoint->Add( ipTmp._faceIDs );
                      else                n->_intPoint = &ipTmp;
                    }
                    break;
                  }
              }
            // set coplanarPolyg to be re-created next
            for ( size_t iP = 0; iP < _polygons.size(); ++iP )
              if ( coplanarPolyg == & _polygons[ iP ] )
              {
                iPolygon = iP;
                _polygons[ iPolygon ]._links.clear();
                _polygons[ iPolygon ]._polyLinks.clear();
                break;
              }
            _polygons.pop_back();
            usedFaceIDs.erase( curFace );
            continue;
          } // if ( coplanarPolyg )
        } // if ( hasEdgeIntersections ) - search for coplanarPolyg

        iPolygon = _polygons.size();

      } // end of case ( polygon._links.size() > 2 )
    } // while ( nbFreeLinks > 0 )

    if ( ! checkPolyhedronSize() )
    {
      return;
    }

    for ( size_t i = 0; i < 8; ++i )
      if ( _hexNodes[ i ]._intPoint == &ipTmp )
        _hexNodes[ i ]._intPoint = 0;

    // create a classic cell if possible

    int nbPolygons = 0;
    for ( size_t iF = 0; iF < _polygons.size(); ++iF )
      nbPolygons += (_polygons[ iF ]._links.size() > 0 );

    //const int nbNodes = _nbCornerNodes + nbIntersections;
    int nbNodes = 0;
    for ( size_t i = 0; i < 8; ++i )
      nbNodes += _hexNodes[ i ].IsUsedInFace();
    for ( size_t i = 0; i < _intNodes.size(); ++i )
      nbNodes += _intNodes[ i ].IsUsedInFace();

    bool isClassicElem = false;
    if (      nbNodes == 8 && nbPolygons == 6 ) isClassicElem = addHexa();
    else if ( nbNodes == 4 && nbPolygons == 4 ) isClassicElem = addTetra();
    else if ( nbNodes == 6 && nbPolygons == 5 ) isClassicElem = addPenta();
    else if ( nbNodes == 5 && nbPolygons == 5 ) isClassicElem = addPyra ();
    if ( !isClassicElem )
    {
      _volumeDefs._nodes.clear();
      _volumeDefs._quantities.clear();

      for ( size_t iF = 0; iF < _polygons.size(); ++iF )
      {
        const size_t nbLinks = _polygons[ iF ]._links.size();
        if ( nbLinks == 0 ) continue;
        _volumeDefs._quantities.push_back( nbLinks );
        for ( size_t iL = 0; iL < nbLinks; ++iL )
          _volumeDefs._nodes.push_back( _polygons[ iF ]._links[ iL ].FirstNode() );
      }
    }
  }
  //================================================================================
  /*!
   * \brief Create elements in the mesh
   */
  int Hexahedron::MakeElements(SMESH_MesherHelper&                      helper,
                               const map< TGeomID, vector< TGeomID > >& edge2faceIDsMap)
  {
    SMESHDS_Mesh* mesh = helper.GetMeshDS();

    size_t nbCells[3] = { _grid->_coords[0].size() - 1,
                          _grid->_coords[1].size() - 1,
                          _grid->_coords[2].size() - 1 };
    const size_t nbGridCells = nbCells[0] * nbCells[1] * nbCells[2];
    vector< Hexahedron* > allHexa( nbGridCells, 0 );
    int nbIntHex = 0;

    // set intersection nodes from GridLine's to links of allHexa
    int i,j,k, iDirOther[3][2] = {{ 1,2 },{ 0,2 },{ 0,1 }};
    for ( int iDir = 0; iDir < 3; ++iDir )
    {
      int dInd[4][3] = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} };
      dInd[1][ iDirOther[iDir][0] ] = -1;
      dInd[2][ iDirOther[iDir][1] ] = -1;
      dInd[3][ iDirOther[iDir][0] ] = -1; dInd[3][ iDirOther[iDir][1] ] = -1;
      // loop on GridLine's parallel to iDir
      LineIndexer lineInd = _grid->GetLineIndexer( iDir );
      for ( ; lineInd.More(); ++lineInd )
      {
        GridLine& line = _grid->_lines[ iDir ][ lineInd.LineIndex() ];
        multiset< F_IntersectPoint >::const_iterator ip = line._intPoints.begin();
        for ( ; ip != line._intPoints.end(); ++ip )
        {
          // if ( !ip->_node ) continue; // intersection at a grid node
          lineInd.SetIndexOnLine( ip->_indexOnLine );
          for ( int iL = 0; iL < 4; ++iL ) // loop on 4 cells sharing a link
          {
            i = int(lineInd.I()) + dInd[iL][0];
            j = int(lineInd.J()) + dInd[iL][1];
            k = int(lineInd.K()) + dInd[iL][2];
            if ( i < 0 || i >= nbCells[0] ||
                 j < 0 || j >= nbCells[1] ||
                 k < 0 || k >= nbCells[2] ) continue;

            const size_t hexIndex = _grid->CellIndex( i,j,k );
            Hexahedron *& hex = allHexa[ hexIndex ];
            if ( !hex)
            {
              hex = new Hexahedron( *this );
              hex->_i = i;
              hex->_j = j;
              hex->_k = k;
              ++nbIntHex;
            }
            const int iLink = iL + iDir * 4;
            hex->_hexLinks[iLink]._fIntPoints.push_back( &(*ip) );
            hex->_nbFaceIntNodes += bool( ip->_node );
          }
        }
      }
    }

    // implement geom edges into the mesh
    addEdges( helper, allHexa, edge2faceIDsMap );

    // add not split hexadrons to the mesh
    int nbAdded = 0;
    vector< Hexahedron* > intHexa( nbIntHex, (Hexahedron*) NULL );
    for ( size_t i = 0; i < allHexa.size(); ++i )
    {
      Hexahedron * & hex = allHexa[ i ];
      if ( hex )
      {
        intHexa.push_back( hex );
        if ( hex->_nbFaceIntNodes > 0 || hex->_eIntPoints.size() > 0 )
          continue; // treat intersected hex later
        this->init( hex->_i, hex->_j, hex->_k );
      }
      else
      {    
        this->init( i );
      }
      if (( _nbCornerNodes == 8 ) &&
          ( _nbBndNodes < _nbCornerNodes || !isInHole() ))
      {
        // order of _hexNodes is defined by enum SMESH_Block::TShapeID
        SMDS_MeshElement* el =
          mesh->AddVolume( _hexNodes[0].Node(), _hexNodes[2].Node(),
                           _hexNodes[3].Node(), _hexNodes[1].Node(),
                           _hexNodes[4].Node(), _hexNodes[6].Node(),
                           _hexNodes[7].Node(), _hexNodes[5].Node() );
        mesh->SetMeshElementOnShape( el, helper.GetSubShapeID() );
        ++nbAdded;
        if ( hex )
          intHexa.pop_back();
      }
      else if ( _nbCornerNodes > 3  && !hex )
      {
        // all intersection of hex with geometry are at grid nodes
        hex = new Hexahedron( *this );
        hex->_i = _i;
        hex->_j = _j;
        hex->_k = _k;
        intHexa.push_back( hex );
      }
    }

    // add elements resulted from hexadron intersection
#ifdef WITH_TBB
    tbb::parallel_for ( tbb::blocked_range<size_t>( 0, intHexa.size() ),
                        ParallelHexahedron( intHexa ),
                        tbb::simple_partitioner()); // ComputeElements() is called here
    for ( size_t i = 0; i < intHexa.size(); ++i )
      if ( Hexahedron * hex = intHexa[ i ] )
        nbAdded += hex->addElements( helper );
#else
    for ( size_t i = 0; i < intHexa.size(); ++i )
      if ( Hexahedron * hex = intHexa[ i ] )
      {
        hex->ComputeElements();
        nbAdded += hex->addElements( helper );
      }
#endif

    for ( size_t i = 0; i < allHexa.size(); ++i )
      if ( allHexa[ i ] )
        delete allHexa[ i ];

    return nbAdded;
  }

  //================================================================================
  /*!
   * \brief Implements geom edges into the mesh
   */
  void Hexahedron::addEdges(SMESH_MesherHelper&                      helper,
                            vector< Hexahedron* >&                   hexes,
                            const map< TGeomID, vector< TGeomID > >& edge2faceIDsMap)
  {
    if ( edge2faceIDsMap.empty() ) return;

    // Prepare planes for intersecting with EDGEs
    GridPlanes pln[3];
    {
      for ( int iDirZ = 0; iDirZ < 3; ++iDirZ ) // iDirZ gives normal direction to planes
      {
        GridPlanes& planes = pln[ iDirZ ];
        int iDirX = ( iDirZ + 1 ) % 3;
        int iDirY = ( iDirZ + 2 ) % 3;
        planes._zNorm  = ( _grid->_axes[ iDirX ] ^ _grid->_axes[ iDirY ] ).Normalized();
        planes._zProjs.resize ( _grid->_coords[ iDirZ ].size() );
        planes._zProjs [0] = 0;
        const double       zFactor = _grid->_axes[ iDirZ ] * planes._zNorm;
        const vector< double > & u = _grid->_coords[ iDirZ ];
        for ( int i = 1; i < planes._zProjs.size(); ++i )
        {
          planes._zProjs [i] = zFactor * ( u[i] - u[0] );
        }
      }
    }
    const double deflection = _grid->_minCellSize / 20.;
    const double tol        = _grid->_tol;
    E_IntersectPoint ip;

    // Intersect EDGEs with the planes
    map< TGeomID, vector< TGeomID > >::const_iterator e2fIt = edge2faceIDsMap.begin();
    for ( ; e2fIt != edge2faceIDsMap.end(); ++e2fIt )
    {
      const TGeomID  edgeID = e2fIt->first;
      const TopoDS_Edge & E = TopoDS::Edge( _grid->_shapes( edgeID ));
      BRepAdaptor_Curve curve( E );
      TopoDS_Vertex v1 = helper.IthVertex( 0, E, false ); 
      TopoDS_Vertex v2 = helper.IthVertex( 1, E, false ); 

      ip._faceIDs = e2fIt->second;
      ip._shapeID = edgeID;

      // discretize the EGDE
      GCPnts_UniformDeflection discret( curve, deflection, true );
      if ( !discret.IsDone() || discret.NbPoints() < 2 )
        continue;

      // perform intersection
      for ( int iDirZ = 0; iDirZ < 3; ++iDirZ )
      {
        GridPlanes& planes = pln[ iDirZ ];
        int      iDirX = ( iDirZ + 1 ) % 3;
        int      iDirY = ( iDirZ + 2 ) % 3;
        double    xLen = _grid->_coords[ iDirX ].back() - _grid->_coords[ iDirX ][0];
        double    yLen = _grid->_coords[ iDirY ].back() - _grid->_coords[ iDirY ][0];
        double    zLen = _grid->_coords[ iDirZ ].back() - _grid->_coords[ iDirZ ][0];
        int dIJK[3], d000[3] = { 0,0,0 };
        double o[3] = { _grid->_coords[0][0],
                        _grid->_coords[1][0],
                        _grid->_coords[2][0] };

        // locate the 1st point of a segment within the grid
        gp_XYZ p1     = discret.Value( 1 ).XYZ();
        double u1     = discret.Parameter( 1 );
        double zProj1 = planes._zNorm * ( p1 - _grid->_origin );

        _grid->ComputeUVW( p1, ip._uvw );
        int iX1 = int(( ip._uvw[iDirX] - o[iDirX]) / xLen * (_grid->_coords[ iDirX ].size() - 1));
        int iY1 = int(( ip._uvw[iDirY] - o[iDirY]) / yLen * (_grid->_coords[ iDirY ].size() - 1));
        int iZ1 = int(( ip._uvw[iDirZ] - o[iDirZ]) / zLen * (_grid->_coords[ iDirZ ].size() - 1));
        locateValue( iX1, ip._uvw[iDirX], _grid->_coords[ iDirX ], dIJK[ iDirX ], tol );
        locateValue( iY1, ip._uvw[iDirY], _grid->_coords[ iDirY ], dIJK[ iDirY ], tol );
        locateValue( iZ1, ip._uvw[iDirZ], _grid->_coords[ iDirZ ], dIJK[ iDirZ ], tol );

        int ijk[3]; // grid index where a segment intersect a plane
        ijk[ iDirX ] = iX1;
        ijk[ iDirY ] = iY1;
        ijk[ iDirZ ] = iZ1;

        // add the 1st vertex point to a hexahedron
        if ( iDirZ == 0 )
        {
          ip._point   = p1;
          ip._shapeID = _grid->_shapes.Add( v1 );
          _grid->_edgeIntP.push_back( ip );
          if ( !addIntersection( _grid->_edgeIntP.back(), hexes, ijk, d000 ))
            _grid->_edgeIntP.pop_back();
          ip._shapeID = edgeID;
        }
        for ( int iP = 2; iP <= discret.NbPoints(); ++iP )
        {
          // locate the 2nd point of a segment within the grid
          gp_XYZ p2     = discret.Value( iP ).XYZ();
          double u2     = discret.Parameter( iP );
          double zProj2 = planes._zNorm * ( p2 - _grid->_origin );
          int    iZ2    = iZ1;
          if ( Abs( zProj2 - zProj1 ) > std::numeric_limits<double>::min() )
          {
            locateValue( iZ2, zProj2, planes._zProjs, dIJK[ iDirZ ], tol );

            // treat intersections with planes between 2 end points of a segment
            int dZ = ( iZ1 <= iZ2 ) ? +1 : -1;
            int iZ = iZ1 + ( iZ1 < iZ2 );
            for ( int i = 0, nb = Abs( iZ1 - iZ2 ); i < nb; ++i, iZ += dZ )
            {
              ip._point = findIntPoint( u1, zProj1, u2, zProj2,
                                        planes._zProjs[ iZ ],
                                        curve, planes._zNorm, _grid->_origin );
              _grid->ComputeUVW( ip._point.XYZ(), ip._uvw );
              locateValue( ijk[iDirX], ip._uvw[iDirX], _grid->_coords[iDirX], dIJK[iDirX], tol );
              locateValue( ijk[iDirY], ip._uvw[iDirY], _grid->_coords[iDirY], dIJK[iDirY], tol );
              ijk[ iDirZ ] = iZ;

              // add ip to hex "above" the plane
              _grid->_edgeIntP.push_back( ip );
              dIJK[ iDirZ ] = 0;
              bool added = addIntersection(_grid->_edgeIntP.back(), hexes, ijk, dIJK);

              // add ip to hex "below" the plane
              ijk[ iDirZ ] = iZ-1;
              if ( !addIntersection( _grid->_edgeIntP.back(), hexes, ijk, dIJK ) &&
                   !added)
                _grid->_edgeIntP.pop_back();
            }
          }
          iZ1    = iZ2;
          p1     = p2;
          u1     = u2;
          zProj1 = zProj2;
        }
        // add the 2nd vertex point to a hexahedron
        if ( iDirZ == 0 )
        {
          ip._shapeID = _grid->_shapes.Add( v2 );
          ip._point = p1;
          _grid->ComputeUVW( p1, ip._uvw );
          locateValue( ijk[iDirX], ip._uvw[iDirX], _grid->_coords[iDirX], dIJK[iDirX], tol );
          locateValue( ijk[iDirY], ip._uvw[iDirY], _grid->_coords[iDirY], dIJK[iDirY], tol );
          ijk[ iDirZ ] = iZ1;
          _grid->_edgeIntP.push_back( ip );
          if ( !addIntersection( _grid->_edgeIntP.back(), hexes, ijk, d000 ))
            _grid->_edgeIntP.pop_back();
          ip._shapeID = edgeID;
        }
      } // loop on 3 grid directions
    } // loop on EDGEs

  }

  //================================================================================
  /*!
   * \brief Finds intersection of a curve with a plane
   *  \param [in] u1 - parameter of one curve point
   *  \param [in] proj1 - projection of the curve point to the plane normal
   *  \param [in] u2 - parameter of another curve point
   *  \param [in] proj2 - projection of the other curve point to the plane normal
   *  \param [in] proj - projection of a point where the curve intersects the plane
   *  \param [in] curve - the curve
   *  \param [in] axis - the plane normal
   *  \param [in] origin - the plane origin
   *  \return gp_Pnt - the found intersection point
   */
  gp_Pnt Hexahedron::findIntPoint( double u1, double proj1,
                                   double u2, double proj2,
                                   double proj,
                                   BRepAdaptor_Curve& curve,
                                   const gp_XYZ& axis,
                                   const gp_XYZ& origin)
  {
    double r = (( proj - proj1 ) / ( proj2 - proj1 ));
    double u = u1 * ( 1 - r ) + u2 * r;
    gp_Pnt p = curve.Value( u );
    double newProj =  axis * ( p.XYZ() - origin );
    if ( Abs( proj - newProj ) > _grid->_tol / 10. )
    {
      if ( r > 0.5 )
        return findIntPoint( u2, proj2, u, newProj, proj, curve, axis, origin );
      else
        return findIntPoint( u1, proj2, u, newProj, proj, curve, axis, origin );
    }
    return p;
  }

  //================================================================================
  /*!
   * \brief Returns indices of a hexahedron sub-entities holding a point
   *  \param [in] ip - intersection point
   *  \param [out] facets - 0-3 facets holding a point
   *  \param [out] sub - index of a vertex or an edge holding a point
   *  \return int - number of facets holding a point
   */
  int Hexahedron::getEntity( const E_IntersectPoint* ip, int* facets, int& sub )
  {
    enum { X = 1, Y = 2, Z = 4 }; // == 001, 010, 100
    int nbFacets = 0;
    int vertex = 0, egdeMask = 0;

    if ( Abs( _grid->_coords[0][ _i   ] - ip->_uvw[0] ) < _grid->_tol ) {
      facets[ nbFacets++ ] = SMESH_Block::ID_F0yz;
      egdeMask |= X;
    }
    else if ( Abs( _grid->_coords[0][ _i+1 ] - ip->_uvw[0] ) < _grid->_tol ) {
      facets[ nbFacets++ ] = SMESH_Block::ID_F1yz;
      vertex   |= X;
      egdeMask |= X;
    }
    if ( Abs( _grid->_coords[1][ _j   ] - ip->_uvw[1] ) < _grid->_tol ) {
      facets[ nbFacets++ ] = SMESH_Block::ID_Fx0z;
      egdeMask |= Y;
    }
    else if ( Abs( _grid->_coords[1][ _j+1 ] - ip->_uvw[1] ) < _grid->_tol ) {
      facets[ nbFacets++ ] = SMESH_Block::ID_Fx1z;
      vertex   |= Y;
      egdeMask |= Y;
    }
    if ( Abs( _grid->_coords[2][ _k   ] - ip->_uvw[2] ) < _grid->_tol ) {
      facets[ nbFacets++ ] = SMESH_Block::ID_Fxy0;
      egdeMask |= Z;
    }
    else if ( Abs( _grid->_coords[2][ _k+1 ] - ip->_uvw[2] ) < _grid->_tol ) {
      facets[ nbFacets++ ] = SMESH_Block::ID_Fxy1;
      vertex   |= Z;
      egdeMask |= Z;
    }

    switch ( nbFacets )
    {
    case 0: sub = 0;         break;
    case 1: sub = facets[0]; break;
    case 2: {
      const int edge [3][8] = {
        { SMESH_Block::ID_E00z, SMESH_Block::ID_E10z,
          SMESH_Block::ID_E01z, SMESH_Block::ID_E11z },
        { SMESH_Block::ID_E0y0, SMESH_Block::ID_E1y0, 0, 0,
          SMESH_Block::ID_E0y1, SMESH_Block::ID_E1y1 },
        { SMESH_Block::ID_Ex00, 0, SMESH_Block::ID_Ex10, 0,
          SMESH_Block::ID_Ex01, 0, SMESH_Block::ID_Ex11 }
      };
      switch ( egdeMask ) {
      case X | Y: sub = edge[ 0 ][ vertex ]; break;
      case X | Z: sub = edge[ 1 ][ vertex ]; break;
      default:    sub = edge[ 2 ][ vertex ];
      }
      break;
    }
    //case 3:
    default:
      sub = vertex + SMESH_Block::ID_FirstV;
    }

    return nbFacets;
  }
  //================================================================================
  /*!
   * \brief Adds intersection with an EDGE
   */
  bool Hexahedron::addIntersection( const E_IntersectPoint& ip,
                                    vector< Hexahedron* >&  hexes,
                                    int ijk[], int dIJK[] )
  {
    bool added = false;

    size_t hexIndex[4] = {
      _grid->CellIndex( ijk[0], ijk[1], ijk[2] ),
      dIJK[0] ? _grid->CellIndex( ijk[0]+dIJK[0], ijk[1], ijk[2] ) : -1,
      dIJK[1] ? _grid->CellIndex( ijk[0], ijk[1]+dIJK[1], ijk[2] ) : -1,
      dIJK[2] ? _grid->CellIndex( ijk[0], ijk[1], ijk[2]+dIJK[2] ) : -1
    };
    for ( int i = 0; i < 4; ++i )
    {
      if ( /*0 <= hexIndex[i] &&*/ hexIndex[i] < hexes.size() && hexes[ hexIndex[i] ] )
      {
        Hexahedron* h = hexes[ hexIndex[i] ];
        // check if ip is really inside the hex
#ifdef _DEBUG_
        if ( h->isOutParam( ip._uvw ))
          throw SALOME_Exception("ip outside a hex");
#endif
        h->_eIntPoints.push_back( & ip );
        added = true;
      }
    }
    return added;
  }
  //================================================================================
  /*!
   * \brief Finds nodes at a path from one node to another via intersections with EDGEs
   */
  bool Hexahedron::findChain( _Node*          n1,
                              _Node*          n2,
                              _Face&          quad,
                              vector<_Node*>& chn )
  {
    chn.clear();
    chn.push_back( n1 );
    for ( size_t iP = 0; iP < quad._eIntNodes.size(); ++iP )
      if ( !quad._eIntNodes[ iP ]->IsUsedInFace( &quad ) &&
           n1->IsLinked( quad._eIntNodes[ iP ]->_intPoint ) &&
           n2->IsLinked( quad._eIntNodes[ iP ]->_intPoint ))
      {
        chn.push_back( quad._eIntNodes[ iP ]);
        chn.push_back( n2 );
        quad._eIntNodes[ iP ]->_usedInFace = &quad;
        return true;
      }
    bool found;
    do
    {
      found = false;
      for ( size_t iP = 0; iP < quad._eIntNodes.size(); ++iP )
        if ( !quad._eIntNodes[ iP ]->IsUsedInFace( &quad ) &&
             chn.back()->IsLinked( quad._eIntNodes[ iP ]->_intPoint ))
        {
          chn.push_back( quad._eIntNodes[ iP ]);
          found = (quad._eIntNodes[ iP ]->_usedInFace = &quad);
          break;
        }
    } while ( found && ! chn.back()->IsLinked( n2->_intPoint ) );

    if ( chn.back() != n2 && chn.back()->IsLinked( n2->_intPoint ))
      chn.push_back( n2 );

    return chn.size() > 1;
  }
  //================================================================================
  /*!
   * \brief Try to heal a polygon whose ends are not connected
   */
  bool Hexahedron::closePolygon( _Face* polygon, vector<_Node*>& chainNodes ) const
  {
    int i = -1, nbLinks = polygon->_links.size();
    if ( nbLinks < 3 )
      return false;
    vector< _OrientedLink > newLinks;
    // find a node lying on the same FACE as the last one
    _Node*   node = polygon->_links.back().LastNode();
    int avoidFace = node->IsLinked( polygon->_links.back().FirstNode()->_intPoint );
    for ( i = nbLinks - 2; i >= 0; --i )
      if ( node->IsLinked( polygon->_links[i].FirstNode()->_intPoint, avoidFace ))
        break;
    if ( i >= 0 )
    {
      for ( ; i < nbLinks; ++i )
        newLinks.push_back( polygon->_links[i] );
    }
    else
    {
      // find a node lying on the same FACE as the first one
      node      = polygon->_links[0].FirstNode();
      avoidFace = node->IsLinked( polygon->_links[0].LastNode()->_intPoint );
      for ( i = 1; i < nbLinks; ++i )
        if ( node->IsLinked( polygon->_links[i].LastNode()->_intPoint, avoidFace ))
          break;
      if ( i < nbLinks )
        for ( nbLinks = i + 1, i = 0; i < nbLinks; ++i )
          newLinks.push_back( polygon->_links[i] );
    }
    if ( newLinks.size() > 1 )
    {
      polygon->_links.swap( newLinks );
      chainNodes.clear();
      chainNodes.push_back( polygon->_links.back().LastNode() );
      chainNodes.push_back( polygon->_links[0].FirstNode() );
      return true;
    }
    return false;
  }
  //================================================================================
  /*!
   * \brief Finds nodes on the same EDGE as the first node of avoidSplit.
   *
   * This function is for a case where an EDGE lies on a quad which lies on a FACE
   * so that a part of quad in ON and another part in IN
   */
  bool Hexahedron::findChainOnEdge( const vector< _OrientedLink >& splits,
                                    const _OrientedLink&           prevSplit,
                                    const _OrientedLink&           avoidSplit,
                                    size_t &                       iS,
                                    _Face&                         quad,
                                    vector<_Node*>&                chn )
  {
    if ( !isImplementEdges() )
      return false;

    _Node* pn1 = prevSplit.FirstNode();
    _Node* pn2 = prevSplit.LastNode();
    int avoidFace = pn1->IsLinked( pn2->_intPoint ); // FACE under the quad
    if ( avoidFace < 1 && pn1->_intPoint )
      return false;

    _Node* n, *stopNode = avoidSplit.LastNode();

    chn.clear();
    if ( !quad._eIntNodes.empty() )
    {
      chn.push_back( pn2 );
      bool found;
      do
      {
        found = false;
        for ( size_t iP = 0; iP < quad._eIntNodes.size(); ++iP )
          if (( !quad._eIntNodes[ iP ]->IsUsedInFace( &quad )) &&
              ( chn.back()->IsLinked( quad._eIntNodes[ iP ]->_intPoint, avoidFace )) &&
              ( !avoidFace || quad._eIntNodes[ iP ]->IsOnFace( avoidFace )))
          {
            chn.push_back( quad._eIntNodes[ iP ]);
            found = (quad._eIntNodes[ iP ]->_usedInFace = &quad);
            break;
          }
      } while ( found );
      pn2 = chn.back();
    }

    int i;
    for ( i = splits.size()-1; i >= 0; --i )
    {
      if ( !splits[i] )
        continue;

      n = splits[i].LastNode();
      if ( n == stopNode )
        break;
      if (( n != pn1 ) &&
          ( n->IsLinked( pn2->_intPoint, avoidFace )) &&
          ( !avoidFace || n->IsOnFace( avoidFace )))
        break;

      n = splits[i].FirstNode();
      if ( n == stopNode )
        break;
      if (( n->IsLinked( pn2->_intPoint, avoidFace )) &&
          ( !avoidFace || n->IsOnFace( avoidFace )))
        break;
      n = 0;
    }
    if ( n && n != stopNode)
    {
      if ( chn.empty() )
        chn.push_back( pn2 );
      chn.push_back( n );
      iS = i-1;
      return true;
    }
    return false;
  }
  //================================================================================
  /*!
   * \brief Checks transition at the ginen intersection node of a link
   */
  bool Hexahedron::isOutPoint( _Link& link, int iP, SMESH_MesherHelper& helper ) const
  {
    bool isOut = false;

    const bool moreIntPoints = ( iP+1 < link._fIntPoints.size() );

    // get 2 _Node's
    _Node* n1 = link._fIntNodes[ iP ];
    if ( !n1->Node() )
      n1 = link._nodes[0];
    _Node* n2 = moreIntPoints ? link._fIntNodes[ iP+1 ] : 0;
    if ( !n2 || !n2->Node() )
      n2 = link._nodes[1];
    if ( !n2->Node() )
      return true;

    // get all FACEs under n1 and n2
    set< TGeomID > faceIDs;
    if ( moreIntPoints ) faceIDs.insert( link._fIntPoints[iP+1]->_faceIDs.begin(),
                                         link._fIntPoints[iP+1]->_faceIDs.end() );
    if ( n2->_intPoint ) faceIDs.insert( n2->_intPoint->_faceIDs.begin(),
                                         n2->_intPoint->_faceIDs.end() );
    if ( faceIDs.empty() )
      return false; // n2 is inside
    if ( n1->_intPoint ) faceIDs.insert( n1->_intPoint->_faceIDs.begin(),
                                         n1->_intPoint->_faceIDs.end() );
    faceIDs.insert( link._fIntPoints[iP]->_faceIDs.begin(),
                    link._fIntPoints[iP]->_faceIDs.end() );

    // get a point between 2 nodes
    gp_Pnt p1      = n1->Point();
    gp_Pnt p2      = n2->Point();
    gp_Pnt pOnLink = 0.8 * p1.XYZ() + 0.2 * p2.XYZ();

    TopLoc_Location loc;

    set< TGeomID >::iterator faceID = faceIDs.begin();
    for ( ; faceID != faceIDs.end(); ++faceID )
    {
      // project pOnLink on a FACE
      if ( *faceID < 1 ) continue;
      const TopoDS_Face& face = TopoDS::Face( _grid->_shapes( *faceID ));
      GeomAPI_ProjectPointOnSurf& proj =
        helper.GetProjector( face, loc, 0.1*_grid->_tol );
      gp_Pnt testPnt = pOnLink.Transformed( loc.Transformation().Inverted() );
      proj.Perform( testPnt );
      if ( proj.IsDone() && proj.NbPoints() > 0 )       
      {
        Standard_Real u,v;
        proj.LowerDistanceParameters( u,v );

        if ( proj.LowerDistance() <= 0.1 * _grid->_tol )
        {
          isOut = false;
        }
        else
        {
          // find isOut by normals
          gp_Dir normal;
          if ( GeomLib::NormEstim( BRep_Tool::Surface( face, loc ),
                                   gp_Pnt2d( u,v ),
                                   0.1*_grid->_tol,
                                   normal ) < 3 )
          {
            if ( face.Orientation() == TopAbs_REVERSED )
              normal.Reverse();
            gp_Vec v( proj.NearestPoint(), testPnt );
            isOut = ( v * normal > 0 );
          }
        }
        if ( !isOut )
        {
          // classify a projection
          if ( !n1->IsOnFace( *faceID ) || !n2->IsOnFace( *faceID ))
          {
            BRepTopAdaptor_FClass2d cls( face, Precision::Confusion() );
            TopAbs_State state = cls.Perform( gp_Pnt2d( u,v ));
            if ( state == TopAbs_OUT )
            {
              isOut = true;
              continue;
            }
          }
          return false;
        }
      }
    }
    return isOut;
  }
  //================================================================================
  /*!
   * \brief Sort nodes on a FACE
   */
  void Hexahedron::sortVertexNodes(vector<_Node*>& nodes, _Node* curNode, TGeomID faceID)
  {
    if ( nodes.size() > 20 ) return;

    // get shapes under nodes
    TGeomID nShapeIds[20], *nShapeIdsEnd = &nShapeIds[0] + nodes.size();
    for ( size_t i = 0; i < nodes.size(); ++i )
      if ( !( nShapeIds[i] = nodes[i]->ShapeID() ))
        return;

    // get shapes of the FACE
    const TopoDS_Face&  face = TopoDS::Face( _grid->_shapes( faceID ));
    list< TopoDS_Edge > edges;
    list< int >         nbEdges;
    int nbW = SMESH_Block::GetOrderedEdges (face, edges, nbEdges);
    if ( nbW > 1 ) {
      // select a WIRE - remove EDGEs of irrelevant WIREs from edges
      list< TopoDS_Edge >::iterator e = edges.begin(), eEnd = e;
      list< int >::iterator nE = nbEdges.begin();
      for ( ; nbW > 0; ++nE, --nbW )
      {
        std::advance( eEnd, *nE );
        for ( ; e != eEnd; ++e )
          for ( int i = 0; i < 2; ++i )
          {
            TGeomID id = i==0 ?
              _grid->_shapes.FindIndex( *e ) :
              _grid->_shapes.FindIndex( SMESH_MesherHelper::IthVertex( 0, *e ));
            if (( id > 0 ) &&
                ( std::find( &nShapeIds[0], nShapeIdsEnd, id ) != nShapeIdsEnd ))
            {
              edges.erase( eEnd, edges.end() ); // remove rest wires
              e = eEnd = edges.end();
              --e;
              nbW = 0;
              break;
            }
          }
        if ( nbW > 0 )
          edges.erase( edges.begin(), eEnd ); // remove a current irrelevant wire
      }
    }
    // rotate edges to have the first one at least partially out of the hexa
    list< TopoDS_Edge >::iterator e = edges.begin(), eMidOut = edges.end();
    for ( ; e != edges.end(); ++e )
    {
      if ( !_grid->_shapes.FindIndex( *e ))
        continue;
      bool isOut = false;
      gp_Pnt p;
      double uvw[3], f,l;
      for ( int i = 0; i < 2 && !isOut; ++i )
      {
        if ( i == 0 )
        {
          TopoDS_Vertex v = SMESH_MesherHelper::IthVertex( 0, *e );
          p = BRep_Tool::Pnt( v );
        }
        else if ( eMidOut == edges.end() )
        {
          TopLoc_Location loc;
          Handle(Geom_Curve) c = BRep_Tool::Curve( *e, loc, f, l);
          if ( c.IsNull() ) break;
          p = c->Value( 0.5 * ( f + l )).Transformed( loc );
        }
        else
        {
          continue;
        }

        _grid->ComputeUVW( p.XYZ(), uvw );
        if ( isOutParam( uvw ))
        {
          if ( i == 0 )
            isOut = true;
          else
            eMidOut = e;
        }
      }
      if ( isOut )
        break;
    }
    if ( e != edges.end() )
      edges.splice( edges.end(), edges, edges.begin(), e );
    else if ( eMidOut != edges.end() )
      edges.splice( edges.end(), edges, edges.begin(), eMidOut );

    // sort nodes accoring to the order of edges
    _Node*  orderNodes   [20];
    TGeomID orderShapeIDs[20];
    int nbN = 0;
    TGeomID id, *pID;
    for ( e = edges.begin(); e != edges.end(); ++e )
    {
      if (( id = _grid->_shapes.FindIndex( SMESH_MesherHelper::IthVertex( 0, *e ))) &&
          (( pID = std::find( &nShapeIds[0], nShapeIdsEnd, id )) != nShapeIdsEnd ))
      {
        orderShapeIDs[ nbN ] = id;
        orderNodes   [ nbN++ ] = nodes[ pID - &nShapeIds[0] ];
        *pID = -1;
      }
      if (( id = _grid->_shapes.FindIndex( *e )) &&
          (( pID = std::find( &nShapeIds[0], nShapeIdsEnd, id )) != nShapeIdsEnd ))
      {
        orderShapeIDs[ nbN ] = id;
        orderNodes   [ nbN++ ] = nodes[ pID - &nShapeIds[0] ];
        *pID = -1;
      }
    }
    if ( nbN != nodes.size() )
      return;

    bool reverse = ( orderNodes[0    ]->Point().SquareDistance( curNode->Point() ) >
                     orderNodes[nbN-1]->Point().SquareDistance( curNode->Point() ));

    for ( size_t i = 0; i < nodes.size(); ++i )
      nodes[ i ] = orderNodes[ reverse ? nbN-1-i : i ];
  }

  //================================================================================
  /*!
   * \brief Adds computed elements to the mesh
   */
  int Hexahedron::addElements(SMESH_MesherHelper& helper)
  {
    int nbAdded = 0;
    // add elements resulted from hexahedron intersection
    //for ( size_t i = 0; i < _volumeDefs.size(); ++i )
    {
      vector< const SMDS_MeshNode* > nodes( _volumeDefs._nodes.size() );
      for ( size_t iN = 0; iN < nodes.size(); ++iN )
        if ( !( nodes[iN] = _volumeDefs._nodes[iN]->Node() ))
        {
          if ( const E_IntersectPoint* eip = _volumeDefs._nodes[iN]->EdgeIntPnt() )
            nodes[iN] = _volumeDefs._nodes[iN]->_intPoint->_node =
              helper.AddNode( eip->_point.X(),
                              eip->_point.Y(),
                              eip->_point.Z() );
          else
            throw SALOME_Exception("Bug: no node at intersection point");
        }

      if ( !_volumeDefs._quantities.empty() )
      {
        helper.AddPolyhedralVolume( nodes, _volumeDefs._quantities );
      }
      else
      {
        switch ( nodes.size() )
        {
        case 8: helper.AddVolume( nodes[0],nodes[1],nodes[2],nodes[3],
                                  nodes[4],nodes[5],nodes[6],nodes[7] );
          break;
        case 4: helper.AddVolume( nodes[0],nodes[1],nodes[2],nodes[3] );
          break;
        case 6: helper.AddVolume( nodes[0],nodes[1],nodes[2],nodes[3], nodes[4],nodes[5] );
          break;
        case 5:
          helper.AddVolume( nodes[0],nodes[1],nodes[2],nodes[3],nodes[4] );
          break;
        }
      }
      nbAdded += int ( _volumeDefs._nodes.size() > 0 );
    }

    return nbAdded;
  }
  //================================================================================
  /*!
   * \brief Return true if the element is in a hole
   */
  bool Hexahedron::isInHole() const
  {
    if ( !_vIntNodes.empty() )
      return false;

    const int ijk[3] = { (int)_i, (int)_j, (int)_k };
    F_IntersectPoint curIntPnt;

    // consider a cell to be in a hole if all links in any direction
    // comes OUT of geometry
    for ( int iDir = 0; iDir < 3; ++iDir )
    {
      const vector<double>& coords = _grid->_coords[ iDir ];
      LineIndexer               li = _grid->GetLineIndexer( iDir );
      li.SetIJK( _i,_j,_k );
      size_t lineIndex[4] = { li.LineIndex  (),
                              li.LineIndex10(),
                              li.LineIndex01(),
                              li.LineIndex11() };
      bool allLinksOut = true, hasLinks = false;
      for ( int iL = 0; iL < 4 && allLinksOut; ++iL ) // loop on 4 links parallel to iDir
      {
        const _Link& link = _hexLinks[ iL + 4*iDir ];
        // check transition of the first node of a link
        const F_IntersectPoint* firstIntPnt = 0;
        if ( link._nodes[0]->Node() ) // 1st node is a hexa corner
        {
          curIntPnt._paramOnLine = coords[ ijk[ iDir ]] - coords[0];
          const GridLine& line = _grid->_lines[ iDir ][ lineIndex[ iL ]];
          multiset< F_IntersectPoint >::const_iterator ip =
            line._intPoints.upper_bound( curIntPnt );
          --ip;
          firstIntPnt = &(*ip);
        }
        else if ( !link._fIntPoints.empty() )
        {
          firstIntPnt = link._fIntPoints[0];
        }

        if ( firstIntPnt )
        {
          hasLinks = true;
          allLinksOut = ( firstIntPnt->_transition == Trans_OUT );
        }
      }
      if ( hasLinks && allLinksOut )
        return true;
    }
    return false;
  }

  //================================================================================
  /*!
   * \brief Return true if a polyhedron passes _sizeThreshold criterion
   */
  bool Hexahedron::checkPolyhedronSize() const
  {
    double volume = 0;
    for ( size_t iP = 0; iP < _polygons.size(); ++iP )
    {
      const _Face& polygon = _polygons[iP];
      if ( polygon._links.empty() )
        continue;
      gp_XYZ area (0,0,0);
      gp_XYZ p1 = polygon._links[ 0 ].FirstNode()->Point().XYZ();
      for ( size_t iL = 0; iL < polygon._links.size(); ++iL )
      {
        gp_XYZ p2 = polygon._links[ iL ].LastNode()->Point().XYZ();
        area += p1 ^ p2;
        p1 = p2;
      }
      volume += p1 * area;
    }
    volume /= 6;

    double initVolume = _sideLength[0] * _sideLength[1] * _sideLength[2];

    return volume > initVolume / _sizeThreshold;
  }
  //================================================================================
  /*!
   * \brief Tries to create a hexahedron
   */
  bool Hexahedron::addHexa()
  {
    int nbQuad = 0, iQuad = -1;
    for ( size_t i = 0; i < _polygons.size(); ++i )
    {
      if ( _polygons[i]._links.empty() )
        continue;
      if ( _polygons[i]._links.size() != 4 )
        return false;
      ++nbQuad;
      if ( iQuad < 0 )
        iQuad = i;
    }
    if ( nbQuad != 6 )
      return false;

    _Node* nodes[8];
    int nbN = 0;
    for ( int iL = 0; iL < 4; ++iL )
    {
      // a base node
      nodes[iL] = _polygons[iQuad]._links[iL].FirstNode();
      ++nbN;

      // find a top node above the base node
      _Link* link = _polygons[iQuad]._links[iL]._link;
      if ( !link->_faces[0] || !link->_faces[1] )
        return debugDumpLink( link );
      // a quadrangle sharing <link> with _polygons[iQuad]
      _Face* quad = link->_faces[ bool( link->_faces[0] == & _polygons[iQuad] )];
      for ( int i = 0; i < 4; ++i )
        if ( quad->_links[i]._link == link )
        {
          // 1st node of a link opposite to <link> in <quad>
          nodes[iL+4] = quad->_links[(i+2)%4].FirstNode();
          ++nbN;
          break;
        }
    }
    if ( nbN == 8 )
      _volumeDefs.set( &nodes[0], 8 );

    return nbN == 8;
  }
  //================================================================================
  /*!
   * \brief Tries to create a tetrahedron
   */
  bool Hexahedron::addTetra()
  {
    int iTria = -1;
    for ( size_t i = 0; i < _polygons.size() && iTria < 0; ++i )
      if ( _polygons[i]._links.size() == 3 )
        iTria = i;
    if ( iTria < 0 )
      return false;

    _Node* nodes[4];
    nodes[0] = _polygons[iTria]._links[0].FirstNode();
    nodes[1] = _polygons[iTria]._links[1].FirstNode();
    nodes[2] = _polygons[iTria]._links[2].FirstNode();

    _Link* link = _polygons[iTria]._links[0]._link;
    if ( !link->_faces[0] || !link->_faces[1] )
      return debugDumpLink( link );

    // a triangle sharing <link> with _polygons[0]
    _Face* tria = link->_faces[ bool( link->_faces[0] == & _polygons[iTria] )];
    for ( int i = 0; i < 3; ++i )
      if ( tria->_links[i]._link == link )
      {
        nodes[3] = tria->_links[(i+1)%3].LastNode();
        _volumeDefs.set( &nodes[0], 4 );
        return true;
      }

    return false;
  }
  //================================================================================
  /*!
   * \brief Tries to create a pentahedron
   */
  bool Hexahedron::addPenta()
  {
    // find a base triangular face
    int iTri = -1;
    for ( int iF = 0; iF < 5 && iTri < 0; ++iF )
      if ( _polygons[ iF ]._links.size() == 3 )
        iTri = iF;
    if ( iTri < 0 ) return false;

    // find nodes
    _Node* nodes[6];
    int nbN = 0;
    for ( int iL = 0; iL < 3; ++iL )
    {
      // a base node
      nodes[iL] = _polygons[ iTri ]._links[iL].FirstNode();
      ++nbN;

      // find a top node above the base node
      _Link* link = _polygons[ iTri ]._links[iL]._link;
      if ( !link->_faces[0] || !link->_faces[1] )
        return debugDumpLink( link );
      // a quadrangle sharing <link> with a base triangle
      _Face* quad = link->_faces[ bool( link->_faces[0] == & _polygons[ iTri ] )];
      if ( quad->_links.size() != 4 ) return false;
      for ( int i = 0; i < 4; ++i )
        if ( quad->_links[i]._link == link )
        {
          // 1st node of a link opposite to <link> in <quad>
          nodes[iL+3] = quad->_links[(i+2)%4].FirstNode();
          ++nbN;
          break;
        }
    }
    if ( nbN == 6 )
      _volumeDefs.set( &nodes[0], 6 );

    return ( nbN == 6 );
  }
  //================================================================================
  /*!
   * \brief Tries to create a pyramid
   */
  bool Hexahedron::addPyra()
  {
    // find a base quadrangle
    int iQuad = -1;
    for ( int iF = 0; iF < 5 && iQuad < 0; ++iF )
      if ( _polygons[ iF ]._links.size() == 4 )
        iQuad = iF;
    if ( iQuad < 0 ) return false;

    // find nodes
    _Node* nodes[5];
    nodes[0] = _polygons[iQuad]._links[0].FirstNode();
    nodes[1] = _polygons[iQuad]._links[1].FirstNode();
    nodes[2] = _polygons[iQuad]._links[2].FirstNode();
    nodes[3] = _polygons[iQuad]._links[3].FirstNode();

    _Link* link = _polygons[iQuad]._links[0]._link;
    if ( !link->_faces[0] || !link->_faces[1] )
      return debugDumpLink( link );

    // a triangle sharing <link> with a base quadrangle
    _Face* tria = link->_faces[ bool( link->_faces[0] == & _polygons[ iQuad ] )];
    if ( tria->_links.size() != 3 ) return false;
    for ( int i = 0; i < 3; ++i )
      if ( tria->_links[i]._link == link )
      {
        nodes[4] = tria->_links[(i+1)%3].LastNode();
        _volumeDefs.set( &nodes[0], 5 );
        return true;
      }

    return false;
  }
  //================================================================================
  /*!
   * \brief Dump a link and return \c false
   */
  bool Hexahedron::debugDumpLink( Hexahedron::_Link* link )
  {
#ifdef _DEBUG_
    gp_Pnt p1 = link->_nodes[0]->Point(), p2 = link->_nodes[1]->Point();
    cout << "BUG: not shared link. IKJ = ( "<< _i << " " << _j << " " << _k << " )" << endl
         << "n1 (" << p1.X() << ", "<< p1.Y() << ", "<< p1.Z() << " )" << endl
         << "n2 (" << p2.X() << ", "<< p2.Y() << ", "<< p2.Z() << " )" << endl;
#endif
    return false;
  }
  //================================================================================
  /*!
   * \brief Classify a point by grid paremeters
   */
  bool Hexahedron::isOutParam(const double uvw[3]) const
  {
    return (( _grid->_coords[0][ _i   ] - _grid->_tol > uvw[0] ) ||
            ( _grid->_coords[0][ _i+1 ] + _grid->_tol < uvw[0] ) ||
            ( _grid->_coords[1][ _j   ] - _grid->_tol > uvw[1] ) ||
            ( _grid->_coords[1][ _j+1 ] + _grid->_tol < uvw[1] ) ||
            ( _grid->_coords[2][ _k   ] - _grid->_tol > uvw[2] ) ||
            ( _grid->_coords[2][ _k+1 ] + _grid->_tol < uvw[2] ));
  }

  //================================================================================
  /*!
   * \brief computes exact bounding box with axes parallel to given ones
   */
  //================================================================================

  void getExactBndBox( const vector< TopoDS_Shape >& faceVec,
                       const double*                 axesDirs,
                       Bnd_Box&                      shapeBox )
  {
    BRep_Builder b;
    TopoDS_Compound allFacesComp;
    b.MakeCompound( allFacesComp );
    for ( size_t iF = 0; iF < faceVec.size(); ++iF )
      b.Add( allFacesComp, faceVec[ iF ] );

    double sP[6]; // aXmin, aYmin, aZmin, aXmax, aYmax, aZmax
    shapeBox.Get(sP[0],sP[1],sP[2],sP[3],sP[4],sP[5]);
    double farDist = 0;
    for ( int i = 0; i < 6; ++i )
      farDist = Max( farDist, 10 * sP[i] );

    gp_XYZ axis[3] = { gp_XYZ( axesDirs[0], axesDirs[1], axesDirs[2] ),
                       gp_XYZ( axesDirs[3], axesDirs[4], axesDirs[5] ),
                       gp_XYZ( axesDirs[6], axesDirs[7], axesDirs[8] ) };
    axis[0].Normalize();
    axis[1].Normalize();
    axis[2].Normalize();

    gp_Mat basis( axis[0], axis[1], axis[2] );
    gp_Mat bi = basis.Inverted();

    gp_Pnt pMin, pMax;
    for ( int iDir = 0; iDir < 3; ++iDir )
    {
      gp_XYZ axis0 = axis[ iDir ];
      gp_XYZ axis1 = axis[ ( iDir + 1 ) % 3 ];
      gp_XYZ axis2 = axis[ ( iDir + 2 ) % 3 ];
      for ( int isMax = 0; isMax < 2; ++isMax )
      {
        double shift = isMax ? farDist : -farDist;
        gp_XYZ orig = shift * axis0;
        gp_XYZ norm = axis1 ^ axis2;
        gp_Pln pln( orig, norm );
        norm = pln.Axis().Direction().XYZ();
        BRepBuilderAPI_MakeFace plane( pln, -farDist, farDist, -farDist, farDist );

        gp_Pnt& pAxis = isMax ? pMax : pMin;
        gp_Pnt pPlane, pFaces;
        double dist = GEOMUtils::GetMinDistance( plane, allFacesComp, pPlane, pFaces );
        if ( dist < 0 )
        {
          Bnd_B3d bb;
          gp_XYZ corner;
          for ( int i = 0; i < 2; ++i ) {
            corner.SetCoord( 1, sP[ i*3 ]);
            for ( int j = 0; j < 2; ++j ) {
              corner.SetCoord( 2, sP[ i*3 + 1 ]);
              for ( int k = 0; k < 2; ++k )
              {
                corner.SetCoord( 3, sP[ i*3 + 2 ]);
                corner *= bi;
                bb.Add( corner );
              }
            }
          }
          corner = isMax ? bb.CornerMax() : bb.CornerMin();
          pAxis.SetCoord( iDir+1, corner.Coord( iDir+1 ));
        }
        else
        {
          gp_XYZ pf = pFaces.XYZ() * bi;
          pAxis.SetCoord( iDir+1, pf.Coord( iDir+1 ) );
        }
      }
    } // loop on 3 axes

    shapeBox.SetVoid();
    shapeBox.Add( pMin );
    shapeBox.Add( pMax );

    return;
  }

} // namespace

//=============================================================================
/*!
 * \brief Generates 3D structured Cartesian mesh in the internal part of
 * solid shapes and polyhedral volumes near the shape boundary.
 *  \param theMesh - mesh to fill in
 *  \param theShape - a compound of all SOLIDs to mesh
 *  \retval bool - true in case of success
 */
//=============================================================================

bool StdMeshers_Cartesian_3D::Compute(SMESH_Mesh &         theMesh,
                                      const TopoDS_Shape & theShape)
{
  // The algorithm generates the mesh in following steps:

  // 1) Intersection of grid lines with the geometry boundary.
  // This step allows to find out if a given node of the initial grid is
  // inside or outside the geometry.

  // 2) For each cell of the grid, check how many of it's nodes are outside
  // of the geometry boundary. Depending on a result of this check
  // - skip a cell, if all it's nodes are outside
  // - skip a cell, if it is too small according to the size threshold
  // - add a hexahedron in the mesh, if all nodes are inside
  // - add a polyhedron in the mesh, if some nodes are inside and some outside

  _computeCanceled = false;

  SMESH_MesherHelper helper( theMesh );

  try
  {
    Grid grid;
    grid._helper = &helper;

    vector< TopoDS_Shape > faceVec;
    {
      TopTools_MapOfShape faceMap;
      TopExp_Explorer fExp;
      for ( fExp.Init( theShape, TopAbs_FACE ); fExp.More(); fExp.Next() )
        if ( !faceMap.Add( fExp.Current() ))
          faceMap.Remove( fExp.Current() ); // remove a face shared by two solids

      for ( fExp.ReInit(); fExp.More(); fExp.Next() )
        if ( faceMap.Contains( fExp.Current() ))
          faceVec.push_back( fExp.Current() );
    }
    vector<FaceGridIntersector> facesItersectors( faceVec.size() );
    map< TGeomID, vector< TGeomID > > edge2faceIDsMap;
    TopExp_Explorer eExp;
    Bnd_Box shapeBox;
    for ( int i = 0; i < faceVec.size(); ++i )
    {
      facesItersectors[i]._face   = TopoDS::Face    ( faceVec[i] );
      facesItersectors[i]._faceID = grid._shapes.Add( faceVec[i] );
      facesItersectors[i]._grid   = &grid;
      shapeBox.Add( facesItersectors[i].GetFaceBndBox() );

      if ( _hyp->GetToAddEdges() )
      {
        helper.SetSubShape( faceVec[i] );
        for ( eExp.Init( faceVec[i], TopAbs_EDGE ); eExp.More(); eExp.Next() )
        {
          const TopoDS_Edge& edge = TopoDS::Edge( eExp.Current() );
          if ( !SMESH_Algo::isDegenerated( edge ) &&
               !helper.IsRealSeam( edge ))
            edge2faceIDsMap[ grid._shapes.Add( edge )].push_back( facesItersectors[i]._faceID );
        }
      }
    }

    getExactBndBox( faceVec, _hyp->GetAxisDirs(), shapeBox );

    vector<double> xCoords, yCoords, zCoords;
    _hyp->GetCoordinates( xCoords, yCoords, zCoords, shapeBox );

    grid.SetCoordinates( xCoords, yCoords, zCoords, _hyp->GetAxisDirs(), shapeBox );

    if ( _computeCanceled ) return false;

#ifdef WITH_TBB
    { // copy partner faces and curves of not thread-safe types
      set< const Standard_Transient* > tshapes;
      BRepBuilderAPI_Copy copier;
      for ( size_t i = 0; i < facesItersectors.size(); ++i )
      {
        if ( !facesItersectors[i].IsThreadSafe(tshapes) )
        {
          copier.Perform( facesItersectors[i]._face );
          facesItersectors[i]._face = TopoDS::Face( copier );
        }
      }
    }
    // Intersection of grid lines with the geometry boundary.
    tbb::parallel_for ( tbb::blocked_range<size_t>( 0, facesItersectors.size() ),
                        ParallelIntersector( facesItersectors ),
                        tbb::simple_partitioner());
#else
    for ( size_t i = 0; i < facesItersectors.size(); ++i )
      facesItersectors[i].Intersect();
#endif

    // put interesection points onto the GridLine's; this is done after intersection
    // to avoid contention of facesItersectors for writing into the same GridLine
    // in case of parallel work of facesItersectors
    for ( size_t i = 0; i < facesItersectors.size(); ++i )
      facesItersectors[i].StoreIntersections();

    TopExp_Explorer solidExp (theShape, TopAbs_SOLID);
    helper.SetSubShape( solidExp.Current() );
    helper.SetElementsOnShape( true );

    if ( _computeCanceled ) return false;

    // create nodes on the geometry
    grid.ComputeNodes(helper);

    if ( _computeCanceled ) return false;

    // create volume elements
    Hexahedron hex( _hyp->GetSizeThreshold(), &grid );
    int nbAdded = hex.MakeElements( helper, edge2faceIDsMap );

    SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();
    if ( nbAdded > 0 )
    {
      // make all SOLIDs computed
      if ( SMESHDS_SubMesh* sm1 = meshDS->MeshElements( solidExp.Current()) )
      {
        SMDS_ElemIteratorPtr volIt = sm1->GetElements();
        for ( ; solidExp.More() && volIt->more(); solidExp.Next() )
        {
          const SMDS_MeshElement* vol = volIt->next();
          sm1->RemoveElement( vol, /*isElemDeleted=*/false );
          meshDS->SetMeshElementOnShape( vol, solidExp.Current() );
        }
      }
      // make other sub-shapes computed
      setSubmeshesComputed( theMesh, theShape );
    }

    // remove free nodes
    if ( SMESHDS_SubMesh * smDS = meshDS->MeshElements( helper.GetSubShapeID() ))
    {
      TIDSortedNodeSet nodesToRemove;
      // get intersection nodes
      for ( int iDir = 0; iDir < 3; ++iDir )
      {
        vector< GridLine >& lines = grid._lines[ iDir ];
        for ( size_t i = 0; i < lines.size(); ++i )
        {
          multiset< F_IntersectPoint >::iterator ip = lines[i]._intPoints.begin();
          for ( ; ip != lines[i]._intPoints.end(); ++ip )
            if ( ip->_node && ip->_node->NbInverseElements() == 0 )
              nodesToRemove.insert( nodesToRemove.end(), ip->_node );
        }
      }
      // get grid nodes
      for ( size_t i = 0; i < grid._nodes.size(); ++i )
        if ( grid._nodes[i] && grid._nodes[i]->NbInverseElements() == 0 )
          nodesToRemove.insert( nodesToRemove.end(), grid._nodes[i] );

      // do remove
      TIDSortedNodeSet::iterator n = nodesToRemove.begin();
      for ( ; n != nodesToRemove.end(); ++n )
        meshDS->RemoveFreeNode( *n, smDS, /*fromGroups=*/false );
    }

    return nbAdded;

  }
  // SMESH_ComputeError is not caught at SMESH_submesh level for an unknown reason
  catch ( SMESH_ComputeError& e)
  {
    return error( SMESH_ComputeErrorPtr( new SMESH_ComputeError( e )));
  }
  return false;
}

//=============================================================================
/*!
 *  Evaluate
 */
//=============================================================================

bool StdMeshers_Cartesian_3D::Evaluate(SMESH_Mesh &         theMesh,
                                       const TopoDS_Shape & theShape,
                                       MapShapeNbElems&     theResMap)
{
  // TODO
//   std::vector<int> aResVec(SMDSEntity_Last);
//   for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aResVec[i] = 0;
//   if(IsQuadratic) {
//     aResVec[SMDSEntity_Quad_Cartesian] = nb2d_face0 * ( nb2d/nb1d );
//     int nb1d_face0_int = ( nb2d_face0*4 - nb1d ) / 2;
//     aResVec[SMDSEntity_Node] = nb0d_face0 * ( 2*nb2d/nb1d - 1 ) - nb1d_face0_int * nb2d/nb1d;
//   }
//   else {
//     aResVec[SMDSEntity_Node] = nb0d_face0 * ( nb2d/nb1d - 1 );
//     aResVec[SMDSEntity_Cartesian] = nb2d_face0 * ( nb2d/nb1d );
//   }
//   SMESH_subMesh * sm = aMesh.GetSubMesh(aShape);
//   aResMap.insert(std::make_pair(sm,aResVec));

  return true;
}

//=============================================================================
namespace
{
  /*!
   * \brief Event listener setting/unsetting _alwaysComputed flag to
   *        submeshes of inferior levels to prevent their computing
   */
  struct _EventListener : public SMESH_subMeshEventListener
  {
    string _algoName;

    _EventListener(const string& algoName):
      SMESH_subMeshEventListener(/*isDeletable=*/true,"StdMeshers_Cartesian_3D::_EventListener"),
      _algoName(algoName)
    {}
    // --------------------------------------------------------------------------------
    // setting/unsetting _alwaysComputed flag to submeshes of inferior levels
    //
    static void setAlwaysComputed( const bool     isComputed,
                                   SMESH_subMesh* subMeshOfSolid)
    {
      SMESH_subMeshIteratorPtr smIt =
        subMeshOfSolid->getDependsOnIterator(/*includeSelf=*/false, /*complexShapeFirst=*/false);
      while ( smIt->more() )
      {
        SMESH_subMesh* sm = smIt->next();
        sm->SetIsAlwaysComputed( isComputed );
      }
      subMeshOfSolid->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
    }

    // --------------------------------------------------------------------------------
    // unsetting _alwaysComputed flag if "Cartesian_3D" was removed
    //
    virtual void ProcessEvent(const int          event,
                              const int          eventType,
                              SMESH_subMesh*     subMeshOfSolid,
                              SMESH_subMeshEventListenerData* data,
                              const SMESH_Hypothesis*         hyp = 0)
    {
      if ( eventType == SMESH_subMesh::COMPUTE_EVENT )
      {
        setAlwaysComputed( subMeshOfSolid->GetComputeState() == SMESH_subMesh::COMPUTE_OK,
                           subMeshOfSolid );
      }
      else
      {
        SMESH_Algo* algo3D = subMeshOfSolid->GetAlgo();
        if ( !algo3D || _algoName != algo3D->GetName() )
          setAlwaysComputed( false, subMeshOfSolid );
      }
    }

    // --------------------------------------------------------------------------------
    // set the event listener
    //
    static void SetOn( SMESH_subMesh* subMeshOfSolid, const string& algoName )
    {
      subMeshOfSolid->SetEventListener( new _EventListener( algoName ),
                                        /*data=*/0,
                                        subMeshOfSolid );
    }

  }; // struct _EventListener

} // namespace

//================================================================================
/*!
 * \brief Sets event listener to submeshes if necessary
 *  \param subMesh - submesh where algo is set
 * This method is called when a submesh gets HYP_OK algo_state.
 * After being set, event listener is notified on each event of a submesh.
 */
//================================================================================

void StdMeshers_Cartesian_3D::SetEventListener(SMESH_subMesh* subMesh)
{
  _EventListener::SetOn( subMesh, GetName() );
}

//================================================================================
/*!
 * \brief Set _alwaysComputed flag to submeshes of inferior levels to avoid their computing
 */
//================================================================================

void StdMeshers_Cartesian_3D::setSubmeshesComputed(SMESH_Mesh&         theMesh,
                                                   const TopoDS_Shape& theShape)
{
  for ( TopExp_Explorer soExp( theShape, TopAbs_SOLID ); soExp.More(); soExp.Next() )
    _EventListener::setAlwaysComputed( true, theMesh.GetSubMesh( soExp.Current() ));
}

